#define NOMINMAX

#include <D2RLPlugin/api.h>

#include "static-field-rework-policy.hpp"

#include <Windows.h>

#include <array>
#include <atomic>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

namespace RuffnecKk::StaticFieldRework {
namespace {

constexpr std::uint32_t SupportedBuild = 92777;
constexpr wchar_t ConfigFileName[] = L"StaticFieldRework.toml";
constexpr std::uintptr_t StaticFieldHandlerRva = 0x5546B0;
constexpr std::uintptr_t CurseHandlerRva = 0x55D6B0;

constexpr auto StaticFieldHandlerExpected = std::to_array<std::uint8_t>({
    0x40, 0x53, 0x55, 0x56, 0x57, 0x41, 0x56, 0x41,
    0x57, 0x48, 0x81, 0xEC, 0xA8, 0x00, 0x00, 0x00,
    0x48, 0x8B, 0x05, 0x01, 0x6C, 0x47, 0x02, 0x48,
    0x33, 0xC4, 0x48, 0x89, 0x84, 0x24, 0x98, 0x00,
    0x00, 0x00, 0x48, 0x8B, 0xFA, 0x48, 0x8B, 0xD9,
    0x0F, 0xB6, 0x89, 0x06, 0x01, 0x00, 0x00, 0x41,
    0x8B, 0xD0, 0x45, 0x8B, 0xF1, 0x41, 0x8B, 0xE8,
});

constexpr auto CurseHandlerExpected = std::to_array<std::uint8_t>({
    0x40, 0x55, 0x53, 0x56, 0x57, 0x41, 0x54, 0x41,
    0x55, 0x41, 0x56, 0x41, 0x57, 0x48, 0x8D, 0x6C,
    0x24, 0xE1, 0x48, 0x81, 0xEC, 0xE8, 0x00, 0x00,
    0x00, 0x48, 0x8B, 0x05, 0xF8, 0xDB, 0x46, 0x02,
    0x48, 0x33, 0xC4, 0x48, 0x89, 0x45, 0x07, 0x4C,
    0x8B, 0xEA, 0x44, 0x89, 0x4C, 0x24, 0x34, 0x48,
    0x8B, 0xF1, 0x44, 0x89, 0x44, 0x24, 0x38, 0x0F,
    0xB6, 0x89, 0x06, 0x01, 0x00, 0x00, 0x41, 0x8B,
});

using SkillHandlerFn = std::int32_t(__fastcall*)(
    void*, void*, std::int32_t, std::int32_t) noexcept;

struct Config {
    bool enabled{};
    bool diagnostics{};
};

const D2RL::PluginContext* Context{};
std::uint8_t* Base{};
Config Settings{};
std::string LoadedConfigPath{"built-in disabled defaults"};
SkillHandlerFn OriginalStaticField{};
SkillHandlerFn ApplyCurse{};
std::atomic<bool> Operational{};
std::atomic<std::uint64_t> DebuffCasts{};

template <typename Fn>
Fn At(std::uintptr_t rva) noexcept {
    return reinterpret_cast<Fn>(Base + rva);
}

std::string Trim(std::string value) {
    const auto isSpace = [](unsigned char character) {
        return std::isspace(character) != 0;
    };
    value.erase(value.begin(), std::find_if_not(value.begin(), value.end(), isSpace));
    value.erase(std::find_if_not(value.rbegin(), value.rend(), isSpace).base(), value.end());
    return value;
}

bool ParseBoolean(const std::string& value, const std::string& key) {
    if (value == "true") return true;
    if (value == "false") return false;
    throw std::invalid_argument(key + " must be true or false");
}

Config ParseConfig(std::istream& input) {
    Config config{};
    bool sawEnabled{};
    bool sawDiagnostics{};
    std::string line;
    std::size_t lineNumber{};
    while (std::getline(input, line)) {
        ++lineNumber;
        if (const auto comment = line.find('#'); comment != std::string::npos) {
            line.erase(comment);
        }
        line = Trim(std::move(line));
        if (line.empty()) continue;
        const auto separator = line.find('=');
        if (separator == std::string::npos || line.find('=', separator + 1) != std::string::npos) {
            throw std::invalid_argument("line " + std::to_string(lineNumber) + " must contain one '='");
        }
        const auto key = Trim(line.substr(0, separator));
        const auto value = Trim(line.substr(separator + 1));
        if (key == "enabled") {
            if (sawEnabled) throw std::invalid_argument("enabled is duplicated");
            config.enabled = ParseBoolean(value, key);
            sawEnabled = true;
        } else if (key == "diagnostics") {
            if (sawDiagnostics) throw std::invalid_argument("diagnostics is duplicated");
            config.diagnostics = ParseBoolean(value, key);
            sawDiagnostics = true;
        } else {
            throw std::invalid_argument("unknown setting: " + key);
        }
    }
    return config;
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
    std::error_code error;
    const auto currentPath = std::filesystem::current_path(error);
    const auto globalConfigDirectory = error
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
    LoadedConfigPath = "built-in disabled defaults";
    for (const auto& path : ConfigCandidates()) {
        std::error_code error;
        if (!std::filesystem::is_regular_file(path, error)) continue;
        try {
            std::ifstream input(path);
            if (!input.is_open()) {
                throw std::runtime_error("configuration file cannot be opened");
            }
            Settings = ParseConfig(input);
            LoadedConfigPath = path.string();
            return true;
        } catch (const std::exception& exception) {
            const auto message = std::string("StaticFieldRework: invalid ")
                + path.string() + " (" + exception.what() + ").";
            Context->LogError(message.c_str());
            return false;
        }
    }
    return true;
}

std::int32_t HookStaticField(
        void* game,
        void* unit,
        std::int32_t skillId,
        std::int32_t skillLevel) noexcept {
    const auto nativeResult = OriginalStaticField(game, unit, skillId, skillLevel);
    if (!ShouldApplyDebuff(
            Operational.load(std::memory_order_acquire),
            nativeResult,
            skillId,
            skillLevel)) {
        return nativeResult;
    }

    const auto debuffResult = ApplyCurse(game, unit, skillId, skillLevel);
    if (debuffResult != 0) {
        const auto cast = DebuffCasts.fetch_add(1, std::memory_order_relaxed) + 1;
        if (Settings.diagnostics) {
            char message[192]{};
            std::snprintf(
                message,
                sizeof(message),
                "StaticFieldRework: applied debuff for skill level %d (cast %llu).",
                skillLevel,
                static_cast<unsigned long long>(cast));
            Context->LogInfo(message);
        }
    }
    return nativeResult;
}

auto Status(
        D2R::Game::Client*,
        const D2RL::ConsoleCommandContext* command,
        void*) noexcept -> D2RL::ConsoleCommandResult {
    if (!command || !command->plugin) return D2RL::ConsoleCommandResult::Failed;
    char message[384]{};
    std::snprintf(
        message,
        sizeof(message),
        "StaticFieldRework 0.1.0: %s; debuff casts=%llu; config=%s.",
        Operational.load(std::memory_order_acquire) ? "active" : "disabled",
        static_cast<unsigned long long>(DebuffCasts.load(std::memory_order_relaxed)),
        LoadedConfigPath.c_str());
    command->plugin->WriteConsoleMessage(message);
    return D2RL::ConsoleCommandResult::Handled;
}

bool ValidateSignatures() noexcept {
    if (std::memcmp(
            Base + StaticFieldHandlerRva,
            StaticFieldHandlerExpected.data(),
            StaticFieldHandlerExpected.size()) != 0) {
        Context->LogError(
            "StaticFieldRework: Static Field handler signature mismatch; plugin refused.");
        return false;
    }
    if (std::memcmp(
            Base + CurseHandlerRva,
            CurseHandlerExpected.data(),
            CurseHandlerExpected.size()) != 0) {
        Context->LogError(
            "StaticFieldRework: state applicator signature mismatch; plugin refused.");
        return false;
    }
    return true;
}

} // namespace
} // namespace RuffnecKk::StaticFieldRework

