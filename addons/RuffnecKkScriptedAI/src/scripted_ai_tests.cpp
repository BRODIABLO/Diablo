#include "scripted_ai_config.hpp"
#include "scripted_ai_bridge.hpp"
#include "scripted_ai_fingerprint.hpp"
#include "scripted_ai_ownership.hpp"
#include "scripted_ai_sandbox.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <memory>
#include <set>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace {

using namespace ruffneckk::scripted_ai;

int Failures{};

void Expect(bool condition, std::string_view message) {
    if (condition) return;
    ++Failures;
    std::cerr << "FAIL: " << message << '\n';
}

[[nodiscard]] auto ReadAll(const std::filesystem::path& path)
        -> std::vector<std::uint8_t> {
    std::ifstream input(path, std::ios::binary);
    if (!input.is_open()) {
        throw std::runtime_error("cannot open " + path.string());
    }
    return {
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>(),
    };
}

[[nodiscard]] auto ReadText(const std::filesystem::path& path) -> std::string {
    const auto bytes = ReadAll(path);
    return {bytes.begin(), bytes.end()};
}

[[nodiscard]] auto ReadU16(
        std::span<const std::uint8_t> bytes,
        std::size_t offset) -> std::uint16_t {
    if (offset + 2U > bytes.size()) throw std::runtime_error("truncated PE");
    return static_cast<std::uint16_t>(bytes[offset])
        | static_cast<std::uint16_t>(bytes[offset + 1U] << 8U);
}

[[nodiscard]] auto ReadU32(
        std::span<const std::uint8_t> bytes,
        std::size_t offset) -> std::uint32_t {
    if (offset + 4U > bytes.size()) throw std::runtime_error("truncated PE");
    return static_cast<std::uint32_t>(bytes[offset])
        | (static_cast<std::uint32_t>(bytes[offset + 1U]) << 8U)
        | (static_cast<std::uint32_t>(bytes[offset + 2U]) << 16U)
        | (static_cast<std::uint32_t>(bytes[offset + 3U]) << 24U);
}

struct PeSection {
    std::string name;
    std::uint32_t virtualSize{};
    std::uint32_t virtualAddress{};
    std::uint32_t rawSize{};
    std::uint32_t rawOffset{};
};

[[nodiscard]] auto ParseSections(std::span<const std::uint8_t> bytes)
        -> std::vector<PeSection> {
    const auto peOffset = static_cast<std::size_t>(ReadU32(bytes, 0x3CU));
    if (ReadU32(bytes, peOffset) != 0x00004550U) {
        throw std::runtime_error("invalid PE signature");
    }
    const auto count = ReadU16(bytes, peOffset + 6U);
    const auto optionalSize = ReadU16(bytes, peOffset + 20U);
    const auto table = peOffset + 24U + optionalSize;
    std::vector<PeSection> result;
    result.reserve(count);
    for (std::size_t index{}; index < count; ++index) {
        const auto offset = table + index * 40U;
        if (offset + 40U > bytes.size()) {
            throw std::runtime_error("truncated PE section table");
        }
        std::string name;
        for (std::size_t character{}; character < 8U; ++character) {
            const auto value = bytes[offset + character];
            if (value == 0U) break;
            name.push_back(static_cast<char>(value));
        }
        result.push_back({
            .name = std::move(name),
            .virtualSize = ReadU32(bytes, offset + 8U),
            .virtualAddress = ReadU32(bytes, offset + 12U),
            .rawSize = ReadU32(bytes, offset + 16U),
            .rawOffset = ReadU32(bytes, offset + 20U),
        });
    }
    return result;
}

[[nodiscard]] auto RvaOffset(
        std::span<const std::uint8_t> bytes,
        const std::vector<PeSection>& sections,
        std::uintptr_t rva) -> std::size_t {
    for (const auto& section : sections) {
        const auto start = static_cast<std::uintptr_t>(
            section.virtualAddress);
        const auto size = static_cast<std::uintptr_t>(
            std::max(section.virtualSize, section.rawSize));
        if (rva < start || rva >= start + size) continue;
        const auto offset = static_cast<std::size_t>(section.rawOffset)
            + static_cast<std::size_t>(rva - start);
        if (offset >= bytes.size()) throw std::runtime_error("RVA is truncated");
        return offset;
    }
    throw std::runtime_error("RVA is outside the PE sections");
}

