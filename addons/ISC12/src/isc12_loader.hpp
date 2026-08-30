#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>

namespace D2RL {
struct PluginContext;
}

namespace ruffneckk::isc12 {

enum class LoaderInstallResult : std::uint8_t {
    Active,
    FailedBeforeMutation,
    PartialCommitColdRestartRequired,
};

template <typename ReserveTailAttempt, typename PatchTail, typename ActivateTail,
          typename PatchCap, typename PublishCap>
auto CommitLoaderMutation(
        ReserveTailAttempt&& reserveTailAttempt,
        PatchTail&& patchTail,
        ActivateTail&& activateTail,
        PatchCap&& patchCap,
        PublishCap&& publishCap) noexcept -> LoaderInstallResult {
    // The PluginSDK forwards a bool from its patch service but does not
    // guarantee that a false result left the target bytes untouched. Once a
    // native write is attempted, its relay/state lifetime therefore becomes
    // process-bound even before the call returns.
    reserveTailAttempt();
    if (!patchTail()) {
        return LoaderInstallResult::PartialCommitColdRestartRequired;
    }
    activateTail();
    if (!patchCap()) {
        return LoaderInstallResult::PartialCommitColdRestartRequired;
    }
    publishCap();
    return LoaderInstallResult::Active;
}

struct LoaderRuntimeStatus {
    bool prepared{};
    bool tailPatchInstalled{};
    bool capPatchInstalled{};
    bool operational{};
    bool coldRestartRequired{};
    std::uint64_t buildCalls{};
    std::uint64_t lastRowCount{};
    std::uint64_t lastDescriptionCount{};
};

inline auto CanEncodeRel32(
        std::uintptr_t instructionAddress,
        std::uintptr_t targetAddress) noexcept -> bool {
    if (instructionAddress
            > std::numeric_limits<std::uintptr_t>::max() - 5U) {
        return false;
    }
    const auto next = instructionAddress + 5U;
    if (targetAddress >= next) {
        return targetAddress - next
            <= static_cast<std::uintptr_t>(
                std::numeric_limits<std::int32_t>::max());
    }
    return next - targetAddress
        <= static_cast<std::uintptr_t>(
            static_cast<std::uint64_t>(
                std::numeric_limits<std::int32_t>::max())
            + 1ULL);
}

auto PrepareLoaderExtension(
    const D2RL::PluginContext* context,
    std::uint8_t* base,
    std::size_t imageSize,
    bool diagnostics,
    std::string& error) noexcept -> bool;

auto InstallLoaderExtension(std::string& error) noexcept
    -> LoaderInstallResult;

auto ShutdownLoaderExtension() noexcept -> void;
auto GetLoaderRuntimeStatus() noexcept -> LoaderRuntimeStatus;

} // namespace ruffneckk::isc12
