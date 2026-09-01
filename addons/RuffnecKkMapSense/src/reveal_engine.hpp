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
    std::uint64_t staticRoomCandidates{};
    std::uint64_t staticRoomsMaterialized{};
    std::uint64_t staticRoomsReleased{};
    std::uint64_t staticRoomFailures{};
};

inline constexpr std::uint32_t StaticPoiRoomWarpMask = 0x00000FF0U;
inline constexpr std::uint32_t StaticPoiRoomWaypointMask = 0x00030000U;
inline constexpr std::uint32_t StaticPoiRoomHasRoomMask = 0x00100000U;
inline constexpr std::uint32_t StaticPoiRoomPresetUnitsAddedMask =
    0x02000000U;

enum class StaticPoiRoomAction : std::uint8_t {
    Ignore,
    Reuse,
    Materialize,
    Wait,
};

// Selects only rooms that can own an exact native exit or waypoint. The
// distant-name path never asks D2R to create an ActiveRoom.
[[nodiscard]] constexpr auto SelectStaticPoiRoomAction(
        std::uint32_t roomFlags,
        bool hasRoomTile,
        bool hasCompletePresetUnits,
        bool hasActiveRoom) noexcept -> StaticPoiRoomAction {
    const bool needsRoomTile =
        (roomFlags & StaticPoiRoomWarpMask) != 0U && !hasRoomTile;
    const bool needsPresetUnit =
        (roomFlags & StaticPoiRoomWaypointMask) != 0U
            && !hasCompletePresetUnits;
    if (!needsRoomTile && !needsPresetUnit) {
        return (roomFlags
                & (StaticPoiRoomWarpMask | StaticPoiRoomWaypointMask)) != 0U
            ? StaticPoiRoomAction::Reuse
            : StaticPoiRoomAction::Ignore;
    }
    if (hasActiveRoom || (roomFlags & StaticPoiRoomHasRoomMask) != 0U) {
        return StaticPoiRoomAction::Wait;
    }
    return StaticPoiRoomAction::Materialize;
}

struct StaticClientRoomLease final {
    void* room{};
    bool owned{};
};

inline constexpr std::int32_t UnknownRevealDifficulty = -1;
inline constexpr std::int32_t UnknownRevealLevelId = -1;
inline constexpr std::size_t RevealDifficultyCount = 3U;
inline constexpr std::size_t RevealPersistenceActCount = 5U;
inline constexpr std::size_t RevealPersistenceLevelCapacity = 256U;
inline constexpr std::size_t ProgressiveRevealVisCapacity = 8U;
inline constexpr std::size_t ProgressiveRevealLevelCapacity = 256U;

// Pure coordinator state shared with policy tests. A native automap
// observation may arrive while a readiness retry for the same level is
// already pending; that observation must upgrade the existing request rather
// than being discarded as a duplicate.
struct RevealReplayRequestState final {
    std::uint64_t sessionGeneration{};
    std::int32_t targetLevelId{UnknownRevealLevelId};
    std::uint32_t retriesRemaining{};
    bool playerReady{};
    bool automapObserved{};
    bool reconcilePending{};
    bool callbackQueued{};
};

[[nodiscard]] constexpr auto MergePendingRevealAutomapObservation(
        RevealReplayRequestState& pending,
        std::int32_t targetLevelId,
        bool automapObserved) noexcept -> bool {
    if (!pending.reconcilePending) return false;
    if (targetLevelId > 0 && pending.targetLevelId != targetLevelId) {
        return false;
    }
    pending.automapObserved = pending.automapObserved || automapObserved;
    return true;
}

[[nodiscard]] constexpr auto CanConfirmReplayedLevelReveal(
        bool automapObserved,
        RevealOutcome outcome) noexcept -> bool {
    return automapObserved && outcome == RevealOutcome::Complete;
}

struct RevealReplaySubmissionPolicy final {
    bool waitForAutomap{};
    bool submitWholeAct{};
    bool submitCurrentLevel{};
};