[[nodiscard]] auto CountMatches(
        std::span<const std::uint8_t> haystack,
        std::span<const std::uint8_t> needle) -> std::size_t {
    if (needle.empty()) return 0U;
    std::size_t count{};
    auto remaining = haystack;
    while (remaining.size() >= needle.size()) {
        const auto match = std::search(
            remaining.begin(),
            remaining.end(),
            needle.begin(),
            needle.end());
        if (match == remaining.end()) break;
        ++count;
        const auto consumed = static_cast<std::size_t>(
            std::distance(remaining.begin(), match)) + 1U;
        remaining = remaining.subspan(consumed);
    }
    return count;
}

void TestConfig() {
    const auto shippedText = ReadText(SCRIPTED_AI_CONFIG_FILE);
    const auto config = ParseConfig(shippedText);
    Expect(!config.enabled, "shipped config must be disabled");
    Expect(!config.diagnostics, "shipped diagnostics must be disabled");
    Expect(
        config.scriptDirectory == "scripts/ruffneckk-scripted-ai",
        "shipped script directory must be stable");
    Expect(
        config.limits.maxSourceBytes == HardSandboxLimits.maxSourceBytes,
        "shipped source cap must equal the compiled hard cap");
    Expect(
        config.limits.sessionHeapBytes == HardSandboxLimits.sessionHeapBytes,
        "shipped heap cap must equal the compiled hard cap");
    Expect(
        config.limits.maxInstructionsPerThink
            == HardSandboxLimits.maxInstructionsPerThink,
        "shipped think instruction cap must equal the compiled hard cap");

    const auto rejects = [](std::string_view text) {
        try {
            (void)ParseConfig(text);
            return false;
        } catch (...) {
            return true;
        }
    };
    Expect(
        rejects("schema_version=1\nunknown=true\n"),
        "unknown TOML keys must be rejected");
    Expect(
        rejects("schema_version=1\nscript_directory='../escape'\n"),
        "parent traversal must be rejected");
    Expect(
        rejects(
            "schema_version=1\n[sandbox]\nsession_heap_bytes=16777217\n"),
        "sandbox values above hard caps must be rejected");
    Expect(
        rejects(
            "schema_version=1\n[sandbox]\ninstruction_hook_interval=333\n"),
        "unaligned instruction budgets must be rejected");
    Expect(
        rejects("schema_version='1'\n"),
        "wrong TOML types must be rejected");

    const auto candidates = BuildConfigCandidates(
        L"C:/game/mods/Test/d2rloader/config",
        L"C:/game/d2rloader/config",
        L"C:/game/d2rloader/config");
    Expect(candidates.size() == 2U, "duplicate config scopes must collapse");
    Expect(
        candidates.front().generic_wstring().find(L"mods/Test")
            != std::wstring::npos,
        "active-mod config must have first priority");
}

struct MockFingerprint {
    std::size_t calls{};
    std::size_t failAt{NativeWindowCount};
};

[[nodiscard]] auto MockNativeCheck(
        void* userData,
        std::uintptr_t rva,
        std::span<const std::uint8_t> expected) noexcept -> bool {
    auto& state = *static_cast<MockFingerprint*>(userData);
    const auto index = state.calls++;
    if (index >= NativeFingerprint().size()) return false;
    const auto& window = NativeFingerprint()[index];
    return index != state.failAt && rva == window.rva
        && expected.size() == window.size;
}

