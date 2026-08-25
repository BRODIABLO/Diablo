#include "player_sequence_policy.hpp"

#include <cassert>
#include <cstddef>
#include <fstream>
#include <iterator>
#include <string>

using namespace ruffneckk::player_sequence_tables;

namespace {

std::string ReadFile(const char* path) {
    std::ifstream input(path, std::ios::binary);
    assert(input.is_open());
    return {
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>()};
}

void ReplaceOnce(
        std::string& text,
        const std::string& needle,
        const std::string& replacement) {
    const auto offset = text.find(needle);
    assert(offset != std::string::npos);
    text.replace(offset, needle.size(), replacement);
}

void AssertCrLfOnly(const std::string& text) {
    assert(text.ends_with("\r\n"));
    for (std::size_t index = 0; index < text.size(); ++index) {
        if (text[index] == '\n') {
            assert(index > 0 && text[index - 1] == '\r');
        }
    }
}

} // namespace

int main() {
    const auto configText = ReadFile(PLAYER_SEQUENCE_CONFIG_FILE);
    const auto routeText = ReadFile(PLAYER_SEQUENCE_ROUTE_FILE);
    const auto recordText = ReadFile(PLAYER_SEQUENCE_RECORD_FILE);
    const auto pluginText = ReadFile(PLAYER_SEQUENCE_PLUGIN_FILE);

    Config config{};
    std::string error;
    assert(ParseToml(configText, config, error));
    assert(config.enabled);
    assert(!config.diagnostics);
    assert(!ParseToml("enabled = 1\n", config, error));
    assert(!ParseToml("unknown = true\n", config, error));
    assert(!ParseToml("[diagnostics]\nverbose = true\n", config, error));

    AssertCrLfOnly(routeText);
    AssertCrLfOnly(recordText);
    ParsedTables parsed{};
    assert(ParsePlayerSequenceTables(routeText, recordText, parsed, error));
    assert(parsed.recordSetByRoute.size() == 350);
    assert(parsed.availableRoutes == 235);
    assert(parsed.recordSets.size() == 47);
    std::size_t records{};
    for (const auto& recordSet : parsed.recordSets) {
        assert(!recordSet.name.empty());
        assert(!recordSet.records.empty());
        records += recordSet.records.size();
    }
    assert(records == 808);
    assert(parsed.recordSetByRoute[(23 - 1) * WeaponClassCount]
        != parsed.recordSetByRoute[(24 - 1) * WeaponClassCount]);
    assert(parsed.recordSetByRoute[(24 - 1) * WeaponClassCount + 1]
        != parsed.recordSetByRoute[(24 - 1) * WeaponClassCount + 3]);
    assert(parsed.recordSetByRoute[(25 - 1) * WeaponClassCount + 1]
        != parsed.recordSetByRoute[(25 - 1) * WeaponClassCount + 3]);

    auto invalidRoutes = routeText;
    ReplaceOnce(invalidRoutes, "\r\n1\tJab\t1HT\t", "\r\n1\tJab\tHTH\t");
    assert(!ParsePlayerSequenceTables(invalidRoutes, recordText, parsed, error));
    assert(error.find("duplicate") != std::string::npos);

    invalidRoutes = routeText;
    const auto firstData = invalidRoutes.find("\r\n") + 2;
    const auto firstRecordSet = invalidRoutes.find('\t', invalidRoutes.find('\t',
        invalidRoutes.find('\t', firstData) + 1) + 1) + 1;
    const auto firstRecordSetEnd = invalidRoutes.find('\t', firstRecordSet);
    invalidRoutes.replace(
        firstRecordSet,
        firstRecordSetEnd - firstRecordSet,
        "UndefinedRecordSet");
    assert(!ParsePlayerSequenceTables(invalidRoutes, recordText, parsed, error));
    assert(error.find("undefined recordset") != std::string::npos);

    auto invalidRecords = recordText;
    ReplaceOnce(invalidRecords, "\tA1\t", "\tZZ\t");
    assert(!ParsePlayerSequenceTables(routeText, invalidRecords, parsed, error));
    assert(error.find("unknown player mode") != std::string::npos);

    invalidRecords = recordText;
    invalidRecords += "UnusedSet\tA1\t0\t0\t0\t0\r\n";
    assert(!ParsePlayerSequenceTables(routeText, invalidRecords, parsed, error));
    assert(error.find("unused recordset") != std::string::npos);

    invalidRoutes = std::string("\xEF\xBB\xBF") + routeText;
    assert(!ParsePlayerSequenceTables(invalidRoutes, recordText, parsed, error));
    assert(error.find("BOM") != std::string::npos);

    assert(pluginText.find("0x2386650") != std::string::npos);
    assert(pluginText.find("FirstActivePlayerSequencePointerRva") != std::string::npos);
    assert(pluginText.find("D2RL::PluginFlags::Shared | D2RL::PluginFlags::NativeHooks")
        != std::string::npos);
    assert(pluginText.find("ModScopedOnly") == std::string::npos);
    assert(pluginText.find("author = \"RuffnecKk\"") != std::string::npos);
    assert(pluginText.find("std::strcmp(runtimeBuild, \"93847\")")
        != std::string::npos);
    return 0;
}
