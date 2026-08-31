#define NOMINMAX
#include <D2RLPlugin/api.h>
#include <D2RLPlugin/diagnostics.h>
#include <D2RLPlugin/input.h>
#include <D2RLPlugin/threads.h>

#include "autosort_config.hpp"
#include "autosort_planner.hpp"

#include <Windows.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {
using namespace ruffneckk::autosort;

constexpr wchar_t ConfigFileName[] = L"ruffneckk-autosort.toml";
constexpr std::int32_t InventoryInterfaceState = 1;
constexpr std::int32_t StashInterfaceState = 0x18;
constexpr std::uint8_t InventoryPage = 0;
constexpr std::uint8_t StashPage = 4;
constexpr std::int32_t ItemUnitType = 4;
constexpr std::size_t MaximumAutoSortItems = 256;
constexpr std::uintptr_t ItemTypesRecordsOffset = 0x1348;
constexpr std::uintptr_t ItemTypesCountOffset = 0x1350;
constexpr std::size_t ItemTypeRecordStride = 0xE8;

constexpr std::uintptr_t PlannerRva = 0x15E790;
constexpr std::uintptr_t AutoSortWrapperRva = 0x160B00;
constexpr std::uintptr_t PlannerCallSiteRva = 0x160C4A;
constexpr std::uintptr_t PacketSenderRva = 0x0ECC70;
constexpr std::uintptr_t ServerCallbackRva = 0x4AE440;
constexpr std::uintptr_t GetLocalDataContextRva = 0x08B2D0;
constexpr std::uintptr_t GetLocalPlayerRva = 0x09A480;
constexpr std::uintptr_t IsUiStateOpenRva = 0x0CE500;
constexpr std::uintptr_t GetUnitByIdAndTypeRva = 0x09A5D0;
constexpr std::uintptr_t GetUnitIdRva = 0x34A330;
constexpr std::uintptr_t GetItemDataContextRva = 0x34A0E0;
constexpr std::uintptr_t IsOnlineStateRva = 0x08D3B0;
constexpr std::uintptr_t GetInventoryGridRva = 0x34A410;
constexpr std::uintptr_t BuildGridContextRva = 0x3C6D80;
constexpr std::uintptr_t GetItemDimensionsRva = 0x371850;
constexpr std::uintptr_t GetItemCodeRva = 0x36EF50;
constexpr std::uintptr_t GetItemQualityRva = 0x36CF60;
constexpr std::uintptr_t CheckItemTypeRva = 0x373890;
constexpr std::uintptr_t GetDataTablesRva = 0x300A90;

constexpr std::array<std::uint8_t, 32> PlannerExpected{
    0x48,0x89,0x5C,0x24,0x20,0x55,0x56,0x57,
    0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57,
    0x48,0x8D,0xAC,0x24,0x60,0xFD,0xFF,0xFF,
    0x48,0x81,0xEC,0xA0,0x03,0x00,0x00,0x48,
};
constexpr std::array<std::uint8_t, 32> AutoSortWrapperExpected{
    0x40,0x55,0x56,0x41,0x54,0x41,0x55,0x48,
    0x8D,0xAC,0x24,0x58,0xEC,0xFF,0xFF,0xB8,
    0xA8,0x14,0x00,0x00,0xE8,0xC7,0x05,0x17,
    0x01,0x48,0x2B,0xE0,0x48,0x8B,0x05,0xA5,
};
constexpr std::array<std::uint8_t, 29> PlannerCallSiteExpected{
    0x4C,0x89,0x64,0x24,0x20,0x48,0x8D,0x95,
    0x80,0x07,0x00,0x00,0x48,0x8D,0x8D,0x80,
    0x0F,0x00,0x00,0xE8,0x2E,0xDB,0xFF,0xFF,
    0x84,0xC0,0x0F,0x84,0xDB,
};
constexpr std::array<std::uint8_t, 32> PacketSenderExpected{
    0x48,0x81,0xEC,0x48,0x05,0x00,0x00,0x48,
    0x8B,0x05,0x4A,0xE6,0x8D,0x02,0x48,0x33,
    0xC4,0x48,0x89,0x84,0x24,0x30,0x05,0x00,
    0x00,0x4C,0x8B,0x94,0x24,0x78,0x05,0x00,
};
constexpr std::array<std::uint8_t, 32> ServerCallbackExpected{
    0x40,0x55,0x53,0x57,0x48,0x8D,0xAC,0x24,
    0xA0,0xD3,0xFF,0xFF,0xB8,0x60,0x2D,0x00,
    0x00,0xE8,0x8A,0x2C,0xE2,0x00,0x48,0x2B,
    0xE0,0x48,0x8B,0x05,0x68,0xCE,0x51,0x02,
};
constexpr std::array<std::uint8_t, 32> GetLocalDataContextExpected{
    0x8B,0x05,0x2E,0x84,0x99,0x02,0xC3,0xCC,
    0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,
    0x8B,0x05,0x76,0x84,0x99,0x02,0xC3,0xCC,
    0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,
};
constexpr std::array<std::uint8_t, 32> GetLocalPlayerExpected{
    0x48,0x89,0x5C,0x24,0x08,0x57,0x48,0x83,
    0xEC,0x20,0x83,0xF9,0x08,0x0F,0x83,0x85,
    0x00,0x00,0x00,0x8B,0xD9,0x48,0x89,0x5C,
    0x24,0x38,0x48,0x83,0xFB,0x08,0x72,0x19,
};
constexpr std::array<std::uint8_t, 15> IsUiStateOpenExpected{
    0x48,0x63,0xC1,0x48,0x8D,0x0D,0x96,0xC8,
    0x95,0x02,0x0F,0xB6,0x04,0x08,0xC3,
};
constexpr std::array<std::uint8_t, 32> GetUnitByIdAndTypeExpected{
    0x4C,0x63,0xCA,0x48,0x8D,0x05,0x36,0x93,
    0x98,0x02,0x8B,0xD1,0x44,0x8B,0xC1,0x49,
    0x8B,0xC9,0x83,0xE2,0x7F,0x48,0xC1,0xE1,
    0x0A,0x48,0x03,0xC8,0xE9,0x7F,0x4C,0x00,
};
constexpr std::array<std::uint8_t, 32> GetUnitIdExpected{
    0x48,0x83,0xEC,0x28,0x48,0x85,0xC9,0x75,
    0x1D,0x88,0x4C,0x24,0x30,0x48,0x8D,0x4C,
    0x24,0x30,0xE8,0x39,0xCA,0xFF,0xFF,0x84,
    0xC0,0x74,0x01,0xCC,0xB8,0xFF,0xFF,0xFF,
};
constexpr std::array<std::uint8_t, 24> GetItemDataContextExpected{
    0x48,0x83,0xEC,0x28,0x48,0x85,0xC9,0x75,
    0x1A,0x88,0x4C,0x24,0x30,0x48,0x8D,0x4C,
    0x24,0x30,0xE8,0x49,0xC7,0xFF,0xFF,0x84,
};
constexpr std::array<std::uint8_t, 21> IsOnlineStateExpected{
    0x48,0x8B,0x05,0x29,0x63,0x99,0x02,0x48,
    0x85,0xC0,0x74,0x08,0x80,0x78,0x5C,0x01,
    0x0F,0x94,0xC0,0xC3,0xC3,
};
constexpr std::array<std::uint8_t, 32> GetInventoryGridExpected{
    0x48,0x89,0x5C,0x24,0x10,0x48,0x89,0x6C,
    0x24,0x18,0x48,0x89,0x74,0x24,0x20,0x57,
    0x48,0x83,0xEC,0x20,0x0F,0xB6,0xEA,0x41,
    0x8B,0xF0,0x48,0x8B,0xF9,0xBB,0xFF,0xFF,
};
constexpr std::array<std::uint8_t, 32> BuildGridContextExpected{
    0x48,0x89,0x5C,0x24,0x08,0x48,0x89,0x6C,
    0x24,0x10,0x48,0x89,0x74,0x24,0x18,0x57,
    0x48,0x83,0xEC,0x30,0x49,0x8B,0xE9,0x41,
    0x8B,0xD8,0x8B,0xFA,0xE8,0xEF,0x9C,0xF3,
};
constexpr std::array<std::uint8_t, 32> GetItemDimensionsExpected{
    0x48,0x89,0x74,0x24,0x18,0x48,0x89,0x7C,
    0x24,0x20,0x41,0x56,0x48,0x83,0xEC,0x20,
    0x49,0x8B,0xF0,0x4C,0x8B,0xF2,0x48,0x8B,
    0xF9,0x48,0x85,0xC9,0x74,0x0A,0xE8,0x5D,
};
constexpr std::array<std::uint8_t, 32> GetItemCodeExpected{
    0x48,0x89,0x5C,0x24,0x10,0x57,0x48,0x83,
    0xEC,0x20,0x48,0x8B,0xF9,0x48,0x85,0xC9,
    0x75,0x13,0x88,0x4C,0x24,0x30,0x48,0x8D,
    0x4C,0x24,0x30,0xE8,0x80,0x83,0xFF,0xFF,
};
constexpr std::array<std::uint8_t, 32> GetItemQualityExpected{
    0x40,0x53,0x48,0x83,0xEC,0x20,0x48,0x8B,
    0xD9,0x48,0x85,0xC9,0x74,0x0A,0xE8,0x5D,
    0xEA,0xFD,0xFF,0x83,0xF8,0x04,0x74,0x19,
    0x48,0x8D,0x4C,0x24,0x30,0xC6,0x44,0x24,
};
constexpr std::array<std::uint8_t, 24> CheckItemTypeExpected{
    0x48,0x89,0x5C,0x24,0x10,0x48,0x89,0x6C,
    0x24,0x18,0x48,0x89,0x74,0x24,0x20,0x57,
    0x41,0x56,0x41,0x57,0x48,0x83,0xEC,0x20,
};
constexpr std::array<std::uint8_t, 24> GetDataTablesExpected{
    0x48,0x83,0xEC,0x28,0x0F,0xB6,0xC1,0x48,
    0x89,0x44,0x24,0x38,0x48,0x83,0xF8,0x04,
    0x72,0x19,0x48,0x8D,0x44,0x24,0x38,0x48,
};

