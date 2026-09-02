#include "scripted_ai_executor.hpp"

#include <algorithm>
#include <chrono>

namespace ruffneckk::scripted_ai {

auto ReadMicroseconds(const ThinkTiming& timing) noexcept -> std::uint64_t {
    if (timing.now != nullptr) return timing.now(timing.userData);
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch())
            .count());
}

EphemeralThinkHandle::EphemeralThinkHandle(
        std::uint64_t sessionGeneration,
        std::uint64_t thinkToken,
        ThinkSnapshot snapshot,
        ThinkCapabilities& capabilities,
        ThinkTiming timing) noexcept
    : sessionGeneration_(sessionGeneration),
      thinkToken_(thinkToken),
      snapshot_(snapshot),
      capabilities_(&capabilities),
      timing_(timing) {
    if (sessionGeneration_ == 0U || thinkToken_ == 0U) valid_ = false;
}

EphemeralThinkHandle::~EphemeralThinkHandle() noexcept {
    Invalidate();
}

auto EphemeralThinkHandle::SessionGeneration() const noexcept
        -> std::uint64_t {
    return sessionGeneration_;
}

auto EphemeralThinkHandle::ThinkToken() const noexcept -> std::uint64_t {
    return thinkToken_;
}

auto EphemeralThinkHandle::Snapshot() const noexcept -> ThinkSnapshot {
    return snapshot_;
}

auto EphemeralThinkHandle::IsValid() const noexcept -> bool {
    return valid_ && capabilities_ != nullptr;
}

auto EphemeralThinkHandle::TryAction(const ActionIntent& intent) noexcept
        -> CapabilityResult {
    if (!IsValid()) {
        staleAccess_ = true;
        return CapabilityResult::Rejected;
    }
    if (committed_ || fallbackScheduled_) {
        secondActionAttempt_ = true;
        return CapabilityResult::Rejected;
    }
    if (!IsValidActionIntent(intent)) {
        capabilityError_ = true;
        return CapabilityResult::Error;
    }

    ++actionAttempts_;
    const auto before = ReadMicroseconds(timing_);
    const auto result = capabilities_->TryAction(intent);
    const auto after = ReadMicroseconds(timing_);
    if (after >= before) {
        capabilityMicroseconds_ += after - before;
    }

    if (result == CapabilityResult::Accepted) {
        committedAction_ = intent;
        committed_ = true;
    } else if (result == CapabilityResult::FallbackScheduled) {
        fallbackScheduled_ = true;
    } else if (result == CapabilityResult::Error) {
        capabilityError_ = true;
    }
    return result;
}

void EphemeralThinkHandle::Invalidate() noexcept {
    valid_ = false;
    capabilities_ = nullptr;
}

auto EphemeralThinkHandle::HasCommittedAction() const noexcept -> bool {
    return committed_;
}

auto EphemeralThinkHandle::CommittedAction() const noexcept -> ActionIntent {
    return committedAction_;
}

auto EphemeralThinkHandle::HasScheduledFallback() const noexcept -> bool {
    return fallbackScheduled_;
}

auto EphemeralThinkHandle::ActionAttempts() const noexcept -> std::uint32_t {
    return actionAttempts_;
}

auto EphemeralThinkHandle::CapabilityMicroseconds() const noexcept
        -> std::uint64_t {
    return capabilityMicroseconds_;
}

auto EphemeralThinkHandle::HadCapabilityError() const noexcept -> bool {
    return capabilityError_;
}

auto EphemeralThinkHandle::HadStaleAccess() const noexcept -> bool {
    return staleAccess_;
}

auto EphemeralThinkHandle::HadSecondActionAttempt() const noexcept -> bool {
    return secondActionAttempt_;
}

} // namespace ruffneckk::scripted_ai
