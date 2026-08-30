#include "isc12_config.hpp"
#include "isc12_contract.hpp"
#include "isc12_atomic_file.hpp"
#include "isc12_codec_patch.hpp"
#include "isc12_envelope.hpp"
#include "isc12_loader.hpp"
#include "isc12_native_sites.hpp"
#include "isc12_persistence_policy.hpp"
#include "isc12_player_stat_preflight.hpp"
#include "isc12_schema.hpp"
#include "isc12_store_kind.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {
int Failures{};

auto ReadBytes(const std::filesystem::path& path) -> std::vector<std::uint8_t> {
    std::ifstream input(path, std::ios::binary);
    return {
        std::istreambuf_iterator<char>{input},
        std::istreambuf_iterator<char>{},
    };
}

auto WriteBytes(
        const std::filesystem::path& path,
        std::span<const std::uint8_t> bytes) -> void {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output.write(
        reinterpret_cast<const char*>(bytes.data()),
        static_cast<std::streamsize>(bytes.size()));
}

auto PartialWriteFile(
        HANDLE handle,
        LPCVOID bytes,
        DWORD size,
        LPDWORD written,
        LPOVERLAPPED overlapped) noexcept -> BOOL {
    return ::WriteFile(handle, bytes, std::min<DWORD>(size, 2), written, overlapped);
}

auto FailedWriteFile(
        HANDLE,
        LPCVOID,
        DWORD,
        LPDWORD written,
        LPOVERLAPPED) noexcept -> BOOL {
    if (written) *written = 0;
    ::SetLastError(ERROR_WRITE_FAULT);
    return FALSE;
}

auto FailedFlushFileBuffers(HANDLE) noexcept -> BOOL {
    ::SetLastError(ERROR_WRITE_FAULT);
    return FALSE;
}

auto FailedReplaceFileW(
        LPCWSTR,
        LPCWSTR,
        LPCWSTR,
        DWORD,
        LPVOID,
        LPVOID) noexcept -> BOOL {
    ::SetLastError(ERROR_ACCESS_DENIED);
    return FALSE;
}

auto FailedReplaceAfterBackupFileW(
        LPCWSTR canonicalPath,
        LPCWSTR,
        LPCWSTR backupPath,
        DWORD,
        LPVOID,
        LPVOID) noexcept -> BOOL {
    if (!::MoveFileExW(
            canonicalPath, backupPath, MOVEFILE_WRITE_THROUGH)) {
        return FALSE;
    }
    ::SetLastError(ERROR_UNABLE_TO_MOVE_REPLACEMENT_2);
    return FALSE;
}

auto FailedMoveFileExW(LPCWSTR, LPCWSTR, DWORD) noexcept -> BOOL {
    ::SetLastError(ERROR_ACCESS_DENIED);
    return FALSE;
}

auto HasAtomicSibling(const std::filesystem::path& directory) -> bool {
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        const auto name = entry.path().filename().wstring();
        if (name.find(L".isc12.tmp.") != std::wstring::npos
                || name.find(L".isc12.bak.") != std::wstring::npos) {
            return true;
        }
    }
    return false;
}

auto WriteU16ForTest(
        std::span<std::uint8_t> bytes,
        std::size_t offset,
        std::uint16_t value) -> void {
    bytes[offset] = static_cast<std::uint8_t>(value);
    bytes[offset + 1] = static_cast<std::uint8_t>(value >> 8U);
}

auto WriteU32ForTest(
        std::span<std::uint8_t> bytes,
        std::size_t offset,
        std::uint32_t value) -> void {
    for (std::size_t index{}; index < sizeof(value); ++index) {
        bytes[offset + index] = static_cast<std::uint8_t>(
            value >> (index * 8U));
    }
}

struct CodecFixtureSite {
    std::uintptr_t rva{};
    std::vector<std::uint8_t> bytes{};
};

struct CodecPatchFixture {
    std::vector<CodecFixtureSite> sites{};
    std::size_t verifyCalls{};
    std::size_t writeCalls{};
    std::size_t flushCalls{};
    std::size_t failWriteAttempt{(std::numeric_limits<std::size_t>::max)()};
    std::size_t failFlushAttempt{(std::numeric_limits<std::size_t>::max)()};
    std::array<std::size_t, 32> writesAtFlush{};
    std::array<std::uintptr_t, 32> flushFirstRvas{};
    std::array<std::size_t, 32> flushSizes{};
};

auto MakeCodecPatchSetFixture(
        std::span<const ruffneckk::isc12::CodecPatchGroup> groups)
        -> CodecPatchFixture {
    CodecPatchFixture fixture;
    std::size_t siteCount{};
    for (const auto& group : groups) {
        siteCount += group.sites.size() + group.witnesses.size();
    }
    fixture.sites.reserve(siteCount);
    for (const auto& group : groups) {
        for (const auto& site : group.sites) {
            fixture.sites.push_back({
                .rva = site.pattern.rva,
                .bytes = std::vector<std::uint8_t>(
                    site.pattern.bytes.begin(), site.pattern.bytes.end()),
            });
        }
        for (const auto& witness : group.witnesses) {
            fixture.sites.push_back({
                .rva = witness.rva,
                .bytes = std::vector<std::uint8_t>(
                    witness.bytes.begin(), witness.bytes.end()),
            });
        }
    }
    return fixture;
}

auto FindCodecFixtureSite(
        CodecPatchFixture& fixture,
        std::uintptr_t rva) noexcept -> CodecFixtureSite* {
    for (auto& site : fixture.sites) {
        if (site.rva == rva) return &site;
    }
    return nullptr;
}

auto VerifyCodecFixturePattern(
        void* context,
        const ruffneckk::isc12::NativePattern& pattern) noexcept -> bool {
    auto& fixture = *static_cast<CodecPatchFixture*>(context);
    ++fixture.verifyCalls;
    for (const auto& site : fixture.sites) {
        if (site.rva != pattern.rva
                || site.bytes.size() != pattern.bytes.size()) {
            continue;
        }
        for (std::size_t index{}; index < site.bytes.size(); ++index) {
            if ((site.bytes[index] & pattern.mask[index])
                    != (pattern.bytes[index] & pattern.mask[index])) {
                return false;
            }
        }
        return true;
    }
    return false;
}

auto WriteCodecFixtureByte(
        void* context,
        std::uintptr_t rva,
        std::uint8_t expected,
        std::uint8_t replacement) noexcept -> bool {
    auto& fixture = *static_cast<CodecPatchFixture*>(context);
    const auto attempt = fixture.writeCalls++;
    if (attempt == fixture.failWriteAttempt) return false;
    for (auto& site : fixture.sites) {
        if (rva < site.rva) continue;
        const auto offset = rva - site.rva;
        if (offset >= site.bytes.size()) continue;
        if (site.bytes[offset] != expected) return false;
        site.bytes[offset] = replacement;
        return true;
    }
    return false;
}

auto FlushCodecFixtureInstructionCache(
        void* context,
        std::uintptr_t firstRva,
        std::size_t size) noexcept -> bool {
    auto& fixture = *static_cast<CodecPatchFixture*>(context);
    const auto attempt = fixture.flushCalls++;
    if (attempt < fixture.writesAtFlush.size()) {
        fixture.writesAtFlush[attempt] = fixture.writeCalls;
        fixture.flushFirstRvas[attempt] = firstRva;
        fixture.flushSizes[attempt] = size;
    }
    return attempt != fixture.failFlushAttempt;
}

auto EncodeTwelveBitFixture(
        std::span<const std::uint16_t> ids,
        std::vector<std::uint8_t>& output) -> bool {
    using namespace ruffneckk::isc12;
    for (const auto id : ids) {
        if (!IsValidStatId(id)) return false;
    }
    const auto fieldCount = ids.size() + 1U;
    if (fieldCount > (std::numeric_limits<std::size_t>::max)()
            / SerializedBitWidth) {
        return false;
    }
    const auto bitCount = fieldCount * SerializedBitWidth;
    std::vector<std::uint8_t> staged((bitCount + 7U) / 8U, 0);
    auto writeField = [&](std::size_t fieldIndex, std::uint16_t value) {
        const auto bitBase = fieldIndex * SerializedBitWidth;
        for (std::size_t bit{}; bit < SerializedBitWidth; ++bit) {
            if ((value & (1U << bit)) == 0) continue;
            const auto destinationBit = bitBase + bit;
            staged[destinationBit / 8U] |= static_cast<std::uint8_t>(
                1U << (destinationBit % 8U));
        }
    };
    for (std::size_t index{}; index < ids.size(); ++index) {
        writeField(index, ids[index]);
    }
    writeField(ids.size(), SerializedSentinel);
    output = std::move(staged);
    return true;
}

auto DecodeTwelveBitFixture(
        std::span<const std::uint8_t> bytes,
        std::vector<std::uint16_t>& output) -> bool {
    using namespace ruffneckk::isc12;
    std::vector<std::uint16_t> staged;
    const auto availableFields = bytes.size() * 8U / SerializedBitWidth;
    for (std::size_t field{}; field < availableFields; ++field) {
        std::uint16_t value{};
        const auto bitBase = field * SerializedBitWidth;
        for (std::size_t bit{}; bit < SerializedBitWidth; ++bit) {
            const auto sourceBit = bitBase + bit;
            if ((bytes[sourceBit / 8U]
                    & static_cast<std::uint8_t>(1U << (sourceBit % 8U))) != 0) {
                value |= static_cast<std::uint16_t>(1U << bit);
            }
        }
        if (value == SerializedSentinel) {
            output = std::move(staged);
            return true;
        }
        if (!IsValidStatId(value)) return false;
        staged.push_back(value);
    }
    return false;
}

auto EncodePlayerStatPreflightFixture(
        std::span<const std::uint16_t> ids,
        std::span<const ruffneckk::isc12::ItemStatSemanticRow> schema,
        ruffneckk::isc12::PlayerStatStreamKind kind,
        bool includeSentinel) -> std::vector<std::uint8_t> {
    using namespace ruffneckk::isc12;
    std::vector<std::uint8_t> bytes{0x67, 0x66};
    std::size_t bitPosition = 16;
    auto appendBits = [&](std::uint16_t value, std::size_t width) {
        const auto requiredBits = bitPosition + width;
        bytes.resize((requiredBits + 7U) / 8U, 0);
        for (std::size_t bit{}; bit < width; ++bit) {
            if ((value & static_cast<std::uint16_t>(1U << bit)) == 0) {
                continue;
            }
            const auto destination = bitPosition + bit;
            bytes[destination / 8U] |= static_cast<std::uint8_t>(
                1U << (destination % 8U));
        }
        bitPosition += width;
    };
    for (const auto id : ids) {
        appendBits(id, SerializedBitWidth);
        if (id < schema.size()) {
            appendBits(0, schema[id].csvParamBits);
            appendBits(0, kind == PlayerStatStreamKind::Auxiliary
                ? 32U
                : schema[id].csvBits);
        }
    }
    if (includeSentinel) appendBits(SerializedSentinel, SerializedBitWidth);
    return bytes;
}

