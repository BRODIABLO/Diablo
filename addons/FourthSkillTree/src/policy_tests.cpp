#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <string>

#include "fourth_skill_tree_policy.hpp"

#undef assert
#define assert(condition)                                                      \
    do {                                                                       \
        if (!(condition)) {                                                    \
            std::fprintf(                                                      \
                stderr, "check failed at %s:%d: %s\n",                       \
                __FILE__, __LINE__, #condition);                               \
            std::exit(1);                                                      \
        }                                                                      \
    } while (false)

using namespace ruffneckk::fourth_skill_tree;

namespace {

std::string ReadFile(const char* path) {
    std::ifstream input(path, std::ios::binary);
    assert(input.is_open());
    return {
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>()};
}

constexpr std::string_view SkillsHeader =
    "skill\tcharclass\tskilldesc\treqskill1\treqskill2\treqskill3\r\n";
constexpr std::string_view DescriptionsHeader =
    "skilldesc\tSkillPage\tSkillRow\tSkillColumn\r\n";

std::string ValidSkills() {
    return std::string(SkillsHeader)
        + "Base Skill\tama\tbase skill\t\t\t\r\n"
        + "Fourth Probe\tama\tfourth probe\tBase Skill\t\t\r\n";
}

std::string ValidDescriptions() {
    return std::string(DescriptionsHeader)
        + "base skill\t1\t1\t1\r\n"
        + "fourth probe\t4\t2\t2\r\n";
}

std::pair<std::string, std::string> GeneratedContract(std::size_t count) {
    std::string skills(SkillsHeader);
    std::string descriptions(DescriptionsHeader);
    for (std::size_t index = 0; index < count; ++index) {
        const auto skill = "Generated " + std::to_string(index);
        const auto description = "generated " + std::to_string(index);
        skills += skill + "\tama\t" + description + "\t\t\t\r\n";
        descriptions += description;
        if (index + 1 == count) {
            descriptions += "\t4\t6\t3\r\n";
        } else {
            descriptions += "\t1\t1\t1\r\n";
        }
    }
    return {skills, descriptions};
}

void ReplaceOnce(
        std::string& text,
        std::string_view needle,
        std::string_view replacement) {
    const auto offset = text.find(needle);
    assert(offset != std::string::npos);
    text.replace(offset, needle.size(), replacement);
}

} // namespace

int main(int argumentCount, char** arguments) {
    const auto configText = ReadFile(FOURTH_SKILL_TREE_CONFIG_FILE);
    const auto pluginText = ReadFile(FOURTH_SKILL_TREE_PLUGIN_FILE);

    Config config{};
    std::string error;
    assert(ParseToml(configText, config, error));
    assert(!config.enabled);
    assert(!config.diagnostics);
    assert(!ParseToml("config_version = 1\nenabled = 1\n", config, error));
    assert(!ParseToml("config_version = 2\nenabled = false\n", config, error));
    assert(!ParseToml(
        "config_version = 1\nenabled = false\nunknown = true\n",
        config,
        error));
    assert(!ParseToml(
        "config_version = 1\nenabled = false\n[diagnostics]\nverbose = true\n",
        config,
        error));
    assert(IsSupportedBuild("92777"));
    assert(IsSupportedBuild("93847"));
    assert(!IsSupportedBuild(""));
    assert(!IsSupportedBuild("92776"));
    assert(!IsSupportedBuild("93848"));

    ContractSummary summary{};
    auto skills = ValidSkills();
    auto descriptions = ValidDescriptions();
    assert(ValidateContract(skills, descriptions, summary, error));
    assert(summary.skillRows == 2);
    assert(summary.skillDescRows == 2);
    assert(summary.FourthPageSkillCount() == 1);
    assert(summary.classes.size() == 1);
    assert(summary.classes.front().totalSkills == 2);
    assert(summary.classes.front().fourthPageSkills.front().row == 2);
    assert(summary.classes.front().fourthPageSkills.front().column == 2);

    auto [thirtyOneSkills, thirtyOneDescriptions] = GeneratedContract(31);
    assert(ValidateContract(
        thirtyOneSkills, thirtyOneDescriptions, summary, error));
    assert(summary.classes.front().totalSkills == 31);
    assert(summary.FourthPageSkillCount() == 1);

    auto invalidDescriptions = descriptions;
    ReplaceOnce(invalidDescriptions, "\t4\t2\t2", "\t4\t7\t2");
    assert(!ValidateContract(skills, invalidDescriptions, summary, error));
    assert(error.find("SkillRow 1 through 6") != std::string::npos);

    auto duplicateSkills = skills
        + std::string("Fourth Probe Two\tama\tfourth probe two\t\t\t\r\n");
    auto duplicateDescriptions = descriptions
        + std::string("fourth probe two\t4\t2\t2\r\n");
    assert(!ValidateContract(
        duplicateSkills, duplicateDescriptions, summary, error));
    assert(error.find("duplicate fourth-page cell") != std::string::npos);

    auto missingPrerequisite = skills;
    ReplaceOnce(missingPrerequisite, "Base Skill\t\t\r\n", "Missing Skill\t\t\r\n");
    assert(!ValidateContract(
        missingPrerequisite, descriptions, summary, error));
    assert(error.find("unknown prerequisite") != std::string::npos);

    auto cycleSkills = std::string(SkillsHeader)
        + "Fourth A\tama\tfourth a\tFourth B\t\t\r\n"
        + "Fourth B\tama\tfourth b\tFourth A\t\t\r\n";
    auto cycleDescriptions = std::string(DescriptionsHeader)
        + "fourth a\t4\t1\t1\r\n"
        + "fourth b\t4\t2\t1\r\n";
    assert(!ValidateContract(
        cycleSkills, cycleDescriptions, summary, error));
    assert(error.find("cycle") != std::string::npos);

    auto [tooManySkills, tooManyDescriptions] = GeneratedContract(256);
    assert(!ValidateContract(
        tooManySkills, tooManyDescriptions, summary, error));
    assert(error.find("at most 255") != std::string::npos);

    assert(pluginText.find("author = \"RuffnecKk\"") != std::string::npos);
    assert(pluginText.find("PluginFlags::Shared") != std::string::npos);
    assert(pluginText.find("ModScopedOnly") == std::string::npos);
    assert(pluginText.find("IsSupportedBuild(runtimeBuild)")
        != std::string::npos);
    assert(pluginText.find("PatchJmp") == std::string::npos);
    assert(pluginText.find("PatchCall") == std::string::npos);

    if (argumentCount == 3) {
        const auto activeSkills = ReadFile(arguments[1]);
        const auto activeDescriptions = ReadFile(arguments[2]);
        ContractSummary activeSummary{};
        if (!ValidateContract(
                activeSkills,
                activeDescriptions,
                activeSummary,
                error)) {
            std::fprintf(stderr, "active-mod contract failed: %s\n", error.c_str());
            return 2;
        }
        std::fprintf(
            stdout,
            "active-mod contract=PASS skill-rows=%zu skilldesc-rows=%zu page4=%zu classes=%zu\n",
            activeSummary.skillRows,
            activeSummary.skillDescRows,
            activeSummary.FourthPageSkillCount(),
            activeSummary.classes.size());
    }
    return 0;
}
