#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace D2RL {
struct PluginContext;
}

namespace RuffnecKk::MapSense {

enum class RevealOutcome : std::uint32_t {
    Complete,
    Unavailable,
    Armed,
    Disarmed,
};

struct RevealCounters {
    std::uint64_t levels{};
    std::uint64_t rooms{};
    std::uint64_t failures{};
    std::uint64_t traversalLimits{};
};

inline constexpr std::int32_t UnknownRevealDifficulty = -1;
inline constexpr std::size_t RevealDifficultyCount = 3U;
inline constexpr std::size_t RevealPersistenceActCount = 5U;
inline constexpr std::size_t RevealPersistenceLevelCapacity = 256U;

// Authoritative contiguous act boundaries from the shipped D2R 3.3 Levels.txt
// catalog (level ids 1..137). This lets reveal persistence bind an accepted
// request to the DRLG that was actually resolved, rather than guessing from a
// later lifecycle transition.
[[nodiscard]] constexpr auto RevealActForLevelId(
        std::int32_t levelId) noexcept -> std::int32_t {
    if (levelId >= 1 && levelId <= 39) return 0;
    if (levelId >= 40 && levelId <= 74) return 1;
    if (levelId >= 75 && levelId <= 102) return 2;
    if (levelId >= 103 && levelId <= 108) return 3;
    if (levelId >= 109 && levelId <= 137) return 4;
    return -1;
}

enum class RevealDifficultyObservation : std::uint8_t {
    Invalid,
    Initialized,
    Unchanged,
    Changed,
};

// Pure process-lifetime intent and per-session de-duplication state. It stores
// only stable ids, never DRLG pointers, generated coordinates, or automap bits.
// Callers must first observe a validated 0..2 client difficulty.
class RevealPersistenceState final {
public:
    void ResetProcess() noexcept {
        difficulty_ = UnknownRevealDifficulty;
        revealAll_ = false;
        rememberedActs_.fill(false);
        rememberedLevelCount_ = 0U;
        sessionGeneration_ = 0U;
        ResetSessionAcceptance();
    }

    void BeginSession(std::uint64_t sessionGeneration) noexcept {
        if (sessionGeneration_ == sessionGeneration) return;
        sessionGeneration_ = sessionGeneration;
        ResetSessionAcceptance();
    }

    [[nodiscard]] auto ObserveDifficulty(
            std::int32_t difficulty) noexcept
            -> RevealDifficultyObservation {
        if (!IsValidDifficulty(difficulty)) {
            return RevealDifficultyObservation::Invalid;
        }
        if (difficulty_ == UnknownRevealDifficulty) {
            difficulty_ = difficulty;
            return RevealDifficultyObservation::Initialized;
        }
        if (difficulty_ == difficulty) {
            return RevealDifficultyObservation::Unchanged;
        }
        difficulty_ = difficulty;
        revealAll_ = false;
        rememberedActs_.fill(false);
        rememberedLevelCount_ = 0U;
        ResetSessionAcceptance();
        return RevealDifficultyObservation::Changed;
    }

    [[nodiscard]] auto Difficulty() const noexcept -> std::int32_t {
        return difficulty_;
    }

    [[nodiscard]] auto SessionGeneration() const noexcept -> std::uint64_t {
        return sessionGeneration_;
    }

    [[nodiscard]] auto SetRevealAll(
            std::int32_t difficulty,
            bool armed) noexcept -> bool {
        if (!MatchesDifficulty(difficulty)) return false;
        revealAll_ = armed;
        return true;
    }

    void ClearRevealAll() noexcept {
        revealAll_ = false;
    }

    [[nodiscard]] auto IsRevealAllArmed(
            std::int32_t difficulty) const noexcept -> bool {
        return MatchesDifficulty(difficulty) && revealAll_;
    }

    [[nodiscard]] auto RememberLevel(
            std::int32_t difficulty,
            std::int32_t levelId) noexcept -> bool {
        if (!MatchesDifficulty(difficulty) || levelId <= 0) return false;
        return AddUnique(
            rememberedLevels_,
            rememberedLevelCount_,
            levelId);
    }

    [[nodiscard]] auto RememberAct(
            std::int32_t difficulty,
            std::int32_t act) noexcept -> bool {
        if (!MatchesDifficulty(difficulty) || !IsValidAct(act)) return false;
        rememberedActs_[static_cast<std::size_t>(act)] = true;
        return true;
    }

    [[nodiscard]] auto HasAnyIntent(
            std::int32_t difficulty) const noexcept -> bool {
        if (!MatchesDifficulty(difficulty)) return false;
        if (revealAll_ || rememberedLevelCount_ != 0U) return true;
        for (const bool remembered : rememberedActs_) {
            if (remembered) return true;
        }
        return false;
    }

    [[nodiscard]] auto HasAnyIntent() const noexcept -> bool {
        return difficulty_ != UnknownRevealDifficulty
            && HasAnyIntent(difficulty_);
    }

    [[nodiscard]] auto ShouldReplayLevel(
            std::int32_t difficulty,
            std::int32_t levelId) const noexcept -> bool {
        return sessionGeneration_ != 0U
            && MatchesDifficulty(difficulty)
            && Contains(
                rememberedLevels_,
                rememberedLevelCount_,
                levelId)
            && !Contains(
                acceptedLevels_,
                acceptedLevelCount_,
                levelId);
    }

