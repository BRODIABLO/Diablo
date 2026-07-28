#define NOMINMAX
#include <D2RLPlugin/api.h>
#include <nlohmann/json.hpp>
#include "cube_quick_move_policy.hpp"

#include <Windows.h>
#include <intrin.h>

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using ruffneckk::cube_quick_move::ShouldRecomputeBottomRight;
using ruffneckk::cube_quick_move::TryFindBottomRight;

constexpr std::uint32_t SupportedBuild = 92777;
constexpr wchar_t ConfigFileName[] = L"CubeQuickMove.json";

constexpr std::uintptr_t FindFreePositionRva = 0x3865B0;
constexpr std::uintptr_t GetItemDimensionsRva = 0x371850;
constexpr std::uintptr_t GetItemDataContextRva = 0x34A0E0;
constexpr std::uintptr_t BuildGridContextRva = 0x3C6D80;
constexpr std::uintptr_t ResolveOccupancyGridRva = 0x38B070;

struct CubeCallSite {
    std::uintptr_t rva;
    std::array<std::uint8_t, 5> expected;
};

constexpr auto MakeCallSite(std::uintptr_t rva) noexcept -> CubeCallSite {
    const auto displacement = static_cast<std::uint32_t>(
        FindFreePositionRva - (rva + 5)
    );
    return {
        rva,
        {
            0xE8,
            static_cast<std::uint8_t>(displacement),
            static_cast<std::uint8_t>(displacement >> 8),
            static_cast<std::uint8_t>(displacement >> 16),
            static_cast<std::uint8_t>(displacement >> 24),
        }
    };
}

// The 36 direct callers include nine sites whose sixth argument is proven to
// be the constant page 0, 2 or 4. Keep those untouched. These 27 sites are the
// complete remaining set: eight pass page 3 explicitly and nineteen carry a
// dynamic page value that can reach the Cube path.
constexpr std::array<CubeCallSite, 27> CubeCallSites{{
    MakeCallSite(0x0FA33D),
    MakeCallSite(0x15A25C),
    MakeCallSite(0x15E760),
    MakeCallSite(0x15F94F),
    MakeCallSite(0x2C7306),
    MakeCallSite(0x38CC86),
    MakeCallSite(0x40FFCB),
    MakeCallSite(0x417356),
    MakeCallSite(0x42AB9F),
    MakeCallSite(0x471D62),
    MakeCallSite(0x4A90C7),
    MakeCallSite(0x4AB3B5),
    MakeCallSite(0x4AD94B),
    MakeCallSite(0x4B4B5C),
    MakeCallSite(0x4B88ED),
    MakeCallSite(0x4BBA73),
    MakeCallSite(0x4C21D6),
    MakeCallSite(0x4C4181),
    MakeCallSite(0x4C78D2),
    MakeCallSite(0x4F2C8B),
    MakeCallSite(0x4FBC0E),
    MakeCallSite(0x4FC395),
    MakeCallSite(0x527DC2),
    MakeCallSite(0x528053),
    MakeCallSite(0x541230),
    MakeCallSite(0x54128F),
    MakeCallSite(0x541DA7),
}};
constexpr std::array<std::uint8_t, 32> FindFreePositionExpected{
    0x40, 0x55, 0x53, 0x56, 0x57, 0x41, 0x54, 0x41,
    0x56, 0x41, 0x57, 0x48, 0x8B, 0xEC, 0x48, 0x83,
    0xEC, 0x60, 0x48, 0x8B, 0x05, 0xFF, 0x4C, 0x64,
    0x02, 0x48, 0x33, 0xC4, 0x48, 0x89, 0x45, 0xF0
};
constexpr std::array<std::uint8_t, 32> GetItemDimensionsExpected{
    0x48, 0x89, 0x74, 0x24, 0x18, 0x48, 0x89, 0x7C,
    0x24, 0x20, 0x41, 0x56, 0x48, 0x83, 0xEC, 0x20,
    0x49, 0x8B, 0xF0, 0x4C, 0x8B, 0xF2, 0x48, 0x8B,
    0xF9, 0x48, 0x85, 0xC9, 0x74, 0x0A, 0xE8, 0x5D
};
constexpr std::array<std::uint8_t, 24> GetItemDataContextExpected{
    0x48, 0x83, 0xEC, 0x28, 0x48, 0x85, 0xC9, 0x75,
    0x1A, 0x88, 0x4C, 0x24, 0x30, 0x48, 0x8D, 0x4C,
    0x24, 0x30, 0xE8, 0x49, 0xC7, 0xFF, 0xFF, 0x84
};
constexpr std::array<std::uint8_t, 32> BuildGridContextExpected{
    0x48, 0x89, 0x5C, 0x24, 0x08, 0x48, 0x89, 0x6C,
    0x24, 0x10, 0x48, 0x89, 0x74, 0x24, 0x18, 0x57,
    0x48, 0x83, 0xEC, 0x30, 0x49, 0x8B, 0xE9, 0x41,
    0x8B, 0xD8, 0x8B, 0xFA, 0xE8, 0xEF, 0x9C, 0xF3
};
constexpr std::array<std::uint8_t, 32> ResolveOccupancyGridExpected{
    0x4C, 0x8B, 0xDC, 0x49, 0x89, 0x5B, 0x20, 0x57,
    0x48, 0x83, 0xEC, 0x30, 0x49, 0x8B, 0xF8, 0x48,
    0x39, 0x51, 0x28, 0x0F, 0x86, 0x01, 0x01, 0x00,
    0x00, 0x49, 0x89, 0x73, 0x18, 0x48, 0x8D, 0x71
};