namespace {

constexpr D2RL::PluginInfo Info{
    .infoSize = D2RL::PluginInfoSize,
    .apiVersion = D2RL_PLUGIN_API_VERSION,
    .id = "ruffneckk-static-field-rework",
    .name = "Static Field Rework",
    .version = "0.1.0",
    .author = "RuffnecKk",
    .description = "Adds a timed lightning-resistance debuff to Static Field.",
    .flags = D2RL::PluginFlags::NativeHooks,
};

} // namespace

D2RL_PLUGIN_EXPORT auto D2RLoaderGetPluginInfo() noexcept -> const D2RL::PluginInfo* {
    return &Info;
}

D2RL_PLUGIN_EXPORT auto D2RLoaderLoadPlugin(
        const D2RL::PluginContext* context) noexcept -> bool {
    using namespace RuffnecKk::StaticFieldRework;
    Context = context;
    Base = context ? reinterpret_cast<std::uint8_t*>(context->exeBase) : nullptr;
    Operational.store(false, std::memory_order_release);
    DebuffCasts.store(0, std::memory_order_relaxed);
    if (!Context || !Base) return false;
    if (context->modDataVersionBuild != 0
            && context->modDataVersionBuild != SupportedBuild) {
        context->LogError("StaticFieldRework: supports only D2R 3.2 build 92777.");
        return false;
    }
    if (!LoadConfig()) return false;
    if (!Settings.enabled) {
        const auto message = std::string(
            "StaticFieldRework 0.1.0 by RuffnecKk loaded disabled; config=")
            + LoadedConfigPath + ".";
        context->LogInfo(message.c_str());
        return true;
    }
    if (!ValidateSignatures()) return false;
    ApplyCurse = At<SkillHandlerFn>(CurseHandlerRva);
    if (!context->InstallInlineHook(
            StaticFieldHandlerRva,
            StaticFieldHandlerExpected.data(),
            static_cast<std::uint32_t>(StaticFieldHandlerExpected.size()),
            HookStaticField,
            &OriginalStaticField)) {
        context->LogError(
            "StaticFieldRework: Static Field hook could not be installed.");
        return false;
    }
    Operational.store(true, std::memory_order_release);
    if (!context->RegisterConsoleCommand(
            "static-field-rework",
            Status,
            "Show Static Field Rework status.")) {
        context->LogWarn("StaticFieldRework: status command could not be registered.");
    }
    const auto message = std::string(
        "StaticFieldRework 0.1.0 by RuffnecKk active; config=")
        + LoadedConfigPath + ".";
    context->LogInfo(message.c_str());
    return true;
}

D2RL_PLUGIN_EXPORT void D2RLoaderUnloadPlugin() noexcept {
    RuffnecKk::StaticFieldRework::Operational.store(false, std::memory_order_release);
}