void TestFingerprintDefinitions() {
    const auto& windows = NativeFingerprint();
    Expect(windows.size() == NativeWindowCount, "fingerprint needs 22 windows");
    std::set<std::string_view> names;
    std::set<std::uintptr_t> rvas;
    std::size_t hookTargets{};
    for (const auto& window : windows) {
        std::vector<std::uint8_t> bytes;
        std::string error;
        Expect(
            DecodeNativeWindow(window, bytes, error),
            "every native window must decode");
        Expect(bytes.size() == window.size, "decoded window size must match");
        Expect(
            window.expectedSha256.size() == 64U,
            "every window must carry a SHA-256 witness");
        Expect(names.insert(window.name).second, "window names must be unique");
        Expect(rvas.insert(window.rva).second, "window RVAs must be unique");
        if (window.hookTarget) {
            ++hookTargets;
            Expect(
                window.rva == ResolverHookRva,
                "the resolver must be the only hook target");
        }
    }
    Expect(hookTargets == 1U, "exactly one window must be hook-owned");

    MockFingerprint accepted{};
    const auto positive = ValidateNativeFingerprint(
        MockNativeCheck,
        &accepted);
    Expect(positive.accepted, "positive native fingerprint must pass");
    Expect(
        accepted.calls == NativeWindowCount,
        "positive fingerprint must inspect every window");

    MockFingerprint rejected{.failAt = 7U};
    const auto negative = ValidateNativeFingerprint(
        MockNativeCheck,
        &rejected);
    Expect(!negative.accepted, "a changed native window must fail closed");
    Expect(negative.failedIndex == 7U, "failure must identify its window");
}

void TestCanonicalImage(const std::filesystem::path& path) {
    const auto image = ReadAll(path);
    const auto sections = ParseSections(image);
    const auto text = std::find_if(
        sections.begin(),
        sections.end(),
        [](const PeSection& section) { return section.name == ".text"; });
    if (text == sections.end()) throw std::runtime_error("PE has no .text");
    const auto textOffset = static_cast<std::size_t>(text->rawOffset);
    const auto textSize = static_cast<std::size_t>(text->rawSize);
    if (textOffset + textSize > image.size()) {
        throw std::runtime_error("truncated .text section");
    }
    const std::span<const std::uint8_t> textBytes(
        image.data() + textOffset,
        textSize);

    for (const auto& window : NativeFingerprint()) {
        std::vector<std::uint8_t> expected;
        std::string error;
        if (!DecodeNativeWindow(window, expected, error)) {
            throw std::runtime_error(error);
        }
        const auto offset = RvaOffset(image, sections, window.rva);
        if (offset + expected.size() > image.size()) {
            throw std::runtime_error("native window is truncated");
        }
        Expect(
            std::equal(
                expected.begin(),
                expected.end(),
                image.begin() + static_cast<std::ptrdiff_t>(offset)),
            std::string(window.name) + " must match the governed image");
        Expect(
            CountMatches(textBytes, expected) == 1U,
            std::string(window.name) + " must be unique in .text");
    }
}

void TestOwnership() {
    Expect(
        IsSafeBeforeInstall({
            .state = OwnershipState::Unchanged,
            .kind = OwnershipKind::Unknown,
            .ownerCount = 0U,
        }),
        "vanilla unowned resolver must be accepted before install");
    Expect(
        !IsSafeBeforeInstall({
            .state = OwnershipState::Tracked,
            .kind = OwnershipKind::InlineHook,
            .ownerCount = 1U,
            .ownerPluginId = "other",
        }),
        "tracked resolver must be rejected before install");
    Expect(
        IsExclusivelyOwnedAfterInstall({
            .state = OwnershipState::Tracked,
            .kind = OwnershipKind::InlineHook,
            .ownerCount = 1U,
            .ownerPluginId = "ruffneckk-scripted-ai",
        }, "ruffneckk-scripted-ai"),
        "the exact managed owner must pass after install");
    Expect(
        !IsExclusivelyOwnedAfterInstall({
            .state = OwnershipState::Tracked,
            .kind = OwnershipKind::Multiple,
            .ownerCount = 2U,
        }, "ruffneckk-scripted-ai"),
        "multiple owners must fail closed after install");
}