using PlannerFn = bool(__fastcall*)(
    std::uint32_t*,
    std::uint32_t*,
    const std::uint32_t*,
    std::size_t,
    void*,
    std::uint8_t) noexcept;
using AutoSortWrapperFn = void(__fastcall*)(
    std::uint32_t, std::uint8_t) noexcept;
using GetLocalDataContextFn = std::int32_t(__fastcall*)() noexcept;
using GetLocalPlayerFn = void*(__fastcall*)(std::int32_t) noexcept;
using IsUiStateOpenFn = std::int32_t(__fastcall*)(std::int32_t) noexcept;
using GetUnitByIdAndTypeFn = void*(__fastcall*)(
    std::uint32_t, std::int32_t) noexcept;
using GetUnitIdFn = std::int32_t(__fastcall*)(void*) noexcept;
using GetItemDataContextFn = std::uint8_t(__fastcall*)(const void*) noexcept;
using IsOnlineStateFn = bool(__fastcall*)() noexcept;
using GetInventoryGridFn = std::int32_t(__fastcall*)(
    void*, std::uint8_t, bool) noexcept;
using BuildGridContextFn = void(__fastcall*)(
    std::uint8_t, std::int32_t, std::int32_t, void*) noexcept;
using GetItemDimensionsFn = void(__fastcall*)(
    void*, std::uint8_t*, std::uint8_t*, const char*, std::uint32_t) noexcept;
using GetItemCodeFn = std::uint32_t(__fastcall*)(void*) noexcept;
using GetItemQualityFn = std::int32_t(__fastcall*)(void*) noexcept;
using CheckItemTypeFn = std::int32_t(__fastcall*)(
    const void*, std::int32_t) noexcept;
using GetDataTablesFn = std::uint8_t*(__fastcall*)(std::uint8_t) noexcept;

struct ResolvedType {
    std::uint32_t code{};
    std::int32_t id{-1};
    bool requiredByCustomRule{};
};

struct ResolvedTypeCache {
    const void* dataTables{};
    const void* records{};
    std::uint64_t recordCount{};
    bool initialized{};
    bool valid{};
    std::vector<ResolvedType> types;
};

const D2RL::PluginContext* Context{};
std::uint8_t* Base{};
Config Settings{};
std::string LoadedConfigPath{"embedded defaults"};
const D2RL::DiagnosticsServiceV1* DiagnosticsService{};
const D2RL::InputServiceV1* InputService{};
const D2RL::ThreadServiceV1* ThreadService{};
D2RL::Input::ActionHandle AutoSortAction{D2RL::Input::InvalidHandle};

PlannerFn OriginalPlanner{};
AutoSortWrapperFn AutoSortWrapper{};
GetLocalDataContextFn GetLocalDataContext{};
GetLocalPlayerFn GetLocalPlayer{};
IsUiStateOpenFn IsUiStateOpen{};
GetUnitByIdAndTypeFn GetUnitByIdAndType{};
GetUnitIdFn GetUnitId{};
GetItemDataContextFn GetItemDataContext{};
IsOnlineStateFn IsOnlineState{};
GetInventoryGridFn GetInventoryGrid{};
BuildGridContextFn BuildGridContext{};
GetItemDimensionsFn GetItemDimensions{};
GetItemCodeFn GetItemCode{};
GetItemQualityFn GetItemQuality{};
CheckItemTypeFn CheckItemType{};
GetDataTablesFn GetDataTables{};

std::atomic_bool Operational{};
std::atomic_bool Stopping{};
std::atomic_bool UiWorkPending{};
std::atomic_bool TypeResolutionErrorLogged{};
std::atomic<std::uint64_t> ControlsRequests{};
std::atomic<std::uint64_t> ControllerPlans{};
std::atomic<std::uint64_t> ControlsPlans{};
std::atomic<std::uint64_t> InspectionPlans{};
std::atomic<std::uint64_t> PlansAccepted{};
std::atomic<std::uint64_t> PlansRefused{};
std::atomic<std::uint64_t> ItemsMoved{};
std::atomic<std::uint64_t> NoChangePlans{};
std::atomic<std::uint64_t> PackingFallbackPlans{};
std::atomic<std::uint32_t> PlanFailureDiagnosticsSeen{};
thread_local bool ForcedControlsPlan{};
thread_local bool InspectionOnlyPlan{};
thread_local ResolvedTypeCache TypeCache{};