// Native reveal calls are UI-thread only and expensive. A real automap pass is
// the readiness witness. Whole-act work is scheduled once; after that work is
// accepted, a newly entered level still needs its own local fallback if it was
// not reached by the generated Vis graph.
[[nodiscard]] constexpr auto MakeRevealReplaySubmissionPolicy(
        bool automapObserved,
        bool revealWholeAct,
        bool actAccepted,
        bool levelAccepted) noexcept -> RevealReplaySubmissionPolicy {
    if (!automapObserved) {
        return {.waitForAutomap = true};
    }
    if (revealWholeAct) {
        return {
            .waitForAutomap = false,
            .submitWholeAct = !actAccepted,
            .submitCurrentLevel = actAccepted && !levelAccepted,
        };
    }
    return {
        .waitForAutomap = false,
        .submitWholeAct = false,
        .submitCurrentLevel = !levelAccepted,
    };
}

// Pure bounded breadth-first queue used by passive distant-name discovery.
// It stores stable LevelIds only. Native Level/Room pointers never survive an
// individual UI-thread callback.
class ProgressiveRevealGraphState final {
public:
    void Reset() noexcept {
        levels_.fill(UnknownRevealLevelId);
        levelCount_ = 0U;
        cursor_ = 0U;
    }

    [[nodiscard]] auto Begin(std::int32_t rootLevelId) noexcept -> bool {
        Reset();
        return Add(rootLevelId);
    }

    [[nodiscard]] auto Add(std::int32_t levelId) noexcept -> bool {
        if (levelId <= 0) return false;
        if (Contains(levelId)) return true;
        if (levelCount_ >= levels_.size()) return false;
        levels_[levelCount_++] = levelId;
        return true;
    }

    [[nodiscard]] auto Contains(std::int32_t levelId) const noexcept -> bool {
        for (std::size_t index = 0U; index < levelCount_; ++index) {
            if (levels_[index] == levelId) return true;
        }
        return false;
    }

    [[nodiscard]] auto HasCurrent() const noexcept -> bool {
        return cursor_ < levelCount_;
    }

    [[nodiscard]] auto Current() const noexcept -> std::int32_t {
        return HasCurrent() ? levels_[cursor_] : UnknownRevealLevelId;
    }

    [[nodiscard]] auto Advance() noexcept -> bool {
        if (!HasCurrent()) return false;
        ++cursor_;
        return true;
    }

    [[nodiscard]] auto LevelCount() const noexcept -> std::size_t {
        return levelCount_;
    }

    [[nodiscard]] auto LevelAt(std::size_t index) const noexcept
            -> std::int32_t {
        return index < levelCount_
            ? levels_[index]
            : UnknownRevealLevelId;
    }

