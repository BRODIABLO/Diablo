#include "readable_items_config.hpp"
#include "readable_items_audio.hpp"

#include <nlohmann/json.hpp>

#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using ruffneckk::readable_items::DefaultTooltip;
using ruffneckk::readable_items::DecodeFlacToPcm32;
using ruffneckk::readable_items::FindEntry;
using ruffneckk::readable_items::IsReadablePSpell;
using ruffneckk::readable_items::PackItemCode;
using ruffneckk::readable_items::ParsePcmWave;
using ruffneckk::readable_items::ParseConfig;
using ruffneckk::readable_items::ReaderState;
using ruffneckk::readable_items::ScrollOffsetFromTrack;

void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

template<class Callback>
void RequireRejected(Callback callback, const char* message) {
    try {
        callback();
    } catch (const std::exception&) {
        return;
    }
    throw std::runtime_error(message);
}

} // namespace

int main(int argc, char** argv) {
    try {
        Require(argc <= 2, "expected at most one configuration path");

        const auto valid = nlohmann::json::parse(R"json(
        {
          // Comments are accepted by the runtime JSON reader.
          "enabled": true,
          "items": [
            {
              "code":"dmy",
              "title":"Clue Scroll Test",
              "text":"Follow the red stones.",
              "audioFile":"data/global/sfx/item/readable_items_test.wav"
            }
          ]
        })json", nullptr, true, true);

        const auto config = ParseConfig(valid);
        Require(config.enabled, "valid configuration must be enabled");
        Require(config.tooltip == DefaultTooltip, "default tooltip changed unexpectedly");
        Require(config.items.size() == 1, "valid fixture was not loaded");
        Require(IsReadablePSpell(-2), "the Readable Items pSpell sentinel changed");
        Require(!IsReadablePSpell(-1) && !IsReadablePSpell(0)
                && !IsReadablePSpell(2) && !IsReadablePSpell(14)
                && !IsReadablePSpell(15) && !IsReadablePSpell(16),
                "a vanilla or unproven pSpell unexpectedly activates Readable Items");
        Require(PackItemCode("tsc") == 0x20637374U,
                "three-character item codes must use ItemsTxt space padding");
        Require(PackItemCode("a") == 0x20202061U,
                "short item codes must be padded to four bytes");
        Require(PackItemCode("abcd") == 0x64636261U,
                "four-character item codes must retain all bytes");
        const auto* fixture = FindEntry(config, PackItemCode("dmy"));
        Require(fixture != nullptr, "packed dmy code was not resolved");
        Require(fixture->title == "Clue Scroll Test", "fixture title changed unexpectedly");
        Require(fixture->audioFile == "data/global/sfx/item/readable_items_test.wav",
                "optional audio path was not preserved");

        const auto flacConfig = ParseConfig(nlohmann::json{
            {"items", nlohmann::json::array({
                {{"code", "flc"}, {"title", "FLAC"}, {"text", "Lossless"},
                 {"audioFile", "audio/test.FLAC"}}
            })}});
        Require(flacConfig.items.front().audioFile == "audio/test.FLAC",
                "FLAC audio path was not accepted");

        ReaderState reader;
        Require(reader.Open(*fixture, 40, 8), "valid fixture did not open");
        Require(reader.IsOpen(), "reader must be open after a valid activation");
        Require(reader.PackedCode() == PackItemCode("dmy"), "active code was not retained");
        Require(reader.Title() == fixture->title, "active title was not retained");
        Require(reader.Text() == fixture->text, "active text was not retained");
        Require(reader.ScrollOffset() == 0, "reader must open at the first line");
        Require(reader.FollowingLatest(), "new dialogue must follow the newest line");
        Require(!reader.CanScrollUp(), "reader must not scroll above the first line");
        Require(reader.CanScrollDown(), "long text must permit downward scrolling");

        reader.SetRevealCharacterCount(12);
        reader.AdvanceReveal(3);
        Require(reader.RevealedCharacters() == 3, "progressive reveal did not advance");
        Require(!reader.RevealComplete(), "partial dialogue was marked complete");
        reader.AdvanceReveal(1000);
        Require(reader.RevealComplete(), "dialogue reveal was not clamped at completion");
        reader.SetRevealCharacterCount(5);
        Require(reader.RevealedCharacters() == 5,
                "shrinking reveal content did not clamp progress");

        reader.ScrollLines(1);
        Require(reader.ScrollOffset() == 1, "single-line downward scroll failed");
        Require(!reader.FollowingLatest(), "manual review must pause automatic following");
        reader.ScrollLines(1000);
        Require(reader.ScrollOffset() == 32, "downward scroll was not clamped");
        Require(reader.FollowingLatest(), "reaching the latest line must resume following");
        Require(!reader.CanScrollDown(), "reader must stop at the final viewport");
        reader.ScrollLines(-3);
        Require(reader.ScrollOffset() == 29, "upward scroll failed");
        reader.ScrollLines(-1000);
        Require(reader.ScrollOffset() == 0, "upward scroll was not clamped");

        reader.SetScrollOffset(12);
        Require(reader.ScrollOffset() == 12, "absolute scrollbar movement failed");
        Require(!reader.FollowingLatest(),
                "dragging away from the end must pause automatic following");
        reader.SetScrollOffset(1000);
        Require(reader.ScrollOffset() == 32,
                "absolute scrollbar movement was not clamped");
        Require(reader.FollowingLatest(),
                "dragging to the end must resume automatic following");

        Require(ScrollOffsetFromTrack(100.0F, 100.0F, 300.0F, 40.0F, 20.0F, 32) == 0,
                "scrollbar pointer before the track was not clamped");
        Require(ScrollOffsetFromTrack(200.0F, 100.0F, 300.0F, 40.0F, 20.0F, 32) == 16,
                "scrollbar midpoint did not map to the midpoint offset");
        Require(ScrollOffsetFromTrack(500.0F, 100.0F, 300.0F, 40.0F, 20.0F, 32) == 32,
                "scrollbar pointer after the track was not clamped");
        Require(ScrollOffsetFromTrack(200.0F, 100.0F, 100.0F, 40.0F, 20.0F, 32) == 0,
                "degenerate scrollbar track must stay at offset zero");

        reader.ResumeFollowingLatest();
        reader.FollowLatestLine(15);
        Require(reader.ScrollOffset() == 8,
                "automatic dialogue scrolling did not keep the newest line visible");

        reader.SetViewport(4, 8);
        Require(reader.ScrollOffset() == 0, "short text must reset an obsolete offset");
        Require(!reader.CanScrollDown(), "short text must not scroll");
        reader.SetViewport(40, 0);
        reader.ScrollLines(5);
        Require(reader.ScrollOffset() == 0, "zero-height viewport must not scroll");

        const ruffneckk::readable_items::Entry second{
            PackItemCode("dm2"), "dm2", "Second clue", "A second message."};
        Require(reader.Open(second, 2, 8), "second fixture did not open");
        Require(reader.PackedCode() == PackItemCode("dm2"), "reader did not switch objects");
        Require(reader.ScrollOffset() == 0, "switching objects must reset scrolling");

        const ruffneckk::readable_items::Entry invalid{};
        Require(!reader.Open(invalid, 1, 1), "invalid entry must not replace active content");
        Require(reader.PackedCode() == PackItemCode("dm2"),
                "rejected entry unexpectedly replaced active content");
        reader.Close();
        Require(!reader.IsOpen(), "reader did not close");
        Require(reader.PackedCode() == 0, "closing must clear the active code");
        Require(reader.Title().empty() && reader.Text().empty(),
                "closing must clear active content");

        const auto disabled = ParseConfig(nlohmann::json{
            {"enabled", false}, {"tooltip", "Read it"}, {"items", nlohmann::json::array()}});
        Require(!disabled.enabled, "disabled empty configuration must remain valid");
        Require(disabled.tooltip == "Read it", "custom tooltip was not preserved");

        RequireRejected([] {
            ParseConfig(nlohmann::json{{"enabled", true}, {"items", nlohmann::json::array()}});
        }, "enabled empty configuration must be rejected");

        RequireRejected([] {
            ParseConfig(nlohmann::json{
                {"items", nlohmann::json::array({
                    {{"code", "dmy"}, {"title", "First"}, {"text", "One"}},
                    {{"code", "dmy"}, {"title", "Second"}, {"text", "Two"}}
                })}});
        }, "duplicate item codes must be rejected");

        RequireRejected([] {
            ParseConfig(nlohmann::json{
                {"items", nlohmann::json::array({
                    {{"code", "too-long"}, {"title", "Bad"}, {"text", "Bad"}}
                })}});
        }, "oversized item codes must be rejected");

        RequireRejected([] {
            ParseConfig(nlohmann::json{
                {"items", nlohmann::json::array({
                    {{"code", "dmy"}, {"title", "Bad"}, {"text", "Bad"}, {"audioFile", "../test.wav"}}
                })}});
        }, "parent traversal in an audio path must be rejected");

        RequireRejected([] {
            ParseConfig(nlohmann::json{
                {"items", nlohmann::json::array({
                    {{"code", "dmy"}, {"title", "Bad"}, {"text", "Bad"}, {"audioFile", "C:\\test.wav"}}
                })}});
        }, "absolute audio paths must be rejected");

        RequireRejected([] {
            ParseConfig(nlohmann::json{
                {"items", nlohmann::json::array({
                    {{"code", "dmy"}, {"title", "Bad"}, {"text", "Bad"}, {"audioFile", "audio/test.mp3"}}
                })}});
        }, "unsupported audio extensions must be rejected");

        RequireRejected([] {
            const std::vector<std::uint8_t> invalidFlac{'f', 'L', 'a', 'C', 0, 0, 0, 0};
            (void)DecodeFlacToPcm32(invalidFlac);
        }, "truncated FLAC data must be rejected");

        RequireRejected([] {
            ParseConfig(nlohmann::json{
                {"unknown", true},
                {"items", nlohmann::json::array({
                    {{"code", "dmy"}, {"title", "Bad"}, {"text", "Bad"}}
                })}});
        }, "unknown root setting must be rejected");

        if (argc == 2) {
            std::ifstream input(argv[1]);
            Require(input.good(), "bundled configuration could not be opened");
            const auto bundledJson = nlohmann::json::parse(input, nullptr, true, true);
            const auto bundled = ParseConfig(bundledJson);
            Require(bundled.enabled, "bundled configuration must be enabled");
            Require(FindEntry(bundled, PackItemCode("dmy")) != nullptr,
                    "bundled configuration must contain the dmy fixture");
            const auto* witness = FindEntry(bundled, PackItemCode("rds"));
            Require(witness != nullptr,
                    "bundled configuration must contain the spawnable rds witness");
            Require(!witness->audioFile.empty(),
                    "bundled rds witness must configure an audio file");

            const auto audioPath = std::filesystem::path(argv[1]).parent_path()
                / std::filesystem::path(witness->audioFile);
            std::ifstream audio(audioPath, std::ios::binary | std::ios::ate);
            Require(audio.good(), "bundled audio witness could not be opened");
            const auto audioSize = audio.tellg();
            Require(audioSize > 0, "bundled audio witness is empty");
            std::vector<std::uint8_t> audioBytes(static_cast<std::size_t>(audioSize));
            audio.seekg(0, std::ios::beg);
            audio.read(
                reinterpret_cast<char*>(audioBytes.data()),
                static_cast<std::streamsize>(audioBytes.size()));
            Require(audio.good(), "bundled audio witness could not be read");
            if (audioPath.extension() == ".flac") {
                const auto flac = DecodeFlacToPcm32(audioBytes);
                Require(flac.channels == 2, "bundled FLAC witness must be stereo");
                Require(flac.sampleRate == 48000,
                        "bundled FLAC witness must use a 48 kHz sample rate");
                Require(flac.sourceBitsPerSample == 16,
                        "bundled FLAC witness must preserve the 16-bit source");
                Require(flac.frameCount > 0 && !flac.samples.empty(),
                        "bundled FLAC witness decoded no samples");
            } else {
                const auto wave = ParsePcmWave(audioBytes);
                Require(wave.channels == 2, "bundled WAV witness must be stereo");
                Require(wave.sampleRate == 48000,
                        "bundled WAV witness must use a 48 kHz sample rate");
                Require(wave.bitsPerSample == 16,
                        "bundled WAV witness must use 16-bit PCM samples");
            }
        }

        std::cout << "ReadableItems configuration tests: VALID\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& exception) {
        std::cerr << "ReadableItems configuration tests: INVALID ("
                  << exception.what() << ")\n";
        return EXIT_FAILURE;
    }
}
