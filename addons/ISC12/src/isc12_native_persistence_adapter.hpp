#pragma once

#include "isc12_persistence_policy.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <string_view>

namespace ruffneckk::isc12 {

inline constexpr std::size_t NativePersistencePathCapacity = 0x30C;
inline constexpr std::uint64_t MaximumNativePhysicalStoreLength =
    (std::numeric_limits<std::uint32_t>::max)();
inline constexpr std::uint64_t MaximumNativeInnerStoreLength =
    MaximumNativePhysicalStoreLength - EnvelopeHeaderSize;

enum class NativePersistenceDisposition : std::uint8_t {
    Vanilla,
    Success,
    Reject,
    Fatal,
};

enum class NativePersistenceError : std::uint8_t {
    None,
    CodecNotReady,
    SchemaUnavailable,
    StorePreparation,
    PathBuffer,
    PathTraversal,
    PathMismatch,
    PathConversion,
    ReplaceBuffer,
    ClearRejectedRead,
    AtomicCommit,
    PhysicalLength,
};

struct NativePersistenceResult {
    NativePersistenceDisposition disposition{
        NativePersistenceDisposition::Reject};
    StoreKind storeKind{StoreKind::Other};
    NativePersistenceError error{NativePersistenceError::None};
    PersistenceError persistenceError{PersistenceError::None};
    EnvelopeError envelopeError{EnvelopeError::None};
};

using ReplaceNativeReadBufferCallback = bool (*)(
    void* context,
    std::span<const std::uint8_t> innerBytes) noexcept;
using ClearRejectedNativeReadCallback = bool (*)(void* context) noexcept;

struct NativeReadCallbacks {
    void* context{};
    ReplaceNativeReadBufferCallback replaceBuffer{};
    ClearRejectedNativeReadCallback clearRejectedRead{};
};

struct NativeReadRequest {
    std::string_view storeName;
    bool codecReady{};
    const Sha256Digest* schemaHash{};
    std::uint32_t readStatus{};
    std::uint64_t announcedLength{};
    std::uint64_t actualLength{};
    std::span<const std::uint8_t> physicalBytes;
};

inline auto NativeReadMaySnapshot(
        std::uint32_t nativeStatus,
        std::uint32_t nativeSuccessStatus,
        std::uint64_t announcedLength,
        std::uint32_t actualLength) noexcept -> bool {
    return nativeStatus == nativeSuccessStatus
        && announcedLength == actualLength;
}

inline auto NativeWriteLengthSupported(
        std::uint64_t innerLength) noexcept -> bool {
    return innerLength <= MaximumNativeInnerStoreLength;
}

using AtomicNativeCommitCallback = bool (*)(
    void* context,
    std::wstring_view widePath,
    std::span<const std::uint8_t> physicalBytes) noexcept;

struct NativeWriteCallbacks {
    void* context{};
    AtomicNativeCommitCallback atomicCommit{};
};

struct NativeWriteRequest {
    std::string_view storeName;
    // The span models the complete bounded native char buffer. A terminal NUL
    // must occur within NativePersistencePathCapacity bytes. Bytes after that
    // first NUL are ignored exactly as MultiByteToWideChar(..., -1) ignores
    // them.
    std::span<const char> nativePathUtf8;
    bool codecReady{};
    const Sha256Digest* schemaHash{};
    std::span<const std::uint8_t> innerBytes;
};

// Non-target manager objects are returned as Vanilla before any callback is
// inspected or invoked. Target rejection always clears the native read state;
// a successful replacement callback must consume/copy the staged span before
// returning. A failed replacement is followed by the same rejection cleanup.
auto AdaptNativeStoreRead(
    const NativeReadRequest& request,
    const NativeReadCallbacks& callbacks) noexcept -> NativePersistenceResult;

// The owned UTF-16 path and staged physical bytes remain valid only during the
// atomicCommit callback. atomicCommit must return false without committing any
// destination mutation. A true return is Success; every target failure before
// that certain commit is Reject. The selected native continuations, not this
// pure adapter, own timestamp/state/close/unlock/status processing.
auto AdaptNativeStoreWrite(
    const NativeWriteRequest& request,
    const NativeWriteCallbacks& callbacks) noexcept -> NativePersistenceResult;

} // namespace ruffneckk::isc12
