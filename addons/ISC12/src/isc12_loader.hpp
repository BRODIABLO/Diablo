#pragma once

#include "isc12_codec_patch.hpp"
#include "isc12_envelope.hpp"

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
    QuiescenceRequired,
    PartialCommitColdRestartRequired,
};

template <typename ReserveTailAttempt, typename PatchTail, typename ActivateTail,
          typename PatchCap, typename PublishCap>
auto CommitLoaderMutation(
        const NativePublicationQuiescenceLease& quiescence,
        ReserveTailAttempt&& reserveTailAttempt,
        PatchTail&& patchTail,
        ActivateTail&& activateTail,
        PatchCap&& patchCap,
        PublishCap&& publishCap) noexcept -> LoaderInstallResult {
    if (!quiescence.IsHeld()) {
        return LoaderInstallResult::QuiescenceRequired;
    }
    // The PluginSDK forwards a bool from its patch service but does not
    // guarantee that a false result left the target bytes untouched. Once a
    // native write is attempted, its relay/state lifetime therefore becomes
    // process-bound even before the call returns.
    reserveTailAttempt();
    if (!quiescence.IsHeld()) {
        return LoaderInstallResult::QuiescenceRequired;
    }
    if (!patchTail()) {
        return LoaderInstallResult::PartialCommitColdRestartRequired;
    }
    if (!quiescence.IsHeld()) {
        return LoaderInstallResult::PartialCommitColdRestartRequired;
    }
    activateTail();
    if (!quiescence.IsHeld()) {
        return LoaderInstallResult::PartialCommitColdRestartRequired;
    }
    if (!patchCap()) {
        return LoaderInstallResult::PartialCommitColdRestartRequired;
    }
    if (!quiescence.IsHeld()) {
        return LoaderInstallResult::PartialCommitColdRestartRequired;
    }
    publishCap();
    return LoaderInstallResult::Active;
}

struct LoaderRuntimeStatus {
    bool prepared{};
    bool persistencePrepared{};
    bool tailPatchInstalled{};
    bool capPatchInstalled{};
    bool operational{};
    bool schemaReady{};
    bool persistenceCodecReady{};
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

auto InstallLoaderExtension(
    const NativePublicationQuiescenceLease& quiescence,
    std::string& error) noexcept
    -> LoaderInstallResult;

auto ShutdownLoaderExtension() noexcept -> void;
auto GetLoaderRuntimeStatus() noexcept -> LoaderRuntimeStatus;
auto TryGetPublishedSchemaHash(Sha256Digest& output) noexcept -> bool;

} // namespace ruffneckk::isc12
