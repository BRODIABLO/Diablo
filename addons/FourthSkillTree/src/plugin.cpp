#define NOMINMAX

#include <D2RLPlugin/api.h>

#include "fourth_skill_tree_policy.hpp"

#include <Windows.h>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <vector>

namespace RuffnecKk::FourthSkillTree {
namespace {

using namespace ruffneckk::fourth_skill_tree;

constexpr wchar_t ConfigFileName[] = L"fourth-skill-tree.toml";
constexpr wchar_t SkillsTableName[] = L"skills.txt";
constexpr wchar_t SkillDescTableName[] = L"skilldesc.txt";

enum class OperationalState {
    Disabled,
    ContractValidated,
};

struct LoadedTables {
    std::filesystem::path skillsPath;
    std::filesystem::path skillDescPath;
    std::string skillsText;
    std::string skillDescText;
};

const D2RL::PluginContext* Context{};
Config Settings{};
ContractSummary Contract{};
std::string LoadedConfigPath{"built-in disabled defaults"};
std::string LoadedTablePaths{"none"};
std::atomic<OperationalState> State{OperationalState::Disabled};

constexpr D2RL::PluginInfo Info{
    .infoSize = D2RL::PluginInfoSize,
    .apiVersion = D2RL_PLUGIN_API_VERSION,
    .id = "fourth-skill-tree",
    .name = "Fourth Skill Tree Framework",
    .version = "0.1.0",
    .author = "RuffnecKk",
    .description = "Validates active-mod data for a moddable fourth skill tree.",
    .flags = D2RL::PluginFlags::Shared,
};

std::string DisplayPath(const std::filesystem::path& path) {
    try {
        return path.string();
    } catch (...) {
        return "<unprintable path>";
    }
}

bool InspectRegularFile(
        const std::filesystem::path& path,
        bool& exists,
        std::string& error) {
    exists = false;
    std::error_code inspectError;
    const auto status = std::filesystem::status(path, inspectError);
    if (inspectError) {
        if (inspectError == std::errc::no_such_file_or_directory) return true;
        error = "cannot inspect " + DisplayPath(path)
            + ": " + inspectError.message();
        return false;
    }
    if (!std::filesystem::exists(status)) return true;
    if (!std::filesystem::is_regular_file(status)) {
        error = "expected a regular file: " + DisplayPath(path);
        return false;
    }
    exists = true;
    return true;
}

bool ReadTextFile(
        const std::filesystem::path& path,
        std::size_t maximumBytes,
        std::string& text,
        std::string& error) {
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input.is_open()) {
        error = "cannot open " + DisplayPath(path);
        return false;
    }
    const auto size = input.tellg();
    if (size < 0 || static_cast<std::uint64_t>(size) > maximumBytes) {
        error = "file exceeds its safety limit: " + DisplayPath(path);
        return false;
    }
    text.resize(static_cast<std::size_t>(size));
    input.seekg(0, std::ios::beg);
    if (!text.empty()
            && !input.read(text.data(), static_cast<std::streamsize>(text.size()))) {
        error = "cannot read " + DisplayPath(path);
        return false;
    }
    return true;
}

std::vector<std::filesystem::path> ConfigCandidates() {
    std::vector<std::filesystem::path> directories;
    if (Context && Context->activeMod && Context->activeMod[0] != '\0'
            && Context->modSupportDirectory
            && Context->modSupportDirectory[0] != L'\0') {
        directories.emplace_back(
            std::filesystem::path(Context->modSupportDirectory) / L"config");
    }
    if (Context && Context->pluginConfigPath
            && Context->pluginConfigPath[0] != L'\0') {
        directories.emplace_back(
            std::filesystem::path(Context->pluginConfigPath).parent_path());
    }
    std::error_code currentError;
    const auto current = std::filesystem::current_path(currentError);
    if (!currentError) directories.emplace_back(current / L"d2rloader" / L"config");

    std::vector<std::filesystem::path> result;
    for (const auto& directory : directories) {
        const auto candidate = (directory / ConfigFileName).lexically_normal();
        if (std::find(result.begin(), result.end(), candidate) == result.end()) {
            result.emplace_back(candidate);
        }
    }
    return result;
}

bool LoadConfig() noexcept {
    Settings = {};
    LoadedConfigPath = "built-in disabled defaults";
    for (const auto& path : ConfigCandidates()) {
        bool exists{};
        std::string error;
        if (!InspectRegularFile(path, exists, error)) {
            const auto message = "FourthSkillTree: " + error + ".";
            Context->LogError(message.c_str());
            return false;
        }
        if (!exists) continue;
        std::string text;
        if (!ReadTextFile(path, 64 * 1024, text, error)) {
            const auto message = "FourthSkillTree: " + error + ".";
            Context->LogError(message.c_str());
            return false;
        }
        Config parsed{};
        if (!ParseToml(text, parsed, error)) {
            const auto message = "FourthSkillTree: invalid config "
                + DisplayPath(path) + ": " + error + ".";
            Context->LogError(message.c_str());
            return false;
        }
        Settings = parsed;
        LoadedConfigPath = DisplayPath(path);
        return true;
    }
    return true;
}

std::vector<std::filesystem::path> ExcelCandidates() {
    std::vector<std::filesystem::path> result;
    if (!Context || !Context->modDirectory
            || Context->modDirectory[0] == L'\0') {
        return result;
    }
    const std::filesystem::path root(Context->modDirectory);
    if (Context->activeMod && Context->activeMod[0] != '\0') {
        result.emplace_back((
            root / (std::string(Context->activeMod) + ".mpq")
            / L"data/global/excel").lexically_normal());
    }
    result.emplace_back((root / L"data/global/excel").lexically_normal());
    result.erase(std::unique(result.begin(), result.end()), result.end());
    return result;
}

bool FindAndLoadTables(
        std::optional<LoadedTables>& result,
        std::string& error) {
    result.reset();
    if (!Context || !Context->activeMod || Context->activeMod[0] == '\0') {
        error = "enabled validation requires an active mod";
        return false;
    }
    const auto candidates = ExcelCandidates();
    if (candidates.empty()) {
        error = "D2RLoader did not expose the active mod directory";
        return false;
    }
    for (const auto& directory : candidates) {
        const auto skillsPath = directory / SkillsTableName;
        const auto skillDescPath = directory / SkillDescTableName;
        bool hasSkills{};
        bool hasSkillDesc{};
        if (!InspectRegularFile(skillsPath, hasSkills, error)
                || !InspectRegularFile(skillDescPath, hasSkillDesc, error)) {
            return false;
        }
        if (!hasSkills && !hasSkillDesc) continue;
        if (hasSkills != hasSkillDesc) {
            error = "skills.txt and skilldesc.txt must be installed together under "
                + DisplayPath(directory);
            return false;
        }
        LoadedTables loaded{};
        loaded.skillsPath = skillsPath;
        loaded.skillDescPath = skillDescPath;
        if (!ReadTextFile(
                skillsPath, MaximumTableBytes, loaded.skillsText, error)
                || !ReadTextFile(
                    skillDescPath,
                    MaximumTableBytes,
                    loaded.skillDescText,
                    error)) {
            return false;
        }
        result.emplace(std::move(loaded));
        return true;
    }
    error = "the active mod does not expose skills.txt and skilldesc.txt";
    return false;
}

void LogContract() {
    if (!Settings.diagnostics || !Context) return;
    for (const auto& classContract : Contract.classes) {
        char message[320]{};
        std::snprintf(
            message,
            sizeof(message),
            "FourthSkillTree: class=%s source-skills=%zu fourth-page-skills=%zu.",
            classContract.classCode.c_str(),
            classContract.totalSkills,
            classContract.fourthPageSkills.size());
        Context->LogInfo(message);
    }
}

auto Status(
        D2R::Game::Client*,
        const D2RL::ConsoleCommandContext* command,
        void*) noexcept -> D2RL::ConsoleCommandResult {
    if (!command || !command->plugin) {
        return D2RL::ConsoleCommandResult::Failed;
    }
    std::string classes;
    for (const auto& classContract : Contract.classes) {
        if (!classes.empty()) classes += ", ";
        classes += classContract.classCode + "="
            + std::to_string(classContract.totalSkills) + "/"
            + std::to_string(classContract.fourthPageSkills.size());
    }
    if (classes.empty()) classes = "none";
    char message[2048]{};
    std::snprintf(
        message,
        sizeof(message),
        "Fourth Skill Tree Framework 0.1.0: state=%s; milestone=contract-validator; fourth-page-skills=%zu; classes[source/page4]=%s; config=%s; tables=%s; hooks=none.",
        State.load(std::memory_order_acquire)
                == OperationalState::ContractValidated
            ? "validated"
            : "disabled",
        Contract.FourthPageSkillCount(),
        classes.c_str(),
        LoadedConfigPath.c_str(),
        LoadedTablePaths.c_str());
    command->plugin->WriteConsoleMessage(message);
    return D2RL::ConsoleCommandResult::Handled;
}

void ResetState() noexcept {
    Settings = {};
    Contract = {};
    LoadedConfigPath = "built-in disabled defaults";
    LoadedTablePaths = "none";
    State.store(OperationalState::Disabled, std::memory_order_release);
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
    ResetState();
    Context = context;
    if (!LoadConfig()) return false;
    if (!context->RegisterConsoleCommand(
            "fourth-skill-tree",
            Status,
            "Show the Fourth Skill Tree data-contract validation status.")) {
        context->LogWarn(
            "FourthSkillTree: optional status command was not registered.");
    }
    if (!Settings.enabled) {
        context->LogInfo(
            "Fourth Skill Tree Framework 0.1.0 by RuffnecKk loaded disabled; no table validation or hook was installed.");
        return true;
    }

    const auto* runtimeBuild = D2RL::GetBuildName(context);
    if (!runtimeBuild || !IsSupportedBuild(runtimeBuild)) {
        context->LogError(
            "FourthSkillTree: only governed D2R builds 92777 and 93847 are supported.");
        return false;
    }

    std::optional<LoadedTables> tables;
    std::string error;
    if (!FindAndLoadTables(tables, error) || !tables) {
        const auto message = "FourthSkillTree: contract validation refused: "
            + error + ".";
        context->LogError(message.c_str());
        return false;
    }
    ContractSummary contract{};
    if (!ValidateContract(
            tables->skillsText,
            tables->skillDescText,
            contract,
            error)) {
        const auto message = "FourthSkillTree: invalid active-mod contract: "
            + error + ".";
        context->LogError(message.c_str());
        return false;
    }
    Contract = std::move(contract);
    LoadedTablePaths = DisplayPath(tables->skillsPath) + " | "
        + DisplayPath(tables->skillDescPath);
    State.store(OperationalState::ContractValidated, std::memory_order_release);
    LogContract();

    char message[768]{};
    std::snprintf(
        message,
        sizeof(message),
        "Fourth Skill Tree Framework 0.1.0 by RuffnecKk validated %zu SkillPage=4 row(s) for D2R %s; validation-only milestone, no UI hook installed; installation=%s; TOML=%s.",
        Contract.FourthPageSkillCount(),
        runtimeBuild,
        context->loadScope == D2RL::LoadScope::Mod ? "mod-local" : "global",
        LoadedConfigPath.c_str());
    context->LogInfo(message);
    return true;
}

D2RL_PLUGIN_EXPORT void D2RLoaderUnloadPlugin() noexcept {
    ResetState();
    Context = nullptr;
}

} // namespace RuffnecKk::FourthSkillTree