enum class PlanFailureDiagnostic : std::uint32_t {
    InvalidInput = 0,
    GridDimensions,
    ItemLookup,
    TypeResolution,
    ItemDimensions,
    CurrentPosition,
    Packing,
    PlacementLookup,
};

void LogPlanFailureOnce(
        PlanFailureDiagnostic diagnostic,
        const char* reason) noexcept {
    const auto bit = std::uint32_t{1}
        << static_cast<std::uint32_t>(diagnostic);
    if ((PlanFailureDiagnosticsSeen.fetch_or(
            bit, std::memory_order_acq_rel) & bit) != 0
            || !Context) {
        return;
    }
    char message[384]{};
    std::snprintf(
        message,
        sizeof(message),
        "AutoSort: custom plan refused before any move: %s",
        reason);
    Context->LogWarn(message);
}

constexpr D2RL::PluginInfo Info{
    .infoSize = D2RL::PluginInfoSize,
    .apiVersion = D2RL_PLUGIN_API_VERSION,
    .id = "autosort",
    .name = "AutoSort",
    .version = "0.1.1",
    .author = "RuffnecKk",
    .description = "Sorts the visible inventory or stash tab into configurable compact groups.",
    .flags = D2RL::PluginFlags::Shared | D2RL::PluginFlags::NativeHooks,
};

template<class T>
T At(std::uintptr_t rva) noexcept {
    return reinterpret_cast<T>(Base + rva);
}

template<std::size_t Size>
bool RawMatches(
        std::uintptr_t rva,
        const std::array<std::uint8_t, Size>& expected) noexcept {
    return Base
        && std::memcmp(Base + rva, expected.data(), expected.size()) == 0;
}

template<std::size_t Size>
bool Matches(
        std::uintptr_t rva,
        const std::array<std::uint8_t, Size>& expected) noexcept {
    return Context && Context->CheckExpectedBytes(
        rva,
        expected.data(),
        static_cast<std::uint32_t>(expected.size()));
}

bool IsExecutableAddress(const void* address) noexcept {
    if (!address) return false;
    MEMORY_BASIC_INFORMATION region{};
    if (VirtualQuery(address, &region, sizeof(region)) == 0
            || region.State != MEM_COMMIT) {
        return false;
    }
    const auto protection = region.Protect & 0xFF;
    return protection == PAGE_EXECUTE
        || protection == PAGE_EXECUTE_READ
        || protection == PAGE_EXECUTE_READWRITE
        || protection == PAGE_EXECUTE_WRITECOPY;
}

std::vector<std::filesystem::path> ConfigCandidates() {
    std::filesystem::path activeModConfigDirectory;
    std::filesystem::path scopeConfigDirectory;
    if (Context && Context->activeMod && Context->activeMod[0] != '\0'
            && Context->modSupportDirectory
            && Context->modSupportDirectory[0] != L'\0') {
        activeModConfigDirectory =
            std::filesystem::path(Context->modSupportDirectory) / L"config";
    }
    if (Context && Context->pluginConfigPath
            && Context->pluginConfigPath[0] != L'\0') {
        scopeConfigDirectory =
            std::filesystem::path(Context->pluginConfigPath).parent_path();
    }
    std::error_code currentPathError;
    const auto currentPath = std::filesystem::current_path(currentPathError);
    const auto globalConfigDirectory = currentPathError
        ? std::filesystem::path{}
        : currentPath / L"d2rloader" / L"config";
    return BuildConfigCandidates(
        activeModConfigDirectory,
        scopeConfigDirectory,
        globalConfigDirectory,
        ConfigFileName);
}

bool LoadConfig() noexcept {
    Settings = {};
    LoadedConfigPath = "embedded defaults";
    for (const auto& path : ConfigCandidates()) {
        std::error_code statusError;
        if (!std::filesystem::is_regular_file(path, statusError)) continue;
        try {
            std::ifstream input(path, std::ios::binary);
            if (!input.is_open()) {
                throw std::runtime_error(
                    "configuration file cannot be opened");
            }
            const std::string text{
                std::istreambuf_iterator<char>(input),
                std::istreambuf_iterator<char>()};
            Config parsed{};
            std::string error;
            if (!ParseToml(text, parsed, error)) {
                throw std::invalid_argument(error);
            }
            Settings = std::move(parsed);
            LoadedConfigPath = path.string();
            return true;
        } catch (const std::exception& exception) {
            const auto message = std::string("AutoSort: invalid ")
                + path.string() + " (" + exception.what() + ").";
            Context->LogError(message.c_str());
            return false;
        }
    }
    Context->LogWarn(
        "AutoSort: no TOML was found; embedded defaults are active.");
    return true;
}

void LogRuntimeIdentity() noexcept {
    const auto* buildName = D2RL::GetBuildName(Context);
    const auto* buildVersion = D2RL::GetBuildVersion(Context);
    char message[320]{};
    std::snprintf(
        message,
        sizeof(message),
        "AutoSort: observed D2R build-name=%s; version=%s; validating the complete native fingerprint.",
        buildName && buildName[0] != '\0' ? buildName : "<unavailable>",
        buildVersion && buildVersion[0] != '\0'
            ? buildVersion
            : "<unavailable>");
    Context->LogInfo(message);
}

bool QueryDiagnosticsService() noexcept {
    const auto result = Context->QueryService(
        D2RL::ServiceId::Diagnostics,
        D2RL::DiagnosticsServiceV1Version,
        &DiagnosticsService);
    if (result != D2RL::ServiceQueryResult::Success) {
        DiagnosticsService = nullptr;
        Context->LogWarn(
            "AutoSort: DiagnosticsService v1 is unavailable; exact entry signatures remain mandatory.");
        return true;
    }
    if (!D2RL::HasDiagnosticsServiceV1Field(
            DiagnosticsService,
            D2RL::DiagnosticsServiceV1RequiredSize)
            || DiagnosticsService->queryHookStatus == nullptr) {
        Context->LogError(
            "AutoSort: DiagnosticsService v1 returned an invalid contract.");
        DiagnosticsService = nullptr;
        return false;
    }
    return true;
}

