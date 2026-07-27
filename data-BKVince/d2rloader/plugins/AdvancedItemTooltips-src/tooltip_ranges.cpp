#include "tooltip_ranges.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdlib>
#include <fstream>
#include <map>
#include <set>
#include <unordered_set>

namespace tcp::tooltips {
namespace {

using Row = std::unordered_map<std::string, std::string>;
constexpr char ColorMarker[] = "\xEE\x81\xBE";

bool IsColorMarker(std::string_view text, std::size_t index) {
    return index + 3 < text.size()
        && static_cast<unsigned char>(text[index]) == 0xEE
        && static_cast<unsigned char>(text[index + 1]) == 0x81
        && static_cast<unsigned char>(text[index + 2]) == 0xBE;
}

std::string Lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::string Trim(std::string value) {
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back()))) value.pop_back();
    std::size_t first{};
    while (first < value.size() && std::isspace(static_cast<unsigned char>(value[first]))) ++first;
    return value.substr(first);
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

bool ReadTsv(const std::filesystem::path& path, std::vector<Row>& rows, std::string& error) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        error = "Cannot open " + path.string();
        return false;
    }
    std::string line;
    if (!std::getline(input, line)) {
        error = "Missing header in " + path.string();
        return false;
    }
    auto headers = SplitTabs(std::move(line));
    for (auto& header : headers) header = Lower(Trim(std::move(header)));
    while (std::getline(input, line)) {
        const auto values = SplitTabs(std::move(line));
        Row row;
        for (std::size_t index = 0; index < headers.size() && index < values.size(); ++index) {
            if (!values[index].empty()) row.emplace(headers[index], values[index]);
        }
        rows.emplace_back(std::move(row));
    }
    return true;
}

std::string Get(const Row& row, std::string_view key) {
    const auto found = row.find(std::string(key));
    return found == row.end() ? std::string{} : found->second;
}

std::int32_t Number(const Row& row, std::string_view key) {
    const auto value = Get(row, key);
    std::int32_t result{};
    if (value.empty()) return result;
    const auto parsed = std::from_chars(value.data(), value.data() + value.size(), result);
    return parsed.ec == std::errc{} ? result : 0;
}

std::string StripColors(std::string_view text) {
    std::string clean;
    clean.reserve(text.size());
    for (std::size_t index = 0; index < text.size();) {
        if (IsColorMarker(text, index)) {
            index += 4;
        } else if (static_cast<unsigned char>(text[index]) == 0xFF
            && index + 2 < text.size() && text[index + 1] == 'c') {
            index += 3;
        } else if (static_cast<unsigned char>(text[index]) == 0xC3
            && index + 3 < text.size()
            && static_cast<unsigned char>(text[index + 1]) == 0xBF
            && text[index + 2] == 'c') {
            index += 4;
        } else {
            clean.push_back(text[index++]);
        }
    }
    return clean;
}

char LastColor(std::string_view line) {
    char result = '0';
    for (std::size_t index = 0; index + 3 < line.size(); ++index) {
        if (IsColorMarker(line, index)) {
            result = line[index + 3];
            index += 3;
        }
    }
    return result;
}

std::string NormalizeWords(std::string_view text) {
    const auto clean = StripColors(text);
    std::string normalized;
    bool space{};
    for (const auto raw : clean) {
        const auto ch = static_cast<unsigned char>(raw);
        if (std::isalpha(ch) || ch >= 0x80) {
            if (space && !normalized.empty()) normalized.push_back(' ');
            normalized.push_back(ch < 0x80 ? static_cast<char>(std::tolower(ch)) : raw);
            space = false;
        } else if (!normalized.empty()) {
            space = true;
        }
    }
    return normalized;
}

bool DescriptionFits(std::string_view line, const ModifierRange& range) {
    const auto anchor = NormalizeWords(range.anchor);
    return !anchor.empty() && NormalizeWords(line).find(anchor) != std::string::npos;
}

bool RollFits(std::int32_t roll, const ModifierRange& range) {
    const auto magnitude = std::llabs(static_cast<long long>(roll));
    const auto low = std::min(
        std::llabs(static_cast<long long>(range.minimum)),
        std::llabs(static_cast<long long>(range.maximum)));
    const auto high = std::max(
        std::llabs(static_cast<long long>(range.minimum)),
        std::llabs(static_cast<long long>(range.maximum)));
    return magnitude >= low && magnitude <= high;
}

void AddRecord(const std::vector<std::vector<ModifierRange>>& records,
    std::size_t index, std::vector<ModifierRange>& output) {
    if (index < records.size()) {
        output.insert(output.end(), records[index].begin(), records[index].end());
    }
}

