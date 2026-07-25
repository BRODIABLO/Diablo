#define NOMINMAX
#include <Windows.h>
#include <D2RLPlugin/api.h>
#include <nlohmann/json.hpp>

#include <array>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
constexpr std::uint32_t SupportedBuild = 92777;
constexpr std::uintptr_t ApplyMonsterStatRegenRva = 0x448C00;
constexpr std::uintptr_t GetUnitStatRva = 0x2F5020;
constexpr std::uintptr_t GetUnitBaseStatRva = 0x2F48C0;
constexpr std::uintptr_t CheckLifeStateMaskRva = 0x335E80;
constexpr std::uintptr_t GetUnitRoomRva = 0x34B440;
constexpr std::uintptr_t IsRoomInTownRva = 0x2F0750;
constexpr std::uintptr_t SetEventRva = 0x48B720;
constexpr std::ptrdiff_t GameFrameOffset = 0x170;
constexpr std::int32_t MonsterUnitType = 1;
constexpr std::int32_t HitpointsStat = 6;
constexpr std::int32_t HitpointRegenStat = 74;
constexpr std::int32_t StatRegenEvent = 3;
constexpr wchar_t ConfigFileName[] = L"PreventMercDeathInTown.json";

constexpr std::array<std::uint8_t, 21> ApplyMonsterStatRegenExpected{
    0x40, 0x53, 0x55, 0x57, 0x48, 0x81, 0xEC, 0x90,
    0x00, 0x00, 0x00, 0x48, 0x8B, 0x05, 0xB6, 0x26,
    0x58, 0x02, 0x48, 0x33, 0xC4
};

struct UnitView {
    std::uint32_t unitType;
    std::uint32_t classId;
};

struct Config {
    bool enabled{};
};

using ApplyMonsterStatRegenFn = void(*)(void*, void*, std::int32_t, std::int32_t) noexcept;
using GetUnitStatFn = std::int32_t(*)(void*, std::int32_t, std::int32_t) noexcept;
using CheckLifeStateMaskFn = std::int32_t(*)(void*) noexcept;
using GetUnitRoomFn = void*(*)(void*) noexcept;
using IsRoomInTownFn = std::int32_t(*)(void*) noexcept;
using SetEventFn = void(*)(
    void*, void*, std::int32_t, std::int32_t, std::int32_t, std::int32_t
) noexcept;

const D2RL::PluginContext* Context{};
std::uint8_t* Base{};
Config Settings{};
ApplyMonsterStatRegenFn OriginalApplyMonsterStatRegen{};
GetUnitStatFn GetUnitStat{};
GetUnitStatFn GetUnitBaseStat{};
CheckLifeStateMaskFn CheckLifeStateMask{};
GetUnitRoomFn GetUnitRoom{};
IsRoomInTownFn IsRoomInTown{};
SetEventFn ScheduleEvent{};
std::string LoadedConfigPath{"not found (vanilla fallback)"};
std::atomic<std::uint64_t> PreventedDeaths{};

constexpr D2RL::PluginInfo Info{
    .infoSize = D2RL::PluginInfoSize,
    .apiVersion = D2RL_PLUGIN_API_VERSION,
    .id = "prevent-merc-death-in-town",
    .name = "Prevent Merc Death in Town",
    .version = "0.1.0",
    .author = "RuffnecKk",
    .description = "Keeps mercenaries alive while persistent damage remains active in town.",
    .flags = D2RL::PluginFlags::NativeHooks,
};

template <typename T>
T At(std::uintptr_t rva) noexcept {
    return reinterpret_cast<T>(Base + rva);
}

bool IsHirelingClass(std::uint32_t classId) noexcept {
    return classId == 271 || classId == 338 || classId == 359
        || classId == 560 || classId == 561;
}

bool LoadConfig() noexcept {
    Settings = {};
    LoadedConfigPath = "not found (vanilla fallback)";
    std::vector<std::filesystem::path> candidates;
    if (Context && Context->modDirectory && Context->modDirectory[0] != L'\0') {
        candidates.emplace_back(std::filesystem::path(Context->modDirectory) / ConfigFileName);
    }
    candidates.emplace_back(ConfigFileName);

    for (const auto& path : candidates) {
        std::error_code error;
        if (!std::filesystem::is_regular_file(path, error)) continue;
        try {
            std::ifstream input(path);
            if (!input.is_open()) continue;
            const auto config = nlohmann::json::parse(input, nullptr, true, true);
            if (!config.is_object()) throw std::invalid_argument("configuration root must be an object");
            for (const auto& [key, value] : config.items()) {
                (void)value;
                if (key != "enabled") throw std::invalid_argument("unknown setting: " + key);
            }
            if (!config.contains("enabled") || !config.at("enabled").is_boolean()) {
                throw std::invalid_argument("enabled must be a boolean");
            }
            Settings.enabled = config.at("enabled").get<bool>();
            LoadedConfigPath = path.string();
            return true;
        } catch (const std::exception& exception) {
            const auto message = std::string("PreventMercDeathInTown: invalid ")
                + path.string() + " (" + exception.what() + ").";
            Context->LogError(message.c_str());
            return false;
        }
    }
    return true;
}

