#pragma once

#include "book_of_lore_config.hpp"

#include <cstdint>
#include <cstring>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace ruffneckk::book_of_lore {

struct PlayerContext {
    std::uint64_t playerId{};
    std::uint32_t difficulty{1};
    std::uint32_t act{1};
    std::uint32_t area{1};
    std::uint32_t level{1};
    std::string playerClass;
    std::unordered_set<std::string> completedQuests;
    bool inTown{};
};

struct PlayerVariables {
    std::string name;
    std::string className;
    std::uint32_t level{1};
    std::string difficultyName;
    std::uint32_t act{1};
    std::string areaName;
    std::string title;
};

inline bool IsEligible(const Message& message, const PlayerContext& context) {
    const auto& filters = message.filters;
    if (filters.town != context.inTown) return false;

    if (filters.difficulty) {
        if (context.inTown) {
            if (context.difficulty != *filters.difficulty) return false;
        } else if (context.difficulty < *filters.difficulty) {
            return false;
        }
    }
    if (filters.act) {
        if (context.inTown) {
            if (context.act != *filters.act) return false;
        } else if (context.act < *filters.act) {
            return false;
        }
    }
    if (!context.inTown && filters.area && context.area < *filters.area) return false;
    if (filters.quest && !context.completedQuests.contains(*filters.quest)) return false;
    if (filters.playerClass && context.playerClass != *filters.playerClass) return false;
    if (filters.minLevel && context.level < *filters.minLevel) return false;
    if (filters.maxLevel
        && (!filters.minLevel || *filters.maxLevel >= *filters.minLevel)
        && context.level > *filters.maxLevel) return false;
    return true;
}

inline const Message* FindMessageById(
        const std::vector<Message>& messages, std::string_view id) noexcept {
    for (const auto& message : messages) {
        if (message.id == id) return &message;
    }
    return nullptr;
}

inline std::vector<const Message*> EligibleMessages(
        const std::vector<Message>& messages, const PlayerContext& context) {
    std::vector<const Message*> eligible;
    eligible.reserve(messages.size());
    for (const auto& message : messages) {
        if (IsEligible(message, context)) eligible.push_back(&message);
    }
    return eligible;
}

inline void ReplaceAll(
        std::string& text, std::string_view token, std::string_view replacement) {
    std::size_t position{};
    while ((position = text.find(token, position)) != std::string::npos) {
        text.replace(position, token.size(), replacement);
        position += replacement.size();
    }
}

inline std::string ExpandVariables(
        std::string text, const PlayerVariables& variables) {
    ReplaceAll(text, "##00", variables.name);
    ReplaceAll(text, "##01", variables.className);
    ReplaceAll(text, "##02", "Level " + std::to_string(variables.level));
    ReplaceAll(text, "##03", variables.difficultyName);
    ReplaceAll(text, "##04", "Act " + std::to_string(variables.act));
    ReplaceAll(text, "##05", variables.areaName);
    ReplaceAll(text, "##06", variables.title);
    return text;
}

inline std::string BuildScrollText(const Message& message) {
    if (message.title.empty()) return message.text;
    if (message.text.empty()) return message.title;
    return message.title + "\n\n" + message.text;
}

inline bool IsObjectScrollMessage(
        const std::uint8_t* packet,
        std::uint16_t expectedStringId) noexcept {
    if (!packet
        || packet[1] != 2
        || packet[6] == 0
        || packet[8] != 0) {
        return false;
    }
    std::uint16_t stringId{};
    std::memcpy(&stringId, packet + 0x0A, sizeof(stringId));
    return stringId == expectedStringId;
}

class SessionSelections {
public:
    const Message* Resolve(
            std::string_view bookId,
            const PlayerContext& context,
            const std::vector<Message>& messages,
            std::uint64_t randomOrdinal) {
        if (bookId.empty()) return nullptr;

        if (!context.inTown) {
            auto& book = books_[std::string(bookId)];
            if (const auto selected = book.byPlayer.find(context.playerId);
                selected != book.byPlayer.end()) {
                if (const auto* message = FindMessageById(messages, selected->second)) {
                    return message;
                }
                book.byPlayer.erase(selected);
            }

            for (const auto& sharedId : book.shared) {
                if (const auto* shared = FindMessageById(messages, sharedId);
                    shared && IsEligible(*shared, context)) {
                    book.byPlayer[context.playerId] = shared->id;
                    return shared;
                }
            }
        }

        const auto eligible = EligibleMessages(messages, context);
        if (eligible.empty()) return nullptr;
        const auto* selected = eligible[
            static_cast<std::size_t>(randomOrdinal % eligible.size())
        ];

        if (!context.inTown) {
            auto& book = books_[std::string(bookId)];
            book.byPlayer[context.playerId] = selected->id;
            if (selected->allSame) {
                bool alreadyShared{};
                for (const auto& sharedId : book.shared) {
                    if (sharedId == selected->id) {
                        alreadyShared = true;
                        break;
                    }
                }
                if (!alreadyShared) book.shared.push_back(selected->id);
            }
        }
        return selected;
    }

    void Clear() noexcept {
        books_.clear();
    }

    std::size_t BookCount() const noexcept {
        return books_.size();
    }

private:
    struct BookState {
        std::unordered_map<std::uint64_t, std::string> byPlayer;
        std::vector<std::string> shared;
    };

    std::unordered_map<std::string, BookState> books_;
};

} // namespace ruffneckk::book_of_lore