void TestSandbox() {
    std::string error;
    auto sandbox = Sandbox::Create(HardSandboxLimits, error);
    Expect(sandbox != nullptr, "default sandbox must be constructible");
    if (!sandbox) return;

    for (const auto name : {
            "collectgarbage", "dofile", "load", "loadfile", "pcall",
            "print", "require", "package", "io", "os", "debug",
            "coroutine", "ffi", "string", "utf8", "warn", "xpcall"}) {
        Expect(!sandbox->HasGlobal(name), "dangerous Lua global must be absent");
    }
    Expect(sandbox->HasGlobal("_G"), "bounded base library must be available");
    Expect(
        sandbox->ExecuteForTesting(
            "assert(math.random == nil and math.randomseed == nil)",
            ExecutionPhase::Load,
            error),
        "random and bytecode dump entry points must be absent");

    TreeSummary summary{};
    Expect(
        sandbox->LoadBehaviorTree(
            "return {kind='selector', children={{kind='cast', skill=1}, {kind='wander'}}}",
            summary,
            error),
        "a bounded declarative tree must load");
    Expect(summary.nodeCount == 3U, "tree summary must count every node");
    Expect(summary.maximumDepth == 2U, "tree summary must report depth");
    Expect(summary.maximumChildren == 2U, "tree summary must report fanout");
    Expect(
        sandbox->MemoryUsed() <= HardSandboxLimits.sessionHeapBytes,
        "session allocator must remain inside its hard cap");

    Sandbox::ScriptHandle isolatedFirst = Sandbox::InvalidScriptHandle;
    Sandbox::ScriptHandle isolatedSecond = Sandbox::InvalidScriptHandle;
    Expect(
        sandbox->CompileBehaviorTree(
            "_G.poison=17; math.abs=nil; return {kind='first'}",
            isolatedFirst,
            summary,
            error),
        "first isolated tree must compile and remain retained");
    Expect(
        sandbox->CompileBehaviorTree(
            "assert(poison==nil and type(math.abs)=='function'); return {kind='second'}",
            isolatedSecond,
            summary,
            error),
        "one script must not mutate the next script environment");
    Expect(
        isolatedFirst >= 0 && isolatedSecond >= 0
            && isolatedFirst != isolatedSecond,
        "retained behavior trees must receive distinct handles");
    sandbox->ReleaseBehaviorTree(isolatedFirst);
    sandbox->ReleaseBehaviorTree(isolatedSecond);

    Expect(
        !sandbox->LoadBehaviorTree(
            "local t={kind='selector'}; t.children={t}; return t",
            summary,
            error),
        "cycles and shared behavior-tree nodes must be rejected");
    Expect(
        !sandbox->LoadBehaviorTree(
            "return setmetatable({kind='wander'}, {})",
            summary,
            error),
        "behavior-tree metatables must be rejected");
    Expect(
        !sandbox->ExecuteForTesting(
            "while true do end",
            ExecutionPhase::Load,
            error),
        "load instruction budget must interrupt an infinite script");
    const std::string bytecodePrefix("\x1bLua", 4U);
    Expect(
        !sandbox->ExecuteForTesting(
            bytecodePrefix,
            ExecutionPhase::Load,
            error),
        "binary Lua chunks must be rejected");

    auto depthLimits = HardSandboxLimits;
    depthLimits.maxTreeDepth = 2U;
    auto depthSandbox = Sandbox::Create(depthLimits, error);
    Expect(depthSandbox != nullptr, "lower tree depth must be accepted");
    if (depthSandbox) {
        Expect(
            !depthSandbox->LoadBehaviorTree(
                "return {kind='a',children={{kind='b',children={{kind='c'}}}}}",
                summary,
                error),
            "tree depth cap must fail closed");
    }

    auto nodeLimits = HardSandboxLimits;
    nodeLimits.maxTreeNodes = 3U;
    auto nodeSandbox = Sandbox::Create(nodeLimits, error);
    Expect(nodeSandbox != nullptr, "lower tree node cap must be accepted");
    if (nodeSandbox) {
        Expect(
            !nodeSandbox->LoadBehaviorTree(
                "return {kind='root',children={{kind='a'},{kind='b'},{kind='c'}}}",
                summary,
                error),
            "tree node cap must fail closed");
    }

    auto childLimits = HardSandboxLimits;
    childLimits.maxChildrenPerNode = 2U;
    auto childSandbox = Sandbox::Create(childLimits, error);
    Expect(childSandbox != nullptr, "lower child cap must be accepted");
    if (childSandbox) {
        Expect(
            !childSandbox->LoadBehaviorTree(
                "return {kind='root',children={{kind='a'},{kind='b'},{kind='c'}}}",
                summary,
                error),
            "tree child cap must fail closed");
    }

    auto growthLimits = HardSandboxLimits;
    growthLimits.perThinkHeapGrowthBytes = 4U * 1024U;
    auto growthSandbox = Sandbox::Create(growthLimits, error);
    Expect(growthSandbox != nullptr, "lower think heap cap must be accepted");
    if (growthSandbox) {
        Expect(
            !growthSandbox->ExecuteForTesting(
                "local t={} for i=1,2000 do t[i]={i,i,i,i,i,i,i,i} end",
                ExecutionPhase::Think,
                error),
            "per-think heap growth must fail closed");
    }

    auto sourceLimits = HardSandboxLimits;
    sourceLimits.maxSourceBytes = 1'024U;
    auto sourceSandbox = Sandbox::Create(sourceLimits, error);
    Expect(sourceSandbox != nullptr, "lower source cap must be accepted");
    if (sourceSandbox) {
        const std::string oversized(1'025U, ' ');
        Expect(
            !sourceSandbox->ExecuteForTesting(
                oversized,
                ExecutionPhase::Load,
                error),
            "source size cap must fail closed before compilation");
    }
}

struct TemporaryScriptTree {
    std::filesystem::path root;

    TemporaryScriptTree() {
        const auto nonce = std::chrono::steady_clock::now()
            .time_since_epoch().count();
        root = std::filesystem::temp_directory_path()
            / ("ruffneckk-scripted-ai-tests-" + std::to_string(nonce));
        std::filesystem::create_directories(root / "nested");
    }

    ~TemporaryScriptTree() {
        std::error_code error;
        std::filesystem::remove_all(root, error);
    }
};

void WriteTestScript(
        const std::filesystem::path& path,
        std::string_view source) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output.is_open()) {
        throw std::runtime_error("cannot create test script " + path.string());
    }
    output.write(source.data(), static_cast<std::streamsize>(source.size()));
    if (!output) {
        throw std::runtime_error("cannot write test script " + path.string());
    }
}

