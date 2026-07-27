#define NOMINMAX
#include <D2RLPlugin/api.h>
#include <nlohmann/json.hpp>
#include "config_policy.hpp"

#include <Windows.h>

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
using ruffneckk::ground_item_label_limit::DefaultLimit;
using ruffneckk::ground_item_label_limit::ExpandedLimit;
using ruffneckk::ground_item_label_limit::ParseConfig;
using ruffneckk::ground_item_label_limit::VanillaLimit;

constexpr std::uint32_t SupportedBuild = 92777;
constexpr wchar_t ConfigFileName[] = L"GroundItemLabelLimit.json";

using Config = ruffneckk::ground_item_label_limit::Config;

struct Signature {
    std::uint64_t rva;
    std::array<std::uint8_t, 14> expected;
    std::uint32_t size;
};

struct PatchSite {
    std::uint64_t rva;
    std::array<std::uint8_t, 6> expected;
    std::array<std::uint8_t, 6> limit64;
    std::array<std::uint8_t, 6> limit128;
    std::uint32_t size;
};

constexpr std::array<Signature, 7> Signatures{{
    {0x1516EBE, {0x48, 0x83, 0xF8, 0x20, 0x76, 0x70, 0x49, 0x8B, 0x0E}, 9},
    {0x1516ECE, {0x48, 0x8D, 0xB1, 0x80, 0x28, 0x00, 0x00, 0x4C, 0x8D, 0x0C, 0x0F}, 11},
    {0x1516F41, {0x41, 0xB9, 0x20, 0x00, 0x00, 0x00, 0x48, 0x8D, 0x55, 0x88}, 10},
    {0x1519A14, {0x49, 0x83, 0xBE, 0x28, 0x29, 0x00, 0x00, 0x20, 0x44, 0x0F, 0x28, 0x5C, 0x24, 0x70}, 14},
    {0x1519A4F, {0x48, 0x83, 0xF8, 0x20, 0x73, 0x5B, 0xE8, 0x66, 0xC9, 0xCE, 0xFF}, 11},
    {0x1519AAA, {0x48, 0x83, 0xF8, 0x20, 0x72, 0xA5, 0x76, 0x4F}, 8},
    {0x1519AF9, {0x49, 0x83, 0x7C, 0x24, 0x10, 0x20, 0x77, 0xB1}, 8},
}};

constexpr std::array<PatchSite, 7> PatchSites{{
    {
        0x1516EBE,
        {0x48, 0x83, 0xF8, 0x20},
        {0x48, 0x83, 0xF8, 0x40},
        {0x66, 0x3D, 0x80, 0x00},
        4,
    },
    {
        0x1516ED1,
        {0x80, 0x28, 0x00, 0x00},
        {0x00, 0x51, 0x00, 0x00},
        {0x00, 0xA2, 0x00, 0x00},
        4,
    },
    {
        0x1516F43,
        {0x20, 0x00, 0x00, 0x00},
        {0x40, 0x00, 0x00, 0x00},
        {0x80, 0x00, 0x00, 0x00},
        4,
    },
    {
        0x1519A1B,
        {0x20},
        {0x40},
        {0x7F},
        1,
    },
    {
        0x1519A4F,
        {0x48, 0x83, 0xF8, 0x20},
        {0x48, 0x83, 0xF8, 0x40},
        {0x66, 0x3D, 0x80, 0x00},
        4,
    },
    {
        0x1519AAA,
        {0x48, 0x83, 0xF8, 0x20},
        {0x48, 0x83, 0xF8, 0x40},
        {0x66, 0x3D, 0x80, 0x00},
        4,
    },
    {
        0x1519AF9,
        {0x49, 0x83, 0x7C, 0x24, 0x10, 0x20},
        {0x49, 0x83, 0x7C, 0x24, 0x10, 0x40},
        {0x41, 0x80, 0x7C, 0x24, 0x10, 0x80},
        6,
    },
}};

