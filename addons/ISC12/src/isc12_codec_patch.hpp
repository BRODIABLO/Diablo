#pragma once

#include "isc12_native_sites.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

namespace ruffneckk::isc12 {

enum class CodecPatchGroupId : std::uint8_t {
    AuxiliaryPlayer,
    PlayerSave,
    PlayerPreview,
};

struct CodecByteMutation {
    std::size_t patternOffset{};
    std::uint8_t expected{};
    std::uint8_t replacement{};
    enum class ReplacementSource : std::uint8_t {
        Literal,
        PlayerSaveFinalizeRel32,
    } source{ReplacementSource::Literal};
    std::uint8_t sourceByteIndex{};
};

class LoaderCodecPatchAuthority;

class CodecPatchActivationTargets {
public:
    constexpr CodecPatchActivationTargets() noexcept = default;

    [[nodiscard]] constexpr auto PlayerSaveFinalizeRelayRva()
            const noexcept -> std::uintptr_t {
        return playerSaveFinalizeRelayRva_;
    }

#if defined(ISC12_CODEC_PATCH_TESTING)
    [[nodiscard]] static constexpr auto ForTesting(
            std::uintptr_t playerSaveFinalizeRelayRva) noexcept
            -> CodecPatchActivationTargets {
        return CodecPatchActivationTargets{playerSaveFinalizeRelayRva};
    }
#endif

private:
    // Only the loader authority may bind this contract to the copied,
    // process-lifetime RX leaf prepared from the governed relay template.
    explicit constexpr CodecPatchActivationTargets(
            std::uintptr_t playerSaveFinalizeRelayRva) noexcept
        : playerSaveFinalizeRelayRva_{playerSaveFinalizeRelayRva} {}

    std::uintptr_t playerSaveFinalizeRelayRva_{};

    friend class LoaderCodecPatchAuthority;
};

struct CodecPatchSite {
    NativePattern pattern{};
    std::span<const CodecByteMutation> mutations{};
};

struct CodecPatchGroup {
    CodecPatchGroupId id{};
    const char* name{};
    std::span<const CodecPatchSite> sites{};
    // Unchanged owner/capacity/error-path fingerprints that must match before
    // any byte in this group may be published.
    std::span<const NativePattern> witnesses{};
};

enum class CodecPatchPlanError : std::uint8_t {
    None,
    EmptyGroup,
    InvalidPattern,
    InvalidMutation,
    DuplicateMutation,
    InvalidActivationTarget,
};

enum class CodecPatchCommitStatus : std::uint8_t {
    Active,
    InvalidPlan,
    QuiescenceRequired,
    PreflightFailed,
    PartialCommitColdRestartRequired,
};

struct CodecPatchCommitResult {
    CodecPatchCommitStatus status{CodecPatchCommitStatus::InvalidPlan};
    CodecPatchPlanError planError{CodecPatchPlanError::None};
    std::size_t attemptedMutations{};
    std::size_t confirmedMutations{};
    std::size_t confirmedNoOpMutations{};
    std::size_t confirmedFlushes{};
};

using VerifyCodecPatternFn = bool (*)(
    void* context,
    const NativePattern& pattern) noexcept;
using WriteCodecByteFn = bool (*)(
    void* context,
    std::uintptr_t rva,
    std::uint8_t expected,
    std::uint8_t replacement) noexcept;
using FlushCodecInstructionCacheFn = bool (*)(
    void* context,
    std::uintptr_t firstRva,
    std::size_t size) noexcept;

struct CodecPatchCallbacks {
    void* context{};
    VerifyCodecPatternFn verifyPattern{};
    WriteCodecByteFn writeByte{};
    FlushCodecInstructionCacheFn flushInstructionCache{};
};

auto PreparedCodecPatchGroups() noexcept -> std::span<const CodecPatchGroup>;
auto ValidateCodecPatchGroup(const CodecPatchGroup& group) noexcept
    -> CodecPatchPlanError;

// Only the complete canonical G2-G4 set can be committed. The caller must
// prove publication quiescence before invoking any writer. A false write or
// instruction-cache flush is an uncertain native mutation and must force a
// cold restart; rollback cannot safely guess which bytes were published.
auto CommitPreparedCodecPatchSet(
    bool publicationQuiescent,
    const CodecPatchActivationTargets& activationTargets,
    const CodecPatchCallbacks& callbacks) noexcept -> CodecPatchCommitResult;

} // namespace ruffneckk::isc12