struct Config {
    bool enabled{true};
};

using FindFreePositionFn = std::int32_t(__fastcall*)(
    void* inventory,
    void* item,
    std::int32_t inventoryRecordId,
    std::int32_t* freeX,
    std::int32_t* freeY,
    std::uint8_t page
) noexcept;
using GetItemDimensionsFn = void(__fastcall*)(
    void* item,
    std::uint8_t* width,
    std::uint8_t* height,
    const char* sourceFile,
    std::int32_t sourceLine
) noexcept;
using GetItemDataContextFn = std::uint8_t(__fastcall*)(void* item) noexcept;
using BuildGridContextFn = void(__fastcall*)(
    std::uint8_t itemContext,
    std::int32_t inventoryRecordId,
    std::int32_t reserved,
    void* gridContext
) noexcept;
using ResolveOccupancyGridFn = void*(__fastcall*)(
    void* inventory,
    std::int32_t gridIndex,
    void* gridContext
) noexcept;
const D2RL::PluginContext* Context{};
std::uint8_t* Base{};
void* RelayStub{};
std::uint64_t RelayRva{};
Config Settings{};
std::string LoadedConfigPath{"built-in defaults"};
FindFreePositionFn FindFreePosition{};
GetItemDimensionsFn GetItemDimensions{};
GetItemDataContextFn GetItemDataContext{};
BuildGridContextFn BuildGridContext{};
ResolveOccupancyGridFn ResolveOccupancyGrid{};
std::atomic<std::uint64_t> CubeCalls{};
std::atomic<std::uint64_t> PageThreeCalls{};
std::atomic<std::uint64_t> RedirectedPlacements{};
std::atomic<std::uint64_t> VanillaPlacements{};
std::atomic<std::uint64_t> SafeFallbacks{};
std::atomic<std::uint64_t> LastCallSiteRva{};
std::atomic<std::uint64_t> LastPageThreeCallSiteRva{};
std::atomic_bool FirstPlacementReported{};
std::atomic_bool FirstFallbackReported{};

constexpr D2RL::PluginInfo Info{
    .infoSize = D2RL::PluginInfoSize,
    .apiVersion = D2RL_PLUGIN_API_VERSION,
    .id = "cube-quick-move",
    .name = "Cube Quick Move",
    .version = "0.1.3",
    .author = "RuffnecKk",
    .description = "Places quick-moved Cube items from the bottom-right.",
    .flags = D2RL::PluginFlags::NativeHooks,
};

template<class T>
T At(std::uintptr_t rva) noexcept {
    return reinterpret_cast<T>(Base + rva);
}

template<std::size_t Size>
bool Matches(std::uintptr_t rva, const std::array<std::uint8_t, Size>& expected) noexcept {
    return std::memcmp(Base + rva, expected.data(), Size) == 0;
}

