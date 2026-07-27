#pragma once

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace ruffneckk::readable_items {

inline constexpr std::size_t MaximumItems = 256;
inline constexpr std::size_t MaximumTooltipBytes = 128;
inline constexpr std::size_t MaximumTitleBytes = 128;
inline constexpr std::size_t MaximumTextBytes = 8192;
inline constexpr std::string_view DefaultTooltip = "Right-click to read...";

struct Entry {
    std::uint32_t packedCode{};
    std::string code;
    std::string title;
    std::string text;
};

struct Config {
    bool enabled{true};
    std::string tooltip{DefaultTooltip};
    std::vector<Entry> items;
};

inline void ValidateObject(
    const nlohmann::json& value,
    std::string_view label,
    std::initializer_list<std::string_view> allowed
) {
    if (!value.is_object()) {
        throw std::invalid_argument(std::string(label) + " must be an object");
    }
    for (const auto& [key, ignored] : value.items()) {
        (void)ignored;
        bool known{};
        for (const auto candidate : allowed) {
            if (key == candidate) {
                known = true;
                break;
            }
        }
        if (!known) {
            throw std::invalid_argument(std::string(label) + " has unknown setting: " + key);
        }
    }
}

inline std::string ReadBoundedString(
    const nlohmann::json& object,
    std::string_view key,
    std::string_view label,
    std::size_t maximumBytes
) {
    if (!object.contains(key) || !object.at(key).is_string()) {
        throw std::invalid_argument(std::string(label) + " must be a string");
    }
    auto value = object.at(key).get<std::string>();
    if (value.empty() || value.size() > maximumBytes) {
        throw std::out_of_range(
            std::string(label) + " must contain 1 to " + std::to_string(maximumBytes) + " bytes");
    }
    return value;
}

inline std::uint32_t PackItemCode(std::string_view code) {
    if (code.empty() || code.size() > 4) {
        throw std::out_of_range("item code must contain 1 to 4 ASCII bytes");
    }
    for (const unsigned char character : code) {
        if (character < 0x21 || character > 0x7E) {
            throw std::invalid_argument("item code must contain visible ASCII characters only");
        }
    }

    // Compiled ItemsTxt codes are fixed-width four-byte values padded with
    // ASCII spaces.  A three-character JSON code such as "tsc" therefore
    // needs to match the runtime value 0x20637374, not 0x00637374.
    std::uint32_t packed{0x20202020U};
    std::memcpy(&packed, code.data(), code.size());
    return packed;
}

inline Config ParseConfig(const nlohmann::json& root) {
    ValidateObject(root, "configuration root", {"enabled", "tooltip", "items"});

    Config result;
    if (root.contains("enabled")) {
        if (!root.at("enabled").is_boolean()) {
            throw std::invalid_argument("enabled must be a boolean");
        }
        result.enabled = root.at("enabled").get<bool>();
    }
    if (root.contains("tooltip")) {
        result.tooltip = ReadBoundedString(
            root, "tooltip", "tooltip", MaximumTooltipBytes);
    }
    if (!root.contains("items") || !root.at("items").is_array()) {
        throw std::invalid_argument("items must be an array");
    }
    if (root.at("items").size() > MaximumItems) {
        throw std::out_of_range("items exceeds the 256-entry limit");
    }

    std::unordered_set<std::uint32_t> codes;
    for (std::size_t index{}; index < root.at("items").size(); ++index) {
        const auto& item = root.at("items").at(index);
        const auto label = std::string("items[") + std::to_string(index) + "]";
        ValidateObject(item, label, {"code", "title", "text"});

        Entry entry;
        entry.code = ReadBoundedString(item, "code", label + ".code", 4);
        entry.packedCode = PackItemCode(entry.code);
        entry.title = ReadBoundedString(
            item, "title", label + ".title", MaximumTitleBytes);
        entry.text = ReadBoundedString(
            item, "text", label + ".text", MaximumTextBytes);
        if (!codes.insert(entry.packedCode).second) {
            throw std::invalid_argument("duplicate item code: " + entry.code);
        }
        result.items.push_back(std::move(entry));
    }

    if (result.enabled && result.items.empty()) {
        throw std::invalid_argument("enabled configuration must contain at least one item");
    }
    return result;
}

inline const Entry* FindEntry(const Config& config, std::uint32_t packedCode) noexcept {
    for (const auto& item : config.items) {
        if (item.packedCode == packedCode) return &item;
    }
    return nullptr;
}

class ReaderState final {
public:
    bool Open(
        const Entry& entry,
        std::size_t renderedLineCount,
        std::size_t visibleLineCount
    ) {
        if (entry.packedCode == 0 || entry.title.empty() || entry.text.empty()) {
            return false;
        }

        active_ = true;
        packedCode_ = entry.packedCode;
        title_ = entry.title;
        text_ = entry.text;
        scrollOffset_ = 0;
        totalRevealCharacters_ = 0;
        revealedCharacters_ = 0;
        followingLatest_ = true;
        SetViewport(renderedLineCount, visibleLineCount);
        return true;
    }

    void Close() noexcept {
        active_ = false;
        packedCode_ = 0;
        title_.clear();
        text_.clear();
        renderedLineCount_ = 0;
        visibleLineCount_ = 0;
        scrollOffset_ = 0;
        totalRevealCharacters_ = 0;
        revealedCharacters_ = 0;
        followingLatest_ = true;
    }

    void SetViewport(
        std::size_t renderedLineCount,
        std::size_t visibleLineCount
    ) noexcept {
        renderedLineCount_ = renderedLineCount;
        visibleLineCount_ = visibleLineCount;
        scrollOffset_ = std::min(scrollOffset_, MaximumScrollOffset());
    }

    void ScrollLines(std::ptrdiff_t delta) noexcept {
        if (!active_ || delta == 0) return;

        if (delta < 0) {
            followingLatest_ = false;
            const auto magnitude = static_cast<std::size_t>(-(delta + 1)) + 1;
            scrollOffset_ = magnitude >= scrollOffset_ ? 0 : scrollOffset_ - magnitude;
            return;
        }

        const auto maximum = MaximumScrollOffset();
        const auto distance = static_cast<std::size_t>(delta);
        scrollOffset_ = distance >= maximum - scrollOffset_
            ? maximum
            : scrollOffset_ + distance;
        // A deliberate scroll pauses the typewriter follow mode until the
        // player returns to the newest visible line.
        followingLatest_ = scrollOffset_ == maximum;
    }

    void SetRevealCharacterCount(std::size_t total) noexcept {
        totalRevealCharacters_ = total;
        revealedCharacters_ = std::min(revealedCharacters_, totalRevealCharacters_);
    }

    void AdvanceReveal(std::size_t count) noexcept {
        if (!active_ || count == 0) return;
        const auto remaining = totalRevealCharacters_ - revealedCharacters_;
        revealedCharacters_ += std::min(count, remaining);
    }

    void RevealAll() noexcept {
        if (active_) revealedCharacters_ = totalRevealCharacters_;
    }

    void FollowLatestLine(std::size_t latestLine) noexcept {
        if (!active_ || !followingLatest_ || visibleLineCount_ == 0
            || renderedLineCount_ == 0) {
            return;
        }
        latestLine = std::min(latestLine, renderedLineCount_ - 1);
        scrollOffset_ = latestLine + 1 > visibleLineCount_
            ? latestLine + 1 - visibleLineCount_
            : 0;
        scrollOffset_ = std::min(scrollOffset_, MaximumScrollOffset());
    }

    void ResumeFollowingLatest() noexcept {
        followingLatest_ = true;
        scrollOffset_ = MaximumScrollOffset();
    }

    void SetScrollOffset(std::size_t offset) noexcept {
        if (!active_) return;
        const auto maximum = MaximumScrollOffset();
        scrollOffset_ = std::min(offset, maximum);
        followingLatest_ = scrollOffset_ == maximum;
    }

    [[nodiscard]] bool IsOpen() const noexcept { return active_; }
    [[nodiscard]] std::uint32_t PackedCode() const noexcept { return packedCode_; }
    [[nodiscard]] const std::string& Title() const noexcept { return title_; }
    [[nodiscard]] const std::string& Text() const noexcept { return text_; }
    [[nodiscard]] std::size_t ScrollOffset() const noexcept { return scrollOffset_; }
    [[nodiscard]] std::size_t RenderedLineCount() const noexcept {
        return renderedLineCount_;
    }
    [[nodiscard]] std::size_t VisibleLineCount() const noexcept {
        return visibleLineCount_;
    }
    [[nodiscard]] bool CanScrollUp() const noexcept {
        return active_ && scrollOffset_ > 0;
    }
    [[nodiscard]] bool CanScrollDown() const noexcept {
        return active_ && scrollOffset_ < MaximumScrollOffset();
    }
    [[nodiscard]] std::size_t RevealedCharacters() const noexcept {
        return revealedCharacters_;
    }
    [[nodiscard]] std::size_t TotalRevealCharacters() const noexcept {
        return totalRevealCharacters_;
    }
    [[nodiscard]] bool RevealComplete() const noexcept {
        return active_ && totalRevealCharacters_ > 0
            && revealedCharacters_ == totalRevealCharacters_;
    }
    [[nodiscard]] bool FollowingLatest() const noexcept {
        return followingLatest_;
    }

private:
    [[nodiscard]] std::size_t MaximumScrollOffset() const noexcept {
        if (!active_ || visibleLineCount_ == 0
            || renderedLineCount_ <= visibleLineCount_) {
            return 0;
        }
        return renderedLineCount_ - visibleLineCount_;
    }

    bool active_{};
    std::uint32_t packedCode_{};
    std::string title_;
    std::string text_;
    std::size_t renderedLineCount_{};
    std::size_t visibleLineCount_{};
    std::size_t scrollOffset_{};
    std::size_t totalRevealCharacters_{};
    std::size_t revealedCharacters_{};
    bool followingLatest_{true};
};

} // namespace ruffneckk::readable_items
