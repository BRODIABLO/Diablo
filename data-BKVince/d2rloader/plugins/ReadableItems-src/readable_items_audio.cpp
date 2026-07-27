#define DR_FLAC_IMPLEMENTATION
#include <dr_flac.h>

#include "readable_items_audio.hpp"

#include <limits>
#include <memory>
#include <stdexcept>

namespace ruffneckk::readable_items {
namespace {

struct FlacCloser final {
    void operator()(drflac* decoder) const noexcept {
        drflac_close(decoder);
    }
};

} // namespace

DecodedFlac DecodeFlacToPcm32(std::span<const std::uint8_t> bytes) {
    if (bytes.empty() || bytes.size() > MaximumAudioFileBytes) {
        throw std::out_of_range("FLAC file is empty or exceeds 64 MiB");
    }

    std::unique_ptr<drflac, FlacCloser> decoder{
        drflac_open_memory(bytes.data(), bytes.size(), nullptr)};
    if (!decoder) {
        throw std::invalid_argument("audio file is not a valid native FLAC stream");
    }
    if (decoder->channels == 0 || decoder->channels > 2) {
        throw std::invalid_argument("FLAC file must be mono or stereo");
    }
    if (decoder->sampleRate < 8000 || decoder->sampleRate > 192000) {
        throw std::out_of_range("FLAC sample rate must be between 8 and 192 kHz");
    }
    if (decoder->bitsPerSample == 0 || decoder->bitsPerSample > 32) {
        throw std::invalid_argument("FLAC bit depth must be between 1 and 32 bits");
    }
    if (decoder->totalPCMFrameCount == 0) {
        throw std::invalid_argument("FLAC stream must declare a non-empty frame count");
    }

    constexpr auto SampleBytes = sizeof(std::int32_t);
    const auto maximumFrames = MaximumDecodedAudioBytes
        / (static_cast<std::size_t>(decoder->channels) * SampleBytes);
    if (decoder->totalPCMFrameCount > maximumFrames
        || decoder->totalPCMFrameCount
            > std::numeric_limits<std::size_t>::max() / decoder->channels) {
        throw std::out_of_range("decoded FLAC audio exceeds the 128 MiB limit");
    }

    DecodedFlac result;
    result.channels = decoder->channels;
    result.sampleRate = decoder->sampleRate;
    result.sourceBitsPerSample = decoder->bitsPerSample;
    result.frameCount = decoder->totalPCMFrameCount;
    result.samples.resize(
        static_cast<std::size_t>(result.frameCount) * result.channels);

    const auto decodedFrames = drflac_read_pcm_frames_s32(
        decoder.get(), result.frameCount, result.samples.data());
    if (decodedFrames != result.frameCount) {
        throw std::invalid_argument("FLAC stream ended before its declared frame count");
    }
    return result;
}

} // namespace ruffneckk::readable_items
