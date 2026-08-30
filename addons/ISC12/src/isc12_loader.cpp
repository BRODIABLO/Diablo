#include <D2RLPlugin/api.h>

#include "isc12_contract.hpp"
#include "isc12_atomic_file.hpp"
#include "isc12_codec_patch.hpp"
#include "isc12_loader.hpp"
#include "isc12_native_persistence_adapter.hpp"
#include "isc12_native_schema_adapter.hpp"

#include <Windows.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

extern "C" {

void* gISC12LoaderSuccessExit{};
void* gISC12LoaderVanillaExit{};
void* gISC12PersistenceReaderContinueExit{};
void* gISC12PersistenceReaderRejectedExit{};
void* gISC12PersistenceWriterVanillaExit{};
void* gISC12PersistenceWriterCommittedExit{};
void* gISC12PersistenceWriterRejectedExit{};

void ISC12LoaderTailMidHook() noexcept;
extern std::uint8_t ISC12LoaderRelayTemplateBegin;
extern std::uint8_t ISC12LoaderRelayTemplateVanillaExit;
extern std::uint8_t ISC12LoaderRelayTemplateSuccessExit;
extern std::uint8_t ISC12LoaderRelayTemplateStatePointer;
extern std::uint8_t ISC12LoaderRelayTemplateEnd;

void ISC12PersistenceReaderMidHook() noexcept;
void ISC12PersistenceWriterMidHook() noexcept;
extern std::uint8_t ISC12PersistenceRelayTemplateBegin;
extern std::uint8_t ISC12PersistenceRelayTemplateWriterEntry;
extern std::uint8_t ISC12PersistenceRelayTemplateReaderContinueExit;
extern std::uint8_t ISC12PersistenceRelayTemplateReaderRejectedExit;
extern std::uint8_t ISC12PersistenceRelayTemplateWriterVanillaExit;
extern std::uint8_t ISC12PersistenceRelayTemplateWriterCommittedExit;
extern std::uint8_t ISC12PersistenceRelayTemplateWriterRejectedExit;
extern std::uint8_t ISC12PersistenceRelayTemplatePlayerSaveFinalizeEntry;
extern std::uint8_t ISC12PersistenceRelayTemplateStatePointer;
extern std::uint8_t ISC12PersistenceRelayTemplateEnd;

}

