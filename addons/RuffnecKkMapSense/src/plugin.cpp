#include <D2RLPlugin/api.h>

#include <Windows.h>

#include "d3d12_imgui_host.hpp"
#include "imgui_settings_panel.hpp"
#include "mapsense_config.hpp"
#include "navigation_engine.hpp"
#include "navigation_resolver.hpp"
#include "native_automap_marker.hpp"
#include "native_settings_policy.hpp"
#include "overlay_host_api.hpp"
#include "reveal_engine.hpp"
#include "sfilllocation_diagnostic.hpp"

#include <imgui.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <exception>
#include <cmath>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace RuffnecKk::MapSense {
namespace {

constexpr std::size_t MaximumConfigBytes = 65'536;
constexpr std::int32_t UnknownRevealLevelId = -1;
constexpr std::uint32_t RevealReplayRetryLimit = 8U;
constexpr std::uint32_t RevealReplayRetryDelayMilliseconds = 250U;
constexpr std::uint32_t NavigationRefreshRetryLimit = 8U;
constexpr std::uint32_t NavigationRefreshRetryDelayMilliseconds = 250U;
constexpr std::uint64_t DynamicNavigationRefreshIntervalMilliseconds = 1'000U;

enum class Action {
    RevealZone,
    RevealAct,
    ToggleRevealAll,
    DisableRevealAll,
    ToggleMenu,
};

std::array ActionTokens{
    Action::RevealZone,
    Action::RevealAct,
    Action::ToggleRevealAll,
    Action::ToggleMenu,
};

const D2RL::PluginContext* Context{};
const D2RL::InputServiceV1* InputService{};
const D2RL::LifecycleServiceV1* LifecycleService{};
const D2RL::ThreadServiceV1* ThreadService{};
Config Settings{};
std::atomic_bool Operational{};
std::atomic_bool HostApiAvailable{};
std::atomic_bool GameplayReady{};
std::atomic_bool MenuExpanded{};
std::atomic_bool MarkerAvailable{};
std::atomic_uint64_t SessionEpoch{1};
std::atomic_uint64_t CurrentSessionGeneration{};
std::atomic_uint64_t LastDynamicNavigationRefreshTick{};
std::vector<NavigationLineSnapshot> NavigationLineSnapshots;
std::vector<NativeAutomapMarkerSnapshot> MarkerSnapshots;
std::array<D2RL::Input::ActionHandle, ActionTokens.size()> ActionHandles{};
std::array<D2RL::Lifecycle::ListenerHandle, 5> LifecycleHandles{};

struct QueuedAction {
    Action action{};
    std::uint64_t sessionEpoch{};
};

constexpr std::size_t ActionQueueCapacity = 16;
std::array<QueuedAction, ActionQueueCapacity> ActionQueue{};
std::size_t ActionQueueHead{};
std::size_t ActionQueueSize{};
bool ActionDrainScheduled{};
std::atomic_flag ActionQueueLock = ATOMIC_FLAG_INIT;
HANDLE HostRetryStopEvent{};
HANDLE HostRetryWorker{};
std::mutex ConfigSaveMutex;
std::string PendingConfigSave;
bool ConfigSaveDrainScheduled{};
std::mutex HostUiTaskMutex;
D3D12ImGuiUiTaskCallback PendingHostUiTask{};
void* PendingHostUiTaskData{};
bool HostUiTaskScheduled{};

struct NavigationRefreshState {
    std::uint64_t sessionGeneration{};
    std::int32_t levelId{UnknownNavigationLevelId};
    std::uint32_t retriesRemaining{};
    bool hasPendingTarget{};
    bool callbackQueued{};
};

std::mutex NavigationRefreshMutex;
NavigationRefreshState PendingNavigationRefresh{};
PTP_TIMER NavigationRefreshTimer{};

struct RevealReplayRequestState {
    std::uint64_t sessionGeneration{};
    std::int32_t targetLevelId{UnknownRevealLevelId};
    std::uint32_t retriesRemaining{};
    bool playerReady{};
    bool reconcilePending{};
    bool callbackQueued{};
};

std::mutex RevealReplayMutex;
RevealPersistenceState RevealPersistence{};
RevealReplayRequestState PendingRevealReplay{};
PTP_TIMER RevealReplayTimer{};
std::atomic_uint64_t AutomaticLevelRevealRequests{};
std::atomic_uint64_t AutomaticLevelRevealAccepted{};
std::atomic_uint64_t AutomaticLevelRevealRejected{};
std::atomic_uint64_t AutomaticLevelRevealDuplicateRefusals{};

class QueueLockGuard {
public:
    QueueLockGuard() noexcept {
        while (ActionQueueLock.test_and_set(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
    }
    ~QueueLockGuard() {
        ActionQueueLock.clear(std::memory_order_release);
    }

    QueueLockGuard(const QueueLockGuard&) = delete;
    auto operator=(const QueueLockGuard&) -> QueueLockGuard& = delete;
};

auto TrimAndLower(const D2RL::ConsoleCommandContext* command) -> std::string {
    std::string text;
    if (command != nullptr && command->args != nullptr) {
        text.assign(command->args, command->argsLength);
    }
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = text.find_last_not_of(" \t\r\n");
    text = text.substr(first, last - first + 1);
    for (char& character : text) {
        character = static_cast<char>(
            std::tolower(static_cast<unsigned char>(character)));
    }
    return text;
}

auto LoadConfig(Config& config) noexcept -> bool {
    if (Context == nullptr) return false;
    std::array<char, MaximumConfigBytes> buffer{};
    std::uint32_t requiredSize{};
    if (!Context->ReadConfig(
            buffer.data(),
            static_cast<std::uint32_t>(buffer.size()),
            &requiredSize)) {
        char message[192]{};
        std::snprintf(
            message,
            sizeof(message),
            "MapSense: configuration could not be read (required bytes=%u, limit=%zu).",
            requiredSize,
            buffer.size());
        Context->LogError(message);
        return false;
    }

    std::size_t length{};
    while (length < buffer.size() && buffer[length] != '\0') ++length;
    if (length == buffer.size()) {
        Context->LogError(
            "MapSense: configuration is not null-terminated or exceeds 64 KiB.");
        return false;
    }
    try {
        config = ParseConfig(std::string_view(buffer.data(), length));
        return true;
    } catch (const std::exception& exception) {
        const auto message = std::string("MapSense: invalid configuration (")
            + exception.what() + ").";
        Context->LogError(message.c_str());
        return false;
    } catch (...) {
        Context->LogError(
            "MapSense: invalid configuration (unknown parser failure).");
        return false;
    }
}

void WriteOutcome(
        const D2RL::PluginContext* context,
        RevealOutcome outcome,
        Action action) noexcept {
    if (context == nullptr) return;
    const char* message = "MapSense: reveal request failed; enter a game and try again.";
    D2RL::ConsoleMessageKind kind = D2RL::ConsoleMessageKind::Warning;
    switch (outcome) {
        case RevealOutcome::Complete:
            if (action == Action::RevealAct) {
                message = "MapSense: current level revealed; Reveal Act will reveal each level in this act as it loads.";
            } else {
                message = "MapSense: current level revealed and remembered for new games at this difficulty.";
            }
            kind = D2RL::ConsoleMessageKind::Output;
            break;
        case RevealOutcome::Armed:
            message = "MapSense: Reveal All armed; each level will be revealed natively as it loads in this difficulty.";
            kind = D2RL::ConsoleMessageKind::Output;
            break;
        case RevealOutcome::Disarmed:
            message = "MapSense: Reveal All disarmed; already revealed automap data remains visible.";
            kind = D2RL::ConsoleMessageKind::Output;
            break;
        case RevealOutcome::Unavailable:
            break;
    }
    context->WriteConsoleMessage(message, kind);
    if (kind == D2RL::ConsoleMessageKind::Warning) {
        context->LogWarn(message);
    } else {
        context->LogInfo(message);
    }
}

auto ExecuteAction(Action action) noexcept -> RevealOutcome {
    switch (action) {
        case Action::RevealZone: return RevealCurrentZone();
        case Action::RevealAct: return RevealCurrentAct();
        case Action::ToggleRevealAll: return ToggleRevealAll();
        case Action::DisableRevealAll: return DisableRevealAll();
        case Action::ToggleMenu: return RevealOutcome::Unavailable;
    }
    return RevealOutcome::Unavailable;
}

constexpr auto IsIdempotent(Action action) noexcept -> bool {
    return action == Action::RevealZone || action == Action::RevealAct;
}

auto RequestNavigationRefresh(
    std::uint64_t sessionGeneration,
    std::int32_t levelId) noexcept -> bool;
auto ExecuteTrackedRevealAction(Action action) noexcept -> RevealOutcome;

void __cdecl DrainActionsOnUi(
        const D2RL::PluginContext* context,
        void*) noexcept {
    for (;;) {
        QueuedAction request{};
        {
            QueueLockGuard lock;
            if (ActionQueueSize == 0) {
                ActionDrainScheduled = false;
                return;
            }
            request = ActionQueue[ActionQueueHead];
            ActionQueueHead = (ActionQueueHead + 1) % ActionQueueCapacity;
            --ActionQueueSize;
        }
        if (!Operational.load(std::memory_order_acquire) || context == nullptr) {
            continue;
        }
        if (request.sessionEpoch
            != SessionEpoch.load(std::memory_order_acquire)) {
            continue;
        }
        if (request.action == Action::ToggleMenu) {
            if (!GameplayReady.load(std::memory_order_acquire)) {
                SetD3D12ImGuiMenuOpen(false);
                continue;
            }
            bool expanded = MenuExpanded.load(std::memory_order_acquire);
            while (!MenuExpanded.compare_exchange_weak(
                expanded,
                !expanded,
                std::memory_order_acq_rel,
                std::memory_order_acquire)) {
            }
            SetD3D12ImGuiMenuOpen(true);
            continue;
        }
        const auto outcome = ExecuteTrackedRevealAction(request.action);
        WriteOutcome(context, outcome, request.action);
        const auto revealed = request.action == Action::RevealZone
            || request.action == Action::RevealAct;
        const auto accepted = outcome == RevealOutcome::Complete;
        if (revealed && accepted
            && GameplayReady.load(std::memory_order_acquire)) {
            const auto sessionGeneration = CurrentSessionGeneration.load(
                std::memory_order_acquire);
            if (sessionGeneration != 0U
                && !RequestNavigationRefresh(
                    sessionGeneration,
                    UnknownNavigationLevelId)
                && Settings.diagnostics) {
                context->LogWarn(
                    "MapSense navigation: the post-reveal exact-endpoint refresh could not be queued.");
            }
        }
    }
}

auto QueueAction(Action action) noexcept -> bool {
    if (!Operational.load(std::memory_order_acquire)
        || Context == nullptr || ThreadService == nullptr
        || ThreadService->runOnUiThread == nullptr) {
        return false;
    }

    const auto epoch = SessionEpoch.load(std::memory_order_acquire);
    bool scheduleDrain{};
    {
        QueueLockGuard lock;
        if (IsIdempotent(action)) {
            for (std::size_t index = 0; index < ActionQueueSize; ++index) {
                const auto& queued = ActionQueue[
                    (ActionQueueHead + index) % ActionQueueCapacity];
                if (queued.action == action && queued.sessionEpoch == epoch) {
                    return true;
                }
            }
        }
        if (ActionQueueSize == ActionQueueCapacity) return false;
        const auto tail = (ActionQueueHead + ActionQueueSize) % ActionQueueCapacity;
        ActionQueue[tail] = {.action = action, .sessionEpoch = epoch};
        ++ActionQueueSize;

        if (!ActionDrainScheduled) {
            ActionDrainScheduled = true;
            scheduleDrain = true;
        }
    }

    if (scheduleDrain) {
        if (ThreadService->runOnUiThread(Context, DrainActionsOnUi, nullptr)
            != D2RL::Threads::Result::Success) {
            QueueLockGuard lock;
            ActionQueueHead = 0;
            ActionQueueSize = 0;
            ActionDrainScheduled = false;
            return false;
        }
    }
    return true;
}

auto __cdecl OnAction(
        const D2RL::PluginContext* context,
        const D2RL::Input::ActionEvent* event,
        void* userData) noexcept -> D2RL::Input::ActionResult {
    if (!Operational.load(std::memory_order_acquire)
        || context == nullptr || userData == nullptr
        || !D2RL::Input::HasActionEventField(
            event, D2RL::Input::ActionEventRequiredSize)
        || event->kind != D2RL::Input::ActionEventKind::Pressed) {
        return D2RL::Input::ActionResult::Ignored;
    }
    const auto* token = static_cast<const Action*>(userData);
    const auto action = *token;
    const auto virtualKey = static_cast<std::uint32_t>(event->binding.key);
    const bool virtualKeyAccepted =
        MayTriggerMapSenseActionForVirtualKey(virtualKey);
    if (Settings.diagnostics) {
        char message[256]{};
        std::snprintf(
            message,
            sizeof(message),
            "MapSense diagnostic Controls action: action=%u key=0x%02X modifier=%u accepted-by-key-policy=%d gameplay-ready=%d.",
            static_cast<unsigned>(action),
            static_cast<unsigned>(virtualKey),
            static_cast<unsigned>(event->binding.modifier),
            virtualKeyAccepted ? 1 : 0,
            GameplayReady.load(std::memory_order_acquire) ? 1 : 0);
        context->LogInfo(message);
    }
    if (!virtualKeyAccepted) {
        return D2RL::Input::ActionResult::Ignored;
    }
    if (action == Action::ToggleMenu
        && !GameplayReady.load(std::memory_order_acquire)) {
        return D2RL::Input::ActionResult::Ignored;
    }
    if (QueueAction(action)) {
        return D2RL::Input::ActionResult::Handled;
    }
    context->LogWarn("MapSense: the reveal action could not be queued on the UI thread.");
    return D2RL::Input::ActionResult::Ignored;
}

void CancelPendingNavigationRefresh(bool clearQueuedCallback = false) noexcept {
    std::scoped_lock lock(NavigationRefreshMutex);
    PendingNavigationRefresh.sessionGeneration = 0U;
    PendingNavigationRefresh.levelId = UnknownNavigationLevelId;
    PendingNavigationRefresh.retriesRemaining = 0U;
    PendingNavigationRefresh.hasPendingTarget = false;
    if (clearQueuedCallback) {
        PendingNavigationRefresh.callbackQueued = false;
    }
    if (NavigationRefreshTimer != nullptr) {
        SetThreadpoolTimer(NavigationRefreshTimer, nullptr, 0U, 0U);
    }
}

void __cdecl RetryNavigationOnUi(
    const D2RL::PluginContext*,
    void*) noexcept;

void ScheduleNavigationRefreshTimer(
        std::uint32_t delayMilliseconds) noexcept {
    std::scoped_lock lock(NavigationRefreshMutex);
    if (NavigationRefreshTimer == nullptr) return;
    const auto boundedDelay = std::max(delayMilliseconds, 1U);
    ULARGE_INTEGER dueTime{};
    dueTime.QuadPart = static_cast<ULONGLONG>(
        -static_cast<LONGLONG>(boundedDelay) * 10'000LL);
    FILETIME due{
        .dwLowDateTime = dueTime.LowPart,
        .dwHighDateTime = dueTime.HighPart,
    };
    SetThreadpoolTimer(NavigationRefreshTimer, &due, 0U, 0U);
}

void ArmPendingNavigationRefresh(
        std::uint64_t sessionGeneration,
        std::int32_t levelId,
        std::uint32_t delayMilliseconds) noexcept {
    {
        std::scoped_lock lock(NavigationRefreshMutex);
        PendingNavigationRefresh.sessionGeneration = sessionGeneration;
        PendingNavigationRefresh.levelId = levelId;
        PendingNavigationRefresh.retriesRemaining
            = NavigationRefreshRetryLimit;
        PendingNavigationRefresh.hasPendingTarget = true;
    }
    ScheduleNavigationRefreshTimer(delayMilliseconds);
}

auto TryArmPendingNavigationRefresh(
        std::uint64_t sessionGeneration,
        std::int32_t levelId,
        std::uint32_t delayMilliseconds) noexcept -> bool {
    {
        std::scoped_lock lock(NavigationRefreshMutex);
        if (PendingNavigationRefresh.hasPendingTarget
            || PendingNavigationRefresh.callbackQueued) {
            return false;
        }
        PendingNavigationRefresh.sessionGeneration = sessionGeneration;
        PendingNavigationRefresh.levelId = levelId;
        PendingNavigationRefresh.retriesRemaining
            = NavigationRefreshRetryLimit;
        PendingNavigationRefresh.hasPendingTarget = true;
    }
    ScheduleNavigationRefreshTimer(delayMilliseconds);
    return true;
}

auto SchedulePendingNavigationRefresh() noexcept -> bool {
    {
        std::scoped_lock lock(NavigationRefreshMutex);
        if (!PendingNavigationRefresh.hasPendingTarget
            || PendingNavigationRefresh.callbackQueued) {
            return true;
        }
        PendingNavigationRefresh.callbackQueued = true;
    }

    if (!Operational.load(std::memory_order_acquire)
        || Context == nullptr || ThreadService == nullptr
        || ThreadService->runOnUiThread == nullptr
        || ThreadService->runOnUiThread(
            Context,
            RetryNavigationOnUi,
            nullptr) != D2RL::Threads::Result::Success) {
        std::scoped_lock lock(NavigationRefreshMutex);
        PendingNavigationRefresh.callbackQueued = false;
        return false;
    }
    return true;
}

auto RequestNavigationRefresh(
        std::uint64_t sessionGeneration,
        std::int32_t levelId) noexcept -> bool {
    if (!IsNavigationResolverActive()) {
        CancelPendingNavigationRefresh();
        return false;
    }
    const auto result = RefreshNavigationDestinations(
        sessionGeneration,
        levelId,
        Settings.navigation.customLevels.targets);
    if (result == NavigationRefreshResult::Complete) {
        CancelPendingNavigationRefresh();
        return true;
    }
    auto retryLevelId = levelId;
    if (retryLevelId == UnknownNavigationLevelId
        && result == NavigationRefreshResult::PartialRetryable) {
        retryLevelId = GetNavigationResolverCounters().lastLevelId;
    }
    ArmPendingNavigationRefresh(
        sessionGeneration,
        retryLevelId,
        NavigationRefreshRetryDelayMilliseconds);
    return true;
}

void __cdecl RetryNavigationOnUi(
        const D2RL::PluginContext*,
        void*) noexcept {
    NavigationRefreshState attempt{};
    {
        std::scoped_lock lock(NavigationRefreshMutex);
        PendingNavigationRefresh.callbackQueued = false;
        if (!PendingNavigationRefresh.hasPendingTarget) return;
        attempt = PendingNavigationRefresh;
    }

    if (!Operational.load(std::memory_order_acquire)
        || !GameplayReady.load(std::memory_order_acquire)
        || !IsNavigationResolverActive()) {
        CancelPendingNavigationRefresh();
        return;
    }

    const auto result = RefreshNavigationDestinations(
        attempt.sessionGeneration,
        attempt.levelId,
        Settings.navigation.customLevels.targets);
    if (result == NavigationRefreshResult::Complete) {
        std::scoped_lock lock(NavigationRefreshMutex);
        if (PendingNavigationRefresh.hasPendingTarget
            && PendingNavigationRefresh.sessionGeneration
                == attempt.sessionGeneration
            && PendingNavigationRefresh.levelId == attempt.levelId) {
            PendingNavigationRefresh.hasPendingTarget = false;
            PendingNavigationRefresh.retriesRemaining = 0U;
        }
        return;
    }

    bool exhausted{};
    {
        std::scoped_lock lock(NavigationRefreshMutex);
        if (!PendingNavigationRefresh.hasPendingTarget
            || PendingNavigationRefresh.sessionGeneration
                != attempt.sessionGeneration
            || PendingNavigationRefresh.levelId != attempt.levelId) {
            return;
        }
        if (PendingNavigationRefresh.retriesRemaining > 1U) {
            --PendingNavigationRefresh.retriesRemaining;
        } else {
            PendingNavigationRefresh.retriesRemaining = 0U;
            PendingNavigationRefresh.hasPendingTarget = false;
            exhausted = true;
        }
    }

    if (!exhausted) {
        ScheduleNavigationRefreshTimer(
            NavigationRefreshRetryDelayMilliseconds);
    } else if (Context != nullptr && Settings.diagnostics) {
        Context->LogWarn(
            "MapSense navigation: an exact destination preset was not ready; no guessed line was published.");
    }
}

void CALLBACK NavigationRefreshTimerCallback(
        PTP_CALLBACK_INSTANCE,
        void*,
        PTP_TIMER) noexcept {
    if (!Operational.load(std::memory_order_acquire)) return;
    if (!SchedulePendingNavigationRefresh()
        && Context != nullptr && Settings.diagnostics) {
        Context->LogWarn(
            "MapSense navigation: a deferred refresh could not be queued.");
    }
}

void OnNativeAutomapLevelObserved(
        std::int32_t currentLevelId,
        bool levelChanged,
        void*) noexcept {
    if (!Operational.load(std::memory_order_acquire)
        || !GameplayReady.load(std::memory_order_acquire)
        || currentLevelId == UnknownNavigationLevelId) {
        return;
    }
    const auto sessionGeneration = CurrentSessionGeneration.load(
        std::memory_order_acquire);
    if (sessionGeneration == 0U) return;
    if (levelChanged) {
        LastDynamicNavigationRefreshTick.store(
            GetTickCount64(),
            std::memory_order_release);
        ArmPendingNavigationRefresh(sessionGeneration, currentLevelId, 0U);
        return;
    }
    // Only genuinely dynamic progression contracts receive periodic refreshes.
    // Static outdoor boundaries use the bounded level-change retry budget;
    // rescanning their complete collision topology indefinitely can stall D2R.
    if (!HasDynamicMainProgressionTargetFor(currentLevelId)) return;
    const auto counters = GetNavigationResolverCounters();
    if (counters.lastLevelId == currentLevelId
        && (counters.lastProgressionX != 0
            || counters.lastProgressionY != 0)) {
        return;
    }

    const auto now = GetTickCount64();
    auto previous = LastDynamicNavigationRefreshTick.load(
        std::memory_order_acquire);
    while (now - previous
            >= DynamicNavigationRefreshIntervalMilliseconds) {
        if (LastDynamicNavigationRefreshTick.compare_exchange_weak(
                previous,
                now,
                std::memory_order_acq_rel,
                std::memory_order_acquire)) {
            TryArmPendingNavigationRefresh(
                sessionGeneration,
                currentLevelId,
                0U);
            return;
        }
    }
}

[[nodiscard]] auto InitializeNavigationRefreshTimer() noexcept -> bool {
    std::scoped_lock lock(NavigationRefreshMutex);
    if (NavigationRefreshTimer != nullptr) return true;
    NavigationRefreshTimer = CreateThreadpoolTimer(
        NavigationRefreshTimerCallback,
        nullptr,
        nullptr);
    return NavigationRefreshTimer != nullptr;
}

void ShutdownNavigationRefreshTimer() noexcept {
    PTP_TIMER timer{};
    {
        std::scoped_lock lock(NavigationRefreshMutex);
        timer = NavigationRefreshTimer;
        NavigationRefreshTimer = nullptr;
    }
    if (timer == nullptr) return;
    SetThreadpoolTimer(timer, nullptr, 0U, 0U);
    WaitForThreadpoolTimerCallbacks(timer, TRUE);
    CloseThreadpoolTimer(timer);
}

constexpr auto IsKnownRevealAct(std::int32_t act) noexcept -> bool {
    return act >= 0
        && act < static_cast<std::int32_t>(RevealPersistenceActCount);
}

void ResetAutomaticLevelRevealCounters() noexcept {
    AutomaticLevelRevealRequests.store(0U, std::memory_order_relaxed);
    AutomaticLevelRevealAccepted.store(0U, std::memory_order_relaxed);
    AutomaticLevelRevealRejected.store(0U, std::memory_order_relaxed);
    AutomaticLevelRevealDuplicateRefusals.store(
        0U,
        std::memory_order_relaxed);
}

void ScheduleRevealReplayRetryTimer(
        std::uint32_t delayMilliseconds) noexcept {
    std::scoped_lock lock(RevealReplayMutex);
    if (RevealReplayTimer == nullptr) return;
    const auto boundedDelay = std::max(delayMilliseconds, 1U);
    ULARGE_INTEGER dueTime{};
    dueTime.QuadPart = static_cast<ULONGLONG>(
        -static_cast<LONGLONG>(boundedDelay) * 10'000LL);
    FILETIME due{
        .dwLowDateTime = dueTime.LowPart,
        .dwHighDateTime = dueTime.HighPart,
    };
    SetThreadpoolTimer(RevealReplayTimer, &due, 0U, 0U);
}

void CancelPendingRevealReplay() noexcept {
    std::scoped_lock lock(RevealReplayMutex);
    PendingRevealReplay.targetLevelId = UnknownRevealLevelId;
    PendingRevealReplay.retriesRemaining = 0U;
    PendingRevealReplay.reconcilePending = false;
    PendingRevealReplay.callbackQueued = false;
    if (RevealReplayTimer != nullptr) {
        SetThreadpoolTimer(RevealReplayTimer, nullptr, 0U, 0U);
    }
}

void ResetRevealReplayGameplaySession(
        std::uint64_t sessionGeneration) noexcept {
    std::scoped_lock lock(RevealReplayMutex);
    RevealPersistence.BeginSession(sessionGeneration);
    PendingRevealReplay = {};
    PendingRevealReplay.sessionGeneration = sessionGeneration;
    if (sessionGeneration != 0U) {
        ResetAutomaticLevelRevealCounters();
    }
    if (RevealReplayTimer != nullptr) {
        SetThreadpoolTimer(RevealReplayTimer, nullptr, 0U, 0U);
    }
}

void ResetRevealReplayProcessState() noexcept {
    std::scoped_lock lock(RevealReplayMutex);
    RevealPersistence.ResetProcess();
    PendingRevealReplay = {};
    ResetAutomaticLevelRevealCounters();
    if (RevealReplayTimer != nullptr) {
        SetThreadpoolTimer(RevealReplayTimer, nullptr, 0U, 0U);
    }
}

auto ObserveValidatedRevealDifficulty(
        std::int32_t difficulty) noexcept -> bool {
    RevealDifficultyObservation observation{};
    {
        std::scoped_lock lock(RevealReplayMutex);
        observation = RevealPersistence.ObserveDifficulty(difficulty);
        if (observation == RevealDifficultyObservation::Changed) {
            PendingRevealReplay.targetLevelId = UnknownRevealLevelId;
            PendingRevealReplay.retriesRemaining = 0U;
            PendingRevealReplay.reconcilePending = false;
            PendingRevealReplay.callbackQueued = false;
            if (RevealReplayTimer != nullptr) {
                SetThreadpoolTimer(RevealReplayTimer, nullptr, 0U, 0U);
            }
        }
    }
    if (observation == RevealDifficultyObservation::Invalid) return false;
    if (observation == RevealDifficultyObservation::Changed) {
        (void)DisableRevealAll();
        if (Context != nullptr) {
            Context->LogInfo(
                "MapSense: game difficulty changed; remembered Reveal Level, Reveal Act, and Reveal All intents were cleared.");
        }
    }
    return true;
}

auto HasRememberedRevealIntent() noexcept -> bool {
    std::scoped_lock lock(RevealReplayMutex);
    return RevealPersistence.HasAnyIntent();
}

auto ArmRevealReplayRequest(
        std::uint64_t sessionGeneration,
        std::int32_t targetLevelId,
        std::uint32_t delayMilliseconds) noexcept -> bool {
    if (!Operational.load(std::memory_order_acquire)
        || sessionGeneration == 0U) {
        return false;
    }

    {
        std::scoped_lock lock(RevealReplayMutex);
        if (PendingRevealReplay.sessionGeneration != sessionGeneration) {
            return false;
        }
        if (!PendingRevealReplay.playerReady
            || !RevealPersistence.HasAnyIntent()) {
            return true;
        }
        if (PendingRevealReplay.reconcilePending) {
            // ActChanged may arrive without a level id. It must not consume a
            // second budget or replace a more precise LevelChanged target.
            if (targetLevelId <= 0
                || PendingRevealReplay.targetLevelId == targetLevelId) {
                return true;
            }
        }
        PendingRevealReplay.targetLevelId = targetLevelId;
        PendingRevealReplay.retriesRemaining = RevealReplayRetryLimit;
        PendingRevealReplay.reconcilePending = true;
    }
    ScheduleRevealReplayRetryTimer(delayMilliseconds);
    return true;
}

auto RequestRememberedRevealForCurrentSession(
        std::int32_t targetLevelId = UnknownRevealLevelId) noexcept -> bool {
    if (!HasRememberedRevealIntent()) return true;
    if (!GameplayReady.load(std::memory_order_acquire)) {
        // The process-lifetime ids remain remembered. LocalPlayerReady will
        // reconcile them against the new game's generated client DRLG.
        return true;
    }
    const auto sessionGeneration = CurrentSessionGeneration.load(
        std::memory_order_acquire);
    if (sessionGeneration == 0U) return true;
    return ArmRevealReplayRequest(sessionGeneration, targetLevelId, 0U);
}

void SetRevealReplayPlayerReady(
        std::uint64_t sessionGeneration) noexcept {
    std::scoped_lock lock(RevealReplayMutex);
    if (PendingRevealReplay.sessionGeneration == sessionGeneration) {
        PendingRevealReplay.playerReady = true;
    }
}

auto ObserveRevealReplayAct(
        std::uint64_t sessionGeneration,
        std::int32_t,
        std::int32_t) noexcept -> bool {
    bool shouldRequest{};
    {
        std::scoped_lock lock(RevealReplayMutex);
        if (PendingRevealReplay.sessionGeneration != sessionGeneration) {
            return false;
        }
        shouldRequest = PendingRevealReplay.playerReady
            && RevealPersistence.HasAnyIntent();
    }
    if (!shouldRequest) return true;
    return ArmRevealReplayRequest(
        sessionGeneration,
        UnknownRevealLevelId,
        0U);
}

void CompletePendingRevealReplay(
        std::uint64_t sessionGeneration) noexcept {
    std::scoped_lock lock(RevealReplayMutex);
    if (PendingRevealReplay.sessionGeneration != sessionGeneration) return;
    PendingRevealReplay.targetLevelId = UnknownRevealLevelId;
    PendingRevealReplay.retriesRemaining = 0U;
    PendingRevealReplay.reconcilePending = false;
}

auto RetryPendingRevealReplay(
        std::uint64_t sessionGeneration) noexcept -> bool {
    bool retry{};
    {
        std::scoped_lock lock(RevealReplayMutex);
        if (PendingRevealReplay.sessionGeneration != sessionGeneration
            || !PendingRevealReplay.reconcilePending) {
            return true;
        }
        if (PendingRevealReplay.retriesRemaining > 1U) {
            --PendingRevealReplay.retriesRemaining;
            retry = true;
        } else {
            PendingRevealReplay.targetLevelId = UnknownRevealLevelId;
            PendingRevealReplay.retriesRemaining = 0U;
            PendingRevealReplay.reconcilePending = false;
        }
    }
    if (retry) {
        ScheduleRevealReplayRetryTimer(
            RevealReplayRetryDelayMilliseconds);
        return true;
    }
    return false;
}

void __cdecl RetryRememberedRevealOnUi(
        const D2RL::PluginContext*,
        void*) noexcept {
    RevealReplayRequestState attempt{};
    {
        std::scoped_lock lock(RevealReplayMutex);
        PendingRevealReplay.callbackQueued = false;
        if (!PendingRevealReplay.reconcilePending) return;
        attempt = PendingRevealReplay;
    }

    if (!Operational.load(std::memory_order_acquire)
        || !GameplayReady.load(std::memory_order_acquire)
        || attempt.sessionGeneration == 0U
        || attempt.sessionGeneration
            != CurrentSessionGeneration.load(std::memory_order_acquire)) {
        CancelPendingRevealReplay();
        return;
    }

    ClientLevelView current{};
    const bool clientReady = ResolveCurrentClientLevelView(current);
    if (!clientReady
        || !ObserveValidatedRevealDifficulty(current.difficulty)) {
        if (!RetryPendingRevealReplay(attempt.sessionGeneration)
            && Context != nullptr) {
            Context->LogWarn(
                "MapSense: remembered reveal intent expired before the new client map became ready.");
        }
        return;
    }

    {
        std::scoped_lock lock(RevealReplayMutex);
        if (PendingRevealReplay.sessionGeneration
                != attempt.sessionGeneration
            || !PendingRevealReplay.reconcilePending
            || PendingRevealReplay.targetLevelId
                != attempt.targetLevelId) {
            return;
        }
    }

    // A LevelChanged target is authoritative. If D2R still exposes the prior
    // client DRLG, retain the complete new-level budget and retry instead of
    // consuming or crediting the wrong generated level.
    if (attempt.targetLevelId > 0
        && current.levelId != attempt.targetLevelId) {
        if (!RetryPendingRevealReplay(attempt.sessionGeneration)
            && Context != nullptr) {
            Context->LogWarn(
                "MapSense: remembered reveal target did not become the active client level before its bounded retries expired.");
        }
        return;
    }

    bool hasIntent{};
    bool shouldReveal{};
    const auto currentAct = RevealActForLevelId(current.levelId);
    {
        std::scoped_lock lock(RevealReplayMutex);
        if (PendingRevealReplay.sessionGeneration
                != attempt.sessionGeneration
            || !PendingRevealReplay.reconcilePending) {
            return;
        }
        if (PendingRevealReplay.targetLevelId <= 0) {
            PendingRevealReplay.targetLevelId = current.levelId;
        }
        hasIntent = RevealPersistence.HasReplayIntentForLevel(
            current.difficulty,
            currentAct,
            current.levelId);
        shouldReveal = RevealPersistence.ShouldReplayCurrentLevel(
            current.difficulty,
            currentAct,
            current.levelId);
    }

    if (!hasIntent) {
        CompletePendingRevealReplay(attempt.sessionGeneration);
        return;
    }
    if (!shouldReveal) {
        AutomaticLevelRevealDuplicateRefusals.fetch_add(
            1U,
            std::memory_order_relaxed);
        CompletePendingRevealReplay(attempt.sessionGeneration);
        return;
    }

    AutomaticLevelRevealRequests.fetch_add(1U, std::memory_order_relaxed);
    const auto outcome = RevealCurrentZone();
    const bool accepted = outcome == RevealOutcome::Complete;
    (accepted
            ? AutomaticLevelRevealAccepted
            : AutomaticLevelRevealRejected)
        .fetch_add(1U, std::memory_order_relaxed);
    if (accepted) {
        {
            std::scoped_lock lock(RevealReplayMutex);
            if (PendingRevealReplay.sessionGeneration
                    != attempt.sessionGeneration
                || !PendingRevealReplay.reconcilePending) {
                return;
            }
            (void)RevealPersistence.MarkLevelAccepted(
                current.difficulty,
                current.levelId);
            PendingRevealReplay.targetLevelId = UnknownRevealLevelId;
            PendingRevealReplay.retriesRemaining = 0U;
            PendingRevealReplay.reconcilePending = false;
        }
        if (Context != nullptr && Settings.diagnostics) {
            char message[224]{};
            std::snprintf(
                message,
                sizeof(message),
                "MapSense native reveal replay: accepted current level %d for session %llu.",
                current.levelId,
                static_cast<unsigned long long>(attempt.sessionGeneration));
            Context->LogInfo(message);
        }
        return;
    }

    if (!RetryPendingRevealReplay(attempt.sessionGeneration)
        && Context != nullptr) {
        Context->LogWarn(
            "MapSense: remembered reveal intent remained unavailable after its bounded native current-level retries; it stays remembered for the next matching level event.");
    }
}

auto SchedulePendingRevealReplay() noexcept -> bool {
    {
        std::scoped_lock lock(RevealReplayMutex);
        if (!PendingRevealReplay.reconcilePending
            || PendingRevealReplay.callbackQueued) {
            return true;
        }
        PendingRevealReplay.callbackQueued = true;
    }

    if (!Operational.load(std::memory_order_acquire)
        || Context == nullptr || ThreadService == nullptr
        || ThreadService->runOnUiThread == nullptr
        || ThreadService->runOnUiThread(
            Context,
            RetryRememberedRevealOnUi,
            nullptr) != D2RL::Threads::Result::Success) {
        std::scoped_lock lock(RevealReplayMutex);
        PendingRevealReplay.callbackQueued = false;
        return false;
    }
    return true;
}

void CALLBACK RevealReplayTimerCallback(
        PTP_CALLBACK_INSTANCE,
        void*,
        PTP_TIMER) noexcept {
    if (!Operational.load(std::memory_order_acquire)) return;
    std::uint64_t sessionGeneration{};
    {
        std::scoped_lock lock(RevealReplayMutex);
        sessionGeneration = PendingRevealReplay.sessionGeneration;
    }
    if (!SchedulePendingRevealReplay()
        && !RetryPendingRevealReplay(sessionGeneration)
        && Context != nullptr) {
        Context->LogWarn(
            "MapSense: remembered reveal intent could not be queued on D2R's UI thread before its retry budget expired.");
    } else if (Context != nullptr && Settings.diagnostics) {
        // Queue failures are intentionally counted against the same bounded
        // budget; successful scheduling produces no per-frame log traffic.
        std::scoped_lock lock(RevealReplayMutex);
        if (PendingRevealReplay.reconcilePending
            && !PendingRevealReplay.callbackQueued) {
            Context->LogWarn(
                "MapSense: deferred reveal reconciliation will retry on D2R's UI thread.");
        }
    }
}

[[nodiscard]] auto InitializeRevealReplayTimer() noexcept -> bool {
    std::scoped_lock lock(RevealReplayMutex);
    if (RevealReplayTimer != nullptr) return true;
    RevealReplayTimer = CreateThreadpoolTimer(
        RevealReplayTimerCallback,
        nullptr,
        nullptr);
    return RevealReplayTimer != nullptr;
}

void ShutdownRevealReplayTimer() noexcept {
    PTP_TIMER timer{};
    {
        std::scoped_lock lock(RevealReplayMutex);
        timer = RevealReplayTimer;
        RevealReplayTimer = nullptr;
        RevealPersistence.ResetProcess();
        PendingRevealReplay = {};
    }
    if (timer == nullptr) return;
    SetThreadpoolTimer(timer, nullptr, 0U, 0U);
    WaitForThreadpoolTimerCallbacks(timer, TRUE);
    CloseThreadpoolTimer(timer);
}

auto ExecuteTrackedRevealAction(Action action) noexcept -> RevealOutcome {
    if (action == Action::DisableRevealAll) {
        const auto outcome = ExecuteAction(action);
        {
            std::scoped_lock lock(RevealReplayMutex);
            RevealPersistence.ClearRevealAll();
        }
        CancelPendingRevealReplay();
        (void)RequestRememberedRevealForCurrentSession();
        return outcome;
    }

    ClientLevelView current{};
    if (!ResolveCurrentClientLevelView(current)
        || !ObserveValidatedRevealDifficulty(current.difficulty)) {
        return RevealOutcome::Unavailable;
    }

    const auto currentAct = RevealActForLevelId(current.levelId);
    const auto outcome = ExecuteAction(action);
    bool currentLevelRevealed = outcome == RevealOutcome::Complete;
    if (action == Action::ToggleRevealAll
        && outcome == RevealOutcome::Armed) {
        currentLevelRevealed =
            RevealCurrentZone() == RevealOutcome::Complete;
    }
    bool remembered{true};
    {
        std::scoped_lock lock(RevealReplayMutex);
        if (action == Action::RevealZone
            && outcome == RevealOutcome::Complete) {
            remembered = RevealPersistence.RememberLevel(
                current.difficulty,
                current.levelId);
            (void)RevealPersistence.MarkLevelAccepted(
                current.difficulty,
                current.levelId);
        } else if (action == Action::RevealAct
            && outcome == RevealOutcome::Complete) {
            if (IsKnownRevealAct(currentAct)) {
                remembered = RevealPersistence.RememberAct(
                    current.difficulty,
                    currentAct);
            } else {
                // The action succeeded, but an unknown Levels.txt id cannot be
                // persisted safely as an Act intent.
                remembered = false;
            }
            (void)RevealPersistence.MarkLevelAccepted(
                current.difficulty,
                current.levelId);
        } else if (action == Action::ToggleRevealAll
            && (outcome == RevealOutcome::Armed
                || outcome == RevealOutcome::Disarmed)) {
            remembered = RevealPersistence.SetRevealAll(
                current.difficulty,
                outcome == RevealOutcome::Armed);
            if (outcome == RevealOutcome::Armed
                && currentLevelRevealed) {
                (void)RevealPersistence.MarkLevelAccepted(
                    current.difficulty,
                    current.levelId);
            }
        }
    }
    if (!remembered && Context != nullptr) {
        Context->LogWarn(
            "MapSense: a successful reveal could not be added to the bounded process-lifetime intent set.");
    }

    if (action == Action::ToggleRevealAll) {
        CancelPendingRevealReplay();
        if ((!currentLevelRevealed
                || outcome == RevealOutcome::Disarmed)
            && !RequestRememberedRevealForCurrentSession()
            && Context != nullptr) {
            Context->LogWarn(
                "MapSense: remembered reveal intent could not be scheduled for the current session.");
        }
    }
    return outcome;
}

void __cdecl OnGameplayEvent(
        const D2RL::PluginContext*,
        const D2RL::Lifecycle::GameplayEvent* event,
        void*) noexcept {
    if (!D2RL::Lifecycle::HasGameplayEventField(
            event, D2RL::Lifecycle::GameplayEventRequiredSize)) {
        return;
    }
    if (event->kind == D2RL::Lifecycle::GameplayEventKind::GameJoined) {
        CurrentSessionGeneration.store(
            event->sessionGeneration,
            std::memory_order_release);
        LastDynamicNavigationRefreshTick.store(
            0U,
            std::memory_order_release);
        CancelPendingNavigationRefresh();
        ResetNavigationSession(event->sessionGeneration);
        ResetNativeAutomapMarker();
        GameplayReady.store(false, std::memory_order_release);
        MenuExpanded.store(false, std::memory_order_release);
        SetD3D12ImGuiMenuOpen(false);
        SessionEpoch.fetch_add(1, std::memory_order_acq_rel);
        ResetRevealReplayGameplaySession(event->sessionGeneration);
        // InitLevel can run before GameJoined. Preserve the client DRLG it
        // captured while resetting per-game diagnostics. The process-lifetime
        // Reveal Level, Reveal Act, and Reveal All intents intentionally
        // survive Save & Exit within the same validated difficulty.
        BeginRevealSession();
    } else if (event->kind
        == D2RL::Lifecycle::GameplayEventKind::LocalPlayerReady) {
        CurrentSessionGeneration.store(
            event->sessionGeneration,
            std::memory_order_release);
        GameplayReady.store(true, std::memory_order_release);
        MenuExpanded.store(
            Settings.menu.startExpanded || Settings.overlay.startMenuOpen,
            std::memory_order_release);
        SetD3D12ImGuiMenuOpen(true);
        SetRevealReplayPlayerReady(event->sessionGeneration);
        if (!RequestRememberedRevealForCurrentSession()
            && Context != nullptr) {
            Context->LogWarn(
                "MapSense: remembered reveals for the new game could not be scheduled.");
        }
        if (IsNavigationResolverActive()
            && !RequestNavigationRefresh(
                event->sessionGeneration,
                UnknownNavigationLevelId)
            && Context != nullptr && Settings.diagnostics) {
            Context->LogWarn(
                "MapSense navigation: the initial level refresh could not be queued.");
        }
    } else if (event->kind == D2RL::Lifecycle::GameplayEventKind::GameLeft) {
        CancelPendingNavigationRefresh();
        ResetNavigationSession(event->sessionGeneration);
        ResetNativeAutomapMarker();
        GameplayReady.store(false, std::memory_order_release);
        CurrentSessionGeneration.store(0U, std::memory_order_release);
        LastDynamicNavigationRefreshTick.store(
            0U,
            std::memory_order_release);
        MenuExpanded.store(false, std::memory_order_release);
        SetD3D12ImGuiMenuOpen(false);
        SessionEpoch.fetch_add(1, std::memory_order_acq_rel);
        ResetRevealReplayGameplaySession(0U);
        ResetRevealSession();
    } else if (event->kind == D2RL::Lifecycle::GameplayEventKind::ActChanged) {
        ResetNativeAutomapMarker();
        const auto replayQueued = ObserveRevealReplayAct(
            event->sessionGeneration,
            event->previousValue,
            event->currentValue);
        if (!replayQueued && Context != nullptr) {
            Context->LogWarn(
                "MapSense: remembered reveals for the newly loaded act could not be scheduled.");
        }
        if (Settings.diagnostics && Context != nullptr) {
            char message[256]{};
            std::snprintf(
                message,
                sizeof(message),
                "MapSense reveal persistence diagnostic: act-change session=%llu previous=%d current=%d armed=%d queued=%d.",
                static_cast<unsigned long long>(event->sessionGeneration),
                static_cast<std::int32_t>(event->previousValue),
                static_cast<std::int32_t>(event->currentValue),
                IsRevealAllArmed() ? 1 : 0,
                replayQueued ? 1 : 0);
            Context->LogInfo(message);
        }
    } else if (event->kind
        == D2RL::Lifecycle::GameplayEventKind::LevelChanged) {
        CurrentSessionGeneration.store(
            event->sessionGeneration,
            std::memory_order_release);
        LastDynamicNavigationRefreshTick.store(
            0U,
            std::memory_order_release);
        CancelPendingNavigationRefresh();
        ResetNavigationLevel(
            event->sessionGeneration,
            event->currentValue);
        ResetNativeAutomapMarker();
        if (!RequestRememberedRevealForCurrentSession(event->currentValue)
            && Context != nullptr) {
            Context->LogWarn(
                "MapSense: remembered reveals for the changed level could not be scheduled.");
        }
        if (IsNavigationResolverActive()
            && !RequestNavigationRefresh(
                event->sessionGeneration,
                event->currentValue)
            && Context != nullptr && Settings.diagnostics) {
            Context->LogWarn(
                "MapSense navigation: the changed level refresh could not be queued.");
        }
    }
}

void WriteStatus(const D2RL::PluginContext* context) noexcept {
    if (context == nullptr) return;
    const auto counters = GetRevealCounters();
    const auto marker = GetNativeAutomapMarkerCounters();
    const auto renderer = GetD3D12ImGuiHostStatus();
    char message[2048]{};
    std::snprintf(
        message,
        sizeof(message),
            "RuffnecKk MapSense 0.12.5 candidate: active=%s; gameplay=%s; reveal-all=%s; markers=%s; immunity-scan=%s; renderer-hooks=%s; renderer=%s; input=%s; menu=%s; presents=%llu; rendered=%llu; level traversals=%llu; rooms=%llu; failures=%llu; traversal limits=%llu; automap-pulses=%llu; table-scans=%llu; buckets=%llu; table-limits=%llu; automap units=%llu; monsters=%llu; enemy-rejects=dead/unit/class/alignment:%llu/%llu/%llu/%llu; filter-faults=%llu; hostiles=%llu; hostile-bands=0-80/81-140/141-220/>220:%llu/%llu/%llu/%llu; projection-rejects=%llu; clip-rejects=%llu; max-hostile-subtiles=%u; max-accepted-subtiles=%u; max-published-subtiles=%u; accepted=%llu; inserted=%llu; refreshed=%llu; fresh=%llu; expired=%llu; marker waits=%llu; storage faults=%llu; marker faults=%llu.",
        IsRevealEngineActive() ? "true" : "false",
        GameplayReady.load(std::memory_order_acquire) ? "ready" : "inactive",
        IsRevealAllArmed() ? "armed" : "off",
        MarkerAvailable.load(std::memory_order_acquire)
            ? "ready"
            : "unavailable",
        Settings.overlay.enabled
                && Settings.immunities.enabled
                && MarkerAvailable.load(std::memory_order_acquire)
            ? "enabled"
            : "disabled",
        renderer.hooksInstalled ? "installed" : "waiting",
        renderer.rendererInitialized ? "ready" : "waiting",
        renderer.inputSubclassInstalled ? "isolated" : "waiting",
        MenuExpanded.load(std::memory_order_acquire)
            ? "expanded"
            : "launcher",
        static_cast<unsigned long long>(renderer.presentCalls),
        static_cast<unsigned long long>(renderer.renderedFrames),
        static_cast<unsigned long long>(counters.levels),
        static_cast<unsigned long long>(counters.rooms),
        static_cast<unsigned long long>(counters.failures),
        static_cast<unsigned long long>(counters.traversalLimits),
        static_cast<unsigned long long>(marker.automapPulses),
        static_cast<unsigned long long>(marker.monsterTableScans),
        static_cast<unsigned long long>(marker.monsterBucketsVisited),
        static_cast<unsigned long long>(marker.monsterTraversalLimits),
        static_cast<unsigned long long>(marker.unitsObserved),
        static_cast<unsigned long long>(marker.monstersObserved),
        static_cast<unsigned long long>(marker.modeRejected),
        static_cast<unsigned long long>(marker.unitFlagRejected),
        static_cast<unsigned long long>(marker.classRejected),
        static_cast<unsigned long long>(marker.alignmentRejected),
        static_cast<unsigned long long>(marker.metadataFaults),
        static_cast<unsigned long long>(marker.hostilesObserved),
        static_cast<unsigned long long>(marker.hostilesThrough80),
        static_cast<unsigned long long>(marker.hostilesFrom81Through140),
        static_cast<unsigned long long>(marker.hostilesFrom141Through220),
        static_cast<unsigned long long>(marker.hostilesBeyond220),
        static_cast<unsigned long long>(marker.projectionRejected),
        static_cast<unsigned long long>(marker.nativeClipRejected),
        marker.maximumHostileDistance,
        marker.maximumAcceptedDistance,
        marker.maximumPublishedDistance,
        static_cast<unsigned long long>(marker.candidatesAccepted),
        static_cast<unsigned long long>(marker.markersInserted),
        static_cast<unsigned long long>(marker.markersRefreshed),
        static_cast<unsigned long long>(marker.freshMarkers),
        static_cast<unsigned long long>(marker.markersExpired),
        static_cast<unsigned long long>(marker.contentionWaits),
        static_cast<unsigned long long>(marker.storageFailures),
        static_cast<unsigned long long>(marker.accessFaults));
    context->WriteConsoleMessage(message);
    RevealReplayRequestState pendingReveal{};
    std::int32_t revealDifficulty{UnknownRevealDifficulty};
    {
        std::scoped_lock lock(RevealReplayMutex);
        pendingReveal = PendingRevealReplay;
        revealDifficulty = RevealPersistence.Difficulty();
    }
    char revealMessage[512]{};
    std::snprintf(
        revealMessage,
        sizeof(revealMessage),
        "MapSense native reveal replay: automatic calls=%llu; accepted=%llu; rejected=%llu; duplicate refusals=%llu; session=%llu; difficulty=%d; target-level=%d; pending=%s; retries=%u.",
        static_cast<unsigned long long>(
            AutomaticLevelRevealRequests.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(
            AutomaticLevelRevealAccepted.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(
            AutomaticLevelRevealRejected.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(
            AutomaticLevelRevealDuplicateRefusals.load(
                std::memory_order_relaxed)),
        static_cast<unsigned long long>(pendingReveal.sessionGeneration),
        revealDifficulty,
        pendingReveal.targetLevelId,
        pendingReveal.reconcilePending ? "yes" : "no",
        pendingReveal.retriesRemaining);
    context->WriteConsoleMessage(revealMessage);
    const auto averageDiscoveryMicroseconds =
        marker.discoveryTimingSamples != 0U
        ? marker.totalDiscoveryMicroseconds / marker.discoveryTimingSamples
        : 0U;
    const auto averageRefreshMicroseconds =
        marker.refreshTimingSamples != 0U
        ? marker.totalRefreshMicroseconds / marker.refreshTimingSamples
        : 0U;
    char markerMessage[1024]{};
    std::snprintf(
        markerMessage,
        sizeof(markerMessage),
        "MapSense monster pipeline: tracked=%llu; discovery scans=%llu timing avg/max=%llu/%llu us (%llu samples); position refreshes=%llu timing avg/max=%llu/%llu us (%llu samples); id lookups resolved/missing=%llu/%llu; accepted-bands=0-80/81-140/141-220/>220:%llu/%llu/%llu/%llu; clip-rejected-bands=0-80/81-140/141-220/>220:%llu/%llu/%llu/%llu.",
        static_cast<unsigned long long>(marker.trackedCurrent),
        static_cast<unsigned long long>(marker.monsterTableScans),
        static_cast<unsigned long long>(averageDiscoveryMicroseconds),
        static_cast<unsigned long long>(
            marker.maximumDiscoveryMicroseconds),
        static_cast<unsigned long long>(marker.discoveryTimingSamples),
        static_cast<unsigned long long>(marker.monsterPositionRefreshes),
        static_cast<unsigned long long>(averageRefreshMicroseconds),
        static_cast<unsigned long long>(marker.maximumRefreshMicroseconds),
        static_cast<unsigned long long>(marker.refreshTimingSamples),
        static_cast<unsigned long long>(marker.trackedIdsResolved),
        static_cast<unsigned long long>(marker.trackedIdsMissing),
        static_cast<unsigned long long>(marker.acceptedThrough80),
        static_cast<unsigned long long>(marker.acceptedFrom81Through140),
        static_cast<unsigned long long>(marker.acceptedFrom141Through220),
        static_cast<unsigned long long>(marker.acceptedBeyond220),
        static_cast<unsigned long long>(marker.clipRejectedThrough80),
        static_cast<unsigned long long>(
            marker.clipRejectedFrom81Through140),
        static_cast<unsigned long long>(
            marker.clipRejectedFrom141Through220),
        static_cast<unsigned long long>(marker.clipRejectedBeyond220));
    context->WriteConsoleMessage(markerMessage);
    const auto navigation = GetNavigationResolverCounters();
    const auto navigationEngine = GetNavigationEngineStatus();
    NavigationRefreshState pendingNavigation{};
    {
        std::scoped_lock lock(NavigationRefreshMutex);
        pendingNavigation = PendingNavigationRefresh;
    }
    char navigationMessage[1024]{};
    std::snprintf(
        navigationMessage,
        sizeof(navigationMessage),
        "MapSense Direct navigation: resolver=%s; engine-level=%d; engine-destinations=%zu; projected=%zu; observed-level-changes=%llu; refreshes=%llu; rooms=%llu; presets=%llu; exits=%llu; waypoints=%llu; vis-slots=%llu; vis-pairs=%llu; vis-targets-pending=%llu; partial-refreshes=%llu; published=%llu; last-level=%d; last-destinations=%u; waypoint-subtile=(%d,%d); progression-subtile=(%d,%d); pending=%s/%d/%u; unresolved names=%llu; failures=%llu; traversal limits=%llu.",
        IsNavigationResolverActive() ? "ready" : "unavailable",
        navigationEngine.levelId,
        navigationEngine.destinationCount,
        navigationEngine.projectedLineCount,
        static_cast<unsigned long long>(
            navigationEngine.observedLevelChanges),
        static_cast<unsigned long long>(navigation.refreshes),
        static_cast<unsigned long long>(navigation.rooms),
        static_cast<unsigned long long>(navigation.presets),
        static_cast<unsigned long long>(navigation.exits),
        static_cast<unsigned long long>(navigation.waypoints),
        static_cast<unsigned long long>(navigation.visibilitySlots),
        static_cast<unsigned long long>(navigation.visibilityPairs),
        static_cast<unsigned long long>(
            navigation.pendingVisibilityTargets),
        static_cast<unsigned long long>(navigation.partialRefreshes),
        static_cast<unsigned long long>(navigation.published),
        navigation.lastLevelId,
        navigation.lastDestinationCount,
        navigation.lastWaypointX,
        navigation.lastWaypointY,
        navigation.lastProgressionX,
        navigation.lastProgressionY,
        pendingNavigation.hasPendingTarget ? "yes" : "no",
        pendingNavigation.levelId,
        pendingNavigation.retriesRemaining,
        static_cast<unsigned long long>(navigation.unresolvedNames),
        static_cast<unsigned long long>(navigation.failures),
        static_cast<unsigned long long>(navigation.traversalLimits));
    context->WriteConsoleMessage(navigationMessage);
}

auto __cdecl MapSenseCommand(
        D2R::Game::Client*,
        const D2RL::ConsoleCommandContext* command,
        void*) noexcept -> D2RL::ConsoleCommandResult {
    if (command == nullptr || command->plugin == nullptr) {
        return D2RL::ConsoleCommandResult::Failed;
    }
    try {
        const auto action = TrimAndLower(command);
        if (action.empty() || action == "status") {
            WriteStatus(command->plugin);
            return D2RL::ConsoleCommandResult::Handled;
        }
        if (action == "level" || action == "zone") {
            return QueueAction(Action::RevealZone)
                ? D2RL::ConsoleCommandResult::Handled
                : D2RL::ConsoleCommandResult::Failed;
        }
        if (action == "act") {
            return QueueAction(Action::RevealAct)
                ? D2RL::ConsoleCommandResult::Handled
                : D2RL::ConsoleCommandResult::Failed;
        }
        if (action == "all") {
            return QueueAction(Action::ToggleRevealAll)
                ? D2RL::ConsoleCommandResult::Handled
                : D2RL::ConsoleCommandResult::Failed;
        }
        if (action == "off") {
            return QueueAction(Action::DisableRevealAll)
                ? D2RL::ConsoleCommandResult::Handled
                : D2RL::ConsoleCommandResult::Failed;
        }
        if (action == "menu") {
            if (!GameplayReady.load(std::memory_order_acquire)) {
                command->plugin->WriteConsoleMessage(
                    "MapSense settings are available in game only.");
                return D2RL::ConsoleCommandResult::Handled;
            }
            return QueueAction(Action::ToggleMenu)
                ? D2RL::ConsoleCommandResult::Handled
                : D2RL::ConsoleCommandResult::Failed;
        }
        command->plugin->WriteConsoleMessage(
            "Usage: mapsense [status|level|act|all|off|menu].");
        return D2RL::ConsoleCommandResult::InvalidArguments;
    } catch (...) {
        command->plugin->WriteConsoleError(
            "MapSense: unexpected command failure.");
        return D2RL::ConsoleCommandResult::Failed;
    }
}

void UnregisterServices() noexcept {
    if (Context != nullptr && InputService != nullptr
        && InputService->unregisterAction != nullptr) {
        for (auto& handle : ActionHandles) {
            if (handle != D2RL::Input::InvalidHandle) {
                (void)InputService->unregisterAction(Context, handle);
                handle = D2RL::Input::InvalidHandle;
            }
        }
    }
    if (Context != nullptr && LifecycleService != nullptr
        && LifecycleService->unregisterGameplayEventListener != nullptr) {
        for (auto& handle : LifecycleHandles) {
            if (handle != D2RL::Lifecycle::InvalidHandle) {
                (void)LifecycleService->unregisterGameplayEventListener(
                    Context, handle);
                handle = D2RL::Lifecycle::InvalidHandle;
            }
        }
    }
}

auto RegisterInputActions() noexcept -> bool {
    struct Definition {
        const char* logicalId;
        const char* displayName;
    };
    constexpr std::array definitions{
        Definition{"reveal-zone", "Reveal Current Level"},
        Definition{"reveal-act", "Reveal Current Act"},
        Definition{"toggle-reveal-all", "Toggle Reveal All Acts"},
        Definition{"toggle-settings", "Toggle MapSense Settings"},
    };

    for (std::size_t index = 0; index < definitions.size(); ++index) {
        const D2RL::Input::ActionRegistration registration{
            .structSize = D2RL::Input::ActionRegistrationSize,
            .flags = 0,
            .logicalId = definitions[index].logicalId,
            .displayName = definitions[index].displayName,
            .category = "RuffnecKk Suite",
            .defaultPrimary = {
                .key = D2RL::Input::Key::None,
                .modifier = D2RL::Input::Modifier::None,
            },
            .defaultSecondary = {
                .key = D2RL::Input::Key::None,
                .modifier = D2RL::Input::Modifier::None,
            },
            .callback = OnAction,
            .userData = &ActionTokens[index],
        };
        if (InputService->registerAction(
                Context, &registration, &ActionHandles[index])
                != D2RL::Input::Result::Success
            || ActionHandles[index] == D2RL::Input::InvalidHandle) {
            if (index + 1U == definitions.size()) {
                Context->LogWarn(
                    "MapSense: the optional settings Controls action could not be registered.");
                continue;
            }
            Context->LogError(
                "MapSense: a required reveal Controls action could not be registered.");
            return false;
        }
    }
    return true;
}

auto RegisterLifecycleListeners() noexcept -> bool {
    constexpr std::array kinds{
        D2RL::Lifecycle::GameplayEventKind::GameJoined,
        D2RL::Lifecycle::GameplayEventKind::GameLeft,
        D2RL::Lifecycle::GameplayEventKind::LocalPlayerReady,
        D2RL::Lifecycle::GameplayEventKind::ActChanged,
        D2RL::Lifecycle::GameplayEventKind::LevelChanged,
    };
    for (std::size_t index = 0; index < kinds.size(); ++index) {
        const D2RL::Lifecycle::GameplayEventListener listener{
            .structSize = D2RL::Lifecycle::GameplayEventListenerSize,
            .flags = 0,
            .kind = kinds[index],
            .reserved = 0,
            .callback = OnGameplayEvent,
            .userData = nullptr,
        };
        if (LifecycleService->registerGameplayEventListener(
                Context, &listener, &LifecycleHandles[index])
                != D2RL::Lifecycle::Result::Success
            || LifecycleHandles[index] == D2RL::Lifecycle::InvalidHandle) {
            Context->LogError(
                "MapSense: a required gameplay lifecycle listener could not be registered.");
            return false;
        }
    }
    return true;
}

void OnImGuiSettingsAction(ImGuiSettingsAction action) noexcept {
    auto requested = Action::RevealZone;
    switch (action) {
        case ImGuiSettingsAction::RevealLevel:
            requested = Action::RevealZone;
            break;
        case ImGuiSettingsAction::RevealAct:
            requested = Action::RevealAct;
            break;
        case ImGuiSettingsAction::ToggleRevealAll:
            requested = Action::ToggleRevealAll;
            break;
        case ImGuiSettingsAction::RevealAllOff:
            requested = Action::DisableRevealAll;
            break;
    }
    if (!QueueAction(requested) && Context != nullptr) {
        Context->LogWarn(
            "MapSense: the ImGui reveal control could not queue its action.");
    }
}

void __cdecl DrainSettingsSaveOnUi(
        const D2RL::PluginContext* context,
        void*) noexcept {
    for (;;) {
        std::string serialized;
        {
            std::scoped_lock lock(ConfigSaveMutex);
            if (PendingConfigSave.empty()) {
                ConfigSaveDrainScheduled = false;
                return;
            }
            serialized.swap(PendingConfigSave);
        }
        if (!Operational.load(std::memory_order_acquire)
            || context == nullptr) {
            continue;
        }
        if (!context->WriteConfig(serialized.c_str())) {
            context->LogWarn(
                "MapSense: menu settings could not be written to the active configuration scope.");
        }
    }
}

void QueueSettingsSaveFromMenu() noexcept {
    if (!Operational.load(std::memory_order_acquire)
        || Context == nullptr || ThreadService == nullptr
        || ThreadService->runOnUiThread == nullptr) {
        return;
    }

    std::string serialized;
    try {
        serialized = SerializeConfig(Settings);
    }
    catch (...) {
        Context->LogWarn(
            "MapSense: menu settings could not be serialized.");
        return;
    }

    bool schedule{};
    {
        std::scoped_lock lock(ConfigSaveMutex);
        PendingConfigSave = std::move(serialized);
        if (!ConfigSaveDrainScheduled) {
            ConfigSaveDrainScheduled = true;
            schedule = true;
        }
    }
    if (!schedule) return;
    if (ThreadService->runOnUiThread(
            Context, DrainSettingsSaveOnUi, nullptr)
        == D2RL::Threads::Result::Success) {
        return;
    }
    {
        std::scoped_lock lock(ConfigSaveMutex);
        ConfigSaveDrainScheduled = false;
    }
    Context->LogWarn(
        "MapSense: menu settings save could not be queued on the UI thread.");
}

auto DrawMapSensePanel(bool* open, void*) noexcept
        -> D3D12ImGuiPanelBounds {
    if (open == nullptr || !*open) return {};
    if (!GameplayReady.load(std::memory_order_acquire)) {
        *open = false;
        return {};
    }
    auto expanded = MenuExpanded.load(std::memory_order_acquire);
    const auto bounds = DrawImGuiSettingsPanel(
        Settings,
        expanded,
        OnImGuiSettingsAction);
    SetNativeAutomapMarkerEnabled(Settings.overlay.enabled);
    SetNativeAutomapImmunityCollectionEnabled(
        Settings.overlay.enabled && Settings.immunities.enabled);
    MenuExpanded.store(expanded, std::memory_order_release);
    if (bounds.saveRequested) QueueSettingsSaveFromMenu();
    return {
        .visible = bounds.visible,
        .left = bounds.left,
        .top = bounds.top,
        .right = bounds.right,
        .bottom = bounds.bottom,
    };
}

auto WantsMapSenseOwnedOverlay(void*) noexcept -> bool {
    const auto navigationEnabled = Settings.navigation.waypoint.enabled
        || Settings.navigation.progression.enabled
        || Settings.navigation.customLevels.enabled
        || Settings.navigation.quests.enabled;
    return Operational.load(std::memory_order_acquire)
        && GameplayReady.load(std::memory_order_acquire)
        && Settings.overlay.enabled
        && ((navigationEnabled && WantsNavigationLineFrame())
            || WantsNativeAutomapMarkerFrame());
}

auto MarkerStyleFor(MonsterRank rank) noexcept
        -> const MonsterMarkerStyle& {
    switch (rank) {
        case MonsterRank::Normal: return Settings.monsters.normal;
        case MonsterRank::Minion: return Settings.monsters.minion;
        case MonsterRank::Champion: return Settings.monsters.champion;
        case MonsterRank::Unique: return Settings.monsters.unique;
        case MonsterRank::SuperUnique:
            return Settings.monsters.superUniqueBoss;
    }
    return Settings.monsters.normal;
}

auto ToImGuiColor(RgbaColor color, float globalOpacity) noexcept -> ImU32 {
    const auto channel = [](float value) noexcept {
        return static_cast<int>(std::clamp(value, 0.0F, 1.0F) * 255.0F + 0.5F);
    };
    return IM_COL32(
        channel(color.red),
        channel(color.green),
        channel(color.blue),
        channel(color.alpha * globalOpacity));
}

[[nodiscard]] auto NavigationColorFor(
        NavigationLineKind kind,
        float opacity) noexcept -> ImU32 {
    switch (kind) {
        case NavigationLineKind::Waypoint:
            return ToImGuiColor(Settings.navigation.waypoint.color, opacity);
        case NavigationLineKind::Progression:
            return ToImGuiColor(
                Settings.navigation.progression.color,
                opacity);
        case NavigationLineKind::CustomLevel:
            return ToImGuiColor(
                Settings.navigation.customLevels.color,
                opacity);
        case NavigationLineKind::Quest:
            return ToImGuiColor(Settings.navigation.quests.color, opacity);
    }
    return ToImGuiColor(RgbaColor{1.0F, 1.0F, 1.0F, 1.0F}, opacity);
}

[[nodiscard]] auto NavigationLineEnabled(
        NavigationLineKind kind) noexcept -> bool {
    switch (kind) {
        case NavigationLineKind::Waypoint:
            return Settings.navigation.waypoint.enabled;
        case NavigationLineKind::Progression:
            return Settings.navigation.progression.enabled;
        case NavigationLineKind::CustomLevel:
            return Settings.navigation.customLevels.enabled;
        case NavigationLineKind::Quest:
            return Settings.navigation.quests.enabled;
    }
    return false;
}

void DrawNavigationLines(
        ImDrawList* drawList,
        const ImGuiIO& io,
        float opacity) noexcept {
    if (drawList == nullptr || NavigationLineSnapshots.empty()) return;
    const auto thickness = std::clamp(
        Settings.navigation.lineThickness * Settings.overlay.scale,
        MinimumNavigationLineThickness,
        MaximumNavigationLineThickness);
    for (const auto& line : NavigationLineSnapshots) {
        if (!NavigationLineEnabled(line.kind)) continue;
        if (line.nativeWidth <= 0 || line.nativeHeight <= 0) continue;
        const auto scaleX = io.DisplaySize.x
            / static_cast<float>(line.nativeWidth);
        const auto scaleY = io.DisplaySize.y
            / static_cast<float>(line.nativeHeight);
        if (!std::isfinite(scaleX) || !std::isfinite(scaleY)
            || scaleX <= 0.0F || scaleY <= 0.0F) {
            continue;
        }
        const auto scaleTolerance = std::max(
            0.01F,
            std::max(scaleX, scaleY) * 0.01F);
        if (std::abs(scaleX - scaleY) > scaleTolerance) continue;

        const ImVec2 start{
            static_cast<float>(line.startX) * scaleX,
            static_cast<float>(line.startY) * scaleY,
        };
        const ImVec2 end{
            static_cast<float>(line.endX) * scaleX,
            static_cast<float>(line.endY) * scaleY,
        };
        if (!std::isfinite(start.x) || !std::isfinite(start.y)
            || !std::isfinite(end.x) || !std::isfinite(end.y)) {
            continue;
        }
        drawList->AddLine(
            start,
            end,
            NavigationColorFor(line.kind, opacity),
            thickness);
    }
}

void DrawClosedMarkerOutline(
        ImDrawList* drawList,
        const CrossOutline& outline,
        ImU32 color,
        float thickness) noexcept {
    std::array<ImVec2, CrossOutlinePointCount> points{};
    for (std::size_t index = 0; index < outline.size(); ++index) {
        points[index] = ImVec2{outline[index].x, outline[index].y};
    }
    drawList->AddPolyline(
        points.data(),
        static_cast<int>(points.size() - 1U),
        color,
        ImDrawFlags_Closed,
        thickness);
}

[[nodiscard]] auto ImmunityColorFor(
        MonsterImmunity immunity) noexcept -> RgbaColor {
    switch (immunity) {
        case MonsterImmunity::Physical:
            return Settings.immunities.physical;
        case MonsterImmunity::Fire:
            return Settings.immunities.fire;
        case MonsterImmunity::Cold:
            return Settings.immunities.cold;
        case MonsterImmunity::Lightning:
            return Settings.immunities.lightning;
        case MonsterImmunity::Poison:
            return Settings.immunities.poison;
        case MonsterImmunity::Magic:
            return Settings.immunities.magic;
    }
    return Settings.immunities.physical;
}

[[nodiscard]] auto CollectImmunityColors(
        std::uint8_t mask,
        float opacity,
        std::array<ImU32, 6>& colors) noexcept -> std::size_t {
    constexpr std::array Immunities{
        MonsterImmunity::Physical,
        MonsterImmunity::Fire,
        MonsterImmunity::Cold,
        MonsterImmunity::Lightning,
        MonsterImmunity::Poison,
        MonsterImmunity::Magic,
    };
    std::size_t count{};
    for (const auto immunity : Immunities) {
        if ((mask & ImmunityBit(immunity)) == 0U) continue;
        colors[count++] = ToImGuiColor(
            ImmunityColorFor(immunity),
            opacity);
    }
    return count;
}

void DrawColoredImmunityIndicators(
        ImDrawList* drawList,
        ImVec2 markerCenter,
        float markerSize,
        const std::array<ImU32, 6>& colors,
        std::size_t colorCount,
        float opacity) noexcept {
    if (drawList == nullptr || colorCount == 0U) return;

    auto* const font = ImGui::GetFont();
    if (font == nullptr) return;
    const auto fontSize = std::clamp(
        Settings.immunities.indicatorSize * Settings.overlay.scale,
        MinimumImmunityIndicatorSize,
        MaximumImmunityIndicatorSize);
    const auto sampleSize = font->CalcTextSizeA(
        fontSize,
        1000.0F,
        0.0F,
        "i");
    const auto lineHeight = std::max(sampleSize.y, fontSize);
    const auto advance = ComputeColoredImmunityIndicatorAdvance(fontSize);
    const auto rowCount = (colorCount + 2U) / 3U;
    const auto shadow = ToImGuiColor(
        RgbaColor{0.0F, 0.0F, 0.0F, 0.92F},
        opacity);
    const auto startY = std::max(
        2.0F,
        markerCenter.y - (markerSize * 0.5F)
            - std::max(2.0F, Settings.overlay.scale * 2.0F)
            - static_cast<float>(rowCount) * lineHeight);

    std::size_t index{};
    for (std::size_t row = 0U; row < rowCount; ++row) {
        const auto rowItems = std::min<std::size_t>(3U, colorCount - index);
        const auto rowSpan = static_cast<float>(rowItems - 1U) * advance;
        auto x = markerCenter.x - rowSpan * 0.5F - sampleSize.x * 0.5F;
        const auto y = startY + static_cast<float>(row) * lineHeight;
        for (std::size_t column = 0U; column < rowItems; ++column) {
            drawList->AddText(
                font,
                fontSize,
                ImVec2{x + 1.0F, y + 1.0F},
                shadow,
                "i");
            drawList->AddText(
                font,
                fontSize,
                ImVec2{x, y},
                colors[index++],
                "i");
            x += advance;
        }
    }
}

void DrawSplitImmunityHalo(
        ImDrawList* drawList,
        ImVec2 markerCenter,
        float markerSize,
        const std::array<ImU32, 6>& colors,
        std::size_t colorCount,
        float opacity) noexcept {
    if (drawList == nullptr || colorCount == 0U) return;

    constexpr float Tau = 6.28318530717958647692F;
    constexpr float Top = -1.57079632679489661923F;
    const auto thickness = std::clamp(
        Settings.immunities.haloThickness * Settings.overlay.scale,
        MinimumImmunityHaloThickness,
        MaximumImmunityHaloThickness);
    const auto radius = markerSize * 0.5F
        + thickness
        + std::max(1.5F, Settings.overlay.scale * 1.5F);
    const auto backing = ToImGuiColor(
        RgbaColor{0.0F, 0.0F, 0.0F, 0.90F},
        opacity);
    drawList->AddCircle(
        markerCenter,
        radius,
        backing,
        0,
        thickness + std::max(2.0F, Settings.overlay.scale * 2.0F));

    if (colorCount == 1U) {
        drawList->AddCircle(
            markerCenter,
            radius,
            colors[0],
            0,
            thickness);
        return;
    }

    const auto segmentSpan = Tau / static_cast<float>(colorCount);
    const auto gap = std::min(0.10F, segmentSpan * 0.06F);
    for (std::size_t index = 0U; index < colorCount; ++index) {
        const auto start = Top + static_cast<float>(index) * segmentSpan
            + gap;
        const auto finish = Top + static_cast<float>(index + 1U)
            * segmentSpan - gap;
        drawList->PathArcTo(markerCenter, radius, start, finish, 0);
        drawList->PathStroke(colors[index], ImDrawFlags_None, thickness);
    }
}

void DrawMonsterImmunities(
        ImDrawList* drawList,
        const NativeAutomapMarkerSnapshot& marker,
        ImVec2 markerCenter,
        float markerSize,
        float opacity) noexcept {
    if (!Settings.immunities.enabled || marker.immunityMask == 0U) return;

    std::array<ImU32, 6> colors{};
    const auto colorCount = CollectImmunityColors(
        marker.immunityMask,
        opacity,
        colors);
    if (Settings.immunities.style == ImmunityDisplayStyle::ColoredI) {
        DrawColoredImmunityIndicators(
            drawList,
            markerCenter,
            markerSize,
            colors,
            colorCount,
            opacity);
        return;
    }
    DrawSplitImmunityHalo(
        drawList,
        markerCenter,
        markerSize,
        colors,
        colorCount,
        opacity);
}

void DrawMapSenseOwnedOverlay(void*) noexcept {
    if (!WantsMapSenseOwnedOverlay(nullptr)) return;

    const auto navigationLineCount = AcquireNavigationLineSnapshots(
        NavigationLineSnapshots);
    const auto markerCount = AcquireNativeAutomapMarkers(MarkerSnapshots);
    if (navigationLineCount == 0U && markerCount == 0U) return;

    const auto& io = ImGui::GetIO();
    if (io.DisplaySize.x <= 0.0F || io.DisplaySize.y <= 0.0F) {
        return;
    }
    const auto opacity = std::clamp(
        Settings.overlay.opacity,
        0.10F,
        1.0F);
    auto* const drawList = ImGui::GetForegroundDrawList();
    // Navigation is the map underlay. Monster markers and their immunity
    // indicators remain readable because they are always submitted after it.
    DrawNavigationLines(drawList, io, opacity);
    for (const auto& marker : MarkerSnapshots) {
        if (marker.nativeWidth <= 0 || marker.nativeHeight <= 0) continue;
        const auto scaleX = io.DisplaySize.x
            / static_cast<float>(marker.nativeWidth);
        const auto scaleY = io.DisplaySize.y
            / static_cast<float>(marker.nativeHeight);
        if (!std::isfinite(scaleX) || !std::isfinite(scaleY)
            || scaleX <= 0.0F || scaleY <= 0.0F) {
            continue;
        }
        const auto scaleTolerance = std::max(
            0.01F,
            std::max(scaleX, scaleY) * 0.01F);
        if (std::abs(scaleX - scaleY) > scaleTolerance) continue;

        const auto x = static_cast<float>(marker.x) * scaleX;
        const auto y = static_cast<float>(marker.y) * scaleY;
        if (!std::isfinite(x) || !std::isfinite(y)
            || x < 0.0F || y < 0.0F
            || x >= io.DisplaySize.x || y >= io.DisplaySize.y) {
            continue;
        }

        const auto& style = MarkerStyleFor(marker.rank);
        const auto size = std::clamp(
            style.size * Settings.overlay.scale,
            MinimumMonsterMarkerSize,
            MaximumMonsterMarkerSize);
        const auto color = ToImGuiColor(style.color, opacity);
        switch (style.shape) {
            case MonsterMarkerShape::X: {
                const auto thickness = std::clamp(
                    style.thickness * Settings.overlay.scale,
                    MinimumMonsterMarkerThickness,
                    MaximumMonsterMarkerThickness);
                DrawClosedMarkerOutline(
                    drawList,
                    BuildCrossOutline(Vec2{x, y}, size * 0.5F),
                    color,
                    thickness);
                break;
            }
            case MonsterMarkerShape::PlayerCross: {
                const auto thickness = std::clamp(
                    style.thickness * Settings.overlay.scale,
                    MinimumMonsterMarkerThickness,
                    MaximumMonsterMarkerThickness);
                const auto outline = BuildPlayerCrossOutline(Vec2{x, y}, size);
                const auto edgeThickness = thickness
                    + std::max(1.0F, Settings.overlay.scale);
                DrawClosedMarkerOutline(
                    drawList,
                    outline,
                    ToImGuiColor(
                        RgbaColor{0.02F, 0.015F, 0.01F, 0.80F},
                        opacity),
                    edgeThickness);
                DrawClosedMarkerOutline(
                    drawList,
                    outline,
                    color,
                    thickness);
                break;
            }
            case MonsterMarkerShape::Dot: {
                constexpr float OutlineThickness = 1.0F;
                const ImVec2 center{x, y};
                const auto radius = size * 0.5F;
                const auto outlineColor = ToImGuiColor(
                    RgbaColor{0.02F, 0.015F, 0.01F, 0.72F},
                    opacity);
                drawList->AddCircleFilled(
                    center,
                    radius + OutlineThickness,
                    outlineColor);
                drawList->AddCircleFilled(
                    center,
                    radius,
                    color);
                break;
            }
        }
        DrawMonsterImmunities(
            drawList,
            marker,
            ImVec2{x, y},
            size,
            opacity);
    }
}

void LogRendererInfo(const char* message) noexcept {
    if (Context != nullptr && message != nullptr) Context->LogInfo(message);
}

void LogRendererWarning(const char* message) noexcept {
    if (Context != nullptr && message != nullptr) Context->LogWarn(message);
}

void __cdecl DrainHostUiTask(
        const D2RL::PluginContext*,
        void*) noexcept {
    D3D12ImGuiUiTaskCallback task{};
    void* taskData{};
    {
        std::scoped_lock lock(HostUiTaskMutex);
        task = PendingHostUiTask;
        taskData = PendingHostUiTaskData;
        PendingHostUiTask = nullptr;
        PendingHostUiTaskData = nullptr;
        HostUiTaskScheduled = false;
    }
    if (task != nullptr) task(taskData);
}

auto QueueHostUiTask(
        D3D12ImGuiUiTaskCallback task,
        void* taskData,
        void*) noexcept -> bool {
    if (task == nullptr || Context == nullptr || ThreadService == nullptr
        || ThreadService->runOnUiThread == nullptr) {
        return false;
    }

    {
        std::scoped_lock lock(HostUiTaskMutex);
        if (HostUiTaskScheduled) return false;
        PendingHostUiTask = task;
        PendingHostUiTaskData = taskData;
        HostUiTaskScheduled = true;
    }

    if (ThreadService->runOnUiThread(Context, DrainHostUiTask, nullptr)
        == D2RL::Threads::Result::Success) {
        return true;
    }

    {
        std::scoped_lock lock(HostUiTaskMutex);
        PendingHostUiTask = nullptr;
        PendingHostUiTaskData = nullptr;
        HostUiTaskScheduled = false;
    }
    return false;
}

void OnOwnedOverlayDismissal(void*) noexcept {
    // The host serializes this callback with complete Present submissions.
    // Preserve resolver destinations and diagnostics while removing every
    // MapSense-owned automap pixel before D2R handles Tab or Escape itself.
    InvalidateNavigationProjection();
    InvalidateNativeAutomapMarkerFrame();
}

DWORD WINAPI HostRetryWorkerMain(void*) noexcept {
    while (HostRetryStopEvent != nullptr
        && WaitForSingleObject(HostRetryStopEvent, 500U) == WAIT_TIMEOUT) {
        if (TryInstallD3D12ImGuiHooks()) return 0U;
    }
    return 0U;
}

auto StartImGuiHost() noexcept -> bool {
    const D3D12ImGuiHostCallbacks callbacks{
        .drawPanel = DrawMapSensePanel,
        .drawOwnedOverlay = DrawMapSenseOwnedOverlay,
        .wantsOwnedOverlay = WantsMapSenseOwnedOverlay,
        .ownedOverlayDismissal = OnOwnedOverlayDismissal,
        .userData = nullptr,
        .queueUiTask = QueueHostUiTask,
        .uiDispatcherUserData = nullptr,
        .info = LogRendererInfo,
        .warning = LogRendererWarning,
    };
    SetD3D12ImGuiMenuOpen(false);
    if (InitializeD3D12ImGuiHost(callbacks)) return true;

    HostRetryStopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (HostRetryStopEvent == nullptr) return false;
    HostRetryWorker = CreateThread(
        nullptr, 0U, HostRetryWorkerMain, nullptr, 0U, nullptr);
    if (HostRetryWorker != nullptr) return true;
    CloseHandle(HostRetryStopEvent);
    HostRetryStopEvent = nullptr;
    return false;
}

void StopImGuiHost() noexcept {
    if (HostRetryStopEvent != nullptr) SetEvent(HostRetryStopEvent);
    if (HostRetryWorker != nullptr) {
        // The retry worker only performs bounded local D3D12 discovery. It
        // must be fully joined before this DLL can unload its hook code.
        WaitForSingleObject(HostRetryWorker, INFINITE);
        CloseHandle(HostRetryWorker);
        HostRetryWorker = nullptr;
    }
    if (HostRetryStopEvent != nullptr) {
        CloseHandle(HostRetryStopEvent);
        HostRetryStopEvent = nullptr;
    }
    ShutdownD3D12ImGuiHostAndNotifyClients();
}

enum class RendererHandoffResult {
    FloatingDamageAbsent,
    SafeForMapSense,
    Failed,
};

auto GiveMapSenseRendererPriority() noexcept -> RendererHandoffResult {
    if (const auto floatingDamage = GetModuleHandleW(
            L"d2rl-ruffneckk-floating-damage.dll")) {
        using YieldFn = bool(__cdecl*)() noexcept;
        const auto yield = reinterpret_cast<YieldFn>(GetProcAddress(
            floatingDamage,
            "RuffnecKkFloatingDamageUseMapSenseOverlayHost"));
        if (yield != nullptr && yield()) {
            LogRendererInfo(
                "MapSense: Floating Damage confirmed priority MapSense renderer ownership.");
            return RendererHandoffResult::SafeForMapSense;
        }
        LogRendererWarning(
            "MapSense: Floating Damage is present but renderer ownership could not be transferred safely.");
        return RendererHandoffResult::Failed;
    }
    return RendererHandoffResult::FloatingDamageAbsent;
}

} // namespace

extern "C" __declspec(dllexport)
const RuffnecKk::OverlayHost::HostApiV2* __cdecl
RuffnecKkMapSenseGetOverlayHostApi(
        std::uint32_t requestedVersion,
        std::uint32_t callerStructSize) noexcept {
    static const RuffnecKk::OverlayHost::HostApiV2 api{
        .structSize = RuffnecKk::OverlayHost::HostApiV2Size,
        .version = RuffnecKk::OverlayHost::ApiVersion2,
        .imguiAbiFingerprint =
            RuffnecKk::OverlayHost::ImGuiAbiFingerprint,
        .registerClient = RegisterD3D12ImGuiClientV2,
        .unregisterClient = UnregisterD3D12ImGuiClientV2,
    };
    if (!HostApiAvailable.load(std::memory_order_acquire)
        || requestedVersion != api.version
        || callerStructSize < api.structSize) {
        return nullptr;
    }
    return &api;
}

} // namespace RuffnecKk::MapSense

namespace {

constexpr D2RL::PluginInfo PluginInfo{
    .infoSize = D2RL::PluginInfoSize,
    .apiVersion = D2RL_PLUGIN_API_VERSION,
    .id = "ruffneckk-mapsense",
    .name = "RuffnecKk MapSense",
    .version = "0.12.5",
    .author = "RuffnecKk",
    .description = "Reveals maps, marks monsters, and draws direct navigation lines.",
    .flags = D2RL::PluginFlags::Client | D2RL::PluginFlags::NativeHooks,
};

} // namespace

D2RL_PLUGIN_EXPORT auto D2RLoaderGetPluginInfo() noexcept
        -> const D2RL::PluginInfo* {
    return &PluginInfo;
}

D2RL_PLUGIN_EXPORT auto D2RLoaderLoadPlugin(
        const D2RL::PluginContext* context) noexcept -> bool {
    using namespace RuffnecKk::MapSense;
    if (!D2RL::HasContext(context)
        || context->apiVersion != D2RL_PLUGIN_API_VERSION) {
        return false;
    }
    Context = context;
    Operational.store(false, std::memory_order_release);
    HostApiAvailable.store(false, std::memory_order_release);
    GameplayReady.store(false, std::memory_order_release);
    MenuExpanded.store(false, std::memory_order_release);
    MarkerAvailable.store(false, std::memory_order_release);
    SessionEpoch.store(1, std::memory_order_release);
    CurrentSessionGeneration.store(0U, std::memory_order_release);
    LastDynamicNavigationRefreshTick.store(0U, std::memory_order_release);
    {
        QueueLockGuard lock;
        ActionQueueHead = 0;
        ActionQueueSize = 0;
        ActionDrainScheduled = false;
    }
    {
        std::scoped_lock lock(ConfigSaveMutex);
        PendingConfigSave.clear();
        ConfigSaveDrainScheduled = false;
    }
    {
        std::scoped_lock lock(HostUiTaskMutex);
        PendingHostUiTask = nullptr;
        PendingHostUiTaskData = nullptr;
        HostUiTaskScheduled = false;
    }
    CancelPendingNavigationRefresh(true);
    ResetRevealReplayProcessState();
    if (!LoadConfig(Settings)) return false;
    if (!Settings.enabled) {
        context->LogInfo(
            "RuffnecKk MapSense 0.12.5 candidate is disabled; no hook or Controls action was installed.");
        return true;
    }

    try {
        MarkerSnapshots.clear();
        MarkerSnapshots.reserve(MaximumRecentNativeAutomapMarkers);
    } catch (...) {
        context->LogError(
            "MapSense: the bounded renderer marker snapshot reserve could not be allocated.");
        return false;
    }

    const auto* const buildName = D2RL::GetBuildName(context);
    char buildDiagnostic[160]{};
    std::snprintf(
        buildDiagnostic,
        sizeof(buildDiagnostic),
        "MapSense: D2R build-name '%s' is diagnostic only; native compatibility is decided by the complete fail-closed fingerprint.",
        buildName != nullptr ? buildName : "unavailable");
    context->LogInfo(buildDiagnostic);

    if (!ValidateSFillLocationDiagnosticSuppression(context)) return false;

    if (context->QueryService(
            D2RL::ServiceId::Input,
            D2RL::InputServiceV1Version,
            &InputService) != D2RL::ServiceQueryResult::Success
        || !D2RL::HasInputServiceV1Field(
            InputService, D2RL::InputServiceV1RequiredSize)
        || InputService->registerAction == nullptr
        || InputService->unregisterAction == nullptr) {
        context->LogError(
            "MapSense: D2RLoader InputService v1 is unavailable.");
        return false;
    }
    if (context->QueryService(
            D2RL::ServiceId::Thread,
            D2RL::ThreadServiceV1Version,
            &ThreadService) != D2RL::ServiceQueryResult::Success
        || !D2RL::HasThreadServiceV1Field(
            ThreadService, D2RL::ThreadServiceV1RequiredSize)
        || ThreadService->runOnUiThread == nullptr) {
        context->LogError(
            "MapSense: D2RLoader ThreadService v1 is unavailable.");
        return false;
    }
    if (!InitializeNavigationRefreshTimer()) {
        context->LogError(
            "MapSense: the bounded navigation retry timer could not be created.");
        return false;
    }
    if (!InitializeRevealReplayTimer()) {
        context->LogError(
            "MapSense: the bounded reveal persistence retry timer could not be created.");
        ShutdownNavigationRefreshTimer();
        return false;
    }
    if (context->QueryService(
            D2RL::ServiceId::Lifecycle,
            D2RL::LifecycleServiceV1Version,
            &LifecycleService) != D2RL::ServiceQueryResult::Success
        || !D2RL::HasLifecycleServiceV1Field(
            LifecycleService, D2RL::LifecycleServiceV1RequiredSize)
        || LifecycleService->registerGameplayEventListener == nullptr
        || LifecycleService->unregisterGameplayEventListener == nullptr) {
        context->LogError(
            "MapSense: D2RLoader LifecycleService v1 is unavailable.");
        ShutdownRevealReplayTimer();
        ShutdownNavigationRefreshTimer();
        return false;
    }

    if (!RegisterInputActions() || !RegisterLifecycleListeners()) {
        UnregisterServices();
        ShutdownRevealReplayTimer();
        ShutdownNavigationRefreshTimer();
        return false;
    }
    if (!InitializeRevealEngine(context, Settings.diagnostics)) {
        UnregisterServices();
        ShutdownRevealReplayTimer();
        ShutdownNavigationRefreshTimer();
        return false;
    }
    InitializeNavigationEngine();
    if (!InitializeNavigationResolver(
            context,
            Settings.diagnostics)) {
        context->LogWarn(
            "MapSense: Direct navigation is unavailable because its D2R runtime proofs did not validate; Reveal and monster markers remain active.");
    }
    const auto markerAvailable = InitializeNativeAutomapMarker(
        context,
        Settings.diagnostics);
    MarkerAvailable.store(markerAvailable, std::memory_order_release);
    if (markerAvailable) {
        SetNativeAutomapLevelObservedCallback(
            OnNativeAutomapLevelObserved,
            nullptr);
        SetNativeAutomapMarkerEnabled(Settings.overlay.enabled);
        SetNativeAutomapImmunityCollectionEnabled(
            Settings.overlay.enabled && Settings.immunities.enabled);
    } else {
        context->LogWarn(
            "MapSense: monster markers and native navigation projection are unavailable; Reveal and the settings panel remain active.");
    }
    MenuExpanded.store(
        Settings.menu.startExpanded || Settings.overlay.startMenuOpen,
        std::memory_order_release);
    HostApiAvailable.store(true, std::memory_order_release);
    if (GiveMapSenseRendererPriority()
        == RendererHandoffResult::Failed) {
        HostApiAvailable.store(false, std::memory_order_release);
        SetNativeAutomapLevelObservedCallback(nullptr, nullptr);
        ShutdownRevealReplayTimer();
        ShutdownNavigationRefreshTimer();
        ShutdownNativeAutomapMarker();
        ShutdownNavigationResolver();
        ShutdownNavigationEngine();
        StopImGuiHost();
        ShutdownRevealEngine();
        UnregisterServices();
        context->LogError(
            "MapSense: renderer startup was refused to prevent competing DirectX 12 owners.");
        return false;
    }
    if (!StartImGuiHost()) {
        HostApiAvailable.store(false, std::memory_order_release);
        SetNativeAutomapLevelObservedCallback(nullptr, nullptr);
        ShutdownRevealReplayTimer();
        ShutdownNavigationRefreshTimer();
        ShutdownNativeAutomapMarker();
        ShutdownNavigationResolver();
        ShutdownNavigationEngine();
        StopImGuiHost();
        ShutdownRevealEngine();
        UnregisterServices();
        context->LogError(
            "MapSense: the in-frame DirectX 12/ImGui host could not start or schedule a retry.");
        return false;
    }

    if (!InstallSFillLocationDiagnosticSuppression(context)) {
        HostApiAvailable.store(false, std::memory_order_release);
        SetNativeAutomapLevelObservedCallback(nullptr, nullptr);
        ShutdownRevealReplayTimer();
        ShutdownNavigationRefreshTimer();
        ShutdownNativeAutomapMarker();
        ShutdownNavigationResolver();
        ShutdownNavigationEngine();
        StopImGuiHost();
        ShutdownRevealEngine();
        UnregisterServices();
        return false;
    }

    auto registration = D2RL::MakeConsoleCommand(
        "mapsense",
        MapSenseCommand,
        "Control MapSense reveal modes and its in-game settings panel.");
    registration.usage = "mapsense [status|level|act|all|off|menu]";
    if (!context->RegisterConsoleCommand(registration)) {
        context->LogWarn(
            "MapSense: console command registration failed; Controls actions remain available.");
    }

    Operational.store(true, std::memory_order_release);
    context->LogInfo(
        "RuffnecKk MapSense 0.12.5 candidate loaded; native current-level Reveal persistence, fence-synchronized rendering, bounded live monster refresh, and exact static quest navigation are active.");
    return true;
}

D2RL_PLUGIN_EXPORT void D2RLoaderUnloadPlugin() noexcept {
    using namespace RuffnecKk::MapSense;
    Operational.store(false, std::memory_order_release);
    HostApiAvailable.store(false, std::memory_order_release);
    GameplayReady.store(false, std::memory_order_release);
    MarkerAvailable.store(false, std::memory_order_release);
    CurrentSessionGeneration.store(0U, std::memory_order_release);
    LastDynamicNavigationRefreshTick.store(0U, std::memory_order_release);
    SessionEpoch.fetch_add(1, std::memory_order_acq_rel);
    SetNativeAutomapLevelObservedCallback(nullptr, nullptr);
    CancelPendingNavigationRefresh(true);
    CancelPendingRevealReplay();
    ShutdownRevealReplayTimer();
    ShutdownNavigationRefreshTimer();
    ShutdownNativeAutomapMarker();
    ShutdownNavigationResolver();
    ShutdownNavigationEngine();
    StopImGuiHost();
    {
        std::scoped_lock lock(ConfigSaveMutex);
        PendingConfigSave.clear();
        ConfigSaveDrainScheduled = false;
    }
    {
        std::scoped_lock lock(HostUiTaskMutex);
        PendingHostUiTask = nullptr;
        PendingHostUiTaskData = nullptr;
        HostUiTaskScheduled = false;
    }
    ShutdownRevealEngine();
    UnregisterServices();
    ShutdownSFillLocationDiagnosticSuppression();
    InputService = nullptr;
    LifecycleService = nullptr;
    ThreadService = nullptr;
    Context = nullptr;
}
