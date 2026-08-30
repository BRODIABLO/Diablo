#include "isc12_native_persistence_adapter.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <utility>
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

enum class CallbackEvent : std::uint8_t {
    Replace,
    Clear,
    AtomicCommit,
};

struct CallbackState {
    std::array<CallbackEvent, 8> events{};
    std::size_t eventCount{};
    std::size_t replaceCalls{};
    std::size_t clearCalls{};
    std::size_t atomicCalls{};
    bool replaceSucceeds{true};
    bool clearSucceeds{true};
    bool atomicSucceeds{true};
    std::vector<std::uint8_t> readBuffer{0xA5, 0x5A};
    std::wstring committedPath{L"unchanged"};
    std::vector<std::uint8_t> committedBytes{0xCC};
};

auto Record(CallbackState& state, CallbackEvent event) noexcept -> void {
    if (state.eventCount < state.events.size()) {
        state.events[state.eventCount] = event;
    }
    ++state.eventCount;
}

auto ReplaceBuffer(
        void* context,
        std::span<const std::uint8_t> bytes) noexcept -> bool {
    auto& state = *static_cast<CallbackState*>(context);
    Record(state, CallbackEvent::Replace);
    ++state.replaceCalls;
    if (!state.replaceSucceeds) return false;
    try {
        state.readBuffer.assign(bytes.begin(), bytes.end());
        return true;
    } catch (...) {
        return false;
    }
}

auto ClearRejectedRead(void* context) noexcept -> bool {
    auto& state = *static_cast<CallbackState*>(context);
    Record(state, CallbackEvent::Clear);
    ++state.clearCalls;
    if (!state.clearSucceeds) return false;
    state.readBuffer.clear();
    return true;
}

auto AtomicCommit(
        void* context,
        std::wstring_view path,
        std::span<const std::uint8_t> bytes) noexcept -> bool {
    auto& state = *static_cast<CallbackState*>(context);
    Record(state, CallbackEvent::AtomicCommit);
    ++state.atomicCalls;
    if (!state.atomicSucceeds) return false;
    try {
        state.committedPath.assign(path);
        state.committedBytes.assign(bytes.begin(), bytes.end());
        return true;
    } catch (...) {
        return false;
    }
}

auto ReadCallbacks(CallbackState& state)
        -> ruffneckk::isc12::NativeReadCallbacks {
    return {
        .context = &state,
        .replaceBuffer = &ReplaceBuffer,
        .clearRejectedRead = &ClearRejectedRead,
    };
}

auto WriteCallbacks(CallbackState& state)
        -> ruffneckk::isc12::NativeWriteCallbacks {
    return {
        .context = &state,
        .atomicCommit = &AtomicCommit,
    };
}

auto WriteU32(
        std::span<std::uint8_t> bytes,
        std::size_t offset,
        std::uint32_t value) noexcept -> void {
    for (std::size_t index{}; index < sizeof(value); ++index) {
        bytes[offset + index] = static_cast<std::uint8_t>(
            value >> (index * 8U));
    }
}

auto NativePath(const std::string& path) noexcept -> std::span<const char> {
    return {path.c_str(), path.size() + 1};
}

} // namespace