[[nodiscard]] auto MakeAiScriptRow(
        std::uint32_t monStatsId,
        std::string_view script,
        std::uint16_t fallbackAi = 0U,
        std::uint8_t targetProfile = ResolverTargetProfile,
        std::uint8_t enabled = 1U) -> AiScriptTableRow {
    if (script.size() >= AiScriptNameCapacity) {
        throw std::runtime_error("test script name exceeds the row capacity");
    }
    AiScriptTableRow row{
        .monStatsId = monStatsId,
        .fallbackAi = fallbackAi,
        .targetProfile = targetProfile,
        .enabled = enabled,
    };
    std::memcpy(row.script, script.data(), script.size());
    row.script[script.size()] = '\0';
    return row;
}

[[nodiscard]] auto StageTestBundle(
        std::uint64_t revision,
        std::span<const AiScriptTableRow> base,
        std::span<const AiScriptTableRow> rotw,
        const std::filesystem::path& root,
        std::string& error) -> std::shared_ptr<const PreparedBundle> {
    return StagePreparedBundle(
        revision,
        {.revision = revision * 10U + 1U, .rows = base},
        {.revision = revision * 10U + 2U, .rows = rotw},
        root,
        HardSandboxLimits,
        ReadScriptSource,
        error);
}

void TestAiScriptTransaction() {
    Expect(sizeof(AiScriptTableRow) == 76U, "AIScript row ABI must remain 76 bytes");
    Expect(
        offsetof(AiScriptTableRow, fallbackAi) == 70U,
        "AIScript fallback offset must remain frozen");

    TemporaryScriptTree scripts;
    WriteTestScript(
        scripts.root / "nested" / "beast.lua",
        "return {kind='selector', children={{kind='attack'}, {kind='wander'}}}");

    const std::array base{
        MakeAiScriptRow(1U, "nested/beast.lua", 7U),
        MakeAiScriptRow(2U, "nested/beast.lua", 8U),
    };
    const std::array rotw{
        MakeAiScriptRow(3U, "nested/beast.lua", 9U),
    };
    std::string error;
    const auto prepared = StageTestBundle(
        1U,
        base,
        rotw,
        scripts.root,
        error);
    Expect(prepared != nullptr, "a valid Base+RotW AIScript batch must stage");
    if (prepared) {
        Expect(prepared->scripts.size() == 1U, "shared scripts must be copied once");
        Expect(
            prepared->banks[BankIndex(ScriptBank::Base)].bindings.size() == 2U,
            "Base bindings must be copied");
        Expect(
            prepared->banks[BankIndex(ScriptBank::Rotw)].bindings.size() == 1U,
            "RotW bindings must be copied");
        const auto generation = CompileSessionGeneration(
            *prepared,
            44U,
            HardSandboxLimits,
            error);
        Expect(generation != nullptr, "a valid staged batch must compile atomically");
        if (generation) {
            Expect(generation->SessionId() == 44U, "generation must retain session id");
            Expect(generation->HasLuaVm(), "nonempty generation must own one Lua VM");
            Expect(generation->ScriptCount() == 1U, "generation must retain one tree");
        }
    }

    const std::array duplicate{
        MakeAiScriptRow(9U, "nested/beast.lua"),
        MakeAiScriptRow(9U, "nested/beast.lua"),
    };
    Expect(
        !StageTestBundle(2U, duplicate, {}, scripts.root, error),
        "duplicate enabled MonStats rows must reject the whole batch");

    const std::array badFallback{
        MakeAiScriptRow(10U, "nested/beast.lua", StockAiCount),
    };
    Expect(
        !StageTestBundle(3U, badFallback, {}, scripts.root, error),
        "non-stock fallback AI values must reject the whole batch");

    const std::array escaped{
        MakeAiScriptRow(11U, "../escape.lua"),
    };
    Expect(
        !StageTestBundle(4U, escaped, {}, scripts.root, error),
        "script parent traversal must reject the whole batch");

    const std::array disabled{
        MakeAiScriptRow(12U, "", 0U, 0U, 0U),
    };
    const auto empty = StageTestBundle(
        5U,
        disabled,
        {},
        scripts.root / "missing-root-is-safe-when-empty",
        error);
    Expect(empty != nullptr, "disabled rows must not require a script directory");
    if (empty) {
        const auto generation = CompileSessionGeneration(
            *empty,
            45U,
            HardSandboxLimits,
            error);
        Expect(generation != nullptr, "an empty generation must publish");
        Expect(
            generation && !generation->HasLuaVm(),
            "an empty generation must not allocate a Lua VM");
    }
}

