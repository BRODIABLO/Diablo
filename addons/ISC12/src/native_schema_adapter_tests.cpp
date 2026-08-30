#include "isc12_native_schema_adapter.hpp"

#include "isc12_contract.hpp"

#include <array>
#include <cstdint>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {

int Failures{};

#define CHECK(expression) \
    do { \
        if (!(expression)) { \
            std::cerr << "CHECK failed at line " << __LINE__ \
                      << ": " #expression "\n"; \
            ++Failures; \
        } \
    } while (false)

struct LinkerFixture {
    std::uint64_t reportedCount{};
    std::vector<std::string> names;
    mutable std::string sharedBuffer;
    mutable std::size_t countCalls{};
    mutable std::size_t nameCalls{};
};

auto GetFixtureCount(const void* linker) noexcept -> std::uint64_t {
    const auto* fixture = static_cast<const LinkerFixture*>(linker);
    ++fixture->countCalls;
    return fixture->reportedCount;
}

auto GetFixtureName(
        const void* linker,
        std::int32_t ordinal) noexcept -> std::string_view {
    const auto* fixture = static_cast<const LinkerFixture*>(linker);
    ++fixture->nameCalls;
    if (ordinal < 0
            || static_cast<std::size_t>(ordinal) >= fixture->names.size()) {
        return {};
    }
    fixture->sharedBuffer = fixture->names[static_cast<std::size_t>(ordinal)];
    return fixture->sharedBuffer;
}

auto WriteU16(
        std::span<std::uint8_t> bytes,
        std::size_t offset,
        std::uint16_t value) -> void {
    bytes[offset] = static_cast<std::uint8_t>(value);
    bytes[offset + 1] = static_cast<std::uint8_t>(value >> 8U);
}

auto WriteU32(
        std::span<std::uint8_t> bytes,
        std::size_t offset,
        std::uint32_t value) -> void {
    for (std::size_t index{}; index < sizeof(value); ++index) {
        bytes[offset + index] = static_cast<std::uint8_t>(
            value >> (index * 8U));
    }
}

auto MakeRecords(std::size_t count) -> std::vector<std::uint8_t> {
    using namespace ruffneckk::isc12;
    std::vector<std::uint8_t> records(
        count * CompiledItemStatRecordStride);
    for (std::size_t ordinal{}; ordinal < count; ++ordinal) {
        WriteU16(
            records,
            ordinal * CompiledItemStatRecordStride,
            static_cast<std::uint16_t>(ordinal));
    }
    if (!records.empty()) {
        WriteU32(records, 0x13C, 7);
        records[0x15] = 12;
    }
    return records;
}

auto MakeSentinelSnapshot()
        -> ruffneckk::isc12::NativeItemStatCostSchemaSnapshot {
    using namespace ruffneckk::isc12;
    NativeItemStatCostSchemaSnapshot snapshot;
    snapshot.rows.resize(1);
    snapshot.rows[0].statName = "unchanged";
    snapshot.rows[0].sendBits = 0xA5;
    snapshot.effectiveStuff = 8;
    snapshot.schemaHash.fill(0x5A);
    return snapshot;
}

auto IsSentinel(
        const ruffneckk::isc12::NativeItemStatCostSchemaSnapshot& snapshot)
        -> bool {
    return snapshot.rows.size() == 1
        && snapshot.rows[0].statName == "unchanged"
        && snapshot.rows[0].sendBits == 0xA5
        && snapshot.effectiveStuff == 8
        && snapshot.schemaHash.front() == 0x5A
        && snapshot.schemaHash.back() == 0x5A;
}

} // namespace