bool QueryUnchangedHookOwner(
        std::uintptr_t rva,
        const std::uint8_t* expected,
        std::uint32_t expectedSize,
        const char* label) noexcept {
    if (!DiagnosticsService) return true;
    const D2RL::Diagnostics::HookQuery query{
        .structSize = D2RL::Diagnostics::HookQuerySize,
        .flags = 0,
        .rva = rva,
        .expected = expected,
        .expectedSize = expectedSize,
        .reserved = 0,
    };
    D2RL::Diagnostics::HookStatus status{
        .structSize = D2RL::Diagnostics::HookStatusSize,
    };
    const auto result = DiagnosticsService->queryHookStatus(
        Context, &query, &status);
    if (result == D2RL::Diagnostics::Result::Success
            && status.structSize
                >= D2RL::Diagnostics::HookStatusRequiredSize
            && status.state
                == D2RL::Diagnostics::ModificationState::Unchanged
            && status.ownerCount == 0) {
        return true;
    }
    char message[256]{};
    std::snprintf(
        message,
        sizeof(message),
        "AutoSort: %s is not an unowned vanilla entry; hook ownership refused.",
        label);
    Context->LogError(message);
    return false;
}

bool ValidateUiStateEntry() noexcept {
    if (RawMatches(IsUiStateOpenRva, IsUiStateOpenExpected)) return true;
    if (!DiagnosticsService) {
        Context->LogError(
            "AutoSort: UI_IsStateOpen differs and no tracked owner proof is available.");
        return false;
    }
    const D2RL::Diagnostics::HookQuery query{
        .structSize = D2RL::Diagnostics::HookQuerySize,
        .flags = 0,
        .rva = IsUiStateOpenRva,
        .expected = IsUiStateOpenExpected.data(),
        .expectedSize = static_cast<std::uint32_t>(
            IsUiStateOpenExpected.size()),
        .reserved = 0,
    };
    D2RL::Diagnostics::HookStatus status{
        .structSize = D2RL::Diagnostics::HookStatusSize,
    };
    const auto result = DiagnosticsService->queryHookStatus(
        Context, &query, &status);
    if (result != D2RL::Diagnostics::Result::Success
            || status.structSize
                < D2RL::Diagnostics::HookStatusRequiredSize) {
        Context->LogError(
            "AutoSort: DiagnosticsService could not validate UI_IsStateOpen.");
        return false;
    }
    const auto ownerLength = std::find(
        std::begin(status.ownerPluginId),
        std::end(status.ownerPluginId),
        '\0') - std::begin(status.ownerPluginId);
    const std::string_view owner{
        status.ownerPluginId,
        static_cast<std::size_t>(ownerLength)};
    const auto accepted =
        status.state == D2RL::Diagnostics::ModificationState::Tracked
        && status.kind
            == D2RL::Diagnostics::ModificationKind::InlineHook
        && status.ownerCount == 1
        && owner == "ruffneckk-remote-stash"
        && IsExecutableAddress(Base + IsUiStateOpenRva);
    if (!accepted) {
        Context->LogError(
            "AutoSort: UI_IsStateOpen ownership is neither vanilla nor the governed Remote Stash hook.");
    }
    return accepted;
}

bool ValidateNativeFingerprint() noexcept {
    bool valid = Base != nullptr;
    const auto check = [&valid](
            std::uintptr_t rva,
            const auto& expected,
            const char* label) noexcept {
        if (Matches(rva, expected)) return;
        valid = false;
        char message[256]{};
        std::snprintf(
            message,
            sizeof(message),
            "AutoSort: signature mismatch for %s at RVA 0x%llX.",
            label,
            static_cast<unsigned long long>(rva));
        Context->LogError(message);
    };

    check(PlannerRva, PlannerExpected, "INVENTORY_AutoSortPlanner");
    check(AutoSortWrapperRva, AutoSortWrapperExpected,
        "INVENTORY_AutoSort");
    check(PlannerCallSiteRva, PlannerCallSiteExpected,
        "INVENTORY_AutoSort planner callsite");
    check(PacketSenderRva, PacketSenderExpected,
        "CLIENT_SendAutoSortPacket");
    check(ServerCallbackRva, ServerCallbackExpected,
        "SERVER_AutoSortCallback");
    check(GetLocalDataContextRva, GetLocalDataContextExpected,
        "CLIENT_GetLocalDataContext");
    check(GetLocalPlayerRva, GetLocalPlayerExpected,
        "CLIENT_GetLocalPlayer");
    check(GetUnitByIdAndTypeRva, GetUnitByIdAndTypeExpected,
        "CLIENT_GetUnitByIdAndType");
    check(GetUnitIdRva, GetUnitIdExpected, "UNITS_GetUnitId");
    check(GetItemDataContextRva, GetItemDataContextExpected,
        "GetItemDataContext");
    check(IsOnlineStateRva, IsOnlineStateExpected,
        "CLIENT_IsOnlineState");
    check(GetInventoryGridRva, GetInventoryGridExpected,
        "UNITS_GetInventoryGrid");
    check(BuildGridContextRva, BuildGridContextExpected,
        "INVENTORY_BuildGridContext");
    check(GetItemDimensionsRva, GetItemDimensionsExpected,
        "ITEMS_GetDimensions");
    check(GetItemCodeRva, GetItemCodeExpected,
        "ITEMS_GetItemCode");
    check(GetItemQualityRva, GetItemQualityExpected,
        "ITEMS_GetItemQuality");
    check(CheckItemTypeRva, CheckItemTypeExpected,
        "ITEMS_CheckItemTypeId");
    check(GetDataTablesRva, GetDataTablesExpected,
        "GetDataTablesForContext");
    if (!ValidateUiStateEntry()) valid = false;
    if (valid && !QueryUnchangedHookOwner(
            PlannerRva,
            PlannerExpected.data(),
            static_cast<std::uint32_t>(PlannerExpected.size()),
            "INVENTORY_AutoSortPlanner")) {
        valid = false;
    }
    return valid;
}

void ResolveNativeFunctions() noexcept {
    AutoSortWrapper = At<AutoSortWrapperFn>(AutoSortWrapperRva);
    GetLocalDataContext = At<GetLocalDataContextFn>(
        GetLocalDataContextRva);
    GetLocalPlayer = At<GetLocalPlayerFn>(GetLocalPlayerRva);
    IsUiStateOpen = At<IsUiStateOpenFn>(IsUiStateOpenRva);
    GetUnitByIdAndType = At<GetUnitByIdAndTypeFn>(
        GetUnitByIdAndTypeRva);
    GetUnitId = At<GetUnitIdFn>(GetUnitIdRva);
    GetItemDataContext = At<GetItemDataContextFn>(
        GetItemDataContextRva);
    IsOnlineState = At<IsOnlineStateFn>(IsOnlineStateRva);
    GetInventoryGrid = At<GetInventoryGridFn>(GetInventoryGridRva);
    BuildGridContext = At<BuildGridContextFn>(BuildGridContextRva);
    GetItemDimensions = At<GetItemDimensionsFn>(GetItemDimensionsRva);
    GetItemCode = At<GetItemCodeFn>(GetItemCodeRva);
    GetItemQuality = At<GetItemQualityFn>(GetItemQualityRva);
    CheckItemType = At<CheckItemTypeFn>(CheckItemTypeRva);
    GetDataTables = At<GetDataTablesFn>(GetDataTablesRva);
}

