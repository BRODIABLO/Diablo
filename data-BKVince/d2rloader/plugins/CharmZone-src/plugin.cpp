#include <D2RLPlugin/api.h>

#include "charm_zone_config.hpp"
#include "charm_zone_policy.hpp"

#include <Windows.h>
#include <array>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <string_view>

namespace {
using namespace ruffneckk::charm_zone;

constexpr std::uint32_t SupportedBuild = 92777;
constexpr std::uintptr_t IsCharmUsableRva = 0x36AE00;
constexpr std::uintptr_t RenderItemIconRva = 0x15BB80;
constexpr std::uintptr_t CapturePacketStateRva = 0x382D20;
constexpr std::uintptr_t GetDimensionsRva = 0x371850;
constexpr std::uintptr_t CheckItemTypeRva = 0x373890;
constexpr std::size_t MaximumVisualItems = 128;
constexpr std::size_t MaximumInactiveCharms = 64;
constexpr char OverlayOwner[] = "CharmZone";

constexpr char DefaultToml[] = R"toml(# CharmZone configuration for BKVince.
# Changes take effect after a cold start.

[general]
enabled = true

[zone]
# Inventory grid dimensions and the fully active rectangle, in item cells.
grid_width = 11
grid_height = 8
left = 0
top = 4
width = 11
height = 4

[visual]
enabled = true
overlay_alpha = 0.45
# Base BKVince inventory cell size before the runtime UI scale is applied.
cell_size = 98.0
tooltip = "Inactive outside Charm Zone"
)toml";

constexpr std::array<std::uint8_t, 32> IsCharmUsableExpected{
    0x48, 0x89, 0x5C, 0x24, 0x08, 0x57, 0x48, 0x83,
    0xEC, 0x40, 0x48, 0x8B, 0xFA, 0x48, 0x8B, 0xD9,
    0xE8, 0x0B, 0xFF, 0xFF, 0xFF, 0x85, 0xC0, 0x74,
    0x69, 0xBA, 0x0D, 0x00, 0x00, 0x00, 0x48, 0x8B
};
constexpr std::array<std::uint8_t, 19> RenderItemIconExpected{
    0x40, 0x53, 0x55, 0x56, 0x48, 0x81, 0xEC, 0x40,
    0x01, 0x00, 0x00, 0x0F, 0x29, 0xB4, 0x24, 0x20,
    0x01, 0x00, 0x00
};
constexpr std::array<std::uint8_t, 32> CapturePacketStateExpected{
    0x48, 0x89, 0x5C, 0x24, 0x10, 0x48, 0x89, 0x74,
    0x24, 0x18, 0x57, 0x48, 0x83, 0xEC, 0x20, 0x48,
    0x8B, 0xF1, 0x48, 0x8B, 0xDA, 0x48, 0x8B, 0xCA,
    0xE8, 0x23, 0x7E, 0xFC, 0xFF, 0x48, 0x8B, 0xCB
};
constexpr std::array<std::uint8_t, 32> GetDimensionsExpected{
    0x48, 0x89, 0x74, 0x24, 0x18, 0x48, 0x89, 0x7C,
    0x24, 0x20, 0x41, 0x56, 0x48, 0x83, 0xEC, 0x20,
    0x49, 0x8B, 0xF0, 0x4C, 0x8B, 0xF2, 0x48, 0x8B,
    0xF9, 0x48, 0x85, 0xC9, 0x74, 0x0A, 0xE8, 0x5D
};
constexpr std::array<std::uint8_t, 32> CheckItemTypeExpected{
    0x48, 0x89, 0x5C, 0x24, 0x10, 0x48, 0x89, 0x6C,
    0x24, 0x18, 0x48, 0x89, 0x74, 0x24, 0x20, 0x57,
    0x41, 0x56, 0x41, 0x57, 0x48, 0x83, 0xEC, 0x20,
    0x48, 0x63, 0xF2, 0x48, 0x8B, 0xF9, 0x48, 0x85
};

