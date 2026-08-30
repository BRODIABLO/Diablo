#include "isc12_persistence_policy.hpp"

#include "isc12_store_kind.hpp"

namespace ruffneckk::isc12 {

auto PrepareStoreRead(
        std::string_view storeName,
        std::uint32_t readStatus,
        std::uint64_t announcedLength,
        std::uint64_t actualLength,
        std::span<const std::uint8_t> physicalBytes,
        const Sha256Digest& schemaHash,
        std::vector<std::uint8_t>& innerOutput) noexcept
        -> StorePreparationResult {
    const auto kind = ClassifyStoreName(storeName);
    if (kind == StoreKind::Other) {
        return {
            .disposition = StorePreparation::PassThrough,
            .storeKind = kind,
            .error = PersistenceError::None,
        };
    }
    if (readStatus != 0) {
        return {
            .disposition = StorePreparation::Rejected,
            .storeKind = kind,
            .error = PersistenceError::ReadFailure,
        };
    }
    if (announcedLength != actualLength
            || announcedLength != physicalBytes.size()) {
        return {
            .disposition = StorePreparation::Rejected,
            .storeKind = kind,
            .error = PersistenceError::ReadLength,
        };
    }
    const auto validation = ValidateEnvelope(kind, physicalBytes, schemaHash);
    if (!validation) {
        return {
            .disposition = StorePreparation::Rejected,
            .storeKind = kind,
            .error = PersistenceError::Envelope,
            .envelopeError = validation.error,
        };
    }
    try {
        std::vector<std::uint8_t> staged(
            validation.payload.begin(), validation.payload.end());
        innerOutput.swap(staged);
        return {
            .disposition = StorePreparation::Prepared,
            .storeKind = kind,
            .error = PersistenceError::None,
        };
    } catch (...) {
        return {
            .disposition = StorePreparation::Rejected,
            .storeKind = kind,
            .error = PersistenceError::Allocation,
        };
    }
}

auto PrepareStoreWrite(
        std::string_view storeName,
        std::span<const std::uint8_t> innerBytes,
        const Sha256Digest& schemaHash,
        std::vector<std::uint8_t>& physicalOutput) noexcept
        -> StorePreparationResult {
    const auto kind = ClassifyStoreName(storeName);
    if (kind == StoreKind::Other) {
        return {
            .disposition = StorePreparation::PassThrough,
            .storeKind = kind,
            .error = PersistenceError::None,
        };
    }
    const auto envelopeError = BuildEnvelope(
        kind, innerBytes, schemaHash, physicalOutput);
    if (envelopeError != EnvelopeError::None) {
        return {
            .disposition = StorePreparation::Rejected,
            .storeKind = kind,
            .error = envelopeError == EnvelopeError::Allocation
                ? PersistenceError::Allocation
                : PersistenceError::Envelope,
            .envelopeError = envelopeError,
        };
    }
    return {
        .disposition = StorePreparation::Prepared,
        .storeKind = kind,
        .error = PersistenceError::None,
    };
}

} // namespace ruffneckk::isc12
