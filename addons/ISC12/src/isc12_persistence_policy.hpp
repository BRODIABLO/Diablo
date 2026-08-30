#pragma once

#include "isc12_envelope.hpp"

#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace ruffneckk::isc12 {

enum class StorePreparation : std::uint8_t {
    PassThrough,
    Prepared,
    Rejected,
};

enum class PersistenceError : std::uint8_t {
    None,
    ReadFailure,
    ReadLength,
    Allocation,
    Envelope,
};

struct StorePreparationResult {
    StorePreparation disposition{StorePreparation::Rejected};
    StoreKind storeKind{StoreKind::Other};
    PersistenceError error{PersistenceError::Envelope};
    EnvelopeError envelopeError{EnvelopeError::None};
};

// Non-target manager objects remain byte-exact vanilla pass-through. A target
// is released only after complete-read, envelope, schema, hash and inner-store
// validation. On pass-through or rejection, innerOutput is unchanged.
auto PrepareStoreRead(
    std::string_view storeName,
    std::uint32_t readStatus,
    std::uint64_t announcedLength,
    std::uint64_t actualLength,
    std::span<const std::uint8_t> physicalBytes,
    const Sha256Digest& schemaHash,
    std::vector<std::uint8_t>& innerOutput) noexcept
    -> StorePreparationResult;

// On pass-through or rejection, physicalOutput is unchanged.
auto PrepareStoreWrite(
    std::string_view storeName,
    std::span<const std::uint8_t> innerBytes,
    const Sha256Digest& schemaHash,
    std::vector<std::uint8_t>& physicalOutput) noexcept
    -> StorePreparationResult;

} // namespace ruffneckk::isc12
