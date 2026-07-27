#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace ruffneckk::readable_items {

inline constexpr std::size_t MaximumAudioFileBytes = 64U * 1024U * 1024U;
inline constexpr std::size_t MaximumDecodedAudioBytes = 128U * 1024U * 1024U;

struct PcmWaveView final {
    std::uint16_t channels{};
    std::uint32_t sampleRate{};
    std::uint16_t bitsPerSample{};
    std::uint16_t blockAlign{};
    std::size_t dataOffset{};
    std::size_t dataSize{};
};

struct DecodedFlac final {
    std::uint16_t channels{};
    std::uint32_t sampleRate{};
    std::uint16_t sourceBitsPerSample{};
    std::uint64_t frameCount{};
    std::vector<std::int32_t> samples;
};

DecodedFlac DecodeFlacToPcm32(std::span<const std::uint8_t> bytes);

inline std::uint16_t ReadLittleEndian16(
    std::span<const std::uint8_t> bytes,
    std::size_t offset
) {
    if (offset > bytes.size() || bytes.size() - offset < 2) {
        throw std::invalid_argument("truncated 16-bit WAV field");
    }
    return static_cast<std::uint16_t>(bytes[offset])
        | static_cast<std::uint16_t>(bytes[offset + 1]) << 8U;
}

inline std::uint32_t ReadLittleEndian32(
    std::span<const std::uint8_t> bytes,
    std::size_t offset
) {
    if (offset > bytes.size() || bytes.size() - offset < 4) {
        throw std::invalid_argument("truncated 32-bit WAV field");
    }
    return static_cast<std::uint32_t>(bytes[offset])
        | static_cast<std::uint32_t>(bytes[offset + 1]) << 8U
        | static_cast<std::uint32_t>(bytes[offset + 2]) << 16U
        | static_cast<std::uint32_t>(bytes[offset + 3]) << 24U;
}

inline bool HasChunkId(
    std::span<const std::uint8_t> bytes,
    std::size_t offset,
    std::string_view expected
) noexcept {
    if (expected.size() != 4 || offset > bytes.size()
        || bytes.size() - offset < expected.size()) {
        return false;
    }
    for (std::size_t index{}; index < expected.size(); ++index) {
        if (bytes[offset + index] != static_cast<std::uint8_t>(expected[index])) {
            return false;
        }
    }
    return true;
}

inline PcmWaveView ParsePcmWave(std::span<const std::uint8_t> bytes) {
    if (bytes.size() < 12 || !HasChunkId(bytes, 0, "RIFF")
        || !HasChunkId(bytes, 8, "WAVE")) {
        throw std::invalid_argument("audio file is not a RIFF/WAVE file");
    }
    if (bytes.size() > MaximumAudioFileBytes) {
        throw std::out_of_range("WAV file exceeds the 64 MiB limit");
    }

    const auto riffSize = static_cast<std::size_t>(ReadLittleEndian32(bytes, 4));
    if (riffSize < 4 || riffSize > bytes.size() - 8) {
        throw std::invalid_argument("invalid RIFF size");
    }

    bool foundFormat{};
    bool foundData{};
    std::uint16_t formatTag{};
    std::uint32_t bytesPerSecond{};
    PcmWaveView result;

    std::size_t cursor = 12;
    while (cursor <= bytes.size() && bytes.size() - cursor >= 8) {
        const auto chunkSize = static_cast<std::size_t>(
            ReadLittleEndian32(bytes, cursor + 4));
        const auto payload = cursor + 8;
        if (chunkSize > bytes.size() - payload) {
            throw std::invalid_argument("truncated WAV chunk");
        }

        if (HasChunkId(bytes, cursor, "fmt ") && !foundFormat) {
            if (chunkSize < 16) {
                throw std::invalid_argument("WAV format chunk is too small");
            }
            formatTag = ReadLittleEndian16(bytes, payload);
            result.channels = ReadLittleEndian16(bytes, payload + 2);
            result.sampleRate = ReadLittleEndian32(bytes, payload + 4);
            bytesPerSecond = ReadLittleEndian32(bytes, payload + 8);
            result.blockAlign = ReadLittleEndian16(bytes, payload + 12);
            result.bitsPerSample = ReadLittleEndian16(bytes, payload + 14);
            foundFormat = true;
        } else if (HasChunkId(bytes, cursor, "data") && !foundData) {
            result.dataOffset = payload;
            result.dataSize = chunkSize;
            foundData = true;
        }

        const auto paddedSize = chunkSize + (chunkSize & 1U);
        if (paddedSize > bytes.size() - payload) break;
        cursor = payload + paddedSize;
    }

    if (!foundFormat || !foundData) {
        throw std::invalid_argument("WAV file requires fmt and data chunks");
    }
    if (formatTag != 1) {
        throw std::invalid_argument("only uncompressed PCM WAV files are supported");
    }
    if (result.channels == 0 || result.channels > 2) {
        throw std::invalid_argument("WAV file must be mono or stereo");
    }
    if (result.sampleRate < 8000 || result.sampleRate > 192000) {
        throw std::out_of_range("WAV sample rate must be between 8 and 192 kHz");
    }
    if (result.bitsPerSample != 16) {
        throw std::invalid_argument("WAV file must use 16-bit PCM samples");
    }

    const auto expectedBlockAlign = static_cast<std::uint16_t>(
        result.channels * (result.bitsPerSample / 8U));
    const auto expectedBytesPerSecond = result.sampleRate * expectedBlockAlign;
    if (result.blockAlign != expectedBlockAlign
        || bytesPerSecond != expectedBytesPerSecond) {
        throw std::invalid_argument("WAV byte rate or block alignment is inconsistent");
    }
    if (result.dataSize == 0 || result.dataSize % result.blockAlign != 0) {
        throw std::invalid_argument("WAV sample data is empty or misaligned");
    }
    return result;
}

} // namespace ruffneckk::readable_items
