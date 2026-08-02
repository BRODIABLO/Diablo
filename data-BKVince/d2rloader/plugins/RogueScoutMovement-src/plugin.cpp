#define NOMINMAX
#include <D2RLPlugin/api.h>

#include "rogue_movement_policy.hpp"

#include <Windows.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <charconv>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace {

using ruffneckk::rogue_movement::Decide;
using ruffneckk::rogue_movement::IsSupportedVelocity;
using ruffneckk::rogue_movement::Policy;

constexpr std::uint32_t SupportedBuild = 92777;
constexpr std::uintptr_t PetMoveRva = 0x5C1460;
constexpr std::uintptr_t SetVelocityParamsRva = 0x4473F0;
constexpr std::uintptr_t GetClassIdRva = 0x349860;
constexpr std::uintptr_t GetUnitTypeRva = 0x34B9D0;
constexpr std::uintptr_t GetUnitRoomRva = 0x34B440;
constexpr std::uintptr_t IsRoomInTownRva = 0x2F0750;
constexpr std::ptrdiff_t MonsterDataOffset = 0x10;
constexpr std::ptrdiff_t AiParamOffset = 0x38;
constexpr std::ptrdiff_t AiVelocityPercentOffset = 0x24;
constexpr wchar_t ConfigFileName[] = L"rogue-scout-movement.toml";

constexpr std::array<std::uint8_t, 31> PetMoveExpected{
    0x48, 0x89, 0x5C, 0x24, 0x08, 0x48, 0x89, 0x74, 0x24, 0x10,
    0x48, 0x89, 0x7C, 0x24, 0x18, 0x55, 0x41, 0x54, 0x41, 0x55,
    0x41, 0x56, 0x41, 0x57, 0x48, 0x8B, 0xEC, 0x48, 0x83, 0xEC, 0x70
};
constexpr std::array<std::uint8_t, 15> SetVelocityParamsExpected{
    0x4C, 0x8B, 0xD1, 0x85, 0xD2, 0x74, 0x03, 0x89, 0x51, 0x20,
    0x45, 0x85, 0xC0, 0x74, 0x04
};
constexpr std::array<std::uint8_t, 13> GetClassIdExpected{
    0x48, 0x83, 0xEC, 0x28, 0x48, 0x85, 0xC9, 0x75, 0x1D, 0x88,
    0x4C, 0x24, 0x30
};
constexpr std::array<std::uint8_t, 13> GetUnitTypeExpected{
    0x48, 0x83, 0xEC, 0x28, 0x48, 0x85, 0xC9, 0x75, 0x1D, 0x88,
    0x4C, 0x24, 0x30
};
constexpr std::array<std::uint8_t, 12> GetUnitRoomExpected{
    0x40, 0x53, 0x48, 0x83, 0xEC, 0x20, 0x48, 0x8B, 0xD9, 0x48,
    0x85, 0xC9
};
constexpr std::array<std::uint8_t, 15> IsRoomInTownExpected{
    0x48, 0x83, 0xEC, 0x28, 0x48, 0x85, 0xC9, 0x75, 0x07, 0x33,
    0xC0, 0x48, 0x83, 0xC4, 0x28
};

struct Config {
    Policy movement{};
    bool diagnostics{};
};

struct UnitInspection {
    std::int32_t unitType{};
    std::int32_t classId{};
    void* aiParam{};
    bool inTown{};
};

struct VelocityOverride {
    void* aiParam{};
    std::int32_t bonusPercent{};
};

using PetMoveFn = std::int32_t(__fastcall*)(
    void*, void*, void*, std::int32_t, std::int32_t, std::int32_t, std::uint8_t
) noexcept;
using SetVelocityParamsFn = void(__fastcall*)(
    void*, std::int32_t, std::int32_t, std::uint8_t
) noexcept;
using GetUnitValueFn = std::int32_t(__fastcall*)(void*) noexcept;
using GetUnitRoomFn = void*(__fastcall*)(void*) noexcept;
using IsRoomInTownFn = std::int32_t(__fastcall*)(void*) noexcept;