    [[nodiscard]] auto Cursor() const noexcept -> std::size_t {
        return cursor_;
    }

private:
    std::array<std::int32_t, ProgressiveRevealLevelCapacity> levels_{};
    std::size_t levelCount_{};
    std::size_t cursor_{};
};

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

    // Reveal Act and Reveal All are process-lifetime intents. Whole-act work is
    // de-duplicated per session independently from single-level requests.
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
        if (!HasReplayIntentForLevel(difficulty, act, levelId)) return false;
        if (ShouldRevealWholeAct(difficulty, act)) {
            return !acceptedActs_[static_cast<std::size_t>(act)]
                || !Contains(
                    acceptedLevels_,
                    acceptedLevelCount_,
                    levelId);
        }
        return !Contains(acceptedLevels_, acceptedLevelCount_, levelId);
    }

    [[nodiscard]] auto ShouldRevealWholeAct(
            std::int32_t difficulty,
            std::int32_t act) const noexcept -> bool {
        if (!MatchesDifficulty(difficulty) || !IsValidAct(act)) return false;
        return revealAll_
            || rememberedActs_[static_cast<std::size_t>(act)];
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

    [[nodiscard]] auto IsActAccepted(
            std::int32_t difficulty,
            std::int32_t act) const noexcept -> bool {
        return MatchesDifficulty(difficulty) && IsValidAct(act)
            && acceptedActs_[static_cast<std::size_t>(act)];
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

    [[nodiscard]] auto MarkActAccepted(
            std::int32_t difficulty,
            std::int32_t act) noexcept -> bool {
        if (sessionGeneration_ == 0U
            || !MatchesDifficulty(difficulty) || !IsValidAct(act)) {
            return false;
        }
        acceptedActs_[static_cast<std::size_t>(act)] = true;
        return true;
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
        acceptedActs_.fill(false);
        acceptedLevelCount_ = 0U;
    }

    std::int32_t difficulty_{UnknownRevealDifficulty};
    bool revealAll_{};
    std::array<bool, RevealPersistenceActCount> rememberedActs_{};
    std::array<std::int32_t,
        RevealPersistenceLevelCapacity> rememberedLevels_{};
    std::size_t rememberedLevelCount_{};
    std::uint64_t sessionGeneration_{};
    std::array<bool, RevealPersistenceActCount> acceptedActs_{};
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
    // The original game seed stored by DRLG allocation, and the one-step LCG
    // value D2R uses as the base for each generated Level. Resolve succeeds
    // only when this pair matches the governed native seed contract.
    std::uint32_t mapSeed{};
    std::uint32_t drlgStartSeed{};
    void* activeRoom{};
    void* drlg{};
    void* level{};
};

inline constexpr std::uint64_t DrlgSeedMultiplier = UINT64_C(0x6AC690C5);
inline constexpr std::uint32_t DrlgSeedHighWord = 0x29AU;

[[nodiscard]] constexpr auto DeriveDrlgStartSeed(
        std::uint32_t mapSeed) noexcept -> std::uint32_t {
    return static_cast<std::uint32_t>(
        static_cast<std::uint64_t>(mapSeed) * DrlgSeedMultiplier
        + DrlgSeedHighWord);
}

using RevealLevelInitializedCallback = void(*)(
    std::uint8_t dataContext,
    void* level,
    void* userData) noexcept;

auto InitializeRevealEngine(
    const D2RL::PluginContext* context,
    bool diagnostics) noexcept -> bool;
void ShutdownRevealEngine() noexcept;
// Observes the level only for the duration of MapSense's existing InitLevel
// hook. The callback must not retain the borrowed native pointer.
void SetRevealLevelInitializedCallback(
    RevealLevelInitializedCallback callback,
    void* userData) noexcept;
// Session resets clear native DRLG references and diagnostics but intentionally
// preserve process-lifetime Reveal Level, Reveal Act, and Reveal All intents.
// The plugin coordinator invalidates them on a validated difficulty change.
void BeginRevealSession() noexcept;
void ResetRevealSession() noexcept;

auto RevealCurrentZone() noexcept -> RevealOutcome;
// Uses D2RCore's public revealmap command for the normal act-wide automap
// reveal. Distant-name discovery is a separate passive plugin operation.
auto RevealCurrentAct() noexcept -> RevealOutcome;
auto ArmRevealAll() noexcept -> RevealOutcome;
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
// Builds only DrlgRoom static tile/preset descriptors. It deliberately stops
// before D2R's ActiveRoom allocator and returns an owned lease that must be
// released immediately after the descriptors have been copied.
[[nodiscard]] auto PrepareStaticClientRoom(
    std::uint8_t dataContext,
    void* drlgRoom,
    StaticClientRoomLease& lease) noexcept -> bool;
[[nodiscard]] auto ReleaseStaticClientRoom(
    StaticClientRoomLease& lease) noexcept -> bool;
auto IsRevealEngineActive() noexcept -> bool;
auto IsRevealAllArmed() noexcept -> bool;
auto GetRevealCounters() noexcept -> RevealCounters;

} // namespace RuffnecKk::MapSense