bool LoadConfig() noexcept {
    Settings = {};
    LoadedConfigPath = "built-in defaults";

    std::vector<std::filesystem::path> candidates;
    if (Context && Context->modDirectory && Context->modDirectory[0] != L'\0') {
        candidates.emplace_back(std::filesystem::path(Context->modDirectory) / ConfigFileName);
    }
    candidates.emplace_back(ConfigFileName);

    bool malformedConfigFound{};
    for (const auto& path : candidates) {
        std::error_code error;
        if (!std::filesystem::is_regular_file(path, error)) continue;

        try {
            std::ifstream input(path);
            if (!input.is_open()) continue;
            const auto config = nlohmann::json::parse(input, nullptr, true, true);
            if (!config.is_object()) {
                throw std::invalid_argument("configuration root must be an object");
            }
            for (const auto& [key, value] : config.items()) {
                (void)value;
                if (key != "enabled") {
                    throw std::invalid_argument("unknown setting: " + key);
                }
            }
            if (config.contains("enabled") && !config.at("enabled").is_boolean()) {
                throw std::invalid_argument("enabled must be a boolean");
            }
            Settings.enabled = config.value("enabled", true);
            LoadedConfigPath = path.string();
            return true;
        } catch (const std::exception& exception) {
            malformedConfigFound = true;
            if (Context) {
                const auto message = std::string("CubeQuickMove: invalid ")
                    + path.string() + " (" + exception.what() + ").";
                Context->LogError(message.c_str());
            }
        }
    }
    return !malformedConfigFound;
}

bool ValidateRuntime() noexcept {
    for (const auto& callSite : CubeCallSites) {
        if (!Matches(callSite.rva, callSite.expected)) return false;
    }
    return Matches(FindFreePositionRva, FindFreePositionExpected)
        && Matches(GetItemDimensionsRva, GetItemDimensionsExpected)
        && Matches(GetItemDataContextRva, GetItemDataContextExpected)
        && Matches(BuildGridContextRva, BuildGridContextExpected)
        && Matches(ResolveOccupancyGridRva, ResolveOccupancyGridExpected);
}

bool ResolveBottomRight(
    void* occupancyGrid,
    std::uint8_t width,
    std::uint8_t height,
    std::int32_t* freeX,
    std::int32_t* freeY
) noexcept {
    if (!occupancyGrid) return false;
    auto* bytes = static_cast<std::uint8_t*>(occupancyGrid);
    const auto gridWidth = bytes[0x10];
    const auto gridHeight = bytes[0x11];
    const auto cells = *reinterpret_cast<std::uintptr_t**>(bytes + 0x18);
    return TryFindBottomRight(
        cells,
        gridWidth,
        gridHeight,
        width,
        height,
        freeX,
        freeY
    );
}

void* AllocateNear(void* hint, std::size_t size) noexcept {
    SYSTEM_INFO systemInfo{};
    GetSystemInfo(&systemInfo);
    const auto granularity = static_cast<std::uintptr_t>(
        systemInfo.dwAllocationGranularity
    );
    const auto base = reinterpret_cast<std::uintptr_t>(hint)
        & ~(granularity - 1);

    for (std::uintptr_t delta = granularity;
         delta < 0x70000000ULL;
         delta += granularity) {
        if (base <= std::numeric_limits<std::uintptr_t>::max() - delta) {
            if (auto* memory = VirtualAlloc(
                    reinterpret_cast<void*>(base + delta),
                    size,
                    MEM_COMMIT | MEM_RESERVE,
                    PAGE_EXECUTE_READWRITE
                )) {
                return memory;
            }
        }
        // The loader API addresses rel32 targets as RVAs from exeBase. Keeping
        // the relay above the executable base avoids an unsigned underflow.
    }
    return nullptr;
}

bool InstallCallSiteRedirect(void* target) noexcept {
    constexpr std::size_t RelaySize = 14;
    auto* callSite = Base + CubeCallSites.front().rva;
    RelayStub = AllocateNear(callSite, RelaySize);
    if (!RelayStub) return false;

    auto* relay = static_cast<std::uint8_t*>(RelayStub);
    relay[0] = 0xFF;
    relay[1] = 0x25;
    relay[2] = 0x00;
    relay[3] = 0x00;
    relay[4] = 0x00;
    relay[5] = 0x00;
    const auto targetAddress = reinterpret_cast<std::uint64_t>(target);
    std::memcpy(relay + 6, &targetAddress, sizeof(targetAddress));
    FlushInstructionCache(GetCurrentProcess(), RelayStub, RelaySize);

    DWORD previousProtection{};
    if (!VirtualProtect(
            RelayStub,
            RelaySize,
            PAGE_EXECUTE_READ,
            &previousProtection
        )) {
        VirtualFree(RelayStub, 0, MEM_RELEASE);
        RelayStub = nullptr;
        return false;
    }

    const auto relayAddress = reinterpret_cast<std::uintptr_t>(RelayStub);
    const auto baseAddress = reinterpret_cast<std::uintptr_t>(Base);
    if (relayAddress < baseAddress) {
        VirtualFree(RelayStub, 0, MEM_RELEASE);
        RelayStub = nullptr;
        return false;
    }

    RelayRva = static_cast<std::uint64_t>(relayAddress - baseAddress);
    for (const auto& site : CubeCallSites) {
        if (!Context->PatchCallRel32(
                site.rva,
                site.expected.data(),
                static_cast<std::uint32_t>(site.expected.size()),
                RelayRva,
                static_cast<std::uint32_t>(site.expected.size())
            )) {
            return false;
        }
    }
    return true;
}