const D2RL::PluginContext* Context{};
std::uint8_t* Base{};
Config Settings{};
std::string LoadedConfigPath{"built-in defaults"};
PetMoveFn OriginalPetMove{};
SetVelocityParamsFn OriginalSetVelocityParams{};
GetUnitValueFn GetClassId{};
GetUnitValueFn GetUnitType{};
GetUnitRoomFn GetUnitRoom{};
IsRoomInTownFn IsRoomInTown{};
thread_local VelocityOverride ActiveVelocityOverride{};
std::atomic<std::uint64_t> TownWalks{};
std::atomic<std::uint64_t> OutsideRuns{};
std::atomic<std::uint64_t> VelocityOverrides{};
std::atomic<std::uint64_t> InspectionFallbacks{};
std::atomic<bool> TownDecisionLogged{};
std::atomic<bool> OutsideDecisionLogged{};

constexpr D2RL::PluginInfo Info{
    .infoSize = D2RL::PluginInfoSize,
    .apiVersion = D2RL_PLUGIN_API_VERSION,
    .id = "rogue-scout-movement",
    .name = "RogueScoutMovement",
    .version = "0.1.0",
    .author = "RuffnecKk",
    .description = "Makes Act I Rogue Scouts walk in town and run while following elsewhere.",
    .flags = D2RL::PluginFlags::NativeHooks,
};

template <typename T>
T At(std::uintptr_t rva) noexcept {
    return reinterpret_cast<T>(Base + rva);
}

