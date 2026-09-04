#include "automap_serialization_policy.hpp"

#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace {

int Failures{};

#define CHECK(expression)                                                     \
    do {                                                                      \
        if (!(expression)) {                                                  \
            std::cerr << __FILE__ << ':' << __LINE__                          \
                      << ": CHECK failed: " #expression << '\n';             \
            ++Failures;                                                       \
        }                                                                     \
    } while (false)

auto ReadAll(const char* path) -> std::string {
    std::ifstream input(path, std::ios::binary);
    return {
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>(),
    };
}

void CheckByteCountPolicy() {
    using namespace RuffnecKk::AutomapSerializationFix;

    const auto vanillaMaximum = ComputeSerializedCellByteCount(5'461U);
    CHECK(vanillaMaximum.representable);
    CHECK(vanillaMaximum.byteCount == 32'766U);

    const auto firstFormerlyRejected =
        ComputeSerializedCellByteCount(5'462U);
    CHECK(firstFormerlyRejected.representable);
    CHECK(firstFormerlyRejected.byteCount == 32'772U);

    const auto crashWitness = ComputeSerializedCellByteCount(7'556U);
    CHECK(crashWitness.representable);
    CHECK(crashWitness.byteCount == 45'336U);

    const auto maximum = ComputeSerializedByteCount(
        MaximumSerializedWordCount);
    CHECK(maximum.representable);
    CHECK(maximum.byteCount == 0xFFFFFFFEU);

    const auto overflow = ComputeSerializedByteCount(
        MaximumSerializedWordCount + 1U);
    CHECK(!overflow.representable);
    CHECK(overflow.byteCount == 0U);

    CHECK(SerializerByteCountOriginal.size() == 13U);
    CHECK(SerializerByteCountPatched.size() == 13U);
    CHECK(SerializerByteCountOriginal != SerializerByteCountPatched);
    constexpr std::array<std::uint8_t, 13U> expectedPatch{
        0x33, 0xC9, 0x8B, 0x56, 0x08, 0x03, 0xD2,
        0x0F, 0x43, 0xCA, 0x41, 0x89, 0x0F,
    };
    CHECK(SerializerByteCountPatched == expectedPatch);
}

void CheckSourcePolicy() {
    const auto plugin = ReadAll(AUTOMAP_SERIALIZATION_PLUGIN_FILE);
    const auto cmake = ReadAll(AUTOMAP_SERIALIZATION_CMAKE_FILE);
    const auto resource = ReadAll(AUTOMAP_SERIALIZATION_RESOURCE_FILE);

    CHECK(!plugin.empty());
    CHECK(plugin.find(".author = \"RuffnecKk\"") != std::string::npos);
    CHECK(plugin.find(".version = \"0.1.0\"") != std::string::npos);
    CHECK(plugin.find("Prevents large explored maps from crashing during area transitions.")
        != std::string::npos);
    CHECK(plugin.find("D2RL::PluginFlags::Client") != std::string::npos);
    CHECK(plugin.find("D2RL::PluginFlags::NativeHooks") != std::string::npos);
    CHECK(plugin.find("ModScopedOnly") == std::string::npos);
    CHECK(plugin.find("92777") == std::string::npos);
    CHECK(plugin.find("93847") == std::string::npos);
    CHECK(plugin.find("93787") == std::string::npos);
    CHECK(plugin.find("strcmp(runtimeBuild") == std::string::npos);
    CHECK(plugin.find("PatchBytes(") != std::string::npos);
    CHECK(plugin.find("CheckExpectedBytes(") != std::string::npos);
    CHECK(plugin.find("GetBuildName(context)") != std::string::npos);
    CHECK(plugin.find("config=none") != std::string::npos);
    CHECK(plugin.find("toml") == std::string::npos);
    CHECK(plugin.find("ConfigFile") == std::string::npos);

    CHECK(cmake.find("GIT_TAG 4933e2c42cb2592958cd0df3b6dc5003102252d1")
        != std::string::npos);
    CHECK(cmake.find("d2rlplugin_embed_config") == std::string::npos);
    CHECK(cmake.find("/W4 /WX") != std::string::npos);
    CHECK(resource.find("FILEVERSION 0,1,0,0") != std::string::npos);
    CHECK(resource.find("d2rl-ruffneckk-automap-serialization-fix.dll")
        != std::string::npos);
}

} // namespace

int main() {
    CheckByteCountPolicy();
    CheckSourcePolicy();
    if (Failures != 0) {
        std::cerr << Failures << " test(s) failed.\n";
        return 1;
    }
    std::cout << "Automap Serialization Fix policy tests passed.\n";
    return 0;
}