const std::vector<std::uint32_t>& BuiltInTypeCodes() {
    static const std::vector<std::uint32_t> codes{
        *PackCode("ring"), *PackCode("amul"), *PackCode("jwly"),
        *PackCode("char"), *PackCode("hpot"), *PackCode("mpot"),
        *PackCode("rpot"), *PackCode("apot"), *PackCode("wpot"),
        *PackCode("tpot"), *PackCode("key"), *PackCode("ukey"),
        *PackCode("scro"), *PackCode("book"), *PackCode("gem"),
        *PackCode("rune"), *PackCode("jewl"), *PackCode("ques"),
        *PackCode("armo"), *PackCode("weap"), *PackCode("misc"),
        *PackCode("tors"), *PackCode("helm"), *PackCode("shld"),
        *PackCode("glov"), *PackCode("belt"), *PackCode("boot"),
        *PackCode("tkni"), *PackCode("taxe"), *PackCode("jave"),
        *PackCode("swor"), *PackCode("axe"), *PackCode("mace"),
        *PackCode("hamm"), *PackCode("club"), *PackCode("knif"),
        *PackCode("scep"), *PackCode("wand"), *PackCode("staf"),
        *PackCode("spea"), *PackCode("pole"), *PackCode("bow"),
        *PackCode("xbow"), *PackCode("h2h"), *PackCode("orb"),
    };
    return codes;
}

bool IsConfiguredTypeCode(std::uint32_t code) noexcept {
    const auto custom = std::any_of(
        Settings.customRules.begin(),
        Settings.customRules.end(),
        [&](const CustomRule& rule) {
            return Contains(rule.itemTypeCodes, code);
        });
    if (custom) return true;
    return std::any_of(
        Settings.exclusions.begin(),
        Settings.exclusions.end(),
        [&](const ExclusionRule& rule) {
            return Contains(rule.itemTypeCodes, code);
        });
}

std::int32_t FindItemTypeId(
        const void* records,
        std::uint64_t count,
        std::uint32_t code) noexcept {
    if (!records || count == 0 || count > 4096) return -1;
    const auto* bytes = static_cast<const std::uint8_t*>(records);
    for (std::uint64_t index = 0; index < count; ++index) {
        std::uint32_t recordCode{};
        std::memcpy(
            &recordCode,
            bytes + index * ItemTypeRecordStride,
            sizeof(recordCode));
        if (NormalizePackedCode(recordCode)
                == NormalizePackedCode(code)) {
            return static_cast<std::int32_t>(index);
        }
    }
    return -1;
}

bool RefreshTypeCache(const void* item) noexcept {
    if (!item || !GetItemDataContext || !GetDataTables) return false;
    const auto itemContext = GetItemDataContext(item);
    auto* dataTables = GetDataTables(itemContext);
    if (!dataTables) return false;
    const auto* records = *reinterpret_cast<const std::uint8_t* const*>(
        dataTables + ItemTypesRecordsOffset);
    const auto recordCount = *reinterpret_cast<const std::uint64_t*>(
        dataTables + ItemTypesCountOffset);
    if (!records || recordCount == 0 || recordCount > 4096) return false;
    if (TypeCache.initialized
            && TypeCache.dataTables == dataTables
            && TypeCache.records == records
            && TypeCache.recordCount == recordCount) {
        return TypeCache.valid;
    }

    TypeCache = {};
    TypeCache.initialized = true;
    TypeCache.dataTables = dataTables;
    TypeCache.records = records;
    TypeCache.recordCount = recordCount;
    std::vector<std::uint32_t> universe{
        BuiltInTypeCodes().begin(), BuiltInTypeCodes().end()};
    for (const auto& rule : Settings.customRules) {
        universe.insert(
            universe.end(),
            rule.itemTypeCodes.begin(),
            rule.itemTypeCodes.end());
    }
    for (const auto& rule : Settings.exclusions) {
        universe.insert(
            universe.end(),
            rule.itemTypeCodes.begin(),
            rule.itemTypeCodes.end());
    }
    std::sort(universe.begin(), universe.end());
    universe.erase(std::unique(universe.begin(), universe.end()), universe.end());
    TypeCache.types.reserve(universe.size());
    TypeCache.valid = true;
    for (const auto code : universe) {
        const auto id = FindItemTypeId(records, recordCount, code);
        const auto required = IsConfiguredTypeCode(code);
        TypeCache.types.push_back(ResolvedType{
            .code = code,
            .id = id,
            .requiredByCustomRule = required,
        });
        if (required && id < 0) TypeCache.valid = false;
    }
    if (!TypeCache.valid
            && !TypeResolutionErrorLogged.exchange(
                true, std::memory_order_acq_rel)) {
        Context->LogError(
            "AutoSort: a configured item type code does not exist in the active itemtypes table; this sort was refused.");
    }
    return TypeCache.valid;
}

bool ReadGridDimensions(
        void* player,
        std::uint8_t page,
        std::uint8_t& width,
        std::uint8_t& height) noexcept {
    if (!player) return false;
    const auto dataContext = GetItemDataContext(player);
    const auto gridId = GetInventoryGrid(
        player, page, !IsOnlineState());
    if (gridId < 0) return false;
    std::array<std::uint8_t, 24> gridContext{};
    BuildGridContext(
        dataContext, gridId, 0, gridContext.data());
    // INVENTORY_BuildGridContext copies the 24-byte Inventory.txt grid
    // descriptor whose width and height are its first two bytes. The +0x10
    // and +0x11 layout belongs to the separate resolved occupancy grid.
    width = gridContext[0];
    height = gridContext[1];
    return width > 0 && height > 0 && width <= 16 && height <= 16;
}

std::string JoinItemTypeCodes(
        const std::vector<std::uint32_t>& codes) {
    std::string result{"["};
    for (std::size_t index = 0; index < codes.size(); ++index) {
        if (index != 0) result += ',';
        result += UnpackCode(codes[index]);
    }
    result += ']';
    return result;
}

void LogItemInspection(
        std::uint32_t guid,
        std::uint8_t width,
        std::uint8_t height,
        Position position,
        const ItemTraits& traits,
        const Classification& classification) {
    if (!Context || !Settings.diagnostics
            || !Settings.diagnosticsDryRun
            || !Settings.diagnosticsLogItems) {
        return;
    }
    std::string source{"built-in"};
    std::string group{CategoryName(classification.category)};
    if (classification.exclusionRuleIndex) {
        source = "exclusion";
        group = Settings.exclusions[*classification.exclusionRuleIndex].name;
    } else if (classification.customRuleIndex) {
        source = "custom";
        group = Settings.customRules[*classification.customRuleIndex].name;
    }
    auto code = UnpackCode(traits.itemCode);
    if (code.empty()) code = "<none>";
    const auto types = JoinItemTypeCodes(traits.itemTypeCodes);
    char prefix[512]{};
    std::snprintf(
        prefix,
        sizeof(prefix),
        "AutoSort inspect item: guid=%u code=%s types=%s quality=%u size=%ux%u pos=%u,%u source=%s group=%s category=%.*s subgroup=%.*s anchor=%.*s excluded=%s.",
        guid,
        code.c_str(),
        types.c_str(),
        static_cast<unsigned>(traits.quality),
        static_cast<unsigned>(width),
        static_cast<unsigned>(height),
        static_cast<unsigned>(position.x),
        static_cast<unsigned>(position.y),
        source.c_str(),
        group.c_str(),
        static_cast<int>(CategoryName(classification.category).size()),
        CategoryName(classification.category).data(),
        static_cast<int>(SubgroupName(classification.subgroup).size()),
        SubgroupName(classification.subgroup).data(),
        static_cast<int>(AnchorName(classification.anchor).size()),
        AnchorName(classification.anchor).data(),
        classification.excluded ? "true" : "false");
    Context->LogInfo(prefix);
}