    // Reveal Act and Reveal All are process-lifetime intents, but native
    // materialization is intentionally limited to the active client level.
    // Each loaded level is accepted independently for the current gameplay
    // session so no act-wide DRLG initialization or console worker is needed.
    [[nodiscard]] auto HasReplayIntentForLevel(
            std::int32_t difficulty,
            std::int32_t act,
            std::int32_t levelId) const noexcept -> bool {
        if (sessionGeneration_ == 0U
            || !MatchesDifficulty(difficulty) || levelId <= 0) {
            return false;
        }
        const bool rememberedLevel = Contains(
            rememberedLevels_,
            rememberedLevelCount_,
            levelId);
        const bool rememberedAct = IsValidAct(act)
            && rememberedActs_[static_cast<std::size_t>(act)];
        return rememberedLevel || rememberedAct || revealAll_;
    }

    [[nodiscard]] auto ShouldReplayCurrentLevel(
            std::int32_t difficulty,
            std::int32_t act,
            std::int32_t levelId) const noexcept -> bool {
        return HasReplayIntentForLevel(difficulty, act, levelId)
            && !Contains(
                acceptedLevels_,
                acceptedLevelCount_,
                levelId);
    }

    [[nodiscard]] auto IsLevelAccepted(
            std::int32_t difficulty,
            std::int32_t levelId) const noexcept -> bool {
        return MatchesDifficulty(difficulty) && levelId > 0
            && Contains(
                acceptedLevels_,
                acceptedLevelCount_,
                levelId);
    }

    [[nodiscard]] auto MarkLevelAccepted(
            std::int32_t difficulty,
            std::int32_t levelId) noexcept -> bool {
        if (sessionGeneration_ == 0U
            || !MatchesDifficulty(difficulty) || levelId <= 0) {
            return false;
        }
        return AddUnique(acceptedLevels_, acceptedLevelCount_, levelId);
    }

private:
    static constexpr auto IsValidDifficulty(
            std::int32_t difficulty) noexcept -> bool {
        return difficulty >= 0
            && difficulty < static_cast<std::int32_t>(RevealDifficultyCount);
    }

    static constexpr auto IsValidAct(std::int32_t act) noexcept -> bool {
        return act >= 0
            && act < static_cast<std::int32_t>(RevealPersistenceActCount);
    }

    [[nodiscard]] auto MatchesDifficulty(
            std::int32_t difficulty) const noexcept -> bool {
        return IsValidDifficulty(difficulty) && difficulty_ == difficulty;
    }

    static auto Contains(
            const std::array<std::int32_t,
                RevealPersistenceLevelCapacity>& values,
            std::size_t count,
            std::int32_t value) noexcept -> bool {
        for (std::size_t index = 0U; index < count; ++index) {
            if (values[index] == value) return true;
        }
        return false;
    }

    static auto AddUnique(
            std::array<std::int32_t,
                RevealPersistenceLevelCapacity>& values,
            std::size_t& count,
            std::int32_t value) noexcept -> bool {
        if (Contains(values, count, value)) return true;
        if (count >= values.size()) return false;
        values[count++] = value;
        return true;
    }

    void ResetSessionAcceptance() noexcept {
        acceptedLevelCount_ = 0U;
    }

    std::int32_t difficulty_{UnknownRevealDifficulty};
    bool revealAll_{};
    std::array<bool, RevealPersistenceActCount> rememberedActs_{};
    std::array<std::int32_t,
        RevealPersistenceLevelCapacity> rememberedLevels_{};
    std::size_t rememberedLevelCount_{};
    std::uint64_t sessionGeneration_{};
    std::array<std::int32_t,
        RevealPersistenceLevelCapacity> acceptedLevels_{};
    std::size_t acceptedLevelCount_{};
};

// Borrowed native view resolved from the client DRLG captured by the existing
// InitLevel hook. Pointers are valid only during the synchronous UI-thread
// operation that requested the view; callers must never retain them.
struct ClientLevelView final {
    std::uint8_t dataContext{};
    // Validated client DRLG difficulty: 0 Normal, 1 Nightmare, 2 Hell.
    std::uint8_t difficulty{};
    std::int32_t levelId{};
    void* activeRoom{};
    void* drlg{};
    void* level{};
};

auto InitializeRevealEngine(
    const D2RL::PluginContext* context,
    bool diagnostics) noexcept -> bool;
void ShutdownRevealEngine() noexcept;
// Session resets clear native DRLG references and diagnostics but intentionally
// preserve process-lifetime Reveal Level, Reveal Act, and Reveal All intents.
// The plugin coordinator invalidates them on a validated difficulty change.
void BeginRevealSession() noexcept;
void ResetRevealSession() noexcept;

auto RevealCurrentZone() noexcept -> RevealOutcome;
auto RevealCurrentAct() noexcept -> RevealOutcome;
auto ToggleRevealAll() noexcept -> RevealOutcome;
auto DisableRevealAll() noexcept -> RevealOutcome;

// Navigation and Reveal must inspect the same client-side generated map.
// These helpers centralize the governed DRLG_GetLevel/DRLG_InitLevel and
// DRLGROOM_CreateActiveRoom calls already owned and fingerprinted here.
[[nodiscard]] auto ResolveCurrentClientLevelView(
    ClientLevelView& output) noexcept -> bool;
[[nodiscard]] auto ResolveClientLevelById(
    const ClientLevelView& current,
    std::int32_t levelId,
    void*& outputLevel) noexcept -> bool;
[[nodiscard]] auto MaterializeClientRoom(
    std::uint8_t dataContext,
    void* drlgRoom) noexcept -> void*;

auto IsRevealEngineActive() noexcept -> bool;
auto IsRevealAllArmed() noexcept -> bool;
auto GetRevealCounters() noexcept -> RevealCounters;

} // namespace RuffnecKk::MapSense
