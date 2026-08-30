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
inline constexpr std::array GenericItemReaderFirstMutations{
    CodecByteMutation{1, 0x09, 0x0C},
    CodecByteMutation{11, 0xF6, 0xF3},
    CodecByteMutation{21, 0x01, 0x0F},
};
inline constexpr std::array GenericItemReaderNextMutations{
    CodecByteMutation{1, 0x09, 0x0C},
    CodecByteMutation{17, 0x01, 0x0F},
};
inline constexpr std::array GenericItemWriterIdMutations{
    CodecByteMutation{2, 0x01, 0x0F},
    CodecByteMutation{12, 0x09, 0x0C},
};
inline constexpr std::array GenericItemWriterTerminatorMutations{
    CodecByteMutation{2, 0x01, 0x0F},
    CodecByteMutation{7, 0x09, 0x0C},
};
inline constexpr std::array AuxiliaryReaderRelayCallMutations{
    CodecByteMutation{26, 0x8E, 0x00,
        CodecByteMutation::ReplacementSource::AuxiliaryReaderRelayRel32, 0},
    CodecByteMutation{27, 0xEF, 0x00,
        CodecByteMutation::ReplacementSource::AuxiliaryReaderRelayRel32, 1},
    CodecByteMutation{28, 0xFF, 0x00,
        CodecByteMutation::ReplacementSource::AuxiliaryReaderRelayRel32, 2},
    CodecByteMutation{29, 0xFF, 0x00,
        CodecByteMutation::ReplacementSource::AuxiliaryReaderRelayRel32, 3},
};
inline constexpr std::array PlayerReaderPrimaryRelayCallMutations{
    CodecByteMutation{35, 0x11, 0x00,
        CodecByteMutation::ReplacementSource::PlayerReaderRelayRel32, 0},
    CodecByteMutation{36, 0x4B, 0x00,
        CodecByteMutation::ReplacementSource::PlayerReaderRelayRel32, 1},
    CodecByteMutation{37, 0x00, 0x00,
        CodecByteMutation::ReplacementSource::PlayerReaderRelayRel32, 2},
    CodecByteMutation{38, 0x00, 0x00,
        CodecByteMutation::ReplacementSource::PlayerReaderRelayRel32, 3},
};
inline constexpr std::array PlayerReaderLegacyRelayCallMutations{
    CodecByteMutation{17, 0x27, 0x00,
        CodecByteMutation::ReplacementSource::PlayerReaderRelayRel32, 0},
    CodecByteMutation{18, 0x2D, 0x00,
        CodecByteMutation::ReplacementSource::PlayerReaderRelayRel32, 1},
    CodecByteMutation{19, 0x00, 0x00,
        CodecByteMutation::ReplacementSource::PlayerReaderRelayRel32, 2},
    CodecByteMutation{20, 0x00, 0x00,
        CodecByteMutation::ReplacementSource::PlayerReaderRelayRel32, 3},
};
inline constexpr std::array PlayerPreviewRelayCallMutations{
    CodecByteMutation{16, 0x7B, 0x00,
        CodecByteMutation::ReplacementSource::PlayerPreviewRelayRel32, 0},
    CodecByteMutation{17, 0x11, 0x00,
        CodecByteMutation::ReplacementSource::PlayerPreviewRelayRel32, 1},
    CodecByteMutation{18, 0x40, 0x00,
        CodecByteMutation::ReplacementSource::PlayerPreviewRelayRel32, 2},
    CodecByteMutation{19, 0x00, 0x00,
        CodecByteMutation::ReplacementSource::PlayerPreviewRelayRel32, 3},
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

inline constexpr std::array G1Sites{
    CodecPatchSite{
        NativePattern{"codec.g1-reader-first", 0x37AB2B,
            GenericItemReaderFirstBytes, GenericItemReaderFirstMask},
        GenericItemReaderFirstMutations},
    CodecPatchSite{
        NativePattern{"codec.g1-reader-next", 0x37B7D4,
            GenericItemReaderNextBytes, GenericItemReaderNextMask},
        GenericItemReaderNextMutations},
    CodecPatchSite{
        NativePattern{"codec.g1-writer-id", 0x37F186,
            GenericItemWriterIdBytes, GenericItemWriterIdMask},
        GenericItemWriterIdMutations},
    CodecPatchSite{
        NativePattern{"codec.g1-writer-terminator", 0x37F983,
            GenericItemWriterTerminatorBytes,
            GenericItemWriterTerminatorMask},
        GenericItemWriterTerminatorMutations},
};

// Decoder/serializer entries may be owned by a compatible PluginSDK inline
// hook before ISC12 loads. The four unique interior mutation windows remain the
// G1 fingerprint; entry ownership is diagnosed separately and never claimed by
// this literal patch group.
inline constexpr std::array<NativePattern, 0> G1Witnesses{};

inline constexpr std::array G2Sites{
    // Publish the fail-closed reader guard before changing any ID width.
    CodecPatchSite{
        NativePattern{"codec.g2-reader-call", 0x531A54,
            AuxPlayerReaderCallBytes, AuxPlayerReaderCallMask},
        AuxiliaryReaderRelayCallMutations},
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
    NativePattern{"codec.g2-reader-malformed-return", 0x530BCF,
        AuxPlayerReaderMalformedReturnBytes,
        AuxPlayerReaderMalformedReturnMask},
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
    // Both exhaustive G3 callers are guarded before the modern width sites.
    CodecPatchSite{
        NativePattern{"codec.g3-reader-primary-call", 0x52EC28,
            PlayerReaderPrimaryCallBytes, PlayerReaderPrimaryCallMask},
        PlayerReaderPrimaryRelayCallMutations},
    CodecPatchSite{
        NativePattern{"codec.g3-reader-legacy-call", 0x530A24,
            PlayerReaderLegacyCallBytes, PlayerReaderLegacyCallMask},
        PlayerReaderLegacyRelayCallMutations},
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
    NativePattern{"codec.g3-reader-modern-tail", 0x533910,
        PlayerReaderModernTailBytes, PlayerReaderModernTailMask},
    NativePattern{"codec.g3-reader-legacy-malformed-return", 0x5338ED,
        PlayerReaderLegacyMalformedReturnBytes,
        PlayerReaderLegacyMalformedReturnMask},
    NativePattern{"codec.g3-reader-modern-malformed-return", 0x533ABF,
        PlayerReaderModernMalformedReturnBytes,
        PlayerReaderModernMalformedReturnMask},
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
    // The copy wrapper validates the complete v105 D2S and its branch-B stat
    // stream before either 12-bit preview reader can run.
    CodecPatchSite{
        NativePattern{"codec.g4-preview-buffer-read", 0x61CF81,
            PlayerPreviewBufferReadBytes, PlayerPreviewBufferReadMask},
        PlayerPreviewRelayCallMutations},
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
    NativePattern{"codec.g4-preview-buffer-read-owner", 0xA1E110,
        PlayerPreviewBufferReadOwnerBytes,
        PlayerPreviewBufferReadOwnerMask},
    NativePattern{"codec.g4-preview-buffer-read-layout", 0xA1E194,
        PlayerPreviewBufferReadLayoutBytes,
        PlayerPreviewBufferReadLayoutMask},
    NativePattern{"codec.g4-preview-buffer-read-return", 0xA1E1C6,
        PlayerPreviewBufferReadReturnBytes,
        PlayerPreviewBufferReadReturnMask},
    NativePattern{"codec.g4-preview-context-source", 0x61D43F,
        PlayerPreviewContextSourceBytes,
        PlayerPreviewContextSourceMask},
    NativePattern{"codec.g4-preview-branch-b-layout", 0x61D5E4,
        PlayerPreviewBranchBLayoutBytes,
        PlayerPreviewBranchBLayoutMask},
    NativePattern{"codec.g4-preview-reject-exit", 0x61D87D,
        PlayerPreviewRejectExitBytes,
        PlayerPreviewRejectExitMask},
    NativePattern{"codec.g4-data-context-owner", 0x300A90,
        DataTablesContextOwnerBytes,
        DataTablesContextOwnerMask},
    NativePattern{"codec.g4-preview-a-legacy-first", 0x61D247,
        PreviewReaderAFirstBytes, PreviewReaderAFirstMask},
    NativePattern{"codec.g4-preview-a-legacy-next", 0x61D290,
        PreviewReaderANextBytes, PreviewReaderANextMask},
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
    // The exhaustive G2/G4 guards publish before G1's literal-only windows.
    // The live quiescence lease still spans the whole canonical transaction.
    CodecPatchGroup{
        CodecPatchGroupId::GenericItem,
        "G1-generic-item-codec",
        G1Sites,
        G1Witnesses},
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

auto IsRel32Source(CodecByteMutation::ReplacementSource source) noexcept
        -> bool {
    using Source = CodecByteMutation::ReplacementSource;
    return source == Source::AuxiliaryReaderRelayRel32
        || source == Source::PlayerReaderRelayRel32
        || source == Source::PlayerPreviewRelayRel32
        || source == Source::PlayerSaveFinalizeRel32;
}

auto RelayTargetRva(
        CodecByteMutation::ReplacementSource source,
        const CodecPatchActivationTargets& activationTargets) noexcept
        -> std::uintptr_t {
    using Source = CodecByteMutation::ReplacementSource;
    switch (source) {
    case Source::AuxiliaryReaderRelayRel32:
        return activationTargets.AuxiliaryReaderRelayRva();
    case Source::PlayerReaderRelayRel32:
        return activationTargets.PlayerReaderRelayRva();
    case Source::PlayerPreviewRelayRel32:
        return activationTargets.PlayerPreviewRelayRva();
    case Source::PlayerSaveFinalizeRel32:
        return activationTargets.PlayerSaveFinalizeRelayRva();
    case Source::Literal:
        return 0;
    }
    return 0;
}

auto EncodeRel32(
        std::uintptr_t target,
        std::uintptr_t nextInstructionRva,
        std::array<std::uint8_t, 4>& output) noexcept -> bool {
    if (target == 0) return false;
    std::int32_t displacement{};
    if (target >= nextInstructionRva) {
        const auto distance = target - nextInstructionRva;
        if (distance > static_cast<std::uintptr_t>(
                (std::numeric_limits<std::int32_t>::max)())) {
            return false;
        }
        displacement = static_cast<std::int32_t>(distance);
    } else {
        const auto distance = nextInstructionRva - target;
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
    return true;
}

auto ResolveCodecReplacement(
        const CodecPatchSite& site,
        const CodecByteMutation& mutation,
        const CodecPatchActivationTargets& activationTargets,
        std::uint8_t& output) noexcept -> bool {
    if (mutation.source == CodecByteMutation::ReplacementSource::Literal) {
        output = mutation.replacement;
        return true;
    }
    if (!IsRel32Source(mutation.source)
            || mutation.sourceByteIndex >= 4
            || mutation.patternOffset < mutation.sourceByteIndex) {
        return false;
    }
    const auto displacementOffset =
        mutation.patternOffset - mutation.sourceByteIndex;
    if (site.pattern.bytes.size() < 4U
            || displacementOffset == 0
            || displacementOffset > site.pattern.bytes.size() - 4U
            || site.pattern.bytes[displacementOffset - 1U] != 0xE8
            || site.pattern.rva >
                (std::numeric_limits<std::uintptr_t>::max)()
                    - displacementOffset - 4U) {
        return false;
    }
    const auto nextInstructionRva =
        site.pattern.rva + displacementOffset + 4U;
    std::array<std::uint8_t, 4> encoded{};
    if (!EncodeRel32(
            RelayTargetRva(mutation.source, activationTargets),
            nextInstructionRva,
            encoded)) {
        return false;
    }
    bool redirectsToOriginal = true;
    for (std::size_t index{}; index < encoded.size(); ++index) {
        redirectsToOriginal = redirectsToOriginal
            && encoded[index]
                == site.pattern.bytes[displacementOffset + index];
    }
    if (redirectsToOriginal) return false;
    output = encoded[mutation.sourceByteIndex];
    return true;
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
        bool hasRel32{};
        auto rel32Source = CodecByteMutation::ReplacementSource::Literal;
        std::size_t rel32Offset{};
        std::array<bool, 4> rel32Bytes{};
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
            } else if (IsRel32Source(mutation.source)) {
                if (mutation.replacement != 0
                        || mutation.sourceByteIndex >= rel32Bytes.size()
                        || mutation.patternOffset
                            < mutation.sourceByteIndex) {
                    return CodecPatchPlanError::InvalidMutation;
                }
                const auto candidateOffset =
                    mutation.patternOffset - mutation.sourceByteIndex;
                if (!hasRel32) {
                    hasRel32 = true;
                    rel32Source = mutation.source;
                    rel32Offset = candidateOffset;
                } else if (mutation.source != rel32Source
                        || candidateOffset != rel32Offset) {
                    return CodecPatchPlanError::InvalidMutation;
                }
                if (rel32Bytes[mutation.sourceByteIndex]) {
                    return CodecPatchPlanError::DuplicateMutation;
                }
                rel32Bytes[mutation.sourceByteIndex] = true;
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
        if (hasRel32) {
            if (pattern.bytes.size() < 4U
                    || rel32Offset == 0
                    || rel32Offset > pattern.bytes.size() - 4U
                    || pattern.bytes[rel32Offset - 1U] != 0xE8) {
                return CodecPatchPlanError::InvalidMutation;
            }
            for (const auto observed : rel32Bytes) {
                if (!observed) {
                    return CodecPatchPlanError::InvalidMutation;
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
        const CodecPublicationQuiescenceLease& quiescence,
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

    std::array<std::uint8_t, PreparedCodecMutationCount>
        resolvedReplacements{};
    std::size_t resolvedCount{};
    for (const auto& group : groups) {
        for (const auto& site : group.sites) {
            for (const auto& mutation : site.mutations) {
                if (resolvedCount >= resolvedReplacements.size()
                        || !ResolveCodecReplacement(
                            site,
                            mutation,
                            activationTargets,
                            resolvedReplacements[resolvedCount])) {
                    return {
                        .status = CodecPatchCommitStatus::InvalidPlan,
                        .planError = IsRel32Source(mutation.source)
                            ? CodecPatchPlanError::InvalidActivationTarget
                            : CodecPatchPlanError::InvalidMutation,
                    };
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
    if (!quiescence.IsHeld()) {
        return {.status = CodecPatchCommitStatus::QuiescenceRequired};
    }
    for (const auto& group : groups) {
        for (const auto& site : group.sites) {
            if (!quiescence.IsHeld()) {
                return {.status = CodecPatchCommitStatus::QuiescenceRequired};
            }
            if (!callbacks.verifyPattern(callbacks.context, site.pattern)) {
                return {.status = CodecPatchCommitStatus::PreflightFailed};
            }
        }
        for (const auto& witness : group.witnesses) {
            if (!quiescence.IsHeld()) {
                return {.status = CodecPatchCommitStatus::QuiescenceRequired};
            }
            if (!callbacks.verifyPattern(callbacks.context, witness)) {
                return {.status = CodecPatchCommitStatus::PreflightFailed};
            }
        }
    }
    if (!quiescence.IsHeld()) {
        return {.status = CodecPatchCommitStatus::QuiescenceRequired};
    }

    CodecPatchCommitResult result{
        .status = CodecPatchCommitStatus::Active,
    };
    std::size_t replacementIndex{};
    bool nativeWriteAttempted{};
    for (const auto& group : groups) {
        for (const auto& site : group.sites) {
            for (const auto& mutation : site.mutations) {
                if (!quiescence.IsHeld()) {
                    result.status = nativeWriteAttempted
                        ? CodecPatchCommitStatus::
                            PartialCommitColdRestartRequired
                        : CodecPatchCommitStatus::QuiescenceRequired;
                    return result;
                }
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
                nativeWriteAttempted = true;
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
            if (!quiescence.IsHeld()) {
                result.status = nativeWriteAttempted
                    ? CodecPatchCommitStatus::
                        PartialCommitColdRestartRequired
                    : CodecPatchCommitStatus::QuiescenceRequired;
                return result;
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
            if (!quiescence.IsHeld()) {
                result.status = nativeWriteAttempted
                    ? CodecPatchCommitStatus::
                        PartialCommitColdRestartRequired
                    : CodecPatchCommitStatus::QuiescenceRequired;
                return result;
            }
        }
    }
    return result;
}

static_assert(
    GenericItemReaderFirstMutations.size()
        + GenericItemReaderNextMutations.size()
        + GenericItemWriterIdMutations.size()
        + GenericItemWriterTerminatorMutations.size()
        + WidthAndSentinelMutations.size() * 4
        + WriterIdMutation.size() * 2
        + WriterTerminatorMutations.size() * 2
        + PreviewWidthAndSentinelMutations.size() * 2
        + AuxiliaryReaderRelayCallMutations.size()
        + PlayerReaderPrimaryRelayCallMutations.size()
        + PlayerReaderLegacyRelayCallMutations.size()
        + PlayerPreviewRelayCallMutations.size()
        + PlayerSaveRelayCallMutations.size()
        + OverflowGuardStatusMutations.size()
    == PreparedCodecMutationCount);
static_assert(
    G1Sites.size() + G2Sites.size() + G3Sites.size() + G4Sites.size()
    == PreparedCodecMutableSiteCount);
static_assert(
    G1Witnesses.size() + G2Witnesses.size() + G3Witnesses.size()
        + G4Witnesses.size()
    == PreparedCodecWitnessCount);

} // namespace ruffneckk::isc12
