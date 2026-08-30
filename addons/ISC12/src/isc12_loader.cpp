#include <D2RLPlugin/api.h>

#include "isc12_contract.hpp"
#include "isc12_loader.hpp"

#include <Windows.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <string_view>
#include <type_traits>

extern "C" {

void* gISC12LoaderSuccessExit{};
void* gISC12LoaderVanillaExit{};

void ISC12LoaderTailMidHook() noexcept;
extern std::uint8_t ISC12LoaderRelayTemplateBegin;
extern std::uint8_t ISC12LoaderRelayTemplateVanillaExit;
extern std::uint8_t ISC12LoaderRelayTemplateSuccessExit;
extern std::uint8_t ISC12LoaderRelayTemplateStatePointer;
extern std::uint8_t ISC12LoaderRelayTemplateEnd;

}

namespace ruffneckk::isc12 {
namespace {

constexpr std::uintptr_t TailPatchRva = 0x31F0AB;
constexpr std::uintptr_t VanillaContinuationRva = 0x31F0B3;
constexpr std::uintptr_t EpilogueRva = 0x31F1B3;
constexpr std::uintptr_t CountImmediateRva = 0x31ED38;
constexpr std::uintptr_t NativeQsortRva = 0x12E6F60;
constexpr std::uintptr_t NativeComparatorRva = 0x320880;
constexpr std::uintptr_t NativeVectorResizeRva = 0x323810;

constexpr std::size_t RecordsPointerOffset = 0x1258;
constexpr std::size_t RecordCountOffset = 0x1260;
constexpr std::size_t DescriptionVectorOffset = 0x1278;
constexpr std::size_t RecordStride = 0x144;
constexpr std::size_t DescriptionPriorityOffset = 0x30;
constexpr std::size_t DescriptionFunctionOffset = 0x32;

constexpr auto TailExpected = std::to_array<std::uint8_t>({
    0x4C, 0x8B, 0xBC, 0x24, 0x40, 0x0F, 0x00, 0x00,
});
constexpr auto CountImmediateExpected = std::to_array<std::uint8_t>({
    0xFF, 0x01, 0x00, 0x00,
});

constexpr std::size_t RelayStateHandlerOffset = 0x10;
constexpr std::size_t RelayStateVanillaOffset = 0x18;
constexpr std::size_t RelayStateEpilogueOffset = 0x20;
constexpr std::size_t MaximumRelayTemplateSize = 512;
constexpr ULONGLONG ShutdownRundownTimeoutMilliseconds = 5000;
constexpr DWORD MsvcCppExceptionCode = 0xE06D7363;
constexpr std::uint64_t NativeVectorCapacityMask =
    UINT64_C(0x7FFFFFFFFFFFFFFF);

struct RelayState {
    volatile LONG activeCallbacks{};
    volatile LONG operational{};
    volatile LONG capMayBeExtended{};
    LONG reserved{};
    void* handler{};
    void* vanillaContinuation{};
    void* epilogue{};
};

struct NativeVectorU16 {
    std::uint16_t* begin{};
    std::uint64_t size{};
    std::uint64_t capacityAndFlags{};
};

using NativeComparatorFn = int(__cdecl*)(const void*, const void*);
using NativeQsortFn = void(__cdecl*)(
    void*, std::size_t, std::size_t, NativeComparatorFn);
using NativeVectorResizeFn = void(__fastcall*)(void*, std::size_t);

static_assert(sizeof(DescriptionEntry) == 4);
static_assert(offsetof(DescriptionEntry, statId) == 0);
static_assert(offsetof(DescriptionEntry, priority) == 2);
static_assert(offsetof(RelayState, capMayBeExtended) == 8);
static_assert(offsetof(RelayState, handler) == RelayStateHandlerOffset);
static_assert(
    offsetof(RelayState, vanillaContinuation) == RelayStateVanillaOffset);
static_assert(offsetof(RelayState, epilogue) == RelayStateEpilogueOffset);
static_assert(sizeof(NativeVectorU16) == 24);
static_assert(offsetof(NativeVectorU16, begin) == 0);
static_assert(offsetof(NativeVectorU16, size) == 8);
static_assert(offsetof(NativeVectorU16, capacityAndFlags) == 16);

const D2RL::PluginContext* LoaderContext{};
std::uint8_t* LoaderBase{};
std::size_t LoaderImageSize{};
void* RelayPage{};
std::size_t RelayPageSize{};
RelayState* State{};
std::size_t StatePageSize{};
NativeQsortFn NativeQsort{};
NativeComparatorFn NativeComparator{};
NativeVectorResizeFn NativeVectorResize{};
bool Prepared{};
bool AnyMutationInstalled{};
bool TailPatchInstalled{};
bool CapPatchInstalled{};
bool ColdRestartRequired{};
std::atomic_uint64_t BuildCalls{};
std::atomic_uint64_t LastRowCount{};
std::atomic_uint64_t LastDescriptionCount{};
thread_local bool NativeVectorMutationStarted{};
thread_local bool CurrentRowCountKnown{};
thread_local std::uint64_t CurrentRowCount{};

auto SetError(std::string& error, std::string_view message) noexcept -> bool {
    try {
        error.assign(message.data(), message.size());
    } catch (...) {
        // Preserve the loader ABI if even diagnostic allocation fails.
    }
    return false;
}

auto IsAccessibleRange(
        const void* address,
        std::size_t size,
        bool requireWrite,
        bool requireExecute = false) noexcept -> bool {
    if (!address || size == 0) return false;
    const auto start = reinterpret_cast<std::uintptr_t>(address);
    if (start > std::numeric_limits<std::uintptr_t>::max() - size) {
        return false;
    }
    const auto end = start + size;
    auto cursor = start;
    while (cursor < end) {
        MEMORY_BASIC_INFORMATION memory{};
        if (VirtualQuery(
                reinterpret_cast<const void*>(cursor),
                &memory,
                sizeof(memory)) != sizeof(memory)
                || memory.State != MEM_COMMIT
                || (memory.Protect & (PAGE_GUARD | PAGE_NOACCESS)) != 0) {
            return false;
        }
        const auto protection = memory.Protect & 0xFFU;
        const bool readable = protection == PAGE_READONLY
            || protection == PAGE_READWRITE
            || protection == PAGE_WRITECOPY
            || protection == PAGE_EXECUTE_READ
            || protection == PAGE_EXECUTE_READWRITE
            || protection == PAGE_EXECUTE_WRITECOPY;
        const bool writable = protection == PAGE_READWRITE
            || protection == PAGE_WRITECOPY
            || protection == PAGE_EXECUTE_READWRITE
            || protection == PAGE_EXECUTE_WRITECOPY;
        const bool executable = protection == PAGE_EXECUTE
            || protection == PAGE_EXECUTE_READ
            || protection == PAGE_EXECUTE_READWRITE
            || protection == PAGE_EXECUTE_WRITECOPY;
        if (!readable || (requireWrite && !writable)
                || (requireExecute && !executable)) {
            return false;
        }
        const auto regionStart = reinterpret_cast<std::uintptr_t>(
            memory.BaseAddress);
        if (regionStart > std::numeric_limits<std::uintptr_t>::max()
                - memory.RegionSize) {
            return false;
        }
        const auto regionEnd = regionStart + memory.RegionSize;
        if (regionEnd <= cursor) return false;
        cursor = std::min(regionEnd, end);
    }
    return true;
}

template <typename Value>
auto SafeRead(const void* source, Value& destination) noexcept -> bool {
    static_assert(std::is_trivially_copyable_v<Value>);
    if (!IsAccessibleRange(source, sizeof(Value), false)) return false;
    __try {
        std::memcpy(&destination, source, sizeof(Value));
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

auto SafeWrite(
        void* destination,
        const void* source,
        std::size_t size) noexcept -> bool {
    if (size == 0) return true;
    if (!source || !IsAccessibleRange(destination, size, true)) return false;
    __try {
        std::memcpy(destination, source, size);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

auto NativeExceptionFilter(DWORD code) noexcept -> int {
    return code == MsvcCppExceptionCode
        ? EXCEPTION_CONTINUE_SEARCH
        : EXCEPTION_EXECUTE_HANDLER;
}

auto InvokeNativeQsort(
        void* entries,
        std::size_t count) -> bool {
    __try {
        NativeQsort(entries, count, sizeof(DescriptionEntry), NativeComparator);
        return true;
    } __except (NativeExceptionFilter(GetExceptionCode())) {
        return false;
    }
}

auto InvokeNativeResize(void* vector, std::size_t count) -> bool {
    __try {
        NativeVectorResize(vector, count);
        return true;
    } __except (NativeExceptionFilter(GetExceptionCode())) {
        return false;
    }
}

[[noreturn]] auto FailClosed(
        const char* reason,
        std::uint64_t rowCount) noexcept -> void {
    if (LoaderContext) {
        char message[320]{};
        std::snprintf(
            message,
            sizeof(message),
            "ISC12: fail-closed loader stop (%s; rows=%llu).",
            reason ? reason : "unspecified native failure",
            static_cast<unsigned long long>(rowCount));
        LoaderContext->LogError(message);
    }
    RaiseFailFastException(nullptr, nullptr, 0);
    TerminateProcess(GetCurrentProcess(), ERROR_INVALID_DATA);
    __assume(false);
}

auto PreMutationFailure(const char* reason) noexcept -> std::uint32_t {
    if (CurrentRowCountKnown
            && CurrentRowCount <= LegacySerializedSentinel) {
        return 0;
    }
    FailClosed(reason, CurrentRowCount);
}

auto ValidateImageTarget(
        std::uintptr_t rva,
        std::size_t size,
        bool executable) noexcept -> bool {
    return LoaderBase && rva <= LoaderImageSize
        && size <= LoaderImageSize - rva
        && IsAccessibleRange(LoaderBase + rva, size, false, executable);
}

auto ReleaseUnpatchedResources() noexcept -> void {
    if (AnyMutationInstalled) return;
    if (RelayPage) {
        VirtualFree(RelayPage, 0, MEM_RELEASE);
        RelayPage = nullptr;
        RelayPageSize = 0;
    }
    if (State) {
        VirtualFree(State, 0, MEM_RELEASE);
        State = nullptr;
        StatePageSize = 0;
    }
    gISC12LoaderSuccessExit = nullptr;
    gISC12LoaderVanillaExit = nullptr;
}

auto AllocateRelayPageNear(std::uint8_t* base) noexcept -> void* {
    if (!base) return nullptr;
    SYSTEM_INFO systemInfo{};
    GetSystemInfo(&systemInfo);
    const auto granularity = static_cast<std::uintptr_t>(
        systemInfo.dwAllocationGranularity);
    const auto pageSize = static_cast<std::size_t>(systemInfo.dwPageSize);
    if (granularity == 0 || pageSize < MaximumRelayTemplateSize) {
        return nullptr;
    }

    const auto patchSite = reinterpret_cast<std::uintptr_t>(
        base + TailPatchRva);
    const auto aligned = patchSite & ~(granularity - 1U);
    for (std::uintptr_t delta = granularity;
            delta < UINT64_C(0x70000000);
            delta += granularity) {
        if (aligned > std::numeric_limits<std::uintptr_t>::max() - delta) {
            break;
        }
        const auto candidate = aligned + delta;
        if (!CanEncodeRel32(patchSite, candidate)) break;
        if (auto* allocation = VirtualAlloc(
                reinterpret_cast<void*>(candidate),
                pageSize,
                MEM_COMMIT | MEM_RESERVE,
                PAGE_READWRITE)) {
            RelayPageSize = pageSize;
            return allocation;
        }
    }
    return nullptr;
}

auto PrepareRelay(std::string& error) noexcept -> bool {
    SYSTEM_INFO systemInfo{};
    GetSystemInfo(&systemInfo);
    const auto pageSize = static_cast<std::size_t>(systemInfo.dwPageSize);
    if (pageSize < sizeof(RelayState)) {
        return SetError(error, "invalid Windows page size for relay state");
    }
    State = static_cast<RelayState*>(VirtualAlloc(
        nullptr,
        pageSize,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_READWRITE));
    if (!State) return SetError(error, "relay state allocation failed");
    StatePageSize = pageSize;

    RelayPage = AllocateRelayPageNear(LoaderBase);
    if (!RelayPage) {
        ReleaseUnpatchedResources();
        return SetError(
            error,
            "no persistent relay page is available within rel32 reach");
    }

    const auto templateBegin = reinterpret_cast<std::uintptr_t>(
        &ISC12LoaderRelayTemplateBegin);
    const auto templateVanillaExit = reinterpret_cast<std::uintptr_t>(
        &ISC12LoaderRelayTemplateVanillaExit);
    const auto templateSuccessExit = reinterpret_cast<std::uintptr_t>(
        &ISC12LoaderRelayTemplateSuccessExit);
    const auto templateStatePointer = reinterpret_cast<std::uintptr_t>(
        &ISC12LoaderRelayTemplateStatePointer);
    const auto templateEnd = reinterpret_cast<std::uintptr_t>(
        &ISC12LoaderRelayTemplateEnd);
    if (templateEnd <= templateBegin) {
        ReleaseUnpatchedResources();
        return SetError(error, "persistent relay template layout is invalid");
    }
    const auto templateSizeAddress = templateEnd - templateBegin;
    if (templateSizeAddress < sizeof(void*)
            || templateVanillaExit < templateBegin
            || templateVanillaExit >= templateEnd
            || templateSuccessExit < templateBegin
            || templateSuccessExit >= templateEnd
            || templateStatePointer < templateBegin
            || templateStatePointer > templateEnd - sizeof(void*)) {
        ReleaseUnpatchedResources();
        return SetError(error, "persistent relay template layout is invalid");
    }
    const auto templateSize = static_cast<std::size_t>(
        templateSizeAddress);
    const auto statePointerOffset = static_cast<std::size_t>(
        templateStatePointer - templateBegin);
    const auto vanillaExitOffset = static_cast<std::size_t>(
        templateVanillaExit - templateBegin);
    const auto successExitOffset = static_cast<std::size_t>(
        templateSuccessExit - templateBegin);
    if (templateSize > MaximumRelayTemplateSize
            || templateSize > RelayPageSize) {
        ReleaseUnpatchedResources();
        return SetError(error, "persistent relay template is too large");
    }

    std::memcpy(
        RelayPage,
        reinterpret_cast<const void*>(templateBegin),
        templateSize);
    auto* const copiedStatePointer =
        static_cast<std::uint8_t*>(RelayPage) + statePointerOffset;
    std::memcpy(copiedStatePointer, &State, sizeof(State));

    State->handler = reinterpret_cast<void*>(&ISC12LoaderTailMidHook);
    State->vanillaContinuation = LoaderBase + VanillaContinuationRva;
    State->epilogue = LoaderBase + EpilogueRva;
    gISC12LoaderVanillaExit =
        static_cast<std::uint8_t*>(RelayPage) + vanillaExitOffset;
    gISC12LoaderSuccessExit =
        static_cast<std::uint8_t*>(RelayPage) + successExitOffset;

    DWORD previousProtection{};
    if (!VirtualProtect(
            RelayPage,
            RelayPageSize,
            PAGE_EXECUTE_READ,
            &previousProtection)) {
        ReleaseUnpatchedResources();
        return SetError(error, "persistent relay protection failed");
    }
    if (!FlushInstructionCache(
            GetCurrentProcess(), RelayPage, templateSize)) {
        ReleaseUnpatchedResources();
        return SetError(error, "persistent relay cache flush failed");
    }
    return true;
}

auto BuildDescriptionIndexNative(void* dataTables) -> std::uint32_t {
    ++BuildCalls;
    NativeVectorMutationStarted = false;
    CurrentRowCountKnown = false;
    CurrentRowCount = 0;
    LastDescriptionCount.store(0, std::memory_order_release);

    if (!dataTables) return PreMutationFailure("null DataTables pointer");
    const auto dataTablesAddress = reinterpret_cast<std::uintptr_t>(dataTables);
    if (dataTablesAddress
            > std::numeric_limits<std::uintptr_t>::max()
                - DescriptionVectorOffset - sizeof(NativeVectorU16)) {
        return PreMutationFailure("DataTables address overflow");
    }

    std::uint64_t rowCount{};
    if (!SafeRead(
            reinterpret_cast<const void*>(
                dataTablesAddress + RecordCountOffset),
            rowCount)) {
        return PreMutationFailure("ItemStatCost count is unreadable");
    }
    CurrentRowCount = rowCount;
    CurrentRowCountKnown = true;
    LastRowCount.store(rowCount, std::memory_order_release);
    if (rowCount > MaximumRecordCount) {
        FailClosed("ItemStatCost count exceeds 4095", rowCount);
    }

    std::uint8_t* records{};
    if (!SafeRead(
            reinterpret_cast<const void*>(
                dataTablesAddress + RecordsPointerOffset),
            records)) {
        return PreMutationFailure("ItemStatCost record pointer is unreadable");
    }
    const auto recordBytes = static_cast<std::size_t>(rowCount) * RecordStride;
    if (rowCount != 0
            && (!records
                || !IsAccessibleRange(records, recordBytes, false))) {
        return PreMutationFailure("ItemStatCost records are unreadable");
    }

    std::array<DescriptionEntry, MaximumRecordCount> staged;
    std::size_t descriptionCount{};
    for (std::size_t rowIndex{};
            rowIndex < static_cast<std::size_t>(rowCount);
            ++rowIndex) {
        const auto* const record = records + rowIndex * RecordStride;
        std::uint8_t function{};
        if (!SafeRead(record + DescriptionFunctionOffset, function)) {
            return PreMutationFailure("DescFunc is unreadable");
        }
        if (function == 0) continue;
        std::int16_t priority{};
        if (!SafeRead(record + DescriptionPriorityOffset, priority)) {
            return PreMutationFailure("DescPriority is unreadable");
        }
        staged[descriptionCount++] = {
            static_cast<std::uint16_t>(rowIndex),
            priority,
        };
    }
    LastDescriptionCount.store(descriptionCount, std::memory_order_release);

    if (descriptionCount > 1
            && !InvokeNativeQsort(staged.data(), descriptionCount)) {
        return PreMutationFailure("native DescPriority sort failed");
    }

    auto* const vectorAddress = reinterpret_cast<void*>(
        dataTablesAddress + DescriptionVectorOffset);
    NativeVectorU16 vectorBefore{};
    if (!SafeRead(vectorAddress, vectorBefore)
            || !IsAccessibleRange(
                vectorAddress, sizeof(NativeVectorU16), true)) {
        return PreMutationFailure("description vector layout is inaccessible");
    }
    const auto capacityBefore =
        vectorBefore.capacityAndFlags & NativeVectorCapacityMask;
    if (vectorBefore.size > capacityBefore
            || (capacityBefore != 0 && !vectorBefore.begin)) {
        return PreMutationFailure("description vector layout is invalid");
    }

    NativeVectorMutationStarted = true;
    if (!InvokeNativeResize(vectorAddress, descriptionCount)) {
        FailClosed("native description vector resize failed", rowCount);
    }

    NativeVectorU16 vectorAfter{};
    if (!SafeRead(vectorAddress, vectorAfter)) {
        FailClosed("resized description vector is unreadable", rowCount);
    }
    const auto capacityAfter =
        vectorAfter.capacityAndFlags & NativeVectorCapacityMask;
    const auto outputBytes = descriptionCount * sizeof(std::uint16_t);
    if (vectorAfter.size != descriptionCount
            || capacityAfter < vectorAfter.size
            || (descriptionCount != 0 && !vectorAfter.begin)) {
        FailClosed("resized description vector has an invalid layout", rowCount);
    }
    if (descriptionCount != 0) {
        std::array<std::uint16_t, MaximumRecordCount> ids;
        for (std::size_t index{}; index < descriptionCount; ++index) {
            ids[index] = staged[index].statId;
        }
        if (!SafeWrite(vectorAfter.begin, ids.data(), outputBytes)) {
            FailClosed("resized description vector is unwritable", rowCount);
        }
    }

    NativeVectorMutationStarted = false;
    return 1;
}

} // namespace

extern "C" auto ISC12BuildDescriptionIndex(void* dataTables) noexcept
        -> std::uint32_t {
    try {
        return BuildDescriptionIndexNative(dataTables);
    } catch (...) {
        if (NativeVectorMutationStarted
                || !CurrentRowCountKnown
                || CurrentRowCount > LegacySerializedSentinel) {
            FailClosed(
                NativeVectorMutationStarted
                    ? "exception after native vector mutation"
                    : "exception on an extended ItemStatCost table",
                CurrentRowCount);
        }
        return 0;
    }
}

auto PrepareLoaderExtension(
        const D2RL::PluginContext* context,
        std::uint8_t* base,
        std::size_t imageSize,
        bool,
        std::string& error) noexcept -> bool {
    error.clear();
    if (!context || !base || imageSize == 0
            || context->exeBase != reinterpret_cast<std::uintptr_t>(base)) {
        return SetError(error, "loader extension received an invalid context");
    }
    if (AnyMutationInstalled) {
        return SetError(
            error,
            "loader extension cannot be prepared again after native mutation");
    }

    ReleaseUnpatchedResources();
    Prepared = false;
    TailPatchInstalled = false;
    CapPatchInstalled = false;
    ColdRestartRequired = false;
    LoaderContext = context;
    LoaderBase = base;
    LoaderImageSize = imageSize;
    BuildCalls.store(0, std::memory_order_release);
    LastRowCount.store(0, std::memory_order_release);
    LastDescriptionCount.store(0, std::memory_order_release);

    if (!ValidateImageTarget(TailPatchRva, TailExpected.size(), true)
            || !ValidateImageTarget(
                CountImmediateRva, CountImmediateExpected.size(), true)
            || !ValidateImageTarget(NativeQsortRva, 1, true)
            || !ValidateImageTarget(NativeComparatorRva, 1, true)
            || !ValidateImageTarget(NativeVectorResizeRva, 1, true)
            || !ValidateImageTarget(VanillaContinuationRva, 1, true)
            || !ValidateImageTarget(EpilogueRva, 1, true)) {
        return SetError(error, "loader extension target lies outside executable image");
    }

    NativeQsort = reinterpret_cast<NativeQsortFn>(LoaderBase + NativeQsortRva);
    NativeComparator = reinterpret_cast<NativeComparatorFn>(
        LoaderBase + NativeComparatorRva);
    NativeVectorResize = reinterpret_cast<NativeVectorResizeFn>(
        LoaderBase + NativeVectorResizeRva);
    if (!PrepareRelay(error)) return false;

    Prepared = true;
    return true;
}

auto InstallLoaderExtension(std::string& error) noexcept
        -> LoaderInstallResult {
    error.clear();
    if (!Prepared || !LoaderContext || !LoaderBase || !RelayPage || !State) {
        SetError(error, "loader extension was not prepared");
        return LoaderInstallResult::FailedBeforeMutation;
    }

    const auto baseAddress = reinterpret_cast<std::uintptr_t>(LoaderBase);
    const auto relayAddress = reinterpret_cast<std::uintptr_t>(RelayPage);
    if (relayAddress < baseAddress
            || !CanEncodeRel32(baseAddress + TailPatchRva, relayAddress)) {
        SetError(error, "persistent relay is outside rel32 reach");
        return LoaderInstallResult::FailedBeforeMutation;
    }
    const auto relayRva = relayAddress - baseAddress;
    const auto result = CommitLoaderMutation(
        [&]() noexcept {
            // A false PluginSDK result does not prove that this non-aligned
            // seam remained untouched. Reserve the relay/state for the rest
            // of the process before the first native write is attempted.
            AnyMutationInstalled = true;
        },
        [&]() noexcept {
            return LoaderContext->PatchJmpRel32(
                TailPatchRva,
                TailExpected.data(),
                static_cast<std::uint32_t>(TailExpected.size()),
                relayRva,
                static_cast<std::uint32_t>(TailExpected.size()));
        },
        [&]() noexcept {
            TailPatchInstalled = true;
            // PatchWriteU32 returning false does not prove that its four-byte
            // target remained untouched. Publish the conservative guard before
            // attempting that write so an inactive relay never assumes 511.
            InterlockedExchange(&State->capMayBeExtended, 1);
            InterlockedExchange(&State->operational, 1);
        },
        [&]() noexcept {
            return LoaderContext->PatchWriteU32(
                CountImmediateRva,
                CountImmediateExpected.data(),
                static_cast<std::uint32_t>(CountImmediateExpected.size()),
                SerializedSentinel);
        },
        [&]() noexcept {
            CapPatchInstalled = true;
        });

    if (result == LoaderInstallResult::FailedBeforeMutation) {
        SetError(error, "DescFunc tail seam is already owned");
        return result;
    }
    if (result
            == LoaderInstallResult::PartialCommitColdRestartRequired) {
        ColdRestartRequired = true;
        if (!TailPatchInstalled) {
            std::array<std::uint8_t, TailExpected.size()> observedSeam{};
            const bool observed = SafeRead(
                LoaderBase + TailPatchRva, observedSeam);
            char message[384]{};
            if (observed) {
                std::snprintf(
                    message,
                    sizeof(message),
                    "DescFunc tail publication returned an uncertain result "
                    "(observed seam=%02X %02X %02X %02X %02X %02X %02X %02X); "
                    "relay/state retained and cold restart required",
                    static_cast<unsigned>(observedSeam[0]),
                    static_cast<unsigned>(observedSeam[1]),
                    static_cast<unsigned>(observedSeam[2]),
                    static_cast<unsigned>(observedSeam[3]),
                    static_cast<unsigned>(observedSeam[4]),
                    static_cast<unsigned>(observedSeam[5]),
                    static_cast<unsigned>(observedSeam[6]),
                    static_cast<unsigned>(observedSeam[7]));
            } else {
                std::snprintf(
                    message,
                    sizeof(message),
                    "DescFunc tail publication returned an uncertain result "
                    "(seam unreadable); relay/state retained and cold restart "
                    "required");
            }
            SetError(error, message);
            if (LoaderContext) LoaderContext->LogError(message);
            FailClosed(
                "DescFunc tail publication returned an uncertain result; "
                "cold restart required",
                0);
        }
        std::uint32_t observedCap{};
        const bool observed = SafeRead(
            LoaderBase + CountImmediateRva, observedCap);
        char message[256]{};
        if (observed) {
            std::snprintf(
                message,
                sizeof(message),
                "count-cap commit failed after the safe tail was installed "
                "(observed immediate=0x%08X); cold restart required",
                observedCap);
        } else {
            std::snprintf(
                message,
                sizeof(message),
                "count-cap commit failed after the safe tail was installed "
                "(immediate unreadable); cold restart required");
        }
        SetError(error, message);
        return result;
    }
    return result;
}

auto ShutdownLoaderExtension() noexcept -> void {
    if (State) {
        InterlockedExchange(&State->operational, 0);
        const auto deadline = GetTickCount64()
            + ShutdownRundownTimeoutMilliseconds;
        while (InterlockedCompareExchange(
                &State->activeCallbacks, 0, 0) != 0) {
            if (GetTickCount64() >= deadline) {
                FailClosed(
                    "loader relay rundown timed out during unload",
                    LastRowCount.load(std::memory_order_acquire));
            }
            YieldProcessor();
        }
        State->handler = nullptr;
    }
    Prepared = false;
    if (!AnyMutationInstalled) {
        ReleaseUnpatchedResources();
        LoaderContext = nullptr;
        LoaderBase = nullptr;
        LoaderImageSize = 0;
        NativeQsort = nullptr;
        NativeComparator = nullptr;
        NativeVectorResize = nullptr;
        gISC12LoaderSuccessExit = nullptr;
        gISC12LoaderVanillaExit = nullptr;
    }
    // Once any native patch call is attempted, the RX relay and RW guard state
    // remain process-lifetime. A false PluginSDK result cannot prove that the
    // target bytes stayed untouched. The inactive relay never enters the
    // unloaded DLL.
}

auto GetLoaderRuntimeStatus() noexcept -> LoaderRuntimeStatus {
    LoaderRuntimeStatus status{
        .prepared = Prepared,
        .tailPatchInstalled = TailPatchInstalled,
        .capPatchInstalled = CapPatchInstalled,
        .operational = State
            && InterlockedCompareExchange(&State->operational, 0, 0) != 0,
        .coldRestartRequired = ColdRestartRequired,
        .buildCalls = BuildCalls.load(std::memory_order_acquire),
        .lastRowCount = LastRowCount.load(std::memory_order_acquire),
        .lastDescriptionCount =
            LastDescriptionCount.load(std::memory_order_acquire),
    };
    return status;
}

} // namespace ruffneckk::isc12