std::vector<ModifierRange> Combine(std::vector<ModifierRange> raw) {
    std::map<std::string, ModifierRange> combined;
    for (auto& range : raw) {
        auto [entry, inserted] = combined.try_emplace(range.key, range);
        if (!inserted) {
            entry->second.minimum += range.minimum;
            entry->second.maximum += range.maximum;
            entry->second.priority = std::max(entry->second.priority, range.priority);
        }
    }
    std::vector<ModifierRange> result;
    for (auto& [_, range] : combined) result.push_back(std::move(range));
    std::sort(result.begin(), result.end(), [](const auto& left, const auto& right) {
        return left.priority > right.priority;
    });
    return result;
}

std::optional<RangeCatalog::PropertyInfo> DecodeProperty(
    const Row& row, const std::unordered_map<std::string, std::int32_t>& priorities) {
    const auto code = Lower(Get(row, "code"));
    const auto tooltip = Get(row, "*tooltip");
    const auto function = Number(row, "func1");
    auto stat = Lower(Get(row, "stat1"));
    if (code.empty() || tooltip.empty()) return std::nullopt;
    if (function == 5) stat = "mindamage";
    else if (function == 6) stat = "maxdamage";
    else if (function == 7) stat = "enhanced_damage";
    else if (function != 1 && function != 2 && function != 3 && function != 8) return std::nullopt;
    if (stat.empty()) return std::nullopt;
    const auto priority = priorities.contains(stat) ? priorities.at(stat) : 0;
    return RangeCatalog::PropertyInfo{stat, tooltip, priority};
}

std::vector<ModifierRange> ReadMagicModifiers(const Row& row,
    const std::unordered_map<std::string, RangeCatalog::PropertyInfo>& properties) {
    std::vector<ModifierRange> result;
    for (std::size_t slot = 1; slot <= 3; ++slot) {
        const auto number = std::to_string(slot);
        const auto code = Lower(Get(row, "mod" + number + "code"));
        const auto property = properties.find(code);
        if (property == properties.end()) continue;
        auto minimum = Number(row, "mod" + number + "min");
        auto maximum = Number(row, "mod" + number + "max");
        if (minimum > maximum) std::swap(minimum, maximum);
        result.push_back({property->second.key, property->second.anchor,
            minimum, maximum, property->second.priority});
    }
    return result;
}

std::vector<ModifierRange> ReadItemModifiers(const Row& row, std::size_t count,
    const std::unordered_map<std::string, RangeCatalog::PropertyInfo>& properties,
    std::string_view codePrefix = "prop", std::string_view minPrefix = "min",
    std::string_view maxPrefix = "max") {
    std::vector<ModifierRange> result;
    for (std::size_t slot = 1; slot <= count; ++slot) {
        const auto number = std::to_string(slot);
        const auto code = Lower(Get(row, std::string(codePrefix) + number));
        const auto property = properties.find(code);
        if (property == properties.end()) continue;
        auto minimum = Number(row, std::string(minPrefix) + number);
        auto maximum = Number(row, std::string(maxPrefix) + number);
        if (minimum > maximum) std::swap(minimum, maximum);
        result.push_back({property->second.key, property->second.anchor,
            minimum, maximum, property->second.priority});
    }
    return result;
}

std::string RecipeInputToken(std::string value) {
    value = Lower(Trim(std::move(value)));
    const auto comma = value.find(',');
    if (comma != std::string::npos) value.resize(comma);
    return Trim(std::move(value));
}

} // namespace