struct PacketState {
    std::uint32_t mode{};
    std::uint32_t inventoryPage{};
    std::uint16_t x{};
    std::uint16_t y{};
    std::uint32_t nodePage{};
};
static_assert(sizeof(PacketState) == 16);

struct VisualItem {
    void* item{};
    std::int32_t x{};
    std::int32_t y{};
    float scale{};
    std::uint8_t width{};
    std::uint8_t height{};
};

using IsCharmUsableFn = std::int32_t(__fastcall*)(void*, void*) noexcept;
using RenderItemIconFn = void(__fastcall*)(
    void*, std::uint64_t, float, void*) noexcept;
using CapturePacketStateFn = PacketState*(__fastcall*)(
    PacketState*, void*) noexcept;
using GetDimensionsFn = void(__fastcall*)(
    void*, std::uint8_t*, std::uint8_t*, const char*, std::int32_t) noexcept;
using CheckItemTypeFn = std::int32_t(__fastcall*)(void*, std::int32_t) noexcept;
using OverlayCallback = void(__cdecl*)(
    void*, float, float, HWND) noexcept;
using RegisterOverlayFn = bool(__cdecl*)(
    const char*, OverlayCallback) noexcept;
using AddRectFn = void(__cdecl*)(
    void*, float, float, float, float,
    float, float, float, float, float) noexcept;
using AddRectFilledFn = void(__cdecl*)(
    void*, float, float, float, float,
    float, float, float, float) noexcept;
using AddTooltipFn = void(__cdecl*)(
    void*, float, float, float, float, const char*) noexcept;

const D2RL::PluginContext* Context{};
std::uint8_t* Base{};
Config Settings{};
IsCharmUsableFn OriginalIsCharmUsable{};
RenderItemIconFn OriginalRenderItemIcon{};
CapturePacketStateFn CapturePacketState{};
GetDimensionsFn GetDimensions{};
CheckItemTypeFn CheckItemType{};

std::mutex VisualMutex;
std::array<VisualItem, MaximumVisualItems> VisualItems{};
std::size_t VisualItemCount{};
std::array<std::atomic<void*>, MaximumInactiveCharms> InactiveCharms{};
std::atomic<bool> OverlayHosted{};
bool VisualHookInstalled{};
RegisterOverlayFn RegisterOverlay{};
AddRectFn AddRect{};
AddRectFilledFn AddRectFilled{};
AddTooltipFn AddTooltip{};
HANDLE OverlayStopEvent{};
HANDLE OverlayWorker{};

std::atomic<std::uint64_t> NativeCharmChecks{};
std::atomic<std::uint64_t> AllowedCharms{};
std::atomic<std::uint64_t> SuppressedCharms{};
std::atomic<std::uint64_t> ClassificationFailures{};
std::atomic<std::uint64_t> VisualHookCalls{};
std::atomic<std::uint64_t> VisualHostNotReady{};
std::atomic<std::uint64_t> VisualTrackedCandidates{};
std::atomic<std::uint64_t> VisualPlacementFailures{};
std::atomic<std::uint64_t> VisualInsideZone{};
std::atomic<std::uint64_t> CapturedVisualItems{};
std::atomic<std::uint64_t> DroppedVisualItems{};

constexpr D2RL::PluginInfo Info{
    .infoSize = D2RL::PluginInfoSize,
    .apiVersion = D2RL_PLUGIN_API_VERSION,
    .id = "charm-zone",
    .name = "CharmZone",
    .version = "0.3.1",
    .author = "RuffnecKk",
    .description = "Disables charms placed outside the configured inventory zone.",
    .flags = D2RL::PluginFlags::NativeHooks,
};

template<class T>
T At(std::uintptr_t rva) noexcept {
    return reinterpret_cast<T>(Base + rva);
}

