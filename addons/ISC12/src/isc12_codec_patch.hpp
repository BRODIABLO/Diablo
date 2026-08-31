#pragma once

#include "isc12_native_sites.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

namespace ruffneckk::isc12 {

enum class CodecPatchGroupId : std::uint8_t {
    FullItemTransport,
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
        Packet9CQueueRelayRel32,
        Packet9DQueueRelayRel32,
        Packet9CEntryRelayRel32,
        Packet9DEntryRelayRel32,
    } source{ReplacementSource::Literal};
    std::uint8_t sourceByteIndex{};
};

class LoaderCodecPatchAuthority;

// A live lease is the only acceptable proof that no native consumer can
// execute while the canonical codec transaction is preflighted and published.
// The pinned PluginSDK exposes no such authority, so production code has no
// issuer yet. This deliberately keeps every codec mutation unreachable until a
// loader-owned, documented quiescence transaction is available.
class NativePublicationQuiescenceLease final {
public:
    using ValidateFn = bool (*)(void* context) noexcept;
    using ReleaseFn = void (*)(void* context) noexcept;

    constexpr NativePublicationQuiescenceLease() noexcept = default;
    NativePublicationQuiescenceLease(
        const NativePublicationQuiescenceLease&) = delete;
    auto operator=(const NativePublicationQuiescenceLease&)
        -> NativePublicationQuiescenceLease& = delete;

    NativePublicationQuiescenceLease(
            NativePublicationQuiescenceLease&& other) noexcept
        : context_{other.context_},
          validate_{other.validate_},
          release_{other.release_} {
        other.context_ = nullptr;
        other.validate_ = nullptr;
        other.release_ = nullptr;
    }

    auto operator=(NativePublicationQuiescenceLease&& other) noexcept
            -> NativePublicationQuiescenceLease& {
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

    ~NativePublicationQuiescenceLease() { Reset(); }

    [[nodiscard]] auto IsHeld() const noexcept -> bool {
        return validate_ && validate_(context_);
    }

#if defined(ISC12_CODEC_PATCH_TESTING)
    [[nodiscard]] static auto ForTesting(
            void* context,
            ValidateFn validate,
            ReleaseFn release = nullptr) noexcept
            -> NativePublicationQuiescenceLease {
        return NativePublicationQuiescenceLease{context, validate, release};
    }
#endif

private:
    constexpr NativePublicationQuiescenceLease(
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
    [[nodiscard]] constexpr auto Packet9CQueueRelayRva()
            const noexcept -> std::uintptr_t {
        return packet9CQueueRelayRva_;
    }
    [[nodiscard]] constexpr auto Packet9DQueueRelayRva()
            const noexcept -> std::uintptr_t {
        return packet9DQueueRelayRva_;
    }
    [[nodiscard]] constexpr auto Packet9CEntryRelayRva()
            const noexcept -> std::uintptr_t {
        return packet9CEntryRelayRva_;
    }
    [[nodiscard]] constexpr auto Packet9DEntryRelayRva()
            const noexcept -> std::uintptr_t {
        return packet9DEntryRelayRva_;
    }

#if defined(ISC12_CODEC_PATCH_TESTING)
    [[nodiscard]] static constexpr auto ForTesting(
            std::uintptr_t auxiliaryReaderRelayRva,
            std::uintptr_t playerReaderRelayRva,
            std::uintptr_t playerPreviewRelayRva,
            std::uintptr_t playerSaveFinalizeRelayRva,
            std::uintptr_t packet9CQueueRelayRva,
            std::uintptr_t packet9DQueueRelayRva,
            std::uintptr_t packet9CEntryRelayRva,
            std::uintptr_t packet9DEntryRelayRva) noexcept
            -> CodecPatchActivationTargets {
        return CodecPatchActivationTargets{
            auxiliaryReaderRelayRva,
            playerReaderRelayRva,
            playerPreviewRelayRva,
            playerSaveFinalizeRelayRva,
            packet9CQueueRelayRva,
            packet9DQueueRelayRva,
            packet9CEntryRelayRva,
            packet9DEntryRelayRva};
    }
#endif

private:
    // Only the loader authority may bind this contract to the copied,
    // process-lifetime RX entries prepared from the governed relay template.
    explicit constexpr CodecPatchActivationTargets(
            std::uintptr_t auxiliaryReaderRelayRva,
            std::uintptr_t playerReaderRelayRva,
            std::uintptr_t playerPreviewRelayRva,
            std::uintptr_t playerSaveFinalizeRelayRva,
            std::uintptr_t packet9CQueueRelayRva,
            std::uintptr_t packet9DQueueRelayRva,
            std::uintptr_t packet9CEntryRelayRva,
            std::uintptr_t packet9DEntryRelayRva) noexcept
        : auxiliaryReaderRelayRva_{auxiliaryReaderRelayRva},
          playerReaderRelayRva_{playerReaderRelayRva},
          playerPreviewRelayRva_{playerPreviewRelayRva},
          playerSaveFinalizeRelayRva_{playerSaveFinalizeRelayRva},
          packet9CQueueRelayRva_{packet9CQueueRelayRva},
          packet9DQueueRelayRva_{packet9DQueueRelayRva},
          packet9CEntryRelayRva_{packet9CEntryRelayRva},
          packet9DEntryRelayRva_{packet9DEntryRelayRva} {}

    std::uintptr_t auxiliaryReaderRelayRva_{};
    std::uintptr_t playerReaderRelayRva_{};
    std::uintptr_t playerPreviewRelayRva_{};
    std::uintptr_t playerSaveFinalizeRelayRva_{};
    std::uintptr_t packet9CQueueRelayRva_{};
    std::uintptr_t packet9DQueueRelayRva_{};
    std::uintptr_t packet9CEntryRelayRva_{};
    std::uintptr_t packet9DEntryRelayRva_{};

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
    WitnessOverlapsMutation,
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
using ReserveCodecMutationLifetimeFn = void (*)(void* context) noexcept;

struct CodecPatchCallbacks {
    void* context{};
    VerifyCodecPatternFn verifyPattern{};
    WriteCodecByteFn writeByte{};
    FlushCodecInstructionCacheFn flushInstructionCache{};
    // Called after the complete quiescent preflight and immediately before
    // the first native write attempt. The prepared relay/state and registered
    // unwind table must then remain valid for the lifetime of the process,
    // even when the write callback reports failure.
    ReserveCodecMutationLifetimeFn reserveMutationLifetime{};
};

auto PreparedCodecPatchGroups() noexcept -> std::span<const CodecPatchGroup>;
auto ValidateCodecPatchGroup(const CodecPatchGroup& group) noexcept
    -> CodecPatchPlanError;

// Only the complete canonical G9/G1-G4 set can be committed. A live,
// non-forgeable
// quiescence lease must span every fingerprint check, write and cache flush. A
// false write/flush or a lease lost after the first attempted mutation is an
// uncertain native mutation and must force a cold restart; rollback cannot
// safely guess which bytes were published.
auto CommitPreparedCodecPatchSet(
    const NativePublicationQuiescenceLease& quiescence,
    const CodecPatchActivationTargets& activationTargets,
    const CodecPatchCallbacks& callbacks) noexcept -> CodecPatchCommitResult;

} // namespace ruffneckk::isc12
