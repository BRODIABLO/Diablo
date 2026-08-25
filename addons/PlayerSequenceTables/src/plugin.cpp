#define NOMINMAX

#include <D2RLPlugin/api.h>

#include "player_sequence_policy.hpp"

#include <Windows.h>
#include <bcrypt.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace RuffnecKk::PlayerSequenceTables {
namespace {

using namespace ruffneckk::player_sequence_tables;

constexpr wchar_t ConfigFileName[] = L"ruffneckk-player-sequence-tables.toml";
constexpr wchar_t RouteTableName[] = L"playerseqmap.txt";
constexpr wchar_t RecordTableName[] = L"playerseq.txt";
constexpr std::uintptr_t PlayerSequenceTableRva = 0x2386650;
constexpr std::uintptr_t FirstActivePlayerSequencePointerRva
    = PlayerSequenceTableRva + sizeof(std::uintptr_t);
constexpr std::uint64_t DescriptorExtra = 0x100;

constexpr std::array<std::uintptr_t, SequenceCount> OriginalGroupRvas{
    0x2383150, 0x2383300, 0x23834B0, 0x2383660, 0x23837E0,
    0x2383990, 0x2383BB0, 0x2383E10, 0x2384020, 0x23841A0,
    0x2384360, 0x2384530, 0x23846E0, 0x2384AC0, 0x2384C60,
    0x2384E60, 0x2385030, 0x23853B0, 0x2385200, 0x2385570,
    0x2385750, 0x2385900, 0x2385AD0, 0x2385EE0, 0x2386350,
};

#pragma pack(push, 1)
struct PlayerSequenceRecord {
    std::uint16_t sequence{};
    std::uint8_t mode{};
    std::uint8_t frame{};
    std::uint8_t direction{};
    std::uint8_t event{};
};
#pragma pack(pop)

struct PlayerSequenceDescriptor {
    const PlayerSequenceRecord* records{};
    std::int32_t sequenceFrameCount{};
    std::int32_t animationFrameCount{};
    std::uint64_t extra{DescriptorExtra};
};

static_assert(sizeof(PlayerSequenceRecord) == 6);
static_assert(sizeof(PlayerSequenceDescriptor) == 24);
static_assert(offsetof(PlayerSequenceDescriptor, records) == 0);
static_assert(offsetof(PlayerSequenceDescriptor, sequenceFrameCount) == 8);
static_assert(offsetof(PlayerSequenceDescriptor, animationFrameCount) == 12);
static_assert(offsetof(PlayerSequenceDescriptor, extra) == 16);
static_assert(sizeof(std::uintptr_t) == 8);

enum class OperationalState {
    Disabled,
    Vanilla,
    Active,
};

struct LoadedTables {
    std::filesystem::path routePath;
    std::filesystem::path recordPath;
    std::string routeText;
    std::string recordText;
};

struct ArenaResult {
    void* allocation{};
    std::size_t bytes{};
    std::array<std::uintptr_t, SequenceCount> groupPointers{};
};

const D2RL::PluginContext* Context{};
std::uint8_t* Base{};
Config Settings{};
std::string LoadedConfigPath{"built-in defaults"};
std::string LoadedTablePaths{"none"};
std::string LoadedTableHash{"none"};
std::size_t LoadedRecordSets{};
std::size_t LoadedRecords{};
std::size_t LoadedRoutes{};
void* ProcessLifetimeArena{};
std::size_t ProcessLifetimeArenaBytes{};
std::atomic<OperationalState> State{OperationalState::Vanilla};

std::string DisplayPath(const std::filesystem::path& path) {
    try {
        return path.string();
    } catch (...) {
        return "<unprintable path>";
    }
}

std::vector<std::filesystem::path> ConfigCandidates() {
    std::vector<std::filesystem::path> directories;
    if (Context && Context->activeMod && Context->activeMod[0] != '\0'
            && Context->modSupportDirectory
            && Context->modSupportDirectory[0] != L'\0') {
        directories.emplace_back(
            std::filesystem::path(Context->modSupportDirectory) / L"config");
    }
    if (Context && Context->pluginConfigPath
            && Context->pluginConfigPath[0] != L'\0') {
        directories.emplace_back(
            std::filesystem::path(Context->pluginConfigPath).parent_path());
    }
    std::error_code error;
    const auto current = std::filesystem::current_path(error);
    if (!error) directories.emplace_back(current / L"d2rloader" / L"config");

    std::vector<std::filesystem::path> result;
    for (const auto& directory : directories) {
        const auto candidate = (directory / ConfigFileName).lexically_normal();
        if (std::find(result.begin(), result.end(), candidate) == result.end()) {
            result.emplace_back(candidate);
        }
    }
    return result;
}

bool ReadTextFile(
        const std::filesystem::path& path,
        std::size_t maximumBytes,
        std::string& text,
        std::string& error) {
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input.is_open()) {
        error = "cannot open " + DisplayPath(path);
        return false;
    }
    const auto size = input.tellg();
    if (size < 0 || static_cast<std::uint64_t>(size) > maximumBytes) {
        error = "file exceeds its safety limit: " + DisplayPath(path);
        return false;
    }
    text.resize(static_cast<std::size_t>(size));
    input.seekg(0, std::ios::beg);
    if (!text.empty()
            && !input.read(text.data(), static_cast<std::streamsize>(text.size()))) {
        error = "cannot read " + DisplayPath(path);
        return false;
    }
    return true;
}

