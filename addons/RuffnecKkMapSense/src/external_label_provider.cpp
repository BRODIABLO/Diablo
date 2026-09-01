#include "external_label_provider.hpp"

#include "external_atlas_cache.hpp"
#include "native_automap_poi.hpp"

#include <D2RLPlugin/api.h>

#include <Windows.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <limits>
#include <iterator>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

extern "C" IMAGE_DOS_HEADER __ImageBase;

namespace RuffnecKk::MapSense {
namespace {

constexpr wchar_t HelperFileName[] = L"RuffnecKkMapSenseMapgen.exe";
constexpr std::uint32_t ProtocolVersion = 1U;
constexpr std::size_t MaximumResponseBytes = 2U * 1'024U * 1'024U;
constexpr std::size_t MaximumAtlasLevels = ExternalAtlasLevelCapacity;
constexpr std::size_t MaximumAtlasExits = ExternalAtlasEdgeCapacity;
constexpr std::size_t MaximumAtlasWaypoints = 512U;
constexpr std::size_t MaximumWitnessRooms = 4'096U;
constexpr std::uint32_t HelperTimeoutMilliseconds = 5'000U;

constexpr std::size_t LevelFirstRoomOffset = 0x10;
constexpr std::size_t LevelPositionXOffset = 0x24;
constexpr std::size_t LevelPositionYOffset = 0x28;
constexpr std::size_t DrlgRoomNextOffset = 0x48;
constexpr std::size_t DrlgRoomTileXOffset = 0x60;
constexpr std::size_t DrlgRoomTileYOffset = 0x64;
constexpr std::size_t DrlgRoomWidthOffset = 0x68;
constexpr std::size_t DrlgRoomHeightOffset = 0x6C;
constexpr std::int32_t SubtilesPerGameTile = 5;
constexpr std::uint32_t ExternalExitStableType = 5U;
constexpr std::uint32_t ExternalWaypointStableType = 2U;
constexpr std::uint32_t ExternalLevelStableType = 6U;

struct RoomRectangle final {
    std::int32_t x{};
    std::int32_t y{};
    std::int32_t width{};
    std::int32_t height{};

    [[nodiscard]] auto operator<=>(const RoomRectangle&) const = default;
};

struct AtlasLevel final {
    std::int32_t levelId{};
    std::int32_t originSubtileX{};
    std::int32_t originSubtileY{};
    std::int32_t anchorSubtileX{};
    std::int32_t anchorSubtileY{};
    std::size_t roomCount{};
};

struct AtlasExit final {
    std::int32_t sourceLevelId{};
    std::int32_t targetLevelId{};
    std::int32_t subtileX{};
    std::int32_t subtileY{};
    std::int32_t kind{};
};

struct AtlasWaypoint final {
    std::int32_t levelId{};
    std::int32_t subtileX{};
    std::int32_t subtileY{};
    std::int32_t classId{};
};

struct ParsedAtlas final {
    std::vector<AtlasLevel> levels;
    std::vector<AtlasExit> exits;
    std::vector<AtlasWaypoint> waypoints;
    std::vector<RoomRectangle> witnessRooms;
    double helperElapsedMilliseconds{};
};

struct Request final {
    std::uint64_t serial{};
    std::uint64_t sessionGeneration{};
    std::uint32_t seed{};
    std::uint8_t difficulty{};
    std::int32_t act{};
    std::int32_t currentLevelId{};
    std::int32_t currentOriginSubtileX{};
    std::int32_t currentOriginSubtileY{};
    std::vector<RoomRectangle> currentRooms;
};

struct RequestKey final {
    std::uint64_t sessionGeneration{};
    std::uint32_t seed{};
    std::uint8_t difficulty{};
    std::int32_t act{};
    std::int32_t currentLevelId{};

    [[nodiscard]] auto operator==(const RequestKey&) const -> bool = default;
};

struct NativeLevelWitness final {
    std::int32_t originSubtileX{};
    std::int32_t originSubtileY{};
    std::array<RoomRectangle, MaximumWitnessRooms> rooms{};
    std::size_t roomCount{};
};

class UniqueHandle final {
public:
    UniqueHandle() noexcept = default;
    explicit UniqueHandle(HANDLE value) noexcept : value_(value) {}
    ~UniqueHandle() {
        Reset();
    }

    UniqueHandle(const UniqueHandle&) = delete;
    auto operator=(const UniqueHandle&) -> UniqueHandle& = delete;

    UniqueHandle(UniqueHandle&& other) noexcept
        : value_(std::exchange(other.value_, nullptr)) {}
    auto operator=(UniqueHandle&& other) noexcept -> UniqueHandle& {
        if (this != &other) {
            Reset(std::exchange(other.value_, nullptr));
        }
        return *this;
    }

