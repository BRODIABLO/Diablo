#include "readable_items_config.hpp"

#include <nlohmann/json.hpp>

#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

using ruffneckk::readable_items::DefaultTooltip;
using ruffneckk::readable_items::FindEntry;
using ruffneckk::readable_items::PackItemCode;
using ruffneckk::readable_items::ParseConfig;
using ruffneckk::readable_items::ReaderState;

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
            {"code":"dmy", "title":"Clue Scroll Test", "text":"Follow the red stones."}
          ]
        })json", nullptr, true, true);

        const auto config = ParseConfig(valid);
        Require(config.enabled, "valid configuration must be enabled");
        Require(config.tooltip == DefaultTooltip, "default tooltip changed unexpectedly");
        Require(config.items.size() == 1, "valid fixture was not loaded");
        Require(PackItemCode("tsc") == 0x20637374U,
                "three-character item codes must use ItemsTxt space padding");
        Require(PackItemCode("a") == 0x20202061U,
                "short item codes must be padded to four bytes");
        Require(PackItemCode("abcd") == 0x64636261U,
                "four-character item codes must retain all bytes");
        const auto* fixture = FindEntry(config, PackItemCode("dmy"));
        Require(fixture != nullptr, "packed dmy code was not resolved");
        Require(fixture->title == "Clue Scroll Test", "fixture title changed unexpectedly");

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
                    {{"code", "dmy"}, {"title", "Bad"}, {"text", "Bad"}, {"audioFile", "test.wav"}}
                })}});
        }, "phase-two audio setting must not be silently accepted");

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
            Require(FindEntry(bundled, PackItemCode("tsc")) != nullptr,
                    "bundled configuration must contain the spawnable tsc witness");
        }

        std::cout << "ReadableItems configuration tests: VALID\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& exception) {
        std::cerr << "ReadableItems configuration tests: INVALID ("
                  << exception.what() << ")\n";
        return EXIT_FAILURE;
    }
}
