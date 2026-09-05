#include "doll_explosion_policy.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>

using namespace ruffneckk::doll_explosion;

namespace {

int Failures{};

#define CHECK(expression)                                                      \
    do {                                                                       \
        if (!(expression)) {                                                   \
            std::cerr << __FILE__ << ':' << __LINE__                           \
                      << ": CHECK failed: " #expression << '\n';               \
            ++Failures;                                                        \
        }                                                                      \
    } while (false)

auto ReadFile(const std::filesystem::path& path) -> std::string {
    std::ifstream input(path, std::ios::binary);
    return {
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>(),
    };
}

constexpr std::string_view ValidToml = R"toml(
config_version = 1

[targets]
monstats_ids = [212, 213, 214, 215, 216, 690, 691]

[explosion]
delay_frames = 25
radius = 4

[damage]
formula = "fixed"

[damage.fixed]
normal = [18, 30]
nightmare = [54, 96]
hell = [318, 540]

[damage.source_max_life_percent]
normal = [30, 50]
nightmare = [21, 35]
hell = [12, 20]

[diagnostics]
show_usage_counters = false
)toml";

auto Parses(std::string_view text) -> bool {
    Config config{};
    std::string error;
    return ParseToml(text, config, error);
}

auto Replace(
        std::string text,
        std::string_view from,
        std::string_view to) -> std::string {
    const auto position = text.find(from);
    if (position != std::string::npos) {
        text.replace(position, from.size(), to);
    }
    return text;
}

void TestDefaultsAndPolicy() {
    const Config defaults{};
    CHECK(defaults.schemaVersion == 1);
    CHECK(defaults.targetMonsterIds.size() == 7);
    for (const auto id : {212, 213, 214, 215, 216, 690, 691}) {
        CHECK(IsTargetMonster(defaults, id));
    }
    CHECK(!IsTargetMonster(defaults, 777));
    CHECK(defaults.delayFrames == 25);
    CHECK(defaults.radius == 4);
    CHECK(defaults.formula == DamageFormula::Fixed);
    CHECK((defaults.fixed.normal == InclusiveRange{18, 30}));
    CHECK((defaults.fixed.nightmare == InclusiveRange{54, 96}));
    CHECK((defaults.fixed.hell == InclusiveRange{318, 540}));

    CHECK(SelectRange(defaults.fixed, 0) == &defaults.fixed.normal);
    CHECK(SelectRange(defaults.fixed, 1) == &defaults.fixed.nightmare);
    CHECK(SelectRange(defaults.fixed, 2) == &defaults.fixed.hell);
    CHECK(SelectRange(defaults.fixed, 3) == nullptr);
    CHECK(InclusiveSpan({18, 30}) == 13);
    CHECK(ApplyInclusiveRoll({18, 30}, 0) == 18);
    CHECK(ApplyInclusiveRoll({18, 30}, 12) == 30);
    CHECK(ScaleFixedDamage(18) == 18 * 256);
    CHECK(ScaleFixedDamage(540) == 540 * 256);
    CHECK(!ScaleFixedDamage(MaximumFixedDamage + 1).has_value());
    CHECK(ScaleMaxLifePercent(100 * 256, 30) == 30 * 256);
    CHECK(ScaleMaxLifePercent(101 * 256, 35) == 9049);
    CHECK(!ScaleMaxLifePercent(0, 30).has_value());
    CHECK(!ScaleMaxLifePercent(256, 101).has_value());
}

void TestParsing() {
    Config config{};
    std::string error;
    CHECK(ParseToml(ValidToml, config, error));
    CHECK(error.empty());
    CHECK(config.targetMonsterIds.size() == 7);
    CHECK(config.delayFrames == 25);
    CHECK(config.radius == 4);
    CHECK(config.formula == DamageFormula::Fixed);
    CHECK(!config.diagnostics);

    auto percent = Replace(
        std::string(ValidToml),
        "formula = \"fixed\"",
        "formula = \"source_max_life_percent\"");
    CHECK(ParseToml(percent, config, error));
    CHECK(config.formula == DamageFormula::SourceMaxLifePercent);

    CHECK(!Parses(Replace(
        std::string(ValidToml), "config_version = 1", "config_version = 2")));
    CHECK(!Parses(Replace(
        std::string(ValidToml), "config_version = 1\n", "")));
    CHECK(!Parses(std::string(ValidToml) + "\nunknown = true\n"));
    CHECK(!Parses(Replace(
        std::string(ValidToml),
        "monstats_ids = [212, 213, 214, 215, 216, 690, 691]",
        "monstats_ids = []")));
    CHECK(!Parses(Replace(
        std::string(ValidToml),
        "monstats_ids = [212, 213, 214, 215, 216, 690, 691]",
        "monstats_ids = [212, 212]")));
    CHECK(!Parses(Replace(
        std::string(ValidToml),
        "monstats_ids = [212, 213, 214, 215, 216, 690, 691]",
        "monstats_ids = [65536]")));
    CHECK(!Parses(Replace(
        std::string(ValidToml), "delay_frames = 25", "delay_frames = 32768")));
    CHECK(!Parses(Replace(
        std::string(ValidToml), "radius = 4", "radius = 0")));
    CHECK(!Parses(Replace(
        std::string(ValidToml), "radius = 4", "radius = 65")));
    CHECK(!Parses(Replace(
        std::string(ValidToml), "formula = \"fixed\"", "formula = \"script\"")));
    CHECK(!Parses(Replace(
        std::string(ValidToml), "normal = [18, 30]", "normal = [31, 30]")));
    CHECK(!Parses(Replace(
        std::string(ValidToml),
        "normal = [18, 30]",
        "normal = [0, 8388608]")));
    CHECK(!Parses(Replace(
        std::string(ValidToml),
        "normal = [30, 50]",
        "normal = [30, 101]")));
    CHECK(!Parses(Replace(
        std::string(ValidToml),
        "show_usage_counters = false",
        "show_usage_counters = 0")));
    CHECK(!Parses(Replace(
        std::string(ValidToml), "[diagnostics]", "[other]")));
    CHECK(!Parses(Replace(
        std::string(ValidToml),
        "radius = 4",
        "radius = 4\nextra = true")));
}