void TestBridgeLifecycle() {
    TemporaryScriptTree scripts;
    WriteTestScript(
        scripts.root / "good.lua",
        "return {kind='selector', children={{kind='attack'}, {kind='wander'}}}");
    WriteTestScript(
        scripts.root / "replacement.lua",
        "return {kind='sequence', children={{kind='chase'}, {kind='attack'}}}");
    WriteTestScript(
        scripts.root / "invalid.lua",
        "return {kind='selector', children={42}}");

    const std::array goodRows{MakeAiScriptRow(20U, "good.lua", 4U)};
    const std::array replacementRows{
        MakeAiScriptRow(20U, "replacement.lua", 4U),
    };
    const std::array invalidRows{
        MakeAiScriptRow(20U, "invalid.lua", 4U),
    };
    std::string error;
    auto good = StageTestBundle(10U, goodRows, goodRows, scripts.root, error);
    auto replacement = StageTestBundle(
        12U,
        replacementRows,
        replacementRows,
        scripts.root,
        error);
    auto invalid = StageTestBundle(
        11U,
        invalidRows,
        invalidRows,
        scripts.root,
        error);
    Expect(good && replacement && invalid, "lifecycle fixtures must stage as source snapshots");
    if (!good || !replacement || !invalid) return;

    BridgeCoordinator bridge(HardSandboxLimits);
    Expect(bridge.PublishPrepared(good, error), "source snapshot must enter pending state");
    Expect(!bridge.Prepared(), "pending source must not publish before compilation");

    // A remote client announces the session, but runOnGameThread returns
    // Unavailable, so this test deliberately never calls reconciliation.
    bridge.AnnounceGameJoined(100U);
    Expect(!bridge.ActiveFor(100U), "remote client path must never publish a VM");
    Expect(!bridge.Prepared(), "remote client path must not publish uncompiled data");

    // A late queued host callback sees the matching GameLeft cancellation and
    // cannot resurrect the abandoned session.
    bridge.AnnounceGameLeft(100U);
    Expect(
        bridge.ReconcileAuthoritativeSession(error),
        "late cancellation callback must reconcile to no session");
    Expect(!bridge.ActiveFor(100U), "cancelled session must remain unpublished");

    bridge.AnnounceGameJoined(200U);
    Expect(
        bridge.ReconcileAuthoritativeSession(error),
        "host game-thread callback must compile the pending generation");
    auto first = bridge.ActiveFor(200U);
    Expect(first != nullptr, "host must publish its session generation");
    Expect(
        bridge.Prepared() && bridge.Prepared()->lifecycleRevision == 10U,
        "successful compile must publish its source revision");

    Expect(bridge.PublishPrepared(invalid, error), "invalid tree source must stage first");
    Expect(
        !bridge.ReconcileAuthoritativeSession(error),
        "invalid tree must reject the full publication transaction");
    Expect(
        bridge.ActiveFor(200U) == first,
        "invalid replacement must preserve the prior same-session generation");
    Expect(
        bridge.Prepared() && bridge.Prepared()->lifecycleRevision == 10U,
        "invalid replacement must preserve the prior prepared revision");

    std::weak_ptr<const SessionGeneration> oldGeneration = first;
    Expect(
        bridge.PublishPrepared(replacement, error)
            && bridge.ReconcileAuthoritativeSession(error),
        "valid replacement must publish atomically");
    const auto second = bridge.ActiveFor(200U);
    Expect(
        second && second->LifecycleRevision() == 12U,
        "replacement generation must expose the new revision");
    first.reset();
    Expect(
        oldGeneration.expired(),
        "replaced generation and its Lua VM must be destroyed after readers release it");

    bridge.AnnounceGameJoined(201U);
    bridge.AnnounceGameLeft(201U);
    Expect(
        bridge.ReconcileAuthoritativeSession(error),
        "join followed by leave must cancel before compilation");
    Expect(!bridge.ActiveFor(201U), "cancelled newer session must have no generation");
}

