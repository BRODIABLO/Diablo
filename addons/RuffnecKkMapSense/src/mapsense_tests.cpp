#include "d3d12_imgui_host.hpp"
#include "mapsense_config.hpp"
#include "navigation_engine.hpp"
#include "navigation_level_catalog.hpp"
#include "navigation_policy.hpp"
#include "navigation_resolver.hpp"
#include "native_automap_marker.hpp"
#include "native_settings_layout.hpp"
#include "native_settings_panel.hpp"
#include "native_settings_policy.hpp"
#include "overlay_scene.hpp"
#include "reveal_engine.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <iostream>
#include <limits>
#include <set>
#include <sstream>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

namespace {

int Failures{};

void Check(bool condition, const char* expression, int line) {
    if (condition) return;
    std::cerr << "FAIL line " << line << ": " << expression << '\n';
    ++Failures;
}

#define CHECK(expression) Check(static_cast<bool>(expression), #expression, __LINE__)

template <class Callable>
auto Throws(Callable&& callable) -> bool {
    try {
        callable();
        return false;
    } catch (const std::exception&) {
        return true;
    }
}

auto ReadFile(const char* path) -> std::string {
    std::ifstream input(path, std::ios::binary);
    if (!input.is_open()) {
        throw std::runtime_error("cannot open shipped configuration");
    }
    std::ostringstream text;
    text << input.rdbuf();
    return text.str();
}

auto ProjectNavigationClientIdentity(
        void*,
        std::int32_t clientX,
        std::int32_t clientY,
        RuffnecKk::MapSense::NavigationNativePoint& output) noexcept -> bool {
    output = {.x = clientX, .y = clientY};
    return true;
}

void CheckNavigationProjectionDiagnosticCacheContract() {
    using RuffnecKk::MapSense::Detail::
        MaximumNavigationProjectionDiagnosticEntries;
    using RuffnecKk::MapSense::Detail::
        NavigationProjectionDiagnosticCache;

    NavigationProjectionDiagnosticCache cache;
    constexpr auto waypointId = UINT64_C(18214375364822279195);
    constexpr auto progressionId = UINT64_C(17344995562435206827);
    static_assert(waypointId % 16U == progressionId % 16U);

    CHECK(cache.ShouldLog(111, 0U, waypointId, 101U));
    CHECK(cache.ShouldLog(111, 1U, progressionId, 202U));
    CHECK(!cache.ShouldLog(111, 0U, waypointId, 101U));
    CHECK(!cache.ShouldLog(111, 1U, progressionId, 202U));
    CHECK(cache.ShouldLog(111, 0U, waypointId, 303U));

    CHECK(cache.ShouldLog(123, 0U, waypointId, 101U));
    CHECK(cache.ShouldLog(123, 1U, progressionId, 202U));

    cache.Reset();
    for (std::size_t index = 0U;
            index < MaximumNavigationProjectionDiagnosticEntries;
            ++index) {
        CHECK(cache.ShouldLog(
            7,
            static_cast<std::uint8_t>(index % 4U),
            static_cast<std::uint64_t>(index),
            static_cast<std::uint64_t>(index + 1U)));
    }
    CHECK(!cache.ShouldLog(7, 0U, UINT64_MAX, UINT64_MAX));
}

void CheckRevealPersistenceContract() {
    using namespace RuffnecKk::MapSense;

    RevealPersistenceState state;
    state.ResetProcess();
    state.BeginSession(101U);

    CHECK(state.Difficulty() == UnknownRevealDifficulty);
    CHECK(state.ObserveDifficulty(UnknownRevealDifficulty)
        == RevealDifficultyObservation::Invalid);
    CHECK(!state.HasAnyIntent());
    CHECK(!state.RememberLevel(0, 3));
    CHECK(!state.RememberAct(0, 0));
    CHECK(!state.SetRevealAll(0, true));

    CHECK(state.ObserveDifficulty(0)
        == RevealDifficultyObservation::Initialized);
    CHECK(state.RememberLevel(0, 3));
    CHECK(state.RememberLevel(0, 3));
    CHECK(state.RememberAct(0, 0));
    CHECK(state.RememberAct(0, 0));
    CHECK(state.SetRevealAll(0, true));
    CHECK(state.HasAnyIntent(0));
    CHECK(state.ShouldReplayLevel(0, 3));
    CHECK(state.HasReplayIntentForLevel(0, 0, 3));
    CHECK(state.HasReplayIntentForLevel(0, 1, 40));
    CHECK(state.ShouldReplayCurrentLevel(0, 0, 3));
    CHECK(state.ShouldReplayCurrentLevel(0, 1, 40));

    CHECK(state.MarkLevelAccepted(0, 3));
    CHECK(!state.ShouldReplayLevel(0, 3));
    CHECK(!state.ShouldReplayCurrentLevel(0, 0, 3));
    CHECK(state.ShouldReplayCurrentLevel(0, 0, 4));
    CHECK(state.IsLevelAccepted(0, 3));

    // Re-entering the same session cannot accidentally reopen accepted work.
    state.BeginSession(101U);
    CHECK(!state.ShouldReplayLevel(0, 3));
    CHECK(!state.ShouldReplayCurrentLevel(0, 0, 3));

    // A new game in the same difficulty replays stable ids on fresh geometry.
    state.BeginSession(102U);
    CHECK(state.ShouldReplayLevel(0, 3));
    CHECK(state.ShouldReplayCurrentLevel(0, 0, 3));
    CHECK(state.ShouldReplayCurrentLevel(0, 1, 40));

    // Unknown difficulty is fail-closed and cannot consume or reuse the state.
    CHECK(state.ObserveDifficulty(UnknownRevealDifficulty)
        == RevealDifficultyObservation::Invalid);
    CHECK(!state.ShouldReplayLevel(UnknownRevealDifficulty, 3));
    CHECK(!state.ShouldReplayCurrentLevel(
        UnknownRevealDifficulty, 0, 3));
    CHECK(state.ShouldReplayLevel(0, 3));

    // A real act transition cannot credit the accepted old-act reveal to the
    // newly entered act. The current DRLG's LevelId supplies the stable ActId.
    CHECK(RevealActForLevelId(39) == 0);
    CHECK(RevealActForLevelId(40) == 1);
    CHECK(RevealActForLevelId(74) == 1);
    CHECK(RevealActForLevelId(75) == 2);
    CHECK(RevealActForLevelId(102) == 2);
    CHECK(RevealActForLevelId(103) == 3);
    CHECK(RevealActForLevelId(108) == 3);
    CHECK(RevealActForLevelId(109) == 4);
    CHECK(RevealActForLevelId(137) == 4);
    CHECK(RevealActForLevelId(0) == -1);
    CHECK(RevealActForLevelId(138) == -1);
    CHECK(state.ShouldReplayCurrentLevel(
        0, RevealActForLevelId(39), 39));
    CHECK(state.MarkLevelAccepted(0, 39));
    CHECK(!state.ShouldReplayCurrentLevel(
        0, RevealActForLevelId(39), 39));
    CHECK(state.ShouldReplayCurrentLevel(
        0, RevealActForLevelId(40), 40));

    // Every real 0/1/2 transition invalidates all remembered intents.
    CHECK(state.ObserveDifficulty(1)
        == RevealDifficultyObservation::Changed);
    CHECK(state.Difficulty() == 1);
    CHECK(!state.HasAnyIntent());
    CHECK(!state.ShouldReplayLevel(1, 3));
    CHECK(!state.ShouldReplayCurrentLevel(1, 0, 3));

    CHECK(state.RememberLevel(1, 40));
    CHECK(state.RememberAct(1, 2));
    CHECK(state.SetRevealAll(1, true));
    CHECK(state.ObserveDifficulty(2)
        == RevealDifficultyObservation::Changed);
    CHECK(state.Difficulty() == 2);
    CHECK(!state.HasAnyIntent());

    CHECK(state.RememberLevel(2, 109));
    CHECK(state.RememberAct(2, 4));
    CHECK(state.SetRevealAll(2, true));
    CHECK(state.ObserveDifficulty(0)
        == RevealDifficultyObservation::Changed);
    CHECK(state.Difficulty() == 0);
    CHECK(!state.HasAnyIntent());
}

void CheckNavigationEngineContract() {
    using namespace RuffnecKk::MapSense;

    CHECK(ShouldRequestNavigationRefresh(
        NavigationAutomapObservationResult::LevelChanged,
        false));
    CHECK(!ShouldRequestNavigationRefresh(
        NavigationAutomapObservationResult::LevelChanged,
        true));
    CHECK(!ShouldRequestNavigationRefresh(
        NavigationAutomapObservationResult::Projected,
        false));

    InitializeNavigationEngine();
    ResetNavigationSession(41U);
    CHECK(!BindNavigationLevelForPublish(40U, 3));
    CHECK(BindNavigationLevelForPublish(41U, 3));
    CHECK(!BindNavigationLevelForPublish(41U, 4));

    const std::array destinations{
        NavigationSubtileDestination{
            .destinationId = 1001U,
            .subtileX = 20,
            .subtileY = 10,
            .kind = NavigationLineKind::Waypoint,
            .exactClientX = 321,
            .exactClientY = 123,
            .useExactClientCoordinates = true,
        },
        NavigationSubtileDestination{
            .destinationId = 1002U,
            .subtileX = 60,
            .subtileY = 20,
            .kind = NavigationLineKind::Progression,
        },
    };
    CHECK(!PublishNavigationDestinations(
        40U,
        3,
        destinations.data(),
        destinations.size()));
    CHECK(!PublishNavigationDestinations(
        41U,
        4,
        destinations.data(),
        destinations.size()));
    CHECK(PublishNavigationDestinations(
        41U,
        3,
        destinations.data(),
        destinations.size()));

    std::uint8_t borrowedContext{};
    auto pass = NavigationAutomapPass{
        .currentLevelId = 3,
        .playerClientX = 100,
        .playerClientY = 100,
        .nativeWidth = 800,
        .nativeHeight = 600,
        .clipLeft = 0,
        .clipTop = 0,
        .clipWidth = 800,
        .clipHeight = 600,
        .projectClient = ProjectNavigationClientIdentity,
        .borrowedAutomapContext = &borrowedContext,
    };
    CHECK(ObserveNavigationAutomapPass(pass)
        == NavigationAutomapObservationResult::Projected);

    std::vector<NavigationLineSnapshot> lines;
    CHECK(AcquireNavigationLineSnapshots(lines) == 2U);
    CHECK(lines[0].destinationId == 1001U);
    CHECK(lines[0].sessionGeneration == 41U);
    CHECK(lines[0].levelId == 3);
    CHECK(lines[0].startX == 100);
    CHECK(lines[0].startY == 100);
    CHECK(lines[0].endX == 321);
    CHECK(lines[0].endY == 123);
    CHECK(lines[0].kind == NavigationLineKind::Waypoint);
    CHECK(lines[1].destinationId == 1002U);
    CHECK(lines[1].endX == 599);
    CHECK(lines[1].endY == 599);
    CHECK(lines[1].kind == NavigationLineKind::Progression);
    CHECK(WantsNavigationLineFrame());

    const auto projectedStatus = GetNavigationEngineStatus();
    InvalidateNavigationProjection();
    CHECK(AcquireNavigationLineSnapshots(lines) == 0U);
    CHECK(!WantsNavigationLineFrame());
    const auto invalidatedStatus = GetNavigationEngineStatus();
    CHECK(invalidatedStatus.sessionGeneration
        == projectedStatus.sessionGeneration);
    CHECK(invalidatedStatus.destinationRevision
        == projectedStatus.destinationRevision);
    CHECK(invalidatedStatus.levelId == projectedStatus.levelId);
    CHECK(invalidatedStatus.destinationCount
        == projectedStatus.destinationCount);
    CHECK(invalidatedStatus.projectedLineCount == 0U);
    CHECK(ObserveNavigationAutomapPass(pass)
        == NavigationAutomapObservationResult::Projected);
    CHECK(AcquireNavigationLineSnapshots(lines) == 2U);

    const std::array captiveCages{
        NavigationSubtileDestination{
            .destinationId = 3001U,
            .subtileX = 10,
            .subtileY = 3,
            .kind = NavigationLineKind::Quest,
            .exactClientX = 112,
            .exactClientY = 104,
            .useExactClientCoordinates = true,
            .selection = NavigationDestinationSelection::NearestToPlayer,
        },
        NavigationSubtileDestination{
            .destinationId = 3002U,
            .subtileX = 20,
            .subtileY = 3,
            .kind = NavigationLineKind::Quest,
            .exactClientX = 272,
            .exactClientY = 184,
            .useExactClientCoordinates = true,
            .selection = NavigationDestinationSelection::NearestToPlayer,
        },
        NavigationSubtileDestination{
            .destinationId = 3003U,
            .subtileX = 30,
            .subtileY = 3,
            .kind = NavigationLineKind::Quest,
            .exactClientX = 432,
            .exactClientY = 264,
            .useExactClientCoordinates = true,
            .selection = NavigationDestinationSelection::NearestToPlayer,
        },
    };
    CHECK(PublishNavigationDestinations(
        41U,
        3,
        captiveCages.data(),
        captiveCages.size()));
    pass.playerClientX = 100;
    pass.playerClientY = 100;
    CHECK(ObserveNavigationAutomapPass(pass)
        == NavigationAutomapObservationResult::Projected);
    CHECK(AcquireNavigationLineSnapshots(lines) == 1U);
    CHECK(lines[0].destinationId == 3001U);
    CHECK(lines[0].kind == NavigationLineKind::Quest);
    CHECK(lines[0].endX == 112);
    CHECK(lines[0].endY == 104);

    pass.playerClientX = 400;
    pass.playerClientY = 240;
    CHECK(ObserveNavigationAutomapPass(pass)
        == NavigationAutomapObservationResult::Projected);
    CHECK(AcquireNavigationLineSnapshots(lines) == 1U);
    CHECK(lines[0].destinationId == 3003U);
    CHECK(lines[0].endX == 432);
    CHECK(lines[0].endY == 264);

    CHECK(PublishNavigationDestinations(
        41U,
        3,
        destinations.data(),
        destinations.size()));
    pass.playerClientX = 100;
    pass.playerClientY = 100;
    CHECK(ObserveNavigationAutomapPass(pass)
        == NavigationAutomapObservationResult::Projected);
    CHECK(AcquireNavigationLineSnapshots(lines) == 2U);

    pass.currentLevelId = UnknownNavigationLevelId;
    CHECK(ObserveNavigationAutomapPass(pass)
        == NavigationAutomapObservationResult::Ignored);
    CHECK(AcquireNavigationLineSnapshots(lines) == 0U);
    CHECK(!WantsNavigationLineFrame());
    pass.currentLevelId = 3;
    CHECK(ObserveNavigationAutomapPass(pass)
        == NavigationAutomapObservationResult::Projected);
    CHECK(AcquireNavigationLineSnapshots(lines) == 2U);

    pass.currentLevelId = 4;
    pass.inTown = true;
    CHECK(ObserveNavigationAutomapPass(pass)
        == NavigationAutomapObservationResult::LevelChanged);
    CHECK(AcquireNavigationLineSnapshots(lines) == 0U);
    CHECK(!WantsNavigationLineFrame());
    const auto changedStatus = GetNavigationEngineStatus();
    CHECK(changedStatus.levelId == 4);
    CHECK(changedStatus.destinationCount == 0U);
    CHECK(changedStatus.observedLevelChanges == 1U);
    CHECK(!PublishNavigationDestinations(
        41U,
        3,
        destinations.data(),
        destinations.size()));
    CHECK(BindNavigationLevelForPublish(41U, 4));
    CHECK(PublishNavigationDestinations(
        41U,
        4,
        destinations.data(),
        destinations.size()));
    CHECK(GetNavigationEngineStatus().destinationCount == 2U);
    CHECK(ObserveNavigationAutomapPass(pass)
        == NavigationAutomapObservationResult::Ignored);
    CHECK(AcquireNavigationLineSnapshots(lines) == 0U);
    CHECK(!WantsNavigationLineFrame());
    const auto townStatus = GetNavigationEngineStatus();
    CHECK(townStatus.levelId == 4);
    CHECK(townStatus.destinationCount == 0U);
    CHECK(townStatus.projectedLineCount == 0U);
    ShutdownNavigationEngine();
}

void CheckNavigationPolicyContract() {
    using namespace RuffnecKk::MapSense;

    CHECK(MainProgressionTargetFor(3).value_or(-1) == 4);
    CHECK(MainProgressionTargetFor(6).value_or(-1) == 7);
    CHECK(MainProgressionTargetFor(7).value_or(-1) == 26);
    CHECK(MainProgressionTargetFor(28).value_or(-1) == 29);
    CHECK(MainProgressionTargetFor(29).value_or(-1) == 30);
    CHECK(MainProgressionTargetFor(40).value_or(-1) == 41);
    CHECK(MainProgressionTargetFor(76).value_or(-1) == 78);
    CHECK(MainProgressionTargetFor(107).value_or(-1) == 108);
    CHECK(MainProgressionTargetFor(130).value_or(-1) == 131);
    CHECK(!MainProgressionTargetFor(37).has_value());
    CHECK(MainProgressionTargetFor(54).value_or(-1) == 74);
    CHECK(MainProgressionTargetFor(66).value_or(-1) == 73);
    CHECK(MainProgressionTargetFor(74).value_or(-1) == 46);
    CHECK(MainProgressionTargetFor(102).value_or(-1) == 103);
    CHECK(MainProgressionTargetFor(108).value_or(-1) == 109);
    CHECK(MainProgressionTargetFor(131).value_or(-1) == 132);

    std::array<std::int32_t, 4U> preparationTargets{};
    const std::array tamoeCustomTargets{12, 12, 7, -1};
    const auto tamoePreparationCount = BuildNavigationPreparationTargets(
        7,
        tamoeCustomTargets,
        preparationTargets);
    CHECK(tamoePreparationCount == 2U);
    CHECK(preparationTargets[0] == 26);
    CHECK(preparationTargets[1] == 12);
    CHECK(BuildNavigationPreparationTargets(
        28,
        std::span<const std::int32_t>{},
        preparationTargets) == 1U);
    CHECK(preparationTargets[0] == 29);
    CHECK(BuildNavigationPreparationTargets(
        29,
        std::span<const std::int32_t>{},
        preparationTargets) == 1U);
    CHECK(preparationTargets[0] == 30);
    CHECK(BuildNavigationPreparationTargets(
        74,
        std::span<const std::int32_t>{},
        preparationTargets) == 0U);

    struct ProgressionRegression final {
        std::int32_t from{};
        std::array<std::int32_t, MaximumMainProgressionTargets> targets{};
        std::size_t targetCount{};
        bool inTown{};
        std::int32_t dynamicObjectClassId{-1};
    };
    constexpr std::array progressionMatrix{
        ProgressionRegression{1, {2, -1}, 1U, true},
        ProgressionRegression{2, {3, -1}, 1U},
        ProgressionRegression{3, {4, -1}, 1U},
        ProgressionRegression{4, {10, -1}, 1U},
        ProgressionRegression{5, {6, -1}, 1U},
        ProgressionRegression{6, {7, -1}, 1U},
        ProgressionRegression{7, {26, -1}, 1U},
        ProgressionRegression{9, {13, -1}, 1U},
        ProgressionRegression{10, {5, -1}, 1U},
        ProgressionRegression{11, {15, -1}, 1U},
        ProgressionRegression{12, {16, -1}, 1U},
        ProgressionRegression{26, {27, -1}, 1U},
        ProgressionRegression{27, {28, -1}, 1U},
        ProgressionRegression{28, {29, -1}, 1U},
        ProgressionRegression{29, {30, -1}, 1U},
        ProgressionRegression{30, {31, -1}, 1U},
        ProgressionRegression{31, {32, -1}, 1U},
        ProgressionRegression{32, {33, -1}, 1U},
        ProgressionRegression{33, {34, -1}, 1U},
        ProgressionRegression{34, {35, -1}, 1U},
        ProgressionRegression{35, {36, -1}, 1U},
        ProgressionRegression{36, {37, -1}, 1U},

        ProgressionRegression{40, {41, -1}, 1U, true},
        ProgressionRegression{41, {42, -1}, 1U},
        ProgressionRegression{42, {43, -1}, 1U},
        ProgressionRegression{43, {44, -1}, 1U},
        ProgressionRegression{44, {45, -1}, 1U},
        ProgressionRegression{45, {58, -1}, 1U},
        ProgressionRegression{50, {51, -1}, 1U},
        ProgressionRegression{51, {52, -1}, 1U},
        ProgressionRegression{52, {53, -1}, 1U},
        ProgressionRegression{53, {54, -1}, 1U},
        ProgressionRegression{54, {74, -1}, 1U, false, 298},
        ProgressionRegression{55, {59, -1}, 1U},
        ProgressionRegression{58, {61, -1}, 1U},
        ProgressionRegression{66, {73, -1}, 1U, false, 100},
        ProgressionRegression{67, {73, -1}, 1U, false, 100},
        ProgressionRegression{68, {73, -1}, 1U, false, 100},
        ProgressionRegression{69, {73, -1}, 1U, false, 100},
        ProgressionRegression{70, {73, -1}, 1U, false, 100},
        ProgressionRegression{71, {73, -1}, 1U, false, 100},
        ProgressionRegression{72, {73, -1}, 1U, false, 100},
        ProgressionRegression{74, {46, -1}, 1U, false, 60},

        ProgressionRegression{75, {76, -1}, 1U, true},
        ProgressionRegression{76, {78, 77}, 2U},
        ProgressionRegression{77, {78, -1}, 1U},
        ProgressionRegression{78, {79, -1}, 1U},
        ProgressionRegression{79, {80, -1}, 1U},
        ProgressionRegression{80, {81, -1}, 1U},
        ProgressionRegression{81, {82, -1}, 1U},
        ProgressionRegression{82, {83, -1}, 1U},
        ProgressionRegression{83, {100, -1}, 1U},
        ProgressionRegression{86, {87, -1}, 1U},
        ProgressionRegression{87, {90, -1}, 1U},
        ProgressionRegression{100, {101, -1}, 1U},
        ProgressionRegression{101, {102, -1}, 1U},
        ProgressionRegression{102, {103, -1}, 1U, false, 342},

        ProgressionRegression{103, {104, -1}, 1U, true},
        ProgressionRegression{104, {105, -1}, 1U},
        ProgressionRegression{105, {106, -1}, 1U},
        ProgressionRegression{106, {107, -1}, 1U},
        ProgressionRegression{107, {108, -1}, 1U},
        ProgressionRegression{108, {109, -1}, 1U, false, 566},

        ProgressionRegression{109, {110, -1}, 1U, true},
        ProgressionRegression{110, {111, -1}, 1U},
        ProgressionRegression{111, {112, -1}, 1U},
        ProgressionRegression{112, {113, -1}, 1U},
        ProgressionRegression{113, {115, -1}, 1U},
        ProgressionRegression{115, {117, -1}, 1U},
        ProgressionRegression{117, {118, -1}, 1U},
        ProgressionRegression{118, {120, -1}, 1U},
        ProgressionRegression{120, {128, -1}, 1U},
        ProgressionRegression{128, {129, -1}, 1U},
        ProgressionRegression{129, {130, -1}, 1U},
        ProgressionRegression{130, {131, -1}, 1U},
        ProgressionRegression{131, {132, -1}, 1U, false, 563},
    };
    std::array<NavigationSubtileDestination, 4U> interiorDestinations{};
    for (const auto& regression : progressionMatrix) {
        CHECK(MainProgressionTargetFor(regression.from).value_or(-1)
            == regression.targets[0]);
        std::array<std::int32_t, MaximumMainProgressionTargets>
            actualTargets{};
        CHECK(MainProgressionTargetsFor(regression.from, actualTargets)
            == regression.targetCount);
        for (std::size_t index = 0U;
                index < regression.targetCount;
                ++index) {
            CHECK(actualTargets[index] == regression.targets[index]);
        }
        std::array<std::int32_t, MaximumStaticQuestRouteTargets>
            questRouteTargets{};
        const auto questRouteCount = StaticQuestRouteTargetsFor(
            regression.from,
            questRouteTargets);
        const auto preparationCount = BuildNavigationPreparationTargets(
            regression.from,
            std::span<const std::int32_t>{},
            preparationTargets);
        const auto isDynamic = regression.dynamicObjectClassId >= 0;
        const auto preparedProgressionCount = isDynamic
            ? 0U : regression.targetCount;
        CHECK(preparationCount
            == preparedProgressionCount + questRouteCount);
        CHECK(HasDynamicMainProgressionTargetFor(regression.from)
            == isDynamic);
        if (isDynamic) {
            CHECK(DynamicMainProgressionTargetFor(
                regression.from,
                regression.dynamicObjectClassId).value_or(-1)
                == regression.targets[0]);
            CHECK(!DynamicMainProgressionTargetFor(
                regression.from,
                regression.dynamicObjectClassId + 1).has_value());
        }
        std::array<
            NavigationExitCandidate,
            MaximumMainProgressionTargets + MaximumStaticQuestRouteTargets>
            exactExits{};
        for (std::size_t index = 0U;
                index < regression.targetCount;
                ++index) {
            if (!isDynamic) {
                CHECK(preparationTargets[index]
                    == regression.targets[index]);
            }
            exactExits[index] = NavigationExitCandidate{
                .destinationId = static_cast<std::uint64_t>(
                    regression.targets[index]),
                .targetLevelId = regression.targets[index],
                .subtileX = 1'000 + regression.from
                    + static_cast<std::int32_t>(index),
                .subtileY = 2'000 + regression.targets[index],
                .exactClientX = isDynamic ? -3'200 : 0,
                .exactClientY = isDynamic ? 4'800 : 0,
                .useExactClientCoordinates = isDynamic,
            };
        }
        for (std::size_t index = 0U;
                index < questRouteCount;
                ++index) {
            CHECK(preparationTargets[preparedProgressionCount + index]
                == questRouteTargets[index]);
            exactExits[regression.targetCount + index] =
                NavigationExitCandidate{
                    .destinationId = static_cast<std::uint64_t>(
                        questRouteTargets[index]),
                    .targetLevelId = questRouteTargets[index],
                    .subtileX = 3'000 + regression.from
                        + static_cast<std::int32_t>(index),
                    .subtileY = 4'000 + questRouteTargets[index],
                };
        }
        const auto exactExitSpan = std::span(
            exactExits.data(),
            regression.targetCount + questRouteCount);
        CHECK(SelectMainProgressionTargetFor(
            regression.from,
            exactExitSpan).value_or(-1) == regression.targets[0]);
        CHECK(EvaluateNavigationResolutionCompleteness(
            regression.from,
            exactExitSpan) == NavigationResolutionCompleteness::Complete);
        CHECK(EvaluateNavigationResolutionCompleteness(
            regression.from,
            std::span<const NavigationExitCandidate>{})
            == (isDynamic && questRouteCount == 0U
                ? NavigationResolutionCompleteness::Complete
                : NavigationResolutionCompleteness::PartialRetryable));
        const auto progressionDestinationCount = BuildNavigationDestinations(
            NavigationPolicyInput{
                .currentLevelId = regression.from,
                .inTown = regression.inTown,
                .exits = exactExitSpan,
            },
            interiorDestinations);
        if (regression.inTown) {
            CHECK(progressionDestinationCount == 0U);
            continue;
        }
        CHECK(progressionDestinationCount == 1U + questRouteCount);
        CHECK(interiorDestinations[0].kind
            == NavigationLineKind::Progression);
        CHECK(interiorDestinations[0].subtileX
            == 1'000 + regression.from);
        CHECK(interiorDestinations[0].subtileY
            == 2'000 + regression.targets[0]);
        CHECK(interiorDestinations[0].useExactClientCoordinates
            == isDynamic);
        if (isDynamic) {
            CHECK(interiorDestinations[0].exactClientX == -3'200);
            CHECK(interiorDestinations[0].exactClientY == 4'800);
        }
        for (std::size_t index = 0U;
                index < questRouteCount;
                ++index) {
            CHECK(interiorDestinations[1U + index].kind
                == NavigationLineKind::Quest);
            CHECK(interiorDestinations[1U + index].destinationId
                == static_cast<std::uint64_t>(questRouteTargets[index]));
        }
    }

    struct QuestRouteRegression final {
        std::int32_t from{};
        std::array<std::int32_t, MaximumStaticQuestRouteTargets> targets{};
        std::size_t targetCount{};
    };
    constexpr std::array questRouteMatrix{
        QuestRouteRegression{2, {8, -1}, 1U},
        QuestRouteRegression{3, {17, -1}, 1U},
        QuestRouteRegression{6, {20, -1}, 1U},
        QuestRouteRegression{20, {21, -1}, 1U},
        QuestRouteRegression{21, {22, -1}, 1U},
        QuestRouteRegression{22, {23, -1}, 1U},
        QuestRouteRegression{23, {24, -1}, 1U},
        QuestRouteRegression{24, {25, -1}, 1U},

        QuestRouteRegression{47, {48, -1}, 1U},
        QuestRouteRegression{48, {49, -1}, 1U},
        QuestRouteRegression{42, {56, -1}, 1U},
        QuestRouteRegression{56, {57, -1}, 1U},
        QuestRouteRegression{57, {60, -1}, 1U},
        QuestRouteRegression{43, {62, -1}, 1U},
        QuestRouteRegression{62, {63, -1}, 1U},
        QuestRouteRegression{63, {64, -1}, 1U},

        QuestRouteRegression{76, {85, -1}, 1U},
        QuestRouteRegression{78, {88, -1}, 1U},
        QuestRouteRegression{88, {89, -1}, 1U},
        QuestRouteRegression{89, {91, -1}, 1U},
        QuestRouteRegression{80, {92, 94}, 2U},
        QuestRouteRegression{81, {92, -1}, 1U},
        QuestRouteRegression{92, {93, -1}, 1U},

        QuestRouteRegression{113, {114, -1}, 1U},
        QuestRouteRegression{121, {122, -1}, 1U},
        QuestRouteRegression{122, {123, -1}, 1U},
        QuestRouteRegression{123, {124, -1}, 1U},
    };
    for (const auto& regression : questRouteMatrix) {
        std::array<std::int32_t, MaximumStaticQuestRouteTargets>
            actualQuestTargets{};
        const auto questTargetCount = StaticQuestRouteTargetsFor(
            regression.from,
            actualQuestTargets);
        CHECK(questTargetCount == regression.targetCount);
        for (std::size_t index = 0U;
                index < regression.targetCount;
                ++index) {
            CHECK(actualQuestTargets[index] == regression.targets[index]);
            CHECK(IsStaticQuestRouteTarget(
                regression.from,
                regression.targets[index]));
        }

        std::array<std::int32_t, MaximumMainProgressionTargets>
            mainTargets{};
        const auto mainTargetCount = MainProgressionTargetsFor(
            regression.from,
            mainTargets);
        std::array<
            NavigationExitCandidate,
            MaximumMainProgressionTargets + MaximumStaticQuestRouteTargets>
            routeExits{};
        for (std::size_t index = 0U; index < mainTargetCount; ++index) {
            routeExits[index] = NavigationExitCandidate{
                .destinationId = static_cast<std::uint64_t>(
                    10'000 + mainTargets[index]),
                .targetLevelId = mainTargets[index],
                .subtileX = 100 + static_cast<std::int32_t>(index),
                .subtileY = 200 + mainTargets[index],
            };
        }
        for (std::size_t index = 0U; index < questTargetCount; ++index) {
            routeExits[mainTargetCount + index] = NavigationExitCandidate{
                .destinationId = static_cast<std::uint64_t>(
                    20'000 + actualQuestTargets[index]),
                .targetLevelId = actualQuestTargets[index],
                .subtileX = 300 + static_cast<std::int32_t>(index),
                .subtileY = 400 + actualQuestTargets[index],
            };
        }
        const auto routeExitCount = mainTargetCount + questTargetCount;
        const auto routeExitSpan = std::span(
            routeExits.data(),
            routeExitCount);
        CHECK(EvaluateNavigationResolutionCompleteness(
            regression.from,
            routeExitSpan) == NavigationResolutionCompleteness::Complete);
        if (mainTargetCount > 0U) {
            CHECK(EvaluateNavigationResolutionCompleteness(
                regression.from,
                std::span(
                    routeExits.data() + mainTargetCount,
                    questTargetCount))
                == NavigationResolutionCompleteness::PartialRetryable);
        }
        CHECK(BuildNavigationDestinations(
            NavigationPolicyInput{
                .currentLevelId = regression.from,
                .exits = routeExitSpan,
            },
            interiorDestinations)
            == (mainTargetCount > 0U ? 1U : 0U) + questTargetCount);

        const auto firstQuestDestination = mainTargetCount > 0U ? 1U : 0U;
        for (std::size_t index = 0U; index < questTargetCount; ++index) {
            const auto& destination =
                interiorDestinations[firstQuestDestination + index];
            CHECK(destination.kind == NavigationLineKind::Quest);
            CHECK(destination.destinationId == static_cast<std::uint64_t>(
                20'000 + actualQuestTargets[index]));
        }
        for (std::size_t omittedQuest = 0U;
                omittedQuest < questTargetCount;
                ++omittedQuest) {
            std::array<
                NavigationExitCandidate,
                MaximumMainProgressionTargets
                    + MaximumStaticQuestRouteTargets>
                incompleteRouteExits{};
            std::size_t incompleteRouteExitCount{};
            for (std::size_t index = 0U; index < routeExitCount; ++index) {
                if (index == mainTargetCount + omittedQuest) continue;
                incompleteRouteExits[incompleteRouteExitCount++] =
                    routeExits[index];
            }
            CHECK(EvaluateNavigationResolutionCompleteness(
                regression.from,
                std::span(
                    incompleteRouteExits.data(),
                    incompleteRouteExitCount))
                == NavigationResolutionCompleteness::PartialRetryable);
        }

        const auto preparationCount = BuildNavigationPreparationTargets(
            regression.from,
            std::span<const std::int32_t>{},
            preparationTargets);
        CHECK(preparationCount == mainTargetCount + questTargetCount);
        for (std::size_t index = 0U; index < mainTargetCount; ++index) {
            CHECK(preparationTargets[index] == mainTargets[index]);
        }
        for (std::size_t index = 0U; index < questTargetCount; ++index) {
            CHECK(preparationTargets[mainTargetCount + index]
                == actualQuestTargets[index]);
        }
    }

    struct QuestRouteExclusion final {
        std::int32_t from{};
        std::int32_t to{};
    };
    constexpr std::array questRouteExclusions{
        // Farming-only side areas.
        QuestRouteExclusion{3, 9},
        QuestRouteExclusion{6, 11},
        QuestRouteExclusion{7, 12},
        QuestRouteExclusion{17, 18},
        QuestRouteExclusion{17, 19},
        QuestRouteExclusion{41, 55},
        QuestRouteExclusion{44, 65},
        QuestRouteExclusion{76, 84},
        QuestRouteExclusion{78, 86},
        QuestRouteExclusion{80, 95},
        QuestRouteExclusion{81, 96},
        QuestRouteExclusion{81, 97},
        QuestRouteExclusion{82, 98},
        QuestRouteExclusion{82, 99},
        QuestRouteExclusion{115, 116},
        QuestRouteExclusion{118, 119},
        QuestRouteExclusion{111, 125},
        QuestRouteExclusion{112, 126},
        QuestRouteExclusion{117, 127},

        // Quest paths already owned by main progression stay green.
        QuestRouteExclusion{3, 4},
        QuestRouteExclusion{4, 10},
        QuestRouteExclusion{44, 45},
        QuestRouteExclusion{45, 58},
        QuestRouteExclusion{58, 61},
        QuestRouteExclusion{54, 74},
        QuestRouteExclusion{74, 46},
        QuestRouteExclusion{83, 100},
        QuestRouteExclusion{107, 108},
        QuestRouteExclusion{118, 120},
        QuestRouteExclusion{120, 128},

        // Secret, Pandemonium and separately quest-selected destinations.
        QuestRouteExclusion{1, 39},
        QuestRouteExclusion{46, 66},
        QuestRouteExclusion{46, 67},
        QuestRouteExclusion{46, 68},
        QuestRouteExclusion{46, 69},
        QuestRouteExclusion{46, 70},
        QuestRouteExclusion{46, 71},
        QuestRouteExclusion{46, 72},
        QuestRouteExclusion{109, 133},
        QuestRouteExclusion{109, 134},
        QuestRouteExclusion{109, 135},
        QuestRouteExclusion{109, 136},
    };
    for (const auto& exclusion : questRouteExclusions) {
        CHECK(!IsStaticQuestRouteTarget(exclusion.from, exclusion.to));
        std::array<std::int32_t, MaximumStaticQuestRouteTargets>
            actualQuestTargets{};
        const auto targetCount = StaticQuestRouteTargetsFor(
            exclusion.from,
            actualQuestTargets);
        CHECK(std::find(
            actualQuestTargets.begin(),
            actualQuestTargets.begin() + targetCount,
            exclusion.to) == actualQuestTargets.begin() + targetCount);
    }
    CHECK(!IsStaticQuestRouteTarget(-1, 8));
    CHECK(!IsStaticQuestRouteTarget(2, -1));
    CHECK(StaticQuestRouteTargetsFor(
        -1,
        preparationTargets) == 0U);

    const std::array tamoeProgressionAndPitExits{
        NavigationExitCandidate{26U, 26, 1'700, 2'600},
        NavigationExitCandidate{12U, 12, 1'701, 2'601},
    };
    CHECK(BuildNavigationDestinations(
        NavigationPolicyInput{
            .currentLevelId = 7,
            .exits = tamoeProgressionAndPitExits,
        },
        interiorDestinations) == 1U);
    CHECK(interiorDestinations[0].destinationId == 26U);
    CHECK(interiorDestinations[0].kind == NavigationLineKind::Progression);

    const std::array spiderMarshOnlyExit{
        NavigationExitCandidate{77U, 77, 1'760, 2'770},
        NavigationExitCandidate{85U, 85, 1'785, 2'850},
    };
    CHECK(SelectMainProgressionTargetFor(
        76,
        spiderMarshOnlyExit).value_or(-1) == 77);
    CHECK(BuildNavigationDestinations(
        NavigationPolicyInput{
            .currentLevelId = 76,
            .exits = spiderMarshOnlyExit,
        },
        interiorDestinations) == 2U);
    CHECK(interiorDestinations[0].destinationId == 77U);
    CHECK(interiorDestinations[0].kind == NavigationLineKind::Progression);
    CHECK(interiorDestinations[1].destinationId == 85U);
    CHECK(interiorDestinations[1].kind == NavigationLineKind::Quest);
    CHECK(EvaluateNavigationResolutionCompleteness(
        76,
        spiderMarshOnlyExit) == NavigationResolutionCompleteness::Complete);

    const std::array spiderBothExits{
        NavigationExitCandidate{77U, 77, 1'760, 2'770},
        NavigationExitCandidate{78U, 78, 1'761, 2'780},
        NavigationExitCandidate{85U, 85, 1'785, 2'850},
    };
    CHECK(SelectMainProgressionTargetFor(
        76,
        spiderBothExits).value_or(-1) == 78);
    CHECK(BuildNavigationDestinations(
        NavigationPolicyInput{
            .currentLevelId = 76,
            .exits = spiderBothExits,
        },
        interiorDestinations) == 2U);
    CHECK(interiorDestinations[0].destinationId == 78U);
    CHECK(interiorDestinations[0].kind == NavigationLineKind::Progression);
    CHECK(interiorDestinations[1].destinationId == 85U);
    CHECK(interiorDestinations[1].kind == NavigationLineKind::Quest);
    CHECK(EvaluateNavigationResolutionCompleteness(
        76,
        spiderBothExits) == NavigationResolutionCompleteness::Complete);
    CHECK(EvaluateNavigationResolutionCompleteness(
        76,
        std::span<const NavigationExitCandidate>{})
        == NavigationResolutionCompleteness::PartialRetryable);
    CHECK(!HasDynamicMainProgressionTargetFor(76));
    CHECK(!DynamicMainProgressionTargetFor(74, 59).has_value());
    CHECK(!DynamicMainProgressionTargetFor(3, -1).has_value());
    const auto arcaneSummoner = PresetMainProgressionTargetFor(74, 1U, 250);
    CHECK(arcaneSummoner.has_value());
    CHECK(arcaneSummoner->targetLevelId == 46);
    CHECK(arcaneSummoner->kind == NavigationPresetProgressionKind::Boss);
    const auto arcaneTome = PresetMainProgressionTargetFor(74, 2U, 357);
    CHECK(arcaneTome.has_value());
    CHECK(arcaneTome->targetLevelId == 46);
    CHECK(arcaneTome->kind
        == NavigationPresetProgressionKind::QuestObject);
    CHECK(!PresetMainProgressionTargetFor(74, 2U, 60).has_value());
    CHECK(!PresetMainProgressionTargetFor(73, 1U, 250).has_value());

    struct QuestPresetRegression final {
        std::int32_t levelId{};
        std::uint32_t presetType{};
        std::int32_t presetClassId{};
        NavigationDestinationSelection selection{
            NavigationDestinationSelection::All};
    };
    constexpr std::array questPresetMatrix{
        QuestPresetRegression{4, 2U, 21},
        QuestPresetRegression{5, 2U, 30},
        QuestPresetRegression{38, 2U, 26},
        QuestPresetRegression{28, 2U, 108},
        QuestPresetRegression{60, 2U, 354},
        QuestPresetRegression{61, 2U, 149},
        QuestPresetRegression{64, 2U, 356},
        QuestPresetRegression{66, 2U, 152},
        QuestPresetRegression{67, 2U, 152},
        QuestPresetRegression{68, 2U, 152},
        QuestPresetRegression{69, 2U, 152},
        QuestPresetRegression{70, 2U, 152},
        QuestPresetRegression{71, 2U, 152},
        QuestPresetRegression{72, 2U, 152},
        QuestPresetRegression{85, 2U, 407},
        QuestPresetRegression{91, 2U, 406},
        QuestPresetRegression{93, 2U, 405},
        QuestPresetRegression{94, 2U, 193},
        QuestPresetRegression{83, 2U, 404},
        QuestPresetRegression{107, 2U, 376},
        QuestPresetRegression{
            111,
            2U,
            473,
            NavigationDestinationSelection::NearestToPlayer},
        QuestPresetRegression{114, 2U, 558},
        QuestPresetRegression{120, 2U, 546},
    };
    for (const auto& regression : questPresetMatrix) {
        const auto target = StaticQuestPresetTargetFor(
            regression.levelId,
            regression.presetType,
            regression.presetClassId);
        CHECK(target.has_value());
        CHECK(target->selection == regression.selection);
        CHECK(!StaticQuestPresetTargetFor(
            regression.levelId,
            regression.presetType == 2U ? 1U : 2U,
            regression.presetClassId).has_value());
    }
    CHECK(!StaticQuestPresetTargetFor(4, 2U, 30).has_value());
    CHECK(!StaticQuestPresetTargetFor(78, 2U, 407).has_value());
    CHECK(!StaticQuestPresetTargetFor(111, 2U, -1).has_value());

    std::int32_t converted{};
    CHECK(CheckedNavigationSubtileCoordinate(1085, 0, converted));
    CHECK(converted == 5425);
    CHECK(CheckedNavigationSubtileCoordinate(100, 3, converted));
    CHECK(converted == 503);
    CHECK(!CheckedNavigationSubtileCoordinate(-1, 0, converted));
    CHECK(!CheckedNavigationSubtileCoordinate(
        (std::numeric_limits<std::int32_t>::max)(),
        0,
        converted));

    NavigationNativePoint client{};
    CHECK(ConvertNavigationSubtileToClientCoordinates(20, 10, client));
    CHECK(client.x == 160);
    CHECK(client.y == 240);
    CHECK(ConvertNavigationSubtileToClientCoordinates(100, 200, client));
    CHECK(client.x == -1'600);
    CHECK(client.y == 2'400);
    CHECK(!ConvertNavigationSubtileToClientCoordinates(-1, 0, client));
    CHECK(!ConvertNavigationSubtileToClientCoordinates(
        (std::numeric_limits<std::int32_t>::max)(),
        (std::numeric_limits<std::int32_t>::max)(),
        client));
    NavigationNativePoint subtile{};
    CHECK(ConvertNavigationClientToSubtileCoordinates(160, 240, subtile));
    CHECK(subtile.x == 20);
    CHECK(subtile.y == 10);
    CHECK(ConvertNavigationClientToSubtileCoordinates(
        -1'600,
        2'400,
        subtile));
    CHECK(subtile.x == 100);
    CHECK(subtile.y == 200);
    CHECK(!ConvertNavigationClientToSubtileCoordinates(161, 240, subtile));

    const std::array exits{
        NavigationExitCandidate{100U, 4, 400, 500},
        NavigationExitCandidate{101U, 9, 600, 700},
    };
    const NavigationPointCandidate waypoint{
        .destinationId = 200U,
        .subtileX = 300,
        .subtileY = 350,
        .exactClientX = -800,
        .exactClientY = 5'200,
        .useExactClientCoordinates = true,
    };
    const std::array quests{
        NavigationPointCandidate{300U, 900, 950},
    };
    const std::array customTargetIds{9};
    std::array<NavigationSubtileDestination, 8U> destinations{};
    const auto count = BuildNavigationDestinations(
        NavigationPolicyInput{
            .currentLevelId = 3,
            .exits = exits,
            .waypoint = &waypoint,
            .questTargets = quests,
            .customTargetLevelIds = customTargetIds,
        },
        destinations);
    CHECK(count == 4U);
    CHECK(destinations[0].kind == NavigationLineKind::Waypoint);
    CHECK(destinations[0].useExactClientCoordinates);
    CHECK(destinations[0].exactClientX == -800);
    CHECK(destinations[0].exactClientY == 5'200);
    CHECK(destinations[1].kind == NavigationLineKind::Progression);
    CHECK(destinations[1].destinationId == 100U);
    CHECK(destinations[2].kind == NavigationLineKind::CustomLevel);
    CHECK(destinations[2].destinationId == 101U);
    CHECK(destinations[3].kind == NavigationLineKind::Quest);
    CHECK(BuildNavigationDestinations(
        NavigationPolicyInput{
            .currentLevelId = 1,
            .inTown = true,
            .exits = exits,
            .waypoint = &waypoint,
            .questTargets = quests,
            .customTargetLevelIds = customTargetIds,
        },
        destinations) == 0U);
    CHECK(EvaluateNavigationResolutionCompleteness(
        3,
        exits) == NavigationResolutionCompleteness::PartialRetryable);
    const std::array coldPlainsCompleteExits{
        exits[0],
        exits[1],
        NavigationExitCandidate{102U, 17, 800, 900},
    };
    CHECK(EvaluateNavigationResolutionCompleteness(
        3,
        coldPlainsCompleteExits)
        == NavigationResolutionCompleteness::Complete);
    CHECK(EvaluateNavigationResolutionCompleteness(
        3,
        std::span<const NavigationExitCandidate>{})
        == NavigationResolutionCompleteness::PartialRetryable);

    const std::array overlapTargetIds{4};
    const auto overlapCount = BuildNavigationDestinations(
        NavigationPolicyInput{
            .currentLevelId = 3,
            .exits = exits,
            .customTargetLevelIds = overlapTargetIds,
        },
        destinations);
    CHECK(overlapCount == 2U);
    CHECK(destinations[0].kind == NavigationLineKind::Progression);
    CHECK(destinations[1].kind == NavigationLineKind::CustomLevel);

    const std::array canyonCorrectTombExit{
        NavigationExitCandidate{
            .destinationId = 700U,
            .targetLevelId = 70,
            .subtileX = 4'600,
            .subtileY = 5'700,
            .exactClientX = -8'800,
            .exactClientY = 9'600,
            .useExactClientCoordinates = true,
        },
    };
    const std::array canyonQuestTarget{
        NavigationPointCandidate{
            .destinationId = canyonCorrectTombExit[0].destinationId,
            .subtileX = canyonCorrectTombExit[0].subtileX,
            .subtileY = canyonCorrectTombExit[0].subtileY,
            .exactClientX = canyonCorrectTombExit[0].exactClientX,
            .exactClientY = canyonCorrectTombExit[0].exactClientY,
            .useExactClientCoordinates = true,
        },
    };
    CHECK(BuildNavigationDestinations(
        NavigationPolicyInput{
            .currentLevelId = 46,
            .exits = canyonCorrectTombExit,
            .questTargets = canyonQuestTarget,
        },
        destinations) == 1U);
    CHECK(destinations[0].kind == NavigationLineKind::Quest);
    CHECK(destinations[0].destinationId == 700U);
    CHECK(destinations[0].useExactClientCoordinates);
    CHECK(destinations[0].exactClientX == -8'800);
    CHECK(destinations[0].exactClientY == 9'600);

    CHECK(BuildNavigationDestinations(
        NavigationPolicyInput{
            .currentLevelId = 46,
            .exits = canyonCorrectTombExit,
            .progressionTargetOverride = 70,
        },
        destinations) == 1U);
    CHECK(destinations[0].kind == NavigationLineKind::Progression);
    CHECK(destinations[0].destinationId == 700U);
    CHECK(destinations[0].useExactClientCoordinates);
    CHECK(destinations[0].exactClientX == -8'800);
    CHECK(destinations[0].exactClientY == 9'600);

    const std::array staffOrificeQuestTarget{
        NavigationPointCandidate{
            .destinationId = 800U,
            .subtileX = 5'100,
            .subtileY = 5'200,
        },
    };
    CHECK(BuildNavigationDestinations(
        NavigationPolicyInput{
            .currentLevelId = 66,
            .questTargets = staffOrificeQuestTarget,
        },
        destinations) == 1U);
    CHECK(destinations[0].kind == NavigationLineKind::Quest);
    CHECK(destinations[0].destinationId == 800U);

    const std::array durielPortalExit{
        NavigationExitCandidate{
            .destinationId = 801U,
            .targetLevelId = 73,
            .subtileX = 5'300,
            .subtileY = 5'400,
        },
    };
    CHECK(BuildNavigationDestinations(
        NavigationPolicyInput{
            .currentLevelId = 66,
            .exits = durielPortalExit,
            .questTargets = staffOrificeQuestTarget,
        },
        destinations) == 1U);
    CHECK(destinations[0].kind == NavigationLineKind::Progression);
    CHECK(destinations[0].destinationId == 801U);

    std::array<NavigationSubtileDestination, 1U> bounded{};
    CHECK(BuildNavigationDestinations(
        NavigationPolicyInput{
            .currentLevelId = 3,
            .exits = exits,
            .waypoint = &waypoint,
            .questTargets = quests,
            .customTargetLevelIds = customTargetIds,
        },
        bounded) == 1U);
}

void CheckNavigationResolverHelpers() {
    using namespace RuffnecKk::MapSense;

    const std::array waypointCandidates{
        Detail::NavigationWaypointPresetCandidate{100, 200, 503, 1'004, 119},
        Detail::NavigationWaypointPresetCandidate{100, 200, 501, 1'006, 145},
        Detail::NavigationWaypointPresetCandidate{101, 200, 507, 1'004, 156},
    };
    const auto exactWaypoint = Detail::SelectExactWaypointPreset(
        100,
        200,
        waypointCandidates);
    CHECK(exactWaypoint.has_value());
    CHECK(exactWaypoint->subtileX == 503);
    CHECK(exactWaypoint->subtileY == 1'004);
    CHECK(exactWaypoint->classId == 119);
    CHECK(!Detail::SelectExactWaypointPreset(
        99,
        200,
        waypointCandidates).has_value());

    const std::array roomTileLinks{
        Detail::NavigationRoomTileLinkCandidate{501, 29},
        Detail::NavigationRoomTileLinkCandidate{502, 30},
        Detail::NavigationRoomTileLinkCandidate{503, 12},
        Detail::NavigationRoomTileLinkCandidate{504, -1},
        Detail::NavigationRoomTileLinkCandidate{505, 14},
    };
    CHECK(Detail::SelectRoomTileTargetLevel(501, roomTileLinks)
        .value_or(-1) == 29);
    CHECK(Detail::SelectRoomTileTargetLevel(502, roomTileLinks)
        .value_or(-1) == 30);
    CHECK(Detail::SelectRoomTileTargetLevel(503, roomTileLinks)
        .value_or(-1) == 12);
    CHECK(Detail::SelectRoomTileTargetLevel(505, roomTileLinks)
        .value_or(-1) == 14);
    CHECK(!Detail::SelectRoomTileTargetLevel(
        504,
        roomTileLinks).has_value());
    CHECK(!Detail::SelectRoomTileTargetLevel(
        999,
        roomTileLinks).has_value());

    const std::array tamoeVisibleTargets{26, 12, 6, 0, 0, 0, 0, 0};
    CHECK(Detail::IsDirectVisibleTarget(7, 26, tamoeVisibleTargets));
    CHECK(Detail::IsDirectVisibleTarget(7, 12, tamoeVisibleTargets));
    CHECK(!Detail::IsDirectVisibleTarget(7, 65, tamoeVisibleTargets));
    CHECK(!Detail::IsDirectVisibleTarget(7, 7, tamoeVisibleTargets));

    std::array<NavigationSubtileDestination, 4U> routeDestinations{};
    const std::array barracksExit{
        NavigationExitCandidate{1U, 29, 1'100, 1'200},
    };
    CHECK(BuildNavigationDestinations(
        NavigationPolicyInput{.currentLevelId = 28, .exits = barracksExit},
        routeDestinations) == 1U);
    CHECK(routeDestinations[0].kind == NavigationLineKind::Progression);
    CHECK(routeDestinations[0].subtileX == 1'100);
    const std::array jailOneExit{
        NavigationExitCandidate{2U, 30, 1'300, 1'400},
    };
    CHECK(BuildNavigationDestinations(
        NavigationPolicyInput{.currentLevelId = 29, .exits = jailOneExit},
        routeDestinations) == 1U);
    CHECK(routeDestinations[0].kind == NavigationLineKind::Progression);
    CHECK(routeDestinations[0].subtileY == 1'400);
    const std::array pitExit{
        NavigationExitCandidate{3U, 12, 1'500, 1'600},
    };
    const std::array pitTarget{12};
    CHECK(BuildNavigationDestinations(
        NavigationPolicyInput{
            .currentLevelId = 7,
            .exits = pitExit,
            .customTargetLevelIds = pitTarget,
        },
        routeDestinations) == 1U);
    CHECK(routeDestinations[0].kind == NavigationLineKind::CustomLevel);
    CHECK(routeDestinations[0].subtileX == 1'500);
    CHECK(routeDestinations[0].subtileY == 1'600);
    const std::array undergroundTwoExit{
        NavigationExitCandidate{4U, 14, 1'700, 1'800},
    };
    const std::array undergroundTwoTarget{14};
    CHECK(BuildNavigationDestinations(
        NavigationPolicyInput{
            .currentLevelId = 10,
            .exits = undergroundTwoExit,
            .customTargetLevelIds = undergroundTwoTarget,
        },
        routeDestinations) == 1U);
    CHECK(routeDestinations[0].kind == NavigationLineKind::CustomLevel);
    CHECK(routeDestinations[0].subtileX == 1'700);
    CHECK(routeDestinations[0].subtileY == 1'800);

    const auto source = Detail::NavigationRoomRectangle{
        .levelId = 3,
        .tileX = 100,
        .tileY = 200,
        .width = 2,
        .height = 2,
    };
    std::array<std::uint16_t, 100U> sourceCollisionCells{};
    std::array<std::uint16_t, 100U> neighbourCollisionCells{};
    const auto collisionGrid = [](
            std::array<std::uint16_t, 100U>& cells,
            std::int32_t originX,
            std::int32_t originY) noexcept {
        return Detail::NavigationCollisionGridView{
            .originX = originX,
            .originY = originY,
            .width = 10,
            .height = 10,
            .cells = std::span<const std::uint16_t>(cells),
        };
    };
    const auto setCell = [](
            std::array<std::uint16_t, 100U>& cells,
            std::int32_t x,
            std::int32_t y,
            std::uint16_t value) noexcept {
        cells[static_cast<std::size_t>(y * 10 + x)] = value;
    };
    Detail::NavigationOutdoorOpening opening{};

    std::int32_t checkedSubtile{123};
    CHECK(Detail::TryAddNavigationSubtileOffset(
        (std::numeric_limits<std::int32_t>::max)() - 1,
        1,
        checkedSubtile));
    CHECK(checkedSubtile == (std::numeric_limits<std::int32_t>::max)());
    checkedSubtile = 123;
    CHECK(!Detail::TryAddNavigationSubtileOffset(
        (std::numeric_limits<std::int32_t>::max)(),
        1,
        checkedSubtile));
    CHECK(checkedSubtile == 123);
    CHECK(!Detail::TryAddNavigationSubtileOffset(-1, 1, checkedSubtile));

    sourceCollisionCells.fill(1U);
    neighbourCollisionCells.fill(1U);
    for (std::int32_t y = 2; y < 7; ++y) {
        setCell(sourceCollisionCells, 8, y, 0U);
        setCell(sourceCollisionCells, 9, y, 0U);
        setCell(neighbourCollisionCells, 0, y, 0U);
        setCell(neighbourCollisionCells, 1, y, 0U);
    }
    CHECK(Detail::FindOutdoorCollisionOpening(
        source,
        Detail::NavigationRoomRectangle{4, 102, 200, 2, 2},
        collisionGrid(sourceCollisionCells, 500, 1'000),
        collisionGrid(neighbourCollisionCells, 510, 1'000),
        opening));
    CHECK(opening.subtileX == 510);
    CHECK(opening.subtileY == 1'004);
    CHECK(opening.spanSubtiles == 5);

    // The absolute midpoint remains representable even when adding it to the
    // grid origin as an intermediate 32-bit expression would overflow.
    constexpr std::int32_t highTileY = 214'748'364;
    constexpr std::int32_t highOriginY = highTileY * 5;
    sourceCollisionCells.fill(1U);
    neighbourCollisionCells.fill(1U);
    for (std::int32_t y = 7; y < 10; ++y) {
        setCell(sourceCollisionCells, 8, y, 0U);
        setCell(sourceCollisionCells, 9, y, 0U);
        setCell(neighbourCollisionCells, 0, y, 0U);
        setCell(neighbourCollisionCells, 1, y, 0U);
    }
    CHECK(Detail::FindOutdoorCollisionOpening(
        Detail::NavigationRoomRectangle{3, 100, highTileY, 2, 2},
        Detail::NavigationRoomRectangle{4, 102, highTileY, 2, 2},
        collisionGrid(sourceCollisionCells, 500, highOriginY),
        collisionGrid(neighbourCollisionCells, 510, highOriginY),
        opening));
    CHECK(opening.subtileX == 510);
    CHECK(opening.subtileY == highOriginY + 8);
    CHECK(opening.spanSubtiles == 3);

    sourceCollisionCells.fill(1U);
    neighbourCollisionCells.fill(1U);
    for (std::int32_t y = 3; y < 7; ++y) {
        setCell(sourceCollisionCells, 0, y, 0U);
        setCell(sourceCollisionCells, 1, y, 0U);
        setCell(neighbourCollisionCells, 8, y, 0U);
        setCell(neighbourCollisionCells, 9, y, 0U);
    }
    CHECK(Detail::FindOutdoorCollisionOpening(
        source,
        Detail::NavigationRoomRectangle{4, 98, 200, 2, 2},
        collisionGrid(sourceCollisionCells, 500, 1'000),
        collisionGrid(neighbourCollisionCells, 490, 1'000),
        opening));
    CHECK(opening.subtileX == 499);
    CHECK(opening.subtileY == 1'005);
    CHECK(opening.spanSubtiles == 4);

    sourceCollisionCells.fill(1U);
    neighbourCollisionCells.fill(1U);
    for (std::int32_t x = 1; x < 7; ++x) {
        setCell(sourceCollisionCells, x, 8, 0U);
        setCell(sourceCollisionCells, x, 9, 0U);
        setCell(neighbourCollisionCells, x, 0, 0U);
        setCell(neighbourCollisionCells, x, 1, 0U);
    }
    CHECK(Detail::FindOutdoorCollisionOpening(
        source,
        Detail::NavigationRoomRectangle{4, 100, 202, 2, 2},
        collisionGrid(sourceCollisionCells, 500, 1'000),
        collisionGrid(neighbourCollisionCells, 500, 1'010),
        opening));
    CHECK(opening.subtileX == 504);
    CHECK(opening.subtileY == 1'010);
    CHECK(opening.spanSubtiles == 6);

    sourceCollisionCells.fill(1U);
    neighbourCollisionCells.fill(1U);
    for (std::int32_t x = 4; x < 7; ++x) {
        setCell(sourceCollisionCells, x, 0, 0U);
        setCell(sourceCollisionCells, x, 1, 0U);
        setCell(neighbourCollisionCells, x, 8, 0U);
        setCell(neighbourCollisionCells, x, 9, 0U);
    }
    CHECK(Detail::FindOutdoorCollisionOpening(
        source,
        Detail::NavigationRoomRectangle{4, 100, 198, 2, 2},
        collisionGrid(sourceCollisionCells, 500, 1'000),
        collisionGrid(neighbourCollisionCells, 500, 990),
        opening));
    CHECK(opening.subtileX == 505);
    CHECK(opening.subtileY == 999);
    CHECK(opening.spanSubtiles == 3);

    // A broad source-side gap is reduced to the exact intersection exposed by
    // the destination room. This is the Tamoe/Monastery regression contract.
    sourceCollisionCells.fill(1U);
    neighbourCollisionCells.fill(1U);
    for (std::int32_t y = 0; y < 10; ++y) {
        setCell(sourceCollisionCells, 8, y, 0U);
        setCell(sourceCollisionCells, 9, y, 0U);
    }
    for (std::int32_t y = 4; y < 7; ++y) {
        setCell(neighbourCollisionCells, 0, y, 0U);
        setCell(neighbourCollisionCells, 1, y, 0U);
    }
    CHECK(Detail::FindOutdoorCollisionOpening(
        source,
        Detail::NavigationRoomRectangle{4, 102, 200, 2, 2},
        collisionGrid(sourceCollisionCells, 500, 1'000),
        collisionGrid(neighbourCollisionCells, 510, 1'000),
        opening));
    CHECK(opening.subtileX == 510);
    CHECK(opening.subtileY == 1'005);
    CHECK(opening.spanSubtiles == 3);

    neighbourCollisionCells.fill(1U);
    for (std::int32_t y = 4; y < 6; ++y) {
        setCell(neighbourCollisionCells, 0, y, 0U);
        setCell(neighbourCollisionCells, 1, y, 0U);
    }
    CHECK(!Detail::FindOutdoorCollisionOpening(
        source,
        Detail::NavigationRoomRectangle{4, 102, 200, 2, 2},
        collisionGrid(sourceCollisionCells, 500, 1'000),
        collisionGrid(neighbourCollisionCells, 510, 1'000),
        opening));
    CHECK(!Detail::FindOutdoorCollisionOpening(
        source,
        Detail::NavigationRoomRectangle{4, 102, 202, 2, 2},
        collisionGrid(sourceCollisionCells, 500, 1'000),
        collisionGrid(neighbourCollisionCells, 510, 1'000),
        opening));
    auto wrongGrid = collisionGrid(sourceCollisionCells, 500, 1'000);
    wrongGrid.originX = 499;
    CHECK(!Detail::FindOutdoorCollisionOpening(
        source,
        Detail::NavigationRoomRectangle{4, 102, 200, 2, 2},
        wrongGrid,
        collisionGrid(neighbourCollisionCells, 510, 1'000),
        opening));

    // Room fragments from both sides of one real level boundary must merge
    // before intersection. The endpoint stays on the current level's outer
    // edge, not on an arbitrary destination Room edge.
    std::array fragmentedSourceSpans{
        Detail::NavigationBoundarySpan{
            3, Detail::NavigationBoundarySide::Right, 1'040, 1'043},
        Detail::NavigationBoundarySpan{
            3, Detail::NavigationBoundarySide::Right, 1'043, 1'047},
    };
    std::array fragmentedTargetSpans{
        Detail::NavigationBoundarySpan{
            26, Detail::NavigationBoundarySide::Right, 1'039, 1'044},
        Detail::NavigationBoundarySpan{
            26, Detail::NavigationBoundarySide::Right, 1'044, 1'048},
    };
    std::size_t mergedSourceSpanCount{};
    std::size_t mergedTargetSpanCount{};
    CHECK(Detail::MergeOutdoorBoundarySpans(
        fragmentedSourceSpans,
        fragmentedSourceSpans.size(),
        mergedSourceSpanCount));
    CHECK(Detail::MergeOutdoorBoundarySpans(
        fragmentedTargetSpans,
        fragmentedTargetSpans.size(),
        mergedTargetSpanCount));
    CHECK(mergedSourceSpanCount == 1U);
    CHECK(mergedTargetSpanCount == 1U);
    CHECK(fragmentedSourceSpans[0].startSubtile == 1'040);
    CHECK(fragmentedSourceSpans[0].endSubtile == 1'047);
    CHECK(fragmentedTargetSpans[0].startSubtile == 1'039);
    CHECK(fragmentedTargetSpans[0].endSubtile == 1'048);
    const auto levelBounds = Detail::NavigationLevelSubtileBounds{
        .left = 500,
        .top = 1'000,
        .right = 800,
        .bottom = 1'300,
    };
    CHECK(Detail::FindUniqueOutdoorLevelBoundaryOpening(
        levelBounds,
        26,
        std::span(fragmentedSourceSpans).first(mergedSourceSpanCount),
        std::span(fragmentedTargetSpans).first(mergedTargetSpanCount),
        opening) == Detail::NavigationOutdoorBoundaryMatchResult::Found);
    CHECK(opening.subtileX == 799);
    CHECK(opening.subtileY == 1'043);
    CHECK(opening.spanSubtiles == 7);

    // Runtime spans retain the exact RoomsNear pair and its fixed seam. Two
    // fragments of that same pair may merge, and the endpoint must stay on
    // the source cell beside the shared seam rather than being reconstructed
    // from the complete level's outer bounds.
    std::array pairedSourceFragments{
        Detail::NavigationBoundarySpan{
            .targetLevelId = 7,
            .side = Detail::NavigationBoundarySide::Right,
            .startSubtile = 1'040,
            .endSubtile = 1'043,
            .fixedSubtile = 650,
            .sourceRoomIdentity = 0x1000U,
            .targetRoomIdentity = 0x2000U,
        },
        Detail::NavigationBoundarySpan{
            .targetLevelId = 7,
            .side = Detail::NavigationBoundarySide::Right,
            .startSubtile = 1'043,
            .endSubtile = 1'047,
            .fixedSubtile = 650,
            .sourceRoomIdentity = 0x1000U,
            .targetRoomIdentity = 0x2000U,
        },
    };
    std::array pairedTargetFragments{
        Detail::NavigationBoundarySpan{
            .targetLevelId = 26,
            .side = Detail::NavigationBoundarySide::Right,
            .startSubtile = 1'039,
            .endSubtile = 1'044,
            .fixedSubtile = 650,
            .sourceRoomIdentity = 0x1000U,
            .targetRoomIdentity = 0x2000U,
        },
        Detail::NavigationBoundarySpan{
            .targetLevelId = 26,
            .side = Detail::NavigationBoundarySide::Right,
            .startSubtile = 1'044,
            .endSubtile = 1'048,
            .fixedSubtile = 650,
            .sourceRoomIdentity = 0x1000U,
            .targetRoomIdentity = 0x2000U,
        },
    };
    std::size_t pairedSourceCount{};
    std::size_t pairedTargetCount{};
    CHECK(Detail::MergeOutdoorBoundarySpans(
        pairedSourceFragments,
        pairedSourceFragments.size(),
        pairedSourceCount));
    CHECK(Detail::MergeOutdoorBoundarySpans(
        pairedTargetFragments,
        pairedTargetFragments.size(),
        pairedTargetCount));
    CHECK(pairedSourceCount == 1U);
    CHECK(pairedTargetCount == 1U);
    CHECK(Detail::FindUniqueOutdoorLevelBoundaryOpening(
        levelBounds,
        26,
        std::span(pairedSourceFragments).first(pairedSourceCount),
        std::span(pairedTargetFragments).first(pairedTargetCount),
        opening) == Detail::NavigationOutdoorBoundaryMatchResult::Found);
    CHECK(opening.subtileX == 649);
    CHECK(opening.subtileY == 1'043);
    CHECK(opening.spanSubtiles == 7);

    // Exact RoomsNear identities remain separate even when side, fixed seam,
    // and projected interval are identical. A target fragment from the other
    // pair must not cross-match the first source pair.
    std::array parallelSourceSpans{
        pairedSourceFragments[0],
        Detail::NavigationBoundarySpan{
            .targetLevelId = 7,
            .side = Detail::NavigationBoundarySide::Right,
            .startSubtile = 1'040,
            .endSubtile = 1'047,
            .fixedSubtile = 650,
            .sourceRoomIdentity = 0x3000U,
            .targetRoomIdentity = 0x4000U,
        },
    };
    std::size_t parallelSourceCount{};
    CHECK(Detail::MergeOutdoorBoundarySpans(
        parallelSourceSpans,
        parallelSourceSpans.size(),
        parallelSourceCount));
    CHECK(parallelSourceCount == 2U);
    const std::array crossedPairTarget{
        Detail::NavigationBoundarySpan{
            .targetLevelId = 26,
            .side = Detail::NavigationBoundarySide::Right,
            .startSubtile = 1'040,
            .endSubtile = 1'047,
            .fixedSubtile = 650,
            .sourceRoomIdentity = 0x3000U,
            .targetRoomIdentity = 0x4000U,
        },
    };
    CHECK(Detail::FindUniqueOutdoorLevelBoundaryOpening(
        levelBounds,
        26,
        std::span(pairedSourceFragments).first(pairedSourceCount),
        crossedPairTarget,
        opening) == Detail::NavigationOutdoorBoundaryMatchResult::NotFound);

    // If two explicit RoomsNear pairs expose different valid seams, the
    // resolver must fail closed instead of selecting either one.
    const std::array explicitAmbiguousSource{
        pairedSourceFragments[0],
        Detail::NavigationBoundarySpan{
            .targetLevelId = 7,
            .side = Detail::NavigationBoundarySide::Right,
            .startSubtile = 1'050,
            .endSubtile = 1'056,
            .fixedSubtile = 700,
            .sourceRoomIdentity = 0x3000U,
            .targetRoomIdentity = 0x4000U,
        },
    };
    const std::array explicitAmbiguousTarget{
        pairedTargetFragments[0],
        Detail::NavigationBoundarySpan{
            .targetLevelId = 26,
            .side = Detail::NavigationBoundarySide::Right,
            .startSubtile = 1'050,
            .endSubtile = 1'056,
            .fixedSubtile = 700,
            .sourceRoomIdentity = 0x3000U,
            .targetRoomIdentity = 0x4000U,
        },
    };
    CHECK(Detail::FindUniqueOutdoorLevelBoundaryOpening(
        levelBounds,
        26,
        explicitAmbiguousSource,
        explicitAmbiguousTarget,
        opening) == Detail::NavigationOutdoorBoundaryMatchResult::Ambiguous);

    CHECK(Detail::HasNavigationOutdoorVisibilitySlot(0x10U, 0U));
    CHECK(Detail::HasNavigationOutdoorVisibilitySlot(0x800U, 7U));
    CHECK(!Detail::HasNavigationOutdoorVisibilitySlot(0x20U, 0U));
    CHECK(!Detail::HasNavigationOutdoorVisibilitySlot(0x800U, 8U));
    CHECK(Detail::IsNavigationPlayerPathOpen(0U));
    CHECK(!Detail::IsNavigationPlayerPathOpen(0x0001U));
    CHECK(!Detail::IsNavigationPlayerPathOpen(0x0008U));
    CHECK(!Detail::IsNavigationPlayerPathOpen(0x0400U));
    CHECK(!Detail::IsNavigationPlayerPathOpen(0x0800U));
    CHECK(!Detail::IsNavigationPlayerPathOpen(0x1000U));
    CHECK(Detail::IsNavigationPlayerPathOpen(0x0080U));
    CHECK(Detail::IsNavigationPlayerPathOpen(0x0100U));

    Detail::NavigationOutdoorOpening nativeMonasteryAnchor{};
    CHECK(Detail::TryMakeNavigationLevelTileAnchor(
        100,
        200,
        27,
        13,
        nativeMonasteryAnchor));
    CHECK(nativeMonasteryAnchor.subtileX == 635);
    CHECK(nativeMonasteryAnchor.subtileY == 1'065);
    CHECK(nativeMonasteryAnchor.spanSubtiles == 0);
    CHECK(!Detail::TryMakeNavigationLevelTileAnchor(
        (std::numeric_limits<std::int32_t>::max)(),
        200,
        27,
        13,
        nativeMonasteryAnchor));

    // Multiple exact player paths to one outdoor target are legitimate in the
    // jungle generator. Strict callers can reject them, while runtime keeps
    // the first geographically sorted path; width is not selection evidence.
    const auto tamoeBounds = Detail::NavigationLevelSubtileBounds{
        .left = 14'900,
        .top = 4'900,
        .right = 15'500,
        .bottom = 5'500,
    };
    const std::array tamoeFacadeSource{
        Detail::NavigationBoundarySpan{
            .targetLevelId = 7,
            .side = Detail::NavigationBoundarySide::Bottom,
            .startSubtile = 15'040,
            .endSubtile = 15'044,
            .fixedSubtile = 5'091,
            .sourceRoomIdentity = 0x5000U,
            .targetRoomIdentity = 0x6000U,
        },
        Detail::NavigationBoundarySpan{
            .targetLevelId = 7,
            .side = Detail::NavigationBoundarySide::Bottom,
            .startSubtile = 15'142,
            .endSubtile = 15'182,
            .fixedSubtile = 5'091,
            .sourceRoomIdentity = 0x7000U,
            .targetRoomIdentity = 0x8000U,
        },
    };
    const std::array tamoeFacadeTarget{
        Detail::NavigationBoundarySpan{
            .targetLevelId = 26,
            .side = Detail::NavigationBoundarySide::Bottom,
            .startSubtile = 15'040,
            .endSubtile = 15'044,
            .fixedSubtile = 5'091,
            .sourceRoomIdentity = 0x5000U,
            .targetRoomIdentity = 0x6000U,
        },
        Detail::NavigationBoundarySpan{
            .targetLevelId = 26,
            .side = Detail::NavigationBoundarySide::Bottom,
            .startSubtile = 15'142,
            .endSubtile = 15'182,
            .fixedSubtile = 5'091,
            .sourceRoomIdentity = 0x7000U,
            .targetRoomIdentity = 0x8000U,
        },
    };
    CHECK(Detail::FindUniqueOutdoorLevelBoundaryOpening(
        tamoeBounds,
        26,
        tamoeFacadeSource,
        tamoeFacadeTarget,
        opening) == Detail::NavigationOutdoorBoundaryMatchResult::Ambiguous);
    CHECK(Detail::FindUniqueOutdoorLevelBoundaryOpening(
        tamoeBounds,
        26,
        tamoeFacadeSource,
        tamoeFacadeTarget,
        opening,
        Detail::NavigationOutdoorOpeningSelectionPolicy::
            AcceptStablePlayerPath)
        == Detail::NavigationOutdoorBoundaryMatchResult::Found);
    CHECK(opening.subtileX == 15'042);
    CHECK(opening.subtileY == 5'090);
    CHECK(opening.spanSubtiles == 4);

    std::array partiallyIdentifiedSpan{
        Detail::NavigationBoundarySpan{
            .targetLevelId = 7,
            .side = Detail::NavigationBoundarySide::Right,
            .startSubtile = 1'040,
            .endSubtile = 1'047,
            .fixedSubtile = 650,
            .sourceRoomIdentity = 0x1000U,
        },
    };
    std::size_t invalidMergedCount{};
    CHECK(!Detail::MergeOutdoorBoundarySpans(
        partiallyIdentifiedSpan,
        partiallyIdentifiedSpan.size(),
        invalidMergedCount));

    // The already merged source frontier is cacheable across targets; only
    // the independently collected destination spans carry the target id.
    const std::array secondTargetBoundary{
        Detail::NavigationBoundarySpan{
            27, Detail::NavigationBoundarySide::Right, 1'041, 1'045},
    };
    CHECK(Detail::FindUniqueOutdoorLevelBoundaryOpening(
        levelBounds,
        27,
        std::span(fragmentedSourceSpans).first(mergedSourceSpanCount),
        secondTargetBoundary,
        opening) == Detail::NavigationOutdoorBoundaryMatchResult::Found);
    CHECK(opening.subtileX == 799);
    CHECK(opening.subtileY == 1'043);
    CHECK(opening.spanSubtiles == 4);

    // A broad destination-side seam is harmless when the complete current
    // level boundary exposes only the narrow, real road opening.
    const std::array narrowOuterBoundary{
        Detail::NavigationBoundarySpan{
            3, Detail::NavigationBoundarySide::Right, 1'100, 1'104},
    };
    const std::array broadDestinationBoundary{
        Detail::NavigationBoundarySpan{
            26, Detail::NavigationBoundarySide::Right, 1'060, 1'140},
    };
    CHECK(Detail::FindUniqueOutdoorLevelBoundaryOpening(
        levelBounds,
        26,
        narrowOuterBoundary,
        broadDestinationBoundary,
        opening) == Detail::NavigationOutdoorBoundaryMatchResult::Found);
    CHECK(opening.subtileX == 799);
    CHECK(opening.subtileY == 1'102);
    CHECK(opening.spanSubtiles == 4);

    // If two disconnected openings survive full-level validation, neither
    // the widest nor the lowest coordinate may be guessed.
    const std::array ambiguousSourceBoundary{
        Detail::NavigationBoundarySpan{
            3, Detail::NavigationBoundarySide::Left, 1'010, 1'050},
        Detail::NavigationBoundarySpan{
            3, Detail::NavigationBoundarySide::Right, 1'100, 1'104},
    };
    const std::array ambiguousTargetBoundary{
        Detail::NavigationBoundarySpan{
            26, Detail::NavigationBoundarySide::Left, 1'010, 1'050},
        Detail::NavigationBoundarySpan{
            26, Detail::NavigationBoundarySide::Right, 1'100, 1'104},
    };
    CHECK(Detail::FindUniqueOutdoorLevelBoundaryOpening(
        levelBounds,
        26,
        ambiguousSourceBoundary,
        ambiguousTargetBoundary,
        opening) == Detail::NavigationOutdoorBoundaryMatchResult::Ambiguous);

    std::array<Detail::NavigationExitSelection, 3U> selections{};
    std::size_t selectionCount{};
    const auto makeSelection = [](
            std::uint64_t destinationId,
            std::int32_t targetLevelId,
            std::int32_t subtileX,
            std::int32_t subtileY,
            Detail::NavigationExitEvidence evidence,
            std::int32_t span) noexcept {
        return Detail::NavigationExitSelection{
            .candidate = NavigationExitCandidate{
                .destinationId = destinationId,
                .targetLevelId = targetLevelId,
                .subtileX = subtileX,
                .subtileY = subtileY,
            },
            .evidence = evidence,
            .spanSubtiles = span,
        };
    };
    CHECK(Detail::UpsertExitSelection(
        3,
        selections,
        selectionCount,
        makeSelection(
            10U,
            4,
            100,
            200,
            Detail::NavigationExitEvidence::OutdoorCollision,
            10)));
    CHECK(selectionCount == 1U);
    CHECK(Detail::UpsertExitSelection(
        3,
        selections,
        selectionCount,
        makeSelection(
            11U,
            4,
            90,
            190,
            Detail::NavigationExitEvidence::OutdoorCollision,
            8)));
    CHECK(selections[0].candidate.destinationId == 10U);
    CHECK(Detail::UpsertExitSelection(
        3,
        selections,
        selectionCount,
        makeSelection(
            12U,
            4,
            120,
            220,
            Detail::NavigationExitEvidence::OutdoorCollision,
            20)));
    CHECK(selections[0].candidate.destinationId == 12U);
    CHECK(Detail::UpsertExitSelection(
        3,
        selections,
        selectionCount,
        makeSelection(
            19U,
            4,
            110,
            210,
            Detail::NavigationExitEvidence::OutdoorCollision,
            21)));
    CHECK(selections[0].candidate.destinationId == 19U);
    CHECK(Detail::UpsertExitSelection(
        3,
        selections,
        selectionCount,
        makeSelection(
            13U,
            4,
            300,
            400,
            Detail::NavigationExitEvidence::RoomTile,
            0)));
    CHECK(selections[0].candidate.destinationId == 13U);
    CHECK(selections[0].evidence
        == Detail::NavigationExitEvidence::RoomTile);
    CHECK(Detail::UpsertExitSelection(
        3,
        selections,
        selectionCount,
        makeSelection(
            21U,
            4,
            305,
            405,
            Detail::NavigationExitEvidence::QuestPreset,
            0)));
    CHECK(selections[0].candidate.destinationId == 21U);
    CHECK(Detail::UpsertExitSelection(
        3,
        selections,
        selectionCount,
        makeSelection(
            22U,
            4,
            307,
            407,
            Detail::NavigationExitEvidence::BossPreset,
            0)));
    CHECK(selections[0].candidate.destinationId == 22U);
    CHECK(Detail::UpsertExitSelection(
        3,
        selections,
        selectionCount,
        makeSelection(
            20U,
            4,
            310,
            410,
            Detail::NavigationExitEvidence::RuntimeObject,
            0)));
    CHECK(selections[0].candidate.destinationId == 20U);
    CHECK(Detail::UpsertExitSelection(
        3,
        selections,
        selectionCount,
        makeSelection(
            14U,
            4,
            50,
            60,
            Detail::NavigationExitEvidence::OutdoorCollision,
            100)));
    CHECK(selections[0].candidate.destinationId == 20U);
    CHECK(!Detail::UpsertExitSelection(
        3,
        selections,
        selectionCount,
        makeSelection(
            15U,
            3,
            50,
            60,
            Detail::NavigationExitEvidence::RoomTile,
            0)));
    CHECK(Detail::UpsertExitSelection(
        3,
        selections,
        selectionCount,
        makeSelection(
            16U,
            5,
            500,
            600,
            Detail::NavigationExitEvidence::OutdoorCollision,
            10)));
    CHECK(Detail::UpsertExitSelection(
        3,
        selections,
        selectionCount,
        makeSelection(
            17U,
            6,
            700,
            800,
            Detail::NavigationExitEvidence::OutdoorCollision,
            10)));
    CHECK(selectionCount == selections.size());
    CHECK(!Detail::UpsertExitSelection(
        3,
        selections,
        selectionCount,
        makeSelection(
            18U,
            7,
            900,
            1'000,
            Detail::NavigationExitEvidence::OutdoorCollision,
            10)));
}

void CheckNavigationLevelCatalogContract() {
    using namespace RuffnecKk::MapSense;

    CHECK(ResolveCanonicalLevelName("Pit Level 1").value_or(-1) == 12);
    CHECK(ResolveCanonicalLevelName("Underground Passage Level 2")
        .value_or(-1) == 14);
    CHECK(ResolveCanonicalLevelName("mausoleum").value_or(-1) == 19);
    CHECK(ResolveCanonicalLevelName("ANCIENT TUNNELS").value_or(-1) == 65);
    CHECK(ResolveCanonicalLevelName("Icy Cellar").value_or(-1) == 119);
    CHECK(!ResolveCanonicalLevelName("Tal Rasha's Tomb").has_value());
    CHECK(!ResolveCanonicalLevelName("Sewers Level 1").has_value());
    CHECK(!ResolveCanonicalLevelName("Tristram").has_value());
    CHECK(!ResolveCanonicalLevelName("Not A D2R Level").has_value());
}

struct NativeLayoutAudit {
    std::set<std::string> names;
    std::set<std::string> messages;
    std::vector<std::array<int, 4>> actionRects;
    int buttonCount{};
    int clickCatcherCount{};
    int toggleCount{};
    int tabBarCount{};
    int scrollViewCount{};
    int scrollControllerCount{};
    int tableCount{};
    bool namesUnique{true};
    bool textRectsExplicit{true};
};

void AuditNativeLayoutNode(
        const nlohmann::json& node,
        NativeLayoutAudit& audit) {
    if (!node.is_object()) return;
    const auto type = node.value("type", std::string{});
    const auto name = node.value("name", std::string{});
    if (name.empty() || !audit.names.insert(name).second) {
        audit.namesUnique = false;
    }

    if (type == "ButtonWidget") ++audit.buttonCount;
    if (type == "ClickCatcherWidget") ++audit.clickCatcherCount;
    if (type == "ToggleButtonWidget") ++audit.toggleCount;
    if (type == "TabBarWidget") ++audit.tabBarCount;
    if (type == "ScrollViewWidget") ++audit.scrollViewCount;
    if (type == "ScrollControllerWidget") ++audit.scrollControllerCount;
    if (type == "TableWidget") ++audit.tableCount;

    if (node.contains("fields") && node["fields"].is_object()) {
        const auto& fields = node["fields"];
        if (fields.contains("onClickMessage")
            && fields["onClickMessage"].is_string()) {
            audit.messages.insert(
                fields["onClickMessage"].get<std::string>());
        }
        if (type == "TextBoxWidget") {
            audit.textRectsExplicit = audit.textRectsExplicit
                && fields.contains("rect")
                && fields["rect"].is_object()
                && fields["rect"].value("width", 0) > 0
                && fields["rect"].value("height", 0) > 0;
        }
        if (type == "ButtonWidget"
            && name != "CloseButton"
            && fields.contains("rect")
            && fields["rect"].is_object()) {
            const auto& rect = fields["rect"];
            audit.actionRects.push_back({
                rect.value("x", 0),
                rect.value("y", 0),
                527,
                117,
            });
        }
    }

    if (!node.contains("children") || !node["children"].is_array()) return;
    for (const auto& child : node["children"]) {
        AuditNativeLayoutNode(child, audit);
    }
}

auto RectsOverlap(
        const std::array<int, 4>& left,
        const std::array<int, 4>& right) -> bool {
    return left[0] < right[0] + right[2]
        && left[0] + left[2] > right[0]
        && left[1] < right[1] + right[3]
        && left[1] + left[3] > right[1];
}

} // namespace

int main(int argc, char** argv) {
    using namespace RuffnecKk::MapSense;

    CheckNavigationProjectionDiagnosticCacheContract();
    CheckRevealPersistenceContract();
    CheckNavigationEngineContract();
    CheckNavigationLevelCatalogContract();
    CheckNavigationPolicyContract();
    CheckNavigationResolverHelpers();

    static_assert(CurrentConfigSchemaVersion == 9);
    static_assert(Detail::SquaredWorldSubtileDistance(0U, 0U, 3U, 4U)
        == 25U);
    static_assert(Detail::ClassifyWorldSubtileDistanceSquared(79U * 79U)
        == Detail::WorldSubtileDistanceBand::Through80);
    static_assert(Detail::ClassifyWorldSubtileDistanceSquared(80U * 80U)
        == Detail::WorldSubtileDistanceBand::Through80);
    static_assert(Detail::ClassifyWorldSubtileDistanceSquared(81U * 81U)
        == Detail::WorldSubtileDistanceBand::From81Through140);
    static_assert(Detail::ClassifyWorldSubtileDistanceSquared(140U * 140U)
        == Detail::WorldSubtileDistanceBand::From81Through140);
    static_assert(Detail::ClassifyWorldSubtileDistanceSquared(141U * 141U)
        == Detail::WorldSubtileDistanceBand::From141Through220);
    static_assert(Detail::ClassifyWorldSubtileDistanceSquared(219U * 219U)
        == Detail::WorldSubtileDistanceBand::From141Through220);
    static_assert(Detail::ClassifyWorldSubtileDistanceSquared(220U * 220U)
        == Detail::WorldSubtileDistanceBand::From141Through220);
    static_assert(Detail::ClassifyWorldSubtileDistanceSquared(221U * 221U)
        == Detail::WorldSubtileDistanceBand::Beyond220);
    static_assert(ShapeFor(MonsterRank::Normal) == MonsterShape::Circle);
    static_assert(ShapeFor(MonsterRank::Minion) == MonsterShape::Triangle);
    static_assert(ShapeFor(MonsterRank::Champion) == MonsterShape::Diamond);
    static_assert(ShapeFor(MonsterRank::Unique) == MonsterShape::Star);
    static_assert(ShapeFor(MonsterRank::SuperUnique) == MonsterShape::Hexagon);
    static_assert(
        Detail::ClassifyMonsterRankFlags(0x00) == MonsterRank::Normal);
    static_assert(
        Detail::ClassifyMonsterRankFlags(0x02) == MonsterRank::SuperUnique);
    static_assert(
        Detail::ClassifyMonsterRankFlags(0x04) == MonsterRank::Champion);
    static_assert(
        Detail::ClassifyMonsterRankFlags(0x10) == MonsterRank::Minion);
    static_assert(
        Detail::ClassifyMonsterRankFlags(0x08) == MonsterRank::Unique);
    static_assert(
        Detail::ClassifyMonsterRankFlags(0x0C) == MonsterRank::Champion);
    static_assert(
        Detail::ClassifyMonsterRankFlags(0x0A) == MonsterRank::SuperUnique);
    static_assert(
        Detail::ClassifyMonsterRankFlags(0x0E) == MonsterRank::SuperUnique);
    static_assert(
        Detail::ClassifyMonsterRankFlags(0x14) == MonsterRank::Champion);
    static_assert(
        Detail::ClassifyMonsterRankFlags(0x18) == MonsterRank::Unique);
    static_assert(
        Detail::ClassifyMonsterRankFlags(0x1E) == MonsterRank::SuperUnique);
    static_assert(
        Detail::ClassifyMonsterRankFlags(0x01) == MonsterRank::Normal);
    static_assert(
        Detail::ClassifyMonsterRankFlags(0x20) == MonsterRank::Normal);
    static_assert(
        Detail::ClassifyMonsterRankFlags(0x40) == MonsterRank::Normal);
    static_assert(
        Detail::ClassifyMonsterRankFlags(0x80) == MonsterRank::Normal);
    static_assert(Detail::IsEnemyMarkerUnitEligible(0U));
    static_assert(!Detail::IsEnemyMarkerUnitEligible(
        Detail::UnitIsMercenaryFlag));
    static_assert(!Detail::IsEnemyMarkerUnitEligible(
        Detail::UnitIsAsyncFlag));
    static_assert(!Detail::IsEnemyMarkerUnitEligible(
        Detail::UnitIsMercenaryFlag | Detail::UnitIsAsyncFlag));
    static_assert(Detail::IsEnemyMarkerClassEligible(
        Detail::MonStatsKillableFlag));
    static_assert(!Detail::IsEnemyMarkerClassEligible(0U));
    static_assert(!Detail::IsEnemyMarkerClassEligible(
        Detail::MonStatsKillableFlag | Detail::MonStatsNpcFlag));
    static_assert(!Detail::IsEnemyMarkerClassEligible(
        Detail::MonStatsKillableFlag | Detail::MonStatsInteractFlag));
    static_assert(!Detail::IsEnemyMarkerClassEligible(
        Detail::MonStatsKillableFlag | Detail::MonStatsInTownFlag));
    static_assert(Detail::IsEnemyMarkerAlignmentEligible(
        Detail::EvilAlignment));
    static_assert(!Detail::IsEnemyMarkerAlignmentEligible(1));
    static_assert(!Detail::IsEnemyMarkerAlignmentEligible(2));
    static_assert(Detail::BuildMonsterImmunityMask(
        std::array<std::int32_t, 6>{99, 99, 99, 99, 99, 99}) == 0U);
    static_assert(Detail::BuildMonsterImmunityMask(
        std::array<std::int32_t, 6>{100, 99, 99, 99, 99, 99})
        == ImmunityBit(MonsterImmunity::Physical));
    static_assert(Detail::BuildMonsterImmunityMask(
        std::array<std::int32_t, 6>{99, 100, 99, 99, 99, 99})
        == ImmunityBit(MonsterImmunity::Fire));
    static_assert(Detail::BuildMonsterImmunityMask(
        std::array<std::int32_t, 6>{99, 99, 100, 99, 99, 99})
        == ImmunityBit(MonsterImmunity::Cold));
    static_assert(Detail::BuildMonsterImmunityMask(
        std::array<std::int32_t, 6>{99, 99, 99, 100, 99, 99})
        == ImmunityBit(MonsterImmunity::Lightning));
    static_assert(Detail::BuildMonsterImmunityMask(
        std::array<std::int32_t, 6>{99, 99, 99, 99, 100, 99})
        == ImmunityBit(MonsterImmunity::Poison));
    static_assert(Detail::BuildMonsterImmunityMask(
        std::array<std::int32_t, 6>{99, 99, 99, 99, 99, 100})
        == ImmunityBit(MonsterImmunity::Magic));
    static_assert(Detail::BuildMonsterImmunityMask(
        std::array<std::int32_t, 6>{100, 101, 102, 103, 104, 105})
        == 0x3FU);
    static_assert(Detail::BuildMonsterImmunityMask(
        std::array<std::int32_t, 6>{100, 99, 99, 100, 99, 100})
        == static_cast<std::uint8_t>(
            ImmunityBit(MonsterImmunity::Physical)
            | ImmunityBit(MonsterImmunity::Lightning)
            | ImmunityBit(MonsterImmunity::Magic)));
    static_assert(ColorFor(Element::Fire) == ScenePalette::Fire);
    static_assert(ColorFor(Element::Cold) == ScenePalette::Cold);
    static_assert(std::is_const_v<SceneSnapshotPtr::element_type>);

    const auto nativeLayout = nlohmann::json::parse(
        NativeSettingsPanelLayoutView.begin(),
        NativeSettingsPanelLayoutView.end());
    CHECK(nativeLayout.value("type", std::string{}) == "Panel");
    CHECK(nativeLayout.value("name", std::string{})
        == NativeSettingsPanelQualifiedName);

    NativeLayoutAudit layoutAudit{};
    AuditNativeLayoutNode(nativeLayout, layoutAudit);
    CHECK(layoutAudit.namesUnique);
    CHECK(layoutAudit.buttonCount == 5);
    CHECK(layoutAudit.clickCatcherCount == 1);
    CHECK(layoutAudit.toggleCount == 0);
    CHECK(layoutAudit.tabBarCount == 0);
    CHECK(layoutAudit.scrollViewCount == 0);
    CHECK(layoutAudit.scrollControllerCount == 0);
    CHECK(layoutAudit.tableCount == 0);
    CHECK(layoutAudit.textRectsExplicit);
    CHECK(layoutAudit.actionRects.size() == 4U);
    for (std::size_t left = 0; left < layoutAudit.actionRects.size(); ++left) {
        for (std::size_t right = left + 1U;
             right < layoutAudit.actionRects.size();
             ++right) {
            CHECK(!RectsOverlap(
                layoutAudit.actionRects[left],
                layoutAudit.actionRects[right]));
        }
    }

    const std::set<std::string> expectedMessages{
        "PanelManager:ClosePanel:ruffneckk-mapsense/MapSenseControls",
        "PanelManager:OpenPanel:RuffnecKkMapSenseRevealLevel",
        "PanelManager:OpenPanel:RuffnecKkMapSenseRevealAct",
        "PanelManager:OpenPanel:RuffnecKkMapSenseRevealAll",
        "PanelManager:OpenPanel:RuffnecKkMapSenseRevealOff",
    };
    CHECK(layoutAudit.messages == expectedMessages);

    auto lowerLayout = std::string(NativeSettingsPanelLayoutView);
    for (auto& character : lowerLayout) {
        character = static_cast<char>(
            std::tolower(static_cast<unsigned char>(character)));
    }
    constexpr std::array forbiddenCopy{
        "preview",
        "unsaved",
        "draft",
        "diagnostic",
        "runtime",
        "planned",
        "prototype",
        "automap addition",
        "apply",
        "default",
        "discard",
    };
    for (const auto* text : forbiddenCopy) {
        CHECK(lowerLayout.find(text) == std::string::npos);
    }

    CHECK(ClassifyNativeSettingsMessage(
        "PanelManager",
        "OpenPanel",
        "RuffnecKkMapSenseRevealLevel")
        == NativeSettingsAction::RevealLevel);
    CHECK(ClassifyNativeSettingsMessage(
        "PanelManager",
        "OpenPanel",
        "RuffnecKkMapSenseRevealAct")
        == NativeSettingsAction::RevealAct);
    CHECK(ClassifyNativeSettingsMessage(
        "PanelManager",
        "OpenPanel",
        "RuffnecKkMapSenseRevealAll")
        == NativeSettingsAction::ToggleRevealAll);
    CHECK(ClassifyNativeSettingsMessage(
        "PanelManager",
        "OpenPanel",
        "RuffnecKkMapSenseRevealOff")
        == NativeSettingsAction::DisableRevealAll);
    CHECK(!ClassifyNativeSettingsMessage(
        "PanelManager",
        "ClosePanel",
        "RuffnecKkMapSenseRevealLevel").has_value());
    CHECK(!ClassifyNativeSettingsMessage(
        "PanelManager",
        "OpenPanel",
        "RuffnecKkMapSenseRevealLevelExtra").has_value());
    CHECK(!ClassifyNativeSettingsMessage(
        "PanelManager",
        "OpenPanel",
        "ruffneckkMapSenseRevealLevel").has_value());
    CHECK(!ClassifyNativeSettingsMessage(
        "OtherTarget",
        "OpenPanel",
        "RuffnecKkMapSenseRevealLevel").has_value());

    const auto configured = ParseConfig(R"toml(
    schema_version = 9
[general]
enabled = false
[overlay]
opacity = 0.75
scale = 1.25
frame_rate = 90
[monsters]
detection_radius = 142
[monsters.normal]
shape = "x"
color = "#EDEDEDFF"
size = 16
thickness = 1.50
[monsters.minion]
shape = "player_cross"
color = "#FFD43BCC"
size = 17
thickness = 2.50
[monsters.champion]
shape = "dot"
color = "#3D8BFFFF"
size = 19
[monsters.unique]
shape = "player_cross"
color = "#FF8A24FF"
size = 23
thickness = 4.00
[monsters.super_unique_boss]
shape = "dot"
color = "#FF3B30FF"
size = 27
[immunities]
enabled = true
style = "split_halo"
indicator_size = 19
halo_thickness = 3.5
physical = "#AABBCCDD"
fire = "#FF2200CC"
[navigation]
line_thickness = 3.25
[navigation.waypoint]
enabled = false
color = "#1020F0CC"
[navigation.progression]
enabled = true
color = "#20F040DD"
[navigation.quests]
enabled = false
color = "#F02030EE"
[navigation.custom_levels]
enabled = true
color = "#A040F0FF"
targets = [
  { level_id = 12 },
  { level_name = "Mausoleum" },
  { level_name = "Ancient Tunnels" },
  { level_id = 119 },
]
[hud]
session_timer = true
[menu]
show_launcher = false
start_expanded = true
remember_position = false
position_x = 0.23
position_y = 0.81
[diagnostics]
enabled = true
)toml");
    CHECK(!configured.enabled);
    CHECK(configured.diagnostics);
    CHECK(configured.overlay.opacity == 0.75F);
    CHECK(configured.overlay.scale == 1.25F);
    CHECK(configured.overlay.frameRate == 90);
    CHECK(configured.monsters.normal.shape == MonsterMarkerShape::X);
    CHECK(configured.monsters.minion.shape
        == MonsterMarkerShape::PlayerCross);
    CHECK(configured.monsters.champion.shape == MonsterMarkerShape::Dot);
    CHECK(configured.monsters.unique.shape
        == MonsterMarkerShape::PlayerCross);
    CHECK(configured.monsters.superUniqueBoss.shape
        == MonsterMarkerShape::Dot);
    CHECK(configured.monsters.normal.size == 16.0F);
    CHECK(configured.monsters.minion.size == 17.0F);
    CHECK(configured.monsters.champion.size == 19.0F);
    CHECK(configured.monsters.unique.size == 23.0F);
    CHECK(configured.monsters.superUniqueBoss.size == 27.0F);
    CHECK(configured.monsters.normal.thickness == 1.50F);
    CHECK(configured.monsters.minion.thickness == 2.50F);
    CHECK(configured.monsters.champion.thickness == 2.0F);
    CHECK(configured.monsters.unique.thickness == 4.0F);
    CHECK(configured.monsters.superUniqueBoss.thickness == 2.0F);
    CHECK(ColorToHex(configured.monsters.normal.color) == "#EDEDEDFF");
    CHECK(ColorToHex(configured.monsters.minion.color) == "#FFD43BCC");
    CHECK(ColorToHex(configured.monsters.champion.color) == "#3D8BFFFF");
    CHECK(ColorToHex(configured.monsters.unique.color) == "#FF8A24FF");
    CHECK(ColorToHex(configured.monsters.superUniqueBoss.color)
        == "#FF3B30FF");
    CHECK(configured.immunities.enabled);
    CHECK(configured.immunities.style == ImmunityDisplayStyle::SplitHalo);
    CHECK(configured.immunities.indicatorSize == 19.0F);
    CHECK(configured.immunities.haloThickness == 3.5F);
    CHECK(ColorToHex(configured.immunities.physical) == "#AABBCCDD");
    CHECK(configured.immunities.fire.red == 1.0F);
    CHECK(configured.immunities.fire.alpha > 0.79F);
    CHECK(configured.navigation.lineThickness == 3.25F);
    CHECK(!configured.navigation.waypoint.enabled);
    CHECK(ColorToHex(configured.navigation.waypoint.color) == "#1020F0CC");
    CHECK(configured.navigation.progression.enabled);
    CHECK(ColorToHex(configured.navigation.progression.color) == "#20F040DD");
    CHECK(!configured.navigation.quests.enabled);
    CHECK(ColorToHex(configured.navigation.quests.color) == "#F02030EE");
    CHECK(configured.navigation.customLevels.enabled);
    CHECK(ColorToHex(configured.navigation.customLevels.color) == "#A040F0FF");
    CHECK(configured.navigation.customLevels.targets.size() == 4U);
    CHECK(std::get<std::int32_t>(
        configured.navigation.customLevels.targets[0]) == 12);
    CHECK(std::get<std::string>(
        configured.navigation.customLevels.targets[1]) == "Mausoleum");
    CHECK(std::get<std::string>(
        configured.navigation.customLevels.targets[2])
        == "Ancient Tunnels");
    CHECK(std::get<std::int32_t>(
        configured.navigation.customLevels.targets[3]) == 119);
    CHECK(configured.hud.sessionTimer);
    CHECK(!configured.menu.showLauncher);
    CHECK(configured.menu.startExpanded);
    CHECK(!configured.menu.rememberPosition);
    CHECK(configured.menu.positionX == 0.23F);
    CHECK(configured.menu.positionY == 0.81F);

    const auto serialized = SerializeConfig(configured);
    CHECK(serialized.find("[hud]") == std::string::npos);
    CHECK(serialized.find("start_menu_open") == std::string::npos);
    CHECK(serialized.find("marker_size") == std::string::npos);
    constexpr std::array legacyMonsterToggleKeys{
        "\nnormal = ",
        "\nminion = ",
        "\nchampion = ",
        "\nunique = ",
        "\nsuper_unique = ",
    };
    for (const auto* key : legacyMonsterToggleKeys) {
        CHECK(serialized.find(key) == std::string::npos);
    }
    CHECK(serialized.find("[monsters.normal]\nshape = \"x\"")
        != std::string::npos);
    CHECK(serialized.find(
        "[monsters.minion]\nshape = \"player_cross\"")
        != std::string::npos);
    CHECK(serialized.find("[monsters.champion]\nshape = \"dot\"")
        != std::string::npos);
    CHECK(serialized.find(
        "[monsters.unique]\nshape = \"player_cross\"")
        != std::string::npos);
    CHECK(serialized.find(
        "[monsters.super_unique_boss]\nshape = \"dot\"")
        != std::string::npos);
    CHECK(serialized.find("[immunities]\nenabled = true")
        != std::string::npos);
    CHECK(serialized.find("style = \"split_halo\"")
        != std::string::npos);
    CHECK(serialized.find("indicator_size = 19.00")
        != std::string::npos);
    CHECK(serialized.find("detection_radius") == std::string::npos);
    CHECK(serialized.find("marker_thickness") == std::string::npos);
    CHECK(serialized.find("size = 16.00\nthickness = 1.50")
        != std::string::npos);
    CHECK(serialized.find("size = 17.00\nthickness = 2.50")
        != std::string::npos);
    CHECK(serialized.find(
        "[monsters.champion]\nshape = \"dot\"\n"
        "color = \"#3D8BFFFF\"\nsize = 19.00\n\n")
        != std::string::npos);
    CHECK(serialized.find("size = 23.00\nthickness = 4.00")
        != std::string::npos);
    CHECK(serialized.find(
        "[monsters.super_unique_boss]\nshape = \"dot\"\n"
        "color = \"#FF3B30FF\"\nsize = 27.00\n\n")
        != std::string::npos);
    CHECK(serialized.find("halo_thickness = 3.50")
        != std::string::npos);
    CHECK(serialized.find("[navigation]\nline_thickness = 3.25")
        != std::string::npos);
    CHECK(serialized.find("[navigation.waypoint]\nenabled = false")
        != std::string::npos);
    CHECK(serialized.find("[navigation.progression]\nenabled = true")
        != std::string::npos);
    CHECK(serialized.find("[navigation.quests]\nenabled = false")
        != std::string::npos);
    CHECK(serialized.find("[navigation.custom_levels]\nenabled = true")
        != std::string::npos);
    CHECK(serialized.find("{ level_id = 12 }") != std::string::npos);
    CHECK(serialized.find("{ level_name = \"Mausoleum\" }")
        != std::string::npos);
    CHECK(serialized.find("{ level_name = \"Ancient Tunnels\" }")
        != std::string::npos);
    CHECK(serialized.find("{ level_id = 119 }") != std::string::npos);
    CHECK(serialized.find("[menu]") != std::string::npos);
    const auto roundTrip = ParseConfig(serialized);
    CHECK(!roundTrip.enabled);
    CHECK(roundTrip.overlay.frameRate == 90);
    CHECK(roundTrip.monsters.normal.shape == MonsterMarkerShape::X);
    CHECK(roundTrip.monsters.minion.shape
        == MonsterMarkerShape::PlayerCross);
    CHECK(roundTrip.monsters.champion.shape == MonsterMarkerShape::Dot);
    CHECK(roundTrip.monsters.unique.shape
        == MonsterMarkerShape::PlayerCross);
    CHECK(roundTrip.monsters.superUniqueBoss.shape
        == MonsterMarkerShape::Dot);
    CHECK(roundTrip.monsters.normal.size == 16.0F);
    CHECK(roundTrip.monsters.minion.size == 17.0F);
    CHECK(roundTrip.monsters.champion.size == 19.0F);
    CHECK(roundTrip.monsters.unique.size == 23.0F);
    CHECK(roundTrip.monsters.superUniqueBoss.size == 27.0F);
    CHECK(roundTrip.monsters.normal.thickness == 1.50F);
    CHECK(roundTrip.monsters.minion.thickness == 2.50F);
    CHECK(roundTrip.monsters.champion.thickness == 2.0F);
    CHECK(roundTrip.monsters.unique.thickness == 4.0F);
    CHECK(roundTrip.monsters.superUniqueBoss.thickness == 2.0F);
    CHECK(ColorToHex(roundTrip.monsters.normal.color) == "#EDEDEDFF");
    CHECK(ColorToHex(roundTrip.monsters.minion.color) == "#FFD43BCC");
    CHECK(ColorToHex(roundTrip.monsters.champion.color) == "#3D8BFFFF");
    CHECK(ColorToHex(roundTrip.monsters.unique.color) == "#FF8A24FF");
    CHECK(ColorToHex(roundTrip.monsters.superUniqueBoss.color)
        == "#FF3B30FF");
    CHECK(roundTrip.immunities.enabled);
    CHECK(roundTrip.immunities.style == ImmunityDisplayStyle::SplitHalo);
    CHECK(roundTrip.immunities.indicatorSize == 19.0F);
    CHECK(roundTrip.immunities.haloThickness == 3.5F);
    CHECK(ColorToHex(roundTrip.immunities.physical) == "#AABBCCDD");
    CHECK(ColorToHex(roundTrip.immunities.fire) == "#FF2200CC");
    CHECK(roundTrip.navigation.lineThickness == 3.25F);
    CHECK(!roundTrip.navigation.waypoint.enabled);
    CHECK(ColorToHex(roundTrip.navigation.waypoint.color) == "#1020F0CC");
    CHECK(roundTrip.navigation.progression.enabled);
    CHECK(ColorToHex(roundTrip.navigation.progression.color) == "#20F040DD");
    CHECK(!roundTrip.navigation.quests.enabled);
    CHECK(ColorToHex(roundTrip.navigation.quests.color) == "#F02030EE");
    CHECK(roundTrip.navigation.customLevels.enabled);
    CHECK(ColorToHex(roundTrip.navigation.customLevels.color) == "#A040F0FF");
    CHECK(roundTrip.navigation.customLevels.targets
        == configured.navigation.customLevels.targets);
    CHECK(!roundTrip.hud.mercenaryHealth);
    CHECK(!roundTrip.hud.sessionTimer);
    CHECK(!roundTrip.hud.experienceTracker);
    CHECK(!roundTrip.menu.showLauncher);
    CHECK(roundTrip.menu.startExpanded);
    CHECK(!roundTrip.menu.rememberPosition);
    CHECK(roundTrip.menu.positionX == 0.23F);
    CHECK(roundTrip.menu.positionY == 0.81F);

    const auto schema7QuestMigration = ParseConfig(R"toml(
schema_version = 7
[navigation.quests]
enabled = false
color = "#F02030EE"
)toml");
    CHECK(schema7QuestMigration.navigation.quests.enabled);
    CHECK(ColorToHex(schema7QuestMigration.navigation.quests.color)
        == "#F02030EE");

    const auto defaults = ParseConfig("schema_version = 4");
    CHECK(defaults.overlay.opacity == 1.0F);
    CHECK(defaults.monsters.normal.shape
        == MonsterMarkerShape::PlayerCross);
    CHECK(defaults.monsters.minion.shape
        == MonsterMarkerShape::PlayerCross);
    CHECK(defaults.monsters.champion.shape
        == MonsterMarkerShape::PlayerCross);
    CHECK(defaults.monsters.unique.shape
        == MonsterMarkerShape::PlayerCross);
    CHECK(defaults.monsters.superUniqueBoss.shape
        == MonsterMarkerShape::PlayerCross);
    CHECK(defaults.monsters.normal.size == 18.0F);
    CHECK(defaults.monsters.minion.size == 18.0F);
    CHECK(defaults.monsters.champion.size == 20.0F);
    CHECK(defaults.monsters.unique.size == 22.0F);
    CHECK(defaults.monsters.superUniqueBoss.size == 24.0F);
    CHECK(defaults.monsters.normal.thickness == 2.0F);
    CHECK(defaults.monsters.minion.thickness == 2.0F);
    CHECK(defaults.monsters.champion.thickness == 2.0F);
    CHECK(defaults.monsters.unique.thickness == 2.0F);
    CHECK(defaults.monsters.superUniqueBoss.thickness == 2.0F);
    CHECK(ColorToHex(defaults.monsters.normal.color) == "#FFFFFFFF");
    CHECK(ColorToHex(defaults.monsters.minion.color) == "#FFD43BFF");
    CHECK(ColorToHex(defaults.monsters.champion.color) == "#3D8BFFFF");
    CHECK(ColorToHex(defaults.monsters.unique.color) == "#FF8A24FF");
    CHECK(ColorToHex(defaults.monsters.superUniqueBoss.color) == "#FF3B30FF");
    CHECK(defaults.immunities.enabled);
    CHECK(defaults.immunities.style == ImmunityDisplayStyle::ColoredI);
    CHECK(defaults.immunities.indicatorSize
        == DefaultImmunityIndicatorSize);
    CHECK(defaults.immunities.haloThickness
        == DefaultImmunityHaloThickness);
    CHECK(ColorToHex(defaults.immunities.physical) == "#D8C39AFF");
    CHECK(defaults.navigation.lineThickness
        == DefaultNavigationLineThickness);
    CHECK(defaults.navigation.waypoint.enabled);
    CHECK(ColorToHex(defaults.navigation.waypoint.color) == "#3D8BFFFF");
    CHECK(defaults.navigation.progression.enabled);
    CHECK(ColorToHex(defaults.navigation.progression.color) == "#57E03DFF");
    CHECK(defaults.navigation.quests.enabled);
    CHECK(ColorToHex(defaults.navigation.quests.color) == "#FF3B30FF");
    CHECK(!defaults.navigation.customLevels.enabled);
    CHECK(ColorToHex(defaults.navigation.customLevels.color) == "#C75CFFFF");
    CHECK(defaults.navigation.customLevels.targets.empty());
    CHECK(!defaults.hud.mercenaryHealth);
    CHECK(!defaults.hud.sessionTimer);
    CHECK(!defaults.hud.experienceTracker);
    CHECK(defaults.menu.showLauncher);
    CHECK(!defaults.menu.startExpanded);
    CHECK(defaults.menu.rememberPosition);
    CHECK(defaults.menu.positionX == 0.86F);
    CHECK(defaults.menu.positionY == 0.04F);

    const auto schema4GreyMigration = ParseConfig(R"toml(
schema_version = 4
[immunities]
enabled = false
physical = "#C7C7C7FF"
)toml");
    CHECK(!schema4GreyMigration.immunities.enabled);
    CHECK(schema4GreyMigration.immunities.style
        == ImmunityDisplayStyle::ColoredI);
    CHECK(ColorToHex(schema4GreyMigration.immunities.physical)
        == "#D8C39AFF");

    const auto schema4CustomPhysical = ParseConfig(R"toml(
schema_version = 4
[immunities]
physical = "#ABCDEFCC"
)toml");
    CHECK(ColorToHex(schema4CustomPhysical.immunities.physical)
        == "#ABCDEFCC");

    const auto schema3Migration = ParseConfig(R"toml(
schema_version = 3
[general]
enabled = false
[overlay]
opacity = 0.75
scale = 1.25
frame_rate = 90
[monsters]
detection_radius = 420
[monsters.normal]
color = "#EDEDEDFF"
size = 16
[monsters.minion]
color = "#FFD43BCC"
size = 17
[monsters.champion]
color = "#3D8BFFFF"
size = 19
[monsters.unique]
color = "#FF8A24FF"
size = 23
[monsters.super_unique_boss]
color = "#FF3B30FF"
size = 27
[immunities]
fire = "#FF2200CC"
[hud]
session_timer = true
[menu]
show_launcher = false
start_expanded = true
remember_position = false
position_x = 0.23
position_y = 0.81
[diagnostics]
enabled = true
)toml");
    CHECK(!schema3Migration.enabled);
    CHECK(schema3Migration.diagnostics);
    CHECK(schema3Migration.overlay.opacity == 0.75F);
    CHECK(schema3Migration.overlay.scale == 1.25F);
    CHECK(schema3Migration.overlay.frameRate == 90);
    CHECK(schema3Migration.monsters.normal.shape == MonsterMarkerShape::X);
    CHECK(schema3Migration.monsters.minion.shape == MonsterMarkerShape::X);
    CHECK(schema3Migration.monsters.champion.shape == MonsterMarkerShape::X);
    CHECK(schema3Migration.monsters.unique.shape == MonsterMarkerShape::X);
    CHECK(schema3Migration.monsters.superUniqueBoss.shape
        == MonsterMarkerShape::X);
    CHECK(schema3Migration.monsters.normal.size == 16.0F);
    CHECK(schema3Migration.monsters.minion.size == 17.0F);
    CHECK(schema3Migration.monsters.champion.size == 19.0F);
    CHECK(schema3Migration.monsters.unique.size == 23.0F);
    CHECK(schema3Migration.monsters.superUniqueBoss.size == 27.0F);
    CHECK(schema3Migration.monsters.normal.thickness == 2.0F);
    CHECK(schema3Migration.monsters.minion.thickness == 2.0F);
    CHECK(schema3Migration.monsters.champion.thickness == 2.0F);
    CHECK(schema3Migration.monsters.unique.thickness == 2.0F);
    CHECK(schema3Migration.monsters.superUniqueBoss.thickness == 2.0F);
    CHECK(ColorToHex(schema3Migration.monsters.normal.color) == "#EDEDEDFF");
    CHECK(ColorToHex(schema3Migration.monsters.minion.color) == "#FFD43BCC");
    CHECK(ColorToHex(schema3Migration.monsters.champion.color)
        == "#3D8BFFFF");
    CHECK(ColorToHex(schema3Migration.monsters.unique.color) == "#FF8A24FF");
    CHECK(ColorToHex(schema3Migration.monsters.superUniqueBoss.color)
        == "#FF3B30FF");
    CHECK(schema3Migration.immunities.enabled);
    CHECK(schema3Migration.immunities.style
        == ImmunityDisplayStyle::ColoredI);
    CHECK(ColorToHex(schema3Migration.immunities.physical)
        == "#D8C39AFF");
    CHECK(ColorToHex(schema3Migration.immunities.fire) == "#FF2200CC");
    CHECK(schema3Migration.hud.sessionTimer);
    CHECK(!schema3Migration.menu.showLauncher);
    CHECK(schema3Migration.menu.startExpanded);
    CHECK(!schema3Migration.menu.rememberPosition);
    CHECK(schema3Migration.menu.positionX == 0.23F);
    CHECK(schema3Migration.menu.positionY == 0.81F);

    const auto legacySchema1 = ParseConfig(R"toml(
schema_version = 1
[monsters]
normal = false
minion = false
champion = false
unique = false
super_unique = false
marker_size = 13
[immunities]
enabled = false
physical = "#C7C7C7FF"
)toml");
    CHECK(legacySchema1.monsters.normal.shape == MonsterMarkerShape::X);
    CHECK(legacySchema1.monsters.minion.shape == MonsterMarkerShape::X);
    CHECK(legacySchema1.monsters.champion.shape == MonsterMarkerShape::X);
    CHECK(legacySchema1.monsters.unique.shape == MonsterMarkerShape::X);
    CHECK(legacySchema1.monsters.superUniqueBoss.shape == MonsterMarkerShape::X);
    CHECK(legacySchema1.monsters.normal.size == 13.0F);
    CHECK(legacySchema1.monsters.minion.size == 13.0F);
    CHECK(legacySchema1.monsters.champion.size == 13.0F);
    CHECK(legacySchema1.monsters.unique.size == 13.0F);
    CHECK(legacySchema1.monsters.superUniqueBoss.size == 13.0F);
    CHECK(!legacySchema1.immunities.enabled);
    CHECK(legacySchema1.immunities.style
        == ImmunityDisplayStyle::ColoredI);
    CHECK(ColorToHex(legacySchema1.immunities.physical) == "#D8C39AFF");

    const auto legacyRuntime = ParseConfig(R"toml(
schema_version = 2
[overlay]
start_menu_open = true
opacity = 0.90
[monsters]
normal = false
minion = false
champion = false
unique = false
super_unique = false
marker_size = 14
[hud]
mercenary_health = true
session_timer = true
experience_tracker = true
show_with_automap_only = true
)toml");
    CHECK(legacyRuntime.overlay.startMenuOpen);
    CHECK(legacyRuntime.overlay.opacity == 0.90F);
    CHECK(legacyRuntime.monsters.normal.shape == MonsterMarkerShape::X);
    CHECK(legacyRuntime.monsters.minion.shape == MonsterMarkerShape::X);
    CHECK(legacyRuntime.monsters.champion.shape == MonsterMarkerShape::X);
    CHECK(legacyRuntime.monsters.unique.shape == MonsterMarkerShape::X);
    CHECK(legacyRuntime.monsters.superUniqueBoss.shape
        == MonsterMarkerShape::X);
    CHECK(legacyRuntime.monsters.normal.size == 14.0F);
    CHECK(legacyRuntime.monsters.minion.size == 14.0F);
    CHECK(legacyRuntime.monsters.champion.size == 14.0F);
    CHECK(legacyRuntime.monsters.unique.size == 14.0F);
    CHECK(legacyRuntime.monsters.superUniqueBoss.size == 14.0F);
    CHECK(legacyRuntime.hud.mercenaryHealth);
    CHECK(legacyRuntime.hud.sessionTimer);
    CHECK(legacyRuntime.hud.experienceTracker);
    CHECK(legacyRuntime.hud.showWithAutomapOnly);
    CHECK(legacyRuntime.immunities.style
        == ImmunityDisplayStyle::ColoredI);
    CHECK(ColorToHex(legacyRuntime.immunities.physical) == "#D8C39AFF");

    CHECK(Throws([] { ParseConfig(""); }));
    CHECK(Throws([] { ParseConfig("schema_version = 10"); }));
    CHECK(Throws([] { ParseConfig("schema_version = true"); }));
    CHECK(Throws([] {
        ParseConfig(
            "schema_version = 5\n[navigation]\nline_thickness = 2");
    }));
    CHECK(Throws([] {
        ParseConfig("schema_version = 1\nunknown = true");
    }));
    CHECK(Throws([] {
        ParseConfig("schema_version = 1\n[general]\nenabled = 1");
    }));
    CHECK(Throws([] {
        ParseConfig("schema_version = 1\ngeneral = true");
    }));
    CHECK(Throws([] {
        ParseConfig("schema_version = 1\n[diagnostics]\nverbose = true");
    }));
    CHECK(Throws([] {
        ParseConfig("schema_version = 3\n[overlay]\nopacity = 1.5");
    }));
    CHECK(Throws([] {
        ParseConfig("schema_version = 3\n[overlay]\nframe_rate = 12");
    }));
    CHECK(Throws([] {
        ParseConfig("schema_version = 3\n[immunities]\nfire = \"red\"");
    }));
    CHECK(Throws([] {
        ParseConfig(
            "schema_version = 5\n[immunities]\nstyle = \"unknown\"");
    }));
    CHECK(Throws([] {
        ParseConfig("schema_version = 5\n[immunities]\nstyle = 1");
    }));
    CHECK(Throws([] {
        ParseConfig(
            "schema_version = 5\n[immunities]\nindicator_size = 7");
    }));
    CHECK(Throws([] {
        ParseConfig(
            "schema_version = 5\n[immunities]\nindicator_size = 33");
    }));
    CHECK(Throws([] {
        ParseConfig(
            "schema_version = 5\n[immunities]\nhalo_thickness = 0.5");
    }));
    CHECK(Throws([] {
        ParseConfig(
            "schema_version = 5\n[immunities]\nhalo_thickness = 6.5");
    }));
    CHECK(Throws([] {
        ParseConfig(
            "schema_version = 6\n[navigation]\nline_thickness = 0.5");
    }));
    CHECK(Throws([] {
        ParseConfig(
            "schema_version = 6\n[navigation]\nline_thickness = 8.5");
    }));
    const auto minimumNavigationThickness = ParseConfig(
        "schema_version = 6\n[navigation]\nline_thickness = 1");
    CHECK(minimumNavigationThickness.navigation.lineThickness == 1.0F);
    const auto maximumNavigationThickness = ParseConfig(
        "schema_version = 6\n[navigation]\nline_thickness = 8");
    CHECK(maximumNavigationThickness.navigation.lineThickness == 8.0F);
    CHECK(Throws([] {
        ParseConfig(
            "schema_version = 6\n[navigation]\nunknown = true");
    }));
    CHECK(Throws([] {
        ParseConfig(
            "schema_version = 6\n[navigation.waypoint]\nunknown = true");
    }));
    CHECK(Throws([] {
        ParseConfig(
            "schema_version = 6\n[navigation.custom_levels]\n"
            "targets = 12");
    }));
    CHECK(Throws([] {
        ParseConfig(
            "schema_version = 6\n[navigation.custom_levels]\n"
            "targets = [12]");
    }));
    CHECK(Throws([] {
        ParseConfig(
            "schema_version = 6\n[navigation.custom_levels]\n"
            "targets = [{}]");
    }));
    CHECK(Throws([] {
        ParseConfig(
            "schema_version = 6\n[navigation.custom_levels]\n"
            "targets = [{ level_id = 12, level_name = \"Pit Level 1\" }]");
    }));
    CHECK(Throws([] {
        ParseConfig(
            "schema_version = 6\n[navigation.custom_levels]\n"
            "targets = [{ level_id = \"12\" }]");
    }));
    CHECK(Throws([] {
        ParseConfig(
            "schema_version = 6\n[navigation.custom_levels]\n"
            "targets = [{ level_id = 0 }]");
    }));
    CHECK(Throws([] {
        ParseConfig(
            "schema_version = 6\n[navigation.custom_levels]\n"
            "targets = [{ level_id = 2147483648 }]");
    }));
    CHECK(Throws([] {
        ParseConfig(
            "schema_version = 6\n[navigation.custom_levels]\n"
            "targets = [{ level_id = 65536 }]");
    }));
    CHECK(Throws([] {
        ParseConfig(
            "schema_version = 6\n[navigation.custom_levels]\n"
            "targets = [{ level_name = 12 }]");
    }));
    CHECK(Throws([] {
        ParseConfig(
            "schema_version = 6\n[navigation.custom_levels]\n"
            "targets = [{ level_name = \"\" }]");
    }));
    CHECK(Throws([] {
        ParseConfig(
            "schema_version = 6\n[navigation.custom_levels]\n"
            "targets = [{ level_name = \" Mausoleum\" }]");
    }));
    CHECK(Throws([] {
        ParseConfig(
            "schema_version = 6\n[navigation.custom_levels]\n"
            "targets = [{ level_name = \"Mausoleum\\n\" }]");
    }));
    CHECK(Throws([] {
        ParseConfig(
            "schema_version = 6\n[navigation.custom_levels]\n"
            "targets = [{ level_name = \"Mausoleum\", extra = true }]");
    }));
    CHECK(Throws([] {
        ParseConfig(
            "schema_version = 6\n[navigation.custom_levels]\n"
            "targets = [{ level_id = 12 }, { level_id = 12 }]");
    }));
    CHECK(Throws([] {
        ParseConfig(
            "schema_version = 6\n[navigation.custom_levels]\n"
            "targets = [{ level_name = \"Mausoleum\" }, "
            "{ level_name = \"Mausoleum\" }]");
    }));
    const auto minimumImmunityControls = ParseConfig(
        "schema_version = 5\n[immunities]\n"
        "indicator_size = 8\nhalo_thickness = 1");
    CHECK(minimumImmunityControls.immunities.indicatorSize == 8.0F);
    CHECK(minimumImmunityControls.immunities.haloThickness == 1.0F);
    const auto maximumImmunityControls = ParseConfig(
        "schema_version = 5\n[immunities]\n"
        "indicator_size = 32\nhalo_thickness = 6");
    CHECK(maximumImmunityControls.immunities.indicatorSize == 32.0F);
    CHECK(maximumImmunityControls.immunities.haloThickness == 6.0F);
    CHECK(Throws([] {
        ParseConfig("schema_version = 3\n[menu]\nshow_launcher = 1");
    }));
    CHECK(Throws([] {
        ParseConfig("schema_version = 3\n[menu]\nposition_x = -0.01");
    }));
    CHECK(Throws([] {
        ParseConfig("schema_version = 3\n[menu]\nposition_y = 1.01");
    }));
    CHECK(Throws([] {
        ParseConfig("schema_version = 3\n[menu]\nposition = 0.5");
    }));
    // detection_radius was never a scan radius: it only filtered monsters
    // after D2R's complete client table had already been traversed. Preserve
    // old files by accepting the retired key regardless of its former units
    // or TOML type, then prove that the next save removes it.
    for (const auto* retiredRadius : {
            "schema_version = 3\n[monsters]\ndetection_radius = 59",
            "schema_version = 4\n[monsters]\ndetection_radius = 2501",
            "schema_version = 7\n[monsters]\ndetection_radius = 221",
            "schema_version = 8\n[monsters]\ndetection_radius = \"retired\""}) {
        const auto migratedRadius = ParseConfig(retiredRadius);
        CHECK(SerializeConfig(migratedRadius).find("detection_radius")
            == std::string::npos);
    }
    for (const auto* removedMasterThickness : {
            "schema_version = 3\n[monsters]\nmarker_thickness = 2",
            "schema_version = 8\n[monsters]\nmarker_thickness = 2",
            "schema_version = 9\n[monsters]\nmarker_thickness = 2"}) {
        CHECK(Throws([removedMasterThickness] {
            ParseConfig(removedMasterThickness);
        }));
    }
    const auto markerThicknessBoundaries = ParseConfig(R"toml(
schema_version = 9
[monsters.normal]
shape = "x"
thickness = 1
[monsters.unique]
shape = "player_cross"
thickness = 5
)toml");
    CHECK(markerThicknessBoundaries.monsters.normal.thickness == 1.0F);
    CHECK(markerThicknessBoundaries.monsters.unique.thickness == 5.0F);
    CHECK(Throws([] {
        ParseConfig(
            "schema_version = 9\n[monsters.normal]\n"
            "shape = \"x\"\nthickness = 0.5");
    }));
    CHECK(Throws([] {
        ParseConfig(
            "schema_version = 9\n[monsters.normal]\n"
            "shape = \"player_cross\"\nthickness = 5.5");
    }));
    CHECK(Throws([] {
        ParseConfig(
            "schema_version = 9\n[monsters.normal]\n"
            "shape = \"dot\"\nthickness = 2");
    }));
    CHECK(Throws([] {
        ParseConfig(
            "schema_version = 3\n[monsters.normal]\nsize = 41");
    }));
    CHECK(Throws([] {
        ParseConfig(
            "schema_version = 4\n[monsters]\nnormal = false");
    }));
    CHECK(Throws([] {
        ParseConfig(
            "schema_version = 4\n[monsters.normal]\nshape = \"unknown\"");
    }));
    CHECK(Throws([] {
        ParseConfig(
            "schema_version = 4\n[monsters.normal]\nshape = 1");
    }));

    const auto normal = MakeMonsterMarker(MonsterRank::Normal, Vec2{10.0F, 20.0F});
    const auto minion = MakeMonsterMarker(MonsterRank::Minion, Vec2{10.0F, 20.0F});
    const auto champion = MakeMonsterMarker(MonsterRank::Champion, Vec2{10.0F, 20.0F});
    const auto unique = MakeMonsterMarker(MonsterRank::Unique, Vec2{10.0F, 20.0F});
    const auto superUnique = MakeMonsterMarker(MonsterRank::SuperUnique, Vec2{10.0F, 20.0F});
    CHECK(BuildMonsterOutline(normal).size() == 24U);
    CHECK(BuildMonsterOutline(minion).size() == 3U);
    CHECK(BuildMonsterOutline(champion).size() == 4U);
    CHECK(BuildMonsterOutline(unique).size() == 10U);
    CHECK(BuildMonsterOutline(superUnique).size() == 6U);
    CHECK(normal.stroke == ScenePalette::MonsterNormal);
    CHECK(superUnique.stroke == ScenePalette::MonsterSuperUnique);
    CHECK(normal.fill.alpha == 64U);

    const Vec2 crossCenter{10.0F, 20.0F};
    const auto cross = BuildCrossOutline(crossCenter, 8.0F);
    static_assert(std::tuple_size_v<decltype(cross)>
        == CrossOutlinePointCount);
    CHECK(cross.size() == 13U);
    CHECK(cross.front() == cross.back());
    for (std::size_t index = 1U; index < cross.size(); ++index) {
        CHECK(cross[index - 1U] != cross[index]);
        CHECK(std::isfinite(cross[index].x));
        CHECK(std::isfinite(cross[index].y));
    }
    for (std::size_t index = 0U; index < 6U; ++index) {
        const auto& opposite = cross[index + 6U];
        CHECK(std::abs((cross[index].x + opposite.x)
            - (2.0F * crossCenter.x)) < 0.0001F);
        CHECK(std::abs((cross[index].y + opposite.y)
            - (2.0F * crossCenter.y)) < 0.0001F);
    }

    const auto playerCross = BuildPlayerCrossOutline(crossCenter, 20.0F);
    static_assert(std::tuple_size_v<decltype(playerCross)>
        == CrossOutlinePointCount);
    constexpr CrossOutline expectedPlayerCross{{
        Vec2{0.0F, 18.0F},
        Vec2{6.0F, 15.0F},
        Vec2{10.0F, 18.0F},
        Vec2{14.0F, 15.0F},
        Vec2{20.0F, 18.0F},
        Vec2{14.0F, 20.0F},
        Vec2{20.0F, 22.0F},
        Vec2{14.0F, 25.0F},
        Vec2{10.0F, 22.0F},
        Vec2{6.0F, 25.0F},
        Vec2{0.0F, 22.0F},
        Vec2{6.0F, 20.0F},
        Vec2{0.0F, 18.0F},
    }};
    CHECK(playerCross == expectedPlayerCross);

    constexpr std::array immunities{
        Element::Physical,
        Element::Fire,
        Element::Cold,
        Element::Lightning,
    };
    const auto immunityRing = MakeImmunityRing(
        Vec2{50.0F, 75.0F},
        8.0F,
        11.0F,
        immunities,
        0.1F);
    CHECK(immunityRing.arcs.size() == immunities.size());
    CHECK(immunityRing.arcs[0].color == ScenePalette::Physical);
    CHECK(immunityRing.arcs[1].color == ScenePalette::Fire);
    CHECK(immunityRing.arcs[2].color == ScenePalette::Cold);
    CHECK(immunityRing.arcs[3].color == ScenePalette::Lightning);
    CHECK(immunityRing.arcs[0].endRadians > immunityRing.arcs[0].startRadians);

    const auto fireMissile = MakeMissileMarker(
        Element::Fire,
        Vec2{20.0F, 30.0F},
        Vec2{1.0F, -0.25F},
        18.0F,
        3.0F);
    CHECK(fireMissile.color == ScenePalette::Fire);
    CHECK(fireMissile.length == 18.0F);
    CHECK(fireMissile.radius == 3.0F);

    const auto preview = BuildDiagnosticPreview(1280U, 720U, 42U);
    CHECK(preview.sequence == 42U);
    CHECK(preview.viewportWidth == 1280U);
    CHECK(preview.viewportHeight == 720U);
    CHECK(preview.primitives.size() == 13U);
    std::array<std::size_t, std::variant_size_v<Primitive>> primitiveCounts{};
    for (const auto& primitive : preview.primitives) {
        ++primitiveCounts[primitive.index()];
        Validate(primitive);
    }
    CHECK(primitiveCounts[0] == 1U); // line
    CHECK(primitiveCounts[1] == 1U); // circle
    CHECK(primitiveCounts[2] == 1U); // polygon
    CHECK(primitiveCounts[3] == 5U); // all monster ranks
    CHECK(primitiveCounts[4] == 1U); // segmented immunity ring
    CHECK(primitiveCounts[5] == 2U); // missiles
    CHECK(primitiveCounts[6] == 2U); // labels

    SceneExchange exchange;
    CHECK(!exchange.Acquire());
    const auto published = exchange.Publish(preview);
    CHECK(published);
    CHECK(published->sequence == 42U);
    CHECK(exchange.Acquire() == published);
    const auto next = exchange.Publish(BuildDiagnosticPreview(1920U, 1080U, 43U));
    CHECK(next->sequence == 43U);
    CHECK(exchange.Acquire() == next);
    CHECK(published->sequence == 42U);
    exchange.Clear();
    CHECK(!exchange.Acquire());

    CHECK(Throws([] { static_cast<void>(BuildDiagnosticPreview(0U, 720U)); }));
    CHECK(Throws([] {
        static_cast<void>(BuildCrossOutline(Vec2{}, 0.0F));
    }));
    CHECK(Throws([] {
        static_cast<void>(BuildCrossOutline(
            Vec2{std::numeric_limits<float>::infinity(), 0.0F},
            5.0F));
    }));
    CHECK(Throws([] {
        static_cast<void>(BuildPlayerCrossOutline(Vec2{}, 0.0F));
    }));
    CHECK(Throws([] {
        static_cast<void>(BuildPlayerCrossOutline(
            Vec2{},
            std::numeric_limits<float>::infinity()));
    }));
    CHECK(Throws([] {
        static_cast<void>(BuildPlayerCrossOutline(
            Vec2{},
            std::numeric_limits<float>::quiet_NaN()));
    }));
    CHECK(Throws([] {
        static_cast<void>(BuildPlayerCrossOutline(
            Vec2{std::numeric_limits<float>::quiet_NaN(), 0.0F},
            20.0F));
    }));
    CHECK(Throws([] {
        static_cast<void>(MakeMonsterMarker(MonsterRank::Normal, Vec2{}, 0.0F));
    }));
    CHECK(Throws([] {
        static_cast<void>(MakeMissileMarker(Element::Magic, Vec2{}, Vec2{}));
    }));
    CHECK(Throws([] {
        static_cast<void>(MakeMonsterMarker(
            static_cast<MonsterRank>(255U),
            Vec2{},
            5.0F));
    }));
    CHECK(Throws([] {
        static_cast<void>(MakeMissileMarker(
            static_cast<Element>(255U),
            Vec2{},
            Vec2{1.0F, 0.0F}));
    }));
    CHECK(Throws([] {
        constexpr std::array duplicate{Element::Fire, Element::Fire};
        static_cast<void>(MakeImmunityRing(Vec2{}, 8.0F, 10.0F, duplicate));
    }));
    CHECK(Throws([] {
        constexpr std::array one{Element::Poison};
        static_cast<void>(MakeImmunityRing(Vec2{}, 10.0F, 8.0F, one));
    }));
    CHECK(Throws([] {
        SceneSnapshot invalid{
            .sequence = 1U,
            .viewportWidth = 800U,
            .viewportHeight = 600U,
            .primitives = {LinePrimitive{
                .start = Vec2{},
                .end = Vec2{std::numeric_limits<float>::infinity(), 0.0F},
            }},
        };
        Validate(invalid);
    }));

    static_assert(NativeSettingsTabs.size() == 5);
    static_assert(!MayTriggerMapSenseActionForVirtualKey(
        NativeAutomapVirtualKey));
    static_assert(MayTriggerMapSenseActionForVirtualKey(0x24U));
    CHECK(!MayTriggerMapSenseActionForVirtualKey(0x09U));
    CHECK(MayTriggerMapSenseActionForVirtualKey(0x24U));

    CHECK(std::abs(ComputeColoredImmunityIndicatorAdvance(8.0F) - 1.5F)
        < 0.0001F);
    CHECK(std::abs(ComputeColoredImmunityIndicatorAdvance(25.0F) - 3.0F)
        < 0.0001F);
    CHECK(std::abs(ComputeColoredImmunityIndicatorAdvance(32.0F) - 3.84F)
        < 0.0001F);
    CHECK(Throws([] {
        static_cast<void>(ComputeColoredImmunityIndicatorAdvance(0.0F));
    }));
    CHECK(Throws([] {
        static_cast<void>(ComputeColoredImmunityIndicatorAdvance(
            std::numeric_limits<float>::infinity()));
    }));

    static_assert(!ShouldForwardWin32MessageToImGui(WM_KEYDOWN, VK_TAB));
    static_assert(!ShouldForwardWin32MessageToImGui(WM_KEYUP, VK_TAB));
    static_assert(!ShouldForwardWin32MessageToImGui(WM_SYSKEYDOWN, VK_TAB));
    static_assert(!ShouldForwardWin32MessageToImGui(WM_SYSKEYUP, VK_TAB));
    static_assert(!ShouldForwardWin32MessageToImGui(WM_CHAR, L'\t'));
    static_assert(!ShouldForwardWin32MessageToImGui(WM_SYSCHAR, L'\t'));
    CHECK(!ShouldForwardWin32MessageToImGui(WM_KEYDOWN, VK_TAB));
    CHECK(!ShouldForwardWin32MessageToImGui(WM_KEYUP, VK_TAB));
    CHECK(!ShouldForwardWin32MessageToImGui(WM_SYSKEYDOWN, VK_TAB));
    CHECK(!ShouldForwardWin32MessageToImGui(WM_SYSKEYUP, VK_TAB));
    CHECK(!ShouldForwardWin32MessageToImGui(WM_CHAR, L'\t'));
    CHECK(!ShouldForwardWin32MessageToImGui(WM_SYSCHAR, L'\t'));
    CHECK(ShouldForwardWin32MessageToImGui(WM_KEYDOWN, VK_HOME));
    CHECK(ShouldForwardWin32MessageToImGui(WM_CHAR, L'x'));
    CHECK(ShouldForwardWin32MessageToImGui(WM_MOUSEMOVE, 0U));
    constexpr auto RepeatedKeyState = static_cast<LPARAM>(
        std::uint64_t{1} << 30U);
    static_assert(IsInitialOwnedOverlayDismissalMessage(
        WM_KEYDOWN, VK_TAB, 0));
    static_assert(IsInitialOwnedOverlayDismissalMessage(
        WM_SYSKEYDOWN, VK_TAB, 0));
    static_assert(IsInitialOwnedOverlayDismissalMessage(
        WM_KEYDOWN, VK_ESCAPE, 0));
    static_assert(IsInitialOwnedOverlayDismissalMessage(
        WM_SYSKEYDOWN, VK_ESCAPE, 0));
    static_assert(!IsInitialOwnedOverlayDismissalMessage(
        WM_KEYDOWN, VK_TAB, RepeatedKeyState));
    static_assert(!IsInitialOwnedOverlayDismissalMessage(
        WM_KEYDOWN, VK_ESCAPE, RepeatedKeyState));
    static_assert(!IsInitialOwnedOverlayDismissalMessage(
        WM_KEYUP, VK_TAB, 0));
    static_assert(!IsInitialOwnedOverlayDismissalMessage(
        WM_KEYDOWN, VK_HOME, 0));
    CHECK(IsInitialOwnedOverlayDismissalMessage(WM_KEYDOWN, VK_TAB, 0));
    CHECK(IsInitialOwnedOverlayDismissalMessage(
        WM_KEYDOWN, VK_ESCAPE, 0));
    CHECK(!IsInitialOwnedOverlayDismissalMessage(
        WM_KEYDOWN, VK_TAB, RepeatedKeyState));
    CHECK(!IsInitialOwnedOverlayDismissalMessage(
        WM_KEYDOWN, VK_ESCAPE, RepeatedKeyState));
    CHECK(!IsInitialOwnedOverlayDismissalMessage(WM_KEYUP, VK_TAB, 0));
    static_assert(!ShouldSubmitD3D12DrawData(0, 0));
    static_assert(!ShouldSubmitD3D12DrawData(1, 0));
    static_assert(!ShouldSubmitD3D12DrawData(0, 3));
    static_assert(ShouldSubmitD3D12DrawData(1, 3));
    CHECK(!ShouldSubmitD3D12DrawData(0, 0));
    CHECK(!ShouldSubmitD3D12DrawData(1, 0));
    CHECK(ShouldSubmitD3D12DrawData(1, 3));
    static_assert(NativeSettingsTabs[0].tab == NativeSettingsTab::Map);
    static_assert(NativeSettingsTabs[1].tab == NativeSettingsTab::Monsters);
    static_assert(NativeSettingsTabs[2].tab == NativeSettingsTab::Navigation);
    static_assert(NativeSettingsTabs[3].tab == NativeSettingsTab::Projectiles);
    static_assert(NativeSettingsTabs[4].tab == NativeSettingsTab::System);
    CHECK(NativeSettingsTabs[0].label == "Map");
    CHECK(NativeSettingsTabs[2].label == "Navigation");
    CHECK(NativeSettingsTabs[3].label == "Projectiles");
    CHECK(NativeSettingsTabs[0].hasPersistentSettings);
    CHECK(NativeSettingsTabs[1].hasPersistentSettings);
    CHECK(!NativeSettingsTabs[2].hasPersistentSettings);
    CHECK(!NativeSettingsTabs[3].hasPersistentSettings);
    CHECK(NativeSettingsTabs[4].hasPersistentSettings);

    Config policyConfig{};
    auto dotStyleWithHiddenThickness = policyConfig.monsters.normal;
    dotStyleWithHiddenThickness.shape = MonsterMarkerShape::Dot;
    auto sameDotStyle = dotStyleWithHiddenThickness;
    sameDotStyle.thickness = MaximumMonsterMarkerThickness;
    CHECK(SameMonsterMarkerStyle(dotStyleWithHiddenThickness, sameDotStyle));
    dotStyleWithHiddenThickness.shape = MonsterMarkerShape::X;
    sameDotStyle.shape = MonsterMarkerShape::X;
    CHECK(!SameMonsterMarkerStyle(dotStyleWithHiddenThickness, sameDotStyle));
    policyConfig.overlay.followNativeAutomap = false;
    for (const auto key : NativeSettingsToggleKeys) {
        WriteToggle(policyConfig, key, false);
        CHECK(!ReadToggle(policyConfig, key));
        WriteToggle(policyConfig, key, true);
        CHECK(ReadToggle(policyConfig, key));
        CHECK(!ToggleLabel(key).empty());
    }
    CHECK(TabForToggle(ToggleKey::MapSenseEnabled) == NativeSettingsTab::Map);
    CHECK(TabForToggle(ToggleKey::MapOverlayEnabled) == NativeSettingsTab::Map);
    CHECK(TabForToggle(ToggleKey::ImmunitiesEnabled)
        == NativeSettingsTab::Monsters);
    CHECK(TabForToggle(ToggleKey::DiagnosticPreview)
        == NativeSettingsTab::System);
    CHECK(TabForToggle(ToggleKey::DiagnosticsEnabled) == NativeSettingsTab::System);
    CHECK(!policyConfig.overlay.followNativeAutomap);

    SetOverlayOpacity(policyConfig, OverlayOpacityChoice::Low);
    CHECK(policyConfig.overlay.opacity == 0.25F);
    SetOverlayOpacity(policyConfig, OverlayOpacityChoice::Opaque);
    CHECK(policyConfig.overlay.opacity == 1.00F);
    CHECK(NearestOverlayOpacityChoice(0.88F)
        == OverlayOpacityChoice::NearOpaque);
    CHECK(NearestOverlayOpacityChoice(0.625F)
        == OverlayOpacityChoice::Medium);
    CHECK(NearestOverlayOpacityChoice(
        std::numeric_limits<float>::infinity())
        == OverlayOpacityChoice::NearOpaque);
    CHECK(OverlayOpacityLabel(OverlayOpacityChoice::NearOpaque) == "90%");

    static_assert(ImmunityDisplayModes.size() == 3);
    WriteImmunityDisplayMode(
        policyConfig,
        ImmunityDisplayMode::SplitHalo);
    CHECK(policyConfig.immunities.enabled);
    CHECK(policyConfig.immunities.style
        == ImmunityDisplayStyle::SplitHalo);
    CHECK(ReadImmunityDisplayMode(policyConfig)
        == ImmunityDisplayMode::SplitHalo);
    WriteImmunityDisplayMode(policyConfig, ImmunityDisplayMode::Off);
    CHECK(!policyConfig.immunities.enabled);
    CHECK(policyConfig.immunities.style
        == ImmunityDisplayStyle::SplitHalo);
    CHECK(ReadImmunityDisplayMode(policyConfig) == ImmunityDisplayMode::Off);
    WriteImmunityDisplayMode(
        policyConfig,
        ImmunityDisplayMode::ColoredI);
    CHECK(policyConfig.immunities.enabled);
    CHECK(policyConfig.immunities.style
        == ImmunityDisplayStyle::ColoredI);
    CHECK(ReadImmunityDisplayMode(policyConfig)
        == ImmunityDisplayMode::ColoredI);
    CHECK(ImmunityDisplayModeLabel(ImmunityDisplayMode::ColoredI)
        == "Colored i");
    CHECK(ImmunityDisplayModeLabel(ImmunityDisplayMode::SplitHalo)
        == "Split halo");

    ApplyImmunityPalette(policyConfig, ImmunityPalette::Classic);
    CHECK(DetectImmunityPalette(policyConfig) == ImmunityPalette::Classic);
    CHECK(ColorToHex(policyConfig.immunities.physical) == "#D8C39AFF");
    const auto classicFire = policyConfig.immunities.fire;
    ApplyImmunityPalette(policyConfig, ImmunityPalette::HighContrast);
    CHECK(DetectImmunityPalette(policyConfig)
        == ImmunityPalette::HighContrast);
    CHECK(!SameColor(policyConfig.immunities.fire, classicFire));
    ApplyImmunityPalette(policyConfig, ImmunityPalette::ColorBlindSafe);
    CHECK(DetectImmunityPalette(policyConfig)
        == ImmunityPalette::ColorBlindSafe);
    CHECK(ImmunityPaletteLabel(ImmunityPalette::ColorBlindSafe)
        == "Color-blind safe");
    policyConfig.immunities.magic.red = 0.123F;
    CHECK(!DetectImmunityPalette(policyConfig).has_value());

    const auto persistedDefaults = ParseConfig(SerializeConfig(Config{}));
    CHECK(DetectImmunityPalette(persistedDefaults)
        == ImmunityPalette::Classic);

    Config appliedSettings{};
    appliedSettings.enabled = false;
    appliedSettings.diagnostics = true;
    appliedSettings.overlay.enabled = false;
    appliedSettings.overlay.diagnosticPreview = true;
    appliedSettings.overlay.opacity = 0.50F;
    appliedSettings.overlay.scale = 1.25F;
    appliedSettings.overlay.frameRate = 37;
    appliedSettings.overlay.followNativeAutomap = false;
    appliedSettings.monsters.normal.thickness = 3.0F;
    appliedSettings.monsters.normal.color = ParseColor("#ABCDEFCC");
    appliedSettings.monsters.normal.size = 14.0F;
    appliedSettings.hud.sessionTimer = true;
    appliedSettings.menu.showLauncher = false;
    ApplyImmunityPalette(appliedSettings, ImmunityPalette::HighContrast);

    NativeSettingsDraft draftModel(appliedSettings);
    CHECK(!draftModel.Dirty());
    WriteToggle(
        draftModel.Draft(),
        ToggleKey::MapSenseEnabled,
        true);
    SetOverlayOpacity(
        draftModel.Draft(),
        OverlayOpacityChoice::Opaque);
    CHECK(draftModel.Dirty());
    draftModel.Discard();
    CHECK(!draftModel.Dirty());
    CHECK(!draftModel.Draft().enabled);
    CHECK(draftModel.Draft().overlay.opacity == 0.50F);
    CHECK(draftModel.Draft().monsters.normal.thickness == 3.0F);
    CHECK(draftModel.Draft().monsters.normal.size == 14.0F);
    CHECK(ColorToHex(draftModel.Draft().monsters.normal.color)
        == "#ABCDEFCC");

    draftModel.Draft().immunities.style = ImmunityDisplayStyle::SplitHalo;
    CHECK(draftModel.Dirty());
    draftModel.Discard();
    CHECK(!draftModel.Dirty());
    draftModel.Draft().immunities.indicatorSize = 21.0F;
    CHECK(draftModel.Dirty());
    draftModel.Discard();
    draftModel.Draft().immunities.haloThickness = 4.0F;
    CHECK(draftModel.Dirty());
    draftModel.Discard();
    CHECK(!draftModel.Dirty());

    draftModel.ResetToDefaults();
    CHECK(draftModel.Dirty());
    CHECK(draftModel.Draft().enabled);
    CHECK(draftModel.Draft().overlay.opacity == 1.0F);
    CHECK(draftModel.Draft().overlay.scale == 1.0F);
    CHECK(draftModel.Draft().overlay.enabled);
    CHECK(!draftModel.Draft().overlay.diagnosticPreview);
    CHECK(draftModel.Draft().monsters.normal.thickness == 2.0F);
    CHECK(draftModel.Draft().monsters.normal.shape
        == MonsterMarkerShape::PlayerCross);
    CHECK(draftModel.Draft().monsters.minion.shape
        == MonsterMarkerShape::PlayerCross);
    CHECK(draftModel.Draft().monsters.champion.shape
        == MonsterMarkerShape::PlayerCross);
    CHECK(draftModel.Draft().monsters.unique.shape
        == MonsterMarkerShape::PlayerCross);
    CHECK(draftModel.Draft().monsters.superUniqueBoss.shape
        == MonsterMarkerShape::PlayerCross);
    CHECK(draftModel.Draft().monsters.normal.size == 18.0F);
    CHECK(draftModel.Draft().monsters.minion.size == 18.0F);
    CHECK(draftModel.Draft().monsters.champion.size == 20.0F);
    CHECK(draftModel.Draft().monsters.unique.size == 22.0F);
    CHECK(draftModel.Draft().monsters.superUniqueBoss.size == 24.0F);
    CHECK(draftModel.Draft().immunities.style
        == ImmunityDisplayStyle::ColoredI);
    CHECK(draftModel.Draft().immunities.indicatorSize
        == DefaultImmunityIndicatorSize);
    CHECK(draftModel.Draft().immunities.haloThickness
        == DefaultImmunityHaloThickness);
    CHECK(ColorToHex(draftModel.Draft().immunities.physical)
        == "#D8C39AFF");
    CHECK(draftModel.Draft().overlay.frameRate == 37);
    CHECK(!draftModel.Draft().overlay.followNativeAutomap);
    CHECK(draftModel.Draft().hud.sessionTimer);
    CHECK(!draftModel.Draft().menu.showLauncher);

    const auto& newlyApplied = draftModel.Apply();
    CHECK(!draftModel.Dirty());
    CHECK(newlyApplied.enabled);
    CHECK(newlyApplied.overlay.opacity == 1.0F);
    CHECK(newlyApplied.overlay.scale == 1.0F);
    CHECK(newlyApplied.overlay.enabled);
    CHECK(!newlyApplied.overlay.diagnosticPreview);
    CHECK(newlyApplied.overlay.frameRate == 37);
    CHECK(!newlyApplied.overlay.followNativeAutomap);
    CHECK(newlyApplied.monsters.normal.thickness == 2.0F);
    CHECK(newlyApplied.monsters.normal.size == 18.0F);
    CHECK(newlyApplied.hud.sessionTimer);
    CHECK(!newlyApplied.menu.showLauncher);

    if (argc >= 2) {
        try {
            const auto shippedText = ReadFile(argv[1]);
            const auto shippedDocument = toml::parse(shippedText);
            const auto shippedSchema = shippedDocument["schema_version"]
                .value<std::int64_t>();
            CHECK(shippedSchema.has_value());
            CHECK(shippedSchema.value_or(0) == CurrentConfigSchemaVersion);
            const auto shipped = ParseConfig(shippedDocument);
            CHECK(shipped.enabled);
            CHECK(!shipped.diagnostics);
            CHECK(shipped.overlay.opacity == 1.0F);
            CHECK(shippedText.find("detection_radius") == std::string::npos);
            CHECK(shippedText.find("marker_thickness") == std::string::npos);
            CHECK(shipped.monsters.normal.shape
                == MonsterMarkerShape::PlayerCross);
            CHECK(shipped.monsters.minion.shape
                == MonsterMarkerShape::PlayerCross);
            CHECK(shipped.monsters.champion.shape
                == MonsterMarkerShape::PlayerCross);
            CHECK(shipped.monsters.unique.shape
                == MonsterMarkerShape::PlayerCross);
            CHECK(shipped.monsters.superUniqueBoss.shape
                == MonsterMarkerShape::PlayerCross);
            CHECK(shipped.monsters.normal.size == 18.0F);
            CHECK(shipped.monsters.minion.size == 18.0F);
            CHECK(shipped.monsters.champion.size == 20.0F);
            CHECK(shipped.monsters.unique.size == 22.0F);
            CHECK(shipped.monsters.superUniqueBoss.size == 24.0F);
            CHECK(shipped.monsters.normal.thickness == 2.0F);
            CHECK(shipped.monsters.minion.thickness == 2.0F);
            CHECK(shipped.monsters.champion.thickness == 2.0F);
            CHECK(shipped.monsters.unique.thickness == 2.0F);
            CHECK(shipped.monsters.superUniqueBoss.thickness == 2.0F);
            CHECK(ColorToHex(shipped.monsters.normal.color) == "#FFFFFFFF");
            CHECK(ColorToHex(shipped.monsters.minion.color) == "#FFD43BFF");
            CHECK(ColorToHex(shipped.monsters.champion.color) == "#3D8BFFFF");
            CHECK(ColorToHex(shipped.monsters.unique.color) == "#FF8A24FF");
            CHECK(ColorToHex(shipped.monsters.superUniqueBoss.color)
                == "#FF3B30FF");
            CHECK(shipped.immunities.enabled);
            CHECK(shipped.immunities.style
                == ImmunityDisplayStyle::ColoredI);
            CHECK(shipped.immunities.indicatorSize
                == DefaultImmunityIndicatorSize);
            CHECK(shipped.immunities.haloThickness
                == DefaultImmunityHaloThickness);
            CHECK(ColorToHex(shipped.immunities.physical)
                == "#D8C39AFF");
            CHECK(shipped.navigation.lineThickness == 2.0F);
            CHECK(shipped.navigation.waypoint.enabled);
            CHECK(ColorToHex(shipped.navigation.waypoint.color)
                == "#3D8BFFFF");
            CHECK(shipped.navigation.progression.enabled);
            CHECK(ColorToHex(shipped.navigation.progression.color)
                == "#57E03DFF");
            CHECK(shipped.navigation.quests.enabled);
            CHECK(ColorToHex(shipped.navigation.quests.color)
                == "#FF3B30FF");
            CHECK(!shipped.navigation.customLevels.enabled);
            CHECK(ColorToHex(shipped.navigation.customLevels.color)
                == "#C75CFFFF");
            CHECK(shipped.navigation.customLevels.targets.size() == 4U);
            CHECK(std::get<std::int32_t>(
                shipped.navigation.customLevels.targets[0]) == 12);
            CHECK(std::get<std::string>(
                shipped.navigation.customLevels.targets[1])
                == "Mausoleum");
            CHECK(std::get<std::string>(
                shipped.navigation.customLevels.targets[2])
                == "Ancient Tunnels");
            CHECK(std::get<std::int32_t>(
                shipped.navigation.customLevels.targets[3]) == 119);
            CHECK(shipped.menu.showLauncher);
            CHECK(!shipped.menu.startExpanded);
            CHECK(shipped.menu.rememberPosition);
            CHECK(shipped.menu.positionX == 0.86F);
            CHECK(shipped.menu.positionY == 0.04F);
            CHECK(!shipped.hud.mercenaryHealth);
            CHECK(!shipped.hud.sessionTimer);
            CHECK(!shipped.hud.experienceTracker);
        } catch (const std::exception& exception) {
            std::cerr << "FAIL shipped configuration: " << exception.what() << '\n';
            ++Failures;
        }
    }
    return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
