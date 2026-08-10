#include "bkv_combat_config.hpp"
#include "bkv_combat_policy.hpp"

#include <nlohmann/json.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

int Failures{};

void Check(bool condition, const char* expression, int line) {
    if (condition) return;
    std::cerr << "FAIL line " << line << ": " << expression << '\n';
    ++Failures;
}

#define CHECK(expression) Check(static_cast<bool>(expression), #expression, __LINE__)

template <class Callable>
bool Throws(Callable&& callable) {
    try {
        callable();
        return false;
    } catch (const std::exception&) {
        return true;
    }
}

nlohmann::json ReadJson(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input.is_open()) {
        throw std::runtime_error("cannot open test configuration " + path.string());
    }
    return nlohmann::json::parse(input);
}

std::vector<RuffnecKk::BKVCombat::ResolvedMonStatsIdentity> ResolveAll(
    const RuffnecKk::BKVCombat::Config& config) {
    std::vector<RuffnecKk::BKVCombat::ResolvedMonStatsIdentity> result;
    result.reserve(config.classifications.majorBosses.size());
    for (const auto& entry : config.classifications.majorBosses) {
        result.push_back({
            .monstats = entry.monstats,
            .monstatsId = entry.expectedId,
        });
    }
    return result;
}

void CheckFraction(
    RuffnecKk::BKVCombat::DamageFraction actual,
    std::int32_t numerator,
    std::int32_t denominator) {
    CHECK(actual.numerator == numerator);
    CHECK(actual.denominator == denominator);
}

} // namespace