int main() {
    using namespace ruffneckk::isc12;

    static_assert(NativeItemStatCostLinkerOffset == 0x1270);
    static_assert(NativeGetLinkNameCountRva == 0xA12400);
    static_assert(NativeGetLinkNameRva == 0xA12420);
    static_assert(NativeGetLinkNameMode == 0);
    static_assert(CompiledItemStatRecordStride == 0x144);
    static_assert(MaximumSerializedCsvBits == 32);
    static_assert(MaximumSerializedCsvParamBits == 16);
    static_assert(MaximumNativeStatNameLength == 0xFFFF);

    Sha256Digest publishedHash{};
    Sha256Digest candidateHash{};
    CHECK(DecideNativeSchemaGate(
        SchemaError::None, false, publishedHash, candidateHash)
        == NativeSchemaGateDecision::Publish);
    CHECK(DecideNativeSchemaGate(
        SchemaError::None, true, publishedHash, candidateHash)
        == NativeSchemaGateDecision::AcceptExisting);
    candidateHash[17] = 1;
    CHECK(DecideNativeSchemaGate(
        SchemaError::None, true, publishedHash, candidateHash)
        == NativeSchemaGateDecision::FailClosed);
    CHECK(DecideNativeSchemaGate(
        SchemaError::InvalidArgument, false, publishedHash, publishedHash)
        == NativeSchemaGateDecision::FailClosed);
    CHECK(DecideNativeSchemaGate(
        SchemaError::InvalidUtf8, true, publishedHash, publishedHash)
        == NativeSchemaGateDecision::FailClosed);

    constexpr NativeItemStatCostLinkerCallbacks Callbacks{
        &GetFixtureCount,
        &GetFixtureName,
    };

    LinkerFixture zeroRowLinker{
        .reportedCount = 0,
        .names = {},
    };
    auto zeroRowSnapshot = MakeSentinelSnapshot();
    CHECK(BuildNativeItemStatCostSchemaSnapshot(
        &zeroRowLinker, {}, 0, Callbacks, zeroRowSnapshot)
        == SchemaError::InvalidArgument);
    CHECK(zeroRowLinker.countCalls == 0);
    CHECK(zeroRowLinker.nameCalls == 0);
    CHECK(IsSentinel(zeroRowSnapshot));

    auto records = MakeRecords(2);
    LinkerFixture linker{
        .reportedCount = 2,
        .names = {"strength", "energy"},
    };
    auto snapshot = MakeSentinelSnapshot();
    CHECK(BuildNativeItemStatCostSchemaSnapshot(
        &linker, records, 2, Callbacks, snapshot) == SchemaError::None);
    CHECK(snapshot.rows.size() == 2);
    CHECK(snapshot.rows[0].statName == "strength");
    CHECK(snapshot.rows[1].statName == "energy");
    CHECK(snapshot.effectiveStuff == 7);
    CHECK(linker.countCalls == 1);
    CHECK(linker.nameCalls == 2);

    constexpr std::array<std::string_view, 2> ExpectedNames{
        "strength", "energy",
    };
    std::vector<ItemStatSemanticRow> expectedRows;
    std::uint8_t expectedStuff{};
    CHECK(DecodeCompiledItemStatRecords(
        records, 2, ExpectedNames, expectedRows, expectedStuff)
        == SchemaError::None);
    Sha256Digest expectedHash{};
    CHECK(CalculateSchemaHash(expectedRows, expectedStuff, expectedHash)
        == SchemaError::None);
    CHECK(snapshot.schemaHash == expectedHash);

    snapshot = MakeSentinelSnapshot();
    CHECK(BuildNativeItemStatCostSchemaSnapshot(
        nullptr, records, 2, Callbacks, snapshot)
        == SchemaError::InvalidArgument);
    CHECK(IsSentinel(snapshot));

    auto unsafeCodecRecords = records;
    unsafeCodecRecords[0x0B] = static_cast<std::uint8_t>(
        MaximumSerializedCsvParamBits + 1U);
    linker = {
        .reportedCount = 2,
        .names = {"strength", "energy"},
    };
    snapshot = MakeSentinelSnapshot();
    CHECK(BuildNativeItemStatCostSchemaSnapshot(
        &linker, unsafeCodecRecords, 2, Callbacks, snapshot)
        == SchemaError::UnsafeCodecWidth);
    CHECK(IsSentinel(snapshot));

    unsafeCodecRecords = records;
    unsafeCodecRecords[0x0A] = static_cast<std::uint8_t>(
        MaximumSerializedCsvBits + 1U);
    snapshot = MakeSentinelSnapshot();
    CHECK(BuildNativeItemStatCostSchemaSnapshot(
        &linker, unsafeCodecRecords, 2, Callbacks, snapshot)
        == SchemaError::UnsafeCodecWidth);
    CHECK(IsSentinel(snapshot));

    linker = {
        .reportedCount = 3,
        .names = {"strength", "energy"},
    };
    snapshot = MakeSentinelSnapshot();
    CHECK(BuildNativeItemStatCostSchemaSnapshot(
        &linker, records, 2, Callbacks, snapshot)
        == SchemaError::SizeMismatch);
    CHECK(linker.nameCalls == 0);
    CHECK(IsSentinel(snapshot));

    linker = {
        .reportedCount = 2,
        .names = {"strength", "energy"},
    };
    auto badIds = records;
    WriteU16(badIds, CompiledItemStatRecordStride, 0);
    snapshot = MakeSentinelSnapshot();
    CHECK(BuildNativeItemStatCostSchemaSnapshot(
        &linker, badIds, 2, Callbacks, snapshot)
        == SchemaError::RecordIdMismatch);
    CHECK(linker.countCalls == 0);
    CHECK(linker.nameCalls == 0);
    CHECK(IsSentinel(snapshot));

    auto shortRecords = records;
    shortRecords.pop_back();
    snapshot = MakeSentinelSnapshot();
    CHECK(BuildNativeItemStatCostSchemaSnapshot(
        &linker, shortRecords, 2, Callbacks, snapshot)
        == SchemaError::SizeMismatch);
    CHECK(linker.countCalls == 0);
    CHECK(linker.nameCalls == 0);
    CHECK(IsSentinel(snapshot));

    linker = {
        .reportedCount = 2,
        .names = {"strength", ""},
    };
    snapshot = MakeSentinelSnapshot();
    CHECK(BuildNativeItemStatCostSchemaSnapshot(
        &linker, records, 2, Callbacks, snapshot)
        == SchemaError::InvalidUtf8);
    CHECK(IsSentinel(snapshot));

    linker = {
        .reportedCount = 2,
        .names = {"strength", std::string(MaximumNativeStatNameLength + 1, 'x')},
    };
    snapshot = MakeSentinelSnapshot();
    CHECK(BuildNativeItemStatCostSchemaSnapshot(
        &linker, records, 2, Callbacks, snapshot)
        == SchemaError::InvalidUtf8);
    CHECK(IsSentinel(snapshot));

    linker = {
        .reportedCount = 2,
        .names = {"strength", std::string{"a\0b", 3}},
    };
    snapshot = MakeSentinelSnapshot();
    CHECK(BuildNativeItemStatCostSchemaSnapshot(
        &linker, records, 2, Callbacks, snapshot)
        == SchemaError::InvalidUtf8);
    CHECK(IsSentinel(snapshot));

    linker = {
        .reportedCount = 2,
        .names = {"strength", std::string{"\xC0\xAF", 2}},
    };
    snapshot = MakeSentinelSnapshot();
    CHECK(BuildNativeItemStatCostSchemaSnapshot(
        &linker, records, 2, Callbacks, snapshot)
        == SchemaError::InvalidUtf8);
    CHECK(IsSentinel(snapshot));

    auto maximumNameRecords = MakeRecords(1);
    linker = {
        .reportedCount = 1,
        .names = {std::string(MaximumNativeStatNameLength, 'n')},
    };
    snapshot = MakeSentinelSnapshot();
    CHECK(BuildNativeItemStatCostSchemaSnapshot(
        &linker, maximumNameRecords, 1, Callbacks, snapshot)
        == SchemaError::None);
    CHECK(snapshot.rows.size() == 1);
    CHECK(snapshot.rows[0].statName.size() == MaximumNativeStatNameLength);

    linker = {
        .reportedCount = MaximumRecordCount + 1,
        .names = {},
    };
    snapshot = MakeSentinelSnapshot();
    CHECK(BuildNativeItemStatCostSchemaSnapshot(
        &linker, {}, MaximumRecordCount + 1, Callbacks, snapshot)
        == SchemaError::TooManyRows);
    CHECK(linker.countCalls == 0);
    CHECK(linker.nameCalls == 0);
    CHECK(IsSentinel(snapshot));

    snapshot = MakeSentinelSnapshot();
    auto missingCount = Callbacks;
    missingCount.getLinkNameCount = nullptr;
    CHECK(BuildNativeItemStatCostSchemaSnapshot(
        &linker, {}, 0, missingCount, snapshot)
        == SchemaError::InvalidArgument);
    CHECK(IsSentinel(snapshot));

    snapshot = MakeSentinelSnapshot();
    auto missingName = Callbacks;
    missingName.getLinkName = nullptr;
    CHECK(BuildNativeItemStatCostSchemaSnapshot(
        &linker, {}, 0, missingName, snapshot)
        == SchemaError::InvalidArgument);
    CHECK(IsSentinel(snapshot));

    if (Failures != 0) {
        std::cerr << Failures << " native schema adapter test(s) failed\n";
        return 1;
    }
    std::cout << "ISC12 native schema adapter tests passed\n";
    return 0;
}
