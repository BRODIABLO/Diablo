#include "isc12_config.hpp"
#include "isc12_contract.hpp"
#include "isc12_loader.hpp"
#include "isc12_native_sites.hpp"

#include <cstdlib>
#include <bit>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <numeric>
#include <string>
#include <vector>

namespace {
int Failures{};

#define CHECK(expression) \
    do { \
        if (!(expression)) { \
            std::cerr << "CHECK failed at line " << __LINE__ \
                      << ": " #expression "\n"; \
            ++Failures; \
        } \
    } while (false)
} // namespace

int main() {
    using namespace ruffneckk::isc12;

    static_assert(SerializedBitWidth == 12);
    static_assert(SerializedSentinel == 0x0FFF);
    static_assert(MaximumStatId == 4094);
    static_assert(MaximumRecordCount == 4095);
    static_assert(InternalWordSentinel == 0xFFFF);
    static_assert(InstalledHookCount == 0);
    static_assert(InstalledPatchCount == 2);

    CHECK(CanEncodeRel32(0x1000, 0x1005));
    CHECK(CanEncodeRel32(0x80001000, 0x1005));
    CHECK(!CanEncodeRel32(0x1000, UINT64_C(0x80001005)));

    std::vector<std::string> commitEvents;
    auto commitResult = CommitLoaderMutation(
        [&]() noexcept { commitEvents.emplace_back("reserve"); },
        [&]() noexcept {
            commitEvents.emplace_back("tail");
            return true;
        },
        [&]() noexcept { commitEvents.emplace_back("activate"); },
        [&]() noexcept {
            commitEvents.emplace_back("cap");
            return true;
        },
        [&]() noexcept { commitEvents.emplace_back("publish"); });
    CHECK(commitResult == LoaderInstallResult::Active);
    CHECK((commitEvents == std::vector<std::string>{
        "reserve", "tail", "activate", "cap", "publish",
    }));

    commitEvents.clear();
    commitResult = CommitLoaderMutation(
        [&]() noexcept { commitEvents.emplace_back("reserve"); },
        [&]() noexcept {
            commitEvents.emplace_back("tail");
            return false;
        },
        [&]() noexcept { commitEvents.emplace_back("activate"); },
        [&]() noexcept {
            commitEvents.emplace_back("cap");
            return true;
        },
        [&]() noexcept { commitEvents.emplace_back("publish"); });
    CHECK(commitResult
        == LoaderInstallResult::PartialCommitColdRestartRequired);
    CHECK((commitEvents == std::vector<std::string>{"reserve", "tail"}));

    commitEvents.clear();
    commitResult = CommitLoaderMutation(
        [&]() noexcept { commitEvents.emplace_back("reserve"); },
        [&]() noexcept {
            commitEvents.emplace_back("tail");
            return true;
        },
        [&]() noexcept { commitEvents.emplace_back("activate"); },
        [&]() noexcept {
            commitEvents.emplace_back("cap");
            return false;
        },
        [&]() noexcept { commitEvents.emplace_back("publish"); });
    CHECK(commitResult
        == LoaderInstallResult::PartialCommitColdRestartRequired);
    CHECK((commitEvents == std::vector<std::string>{
        "reserve", "tail", "activate", "cap",
    }));

    CHECK(IsValidStatId(0));
    CHECK(IsValidStatId(4094));
    CHECK(!IsValidStatId(4095));
    CHECK(IsValidRecordCount(0));
    CHECK(IsValidRecordCount(4095));
    CHECK(!IsValidRecordCount(4096));
    CHECK(AddedSerializedBits(16, 1) == 51);
    CHECK(AddedSerializedBits(500, 10) == 1530);

    std::vector<DescriptionEntry> descriptions{
        {4094, 1}, {511, 200}, {0, 200}, {1023, 15},
    };
    CHECK(SortDescriptionEntries(descriptions));
    CHECK(descriptions[0].statId == 4094);
    CHECK(descriptions[1].statId == 1023);
    CHECK(descriptions[2].priority == 200);
    CHECK(descriptions[3].priority == 200);
    descriptions.push_back({4095, 500});
    CHECK(!SortDescriptionEntries(descriptions));

    std::vector<DescriptionEntry> signedPriorities{
        {0, std::bit_cast<std::int16_t>(std::uint16_t{0x7FFF})},
        {1, std::bit_cast<std::int16_t>(std::uint16_t{0x8000})},
        {2, std::bit_cast<std::int16_t>(std::uint16_t{0xFFFF})},
        {3, 0},
    };
    CHECK(SortDescriptionEntries(signedPriorities));
    CHECK(signedPriorities[0].statId == 1);
    CHECK(signedPriorities[1].statId == 2);
    CHECK(signedPriorities[2].statId == 3);
    CHECK(signedPriorities[3].statId == 0);

    const auto checkDenseDescriptionBoundary = [](std::size_t count) {
        std::vector<DescriptionSource> rows(count);
        for (std::size_t id = 0; id < count; ++id) {
            rows[id] = {
                1,
                static_cast<std::int16_t>(count - id),
            };
        }
        std::vector<std::uint16_t> index{0xBEEF};
        CHECK(BuildDescriptionIndex(rows, index));
        CHECK(index.size() == count);
        if (!index.empty()) {
            CHECK(index.front() == count - 1);
            CHECK(index.back() == 0);
        }
    };
    checkDenseDescriptionBoundary(511);
    checkDenseDescriptionBoundary(512);
    checkDenseDescriptionBoundary(1023);
    checkDenseDescriptionBoundary(2047);
    checkDenseDescriptionBoundary(4095);

    std::vector<DescriptionSource> sparseRows(4095);
    sparseRows[0] = {1, 30};
    sparseRows[511] = {2, 20};
    sparseRows[1023] = {3, 10};
    sparseRows[2047] = {4, 40};
    sparseRows[4094] = {5, 0};
    std::vector<std::uint16_t> sparseIndex;
    CHECK(BuildDescriptionIndex(sparseRows, sparseIndex));
    CHECK((sparseIndex == std::vector<std::uint16_t>{
        4094, 1023, 511, 0, 2047,
    }));

    std::vector<DescriptionSource> tooManyRows(4096, {1, 1});
    std::vector<std::uint16_t> untouched{7, 8, 9};
    CHECK(!BuildDescriptionIndex(tooManyRows, untouched));
    CHECK((untouched == std::vector<std::uint16_t>{7, 8, 9}));

    Config config{};
    std::string error;
    constexpr std::string_view validToml = R"toml(
config_version = 1
enabled = true
[diagnostics]
enabled = false
)toml";
    CHECK(ParseToml(validToml, config, error));
    CHECK(config.enabled && !config.diagnostics);
    CHECK(!ParseToml(
        "config_version = 2\nenabled = true\n"
        "[diagnostics]\nenabled = false\n",
        config,
        error));
    CHECK(!ParseToml(
        "config_version = 1\nenabled = true\nunknown = 1\n"
        "[diagnostics]\nenabled = false\n",
        config,
        error));

    const auto candidates = BuildConfigCandidates(
        std::filesystem::path{L"mod"},
        std::filesystem::path{L"scope"},
        std::filesystem::path{L"global"},
        std::filesystem::path{L"ruffneckk-isc12.toml"});
    CHECK(candidates.size() == 3);
    const auto deduplicated = BuildConfigCandidates(
        std::filesystem::path{L"same"},
        std::filesystem::path{L"same"},
        std::filesystem::path{L"global"},
        std::filesystem::path{L"ruffneckk-isc12.toml"});
    CHECK(deduplicated.size() == 2);

    CHECK(!FoundationPatterns.empty());
    for (const auto& pattern : FoundationPatterns) {
        CHECK(pattern.id != nullptr);
        CHECK(pattern.rva != 0);
        CHECK(!pattern.bytes.empty());
        CHECK(pattern.bytes.size() == pattern.mask.size());
        CHECK(std::string_view{pattern.id} != "item.decode-entry");
        CHECK(std::string_view{pattern.id} != "item.serialize-entry");
        for (const auto mask : pattern.mask) CHECK(mask == 0xFF);
    }

    std::ifstream configFile(
        std::filesystem::path{ISC12_CONFIG_PATH}, std::ios::binary);
    const std::string configText{
        std::istreambuf_iterator<char>{configFile},
        std::istreambuf_iterator<char>{}};
    CHECK(!configText.empty());
    CHECK(ParseToml(configText, config, error));

    std::ifstream pluginFile(
        std::filesystem::path{ISC12_PLUGIN_SOURCE_PATH}, std::ios::binary);
    const std::string pluginText{
        std::istreambuf_iterator<char>{pluginFile},
        std::istreambuf_iterator<char>{}};
    CHECK(!pluginText.empty());
    CHECK(pluginText.find("D2RL::GetBuildName(context)")
        != std::string::npos);
    CHECK(pluginText.find("without a version allowlist")
        != std::string::npos);
    CHECK(pluginText.find("92777") == std::string::npos);
    CHECK(pluginText.find("93847") == std::string::npos);
    CHECK(pluginText.find("IsSupportedBuild") == std::string::npos);
    CHECK(pluginText.find("RuntimeBuild ==") == std::string::npos);
    CHECK(pluginText.find("RuntimeBuild !=") == std::string::npos);
    CHECK(pluginText.find("InstallInlineHook") == std::string::npos);
    CHECK(pluginText.find("VirtualProtect") == std::string::npos);
    CHECK(pluginText.find("WriteProcessMemory") == std::string::npos);
    CHECK(pluginText.find("ProcessMutexNameFormat") != std::string::npos);
    CHECK(pluginText.find("GetCurrentProcessId()") != std::string::npos);
    CHECK(pluginText.find("RuffnecKk.ISC12.%lu") != std::string::npos);
    constexpr std::string_view publicDescription =
        "Supports up to 4,095 item stat definitions for overhaul mods.";
    CHECK(pluginText.find(publicDescription) != std::string::npos);

    std::ifstream resourceFile(
        std::filesystem::path{ISC12_RESOURCE_SOURCE_PATH}, std::ios::binary);
    const std::string resourceText{
        std::istreambuf_iterator<char>{resourceFile},
        std::istreambuf_iterator<char>{}};
    CHECK(resourceText.find(publicDescription) != std::string::npos);

    std::ifstream loaderFile(
        std::filesystem::path{ISC12_LOADER_SOURCE_PATH}, std::ios::binary);
    const std::string loaderText{
        std::istreambuf_iterator<char>{loaderFile},
        std::istreambuf_iterator<char>{}};
    CHECK(!loaderText.empty());
    const auto reserveTailLifetime = loaderText.find(
        "AnyMutationInstalled = true;");
    const auto tailCommit = loaderText.find("PatchJmpRel32");
    const auto conservativeCapGuard = loaderText.find(
        "InterlockedExchange(&State->capMayBeExtended, 1)");
    const auto capCommit = loaderText.find("LoaderContext->PatchWriteU32");
    CHECK(reserveTailLifetime != std::string::npos);
    CHECK(tailCommit != std::string::npos);
    CHECK(conservativeCapGuard != std::string::npos);
    CHECK(capCommit != std::string::npos);
    CHECK(reserveTailLifetime < tailCommit);
    CHECK(tailCommit < conservativeCapGuard);
    CHECK(conservativeCapGuard < capCommit);
    CHECK(tailCommit < capCommit);
    CHECK(loaderText.find("InstallInlineHook") == std::string::npos);
    CHECK(loaderText.find("MaximumRecordCount") != std::string::npos);
    CHECK(loaderText.find("FailClosed") != std::string::npos);
    CHECK(loaderText.find(
        "DescFunc tail publication returned an uncertain result")
        != std::string::npos);
    CHECK(loaderText.find("ShutdownRundownTimeoutMilliseconds")
        != std::string::npos);
    CHECK(loaderText.find("PAGE_EXECUTE_READ") != std::string::npos);
    CHECK(loaderText.find("PAGE_EXECUTE_READWRITE")
        != std::string::npos);
    CHECK(loaderText.find("0x31F0AB") != std::string::npos);
    CHECK(loaderText.find("0x31ED38") != std::string::npos);

    std::ifstream loaderAsmFile(
        std::filesystem::path{ISC12_LOADER_ASM_PATH}, std::ios::binary);
    const std::string loaderAsmText{
        std::istreambuf_iterator<char>{loaderAsmFile},
        std::istreambuf_iterator<char>{}};
    CHECK(!loaderAsmText.empty());
    CHECK(loaderAsmText.find("mov qword ptr [rsp+20h], rax")
        != std::string::npos);
    CHECK(loaderAsmText.find("mov r9, qword ptr [rsp+48h]")
        != std::string::npos);
    CHECK(loaderAsmText.find("mov r15, qword ptr [rsp+0F40h]")
        != std::string::npos);
    CHECK(loaderAsmText.find("mov r14, qword ptr [rsp+0F48h]")
        != std::string::npos);
    CHECK(loaderAsmText.find("mov r13, qword ptr [rsp+0F50h]")
        != std::string::npos);
    CHECK(loaderAsmText.find("mov rbx, qword ptr [rsp+0F78h]")
        != std::string::npos);
    CHECK(loaderAsmText.find("mov rsi, qword ptr [rsp+0F80h]")
        != std::string::npos);
    CHECK(loaderAsmText.find("mov rdi, qword ptr [rsp+0F88h]")
        != std::string::npos);
    CHECK(loaderAsmText.find("mov ecx, 7") != std::string::npos);
    CHECK(loaderAsmText.find("int 29h") != std::string::npos);
    CHECK(loaderAsmText.find("ud2") == std::string::npos);
    CHECK(loaderAsmText.find("ISC12LoaderRelayTemplateSuccessExit")
        != std::string::npos);
    CHECK(loaderAsmText.find("ISC12LoaderRelayTemplateVanillaExit")
        != std::string::npos);
    const auto firstDecrement = loaderAsmText.find("lock dec");
    const auto handler = loaderAsmText.find("ISC12LoaderTailMidHook PROC");
    CHECK(firstDecrement != std::string::npos);
    CHECK(handler != std::string::npos);
    CHECK(loaderAsmText.find("lock dec", handler) == std::string::npos);

    std::ifstream cmakeFile(
        std::filesystem::path{ISC12_CMAKE_PATH}, std::ios::binary);
    const std::string cmakeText{
        std::istreambuf_iterator<char>{cmakeFile},
        std::istreambuf_iterator<char>{}};
    CHECK(cmakeText.find("RUFFNECKK_PUBLIC_ARCHIVE_ELIGIBLE FALSE")
        != std::string::npos);
    CHECK(cmakeText.find("RUFFNECKK_PUBLIC_ARCHIVE_ELIGIBLE TRUE")
        == std::string::npos);

    return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
