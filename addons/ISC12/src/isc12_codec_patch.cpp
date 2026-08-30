#include "isc12_codec_patch.hpp"

#include <array>
#include <limits>

namespace ruffneckk::isc12 {
namespace {

inline constexpr std::array WidthAndSentinelMutations{
    CodecByteMutation{1, 0x09, 0x0C},
    CodecByteMutation{19, 0x01, 0x0F},
};
inline constexpr std::array WriterIdMutation{
    CodecByteMutation{2, 0x09, 0x0C},
};
inline constexpr std::array WriterTerminatorMutations{
    CodecByteMutation{2, 0x01, 0x0F},
    CodecByteMutation{12, 0x09, 0x0C},
};
inline constexpr std::array PreviewWidthAndSentinelMutations{
    CodecByteMutation{1, 0x09, 0x0C},
    CodecByteMutation{17, 0x01, 0x0F},
};
inline constexpr std::array PlayerSaveRelayCallMutations{
    CodecByteMutation{6, 0x49, 0x00,
        CodecByteMutation::ReplacementSource::PlayerSaveFinalizeRel32, 0},
    CodecByteMutation{7, 0x62, 0x00,
        CodecByteMutation::ReplacementSource::PlayerSaveFinalizeRel32, 1},
    CodecByteMutation{8, 0x4E, 0x00,
        CodecByteMutation::ReplacementSource::PlayerSaveFinalizeRel32, 2},
    CodecByteMutation{9, 0x00, 0x00,
        CodecByteMutation::ReplacementSource::PlayerSaveFinalizeRel32, 3},
};
inline constexpr std::array OverflowGuardStatusMutations{
    CodecByteMutation{11, 0x33, 0x8B},
    CodecByteMutation{12, 0xC0, 0xC2},
};

inline constexpr std::uintptr_t PlayerSaveFinalizeCallNextRva = 0x5353C7;
inline constexpr std::array<std::uint8_t, 4>
    NativePlayerSaveUsedEndDisplacement{0x49, 0x62, 0x4E, 0x00};

inline constexpr std::array G2Sites{
    CodecPatchSite{
        NativePattern{"codec.g2-reader-first", 0x530A99,
            AuxPlayerReaderFirstBytes, AuxPlayerReaderFirstMask},
        WidthAndSentinelMutations},
    CodecPatchSite{
        NativePattern{"codec.g2-reader-next", 0x530BA3,
            AuxPlayerReaderNextBytes, AuxPlayerReaderNextMask},
        WidthAndSentinelMutations},
    CodecPatchSite{
        NativePattern{"codec.g2-writer-id", 0x5340C0,
            AuxPlayerWriterIdBytes, AuxPlayerWriterIdMask},
        WriterIdMutation},
    CodecPatchSite{
        NativePattern{"codec.g2-writer-terminator", 0x534139,
            AuxPlayerWriterTerminatorBytes, AuxPlayerWriterTerminatorMask},
        WriterTerminatorMutations},
};
inline constexpr std::array G2Witnesses{
    NativePattern{"codec.g2-reader-owner", 0x530A00,
        AuxPlayerReaderOwnerBytes, AuxPlayerReaderOwnerMask},
    NativePattern{"codec.g2-reader-marker", 0x530A6B,
        AuxPlayerReaderMarkerBytes, AuxPlayerReaderMarkerMask},
    NativePattern{"codec.g2-reader-fields", 0x530B69,
        AuxPlayerReaderFieldsBytes, AuxPlayerReaderFieldsMask},
    NativePattern{"codec.g2-buffer-allocation", 0x534006,
        AuxPlayerBufferAllocationBytes, AuxPlayerBufferAllocationMask},
    NativePattern{"codec.g2-count-cap-source", 0x533EAD,
        AuxPlayerCountCapSourceBytes, AuxPlayerCountCapSourceMask},
    NativePattern{"codec.g2-count-cap-use", 0x53405C,
        AuxPlayerCountCapUseBytes, AuxPlayerCountCapUseMask},
    NativePattern{"codec.g2-param-guard", 0x5340D2,
        AuxPlayerParamGuardBytes, AuxPlayerParamGuardMask},
    NativePattern{"codec.g2-param-value-writes", 0x5340FD,
        AuxPlayerParamAndValueWritesBytes,
        AuxPlayerParamAndValueWritesMask},
};

inline constexpr std::array G3Sites{
    CodecPatchSite{
        NativePattern{"codec.g3-reader-first", 0x53395E,
            PlayerReaderFirstBytes, PlayerReaderFirstMask},
        WidthAndSentinelMutations},
    CodecPatchSite{
        NativePattern{"codec.g3-reader-next", 0x533A93,
            PlayerReaderNextBytes, PlayerReaderNextMask},
        WidthAndSentinelMutations},
    CodecPatchSite{
        NativePattern{"codec.g3-writer-id", 0x5352F6,
            PlayerWriterIdBytes, PlayerWriterIdMask},
        WriterIdMutation},
    CodecPatchSite{
        NativePattern{"codec.g3-writer-terminator", 0x5353A8,
            PlayerWriterTerminatorBytes, PlayerWriterTerminatorMask},
        WriterTerminatorMutations},
    CodecPatchSite{
        NativePattern{"codec.g3-writer-relay-call", 0x5353BD,
            PlayerSaveWriterRelayCallBytes,
            PlayerSaveWriterRelayCallMask},
        PlayerSaveRelayCallMutations},
    // Publish the overflow result only after the relay call has been fully
    // written and its instruction-cache range has been flushed.
    CodecPatchSite{
        NativePattern{"codec.g3-writer-status-publish", 0x5353C7,
            PlayerSaveWriterStatusPublishBytes,
            PlayerSaveWriterStatusPublishMask},
        OverflowGuardStatusMutations},
};
inline constexpr std::array G3Witnesses{
    NativePattern{"codec.g3-reader-owner", 0x533760,
        PlayerReaderOwnerBytes, PlayerReaderOwnerMask},
    NativePattern{"codec.g3-reader-marker", 0x533924,
        PlayerReaderMarkerBytes, PlayerReaderMarkerMask},
    NativePattern{"codec.g3-reader-param-fields", 0x533A38,
        PlayerReaderParamFieldsBytes, PlayerReaderParamFieldsMask},
    NativePattern{"codec.g3-reader-value-fields", 0x533A52,
        PlayerReaderValueFieldsBytes, PlayerReaderValueFieldsMask},
    NativePattern{"codec.g3-buffer-origin", 0x52F0C3,
        PlayerSaveBufferOriginBytes, PlayerSaveBufferOriginMask},
    NativePattern{"codec.g3-buffer-window", 0x52F0E7,
        PlayerSaveBufferWindowBytes, PlayerSaveBufferWindowMask},
    NativePattern{"codec.g3-writer-setup", 0x52F4E9,
        PlayerSaveWriterSetupBytes, PlayerSaveWriterSetupMask},
    NativePattern{"codec.g3-writer-status-check", 0x52F522,
        PlayerSaveWriterStatusCheckBytes, PlayerSaveWriterStatusCheckMask},
    NativePattern{"codec.g3-writer-initial-bound", 0x535115,
        PlayerSaveWriterInitialBoundBytes,
        PlayerSaveWriterInitialBoundMask},
    NativePattern{"codec.g3-count-cap-use", 0x535162,
        PlayerSaveCountCapUseBytes, PlayerSaveCountCapUseMask},
    NativePattern{"codec.g3-bitwriter-initialization", 0xA1B650,
        BitWriterInitializationBytes, BitWriterInitializationMask},
    NativePattern{"codec.g3-bitwriter-overrun", 0xA1B72C,
        BitWriterOverrunFlagBytes, BitWriterOverrunFlagMask},
    NativePattern{"codec.g3-bitwriter-core", 0xA1B7A0,
        BitWriterSuccessfulCoreBytes, BitWriterSuccessfulCoreMask},
    NativePattern{"codec.g3-bitwriter-used-end", 0xA1B610,
        BitWriterUsedEndBytes, BitWriterUsedEndMask},
    NativePattern{"codec.g3-csvbits-32-bypass", 0x5351DD,
        PlayerSaveCsvBits32BypassBytes,
        PlayerSaveCsvBits32BypassMask},
    NativePattern{"codec.g3-csvbits-clamp", 0x5352DB,
        PlayerSaveCsvBitsClampBytes, PlayerSaveCsvBitsClampMask},
    NativePattern{"codec.g3-csvparambits-16-guard", 0x535308,
        PlayerSaveCsvParamBits16GuardBytes,
        PlayerSaveCsvParamBits16GuardMask},
    NativePattern{"codec.g3-param-value-writes", 0x535352,
        PlayerSaveParamAndValueWritesBytes,
        PlayerSaveParamAndValueWritesMask},
    NativePattern{"codec.g3-bitreader-signed-wrapper", 0xA1B680,
        BitReaderSignedWrapperBytes, BitReaderSignedWrapperMask},
    NativePattern{"codec.g3-stack-caller-capacity", 0x41360B,
        PlayerSaveStackCallerCapacityBytes,
        PlayerSaveStackCallerCapacityMask},
    NativePattern{"codec.g3-dynamic-capacity", 0x41E138,
        PlayerSaveDynamicCapacityBytes, PlayerSaveDynamicCapacityMask},
    NativePattern{"codec.g3-dynamic-allocation", 0x41E186,
        PlayerSaveDynamicAllocationBytes,
        PlayerSaveDynamicAllocationMask},
    NativePattern{"codec.g3-dynamic-call", 0x41E1E9,
        PlayerSaveDynamicCallBytes, PlayerSaveDynamicCallMask},
};

inline constexpr std::array G4Sites{
    CodecPatchSite{
        NativePattern{"codec.g4-preview-a-first", 0x61D247,
            PreviewReaderAFirstBytes, PreviewReaderAFirstMask},
        PreviewWidthAndSentinelMutations},
    CodecPatchSite{
        NativePattern{"codec.g4-preview-a-next", 0x61D290,
            PreviewReaderANextBytes, PreviewReaderANextMask},
        PreviewWidthAndSentinelMutations},
    CodecPatchSite{
        NativePattern{"codec.g4-preview-b-first", 0x61D647,
            PreviewReaderBFirstBytes, PreviewReaderBFirstMask},
        PreviewWidthAndSentinelMutations},
    CodecPatchSite{
        NativePattern{"codec.g4-preview-b-next", 0x61D690,
            PreviewReaderBNextBytes, PreviewReaderBNextMask},
        PreviewWidthAndSentinelMutations},
};
inline constexpr std::array G4Witnesses{
    NativePattern{"codec.g4-preview-decoder", 0x61CEF0,
        PlayerPreviewDecoderEntryBytes, PlayerPreviewDecoderEntryMask},
    NativePattern{"codec.g4-preview-dispatch", 0x61CFFF,
        PlayerPreviewVersionDispatcherBytes,
        PlayerPreviewVersionDispatcherMask},
    NativePattern{"codec.g4-preview-buffer-allocation", 0x61CF41,
        PlayerPreviewBufferAllocationBytes,
        PlayerPreviewBufferAllocationMask},
    NativePattern{"codec.g4-preview-buffer-read", 0x61CF81,
        PlayerPreviewBufferReadBytes, PlayerPreviewBufferReadMask},
    NativePattern{"codec.g4-bitreader-thunk", 0xA1B6C0,
        BitReaderThunkBytes, BitReaderThunkMask},
    NativePattern{"codec.g4-bitreader-overrun", 0xA1BA92,
        BitReaderOverrunFlagBytes, BitReaderOverrunFlagMask},
    NativePattern{"codec.g4-bitreader-core", 0xA1BAD0,
        BitReaderSuccessfulCoreBytes, BitReaderSuccessfulCoreMask},
    NativePattern{"codec.g4-inner-vanilla-magic", 0x61CF95,
        PlayerPreviewInnerVanillaMagicBytes,
        PlayerPreviewInnerVanillaMagicMask},
};

inline constexpr std::array CodecGroups{
    CodecPatchGroup{
        CodecPatchGroupId::AuxiliaryPlayer,
        "G2-aux-player-codec",
        G2Sites,
        G2Witnesses},
    CodecPatchGroup{
        CodecPatchGroupId::PlayerPreview,
        "G4-player-preview-codec",
        G4Sites,
        G4Witnesses},
    // Keep G3 last so its overflow-status publication is the final site in a
    // whole-codec transaction.
    CodecPatchGroup{
        CodecPatchGroupId::PlayerSave,
        "G3-player-save-codec",
        G3Sites,
        G3Witnesses},
};

auto AbsoluteMutationRva(
        const CodecPatchSite& site,
        const CodecByteMutation& mutation,
        std::uintptr_t& output) noexcept -> bool {
    if (mutation.patternOffset > (std::numeric_limits<std::uintptr_t>::max)()
            - site.pattern.rva) {
        return false;
    }
    output = site.pattern.rva + mutation.patternOffset;
    return true;
}

auto MutationFlushRange(
        const CodecPatchSite& site,
        std::uintptr_t& firstRva,
        std::size_t& size) noexcept -> bool {
    if (site.mutations.empty()) return false;
    auto firstOffset = (std::numeric_limits<std::size_t>::max)();
    std::size_t lastOffset{};
    for (const auto& mutation : site.mutations) {
        if (mutation.patternOffset < firstOffset) {
            firstOffset = mutation.patternOffset;
        }
        if (mutation.patternOffset > lastOffset) {
            lastOffset = mutation.patternOffset;
        }
    }
    if (firstOffset > lastOffset
            || firstOffset > (std::numeric_limits<std::uintptr_t>::max)()
                - site.pattern.rva) {
        return false;
    }
    firstRva = site.pattern.rva + firstOffset;
    size = lastOffset - firstOffset + 1U;
    return true;
}

auto EncodePlayerSaveFinalizeRel32(
        const CodecPatchActivationTargets& activationTargets,
        std::array<std::uint8_t, 4>& output) noexcept -> bool {
    const auto target = activationTargets.PlayerSaveFinalizeRelayRva();
    if (target == 0) return false;

    std::int32_t displacement{};
    if (target >= PlayerSaveFinalizeCallNextRva) {
        const auto distance = target - PlayerSaveFinalizeCallNextRva;
        if (distance > static_cast<std::uintptr_t>(
                (std::numeric_limits<std::int32_t>::max)())) {
            return false;
        }
        displacement = static_cast<std::int32_t>(distance);
    } else {
        const auto distance = PlayerSaveFinalizeCallNextRva - target;
        constexpr auto MaximumNegativeDistance =
            static_cast<std::uint64_t>(
                (std::numeric_limits<std::int32_t>::max)()) + 1ULL;
        if (distance > MaximumNegativeDistance) return false;
        displacement = distance == MaximumNegativeDistance
            ? (std::numeric_limits<std::int32_t>::min)()
            : -static_cast<std::int32_t>(distance);
    }

    const auto encoded = static_cast<std::uint32_t>(displacement);
    for (std::size_t index{}; index < output.size(); ++index) {
        output[index] = static_cast<std::uint8_t>(encoded >> (index * 8U));
    }
    // Redirecting back to the native used-end helper would leave EDX
    // undefined and make the final status publication unsound.
    return output != NativePlayerSaveUsedEndDisplacement;
}

auto ResolveCodecReplacement(
        const CodecByteMutation& mutation,
        std::span<const std::uint8_t, 4> playerSaveFinalizeRel32,
        std::uint8_t& output) noexcept -> bool {
    switch (mutation.source) {
    case CodecByteMutation::ReplacementSource::Literal:
        output = mutation.replacement;
        return true;
    case CodecByteMutation::ReplacementSource::PlayerSaveFinalizeRel32:
        if (mutation.sourceByteIndex >= playerSaveFinalizeRel32.size()) {
            return false;
        }
        output = playerSaveFinalizeRel32[mutation.sourceByteIndex];
        return true;
    }
    return false;
}

} // namespace

auto PreparedCodecPatchGroups() noexcept -> std::span<const CodecPatchGroup> {
    return CodecGroups;
}

auto ValidateCodecPatchGroup(const CodecPatchGroup& group) noexcept
        -> CodecPatchPlanError {
    if (!group.name || group.sites.empty()) {
        return CodecPatchPlanError::EmptyGroup;
    }

    for (const auto& site : group.sites) {
        const auto& pattern = site.pattern;
        if (!pattern.id || pattern.rva == 0 || pattern.bytes.empty()
                || pattern.bytes.size() != pattern.mask.size()
                || site.mutations.empty()) {
            return CodecPatchPlanError::InvalidPattern;
        }
        for (const auto& mutation : site.mutations) {
            if (mutation.patternOffset >= pattern.bytes.size()
                    || pattern.mask[mutation.patternOffset] != 0xFF
                    || pattern.bytes[mutation.patternOffset]
                        != mutation.expected) {
                return CodecPatchPlanError::InvalidMutation;
            }
            if (mutation.source
                    == CodecByteMutation::ReplacementSource::Literal) {
                if (mutation.sourceByteIndex != 0
                        || mutation.expected == mutation.replacement) {
                    return CodecPatchPlanError::InvalidMutation;
                }
            } else if (mutation.source
                    == CodecByteMutation::ReplacementSource::
                        PlayerSaveFinalizeRel32) {
                if (mutation.sourceByteIndex >= 4) {
                    return CodecPatchPlanError::InvalidMutation;
                }
            } else {
                return CodecPatchPlanError::InvalidMutation;
            }
            std::uintptr_t mutationRva{};
            if (!AbsoluteMutationRva(site, mutation, mutationRva)) {
                return CodecPatchPlanError::InvalidMutation;
            }

            for (const auto& otherSite : group.sites) {
                for (const auto& otherMutation : otherSite.mutations) {
                    if (&site == &otherSite && &mutation == &otherMutation) {
                        continue;
                    }
                    std::uintptr_t otherRva{};
                    if (!AbsoluteMutationRva(
                            otherSite, otherMutation, otherRva)) {
                        return CodecPatchPlanError::InvalidMutation;
                    }
                    if (otherRva == mutationRva) {
                        return CodecPatchPlanError::DuplicateMutation;
                    }
                }
            }
        }
    }
    for (const auto& witness : group.witnesses) {
        if (!witness.id || witness.rva == 0 || witness.bytes.empty()
                || witness.bytes.size() != witness.mask.size()) {
            return CodecPatchPlanError::InvalidPattern;
        }
    }
    return CodecPatchPlanError::None;
}

auto CommitPreparedCodecPatchSet(
        bool publicationQuiescent,
        const CodecPatchActivationTargets& activationTargets,
        const CodecPatchCallbacks& callbacks) noexcept
        -> CodecPatchCommitResult {
    const std::span<const CodecPatchGroup> groups{CodecGroups};
    if (groups.empty() || !callbacks.verifyPattern || !callbacks.writeByte
            || !callbacks.flushInstructionCache) {
        return {
            .status = CodecPatchCommitStatus::InvalidPlan,
            .planError = groups.empty()
                ? CodecPatchPlanError::EmptyGroup
                : CodecPatchPlanError::None,
        };
    }
    for (const auto& group : groups) {
        const auto planError = ValidateCodecPatchGroup(group);
        if (planError != CodecPatchPlanError::None) {
            return {
                .status = CodecPatchCommitStatus::InvalidPlan,
                .planError = planError,
            };
        }
    }
    for (std::size_t groupIndex{}; groupIndex < groups.size(); ++groupIndex) {
        const auto& group = groups[groupIndex];
        for (const auto& site : group.sites) {
            for (const auto& mutation : site.mutations) {
                std::uintptr_t mutationRva{};
                if (!AbsoluteMutationRva(site, mutation, mutationRva)) {
                    return {
                        .status = CodecPatchCommitStatus::InvalidPlan,
                        .planError = CodecPatchPlanError::InvalidMutation,
                    };
                }
                for (std::size_t otherGroupIndex = groupIndex + 1;
                        otherGroupIndex < groups.size(); ++otherGroupIndex) {
                    for (const auto& otherSite
                            : groups[otherGroupIndex].sites) {
                        for (const auto& otherMutation
                                : otherSite.mutations) {
                            std::uintptr_t otherRva{};
                            if (!AbsoluteMutationRva(
                                    otherSite, otherMutation, otherRva)) {
                                return {
                                    .status = CodecPatchCommitStatus::InvalidPlan,
                                    .planError =
                                        CodecPatchPlanError::InvalidMutation,
                                };
                            }
                            if (otherRva == mutationRva) {
                                return {
                                    .status = CodecPatchCommitStatus::InvalidPlan,
                                    .planError =
                                        CodecPatchPlanError::DuplicateMutation,
                                };
                            }
                        }
                    }
                }
            }
        }
    }

    std::array<std::uint8_t, 4> playerSaveFinalizeRel32{};
    if (!EncodePlayerSaveFinalizeRel32(
            activationTargets, playerSaveFinalizeRel32)) {
        return {
            .status = CodecPatchCommitStatus::InvalidPlan,
            .planError = CodecPatchPlanError::InvalidActivationTarget,
        };
    }
    std::array<std::uint8_t, PreparedCodecMutationCount>
        resolvedReplacements{};
    std::array<bool, 4> observedRelayBytes{};
    std::size_t resolvedCount{};
    for (const auto& group : groups) {
        for (const auto& site : group.sites) {
            for (const auto& mutation : site.mutations) {
                if (resolvedCount >= resolvedReplacements.size()
                        || !ResolveCodecReplacement(
                            mutation,
                            playerSaveFinalizeRel32,
                            resolvedReplacements[resolvedCount])) {
                    return {
                        .status = CodecPatchCommitStatus::InvalidPlan,
                        .planError = CodecPatchPlanError::InvalidMutation,
                    };
                }
                if (mutation.source
                        == CodecByteMutation::ReplacementSource::
                            PlayerSaveFinalizeRel32) {
                    if (observedRelayBytes[mutation.sourceByteIndex]) {
                        return {
                            .status = CodecPatchCommitStatus::InvalidPlan,
                            .planError = CodecPatchPlanError::InvalidMutation,
                        };
                    }
                    observedRelayBytes[mutation.sourceByteIndex] = true;
                }
                ++resolvedCount;
            }
        }
    }
    if (resolvedCount != resolvedReplacements.size()) {
        return {
            .status = CodecPatchCommitStatus::InvalidPlan,
            .planError = CodecPatchPlanError::InvalidMutation,
        };
    }
    for (const auto observed : observedRelayBytes) {
        if (!observed) {
            return {
                .status = CodecPatchCommitStatus::InvalidPlan,
                .planError = CodecPatchPlanError::InvalidMutation,
            };
        }
    }
    if (!publicationQuiescent) {
        return {.status = CodecPatchCommitStatus::QuiescenceRequired};
    }
    for (const auto& group : groups) {
        for (const auto& site : group.sites) {
            if (!callbacks.verifyPattern(callbacks.context, site.pattern)) {
                return {.status = CodecPatchCommitStatus::PreflightFailed};
            }
        }
        for (const auto& witness : group.witnesses) {
            if (!callbacks.verifyPattern(callbacks.context, witness)) {
                return {.status = CodecPatchCommitStatus::PreflightFailed};
            }
        }
    }

    CodecPatchCommitResult result{
        .status = CodecPatchCommitStatus::Active,
    };
    std::size_t replacementIndex{};
    for (const auto& group : groups) {
        for (const auto& site : group.sites) {
            for (const auto& mutation : site.mutations) {
                std::uintptr_t rva{};
                if (!AbsoluteMutationRva(site, mutation, rva)) {
                    result.status = CodecPatchCommitStatus::InvalidPlan;
                    result.planError = CodecPatchPlanError::InvalidMutation;
                    return result;
                }
                ++result.attemptedMutations;
                const auto replacement =
                    resolvedReplacements[replacementIndex++];
                if (replacement == mutation.expected) {
                    ++result.confirmedMutations;
                    ++result.confirmedNoOpMutations;
                    continue;
                }
                if (!callbacks.writeByte(
                        callbacks.context,
                        rva,
                        mutation.expected,
                        replacement)) {
                    result.status = CodecPatchCommitStatus::
                        PartialCommitColdRestartRequired;
                    return result;
                }
                ++result.confirmedMutations;
            }
            std::uintptr_t firstRva{};
            std::size_t flushSize{};
            if (!MutationFlushRange(site, firstRva, flushSize)) {
                result.status = CodecPatchCommitStatus::InvalidPlan;
                result.planError = CodecPatchPlanError::InvalidMutation;
                return result;
            }
            if (!callbacks.flushInstructionCache(
                    callbacks.context, firstRva, flushSize)) {
                result.status = CodecPatchCommitStatus::
                    PartialCommitColdRestartRequired;
                return result;
            }
            ++result.confirmedFlushes;
        }
    }
    return result;
}

static_assert(
    WidthAndSentinelMutations.size() * 4
        + WriterIdMutation.size() * 2
        + WriterTerminatorMutations.size() * 2
        + PreviewWidthAndSentinelMutations.size() * 4
        + PlayerSaveRelayCallMutations.size()
        + OverflowGuardStatusMutations.size()
    == PreparedCodecMutationCount);
static_assert(
    G2Sites.size() + G3Sites.size() + G4Sites.size()
    == PreparedCodecMutableSiteCount);
static_assert(
    G2Witnesses.size() + G3Witnesses.size() + G4Witnesses.size()
    == PreparedCodecWitnessCount);

} // namespace ruffneckk::isc12