#define CHECK(expression) \
    do { \
        if (!(expression)) { \
            std::cerr << "CHECK failed at line " << __LINE__ \
                      << ": " #expression "\n"; \
            ++Failures; \
        } \
    } while (false)
} // namespace

int main() {
    using namespace ruffneckk::isc12;

    static_assert(SerializedBitWidth == 12);
    static_assert(SerializedSentinel == 0x0FFF);
    static_assert(MaximumStatId == 4094);
    static_assert(MaximumRecordCount == 4095);
    static_assert(InternalWordSentinel == 0xFFFF);
    static_assert(CanonicalSchemaDescriptorVersion == 1);
    static_assert(MaximumSerializedCsvBits == 32);
    static_assert(MaximumSerializedCsvParamBits == 16);
    static_assert(InstalledHookCount == 0);
    static_assert(InstalledPatchCount == 2);
    static_assert(PreparedCodecMutableSiteCount == 14);
    static_assert(PreparedCodecMutationCount == 28);
    static_assert(PreparedCodecWitnessCount == 39);
    static_assert(PublishedCodecMutationCount == 0);

    const auto codecGroups = PreparedCodecPatchGroups();
    CHECK(codecGroups.size() == 3);
    std::size_t observedCodecMutationCount{};
    std::size_t observedCodecSiteCount{};
    for (const auto& group : codecGroups) {
        CHECK(ValidateCodecPatchGroup(group) == CodecPatchPlanError::None);
        CHECK(!group.witnesses.empty());
        observedCodecSiteCount += group.sites.size();
        for (const auto& site : group.sites) {
            observedCodecMutationCount += site.mutations.size();
        }
    }
    CHECK(observedCodecMutationCount == PreparedCodecMutationCount);
    CHECK(observedCodecSiteCount == PreparedCodecMutableSiteCount);
    CHECK(codecGroups.back().id == CodecPatchGroupId::PlayerSave);

    constexpr auto codecActivationTargets =
        CodecPatchActivationTargets::ForTesting(0x01F00000);

    auto inactiveCodecSetFixture = MakeCodecPatchSetFixture(codecGroups);
    auto codecSetCallbacks = CodecPatchCallbacks{
        .context = &inactiveCodecSetFixture,
        .verifyPattern = &VerifyCodecFixturePattern,
        .writeByte = &WriteCodecFixtureByte,
        .flushInstructionCache = &FlushCodecFixtureInstructionCache,
    };
    auto codecSetResult = CommitPreparedCodecPatchSet(
        false, codecActivationTargets, codecSetCallbacks);
    CHECK(codecSetResult.status == CodecPatchCommitStatus::QuiescenceRequired);
    CHECK(codecSetResult.attemptedMutations == 0);
    CHECK(inactiveCodecSetFixture.verifyCalls == 0);
    CHECK(inactiveCodecSetFixture.writeCalls == 0);
    CHECK(inactiveCodecSetFixture.flushCalls == 0);

    auto callbacksWithoutFlush = codecSetCallbacks;
    callbacksWithoutFlush.flushInstructionCache = nullptr;
    CHECK(CommitPreparedCodecPatchSet(
        true, codecActivationTargets, callbacksWithoutFlush).status
        == CodecPatchCommitStatus::InvalidPlan);
    CHECK(inactiveCodecSetFixture.verifyCalls == 0);

    auto codecSetFixture = MakeCodecPatchSetFixture(codecGroups);
    codecSetCallbacks.context = &codecSetFixture;
    codecSetResult = CommitPreparedCodecPatchSet(
        true, codecActivationTargets, codecSetCallbacks);
    CHECK(codecSetResult.status == CodecPatchCommitStatus::Active);
    CHECK(codecSetResult.attemptedMutations == PreparedCodecMutationCount);
    CHECK(codecSetResult.confirmedMutations == PreparedCodecMutationCount);
    CHECK(codecSetResult.confirmedFlushes == PreparedCodecMutableSiteCount);
    CHECK(codecSetFixture.flushCalls == PreparedCodecMutableSiteCount);
    for (const auto& group : codecGroups) {
        for (const auto& site : group.sites) {
            const auto fixtureSite = FindCodecFixtureSite(
                codecSetFixture, site.pattern.rva);
            CHECK(fixtureSite != nullptr);
            if (!fixtureSite) continue;
            for (const auto& mutation : site.mutations) {
                if (mutation.source
                        == CodecByteMutation::ReplacementSource::Literal) {
                    CHECK(fixtureSite->bytes[mutation.patternOffset]
                        == mutation.replacement);
                }
            }
        }
    }

    auto corruptCodecSetFixture = MakeCodecPatchSetFixture(codecGroups);
    corruptCodecSetFixture.sites.front().bytes.front() ^= 0xFF;
    codecSetCallbacks.context = &corruptCodecSetFixture;
    codecSetResult = CommitPreparedCodecPatchSet(
        true, codecActivationTargets, codecSetCallbacks);
    CHECK(codecSetResult.status == CodecPatchCommitStatus::PreflightFailed);
    CHECK(codecSetResult.attemptedMutations == 0);
    CHECK(corruptCodecSetFixture.writeCalls == 0);
    CHECK(corruptCodecSetFixture.flushCalls == 0);

    auto corruptWitnessFixture = MakeCodecPatchSetFixture(codecGroups);
    const auto corruptWitnessRva = codecGroups.front().witnesses.front().rva;
    const auto corruptWitness = FindCodecFixtureSite(
        corruptWitnessFixture, corruptWitnessRva);
    CHECK(corruptWitness != nullptr);
    if (corruptWitness) corruptWitness->bytes.front() ^= 0xFF;
    codecSetCallbacks.context = &corruptWitnessFixture;
    codecSetResult = CommitPreparedCodecPatchSet(
        true, codecActivationTargets, codecSetCallbacks);
    CHECK(codecSetResult.status == CodecPatchCommitStatus::PreflightFailed);
    CHECK(corruptWitnessFixture.writeCalls == 0);
    CHECK(corruptWitnessFixture.flushCalls == 0);

    auto partialCodecSetFixture = MakeCodecPatchSetFixture(codecGroups);
    partialCodecSetFixture.failWriteAttempt = 10;
    codecSetCallbacks.context = &partialCodecSetFixture;
    codecSetResult = CommitPreparedCodecPatchSet(
        true, codecActivationTargets, codecSetCallbacks);
    CHECK(codecSetResult.status
        == CodecPatchCommitStatus::PartialCommitColdRestartRequired);
    CHECK(codecSetResult.attemptedMutations == 11);
    CHECK(codecSetResult.confirmedMutations == 10);

    const auto& playerSaveGroup = codecGroups.back();
    CHECK(playerSaveGroup.sites.size() >= 2);
    CHECK(playerSaveGroup.sites[playerSaveGroup.sites.size() - 2U]
        .pattern.rva == 0x5353BD);
    CHECK(playerSaveGroup.sites.back().pattern.rva == 0x5353C7);
    constexpr std::uintptr_t playerSaveCallNextRva = 0x5353C7;
    constexpr auto expectedRelayDisplacement = static_cast<std::uint32_t>(
        codecActivationTargets.PlayerSaveFinalizeRelayRva()
        - playerSaveCallNextRva);
    const auto activeRelayCall = FindCodecFixtureSite(
        codecSetFixture, 0x5353BD);
    const auto activeStatus = FindCodecFixtureSite(
        codecSetFixture, 0x5353C7);
    CHECK(activeRelayCall != nullptr);
    CHECK(activeStatus != nullptr);
    if (activeRelayCall) {
        for (std::size_t index{}; index < 4; ++index) {
            CHECK(activeRelayCall->bytes[6U + index]
                == static_cast<std::uint8_t>(
                    expectedRelayDisplacement >> (index * 8U)));
        }
    }
    if (activeStatus) {
        CHECK(activeStatus->bytes[11] == 0x8B);
        CHECK(activeStatus->bytes[12] == 0xC2);
    }
    CHECK(codecSetResult.confirmedNoOpMutations == 0);

    constexpr auto mutationsBeforeRelayCall =
        PreparedCodecMutationCount - 6U;
    constexpr auto sitesBeforeRelayCall =
        PreparedCodecMutableSiteCount - 2U;
    CHECK(codecSetFixture.writesAtFlush[sitesBeforeRelayCall]
        == mutationsBeforeRelayCall + 4U);
    CHECK(codecSetFixture.flushFirstRvas[sitesBeforeRelayCall]
        == 0x5353C3);
    CHECK(codecSetFixture.flushSizes[sitesBeforeRelayCall] == 4);
    const auto statusFlush = codecSetFixture.flushCalls - 1U;
    CHECK(codecSetFixture.writesAtFlush[statusFlush]
        == PreparedCodecMutationCount);
    CHECK(codecSetFixture.flushFirstRvas[statusFlush] == 0x5353D2);
    CHECK(codecSetFixture.flushSizes[statusFlush] == 2);

    auto corruptRelayCall = MakeCodecPatchSetFixture(codecGroups);
    const auto corruptCallSite = FindCodecFixtureSite(
        corruptRelayCall, 0x5353BD);
    CHECK(corruptCallSite != nullptr);
    if (corruptCallSite) corruptCallSite->bytes[6] ^= 0xFF;
    codecSetCallbacks.context = &corruptRelayCall;
    codecSetResult = CommitPreparedCodecPatchSet(
        true, codecActivationTargets, codecSetCallbacks);
    CHECK(codecSetResult.status == CodecPatchCommitStatus::PreflightFailed);
    CHECK(corruptRelayCall.writeCalls == 0);

    auto corruptGuardStatus = MakeCodecPatchSetFixture(codecGroups);
    const auto corruptStatusSite = FindCodecFixtureSite(
        corruptGuardStatus, 0x5353C7);
    CHECK(corruptStatusSite != nullptr);
    if (corruptStatusSite) corruptStatusSite->bytes[11] ^= 0xFF;
    codecSetCallbacks.context = &corruptGuardStatus;
    codecSetResult = CommitPreparedCodecPatchSet(
        true, codecActivationTargets, codecSetCallbacks);
    CHECK(codecSetResult.status == CodecPatchCommitStatus::PreflightFailed);
    CHECK(corruptGuardStatus.writeCalls == 0);

    auto invalidTargetFixture = MakeCodecPatchSetFixture(codecGroups);
    codecSetCallbacks.context = &invalidTargetFixture;
    codecSetResult = CommitPreparedCodecPatchSet(
        true, CodecPatchActivationTargets{}, codecSetCallbacks);
    CHECK(codecSetResult.status == CodecPatchCommitStatus::InvalidPlan);
    CHECK(codecSetResult.planError
        == CodecPatchPlanError::InvalidActivationTarget);
    CHECK(invalidTargetFixture.verifyCalls == 0);
    CHECK(invalidTargetFixture.writeCalls == 0);

    constexpr auto nativeUsedEndTarget =
        CodecPatchActivationTargets::ForTesting(0xA1B610);
    codecSetResult = CommitPreparedCodecPatchSet(
        true, nativeUsedEndTarget, codecSetCallbacks);
    CHECK(codecSetResult.planError
        == CodecPatchPlanError::InvalidActivationTarget);
    CHECK(invalidTargetFixture.verifyCalls == 0);

    constexpr auto forwardRel32Overflow =
        CodecPatchActivationTargets::ForTesting(
            playerSaveCallNextRva
            + static_cast<std::uintptr_t>(
                (std::numeric_limits<std::int32_t>::max)())
            + 1U);
    codecSetResult = CommitPreparedCodecPatchSet(
        true, forwardRel32Overflow, codecSetCallbacks);
    CHECK(codecSetResult.planError
        == CodecPatchPlanError::InvalidActivationTarget);
    CHECK(invalidTargetFixture.verifyCalls == 0);

    auto noOpRelayByteFixture = MakeCodecPatchSetFixture(codecGroups);
    codecSetCallbacks.context = &noOpRelayByteFixture;
    constexpr auto noOpRelayByteTarget =
        CodecPatchActivationTargets::ForTesting(0x00600000);
    codecSetResult = CommitPreparedCodecPatchSet(
        true, noOpRelayByteTarget, codecSetCallbacks);
    CHECK(codecSetResult.status == CodecPatchCommitStatus::Active);
    CHECK(codecSetResult.confirmedMutations == PreparedCodecMutationCount);
    CHECK(codecSetResult.confirmedNoOpMutations == 1);
    CHECK(noOpRelayByteFixture.writeCalls
        == PreparedCodecMutationCount - 1U);
    CHECK(noOpRelayByteFixture.flushCalls == PreparedCodecMutableSiteCount);

    auto failedCallWrite = MakeCodecPatchSetFixture(codecGroups);
    failedCallWrite.failWriteAttempt = mutationsBeforeRelayCall;
    codecSetCallbacks.context = &failedCallWrite;
    codecSetResult = CommitPreparedCodecPatchSet(
        true, codecActivationTargets, codecSetCallbacks);
    CHECK(codecSetResult.status == CodecPatchCommitStatus::
        PartialCommitColdRestartRequired);
    CHECK(codecSetResult.confirmedMutations == mutationsBeforeRelayCall);
    const auto failedCallStatus = FindCodecFixtureSite(
        failedCallWrite, 0x5353C7);
    CHECK(failedCallStatus != nullptr);
    if (failedCallStatus) {
        CHECK(failedCallStatus->bytes[11] == 0x33);
        CHECK(failedCallStatus->bytes[12] == 0xC0);
    }

    auto failedCallFlush = MakeCodecPatchSetFixture(codecGroups);
    failedCallFlush.failFlushAttempt = sitesBeforeRelayCall;
    codecSetCallbacks.context = &failedCallFlush;
    codecSetResult = CommitPreparedCodecPatchSet(
        true, codecActivationTargets, codecSetCallbacks);
    CHECK(codecSetResult.status == CodecPatchCommitStatus::
        PartialCommitColdRestartRequired);
    CHECK(codecSetResult.confirmedMutations
        == mutationsBeforeRelayCall + 4U);
    CHECK(codecSetResult.confirmedFlushes == sitesBeforeRelayCall);
    const auto failedCallFlushStatus = FindCodecFixtureSite(
        failedCallFlush, 0x5353C7);
    CHECK(failedCallFlushStatus != nullptr);
    if (failedCallFlushStatus) {
        CHECK(failedCallFlushStatus->bytes[11] == 0x33);
        CHECK(failedCallFlushStatus->bytes[12] == 0xC0);
    }

    auto failedFirstStatusByte = MakeCodecPatchSetFixture(codecGroups);
    failedFirstStatusByte.failWriteAttempt =
        PreparedCodecMutationCount - 2U;
    codecSetCallbacks.context = &failedFirstStatusByte;
    codecSetResult = CommitPreparedCodecPatchSet(
        true, codecActivationTargets, codecSetCallbacks);
    CHECK(codecSetResult.status == CodecPatchCommitStatus::
        PartialCommitColdRestartRequired);
    const auto failedFirstStatus = FindCodecFixtureSite(
        failedFirstStatusByte, 0x5353C7);
    CHECK(failedFirstStatus != nullptr);
    if (failedFirstStatus) {
        CHECK(failedFirstStatus->bytes[11] == 0x33);
        CHECK(failedFirstStatus->bytes[12] == 0xC0);
    }

    auto failedSecondStatusByte = MakeCodecPatchSetFixture(codecGroups);
    failedSecondStatusByte.failWriteAttempt =
        PreparedCodecMutationCount - 1U;
    codecSetCallbacks.context = &failedSecondStatusByte;
    codecSetResult = CommitPreparedCodecPatchSet(
        true, codecActivationTargets, codecSetCallbacks);
    CHECK(codecSetResult.status == CodecPatchCommitStatus::
        PartialCommitColdRestartRequired);
    const auto failedSecondStatus = FindCodecFixtureSite(
        failedSecondStatusByte, 0x5353C7);
    CHECK(failedSecondStatus != nullptr);
    if (failedSecondStatus) {
        CHECK(failedSecondStatus->bytes[11] == 0x8B);
        CHECK(failedSecondStatus->bytes[12] == 0xC0);
    }

    auto failedStatusFlush = MakeCodecPatchSetFixture(codecGroups);
    failedStatusFlush.failFlushAttempt =
        PreparedCodecMutableSiteCount - 1U;
    codecSetCallbacks.context = &failedStatusFlush;
    codecSetResult = CommitPreparedCodecPatchSet(
        true, codecActivationTargets, codecSetCallbacks);
    CHECK(codecSetResult.status == CodecPatchCommitStatus::
        PartialCommitColdRestartRequired);
    CHECK(codecSetResult.confirmedMutations == PreparedCodecMutationCount);
    CHECK(codecSetResult.confirmedFlushes
        == PreparedCodecMutableSiteCount - 1U);

    const auto invalidCodecCallbacks = CodecPatchCallbacks{};
    CHECK(CommitPreparedCodecPatchSet(
        true, codecActivationTargets, invalidCodecCallbacks).status
        == CodecPatchCommitStatus::InvalidPlan);

    constexpr std::array<std::uint16_t, 8> BoundaryStatIds{
        0, 510, 511, 512, 1022, 1023, 2047, 4094,
    };
    std::vector<std::uint8_t> encodedBoundaryIds{0xCC};
    CHECK(EncodeTwelveBitFixture(BoundaryStatIds, encodedBoundaryIds));
    CHECK(encodedBoundaryIds.size()
        == ((BoundaryStatIds.size() + 1U) * SerializedBitWidth + 7U) / 8U);
    std::vector<std::uint16_t> decodedBoundaryIds{0xFFFF};
    CHECK(DecodeTwelveBitFixture(encodedBoundaryIds, decodedBoundaryIds));
    CHECK(decodedBoundaryIds == std::vector<std::uint16_t>(
        BoundaryStatIds.begin(), BoundaryStatIds.end()));
    auto missingSentinelBytes = encodedBoundaryIds;
    missingSentinelBytes.resize(missingSentinelBytes.size() - 2U);
    const auto decodedBeforeMissingSentinel = decodedBoundaryIds;
    CHECK(!DecodeTwelveBitFixture(
        missingSentinelBytes, decodedBoundaryIds));
    CHECK(decodedBoundaryIds == decodedBeforeMissingSentinel);
    constexpr std::array<std::uint16_t, 1> InvalidStatIds{
        SerializedSentinel,
    };
    const auto encodedBeforeInvalid = encodedBoundaryIds;
    CHECK(!EncodeTwelveBitFixture(InvalidStatIds, encodedBoundaryIds));
    CHECK(encodedBoundaryIds == encodedBeforeInvalid);

    std::array<ItemStatSemanticRow, 2> preflightSchema{};
    preflightSchema[0].csvBits = 17;
    preflightSchema[0].csvParamBits = 5;
    preflightSchema[1].csvBits = 32;
    preflightSchema[1].csvParamBits = 16;
    constexpr std::array<std::uint16_t, 2> PreflightIds{0, 1};
    auto auxiliaryStream = EncodePlayerStatPreflightFixture(
        PreflightIds, preflightSchema, PlayerStatStreamKind::Auxiliary, true);
    PlayerStatPreflightResult preflightResult{0xFFFF, 0xFFFF};
    CHECK(PreflightPlayerStatStream(
        auxiliaryStream,
        preflightSchema,
        PlayerStatStreamKind::Auxiliary,
        preflightResult) == PlayerStatPreflightError::None);
    CHECK(preflightResult.entryCount == PreflightIds.size());
    CHECK(preflightResult.consumedBits
        == 16U + 12U + 5U + 32U + 12U + 16U + 32U + 12U);

    auto regularStream = EncodePlayerStatPreflightFixture(
        PreflightIds, preflightSchema, PlayerStatStreamKind::Regular, true);
    CHECK(PreflightPlayerStatStream(
        regularStream,
        preflightSchema,
        PlayerStatStreamKind::Regular,
        preflightResult) == PlayerStatPreflightError::None);
    CHECK(preflightResult.consumedBits
        == 16U + 12U + 5U + 17U + 12U + 16U + 32U + 12U);

    auto emptyStream = EncodePlayerStatPreflightFixture(
        std::span<const std::uint16_t>{},
        preflightSchema,
        PlayerStatStreamKind::Regular,
        true);
    CHECK(PreflightPlayerStatStream(
        emptyStream,
        preflightSchema,
        PlayerStatStreamKind::Regular,
        preflightResult) == PlayerStatPreflightError::None);
    CHECK(preflightResult.entryCount == 0);

    const auto preflightBeforeFailure = preflightResult;
    auto missingPreflightSentinel = EncodePlayerStatPreflightFixture(
        PreflightIds, preflightSchema, PlayerStatStreamKind::Regular, false);
    CHECK(PreflightPlayerStatStream(
        missingPreflightSentinel,
        preflightSchema,
        PlayerStatStreamKind::Regular,
        preflightResult) == PlayerStatPreflightError::Truncated);
    CHECK(preflightResult.consumedBits
        == preflightBeforeFailure.consumedBits);
    CHECK(preflightResult.entryCount == preflightBeforeFailure.entryCount);
    constexpr std::array<std::uint8_t, 2> MarkerWithoutSentinel{0x67, 0x66};
    CHECK(PreflightPlayerStatStream(
        MarkerWithoutSentinel,
        preflightSchema,
        PlayerStatStreamKind::Regular,
        preflightResult) == PlayerStatPreflightError::MissingSentinel);

    auto truncatedPreflight = regularStream;
    truncatedPreflight.pop_back();
    CHECK(PreflightPlayerStatStream(
        truncatedPreflight,
        preflightSchema,
        PlayerStatStreamKind::Regular,
        preflightResult) == PlayerStatPreflightError::Truncated);
    auto invalidMarkerStream = regularStream;
    invalidMarkerStream.front() ^= 0xFF;
    CHECK(PreflightPlayerStatStream(
        invalidMarkerStream,
        preflightSchema,
        PlayerStatStreamKind::Regular,
        preflightResult) == PlayerStatPreflightError::InvalidMarker);

    constexpr std::array<std::uint16_t, 1> InvalidPreflightId{2};
    auto invalidIdStream = EncodePlayerStatPreflightFixture(
        InvalidPreflightId,
        preflightSchema,
        PlayerStatStreamKind::Regular,
        true);
    CHECK(PreflightPlayerStatStream(
        invalidIdStream,
        preflightSchema,
        PlayerStatStreamKind::Regular,
        preflightResult) == PlayerStatPreflightError::InvalidStatId);

    auto unserializedSchema = preflightSchema;
    unserializedSchema[0].csvBits = 0;
    auto unserializedRegularStream = EncodePlayerStatPreflightFixture(
        std::span<const std::uint16_t>{PreflightIds.data(), 1},
        unserializedSchema,
        PlayerStatStreamKind::Regular,
        true);
    CHECK(PreflightPlayerStatStream(
        unserializedRegularStream,
        unserializedSchema,
        PlayerStatStreamKind::Regular,
        preflightResult) == PlayerStatPreflightError::UnserializedStat);
    auto unserializedAuxiliaryStream = EncodePlayerStatPreflightFixture(
        std::span<const std::uint16_t>{PreflightIds.data(), 1},
        unserializedSchema,
        PlayerStatStreamKind::Auxiliary,
        true);
    CHECK(PreflightPlayerStatStream(
        unserializedAuxiliaryStream,
        unserializedSchema,
        PlayerStatStreamKind::Auxiliary,
        preflightResult) == PlayerStatPreflightError::UnserializedStat);

    std::vector<std::uint16_t> tooManyPreflightIds(
        MaximumPlayerStatEntries + 1U, 0);
    auto tooManyStream = EncodePlayerStatPreflightFixture(
        tooManyPreflightIds,
        preflightSchema,
        PlayerStatStreamKind::Regular,
        true);
    CHECK(PreflightPlayerStatStream(
        tooManyStream,
        preflightSchema,
        PlayerStatStreamKind::Regular,
        preflightResult) == PlayerStatPreflightError::TooManyEntries);

    auto unsafePreflightSchema = preflightSchema;
    unsafePreflightSchema[0].csvBits = 33;
    CHECK(PreflightPlayerStatStream(
        regularStream,
        unsafePreflightSchema,
        PlayerStatStreamKind::Regular,
        preflightResult) == PlayerStatPreflightError::UnsafeSchema);
    unsafePreflightSchema = preflightSchema;
    unsafePreflightSchema[0].csvParamBits = 17;
    CHECK(PreflightPlayerStatStream(
        regularStream,
        unsafePreflightSchema,
        PlayerStatStreamKind::Auxiliary,
        preflightResult) == PlayerStatPreflightError::UnsafeSchema);

    CHECK(ClassifyStoreName("Hero.d2s") == StoreKind::D2S);
    CHECK(ClassifyStoreName("Hero.With.Dots.d2s") == StoreKind::D2S);
    CHECK(ClassifyStoreName(".d2s") == StoreKind::Other);
    CHECK(ClassifyStoreName("Hero.D2S") == StoreKind::Other);
    CHECK(ClassifyStoreName("folder/Hero.d2s") == StoreKind::Other);
    CHECK(ClassifyStoreName("folder\\Hero.d2s") == StoreKind::Other);
    CHECK(ClassifyStoreName("C:Hero.d2s") == StoreKind::Other);
    CHECK(ClassifyStoreName("Hero.d2s:stream.d2s") == StoreKind::Other);
    CHECK(ClassifyStoreName("Hero?.d2s") == StoreKind::Other);
    CHECK(ClassifyStoreName("CON.d2s") == StoreKind::Other);
    CHECK(ClassifyStoreName("aux.any.d2s") == StoreKind::Other);
    CHECK(ClassifyStoreName("LPT1.d2s") == StoreKind::Other);
    CHECK(ClassifyStoreName("Hero.d2s ") == StoreKind::Other);
    CHECK(ClassifyStoreName("arbitrary.d2i") == StoreKind::Other);
    CHECK(ClassifyStoreName("SharedStashSoftCoreV2.d2i") == StoreKind::D2I);
    CHECK(ClassifyStoreName("SharedStashHardCoreV2.d2i") == StoreKind::D2I);
    CHECK(ClassifyStoreName("ModernSharedStashSoftCoreV2.d2i")
        == StoreKind::D2I);
    CHECK(ClassifyStoreName("ModernSharedStashHardCoreV2.d2i")
        == StoreKind::D2I);
    CHECK(ClassifyStoreName("modernSharedStashSoftCoreV2.d2i")
        == StoreKind::Other);
    constexpr char EmbeddedNullName[]{'H','e','r','o','\0','.','d','2','s'};
    CHECK(ClassifyStoreName(std::string_view{
        EmbeddedNullName, sizeof(EmbeddedNullName)}) == StoreKind::Other);

    std::array<ItemStatSemanticRow, 2> schemaRows{};
    schemaRows[0] = {
        .statName = "strength",
        .sendOther = true,
        .isSigned = true,
        .sendBits = 11,
        .sendParamBits = 2,
        .updateAnimRate = true,
        .saved = true,
        .csvSigned = true,
        .csvBits = 20,
        .csvParamBits = 3,
        .callback = true,
        .hasMinimum = true,
        .minimumAccumulator = -7,
        .encode = 3,
        .add = -32,
        .multiply = 55,
        .valueShift = 8,
        .saveBits = 10,
        .saveAdd = 1024,
        .saveParamBits = 5,
        .keepZero = true,
        .operation = 13,
        .operationParam = 2,
        .operationBase = 1,
        .operationStat1 = 0,
        .operationStat2 = InvalidStatReference,
        .operationStat3 = 999,
        .direct = true,
        .maximumStat = 1,
        .damageRelated = true,
        .itemEvent1 = 7,
        .itemEventFunction1 = 21,
        .itemEvent2 = InvalidStatReference,
        .itemEventFunction2 = 0,
    };
    schemaRows[1].statName = "energy";
    std::vector<std::uint8_t> schemaDescriptor{0xCC};
    CHECK(BuildSchemaDescriptor(schemaRows, 0, schemaDescriptor)
        == SchemaError::None);
    constexpr std::size_t SchemaDomainSize = sizeof(SchemaDomainTag);
    CHECK(schemaDescriptor.size()
        == SchemaDomainSize + 10 + 2 * 54
            + schemaRows[0].statName.size() + schemaRows[1].statName.size());
    CHECK(std::equal(
        SchemaDomainTag, SchemaDomainTag + SchemaDomainSize,
        schemaDescriptor.begin()));
    CHECK(schemaDescriptor[SchemaDomainSize] == 1);
    CHECK(schemaDescriptor[SchemaDomainSize + 1] == 0);
    CHECK(schemaDescriptor[SchemaDomainSize + 2] == SerializedBitWidth);
    CHECK(schemaDescriptor[SchemaDomainSize + 3] == 0xFF);
    CHECK(schemaDescriptor[SchemaDomainSize + 4] == 0x0F);
    CHECK(schemaDescriptor[SchemaDomainSize + 5] == schemaRows.size());
    CHECK(schemaDescriptor[SchemaDomainSize + 9] == DefaultEffectiveStuff);
    auto normalizedSchemaDescriptor = schemaDescriptor;
    CHECK(BuildSchemaDescriptor(schemaRows, 6, normalizedSchemaDescriptor)
        == SchemaError::None);
    CHECK(normalizedSchemaDescriptor == schemaDescriptor);
    auto invalidReferenceRows = schemaRows;
    invalidReferenceRows[0].operationStat3 = InvalidStatReference;
    CHECK(BuildSchemaDescriptor(
        invalidReferenceRows, 6, normalizedSchemaDescriptor)
        == SchemaError::None);
    CHECK(normalizedSchemaDescriptor == schemaDescriptor);

    Sha256Digest schemaGoldenHash{};
    CHECK(CalculateSchemaHash(schemaRows, 6, schemaGoldenHash)
        == SchemaError::None);
    constexpr Sha256Digest ExpectedSchemaGoldenHash{
        0x37,0xEB,0x7F,0x26,0x18,0x66,0x7E,0x3D,
        0xD3,0x40,0x67,0x4E,0xDF,0x15,0xF3,0x25,
        0xB1,0x8B,0xD2,0x50,0x23,0x6E,0x50,0x22,
        0x3A,0xF3,0x91,0x5F,0x27,0x71,0x1D,0xE7,
    };
    CHECK(schemaGoldenHash == ExpectedSchemaGoldenHash);
    auto reorderedRows = schemaRows;
    std::swap(reorderedRows[0], reorderedRows[1]);
    Sha256Digest reorderedHash{};
    CHECK(CalculateSchemaHash(reorderedRows, 6, reorderedHash)
        == SchemaError::None);
    CHECK(reorderedHash != schemaGoldenHash);
    auto renamedRows = schemaRows;
    renamedRows[0].statName = "strength_renamed";
    CHECK(CalculateSchemaHash(renamedRows, 6, reorderedHash)
        == SchemaError::None);
    CHECK(reorderedHash != schemaGoldenHash);
    auto invalidUtf8Rows = schemaRows;
    invalidUtf8Rows[0].statName.assign("\xC0\xAF", 2);
    normalizedSchemaDescriptor = {0x44, 0x55};
    CHECK(BuildSchemaDescriptor(
        invalidUtf8Rows, 6, normalizedSchemaDescriptor)
        == SchemaError::InvalidUtf8);
    CHECK((normalizedSchemaDescriptor
        == std::vector<std::uint8_t>{0x44, 0x55}));
    auto unsafeCodecRows = schemaRows;
    unsafeCodecRows[0].csvParamBits =
        static_cast<std::uint8_t>(MaximumSerializedCsvParamBits + 1U);
    normalizedSchemaDescriptor = {0x66, 0x77};
    CHECK(BuildSchemaDescriptor(
        unsafeCodecRows, 6, normalizedSchemaDescriptor)
        == SchemaError::UnsafeCodecWidth);
    CHECK((normalizedSchemaDescriptor
        == std::vector<std::uint8_t>{0x66, 0x77}));
    unsafeCodecRows = schemaRows;
    unsafeCodecRows[0].csvBits =
        static_cast<std::uint8_t>(MaximumSerializedCsvBits + 1U);
    CHECK(BuildSchemaDescriptor(
        unsafeCodecRows, 6, normalizedSchemaDescriptor)
        == SchemaError::UnsafeCodecWidth);
    CHECK((normalizedSchemaDescriptor
        == std::vector<std::uint8_t>{0x66, 0x77}));

    std::vector<std::uint8_t> compiledRows(
        2 * CompiledItemStatRecordStride);
    WriteU32ForTest(compiledRows, 0x04,
        (1U << 0U) | (1U << 1U) | (1U << 2U) | (1U << 3U)
        | (1U << 8U) | (1U << 9U) | (1U << 10U)
        | (1U << 11U) | (1U << 12U));
    compiledRows[0x08] = 11;
    compiledRows[0x09] = 2;
    compiledRows[0x0A] = 20;
    compiledRows[0x0B] = 3;
    WriteU32ForTest(compiledRows, 0x0C, 55);
    WriteU32ForTest(compiledRows, 0x10, static_cast<std::uint32_t>(-32));
    compiledRows[0x14] = 8;
    compiledRows[0x15] = 10;
    compiledRows[0x16] = 99;
    WriteU32ForTest(compiledRows, 0x18, 1024);
    WriteU32ForTest(compiledRows, 0x1C, 0xDEADBEEF);
    WriteU32ForTest(compiledRows, 0x20, 5);
    WriteU32ForTest(compiledRows, 0x24, 0x123);
    WriteU32ForTest(compiledRows, 0x28, static_cast<std::uint32_t>(-7));
    compiledRows[0x2C] = 3;
    WriteU16ForTest(compiledRows, 0x2E, 1);
    compiledRows[0x30] = 0xA5;
    WriteU16ForTest(compiledRows, 0x44, 7);
    WriteU16ForTest(compiledRows, 0x46, InvalidStatReference);
    WriteU16ForTest(compiledRows, 0x48, 21);
    WriteU16ForTest(compiledRows, 0x4A, 0);
    compiledRows[0x4C] = 1;
    compiledRows[0x50] = 13;
    compiledRows[0x51] = 2;
    WriteU16ForTest(compiledRows, 0x52, 1);
    WriteU16ForTest(compiledRows, 0x54, 0);
    WriteU16ForTest(compiledRows, 0x56, InvalidStatReference);
    WriteU16ForTest(compiledRows, 0x58, InvalidStatReference);
    WriteU32ForTest(compiledRows, 0x13C, 0);
    compiledRows[0x140] = 0x5A;
    WriteU16ForTest(compiledRows, CompiledItemStatRecordStride, 1);
    for (const auto rowOffset : {
            CompiledItemStatRecordStride + 0x2E,
            CompiledItemStatRecordStride + 0x44,
            CompiledItemStatRecordStride + 0x46,
            CompiledItemStatRecordStride + 0x52,
            CompiledItemStatRecordStride + 0x54,
            CompiledItemStatRecordStride + 0x56,
            CompiledItemStatRecordStride + 0x58}) {
        WriteU16ForTest(compiledRows, rowOffset, InvalidStatReference);
    }
    std::vector<ItemStatSemanticRow> decodedRows;
    std::uint8_t decodedStuff{};
    constexpr std::array<std::string_view, 2> CompiledStatNames{
        "strength", "energy",
    };
    CHECK(DecodeCompiledItemStatRecords(
        compiledRows, 2, CompiledStatNames, decodedRows, decodedStuff)
        == SchemaError::None);
    CHECK(decodedRows.size() == schemaRows.size());
    CHECK(decodedStuff == DefaultEffectiveStuff);
    Sha256Digest decodedHash{};
    CHECK(CalculateSchemaHash(decodedRows, decodedStuff, decodedHash)
        == SchemaError::None);
    CHECK(decodedHash == schemaGoldenHash);

    auto displayOnlyCompiledRows = compiledRows;
    std::fill(
        displayOnlyCompiledRows.begin() + 0x30,
        displayOnlyCompiledRows.begin() + 0x44,
        std::uint8_t{0x7C});
    displayOnlyCompiledRows[0x140] ^= 0xFF;
    CHECK(DecodeCompiledItemStatRecords(
        displayOnlyCompiledRows, 2, CompiledStatNames,
        decodedRows, decodedStuff)
        == SchemaError::None);
    CHECK(CalculateSchemaHash(decodedRows, decodedStuff, decodedHash)
        == SchemaError::None);
    CHECK(decodedHash == schemaGoldenHash);
    auto semanticCompiledRows = compiledRows;
    semanticCompiledRows[0x15] ^= 1;
    CHECK(DecodeCompiledItemStatRecords(
        semanticCompiledRows, 2, CompiledStatNames,
        decodedRows, decodedStuff)
        == SchemaError::None);
    CHECK(CalculateSchemaHash(decodedRows, decodedStuff, decodedHash)
        == SchemaError::None);
    CHECK(decodedHash != schemaGoldenHash);
    auto unsafeCodecCompiledRows = compiledRows;
    unsafeCodecCompiledRows[0x0B] =
        static_cast<std::uint8_t>(
            MaximumSerializedCsvParamBits + 1U);
    const auto rowsBeforeUnsafeCodec = decodedRows;
    const auto stuffBeforeUnsafeCodec = decodedStuff;
    CHECK(DecodeCompiledItemStatRecords(
        unsafeCodecCompiledRows, 2, CompiledStatNames,
        decodedRows, decodedStuff) == SchemaError::UnsafeCodecWidth);
    CHECK(decodedRows.size() == rowsBeforeUnsafeCodec.size());
    CHECK(decodedRows.front().statName
        == rowsBeforeUnsafeCodec.front().statName);
    CHECK(decodedStuff == stuffBeforeUnsafeCodec);
    unsafeCodecCompiledRows = compiledRows;
    unsafeCodecCompiledRows[0x0A] = static_cast<std::uint8_t>(
        MaximumSerializedCsvBits + 1U);
    CHECK(DecodeCompiledItemStatRecords(
        unsafeCodecCompiledRows, 2, CompiledStatNames,
        decodedRows, decodedStuff) == SchemaError::UnsafeCodecWidth);
    CHECK(decodedRows.size() == rowsBeforeUnsafeCodec.size());
    CHECK(decodedRows.front().statName
        == rowsBeforeUnsafeCodec.front().statName);
    CHECK(decodedStuff == stuffBeforeUnsafeCodec);
    for (const auto legacyOffset : {0x16U, 0x1CU, 0x24U}) {
        semanticCompiledRows = compiledRows;
        semanticCompiledRows[legacyOffset] ^= 1;
        CHECK(DecodeCompiledItemStatRecords(
            semanticCompiledRows, 2, CompiledStatNames,
            decodedRows, decodedStuff) == SchemaError::None);
        CHECK(CalculateSchemaHash(decodedRows, decodedStuff, decodedHash)
            == SchemaError::None);
        CHECK(decodedHash == schemaGoldenHash);
    }

    auto badRecordIds = compiledRows;
    WriteU16ForTest(badRecordIds, CompiledItemStatRecordStride, 0);
    const auto rowsBeforeBadId = decodedRows.size();
    const auto stuffBeforeBadId = decodedStuff;
    CHECK(DecodeCompiledItemStatRecords(
        badRecordIds, 2, CompiledStatNames,
        decodedRows, decodedStuff) == SchemaError::RecordIdMismatch);
    CHECK(decodedRows.size() == rowsBeforeBadId);
    CHECK(decodedStuff == stuffBeforeBadId);

    std::vector<ItemStatSemanticRow> unchangedRows(1);
    decodedStuff = 8;
    CHECK(DecodeCompiledItemStatRecords(
        compiledRows, 1, CompiledStatNames, unchangedRows, decodedStuff)
        == SchemaError::SizeMismatch);
    CHECK(unchangedRows.size() == 1);
    CHECK(decodedStuff == 8);

    std::array<std::uint8_t, 16> innerD2S{};
    WriteU32ForTest(innerD2S, 0, 0xAA55AA55);
    WriteU32ForTest(innerD2S, 4, InnerFormatVersion);
    WriteU32ForTest(
        innerD2S, 8, static_cast<std::uint32_t>(innerD2S.size()));
    WriteU32ForTest(innerD2S, 12, CalculateD2SChecksum(innerD2S));
    constexpr std::array<std::uint8_t, 16> GoldenInnerD2S{
        0x55,0xAA,0x55,0xAA,0x69,0x00,0x00,0x00,
        0x10,0x00,0x00,0x00,0x00,0x90,0x6D,0x00,
    };
    CHECK(innerD2S == GoldenInnerD2S);
    CHECK(ValidateInnerStore(StoreKind::D2S, innerD2S));

    const auto schemaHash = schemaGoldenHash;
    constexpr Sha256Digest GoldenD2SHash{
        0x61,0x44,0x61,0xA2,0x67,0xBA,0x54,0xE8,
        0x72,0xC5,0x74,0xA6,0x8B,0x54,0x46,0xBE,
        0x52,0xED,0xD2,0x3C,0x2B,0x82,0x58,0x5C,
        0x27,0xFA,0xB6,0x92,0x28,0x8E,0x4B,0x46,
    };
    Sha256Digest calculatedHash{};
    CHECK(CalculateSha256(innerD2S, calculatedHash));
    CHECK(calculatedHash == GoldenD2SHash);

    std::vector<std::uint8_t> envelope{0xCC, 0xDD};
    CHECK(BuildEnvelope(StoreKind::D2S, innerD2S, schemaHash, envelope)
        == EnvelopeError::None);
    CHECK(envelope.size() == EnvelopeHeaderSize + innerD2S.size());
    CHECK(std::equal(
        EnvelopeMagic.begin(), EnvelopeMagic.end(), envelope.begin()));
    constexpr std::array<std::uint8_t, 24> GoldenEnvelopePrefix{
        0x49,0x53,0x43,0x31,0x32,0x0D,0x0A,0x1A,
        0x01,0x00,0x60,0x00,0x01,0x0C,0xFF,0x0F,
        0x00,0x00,0x00,0x00,0x10,0x00,0x00,0x00,
    };
    CHECK(std::equal(
        GoldenEnvelopePrefix.begin(), GoldenEnvelopePrefix.end(),
        envelope.begin()));
    CHECK(std::equal(
        schemaHash.begin(), schemaHash.end(),
        envelope.begin() + EnvelopeSchemaHashOffset));
    CHECK(std::equal(
        GoldenD2SHash.begin(), GoldenD2SHash.end(),
        envelope.begin() + EnvelopePayloadHashOffset));
    auto validation = ValidateEnvelope(StoreKind::D2S, envelope, schemaHash);
    CHECK(validation);
    CHECK(std::equal(
        validation.payload.begin(), validation.payload.end(),
        innerD2S.begin()));
    std::vector<std::uint8_t> secondEnvelope;
    CHECK(BuildEnvelope(
        StoreKind::D2S, validation.payload, schemaHash, secondEnvelope)
        == EnvelopeError::None);
    CHECK(secondEnvelope == envelope);
    CHECK(ValidateEnvelope(
        StoreKind::D2S, secondEnvelope, schemaHash));

    std::vector<std::uint8_t> preparedStore{0xA5, 0x5A};
    const auto preparedStoreBefore = preparedStore;
    auto preparation = PrepareStoreRead(
        "opaque.manager.object", 99, envelope.size(), envelope.size(),
        envelope, schemaHash, preparedStore);
    CHECK(preparation.disposition == StorePreparation::PassThrough);
    CHECK(preparedStore == preparedStoreBefore);
    preparation = PrepareStoreRead(
        "Hero.d2s", 1, envelope.size(), envelope.size(),
        envelope, schemaHash, preparedStore);
    CHECK(preparation.disposition == StorePreparation::Rejected);
    CHECK(preparation.error == PersistenceError::ReadFailure);
    CHECK(preparedStore == preparedStoreBefore);
    preparation = PrepareStoreRead(
        "Hero.d2s", 0, envelope.size(), envelope.size() - 1,
        envelope, schemaHash, preparedStore);
    CHECK(preparation.disposition == StorePreparation::Rejected);
    CHECK(preparation.error == PersistenceError::ReadLength);
    CHECK(preparedStore == preparedStoreBefore);
    preparation = PrepareStoreRead(
        "Hero.d2s", 0, envelope.size(), envelope.size(),
        envelope, schemaHash, preparedStore);
    CHECK(preparation.disposition == StorePreparation::Prepared);
    CHECK(preparedStore == std::vector<std::uint8_t>(
        innerD2S.begin(), innerD2S.end()));

    preparedStore = preparedStoreBefore;
    preparation = PrepareStoreWrite(
        "opaque.manager.object", innerD2S, schemaHash, preparedStore);
    CHECK(preparation.disposition == StorePreparation::PassThrough);
    CHECK(preparedStore == preparedStoreBefore);
    preparation = PrepareStoreWrite(
        "Hero.d2s", innerD2S, schemaHash, preparedStore);
    CHECK(preparation.disposition == StorePreparation::Prepared);
    CHECK(preparedStore == envelope);

    auto legacyInnerD2S = innerD2S;
    WriteU32ForTest(legacyInnerD2S, 4, 91);
    WriteU32ForTest(legacyInnerD2S, 12, 0);
    WriteU32ForTest(
        legacyInnerD2S, 12, CalculateD2SChecksum(legacyInnerD2S));
    CHECK(!ValidateInnerStore(StoreKind::D2S, legacyInnerD2S));

    auto rejectedEnvelope = envelope;
    rejectedEnvelope[0] ^= 0xFF;
    CHECK(ValidateEnvelope(StoreKind::D2S, rejectedEnvelope, schemaHash).error
        == EnvelopeError::Magic);
    rejectedEnvelope = envelope;
    rejectedEnvelope[8] = 2;
    CHECK(ValidateEnvelope(StoreKind::D2S, rejectedEnvelope, schemaHash).error
        == EnvelopeError::Version);
    rejectedEnvelope = envelope;
    rejectedEnvelope[12] = static_cast<std::uint8_t>(StoreKind::D2I);
    CHECK(ValidateEnvelope(StoreKind::D2S, rejectedEnvelope, schemaHash).error
        == EnvelopeError::StoreKind);
    auto wrongSchema = schemaHash;
    wrongSchema[0] ^= 0xFF;
    CHECK(ValidateEnvelope(StoreKind::D2S, envelope, wrongSchema).error
        == EnvelopeError::SchemaHash);
    rejectedEnvelope = envelope;
    rejectedEnvelope[EnvelopePayloadHashOffset] ^= 0xFF;
    CHECK(ValidateEnvelope(StoreKind::D2S, rejectedEnvelope, schemaHash).error
        == EnvelopeError::PayloadHash);
    rejectedEnvelope = envelope;
    rejectedEnvelope.pop_back();
    CHECK(ValidateEnvelope(StoreKind::D2S, rejectedEnvelope, schemaHash).error
        == EnvelopeError::PayloadLength);
    rejectedEnvelope = envelope;
    rejectedEnvelope.push_back(0);
    CHECK(ValidateEnvelope(StoreKind::D2S, rejectedEnvelope, schemaHash).error
        == EnvelopeError::PayloadLength);
    CHECK(ValidateEnvelope(StoreKind::D2S, innerD2S, schemaHash).error
        == EnvelopeError::TooShort);

    std::array<std::uint8_t, 64> innerD2I{};
    WriteU32ForTest(innerD2I, 0, 0xAA55AA55);
    WriteU32ForTest(innerD2I, 4, 2);
    WriteU32ForTest(innerD2I, 8, InnerFormatVersion);
    WriteU16ForTest(
        innerD2I, 0x10, static_cast<std::uint16_t>(innerD2I.size()));
    WriteU16ForTest(innerD2I, 0x12, 0xB8);
    innerD2I[0x14] = 0;
    constexpr Sha256Digest GoldenD2IHash{
        0x56,0x80,0x18,0xBA,0x1B,0xC8,0x25,0x10,
        0xBC,0x7A,0x81,0x00,0x1B,0x0D,0x41,0xFA,
        0xEC,0x24,0x44,0x7A,0x06,0x9B,0xFE,0xCA,
        0x59,0x93,0x67,0xAD,0x9C,0x34,0xC4,0x39,
    };
    CHECK(CalculateSha256(innerD2I, calculatedHash));
    CHECK(calculatedHash == GoldenD2IHash);
    CHECK(ValidateInnerStore(StoreKind::D2I, innerD2I));
    CHECK(BuildEnvelope(StoreKind::D2I, innerD2I, schemaHash, envelope)
        == EnvelopeError::None);
    validation = ValidateEnvelope(StoreKind::D2I, envelope, schemaHash);
    CHECK(validation);
    CHECK(validation.payload.size() == innerD2I.size());
    CHECK(BuildEnvelope(
        StoreKind::D2I, validation.payload, schemaHash, secondEnvelope)
        == EnvelopeError::None);
    CHECK(secondEnvelope == envelope);
    CHECK(ValidateEnvelope(
        StoreKind::D2I, secondEnvelope, schemaHash));

    std::vector<std::uint8_t> mixedD2I;
    mixedD2I.insert(mixedD2I.end(), innerD2I.begin(), innerD2I.end());
    mixedD2I.insert(mixedD2I.end(), innerD2I.begin(), innerD2I.end());
    mixedD2I[64] ^= 0xFF;
    const auto unchangedEnvelope = envelope;
    CHECK(BuildEnvelope(StoreKind::D2I, mixedD2I, schemaHash, envelope)
        == EnvelopeError::InnerPayload);
    CHECK(envelope == unchangedEnvelope);

    const auto atomicDirectory = std::filesystem::temp_directory_path()
        / ("ruffneckk-isc12-atomic-tests-"
            + std::to_string(::GetCurrentProcessId()));
    std::error_code cleanupError;
    std::filesystem::remove_all(atomicDirectory, cleanupError);
    CHECK(std::filesystem::create_directories(atomicDirectory));
    const auto atomicPath = atomicDirectory / "fixture.d2s";
    const std::array<std::uint8_t, 4> originalBytes{1, 2, 3, 4};
    const std::array<std::uint8_t, 7> replacementBytes{9, 8, 7, 6, 5, 4, 3};
    WriteBytes(atomicPath, originalBytes);
    auto atomicResult = WriteFileAtomically(
        atomicPath.wstring(), replacementBytes);
    CHECK(atomicResult.committed);
    CHECK(atomicResult.stage == AtomicWriteStage::Committed);
    CHECK(ReadBytes(atomicPath) == std::vector<std::uint8_t>(
        replacementBytes.begin(), replacementBytes.end()));
    CHECK(!HasAtomicSibling(atomicDirectory));

    std::filesystem::remove(atomicPath, cleanupError);
    atomicResult = WriteFileAtomically(atomicPath.wstring(), originalBytes);
    CHECK(atomicResult.committed);
    CHECK(ReadBytes(atomicPath) == std::vector<std::uint8_t>(
        originalBytes.begin(), originalBytes.end()));

    const auto nulPath = atomicDirectory / L"nul-target.d2s";
    std::filesystem::remove(nulPath, cleanupError);
    auto embeddedNulPath = nulPath.wstring();
    embeddedNulPath.push_back(L'\0');
    embeddedNulPath.append(L".ignored");
    atomicResult = WriteFileAtomically(embeddedNulPath, replacementBytes);
    CHECK(!atomicResult.committed);
    CHECK(atomicResult.stage == AtomicWriteStage::InvalidArgument);
    CHECK(!std::filesystem::exists(nulPath));
    CHECK(!HasAtomicSibling(atomicDirectory));

    auto api = DefaultAtomicFileApi();
    api.writeFile = &PartialWriteFile;
    atomicResult = WriteFileAtomically(
        atomicPath.wstring(), replacementBytes, api);
    CHECK(atomicResult.committed);
    CHECK(ReadBytes(atomicPath) == std::vector<std::uint8_t>(
        replacementBytes.begin(), replacementBytes.end()));

    WriteBytes(atomicPath, originalBytes);
    api = DefaultAtomicFileApi();
    api.writeFile = &FailedWriteFile;
    atomicResult = WriteFileAtomically(
        atomicPath.wstring(), replacementBytes, api);
    CHECK(!atomicResult.committed);
    CHECK(atomicResult.stage == AtomicWriteStage::Write);
    CHECK(ReadBytes(atomicPath) == std::vector<std::uint8_t>(
        originalBytes.begin(), originalBytes.end()));
    CHECK(!HasAtomicSibling(atomicDirectory));

    api = DefaultAtomicFileApi();
    api.flushFileBuffers = &FailedFlushFileBuffers;
    atomicResult = WriteFileAtomically(
        atomicPath.wstring(), replacementBytes, api);
    CHECK(!atomicResult.committed);
    CHECK(atomicResult.stage == AtomicWriteStage::Flush);
    CHECK(ReadBytes(atomicPath) == std::vector<std::uint8_t>(
        originalBytes.begin(), originalBytes.end()));
    CHECK(!HasAtomicSibling(atomicDirectory));

    api = DefaultAtomicFileApi();
    api.replaceFileW = &FailedReplaceFileW;
    atomicResult = WriteFileAtomically(
        atomicPath.wstring(), replacementBytes, api);
    CHECK(!atomicResult.committed);
    CHECK(atomicResult.stage == AtomicWriteStage::Replace);
    CHECK(ReadBytes(atomicPath) == std::vector<std::uint8_t>(
        originalBytes.begin(), originalBytes.end()));
    CHECK(!HasAtomicSibling(atomicDirectory));

    WriteBytes(atomicPath, originalBytes);
    api = DefaultAtomicFileApi();
    api.replaceFileW = &FailedReplaceAfterBackupFileW;
    atomicResult = WriteFileAtomically(
        atomicPath.wstring(), replacementBytes, api);
    CHECK(!atomicResult.committed);
    CHECK(atomicResult.stage == AtomicWriteStage::Rollback);
    CHECK(atomicResult.rollbackAttempted);
    CHECK(atomicResult.rollbackSucceeded);
    CHECK(AtomicFailurePreservedDestination(atomicResult));
    CHECK(ReadBytes(atomicPath) == std::vector<std::uint8_t>(
        originalBytes.begin(), originalBytes.end()));
    CHECK(!HasAtomicSibling(atomicDirectory));

    WriteBytes(atomicPath, originalBytes);
    api = DefaultAtomicFileApi();
    api.replaceFileW = &FailedReplaceAfterBackupFileW;
    api.moveFileExW = &FailedMoveFileExW;
    atomicResult = WriteFileAtomically(
        atomicPath.wstring(), replacementBytes, api);
    CHECK(!atomicResult.committed);
    CHECK(atomicResult.stage == AtomicWriteStage::Rollback);
    CHECK(atomicResult.rollbackAttempted);
    CHECK(!atomicResult.rollbackSucceeded);
    CHECK(!AtomicFailurePreservedDestination(atomicResult));
    CHECK(!std::filesystem::exists(atomicPath));
    CHECK(HasAtomicSibling(atomicDirectory));

    std::filesystem::remove_all(atomicDirectory, cleanupError);

    CHECK(CanEncodeRel32(0x1000, 0x1005));
    CHECK(CanEncodeRel32(0x80001000, 0x1005));
    CHECK(!CanEncodeRel32(0x1000, UINT64_C(0x80001005)));

    std::vector<std::string> commitEvents;
    auto commitResult = CommitLoaderMutation(
        [&]() noexcept { commitEvents.emplace_back("reserve"); },
        [&]() noexcept {
            commitEvents.emplace_back("tail");
            return true;
        },
        [&]() noexcept { commitEvents.emplace_back("activate"); },
        [&]() noexcept {
            commitEvents.emplace_back("cap");
            return true;
        },
        [&]() noexcept { commitEvents.emplace_back("publish"); });
    CHECK(commitResult == LoaderInstallResult::Active);
    CHECK((commitEvents == std::vector<std::string>{
        "reserve", "tail", "activate", "cap", "publish",
    }));

    commitEvents.clear();
    commitResult = CommitLoaderMutation(
        [&]() noexcept { commitEvents.emplace_back("reserve"); },
        [&]() noexcept {
            commitEvents.emplace_back("tail");
            return false;
        },
        [&]() noexcept { commitEvents.emplace_back("activate"); },
        [&]() noexcept {
            commitEvents.emplace_back("cap");
            return true;
        },
        [&]() noexcept { commitEvents.emplace_back("publish"); });
    CHECK(commitResult
        == LoaderInstallResult::PartialCommitColdRestartRequired);
    CHECK((commitEvents == std::vector<std::string>{"reserve", "tail"}));

    commitEvents.clear();
    commitResult = CommitLoaderMutation(
        [&]() noexcept { commitEvents.emplace_back("reserve"); },
        [&]() noexcept {
            commitEvents.emplace_back("tail");
            return true;
        },
        [&]() noexcept { commitEvents.emplace_back("activate"); },
        [&]() noexcept {
            commitEvents.emplace_back("cap");
            return false;
        },
        [&]() noexcept { commitEvents.emplace_back("publish"); });
    CHECK(commitResult
        == LoaderInstallResult::PartialCommitColdRestartRequired);
    CHECK((commitEvents == std::vector<std::string>{
        "reserve", "tail", "activate", "cap",
    }));

    CHECK(IsValidStatId(0));
    CHECK(IsValidStatId(4094));
    CHECK(!IsValidStatId(4095));
    CHECK(IsValidRecordCount(0));
    CHECK(IsValidRecordCount(4095));
    CHECK(!IsValidRecordCount(4096));
    CHECK(AddedSerializedBits(16, 1) == 51);
    CHECK(AddedSerializedBits(500, 10) == 1530);

    std::vector<DescriptionEntry> descriptions{
        {4094, 1}, {511, 200}, {0, 200}, {1023, 15},
    };
    CHECK(SortDescriptionEntries(descriptions));
    CHECK(descriptions[0].statId == 4094);
    CHECK(descriptions[1].statId == 1023);
    CHECK(descriptions[2].priority == 200);
    CHECK(descriptions[3].priority == 200);
    descriptions.push_back({4095, 500});
    CHECK(!SortDescriptionEntries(descriptions));

    std::vector<DescriptionEntry> signedPriorities{
        {0, std::bit_cast<std::int16_t>(std::uint16_t{0x7FFF})},
        {1, std::bit_cast<std::int16_t>(std::uint16_t{0x8000})},
        {2, std::bit_cast<std::int16_t>(std::uint16_t{0xFFFF})},
        {3, 0},
    };
    CHECK(SortDescriptionEntries(signedPriorities));
    CHECK(signedPriorities[0].statId == 1);
    CHECK(signedPriorities[1].statId == 2);
    CHECK(signedPriorities[2].statId == 3);
    CHECK(signedPriorities[3].statId == 0);

    const auto checkDenseDescriptionBoundary = [](std::size_t count) {
        std::vector<DescriptionSource> rows(count);
        for (std::size_t id = 0; id < count; ++id) {
            rows[id] = {
                1,
                static_cast<std::int16_t>(count - id),
            };
        }
        std::vector<std::uint16_t> index{0xBEEF};
        CHECK(BuildDescriptionIndex(rows, index));
        CHECK(index.size() == count);
        if (!index.empty()) {
            CHECK(index.front() == count - 1);
            CHECK(index.back() == 0);
        }
    };
    checkDenseDescriptionBoundary(511);
    checkDenseDescriptionBoundary(512);
    checkDenseDescriptionBoundary(1023);
    checkDenseDescriptionBoundary(2047);
    checkDenseDescriptionBoundary(4095);

    std::vector<DescriptionSource> sparseRows(4095);
    sparseRows[0] = {1, 30};
    sparseRows[511] = {2, 20};
    sparseRows[1023] = {3, 10};
    sparseRows[2047] = {4, 40};
    sparseRows[4094] = {5, 0};
    std::vector<std::uint16_t> sparseIndex;
    CHECK(BuildDescriptionIndex(sparseRows, sparseIndex));
    CHECK((sparseIndex == std::vector<std::uint16_t>{
        4094, 1023, 511, 0, 2047,
    }));

    std::vector<DescriptionSource> tooManyRows(4096, {1, 1});
    std::vector<std::uint16_t> untouched{7, 8, 9};
    CHECK(!BuildDescriptionIndex(tooManyRows, untouched));
    CHECK((untouched == std::vector<std::uint16_t>{7, 8, 9}));

    Config config{};
    std::string error;
    constexpr std::string_view validToml = R"toml(
config_version = 1
enabled = true
[diagnostics]
enabled = false
)toml";
    CHECK(ParseToml(validToml, config, error));
    CHECK(config.enabled && !config.diagnostics);
    CHECK(!ParseToml(
        "config_version = 2\nenabled = true\n"
        "[diagnostics]\nenabled = false\n",
        config,
        error));
    CHECK(!ParseToml(
        "config_version = 1\nenabled = true\nunknown = 1\n"
        "[diagnostics]\nenabled = false\n",
        config,
        error));

    const auto candidates = BuildConfigCandidates(
        std::filesystem::path{L"mod"},
        std::filesystem::path{L"scope"},
        std::filesystem::path{L"global"},
        std::filesystem::path{L"ruffneckk-isc12.toml"});
    CHECK(candidates.size() == 3);
    const auto deduplicated = BuildConfigCandidates(
        std::filesystem::path{L"same"},
        std::filesystem::path{L"same"},
        std::filesystem::path{L"global"},
        std::filesystem::path{L"ruffneckk-isc12.toml"});
    CHECK(deduplicated.size() == 2);

    CHECK(!FoundationPatterns.empty());
    for (const auto& pattern : FoundationPatterns) {
        CHECK(pattern.id != nullptr);
        CHECK(pattern.rva != 0);
        CHECK(!pattern.bytes.empty());
        CHECK(pattern.bytes.size() == pattern.mask.size());
        CHECK(std::string_view{pattern.id} != "item.decode-entry");
        CHECK(std::string_view{pattern.id} != "item.serialize-entry");
        for (const auto mask : pattern.mask) CHECK(mask == 0xFF);
    }

    std::ifstream configFile(
        std::filesystem::path{ISC12_CONFIG_PATH}, std::ios::binary);
    const std::string configText{
        std::istreambuf_iterator<char>{configFile},
        std::istreambuf_iterator<char>{}};
    CHECK(!configText.empty());
    CHECK(ParseToml(configText, config, error));

    std::ifstream pluginFile(
        std::filesystem::path{ISC12_PLUGIN_SOURCE_PATH}, std::ios::binary);
    const std::string pluginText{
        std::istreambuf_iterator<char>{pluginFile},
        std::istreambuf_iterator<char>{}};
    CHECK(!pluginText.empty());
    CHECK(pluginText.find("D2RL::GetBuildName(context)")
        != std::string::npos);
    CHECK(pluginText.find("without a version allowlist")
        != std::string::npos);
    CHECK(pluginText.find("92777") == std::string::npos);
    CHECK(pluginText.find("93847") == std::string::npos);
    CHECK(pluginText.find("IsSupportedBuild") == std::string::npos);
    CHECK(pluginText.find("RuntimeBuild ==") == std::string::npos);
    CHECK(pluginText.find("RuntimeBuild !=") == std::string::npos);
    CHECK(pluginText.find("InstallInlineHook") == std::string::npos);
    CHECK(pluginText.find("VirtualProtect") == std::string::npos);
    CHECK(pluginText.find("WriteProcessMemory") == std::string::npos);
    CHECK(pluginText.find("ProcessMutexNameFormat") != std::string::npos);
    CHECK(pluginText.find("GetCurrentProcessId()") != std::string::npos);
    CHECK(pluginText.find("RuffnecKk.ISC12.%lu") != std::string::npos);
    constexpr std::string_view publicDescription =
        "Supports up to 4,095 item stat definitions for overhaul mods.";
    CHECK(pluginText.find(publicDescription) != std::string::npos);

    std::ifstream resourceFile(
        std::filesystem::path{ISC12_RESOURCE_SOURCE_PATH}, std::ios::binary);
    const std::string resourceText{
        std::istreambuf_iterator<char>{resourceFile},
        std::istreambuf_iterator<char>{}};
    CHECK(resourceText.find(publicDescription) != std::string::npos);

    std::ifstream loaderFile(
        std::filesystem::path{ISC12_LOADER_SOURCE_PATH}, std::ios::binary);
    const std::string loaderText{
        std::istreambuf_iterator<char>{loaderFile},
        std::istreambuf_iterator<char>{}};
    CHECK(!loaderText.empty());
    const auto reserveTailLifetime = loaderText.find(
        "AnyMutationInstalled = true;");
    const auto tailCommit = loaderText.find("PatchJmpRel32");
    const auto conservativeCapGuard = loaderText.find(
        "InterlockedExchange(&State->capMayBeExtended, 1)");
    const auto capCommit = loaderText.find("LoaderContext->PatchWriteU32");
    CHECK(reserveTailLifetime != std::string::npos);
    CHECK(tailCommit != std::string::npos);
    CHECK(conservativeCapGuard != std::string::npos);
    CHECK(capCommit != std::string::npos);
    CHECK(reserveTailLifetime < tailCommit);
    CHECK(tailCommit < conservativeCapGuard);
    CHECK(conservativeCapGuard < capCommit);
    CHECK(tailCommit < capCommit);
    CHECK(loaderText.find("InstallInlineHook") == std::string::npos);
    CHECK(loaderText.find("MaximumRecordCount") != std::string::npos);
    CHECK(loaderText.find("FailClosed") != std::string::npos);
    CHECK(loaderText.find(
        "DescFunc tail publication returned an uncertain result")
        != std::string::npos);
    CHECK(loaderText.find("ShutdownRundownTimeoutMilliseconds")
        != std::string::npos);
    CHECK(loaderText.find("PAGE_EXECUTE_READ") != std::string::npos);
    CHECK(loaderText.find("PAGE_EXECUTE_READWRITE")
        != std::string::npos);
    CHECK(loaderText.find("0x31F0AB") != std::string::npos);
    CHECK(loaderText.find("0x31ED38") != std::string::npos);
    CHECK(loaderText.find(
        "PatchJmpRel32(\n                PersistenceReaderPatchRva")
        == std::string::npos);
    CHECK(loaderText.find(
        "PatchJmpRel32(\n                PersistenceWriterPatchRva")
        == std::string::npos);
    CHECK(loaderText.find("PersistenceState->operational, 1")
        == std::string::npos);
    CHECK(loaderText.find("PersistenceState->codecReady, 1")
        == std::string::npos);
    CHECK(loaderText.find(
        "ISC12PersistenceRelayTemplatePlayerSaveFinalizeEntry")
        != std::string::npos);
    CHECK(loaderText.find("PlayerSaveFinalizeCallRva = 0x5353C2")
        != std::string::npos);
    CHECK(loaderText.find(
        "LoaderCodecPatchAuthority::BindPreparedRelay")
        != std::string::npos);

    std::ifstream loaderAsmFile(
        std::filesystem::path{ISC12_LOADER_ASM_PATH}, std::ios::binary);
    const std::string loaderAsmText{
        std::istreambuf_iterator<char>{loaderAsmFile},
        std::istreambuf_iterator<char>{}};
    CHECK(!loaderAsmText.empty());
    CHECK(loaderAsmText.find("mov qword ptr [rsp+20h], rax")
        != std::string::npos);
    CHECK(loaderAsmText.find("ISC12LoaderTailMidHook PROC FRAME")
        != std::string::npos);
    CHECK(loaderAsmText.find(".allocstack 30h") != std::string::npos);
    CHECK(loaderAsmText.find(".endprolog") != std::string::npos);
    CHECK(loaderAsmText.find("mov r9, qword ptr [rsp+48h]")
        != std::string::npos);
    CHECK(loaderAsmText.find("mov r15, qword ptr [rsp+0F40h]")
        != std::string::npos);
    CHECK(loaderAsmText.find("mov r14, qword ptr [rsp+0F48h]")
        != std::string::npos);
    CHECK(loaderAsmText.find("mov r13, qword ptr [rsp+0F50h]")
        != std::string::npos);
    CHECK(loaderAsmText.find("mov rbx, qword ptr [rsp+0F78h]")
        != std::string::npos);
    CHECK(loaderAsmText.find("mov rsi, qword ptr [rsp+0F80h]")
        != std::string::npos);
    CHECK(loaderAsmText.find("mov rdi, qword ptr [rsp+0F88h]")
        != std::string::npos);
    CHECK(loaderAsmText.find("mov ecx, 7") != std::string::npos);
    CHECK(loaderAsmText.find("int 29h") != std::string::npos);
    CHECK(loaderAsmText.find("ud2") == std::string::npos);
    CHECK(loaderAsmText.find("ISC12LoaderRelayTemplateSuccessExit")
        != std::string::npos);
    CHECK(loaderAsmText.find("ISC12LoaderRelayTemplateVanillaExit")
        != std::string::npos);
    const auto firstDecrement = loaderAsmText.find("lock dec");
    const auto handler = loaderAsmText.find("ISC12LoaderTailMidHook PROC");
    CHECK(firstDecrement != std::string::npos);
    CHECK(handler != std::string::npos);
    CHECK(loaderAsmText.find("lock dec", handler) == std::string::npos);

    std::ifstream persistenceAsmFile(
        std::filesystem::path{ISC12_PERSISTENCE_ASM_PATH},
        std::ios::binary);
    const std::string persistenceAsmText{
        std::istreambuf_iterator<char>{persistenceAsmFile},
        std::istreambuf_iterator<char>{}};
    CHECK(!persistenceAsmText.empty());
    CHECK(persistenceAsmText.find(
        "mov qword ptr [rsp+48h], -1") != std::string::npos);
    CHECK(persistenceAsmText.find(
        "ISC12PersistenceReaderMidHook PROC FRAME")
        != std::string::npos);
    CHECK(persistenceAsmText.find(
        "ISC12PersistenceWriterMidHook PROC FRAME")
        != std::string::npos);
    CHECK(persistenceAsmText.find(".allocstack 20h")
        != std::string::npos);
    CHECK(persistenceAsmText.find(".endprolog")
        != std::string::npos);
    CHECK(persistenceAsmText.find(
        "mov r8d, dword ptr [rsp+50h]") != std::string::npos);
    CHECK(persistenceAsmText.find("mov r9d, eax") != std::string::npos);
    CHECK(persistenceAsmText.find("lea rdx, [rsp+60h]")
        != std::string::npos);
    CHECK(persistenceAsmText.find(
        "ISC12PersistenceRelayTemplateReaderRejectedExit")
        != std::string::npos);
    CHECK(persistenceAsmText.find(
        "ISC12PersistenceRelayTemplateWriterCommittedExit")
        != std::string::npos);
    CHECK(persistenceAsmText.find(
        "ISC12PersistenceRelayTemplatePlayerSaveFinalizeEntry")
        != std::string::npos);
    CHECK(persistenceAsmText.find("cmp qword ptr [rcx+18h], 0")
        != std::string::npos);
    CHECK(persistenceAsmText.find("mov rax, qword ptr [rcx+10h]")
        != std::string::npos);
    CHECK(persistenceAsmText.find("mov edx, dword ptr [rcx+20h]")
        != std::string::npos);
    CHECK(persistenceAsmText.find("PersistenceRelayFailClosed")
        != std::string::npos);
    CHECK(persistenceAsmText.find("mov ecx, 7") != std::string::npos);
    CHECK(persistenceAsmText.find("int 29h") != std::string::npos);

    std::ifstream cmakeFile(
        std::filesystem::path{ISC12_CMAKE_PATH}, std::ios::binary);
    const std::string cmakeText{
        std::istreambuf_iterator<char>{cmakeFile},
        std::istreambuf_iterator<char>{}};
    CHECK(cmakeText.find("RUFFNECKK_PUBLIC_ARCHIVE_ELIGIBLE FALSE")
        != std::string::npos);
    CHECK(cmakeText.find("RUFFNECKK_PUBLIC_ARCHIVE_ELIGIBLE TRUE")
        == std::string::npos);
    CHECK(cmakeText.find("isc12_persistence_relay.asm")
        != std::string::npos);

    return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
