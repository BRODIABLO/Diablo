#define NOMINMAX
#include <D2RLPlugin/api.h>
#include <nlohmann/json.hpp>
#include "limit_policy.hpp"

#include <Windows.h>

#include <array>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using tcp::gamble_screen_limit::EffectiveLimit;
using tcp::gamble_screen_limit::ExpandedLimit;
using tcp::gamble_screen_limit::VanillaLimit;

constexpr std::uint32_t SupportedBuild = 92777;
constexpr std::uintptr_t GambleLoopSignatureRva = 0x541A7C;
constexpr std::uint32_t GambleLoopLimitOffset = 2;

constexpr wchar_t ConfigFileName[] = L"GambleScreenLimit.json";

struct Config {
    bool enabled{true};
};

const D2RL::PluginContext* Context{};
Config Settings{};
std::string LoadedConfigPath{"built-in defaults"};

constexpr D2RL::PluginInfo Info{
    .infoSize = D2RL::PluginInfoSize,
    .apiVersion = D2RL_PLUGIN_API_VERSION,
    .id = "gamble-screen-limit",
    .name = "Gamble Screen Limit",
    .version = "1.2.0",
    .author = "RuffnecKk",
    .description = "Expands the gambling screen with more item choices.",
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
                throw nlohmann::json::type_error::create(
                    302,
                    "configuration root must be an object",
                    &config
                );
            }
            for (auto entry = config.begin(); entry != config.end(); ++entry) {
                if (entry.key() != "enabled") {
                    throw std::invalid_argument(
                        std::string("unknown configuration key: ") + entry.key()
                    );
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
                const auto message = std::string("GambleScreenLimit: invalid ")
                    + path.string() + " (" + exception.what() + ").";
                Context->LogError(message.c_str());
            }
        }
    }

    return !malformedConfigFound;
}

auto Status(D2R::Game::Client*, const D2RL::ConsoleCommandContext* command, void*) noexcept
    -> D2RL::ConsoleCommandResult {
    if (!command || !command->plugin) return D2RL::ConsoleCommandResult::Failed;
    char message[256]{};
    std::snprintf(
        message,
        sizeof(message),
        "GambleScreenLimit 1.2.0: JSON config=%s; enabled=%s; effective limit=%u; vanilla=%u; expanded=%u (fixed).",
        LoadedConfigPath.c_str(),
        Settings.enabled ? "yes" : "no",
        EffectiveLimit(Settings.enabled),
        VanillaLimit,
        ExpandedLimit
    );
    command->plugin->WriteConsoleMessage(message);
    return D2RL::ConsoleCommandResult::Handled;
}

bool InstallPatch() noexcept {
    constexpr std::array<std::uint8_t, 9> expected{
        0x83, 0xFD, 0x0E, 0x0F, 0x8C, 0xDB, 0xFE, 0xFF, 0xFF
    };
    auto replacement = expected;
    replacement[GambleLoopLimitOffset] = static_cast<std::uint8_t>(ExpandedLimit);
    if (!Context->PatchBytes(
            GambleLoopSignatureRva,
            expected.data(),
            static_cast<std::uint32_t>(expected.size()),
            replacement.data(),
            static_cast<std::uint32_t>(replacement.size())
        )) {
        Context->LogError("GambleScreenLimit: gamble-loop signature mismatch; patch refused.");
        return false;
    }
    return true;
}
} // namespace

D2RL_PLUGIN_EXPORT auto D2RLoaderGetPluginInfo() noexcept -> const D2RL::PluginInfo* {
    return &Info;
}

D2RL_PLUGIN_EXPORT auto D2RLoaderLoadPlugin(const D2RL::PluginContext* context) noexcept -> bool {
    if (!context) return false;
    Context = context;
    if (!LoadConfig()) {
        context->LogError("GambleScreenLimit: configuration is missing or invalid.");
        return false;
    }
    if (context->modDataVersionBuild != 0 && context->modDataVersionBuild != SupportedBuild) {
        context->LogError("GambleScreenLimit: only D2R build 92777 is supported.");
        return false;
    }
    if (Settings.enabled && !InstallPatch()) return false;

    if (!context->RegisterConsoleCommand(
            "gamble-screen-limit",
            Status,
            "Show the configured gambling screen item limit."
        )) {
        context->LogWarn("GambleScreenLimit: status command could not be registered.");
    }
    char message[256]{};
    if (Settings.enabled) {
        std::snprintf(
            message,
            sizeof(message),
            "GambleScreenLimit 1.2.0 active: gamble item limit raised from %u to fixed limit %u (JSON config: %s).",
            VanillaLimit,
            ExpandedLimit,
            LoadedConfigPath.c_str()
        );
    } else {
        std::snprintf(
            message,
            sizeof(message),
            "GambleScreenLimit 1.2.0 disabled by JSON config: vanilla gamble item limit %u remains unchanged (JSON config: %s).",
            VanillaLimit,
            LoadedConfigPath.c_str()
        );
    }
    context->LogInfo(message);
    return true;
}

D2RL_PLUGIN_EXPORT void D2RLoaderUnloadPlugin() noexcept {
    Context = nullptr;
}