int main() {
    using namespace ruffneckk::isc12;

    CHECK(NativeReadMaySnapshot(0, 0, 42, 42));
    CHECK(NativeReadMaySnapshot(0x1234, 0x1234, 42, 42));
    CHECK(!NativeReadMaySnapshot(5, 0, 42, 42));
    CHECK(!NativeReadMaySnapshot(0, 0, 42, 41));
    CHECK(!NativeReadMaySnapshot(
        0, 0, UINT64_C(0x100000000), UINT32_MAX));
    CHECK(NativeWriteLengthSupported(MaximumNativeInnerStoreLength));
    CHECK(!NativeWriteLengthSupported(
        MaximumNativeInnerStoreLength + 1));

    static_assert(NativePersistencePathCapacity == 0x30C);
    static_assert(noexcept(AdaptNativeStoreRead(
        std::declval<const NativeReadRequest&>(),
        std::declval<const NativeReadCallbacks&>())));
    static_assert(noexcept(AdaptNativeStoreWrite(
        std::declval<const NativeWriteRequest&>(),
        std::declval<const NativeWriteCallbacks&>())));

    Sha256Digest schemaHash{};
    for (std::size_t index{}; index < schemaHash.size(); ++index) {
        schemaHash[index] = static_cast<std::uint8_t>(index + 1);
    }
    std::array<std::uint8_t, 16> innerD2S{};
    WriteU32(innerD2S, 0, 0xAA55AA55);
    WriteU32(innerD2S, 4, InnerFormatVersion);
    WriteU32(innerD2S, 8, static_cast<std::uint32_t>(innerD2S.size()));
    WriteU32(innerD2S, 12, CalculateD2SChecksum(innerD2S));
    CHECK(ValidateInnerStore(StoreKind::D2S, innerD2S));
    std::vector<std::uint8_t> envelope;
    CHECK(BuildEnvelope(StoreKind::D2S, innerD2S, schemaHash, envelope)
        == EnvelopeError::None);

    {
        CallbackState state;
        const NativeReadRequest request{
            .storeName = "opaque.manager.object",
            .codecReady = false,
            .schemaHash = nullptr,
        };
        const NativeReadCallbacks callbacks{};
        const auto result = AdaptNativeStoreRead(request, callbacks);
        CHECK(result.disposition == NativePersistenceDisposition::Vanilla);
        CHECK(result.storeKind == StoreKind::Other);
        CHECK(state.eventCount == 0);

        const NativeWriteRequest writeRequest{
            .storeName = "opaque.manager.object",
            .nativePathUtf8 = {},
            .codecReady = false,
            .schemaHash = nullptr,
        };
        const NativeWriteCallbacks writeCallbacks{};
        const auto writeResult = AdaptNativeStoreWrite(
            writeRequest, writeCallbacks);
        CHECK(writeResult.disposition
            == NativePersistenceDisposition::Vanilla);
        CHECK(state.eventCount == 0);
    }

    {
        CallbackState state;
        const auto callbacks = ReadCallbacks(state);
        NativeReadRequest request{
            .storeName = "Hero.d2s",
            .codecReady = false,
            .schemaHash = &schemaHash,
        };
        auto result = AdaptNativeStoreRead(request, callbacks);
        CHECK(result.disposition == NativePersistenceDisposition::Reject);
        CHECK(result.error == NativePersistenceError::CodecNotReady);
        CHECK(state.clearCalls == 1);
        CHECK(state.replaceCalls == 0);

        CallbackState noSchemaState;
        request.codecReady = true;
        request.schemaHash = nullptr;
        result = AdaptNativeStoreRead(request, ReadCallbacks(noSchemaState));
        CHECK(result.disposition == NativePersistenceDisposition::Reject);
        CHECK(result.error == NativePersistenceError::SchemaUnavailable);
        CHECK(noSchemaState.clearCalls == 1);
        CHECK(noSchemaState.replaceCalls == 0);

        CallbackState failedCleanupState;
        failedCleanupState.clearSucceeds = false;
        request.codecReady = false;
        request.schemaHash = &schemaHash;
        result = AdaptNativeStoreRead(
            request, ReadCallbacks(failedCleanupState));
        CHECK(result.disposition == NativePersistenceDisposition::Fatal);
        CHECK(result.error == NativePersistenceError::ClearRejectedRead);
        CHECK(failedCleanupState.clearCalls == 1);

        CallbackState missingCleanupState;
        auto missingCleanupCallbacks = ReadCallbacks(missingCleanupState);
        missingCleanupCallbacks.clearRejectedRead = nullptr;
        result = AdaptNativeStoreRead(request, missingCleanupCallbacks);
        CHECK(result.disposition == NativePersistenceDisposition::Fatal);
        CHECK(result.error == NativePersistenceError::ClearRejectedRead);
        CHECK(missingCleanupState.eventCount == 0);
    }

    {
        CallbackState state;
        NativeReadRequest request{
            .storeName = "Hero.d2s",
            .codecReady = true,
            .schemaHash = &schemaHash,
            .readStatus = 0,
            .announcedLength = envelope.size(),
            .actualLength = envelope.size() - 1,
            .physicalBytes = envelope,
        };
        auto result = AdaptNativeStoreRead(request, ReadCallbacks(state));
        CHECK(result.disposition == NativePersistenceDisposition::Reject);
        CHECK(result.error == NativePersistenceError::StorePreparation);
        CHECK(result.persistenceError == PersistenceError::ReadLength);
        CHECK(state.clearCalls == 1);
        CHECK(state.replaceCalls == 0);

        auto invalidEnvelope = envelope;
        invalidEnvelope[0] ^= 0xFF;
        CallbackState invalidState;
        request.actualLength = invalidEnvelope.size();
        request.announcedLength = invalidEnvelope.size();
        request.physicalBytes = invalidEnvelope;
        result = AdaptNativeStoreRead(request, ReadCallbacks(invalidState));
        CHECK(result.disposition == NativePersistenceDisposition::Reject);
        CHECK(result.persistenceError == PersistenceError::Envelope);
        CHECK(result.envelopeError == EnvelopeError::Magic);
        CHECK(invalidState.clearCalls == 1);
        CHECK(invalidState.replaceCalls == 0);

        CallbackState validState;
        request.actualLength = envelope.size();
        request.announcedLength = envelope.size();
        request.physicalBytes = envelope;
        result = AdaptNativeStoreRead(request, ReadCallbacks(validState));
        CHECK(result.disposition == NativePersistenceDisposition::Success);
        CHECK(validState.replaceCalls == 1);
        CHECK(validState.clearCalls == 0);
        CHECK(validState.eventCount == 1);
        CHECK(validState.events[0] == CallbackEvent::Replace);
        CHECK(validState.readBuffer == std::vector<std::uint8_t>(
            innerD2S.begin(), innerD2S.end()));

        CallbackState replaceFailureState;
        replaceFailureState.replaceSucceeds = false;
        result = AdaptNativeStoreRead(
            request, ReadCallbacks(replaceFailureState));
        CHECK(result.disposition == NativePersistenceDisposition::Reject);
        CHECK(result.error == NativePersistenceError::ReplaceBuffer);
        CHECK(replaceFailureState.replaceCalls == 1);
        CHECK(replaceFailureState.clearCalls == 1);
        CHECK(replaceFailureState.eventCount == 2);
        CHECK(replaceFailureState.events[0] == CallbackEvent::Replace);
        CHECK(replaceFailureState.events[1] == CallbackEvent::Clear);

        CallbackState replaceCleanupFailureState;
        replaceCleanupFailureState.replaceSucceeds = false;
        replaceCleanupFailureState.clearSucceeds = false;
        result = AdaptNativeStoreRead(
            request, ReadCallbacks(replaceCleanupFailureState));
        CHECK(result.disposition == NativePersistenceDisposition::Fatal);
        CHECK(result.error == NativePersistenceError::ClearRejectedRead);
        CHECK(replaceCleanupFailureState.replaceCalls == 1);
        CHECK(replaceCleanupFailureState.clearCalls == 1);

        CallbackState missingReplaceState;
        auto missingReplaceCallbacks = ReadCallbacks(missingReplaceState);
        missingReplaceCallbacks.replaceBuffer = nullptr;
        result = AdaptNativeStoreRead(request, missingReplaceCallbacks);
        CHECK(result.disposition == NativePersistenceDisposition::Reject);
        CHECK(result.error == NativePersistenceError::ReplaceBuffer);
        CHECK(missingReplaceState.replaceCalls == 0);
        CHECK(missingReplaceState.clearCalls == 1);
        CHECK(missingReplaceState.eventCount == 1);
        CHECK(missingReplaceState.events[0] == CallbackEvent::Clear);
    }

    const std::string asciiPath{"C:\\save\\Hero.d2s"};
    {
        CallbackState codecState;
        NativeWriteRequest request{
            .storeName = "Hero.d2s",
            .nativePathUtf8 = NativePath(asciiPath),
            .codecReady = false,
            .schemaHash = &schemaHash,
            .innerBytes = innerD2S,
        };
        auto result = AdaptNativeStoreWrite(
            request, WriteCallbacks(codecState));
        CHECK(result.disposition == NativePersistenceDisposition::Reject);
        CHECK(result.error == NativePersistenceError::CodecNotReady);
        CHECK(codecState.atomicCalls == 0);
        CHECK(codecState.eventCount == 0);

        CallbackState schemaState;
        request.codecReady = true;
        request.schemaHash = nullptr;
        result = AdaptNativeStoreWrite(
            request, WriteCallbacks(schemaState));
        CHECK(result.disposition == NativePersistenceDisposition::Reject);
        CHECK(result.error == NativePersistenceError::SchemaUnavailable);
        CHECK(schemaState.atomicCalls == 0);
        CHECK(schemaState.eventCount == 0);
    }

    {
        const std::string traversalPath{"C:\\save\\..\\Hero.d2s"};
        CallbackState traversalState;
        NativeWriteRequest request{
            .storeName = "Hero.d2s",
            .nativePathUtf8 = NativePath(traversalPath),
            .codecReady = true,
            .schemaHash = &schemaHash,
            .innerBytes = innerD2S,
        };
        auto result = AdaptNativeStoreWrite(
            request, WriteCallbacks(traversalState));
        CHECK(result.disposition == NativePersistenceDisposition::Reject);
        CHECK(result.error == NativePersistenceError::PathTraversal);
        CHECK(traversalState.atomicCalls == 0);
        CHECK(traversalState.eventCount == 0);

        const std::string mismatchPath{"C:\\save\\Other.d2s"};
        CallbackState mismatchState;
        request.nativePathUtf8 = NativePath(mismatchPath);
        result = AdaptNativeStoreWrite(
            request, WriteCallbacks(mismatchState));
        CHECK(result.disposition == NativePersistenceDisposition::Reject);
        CHECK(result.error == NativePersistenceError::PathMismatch);
        CHECK(mismatchState.atomicCalls == 0);
        CHECK(mismatchState.eventCount == 0);

        const std::array<char, 8> unterminatedPath{
            'H','e','r','o','.','d','2','s'};
        CallbackState unterminatedState;
        request.nativePathUtf8 = unterminatedPath;
        result = AdaptNativeStoreWrite(
            request, WriteCallbacks(unterminatedState));
        CHECK(result.disposition == NativePersistenceDisposition::Reject);
        CHECK(result.error == NativePersistenceError::PathBuffer);
        CHECK(unterminatedState.eventCount == 0);

        std::array<char, NativePersistencePathCapacity + 1> overlongPath{};
        overlongPath[0] = 'x';
        overlongPath[1] = '\0';
        CallbackState overlongState;
        request.nativePathUtf8 = overlongPath;
        result = AdaptNativeStoreWrite(
            request, WriteCallbacks(overlongState));
        CHECK(result.disposition == NativePersistenceDisposition::Reject);
        CHECK(result.error == NativePersistenceError::PathBuffer);
        CHECK(overlongState.eventCount == 0);
    }

    {
        const std::string utf8StoreName{"H\xC3\xA9ros.d2s"};
        const std::string utf8Path{
            "C:\\sauvegardes\\Qu\xC3\xA9" "bec\\H\xC3\xA9ros.d2s"};
        CallbackState state;
        const NativeWriteRequest request{
            .storeName = utf8StoreName,
            .nativePathUtf8 = NativePath(utf8Path),
            .codecReady = true,
            .schemaHash = &schemaHash,
            .innerBytes = innerD2S,
        };
        const auto result = AdaptNativeStoreWrite(
            request, WriteCallbacks(state));
        CHECK(result.disposition == NativePersistenceDisposition::Success);
        CHECK(state.atomicCalls == 1);
        CHECK(state.eventCount == 1);
        CHECK(state.events[0] == CallbackEvent::AtomicCommit);
        CHECK(state.committedPath
            == L"C:\\sauvegardes\\Qu\u00E9bec\\H\u00E9ros.d2s");
        const auto validation = ValidateEnvelope(
            StoreKind::D2S, state.committedBytes, schemaHash);
        CHECK(validation);
        CHECK(validation.payload.size() == innerD2S.size());
    }

    {
        NativeWriteRequest request{
            .storeName = "Hero.d2s",
            .nativePathUtf8 = NativePath(asciiPath),
            .codecReady = true,
            .schemaHash = &schemaHash,
            .innerBytes = innerD2S,
        };
        CallbackState atomicFailureState;
        atomicFailureState.atomicSucceeds = false;
        auto result = AdaptNativeStoreWrite(
            request, WriteCallbacks(atomicFailureState));
        CHECK(result.disposition == NativePersistenceDisposition::Reject);
        CHECK(result.error == NativePersistenceError::AtomicCommit);
        CHECK(atomicFailureState.atomicCalls == 1);
        CHECK(atomicFailureState.eventCount == 1);
        CHECK(atomicFailureState.events[0] == CallbackEvent::AtomicCommit);
        CHECK(atomicFailureState.committedPath == L"unchanged");
        CHECK(atomicFailureState.committedBytes
            == std::vector<std::uint8_t>{0xCC});

        CallbackState missingAtomicState;
        auto missingAtomicCallbacks = WriteCallbacks(missingAtomicState);
        missingAtomicCallbacks.atomicCommit = nullptr;
        result = AdaptNativeStoreWrite(request, missingAtomicCallbacks);
        CHECK(result.disposition == NativePersistenceDisposition::Reject);
        CHECK(result.error == NativePersistenceError::AtomicCommit);
        CHECK(missingAtomicState.atomicCalls == 0);
        CHECK(missingAtomicState.eventCount == 0);

        CallbackState preparationFailureState;
        const std::array<std::uint8_t, 2> invalidInner{1, 2};
        request.innerBytes = invalidInner;
        result = AdaptNativeStoreWrite(
            request, WriteCallbacks(preparationFailureState));
        CHECK(result.disposition == NativePersistenceDisposition::Reject);
        CHECK(result.error == NativePersistenceError::StorePreparation);
        CHECK(result.persistenceError == PersistenceError::Envelope);
        CHECK(preparationFailureState.atomicCalls == 0);
        CHECK(preparationFailureState.eventCount == 0);
    }

    if (Failures != 0) {
        std::cerr << Failures << " native persistence adapter test(s) failed\n";
        return 1;
    }
    return 0;
}