int main(int argc, char** argv) {
    using namespace RuffnecKk::BKVCombat;

    CHECK(argc == 2);
    if (argc != 2) return EXIT_FAILURE;

    const auto shippedPath = std::filesystem::path(argv[1]);
    const auto shippedJson = ReadJson(shippedPath);
    const auto shipped = ParseConfig(shippedJson);
    CHECK(shipped.schemaVersion == 2);
    CHECK(!shipped.enabled);
    CHECK(!shipped.diagnosticLogging);
    CHECK(!shipped.policies.criticalStrike);
    CHECK(!shipped.policies.deadlyStrike);
    CHECK(!shipped.policies.crushingBlow);
    CHECK(!shipped.policies.lifeSteal);
    CHECK(!shipped.policies.manaSteal);
    CHECK(!shipped.policies.openWounds);
    CHECK(shipped.stats.crushingBlowEfficiencyStatId == -1);
    CHECK(shipped.classifications.majorBosses.size() == RequiredMajorBossCount);

    const std::vector<std::int32_t> expectedIds{
        704, 705, 709, 706, 707, 708, 745, 746, 747, 333,
    };
    for (std::size_t index{}; index < expectedIds.size(); ++index) {
        CHECK(shipped.classifications.majorBosses[index].expectedId
            == expectedIds[index]);
    }

    const auto absent = LoadConfig({
        shippedPath.parent_path() / "BKVCombat-file-does-not-exist.json",
    });
    CHECK(!absent.found);
    CHECK(!absent.config.enabled);
    CHECK(absent.config.classifications.majorBosses.empty());

    const auto loaded = LoadConfig({
        shippedPath.parent_path() / "BKVCombat-file-does-not-exist.json",
        shippedPath,
    });
    CHECK(loaded.found);
    CHECK(loaded.source == shippedPath);
    CHECK(!loaded.config.enabled);

    const auto candidates = BuildConfigCandidates(
        "C:/D2R/mods/BKVince/d2rloader/config",
        "C:/D2R/mods/BKVince/d2rloader/config",
        "C:/D2R/d2rloader/config");
    CHECK(candidates.size() == 2);
    CHECK(candidates[0].filename() == "BKVCombat.json");
    CHECK(candidates[1].filename() == "BKVCombat.json");

    auto enabledJson = shippedJson;
    enabledJson["enabled"] = true;
    enabledJson["policies"]["criticalStrike"] = true;
    enabledJson["policies"]["crushingBlow"] = true;
    const auto enabled = ParseConfig(enabledJson);
    CHECK(IsPolicyEnabled(enabled, PolicyKind::CriticalStrike));
    CHECK(!IsPolicyEnabled(enabled, PolicyKind::DeadlyStrike));
    CHECK(IsPolicyEnabled(enabled, PolicyKind::CrushingBlow));
    CHECK(!IsPolicyEnabled(shipped, PolicyKind::CriticalStrike));

    CHECK(Throws([&] { ParseConfig(nlohmann::json::array()); }));
    CHECK(Throws([&] {
        auto value = shippedJson;
        value["unknown"] = false;
        ParseConfig(value);
    }));
    CHECK(Throws([&] {
        auto value = shippedJson;
        value.erase("enabled");
        ParseConfig(value);
    }));
    CHECK(Throws([&] {
        auto value = shippedJson;
        value["enabled"] = 1;
        ParseConfig(value);
    }));
    CHECK(Throws([&] {
        auto value = shippedJson;
        value["schemaVersion"] = 1;
        ParseConfig(value);
    }));
    CHECK(Throws([&] {
        auto value = shippedJson;
        value.erase("stats");
        ParseConfig(value);
    }));
    CHECK(Throws([&] {
        auto value = shippedJson;
        value["stats"]["crushingBlowEfficiencyStatId"] = -2;
        ParseConfig(value);
    }));
    CHECK(Throws([&] {
        auto value = shippedJson;
        value["stats"]["crushingBlowEfficiencyStatId"] = 65536;
        ParseConfig(value);
    }));
    CHECK(Throws([&] {
        auto value = shippedJson;
        value["stats"]["crushingBlowEfficiencyStatId"] = "393";
        ParseConfig(value);
    }));
    CHECK(Throws([&] {
        auto value = shippedJson;
        value["stats"]["unknown"] = 393;
        ParseConfig(value);
    }));
    {
        auto value = shippedJson;
        value["stats"]["crushingBlowEfficiencyStatId"] = 393;
        CHECK(ParseConfig(value).stats.crushingBlowEfficiencyStatId == 393);
    }
    CHECK(Throws([&] {
        auto value = shippedJson;
        value["policies"].erase("openWounds");
        ParseConfig(value);
    }));
    CHECK(Throws([&] {
        auto value = shippedJson;
        value["policies"]["lifeSteal"] = "yes";
        ParseConfig(value);
    }));
    CHECK(Throws([&] {
        auto value = shippedJson;
        value["classifications"]["majorBosses"].erase(0);
        ParseConfig(value);
    }));
    CHECK(Throws([&] {
        auto value = shippedJson;
        value["classifications"]["majorBosses"][1]["monstats"] =
            value["classifications"]["majorBosses"][0]["monstats"];
        ParseConfig(value);
    }));
    CHECK(Throws([&] {
        auto value = shippedJson;
        value["classifications"]["majorBosses"][1]["expectedId"] =
            value["classifications"]["majorBosses"][0]["expectedId"];
        ParseConfig(value);
    }));
    CHECK(Throws([&] {
        auto value = shippedJson;
        value["classifications"]["majorBosses"][0]["monstats"] = "Bad Key";
        ParseConfig(value);
    }));
    CHECK(Throws([&] {
        auto value = shippedJson;
        value["classifications"]["majorBosses"][0]["expectedId"] = -1;
        ParseConfig(value);
    }));

    const auto resolved = ResolveAll(shipped);
    const auto registry = ValidateMajorBossRegistry(shipped, resolved);
    CHECK(registry.valid);
    CHECK(registry.error.empty());
    CHECK(registry.entries.size() == RequiredMajorBossCount);

    RuntimeMonsterFacts major{
        .monstats = resolved.front().monstats,
        .monstatsId = resolved.front().monstatsId,
        .primeEvil = true,
        .heraldOrAscendant = true,
        .champion = true,
        .unique = true,
        .superUnique = true,
        .boss = true,
    };
    CHECK(ClassifyCrushingBlow(registry, major) == MonsterClass::MajorBoss);

    RuntimeMonsterFacts prime{
        .monstats = "act_boss_not_in_major_list",
        .monstatsId = 42,
        .primeEvil = true,
        .heraldOrAscendant = true,
    };
    CHECK(ClassifyCrushingBlow(registry, prime) == MonsterClass::PrimeEvil);

    RuntimeMonsterFacts herald{
        .monstats = "runtime_classified_monster",
        .monstatsId = 45,
        .heraldOrAscendant = true,
    };
    CHECK(ClassifyCrushingBlow(registry, herald) == MonsterClass::Elite);

    RuntimeMonsterFacts elite{
        .monstats = "elite_monster",
        .monstatsId = 43,
        .superUnique = true,
    };
    CHECK(ClassifyCrushingBlow(registry, elite) == MonsterClass::Elite);

    RuntimeMonsterFacts ordinary{
        .monstats = "ordinary_monster",
        .monstatsId = 44,
    };
    CHECK(ClassifyCrushingBlow(registry, ordinary) == MonsterClass::Ordinary);
    CHECK(ClassifyMonster(registry, ordinary) == MonsterClass::Ordinary);

    auto missingRows = resolved;
    missingRows.pop_back();
    const auto missingRegistry =
        ValidateMajorBossRegistry(enabled, missingRows);
    CHECK(!missingRegistry.valid);
    CHECK(missingRegistry.entries.empty());
    CHECK(!missingRegistry.error.empty());
    CHECK(!ClassifyCrushingBlow(missingRegistry, ordinary).has_value());

    auto mismatchedRows = resolved;
    ++mismatchedRows.front().monstatsId;
    const auto mismatchedRegistry =
        ValidateMajorBossRegistry(enabled, mismatchedRows);
    CHECK(!mismatchedRegistry.valid);
    CHECK(mismatchedRegistry.entries.empty());

    auto duplicateRows = resolved;
    duplicateRows.push_back(resolved.front());
    const auto duplicateRegistry =
        ValidateMajorBossRegistry(enabled, duplicateRows);
    CHECK(!duplicateRegistry.valid);
    CHECK(duplicateRegistry.entries.empty());

    CheckFraction(
        CrushingBlowFraction(MonsterClass::Ordinary, false), 1, 6);
    CheckFraction(
        CrushingBlowFraction(MonsterClass::Ordinary, true), 1, 9);
    CheckFraction(
        CrushingBlowFraction(MonsterClass::Elite, false), 1, 8);
    CheckFraction(
        CrushingBlowFraction(MonsterClass::Elite, true), 1, 12);
    CheckFraction(
        CrushingBlowFraction(MonsterClass::PrimeEvil, false), 1, 16);
    CheckFraction(
        CrushingBlowFraction(MonsterClass::PrimeEvil, true), 1, 24);
    CheckFraction(
        CrushingBlowFraction(MonsterClass::MajorBoss, false), 1, 20);
    CheckFraction(
        CrushingBlowFraction(MonsterClass::MajorBoss, true), 1, 30);

    const auto rangedScaled = ComputeCrushingBlowDamage(
        100000, {1, 9}, 50, 0, 0);
    CHECK(rangedScaled && *rangedScaled == 7407);
    const auto rangedCbe100 = ComputeCrushingBlowDamage(
        100000, {1, 9}, 50, 100, 0);
    CHECK(rangedCbe100 && *rangedCbe100 == 14814);
    const auto negativeCbe = ComputeCrushingBlowDamage(
        100000, {1, 9}, 50, -100, 0);
    CHECK(negativeCbe == rangedScaled);
    const auto resisted = ComputeCrushingBlowDamage(
        100000, {1, 9}, 50, 0, 50);
    CHECK(resisted && *resisted == 3704);
    const auto cbe25 = ComputeCrushingBlowDamage(
        100000, {1, 6}, 0, 25, 0);
    CHECK(cbe25 && *cbe25 == 20833);
    CHECK(!ComputeCrushingBlowDamage(-1, {1, 6}, 0, 0, 0));
    CHECK(!ComputeCrushingBlowDamage(100000, {1, 0}, 0, 0, 0));
    CHECK(!ComputeCrushingBlowDamage(100000, {1, 6}, -1, 0, 0));

    const auto accepted = BuildActivationPlan(enabled, registry);
    CHECK(accepted.accepted);
    CHECK(accepted.active.criticalStrike);
    CHECK(accepted.active.crushingBlow);
    const auto rejected = BuildActivationPlan(enabled, missingRegistry);
    CHECK(!rejected.accepted);
    CHECK(rejected.active == PolicyToggles{});
    CHECK(!rejected.error.empty());

    auto noCrushingBlow = enabled;
    noCrushingBlow.policies.crushingBlow = false;
    noCrushingBlow.policies.openWounds = true;
    const auto independent =
        BuildActivationPlan(noCrushingBlow, missingRegistry);
    CHECK(independent.accepted);
    CHECK(independent.active.openWounds);
    CHECK(!independent.active.crushingBlow);

    const auto disabledPlan = BuildActivationPlan(shipped, missingRegistry);
    CHECK(disabledPlan.accepted);
    CHECK(disabledPlan.active == PolicyToggles{});

    return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