std::int32_t __fastcall HookFindFreePosition(
    void* inventory,
    void* item,
    std::int32_t inventoryRecordId,
    std::int32_t* freeX,
    std::int32_t* freeY,
    std::uint8_t page
) noexcept {
    const auto returnAddress = reinterpret_cast<std::uintptr_t>(_ReturnAddress());
    const auto callSiteRva = Base && returnAddress >= reinterpret_cast<std::uintptr_t>(Base) + 5
        ? returnAddress - reinterpret_cast<std::uintptr_t>(Base) - 5
        : 0;
    LastCallSiteRva.store(callSiteRva, std::memory_order_relaxed);

    const auto vanillaResult = FindFreePosition(
        inventory,
        item,
        inventoryRecordId,
        freeX,
        freeY,
        page
    );
    CubeCalls.fetch_add(1, std::memory_order_relaxed);
    if (page == 3) {
        PageThreeCalls.fetch_add(1, std::memory_order_relaxed);
        LastPageThreeCallSiteRva.store(callSiteRva, std::memory_order_relaxed);
    }

    if (!Settings.enabled || vanillaResult == 0 || page != 3
        || !inventory || !item || !freeX || !freeY) {
        VanillaPlacements.fetch_add(1, std::memory_order_relaxed);
        return vanillaResult;
    }

    const auto vanillaX = *freeX;
    const auto vanillaY = *freeY;
    std::uint8_t width{};
    std::uint8_t height{};

    __try {
        GetItemDimensions(item, &width, &height, "CubeQuickMove", 0);
        if (!ShouldRecomputeBottomRight(
                Settings.enabled,
                vanillaResult,
                page,
                width,
                height
            )) {
            VanillaPlacements.fetch_add(1, std::memory_order_relaxed);
            return vanillaResult;
        }

        alignas(8) std::array<std::uint8_t, 32> gridContext{};
        BuildGridContext(
            GetItemDataContext(item),
            inventoryRecordId,
            0,
            gridContext.data()
        );
        auto* occupancyGrid = ResolveOccupancyGrid(
            inventory,
            static_cast<std::int32_t>(page) + 2,
            gridContext.data()
        );
        if (ResolveBottomRight(occupancyGrid, width, height, freeX, freeY)) {
            const auto redirected = RedirectedPlacements.fetch_add(
                1,
                std::memory_order_relaxed
            ) + 1;
            if (Context && !FirstPlacementReported.exchange(
                    true,
                    std::memory_order_relaxed
                )) {
                char message[260]{};
                std::snprintf(
                    message,
                    sizeof(message),
                    "CubeQuickMove: first redirected placement from call-site RVA 0x%llX: %ux%u at %d,%d (redirected=%llu).",
                    static_cast<unsigned long long>(callSiteRva),
                    static_cast<unsigned>(width),
                    static_cast<unsigned>(height),
                    *freeX,
                    *freeY,
                    static_cast<unsigned long long>(redirected)
                );
                Context->LogInfo(message);
            }
            return 1;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        // Restore the proven vanilla result below.
    }

    *freeX = vanillaX;
    *freeY = vanillaY;
    const auto fallbacks = SafeFallbacks.fetch_add(1, std::memory_order_relaxed) + 1;
    if (Context && !FirstFallbackReported.exchange(true, std::memory_order_relaxed)) {
        char message[240]{};
        std::snprintf(
            message,
            sizeof(message),
            "CubeQuickMove: safe fallback for %ux%u item on page %u; kept vanilla position %d,%d (fallbacks=%llu).",
            static_cast<unsigned>(width),
            static_cast<unsigned>(height),
            static_cast<unsigned>(page),
            vanillaX,
            vanillaY,
            static_cast<unsigned long long>(fallbacks)
        );
        Context->LogWarn(message);
    }
    return vanillaResult;
}

auto Status(D2R::Game::Client*, const D2RL::ConsoleCommandContext* command, void*) noexcept
    -> D2RL::ConsoleCommandResult {
    if (!command || !command->plugin) return D2RL::ConsoleCommandResult::Failed;
    char message[520]{};
    std::snprintf(
        message,
        sizeof(message),
        "CubeQuickMove 0.1.3: enabled=%s; JSON config=%s; callSites=%llu; calls=%llu; page3=%llu; redirected=%llu; vanilla=%llu; safeFallbacks=%llu; lastCallSite=0x%llX; lastPage3CallSite=0x%llX.",
        Settings.enabled ? "true" : "false",
        LoadedConfigPath.c_str(),
        static_cast<unsigned long long>(CubeCallSites.size()),
        static_cast<unsigned long long>(CubeCalls.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(PageThreeCalls.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(RedirectedPlacements.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(VanillaPlacements.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(SafeFallbacks.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(LastCallSiteRva.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(LastPageThreeCallSiteRva.load(std::memory_order_relaxed))
    );
    command->plugin->WriteConsoleMessage(message);
    return D2RL::ConsoleCommandResult::Handled;
}
} // namespace

D2RL_PLUGIN_EXPORT auto D2RLoaderGetPluginInfo() noexcept -> const D2RL::PluginInfo* {
    return &Info;
}

D2RL_PLUGIN_EXPORT auto D2RLoaderLoadPlugin(const D2RL::PluginContext* context) noexcept -> bool {
    if (!context) return false;
    Context = context;
    Base = reinterpret_cast<std::uint8_t*>(context->exeBase);
    RelayRva = 0;
    CubeCalls.store(0, std::memory_order_relaxed);
    PageThreeCalls.store(0, std::memory_order_relaxed);
    RedirectedPlacements.store(0, std::memory_order_relaxed);
    VanillaPlacements.store(0, std::memory_order_relaxed);
    SafeFallbacks.store(0, std::memory_order_relaxed);
    LastCallSiteRva.store(0, std::memory_order_relaxed);
    LastPageThreeCallSiteRva.store(0, std::memory_order_relaxed);
    FirstPlacementReported.store(false, std::memory_order_relaxed);
    FirstFallbackReported.store(false, std::memory_order_relaxed);

    if (!Base || !LoadConfig()) return false;
    if (context->modDataVersionBuild != 0
        && context->modDataVersionBuild != SupportedBuild) {
        context->LogError("CubeQuickMove: only D2R build 92777 is supported.");
        return false;
    }
    if (!ValidateRuntime()) {
        context->LogError("CubeQuickMove: 92777 Cube call-site or helper signature mismatch; plugin refused.");
        return false;
    }

    FindFreePosition = At<FindFreePositionFn>(FindFreePositionRva);
    GetItemDimensions = At<GetItemDimensionsFn>(GetItemDimensionsRva);
    GetItemDataContext = At<GetItemDataContextFn>(GetItemDataContextRva);
    BuildGridContext = At<BuildGridContextFn>(BuildGridContextRva);
    ResolveOccupancyGrid = At<ResolveOccupancyGridFn>(ResolveOccupancyGridRva);

    if (Settings.enabled && !InstallCallSiteRedirect(
            reinterpret_cast<void*>(&HookFindFreePosition)
        )) {
        context->LogError("CubeQuickMove: Cube placement call-site redirects failed.");
        return false;
    }

    if (!context->RegisterConsoleCommand(
            "cube-quick-move",
            Status,
            "Show Cube quick-move placement status and counters."
        )) {
        context->LogWarn("CubeQuickMove: status command could not be registered.");
    }

    char message[520]{};
    if (Settings.enabled) {
        std::snprintf(
            message,
            sizeof(message),
            "CubeQuickMove 0.1.3 active for D2R 3.2.92777; %llu direct call-sites capable of carrying Cube page 3 redirect through relay RVA 0x%llX to a bounded bottom-right grid scan (JSON config: %s).",
            static_cast<unsigned long long>(CubeCallSites.size()),
            static_cast<unsigned long long>(RelayRva),
            LoadedConfigPath.c_str()
        );
    } else {
        std::snprintf(
            message,
            sizeof(message),
            "CubeQuickMove 0.1.3 disabled for D2R 3.2.92777; no call-site redirects installed (JSON config: %s).",
            LoadedConfigPath.c_str()
        );
    }
    context->LogInfo(message);
    return true;
}

D2RL_PLUGIN_EXPORT void D2RLoaderUnloadPlugin() noexcept {
    // D2RLoader restores registered rel32 patches. Keep the 14-byte relay
    // allocated until process exit so no in-flight call can target freed code.
    Context = nullptr;
}