bool InspectRegularFile(
        const std::filesystem::path& path,
        bool& exists,
        std::string& error) {
    exists = false;
    std::error_code inspectError;
    const auto status = std::filesystem::status(path, inspectError);
    if (inspectError) {
        if (inspectError == std::errc::no_such_file_or_directory) return true;
        error = "cannot inspect " + DisplayPath(path)
            + ": " + inspectError.message();
        return false;
    }
    if (!std::filesystem::exists(status)) return true;
    if (!std::filesystem::is_regular_file(status)) {
        error = "expected a regular file: " + DisplayPath(path);
        return false;
    }
    exists = true;
    return true;
}

bool LoadConfig() noexcept {
    Settings = {};
    LoadedConfigPath = "built-in defaults";
    for (const auto& path : ConfigCandidates()) {
        bool exists{};
        std::string error;
        if (!InspectRegularFile(path, exists, error)) {
            const auto message = "PlayerSequenceTables: " + error + ".";
            Context->LogError(message.c_str());
            return false;
        }
        if (!exists) continue;
        std::string text;
        if (!ReadTextFile(path, 64 * 1024, text, error)) {
            const auto message = "PlayerSequenceTables: " + error + ".";
            Context->LogError(message.c_str());
            return false;
        }
        Config parsed{};
        if (!ParseToml(text, parsed, error)) {
            const auto message = "PlayerSequenceTables: invalid config "
                + DisplayPath(path) + ": " + error + ".";
            Context->LogError(message.c_str());
            return false;
        }
        Settings = parsed;
        LoadedConfigPath = DisplayPath(path);
        return true;
    }
    return true;
}

std::vector<std::filesystem::path> ExcelCandidates() {
    std::vector<std::filesystem::path> result;
    if (!Context || !Context->modDirectory
            || Context->modDirectory[0] == L'\0') {
        return result;
    }
    const std::filesystem::path root(Context->modDirectory);
    result.emplace_back((root / L"data/global/excel").lexically_normal());
    if (Context->activeMod && Context->activeMod[0] != '\0') {
        result.emplace_back((
            root / (std::string(Context->activeMod) + ".mpq")
            / L"data/global/excel").lexically_normal());
    }
    result.erase(std::unique(result.begin(), result.end()), result.end());
    return result;
}

bool FindAndLoadTables(
        std::optional<LoadedTables>& result,
        std::string& error) {
    result.reset();
    const bool hasActiveMod = Context && Context->activeMod
        && Context->activeMod[0] != '\0';
    if (!hasActiveMod) return true;

    const auto candidates = ExcelCandidates();
    if (candidates.empty()) {
        error = "D2RLoader did not expose the active mod directory";
        return false;
    }
    for (const auto& directory : candidates) {
        const auto routePath = directory / RouteTableName;
        const auto recordPath = directory / RecordTableName;
        bool hasRoute{};
        bool hasRecords{};
        if (!InspectRegularFile(routePath, hasRoute, error)
                || !InspectRegularFile(recordPath, hasRecords, error)) {
            return false;
        }
        if (!hasRoute && !hasRecords) continue;
        if (hasRoute != hasRecords) {
            error = "playerseqmap.txt and playerseq.txt must be installed together under "
                + DisplayPath(directory);
            return false;
        }
        LoadedTables loaded{routePath, recordPath, {}, {}};
        if (!ReadTextFile(routePath, MaximumTableBytes, loaded.routeText, error)
                || !ReadTextFile(recordPath, MaximumTableBytes, loaded.recordText, error)) {
            return false;
        }
        result = std::move(loaded);
        return true;
    }
    return true;
}