bool RangeCatalog::Load(const std::filesystem::path& excelDirectory, std::string& error) {
    properties_.clear(); suffixes_.clear(); prefixes_.clear(); automagic_.clear();
    uniques_.clear(); sets_.clear(); armor_.clear(); itemTypes_.clear(); crafts_.clear();

    std::vector<Row> stats, properties;
    if (!ReadTsv(excelDirectory / "itemstatcost.txt", stats, error)
        || !ReadTsv(excelDirectory / "properties.txt", properties, error)) return false;
    std::unordered_map<std::string, std::int32_t> priorities;
    for (const auto& row : stats) priorities[Lower(Get(row, "stat"))] = Number(row, "descpriority");
    for (const auto& row : properties) {
        const auto code = Lower(Get(row, "code"));
        if (auto decoded = DecodeProperty(row, priorities)) properties_[code] = std::move(*decoded);
    }

    auto loadMagic = [&](std::string_view file, std::vector<std::vector<ModifierRange>>& target) {
        std::vector<Row> rows;
        if (!ReadTsv(excelDirectory / file, rows, error)) return false;
        target.resize(rows.size() + 1);
        for (std::size_t index = 0; index < rows.size(); ++index)
            target[index + 1] = ReadMagicModifiers(rows[index], properties_);
        return true;
    };
    if (!loadMagic("magicsuffix.txt", suffixes_)
        || !loadMagic("magicprefix.txt", prefixes_)
        || !loadMagic("automagic.txt", automagic_)) return false;

    auto loadItems = [&](std::string_view file, std::size_t count,
        std::vector<std::vector<ModifierRange>>& target) {
        std::vector<Row> rows;
        if (!ReadTsv(excelDirectory / file, rows, error)) return false;
        std::size_t maximum{};
        for (const auto& row : rows) maximum = std::max(maximum,
            static_cast<std::size_t>(std::max(0, Number(row, "*id"))));
        target.resize(maximum + 1);
        for (const auto& row : rows) {
            const auto id = static_cast<std::size_t>(std::max(0, Number(row, "*id")));
            target[id] = ReadItemModifiers(row, count, properties_);
        }
        return true;
    };
    if (!loadItems("uniqueitems.txt", 12, uniques_)
        || !loadItems("setitems.txt", 9, sets_)) return false;

    std::vector<Row> armor;
    if (!ReadTsv(excelDirectory / "armor.txt", armor, error)) return false;
    for (const auto& row : armor) {
        const auto code = Lower(Get(row, "code"));
        if (!code.empty()) armor_[code] = {Number(row, "minac"), Number(row, "maxac")};
    }

    std::unordered_map<std::string, std::vector<std::string>> typeParents;
    std::vector<Row> typeRows;
    if (!ReadTsv(excelDirectory / "itemtypes.txt", typeRows, error)) return false;
    for (const auto& row : typeRows) {
        const auto code = Lower(Get(row, "code"));
        if (code.empty()) continue;
        for (const auto key : {"equiv1", "equiv2"}) {
            const auto parent = Lower(Get(row, key));
            if (!parent.empty()) typeParents[code].push_back(parent);
        }
    }
    auto addItemTable = [&](std::string_view file) {
        std::vector<Row> rows;
        if (!ReadTsv(excelDirectory / file, rows, error)) return false;
        for (const auto& row : rows) {
            const auto code = Lower(Get(row, "code"));
            const auto type = Lower(Get(row, "type"));
            if (code.empty() || type.empty()) continue;
            std::unordered_set<std::string> seen;
            std::vector<std::string> pending{type};
            while (!pending.empty()) {
                auto current = pending.back(); pending.pop_back();
                if (!seen.insert(current).second) continue;
                itemTypes_[code].push_back(current);
                if (const auto found = typeParents.find(current); found != typeParents.end())
                    pending.insert(pending.end(), found->second.begin(), found->second.end());
            }
        }
        return true;
    };
    if (!addItemTable("weapons.txt") || !addItemTable("armor.txt") || !addItemTable("misc.txt")) return false;

    std::vector<Row> cube;
    if (!ReadTsv(excelDirectory / "cubemain.txt", cube, error)) return false;
    for (const auto& row : cube) {
        if (Number(row, "enabled") == 0 || Lower(Get(row, "output")).find("crf") == std::string::npos) continue;
        const auto input = RecipeInputToken(Get(row, "input 1"));
        auto fixed = ReadItemModifiers(row, 5, properties_, "mod ", "mod ", "mod ");
        // Cube columns are "mod N", "mod N min", and "mod N max".
        fixed.clear();
        for (std::size_t slot = 1; slot <= 5; ++slot) {
            const auto number = std::to_string(slot);
            const auto property = properties_.find(Lower(Get(row, "mod " + number)));
            if (property == properties_.end()) continue;
            auto minimum = Number(row, "mod " + number + " min");
            auto maximum = Number(row, "mod " + number + " max");
            if (minimum > maximum) std::swap(minimum, maximum);
            fixed.push_back({property->second.key, property->second.anchor,
                minimum, maximum, property->second.priority});
        }
        if (!input.empty() && !fixed.empty()) crafts_[input].push_back(std::move(fixed));
    }
    return true;
}