bool BuildCustomPlan(
        std::uint32_t* targetPositions,
        const std::uint32_t* currentPositions,
        const std::uint32_t* guids,
        std::size_t count,
        void* player,
        std::uint8_t page) noexcept {
    InspectionOnlyPlan = false;
    if (!targetPositions || !currentPositions || !guids || !player
            || count > MaximumAutoSortItems) {
        LogPlanFailureOnce(
            PlanFailureDiagnostic::InvalidInput,
            "invalid planner input or item count above 256.");
        return false;
    }
    std::uint8_t gridWidth{};
    std::uint8_t gridHeight{};
    if (!ReadGridDimensions(player, page, gridWidth, gridHeight)) {
        LogPlanFailureOnce(
            PlanFailureDiagnostic::GridDimensions,
            "the active inventory grid dimensions could not be resolved.");
        return false;
    }
    const auto reservedRightColumns = page == 0
        ? Settings.fixedRegions.inventoryRightColumns
        : std::uint8_t{};
    if (reservedRightColumns >= gridWidth) {
        LogPlanFailureOnce(
            PlanFailureDiagnostic::GridDimensions,
            "the configured fixed inventory columns leave no movable grid area.");
        return false;
    }
    const auto firstReservedX = static_cast<std::uint8_t>(
        gridWidth - reservedRightColumns);

    std::vector<GridItem> items;
    items.reserve(count);
    for (std::size_t index = 0; index < count; ++index) {
        auto* item = GetUnitByIdAndType(guids[index], ItemUnitType);
        if (!item) {
            LogPlanFailureOnce(
                PlanFailureDiagnostic::ItemLookup,
                "an item GUID from the native transaction could not be resolved on the client.");
            return false;
        }
        if (!RefreshTypeCache(item)) {
            LogPlanFailureOnce(
                PlanFailureDiagnostic::TypeResolution,
                "the active item type table could not be resolved.");
            return false;
        }
        std::uint8_t width{};
        std::uint8_t height{};
        GetItemDimensions(item, &width, &height, "AutoSort", 0);
        if (width == 0 || height == 0) {
            LogPlanFailureOnce(
                PlanFailureDiagnostic::ItemDimensions,
                "an item has invalid zero dimensions.");
            return false;
        }

        ItemTraits traits{
            .itemCode = NormalizePackedCode(GetItemCode(item)),
            .quality = static_cast<std::uint8_t>(std::clamp(
                GetItemQuality(item), 0, 255)),
        };
        traits.itemTypeCodes.reserve(TypeCache.types.size());
        for (const auto& type : TypeCache.types) {
            if (type.id >= 0 && CheckItemType(item, type.id) != 0) {
                traits.itemTypeCodes.push_back(type.code);
            }
        }
        auto classification = Classify(Settings, traits);
        const auto packed = currentPositions[index];
        const auto x = static_cast<std::uint16_t>(packed & 0xFFFF);
        const auto y = static_cast<std::uint16_t>(packed >> 16);
        if (x > std::numeric_limits<std::uint8_t>::max()
                || y > std::numeric_limits<std::uint8_t>::max()) {
            LogPlanFailureOnce(
                PlanFailureDiagnostic::CurrentPosition,
                "a native current position is outside the supported coordinate range.");
            return false;
        }
        const Position current{
            static_cast<std::uint8_t>(x),
            static_cast<std::uint8_t>(y),
        };
        const auto itemRight = static_cast<std::uint16_t>(current.x) + width;
        if (reservedRightColumns != 0 && itemRight > firstReservedX) {
            if (current.x < firstReservedX) {
                LogPlanFailureOnce(
                    PlanFailureDiagnostic::CurrentPosition,
                    "an item crosses the boundary of a configured fixed inventory column.");
                return false;
            }
            classification.anchor = Anchor::Ignore;
        }
        LogItemInspection(
            guids[index],
            width,
            height,
            current,
            traits,
            classification);
        items.push_back(GridItem{
            .guid = guids[index],
            .width = width,
            .height = height,
            .current = current,
            .anchor = classification.anchor,
            .groupOrder = classification.groupOrder,
            .subgroupOrder = classification.subgroupOrder,
            .itemOrder = SemanticItemSortKey(
                classification.category, traits.itemCode),
        });
    }

    if (Settings.diagnosticsDryRun) {
        InspectionOnlyPlan = true;
        InspectionPlans.fetch_add(1, std::memory_order_relaxed);
        char message[256]{};
        std::snprintf(
            message,
            sizeof(message),
            "AutoSort inspect-only: page=%u grid=%ux%u items=%llu; transaction intentionally refused and no item moved.",
            static_cast<unsigned>(page),
            static_cast<unsigned>(gridWidth),
            static_cast<unsigned>(gridHeight),
            static_cast<unsigned long long>(count));
        Context->LogInfo(message);
        return false;
    }

    const auto plannerStarted = std::chrono::steady_clock::now();
    const auto plan = BuildPlan(
        gridWidth,
        gridHeight,
        items,
        Settings.optimizeFreeSpace,
        reservedRightColumns);
    const auto plannerMicros = std::chrono::duration_cast<
        std::chrono::microseconds>(
            std::chrono::steady_clock::now() - plannerStarted).count();
    if (!plan.success || plan.placements.size() != count) {
        char reason[384]{};
        std::snprintf(
            reason,
            sizeof(reason),
            "packing failed for page=%u grid=%ux%u items=%llu planner-us=%lld reason=%s.",
            static_cast<unsigned>(page),
            static_cast<unsigned>(gridWidth),
            static_cast<unsigned>(gridHeight),
            static_cast<unsigned long long>(count),
            static_cast<long long>(plannerMicros),
            plan.error.c_str());
        LogPlanFailureOnce(PlanFailureDiagnostic::Packing, reason);
        return false;
    }
    for (std::size_t index = 0; index < count; ++index) {
        const auto placement = std::lower_bound(
            plan.placements.begin(),
            plan.placements.end(),
            guids[index],
            [](const Placement& candidate, std::uint32_t guid) {
                return candidate.guid < guid;
            });
        if (placement == plan.placements.end()
                || placement->guid != guids[index]
                || placement->to.x >= 16
                || placement->to.y >= 16) {
            LogPlanFailureOnce(
                PlanFailureDiagnostic::PlacementLookup,
                "a planned placement could not be mapped back to the native item order.");
            return false;
        }
        targetPositions[index] =
            static_cast<std::uint32_t>(placement->to.x)
            | (static_cast<std::uint32_t>(placement->to.y) << 16);
    }

    ItemsMoved.fetch_add(plan.movedItemCount, std::memory_order_relaxed);
    if (plan.usedPackingFallback) {
        PackingFallbackPlans.fetch_add(1, std::memory_order_relaxed);
    }
    if (!plan.changed) {
        NoChangePlans.fetch_add(1, std::memory_order_relaxed);
    }
    if (Settings.diagnostics) {
        const auto* packing = plan.usedPackingFallback
            ? "category-dimension-fallback"
            : "strict-hierarchy";
        char message[384]{};
        std::snprintf(
            message,
            sizeof(message),
            "AutoSort: page=%u grid=%ux%u fixed-right=%u items=%llu moved=%llu packing=%s largest-free=%ux%u@%u,%u anchor-penalty=%llu planner-us=%lld.",
            static_cast<unsigned>(page),
            static_cast<unsigned>(gridWidth),
            static_cast<unsigned>(gridHeight),
            static_cast<unsigned>(reservedRightColumns),
            static_cast<unsigned long long>(count),
            static_cast<unsigned long long>(plan.movedItemCount),
            packing,
            static_cast<unsigned>(plan.largestFreeRectangle.width),
            static_cast<unsigned>(plan.largestFreeRectangle.height),
            static_cast<unsigned>(plan.largestFreeRectangle.x),
            static_cast<unsigned>(plan.largestFreeRectangle.y),
            static_cast<unsigned long long>(plan.anchorPenalty),
            static_cast<long long>(plannerMicros));
        Context->LogInfo(message);
    }
    return true;
}