bool HashPart(BCRYPT_HASH_HANDLE handle, std::string_view part) {
    if (part.size() > std::numeric_limits<ULONG>::max()) return false;
    return BCryptHashData(
        handle,
        reinterpret_cast<PUCHAR>(const_cast<char*>(part.data())),
        static_cast<ULONG>(part.size()),
        0) >= 0;
}

std::optional<std::string> CombinedTableHash(const LoadedTables& tables) {
    BCRYPT_ALG_HANDLE algorithm{};
    BCRYPT_HASH_HANDLE hash{};
    DWORD objectBytes{};
    DWORD hashBytes{};
    DWORD returned{};
    if (BCryptOpenAlgorithmProvider(
            &algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0
            || BCryptGetProperty(
                algorithm, BCRYPT_OBJECT_LENGTH,
                reinterpret_cast<PUCHAR>(&objectBytes), sizeof(objectBytes),
                &returned, 0) < 0
            || BCryptGetProperty(
                algorithm, BCRYPT_HASH_LENGTH,
                reinterpret_cast<PUCHAR>(&hashBytes), sizeof(hashBytes),
                &returned, 0) < 0) {
        if (algorithm) BCryptCloseAlgorithmProvider(algorithm, 0);
        return std::nullopt;
    }
    std::vector<std::uint8_t> object(objectBytes);
    std::vector<std::uint8_t> digest(hashBytes);
    if (BCryptCreateHash(
            algorithm, &hash, object.data(), objectBytes,
            nullptr, 0, 0) < 0) {
        BCryptCloseAlgorithmProvider(algorithm, 0);
        return std::nullopt;
    }
    constexpr char Separator[] = "\0RuffnecKk-PlayerSequenceTables\0";
    const bool hashed = HashPart(hash, "playerseqmap.txt")
        && HashPart(hash, std::string_view(Separator, sizeof(Separator) - 1))
        && HashPart(hash, tables.routeText)
        && HashPart(hash, std::string_view(Separator, sizeof(Separator) - 1))
        && HashPart(hash, "playerseq.txt")
        && HashPart(hash, std::string_view(Separator, sizeof(Separator) - 1))
        && HashPart(hash, tables.recordText)
        && BCryptFinishHash(hash, digest.data(), hashBytes, 0) >= 0;
    BCryptDestroyHash(hash);
    BCryptCloseAlgorithmProvider(algorithm, 0);
    if (!hashed) return std::nullopt;

    constexpr char Hex[] = "0123456789ABCDEF";
    std::string encoded(digest.size() * 2, '0');
    for (std::size_t index = 0; index < digest.size(); ++index) {
        encoded[index * 2] = Hex[digest[index] >> 4];
        encoded[index * 2 + 1] = Hex[digest[index] & 0x0F];
    }
    return encoded;
}

constexpr std::size_t AlignUp(std::size_t value, std::size_t alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

bool BuildArena(
        const ParsedTables& tables,
        ArenaResult& result,
        std::string& error) {
    const auto descriptorBytes = sizeof(PlayerSequenceDescriptor) * RouteCount;
    std::size_t recordCount{};
    for (const auto& recordSet : tables.recordSets) {
        recordCount += recordSet.records.size();
    }
    const auto recordOffset = AlignUp(descriptorBytes, alignof(PlayerSequenceRecord));
    const auto totalBytes = recordOffset + recordCount * sizeof(PlayerSequenceRecord);
    void* allocation = VirtualAlloc(
        nullptr, totalBytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (!allocation) {
        error = "cannot allocate the process-lifetime sequence arena";
        return false;
    }

    auto* descriptors = static_cast<PlayerSequenceDescriptor*>(allocation);
    auto* recordCursor = reinterpret_cast<PlayerSequenceRecord*>(
        static_cast<std::uint8_t*>(allocation) + recordOffset);
    std::vector<const PlayerSequenceRecord*> recordPointers(tables.recordSets.size());
    for (std::size_t setIndex = 0; setIndex < tables.recordSets.size(); ++setIndex) {
        const auto& source = tables.recordSets[setIndex].records;
        recordPointers[setIndex] = recordCursor;
        for (const auto& record : source) {
            *recordCursor++ = PlayerSequenceRecord{
                0, record.mode, record.frame, record.direction, record.event};
        }
    }

    for (std::size_t route = 0; route < RouteCount; ++route) {
        auto& descriptor = descriptors[route];
        descriptor = {};
        descriptor.extra = DescriptorExtra;
        const auto setIndex = tables.recordSetByRoute[route];
        if (setIndex < 0) continue;
        const auto count = tables.recordSets[static_cast<std::size_t>(setIndex)].records.size();
        descriptor.records = recordPointers[static_cast<std::size_t>(setIndex)];
        descriptor.sequenceFrameCount = static_cast<std::int32_t>(count);
        descriptor.animationFrameCount = static_cast<std::int32_t>(count);
    }

    DWORD oldProtection{};
    if (!VirtualProtect(allocation, totalBytes, PAGE_READONLY, &oldProtection)) {
        VirtualFree(allocation, 0, MEM_RELEASE);
        error = "cannot protect the sequence arena as read-only";
        return false;
    }
    ArenaResult built{};
    built.allocation = allocation;
    built.bytes = totalBytes;
    for (std::size_t sequence = 0; sequence < SequenceCount; ++sequence) {
        built.groupPointers[sequence] = reinterpret_cast<std::uintptr_t>(
            descriptors + sequence * WeaponClassCount);
    }
    result = built;
    return true;
}

bool InstallGroupPointers(const ArenaResult& arena) {
    std::array<std::uintptr_t, SequenceCount> expected{};
    for (std::size_t index = 0; index < SequenceCount; ++index) {
        expected[index] = reinterpret_cast<std::uintptr_t>(Base)
            + OriginalGroupRvas[index];
    }
    return Context->PatchBytes(
        FirstActivePlayerSequencePointerRva,
        expected.data(),
        static_cast<std::uint32_t>(sizeof(expected)),
        arena.groupPointers.data(),
        static_cast<std::uint32_t>(sizeof(arena.groupPointers)));
}

auto Status(
        D2R::Game::Client*,
        const D2RL::ConsoleCommandContext* command,
        void*) noexcept -> D2RL::ConsoleCommandResult {
    if (!command || !command->plugin) {
        return D2RL::ConsoleCommandResult::Failed;
    }
    const auto state = State.load(std::memory_order_acquire);
    const char* stateName = state == OperationalState::Active
        ? "active"
        : state == OperationalState::Disabled ? "disabled" : "vanilla";
    char message[1024]{};
    std::snprintf(
        message,
        sizeof(message),
        "Player Sequence Tables 0.1.0: %s; routes=%zu; recordsets=%zu; records=%zu; arena=%zu bytes; hash=%s; tables=%s; config=%s.",
        stateName,
        LoadedRoutes,
        LoadedRecordSets,
        LoadedRecords,
        ProcessLifetimeArenaBytes,
        LoadedTableHash.c_str(),
        LoadedTablePaths.c_str(),
        LoadedConfigPath.c_str());
    command->plugin->WriteConsoleMessage(message);
    return D2RL::ConsoleCommandResult::Handled;
}

void RegisterStatusCommand() {
    if (!Context->RegisterConsoleCommand(
            "player-sequences",
            Status,
            "Show Player Sequence Tables status and table hash.")) {
        Context->LogWarn(
            "PlayerSequenceTables: status command could not be registered.");
    }
}

void ResetState() {
    Settings = {};
    LoadedConfigPath = "built-in defaults";
    LoadedTablePaths = "none";
    LoadedTableHash = "none";
    LoadedRecordSets = 0;
    LoadedRecords = 0;
    LoadedRoutes = 0;
    ProcessLifetimeArena = nullptr;
    ProcessLifetimeArenaBytes = 0;
    State.store(OperationalState::Vanilla, std::memory_order_release);
}

} // namespace
} // namespace RuffnecKk::PlayerSequenceTables

namespace {

constexpr D2RL::PluginInfo Info{
    .infoSize = D2RL::PluginInfoSize,
    .apiVersion = D2RL_PLUGIN_API_VERSION,
    .id = "ruffneckk-player-sequence-tables",
    .name = "Player Sequence Tables",
    .version = "0.1.0",
    .author = "RuffnecKk",
    .description = "Loads editable player animation sequences from mod TXT tables.",
    .flags = D2RL::PluginFlags::Shared | D2RL::PluginFlags::NativeHooks,
};

} // namespace

D2RL_PLUGIN_EXPORT auto D2RLoaderGetPluginInfo() noexcept
        -> const D2RL::PluginInfo* {
    return &Info;
}

D2RL_PLUGIN_EXPORT auto D2RLoaderLoadPlugin(
        const D2RL::PluginContext* context) noexcept -> bool {
    using namespace RuffnecKk::PlayerSequenceTables;
    Context = context;
    Base = context ? reinterpret_cast<std::uint8_t*>(context->exeBase) : nullptr;
    ResetState();
    if (!Context || !Base) return false;

    const auto* runtimeBuild = D2RL::GetBuildName(context);
    if (!runtimeBuild || std::strcmp(runtimeBuild, "93847") != 0) {
        context->LogError(
            "PlayerSequenceTables: only D2R 3.3 build 93847 is supported.");
        return false;
    }
    if (!LoadConfig()) return false;
    if (!Settings.enabled) {
        State.store(OperationalState::Disabled, std::memory_order_release);
        RegisterStatusCommand();
        const auto message = std::string(
            "Player Sequence Tables 0.1.0 by RuffnecKk loaded disabled; config=")
            + LoadedConfigPath + ".";
        context->LogInfo(message.c_str());
        return true;
    }

    std::optional<LoadedTables> loaded;
    std::string error;
    if (!FindAndLoadTables(loaded, error)) {
        const auto message = "PlayerSequenceTables: " + error + ".";
        context->LogError(message.c_str());
        return false;
    }
    if (!loaded) {
        State.store(OperationalState::Vanilla, std::memory_order_release);
        RegisterStatusCommand();
        context->LogInfo(
            "Player Sequence Tables 0.1.0 by RuffnecKk: both TXT tables are absent; vanilla player sequences remain unchanged.");
        return true;
    }

    ParsedTables parsed{};
    if (!ParsePlayerSequenceTables(
            loaded->routeText, loaded->recordText, parsed, error)) {
        const auto message = "PlayerSequenceTables: invalid tables: " + error + ".";
        context->LogError(message.c_str());
        return false;
    }
    const auto tableHash = CombinedTableHash(*loaded);
    if (!tableHash) {
        context->LogError(
            "PlayerSequenceTables: cannot calculate the combined SHA-256 table hash.");
        return false;
    }

    ArenaResult arena{};
    if (!BuildArena(parsed, arena, error)) {
        const auto message = "PlayerSequenceTables: " + error + ".";
        context->LogError(message.c_str());
        return false;
    }
    if (!InstallGroupPointers(arena)) {
        VirtualFree(arena.allocation, 0, MEM_RELEASE);
        context->LogError(
            "PlayerSequenceTables: the 25 player-sequence group pointers are already owned or do not match D2R 3.3.93847.");
        return false;
    }

    ProcessLifetimeArena = arena.allocation;
    ProcessLifetimeArenaBytes = arena.bytes;
    LoadedRecordSets = parsed.recordSets.size();
    LoadedRoutes = parsed.availableRoutes;
    for (const auto& set : parsed.recordSets) LoadedRecords += set.records.size();
    LoadedTableHash = *tableHash;
    LoadedTablePaths = DisplayPath(loaded->routePath) + " + "
        + DisplayPath(loaded->recordPath);
    State.store(OperationalState::Active, std::memory_order_release);
    RegisterStatusCommand();

    const auto message = std::string(
        "Player Sequence Tables 0.1.0 by RuffnecKk active; routes=")
        + std::to_string(LoadedRoutes)
        + "; recordsets=" + std::to_string(LoadedRecordSets)
        + "; records=" + std::to_string(LoadedRecords)
        + "; hash=" + LoadedTableHash
        + "; restart required after TXT edits.";
    context->LogInfo(message.c_str());
    if (Settings.diagnostics) {
        const auto diagnostic = "PlayerSequenceTables diagnostic: tables="
            + LoadedTablePaths + "; arena="
            + std::to_string(ProcessLifetimeArenaBytes) + " bytes.";
        context->LogInfo(diagnostic.c_str());
    }
    return true;
}

D2RL_PLUGIN_EXPORT void D2RLoaderUnloadPlugin() noexcept {
    using namespace RuffnecKk::PlayerSequenceTables;
    State.store(OperationalState::Vanilla, std::memory_order_release);
    // Deliberately retain ProcessLifetimeArena. D2R caches record pointers in
    // live units, so freeing it before process exit could create dangling reads.
}
