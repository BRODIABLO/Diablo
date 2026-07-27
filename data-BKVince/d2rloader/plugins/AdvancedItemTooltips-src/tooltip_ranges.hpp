#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace tcp::tooltips {

struct ModifierRange {
    std::string key;
    std::string anchor;
    std::int32_t minimum{};
    std::int32_t maximum{};
    std::int32_t priority{};
};

struct ArmorRange {
    std::int32_t minimum{};
    std::int32_t maximum{};
};

struct ItemAffixIds {
    std::uint32_t quality{};
    std::uint32_t fileIndex{};
    std::uint16_t autoPrefix{};
    std::uint16_t rarePrefix{};
    std::uint16_t rareSuffix{};
    std::uint16_t magicPrefix[3]{};
    std::uint16_t magicSuffix[3]{};
};

class RangeCatalog {
public:
    struct PropertyInfo {
        std::string key;
        std::string anchor;
        std::int32_t priority{};
    };

    bool Load(const std::filesystem::path& excelDirectory, std::string& error);
    [[nodiscard]] std::vector<std::vector<ModifierRange>> ResolveCandidates(
        const ItemAffixIds& ids,
        std::string_view itemCode
    ) const;
    [[nodiscard]] std::optional<ArmorRange> FindArmor(std::string_view code) const;
    [[nodiscard]] std::size_t PropertyCount() const noexcept { return properties_.size(); }

private:
    std::unordered_map<std::string, PropertyInfo> properties_;
    std::vector<std::vector<ModifierRange>> suffixes_;
    std::vector<std::vector<ModifierRange>> prefixes_;
    std::vector<std::vector<ModifierRange>> automagic_;
    std::vector<std::vector<ModifierRange>> uniques_;
    std::vector<std::vector<ModifierRange>> sets_;
    std::unordered_map<std::string, ArmorRange> armor_;
    std::unordered_map<std::string, std::vector<std::string>> itemTypes_;
    std::unordered_map<std::string, std::vector<std::vector<ModifierRange>>> crafts_;
};

[[nodiscard]] std::string AppendConsensusRanges(
    std::string_view tooltip,
    const std::vector<std::vector<ModifierRange>>& candidates
);

[[nodiscard]] std::string FormatPositiveRange(
    std::int32_t minimum,
    std::int32_t maximum,
    char restoreColor = '0'
);

[[nodiscard]] std::optional<std::int32_t> FirstSignedInteger(std::string_view text);

} // namespace tcp::tooltips
