#include "melee_splash_config.hpp"
#include "melee_splash_bkvcombat_interop.hpp"
#include "melee_splash_gameplay.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>

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

bool ParseFileEnabled(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input.is_open()) {
        throw std::runtime_error("cannot open shipped configuration " + path.string());
    }
    return RuffnecKk::MeleeSplash::ParseConfig(
        nlohmann::json::parse(input)).enabled;
}

auto __fastcall ResolveCriticalDeadlyFixture(
        void*, void*, void*, void*, std::int32_t, std::uint32_t) noexcept
        -> RuffnecKk::MeleeSplash::BKVCombatInterop::CriticalDeadlyOutcome {
    return RuffnecKk::MeleeSplash::BKVCombatInterop::
        CriticalDeadlyOutcome::None;
}

std::uint64_t __cdecl ActiveCapabilitiesFixture() noexcept {
    return RuffnecKk::MeleeSplash::BKVCombatInterop::
        CriticalDeadlyResolverCapability;
}

std::uint64_t __cdecl InactiveCapabilitiesFixture() noexcept {
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    using namespace RuffnecKk::MeleeSplash;

    const auto defaults = ParseConfig(nlohmann::json::object());
    CHECK(!defaults.enabled);
    CHECK(defaults.activationMode == ActivationMode::AllEligibleMelee);
    CHECK(defaults.allowNormalAttack);
    CHECK(defaults.includedSkillIds.empty());
    CHECK(defaults.excludedSkillIds.empty());
    CHECK(!defaults.requireGateStat);
    CHECK(defaults.gateStatId == -1);
    CHECK(defaults.increasedRadiusStatId == -1);
    CHECK(defaults.radiusPercentPerTile == 20);
    CHECK(defaults.splashDamagePercentStatId == -1);
    CHECK(defaults.baseSplashDamagePercent == 100);
    CHECK(defaults.baseRadiusNormalWeapon == 4);
    CHECK(defaults.baseRadiusExceptionalEliteWeapon == 5);
    CHECK(defaults.maximumRadiusTiles == 0);
    CHECK(!defaults.diagnosticLogging);
    CHECK(defaults.skillOverrides.empty());
    CHECK(!defaults.legacyEvent20Suppression.enabled);
    CHECK(defaults.legacyEvent20Suppression.statId == -1);
    CHECK(defaults.legacyEvent20Suppression.layer == -1);
    CHECK(defaults.legacyEvent20Suppression.playerAttackersOnly);
    CHECK(ActivationModeName(defaults.activationMode) == "allEligibleMelee");

    const auto configured = ParseConfig(nlohmann::json::parse(R"json(
    {
      "enabled": true,
      "activationMode": "whitelist",
      "allowNormalAttack": false,
      "includedSkillIds": [10, 10, 20],
      "excludedSkillIds": [20],
      "requireGateStat": true,
      "gateStatId": 384,
      "increasedRadiusStatId": 391,
      "radiusPercentPerTile": 20,
      "splashDamagePercentStatId": 392,
      "baseSplashDamagePercent": 100,
      "baseRadiusNormalWeapon": 4,
      "baseRadiusExceptionalEliteWeapon": 5,
      "maximumRadiusTiles": 8,
      "diagnosticLogging": true,
      "skillOverrides": {
        "10": {
          "enabled": true,
          "baseRadiusTiles": 7,
          "baseSplashDamagePercent": 125,
          "requireGateStat": false
        },
        "30": {"enabled": false}
      },
      "legacyEvent20Suppression": {
        "enabled": true,
        "statId": 384,
        "layer": 430,
        "playerAttackersOnly": true
      }
    }
    )json"));
    CHECK(configured.enabled);
    CHECK(configured.activationMode == ActivationMode::Whitelist);
    CHECK(!configured.allowNormalAttack);
    CHECK(configured.includedSkillIds.size() == 2);
    CHECK(configured.requireGateStat);
    CHECK(configured.gateStatId == 384);
    CHECK(configured.increasedRadiusStatId == 391);
    CHECK(configured.splashDamagePercentStatId == 392);
    CHECK(configured.maximumRadiusTiles == 8);
    CHECK(configured.diagnosticLogging);
    CHECK(configured.skillOverrides.size() == 2);
    CHECK(configured.skillOverrides.at(10).baseRadiusTiles == 7);
    CHECK(configured.skillOverrides.at(10).baseSplashDamagePercent == 125);
    CHECK(configured.skillOverrides.at(10).requireGateStat == false);
    CHECK(configured.legacyEvent20Suppression.enabled);
    CHECK(configured.legacyEvent20Suppression.statId == 384);
    CHECK(configured.legacyEvent20Suppression.layer == 430);

    CHECK(IsSkillEnabled(configured, 10));
    CHECK(!IsSkillEnabled(configured, 20));
    CHECK(!IsSkillEnabled(configured, 30));
    CHECK(!IsSkillEnabled(configured, NormalAttackSkillId));
    CHECK(!RequiresGateStat(configured, 10));
    CHECK(RequiresGateStat(configured, 20));
    CHECK(ResolveRadiusTiles(configured, 10, false, 0) == 7);
    CHECK(ResolveRadiusTiles(configured, 10, false, 19) == 7);
    CHECK(ResolveRadiusTiles(configured, 10, false, 20) == 8);
    CHECK(ResolveRadiusTiles(configured, 10, false, 40) == 8);
    CHECK(ResolveSplashDamagePercent(configured, 10, 50) == 175);

    Config policy = defaults;
    policy.enabled = true;
    CHECK(IsSkillEnabled(policy, NormalAttackSkillId));
    CHECK(IsSkillEnabled(policy, 10));
    policy.activationMode = ActivationMode::Whitelist;
    CHECK(!IsSkillEnabled(policy, NormalAttackSkillId));
    CHECK(!IsSkillEnabled(policy, 10));
    policy.includedSkillIds = {0, 10};
    CHECK(IsSkillEnabled(policy, NormalAttackSkillId));
    CHECK(IsSkillEnabled(policy, 10));
    policy.excludedSkillIds = {0, 10};
    CHECK(!IsSkillEnabled(policy, NormalAttackSkillId));
    CHECK(!IsSkillEnabled(policy, 10));
    policy.activationMode = ActivationMode::Blacklist;
    CHECK(!IsSkillEnabled(policy, 10));
    CHECK(IsSkillEnabled(policy, 11));

    Config arithmetic = defaults;
    arithmetic.maximumRadiusTiles = 0;
    CHECK(ResolveRadiusTiles(arithmetic, 10, false, -20) == 4);
    CHECK(ResolveRadiusTiles(arithmetic, 10, false, 19) == 4);
    CHECK(ResolveRadiusTiles(arithmetic, 10, false, 20) == 5);
    CHECK(ResolveRadiusTiles(arithmetic, 10, false, 40) == 6);
    CHECK(ResolveRadiusTiles(arithmetic, 10, true, 40) == 7);
    CHECK(ResolveSplashDamagePercent(arithmetic, 10, 0) == 100);
    CHECK(ResolveSplashDamagePercent(arithmetic, 10, 50) == 150);
    CHECK(ResolveSplashDamagePercent(arithmetic, 10, -150) == 0);

    const LegacyEvent20Suppression suppression{
        .enabled = true,
        .statId = 384,
        .layer = 430,
        .playerAttackersOnly = true,
    };
    const auto matchingToken =
        (static_cast<std::uint32_t>(384) << 16) | 430U;
    CHECK(MatchesLegacyEvent20Suppression(suppression, matchingToken, true));
    CHECK(!MatchesLegacyEvent20Suppression(suppression, matchingToken, false));
    CHECK(!MatchesLegacyEvent20Suppression(
        suppression,
        (static_cast<std::uint32_t>(383) << 16) | 430U,
        true));
    CHECK(!MatchesLegacyEvent20Suppression(
        suppression,
        (static_cast<std::uint32_t>(384) << 16) | 431U,
        true));
    Config suppressionConfig = defaults;
    suppressionConfig.legacyEvent20Suppression = suppression;
    CHECK(!MatchesLegacyEvent20Suppression(
        suppressionConfig, matchingToken, true));
    suppressionConfig.enabled = true;
    CHECK(MatchesLegacyEvent20Suppression(
        suppressionConfig, matchingToken, true));

    std::vector<UnitIdentity> targets;
    const UnitIdentity attacker{.type = 0, .guid = 1};
    const UnitIdentity primary{.type = 1, .guid = 2};
    const UnitIdentity secondary{.type = 1, .guid = 3};
    CHECK(!AppendUniqueSecondaryTarget(targets, attacker, attacker, primary));
    CHECK(!AppendUniqueSecondaryTarget(targets, primary, attacker, primary));
    CHECK(AppendUniqueSecondaryTarget(targets, secondary, attacker, primary));
    CHECK(!AppendUniqueSecondaryTarget(targets, secondary, attacker, primary));
    CHECK(targets.size() == 1);

    NativeSeedPair seed{.low = 1, .high = 2};
    const auto firstRoll = RollNativePercent(seed);
    CHECK(firstRoll == seed.low % 100U);
    CHECK(seed.low == 0x6AC690C7U);
    CHECK(seed.high == 0U);

    NativeSeedPair guaranteedSeed{.low = 1, .high = 2};
    CHECK(RollNative92777CriticalDeadly(
              guaranteedSeed,
              {.weaponMastery = 100,
               .passiveCritical = 100,
               .deadlyStrike = 100})
        == Native92777CriticalOutcome::WeaponMastery);
    NativeSeedPair passiveSeed{.low = 1, .high = 2};
    CHECK(RollNative92777CriticalDeadly(
              passiveSeed,
              {.weaponMastery = 0,
               .passiveCritical = 100,
               .deadlyStrike = 100})
        == Native92777CriticalOutcome::PassiveCritical);
    NativeSeedPair nonMasteryModeSeed{.low = 1, .high = 2};
    CHECK(RollNative92777CriticalDeadly(
              nonMasteryModeSeed,
              {.weaponMastery = 100,
               .passiveCritical = 100,
               .deadlyStrike = 100},
              false)
        == Native92777CriticalOutcome::PassiveCritical);
    NativeSeedPair deadlySeed{.low = 1, .high = 2};
    CHECK(RollNative92777CriticalDeadly(
              deadlySeed,
              {.weaponMastery = 0,
               .passiveCritical = 0,
               .deadlyStrike = 100})
        == Native92777CriticalOutcome::DeadlyStrike);

    NativeSeedPair noChanceSeed{.low = 1, .high = 2};
    CHECK(RollNative92777CriticalDeadly(
              noChanceSeed,
              {.weaponMastery = 0,
               .passiveCritical = 0,
               .deadlyStrike = 0})
        == Native92777CriticalOutcome::None);
    CHECK(noChanceSeed.low == 1U);
    CHECK(noChanceSeed.high == 2U);

    NativeSeedPair threeRollSeed{.low = 1, .high = 2};
    CHECK(RollNative92777CriticalDeadly(
              threeRollSeed,
              {.weaponMastery = 87,
               .passiveCritical = 83,
               .deadlyStrike = 100})
        == Native92777CriticalOutcome::DeadlyStrike);
    CHECK(threeRollSeed.low == 0x080651D7U);
    CHECK(threeRollSeed.high == 0x1916F35CU);

    CHECK(ApplyNative92777CriticalMultiplier(
              100, Native92777CriticalOutcome::DeadlyStrike)
        == 200);
    CHECK(static_cast<std::uint32_t>(ApplyNative92777CriticalMultiplier(
              std::numeric_limits<std::int32_t>::max(),
              Native92777CriticalOutcome::PassiveCritical))
        == 0xFFFFFFFEU);
    CHECK(ScaleFixedDamage(101, 50) == 50);
    CHECK(ScaleFixedDamage(101, 0) == 0);
    CHECK(ClampNativeRadius(-1) == 0);
    CHECK(ClampNativeRadius(MaximumSafeNativeRadius + 1)
        == MaximumSafeNativeRadius);

    const auto candidates = BuildConfigCandidates(
        "C:/D2R/mods/Test/d2rloader/config",
        "C:/D2R/mods/Test/d2rloader/config",
        "C:/D2R/d2rloader/config");
    CHECK(candidates.size() == 2);
    CHECK(candidates[0] == std::filesystem::path(
        "C:/D2R/mods/Test/d2rloader/config/MeleeSplash.json"));
    CHECK(candidates[1] == std::filesystem::path(
        "C:/D2R/d2rloader/config/MeleeSplash.json"));
    const auto absent = LoadConfig({});
    CHECK(!absent.found);
    CHECK(!absent.config.enabled);
    CHECK(absent.source.empty());

    using namespace BKVCombatInterop;
    const ApiV1 compatibleApi{
        .size = sizeof(ApiV1),
        .version = ApiVersion,
        .compiledCapabilities = CriticalDeadlyResolverCapability,
        .resolveCriticalDeadly = ResolveCriticalDeadlyFixture,
        .getActiveCapabilities = ActiveCapabilitiesFixture,
    };
    CHECK(IsCompatibleApiV1(&compatibleApi));
    CHECK(HasActiveCriticalDeadlyResolver(&compatibleApi));
    auto incompatibleApi = compatibleApi;
    incompatibleApi.size = static_cast<std::uint32_t>(sizeof(ApiV1) - 1);
    CHECK(!IsCompatibleApiV1(&incompatibleApi));
    incompatibleApi = compatibleApi;
    incompatibleApi.version = ApiVersion + 1;
    CHECK(!IsCompatibleApiV1(&incompatibleApi));
    incompatibleApi = compatibleApi;
    incompatibleApi.compiledCapabilities = 0;
    CHECK(!IsCompatibleApiV1(&incompatibleApi));
    incompatibleApi = compatibleApi;
    incompatibleApi.resolveCriticalDeadly = nullptr;
    CHECK(!IsCompatibleApiV1(&incompatibleApi));
    incompatibleApi = compatibleApi;
    incompatibleApi.getActiveCapabilities = InactiveCapabilitiesFixture;
    CHECK(IsCompatibleApiV1(&incompatibleApi));
    CHECK(!HasActiveCriticalDeadlyResolver(&incompatibleApi));

    CHECK(Throws([] { ParseConfig(nlohmann::json::array()); }));
    CHECK(Throws([] { ParseConfig({{"unknown", true}}); }));
    CHECK(Throws([] { ParseConfig({{"enabled", 1}}); }));
    CHECK(Throws([] { ParseConfig({{"activationMode", "allowEverything"}}); }));
    CHECK(Throws([] { ParseConfig({{"includedSkillIds", "10"}}); }));
    CHECK(Throws([] { ParseConfig({{"includedSkillIds", {-1}}}); }));
    CHECK(Throws([] { ParseConfig({{"gateStatId", 65536}}); }));
    CHECK(Throws([] { ParseConfig({{"radiusPercentPerTile", 0}}); }));
    CHECK(Throws([] { ParseConfig({{"baseSplashDamagePercent", -1}}); }));
    CHECK(Throws([] { ParseConfig({{"requireGateStat", true}}); }));
    CHECK(Throws([] {
        ParseConfig({
            {"skillOverrides", {{"10", {{"requireGateStat", true}}}}},
        });
    }));
    CHECK(Throws([] {
        ParseConfig({{"skillOverrides", {{"01", nlohmann::json::object()}}}});
    }));
    CHECK(Throws([] {
        ParseConfig({
            {"skillOverrides", {{"10", {{"unknown", true}}}}},
        });
    }));
    CHECK(Throws([] {
        ParseConfig({{"legacyEvent20Suppression", {{"enabled", true}}}});
    }));
    CHECK(Throws([] {
        ParseConfig({
            {"legacyEvent20Suppression",
             {{"enabled", true}, {"statId", 384}, {"layer", -1}}},
        });
    }));
    CHECK(Throws([] {
        ParseConfig({
            {"legacyEvent20Suppression", {{"unknown", true}}},
        });
    }));

    if (argc >= 2) CHECK(!ParseFileEnabled(argv[1]));
    if (argc >= 3) {
        try {
            CHECK(ParseFileEnabled(argv[2]));
            const auto firstExistingWins = LoadConfig({argv[1], argv[2]});
            CHECK(firstExistingWins.found);
            CHECK(!firstExistingWins.config.enabled);
            CHECK(firstExistingWins.source == std::filesystem::path(argv[1]));

            const auto missing = std::filesystem::path(argv[1]).parent_path()
                / "does-not-exist.json";
            const auto fallsThroughMissing = LoadConfig({missing, argv[2]});
            CHECK(fallsThroughMissing.found);
            CHECK(fallsThroughMissing.config.enabled);
            CHECK(fallsThroughMissing.source == std::filesystem::path(argv[2]));
        } catch (const std::exception& exception) {
            std::cerr << "FAIL shipped configuration load: "
                      << exception.what() << '\n';
            ++Failures;
        }
    }

    return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
