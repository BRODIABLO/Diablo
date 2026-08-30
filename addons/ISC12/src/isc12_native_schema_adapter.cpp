#include "isc12_native_schema_adapter.hpp"

#include "isc12_contract.hpp"

#include <limits>
#include <string>
#include <utility>

namespace ruffneckk::isc12 {
namespace {

auto ReadRecordId(
        std::span<const std::uint8_t> recordBytes,
        std::size_t recordOffset) noexcept -> std::uint16_t {
    return static_cast<std::uint16_t>(recordBytes[recordOffset])
        | static_cast<std::uint16_t>(recordBytes[recordOffset + 1]) << 8U;
}

} // namespace

auto BuildNativeItemStatCostSchemaSnapshot(
        const void* linker,
        std::span<const std::uint8_t> recordBytes,
        std::size_t rowCount,
        NativeItemStatCostLinkerCallbacks callbacks,
        NativeItemStatCostSchemaSnapshot& output) noexcept -> SchemaError {
    if (!linker || !callbacks.getLinkNameCount || !callbacks.getLinkName) {
        return SchemaError::InvalidArgument;
    }
    if (rowCount == 0) return SchemaError::InvalidArgument;
    if (rowCount > MaximumRecordCount) return SchemaError::TooManyRows;
    if (rowCount > (std::numeric_limits<std::size_t>::max)()
            / CompiledItemStatRecordStride
            || recordBytes.size()
                != rowCount * CompiledItemStatRecordStride) {
        return SchemaError::SizeMismatch;
    }
    for (std::size_t ordinal{}; ordinal < rowCount; ++ordinal) {
        const auto offset = ordinal * CompiledItemStatRecordStride;
        if (ReadRecordId(recordBytes, offset) != ordinal) {
            return SchemaError::RecordIdMismatch;
        }
    }
    if (callbacks.getLinkNameCount(linker) != rowCount) {
        return SchemaError::SizeMismatch;
    }

    try {
        std::vector<std::string> copiedNames;
        copiedNames.reserve(rowCount);
        for (std::size_t ordinal{}; ordinal < rowCount; ++ordinal) {
            const auto name = callbacks.getLinkName(
                linker, static_cast<std::int32_t>(ordinal));
            if (name.empty() || name.size() > MaximumNativeStatNameLength) {
                return SchemaError::InvalidUtf8;
            }
            copiedNames.emplace_back(name);
        }

        std::vector<std::string_view> nameViews;
        nameViews.reserve(copiedNames.size());
        for (const auto& name : copiedNames) nameViews.emplace_back(name);

        NativeItemStatCostSchemaSnapshot staged;
        const auto decode = DecodeCompiledItemStatRecords(
            recordBytes,
            rowCount,
            nameViews,
            staged.rows,
            staged.effectiveStuff);
        if (decode != SchemaError::None) return decode;
        const auto hash = CalculateSchemaHash(
            staged.rows, staged.effectiveStuff, staged.schemaHash);
        if (hash != SchemaError::None) return hash;

        output = std::move(staged);
        return SchemaError::None;
    } catch (...) {
        return SchemaError::Allocation;
    }
}

} // namespace ruffneckk::isc12