bool __fastcall HookPlanner(
        std::uint32_t* targetPositions,
        std::uint32_t* currentPositions,
        const std::uint32_t* guids,
        std::size_t count,
        void* player,
        std::uint8_t page) noexcept {
    if (!OriginalPlanner) return false;
    const auto nativeValid = OriginalPlanner(
        targetPositions,
        currentPositions,
        guids,
        count,
        player,
        page);
    if (!nativeValid || !Operational.load(std::memory_order_acquire)) {
        return nativeValid;
    }
    const auto controlsPlan = ForcedControlsPlan;
    if (!controlsPlan && !Settings.controllerActionEnabled) {
        return nativeValid;
    }
    if (controlsPlan) {
        ControlsPlans.fetch_add(1, std::memory_order_relaxed);
    } else {
        ControllerPlans.fetch_add(1, std::memory_order_relaxed);
    }
    try {
        if (!BuildCustomPlan(
                targetPositions,
                currentPositions,
                guids,
                count,
                player,
                page)) {
            if (InspectionOnlyPlan) return false;
            PlansRefused.fetch_add(1, std::memory_order_relaxed);
            return false;
        }
        PlansAccepted.fetch_add(1, std::memory_order_relaxed);
        return true;
    } catch (...) {
        PlansRefused.fetch_add(1, std::memory_order_relaxed);
        if (Context) {
            Context->LogError(
                "AutoSort: planner exception caught; the native transaction was refused before any move.");
        }
        return false;
    }
}

bool QueryInputServices() noexcept {
    if (Context->QueryService(
            D2RL::ServiceId::Input,
            D2RL::InputServiceV1Version,
            &InputService) != D2RL::ServiceQueryResult::Success
            || !D2RL::HasInputServiceV1Field(
                InputService, D2RL::InputServiceV1RequiredSize)
            || InputService->registerAction == nullptr
            || InputService->unregisterAction == nullptr) {
        Context->LogError(
            "AutoSort: D2RLoader InputService v1 is unavailable.");
        InputService = nullptr;
        return false;
    }
    if (Context->QueryService(
            D2RL::ServiceId::Thread,
            D2RL::ThreadServiceV1Version,
            &ThreadService) != D2RL::ServiceQueryResult::Success
            || !D2RL::HasThreadServiceV1Field(
                ThreadService, D2RL::ThreadServiceV1RequiredSize)
            || ThreadService->runOnUiThread == nullptr) {
        Context->LogError(
            "AutoSort: D2RLoader ThreadService v1 is unavailable.");
        ThreadService = nullptr;
        return false;
    }
    return true;
}

void ExecuteControlsSort() noexcept {
    if (!Operational.load(std::memory_order_acquire)
            || Stopping.load(std::memory_order_acquire)) {
        return;
    }
    std::optional<std::uint8_t> page;
    if (IsUiStateOpen(StashInterfaceState) != 0) {
        page = StashPage;
    } else if (IsUiStateOpen(InventoryInterfaceState) != 0) {
        page = InventoryPage;
    }
    if (!page) return;
    auto* player = GetLocalPlayer(GetLocalDataContext());
    if (!player) return;
    const auto unitId = GetUnitId(player);
    if (unitId < 0) return;
    struct ForcedPlanScope {
        ForcedPlanScope() noexcept { ForcedControlsPlan = true; }
        ~ForcedPlanScope() noexcept { ForcedControlsPlan = false; }
    } forcedPlanScope;
    AutoSortWrapper(static_cast<std::uint32_t>(unitId), *page);
}

DWORD WINAPI ReleasePinnedModule(void* parameter) noexcept {
    FreeLibraryAndExitThread(static_cast<HMODULE>(parameter), 0);
}

void ReleasePinnedModuleAfterReturn(HMODULE module) noexcept {
    if (!module) return;
    const auto thread = CreateThread(
        nullptr, 0, ReleasePinnedModule, module, 0, nullptr);
    if (thread) CloseHandle(thread);
}

void __cdecl RunSortOnUiThread(
        const D2RL::PluginContext*,
        void* userData) noexcept {
    const auto module = static_cast<HMODULE>(userData);
    UiWorkPending.store(false, std::memory_order_release);
    ExecuteControlsSort();
    ReleasePinnedModuleAfterReturn(module);
}

bool QueueControlsSort() noexcept {
    if (Stopping.load(std::memory_order_acquire)
            || !Operational.load(std::memory_order_acquire)) {
        return false;
    }
    if (UiWorkPending.exchange(true, std::memory_order_acq_rel)) {
        return true;
    }
    HMODULE module{};
    if (!GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
            reinterpret_cast<LPCWSTR>(&RunSortOnUiThread),
            &module)) {
        UiWorkPending.store(false, std::memory_order_release);
        return false;
    }
    if (ThreadService->runOnUiThread(
            Context,
            RunSortOnUiThread,
            module) != D2RL::Threads::Result::Success) {
        UiWorkPending.store(false, std::memory_order_release);
        FreeLibrary(module);
        return false;
    }
    ControlsRequests.fetch_add(1, std::memory_order_relaxed);
    return true;
}

D2RL::Input::ActionResult __cdecl OnControlsAction(
        const D2RL::PluginContext*,
        const D2RL::Input::ActionEvent* event,
        void*) noexcept {
    if (!Operational.load(std::memory_order_acquire)
            || !D2RL::Input::HasActionEventField(
                event, D2RL::Input::ActionEventRequiredSize)
            || event->action != AutoSortAction) {
        return D2RL::Input::ActionResult::Ignored;
    }
    if (event->kind == D2RL::Input::ActionEventKind::Released) {
        return D2RL::Input::ActionResult::Handled;
    }
    if (event->kind != D2RL::Input::ActionEventKind::Pressed) {
        return D2RL::Input::ActionResult::Ignored;
    }
    return QueueControlsSort()
        ? D2RL::Input::ActionResult::Handled
        : D2RL::Input::ActionResult::Ignored;
}