void TestPluginPolicy() {
    const auto source = ReadText(SCRIPTED_AI_PLUGIN_SOURCE_FILE);
    Expect(
        source.find("PluginFlags::Server | D2RL::PluginFlags::NativeHooks")
            != source.npos,
        "plugin must declare the server native-hook role");
    Expect(
        source.find("ModScopedOnly") == source.npos,
        "plugin must remain global/mod-local hybrid");
    Expect(
        source.find("GetBuildName") != source.npos,
        "reported build name must remain diagnostic");
    Expect(
        source.find("3.2.92777") == source.npos
            && source.find("3.3.93847") == source.npos,
        "plugin source must not contain a build allowlist");
    Expect(
        source.find("InstallInlineHook") == source.npos,
        "bridge build must not install its future hook");
    Expect(
        source.find("CustomTableServiceV1") != source.npos
            && source.find("AiScriptTableName") != source.npos,
        "bridge build must own a private AIScript custom table");
    Expect(
        source.find("runOnGameThread") != source.npos
            && source.find("GameJoined") != source.npos
            && source.find("GameLeft") != source.npos,
        "bridge build must route lifecycle publication through the game thread");
    Expect(
        source.find("D2_AI_Attack") == source.npos
            && source.find("D2MonUseSkill") == source.npos,
        "bridge gate must not contain gameplay action helpers");

    const auto bridgeSource = ReadText(SCRIPTED_AI_BRIDGE_SOURCE_FILE);
    Expect(
        bridgeSource.find("CompileSessionGeneration") != bridgeSource.npos
            && bridgeSource.find("pending_") != bridgeSource.npos,
        "bridge core must compile pending snapshots before publication");
}

} // namespace

int main(int argc, char** argv) {
    try {
        TestConfig();
        TestFingerprintDefinitions();
        TestOwnership();
        TestSandbox();
        TestAiScriptTransaction();
        TestBridgeLifecycle();
        TestPluginPolicy();
        if (argc > 1) {
            TestCanonicalImage(argv[1]);
        } else {
            std::cout << "NOTE: governed PE uniqueness test was not configured\n";
        }
    } catch (const std::exception& exception) {
        ++Failures;
        std::cerr << "FAIL: unexpected exception: " << exception.what() << '\n';
    }

    if (Failures != 0) {
        std::cerr << Failures << " Scripted AI bridge assertion(s) failed\n";
        return 1;
    }
    std::cout << "Scripted AI bridge tests PASS\n";
    return 0;
}
