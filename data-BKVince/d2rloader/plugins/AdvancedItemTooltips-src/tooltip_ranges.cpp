#include "tooltip_ranges.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <cstdlib>
#include <fstream>
#include <map>
#include <set>
#include <tuple>
#include <unordered_set>

namespace tcp::tooltips {
namespace {

using Row = std::unordered_map<std::string, std::string>;
constexpr char ColorMarker[] = "\xEE\x81\xBE";

struct ColorCode {
    char value{};
    std::size_t length{};
};

std::optional<ColorCode> ReadColorCode(std::string_view text, std::size_t index) {
    if (index + 3 < text.size()
        && static_cast<unsigned char>(text[index]) == 0xEE
        && static_cast<unsigned char>(text[index + 1]) == 0x81
        && static_cast<unsigned char>(text[index + 2]) == 0xBE) {
        return ColorCode{text[index + 3], 4};
    }
    if (index + 2 < text.size()
        && static_cast<unsigned char>(text[index]) == 0xFF
        && text[index + 1] == 'c') {
        return ColorCode{text[index + 2], 3};
    }
    if (index + 3 < text.size()
        && static_cast<unsigned char>(text[index]) == 0xC3
        && static_cast<unsigned char>(text[index + 1]) == 0xBF
        && text[index + 2] == 'c') {
        return ColorCode{text[index + 3], 4};
    }
    return std::nullopt;
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
        if (const auto color = ReadColorCode(text, index)) {
            index += color->length;
        } else {
            clean.push_back(text[index++]);
        }
    }
    return clean;
}

char LastColor(std::string_view line, char inheritedColor) {
    char result = inheritedColor;
    for (std::size_t index = 0; index < line.size();) {
        if (const auto color = ReadColorCode(line, index)) {
            result = color->value;
            index += color->length;
        } else {
            ++index;
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

std::string NormalizeAnchor(std::string_view text) {
    std::string withoutPlaceholders;
    withoutPlaceholders.reserve(text.size());
    bool placeholder{};
    for (const auto ch : text) {
        if (ch == '[') placeholder = true;
        else if (ch == ']') placeholder = false;
        else if (!placeholder) withoutPlaceholders.push_back(ch);
    }
    return NormalizeWords(withoutPlaceholders);
}

std::vector<std::int32_t> SignedIntegers(std::string_view text) {
    const auto clean = StripColors(text);
    std::vector<std::int32_t> values;
    for (std::size_t index = 0; index < clean.size(); ++index) {
        if (!std::isdigit(static_cast<unsigned char>(clean[index]))
            && !((clean[index] == '+' || clean[index] == '-') && index + 1 < clean.size()
                && std::isdigit(static_cast<unsigned char>(clean[index + 1])))) continue;
        const auto start = index;
        if (clean[index] == '+' || clean[index] == '-') ++index;
        while (index < clean.size() && std::isdigit(static_cast<unsigned char>(clean[index]))) ++index;
        std::int32_t value{};
        const auto parseStart = clean[start] == '+' ? start + 1 : start;
        const auto parsed = std::from_chars(clean.data() + parseStart, clean.data() + index, value);
        if (parsed.ec == std::errc{}) values.push_back(value);
        if (index) --index;
    }
    return values;
}

std::optional<std::size_t> CompoundDamageSlot(std::string_view normalizedLine,
    std::string_view key) {
    if (!normalizedLine.starts_with("adds ") || !normalizedLine.ends_with(" damage"))
        return std::nullopt;
    constexpr std::array components{
        std::array<std::string_view, 3>{"mindamage", "maxdamage", " damage"},
        std::array<std::string_view, 3>{"firemindam", "firemaxdam", "fire damage"},
        std::array<std::string_view, 3>{"lightmindam", "lightmaxdam", "lightning damage"},
        std::array<std::string_view, 3>{"magicmindam", "magicmaxdam", "magic damage"},
        std::array<std::string_view, 3>{"coldmindam", "coldmaxdam", "cold damage"}
    };
    for (const auto& component : components) {
        if (normalizedLine.find(component[2]) == std::string_view::npos) continue;
        if (key == component[0]) return 0;
        if (key == component[1]) return 1;
    }
    return std::nullopt;
}

struct VisibleRoll {
    std::int32_t value{};
    std::size_t slot{};
};

std::optional<VisibleRoll> RollForRange(std::string_view line, const ModifierRange& range) {
    const auto values = SignedIntegers(line);
    if (values.empty()) return std::nullopt;
    if (const auto slot = CompoundDamageSlot(NormalizeWords(line), range.key)) {
        if (*slot >= values.size()) return std::nullopt;
        return VisibleRoll{values[*slot], *slot};
    }
    return VisibleRoll{values.front(), 0};
}

bool DescriptionFits(std::string_view line, const ModifierRange& range) {
    const auto anchor = NormalizeAnchor(range.anchor);
    const auto normalizedLine = NormalizeWords(line);
    const auto cleanLine = StripColors(line);
    const auto firstVisible = cleanLine.find_first_not_of(" \t\r");
    const auto hasExplicitSign = firstVisible != std::string::npos
        && (cleanLine[firstVisible] == '+' || cleanLine[firstVisible] == '-');
    if (anchor.empty()) return false;
    // Structural item lines are metadata, not rolled modifiers. Without this
    // gate, "Required Strength: 110" can match a variable Strength affix and
    // reject the item's complete candidate. Keep the patterns specific so
    // real modifiers such as Defense vs. Missile remain eligible.
    constexpr std::array metadataPrefixes{
        "required ", "one hand damage ", "two hand damage ", "throw damage ",
        "durability ", "chance to block ", "item level ", "affix level ",
        "base defense ", "max sockets ", "socketed ", "sell value ",
        "cost ", "quantity "};
    if ((normalizedLine == "defense" && !hasExplicitSign)
        || std::any_of(metadataPrefixes.begin(), metadataPrefixes.end(),
            [&](std::string_view prefix) {
                return normalizedLine.starts_with(prefix)
                    || (prefix.ends_with(' ')
                        && normalizedLine == prefix.substr(0, prefix.size() - 1));
            }))
        return false;
    if (CompoundDamageSlot(normalizedLine, range.key)) return true;
    auto position = std::size_t{};
    auto start = std::size_t{};
    while (start < anchor.size()) {
        const auto end = anchor.find(' ', start);
        const auto word = anchor.substr(start, end - start);
        position = normalizedLine.find(word, position);
        if (position == std::string::npos) return false;
        position += word.size();
        if (end == std::string::npos) break;
        start = end + 1;
    }
    if (!range.parameter.empty()) {
        const auto parameter = NormalizeWords(range.parameter);
        if (!parameter.empty() && normalizedLine.find(parameter) == std::string::npos) return false;
    }
    return true;
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
    else if (function != 1 && function != 2 && function != 3 && function != 8
        && function != 9 && function != 10 && function != 13
        && function != 21 && function != 22 && function != 24) return std::nullopt;
    if (stat.empty()) return std::nullopt;
    const auto priority = priorities.contains(stat) ? priorities.at(stat) : 0;
    const auto parameterized = function == 9 || function == 10 || function == 22 || function == 24;
    return RangeCatalog::PropertyInfo{stat, tooltip, priority, function, parameterized};
}

ModifierRange MakeRange(const std::string& code, const RangeCatalog::PropertyInfo& property,
    std::string parameter, std::int32_t minimum, std::int32_t maximum) {
    if (minimum > maximum) std::swap(minimum, maximum);
    auto key = property.key;
    if (property.parameterized) {
        parameter = Trim(std::move(parameter));
        key += ":" + code + ":" + Lower(parameter);
    } else {
        parameter.clear();
    }
    return {std::move(key), property.anchor, minimum, maximum, property.priority,
        std::move(parameter)};
}

void AppendPropertyRanges(
    const std::unordered_map<std::string, RangeCatalog::PropertyInfo>& properties,
    const std::string& code, std::string parameter, std::int32_t minimum,
    std::int32_t maximum, std::vector<ModifierRange>& result) {
    // Property functions 15/16 use the TXT minimum as the visible minimum
    // damage and the TXT maximum as the visible maximum damage. Model both
    // components separately so a collapsed "Adds X-Y Damage" tooltip can
    // still combine affixes, jewels, gems and runes without treating X and Y
    // as a single scalar roll.
    constexpr std::array compositeDamage{
        std::array<std::string_view, 3>{"dmg-fire", "fire-min", "fire-max"},
        std::array<std::string_view, 3>{"dmg-ltng", "ltng-min", "ltng-max"},
        std::array<std::string_view, 3>{"dmg-mag", "mag-min", "mag-max"},
        std::array<std::string_view, 3>{"dmg-cold", "cold-min", "cold-max"},
        std::array<std::string_view, 3>{"dmg-norm", "dmg-min", "dmg-max"}
    };
    if (const auto composite = std::find_if(compositeDamage.begin(), compositeDamage.end(),
            [&](const auto& entry) { return entry[0] == code; });
        composite != compositeDamage.end()) {
        const auto low = properties.find(std::string((*composite)[1]));
        const auto high = properties.find(std::string((*composite)[2]));
        if (low != properties.end() && high != properties.end()) {
            result.push_back(MakeRange(std::string((*composite)[1]), low->second, {},
                minimum, minimum));
            result.push_back(MakeRange(std::string((*composite)[2]), high->second, {},
                maximum, maximum));
        }
        return;
    }
    const auto property = properties.find(code);
    if (property == properties.end()) return;
    if (property->second.parameterized
        && std::none_of(parameter.begin(), parameter.end(), [](unsigned char ch) {
            return std::isalpha(ch) != 0 || ch >= 0x80;
        })) return;
    auto display = MakeRange(code, property->second, parameter, minimum, maximum);
    const auto appendComposite = [&](std::initializer_list<std::string_view> components) {
        display.key = "display:" + code;
        result.push_back(display);
        for (const auto component : components) {
            const auto found = properties.find(std::string(component));
            if (found == properties.end()) continue;
            result.push_back(MakeRange(std::string(component), found->second, parameter,
                minimum, maximum));
        }
    };
    if (code == "res-all") {
        appendComposite({"res-fire", "res-ltng", "res-cold", "res-pois"});
    } else if (code == "all-stats") {
        appendComposite({"str", "dex", "vit", "enr"});
    } else {
        result.push_back(std::move(display));
    }
}

std::vector<ModifierRange> ReadMagicModifiers(const Row& row,
    const std::unordered_map<std::string, RangeCatalog::PropertyInfo>& properties) {
    std::vector<ModifierRange> result;
    for (std::size_t slot = 1; slot <= 3; ++slot) {
        const auto number = std::to_string(slot);
        const auto code = Lower(Get(row, "mod" + number + "code"));
        AppendPropertyRanges(properties, code, Get(row, "mod" + number + "param"),
            Number(row, "mod" + number + "min"),
            Number(row, "mod" + number + "max"), result);
    }
    return result;
}

std::vector<ModifierRange> ReadItemModifiers(const Row& row, std::size_t count,
    const std::unordered_map<std::string, RangeCatalog::PropertyInfo>& properties,
    std::string_view codePrefix = "prop", std::string_view minPrefix = "min",
    std::string_view maxPrefix = "max", std::string_view paramPrefix = "par") {
    std::vector<ModifierRange> result;
    for (std::size_t slot = 1; slot <= count; ++slot) {
        const auto number = std::to_string(slot);
        const auto code = Lower(Get(row, std::string(codePrefix) + number));
        AppendPropertyRanges(properties, code, Get(row, std::string(paramPrefix) + number),
            Number(row, std::string(minPrefix) + number),
            Number(row, std::string(maxPrefix) + number), result);
    }
    return result;
}

std::string RecipeInputToken(std::string value) {
    value = Lower(Trim(std::move(value)));
    const auto comma = value.find(',');
    if (comma != std::string::npos) value.resize(comma);
    return Trim(std::move(value));
}

enum class SocketCategory { Unknown, Weapon, Armor, Shield };

SocketCategory FindSocketCategory(
    const std::unordered_map<std::string, std::vector<std::string>>& itemTypes,
    std::string_view itemCode) {
    const auto types = itemTypes.find(Lower(std::string(itemCode)));
    if (types == itemTypes.end()) return SocketCategory::Unknown;
    if (std::find(types->second.begin(), types->second.end(), "shld") != types->second.end())
        return SocketCategory::Shield;
    if (std::find(types->second.begin(), types->second.end(), "weap") != types->second.end())
        return SocketCategory::Weapon;
    if (std::find(types->second.begin(), types->second.end(), "armo") != types->second.end())
        return SocketCategory::Armor;
    return SocketCategory::Unknown;
}

} // namespace

bool RangeCatalog::Load(const std::filesystem::path& excelDirectory, std::string& error) {
    properties_.clear(); suffixes_.clear(); prefixes_.clear(); automagic_.clear();
    superiors_.clear();
    uniques_.clear(); sets_.clear(); armor_.clear(); itemTypes_.clear(); crafts_.clear();
    runes_.clear(); runewords_.clear(); runewordKeys_.clear();

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
        target.reserve(rows.size() + 1);
        target.emplace_back(); // Affix id 0 means no affix.
        for (const auto& row : rows) {
            // The TXT compiler treats the named Expansion row as a section
            // delimiter, not as a compiled affix record. Keeping it in the
            // vector shifts every later runtime id by one.
            if (Lower(Get(row, "name")) == "expansion") continue;
            target.push_back(ReadMagicModifiers(row, properties_));
        }
        return true;
    };
    if (!loadMagic("magicsuffix.txt", suffixes_)
        || !loadMagic("magicprefix.txt", prefixes_)
        || !loadMagic("automagic.txt", automagic_)) return false;

    // Superior quality stores the selected qualityitems.txt row in fileIndex.
    // Unlike the three affix tables, qualityitems has no reserved zero id and
    // its records are compiled in physical row order.
    std::vector<Row> superiorRows;
    if (!ReadTsv(excelDirectory / "qualityitems.txt", superiorRows, error)) return false;
    for (const auto& row : superiorRows)
        superiors_.push_back(ReadMagicModifiers(row, properties_));

    auto loadItems = [&](std::string_view file, std::size_t count,
        std::vector<std::vector<ModifierRange>>& target) {
        std::vector<Row> rows;
        if (!ReadTsv(excelDirectory / file, rows, error)) return false;
        target.clear();
        for (const auto& row : rows) {
            const auto text = Trim(Get(row, "*id"));
            // Section labels such as Expansion, Armor and Rings have no *ID.
            // They are not compiled records and must not overwrite the genuine
            // record 0 (The Gnasher / Civerb's Ward).
            if (text.empty()) continue;
            std::size_t id{};
            const auto parsed = std::from_chars(text.data(), text.data() + text.size(), id);
            if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size()) {
                error = "Invalid *ID in " + std::string(file) + ": " + text;
                return false;
            }
            if (target.size() <= id) target.resize(id + 1);
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

    std::vector<Row> gems;
    if (!ReadTsv(excelDirectory / "gems.txt", gems, error)) return false;
    const auto readGemModifiers = [&](const Row& row, std::string_view group) {
        std::vector<ModifierRange> result;
        for (std::size_t slot = 1; slot <= 3; ++slot) {
            const auto stem = std::string(group) + "mod" + std::to_string(slot);
            const auto code = Lower(Get(row, stem + "code"));
            AppendPropertyRanges(properties_, code, Get(row, stem + "param"),
                Number(row, stem + "min"), Number(row, stem + "max"), result);
        }
        return result;
    };
    for (const auto& row : gems) {
        const auto code = Lower(Get(row, "code"));
        if (code.empty()) continue;
        runes_[code] = {
            readGemModifiers(row, "weapon"),
            readGemModifiers(row, "helm"),
            readGemModifiers(row, "shield")
        };
    }

    std::vector<Row> runewords;
    if (!ReadTsv(excelDirectory / "runes.txt", runewords, error)) return false;
    for (const auto& row : runewords) {
        if (Number(row, "complete") == 0) continue;
        const auto key = Trim(Get(row, "name"));
        if (key.empty()) continue;
        RunewordRecord record;
        record.modifiers = ReadItemModifiers(
            row, 7, properties_, "t1code", "t1min", "t1max", "t1param");
        for (std::size_t slot = 1; slot <= 6; ++slot) {
            const auto rune = Lower(Trim(Get(row, "rune" + std::to_string(slot))));
            if (!rune.empty()) record.runes.push_back(rune);
        }
        if (!runewords_.emplace(key, std::move(record)).second) {
            error = "Duplicate active runeword key in runes.txt: " + key;
            return false;
        }
        runewordKeys_.push_back(key);
    }

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
            const auto code = Lower(Get(row, "mod " + number));
            auto minimum = Number(row, "mod " + number + " min");
            auto maximum = Number(row, "mod " + number + " max");
            AppendPropertyRanges(properties_, code, Get(row, "mod " + number + " param"),
                minimum, maximum, fixed);
        }
        if (!input.empty() && !fixed.empty()) crafts_[input].push_back(std::move(fixed));
    }
    return true;
}

std::vector<std::vector<ModifierRange>> RangeCatalog::ResolveCandidates(
    const ItemAffixIds& ids, std::string_view itemCode, std::string_view runewordKey) const {
    std::vector<ModifierRange> affixes;
    const auto suffixCount = suffixes_.empty() ? 0U : suffixes_.size() - 1U;
    const auto prefixCount = prefixes_.empty() ? 0U : prefixes_.size() - 1U;
    auto suffix = [&](std::uint16_t id) { AddRecord(suffixes_, id, affixes); };
    auto prefix = [&](std::uint16_t id) {
        AddRecord(prefixes_, id > suffixCount ? id - suffixCount : id, affixes);
    };
    // rarePrefix/rareSuffix choose the generated rare/crafted display name
    // (for example, "Stone Razor"). They are 8-bit RarePrefix/RareSuffix
    // table ids, not MagicPrefix/MagicSuffix stat affixes. Only the three
    // magic prefix/suffix slots below contribute item properties.
    // A runeword is created from a non-magic base. Its six magic affix slots
    // are not authoritative after IFLAG_RUNEWORD is set (the first one is
    // explicitly repurposed as the runeword string id, and the remaining
    // bytes can retain runtime payload). Never decode any of them as affixes.
    if (!ids.runeword) {
        for (const auto id : ids.magicPrefix) prefix(id);
        for (const auto id : ids.magicSuffix) suffix(id);
    }
    if (ids.autoPrefix) {
        const auto offset = suffixCount + prefixCount;
        AddRecord(automagic_, ids.autoPrefix > offset ? ids.autoPrefix - offset : ids.autoPrefix, affixes);
    }
    if (ids.quality == 5) AddRecord(sets_, ids.fileIndex, affixes);
    if (ids.quality == 7) AddRecord(uniques_, ids.fileIndex, affixes);
    if (!runewordKey.empty()) {
        const auto runeword = runewords_.find(std::string(runewordKey));
        if (runeword != runewords_.end()) {
            affixes.insert(affixes.end(), runeword->second.modifiers.begin(),
                runeword->second.modifiers.end());
            const auto category = FindSocketCategory(itemTypes_, itemCode);
            for (const auto& runeCode : runeword->second.runes) {
                const auto rune = runes_.find(runeCode);
                if (rune == runes_.end() || category == SocketCategory::Unknown) continue;
                const auto* modifiers = &rune->second.armor;
                if (category == SocketCategory::Weapon) modifiers = &rune->second.weapon;
                else if (category == SocketCategory::Shield) modifiers = &rune->second.shield;
                affixes.insert(affixes.end(), modifiers->begin(), modifiers->end());
            }
        }
    }

    std::vector<std::vector<ModifierRange>> result;
    if (ids.quality == 3) {
        // Runtime captures prove that D2R's superior fileIndex is not mapped
        // consistently to the physical qualityitems.txt row across every
        // generated record. Preserve both observed interpretations as
        // alternatives. Whole-tooltip validation then selects the candidate
        // that can reproduce all visible superior properties instead of
        // applying an unsafe global offset or item-specific exception.
        for (const auto record : {ids.fileIndex, ids.fileIndex + 1U}) {
            auto candidate = affixes;
            AddRecord(superiors_, record, candidate);
            const auto combined = Combine(std::move(candidate));
            if (std::find_if(result.begin(), result.end(), [&](const auto& existing) {
                    if (existing.size() != combined.size()) return false;
                    for (std::size_t index = 0; index < existing.size(); ++index) {
                        const auto& left = existing[index];
                        const auto& right = combined[index];
                        if (left.key != right.key || left.minimum != right.minimum
                            || left.maximum != right.maximum
                            || left.parameter != right.parameter) return false;
                    }
                    return true;
                }) == result.end()) result.push_back(combined);
        }
    }
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

std::vector<std::vector<ModifierRange>> RangeCatalog::ResolveSocketFillerCandidates(
    const ItemAffixIds& ids, std::string_view fillerCode, std::string_view parentCode) const {
    auto result = ResolveCandidates(ids, fillerCode);
    const auto gem = runes_.find(Lower(std::string(fillerCode)));
    if (gem == runes_.end()) return result;

    const auto category = FindSocketCategory(itemTypes_, parentCode);
    if (category == SocketCategory::Unknown) return result;
    const auto* modifiers = &gem->second.armor;
    if (category == SocketCategory::Weapon) modifiers = &gem->second.weapon;
    else if (category == SocketCategory::Shield) modifiers = &gem->second.shield;

    for (auto& candidate : result) {
        candidate.insert(candidate.end(), modifiers->begin(), modifiers->end());
        candidate = Combine(std::move(candidate));
    }
    return result;
}

std::optional<ArmorRange> RangeCatalog::FindArmor(std::string_view code) const {
    const auto found = armor_.find(Lower(std::string(code)));
    return found == armor_.end() ? std::nullopt : std::optional(found->second);
}

std::vector<std::vector<ModifierRange>> MergeCandidateSources(
    const std::vector<std::vector<ModifierRange>>& parent,
    const std::vector<std::vector<ModifierRange>>& child) {
    if (child.empty()) return parent;
    if (parent.empty()) return child;
    std::vector<std::vector<ModifierRange>> merged;
    merged.reserve(parent.size() * child.size());
    for (const auto& parentCandidate : parent) {
        for (const auto& childCandidate : child) {
            auto candidate = parentCandidate;
            candidate.insert(candidate.end(), childCandidate.begin(), childCandidate.end());
            merged.push_back(Combine(std::move(candidate)));
        }
    }
    return merged;
}

std::optional<std::int32_t> FirstSignedInteger(std::string_view text) {
    const auto values = SignedIntegers(text);
    return values.empty() ? std::nullopt : std::optional(values.front());
}

std::string FormatPositiveRange(std::int32_t minimum, std::int32_t maximum, char restoreColor) {
    auto first = std::llabs(static_cast<long long>(minimum));
    auto second = std::llabs(static_cast<long long>(maximum));
    if (first > second) std::swap(first, second);
    std::string result(ColorMarker, 3);
    // SlashDiablo's DARK_GREEN entry is ':' rather than the vanilla set-item
    // green (2). Keep D2R's private renderer marker so the sequence is
    // consumed as color metadata instead of falling back to another font.
    result += ":[" + std::to_string(first) + " - " + std::to_string(second) + "]";
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

    // Crafted items do not retain the cubemain recipe that created them. Use
    // the complete rendered tooltip to eliminate recipe candidates that
    // cannot reproduce a displayed stat before resolving any individual
    // line. A Caster Amulet with 15 FCR, for example, rejects the non-Caster
    // candidates whose fixed suffix can only produce 10 FCR; its Mana line can
    // then safely include the recipe's additional 10-20 Mana.
    std::vector<bool> modeledLines;
    modeledLines.reserve(lines.size());
    for (const auto& line : lines) {
        // A rendered value can include a source that is not represented by
        // the parent item's affix candidate (most notably a socketed jewel).
        // Such a line must not reject the complete candidate and erase ranges
        // for unrelated properties. Only use a line as candidate evidence
        // when at least one candidate can actually reproduce its final roll.
        modeledLines.push_back(!SignedIntegers(line).empty()
            && std::any_of(candidates.begin(), candidates.end(),
            [&](const auto& candidate) {
                return std::any_of(candidate.begin(), candidate.end(),
                    [&](const auto& range) {
                        const auto roll = RollForRange(line, range);
                        return DescriptionFits(line, range) && roll
                            && RollFits(roll->value, range);
                    });
            }));
    }

    std::vector<const std::vector<ModifierRange>*> compatibleCandidates;
    for (const auto& candidate : candidates) {
        bool compatible = true;
        for (std::size_t index = 0; index < lines.size(); ++index) {
            if (!modeledLines[index]) continue;
            bool producesRoll{};
            for (const auto& range : candidate) {
                const auto roll = RollForRange(lines[index], range);
                if (DescriptionFits(lines[index], range)
                    && roll && RollFits(roll->value, range)) producesRoll = true;
            }
            // Once any candidate identifies a rendered line as a modeled
            // property, every surviving recipe must contain and reproduce it.
            // This rejects an Ascended Caster recipe when a standard craft's
            // Experience Gained line is visibly present.
            if (!producesRoll) {
                compatible = false;
                break;
            }
        }
        if (compatible) compatibleCandidates.push_back(&candidate);
    }

    // D2 stores and parses final-tooltip lines bottom-to-top. A native color
    // marker may appear only on the first line of a modifier block; the lines
    // above it inherit that state. Track the renderer state in buffer order so
    // every dark-green suffix restores the actual native color.
    char activeColor = '0';
    for (auto& line : lines) {
        activeColor = LastColor(line, activeColor);
        if (SignedIntegers(line).empty()) continue;
        using ConsensusRange = std::tuple<std::size_t, std::string,
            std::int32_t, std::int32_t>;
        std::optional<std::vector<ConsensusRange>> consensus;
        bool ambiguous{};
        for (const auto* candidate : compatibleCandidates) {
            std::vector<ConsensusRange> matches;
            for (const auto& range : *candidate) {
                if (range.minimum == range.maximum || !DescriptionFits(line, range)) continue;
                const auto roll = RollForRange(line, range);
                if (roll && RollFits(roll->value, range))
                    matches.emplace_back(roll->slot, range.key, range.minimum, range.maximum);
            }
            // Recipe identity is not stored on the item. All candidates that
            // survived whole-tooltip validation must agree exactly.
            if (matches.empty()) continue;
            std::sort(matches.begin(), matches.end());
            if (std::adjacent_find(matches.begin(), matches.end(), [](const auto& left,
                    const auto& right) { return std::get<0>(left) == std::get<0>(right); })
                != matches.end()) {
                ambiguous = true;
                break;
            }
            if (!consensus) consensus = matches;
            else if (*consensus != matches) { ambiguous = true; break; }
        }
        if (!ambiguous && consensus) {
            line.push_back(' ');
            for (std::size_t index = 0; index < consensus->size(); ++index) {
                if (index) line += " to ";
                line += FormatPositiveRange(std::get<2>((*consensus)[index]),
                    std::get<3>((*consensus)[index]), activeColor);
            }
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
