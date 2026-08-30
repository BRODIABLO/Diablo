#pragma once

#include "isc12_native_sites.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

namespace ruffneckk::isc12 {

enum class CodecPatchGroupId : std::uint8_t {
    GenericItem,
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
        AuxiliaryReaderRelayRel32,
        PlayerReaderRelayRel32,
        PlayerPreviewRelayRel32,
        PlayerSaveFinalizeRel32,
    } source{ReplacementSource::Literal};
    std::uint8_t sourceByteIndex{};
};

class LoaderCodecPatchAuthority;

// A live lease is the only acceptable proof that no native consumer can
// execute while the canonical codec transaction is preflighted and published.
// The pinned PluginSDK exposes no such authority, so production code has no
// issuer yet. This deliberately keeps every codec mutation unreachable until a
// loader-owned, documented quiescence transaction is available.
class CodecPublicationQuiescenceLease final {
public:
    using ValidateFn = bool (*)(void* context) noexcept;
    using ReleaseFn = void (*)(void* context) noexcept;

    constexpr CodecPublicationQuiescenceLease() noexcept = default;
    CodecPublicationQuiescenceLease(
        const CodecPublicationQuiescenceLease&) = delete;
    auto operator=(const CodecPublicationQuiescenceLease&)
        -> CodecPublicationQuiescenceLease& = delete;

    CodecPublicationQuiescenceLease(
            CodecPublicationQuiescenceLease&& other) noexcept
        : context_{other.context_},
          validate_{other.validate_},
          release_{other.release_} {
        other.context_ = nullptr;
        other.validate_ = nullptr;
        other.release_ = nullptr;
    }

    auto operator=(CodecPublicationQuiescenceLease&& other) noexcept
            -> CodecPublicationQuiescenceLease& {
        if (this == &other) return *this;
        Reset();
        context_ = other.context_;
        validate_ = other.validate_;
        release_ = other.release_;
        other.context_ = nullptr;
        other.validate_ = nullptr;
        other.release_ = nullptr;
        return *this;
    }

    ~CodecPublicationQuiescenceLease() { Reset(); }

    [[nodiscard]] auto IsHeld() const noexcept -> bool {
        return validate_ && validate_(context_);
    }

#if defined(ISC12_CODEC_PATCH_TESTING)
    [[nodiscard]] static auto ForTesting(
            void* context,
            ValidateFn validate,
            ReleaseFn release = nullptr) noexcept
            -> CodecPublicationQuiescenceLease {
        return CodecPublicationQuiescenceLease{context, validate, release};
    }
#endif

private:
    constexpr CodecPublicationQuiescenceLease(
            void* context,
            ValidateFn validate,
            ReleaseFn release) noexcept
        : context_{context}, validate_{validate}, release_{release} {}

    void Reset() noexcept {
        if (release_) release_(context_);
        context_ = nullptr;
        validate_ = nullptr;
        release_ = nullptr;
    }

    void* context_{};
    ValidateFn validate_{};
    ReleaseFn release_{};
};

class CodecPatchActivationTargets {
public:
    constexpr CodecPatchActivationTargets() noexcept = default;

    [[nodiscard]] constexpr auto PlayerSaveFinalizeRelayRva()
            const noexcept -> std::uintptr_t {
        return playerSaveFinalizeRelayRva_;
    }
    [[nodiscard]] constexpr auto AuxiliaryReaderRelayRva()
            const noexcept -> std::uintptr_t {
        return auxiliaryReaderRelayRva_;
    }
    [[nodiscard]] constexpr auto PlayerReaderRelayRva()
            const noexcept -> std::uintptr_t {
        return playerReaderRelayRva_;
    }
    [[nodiscard]] constexpr auto PlayerPreviewRelayRva()
            const noexcept -> std::uintptr_t {
        return playerPreviewRelayRva_;
    }

#if defined(ISC12_CODEC_PATCH_TESTING)
    [[nodiscard]] static constexpr auto ForTesting(
            std::uintptr_t auxiliaryReaderRelayRva,
            std::uintptr_t playerReaderRelayRva,
            std::uintptr_t playerPreviewRelayRva,
            std::uintptr_t playerSaveFinalizeRelayRva) noexcept
            -> CodecPatchActivationTargets {
        return CodecPatchActivationTargets{
            auxiliaryReaderRelayRva,
            playerReaderRelayRva,
            playerPreviewRelayRva,
            playerSaveFinalizeRelayRva};
    }
#endif

private:
    // Only the loader authority may bind this contract to the copied,
    // process-lifetime RX entries prepared from the governed relay template.
    explicit constexpr CodecPatchActivationTargets(
            std::uintptr_t auxiliaryReaderRelayRva,
            std::uintptr_t playerReaderRelayRva,
            std::uintptr_t playerPreviewRelayRva,
            std::uintptr_t playerSaveFinalizeRelayRva) noexcept
        : auxiliaryReaderRelayRva_{auxiliaryReaderRelayRva},
          playerReaderRelayRva_{playerReaderRelayRva},
          playerPreviewRelayRva_{playerPreviewRelayRva},
          playerSaveFinalizeRelayRva_{playerSaveFinalizeRelayRva} {}

    std::uintptr_t auxiliaryReaderRelayRva_{};
    std::uintptr_t playerReaderRelayRva_{};
    std::uintptr_t playerPreviewRelayRva_{};
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

// Only the complete canonical G1-G4 set can be committed. A live, non-forgeable
// quiescence lease must span every fingerprint check, write and cache flush. A
// false write/flush or a lease lost after the first attempted mutation is an
// uncertain native mutation and must force a cold restart; rollback cannot
// safely guess which bytes were published.
auto CommitPreparedCodecPatchSet(
    const CodecPublicationQuiescenceLease& quiescence,
    const CodecPatchActivationTargets& activationTargets,
    const CodecPatchCallbacks& callbacks) noexcept -> CodecPatchCommitResult;

} // namespace ruffneckk::isc12