void TestConfigCandidates() {
    const auto candidates = BuildConfigCandidates(
        L"C:/D2R/mods/BKVince/d2rloader/config",
        L"C:/D2R/mods/BKVince/d2rloader/config",
        L"C:/D2R/d2rloader/config",
        L"ruffneckk-doll-explosion.toml");
    CHECK(candidates.size() == 2);
    CHECK(candidates[0].generic_wstring().find(L"mods/BKVince")
        != std::wstring::npos);
    CHECK(candidates[1].generic_wstring().find(L"d2rloader/config")
        != std::wstring::npos);
}

void TestSourceContracts() {
    const auto plugin = ReadFile(DOLL_EXPLOSION_PLUGIN_SOURCE);
    const auto native = ReadFile(DOLL_EXPLOSION_NATIVE_SOURCE);
    const auto cmake = ReadFile(DOLL_EXPLOSION_CMAKE_SOURCE);
    const auto toml = ReadFile(DOLL_EXPLOSION_TOML_SOURCE);

    CHECK(plugin.find(".author = \"RuffnecKk\"") != std::string::npos);
    CHECK(plugin.find("PluginFlags::Shared | D2RL::PluginFlags::NativeHooks")
        != std::string::npos);
    CHECK(plugin.find("ModScopedOnly") == std::string::npos);
    CHECK(plugin.find("GetBuildName(context)") != std::string::npos);
    CHECK(plugin.find("diagnostic only; validating the complete native fingerprint")
        != std::string::npos);
    CHECK(plugin.find("std::strcmp(runtimeBuild") == std::string::npos);
    CHECK(plugin.find("\"92777\"") == std::string::npos);
    CHECK(plugin.find("\"93847\"") == std::string::npos);
    CHECK(plugin.find("DelayCarrierMissileId = 587") != std::string::npos);
    CHECK(plugin.find("CorpseExplosionMissileId = 117") != std::string::npos);
    CHECK(plugin.find("if (originalResult != 2)") != std::string::npos);
    CHECK(plugin.find("SetMissileTotalFrames(missile, delayFrames)")
        != std::string::npos);
    CHECK(plugin.find("SetMissileCurrentFrames(missile, delayFrames)")
        != std::string::npos);
    CHECK(plugin.find("GetMissileTotalFrames(missile) == delayFrames")
        != std::string::npos);
    CHECK(plugin.find("GetMissileCurrentFrames(missile) == delayFrames")
        != std::string::npos);
    CHECK(plugin.find("GameplayEventKind::GameJoined")
        != std::string::npos);
    CHECK(plugin.find("GameplayEventKind::GameLeft")
        != std::string::npos);
    CHECK(plugin.find("ClearCarriers();") != std::string::npos);
    CHECK(plugin.find("OriginalDeathMode(game, modeChange)")
        != std::string::npos);
    CHECK(plugin.find("CheckState(identity.unit, ReviveStateId)")
        != std::string::npos);
    CHECK(plugin.find("MonStatsDeathDamageMask") != std::string::npos);
    CHECK(plugin.find("armageddoncontrol") == std::string::npos);
    CHECK(plugin.find("PluginPack") == std::string::npos);
    CHECK(plugin.find("eezstreet") == std::string::npos);

    CHECK(native.find("DeathModeRva = 0x444F50") != std::string::npos);
    CHECK(native.find("GenericMissileRva = 0x466B40")
        != std::string::npos);
    CHECK(native.find("ServerDoTableRva = 0x2380E80")
        != std::string::npos);
    CHECK(native.find("ApplyAreaDamageRva = 0x44A120")
        != std::string::npos);
    CHECK(native.find("GetMissileCurrentFramesRva = 0x3BB1E0")
        != std::string::npos);
    CHECK(native.find("GetMissileTotalFramesRva = 0x3BC3E0")
        != std::string::npos);
    CHECK(native.find("SetMissileCurrentFramesRva = 0x3BD450")
        != std::string::npos);
    CHECK(native.find("SetMissileTotalFramesRva = 0x3BDBC0")
        != std::string::npos);
    CHECK(cmake.find("4933e2c42cb2592958cd0df3b6dc5003102252d1")
        != std::string::npos);
    CHECK(cmake.find("RUFFNECKK_PUBLIC_ARCHIVE_ELIGIBLE FALSE")
        != std::string::npos);

    Config shipped{};
    std::string error;
    CHECK(ParseToml(toml, shipped, error));
    CHECK(shipped.delayFrames == 25);
    CHECK(shipped.radius == 4);
    CHECK(shipped.formula == DamageFormula::Fixed);
    CHECK(IsTargetMonster(shipped, 691));
    CHECK(!IsTargetMonster(shipped, 777));
    CHECK(toml.find("enabled =") == std::string::npos);
}

} // namespace

int main() {
    TestDefaultsAndPolicy();
    TestParsing();
    TestConfigCandidates();
    TestSourceContracts();
    return Failures == 0 ? 0 : 1;
}