bool IsLethalHirelingTickInTown(void* game, void* unit) noexcept {
    if (!game || !unit) return false;
    __try {
        const auto* view = static_cast<const UnitView*>(unit);
        if (view->unitType != MonsterUnitType || !IsHirelingClass(view->classId)) return false;

        auto regen = GetUnitStat(unit, HitpointRegenStat, 0);
        if (CheckLifeStateMask(unit)) {
            regen -= GetUnitBaseStat(unit, HitpointRegenStat, 0);
        }
        if (regen >= 0) return false;

        const auto hitpoints = GetUnitStat(unit, HitpointsStat, 0);
        if (static_cast<std::int64_t>(hitpoints) + regen > 0) return false;

        auto* room = GetUnitRoom(unit);
        return room && IsRoomInTown(room) != 0;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

void HookApplyMonsterStatRegen(
    void* game, void* unit, std::int32_t a3, std::int32_t a4
) noexcept {
    if (!IsLethalHirelingTickInTown(game, unit)) {
        OriginalApplyMonsterStatRegen(game, unit, a3, a4);
        return;
    }

    __try {
        const auto frame = *reinterpret_cast<const std::int32_t*>(
            static_cast<const std::uint8_t*>(game) + GameFrameOffset
        );
        ScheduleEvent(game, unit, StatRegenEvent, frame + 1, 0, 0);
        PreventedDeaths.fetch_add(1, std::memory_order_relaxed);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        OriginalApplyMonsterStatRegen(game, unit, a3, a4);
    }
}

auto Status(D2R::Game::Client*, const D2RL::ConsoleCommandContext* command, void*) noexcept
    -> D2RL::ConsoleCommandResult {
    if (!command || !command->plugin) return D2RL::ConsoleCommandResult::Failed;
    char message[320]{};
    std::snprintf(
        message,
        sizeof(message),
        "PreventMercDeathInTown 0.1.0: JSON config=%s; enabled=%s; prevented lethal ticks=%llu.",
        LoadedConfigPath.c_str(),
        Settings.enabled ? "true" : "false",
        static_cast<unsigned long long>(PreventedDeaths.load(std::memory_order_relaxed))
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
    Base = reinterpret_cast<std::uint8_t*>(GetModuleHandleW(nullptr));
    PreventedDeaths.store(0, std::memory_order_relaxed);
    if (!Base || !LoadConfig()) return false;
    if (context->modDataVersionBuild != 0 && context->modDataVersionBuild != SupportedBuild) {
        context->LogError("PreventMercDeathInTown: only D2R build 92777 is supported.");
        return false;
    }

    GetUnitStat = At<GetUnitStatFn>(GetUnitStatRva);
    GetUnitBaseStat = At<GetUnitStatFn>(GetUnitBaseStatRva);
    CheckLifeStateMask = At<CheckLifeStateMaskFn>(CheckLifeStateMaskRva);
    GetUnitRoom = At<GetUnitRoomFn>(GetUnitRoomRva);
    IsRoomInTown = At<IsRoomInTownFn>(IsRoomInTownRva);
    ScheduleEvent = At<SetEventFn>(SetEventRva);

    if (Settings.enabled && !context->InstallInlineHook(
            ApplyMonsterStatRegenRva,
            ApplyMonsterStatRegenExpected.data(),
            static_cast<std::uint32_t>(ApplyMonsterStatRegenExpected.size()),
            HookApplyMonsterStatRegen,
            &OriginalApplyMonsterStatRegen
        )) {
        context->LogError("PreventMercDeathInTown: stat-regen signature mismatch; plugin refused.");
        return false;
    }

    if (!context->RegisterConsoleCommand(
            "prevent-merc-death-in-town",
            Status,
            "Show the persistent-damage protection status."
        )) {
        context->LogWarn("PreventMercDeathInTown: status command could not be registered.");
    }
    context->LogInfo(
        Settings.enabled
            ? "PreventMercDeathInTown 0.1.0 active for D2R 3.2.92777."
            : "PreventMercDeathInTown 0.1.0 loaded disabled; vanilla behavior preserved."
    );
    return true;
}

D2RL_PLUGIN_EXPORT void D2RLoaderUnloadPlugin() noexcept {}
