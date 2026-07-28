#define NOMINMAX
#include <D2RLPlugin/api.h>
#include <nlohmann/json.hpp>
#include "equipped_item_to_cube_policy.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace ruffneckk::equipped_item_to_cube;

constexpr std::uint32_t SupportedBuild = 92777;
constexpr wchar_t ConfigFileName[] = L"EquippedItemToCube.json";
constexpr std::uint64_t EligibilityHandlerContextRva = 0x228B81;

constexpr std::array<std::uint8_t, 47> EligibilityHandlerContextExpected{
    0x48, 0x8B, 0xCF, 0xE8, 0xD7, 0xE7, 0xF9, 0xFF,
    0x85, 0xC0, 0x0F, 0x85, 0x62, 0x01, 0x00, 0x00,
    0xE8, 0x6A, 0xA7, 0xFB, 0xFF, 0x84, 0xC0, 0x0F,
    0x84, 0x55, 0x01, 0x00, 0x00, 0xB9, 0x11, 0x00,
    0x00, 0x00, 0xE8, 0x58, 0x15, 0xFE, 0x00, 0x85,
    0xC0, 0x0F, 0x84, 0xEF, 0x00, 0x00, 0x00,
};

struct Config {
    bool enabled{true};
};

const D2RL::PluginContext* Context{};
std::uint8_t* Base{};
Config Settings{};
std::string LoadedConfigPath{"built-in defaults"};

constexpr D2RL::PluginInfo Info{
    .infoSize = D2RL::PluginInfoSize,
    .apiVersion = D2RL_PLUGIN_API_VERSION,
    .id = "equipped-item-to-cube",
    .name = "Equipped Item to Cube",
    .version = "0.1.0",
    .author = "RuffnecKk",
    .description = "Restores Ctrl-click moves from equipped slots to the Horadric Cube.",
    .flags = D2RL::PluginFlags::None,
};

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
                const auto message = std::string("EquippedItemToCube: invalid ")
                    + path.string() + " (" + exception.what() + ").";
                Context->LogError(message.c_str());
            }
        }
    }
    return !malformedConfigFound;
}

bool ValidateRuntime() noexcept {
    return Base
        && std::memcmp(
            Base + EligibilityHandlerContextRva,
            EligibilityHandlerContextExpected.data(),
            EligibilityHandlerContextExpected.size()
        ) == 0;
}

bool InstallPatch() noexcept {
    if (!Context->PatchBytes(
            EligibilityGuardBranchRva,
            EligibilityGuardBranchExpected.data(),
            static_cast<std::uint32_t>(EligibilityGuardBranchExpected.size()),
            EligibilityGuardBranchReplacement.data(),
            static_cast<std::uint32_t>(EligibilityGuardBranchReplacement.size())
        )) {
        Context->LogError(
            "EquippedItemToCube: equipped-slot eligibility branch mismatch; patch refused."
        );
        return false;
    }
    return true;
}

auto Status(D2R::Game::Client*, const D2RL::ConsoleCommandContext* command, void*) noexcept
    -> D2RL::ConsoleCommandResult {
    if (!command || !command->plugin) return D2RL::ConsoleCommandResult::Failed;
    char message[320]{};
    std::snprintf(
        message,
        sizeof(message),
        "EquippedItemToCube 0.1.0: enabled=%s; JSON config=%s; future owner=misc.equippedItemToCube.",
        Settings.enabled ? "true" : "false",
        LoadedConfigPath.c_str()
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

    if (!LoadConfig()) {
        context->LogError("EquippedItemToCube: configuration is invalid.");
        return false;
    }
    if (context->modDataVersionBuild != 0
        && context->modDataVersionBuild != SupportedBuild) {
        context->LogError("EquippedItemToCube: only D2R build 92777 is supported.");
        return false;
    }
    if (!ValidateRuntime()) {
        context->LogError(
            "EquippedItemToCube: 92777 equipped-item handler signature mismatch; plugin refused."
        );
        return false;
    }
    if (ShouldInstallPatch(Settings.enabled) && !InstallPatch()) return false;

    if (!context->RegisterConsoleCommand(
            "equipped-item-to-cube",
            Status,
            "Show equipped-item Ctrl-click repair status."
        )) {
        context->LogWarn("EquippedItemToCube: status command could not be registered.");
    }

    char message[480]{};
    std::snprintf(
        message,
        sizeof(message),
        "EquippedItemToCube 0.1.0 %s for D2R 3.2.92777; equipped-slot Ctrl-click eligibility %s (JSON config: %s; future owner: misc.equippedItemToCube).",
        Settings.enabled ? "active" : "disabled",
        Settings.enabled ? "restored" : "left unchanged",
        LoadedConfigPath.c_str()
    );
    context->LogInfo(message);
    return true;
}

D2RL_PLUGIN_EXPORT void D2RLoaderUnloadPlugin() noexcept {
    Context = nullptr;
    Base = nullptr;
}
