#pragma once

#include <toml++/toml.hpp>

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ruffneckk::fourth_skill_tree {

inline constexpr std::int64_t ConfigVersion = 1;
inline constexpr std::size_t MaximumTableBytes = 4 * 1024 * 1024;
inline constexpr std::size_t MaximumClassSkills = 255;
inline constexpr std::size_t MaximumFourthPageSkills = 18;
inline constexpr std::int32_t FourthPage = 4;
inline constexpr std::int32_t MinimumRow = 1;
inline constexpr std::int32_t MaximumRow = 6;
inline constexpr std::int32_t MinimumColumn = 1;
inline constexpr std::int32_t MaximumColumn = 3;

[[nodiscard]] inline bool IsSupportedBuild(std::string_view build) noexcept {
    return build == "92777" || build == "93847";
}

struct Config {
    bool enabled{};
    bool diagnostics{};
};

struct Placement {
    std::string skill;
    std::string skillDesc;
    std::string classCode;
    std::int32_t row{};
    std::int32_t column{};
    std::vector<std::string> prerequisites;
};

struct ClassContract {
    std::string classCode;
    std::size_t totalSkills{};
    std::vector<Placement> fourthPageSkills;
};

struct ContractSummary {
    std::size_t skillRows{};
    std::size_t skillDescRows{};
    std::vector<ClassContract> classes;

    [[nodiscard]] std::size_t FourthPageSkillCount() const noexcept {
        std::size_t count{};
        for (const auto& contract : classes) {
            count += contract.fourthPageSkills.size();
        }
        return count;
    }
};

inline bool ParseToml(
        std::string_view input,
        Config& result,
        std::string& error) {
    try {
        const auto root = toml::parse(input);
        for (const auto& [key, value] : root) {
            (void)value;
            if (key != "config_version"
                    && key != "enabled"
                    && key != "diagnostics") {
                error = "unknown top-level setting or section: "
                    + std::string(key.str());
                return false;
            }
        }

        const auto* versionNode = root.get("config_version");
        if (!versionNode || !versionNode->is_integer()) {
            error = "config_version must be an integer";
            return false;
        }
        const auto version = versionNode->value<std::int64_t>();
        if (*version != ConfigVersion) {
            error = "config_version must be 1";
            return false;
        }

        const auto* enabledNode = root.get("enabled");
        if (!enabledNode || !enabledNode->is_boolean()) {
            error = "enabled must be true or false";
            return false;
        }
        const auto enabled = enabledNode->value<bool>();

        Config parsed{};
        parsed.enabled = *enabled;
        if (const auto* diagnosticsNode = root.get("diagnostics")) {
            const auto* diagnostics = diagnosticsNode->as_table();
            if (!diagnostics) {
                error = "diagnostics must be a TOML table";
                return false;
            }
            for (const auto& [key, value] : *diagnostics) {
                (void)value;
                if (key != "enabled") {
                    error = "unknown setting: diagnostics."
                        + std::string(key.str());
                    return false;
                }
            }
            const auto* diagnosticNode = diagnostics->get("enabled");
            if (!diagnosticNode || !diagnosticNode->is_boolean()) {
                error = "diagnostics.enabled must be true or false";
                return false;
            }
            const auto diagnosticEnabled = diagnosticNode->value<bool>();
            parsed.diagnostics = *diagnosticEnabled;
        }

        result = parsed;
        error.clear();
        return true;
    } catch (const toml::parse_error& exception) {
        error = exception.description();
        return false;
    }
}

namespace detail {

struct Table {
    std::vector<std::string> headers;
    std::unordered_map<std::string, std::size_t> headerIndexes;
    std::vector<std::vector<std::string>> rows;
};

inline bool ParseInteger(
        std::string_view text,
        std::int32_t& result) noexcept {
    if (text.empty()) return false;
    std::int32_t parsed{};
    const auto [end, status] = std::from_chars(
        text.data(), text.data() + text.size(), parsed);
    if (status != std::errc{} || end != text.data() + text.size()) return false;
    result = parsed;
    return true;
}

inline std::vector<std::string> SplitTabs(std::string_view line) {
    std::vector<std::string> fields;
    std::size_t start{};
    for (;;) {
        const auto tab = line.find('\t', start);
        fields.emplace_back(line.substr(
            start,
            tab == std::string_view::npos ? line.size() - start : tab - start));
        if (tab == std::string_view::npos) break;
        start = tab + 1;
    }
    return fields;
}

inline bool SplitLines(
        std::string_view input,
        std::vector<std::string_view>& lines,
        std::string& error) {
    lines.clear();
    if (input.empty()) {
        error = "table is empty";
        return false;
    }
    if (input.size() >= 3
            && static_cast<unsigned char>(input[0]) == 0xEF
            && static_cast<unsigned char>(input[1]) == 0xBB
            && static_cast<unsigned char>(input[2]) == 0xBF) {
        error = "UTF-8 BOM is not supported";
        return false;
    }
    if (input.find('\0') != std::string_view::npos) {
        error = "NUL bytes are not supported";
        return false;
    }

    std::size_t start{};
    while (start < input.size()) {
        const auto newline = input.find('\n', start);
        const auto end = newline == std::string_view::npos ? input.size() : newline;
        auto line = input.substr(start, end - start);
        if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
        if (line.find('\r') != std::string_view::npos) {
            error = "lone carriage return is not supported";
            return false;
        }
        if (line.empty()) {
            error = "blank lines are not supported";
            return false;
        }
        lines.emplace_back(line);
        if (newline == std::string_view::npos) break;
        start = newline + 1;
    }
    return !lines.empty();
}

inline bool ParseTable(
        std::string_view tableName,
        std::string_view input,
        std::initializer_list<std::string_view> requiredHeaders,
        Table& result,
        std::string& error) {
    if (input.size() > MaximumTableBytes) {
        error = std::string(tableName) + " exceeds the 4 MiB safety limit";
        return false;
    }

    std::vector<std::string_view> lines;
    if (!SplitLines(input, lines, error)) {
        error = std::string(tableName) + ": " + error;
        return false;
    }

    Table parsed{};
    parsed.headers = SplitTabs(lines.front());
    for (std::size_t index = 0; index < parsed.headers.size(); ++index) {
        if (parsed.headers[index].empty()) {
            error = std::string(tableName) + ": empty header at column "
                + std::to_string(index + 1);
            return false;
        }
        if (!parsed.headerIndexes.emplace(parsed.headers[index], index).second) {
            error = std::string(tableName) + ": duplicate header: "
                + parsed.headers[index];
            return false;
        }
    }
    for (const auto header : requiredHeaders) {
        if (!parsed.headerIndexes.contains(std::string(header))) {
            error = std::string(tableName) + ": missing required header: "
                + std::string(header);
            return false;
        }
    }

    parsed.rows.reserve(lines.size() - 1);
    for (std::size_t lineIndex = 1; lineIndex < lines.size(); ++lineIndex) {
        auto fields = SplitTabs(lines[lineIndex]);
        if (fields.size() != parsed.headers.size()) {
            error = std::string(tableName) + " line "
                + std::to_string(lineIndex + 1) + ": expected "
                + std::to_string(parsed.headers.size()) + " columns, found "
                + std::to_string(fields.size());
            return false;
        }
        parsed.rows.emplace_back(std::move(fields));
    }
    result = std::move(parsed);
    return true;
}

inline const std::string& Field(
        const Table& table,
        const std::vector<std::string>& row,
        std::string_view header) {
    return row[table.headerIndexes.at(std::string(header))];
}

struct SkillRow {
    std::string name;
    std::string classCode;
    std::string skillDesc;
    std::vector<std::string> prerequisites;
};

inline bool DetectCycles(
        const std::vector<Placement>& placements,
        std::string& error) {
    std::unordered_map<std::string, std::size_t> placementBySkill;
    for (std::size_t index = 0; index < placements.size(); ++index) {
        placementBySkill.emplace(placements[index].skill, index);
    }
    std::vector<std::uint8_t> colors(placements.size());
    std::vector<std::string> path;
    std::function<bool(std::size_t)> visit = [&](std::size_t index) {
        if (colors[index] == 2) return true;
        if (colors[index] == 1) {
            error = "fourth-page prerequisite cycle reaches skill: "
                + placements[index].skill;
            return false;
        }
        colors[index] = 1;
        path.emplace_back(placements[index].skill);
        for (const auto& prerequisite : placements[index].prerequisites) {
            const auto found = placementBySkill.find(prerequisite);
            if (found != placementBySkill.end() && !visit(found->second)) {
                return false;
            }
        }
        path.pop_back();
        colors[index] = 2;
        return true;
    };
    for (std::size_t index = 0; index < placements.size(); ++index) {
        if (!visit(index)) return false;
    }
    return true;
}

} // namespace detail

inline bool ValidateContract(
        std::string_view skillsText,
        std::string_view skillDescText,
        ContractSummary& result,
        std::string& error) {
    detail::Table skills{};
    detail::Table skillDesc{};
    if (!detail::ParseTable(
            "skills.txt",
            skillsText,
            {"skill", "charclass", "skilldesc", "reqskill1", "reqskill2", "reqskill3"},
            skills,
            error)
            || !detail::ParseTable(
                "skilldesc.txt",
                skillDescText,
                {"skilldesc", "SkillPage", "SkillRow", "SkillColumn"},
                skillDesc,
                error)) {
        return false;
    }

    std::vector<detail::SkillRow> skillRows;
    skillRows.reserve(skills.rows.size());
    std::unordered_map<std::string, std::size_t> skillByName;
    std::unordered_map<std::string, std::vector<std::size_t>> skillsByDescription;
    std::map<std::string, std::size_t> classCounts;
    for (const auto& row : skills.rows) {
        detail::SkillRow parsed{};
        parsed.name = detail::Field(skills, row, "skill");
        parsed.classCode = detail::Field(skills, row, "charclass");
        parsed.skillDesc = detail::Field(skills, row, "skilldesc");
        if (parsed.name.empty()) {
            error = "skills.txt contains an empty skill key";
            return false;
        }
        if (!skillByName.emplace(parsed.name, skillRows.size()).second) {
            error = "skills.txt contains a duplicate skill key: " + parsed.name;
            return false;
        }
        for (const auto prerequisiteHeader : {"reqskill1", "reqskill2", "reqskill3"}) {
            const auto& prerequisite = detail::Field(skills, row, prerequisiteHeader);
            if (!prerequisite.empty()) parsed.prerequisites.emplace_back(prerequisite);
        }
        if (!parsed.classCode.empty()) ++classCounts[parsed.classCode];
        if (!parsed.skillDesc.empty()) {
            skillsByDescription[parsed.skillDesc].emplace_back(skillRows.size());
        }
        skillRows.emplace_back(std::move(parsed));
    }
    for (const auto& [classCode, count] : classCounts) {
        if (count > MaximumClassSkills) {
            error = "class " + classCode + " has " + std::to_string(count)
                + " skills; the D2S byte contract allows at most 255";
            return false;
        }
    }

    std::set<std::string> descriptionKeys;
    std::vector<Placement> placements;
    std::set<std::string> occupiedCells;
    for (const auto& row : skillDesc.rows) {
        const auto& description = detail::Field(skillDesc, row, "skilldesc");
        if (description.empty()) {
            error = "skilldesc.txt contains an empty skilldesc key";
            return false;
        }
        if (!descriptionKeys.emplace(description).second) {
            error = "skilldesc.txt contains a duplicate skilldesc key: " + description;
            return false;
        }
        const auto& pageText = detail::Field(skillDesc, row, "SkillPage");
        if (pageText != "4") continue;

        const auto linked = skillsByDescription.find(description);
        if (linked == skillsByDescription.end()) {
            error = "fourth-page skilldesc is not referenced by skills.txt: "
                + description;
            return false;
        }
        if (linked->second.size() != 1) {
            error = "fourth-page skilldesc must map to exactly one skill: "
                + description;
            return false;
        }
        const auto& skill = skillRows[linked->second.front()];
        if (skill.classCode.empty()) {
            error = "fourth-page skill has no charclass: " + skill.name;
            return false;
        }

        Placement placement{};
        placement.skill = skill.name;
        placement.skillDesc = description;
        placement.classCode = skill.classCode;
        placement.prerequisites = skill.prerequisites;
        if (!detail::ParseInteger(
                detail::Field(skillDesc, row, "SkillRow"), placement.row)
                || placement.row < MinimumRow || placement.row > MaximumRow) {
            error = "fourth-page skill " + skill.name
                + " must use SkillRow 1 through 6";
            return false;
        }
        if (!detail::ParseInteger(
                detail::Field(skillDesc, row, "SkillColumn"), placement.column)
                || placement.column < MinimumColumn
                || placement.column > MaximumColumn) {
            error = "fourth-page skill " + skill.name
                + " must use SkillColumn 1 through 3";
            return false;
        }
        const auto cell = placement.classCode + ":"
            + std::to_string(placement.row) + ":"
            + std::to_string(placement.column);
        if (!occupiedCells.emplace(cell).second) {
            error = "duplicate fourth-page cell: " + cell;
            return false;
        }
        placements.emplace_back(std::move(placement));
    }

    for (const auto& placement : placements) {
        for (const auto& prerequisite : placement.prerequisites) {
            const auto found = skillByName.find(prerequisite);
            if (found == skillByName.end()) {
                error = "fourth-page skill " + placement.skill
                    + " references an unknown prerequisite: " + prerequisite;
                return false;
            }
            const auto& prerequisiteClass = skillRows[found->second].classCode;
            if (!prerequisiteClass.empty()
                    && prerequisiteClass != placement.classCode) {
                error = "fourth-page skill " + placement.skill
                    + " references a skill from another class: " + prerequisite;
                return false;
            }
        }
    }
    if (!detail::DetectCycles(placements, error)) return false;

    ContractSummary summary{};
    summary.skillRows = skills.rows.size();
    summary.skillDescRows = skillDesc.rows.size();
    for (const auto& [classCode, totalSkills] : classCounts) {
        ClassContract contract{};
        contract.classCode = classCode;
        contract.totalSkills = totalSkills;
        for (const auto& placement : placements) {
            if (placement.classCode == classCode) {
                contract.fourthPageSkills.emplace_back(placement);
            }
        }
        if (contract.fourthPageSkills.size() > MaximumFourthPageSkills) {
            error = "class " + classCode + " has "
                + std::to_string(contract.fourthPageSkills.size())
                + " fourth-page skills; contract version 1 allows at most 18";
            return false;
        }
        summary.classes.emplace_back(std::move(contract));
    }
    result = std::move(summary);
    error.clear();
    return true;
}

} // namespace ruffneckk::fourth_skill_tree