std::vector<std::vector<ModifierRange>> RangeCatalog::ResolveCandidates(
    const ItemAffixIds& ids, std::string_view itemCode) const {
    std::vector<ModifierRange> affixes;
    const auto suffixCount = suffixes_.empty() ? 0U : suffixes_.size() - 1U;
    const auto prefixCount = prefixes_.empty() ? 0U : prefixes_.size() - 1U;
    auto suffix = [&](std::uint16_t id) { AddRecord(suffixes_, id, affixes); };
    auto prefix = [&](std::uint16_t id) {
        AddRecord(prefixes_, id > suffixCount ? id - suffixCount : id, affixes);
    };
    prefix(ids.rarePrefix); suffix(ids.rareSuffix);
    for (const auto id : ids.magicPrefix) prefix(id);
    for (const auto id : ids.magicSuffix) suffix(id);
    if (ids.autoPrefix) {
        const auto offset = suffixCount + prefixCount;
        AddRecord(automagic_, ids.autoPrefix > offset ? ids.autoPrefix - offset : ids.autoPrefix, affixes);
    }
    if (ids.quality == 5) AddRecord(sets_, ids.fileIndex, affixes);
    if (ids.quality == 7) AddRecord(uniques_, ids.fileIndex, affixes);

    std::vector<std::vector<ModifierRange>> result;
    if (ids.quality == 8) {
        std::set<const std::vector<ModifierRange>*> recipes;
        const auto collect = [&](std::string_view token) {
            if (const auto found = crafts_.find(std::string(token)); found != crafts_.end())
                for (const auto& recipe : found->second) recipes.insert(&recipe);
        };
        collect(Lower(std::string(itemCode)));
        if (const auto types = itemTypes_.find(Lower(std::string(itemCode))); types != itemTypes_.end())
            for (const auto& type : types->second) collect(type);
        for (const auto* recipe : recipes) {
            auto candidate = affixes;
            candidate.insert(candidate.end(), recipe->begin(), recipe->end());
            result.push_back(Combine(std::move(candidate)));
        }
    }
    if (result.empty()) result.push_back(Combine(std::move(affixes)));
    return result;
}

std::optional<ArmorRange> RangeCatalog::FindArmor(std::string_view code) const {
    const auto found = armor_.find(Lower(std::string(code)));
    return found == armor_.end() ? std::nullopt : std::optional(found->second);
}

std::optional<std::int32_t> FirstSignedInteger(std::string_view text) {
    const auto clean = StripColors(text);
    for (std::size_t index = 0; index < clean.size(); ++index) {
        if (!std::isdigit(static_cast<unsigned char>(clean[index]))
            && !((clean[index] == '+' || clean[index] == '-') && index + 1 < clean.size()
                && std::isdigit(static_cast<unsigned char>(clean[index + 1])))) continue;
        const auto start = index;
        if (clean[index] == '+' || clean[index] == '-') ++index;
        while (index < clean.size() && std::isdigit(static_cast<unsigned char>(clean[index]))) ++index;
        std::int32_t value{};
        // std::from_chars accepts a leading minus for signed integers, but not
        // the explicit plus used by D2 tooltip stats.
        const auto parseStart = clean[start] == '+' ? start + 1 : start;
        const auto parsed = std::from_chars(clean.data() + parseStart, clean.data() + index, value);
        if (parsed.ec == std::errc{}) return value;
    }
    return std::nullopt;
}

std::string FormatPositiveRange(std::int32_t minimum, std::int32_t maximum, char restoreColor) {
    auto first = std::llabs(static_cast<long long>(minimum));
    auto second = std::llabs(static_cast<long long>(maximum));
    if (first > second) std::swap(first, second);
    std::string result(ColorMarker, 3);
    result += "2[" + std::to_string(first) + " - " + std::to_string(second) + "]";
    result.append(ColorMarker, 3);
    result.push_back(restoreColor);
    return result;
}

std::string AppendConsensusRanges(std::string_view tooltip,
    const std::vector<std::vector<ModifierRange>>& candidates) {
    if (candidates.empty()) return std::string(tooltip);
    std::vector<std::string> lines;
    std::size_t start{};
    while (start <= tooltip.size()) {
        const auto end = tooltip.find('\n', start);
        lines.emplace_back(tooltip.substr(start, end - start));
        if (end == std::string_view::npos) break;
        start = end + 1;
    }
    for (auto& line : lines) {
        const auto roll = FirstSignedInteger(line);
        if (!roll) continue;
        std::optional<std::pair<std::int32_t, std::int32_t>> consensus;
        bool ambiguous{};
        for (const auto& candidate : candidates) {
            std::vector<const ModifierRange*> matches;
            for (const auto& range : candidate)
                if (range.minimum != range.maximum && DescriptionFits(line, range) && RollFits(*roll, range))
                    matches.push_back(&range);
            // Recipe identity is not stored on the item. Discard recipe
            // candidates that cannot produce the displayed roll, then require
            // all surviving candidates to agree exactly.
            if (matches.empty()) continue;
            if (matches.size() != 1) { ambiguous = true; break; }
            const auto current = std::pair{matches[0]->minimum, matches[0]->maximum};
            if (!consensus) consensus = current;
            else if (*consensus != current) { ambiguous = true; break; }
        }
        if (!ambiguous && consensus) {
            line += " " + FormatPositiveRange(consensus->first, consensus->second, LastColor(line));
        }
    }
    std::string result;
    for (std::size_t index = 0; index < lines.size(); ++index) {
        if (index) result.push_back('\n');
        result += lines[index];
    }
    return result;
}

} // namespace tcp::tooltips