bool RegisterControlsAction() noexcept {
    const D2RL::Input::ActionRegistration registration{
        .structSize = D2RL::Input::ActionRegistrationSize,
        .flags = 0,
        .logicalId = "autosort",
        .displayName = "AutoSort",
        .category = "RuffnecKk Suite",
        .defaultPrimary = {
            D2RL::Input::Key::H,
            D2RL::Input::Modifier::Shift,
        },
        .defaultSecondary = {
            D2RL::Input::Key::None,
            D2RL::Input::Modifier::None,
        },
        .callback = OnControlsAction,
        .userData = nullptr,
    };
    D2RL::Input::ActionHandle action{D2RL::Input::InvalidHandle};
    const auto result = InputService->registerAction(
        Context, &registration, &action);
    if (result == D2RL::Input::Result::Success
            && action != D2RL::Input::InvalidHandle) {
        AutoSortAction = action;
        return true;
    }
    Context->LogError(
        "AutoSort: Controls action registration failed.");
    return false;
}

void UnregisterControlsAction() noexcept {
    if (InputService && Context
            && AutoSortAction != D2RL::Input::InvalidHandle) {
        (void)InputService->unregisterAction(Context, AutoSortAction);
    }
    AutoSortAction = D2RL::Input::InvalidHandle;
}

auto Status(
        D2R::Game::Client*,
        const D2RL::ConsoleCommandContext* command,
        void*) noexcept -> D2RL::ConsoleCommandResult {
    if (!command || !command->plugin) {
        return D2RL::ConsoleCommandResult::Failed;
    }
    char message[768]{};
    std::snprintf(
        message,
        sizeof(message),
        "AutoSort 0.1.1: active=%s; controller=%s; Controls=%s; requests=%llu; plans controller/Controls=%llu/%llu; inspections=%llu; accepted=%llu; refused=%llu; moved=%llu; unchanged=%llu; category-dimension-fallback=%llu; config=%s.",
        Operational.load(std::memory_order_acquire) ? "true" : "false",
        Settings.controllerActionEnabled ? "on" : "off",
        Settings.controlsActionEnabled ? "on" : "off",
        static_cast<unsigned long long>(
            ControlsRequests.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(
            ControllerPlans.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(
            ControlsPlans.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(
            InspectionPlans.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(
            PlansAccepted.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(
            PlansRefused.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(
            ItemsMoved.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(
            NoChangePlans.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(
            PackingFallbackPlans.load(std::memory_order_relaxed)),
        LoadedConfigPath.c_str());
    command->plugin->WriteConsoleMessage(message);
    return D2RL::ConsoleCommandResult::Handled;
}

void ResetRuntimeState() noexcept {
    Operational.store(false, std::memory_order_release);
    Stopping.store(false, std::memory_order_release);
    UiWorkPending.store(false, std::memory_order_release);
    TypeResolutionErrorLogged.store(false, std::memory_order_release);
    ControlsRequests.store(0, std::memory_order_relaxed);
    ControllerPlans.store(0, std::memory_order_relaxed);
    ControlsPlans.store(0, std::memory_order_relaxed);
    InspectionPlans.store(0, std::memory_order_relaxed);
    PlansAccepted.store(0, std::memory_order_relaxed);
    PlansRefused.store(0, std::memory_order_relaxed);
    ItemsMoved.store(0, std::memory_order_relaxed);
    NoChangePlans.store(0, std::memory_order_relaxed);
    PackingFallbackPlans.store(0, std::memory_order_relaxed);
    PlanFailureDiagnosticsSeen.store(0, std::memory_order_relaxed);
    InspectionOnlyPlan = false;
    TypeCache = {};
}

} // namespace

D2RL_PLUGIN_EXPORT auto D2RLoaderGetPluginInfo() noexcept
        -> const D2RL::PluginInfo* {
    return &Info;
}

D2RL_PLUGIN_EXPORT auto D2RLoaderLoadPlugin(
        const D2RL::PluginContext* context) noexcept -> bool {
    if (!D2RL::HasContext(context)
            || context->apiVersion != D2RL_PLUGIN_API_VERSION) {
        return false;
    }
    Context = context;
    Base = reinterpret_cast<std::uint8_t*>(context->exeBase);
    ResetRuntimeState();
    LogRuntimeIdentity();
    if (!LoadConfig()) return false;
    if (!Settings.enabled) {
        (void)context->RegisterConsoleCommand(
            "autosort", Status, "Show AutoSort status and counters.");
        context->LogInfo(
            "AutoSort 0.1.1 by RuffnecKk loaded disabled; vanilla AutoSort is unchanged.");
        return true;
    }
    if (Settings.button.enabled) {
        context->LogError(
            "AutoSort: button.enabled requires the later dedicated UI gate; version 0.1 refuses this unsupported setting.");
        return false;
    }
    if (!QueryDiagnosticsService() || !ValidateNativeFingerprint()) {
        context->LogError(
            "AutoSort: complete native fingerprint rejected; vanilla AutoSort remains untouched.");
        return false;
    }
    ResolveNativeFunctions();

    const auto needsPlanner = Settings.controllerActionEnabled
        || Settings.controlsActionEnabled;
    if (Settings.controlsActionEnabled) {
        if (!QueryInputServices() || !RegisterControlsAction()) return false;
    }
    if (needsPlanner && !Context->InstallInlineHook(
            PlannerRva,
            PlannerExpected.data(),
            static_cast<std::uint32_t>(PlannerExpected.size()),
            HookPlanner,
            &OriginalPlanner)) {
        UnregisterControlsAction();
        context->LogError(
            "AutoSort: planner hook installation was refused; vanilla AutoSort remains untouched.");
        return false;
    }

    Operational.store(true, std::memory_order_release);
    if (!context->RegisterConsoleCommand(
            "autosort", Status, "Show AutoSort status and counters.")) {
        context->LogWarn(
            "AutoSort: optional status command could not be registered.");
    }
    char message[640]{};
    std::snprintf(
        message,
        sizeof(message),
        "AutoSort 0.1.1 by RuffnecKk active; native planner/packet/server fingerprint accepted; controller=%s; Controls=%s (default Shift+H); scope=%s; TOML=%s.",
        Settings.controllerActionEnabled ? "custom" : "vanilla",
        Settings.controlsActionEnabled ? "registered" : "disabled",
        context->loadScope == D2RL::LoadScope::Mod ? "mod-local" : "global",
        LoadedConfigPath.c_str());
    context->LogInfo(message);
    return true;
}

D2RL_PLUGIN_EXPORT void D2RLoaderUnloadPlugin() noexcept {
    Stopping.store(true, std::memory_order_release);
    Operational.store(false, std::memory_order_release);
    UnregisterControlsAction();
    InputService = nullptr;
    ThreadService = nullptr;
    DiagnosticsService = nullptr;
    Context = nullptr;
}
