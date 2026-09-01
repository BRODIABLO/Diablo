#pragma once

#include <cstdint>
#include <string_view>

namespace ruffneckk::scripted_ai {

enum class OwnershipState : std::uint32_t {
    Unchanged,
    Tracked,
    Untracked,
};

enum class OwnershipKind : std::uint32_t {
    Unknown,
    BytePatch,
    InlineHook,
    Multiple,
};

struct OwnershipObservation {
    OwnershipState state{OwnershipState::Untracked};
    OwnershipKind kind{OwnershipKind::Unknown};
    std::uint32_t ownerCount{};
    std::string_view ownerPluginId;
};

[[nodiscard]] constexpr auto IsSafeBeforeInstall(
        const OwnershipObservation& observation) noexcept -> bool {
    return observation.state == OwnershipState::Unchanged
        && observation.ownerCount == 0U
        && observation.ownerPluginId.empty();
}

[[nodiscard]] constexpr auto IsExclusivelyOwnedAfterInstall(
        const OwnershipObservation& observation,
        std::string_view ownPluginId) noexcept -> bool {
    return !ownPluginId.empty()
        && observation.state == OwnershipState::Tracked
        && observation.kind == OwnershipKind::InlineHook
        && observation.ownerCount == 1U
        && observation.ownerPluginId == ownPluginId;
}

} // namespace ruffneckk::scripted_ai