bool LoadConfig() noexcept {
    if (!Context->EnsureConfig(DefaultToml)) return false;
    std::array<char, 16384> buffer{};
    std::uint32_t required{};
    if (!Context->ReadConfig(
            buffer.data(), static_cast<std::uint32_t>(buffer.size()), &required)) {
        return false;
    }
    try {
        Settings = ParseConfig(std::string_view(buffer.data()));
        return true;
    } catch (const std::exception& error) {
        char message[384]{};
        std::snprintf(
            message, sizeof(message),
            "CharmZone: invalid configuration: %s", error.what());
        Context->LogError(message);
        return false;
    } catch (...) {
        Context->LogError("CharmZone: invalid configuration.");
        return false;
    }
}

bool IsCharm(void* item) noexcept {
    __try {
        return item && CheckItemType(item, CharmItemTypeId) != 0;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool TryReadPlacement(void* item, ItemPlacement& output) noexcept {
    __try {
        PacketState state{};
        std::uint8_t width{};
        std::uint8_t height{};
        if (!item || CapturePacketState(&state, item) != &state) return false;
        GetDimensions(item, &width, &height, "CharmZone", 0);
        if (width == 0 || height == 0) return false;
        output = {state.inventoryPage, state.x, state.y, width, height};
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool IsTrackedInactiveCharm(void* item) noexcept {
    if (!item) return false;
    for (const auto& slot : InactiveCharms) {
        if (slot.load(std::memory_order_acquire) == item) return true;
    }
    return false;
}

void TrackInactiveCharm(void* item) noexcept {
    if (!item || IsTrackedInactiveCharm(item)) return;
    for (auto& slot : InactiveCharms) {
        void* empty{};
        if (slot.compare_exchange_strong(
                empty, item,
                std::memory_order_release,
                std::memory_order_relaxed)) {
            return;
        }
    }
}

void ForgetInactiveCharm(void* item) noexcept {
    if (!item) return;
    for (auto& slot : InactiveCharms) {
        void* expected = item;
        slot.compare_exchange_strong(
            expected, nullptr,
            std::memory_order_acq_rel,
            std::memory_order_relaxed);
    }
}

std::int32_t __fastcall HookIsCharmUsable(
    void* item,
    void* player) noexcept {
    const auto nativeResult = OriginalIsCharmUsable(item, player);
    if (nativeResult == 0) {
        ForgetInactiveCharm(item);
        return 0;
    }

    NativeCharmChecks.fetch_add(1, std::memory_order_relaxed);
    ItemPlacement placement{};
    if (!TryReadPlacement(item, placement)) {
        ClassificationFailures.fetch_add(1, std::memory_order_relaxed);
        return 0;
    }
    if (IsFullyContained(Settings.zone, placement)) {
        ForgetInactiveCharm(item);
        AllowedCharms.fetch_add(1, std::memory_order_relaxed);
        return nativeResult;
    }
    TrackInactiveCharm(item);
    SuppressedCharms.fetch_add(1, std::memory_order_relaxed);
    return 0;
}

void QueueVisualItem(const VisualItem& item) noexcept {
    std::scoped_lock lock(VisualMutex);
    for (std::size_t index = 0; index < VisualItemCount; ++index) {
        if (VisualItems[index].item == item.item) {
            VisualItems[index] = item;
            return;
        }
    }
    if (VisualItemCount >= VisualItems.size()) {
        DroppedVisualItems.fetch_add(1, std::memory_order_relaxed);
        return;
    }
    VisualItems[VisualItemCount++] = item;
    CapturedVisualItems.fetch_add(1, std::memory_order_relaxed);
}

void __fastcall HookRenderItemIcon(
    void* item,
    std::uint64_t packedScreenXY,
    float scale,
    void* renderParams) noexcept {
    OriginalRenderItemIcon(item, packedScreenXY, scale, renderParams);

    VisualHookCalls.fetch_add(1, std::memory_order_relaxed);

    if (!Settings.enabled
        || !Settings.visual.enabled) {
        return;
    }
    if (!OverlayHosted.load(std::memory_order_acquire)) {
        VisualHostNotReady.fetch_add(1, std::memory_order_relaxed);
        return;
    }
    if (!IsTrackedInactiveCharm(item)) {
        return;
    }
    VisualTrackedCandidates.fetch_add(1, std::memory_order_relaxed);
    if (!IsCharm(item)) {
        ForgetInactiveCharm(item);
        return;
    }

    ItemPlacement placement{};
    if (!TryReadPlacement(item, placement)) {
        VisualPlacementFailures.fetch_add(1, std::memory_order_relaxed);
        return;
    }
    if (IsFullyContained(Settings.zone, placement)) {
        ForgetInactiveCharm(item);
        VisualInsideZone.fetch_add(1, std::memory_order_relaxed);
        return;
    }
    QueueVisualItem({
        item,
        static_cast<std::int32_t>(static_cast<std::uint32_t>(packedScreenXY)),
        static_cast<std::int32_t>(static_cast<std::uint32_t>(packedScreenXY >> 32)),
        scale,
        placement.width,
        placement.height
    });
}

void __cdecl RenderOverlay(
    void* drawList,
    float displayWidth,
    float displayHeight,
    HWND window) noexcept {
    std::array<VisualItem, MaximumVisualItems> items{};
    std::size_t count{};
    {
        std::scoped_lock lock(VisualMutex);
        count = VisualItemCount;
        for (std::size_t index = 0; index < count; ++index) {
            items[index] = VisualItems[index];
        }
        VisualItemCount = 0;
    }

    POINT cursor{};
    const bool hasCursor = window
        && GetCursorPos(&cursor)
        && ScreenToClient(window, &cursor);
    bool tooltipDrawn{};
    for (std::size_t index = 0; index < count; ++index) {
        ScreenRect rect{};
        const auto& item = items[index];
        if (!TryMakeScreenRect(
                item.x, item.y, item.scale, item.width, item.height,
                Settings.visual.cellSize, displayHeight, rect)) {
            continue;
        }
        AddRectFilled(
            drawList, rect.left, rect.top, rect.right, rect.bottom,
            1.0f, 0.0f, 0.0f, Settings.visual.overlayAlpha);
        AddRect(
            drawList, rect.left, rect.top, rect.right, rect.bottom,
            1.0f, 0.12f, 0.12f, 0.95f, 2.0f);
        if (!tooltipDrawn
            && hasCursor
            && ContainsPoint(
                rect, static_cast<float>(cursor.x), static_cast<float>(cursor.y))) {
            AddTooltip(
                drawList,
                static_cast<float>(cursor.x),
                static_cast<float>(cursor.y),
                displayWidth,
                displayHeight,
                Settings.visual.tooltip.c_str());
            tooltipDrawn = true;
        }
    }
}

bool RegisterWithOverlayHost() noexcept {
    const auto host = GetModuleHandleW(L"FloatingDamage.dll");
    if (!host) return false;
    const auto registerOverlay = reinterpret_cast<RegisterOverlayFn>(
        GetProcAddress(host, "FloatingDamageRegisterNamedExternalOverlay"));
    const auto addRect = reinterpret_cast<AddRectFn>(
        GetProcAddress(host, "FloatingDamageOverlayAddRect"));
    const auto addRectFilled = reinterpret_cast<AddRectFilledFn>(
        GetProcAddress(host, "FloatingDamageOverlayAddRectFilled"));
    const auto addTooltip = reinterpret_cast<AddTooltipFn>(
        GetProcAddress(host, "FloatingDamageOverlayAddTooltip"));
    if (!registerOverlay || !addRect || !addRectFilled || !addTooltip) return false;

    AddRect = addRect;
    AddRectFilled = addRectFilled;
    AddTooltip = addTooltip;
    RegisterOverlay = registerOverlay;
    if (!RegisterOverlay(OverlayOwner, RenderOverlay)) return false;
    OverlayHosted.store(true, std::memory_order_release);
    return true;
}

DWORD WINAPI OverlayDiscoveryWorker(void*) noexcept {
    constexpr DWORD DiscoveryIntervalMs = 50;
    constexpr DWORD InitialAttempts = 60;
    for (DWORD attempt = 0; attempt < InitialAttempts; ++attempt) {
        if (WaitForSingleObject(OverlayStopEvent, DiscoveryIntervalMs) != WAIT_TIMEOUT) {
            return 0;
        }
        if (RegisterWithOverlayHost()) {
            Context->LogInfo("CharmZone: red overlay registered with FloatingDamage 1.1.0.");
            return 0;
        }
    }
    Context->LogWarn(
        "CharmZone: FloatingDamage 1.1.0 overlay host is not ready; native charm eligibility enforcement remains active.");
    while (WaitForSingleObject(OverlayStopEvent, 250) == WAIT_TIMEOUT) {
        if (RegisterWithOverlayHost()) {
            Context->LogInfo("CharmZone: delayed red overlay registration succeeded.");
            return 0;
        }
    }
    return 0;
}

bool StartOverlayDiscovery() noexcept {
    OverlayStopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!OverlayStopEvent) return false;
    OverlayWorker = CreateThread(
        nullptr, 0, OverlayDiscoveryWorker, nullptr, 0, nullptr);
    if (OverlayWorker) return true;
    CloseHandle(OverlayStopEvent);
    OverlayStopEvent = nullptr;
    return false;
}

void StopOverlayDiscovery() noexcept {
    if (OverlayStopEvent) SetEvent(OverlayStopEvent);
    if (OverlayWorker) {
        WaitForSingleObject(OverlayWorker, 3000);
        CloseHandle(OverlayWorker);
        OverlayWorker = nullptr;
    }
    if (OverlayStopEvent) {
        CloseHandle(OverlayStopEvent);
        OverlayStopEvent = nullptr;
    }
    if (OverlayHosted.exchange(false, std::memory_order_acq_rel)) {
        const auto host = GetModuleHandleW(L"FloatingDamage.dll");
        const auto unregisterOverlay = host
            ? reinterpret_cast<RegisterOverlayFn>(GetProcAddress(
                host, "FloatingDamageRegisterNamedExternalOverlay"))
            : nullptr;
        if (unregisterOverlay) unregisterOverlay(OverlayOwner, nullptr);
    }
    std::scoped_lock lock(VisualMutex);
    VisualItemCount = 0;
}

bool ValidateRuntime() noexcept {
    const auto check = [](std::uintptr_t rva, const auto& expected) noexcept {
        return Context->CheckExpectedBytes(
            rva,
            expected.data(),
            static_cast<std::uint32_t>(expected.size()));
    };
    return check(IsCharmUsableRva, IsCharmUsableExpected)
        && check(CapturePacketStateRva, CapturePacketStateExpected)
        && check(GetDimensionsRva, GetDimensionsExpected)
        && check(CheckItemTypeRva, CheckItemTypeExpected)
        && (!Settings.visual.enabled
            || check(RenderItemIconRva, RenderItemIconExpected));
}

bool InstallHooks() noexcept {
    if (!Context->InstallInlineHook(
            IsCharmUsableRva,
            IsCharmUsableExpected.data(),
            static_cast<std::uint32_t>(IsCharmUsableExpected.size()),
            HookIsCharmUsable,
            &OriginalIsCharmUsable)) {
        Context->LogError(
            "CharmZone: native charm-eligibility hook was refused; no gameplay enforcement was installed.");
        return false;
    }
    if (Settings.visual.enabled
        && !Context->InstallInlineHook(
            RenderItemIconRva,
            RenderItemIconExpected.data(),
            static_cast<std::uint32_t>(RenderItemIconExpected.size()),
            HookRenderItemIcon,
            &OriginalRenderItemIcon)) {
        Context->LogWarn(
            "CharmZone: item-render hook was refused; gameplay enforcement remains active without the red overlay.");
        return true;
    }
    VisualHookInstalled = Settings.visual.enabled;
    return true;
}

auto Status(
    D2R::Game::Client*,
    const D2RL::ConsoleCommandContext* command,
    void*) noexcept -> D2RL::ConsoleCommandResult {
    if (!command || !command->plugin) return D2RL::ConsoleCommandResult::Failed;
    char message[512]{};
    std::snprintf(
        message,
        sizeof(message),
        "CharmZone 0.3.1: enabled=%s; zone=(%u,%u %ux%u) grid=%ux%u; overlay=%s; native checks=%llu; allowed=%llu; suppressed=%llu; classification failures=%llu; visual calls=%llu; tracked=%llu; host waits=%llu; placement failures=%llu; inside=%llu; captures=%llu; drops=%llu.",
        Settings.enabled ? "true" : "false",
        Settings.zone.left,
        Settings.zone.top,
        Settings.zone.width,
        Settings.zone.height,
        Settings.zone.gridWidth,
        Settings.zone.gridHeight,
        OverlayHosted.load(std::memory_order_acquire) ? "ready" : "waiting",
        static_cast<unsigned long long>(NativeCharmChecks.load()),
        static_cast<unsigned long long>(AllowedCharms.load()),
        static_cast<unsigned long long>(SuppressedCharms.load()),
        static_cast<unsigned long long>(ClassificationFailures.load()),
        static_cast<unsigned long long>(VisualHookCalls.load()),
        static_cast<unsigned long long>(VisualTrackedCandidates.load()),
        static_cast<unsigned long long>(VisualHostNotReady.load()),
        static_cast<unsigned long long>(VisualPlacementFailures.load()),
        static_cast<unsigned long long>(VisualInsideZone.load()),
        static_cast<unsigned long long>(CapturedVisualItems.load()),
        static_cast<unsigned long long>(DroppedVisualItems.load()));
    command->plugin->WriteConsoleMessage(message);
    return D2RL::ConsoleCommandResult::Handled;
}
} // namespace

D2RL_PLUGIN_EXPORT auto D2RLoaderGetPluginInfo() noexcept -> const D2RL::PluginInfo* {
    return &Info;
}

D2RL_PLUGIN_EXPORT auto D2RLoaderLoadPlugin(
    const D2RL::PluginContext* context) noexcept -> bool {
    if (!context) return false;
    Context = context;
    Base = reinterpret_cast<std::uint8_t*>(GetModuleHandleW(nullptr));
    if (!Base || !LoadConfig()) {
        context->LogError("CharmZone: configuration could not be created or read.");
        return false;
    }
    if (context->modDataVersionBuild != 0
        && context->modDataVersionBuild != SupportedBuild) {
        context->LogError("CharmZone: only D2R build 92777 is supported.");
        return false;
    }
    if (!Settings.enabled) {
        context->LogInfo("CharmZone 0.3.1 is disabled; no hooks were installed.");
        return true;
    }

    CapturePacketState = At<CapturePacketStateFn>(CapturePacketStateRva);
    GetDimensions = At<GetDimensionsFn>(GetDimensionsRva);
    CheckItemType = At<CheckItemTypeFn>(CheckItemTypeRva);
    if (!ValidateRuntime()) {
        context->LogError(
            "CharmZone: a required D2R 3.2.92777 signature changed; plugin refused.");
        return false;
    }
    if (!InstallHooks()) return false;
    if (VisualHookInstalled && !StartOverlayDiscovery()) {
        context->LogWarn(
            "CharmZone: overlay discovery could not start; native charm eligibility enforcement remains active.");
    }
    if (!context->RegisterConsoleCommand(
            "charm-zone",
            Status,
            "Show CharmZone geometry, overlay state, and enforcement counters.")) {
        context->LogWarn("CharmZone: status command could not be registered.");
    }
    context->LogInfo(
        "CharmZone 0.3.1 filters the native charm-eligibility predicate by full zone containment (global/mod-local hybrid)." );
    return true;
}

D2RL_PLUGIN_EXPORT void D2RLoaderUnloadPlugin() noexcept {
    StopOverlayDiscovery();
    Context = nullptr;
}
