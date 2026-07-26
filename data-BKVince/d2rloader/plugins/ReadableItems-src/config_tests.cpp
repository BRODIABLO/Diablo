#include "readable_items_config.hpp"

#include <nlohmann/json.hpp>

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
        const auto* fixture = FindEntry(config, PackItemCode("dmy"));
        Require(fixture != nullptr, "packed dmy code was not resolved");
        Require(fixture->title == "Clue Scroll Test", "fixture title changed unexpectedly");

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
        }

        std::cout << "ReadableItems configuration tests: VALID\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& exception) {
        std::cerr << "ReadableItems configuration tests: INVALID ("
                  << exception.what() << ")\n";
        return EXIT_FAILURE;
    }
}
