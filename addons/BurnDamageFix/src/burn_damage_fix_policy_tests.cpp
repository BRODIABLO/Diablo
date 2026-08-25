#include "burn_damage_fix_policy.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <limits>
#include <string>

namespace {
int Failures{};

#define CHECK(expression) \
    do { \
        if (!(expression)) { \
            std::cerr << "CHECK failed at line " << __LINE__ << ": " #expression "\n"; \
            ++Failures; \
        } \
    } while (false)

} // namespace

int main() {
    using namespace ruffneckk::burn_damage_fix;

    CHECK(IsSupportedBuild("92777"));
    CHECK(IsSupportedBuild("93847"));
    CHECK(!IsSupportedBuild(""));
    CHECK(!IsSupportedBuild("92778"));

    Config config{};
    CHECK(ShouldResolveBurn(config, 256, 75));
    CHECK(!ShouldResolveBurn(config, 0, 75));
    CHECK(!ShouldResolveBurn(config, 256, 0));
    config.applyFireResistance = false;
    CHECK(!ShouldResolveBurn(config, 256, 75));

    config = {};
    CHECK(!ShouldWitnessBurningState(config, 256, 75));
    config.diagnostics = true;
    CHECK(ShouldWitnessBurningState(config, 256, 75));
    CHECK(!ShouldWitnessBurningState(config, 0, 75));
    CHECK(!ShouldWitnessBurningState(config, 256, 0));
    config.enabled = false;
    CHECK(!ShouldWitnessBurningState(config, 256, 75));

    CHECK(NormalizeGenericNumerator(256, 0, 0, 0, 123) == 256);
    CHECK(NormalizeGenericNumerator(0, 128, 128, 0, 123) == 128);
    CHECK(NormalizeGenericNumerator(0, 128, 256, 0, 0) == 128);
    CHECK(NormalizeGenericNumerator(0, 128, 256, 0, 127) == 255);
    CHECK(NormalizeGenericNumerator(64, 128, 256, 100, 0) == 320);
    CHECK(NormalizeGenericNumerator(0, 256, 128, 0, 127) == 255);
    CHECK(NormalizeGenericNumerator(-1, 0, 0, 0, 0) == 0);
    CHECK(NormalizeGenericNumerator(
        std::numeric_limits<std::int32_t>::max(), 128, 128, 0, 0)
        == std::numeric_limits<std::int32_t>::max());

    constexpr std::string_view validToml = R"toml(
config_version = 1
enabled = true
[fixes]
normalize_generic_burn = true
apply_fire_resistance = true
[diagnostics]
enabled = false
)toml";
    std::string error;
    CHECK(ParseToml(validToml, config, error));
    CHECK(config.enabled && config.normalizeGenericBurn
        && config.applyFireResistance && !config.diagnostics);
    CHECK(!ParseToml("config_version = 2\nenabled = true\n", config, error));
    CHECK(!ParseToml(std::string(validToml) + "unknown = true\n", config, error));
    CHECK(!ParseToml(
        "config_version = 1\nenabled = 1\n[fixes]\n"
        "normalize_generic_burn = true\napply_fire_resistance = true\n"
        "[diagnostics]\nenabled = false\n",
        config,
        error));

    const auto candidates = BuildConfigCandidates(
        std::filesystem::path{L"mod"},
        std::filesystem::path{L"scope"},
        std::filesystem::path{L"global"},
        std::filesystem::path{L"burn-damage-fix.toml"});
    CHECK(candidates.size() == 3);
    CHECK(candidates.front()
        == std::filesystem::path{L"mod/burn-damage-fix.toml"});
    const auto deduplicated = BuildConfigCandidates(
        std::filesystem::path{L"same"},
        std::filesystem::path{L"same"},
        std::filesystem::path{L"global"},
        std::filesystem::path{L"burn-damage-fix.toml"});
    CHECK(deduplicated.size() == 2);

    CHECK(CanEncodeRel32(0x14044CB32ULL, 0x140500000ULL));
    CHECK(!CanEncodeRel32(0x14044CB32ULL, 0x240500000ULL));

    return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