    [[nodiscard]] auto Get() const noexcept -> HANDLE { return value_; }
    [[nodiscard]] explicit operator bool() const noexcept {
        return value_ != nullptr && value_ != INVALID_HANDLE_VALUE;
    }
    [[nodiscard]] auto Release() noexcept -> HANDLE {
        return std::exchange(value_, nullptr);
    }
    void Reset(HANDLE value = nullptr) noexcept {
        if (*this) CloseHandle(value_);
        value_ = value;
    }

private:
    HANDLE value_{};
};

const D2RL::PluginContext* Context{};
std::atomic_bool Active{};
std::atomic_bool Diagnostics{};
std::mutex StateMutex;
std::condition_variable StateCondition;
std::thread Worker;
bool StopRequested{};
std::optional<Request> PendingRequest;
std::optional<RequestKey> InFlightRequest;
std::optional<RequestKey> PublishedRequest;
std::uint64_t LatestSerial{};
std::uint64_t ActiveSessionGeneration{};
HANDLE ActiveChildProcess{};
std::filesystem::path HelperPath;
std::filesystem::path GeometryCacheRoot;
ExternalLabelAtlasResultCallback ResultCallback{};
void* ResultUserData{};
std::atomic<std::shared_ptr<const ExternalAtlasGeometrySnapshot>>
    PublishedGeometrySnapshot{};

std::atomic_uint64_t Requests{};
std::atomic_uint64_t ProcessesStarted{};
std::atomic_uint64_t AtlasesPublished{};
std::atomic_uint64_t StaleResponses{};
std::atomic_uint64_t FailedResponses{};
std::atomic_uint64_t Timeouts{};
std::atomic_uint64_t GeometryCacheHits{};
std::atomic_uint64_t GeometryAtlasesGenerated{};
std::atomic_uint64_t GeometryFailures{};
std::atomic_uint64_t GeometrySnapshotsPublished{};
std::atomic_uint64_t PublishedGeometryCells{};

[[nodiscard]] auto KeyFor(const Request& request) noexcept -> RequestKey {
    return {
        .sessionGeneration = request.sessionGeneration,
        .seed = request.seed,
        .difficulty = request.difficulty,
        .act = request.act,
        .currentLevelId = request.currentLevelId,
    };
}

[[nodiscard]] auto StableId(
        std::uint32_t type,
        std::int32_t id,
        std::int32_t x,
        std::int32_t y) noexcept -> std::uint64_t {
    auto hash = UINT64_C(1469598103934665603);
    const std::array values{
        type,
        static_cast<std::uint32_t>(id),
        static_cast<std::uint32_t>(x),
        static_cast<std::uint32_t>(y),
    };
    for (const auto value : values) {
        for (std::uint32_t shift = 0U; shift < 32U; shift += 8U) {
            hash ^= static_cast<std::uint8_t>(value >> shift);
            hash *= UINT64_C(1099511628211);
        }
    }
    return hash;
}

[[nodiscard]] auto ResolveHelperPath(
        std::filesystem::path& output) noexcept -> bool {
    output.clear();
    std::array<wchar_t, 32'768U> modulePath{};
    const auto length = GetModuleFileNameW(
        reinterpret_cast<HMODULE>(&__ImageBase),
        modulePath.data(),
        static_cast<DWORD>(modulePath.size()));
    if (length == 0U
        || length >= static_cast<DWORD>(modulePath.size())) {
        return false;
    }
    try {
        output = std::filesystem::path(
            std::wstring_view(modulePath.data(), length)).parent_path()
            / HelperFileName;
    } catch (...) {
        output.clear();
        return false;
    }
    return GetFileAttributesW(output.c_str()) != INVALID_FILE_ATTRIBUTES;
}

[[nodiscard]] auto CaptureNativeLevelWitness(
        const void* levelPointer,
        NativeLevelWitness& output) noexcept -> bool {
    output = {};
    __try {
        const auto* const level = static_cast<const std::uint8_t*>(
            levelPointer);
        output.originSubtileX =
            *reinterpret_cast<const std::int32_t*>(
                level + LevelPositionXOffset) * SubtilesPerGameTile;
        output.originSubtileY =
            *reinterpret_cast<const std::int32_t*>(
                level + LevelPositionYOffset) * SubtilesPerGameTile;

        auto* room = *reinterpret_cast<std::uint8_t* const*>(
            level + LevelFirstRoomOffset);
        while (room != nullptr && output.roomCount < output.rooms.size()) {
            const RoomRectangle rectangle{
                .x = *reinterpret_cast<const std::int32_t*>(
                    room + DrlgRoomTileXOffset),
                .y = *reinterpret_cast<const std::int32_t*>(
                    room + DrlgRoomTileYOffset),
                .width = *reinterpret_cast<const std::int32_t*>(
                    room + DrlgRoomWidthOffset),
                .height = *reinterpret_cast<const std::int32_t*>(
                    room + DrlgRoomHeightOffset),
            };
            if (rectangle.x < 0 || rectangle.y < 0
                || rectangle.width <= 0 || rectangle.height <= 0) {
                return false;
            }
            output.rooms[output.roomCount++] = rectangle;
            room = *reinterpret_cast<std::uint8_t* const*>(
                room + DrlgRoomNextOffset);
        }
        return room == nullptr && output.roomCount != 0U;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        output = {};
        return false;
    }
}

[[nodiscard]] auto CaptureRequest(
        std::uint64_t sessionGeneration,
        const ClientLevelView& current,
        Request& output) noexcept -> bool {
    output = {};
    const auto act = RevealActForLevelId(current.levelId);
    if (sessionGeneration == 0U || current.level == nullptr
        || current.drlg == nullptr || current.levelId <= 0
        || current.difficulty > 2U || act < 0
        || DeriveDrlgStartSeed(current.mapSeed) != current.drlgStartSeed) {
        return false;
    }
    NativeLevelWitness witness{};
    if (!CaptureNativeLevelWitness(current.level, witness)) return false;
    output.sessionGeneration = sessionGeneration;
    output.seed = current.mapSeed;
    output.difficulty = current.difficulty;
    output.act = act;
    output.currentLevelId = current.levelId;
    output.currentOriginSubtileX = witness.originSubtileX;
    output.currentOriginSubtileY = witness.originSubtileY;
    try {
        output.currentRooms.assign(
            witness.rooms.begin(),
            witness.rooms.begin()
                + static_cast<std::ptrdiff_t>(witness.roomCount));
    } catch (...) {
        output = {};
        return false;
    }
    std::sort(output.currentRooms.begin(), output.currentRooms.end());
    return true;
}

[[nodiscard]] auto ReadAvailablePipe(
        HANDLE pipe,
        std::string& output) noexcept -> bool {
    for (;;) {
        DWORD available{};
        if (!PeekNamedPipe(pipe, nullptr, 0U, nullptr, &available, nullptr)) {
            const auto error = GetLastError();
            return error == ERROR_BROKEN_PIPE;
        }
        if (available == 0U) return true;
        std::array<char, 8'192U> buffer{};
        const auto requested = static_cast<DWORD>(std::min<std::size_t>(
            buffer.size(), available));
        DWORD read{};
        if (!ReadFile(pipe, buffer.data(), requested, &read, nullptr)) {
            return GetLastError() == ERROR_BROKEN_PIPE;
        }
        if (read == 0U) return true;
        if (output.size() > MaximumResponseBytes - read) return false;
        output.append(buffer.data(), read);
    }
}

[[nodiscard]] auto RunHelperCommand(
        std::wstring_view arguments,
        std::string& output,
        bool& timedOut,
        const std::filesystem::path* workingDirectoryOverride = nullptr) noexcept
        -> bool {
    output.clear();
    timedOut = false;
    SECURITY_ATTRIBUTES security{
        .nLength = sizeof(SECURITY_ATTRIBUTES),
        .lpSecurityDescriptor = nullptr,
        .bInheritHandle = TRUE,
    };
    HANDLE readRaw{};
    HANDLE writeRaw{};
    if (!CreatePipe(&readRaw, &writeRaw, &security, 0U)) return false;
    UniqueHandle readPipe(readRaw);
    UniqueHandle writePipe(writeRaw);
    if (!SetHandleInformation(
            readPipe.Get(), HANDLE_FLAG_INHERIT, 0U)) {
        return false;
    }

    std::wstring command;
    std::wstring workingDirectory;
    try {
        command = L"\"" + HelperPath.wstring() + L"\" "
            + std::wstring(arguments);
        workingDirectory = workingDirectoryOverride != nullptr
            ? workingDirectoryOverride->wstring()
            : HelperPath.parent_path().wstring();
    } catch (...) {
        return false;
    }
    std::vector<wchar_t> mutableCommand(command.begin(), command.end());
    mutableCommand.push_back(L'\0');

    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    startup.hStdOutput = writePipe.Get();
    startup.hStdError = writePipe.Get();
    PROCESS_INFORMATION processInfo{};
    if (!CreateProcessW(
            HelperPath.c_str(),
            mutableCommand.data(),
            nullptr,
            nullptr,
            TRUE,
            CREATE_NO_WINDOW | BELOW_NORMAL_PRIORITY_CLASS,
            nullptr,
            workingDirectory.c_str(),
            &startup,
            &processInfo)) {
        return false;
    }
    UniqueHandle process(processInfo.hProcess);
    UniqueHandle processThread(processInfo.hThread);
    writePipe.Reset();
    ProcessesStarted.fetch_add(1U, std::memory_order_relaxed);
    {
        std::scoped_lock lock(StateMutex);
        ActiveChildProcess = process.Get();
        if (StopRequested) TerminateProcess(process.Get(), ERROR_CANCELLED);
    }

    const auto started = GetTickCount64();
    bool success = true;
    for (;;) {
        if (!ReadAvailablePipe(readPipe.Get(), output)) {
            success = false;
            break;
        }
        const auto wait = WaitForSingleObject(process.Get(), 5U);
        if (wait == WAIT_OBJECT_0) {
            success = ReadAvailablePipe(readPipe.Get(), output);
            break;
        }
        if (wait == WAIT_FAILED) {
            success = false;
            break;
        }
        if (GetTickCount64() - started > HelperTimeoutMilliseconds) {
            timedOut = true;
            TerminateProcess(process.Get(), ERROR_TIMEOUT);
            WaitForSingleObject(process.Get(), 1'000U);
            success = false;
            break;
        }
    }
    {
        std::scoped_lock lock(StateMutex);
        if (ActiveChildProcess == process.Get()) ActiveChildProcess = nullptr;
    }
    DWORD exitCode{};
    if (!GetExitCodeProcess(process.Get(), &exitCode)
        || exitCode != 0U) {
        success = false;
    }
    return success;
}

[[nodiscard]] auto RunLabelHelper(
        const Request& request,
        std::string& output,
        bool& timedOut) noexcept -> bool {
    try {
        const auto arguments = L"labels "
            + std::to_wstring(request.seed) + L" "
            + std::to_wstring(request.difficulty) + L" "
            + std::to_wstring(request.act) + L" "
            + std::to_wstring(request.currentLevelId);
        return RunHelperCommand(arguments, output, timedOut);
    } catch (...) {
        output.clear();
        timedOut = false;
        return false;
    }
}

[[nodiscard]] auto ReadGeometryArtifact(
        const std::filesystem::path& path,
        std::vector<std::uint8_t>& output) -> bool {
    output.clear();
    std::error_code error;
    const auto size = std::filesystem::file_size(path, error);
    if (error || size < ExternalAtlasGeometryHeaderBytes
        || size > ExternalAtlasGeometryMaximumBytes
        || size > static_cast<std::uintmax_t>(
            (std::numeric_limits<std::size_t>::max)())) {
        return false;
    }
    output.resize(static_cast<std::size_t>(size));
    std::ifstream input(path, std::ios::binary);
    if (!input) return false;
    input.read(
        reinterpret_cast<char*>(output.data()),
        static_cast<std::streamsize>(output.size()));
    return input && input.peek() == std::char_traits<char>::eof();
}

[[nodiscard]] auto PrepareGeometryCacheAct(
        const Request& request,
        std::uint8_t act,
        ExternalAtlasGeometry* output = nullptr) noexcept -> bool {
    if (GeometryCacheRoot.empty() || act >= 5U) return false;
    const ExternalAtlasCacheKey key{
        .seed = request.seed,
        .difficulty = request.difficulty,
        .act = act,
    };
    ExternalAtlasGeometry cached;
    const auto cacheResult = LoadExternalAtlasGeometryCache(
        GeometryCacheRoot,
        key,
        cached);
    if (cacheResult == ExternalAtlasCacheResult::Hit) {
        GeometryCacheHits.fetch_add(1U, std::memory_order_relaxed);
        if (output != nullptr) *output = std::move(cached);
        return true;
    }

    std::filesystem::path target;
    if (!BuildExternalAtlasCachePath(GeometryCacheRoot, key, target)) {
        GeometryFailures.fetch_add(1U, std::memory_order_relaxed);
        return false;
    }
    std::error_code error;
    std::filesystem::create_directories(target.parent_path(), error);
    if (error) {
        GeometryFailures.fetch_add(1U, std::memory_order_relaxed);
        return false;
    }
    auto temporary = target;
    try {
        temporary += L".generated-" + std::to_wstring(GetCurrentProcessId())
            + L"-" + std::to_wstring(GetTickCount64());
        const auto arguments = L"geometry-binary "
            + std::to_wstring(request.seed) + L" "
            + std::to_wstring(request.difficulty) + L" "
            + std::to_wstring(act) + L" \""
            + temporary.filename().wstring() + L"\"";
        std::string diagnostics;
        bool timedOut{};
        const auto generationDirectory = temporary.parent_path();
        const auto launched = RunHelperCommand(
            arguments,
            diagnostics,
            timedOut,
            &generationDirectory);
        if (timedOut) Timeouts.fetch_add(1U, std::memory_order_relaxed);
        std::vector<std::uint8_t> bytes;
        ExternalAtlasGeometry generated;
        const auto valid = launched
            && ReadGeometryArtifact(temporary, bytes)
            && ParseExternalAtlasGeometry(
                bytes,
                key.seed,
                key.difficulty,
                key.act,
                generated)
            && StoreExternalAtlasGeometryCache(
                GeometryCacheRoot,
                key,
                bytes);
        std::filesystem::remove(temporary, error);
        if (!valid) {
            GeometryFailures.fetch_add(1U, std::memory_order_relaxed);
            if (Diagnostics.load(std::memory_order_acquire)
                && Context != nullptr) {
                char message[384]{};
                std::snprintf(
                    message,
                    sizeof(message),
                    "MapSense external atlas geometry: FAIL seed=%u difficulty=%u act=%u launched=%u timed-out=%u artifact-bytes=%zu helper='%.*s'.",
                    request.seed,
                    static_cast<unsigned>(request.difficulty),
                    static_cast<unsigned>(act),
                    static_cast<unsigned>(launched),
                    static_cast<unsigned>(timedOut),
                    bytes.size(),
                    static_cast<int>(std::min<std::size_t>(
                        diagnostics.size(), 160U)),
                    diagnostics.c_str());
                Context->LogWarn(message);
            }
            return false;
        }
        GeometryAtlasesGenerated.fetch_add(1U, std::memory_order_relaxed);
        if (output != nullptr) *output = std::move(generated);
        if (Diagnostics.load(std::memory_order_acquire)
            && Context != nullptr) {
            char message[256]{};
            std::snprintf(
                message,
                sizeof(message),
                "MapSense external atlas geometry: PASS seed=%u difficulty=%u act=%u bytes=%zu source=generated-cache.",
                request.seed,
                static_cast<unsigned>(request.difficulty),
                static_cast<unsigned>(act),
                bytes.size());
            Context->LogInfo(message);
        }
        return true;
    } catch (...) {
        std::filesystem::remove(temporary, error);
        GeometryFailures.fetch_add(1U, std::memory_order_relaxed);
        return false;
    }
}

void PrewarmGeometryCache(const Request& request) noexcept {
    const auto prepare = [&request](std::uint8_t act) noexcept {
        {
            std::scoped_lock lock(StateMutex);
            if (StopRequested || PendingRequest.has_value()) return false;
        }
        ExternalAtlasCacheKey key{
            .seed = request.seed,
            .difficulty = request.difficulty,
            .act = act,
        };
        std::filesystem::path path;
        if (BuildExternalAtlasCachePath(GeometryCacheRoot, key, path)) {
            std::error_code error;
            if (std::filesystem::is_regular_file(path, error) && !error) {
                return true;
            }
        }
        (void)PrepareGeometryCacheAct(request, act);
        return true;
    };
    for (std::uint8_t act = 0U; act < 5U; ++act) {
        if (act == static_cast<std::uint8_t>(request.act)) continue;
        if (!prepare(act)) return;
    }
}

template <typename... Values>
[[nodiscard]] auto ReadExactRecord(
        std::istringstream& stream,
        Values&... values) -> bool {
    if (!((stream >> values) && ...)) return false;
    std::string extra;
    return !(stream >> extra);
}

[[nodiscard]] auto ParseAtlas(
        const Request& request,
        const std::string& response,
        ParsedAtlas& output) -> bool {
    output = {};
    std::istringstream responseStream(response);
    std::string line;
    bool sawHeader{};
    bool sawEnd{};
    std::size_t declaredLevels{};
    std::size_t declaredExits{};
    std::size_t declaredWaypoints{};
    std::size_t declaredRooms{};
    while (std::getline(responseStream, line)) {
        std::istringstream stream(line);
        std::string prefix;
        char type{};
        if (!(stream >> prefix >> type) || prefix != "MS1") continue;
        if (sawEnd) return false;
        if (type == 'H') {
            std::uint32_t version{};
            std::uint32_t seed{};
            std::uint32_t difficulty{};
            std::int32_t act{};
            std::int32_t currentLevel{};
            if (sawHeader || !ReadExactRecord(
                    stream,
                    version,
                    seed,
                    difficulty,
                    act,
                    currentLevel)
                || version != ProtocolVersion || seed != request.seed
                || difficulty != request.difficulty || act != request.act
                || currentLevel != request.currentLevelId) {
                return false;
            }
            sawHeader = true;
        } else if (type == 'L') {
            AtlasLevel level{};
            if (!sawHeader || !ReadExactRecord(
                    stream,
                    level.levelId,
                    level.originSubtileX,
                    level.originSubtileY,
                    level.anchorSubtileX,
                    level.anchorSubtileY,
                    level.roomCount)
                || output.levels.size() >= MaximumAtlasLevels
                || RevealActForLevelId(level.levelId) != request.act
                || level.originSubtileX < 0 || level.originSubtileY < 0
                || level.anchorSubtileX < 0 || level.anchorSubtileY < 0
                || level.roomCount > MaximumWitnessRooms) {
                return false;
            }
            output.levels.push_back(level);
        } else if (type == 'R') {
            std::int32_t levelId{};
            RoomRectangle room{};
            if (!sawHeader || !ReadExactRecord(
                    stream,
                    levelId,
                    room.x,
                    room.y,
                    room.width,
                    room.height)
                || levelId != request.currentLevelId
                || output.witnessRooms.size() >= MaximumWitnessRooms
                || room.x < 0 || room.y < 0
                || room.width <= 0 || room.height <= 0) {
                return false;
            }
            output.witnessRooms.push_back(room);
        } else if (type == 'E') {
            AtlasExit exit{};
            if (!sawHeader || !ReadExactRecord(
                    stream,
                    exit.sourceLevelId,
                    exit.targetLevelId,
                    exit.subtileX,
                    exit.subtileY,
                    exit.kind)
                || output.exits.size() >= MaximumAtlasExits
                || RevealActForLevelId(exit.sourceLevelId) != request.act
                || RevealActForLevelId(exit.targetLevelId) != request.act
                || exit.subtileX < 0 || exit.subtileY < 0
                || (exit.kind != 0 && exit.kind != 1)) {
                return false;
            }
            output.exits.push_back(exit);
        } else if (type == 'W') {
            AtlasWaypoint waypoint{};
            if (!sawHeader || !ReadExactRecord(
                    stream,
                    waypoint.levelId,
                    waypoint.subtileX,
                    waypoint.subtileY,
                    waypoint.classId)
                || output.waypoints.size() >= MaximumAtlasWaypoints
                || RevealActForLevelId(waypoint.levelId) != request.act
                || waypoint.subtileX < 0 || waypoint.subtileY < 0
                || waypoint.classId < 0) {
                return false;
            }
            output.waypoints.push_back(waypoint);
        } else if (type == 'Z') {
            if (!sawHeader || !ReadExactRecord(
                    stream,
                    declaredLevels,
                    declaredExits,
                    declaredWaypoints,
                    declaredRooms,
                    output.helperElapsedMilliseconds)) {
                return false;
            }
            sawEnd = true;
        } else {
            return false;
        }
    }
    if (!sawHeader || !sawEnd
        || declaredLevels != output.levels.size()
        || declaredExits != output.exits.size()
        || declaredWaypoints != output.waypoints.size()
        || declaredRooms != output.witnessRooms.size()) {
        return false;
    }
    std::sort(output.witnessRooms.begin(), output.witnessRooms.end());
    return true;
}

[[nodiscard]] auto ValidateCurrentLevelWitness(
        const Request& request,
        const ParsedAtlas& atlas) noexcept -> bool {
    const auto level = std::find_if(
        atlas.levels.begin(),
        atlas.levels.end(),
        [&request](const AtlasLevel& candidate) noexcept {
            return candidate.levelId == request.currentLevelId;
        });
    return level != atlas.levels.end()
        && level->originSubtileX == request.currentOriginSubtileX
        && level->originSubtileY == request.currentOriginSubtileY
        && level->roomCount == request.currentRooms.size()
        && atlas.witnessRooms == request.currentRooms;
}

[[nodiscard]] auto VisibleLevels(
        const Request& request,
        const ParsedAtlas& atlas) -> std::vector<std::int32_t> {
    std::vector<ExternalAtlasTopologyEdge> edges;
    edges.reserve(atlas.exits.size());
    for (const auto& exit : atlas.exits) {
        edges.push_back({
            .sourceLevelId = exit.sourceLevelId,
            .targetLevelId = exit.targetLevelId,
            .kind = exit.kind,
        });
    }
    std::vector<std::int32_t> visible;
    if (!CollectExternalAtlasVisibleLevels(
            request.currentLevelId,
            edges.data(),
            edges.size(),
            visible)) {
        visible.clear();
    }
    return visible;
}

[[nodiscard]] auto ContainsLevel(
        const std::vector<std::int32_t>& levels,
        std::int32_t levelId) noexcept -> bool {
    return std::binary_search(levels.begin(), levels.end(), levelId);
}

[[nodiscard]] auto BuildExitDefinitions(
        const ParsedAtlas& atlas,
        const std::vector<std::int32_t>& connected,
        std::vector<AutomapExitLabelDefinition>& output) -> bool {
    output.clear();
    std::vector<AtlasExit> uniqueWarps;
    struct SeamGroup final {
        std::int32_t sourceLevelId{};
        std::int32_t targetLevelId{};
        std::vector<std::pair<std::int32_t, std::int32_t>> anchors;
    };
    std::vector<SeamGroup> seamGroups;
    for (const auto& exit : atlas.exits) {
        // A label is anchored in the source map space. Continuous seams stay
        // inside the visible component. Warp targets intentionally may live in
        // a disconnected dungeon/interior coordinate island.
        if (!ContainsLevel(connected, exit.sourceLevelId)
            || (exit.kind == 1
                && !ContainsLevel(connected, exit.targetLevelId))) {
            continue;
        }
        if (exit.kind == 1) {
            auto group = std::find_if(
                seamGroups.begin(),
                seamGroups.end(),
                [&exit](const SeamGroup& candidate) noexcept {
                    return candidate.sourceLevelId == exit.sourceLevelId
                        && candidate.targetLevelId == exit.targetLevelId;
                });
            if (group == seamGroups.end()) {
                seamGroups.push_back({
                    .sourceLevelId = exit.sourceLevelId,
                    .targetLevelId = exit.targetLevelId,
                });
                group = std::prev(seamGroups.end());
            }
            group->anchors.emplace_back(exit.subtileX, exit.subtileY);
            continue;
        }
        const auto duplicate = std::find_if(
            uniqueWarps.begin(),
            uniqueWarps.end(),
            [&exit](const AtlasExit& candidate) noexcept {
                return candidate.sourceLevelId == exit.sourceLevelId
                    && candidate.targetLevelId == exit.targetLevelId
                    && candidate.subtileX == exit.subtileX
                    && candidate.subtileY == exit.subtileY;
            });
        if (duplicate == uniqueWarps.end()) uniqueWarps.push_back(exit);
    }

    for (const auto& exit : uniqueWarps) {
        output.push_back({
            .stableId = StableId(
                ExternalExitStableType,
                exit.targetLevelId,
                exit.subtileX,
                exit.subtileY),
            .sourceLevelId = exit.sourceLevelId,
            .targetLevelId = exit.targetLevelId,
            .subtileX = exit.subtileX,
            .subtileY = exit.subtileY,
        });
    }
    for (auto& group : seamGroups) {
        std::sort(group.anchors.begin(), group.anchors.end());
        group.anchors.erase(
            std::unique(group.anchors.begin(), group.anchors.end()),
            group.anchors.end());
        if (group.anchors.empty()) continue;
        std::int64_t sumX{};
        std::int64_t sumY{};
        for (const auto& [x, y] : group.anchors) {
            sumX += x;
            sumY += y;
        }
        const auto x = static_cast<std::int32_t>(
            sumX / static_cast<std::int64_t>(group.anchors.size()));
        const auto y = static_cast<std::int32_t>(
            sumY / static_cast<std::int64_t>(group.anchors.size()));
        output.push_back({
            .stableId = StableId(
                ExternalExitStableType,
                group.targetLevelId,
                x,
                y),
            .sourceLevelId = group.sourceLevelId,
            .targetLevelId = group.targetLevelId,
            .subtileX = x,
            .subtileY = y,
            .canonicalLevelPairAnchor = true,
        });
    }
    return output.size() <= MaximumAutomapExitLabels;
}

[[nodiscard]] auto PublishAtlas(
        const Request& request,
        const ParsedAtlas& atlas,
        std::vector<std::int32_t>& connected,
        std::size_t& exitCount,
        std::size_t& waypointCount) -> bool {
    connected = VisibleLevels(request, atlas);
    if (!ContainsLevel(connected, request.currentLevelId)
        || connected.empty()) {
        return false;
    }

    std::vector<AutomapExitLabelDefinition> exits;
    if (!BuildExitDefinitions(atlas, connected, exits)) return false;
    std::vector<AutomapWaypointLabelDefinition> waypoints;
    for (const auto& waypoint : atlas.waypoints) {
        if (!ContainsLevel(connected, waypoint.levelId)) continue;
        if (std::find_if(
                waypoints.begin(),
                waypoints.end(),
                [&waypoint](const AutomapWaypointLabelDefinition& existing) {
                    return existing.levelId == waypoint.levelId;
                }) != waypoints.end()) {
            return false;
        }
        waypoints.push_back({
            .stableId = StableId(
                ExternalWaypointStableType,
                waypoint.levelId,
                waypoint.subtileX,
                waypoint.subtileY),
            .levelId = waypoint.levelId,
            .subtileX = waypoint.subtileX,
            .subtileY = waypoint.subtileY,
        });
    }
    if (waypoints.size() > MaximumAutomapWaypointLabels) return false;

    if (!ReplaceNativeAutomapExitLabels(
            request.sessionGeneration,
            exits.data(),
            exits.size())
        || !ReplaceNativeAutomapWaypointLabels(
            request.sessionGeneration,
            waypoints.data(),
            waypoints.size())) {
        return false;
    }
    for (const auto& level : atlas.levels) {
        if (!ContainsLevel(connected, level.levelId)) continue;
        const AutomapLevelLabelDefinition definition{
            .stableId = StableId(
                ExternalLevelStableType,
                level.levelId,
                level.anchorSubtileX,
                level.anchorSubtileY),
            .levelId = level.levelId,
            .subtileX = level.anchorSubtileX,
            .subtileY = level.anchorSubtileY,
        };
        if (!PublishNativeAutomapLevelLabels(
                request.sessionGeneration,
                level.levelId,
                &definition,
                1U)) {
            return false;
        }
    }
    exitCount = exits.size();
    waypointCount = waypoints.size();
    return true;
}

void LogFailure(const Request& request, const char* reason) noexcept {
    if (Context == nullptr) return;
    char message[320]{};
    std::snprintf(
        message,
        sizeof(message),
        "MapSense external labels: rejected session=%llu seed=%u difficulty=%u act=%d level=%d reason=%s.",
        static_cast<unsigned long long>(request.sessionGeneration),
        request.seed,
        static_cast<unsigned>(request.difficulty),
        request.act,
        request.currentLevelId,
        reason);
    Context->LogWarn(message);
}

void NotifyResult(const Request& request, bool published) noexcept {
    const auto callback = ResultCallback;
    if (callback == nullptr) return;
    callback(
        request.sessionGeneration,
        request.difficulty,
        request.act,
        request.currentLevelId,
        published,
        ResultUserData);
}

void WorkerMain() noexcept {
    // Geometry is speculative background work. Keep both this coordinator and
    // every child mapgen process below the game's normal-priority threads.
    (void)SetThreadPriority(
        GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
    for (;;) {
        Request request{};
        {
            std::unique_lock lock(StateMutex);
            StateCondition.wait(lock, [] {
                return StopRequested || PendingRequest.has_value();
            });
            if (StopRequested) return;
            request = std::move(*PendingRequest);
            PendingRequest.reset();
            InFlightRequest = KeyFor(request);
        }

        std::string response;
        bool timedOut{};
        const bool launched = RunLabelHelper(request, response, timedOut);
        if (timedOut) Timeouts.fetch_add(1U, std::memory_order_relaxed);
        ParsedAtlas atlas;
        bool valid = launched;
        try {
            valid = valid && ParseAtlas(request, response, atlas);
        } catch (...) {
            valid = false;
        }
        if (valid && !ValidateCurrentLevelWitness(request, atlas)) {
            valid = false;
            LogFailure(request, "current-level-coordinate-witness-mismatch");
        }

        {
            std::scoped_lock lock(StateMutex);
            InFlightRequest.reset();
            if (request.serial != LatestSerial
                || request.sessionGeneration != ActiveSessionGeneration) {
                StaleResponses.fetch_add(1U, std::memory_order_relaxed);
                continue;
            }
        }
        if (!valid) {
            FailedResponses.fetch_add(1U, std::memory_order_relaxed);
            if (launched) LogFailure(request, "malformed-or-incomplete-response");
            else LogFailure(request, timedOut ? "timeout" : "process-failure");
            NotifyResult(request, false);
            continue;
        }

        std::size_t connectedLevels{};
        std::vector<std::int32_t> visibleLevels;
        std::size_t publishedExits{};
        std::size_t publishedWaypoints{};
        bool published{};
        try {
            published = PublishAtlas(
                request,
                atlas,
                visibleLevels,
                publishedExits,
                publishedWaypoints);
        } catch (...) {
            published = false;
        }
        if (!published) {
            FailedResponses.fetch_add(1U, std::memory_order_relaxed);
            LogFailure(request, "publication-failure");
            NotifyResult(request, false);
            continue;
        }
        connectedLevels = visibleLevels.size();

        ExternalAtlasGeometry geometry;
        const auto geometryReady = PrepareGeometryCacheAct(
            request,
            static_cast<std::uint8_t>(request.act),
            &geometry);
        if (geometryReady) {
            const auto containsGeometryLevel = [&geometry](
                    std::int32_t levelId) noexcept {
                return std::find_if(
                    geometry.levels.begin(),
                    geometry.levels.end(),
                    [levelId](const ExternalAtlasGeometryLevel& level) {
                        return level.levelId == levelId;
                    }) != geometry.levels.end();
            };
            if (!std::all_of(
                    visibleLevels.begin(),
                    visibleLevels.end(),
                    containsGeometryLevel)) {
                GeometryFailures.fetch_add(1U, std::memory_order_relaxed);
                LogFailure(request, "visible-level-missing-from-geometry");
            } else {
                try {
                    auto immutableGeometry =
                        std::make_shared<const ExternalAtlasGeometry>(
                            std::move(geometry));
                    auto snapshot =
                        std::make_shared<const ExternalAtlasGeometrySnapshot>(
                            ExternalAtlasGeometrySnapshot{
                                .sessionGeneration =
                                    request.sessionGeneration,
                                .seed = request.seed,
                                .difficulty = request.difficulty,
                                .act = static_cast<std::uint8_t>(request.act),
                                .currentLevelId = request.currentLevelId,
                                .visibleLevelIds = std::move(visibleLevels),
                                .geometry = std::move(immutableGeometry),
                            });
                    {
                        std::scoped_lock lock(StateMutex);
                        if (request.serial != LatestSerial
                            || request.sessionGeneration
                                != ActiveSessionGeneration) {
                            StaleResponses.fetch_add(
                                1U, std::memory_order_relaxed);
                            continue;
                        }
                    }
                    const auto cellCount = snapshot->geometry->cells.size();
                    PublishedGeometrySnapshot.store(
                        std::move(snapshot),
                        std::memory_order_release);
                    GeometrySnapshotsPublished.fetch_add(
                        1U, std::memory_order_relaxed);
                    PublishedGeometryCells.store(
                        cellCount, std::memory_order_relaxed);
                } catch (...) {
                    GeometryFailures.fetch_add(
                        1U, std::memory_order_relaxed);
                }
            }
        }
        {
            std::scoped_lock lock(StateMutex);
            if (request.serial != LatestSerial
                || request.sessionGeneration != ActiveSessionGeneration) {
                StaleResponses.fetch_add(1U, std::memory_order_relaxed);
                continue;
            }
            PublishedRequest = KeyFor(request);
        }
        AtlasesPublished.fetch_add(1U, std::memory_order_relaxed);
        NotifyResult(request, true);
        if (Context != nullptr) {
            char message[384]{};
            std::snprintf(
                message,
                sizeof(message),
                "MapSense external labels: PASS session=%llu seed=%u difficulty=%u act=%d current-level=%d visible-levels=%zu exits=%zu waypoints=%zu room-witness=%zu helper-ms=%.3f.",
                static_cast<unsigned long long>(request.sessionGeneration),
                request.seed,
                static_cast<unsigned>(request.difficulty),
                request.act,
                request.currentLevelId,
                connectedLevels,
                publishedExits,
                publishedWaypoints,
                request.currentRooms.size(),
                atlas.helperElapsedMilliseconds);
            Context->LogInfo(message);
        }
        PrewarmGeometryCache(request);
    }
}

} // namespace

auto InitializeExternalLabelProvider(
        const D2RL::PluginContext* context,
        bool diagnostics,
        ExternalLabelAtlasResultCallback resultCallback,
        void* resultUserData) noexcept -> bool {
    if (!D2RL::HasContext(context)) return false;
    std::filesystem::path helper;
    if (!ResolveHelperPath(helper)) {
        context->LogWarn(
            "MapSense external labels: helper executable is unavailable; distant labels remain fail-closed.");
        return false;
    }
    std::filesystem::path geometryCacheRoot;
    if (!ResolveExternalAtlasCacheRoot(geometryCacheRoot)) {
        context->LogWarn(
            "MapSense external atlas: persistent cache root is unavailable; labels remain active but geometry generation is disabled.");
    }
    try {
        std::scoped_lock lock(StateMutex);
        if (Active.load(std::memory_order_acquire)) return true;
        Context = context;
        Diagnostics.store(diagnostics, std::memory_order_release);
        ResultCallback = resultCallback;
        ResultUserData = resultUserData;
        HelperPath = std::move(helper);
        GeometryCacheRoot = std::move(geometryCacheRoot);
        PublishedGeometrySnapshot.store(
            std::shared_ptr<const ExternalAtlasGeometrySnapshot>{},
            std::memory_order_release);
        StopRequested = false;
        PendingRequest.reset();
        InFlightRequest.reset();
        PublishedRequest.reset();
        LatestSerial = 0U;
        ActiveSessionGeneration = 0U;
        ActiveChildProcess = nullptr;
        Worker = std::thread(WorkerMain);
        Active.store(true, std::memory_order_release);
    } catch (...) {
        Context = nullptr;
        HelperPath.clear();
        GeometryCacheRoot.clear();
        return false;
    }
    context->LogInfo(
        "MapSense external labels: automatic seed-scoped helper is ready.");
    return true;
}

void ShutdownExternalLabelProvider() noexcept {
    Active.store(false, std::memory_order_release);
    PublishedGeometrySnapshot.store(
        std::shared_ptr<const ExternalAtlasGeometrySnapshot>{},
        std::memory_order_release);
    {
        std::scoped_lock lock(StateMutex);
        StopRequested = true;
        ++LatestSerial;
        PendingRequest.reset();
        if (ActiveChildProcess != nullptr) {
            TerminateProcess(ActiveChildProcess, ERROR_CANCELLED);
        }
    }
    StateCondition.notify_all();
    if (Worker.joinable()) Worker.join();
    {
        std::scoped_lock lock(StateMutex);
        StopRequested = false;
        InFlightRequest.reset();
        PublishedRequest.reset();
        ActiveSessionGeneration = 0U;
        ActiveChildProcess = nullptr;
        HelperPath.clear();
        GeometryCacheRoot.clear();
    }
    Context = nullptr;
    ResultCallback = nullptr;
    ResultUserData = nullptr;
}

void ResetExternalLabelProviderSession(
        std::uint64_t sessionGeneration) noexcept {
    PublishedGeometrySnapshot.store(
        std::shared_ptr<const ExternalAtlasGeometrySnapshot>{},
        std::memory_order_release);
    PublishedGeometryCells.store(0U, std::memory_order_relaxed);
    std::scoped_lock lock(StateMutex);
    ActiveSessionGeneration = sessionGeneration;
    ++LatestSerial;
    PendingRequest.reset();
    PublishedRequest.reset();
}

auto RequestExternalLabelAtlas(
        std::uint64_t sessionGeneration,
        const ClientLevelView& current) noexcept -> bool {
    if (!Active.load(std::memory_order_acquire)) return false;
    Request request;
    try {
        if (!CaptureRequest(sessionGeneration, current, request)) return false;
    } catch (...) {
        return false;
    }
    {
        std::scoped_lock lock(StateMutex);
        if (StopRequested
            || sessionGeneration != ActiveSessionGeneration) {
            return false;
        }
        const auto key = KeyFor(request);
        if ((PublishedRequest && *PublishedRequest == key)
            || (InFlightRequest && *InFlightRequest == key)
            || (PendingRequest && KeyFor(*PendingRequest) == key)) {
            return true;
        }
        request.serial = ++LatestSerial;
        PendingRequest = std::move(request);
    }
    Requests.fetch_add(1U, std::memory_order_relaxed);
    StateCondition.notify_one();
    return true;
}

auto IsExternalLabelProviderActive() noexcept -> bool {
    return Active.load(std::memory_order_acquire);
}

auto AcquireExternalAtlasGeometrySnapshot() noexcept
        -> std::shared_ptr<const ExternalAtlasGeometrySnapshot> {
    return PublishedGeometrySnapshot.load(std::memory_order_acquire);
}

auto WantsExternalAtlasGeometryFrame() noexcept -> bool {
    const auto snapshot = AcquireExternalAtlasGeometrySnapshot();
    return snapshot != nullptr && snapshot->geometry != nullptr
        && !snapshot->visibleLevelIds.empty()
        && !snapshot->geometry->cells.empty();
}

auto HasPersistedExternalRevealMapIntent(
        std::uint32_t seed,
        std::uint8_t difficulty) noexcept -> bool {
    std::filesystem::path root;
    try {
        std::scoped_lock lock(StateMutex);
        root = GeometryCacheRoot;
    } catch (...) {
        return false;
    }
    return HasExternalAtlasRevealMapIntent(root, seed, difficulty);
}

auto SetPersistedExternalRevealMapIntent(
        std::uint32_t seed,
        std::uint8_t difficulty,
        bool enabled) noexcept -> bool {
    std::filesystem::path root;
    try {
        std::scoped_lock lock(StateMutex);
        root = GeometryCacheRoot;
    } catch (...) {
        return false;
    }
    return StoreExternalAtlasRevealMapIntent(
        root, seed, difficulty, enabled);
}

auto GetExternalLabelProviderCounters() noexcept
        -> ExternalLabelProviderCounters {
    return {
        .requests = Requests.load(std::memory_order_relaxed),
        .processesStarted = ProcessesStarted.load(std::memory_order_relaxed),
        .atlasesPublished = AtlasesPublished.load(std::memory_order_relaxed),
        .staleResponses = StaleResponses.load(std::memory_order_relaxed),
        .failedResponses = FailedResponses.load(std::memory_order_relaxed),
        .timeouts = Timeouts.load(std::memory_order_relaxed),
        .geometryCacheHits = GeometryCacheHits.load(
            std::memory_order_relaxed),
        .geometryAtlasesGenerated = GeometryAtlasesGenerated.load(
            std::memory_order_relaxed),
        .geometryFailures = GeometryFailures.load(
            std::memory_order_relaxed),
        .geometrySnapshotsPublished = GeometrySnapshotsPublished.load(
            std::memory_order_relaxed),
        .publishedGeometryCells = PublishedGeometryCells.load(
            std::memory_order_relaxed),
    };
}

} // namespace RuffnecKk::MapSense