namespace ruffneckk::isc12 {

class LoaderCodecPatchAuthority {
public:
    [[nodiscard]] static constexpr auto BindPreparedRelay(
            std::uintptr_t playerSaveFinalizeRelayRva) noexcept
            -> CodecPatchActivationTargets {
        return CodecPatchActivationTargets{playerSaveFinalizeRelayRva};
    }
};

namespace {

constexpr std::uintptr_t TailPatchRva = 0x31F0AB;
constexpr std::uintptr_t VanillaContinuationRva = 0x31F0B3;
constexpr std::uintptr_t EpilogueRva = 0x31F1B3;
constexpr std::uintptr_t CountImmediateRva = 0x31ED38;
constexpr std::uintptr_t NativeQsortRva = 0x12E6F60;
constexpr std::uintptr_t NativeComparatorRva = 0x320880;
constexpr std::uintptr_t NativeVectorResizeRva = 0x323810;
constexpr std::uintptr_t PersistenceReaderPatchRva = 0x9FC654;
constexpr std::uintptr_t PersistenceReaderContinueRva = 0x9FC659;
constexpr std::uintptr_t PersistenceReaderRejectedRva = 0x9FC66F;
constexpr std::uintptr_t PersistenceWriterPatchRva = 0x9F95A2;
constexpr std::uintptr_t PersistenceWriterVanillaRva = 0x9F95A7;
constexpr std::uintptr_t PersistenceWriterCommittedRva = 0x9F95E9;
constexpr std::uintptr_t PersistenceWriterRejectedRva = 0x9F9627;
constexpr std::uintptr_t PlayerSaveFinalizeCallRva = 0x5353C2;
constexpr std::uintptr_t NativeByteBufferResizeRva = 0xA1E1F0;
constexpr std::uintptr_t NativeSetObjectStateRva = 0xA1E200;
constexpr std::uintptr_t NativeSetObjectAuxRva = 0xA1E210;
constexpr std::uintptr_t NativeIoSuccessCodeRva = 0x1E9C0E8;

constexpr std::size_t RecordsPointerOffset = 0x1258;
constexpr std::size_t RecordCountOffset = 0x1260;
constexpr std::size_t DescriptionVectorOffset = 0x1278;
constexpr std::size_t RecordStride = CompiledItemStatRecordStride;
constexpr std::size_t DescriptionPriorityOffset = 0x30;
constexpr std::size_t DescriptionFunctionOffset = 0x32;
constexpr std::size_t SaveObjectBufferPointerOffset = 0x08;
constexpr std::size_t SaveObjectBufferSizeOffset = 0x10;
constexpr std::size_t SaveObjectNamePointerOffset = 0x20;
constexpr std::size_t MaximumNativeStoreNameLength = 255;

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
constexpr std::size_t MaximumPersistenceRelayTemplateSize = 1024;
constexpr ULONGLONG ShutdownRundownTimeoutMilliseconds = 5000;
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

struct PersistenceRelayState {
    volatile LONG activeCallbacks{};
    volatile LONG operational{};
    volatile LONG codecReady{};
    LONG reserved{};
    void* readerHandler{};
    void* writerHandler{};
    void* readerContinue{};
    void* readerRejected{};
    void* writerVanilla{};
    void* writerCommitted{};
    void* writerRejected{};
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
using NativeGetLinkNameCountFn = std::uint64_t(__fastcall*)(const void*);
using NativeGetLinkNameFn = const char*(__fastcall*)(
    const void*, std::int32_t, std::int32_t);
using NativeByteBufferResizeFn = void(__fastcall*)(void*, std::size_t);
using NativeSetObjectStateFn = void(__fastcall*)(void*, std::uint32_t);
using NativeSetObjectAuxFn = void(__fastcall*)(void*, std::uint64_t);

static_assert(sizeof(DescriptionEntry) == 4);
static_assert(offsetof(DescriptionEntry, statId) == 0);
static_assert(offsetof(DescriptionEntry, priority) == 2);
static_assert(offsetof(RelayState, capMayBeExtended) == 8);
static_assert(offsetof(RelayState, handler) == RelayStateHandlerOffset);
static_assert(
    offsetof(RelayState, vanillaContinuation) == RelayStateVanillaOffset);
static_assert(offsetof(RelayState, epilogue) == RelayStateEpilogueOffset);
static_assert(offsetof(PersistenceRelayState, codecReady) == 8);
static_assert(offsetof(PersistenceRelayState, readerHandler) == 0x10);
static_assert(offsetof(PersistenceRelayState, writerHandler) == 0x18);
static_assert(offsetof(PersistenceRelayState, readerContinue) == 0x20);
static_assert(offsetof(PersistenceRelayState, readerRejected) == 0x28);
static_assert(offsetof(PersistenceRelayState, writerVanilla) == 0x30);
static_assert(offsetof(PersistenceRelayState, writerCommitted) == 0x38);
static_assert(offsetof(PersistenceRelayState, writerRejected) == 0x40);
static_assert(sizeof(NativeVectorU16) == 24);
static_assert(offsetof(NativeVectorU16, begin) == 0);
static_assert(offsetof(NativeVectorU16, size) == 8);
static_assert(offsetof(NativeVectorU16, capacityAndFlags) == 16);
static_assert(RecordStride == 0x144);
static_assert(std::is_nothrow_move_assignable_v<
    NativeItemStatCostSchemaSnapshot>);

const D2RL::PluginContext* LoaderContext{};
std::uint8_t* LoaderBase{};
std::size_t LoaderImageSize{};
void* RelayPage{};
std::size_t RelayPageSize{};
RelayState* State{};
std::size_t StatePageSize{};
void* PersistenceRelayPage{};
std::size_t PersistenceRelayPageSize{};
PersistenceRelayState* PersistenceState{};
std::size_t PersistenceStatePageSize{};
void* PersistenceReaderRelayEntry{};
void* PersistenceWriterRelayEntry{};
void* PersistencePlayerSaveFinalizeRelayEntry{};
CodecPatchActivationTargets PreparedCodecActivationTargets{};
NativeQsortFn NativeQsort{};
NativeComparatorFn NativeComparator{};
NativeVectorResizeFn NativeVectorResize{};
NativeGetLinkNameCountFn NativeGetLinkNameCount{};
NativeGetLinkNameFn NativeGetLinkName{};
NativeByteBufferResizeFn NativeByteBufferResize{};
NativeSetObjectStateFn NativeSetObjectState{};
NativeSetObjectAuxFn NativeSetObjectAux{};
bool Prepared{};
bool PersistencePrepared{};
bool AnyMutationInstalled{};
bool TailPatchInstalled{};
bool CapPatchInstalled{};
bool ColdRestartRequired{};
std::atomic_uint64_t BuildCalls{};
std::atomic_uint64_t LastRowCount{};
std::atomic_uint64_t LastDescriptionCount{};
std::atomic_bool SchemaReady{};
SRWLOCK SchemaSnapshotLock = SRWLOCK_INIT;
NativeItemStatCostSchemaSnapshot PublishedSchemaSnapshot;
bool HasPublishedSchemaSnapshot{};
bool SchemaUpdateInProgress{};
thread_local bool NativeVectorMutationStarted{};
thread_local bool CurrentRowCountKnown{};
thread_local std::uint64_t CurrentRowCount{};
thread_local bool NativeSchemaCallbackFailed{};
thread_local std::array<char, MaximumNativeStatNameLength + 1>
    NativeSchemaNameBuffer{};

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

auto NativeExceptionFilter(DWORD code) noexcept -> int {
    return code == EXCEPTION_ACCESS_VIOLATION
            || code == EXCEPTION_IN_PAGE_ERROR
            || code == EXCEPTION_GUARD_PAGE
        ? EXCEPTION_EXECUTE_HANDLER
        : EXCEPTION_CONTINUE_SEARCH;
}

template <typename Value>
auto SafeRead(const void* source, Value& destination) noexcept -> bool {
    static_assert(std::is_trivially_copyable_v<Value>);
    if (!IsAccessibleRange(source, sizeof(Value), false)) return false;
    __try {
        std::memcpy(&destination, source, sizeof(Value));
        return true;
    } __except (NativeExceptionFilter(GetExceptionCode())) {
        return false;
    }
}

auto SafeCopyReadable(
        const void* source,
        void* destination,
        std::size_t size) noexcept -> bool {
    if (size == 0) return true;
    if (!destination || !IsAccessibleRange(source, size, false)) return false;
    __try {
        std::memcpy(destination, source, size);
        return true;
    } __except (NativeExceptionFilter(GetExceptionCode())) {
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
    } __except (NativeExceptionFilter(GetExceptionCode())) {
        return false;
    }
}

auto InvokeNativeGetLinkNameCount(
        const void* linker,
        std::uint64_t& count) noexcept -> bool {
    if (!NativeGetLinkNameCount || !linker) return false;
    __try {
        count = NativeGetLinkNameCount(linker);
        return true;
    } __except (NativeExceptionFilter(GetExceptionCode())) {
        return false;
    }
}

auto InvokeNativeGetLinkName(
        const void* linker,
        std::int32_t ordinal,
        const char*& name) noexcept -> bool {
    if (!NativeGetLinkName || !linker) return false;
    __try {
        name = NativeGetLinkName(
            linker, ordinal, NativeGetLinkNameMode);
        return true;
    } __except (NativeExceptionFilter(GetExceptionCode())) {
        return false;
    }
}

auto IsReadableProtection(DWORD protection) noexcept -> bool {
    const auto baseProtection = protection & 0xFFU;
    return baseProtection == PAGE_READONLY
        || baseProtection == PAGE_READWRITE
        || baseProtection == PAGE_WRITECOPY
        || baseProtection == PAGE_EXECUTE_READ
        || baseProtection == PAGE_EXECUTE_READWRITE
        || baseProtection == PAGE_EXECUTE_WRITECOPY;
}

template <std::size_t Capacity>
auto CopyBoundedReadableCString(
        const char* name,
        std::size_t maximumLength,
        bool allowEmpty,
        std::array<char, Capacity>& output,
        std::string_view& view) noexcept -> bool {
    static_assert(Capacity > 0);
    view = {};
    if (!name || maximumLength >= Capacity) return false;
    const auto scanLimit = maximumLength + 1;
    const auto start = reinterpret_cast<std::uintptr_t>(name);
    std::size_t scanned{};
    while (scanned < scanLimit) {
        if (start > std::numeric_limits<std::uintptr_t>::max() - scanned) {
            return false;
        }
        const auto cursorAddress = start + scanned;
        MEMORY_BASIC_INFORMATION memory{};
        if (VirtualQuery(
                reinterpret_cast<const void*>(cursorAddress),
                &memory,
                sizeof(memory)) != sizeof(memory)
                || memory.State != MEM_COMMIT
                || (memory.Protect & (PAGE_GUARD | PAGE_NOACCESS)) != 0
                || !IsReadableProtection(memory.Protect)) {
            return false;
        }
        const auto regionStart = reinterpret_cast<std::uintptr_t>(
            memory.BaseAddress);
        if (regionStart > std::numeric_limits<std::uintptr_t>::max()
                - memory.RegionSize) {
            return false;
        }
        const auto regionEnd = regionStart + memory.RegionSize;
        if (regionEnd <= cursorAddress) return false;
        const auto readable = static_cast<std::size_t>(
            regionEnd - cursorAddress);
        const auto chunk = (std::min)(readable, scanLimit - scanned);

        std::size_t localLength{};
        bool found{};
        __try {
            const auto* cursor = reinterpret_cast<const char*>(cursorAddress);
            while (localLength < chunk) {
                const auto value = cursor[localLength];
                if (value == '\0') {
                    found = true;
                    break;
                }
                const auto outputOffset = scanned + localLength;
                if (outputOffset >= maximumLength) {
                    return false;
                }
                output[outputOffset] = value;
                ++localLength;
            }
        } __except (NativeExceptionFilter(GetExceptionCode())) {
            return false;
        }
        if (found) {
            const auto length = scanned + localLength;
            if ((!allowEmpty && length == 0) || length > maximumLength) {
                return false;
            }
            output[length] = '\0';
            view = std::string_view{output.data(), length};
            return true;
        }
        scanned += chunk;
    }
    return false;
}

auto CopyBoundedReadableName(
        const char* name,
        std::string_view& view) noexcept -> bool {
    return CopyBoundedReadableCString(
        name,
        MaximumNativeStatNameLength,
        false,
        NativeSchemaNameBuffer,
        view);
}

auto GetLinkNameCountForSnapshot(const void* linker) noexcept
        -> std::uint64_t {
    std::uint64_t count{};
    if (!InvokeNativeGetLinkNameCount(linker, count)) {
        NativeSchemaCallbackFailed = true;
        return (std::numeric_limits<std::uint64_t>::max)();
    }
    return count;
}

auto GetLinkNameForSnapshot(
        const void* linker,
        std::int32_t ordinal) noexcept -> std::string_view {
    const char* nativeName{};
    std::string_view view;
    if (!InvokeNativeGetLinkName(linker, ordinal, nativeName)
            || !CopyBoundedReadableName(nativeName, view)) {
        NativeSchemaCallbackFailed = true;
        return {};
    }
    return view;
}

auto ResetPublishedSchemaSnapshot() noexcept -> void {
    AcquireSRWLockExclusive(&SchemaSnapshotLock);
    PublishedSchemaSnapshot = {};
    HasPublishedSchemaSnapshot = false;
    SchemaUpdateInProgress = false;
    SchemaReady.store(false, std::memory_order_release);
    ReleaseSRWLockExclusive(&SchemaSnapshotLock);
}

// The exclusive lock remains held throughout record/name capture and hashing.
// Future schema consumers take the shared side of this lock and recheck ready,
// so no save can observe the previous snapshot during a table reload.
auto BeginSchemaSnapshotUpdate() noexcept -> void {
    AcquireSRWLockExclusive(&SchemaSnapshotLock);
    SchemaUpdateInProgress = true;
    SchemaReady.store(false, std::memory_order_release);
}

auto CompleteSchemaSnapshotUpdate(
        SchemaError captureResult,
        NativeItemStatCostSchemaSnapshot&& candidate) noexcept
        -> NativeSchemaGateDecision {
    const auto decision = DecideNativeSchemaGate(
        captureResult,
        HasPublishedSchemaSnapshot,
        PublishedSchemaSnapshot.schemaHash,
        candidate.schemaHash);
    if (decision == NativeSchemaGateDecision::Publish) {
        PublishedSchemaSnapshot = std::move(candidate);
        HasPublishedSchemaSnapshot = true;
    }
    SchemaUpdateInProgress = false;
    if (decision != NativeSchemaGateDecision::FailClosed) {
        SchemaReady.store(true, std::memory_order_release);
    }
    ReleaseSRWLockExclusive(&SchemaSnapshotLock);
    return decision;
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

auto InvokeNativeByteBufferResize(
        void* object,
        std::size_t count) noexcept -> bool {
    if (!NativeByteBufferResize || !object) return false;
    __try {
        NativeByteBufferResize(object, count);
        return true;
    } __except (NativeExceptionFilter(GetExceptionCode())) {
        return false;
    }
}

auto InvokeNativeSetObjectState(
        void* object,
        std::uint32_t state) noexcept -> bool {
    if (!NativeSetObjectState || !object) return false;
    __try {
        NativeSetObjectState(object, state);
        return true;
    } __except (NativeExceptionFilter(GetExceptionCode())) {
        return false;
    }
}

auto InvokeNativeSetObjectAux(
        void* object,
        std::uint64_t value) noexcept -> bool {
    if (!NativeSetObjectAux || !object) return false;
    __try {
        NativeSetObjectAux(object, value);
        return true;
    } __except (NativeExceptionFilter(GetExceptionCode())) {
        return false;
    }
}

auto ReadNativeStoreName(
        void* object,
        std::array<char, MaximumNativeStoreNameLength + 1>& storage,
        std::string_view& name) noexcept -> bool {
    name = {};
    if (!object) return false;
    const auto address = reinterpret_cast<std::uintptr_t>(object);
    if (address > std::numeric_limits<std::uintptr_t>::max()
            - SaveObjectNamePointerOffset - sizeof(std::uint64_t)) {
        return false;
    }
    const char* pointer{};
    std::uint64_t length{};
    if (!SafeRead(
            reinterpret_cast<const void*>(
                address + SaveObjectNamePointerOffset),
            pointer)
            || !SafeRead(
                reinterpret_cast<const void*>(
                    address + SaveObjectNamePointerOffset + sizeof(void*)),
                length)
            || !pointer || length == 0
            || length > MaximumNativeStoreNameLength) {
        return false;
    }
    const auto nativeLength = static_cast<std::size_t>(length);
    if (!SafeCopyReadable(pointer, storage.data(), nativeLength)) {
        return false;
    }
    const auto pointerAddress = reinterpret_cast<std::uintptr_t>(pointer);
    if (pointerAddress > std::numeric_limits<std::uintptr_t>::max()
            - nativeLength) {
        return false;
    }
    char terminator{};
    if (!SafeRead(
            reinterpret_cast<const char*>(pointerAddress + nativeLength),
            terminator)
            || terminator != '\0') {
        return false;
    }
    storage[nativeLength] = '\0';
    name = std::string_view{storage.data(), nativeLength};
    return true;
}

auto SnapshotNativeObjectBuffer(
        void* object,
        const std::uint64_t* expectedLength,
        std::uint64_t maximumLength,
        std::vector<std::uint8_t>& bytes) noexcept -> bool {
    if (!object) return false;
    const auto address = reinterpret_cast<std::uintptr_t>(object);
    if (address > std::numeric_limits<std::uintptr_t>::max()
            - SaveObjectBufferSizeOffset - sizeof(std::uint64_t)) {
        return false;
    }
    std::uint8_t* pointer{};
    std::uint64_t length{};
    if (!SafeRead(
            reinterpret_cast<const void*>(
                address + SaveObjectBufferPointerOffset),
            pointer)
            || !SafeRead(
                reinterpret_cast<const void*>(
                    address + SaveObjectBufferSizeOffset),
                length)
            || (expectedLength && length != *expectedLength)
            || length > maximumLength
            || length > (std::numeric_limits<std::size_t>::max)()) {
        return false;
    }
    try {
        std::vector<std::uint8_t> staged(
            static_cast<std::size_t>(length));
        if (!staged.empty()
                && (!pointer
                    || !SafeCopyReadable(
                        pointer, staged.data(), staged.size()))) {
            return false;
        }
        bytes.swap(staged);
        return true;
    } catch (...) {
        return false;
    }
}

auto ReplaceNativeReadBuffer(
        void* context,
        std::span<const std::uint8_t> bytes) noexcept -> bool {
    if (!context) return false;
    const auto address = reinterpret_cast<std::uintptr_t>(context);
    if (address > std::numeric_limits<std::uintptr_t>::max()
            - SaveObjectBufferSizeOffset - sizeof(std::uint64_t)) {
        return false;
    }
    if (!InvokeNativeByteBufferResize(context, bytes.size())) return false;
    std::uint8_t* destination{};
    std::uint64_t observedSize{};
    if (!SafeRead(
            reinterpret_cast<const void*>(
                address + SaveObjectBufferPointerOffset),
            destination)
            || !SafeRead(
                reinterpret_cast<const void*>(
                    address + SaveObjectBufferSizeOffset),
                observedSize)
            || observedSize != bytes.size()
            || (!bytes.empty() && !destination)) {
        return false;
    }
    return SafeWrite(destination, bytes.data(), bytes.size());
}

auto ClearRejectedNativeRead(void* context) noexcept -> bool {
    return InvokeNativeByteBufferResize(context, 0)
        && InvokeNativeSetObjectAux(context, 0)
        && InvokeNativeSetObjectState(context, 0);
}

[[noreturn]] auto FailClosed(
    const char* reason,
    std::uint64_t rowCount) noexcept -> void;

auto CommitNativeStoreAtomically(
        void*,
        std::wstring_view path,
        std::span<const std::uint8_t> bytes) noexcept -> bool {
    const auto result = WriteFileAtomically(path, bytes);
    if (!result.committed) {
        if (LoaderContext) {
            char message[256]{};
            std::snprintf(
                message,
                sizeof(message),
                "ISC12: atomic save commit failed (stage=%u; win32=%lu; "
                "rollback=%s).",
                static_cast<unsigned>(result.stage),
                static_cast<unsigned long>(result.windowsError),
                result.rollbackAttempted
                    ? result.rollbackSucceeded ? "restored" : "failed"
                    : "not-needed");
            LoaderContext->LogError(message);
        }
        if (!AtomicFailurePreservedDestination(result)) {
            FailClosed(
                "atomic save rollback failed after a destination transition",
                0);
        }
        return false;
    }
    if (result.cleanupWarning && LoaderContext) {
        LoaderContext->LogWarn(
            "ISC12: atomic save committed with a backup-cleanup warning.");
    }
    return true;
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

[[noreturn]] auto RejectSchemaSnapshotUpdate(
        SchemaError captureResult,
        const char* reason,
        std::uint64_t rowCount) noexcept -> void {
    NativeItemStatCostSchemaSnapshot rejected;
    const auto decision = CompleteSchemaSnapshotUpdate(
        captureResult, std::move(rejected));
    (void)decision;
    FailClosed(reason, rowCount);
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
    PersistencePrepared = false;
    if (PersistenceRelayPage) {
        VirtualFree(PersistenceRelayPage, 0, MEM_RELEASE);
        PersistenceRelayPage = nullptr;
        PersistenceRelayPageSize = 0;
    }
    if (PersistenceState) {
        VirtualFree(PersistenceState, 0, MEM_RELEASE);
        PersistenceState = nullptr;
        PersistenceStatePageSize = 0;
    }
    PersistenceReaderRelayEntry = nullptr;
    PersistenceWriterRelayEntry = nullptr;
    PersistencePlayerSaveFinalizeRelayEntry = nullptr;
    PreparedCodecActivationTargets = {};
    gISC12PersistenceReaderContinueExit = nullptr;
    gISC12PersistenceReaderRejectedExit = nullptr;
    gISC12PersistenceWriterVanillaExit = nullptr;
    gISC12PersistenceWriterCommittedExit = nullptr;
    gISC12PersistenceWriterRejectedExit = nullptr;
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

auto AllocatePersistenceRelayPageNear(std::uint8_t* base) noexcept -> void* {
    if (!base) return nullptr;
    SYSTEM_INFO systemInfo{};
    GetSystemInfo(&systemInfo);
    const auto granularity = static_cast<std::uintptr_t>(
        systemInfo.dwAllocationGranularity);
    const auto pageSize = static_cast<std::size_t>(systemInfo.dwPageSize);
    if (granularity == 0
            || pageSize < MaximumPersistenceRelayTemplateSize) {
        return nullptr;
    }
    const auto readerSite = reinterpret_cast<std::uintptr_t>(
        base + PersistenceReaderPatchRva);
    const auto writerSite = reinterpret_cast<std::uintptr_t>(
        base + PersistenceWriterPatchRva);
    const auto playerSaveFinalizeSite = reinterpret_cast<std::uintptr_t>(
        base + PlayerSaveFinalizeCallRva);
    const auto aligned = readerSite & ~(granularity - 1U);
    for (std::uintptr_t delta = granularity;
            delta < UINT64_C(0x70000000);
            delta += granularity) {
        if (aligned > std::numeric_limits<std::uintptr_t>::max() - delta) {
            break;
        }
        const auto candidate = aligned + delta;
        if (!CanEncodeRel32(readerSite, candidate)
                || !CanEncodeRel32(writerSite, candidate)
                || !CanEncodeRel32(playerSaveFinalizeSite, candidate)) {
            break;
        }
        if (auto* allocation = VirtualAlloc(
                reinterpret_cast<void*>(candidate),
                pageSize,
                MEM_COMMIT | MEM_RESERVE,
                PAGE_READWRITE)) {
            PersistenceRelayPageSize = pageSize;
            return allocation;
        }
    }
    return nullptr;
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

auto PreparePersistenceRelay(std::string& error) noexcept -> bool {
    SYSTEM_INFO systemInfo{};
    GetSystemInfo(&systemInfo);
    const auto pageSize = static_cast<std::size_t>(systemInfo.dwPageSize);
    if (pageSize < sizeof(PersistenceRelayState)) {
        return SetError(
            error, "invalid Windows page size for persistence state");
    }
    PersistenceState = static_cast<PersistenceRelayState*>(VirtualAlloc(
        nullptr,
        pageSize,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_READWRITE));
    if (!PersistenceState) {
        return SetError(error, "persistence relay state allocation failed");
    }
    PersistenceStatePageSize = pageSize;

    PersistenceRelayPage = AllocatePersistenceRelayPageNear(LoaderBase);
    if (!PersistenceRelayPage) {
        ReleaseUnpatchedResources();
        return SetError(
            error,
            "no persistence relay page is available within rel32 reach");
    }

    const auto begin = reinterpret_cast<std::uintptr_t>(
        &ISC12PersistenceRelayTemplateBegin);
    const auto writerEntry = reinterpret_cast<std::uintptr_t>(
        &ISC12PersistenceRelayTemplateWriterEntry);
    const auto readerContinue = reinterpret_cast<std::uintptr_t>(
        &ISC12PersistenceRelayTemplateReaderContinueExit);
    const auto readerRejected = reinterpret_cast<std::uintptr_t>(
        &ISC12PersistenceRelayTemplateReaderRejectedExit);
    const auto writerVanilla = reinterpret_cast<std::uintptr_t>(
        &ISC12PersistenceRelayTemplateWriterVanillaExit);
    const auto writerCommitted = reinterpret_cast<std::uintptr_t>(
        &ISC12PersistenceRelayTemplateWriterCommittedExit);
    const auto writerRejected = reinterpret_cast<std::uintptr_t>(
        &ISC12PersistenceRelayTemplateWriterRejectedExit);
    const auto playerSaveFinalize = reinterpret_cast<std::uintptr_t>(
        &ISC12PersistenceRelayTemplatePlayerSaveFinalizeEntry);
    const auto statePointer = reinterpret_cast<std::uintptr_t>(
        &ISC12PersistenceRelayTemplateStatePointer);
    const auto end = reinterpret_cast<std::uintptr_t>(
        &ISC12PersistenceRelayTemplateEnd);
    const auto labelInside = [begin, end](std::uintptr_t label) noexcept {
        return label >= begin && label < end;
    };
    if (end <= begin || end - begin < sizeof(void*)
            || !labelInside(writerEntry)
            || !labelInside(readerContinue)
            || !labelInside(readerRejected)
            || !labelInside(writerVanilla)
            || !labelInside(writerCommitted)
            || !labelInside(writerRejected)
            || !labelInside(playerSaveFinalize)
            || statePointer < begin
            || statePointer > end - sizeof(void*)
            || !(begin < readerContinue
                && readerContinue < readerRejected
                && readerRejected < writerEntry
                && writerEntry < writerVanilla
                && writerVanilla < writerCommitted
                && writerCommitted < writerRejected
                && writerRejected < playerSaveFinalize
                && playerSaveFinalize < statePointer)) {
        ReleaseUnpatchedResources();
        return SetError(
            error, "persistence relay template layout is invalid");
    }
    const auto templateSize = static_cast<std::size_t>(end - begin);
    if (templateSize > MaximumPersistenceRelayTemplateSize
            || templateSize > PersistenceRelayPageSize) {
        ReleaseUnpatchedResources();
        return SetError(error, "persistence relay template is too large");
    }
    std::memcpy(
        PersistenceRelayPage,
        reinterpret_cast<const void*>(begin),
        templateSize);
    auto* const copied = static_cast<std::uint8_t*>(PersistenceRelayPage);
    std::memcpy(
        copied + static_cast<std::size_t>(statePointer - begin),
        &PersistenceState,
        sizeof(PersistenceState));

    PersistenceReaderRelayEntry = copied;
    PersistenceWriterRelayEntry =
        copied + static_cast<std::size_t>(writerEntry - begin);
    gISC12PersistenceReaderContinueExit =
        copied + static_cast<std::size_t>(readerContinue - begin);
    gISC12PersistenceReaderRejectedExit =
        copied + static_cast<std::size_t>(readerRejected - begin);
    gISC12PersistenceWriterVanillaExit =
        copied + static_cast<std::size_t>(writerVanilla - begin);
    gISC12PersistenceWriterCommittedExit =
        copied + static_cast<std::size_t>(writerCommitted - begin);
    gISC12PersistenceWriterRejectedExit =
        copied + static_cast<std::size_t>(writerRejected - begin);
    PersistencePlayerSaveFinalizeRelayEntry =
        copied + static_cast<std::size_t>(playerSaveFinalize - begin);

    PersistenceState->readerHandler =
        reinterpret_cast<void*>(&ISC12PersistenceReaderMidHook);
    PersistenceState->writerHandler =
        reinterpret_cast<void*>(&ISC12PersistenceWriterMidHook);
    PersistenceState->readerContinue =
        LoaderBase + PersistenceReaderContinueRva;
    PersistenceState->readerRejected =
        LoaderBase + PersistenceReaderRejectedRva;
    PersistenceState->writerVanilla =
        LoaderBase + PersistenceWriterVanillaRva;
    PersistenceState->writerCommitted =
        LoaderBase + PersistenceWriterCommittedRva;
    PersistenceState->writerRejected =
        LoaderBase + PersistenceWriterRejectedRva;

    const auto baseAddress = reinterpret_cast<std::uintptr_t>(LoaderBase);
    const auto playerSaveFinalizeRelayAddress =
        reinterpret_cast<std::uintptr_t>(
            PersistencePlayerSaveFinalizeRelayEntry);
    if (playerSaveFinalizeRelayAddress < baseAddress) {
        ReleaseUnpatchedResources();
        return SetError(
            error, "persistence finalize relay precedes the D2R image");
    }
    PreparedCodecActivationTargets =
        LoaderCodecPatchAuthority::BindPreparedRelay(
            playerSaveFinalizeRelayAddress - baseAddress);
    if (!CanEncodeRel32(
            baseAddress + PersistenceReaderPatchRva,
            reinterpret_cast<std::uintptr_t>(
                PersistenceReaderRelayEntry))
            || !CanEncodeRel32(
                baseAddress + PersistenceWriterPatchRva,
                reinterpret_cast<std::uintptr_t>(
                    PersistenceWriterRelayEntry))
            || !CanEncodeRel32(
                baseAddress + PlayerSaveFinalizeCallRva,
                reinterpret_cast<std::uintptr_t>(
                    PersistencePlayerSaveFinalizeRelayEntry))) {
        ReleaseUnpatchedResources();
        return SetError(error, "persistence relay lies outside rel32 reach");
    }

    DWORD previousProtection{};
    if (!VirtualProtect(
            PersistenceRelayPage,
            PersistenceRelayPageSize,
            PAGE_EXECUTE_READ,
            &previousProtection)) {
        ReleaseUnpatchedResources();
        return SetError(error, "persistence relay protection failed");
    }
    if (!FlushInstructionCache(
            GetCurrentProcess(),
            PersistenceRelayPage,
            templateSize)) {
        ReleaseUnpatchedResources();
        return SetError(error, "persistence relay cache flush failed");
    }
    PersistencePrepared = true;
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
    if (rowCount == 0) {
        FailClosed("ItemStatCost schema cannot be empty", rowCount);
    }

    BeginSchemaSnapshotUpdate();

    std::uint8_t* records{};
    if (!SafeRead(
            reinterpret_cast<const void*>(
                dataTablesAddress + RecordsPointerOffset),
            records)) {
        RejectSchemaSnapshotUpdate(
            SchemaError::InvalidArgument,
            "ItemStatCost record pointer is unreadable",
            rowCount);
    }
    const auto recordBytes = static_cast<std::size_t>(rowCount) * RecordStride;
    std::vector<std::uint8_t> ownedRecords;
    try {
        ownedRecords.resize(recordBytes);
    } catch (...) {
        RejectSchemaSnapshotUpdate(
            SchemaError::Allocation,
            "ItemStatCost record snapshot allocation failed",
            rowCount);
    }
    if (!records
            || !SafeCopyReadable(
                records, ownedRecords.data(), ownedRecords.size())) {
        RejectSchemaSnapshotUpdate(
            SchemaError::InvalidArgument,
            "ItemStatCost record snapshot failed",
            rowCount);
    }

    void* linker{};
    if (!SafeRead(
            reinterpret_cast<const void*>(
                dataTablesAddress + NativeItemStatCostLinkerOffset),
            linker)) {
        RejectSchemaSnapshotUpdate(
            SchemaError::InvalidArgument,
            "ItemStatCost linker pointer is unreadable",
            rowCount);
    }
    NativeSchemaCallbackFailed = false;
    NativeItemStatCostSchemaSnapshot schemaCandidate;
    const auto schemaResult = BuildNativeItemStatCostSchemaSnapshot(
        linker,
        ownedRecords,
        static_cast<std::size_t>(rowCount),
        NativeItemStatCostLinkerCallbacks{
            &GetLinkNameCountForSnapshot,
            &GetLinkNameForSnapshot,
        },
        schemaCandidate);
    const auto schemaDecision = CompleteSchemaSnapshotUpdate(
        schemaResult, std::move(schemaCandidate));
    if (schemaDecision == NativeSchemaGateDecision::FailClosed) {
        FailClosed(
            schemaResult == SchemaError::None
                ? "ItemStatCost schema diverged after publication"
                : NativeSchemaCallbackFailed
                    ? "ItemStatCost schema linker access failed"
                    : "ItemStatCost schema snapshot is invalid",
            rowCount);
    }

    std::array<DescriptionEntry, MaximumRecordCount> staged;
    std::size_t descriptionCount{};
    for (std::size_t rowIndex{};
            rowIndex < static_cast<std::size_t>(rowCount);
            ++rowIndex) {
        const auto* const record =
            ownedRecords.data() + rowIndex * RecordStride;
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

extern "C" auto ISC12PrepareNativeStoreRead(
        void* object,
        std::uint64_t announcedLength,
        std::uint32_t actualLength,
        std::uint32_t nativeStatus) noexcept -> std::uint32_t {
    static_assert(
        static_cast<std::uint32_t>(
            NativePersistenceDisposition::Vanilla) == 0);
    static_assert(
        static_cast<std::uint32_t>(
            NativePersistenceDisposition::Success) == 1);
    static_assert(
        static_cast<std::uint32_t>(
            NativePersistenceDisposition::Reject) == 2);
    static_assert(
        static_cast<std::uint32_t>(
            NativePersistenceDisposition::Fatal) == 3);

    std::array<char, MaximumNativeStoreNameLength + 1> nameStorage{};
    std::string_view storeName;
    if (!ReadNativeStoreName(object, nameStorage, storeName)) {
        if (!ClearRejectedNativeRead(object)) {
            FailClosed("native reader rejection cleanup failed", 0);
        }
        return static_cast<std::uint32_t>(
            NativePersistenceDisposition::Reject);
    }
    if (ClassifyStoreName(storeName) == StoreKind::Other) {
        return static_cast<std::uint32_t>(
            NativePersistenceDisposition::Vanilla);
    }

    std::uint32_t nativeIoSuccessCode{};
    if (!LoaderBase
            || !SafeRead(
                LoaderBase + NativeIoSuccessCodeRva,
                nativeIoSuccessCode)) {
        FailClosed("native I/O success code became unreadable", 0);
    }

    const bool codecReady = PersistenceState
        && InterlockedCompareExchange(
            &PersistenceState->codecReady, 0, 0) != 0;
    Sha256Digest schemaHash{};
    const bool schemaReady = TryGetPublishedSchemaHash(schemaHash);
    std::vector<std::uint8_t> physicalBytes;
    if (codecReady && schemaReady
            && !NativeReadMaySnapshot(
                nativeStatus,
                nativeIoSuccessCode,
                announcedLength,
                actualLength)) {
        if (!ClearRejectedNativeRead(object)) {
            FailClosed("native reader length rejection cleanup failed", 0);
        }
        return static_cast<std::uint32_t>(
            NativePersistenceDisposition::Reject);
    }
    const std::uint64_t expectedLength = actualLength;
    if (codecReady && schemaReady
            && !SnapshotNativeObjectBuffer(
                object,
                &expectedLength,
                MaximumNativePhysicalStoreLength,
                physicalBytes)) {
        if (!ClearRejectedNativeRead(object)) {
            FailClosed("native reader buffer rejection cleanup failed", 0);
        }
        return static_cast<std::uint32_t>(
            NativePersistenceDisposition::Reject);
    }

    const NativeReadRequest request{
        .storeName = storeName,
        .codecReady = codecReady,
        .schemaHash = schemaReady ? &schemaHash : nullptr,
        .readStatus = nativeStatus == nativeIoSuccessCode ? 0U : 1U,
        .announcedLength = announcedLength,
        .actualLength = actualLength,
        .physicalBytes = physicalBytes,
    };
    const auto result = AdaptNativeStoreRead(
        request,
        NativeReadCallbacks{
            .context = object,
            .replaceBuffer = &ReplaceNativeReadBuffer,
            .clearRejectedRead = &ClearRejectedNativeRead,
        });
    if (result.disposition == NativePersistenceDisposition::Fatal) {
        FailClosed("native reader adapter entered a fatal state", 0);
    }
    return static_cast<std::uint32_t>(result.disposition);
}

extern "C" auto ISC12PrepareNativeStoreWrite(
        void* object,
        const char* nativePath) noexcept -> std::uint32_t {
    std::array<char, MaximumNativeStoreNameLength + 1> nameStorage{};
    std::string_view storeName;
    if (!ReadNativeStoreName(object, nameStorage, storeName)) {
        return static_cast<std::uint32_t>(
            NativePersistenceDisposition::Reject);
    }
    if (ClassifyStoreName(storeName) == StoreKind::Other) {
        return static_cast<std::uint32_t>(
            NativePersistenceDisposition::Vanilla);
    }

    const bool codecReady = PersistenceState
        && InterlockedCompareExchange(
            &PersistenceState->codecReady, 0, 0) != 0;
    Sha256Digest schemaHash{};
    const bool schemaReady = TryGetPublishedSchemaHash(schemaHash);
    std::vector<std::uint8_t> innerBytes;
    if (codecReady && schemaReady
            && !SnapshotNativeObjectBuffer(
                object,
                nullptr,
                MaximumNativeInnerStoreLength,
                innerBytes)) {
        return static_cast<std::uint32_t>(
            NativePersistenceDisposition::Reject);
    }
    std::array<char, NativePersistencePathCapacity> pathStorage{};
    if (!nativePath
            || !SafeCopyReadable(
                nativePath, pathStorage.data(), pathStorage.size())) {
        return static_cast<std::uint32_t>(
            NativePersistenceDisposition::Reject);
    }

    const auto result = AdaptNativeStoreWrite(
        NativeWriteRequest{
            .storeName = storeName,
            .nativePathUtf8 = pathStorage,
            .codecReady = codecReady,
            .schemaHash = schemaReady ? &schemaHash : nullptr,
            .innerBytes = innerBytes,
        },
        NativeWriteCallbacks{
            .context = object,
            .atomicCommit = &CommitNativeStoreAtomically,
        });
    if (result.disposition == NativePersistenceDisposition::Fatal) {
        FailClosed("native writer adapter entered a fatal state", 0);
    }
    return static_cast<std::uint32_t>(result.disposition);
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
    PersistencePrepared = false;
    TailPatchInstalled = false;
    CapPatchInstalled = false;
    ColdRestartRequired = false;
    LoaderContext = context;
    LoaderBase = base;
    LoaderImageSize = imageSize;
    BuildCalls.store(0, std::memory_order_release);
    LastRowCount.store(0, std::memory_order_release);
    LastDescriptionCount.store(0, std::memory_order_release);
    ResetPublishedSchemaSnapshot();

    if (!ValidateImageTarget(TailPatchRva, TailExpected.size(), true)
            || !ValidateImageTarget(
                CountImmediateRva, CountImmediateExpected.size(), true)
            || !ValidateImageTarget(NativeQsortRva, 1, true)
            || !ValidateImageTarget(NativeComparatorRva, 1, true)
            || !ValidateImageTarget(NativeVectorResizeRva, 1, true)
            || !ValidateImageTarget(NativeGetLinkNameCountRva, 1, true)
            || !ValidateImageTarget(NativeGetLinkNameRva, 1, true)
            || !ValidateImageTarget(
                PersistenceReaderPatchRva, 5, true)
            || !ValidateImageTarget(
                PersistenceReaderContinueRva, 1, true)
            || !ValidateImageTarget(
                PersistenceReaderRejectedRva, 1, true)
            || !ValidateImageTarget(
                PersistenceWriterPatchRva, 5, true)
            || !ValidateImageTarget(
                PersistenceWriterVanillaRva, 1, true)
            || !ValidateImageTarget(
                PersistenceWriterCommittedRva, 1, true)
            || !ValidateImageTarget(
                PersistenceWriterRejectedRva, 1, true)
            || !ValidateImageTarget(
                PlayerSaveFinalizeCallRva, 5, true)
            || !ValidateImageTarget(
                NativeByteBufferResizeRva, 1, true)
            || !ValidateImageTarget(
                NativeSetObjectStateRva, 1, true)
            || !ValidateImageTarget(
                NativeSetObjectAuxRva, 1, true)
            || !ValidateImageTarget(
                NativeIoSuccessCodeRva,
                sizeof(std::uint32_t),
                false)
            || !ValidateImageTarget(VanillaContinuationRva, 1, true)
            || !ValidateImageTarget(EpilogueRva, 1, true)) {
        return SetError(error, "loader extension target lies outside executable image");
    }

    NativeQsort = reinterpret_cast<NativeQsortFn>(LoaderBase + NativeQsortRva);
    NativeComparator = reinterpret_cast<NativeComparatorFn>(
        LoaderBase + NativeComparatorRva);
    NativeVectorResize = reinterpret_cast<NativeVectorResizeFn>(
        LoaderBase + NativeVectorResizeRva);
    NativeGetLinkNameCount = reinterpret_cast<NativeGetLinkNameCountFn>(
        LoaderBase + NativeGetLinkNameCountRva);
    NativeGetLinkName = reinterpret_cast<NativeGetLinkNameFn>(
        LoaderBase + NativeGetLinkNameRva);
    NativeByteBufferResize = reinterpret_cast<NativeByteBufferResizeFn>(
        LoaderBase + NativeByteBufferResizeRva);
    NativeSetObjectState = reinterpret_cast<NativeSetObjectStateFn>(
        LoaderBase + NativeSetObjectStateRva);
    NativeSetObjectAux = reinterpret_cast<NativeSetObjectAuxFn>(
        LoaderBase + NativeSetObjectAuxRva);
    if (!PrepareRelay(error)) return false;
    if (!PreparePersistenceRelay(error)) return false;

    Prepared = true;
    return true;
}

auto InstallLoaderExtension(std::string& error) noexcept
        -> LoaderInstallResult {
    error.clear();
    if (!Prepared || !PersistencePrepared
            || !LoaderContext || !LoaderBase || !RelayPage || !State
            || !PersistenceRelayPage || !PersistenceState
            || !PersistenceReaderRelayEntry
            || !PersistenceWriterRelayEntry
            || !PersistencePlayerSaveFinalizeRelayEntry
            || PreparedCodecActivationTargets.PlayerSaveFinalizeRelayRva()
                == 0) {
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
    if (PersistenceState) {
        InterlockedExchange(&PersistenceState->codecReady, 0);
        InterlockedExchange(&PersistenceState->operational, 0);
        const auto deadline = GetTickCount64()
            + ShutdownRundownTimeoutMilliseconds;
        while (InterlockedCompareExchange(
                &PersistenceState->activeCallbacks, 0, 0) != 0) {
            if (GetTickCount64() >= deadline) {
                FailClosed(
                    "persistence relay rundown timed out during unload",
                    LastRowCount.load(std::memory_order_acquire));
            }
            YieldProcessor();
        }
        PersistenceState->readerHandler = nullptr;
        PersistenceState->writerHandler = nullptr;
    }
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
    PersistencePrepared = false;
    ResetPublishedSchemaSnapshot();
    if (!AnyMutationInstalled) {
        ReleaseUnpatchedResources();
        LoaderContext = nullptr;
        LoaderBase = nullptr;
        LoaderImageSize = 0;
        NativeQsort = nullptr;
        NativeComparator = nullptr;
        NativeVectorResize = nullptr;
        NativeGetLinkNameCount = nullptr;
        NativeGetLinkName = nullptr;
        NativeByteBufferResize = nullptr;
        NativeSetObjectState = nullptr;
        NativeSetObjectAux = nullptr;
        gISC12LoaderSuccessExit = nullptr;
        gISC12LoaderVanillaExit = nullptr;
    }
    // Once any native patch call is attempted, the RX relay and RW guard state
    // remain process-lifetime. A false PluginSDK result cannot prove that the
    // target bytes stayed untouched. The inactive relay never enters the
    // unloaded DLL.
}

auto TryGetPublishedSchemaHash(Sha256Digest& output) noexcept -> bool {
    Sha256Digest staged{};
    bool available{};
    AcquireSRWLockShared(&SchemaSnapshotLock);
    if (SchemaReady.load(std::memory_order_acquire)
            && HasPublishedSchemaSnapshot
            && !SchemaUpdateInProgress) {
        staged = PublishedSchemaSnapshot.schemaHash;
        available = SchemaReady.load(std::memory_order_acquire)
            && !SchemaUpdateInProgress;
    }
    ReleaseSRWLockShared(&SchemaSnapshotLock);
    if (!available) return false;
    output = staged;
    return true;
}

auto GetLoaderRuntimeStatus() noexcept -> LoaderRuntimeStatus {
    LoaderRuntimeStatus status{
        .prepared = Prepared,
        .persistencePrepared = PersistencePrepared,
        .tailPatchInstalled = TailPatchInstalled,
        .capPatchInstalled = CapPatchInstalled,
        .operational = State
            && InterlockedCompareExchange(&State->operational, 0, 0) != 0,
        .schemaReady = SchemaReady.load(std::memory_order_acquire),
        .persistenceCodecReady = PersistenceState
            && InterlockedCompareExchange(
                &PersistenceState->codecReady, 0, 0) != 0,
        .coldRestartRequired = ColdRestartRequired,
        .buildCalls = BuildCalls.load(std::memory_order_acquire),
        .lastRowCount = LastRowCount.load(std::memory_order_acquire),
        .lastDescriptionCount =
            LastDescriptionCount.load(std::memory_order_acquire),
    };
    return status;
}

} // namespace ruffneckk::isc12
