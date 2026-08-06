#include "tooltip_ranges.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using tcp::tooltips::AppendConsensusRanges;
using tcp::tooltips::ModifierRange;

namespace {

using AuditRow = std::unordered_map<std::string, std::string>;

std::string Lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::vector<std::string> SplitTabs(std::string line) {
    if (!line.empty() && line.back() == '\r') line.pop_back();
    std::vector<std::string> fields;
    std::size_t start{};
    while (true) {
        const auto tab = line.find('\t', start);
        fields.emplace_back(line.substr(start, tab - start));
        if (tab == std::string::npos) break;
        start = tab + 1;
    }
    return fields;
}

std::vector<AuditRow> ReadAuditRows(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    std::string line;
    if (!std::getline(input, line)) return {};
    auto headers = SplitTabs(std::move(line));
    for (auto& header : headers) header = Lower(std::move(header));
    std::vector<AuditRow> rows;
    while (std::getline(input, line)) {
        const auto values = SplitTabs(std::move(line));
        AuditRow row;
        for (std::size_t index = 0; index < headers.size() && index < values.size(); ++index)
            row.emplace(headers[index], values[index]);
        rows.emplace_back(std::move(row));
    }
    return rows;
}

std::string ReadBinaryText(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

std::string TableWithSingleRow(const std::string& source,
    const std::unordered_map<std::string, std::string>& values) {
    const auto end = source.find('\n');
    if (end == std::string::npos) return {};
    auto header = source.substr(0, end);
    if (!header.empty() && header.back() == '\r') header.pop_back();
    auto fields = SplitTabs(header);
    std::vector<std::string> row(fields.size());
    for (std::size_t index = 0; index < fields.size(); ++index) {
        const auto found = values.find(Lower(fields[index]));
        if (found != values.end()) row[index] = found->second;
    }
    std::string result = header + "\r\n";
    for (std::size_t index = 0; index < row.size(); ++index) {
        if (index) result.push_back('\t');
        result += row[index];
    }
    result += "\r\n";
    return result;
}

bool AppendTableRow(std::string& table,
    const std::unordered_map<std::string, std::string>& values) {
    const auto single = TableWithSingleRow(table, values);
    const auto row = single.find('\n');
    if (row == std::string::npos) return false;
    if (!table.empty() && table.back() != '\n') table += "\r\n";
    table += single.substr(row + 1);
    return true;
}

std::string Value(const AuditRow& row, std::string_view key) {
    const auto found = row.find(std::string(key));
    return found == row.end() ? std::string{} : found->second;
}

std::int32_t AuditNumber(const AuditRow& row, std::string_view key) {
    const auto value = Value(row, key);
    std::int32_t result{};
    if (value.empty()) return result;
    const auto parsed = std::from_chars(value.data(), value.data() + value.size(), result);
    return parsed.ec == std::errc{} && parsed.ptr == value.data() + value.size() ? result : 0;
}

std::size_t CompiledAffixCount(const std::filesystem::path& path) {
    std::size_t count{};
    for (const auto& row : ReadAuditRows(path))
        if (Lower(Value(row, "name")) != "expansion") ++count;
    return count;
}

std::size_t CompiledAffixId(const std::filesystem::path& path, std::string_view name) {
    std::size_t id = 1; // Runtime affix id zero means no affix.
    for (const auto& row : ReadAuditRows(path)) {
        if (Lower(Value(row, "name")) == "expansion") continue;
        if (Value(row, "name") == name) return id;
        ++id;
    }
    return 0;
}

std::size_t CompiledAffixIdForType(const std::filesystem::path& path,
    std::string_view name, std::string_view itemType) {
    std::size_t id = 1;
    for (const auto& row : ReadAuditRows(path)) {
        if (Lower(Value(row, "name")) == "expansion") continue;
        bool typeMatches{};
        for (std::size_t slot = 1; slot <= 7; ++slot)
            if (Value(row, "itype" + std::to_string(slot)) == itemType) typeMatches = true;
        if (Value(row, "name") == name && typeMatches) return id;
        ++id;
    }
    return 0;
}

std::size_t CompiledItemCount(const std::filesystem::path& path) {
    std::size_t count{};
    for (const auto& row : ReadAuditRows(path))
        if (Lower(Value(row, "index")) != "expansion") ++count;
    return count;
}

bool AuditEveryUniqueRecord(const std::filesystem::path& excelDirectory) {
    const auto rows = ReadAuditRows(excelDirectory / "uniqueitems.txt");
    if (rows.empty()) return false;

    tcp::tooltips::RangeCatalog catalog;
    std::string error;
    if (!catalog.Load(excelDirectory, error)) return false;

    std::size_t recordCount{};
    std::size_t blankSectionCount{};
    std::size_t runtimeId{};
    for (const auto& row : rows) {
        if (Lower(Value(row, "index")) == "expansion") continue;
        if (Value(row, "index").empty()) {
            ++blankSectionCount;
        } else {
            ++recordCount;
        }

        tcp::tooltips::ItemAffixIds unique{};
        unique.quality = 7;
        unique.fileIndex = static_cast<std::uint32_t>(runtimeId);
        const auto candidates = catalog.ResolveCandidates(unique, Value(row, "code"));
        if (candidates.size() != 1) return false;
        ++runtimeId;
    }

    // Governed BKVince 3.2 inventory: *ID is deliberately ignored. The named
    // Expansion delimiter is not compiled, while the remaining blank section
    // rows consume the same runtime fileIndex slots as D2R.
    return recordCount == 507
        && blankSectionCount == 0
        && runtimeId == 507;
}

bool AuditEveryRunewordRecord(const std::filesystem::path& excelDirectory,
    const tcp::tooltips::RangeCatalog& catalog) {
    const auto rows = ReadAuditRows(excelDirectory / "runes.txt");
    std::unordered_set<std::string> expected;
    for (const auto& row : rows) {
        if (AuditNumber(row, "complete") == 0) continue;
        const auto key = Value(row, "name");
        if (key.empty() || !expected.insert(key).second) return false;
    }
    const std::unordered_set<std::string> actual(
        catalog.RunewordKeys().begin(), catalog.RunewordKeys().end());
    return expected.size() == 113 && actual == expected;
}

std::string RenderAuditLine(const ModifierRange& range) {
    std::string line = range.anchor;
    const auto roll = std::to_string(range.minimum);
    std::size_t position{};
    while ((position = line.find('#', position)) != std::string::npos) {
        line.replace(position, 1, roll);
        position += roll.size();
    }
    if (!range.parameter.empty()) line += " " + range.parameter;
    return line;
}

bool AuditEveryRunewordAgainstMetadata(const tcp::tooltips::RangeCatalog& catalog) {
    constexpr std::array metadata{
        "Defense: 689", "One-Hand Damage: 12 to 34",
        "Two-Hand Damage: 25 to 80", "Throw Damage: 20 to 60",
        "Base Defense: 133 [128 - 135]", "Chance to Block: 57%",
        "Max Sockets: 4", "Durability: 33 of 55",
        "Required Strength: 110", "Required Dexterity: 35",
        "Required Level: 47", "Item Level: 99", "Affix Level: 85",
        "Socketed (4)"};
    tcp::tooltips::ItemAffixIds ids{};
    ids.quality = 2;
    ids.runeword = true;
    ids.magicPrefix[0] = 1389;
    ids.magicPrefix[1] = 980;
    ids.magicPrefix[2] = 1349;
    ids.magicSuffix[0] = 745;
    ids.magicSuffix[1] = 742;
    ids.magicSuffix[2] = 195;
    std::size_t comparisons{};
    for (const auto& key : catalog.RunewordKeys()) {
        for (const auto code : {"hax", "gth", "buc"}) {
            const auto candidates = catalog.ResolveCandidates(ids, code, key);
            if (candidates.size() != 1) return false;
            for (const auto& range : candidates.front()) {
                if (range.minimum == range.maximum || range.anchor.empty()) continue;
                const auto line = RenderAuditLine(range);
                const auto baseline = AppendConsensusRanges(line, candidates);
                for (const auto metadataLine : metadata) {
                    const auto withMetadata = AppendConsensusRanges(
                        std::string(metadataLine) + "\n" + line, candidates);
                    const auto lastLine = withMetadata.substr(withMetadata.rfind('\n') + 1);
                    if (lastLine != baseline) {
                        std::cerr << "Runeword metadata collision: " << key
                            << " code=" << code << " range=" << range.key
                            << " metadata=" << metadataLine << " line=" << line
                            << " baseline=" << baseline << " actual=" << lastLine << "\n";
                        return false;
                    }
                }
                ++comparisons;
            }
        }
    }
    return comparisons > 500;
}

const ModifierRange* FindRange(
    const std::vector<std::vector<ModifierRange>>& candidates,
    std::string_view key) {
    for (const auto& candidate : candidates)
        for (const auto& range : candidate)
            if (range.key == key) return &range;
    return nullptr;
}

} // namespace

int main(int argc, char** argv) {
#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)
    if (argc == 3 && std::string_view(argv[1]) == "--provider-smoke") {
        constexpr auto blue = "\xEE\x81\xBE" "3";
        constexpr auto darkGreen = "\xEE\x81\xBE" ":";
        const auto excel = std::filesystem::path(argv[2]);
        const auto syntheticUniqueId = CompiledItemCount(excel / "uniqueitems.txt");
        const auto syntheticSetId = CompiledItemCount(excel / "setitems.txt");
        tcp::tooltips::RangeCatalog catalog;
        std::string error;
        const auto vanilla = [&](std::string_view tableName, std::string& text,
                                 std::string& loadError) {
            const auto path = excel / tableName;
            std::ifstream input(path, std::ios::binary);
            if (!input) {
                loadError = "Cannot open " + path.string();
                return false;
            }
            text.assign(std::istreambuf_iterator<char>(input),
                std::istreambuf_iterator<char>());
            if (tableName == "properties.txt") {
                // Public tables frequently omit optional comment columns. One
                // synthetic property also uses a second function/stat slot,
                // which older releases silently discarded.
                if (!AppendTableRow(text, {
                        {"code", "public-multi"}, {"*tooltip", ""},
                        {"func1", "1"}, {"stat1", "item_fastercastrate"},
                        {"func2", "1"}, {"stat2", "maxhp"}
                    })
                    || !AppendTableRow(text, {
                        {"code", "public-fcr"}, {"*tooltip", ""},
                        {"func1", "1"}, {"stat1", "item_fastercastrate"}
                    })
                    || !AppendTableRow(text, {
                        {"code", "public-class"}, {"*tooltip", ""},
                        {"func1", "21"}, {"stat1", "item_addclassskills"},
                        {"val1", "1"}
                    })) {
                    loadError = "Cannot synthesize public property regression rows";
                    return false;
                }
            } else if (tableName == "uniqueitems.txt") {
                // Deliberately duplicate *ID=0. The appended runtime fileIndex
                // is derived from compiled row order, never the comment value.
                if (!AppendTableRow(text, {
                        {"index", "Public Unique Regression"}, {"*id", "0"},
                        {"code", "amu"},
                        {"prop1", "public-multi"}, {"min1", "12"}, {"max1", "18"},
                        {"prop2", "public-class"}, {"min2", "2"}, {"max2", "3"}
                    })) {
                    loadError = "Cannot synthesize public unique regression row";
                    return false;
                }
            } else if (tableName == "setitems.txt") {
                // add func 2 exposes cumulative piece-count groups. Its stale
                // *ID must not hide the newly appended set item.
                if (!AppendTableRow(text, {
                        {"index", "Public Set Regression"}, {"*id", "0"},
                        {"item", "amu"}, {"add func", "2"},
                        {"prop1", "public-fcr"}, {"min1", "10"}, {"max1", "20"},
                        {"aprop1a", "public-fcr"}, {"amin1a", "5"}, {"amax1a", "10"},
                        {"aprop2a", "public-fcr"}, {"amin2a", "5"}, {"amax2a", "10"}
                    })) {
                    loadError = "Cannot synthesize public set regression row";
                    return false;
                }
            } else if (tableName == "runes.txt") {
                // Public mods may intentionally reuse one localization key for
                // several active runeword variants. Add a synthetic second
                // Call to Arms record with a distinct ED roll and the same Ohm
                // contribution to prove that the catalog remains available and
                // lets whole-tooltip validation select the matching variant.
                const auto variant = TableWithSingleRow(text, {
                    {"name", "Runeword13"}, {"complete", "1"},
                    {"rune5", "r27"},
                    {"t1code1", "dmg%"}, {"t1min1", "300"}, {"t1max1", "340"}
                });
                const auto row = variant.find('\n');
                if (row == std::string::npos) {
                    loadError = "Cannot synthesize duplicate runeword regression row";
                    return false;
                }
                if (!text.empty() && text.back() != '\n') text += "\r\n";
                text += variant.substr(row + 1);
            }
            return true;
        };
        std::size_t physicalLoads{};
        std::size_t fallbackLoads{};
        const std::vector<std::filesystem::path> cosmeticPackage{
            excel / "__cosmetic_package_without_excel_tables__"
        };
        const auto provider = [&](std::string_view tableName, std::string& text,
                                  std::string& loadError) {
            return tcp::tooltips::RangeCatalog::LoadLayeredTable(
                cosmeticPackage, vanilla, tableName, text, loadError,
                physicalLoads, fallbackLoads);
        };
        CHECK(catalog.Load(provider, error));
        CHECK(physicalLoads == 0);
        CHECK(fallbackLoads > 0);
        CHECK(catalog.PropertyCount() > 0);
        CHECK(catalog.FindArmor("ci3").has_value());
        CHECK(!catalog.RunewordKeys().empty());
        CHECK(std::count(catalog.RunewordKeys().begin(), catalog.RunewordKeys().end(),
            "Runeword13") == 1);
        tcp::tooltips::ItemAffixIds runeword{};
        runeword.quality = 2;
        runeword.runeword = true;
        const auto variants = catalog.ResolveCandidates(runeword, "hax", "Runeword13");
        CHECK(variants.size() == 2);
        const auto ordinary = AppendConsensusRanges(
            blue + std::string("+275% Enhanced Damage"), variants);
        const auto alternate = AppendConsensusRanges(
            blue + std::string("+375% Enhanced Damage"), variants);
        CHECK(ordinary.find("[250 - 290]") != std::string::npos);
        CHECK(alternate.find("[350 - 390]") != std::string::npos);

        tcp::tooltips::ItemAffixIds publicUnique{};
        publicUnique.quality = 7;
        publicUnique.fileIndex = static_cast<std::uint32_t>(syntheticUniqueId);
        const auto publicUniqueCandidates = catalog.ResolveCandidates(publicUnique, "amu");
        CHECK(publicUniqueCandidates.size() == 1);
        CHECK(FindRange(publicUniqueCandidates, "item_fastercastrate"));
        CHECK(FindRange(publicUniqueCandidates, "item_fastercastrate")->minimum == 12);
        CHECK(FindRange(publicUniqueCandidates, "maxhp"));
        CHECK(FindRange(publicUniqueCandidates, "maxhp")->maximum == 18);
        CHECK(FindRange(publicUniqueCandidates,
            "item_addclassskills:public-class:1"));

        const auto publicLocalization = catalog.BuildLocalization(
            [](std::string_view key) {
                if (key == "ModStr4v") return std::string("+%d%% Faster Cast Rate");
                if (key == "ModStr1u") return std::string("+%d to Life");
                if (key == "ModStr3a") return std::string("+%d to %s Skill Levels");
                if (key == "ItemStats1h") return std::string("Defense: %d");
                if (key == "ItemStast1k") return std::string("to");
                return std::string{};
            });
        const auto publicUniqueTooltip = blue + std::string("+15% Faster Cast Rate\n")
            + blue + "+15 to Life\n"
            + blue + "+2 to Sorceress Skill Levels";
        const auto publicUniqueEnhanced = AppendConsensusRanges(
            publicUniqueTooltip, publicUniqueCandidates, false, &publicLocalization);
        CHECK(publicUniqueEnhanced.find("+15% Faster Cast Rate "
            + std::string(darkGreen) + "[12 - 18]" + blue) != std::string::npos);
        CHECK(publicUniqueEnhanced.find("+15 to Life "
            + std::string(darkGreen) + "[12 - 18]" + blue) != std::string::npos);
        CHECK(publicUniqueEnhanced.find("+2 to Sorceress Skill Levels "
            + std::string(darkGreen) + "[2 - 3]" + blue) != std::string::npos);

        tcp::tooltips::ItemAffixIds publicSet{};
        publicSet.quality = 5;
        publicSet.fileIndex = static_cast<std::uint32_t>(syntheticSetId);
        const auto publicSetCandidates = catalog.ResolveCandidates(publicSet, "amu");
        CHECK(publicSetCandidates.size() == 6);
        const auto publicSetEnhanced = AppendConsensusRanges(
            blue + std::string("+35% Faster Cast Rate"), publicSetCandidates,
            false, &publicLocalization);
        CHECK(publicSetEnhanced.find("[20 - 40]") != std::string::npos);

        tcp::tooltips::RangeCatalog physicalCatalog;
        std::size_t completePhysicalLoads{};
        std::size_t completeFallbackLoads{};
        const auto completePhysicalProvider = [&](std::string_view tableName,
                                                  std::string& text,
                                                  std::string& loadError) {
            return tcp::tooltips::RangeCatalog::LoadLayeredTable(
                {excel}, vanilla, tableName, text, loadError,
                completePhysicalLoads, completeFallbackLoads);
        };
        CHECK(physicalCatalog.Load(completePhysicalProvider, error));
        CHECK(completePhysicalLoads > 0);
        CHECK(completeFallbackLoads == 0);

        std::string tableText;
        std::size_t physicalOnlyLoads{};
        std::size_t physicalFallbackLoads{};
        CHECK(tcp::tooltips::RangeCatalog::LoadLayeredTable(
            {excel}, vanilla, "properties.txt", tableText, error,
            physicalOnlyLoads, physicalFallbackLoads));
        CHECK(physicalOnlyLoads == 1);
        CHECK(physicalFallbackLoads == 0);

        const auto binaryOnly = std::filesystem::temp_directory_path()
            / "advanced-item-tooltips-binary-only";
        std::filesystem::create_directories(binaryOnly);
        const auto binaryTable = binaryOnly / "properties.bin";
        {
            std::ofstream output(binaryTable, std::ios::binary | std::ios::trunc);
            output.put('\0');
        }
        std::size_t blockedPhysicalLoads{};
        std::size_t blockedFallbackLoads{};
        error.clear();
        CHECK(!tcp::tooltips::RangeCatalog::LoadLayeredTable(
            {binaryOnly}, vanilla, "properties.txt", tableText, error,
            blockedPhysicalLoads, blockedFallbackLoads));
        CHECK(error.find("modified binary table without matching TXT")
            != std::string::npos);
        CHECK(blockedPhysicalLoads == 0);
        CHECK(blockedFallbackLoads == 0);
        std::filesystem::remove(binaryTable);
        std::filesystem::remove(binaryOnly);
        return 0;
    }
    constexpr auto blue = "\xEE\x81\xBE" "3";
    constexpr auto darkGreen = "\xEE\x81\xBE" ":";
    const std::string tooltip = blue + std::string("+18% Faster Cast Rate");
    const std::vector<std::vector<ModifierRange>> stacked{{
        {"item_fastercastrate", "+#% Faster Cast Rate", 15, 20, 142}
    }};
    CHECK(tcp::tooltips::FirstSignedInteger(tooltip) == 18);

    CHECK(tcp::tooltips::ExactFlatDefenseTotal(
        blue + std::string("+180 Defense ") + darkGreen + "[150 - 220]" + blue) == 180);
    CHECK(tcp::tooltips::ExactFlatDefenseTotal(
        "Defense: 148\n+250 Defense vs. Missile") == 0);
    CHECK(tcp::tooltips::ExactFlatDefenseTotal(
        "+180 Defense\n+250 Defense vs. Missile\n+61 Defense (Based on Character Level)") == 180);
    CHECK(tcp::tooltips::ExactEnhancedDefensePercent(
        blue + std::string("+120% Enhanced Defense ") + darkGreen + "[90 - 120]" + blue) == 120);
    CHECK(tcp::tooltips::ExactEnhancedDefensePercent(
        "Defense: 352\n+250 Defense vs. Missile") == 0);
    CHECK(!tcp::tooltips::ExactEnhancedDefensePercent(
        "+50% Enhanced Defense\n+20% Enhanced Defense"));

    // Native localization must drive matching; no English tooltip label is
    // required once D2R resolves the active language's format strings.
    const std::unordered_map<std::string, std::vector<std::string>> koreanStatKeys{
        {"item_fasterattackrate", {"ModStr4m"}},
        {"strength", {"ModStr1a"}},
        {"armorclass", {"ModStr1i"}},
        {"item_armor_percent", {"Modstr2v"}},
        {"firemindam", {"ModStr1p"}},
        {"firemaxdam", {"ModStr1o"}}
    };
    const std::unordered_map<std::string, std::string> koreanStrings{
        {"ItemStats1h", "방어력: %d"},
        {"ItemStats1e", "필요 힘: %d"},
        {"ModStr4m", "공격 속도 %+d%%"},
        {"ModStr1a", "힘 %+d"},
        {"ModStr1i", "방어력 %+d"},
        {"Modstr2v", "방어력 %+d%% 증가"},
        {"strModFireDamageRange", "화염 피해 %d - %d 추가"},
        {"ItemStast1k", "-"}
    };
    const auto korean = tcp::tooltips::BuildTooltipLocalization(
        koreanStatKeys,
        [&](std::string_view key) {
            const auto found = koreanStrings.find(std::string(key));
            return found == koreanStrings.end() ? std::string{} : found->second;
        });
    const std::vector<std::vector<ModifierRange>> koreanIas{{
        {"item_fasterattackrate", "+#% Increased Attack Speed", 3, 5, 145,
            "", "item_fasterattackrate"}
    }};
    const auto koreanIasTooltip = blue + std::string("공격 속도 +4%");
    CHECK(AppendConsensusRanges(koreanIasTooltip, koreanIas, false, &korean)
        .find("[3 - 5]") != std::string::npos);
    const std::vector<std::vector<ModifierRange>> koreanStrength{{
        {"strength", "+# to Strength", 5, 10, 0, "", "strength"}
    }};
    CHECK(AppendConsensusRanges("필요 힘: 110", koreanStrength, false, &korean)
        == "필요 힘: 110");
    CHECK(tcp::tooltips::ExactFlatDefenseTotal(
        blue + std::string("방어력 +180 ") + darkGreen + "[150 - 220]" + blue,
        &korean) == 180);
    CHECK(tcp::tooltips::ExactFlatDefenseTotal(
        "방어력: 352\n방어력 +250 / 투사체", &korean) == 0);
    CHECK(tcp::tooltips::ExactEnhancedDefensePercent(
        blue + std::string("방어력 +120% 증가 ") + darkGreen + "[90 - 120]" + blue,
        &korean) == 120);
    const std::vector<std::vector<ModifierRange>> koreanFireDamage{{
        {"firemindam", "Adds #-# Fire Damage", 11, 25, 0, "", "firemindam"},
        {"firemaxdam", "Adds #-# Fire Damage", 31, 50, 0, "", "firemaxdam"}
    }};
    const auto koreanFireTooltip = AppendConsensusRanges(
        blue + std::string("화염 피해 19 - 41 추가"), koreanFireDamage, false, &korean);
    CHECK(koreanFireTooltip.find("[11 - 25]") != std::string::npos);
    CHECK(koreanFireTooltip.find("[31 - 50]") != std::string::npos);

    const auto simplifiedChinese = tcp::tooltips::BuildTooltipLocalization(
        {{"item_fasterattackrate", {"ModStr4m"}}},
        [](std::string_view key) {
            if (key == "ItemStats1h") return std::string("防御: %d");
            if (key == "ModStr4m") return std::string("%+d%% 提高攻击速度");
            if (key == "ItemStast1k") return std::string("至");
            return std::string{};
        });
    CHECK(AppendConsensusRanges(
        blue + std::string("+4% 提高攻击速度"), koreanIas, false, &simplifiedChinese)
        .find("[3 - 5]") != std::string::npos);

    // Ordinary, inherent-ED, flat-defense, runeword and ethereal-superior
    // armor all use the same reconstruction path. The inherent ED sentinel
    // (table max + 1) is mapped back into the visible table range.
    CHECK(tcp::tooltips::ReconstructBaseDefense(148, 0, 0, 133, 148, false) == 148);
    CHECK(tcp::tooltips::ReconstructBaseDefense(352, 120, 0, 114, 159, false) == 159);
    CHECK(tcp::tooltips::ReconstructBaseDefense(251, 55, 180, 25, 45, false) == 45);
    CHECK(tcp::tooltips::ReconstructBaseDefense(689, 411, 0, 128, 135, false) == 135);
    CHECK(tcp::tooltips::ReconstructBaseDefense(39, 47, 0, 14, 17, true) == 25);
    CHECK(!tcp::tooltips::ReconstructBaseDefense(147, 0, 250, 133, 148, false));

    const auto enhanced = AppendConsensusRanges(tooltip, stacked);
    CHECK(enhanced.find(darkGreen + std::string("[15 - 20]") + blue) != std::string::npos);

    const std::vector<std::vector<ModifierRange>> disagreement{
        {{"item_fastercastrate", "+#% Faster Cast Rate", 5, 10, 142}},
        {{"item_fastercastrate", "+#% Faster Cast Rate", 5, 15, 142}}
    };
    CHECK(AppendConsensusRanges(tooltip, disagreement) == tooltip);

    // Recipe selection must use the complete tooltip, not each line in
    // isolation. Mana 83 fits both the standalone 61-90 affix and the Caster
    // aggregate 71-110, while FCR 15 identifies the Caster recipe globally.
    const std::vector<std::vector<ModifierRange>> craftedCandidates{
        {
            {"item_fastercastrate", "+#% Faster Cast Rate", 10, 10, 142},
            {"mana", "+# to Mana", 61, 90, 0}
        },
        {
            {"item_fastercastrate", "+#% Faster Cast Rate", 15, 20, 142},
            {"mana", "+# to Mana", 71, 110, 0}
        }
    };
    const auto craftedTooltip = blue + std::string("+15% Faster Cast Rate\n")
        + blue + "+83 to Mana";
    const auto craftedEnhanced = AppendConsensusRanges(craftedTooltip, craftedCandidates);
    CHECK(craftedEnhanced.find(
        "+15% Faster Cast Rate " + std::string(darkGreen) + "[15 - 20]" + blue)
        != std::string::npos);
    CHECK(craftedEnhanced.find(
        "+83 to Mana " + std::string(darkGreen) + "[71 - 110]" + blue)
        != std::string::npos);
    CHECK(craftedEnhanced.find("[61 - 90]") == std::string::npos);

    // Public Cube-history example: native IAS 3-5 plus a recipe IAS 2-6
    // yields 5-11. A final roll of 7 proves the combined history. At 5 the
    // recipe history remains ambiguous, so preserve the intrinsic 3-5 range
    // instead of making identical item records appear inconsistently modeled.
    const std::vector<std::vector<ModifierRange>> markerlessIasCandidates{
        {{"item_fasterattackrate", "+#% Increased Attack Speed", 3, 5, 100}},
        {{"item_fasterattackrate", "+#% Increased Attack Speed", 5, 11, 100}}
    };
    const auto provenIas = AppendConsensusRanges(
        blue + std::string("+7% Increased Attack Speed"), markerlessIasCandidates);
    CHECK(provenIas.find("[5 - 11]") != std::string::npos);
    const std::vector<std::vector<ModifierRange>> intrinsicIasCandidates{
        {{"item_fasterattackrate", "+#% Increased Attack Speed", 3, 5, 100}}
    };
    const auto ambiguousIas = AppendConsensusRanges(
        blue + std::string("+5% Increased Attack Speed"), markerlessIasCandidates,
        false, nullptr, &intrinsicIasCandidates);
    CHECK(ambiguousIas.find("[3 - 5]") != std::string::npos);
    CHECK(ambiguousIas.find("[5 - 11]") == std::string::npos);

    // Two copies of the same unique must not randomly lose their intrinsic
    // range merely because an unrelated markerless recipe can overlap it.
    const std::vector<std::vector<ModifierRange>> overlappingUniqueCandidates{
        {{"enhanced_damage", "+#% Enhanced Damage", 80, 100, 130}},
        {{"enhanced_damage", "+#% Enhanced Damage", 85, 105, 130}}
    };
    const std::vector<std::vector<ModifierRange>> intrinsicUniqueCandidates{
        {{"enhanced_damage", "+#% Enhanced Damage", 80, 100, 130}}
    };
    for (const auto roll : {93, 96}) {
        const auto copy = AppendConsensusRanges(blue + std::string("+")
            + std::to_string(roll) + "% Enhanced Damage",
            overlappingUniqueCandidates, false, nullptr, &intrinsicUniqueCandidates);
        CHECK(copy.find("[80 - 100]") != std::string::npos);
        CHECK(copy.find("[85 - 105]") == std::string::npos);
    }

    const std::vector<std::vector<ModifierRange>> doubled{{
        {"enhanced_damage", "+#% Enhanced Damage", 150, 300, 130}
    }};
    CHECK(AppendConsensusRanges(blue + std::string("+250% Enhanced Damage"), doubled)
        .find("[150 - 300]") != std::string::npos);

    constexpr auto legacyBlue = "\xFF" "c3";
    constexpr auto utf8LegacyBlue = "\xC3\xBF" "c3";
    const std::vector<std::vector<ModifierRange>> rareRanges{{
        {"item_tohit_undead", "+# to Attack Rating against Undead", 25, 75, 120},
        {"item_damage_undead", "+#% Damage to Undead", 25, 75, 121},
        {"enhanced_damage", "+#% Enhanced Damage", 10, 20, 130}
    }};
    // Native rare modifiers use one legacy blue marker, followed by lines that
    // inherit it in D2's bottom-to-top tooltip buffer.
    const auto rareTooltip = std::string(legacyBlue) + "+32 to Attack Rating against Undead\n"
        + "+51% Damage to Undead\n"
        + "+1 to Maximum Damage\n"
        + "+19% Enhanced Damage";
    const auto rareEnhanced = AppendConsensusRanges(rareTooltip, rareRanges);
    CHECK(rareEnhanced.find(
        darkGreen + std::string("[25 - 75]") + blue + "\n+51% Damage to Undead")
        != std::string::npos);
    CHECK(rareEnhanced.find(
        "+51% Damage to Undead " + std::string(darkGreen) + "[25 - 75]" + blue)
        != std::string::npos);
    CHECK(rareEnhanced.find(
        "+19% Enhanced Damage " + std::string(darkGreen) + "[10 - 20]" + blue)
        != std::string::npos);
    const auto utf8Enhanced = AppendConsensusRanges(
        std::string(utf8LegacyBlue) + "+19% Enhanced Damage", rareRanges);
    CHECK(utf8Enhanced.find(darkGreen + std::string("[10 - 20]") + blue)
        != std::string::npos);

    // A socketed comparison tooltip must retain the ranges of the item's own
    // affixes. The native Socketed line and comparison footer are not affix
    // rolls and must not invalidate the candidate.
    const std::vector<std::vector<ModifierRange>> socketedRareRanges{{
        {"enhanced_damage", "+#% Enhanced Damage", 10, 20, 130},
        {"maxdamage", "+# to Maximum Damage", 1, 1, 71},
        {"item_damage_undead", "+#% Damage to Undead", 25, 75, 121},
        {"item_tohit_undead", "+# to Attack Rating against Undead", 25, 75, 120}
    }};
    const auto socketedRareTooltip = blue + std::string("+19% Enhanced Damage\n")
        + blue + "+1 to Maximum Damage\n"
        + blue + "+51% Damage to Undead\n"
        + blue + "+32 to Attack Rating against Undead\n"
        + blue + "Socketed (2)\n"
        + "Currently Equipped";
    const auto socketedRareEnhanced = AppendConsensusRanges(
        socketedRareTooltip, socketedRareRanges);
    CHECK(socketedRareEnhanced.find("+19% Enhanced Damage "
        + std::string(darkGreen) + "[10 - 20]" + blue) != std::string::npos);
    CHECK(socketedRareEnhanced.find("+51% Damage to Undead "
        + std::string(darkGreen) + "[25 - 75]" + blue) != std::string::npos);
    CHECK(socketedRareEnhanced.find("+32 to Attack Rating against Undead "
        + std::string(darkGreen) + "[25 - 75]" + blue) != std::string::npos);

    // A socketed jewel can raise a parent stat beyond the rare affix's own
    // range. Those combined lines remain unannotated until socket contents are
    // modeled, but they must not erase ranges from independent parent affixes.
    const auto jeweledRareTooltip = blue + std::string("+55% Enhanced Damage\n")
        + blue + "+13 to Maximum Damage\n"
        + blue + "+51% Damage to Undead\n"
        + blue + "+32 to Attack Rating against Undead\n"
        + blue + "Socketed (2)";
    const auto jeweledRareEnhanced = AppendConsensusRanges(
        jeweledRareTooltip, socketedRareRanges);
    CHECK(jeweledRareEnhanced.find("+55% Enhanced Damage "
        + std::string(darkGreen)) == std::string::npos);
    CHECK(jeweledRareEnhanced.find("+13 to Maximum Damage "
        + std::string(darkGreen)) == std::string::npos);
    CHECK(jeweledRareEnhanced.find("+51% Damage to Undead "
        + std::string(darkGreen) + "[25 - 75]" + blue) != std::string::npos);
    CHECK(jeweledRareEnhanced.find("+32 to Attack Rating against Undead "
        + std::string(darkGreen) + "[25 - 75]" + blue) != std::string::npos);

    // Public intrinsic-only mode knows that the visible aggregate contains
    // excluded socket fillers. Keep the parent's own range even when the
    // aggregate roll is outside it; filler-only properties still have no
    // parent candidate and therefore remain unannotated.
    const auto intrinsicJeweledRareEnhanced = AppendConsensusRanges(
        jeweledRareTooltip, socketedRareRanges, true);
    CHECK(intrinsicJeweledRareEnhanced.find("+55% Enhanced Damage "
        + std::string(darkGreen) + "[10 - 20]" + blue) != std::string::npos);
    CHECK(intrinsicJeweledRareEnhanced.find("+13 to Maximum Damage "
        + std::string(darkGreen)) == std::string::npos);
    CHECK(intrinsicJeweledRareEnhanced.find("+51% Damage to Undead "
        + std::string(darkGreen) + "[25 - 75]" + blue) != std::string::npos);

    // Once the socket filler is available, its own rare-affix ranges stack
    // with the parent item before tooltip consensus is calculated.
    const std::vector<std::vector<ModifierRange>> rareJewelRanges{{
        {"enhanced_damage", "+#% Enhanced Damage", 31, 40, 130},
        {"maxdamage", "+# to Maximum Damage", 11, 15, 71},
        {"maxhp", "+# to Life", 16, 20, 80}
    }};
    const auto socketMergedRanges = MergeCandidateSources(
        socketedRareRanges, rareJewelRanges);
    const auto modeledJewelTooltip = blue + std::string("+55% Enhanced Damage\n")
        + blue + "+13 to Maximum Damage\n"
        + blue + "+51% Damage to Undead\n"
        + blue + "+32 to Attack Rating against Undead\n"
        + blue + "+16 to Life\n"
        + blue + "Socketed (2)";
    const auto modeledJewelEnhanced = AppendConsensusRanges(
        modeledJewelTooltip, socketMergedRanges);
    CHECK(modeledJewelEnhanced.find("+55% Enhanced Damage "
        + std::string(darkGreen) + "[41 - 60]" + blue) != std::string::npos);
    CHECK(modeledJewelEnhanced.find("+13 to Maximum Damage "
        + std::string(darkGreen) + "[12 - 16]" + blue) != std::string::npos);
    CHECK(modeledJewelEnhanced.find("+16 to Life "
        + std::string(darkGreen) + "[16 - 20]" + blue) != std::string::npos);

    // Equipping a complete set appends active set-bonus lines to each item.
    // A bonus can reuse a base property's description (for example +300% ED)
    // without belonging to the set item record itself. It must not invalidate
    // the item's independent base ranges.
    const std::vector<std::vector<ModifierRange>> setItemRanges{{
        {"enhanced_damage", "+#% Enhanced Damage", 200, 250, 130},
        {"strength", "+# to Strength", 20, 30, 90},
        {"resist_all", "All Resistances +#", 10, 20, 100}
    }};
    const auto completeSetTooltip = blue + std::string("+208% Enhanced Damage\n")
        + blue + "+23 to Strength\n"
        + blue + "All Resistances +18\n"
        + "\xC3\xBF" "c4+4 to All Skills\n"
        + "\xC3\xBF" "c4+300% Enhanced Damage\n"
        + "\xC3\xBF" "c4Damage +100";
    const auto completeSetEnhanced = AppendConsensusRanges(
        completeSetTooltip, setItemRanges);
    CHECK(completeSetEnhanced.find("+208% Enhanced Damage "
        + std::string(darkGreen) + "[200 - 250]" + blue) != std::string::npos);
    CHECK(completeSetEnhanced.find("+23 to Strength "
        + std::string(darkGreen) + "[20 - 30]" + blue) != std::string::npos);
    CHECK(completeSetEnhanced.find("All Resistances +18 "
        + std::string(darkGreen) + "[10 - 20]" + blue) != std::string::npos);

    if (argc == 2) {
        tcp::tooltips::RangeCatalog catalog;
        std::string error;
        CHECK(catalog.Load(std::filesystem::path(argv[1]), error));
        CHECK(catalog.PropertyCount() > 0);
        CHECK(catalog.FindArmor("ci3")->minimum == 50);
        CHECK(catalog.FindArmor("ci3")->maximum == 60);
        CHECK(AuditEveryUniqueRecord(std::filesystem::path(argv[1])));
        CHECK(AuditEveryRunewordRecord(std::filesystem::path(argv[1]), catalog));
        CHECK(AuditEveryRunewordAgainstMetadata(catalog));

        // automagic.txt uses the unified suffix + prefix + automagic runtime
        // id space. Resolve a real BKVince Armor_fhr auto-affix dynamically
        // from the active tables and require its 5-10 FHR range.
        const auto excel = std::filesystem::path(argv[1]);
        const auto suffixCount = CompiledAffixCount(excel / "magicsuffix.txt");
        const auto englishLocalization = catalog.BuildLocalization(
            [](std::string_view key) {
                static const std::unordered_map<std::string_view, std::string_view> strings{
                    {"ItemStats1h", "Defense: %d"},
                    {"ItemStast1k", "to"},
                    {"Modstr2v", "%+d%% Enhanced Defense"},
                    {"ModStr2h", "Increase Maximum Mana %d%%"},
                    {"ModStr2u", "Damage Reduced by %d"},
                    {"ModStr4p", "%+d%% Faster Hit Recovery"},
                    {"ModStr1i", "%+d Defense"},
                    {"ModStr1c", "%+d to Vitality"},
                    {"ModStr1j", "Fire Resist %+d%%"},
                    {"ModStr1l", "Lightning Resist %+d%%"},
                    {"ModStr1k", "Cold Resist %+d%%"},
                    {"ModStr1n", "Poison Resist %+d%%"},
                    {"strModAllResistances", "All Resistances %+d"},
                    {"ModStr1x", "%d%% Better Chance of Getting Magic Items"},
                    {"ModitemAura", "Level %d %s Aura When Equipped"}
                };
                const auto found = strings.find(key);
                return found == strings.end() ? std::string{} : std::string(found->second);
            });
        const auto autoId = CompiledAffixId(excel / "automagic.txt", "Armor_fhr");
        const auto autoRuntimeId = CompiledAffixCount(excel / "magicsuffix.txt")
            + CompiledAffixCount(excel / "magicprefix.txt") + autoId;
        CHECK(autoId != 0 && autoRuntimeId <= UINT16_MAX);
        tcp::tooltips::ItemAffixIds autoArmor{};
        autoArmor.quality = 2;
        autoArmor.autoPrefix = static_cast<std::uint16_t>(autoRuntimeId);
        const auto autoCandidates = catalog.ResolveCandidates(autoArmor, "qui");
        CHECK(FindRange(autoCandidates, "item_fastergethitrate")->minimum == 5);
        CHECK(FindRange(autoCandidates, "item_fastergethitrate")->maximum == 10);
        CHECK(AppendConsensusRanges(blue + std::string("+7% Faster Hit Recovery"),
            autoCandidates).find("[5 - 10]") != std::string::npos);

        // A superior armor can also carry an automagic property. Both table
        // sources must survive candidate resolution: qualityitems row 8 owns
        // 5-25 ED and 1-2 DR, while Armor_max_mana owns 1-8 maximum Mana.
        const auto maxManaAutoId = CompiledAffixId(excel / "automagic.txt", "Armor_max_mana");
        const auto maxManaRuntimeId = CompiledAffixCount(excel / "magicsuffix.txt")
            + CompiledAffixCount(excel / "magicprefix.txt") + maxManaAutoId;
        CHECK(maxManaAutoId != 0 && maxManaRuntimeId <= UINT16_MAX);
        tcp::tooltips::ItemAffixIds superiorAutoArmor{};
        superiorAutoArmor.quality = 3;
        superiorAutoArmor.fileIndex = 7;
        superiorAutoArmor.autoPrefix = static_cast<std::uint16_t>(maxManaRuntimeId);
        const auto superiorAutoCandidates = catalog.ResolveCandidates(superiorAutoArmor, "lea");
        CHECK(FindRange(superiorAutoCandidates, "item_maxmana_percent")->minimum == 1);
        CHECK(FindRange(superiorAutoCandidates, "item_maxmana_percent")->maximum == 8);
        CHECK(FindRange(superiorAutoCandidates, "item_armor_percent")->minimum == 5);
        CHECK(FindRange(superiorAutoCandidates, "item_armor_percent")->maximum == 25);
        CHECK(FindRange(superiorAutoCandidates, "normal_damage_reduction")->minimum == 1);
        CHECK(FindRange(superiorAutoCandidates, "normal_damage_reduction")->maximum == 2);
        const auto superiorAutoTooltip = blue + std::string("+8% Enhanced Defense\n")
            + blue + "Increase Maximum Mana 3%\n"
            + blue + "Damage Reduced by 2";
        const auto superiorAutoEnhanced = AppendConsensusRanges(
            superiorAutoTooltip, superiorAutoCandidates);
        CHECK(superiorAutoEnhanced.find("+8% Enhanced Defense "
            + std::string(darkGreen) + "[5 - 25]" + blue) != std::string::npos);
        CHECK(superiorAutoEnhanced.find("Increase Maximum Mana 3% "
            + std::string(darkGreen) + "[1 - 8]" + blue) != std::string::npos);
        CHECK(superiorAutoEnhanced.find("Damage Reduced by 2 "
            + std::string(darkGreen) + "[1 - 2]" + blue) != std::string::npos);
        const auto superiorAutoRuntime = catalog.ResolveCandidateSet(
            superiorAutoArmor, "lea", {}, true, {}, superiorAutoTooltip,
            &englishLocalization);
        const auto superiorAutoRuntimeEnhanced = AppendConsensusRanges(
            superiorAutoTooltip, superiorAutoRuntime.candidates, false,
            &englishLocalization, &superiorAutoRuntime.intrinsicCandidates);
        CHECK(superiorAutoRuntimeEnhanced.find("+8% Enhanced Defense "
            + std::string(darkGreen) + "[5 - 25]" + blue) != std::string::npos);

        // Exact runtime trace for the socketed rare Stone Razor. The 8-bit
        // rare name ids 172/7 spell the generated name; they must never be
        // decoded as magic stat affixes. Prefix id 172 happens to collide with
        // Forked (+9-10 max damage), which used to combine with the real fixed
        // +1 max-damage suffix and reject every range on this item.
        tcp::tooltips::ItemAffixIds stoneRazor{};
        stoneRazor.quality = 6;
        stoneRazor.rarePrefix = 172;
        stoneRazor.rareSuffix = 7;
        stoneRazor.magicPrefix[0] = 1389; // Consecrated
        stoneRazor.magicPrefix[1] = 980;  // Jagged
        stoneRazor.magicPrefix[2] = 1349; // Septic
        stoneRazor.magicSuffix[0] = 745;  // of Shock
        stoneRazor.magicSuffix[1] = 742;  // of Frost
        stoneRazor.magicSuffix[2] = 195;  // of Craftsmanship
        const auto stoneCandidates = catalog.ResolveCandidates(stoneRazor, "ssd");
        CHECK(FindRange(stoneCandidates, "maxdamage")->minimum == 1);
        CHECK(FindRange(stoneCandidates, "maxdamage")->maximum == 1);
        const auto stoneTooltip = blue + std::string("Socketed (2)\n")
            + blue + "+6 Poison Damage Over 2 Seconds\n"
            + blue + "+1 Cold Damage\n"
            + blue + "Adds 1-3 Lightning Damage\n"
            + blue + "+32 to Attack Rating against Undead\n"
            + blue + "+51% Damage to Undead\n"
            + blue + "+1 to Maximum Damage\n"
            + blue + "+19% Enhanced Damage";
        const auto stoneEnhanced = AppendConsensusRanges(stoneTooltip, stoneCandidates);
        CHECK(stoneEnhanced.find("+19% Enhanced Damage "
            + std::string(darkGreen) + "[10 - 20]" + blue) != std::string::npos);
        CHECK(stoneEnhanced.find("+51% Damage to Undead "
            + std::string(darkGreen) + "[25 - 75]" + blue) != std::string::npos);
        CHECK(stoneEnhanced.find("+32 to Attack Rating against Undead "
            + std::string(darkGreen) + "[25 - 75]" + blue) != std::string::npos);

        // Exact socket filler from the runtime capture: a rare jewel carrying
        // Ruby (31-40 ED), Vermillion (11-15 max damage), of Burning
        // (11-25 to 31-50 fire damage), and of Hope (9-20 life). Every child
        // component must merge with Stone Razor before tooltip consensus.
        tcp::tooltips::ItemAffixIds rareJewel{};
        rareJewel.quality = 6;
        const auto rubyJewel = CompiledAffixIdForType(
            excel / "magicprefix.txt", "Ruby", "jewl");
        const auto vermillionJewel = CompiledAffixIdForType(
            excel / "magicprefix.txt", "Vermillion", "jewl");
        const auto burningJewel = CompiledAffixIdForType(
            excel / "magicsuffix.txt", "of Burning", "jewl");
        const auto hopeJewel = CompiledAffixIdForType(
            excel / "magicsuffix.txt", "of Hope", "jewl");
        CHECK(rubyJewel && vermillionJewel && burningJewel && hopeJewel);
        rareJewel.magicPrefix[0] = static_cast<std::uint16_t>(suffixCount + rubyJewel);
        rareJewel.magicPrefix[1] = static_cast<std::uint16_t>(suffixCount + vermillionJewel);
        rareJewel.magicSuffix[0] = static_cast<std::uint16_t>(burningJewel);
        rareJewel.magicSuffix[1] = static_cast<std::uint16_t>(hopeJewel);
        const auto rareJewelCandidates = catalog.ResolveSocketFillerCandidates(
            rareJewel, "jew", "ssd");
        const auto socketedStoneCandidates = MergeCandidateSources(
            stoneCandidates, rareJewelCandidates);
        CHECK(FindRange(socketedStoneCandidates, "enhanced_damage"));
        CHECK(FindRange(socketedStoneCandidates, "maxdamage"));
        CHECK(FindRange(socketedStoneCandidates, "firemindam"));
        CHECK(FindRange(socketedStoneCandidates, "firemaxdam"));
        CHECK(FindRange(socketedStoneCandidates, "maxhp"));
        CHECK(FindRange(socketedStoneCandidates, "enhanced_damage")->minimum == 41);
        CHECK(FindRange(socketedStoneCandidates, "enhanced_damage")->maximum == 60);
        CHECK(FindRange(socketedStoneCandidates, "maxdamage")->minimum == 12);
        CHECK(FindRange(socketedStoneCandidates, "maxdamage")->maximum == 16);
        CHECK(FindRange(socketedStoneCandidates, "firemindam")->minimum == 11);
        CHECK(FindRange(socketedStoneCandidates, "firemindam")->maximum == 25);
        CHECK(FindRange(socketedStoneCandidates, "firemaxdam")->minimum == 31);
        CHECK(FindRange(socketedStoneCandidates, "firemaxdam")->maximum == 50);
        CHECK(FindRange(socketedStoneCandidates, "maxhp")->minimum == 9);
        CHECK(FindRange(socketedStoneCandidates, "maxhp")->maximum == 20);
        const auto socketedStoneTooltip = blue + std::string("+55% Enhanced Damage\n")
            + blue + "+13 to Maximum Damage\n"
            + blue + "+51% Damage to Undead\n"
            + blue + "+32 to Attack Rating against Undead\n"
            + blue + "Adds 19-41 Fire Damage\n"
            + blue + "Adds 1-3 Lightning Damage\n"
            + blue + "+1 Cold Damage\n"
            + blue + "+6 Poison Damage Over 2 Seconds\n"
            + blue + "+16 to Life\n"
            + blue + "Socketed (2)";
        const auto socketedStoneEnhanced = AppendConsensusRanges(
            socketedStoneTooltip, socketedStoneCandidates);
        CHECK(socketedStoneEnhanced.find("+55% Enhanced Damage "
            + std::string(darkGreen) + "[41 - 60]" + blue) != std::string::npos);
        CHECK(socketedStoneEnhanced.find("+13 to Maximum Damage "
            + std::string(darkGreen) + "[12 - 16]" + blue) != std::string::npos);
        CHECK(socketedStoneEnhanced.find("Adds 19-41 Fire Damage "
            + std::string(darkGreen) + "[11 - 25]" + blue + " to "
            + darkGreen + "[31 - 50]" + blue) != std::string::npos);
        CHECK(socketedStoneEnhanced.find("+16 to Life "
            + std::string(darkGreen) + "[9 - 20]" + blue) != std::string::npos);
        CHECK(socketedStoneEnhanced.find("+51% Damage to Undead "
            + std::string(darkGreen) + "[25 - 75]" + blue) != std::string::npos);

        // Loose gems and runes use gems.txt, selected by the parent socket
        // category. Fixed bonuses shift overlapping variable parent ranges but
        // never create a fake range of their own.
        tcp::tooltips::ItemAffixIds plainFiller{};
        const auto ohmWeapon = catalog.ResolveSocketFillerCandidates(
            plainFiller, "r27", "ssd");
        CHECK(FindRange(ohmWeapon, "enhanced_damage"));
        CHECK(FindRange(ohmWeapon, "enhanced_damage")->minimum == 50);
        CHECK(FindRange(ohmWeapon, "enhanced_damage")->maximum == 50);
        const auto ralWeapon = catalog.ResolveSocketFillerCandidates(
            plainFiller, "r08", "ssd");
        CHECK(FindRange(ralWeapon, "firemindam"));
        CHECK(FindRange(ralWeapon, "firemaxdam"));
        CHECK(FindRange(ralWeapon, "firemindam")->minimum == 5);
        CHECK(FindRange(ralWeapon, "firemaxdam")->maximum == 30);
        const auto rubyWeapon = catalog.ResolveSocketFillerCandidates(
            plainFiller, "gpr", "ssd");
        CHECK(FindRange(rubyWeapon, "firemindam"));
        CHECK(FindRange(rubyWeapon, "firemaxdam"));
        CHECK(FindRange(rubyWeapon, "firemindam")->minimum == 14);
        CHECK(FindRange(rubyWeapon, "firemaxdam")->maximum == 46);
        const auto umArmor = catalog.ResolveSocketFillerCandidates(
            plainFiller, "r22", "crn");
        const auto umShield = catalog.ResolveSocketFillerCandidates(
            plainFiller, "r22", "paf");
        CHECK(FindRange(umArmor, "fireresist"));
        CHECK(FindRange(umShield, "fireresist"));
        CHECK(FindRange(umArmor, "fireresist")->minimum == 15);
        CHECK(FindRange(umShield, "fireresist")->minimum == 22);

        auto multiFiller = MergeCandidateSources(socketedStoneCandidates, ohmWeapon);
        multiFiller = MergeCandidateSources(multiFiller, ralWeapon);
        multiFiller = MergeCandidateSources(multiFiller, rubyWeapon);
        CHECK(FindRange(multiFiller, "enhanced_damage"));
        CHECK(FindRange(multiFiller, "firemindam"));
        CHECK(FindRange(multiFiller, "firemaxdam"));
        CHECK(FindRange(multiFiller, "enhanced_damage")->minimum == 91);
        CHECK(FindRange(multiFiller, "enhanced_damage")->maximum == 110);
        CHECK(FindRange(multiFiller, "firemindam")->minimum == 30);
        CHECK(FindRange(multiFiller, "firemindam")->maximum == 44);
        CHECK(FindRange(multiFiller, "firemaxdam")->minimum == 107);
        CHECK(FindRange(multiFiller, "firemaxdam")->maximum == 126);

        tcp::tooltips::ItemAffixIds normal{};
        normal.quality = 2;
        normal.runeword = true;
        auto unresolvedRuneword = normal;
        unresolvedRuneword.magicPrefix[0] = 794 + 481;
        CHECK(catalog.ResolveCandidates(unresolvedRuneword, "hax").front().empty());

        const auto pledge = catalog.ResolveCandidates(normal, "buc", "Runeword1");
        CHECK(FindRange(pledge, "fireresist")->minimum == 48);
        CHECK(FindRange(pledge, "lightresist")->minimum == 48);
        CHECK(FindRange(pledge, "coldresist")->minimum == 43);
        CHECK(FindRange(pledge, "poisonresist")->minimum == 48);
        const auto fixedPledgeTooltip = blue + std::string("Fire Resist +48%\n")
            + blue + "Lightning Resist +48%\n"
            + blue + "Cold Resist +43%\n"
            + blue + "Poison Resist +48%";
        CHECK(AppendConsensusRanges(fixedPledgeTooltip, pledge) == fixedPledgeTooltip);

        // Call to Arms combines its variable runeword ED with Ohm's fixed
        // +50% weapon ED. The displayed aggregate must therefore be 250-290,
        // not the singular runeword property's 200-240.
        const auto callToArms = catalog.ResolveCandidates(normal, "hax", "Runeword13");
        CHECK(FindRange(callToArms, "enhanced_damage")->minimum == 250);
        CHECK(FindRange(callToArms, "enhanced_damage")->maximum == 290);
        const auto callToArmsTooltip = blue + std::string("+275% Enhanced Damage");
        CHECK(AppendConsensusRanges(callToArmsTooltip, callToArms).find("[250 - 290]")
            != std::string::npos);

        const auto intrinsicCallToArms = catalog.ResolveCandidates(
            normal, "hax", "Runeword13", false);
        CHECK(FindRange(intrinsicCallToArms, "enhanced_damage")->minimum == 200);
        CHECK(FindRange(intrinsicCallToArms, "enhanced_damage")->maximum == 240);
        CHECK(AppendConsensusRanges(blue + std::string("+225% Enhanced Damage"),
            intrinsicCallToArms).find("[200 - 240]") != std::string::npos);
        CHECK(AppendConsensusRanges(blue + std::string("+275% Enhanced Damage"),
            intrinsicCallToArms, true).find("[200 - 240]") != std::string::npos);

        // Stone on torso armor combines its variable 350-400 runeword ED
        // with Pul's fixed +50% armor ED. It also exposes two independently
        // rolled Strength lines (flat Strength and Strength percent) that use
        // the same localized wording in BKVince.
        auto stoneIds = normal;
        // Runtime runeword payload may leave non-zero values in every field
        // that normally stores magic affix ids. A white runeword base cannot
        // own those affixes, so none may contaminate the Stone candidate.
        stoneIds.magicPrefix[0] = 1389;
        stoneIds.magicPrefix[1] = 980;
        stoneIds.magicPrefix[2] = 1349;
        stoneIds.magicSuffix[0] = 745;
        stoneIds.magicSuffix[1] = 742;
        stoneIds.magicSuffix[2] = 195;
        const auto stone = catalog.ResolveCandidates(stoneIds, "gth", "Runeword137");
        CHECK(FindRange(stone, "item_armor_percent")->minimum == 400);
        CHECK(FindRange(stone, "item_armor_percent")->maximum == 450);
        CHECK(FindRange(stone, "strength")->minimum == 15);
        CHECK(FindRange(stone, "strength")->maximum == 20);
        CHECK(FindRange(stone, "item_str_percent")->minimum == 10);
        CHECK(FindRange(stone, "item_str_percent")->maximum == 15);
        CHECK(FindRange(stone, "vitality")->minimum == 15);
        CHECK(FindRange(stone, "vitality")->maximum == 20);
        const auto stoneRunewordTooltip = std::string("Defense: 689\n")
            + "Max Sockets: 4\n"
            + "Durability: 33 of 55\n"
            + "Required Strength: 110\n"
            + "Required Level: 47\n"
            + blue + "+60% Faster Hit Recovery\n"
            + blue + "+30 to Clay Golem\n"
            + blue + "+420% Enhanced Defense\n"
            + blue + "+300 Defense vs. Missile\n"
            + blue + "+17 to Strength\n"
            + blue + "+12 to Strength\n"
            + blue + "+19 to Vitality\n"
            + blue + "+10 to Energy\n"
            + blue + "All Resistances +15";
        const auto stoneRunewordEnhanced = AppendConsensusRanges(stoneRunewordTooltip, stone);
        CHECK(stoneRunewordEnhanced.find("+420% Enhanced Defense "
            + std::string(darkGreen) + "[400 - 450]" + blue) != std::string::npos);
        CHECK(stoneRunewordEnhanced.find("+17 to Strength "
            + std::string(darkGreen) + "[15 - 20]" + blue) != std::string::npos);
        CHECK(stoneRunewordEnhanced.find("+12 to Strength "
            + std::string(darkGreen) + "[10 - 15]" + blue) != std::string::npos);
        CHECK(stoneRunewordEnhanced.find("+19 to Vitality "
            + std::string(darkGreen) + "[15 - 20]" + blue) != std::string::npos);

        // Dream on a helmet carries variable flat Defense from the runeword
        // and fixed Enhanced Defense from Pul. The structural Defense header
        // must be ignored without suppressing the separate +Defense roll.
        const auto dream = catalog.ResolveCandidates(normal, "crn", "Runeword29");
        CHECK(FindRange(dream, "armorclass")->minimum == 150);
        CHECK(FindRange(dream, "armorclass")->maximum == 220);
        const auto dreamTooltip = std::string("Defense: 254\n")
            + "Max Sockets: 3\n"
            + "Durability: 48 of 50\n"
            + "Required Strength: 55\n"
            + "Required Level: 65\n"
            + blue + "+26% Faster Hit Recovery\n"
            + blue + "+50% Enhanced Defense\n"
            + blue + "+199 Defense\n"
            + blue + "+10 to Vitality\n"
            + blue + "All Resistances +18\n"
            + blue + "30% Better Chance of Getting Magic Items";
        const auto dreamEnhanced = AppendConsensusRanges(dreamTooltip, dream);
        CHECK(dreamEnhanced.find("+199 Defense "
            + std::string(darkGreen) + "[150 - 220]" + blue) != std::string::npos);

        // Superior Crown from the runtime capture uses qualityitems row 8,
        // not automagic.txt: 5-25% Enhanced Defense plus 1-2 flat DR.
        tcp::tooltips::ItemAffixIds superiorCrown{};
        superiorCrown.quality = 3;
        superiorCrown.fileIndex = 7;
        const auto superior = catalog.ResolveCandidates(superiorCrown, "crn");
        CHECK(FindRange(superior, "item_armor_percent")->minimum == 5);
        CHECK(FindRange(superior, "item_armor_percent")->maximum == 25);
        CHECK(FindRange(superior, "normal_damage_reduction")->minimum == 1);
        CHECK(FindRange(superior, "normal_damage_reduction")->maximum == 2);
        const auto superiorTooltip = std::string("Defense: 48\n")
            + "Base Defense: 43 [25 - 45]\n"
            + "Max Sockets: 3\n"
            + "Durability: 48 of 50\n"
            + "Required Strength: 55\n"
            + blue + "+5% Enhanced Defense\n"
            + blue + "Damage Reduced by 2\n"
            + blue + "Socketed (3)";
        const auto superiorEnhanced = AppendConsensusRanges(superiorTooltip, superior);
        CHECK(superiorEnhanced.find("+5% Enhanced Defense "
            + std::string(darkGreen) + "[5 - 25]" + blue) != std::string::npos);
        CHECK(superiorEnhanced.find("Damage Reduced by 2 "
            + std::string(darkGreen) + "[1 - 2]" + blue) != std::string::npos);

        // Runtime captures from real BKVince superior items prove that low
        // fileIndex values resolve directly: Axe fileIndex 0 is 5-30 Target
        // Defense and Leather Armor fileIndex 2 is 5-50 Enhanced Defense.
        tcp::tooltips::ItemAffixIds superiorAxe{};
        superiorAxe.quality = 3;
        superiorAxe.fileIndex = 0;
        const auto superiorAxeCandidates = catalog.ResolveCandidates(superiorAxe, "axe");
        const auto superiorAxeEnhanced = AppendConsensusRanges(
            blue + std::string("-12% Target Defense"), superiorAxeCandidates);
        CHECK(superiorAxeEnhanced.find("-12% Target Defense "
            + std::string(darkGreen) + "[5 - 30]" + blue) != std::string::npos);

        tcp::tooltips::ItemAffixIds superiorLeather{};
        superiorLeather.quality = 3;
        superiorLeather.fileIndex = 2;
        superiorLeather.autoPrefix = static_cast<std::uint16_t>(maxManaRuntimeId);
        const auto superiorLeatherCandidates = catalog.ResolveCandidates(superiorLeather, "lea");
        const auto superiorLeatherTooltip = std::string("Superior Leather Armor (99)\n")
            + "Defense: 39\n"
            + "Max Sockets: 2\n"
            + "Durability: 13 of 13\n"
            + "Required Strength: 5\n"
            + blue + "+47% Enhanced Defense\n"
            + blue + "Increase Maximum Mana 4%\n"
            + blue + "Ethereal (Cannot be Repaired)";
        const auto superiorLeatherEnhanced = AppendConsensusRanges(
            superiorLeatherTooltip, superiorLeatherCandidates);
        CHECK(superiorLeatherEnhanced.find("+47% Enhanced Defense "
            + std::string(darkGreen) + "[5 - 50]" + blue) != std::string::npos);
        CHECK(superiorLeatherEnhanced.find("Increase Maximum Mana 4% "
            + std::string(darkGreen) + "[1 - 8]" + blue) != std::string::npos);

        // The qualityitems roll remains part of the base after it becomes a
        // runeword. Dream + Pul therefore stacks fixed 50 ED with superior
        // row 8's 5-25 ED, while retaining the row's 1-2 flat DR.
        auto superiorDreamIds = normal;
        superiorDreamIds.quality = 3;
        superiorDreamIds.fileIndex = 7;
        const auto superiorDream = catalog.ResolveCandidates(
            superiorDreamIds, "crn", "Runeword29");
        const auto superiorDreamTooltip = std::string("Defense: 251\n")
            + "Max Sockets: 3\n"
            + "Durability: 48 of 50\n"
            + "Required Strength: 55\n"
            + "Required Level: 65\n"
            + blue + "+21% Faster Hit Recovery\n"
            + blue + "+55% Enhanced Defense\n"
            + blue + "+180 Defense\n"
            + blue + "+10 to Vitality\n"
            + blue + "All Resistances +17\n"
            + blue + "Damage Reduced by 2\n"
            + blue + "21% Better Chance of Getting Magic Items\n"
            + blue + "Socketed (3)";
        const auto superiorDreamEnhanced = AppendConsensusRanges(
            superiorDreamTooltip, superiorDream);
        CHECK(superiorDreamEnhanced.find("+55% Enhanced Defense "
            + std::string(darkGreen) + "[55 - 75]" + blue) != std::string::npos);
        CHECK(superiorDreamEnhanced.find("Damage Reduced by 2 "
            + std::string(darkGreen) + "[1 - 2]" + blue) != std::string::npos);
        CHECK(superiorDreamEnhanced.find("+180 Defense "
            + std::string(darkGreen) + "[150 - 220]" + blue) != std::string::npos);
        const auto superiorDreamRuntime = catalog.ResolveCandidateSet(
            superiorDreamIds, "crn", "Runeword29", true, {},
            superiorDreamTooltip, &englishLocalization);
        const auto superiorDreamRuntimeEnhanced = AppendConsensusRanges(
            superiorDreamTooltip, superiorDreamRuntime.candidates, false,
            &englishLocalization, &superiorDreamRuntime.intrinsicCandidates);
        CHECK(superiorDreamRuntimeEnhanced.find("+55% Enhanced Defense "
            + std::string(darkGreen) + "[55 - 75]" + blue) != std::string::npos);
        CHECK(superiorDreamRuntimeEnhanced.find("All Resistances +17 "
            + std::string(darkGreen) + "[14 - 20]" + blue) != std::string::npos);

        // Runtime regression: ordinary magic armor must retain both prefix
        // and suffix ranges while the Cube provenance resolver is active.
        tcp::tooltips::ItemAffixIds saintlyChance{};
        saintlyChance.quality = 4;
        const auto saintly = CompiledAffixIdForType(
            excel / "magicprefix.txt", "Saintly", "armo");
        const auto ofChance = CompiledAffixIdForType(
            excel / "magicsuffix.txt", "of Chance", "glov");
        CHECK(saintly && ofChance);
        saintlyChance.magicPrefix[0] = static_cast<std::uint16_t>(suffixCount + saintly);
        saintlyChance.magicSuffix[0] = static_cast<std::uint16_t>(ofChance);
        const auto saintlyChanceTooltip = blue + std::string("+71% Enhanced Defense\n")
            + blue + "14% Better Chance of Getting Magic Items";
        const auto saintlyChanceRuntime = catalog.ResolveCandidateSet(
            saintlyChance, "tgl", {}, true, {}, saintlyChanceTooltip,
            &englishLocalization);
        const auto saintlyChanceEnhanced = AppendConsensusRanges(
            saintlyChanceTooltip, saintlyChanceRuntime.candidates, false,
            &englishLocalization, &saintlyChanceRuntime.intrinsicCandidates);
        CHECK(saintlyChanceEnhanced.find("+71% Enhanced Defense "
            + std::string(darkGreen) + "[66 - 80]" + blue) != std::string::npos);

        // Exile combines its runeword Enhanced Defense with the Paladin
        // shield's Prismatic automagic All Resistances range.
        auto exileIds = normal;
        const auto prismaticAutoId = CompiledAffixId(
            excel / "automagic.txt", "Prismatic");
        CHECK(prismaticAutoId != 0);
        exileIds.autoPrefix = static_cast<std::uint16_t>(
            CompiledAffixCount(excel / "magicsuffix.txt")
            + CompiledAffixCount(excel / "magicprefix.txt") + prismaticAutoId);
        const auto exileTooltip = blue + std::string("Level 14 Defiance Aura When Equipped\n")
            + blue + "+239% Enhanced Defense\n"
            + blue + "All Resistances +25\n"
            + blue + "30% Better Chance of Getting Magic Items";
        const auto exileRuntime = catalog.ResolveCandidateSet(
            exileIds, "paf", "Runeword37", true, {}, exileTooltip,
            &englishLocalization);
        const auto exileEnhanced = AppendConsensusRanges(
            exileTooltip, exileRuntime.candidates, false,
            &englishLocalization, &exileRuntime.intrinsicCandidates);
        CHECK(exileEnhanced.find("+239% Enhanced Defense "
            + std::string(darkGreen) + "[220 - 260]" + blue) != std::string::npos);
        CHECK(exileEnhanced.find("All Resistances +25 "
            + std::string(darkGreen) + "[25 - 35]" + blue) != std::string::npos);

        const auto crescentMoon = catalog.ResolveCandidates(normal, "hax", "Runeword17");
        const auto* aura = FindRange(crescentMoon, "item_aura:aura:holy shock");
        CHECK(aura && aura->minimum == 14 && aura->maximum == 16);
        const auto auraTooltip = blue + std::string("Level 15 Holy Shock Aura When Equipped");
        const auto englishSkillLocalization = catalog.BuildLocalization(
            [](std::string_view key) {
                if (key == "ModitemAura")
                    return std::string("Level %d %s Aura When Equipped");
                if (key == "ItemStats1h") return std::string("Defense: %d");
                if (key == "ItemStast1k") return std::string("to");
                return std::string{};
            });
        CHECK(AppendConsensusRanges(auraTooltip, crescentMoon, false,
            &englishSkillLocalization).find("[14 - 16]")
            != std::string::npos);

        const auto bone = catalog.ResolveCandidates(normal, "aar", "Runeword8");
        CHECK(FindRange(bone, "item_fastercastrate")->minimum == 15);
        CHECK(FindRange(bone, "item_fastercastrate")->maximum == 20);
        CHECK(FindRange(bone, "item_skillongethit") == nullptr);
        tcp::tooltips::ItemAffixIds slaughterMaul{};
        slaughterMaul.quality = 4;
        // The compiled runtime ids omit the Expansion separator from each TXT.
        slaughterMaul.magicPrefix[0] = 794 + 481; // Expert's: fixed +1 Combat Skills
        slaughterMaul.magicSuffix[0] = 201; // of Slaughter: +15-20 Maximum Damage
        const auto slaughterCandidates = catalog.ResolveCandidates(slaughterMaul, "7gm");
        const auto slaughterTooltip = blue + std::string("+1 to Combat Skills (Barbarian Only)\n")
            + blue + "+18 to Maximum Damage\n"
            + blue + "+50% Damage to Undead";
        const auto slaughterEnhanced = AppendConsensusRanges(slaughterTooltip, slaughterCandidates);
        CHECK(slaughterEnhanced.find(
            darkGreen + std::string("[15 - 20]") + blue) != std::string::npos);
        CHECK(slaughterEnhanced.find("Combat Skills (Barbarian Only) ") == std::string::npos);

        tcp::tooltips::ItemAffixIds gnasher{};
        gnasher.quality = 7;
        gnasher.fileIndex = 0;
        const auto gnasherCandidates = catalog.ResolveCandidates(gnasher, "hax");
        const auto gnasherTooltip = blue + std::string("+25% Increased Attack Speed\n")
            + blue + "+199% Enhanced Damage\n"
            + blue + "+20% Chance of Crushing Blow";
        const auto gnasherEnhanced = AppendConsensusRanges(gnasherTooltip, gnasherCandidates);
        CHECK(gnasherEnhanced.find(
            "+199% Enhanced Damage " + std::string(darkGreen) + "[180 - 200]" + blue)
            != std::string::npos);
        CHECK(gnasherEnhanced.find("Increased Attack Speed ") == std::string::npos);

        tcp::tooltips::ItemAffixIds casterAmulet{};
        casterAmulet.quality = 8;
        casterAmulet.magicPrefix[0] = 794 + 312; // Great Wyrm's: +61-90 Mana
        casterAmulet.magicSuffix[0] = 22; // of the Apprentice: fixed +10 FCR
        const auto candidates = catalog.ResolveCandidates(casterAmulet, "amu");
        bool foundStackedFcr{};
        for (const auto& candidate : candidates) {
            for (const auto& range : candidate) {
                if (range.key == "item_fastercastrate"
                    && range.minimum == 15 && range.maximum == 20) {
                    foundStackedFcr = true;
                }
            }
        }
        CHECK(foundStackedFcr);
        const auto casterTooltip = blue + std::string("+15% Faster Cast Rate\n")
            + blue + "+83 to Mana\n"
            + blue + "Regenerate Mana 5%\n"
            + blue + "+3% to Experience Gained";
        const auto casterEnhanced = AppendConsensusRanges(casterTooltip, candidates);
        CHECK(casterEnhanced.find(
            "+15% Faster Cast Rate " + std::string(darkGreen) + "[15 - 20]" + blue)
            != std::string::npos);
        CHECK(casterEnhanced.find(
            "+83 to Mana " + std::string(darkGreen) + "[71 - 110]" + blue)
            != std::string::npos);
        CHECK(casterEnhanced.find(
            "Regenerate Mana 5% " + std::string(darkGreen) + "[4 - 10]" + blue)
            != std::string::npos);
        CHECK(casterEnhanced.find(
            "+3% to Experience Gained " + std::string(darkGreen) + "[1 - 5]" + blue)
            != std::string::npos);

        // BKVince corruption persists stat 369 (corruptordesc). The completed
        // value 1001 excludes the initial randomizer and activates the eight
        // Annihilus outcomes. The final tooltip then identifies the IAS
        // outcome without retaining the transient 1-1000 roll.
        tcp::tooltips::ItemAffixIds corruptedAnnihilus{};
        corruptedAnnihilus.quality = 7;
        corruptedAnnihilus.fileIndex = 381;
        const auto corruptedCandidates = catalog.ResolveCandidates(
            corruptedAnnihilus, "cm1", {}, true,
            [](std::int32_t statId, std::uint16_t) {
                return statId == 369 ? 1001 : 0;
            });
        const auto corruptedTooltip = blue + std::string("+7% Increased Attack Speed");
        const auto corruptedEnhanced = AppendConsensusRanges(
            corruptedTooltip, corruptedCandidates);
        CHECK(corruptedEnhanced.find("[5 - 10]") != std::string::npos);

        // BKVince augment persists stat 370 (augmented). On an amulet with a
        // native 20-35 MF suffix, the MF/GF augment's fixed +50 MF must shift
        // the displayed range to 70-85; the alternative melee augment is
        // rejected by the complete tooltip.
        tcp::tooltips::ItemAffixIds augmentedAmulet{};
        augmentedAmulet.quality = 4;
        augmentedAmulet.magicSuffix[0] = 74;
        const auto augmentedCandidates = catalog.ResolveCandidates(
            augmentedAmulet, "amu", {}, true,
            [](std::int32_t statId, std::uint16_t) {
                return statId == 370 ? 1 : 0;
            });
        const auto augmentedTooltip = blue
            + std::string("75% Better Chance of Getting Magic Items\n")
            + blue + "100% Extra Gold from Monsters\n"
            + blue + "+2% to Experience Gained";
        const auto augmentedEnhanced = AppendConsensusRanges(
            augmentedTooltip, augmentedCandidates);
        CHECK(augmentedEnhanced.find("[70 - 85]") != std::string::npos);

        // A public markerless recipe is recoverable when the finished tooltip
        // proves it. Great Wyrm's owns 61-90 Mana, a useitem recipe adds 10-20,
        // and a usetype recipe adds 2-6 FCR. The resolver must support their
        // combined history without letting an ambiguous recipe erase the
        // intrinsic Mana range.
        const auto cubeSource = ReadBinaryText(excel / "cubemain.txt");
        auto syntheticCube = TableWithSingleRow(cubeSource, {
            {"description", "Public markerless Mana recipe"},
            {"enabled", "1"},
            {"numinputs", "1"},
            {"input 1", "amu"},
            {"output", "useitem"},
            {"mod 1", "mana"},
            {"mod 1 min", "10"},
            {"mod 1 max", "20"}
        });
        CHECK(AppendTableRow(syntheticCube, {
            {"description", "Public markerless FCR reroll"},
            {"enabled", "1"},
            {"numinputs", "1"},
            {"input 1", "amu,mag"},
            {"output", "usetype,mag"},
            {"mod 1", "cast1"},
            {"mod 1 min", "2"},
            {"mod 1 max", "6"}
        }));
        // Simulate a large public mod containing many structurally compatible
        // Cube mutations. None of these unrelated rows may expand the hover
        // candidate set for this tooltip.
        for (int recipe = 0; recipe < 128; ++recipe) {
            CHECK(AppendTableRow(syntheticCube, {
                {"description", "Unrelated markerless mutation "
                    + std::to_string(recipe)},
                {"enabled", "1"},
                {"numinputs", "1"},
                {"input 1", "amu"},
                {"output", "useitem"},
                {"mod 1", "mana"},
                {"mod 1 min", std::to_string(1000 + recipe)},
                {"mod 1 max", std::to_string(1000 + recipe)}
            }));
        }
        tcp::tooltips::RangeCatalog syntheticCatalog;
        std::string syntheticError;
        CHECK(syntheticCatalog.Load([&](std::string_view tableName,
                std::string& text, std::string& loadError) {
            if (tableName == "cubemain.txt") {
                text = syntheticCube;
                return !text.empty();
            }
            text = ReadBinaryText(excel / tableName);
            if (!text.empty()) return true;
            loadError = "missing synthetic source table";
            return false;
        }, syntheticError));
        tcp::tooltips::ItemAffixIds markerlessAmulet{};
        markerlessAmulet.quality = 4;
        markerlessAmulet.magicPrefix[0] = 794 + 312;
        const auto provenTooltip = blue + std::string("+103 to Mana");
        const auto provenResolution = syntheticCatalog.ResolveCandidateSet(
            markerlessAmulet, "amu", {}, true, {}, provenTooltip);
        CHECK(provenResolution.candidates.size() == 2);
        const auto provenRecipe = AppendConsensusRanges(provenTooltip,
            provenResolution.candidates, false, nullptr,
            &provenResolution.intrinsicCandidates);
        CHECK(provenRecipe.find("[71 - 110]") != std::string::npos);

        const auto ambiguousTooltip = blue + std::string("+83 to Mana");
        const auto ambiguousResolution = syntheticCatalog.ResolveCandidateSet(
            markerlessAmulet, "amu", {}, true, {}, ambiguousTooltip);
        CHECK(ambiguousResolution.candidates.size() == 1);
        const auto ambiguousRecipe = AppendConsensusRanges(ambiguousTooltip,
            ambiguousResolution.candidates, false, nullptr,
            &ambiguousResolution.intrinsicCandidates);
        CHECK(ambiguousRecipe.find("[61 - 90]") != std::string::npos);
        CHECK(ambiguousRecipe.find("[71 - 110]") == std::string::npos);

        const auto combinedTooltip = blue + std::string("+103 to Mana\n")
            + blue + "+4% Faster Cast Rate";
        const auto combinedResolution = syntheticCatalog.ResolveCandidateSet(
            markerlessAmulet, "amu", {}, true, {}, combinedTooltip);
        CHECK(combinedResolution.candidates.size() == 4);
        const auto combinedRecipe = AppendConsensusRanges(combinedTooltip,
            combinedResolution.candidates, false, nullptr,
            &combinedResolution.intrinsicCandidates);
        CHECK(combinedRecipe.find("[71 - 110]") != std::string::npos);
        CHECK(combinedRecipe.find("[2 - 6]") != std::string::npos);

        // usetype,crf defines the crafted item's intrinsic properties; it must
        // not be applied a second time when a later useitem mutation is also
        // compatible with the finished crafted item.
        auto craftedCube = TableWithSingleRow(cubeSource, {
            {"description", "Synthetic Caster Amulet"},
            {"enabled", "1"},
            {"numinputs", "1"},
            {"input 1", "amu,mag"},
            {"output", "usetype,crf"},
            {"mod 1", "cast1"},
            {"mod 1 min", "5"},
            {"mod 1 max", "10"},
            {"mod 2", "mana"},
            {"mod 2 min", "10"},
            {"mod 2 max", "20"}
        });
        CHECK(AppendTableRow(craftedCube, {
            {"description", "Crafted useitem Mana mutation"},
            {"enabled", "1"},
            {"numinputs", "1"},
            {"input 1", "any,crf"},
            {"output", "useitem"},
            {"mod 1", "mana"},
            {"mod 1 min", "2"},
            {"mod 1 max", "4"}
        }));
        tcp::tooltips::RangeCatalog craftedCatalog;
        std::string craftedError;
        CHECK(craftedCatalog.Load([&](std::string_view tableName,
                std::string& text, std::string& loadError) {
            if (tableName == "cubemain.txt") {
                text = craftedCube;
                return true;
            }
            text = ReadBinaryText(excel / tableName);
            if (!text.empty()) return true;
            loadError = "missing crafted synthetic source table";
            return false;
        }, craftedError));
        tcp::tooltips::ItemAffixIds mutatedCraft{};
        mutatedCraft.quality = 8;
        mutatedCraft.magicPrefix[0] = 794 + 312;
        const auto mutatedCraftTooltip = blue + std::string("+8% Faster Cast Rate\n")
            + blue + "+112 to Mana";
        const auto mutatedCraftResolution = craftedCatalog.ResolveCandidateSet(
            mutatedCraft, "amu", {}, true, {}, mutatedCraftTooltip);
        CHECK(mutatedCraftResolution.candidates.size() == 2);
        const auto mutatedCraftEnhanced = AppendConsensusRanges(mutatedCraftTooltip,
            mutatedCraftResolution.candidates, false, nullptr,
            &mutatedCraftResolution.intrinsicCandidates);
        CHECK(mutatedCraftEnhanced.find("[5 - 10]") != std::string::npos);
        CHECK(mutatedCraftEnhanced.find("[73 - 114]") != std::string::npos);
        CHECK(mutatedCraftEnhanced.find("[81 - 130]") == std::string::npos);

        // BKVince Blood Weapon fixed properties must remain resolvable after
        // sockets are added, even when unrelated affix lines are present.
        tcp::tooltips::ItemAffixIds socketedBloodAxe{};
        socketedBloodAxe.quality = 8;
        const auto bloodCandidates = catalog.ResolveCandidates(socketedBloodAxe, "hax");
        const auto bloodTooltip = blue + std::string(
            "10% Chance to cast Level 22 Amplify Damage on striking\n")
            + blue + "+58% Enhanced Damage\n"
            + blue + "+16 to Maximum Damage\n"
            + blue + "3% Life stolen per hit\n"
            + blue + "Prevent Monster Heal\n"
            + blue + "+30 to Life\n"
            + blue + "+3% to Experience Gained\n"
            + blue + "Socketed (3)";
        const auto bloodEnhanced = AppendConsensusRanges(bloodTooltip, bloodCandidates);
        CHECK(bloodEnhanced.find("+58% Enhanced Damage "
            + std::string(darkGreen) + "[50 - 80]" + blue) != std::string::npos);
        CHECK(bloodEnhanced.find("3% Life stolen per hit "
            + std::string(darkGreen) + "[3 - 6]" + blue) != std::string::npos);
        CHECK(bloodEnhanced.find("+30 to Life "
            + std::string(darkGreen) + "[30 - 40]" + blue) != std::string::npos);
        CHECK(bloodEnhanced.find("+3% to Experience Gained "
            + std::string(darkGreen) + "[1 - 5]" + blue) != std::string::npos);
    }
#undef CHECK
}
