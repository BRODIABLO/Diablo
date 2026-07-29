#include "book_of_lore_config.hpp"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

namespace {
using namespace ruffneckk::book_of_lore;

template <typename Function>
void ExpectFailure(Function&& function) {
    bool failed{};
    try {
        function();
    } catch (const std::exception&) {
        failed = true;
    }
    assert(failed);
}

struct TemporaryDirectory {
    std::filesystem::path path;

    TemporaryDirectory() {
        const auto suffix = std::chrono::high_resolution_clock::now()
            .time_since_epoch().count();
        path = std::filesystem::temp_directory_path()
            / ("book-of-lore-tests-" + std::to_string(suffix));
        std::filesystem::create_directories(path);
    }

    ~TemporaryDirectory() {
        std::error_code error;
        std::filesystem::remove_all(path, error);
    }
};

void WriteText(const std::filesystem::path& path, std::string_view text) {
    std::ofstream output(path, std::ios::binary);
    assert(output.is_open());
    output << text;
}
} // namespace

int main(int argc, char** argv) {
    using namespace ruffneckk::book_of_lore;

    const auto parsed = ParseConfig(toml::parse(R"toml(
        # TOML comments and multiline strings are supported.
        enabled = true

        [[messages]]
        id = "act1.road"
        title = "The Forgotten Road"
        text = """
        Welcome, ##00.
        The road remembers you.
        """
        scroll_speed = 24
        all_same = true

        [messages.filters]
        difficulty = 2
        act = 2
        area = 40
        quest = "a1q1"
        player_class = "ama"
        min_level = 20
        max_level = 10
        town = false
    )toml"));
    assert(parsed.enabled);
    assert(parsed.messages.size() == 1);
    assert(parsed.messages[0].id == "act1.road");
    assert(parsed.messages[0].text.find("The road remembers you.") != std::string::npos);
    assert(parsed.messages[0].scrollSpeed == 24);
    assert(parsed.messages[0].allSame);
    assert(parsed.messages[0].filters.maxLevel == 10);

    ExpectFailure([] {
        ParseConfig(toml::parse("enabled = false\nunknown = 1\n"));
    });
    ExpectFailure([] {
        ParseConfig(toml::parse(R"toml(
            [[messages]]
            id = "same"
            title = "One"
            text = "One"
            [[messages]]
            id = "same"
            title = "Two"
            text = "Two"
        )toml"));
    });
    ExpectFailure([] {
        ParseConfig(toml::parse(R"toml(
            [[messages]]
            id = "bad id"
            title = "Bad"
            text = "Bad"
        )toml"));
    });
    ExpectFailure([] {
        ParseConfig(toml::parse(R"toml(
            [[messages]]
            id = "bad-class"
            title = "Bad"
            text = "Bad"
            [messages.filters]
            player_class = "Amazon"
        )toml"));
    });
    ExpectFailure([] {
        ParseConfig(toml::parse(R"toml(
            [[messages]]
            id = "bad-filter"
            title = "Bad"
            text = "Bad"
            [messages.filters]
            difficulty = 4
        )toml"));
    });

    const auto candidates = BuildConfigCandidates(
        std::filesystem::path("active-mod"),
        std::filesystem::path("game")
    );
    assert(candidates.size() == 2);
    assert(candidates[0] == std::filesystem::path("active-mod") / ConfigFileName);
    assert(candidates[1] == std::filesystem::path("game") / ConfigFileName);

    TemporaryDirectory temporary;
    const auto mod = temporary.path / "mod";
    const auto game = temporary.path / "game";
    std::filesystem::create_directories(mod);
    std::filesystem::create_directories(game);
    WriteText(game / ConfigFileName, "enabled = true\nmessages = []\n");
    WriteText(mod / ConfigFileName, "enabled = \"invalid\"\n");
    ExpectFailure([&] { LoadConfiguration(mod, game); });

    WriteText(mod / ConfigFileName, "enabled = false\nmessages = []\n");
    auto loaded = LoadConfiguration(mod, game);
    assert(loaded.path == mod / ConfigFileName);
    assert(!loaded.config.enabled);

    std::filesystem::remove(mod / ConfigFileName);
    loaded = LoadConfiguration(mod, game);
    assert(loaded.path == game / ConfigFileName);
    assert(loaded.config.enabled);

    std::filesystem::remove(game / ConfigFileName);
    loaded = LoadConfiguration(mod, game);
    assert(!loaded.path);
    assert(!loaded.config.enabled);
    assert(loaded.config.messages.empty());

    if (argc == 2) {
        const auto bundled = LoadConfigFile(argv[1]);
        assert(!bundled.enabled);
        assert(bundled.messages.size() == 2);
    }
    return 0;
}
