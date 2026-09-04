#include "scripted_ai_fingerprint.hpp"

#include <array>
#include <exception>
#include <string>

namespace ruffneckk::scripted_ai {
namespace {

constexpr std::array<NativeWindow, NativeWindowCount> Windows{{
    {
        "UNITS_GetClassId",
        0x349860U,
        46U,
        "48 83 EC 28 48 85 C9 75 1D 88 4C 24 30 48 8D 4C 24 30 E8 49 CB FF FF 84 C0 74 01 CC B8 FF FF FF FF 48 83 C4 28 C3 8B 41 04 48 83 C4 28 C3",
        "8FC504A2AD0015A9E8DE617BB65FACF76F3AC45FA897666A3F84693A61C623EE",
        false,
    },
    {
        "UNITS_GetUnitId",
        0x34A330U,
        32U,
        "48 83 EC 28 48 85 C9 75 1D 88 4C 24 30 48 8D 4C 24 30 E8 39 CA FF FF 84 C0 74 01 CC B8 FF FF FF",
        "6333518D3F7FC1CF0642DD3D22BD560599BFD05245702966EBCE085841C26AA6",
        false,
    },
    {
        "UNITS_GetUnitType",
        0x34B9D0U,
        45U,
        "48 83 EC 28 48 85 C9 75 1D 88 4C 24 30 48 8D 4C 24 30 E8 39 9E FF FF 84 C0 74 01 CC B8 06 00 00 00 48 83 C4 28 C3 8B 01 48 83 C4 28 C3",
        "5776823C057470BA2481E039F32EFC928E24375710FAB480D603847DE9544847",
        false,
    },
    {
        "SUNIT_IsDead",
        0x34C2C0U,
        32U,
        "48 83 EC 28 48 85 C9 75 1D 88 4C 24 30 48 8D 4C 24 30 E8 59 94 FF FF 84 C0 74 4D CC B8 01 00 00",
        "FE952CAB2B93812D7A481697972EB8B2D2E4546D66D31118BE31530B41242FCF",
        false,
    },
    {
        "SUNIT_GetServerUnit",
        0x48FE80U,
        50U,
        "48 89 5C 24 08 48 89 74 24 18 57 48 83 EC 20 41 8B D8 8B F2 48 8B F9 48 85 C9 75 13 88 4C 24 38 48 8D 4C 24 38 E8 46 D1 FF FF 84 C0 74 01 CC 83 FE 05",
        "BC08758BF45D82254667B1F0E975EC52C2945BF604ECBF0D8B389A93A1887C34",
        false,
    },
    {
        "AIUTIL_GetDistanceToUnit",
        0x596720U,
        62U,
        "48 89 5C 24 10 48 89 6C 24 18 48 89 74 24 20 57 41 54 41 55 41 56 41 57 48 83 EC 20 48 8B DA 48 8B F9 48 85 C9 75 13 88 4C 24 50 48 8D 4C 24 50 E8 DB F0 AE FF 84 C0 74 01 CC 8B 0F 33 F6",
        "B2EA417FFDE5367A247B296E070FD803FFBB1A04AF48B6F6C1F45444C6A90618",
        false,
    },
    {
        "AITHINK_MinimalTickContextWitness",
        0x4A2ADAU,
        117U,
        "48 8B C3 4D 8B C6 48 89 45 AF 49 8B D7 48 8D 4D 0F E8 50 D4 FE FF 41 B8 E4 03 00 00 48 8D 15 03 BE 87 01 49 8B CE F2 0F 10 00 F2 0F 11 45 E7 8B 40 08 89 45 EF E8 4C 6D EA FF 41 0F B6 8F 06 01 00 00 8B D0 E8 BD 4B BF FF 41 B8 E5 03 00 00 48 89 45 D7 48 8D 15 CC BD 87 01 49 8B CE E8 24 6D EA FF 41 0F B6 8F 06 01 00 00 8B D0 E8 65 4A BF FF 48 89 45 DF",
        "85EE58C57F0381F78B286B2B05B9636883A408CB508EBFCF51BA07F4600F5FA9",
        false,
    },
    {
        "AITHINK_DispatchCategorySwitch",
        0x4A2BD6U,
        28U,
        "48 63 10 83 FA 06 77 53 4C 8D 05 1B D4 B5 FF 41 8B 8C 90 08 2F 4A 00 49 03 C8 FF E1",
        "8C3F59D347304656EBB57A3234B571AC762155BB78D2D08AA66B88B2E36A1846",
        false,
    },
    {
        "AITHINK_Category2TargetSelectionWitness",
        0x4A2C7AU,
        38U,
        "0F B6 45 E7 4C 8D 4D CF 4C 8B 45 AF 49 8B D6 88 44 24 28 49 8B CF 48 8D 45 D3 48 89 44 24 20 E8 B2 2A 0F 00 EB 4D",
        "7BCD196C050D994793561900D8FCAC4DC11865C78F34875F24161B6AF15D85D7",
        false,
    },
    {
        "AITHINK_Category2CallbackHandoffWitness",
        0x4A2CEDU,
        19U,
        "48 89 45 BF 4C 8D 45 AF 49 8B D6 49 8B CF E8 F0 0A 00 00",
        "22A2F7AE079A43FF30AEDAE337296993695992682764907AA90CE48D1458141B",
        false,
    },
    {
        "AITHINK_GetAiTableRecord",
        ResolverHookRva,
        17U,
        "48 89 74 24 20 57 48 83 EC 30 48 89 5C 24 40 33 FF",
        "9106A88437D9E7951FD53955E95EEEC52AB9AA32460D583A15C4F6B46ADB78C9",
        true,
    },
    {
        "AITHINK_UnitMonStatsRecordWitness",
        0x4A3720U,
        17U,
        "48 8B 73 10 48 85 F6 74 05 48 8B 36 EB 03 48 8B F7",
        "544B34BB7798AA3B84B20F566A3FAE565621EEB5C1061F6393C950C0447A8980",
        false,
    },
    {
        "SKILLS_MonsterSkillModeLookupWitness",
        0x33DC79U,
        82U,
        "0F B6 C8 8B D5 E8 5D 9A D5 FF 45 33 C0 4C 8B C8 41 8B C8 48 8D 90 C4 01 00 00 66 39 1A 74 27 41 FF C0 48 FF C1 48 83 C2 02 48 83 F9 08 7C EB 33 C0 48 8B 5C 24 38 48 8B 6C 24 40 48 8B 74 24 48 48 83 C4 20 5F C3 41 8B C0 41 0F BF 84 41 DC 01 00 00",
        "2E7C902457E62260B9CB88486C045B10F363181A1F459E9737C129043C7257B1",
        false,
    },
    {
        "DATATBLS_MonStatsFlagsByteWitness",
        0x44CF23U,
        63U,
        "E8 38 C9 EF FF 41 0F B6 8F 06 01 00 00 8B D0 E8 A9 A7 C4 FF 48 85 C0 75 1C 48 8D 4C 24 40 88 44 24 40 E8 D6 4E C8 FF 84 C0 0F 84 E8 05 00 00 CC E9 E2 05 00 00 0F B6 40 3D 23 05 8A C7 94 01",
        "F7129CBDB060619C5381B09BAF5993E5B6B09F1CA9B571D09EDBC6FD23DD7F25",
        false,
    },
    {
        "AITHINK_SpecialAiLookupWitness",
        0x4A3767U,
        32U,
        "85 FF 74 1C 48 63 C7 48 8D 0D 7B 4A EF 01 48 C1 E0 05 48 03 C1 48 8B 74 24 58 48 83 C4 30 5F C3",
        "0916D113567A61197A92DA63BFADA77EE02FC58F26613245788F4619F6B38193",
        false,
    },
    {
        "AITHINK_NormalAiLookupWitness",
        0x4A3787U,
        63U,
        "48 0F BF 46 52 66 85 C0 78 23 B9 9B 00 00 00 66 3B C1 73 19 48 C1 E0 05 48 8D 0D EA 36 EF 01 48 03 C1 48 8B 74 24 58 48 83 C4 30 5F C3 48 8B 74 24 58 48 8D 05 D0 36 EF 01 48 83 C4 30 5F C3",
        "77EB51353B60204B4926B3D1CE4A00184816E0998040BAA4BCC5A80AB8C44BFD",
        false,
    },
    {
        "AIUTIL_SelectTargetForAiThink",
        0x595750U,
        32U,
        "40 55 53 56 57 41 57 48 8D AC 24 40 FF FF FF 48 81 EC C0 01 00 00 48 8B 05 5B 5B 43 02 48 33 C4",
        "92D0A9C2BA975C74A5543EEE34F5A16823F6089533DB8F02AC456C0BA642B146",
        false,
    },
    {
        "AITACTICS_IdleInNeutralMode",
        0x4A6D10U,
        33U,
        "48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 70 45 85 C0 48 8B F9 BE 01 00 00 00 48 8B CA 41 0F 45 F0",
        "337BB3ECBCB3EA039EE04D6DA1067470A3B3EC34F6E09AB6572710DEDE0714DE",
        false,
    },
    {
        "AITACTICS_IdleRescheduleWitness",
        0x4A6D71U,
        16U,
        "45 8D 41 02 E8 16 4B FE FF 44 8B 8F 70 01 00 00",
        "25C399065D50609C594C9BE75D08E43C655D1950FF54E5FCE6F70F380F33D071",
        false,
    },
    {
        "AITACTICS_ChangeModeAndTargetUnit",
        0x4A78E0U,
        32U,
        "48 89 5C 24 08 57 48 83 EC 50 41 8B C0 4C 8B D2 48 8B F9 4C 8D 44 24 20 8B D0 49 8B CA 49 8B D9",
        "6F1ADF2865408715C4CDCCC98FAE1252F026F8832C86283BDAC337F13DF6480E",
        false,
    },
    {
        "AITACTICS_AttackTerminalWitness",
        0x4A7900U,
        40U,
        "E8 DB F8 F9 FF 41 B8 01 00 00 00 48 89 5C 24 30 48 8D 54 24 20 48 8B CF E8 A3 FC F9 FF 48 8B 5C 24 60 48 83 C4 50 5F C3",
        "50B164D10BDB42ECFD9B52AF2F3B12E70A3111EA41C7FCB9CA99F04367025C18",
        false,
    },
    {
        "AITACTICS_UseSkill",
        0x4A7BC0U,
        28U,
        "48 89 6C 24 10 48 89 74 24 20 57 48 83 EC 50 41 8B E9 48 8B FA 48 8B F1 41 80 F8 10",
        "DA23B45877C895C9DF8E33C8AAA780EB72D00F4BC44BEF6AD30754EB1118AC22",
        false,
    },
    {
        "AITACTICS_CastTerminalWitness",
        0x4A7C9FU,
        70U,
        "E8 1C F9 F9 FF 48 8B 5C 24 60 85 C0 74 15 B8 01 00 00 00 48 8B 6C 24 68 48 8B 74 24 78 48 83 C4 50 5F C3 41 B8 0A 00 00 00 48 8B D7 48 8B CE E8 3D F0 FF FF 48 8B 6C 24 68 33 C0 48 8B 74 24 78 48 83 C4 50 5F C3",
        "380F30880DF00CFA248F7A1FE998FD827D177B588175AF76DECD10E21E0306FE",
        false,
    },
    {
        "D2GAME_AICORE_Escape",
        0x4A7DF0U,
        27U,
        "48 89 5C 24 20 57 41 56 41 57 48 83 EC 40 45 0F B6 F1 49 8B D8 48 8B FA 4C 8B F9",
        "A407042B3A41FBD0AE934E34B8605B8AAEDB4D372FB2D43887CAB3376FE3F5DB",
        false,
    },
    {
        "AITACTICS_RetreatTerminalWitness",
        0x4A7F1DU,
        35U,
        "66 89 6C 24 30 44 8B CE C7 44 24 28 00 00 00 00 44 8B C0 48 8B D7 44 88 74 24 20 49 8B CF E8 00 F6 FF FF",
        "0F7E7FCF35841686CD6867D96C0E8A2B09F98B426E678BFE0F7EFA89B4D91330",
        false,
    },
    {
        "AITACTICS_WalkCloseToUnit",
        0x4A8320U,
        34U,
        "48 89 5C 24 08 48 89 6C 24 18 48 89 74 24 20 57 41 56 41 57 48 83 EC 40 41 0F B6 E8 48 8B DA 4C 8B F1",
        "8EE0E67E8982E3C4669B4C42D0C7CA5CB54773B9DAA5606490A3705C402D0DB4",
        false,
    },
    {
        "AITACTICS_WanderTerminalWitness",
        0x4A84A0U,
        64U,
        "F7 D8 03 CF 41 B9 02 00 00 00 41 80 E0 01 89 4C 24 28 49 8B CE 41 0F 44 C2 45 33 C0 03 C6 89 44 24 20 E8 49 05 00 00 48 8B 5C 24 60 48 8B 6C 24 70 48 8B 74 24 78 48 83 C4 40 41 5F 41 5E 5F C3",
        "C46471854EE3719BEFEE2EE9F608E3AD8686465801D1CF21BD39F8784AA113B0",
        false,
    },
    {
        "AITACTICS_WalkToTargetUnitWithFlags",
        0x4A8740U,
        39U,
        "48 83 EC 48 66 44 89 4C 24 38 33 C0 C6 44 24 30 01 89 44 24 28 89 44 24 20 44 8D 48 02 E8 AE 02 00 00 48 83 C4 48 C3",
        "2B27605BD13ECD4A6E6548474B20E9BA91EC9545AB2B6FA033A9C8DB4280DE54",
        false,
    },
    {
        "AITACTICS_MoveToTarget",
        0x4A8A10U,
        62U,
        "48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 48 83 EC 50 41 8B F1 49 8B E8 48 8B DA 48 8B F9 48 85 C9 74 05 48 85 D2 75 14 48 8D 4C 24 78 C6 44 24 78 00 E8 57 DB FF FF 84 C0 74 01 CC",
        "489DDF25C0A4BEF3A5489BD28F4CE85C51C60FEE2382E0F90D7D30C67768FCB8",
        false,
    },
}};

[[nodiscard]] auto HexNibble(char character, std::uint8_t& value) noexcept
        -> bool {
    if (character >= '0' && character <= '9') {
        value = static_cast<std::uint8_t>(character - '0');
        return true;
    }
    if (character >= 'A' && character <= 'F') {
        value = static_cast<std::uint8_t>(character - 'A' + 10);
        return true;
    }
    if (character >= 'a' && character <= 'f') {
        value = static_cast<std::uint8_t>(character - 'a' + 10);
        return true;
    }
    return false;
}

} // namespace

auto NativeFingerprint() noexcept
        -> const std::array<NativeWindow, NativeWindowCount>& {
    return Windows;
}

auto DecodeNativeWindow(
        const NativeWindow& window,
        std::vector<std::uint8_t>& output,
        std::string& error) -> bool {
    try {
        output.clear();
        output.reserve(window.size);
        std::uint8_t high{};
        bool haveHigh{};
        for (const auto character : window.expectedHex) {
            if (character == ' ' || character == '\t'
                    || character == '\r' || character == '\n') {
                continue;
            }
            std::uint8_t nibble{};
            if (!HexNibble(character, nibble)) {
                error = "native fingerprint contains a non-hex character";
                return false;
            }
            if (!haveHigh) {
                high = nibble;
                haveHigh = true;
            } else {
                output.push_back(static_cast<std::uint8_t>(
                    (high << 4U) | nibble));
                haveHigh = false;
            }
        }
        if (haveHigh || output.size() != window.size) {
            error = "native fingerprint size does not match its declaration";
            output.clear();
            return false;
        }
        error.clear();
        return true;
    } catch (const std::exception& exception) {
        error = exception.what();
        output.clear();
        return false;
    } catch (...) {
        error = "unknown native fingerprint decode failure";
        output.clear();
        return false;
    }
}

auto ValidateNativeFingerprint(
        NativeCheckCallback callback,
        void* userData) -> FingerprintValidationResult {
    FingerprintValidationResult result{};
    if (callback == nullptr) {
        result.error = "native fingerprint callback is unavailable";
        return result;
    }
    for (std::size_t index{}; index < Windows.size(); ++index) {
        std::vector<std::uint8_t> expected;
        if (!DecodeNativeWindow(Windows[index], expected, result.error)) {
            result.failedIndex = index;
            return result;
        }
        if (!callback(userData, Windows[index].rva, expected)) {
            result.failedIndex = index;
            result.error = "native fingerprint mismatch at "
                + std::string(Windows[index].name);
            return result;
        }
    }
    result.accepted = true;
    result.error.clear();
    return result;
}

} // namespace ruffneckk::scripted_ai