const D2RL::PluginContext* Context{};
Config Settings{};
std::string LoadedConfigPath{"built-in defaults"};

constexpr D2RL::PluginInfo Info{
    .infoSize = D2RL::PluginInfoSize,
    .apiVersion = D2RL_PLUGIN_API_VERSION,
    .id = "ground-item-label-limit",
    .name = "Ground Item Label Limit",
    .version = "1.1.0",
    .author = "RuffnecKk",
    .description = "Shows more item labels on the ground at once.",
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

    for (const auto& path : candidates) {
        std::error_code error;
        if (!std::filesystem::is_regular_file(path, error)) continue;

        try {
            std::ifstream input(path);
            if (!input.is_open()) {
                throw std::runtime_error("configuration could not be opened");
            }
            const auto config = nlohmann::json::parse(input, nullptr, true, true);
            Settings = ParseConfig(config);
            LoadedConfigPath = path.string();
            return true;
        } catch (const std::exception& exception) {
            if (Context) {
                const auto message = std::string("GroundItemLabelLimit: invalid ")
                    + path.string() + " (" + exception.what() + ").";
                Context->LogError(message.c_str());
            }
            return false;
        }
    }

    return true;
}

bool ValidateAllSites() noexcept {
    if (!Context || !Context->exeBase) return false;
    for (const auto& signature : Signatures) {
        const auto* address = reinterpret_cast<const std::uint8_t*>(
            Context->exeBase + signature.rva
        );
        if (std::memcmp(address, signature.expected.data(), signature.size) != 0) {
            return false;
        }
    }
    return true;
}

bool InstallPatches() noexcept {
    if (!ValidateAllSites()) {
        Context->LogError("GroundItemLabelLimit: patch-set signature mismatch; no patch was applied.");
        return false;
    }

    for (const auto& site : PatchSites) {
        const auto& replacement = Settings.limit == ExpandedLimit
            ? site.limit128
            : site.limit64;
        if (!Context->PatchBytes(
                site.rva,
                site.expected.data(),
                site.size,
                replacement.data(),
                site.size
            )) {
            Context->LogError("GroundItemLabelLimit: D2RLoader rejected a validated patch site.");
            return false;
        }
    }
    return true;
}

auto Status(D2R::Game::Client*, const D2RL::ConsoleCommandContext* command, void*) noexcept
    -> D2RL::ConsoleCommandResult {
    if (!command || !command->plugin) return D2RL::ConsoleCommandResult::Failed;
    char message[256]{};
    std::snprintf(
        message,
        sizeof(message),
        "GroundItemLabelLimit 1.1.0: JSON config=%s; enabled=%s; label limit=%u; vanilla=%u; allowed=64,128.",
        LoadedConfigPath.c_str(),
        Settings.enabled ? "yes" : "no",
        Settings.enabled ? Settings.limit : VanillaLimit,
        VanillaLimit
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
    if (!LoadConfig()) {
        context->LogError("GroundItemLabelLimit: configuration is invalid.");
        return false;
    }
    if (context->modDataVersionBuild != 0 && context->modDataVersionBuild != SupportedBuild) {
        context->LogError("GroundItemLabelLimit: only D2R build 92777 is supported.");
        return false;
    }
    if (Settings.enabled && !InstallPatches()) return false;

    if (!context->RegisterConsoleCommand(
            "ground-item-label-limit",
            Status,
            "Show the configured ground item label limit."
        )) {
        context->LogWarn("GroundItemLabelLimit: status command could not be registered.");
    }

    char message[256]{};
    if (Settings.enabled) {
        std::snprintf(
            message,
            sizeof(message),
            "GroundItemLabelLimit 1.1.0 active: ground item label limit raised from %u to %u (JSON config: %s).",
            VanillaLimit,
            Settings.limit,
            LoadedConfigPath.c_str()
        );
    } else {
        std::snprintf(
            message,
            sizeof(message),
            "GroundItemLabelLimit 1.1.0 disabled by JSON config: vanilla limit %u remains unchanged (JSON config: %s).",
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