std::string Trim(std::string text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

bool ParseBool(std::string_view value, std::string_view path) {
    if (value == "true") return true;
    if (value == "false") return false;
    throw std::runtime_error(std::string(path) + " must be true or false");
}

std::int32_t ParseInt(std::string_view value, std::string_view path) {
    std::int32_t parsed{};
    const auto* begin = value.data();
    const auto* end = begin + value.size();
    const auto result = std::from_chars(begin, end, parsed);
    if (result.ec != std::errc{} || result.ptr != end) {
        throw std::runtime_error(std::string(path) + " must be an integer");
    }
    return parsed;
}

void ParseConfig(std::istream& input) {
    Config parsed{};
    std::string section;
    std::unordered_set<std::string> seenSections;
    std::unordered_set<std::string> seenKeys;
    std::string line;
    std::size_t lineNumber{};

    while (std::getline(input, line)) {
        ++lineNumber;
        if (const auto comment = line.find('#'); comment != std::string::npos) {
            line.erase(comment);
        }
        line = Trim(line);
        if (line.empty()) continue;

        if (line.front() == '[' && line.back() == ']') {
            section = Trim(line.substr(1, line.size() - 2));
            if (section != "movement" && section != "diagnostics") {
                throw std::runtime_error("unknown section at line " + std::to_string(lineNumber));
            }
            if (!seenSections.insert(section).second) {
                throw std::runtime_error("duplicate section [" + section + "]");
            }
            continue;
        }

        const auto equal = line.find('=');
        if (equal == std::string::npos || line.find('=', equal + 1) != std::string::npos) {
            throw std::runtime_error("invalid assignment at line " + std::to_string(lineNumber));
        }
        const auto key = Trim(line.substr(0, equal));
        const auto value = Trim(line.substr(equal + 1));
        if (key.empty() || value.empty()) {
            throw std::runtime_error("empty key or value at line " + std::to_string(lineNumber));
        }
        const auto qualified = section.empty() ? key : section + "." + key;
        if (!seenKeys.insert(qualified).second) {
            throw std::runtime_error("duplicate setting " + qualified);
        }

        if (section.empty() && key == "enabled") {
            parsed.movement.enabled = ParseBool(value, qualified);
        } else if (section == "movement" && key == "walk_in_town") {
            parsed.movement.walkInTown = ParseBool(value, qualified);
        } else if (section == "movement" && key == "run_outside_town") {
            parsed.movement.runOutsideTown = ParseBool(value, qualified);
        } else if (section == "movement" && key == "town_velocity") {
            parsed.movement.townVelocity = ParseInt(value, qualified);
        } else if (section == "movement" && key == "outside_velocity") {
            parsed.movement.outsideVelocity = ParseInt(value, qualified);
        } else if (section == "diagnostics" && key == "enabled") {
            parsed.diagnostics = ParseBool(value, qualified);
        } else {
            throw std::runtime_error("unknown setting " + qualified);
        }
    }

    if (!IsSupportedVelocity(parsed.movement.townVelocity)
        || !IsSupportedVelocity(parsed.movement.outsideVelocity)) {
        throw std::runtime_error("movement velocities must be integers from 3 through 24");
    }
    Settings = parsed;
}

std::filesystem::path ExecutableDirectory() {
    std::vector<wchar_t> buffer(512);
    for (;;) {
        const auto length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (length == 0) return {};
        if (length < buffer.size() - 1) {
            return std::filesystem::path(std::wstring(buffer.data(), length)).parent_path();
        }
        buffer.resize(buffer.size() * 2);
    }
}

std::vector<std::filesystem::path> ConfigCandidates() {
    std::vector<std::filesystem::path> candidates;
    if (Context && Context->modDirectory && Context->modDirectory[0] != L'\0') {
        candidates.emplace_back(
            std::filesystem::path(Context->modDirectory) / L"d2rloader" / L"config" / ConfigFileName
        );
    }
    if (Context && Context->pluginConfigPath && Context->pluginConfigPath[0] != L'\0') {
        candidates.emplace_back(Context->pluginConfigPath);
    }
    const auto executableDirectory = ExecutableDirectory();
    if (!executableDirectory.empty()) {
        candidates.emplace_back(executableDirectory / L"d2rloader" / L"config" / ConfigFileName);
    }

    std::vector<std::filesystem::path> unique;
    for (const auto& candidate : candidates) {
        const auto normalized = candidate.lexically_normal();
        const auto duplicate = std::find_if(unique.begin(), unique.end(), [&](const auto& current) {
            return _wcsicmp(current.c_str(), normalized.c_str()) == 0;
        });
        if (duplicate == unique.end()) unique.push_back(normalized);
    }
    return unique;
}

bool LoadConfig() noexcept {
    Settings = {};
    LoadedConfigPath = "built-in defaults";
    for (const auto& path : ConfigCandidates()) {
        std::error_code error;
        if (!std::filesystem::is_regular_file(path, error)) continue;
        try {
            std::ifstream input(path, std::ios::binary);
            if (!input.is_open()) throw std::runtime_error("file could not be opened");
            ParseConfig(input);
            LoadedConfigPath = path.string();
            return true;
        } catch (const std::exception& exception) {
            if (Context) {
                const auto message = std::string("RogueScoutMovement: invalid ")
                    + path.string() + " (" + exception.what() + ").";
                Context->LogError(message.c_str());
            }
            return false;
        }
    }
    return true;
}

bool InspectUnit(void* unit, UnitInspection& inspection) noexcept {
    if (!unit) return false;
    __try {
        inspection.unitType = GetUnitType(unit);
        inspection.classId = GetClassId(unit);
        if (inspection.unitType != ruffneckk::rogue_movement::MonsterUnitType
            || inspection.classId != ruffneckk::rogue_movement::RogueHireClassId) {
            return true;
        }
        const auto monsterData = *reinterpret_cast<void**>(
            static_cast<std::uint8_t*>(unit) + MonsterDataOffset
        );
        if (!monsterData) return false;
        inspection.aiParam = *reinterpret_cast<void**>(
            static_cast<std::uint8_t*>(monsterData) + AiParamOffset
        );
        if (!inspection.aiParam) return false;
        const auto room = GetUnitRoom(unit);
        if (!room) return false;
        inspection.inTown = IsRoomInTown(room) != 0;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

class VelocityOverrideScope {
public:
    VelocityOverrideScope(void* aiParam, std::int32_t bonusPercent) noexcept
        : previous_(ActiveVelocityOverride) {
        ActiveVelocityOverride = {.aiParam = aiParam, .bonusPercent = bonusPercent};
    }

    ~VelocityOverrideScope() {
        ActiveVelocityOverride = previous_;
    }

    VelocityOverrideScope(const VelocityOverrideScope&) = delete;
    VelocityOverrideScope& operator=(const VelocityOverrideScope&) = delete;

private:
    VelocityOverride previous_{};
};

void __fastcall HookSetVelocityParams(
    void* aiParam,
    std::int32_t pathType,
    std::int32_t velocityPercent,
    std::uint8_t distance
) noexcept {
    if (!ActiveVelocityOverride.aiParam || aiParam != ActiveVelocityOverride.aiParam) {
        OriginalSetVelocityParams(aiParam, pathType, velocityPercent, distance);
        return;
    }

    const auto configured = ActiveVelocityOverride.bonusPercent;
    if (configured != 0) {
        OriginalSetVelocityParams(aiParam, pathType, configured, distance);
        VelocityOverrides.fetch_add(1, std::memory_order_relaxed);
        return;
    }

    __try {
        *reinterpret_cast<std::int32_t*>(
            static_cast<std::uint8_t*>(aiParam) + AiVelocityPercentOffset
        ) = 0;
        OriginalSetVelocityParams(aiParam, pathType, 0, distance);
        VelocityOverrides.fetch_add(1, std::memory_order_relaxed);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        OriginalSetVelocityParams(aiParam, pathType, velocityPercent, distance);
    }
}

void LogDecisionOnce(bool inTown, std::int32_t run, std::int32_t velocityPercent) noexcept {
    if (!Settings.diagnostics || !Context) return;

    auto& logged = inTown ? TownDecisionLogged : OutsideDecisionLogged;
    if (logged.exchange(true, std::memory_order_relaxed)) return;

    char message[192]{};
    std::snprintf(
        message,
        sizeof(message),
        "RogueScoutMovement: first %s follow decision observed; motion=%s; velocity=%d.",
        inTown ? "town" : "outside-town",
        run != 0 ? "run" : "walk",
        velocityPercent
    );
    Context->LogInfo(message);
}

std::int32_t __fastcall HookPetMove(
    void* game,
    void* owner,
    void* unit,
    std::int32_t motionType,
    std::int32_t run,
    std::int32_t velocityPercent,
    std::uint8_t steps
) noexcept {
    UnitInspection inspection{};
    if (!InspectUnit(unit, inspection)) {
        InspectionFallbacks.fetch_add(1, std::memory_order_relaxed);
        return OriginalPetMove(game, owner, unit, motionType, run, velocityPercent, steps);
    }

    const auto decision = Decide(
        Settings.movement,
        inspection.unitType,
        inspection.classId,
        motionType,
        inspection.inTown,
        run
    );
    if (!decision.applies || !decision.overrideVelocity) {
        return OriginalPetMove(game, owner, unit, motionType, run, velocityPercent, steps);
    }

    if (inspection.inTown) TownWalks.fetch_add(1, std::memory_order_relaxed);
    else OutsideRuns.fetch_add(1, std::memory_order_relaxed);
    LogDecisionOnce(inspection.inTown, decision.run, decision.velocityBonusPercent);

    const VelocityOverrideScope velocityScope(
        inspection.aiParam,
        decision.velocityBonusPercent
    );
    return OriginalPetMove(
        game,
        owner,
        unit,
        motionType,
        decision.run,
        decision.velocityBonusPercent,
        steps
    );
}

bool ValidateRuntime() noexcept {
    return Context->CheckExpectedBytes(
            PetMoveRva, PetMoveExpected.data(), static_cast<std::uint32_t>(PetMoveExpected.size())
        )
        && Context->CheckExpectedBytes(
            SetVelocityParamsRva,
            SetVelocityParamsExpected.data(),
            static_cast<std::uint32_t>(SetVelocityParamsExpected.size())
        )
        && Context->CheckExpectedBytes(
            GetClassIdRva,
            GetClassIdExpected.data(),
            static_cast<std::uint32_t>(GetClassIdExpected.size())
        )
        && Context->CheckExpectedBytes(
            GetUnitTypeRva,
            GetUnitTypeExpected.data(),
            static_cast<std::uint32_t>(GetUnitTypeExpected.size())
        )
        && Context->CheckExpectedBytes(
            GetUnitRoomRva,
            GetUnitRoomExpected.data(),
            static_cast<std::uint32_t>(GetUnitRoomExpected.size())
        )
        && Context->CheckExpectedBytes(
            IsRoomInTownRva,
            IsRoomInTownExpected.data(),
            static_cast<std::uint32_t>(IsRoomInTownExpected.size())
        );
}

bool InstallHooks() noexcept {
    if (!Context->InstallInlineHook(
            SetVelocityParamsRva,
            SetVelocityParamsExpected.data(),
            static_cast<std::uint32_t>(SetVelocityParamsExpected.size()),
            HookSetVelocityParams,
            &OriginalSetVelocityParams
        )) return false;
    return Context->InstallInlineHook(
        PetMoveRva,
        PetMoveExpected.data(),
        static_cast<std::uint32_t>(PetMoveExpected.size()),
        HookPetMove,
        &OriginalPetMove
    );
}

auto Status(D2R::Game::Client*, const D2RL::ConsoleCommandContext* command, void*) noexcept
    -> D2RL::ConsoleCommandResult {
    if (!command || !command->plugin) return D2RL::ConsoleCommandResult::Failed;
    char message[640]{};
    std::snprintf(
        message,
        sizeof(message),
        "RogueScoutMovement 0.1.0: config=%s; enabled=%s; town walk=%s at %d; outside run=%s at %d; town decisions=%llu; outside decisions=%llu; velocity overrides=%llu; safe fallbacks=%llu.",
        LoadedConfigPath.c_str(),
        Settings.movement.enabled ? "true" : "false",
        Settings.movement.walkInTown ? "true" : "false",
        Settings.movement.townVelocity,
        Settings.movement.runOutsideTown ? "true" : "false",
        Settings.movement.outsideVelocity,
        static_cast<unsigned long long>(TownWalks.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(OutsideRuns.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(VelocityOverrides.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(InspectionFallbacks.load(std::memory_order_relaxed))
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
    TownWalks.store(0, std::memory_order_relaxed);
    OutsideRuns.store(0, std::memory_order_relaxed);
    VelocityOverrides.store(0, std::memory_order_relaxed);
    InspectionFallbacks.store(0, std::memory_order_relaxed);
    TownDecisionLogged.store(false, std::memory_order_relaxed);
    OutsideDecisionLogged.store(false, std::memory_order_relaxed);
    if (!Base || !LoadConfig()) return false;
    if (context->modDataVersionBuild != 0 && context->modDataVersionBuild != SupportedBuild) {
        context->LogError("RogueScoutMovement: only D2R build 92777 is supported.");
        return false;
    }

    GetClassId = At<GetUnitValueFn>(GetClassIdRva);
    GetUnitType = At<GetUnitValueFn>(GetUnitTypeRva);
    GetUnitRoom = At<GetUnitRoomFn>(GetUnitRoomRva);
    IsRoomInTown = At<IsRoomInTownFn>(IsRoomInTownRva);

    if (!ValidateRuntime()) {
        context->LogError("RogueScoutMovement: D2R 3.2.92777 signature or movement ABI mismatch; plugin refused.");
        return false;
    }
    if (Settings.movement.enabled && !InstallHooks()) {
        context->LogError("RogueScoutMovement: native hook installation failed.");
        return false;
    }
    if (!context->RegisterConsoleCommand(
            "rogue-scout-movement",
            Status,
            "Show Act I Rogue Scout movement configuration and counters."
        )) {
        context->LogWarn("RogueScoutMovement: status command could not be registered.");
    }
    std::string activation = "RogueScoutMovement 0.1.0 ";
    activation += Settings.movement.enabled ? "active" : "loaded disabled";
    activation += " for D2R 3.2.92777; config=" + LoadedConfigPath;
    activation += "; town walk=";
    activation += Settings.movement.walkInTown ? "true" : "false";
    activation += " at " + std::to_string(Settings.movement.townVelocity);
    activation += "; outside run=";
    activation += Settings.movement.runOutsideTown ? "true" : "false";
    activation += " at " + std::to_string(Settings.movement.outsideVelocity);
    activation += Settings.diagnostics ? "; diagnostics=true." : "; diagnostics=false.";
    context->LogInfo(activation.c_str());
    return true;
}

D2RL_PLUGIN_EXPORT void D2RLoaderUnloadPlugin() noexcept {
    Context = nullptr;
    Base = nullptr;
}
