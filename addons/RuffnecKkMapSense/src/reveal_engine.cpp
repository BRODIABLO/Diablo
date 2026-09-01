#include "reveal_engine.hpp"

#include <D2RLPlugin/api.h>
#include <D2RLPlugin/core_exports.h>

#include <Windows.h>

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

namespace RuffnecKk::MapSense {
namespace {

constexpr std::uintptr_t GetLocalDataContextRva = 0x08B2D0;
constexpr std::uintptr_t GetLocalPlayerRva = 0x09A480;
constexpr std::uintptr_t InitLevelRva = 0x3271C0;
constexpr std::uintptr_t GetLevelRva = 0x3267C0;
constexpr std::uintptr_t CreateActiveRoomRva = 0x3289A0;
constexpr std::uintptr_t PrepareRoomWitnessRva = 0x3289B3;
constexpr std::uintptr_t BuildRoomPipelineWitnessRva = 0x328FDF;
constexpr std::uintptr_t BuildNearRoomLinksRva = 0x3608A0;
constexpr std::uintptr_t AddPresetUnitsRva = 0x3DE0E0;
constexpr std::uintptr_t InitializeStaticRoomRva = 0x3F38D0;
constexpr std::uintptr_t AddStaticRoomTilesRva = 0x3F3930;
constexpr std::uintptr_t LoadRoomTileLibrariesRva = 0x3F3970;
constexpr std::uintptr_t ReleaseStaticRoomRva = 0x3F3AA0;
constexpr std::uintptr_t ReleaseStaticRoomWitnessRva = 0x3F3ADC;
constexpr std::uintptr_t PathGetRoomRva = 0x341C30;
constexpr std::uintptr_t StandardAutomapCallbackRva = 0x0D2240;
constexpr std::uintptr_t ClientDrlgDifficultyWitnessRva = 0x32766A;
constexpr std::uintptr_t DrlgSeedInitializationWitnessRva = 0x326E89;

constexpr std::size_t UnitTypeOffset = 0x00;
constexpr std::size_t UnitDynamicPathOffset = 0x38;
constexpr std::size_t ActiveRoomDrlgRoomOffset = 0x18;
constexpr std::size_t DrlgRoomLevelOffset = 0x90;
constexpr std::size_t LevelFirstRoomOffset = 0x10;
constexpr std::size_t LevelDrlgOffset = 0x1C8;
constexpr std::size_t LevelIdOffset = 0x1F8;
constexpr std::size_t RoomNextOffset = 0x48;
constexpr std::size_t RoomNearCountOffset = 0x18;
constexpr std::size_t RoomFlagsOffset = 0x50;
constexpr std::size_t RoomActiveRoomOffset = 0x58;
constexpr std::size_t RoomTypeOffset = 0x74;
constexpr std::size_t RoomPresetUnitOffset = 0x98;
constexpr std::size_t DrlgDifficultyOffset = 0x830;
constexpr std::size_t DrlgAutomapCallbackOffset = 0x838;
constexpr std::size_t DrlgMapSeedOffset = 0x840;
constexpr std::size_t DrlgStartSeedOffset = 0x860;
constexpr std::uint32_t MaximumRoomsPerLevel = 4096;

using GetLocalDataContextFn = std::int32_t(__fastcall*)() noexcept;
using GetLocalPlayerFn = void*(__fastcall*)(std::int32_t) noexcept;
using InitLevelFn = void(__fastcall*)(std::uint8_t, void*);
using GetLevelFn = void*(__fastcall*)(std::uint8_t, void*, std::uint32_t);
using CreateActiveRoomFn = void*(__fastcall*)(std::uint8_t, void*);
using PrepareStaticRoomFn = void(__fastcall*)(std::uint8_t, void*);
using ReleaseStaticRoomFn = void(__fastcall*)(void*, std::int32_t);
using PathGetRoomFn = void*(__fastcall*)(void*) noexcept;
using RevealActiveRoomFn = void(__fastcall*)(void*);

const D2RL::PluginContext* Context{};
std::uint8_t* Base{};
GetLocalDataContextFn GetLocalDataContext{};
GetLocalPlayerFn GetLocalPlayer{};
InitLevelFn InitLevel{};
InitLevelFn OriginalInitLevel{};
GetLevelFn GetLevel{};
CreateActiveRoomFn CreateActiveRoom{};
PrepareStaticRoomFn BuildNearRoomLinks{};
PrepareStaticRoomFn AddPresetUnits{};
PrepareStaticRoomFn InitializeStaticRoom{};
PrepareStaticRoomFn AddStaticRoomTiles{};
PrepareStaticRoomFn LoadRoomTileLibraries{};
ReleaseStaticRoomFn ReleaseStaticRoom{};
PathGetRoomFn PathGetRoom{};
D2RL::CoreExports::IsInGameFn IsInGame{};
D2RL::CoreExports::ExecuteConsoleCommandFn ExecuteConsoleCommand{};

std::atomic_bool Active{};
std::atomic_bool RevealAllArmed{};
void* ActiveClientDrlg{};
std::uint8_t ActiveClientDataContext{};
std::atomic_flag ClientDrlgLock = ATOMIC_FLAG_INIT;
std::atomic_uint64_t LevelsRevealed{};
std::atomic_uint64_t RoomsRevealed{};
std::atomic_uint64_t RevealFailures{};
std::atomic_uint64_t TraversalLimits{};
std::atomic_uint64_t StaticRoomCandidates{};
std::atomic_uint64_t StaticRoomsMaterialized{};
std::atomic_uint64_t StaticRoomsReleased{};
std::atomic_uint64_t StaticRoomFailures{};
std::atomic<RevealLevelInitializedCallback> LevelInitializedCallback{};
std::atomic<void*> LevelInitializedUserData{};

auto ResolveCoreBridge() noexcept -> bool {
    const HMODULE core = GetModuleHandleA(D2RL::CoreExports::CoreDllName);
    if (core == nullptr) return false;
    IsInGame = reinterpret_cast<D2RL::CoreExports::IsInGameFn>(
        GetProcAddress(core, D2RL::CoreExports::IsInGameInfo.name));
    ExecuteConsoleCommand =
        reinterpret_cast<D2RL::CoreExports::ExecuteConsoleCommandFn>(
            GetProcAddress(
                core,
                D2RL::CoreExports::ExecuteConsoleCommandInfo.name));
    return IsInGame != nullptr && ExecuteConsoleCommand != nullptr;
}

auto SubmitNativeActReveal() noexcept -> bool {
    return IsInGame != nullptr && IsInGame()
        && ExecuteConsoleCommand != nullptr
        && ExecuteConsoleCommand("revealmap");
}

class ClientDrlgLockGuard {
public:
    ClientDrlgLockGuard() noexcept {
        while (ClientDrlgLock.test_and_set(std::memory_order_acquire)) {
        }
    }
    ~ClientDrlgLockGuard() {
        ClientDrlgLock.clear(std::memory_order_release);
    }

    ClientDrlgLockGuard(const ClientDrlgLockGuard&) = delete;
    auto operator=(const ClientDrlgLockGuard&) -> ClientDrlgLockGuard& = delete;
};

void SnapshotClientDrlg(
        std::uint8_t& dataContext,
        std::uint8_t*& drlg) noexcept {
    ClientDrlgLockGuard lock;
    dataContext = ActiveClientDataContext;
    drlg = static_cast<std::uint8_t*>(ActiveClientDrlg);
}

void StoreClientDrlg(
        std::uint8_t dataContext,
        void* drlg) noexcept {
    ClientDrlgLockGuard lock;
    ActiveClientDataContext = dataContext;
    ActiveClientDrlg = drlg;
}

template <typename Function>
auto At(std::uintptr_t rva) noexcept -> Function {
    return reinterpret_cast<Function>(Base + rva);
}

auto RevealLevelUnchecked(
        std::uint8_t dataContext,
        void* level) noexcept -> bool {
    __try {
        if (level == nullptr || CreateActiveRoom == nullptr || Base == nullptr) {
            return false;
        }

        auto* const levelBytes = static_cast<std::uint8_t*>(level);
        auto* const drlg = *reinterpret_cast<std::uint8_t**>(
            levelBytes + LevelDrlgOffset);
        if (drlg == nullptr) return false;

        const auto callback = *reinterpret_cast<RevealActiveRoomFn*>(
            drlg + DrlgAutomapCallbackOffset);
        if (callback == nullptr) return false;
        if (reinterpret_cast<void*>(callback)
            != Base + StandardAutomapCallbackRva) {
            return false;
        }

        auto* room = *reinterpret_cast<std::uint8_t**>(
            levelBytes + LevelFirstRoomOffset);
        if (room == nullptr) return false;
        bool complete = true;
        std::uint32_t roomCount{};
        while (room != nullptr && roomCount < MaximumRoomsPerLevel) {
            if (void* const activeRoom = CreateActiveRoom(dataContext, room)) {
                callback(activeRoom);
                RoomsRevealed.fetch_add(1, std::memory_order_relaxed);
            } else {
                complete = false;
            }
            room = *reinterpret_cast<std::uint8_t**>(room + RoomNextOffset);
            ++roomCount;
        }
        if (room != nullptr) {
            TraversalLimits.fetch_add(1, std::memory_order_relaxed);
            return false;
        }
        if (complete) {
            LevelsRevealed.fetch_add(1, std::memory_order_relaxed);
        }
        return complete;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

auto ResolveClientLevel(ClientLevelView& output) noexcept -> bool {
    std::uint8_t clientDataContext{};
    std::uint8_t* clientDrlg{};
    SnapshotClientDrlg(clientDataContext, clientDrlg);
    __try {
        if (GetLocalDataContext == nullptr || GetLocalPlayer == nullptr
            || PathGetRoom == nullptr || GetLevel == nullptr
            || InitLevel == nullptr || Base == nullptr) {
            return false;
        }

        const std::int32_t context = GetLocalDataContext();
        if (context < 0 || context >= 8) return false;
        auto* const player = static_cast<std::uint8_t*>(GetLocalPlayer(context));
        if (player == nullptr
            || *reinterpret_cast<std::uint32_t*>(player + UnitTypeOffset) != 0) {
            return false;
        }
        void* const path = *reinterpret_cast<void**>(
            player + UnitDynamicPathOffset);
        if (path == nullptr) return false;
        auto* const activeRoom = static_cast<std::uint8_t*>(PathGetRoom(path));
        if (activeRoom == nullptr) return false;
        auto* const drlgRoom = *reinterpret_cast<std::uint8_t**>(
            activeRoom + ActiveRoomDrlgRoomOffset);
        if (drlgRoom == nullptr) return false;
        auto* const serverLevel = *reinterpret_cast<std::uint8_t**>(
            drlgRoom + DrlgRoomLevelOffset);
        if (serverLevel == nullptr) return false;
        const auto currentLevelId = *reinterpret_cast<std::uint32_t*>(
            serverLevel + LevelIdOffset);

        if (clientDrlg == nullptr) return false;
        const auto callback = *reinterpret_cast<RevealActiveRoomFn*>(
            clientDrlg + DrlgAutomapCallbackOffset);
        if (reinterpret_cast<void*>(callback)
            != Base + StandardAutomapCallbackRva) {
            return false;
        }
        const auto difficulty = *reinterpret_cast<const std::uint8_t*>(
            clientDrlg + DrlgDifficultyOffset);
        if (difficulty > 2U) return false;
        const auto mapSeed = *reinterpret_cast<const std::uint32_t*>(
            clientDrlg + DrlgMapSeedOffset);
        const auto drlgStartSeed = *reinterpret_cast<const std::uint32_t*>(
            clientDrlg + DrlgStartSeedOffset);
        if (DeriveDrlgStartSeed(mapSeed) != drlgStartSeed) return false;

        auto* const currentLevel = static_cast<std::uint8_t*>(GetLevel(
            clientDataContext, clientDrlg, currentLevelId));
        if (currentLevel == nullptr) return false;
        if (*reinterpret_cast<void**>(
                currentLevel + LevelFirstRoomOffset) == nullptr) {
            InitLevel(clientDataContext, currentLevel);
        }
        output = {
            .dataContext = clientDataContext,
            .difficulty = difficulty,
            .levelId = static_cast<std::int32_t>(currentLevelId),
            .mapSeed = mapSeed,
            .drlgStartSeed = drlgStartSeed,
            .activeRoom = activeRoom,
            .drlg = clientDrlg,
            .level = currentLevel,
        };
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

auto ResolveClientLevelByIdUnchecked(
        const ClientLevelView& current,
        std::int32_t levelId,
        void*& outputLevel) noexcept -> bool {
    outputLevel = nullptr;
    __try {
        if (GetLevel == nullptr || InitLevel == nullptr
            || current.drlg == nullptr || levelId <= 0) {
            return false;
        }
        auto* const level = static_cast<std::uint8_t*>(GetLevel(
            current.dataContext,
            current.drlg,
            static_cast<std::uint32_t>(levelId)));
        if (level == nullptr
            || *reinterpret_cast<void**>(level + LevelDrlgOffset)
                != current.drlg
            || *reinterpret_cast<const std::int32_t*>(level + LevelIdOffset)
                != levelId) {
            return false;
        }
        if (*reinterpret_cast<void**>(level + LevelFirstRoomOffset) == nullptr) {
            InitLevel(current.dataContext, level);
        }
        if (*reinterpret_cast<void**>(level + LevelFirstRoomOffset) == nullptr) {
            return false;
        }
        outputLevel = level;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

void CaptureClientDrlg(
        std::uint8_t dataContext,
        void* level) noexcept {
    void* capturedDrlg{};
    __try {
        auto* const levelBytes = static_cast<std::uint8_t*>(level);
        auto* const drlg = levelBytes
            ? *reinterpret_cast<std::uint8_t**>(
                levelBytes + LevelDrlgOffset)
            : nullptr;
        const auto callback = drlg
            ? *reinterpret_cast<RevealActiveRoomFn*>(
                drlg + DrlgAutomapCallbackOffset)
            : nullptr;
        if (reinterpret_cast<void*>(callback)
            == Base + StandardAutomapCallbackRva) {
            capturedDrlg = drlg;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
    if (capturedDrlg != nullptr) {
        StoreClientDrlg(dataContext, capturedDrlg);
    }
}

__declspec(noinline) void __fastcall HookInitLevel(
        std::uint8_t dataContext,
        void* level) {
    const auto original = OriginalInitLevel;
    if (original == nullptr) return;
    original(dataContext, level);
    if (!Active.load(std::memory_order_acquire)) return;
    CaptureClientDrlg(dataContext, level);
    const auto callback = LevelInitializedCallback.load(
        std::memory_order_acquire);
    if (callback != nullptr) {
        callback(
            dataContext,
            level,
            LevelInitializedUserData.load(std::memory_order_acquire));
    }
}

auto ValidateRuntime() noexcept -> bool {
    constexpr std::array<std::uint8_t, 10> localContextExpected{
        0x8B, 0x05, 0x2E, 0x84, 0x99, 0x02, 0xC3, 0xCC, 0xCC, 0xCC};
    constexpr std::array<std::uint8_t, 19> localPlayerExpected{
        0x48, 0x89, 0x5C, 0x24, 0x08, 0x57, 0x48, 0x83,
        0xEC, 0x20, 0x83, 0xF9, 0x08, 0x0F, 0x83, 0x85,
        0x00, 0x00, 0x00};
    constexpr std::array<std::uint8_t, 32> getLevelExpected{
        0x48, 0x89, 0x6C, 0x24, 0x10, 0x48, 0x89, 0x74,
        0x24, 0x18, 0x57, 0x48, 0x81, 0xEC, 0x80, 0x02,
        0x00, 0x00, 0x48, 0x8B, 0x82, 0x68, 0x08, 0x00,
        0x00, 0x41, 0x8B, 0xF8, 0x48, 0x8B, 0xF2, 0x0F};
    constexpr std::array<std::uint8_t, 16> pathGetRoomExpected{
        0x48, 0x8B, 0x41, 0x20, 0xC3, 0xCC, 0xCC, 0xCC,
        0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC};
    constexpr std::array<std::uint8_t, 32> initLevelExpected{
        0x48, 0x89, 0x6C, 0x24, 0x20, 0x56, 0x41, 0x56,
        0x41, 0x57, 0x48, 0x83, 0xEC, 0x20, 0x48, 0x8B,
        0x82, 0xC8, 0x01, 0x00, 0x00, 0x4C, 0x8B, 0xF2,
        0x44, 0x0F, 0xB6, 0xF9, 0x8B, 0x90, 0x60, 0x08};
    constexpr std::array<std::uint8_t, 32> createRoomExpected{
        0x48, 0x89, 0x5C, 0x24, 0x08, 0x57, 0x48, 0x83,
        0xEC, 0x20, 0x8B, 0x42, 0x50, 0x48, 0x8B, 0xDA,
        0x0F, 0xB6, 0xF9, 0x0F, 0xBA, 0xE0, 0x18, 0x72,
        0x08, 0xE8, 0xB2, 0xAF, 0x0C, 0x00, 0x8B, 0x43};
    // The native wrapper proves the exact static preconditions: tile-library
    // bit 24, preset-descriptor bit 25 for preset rooms, then HAS_ROOM bit 20.
    constexpr std::array<std::uint8_t, 59> prepareRoomExpected{
        0x0F, 0xBA, 0xE0, 0x18, 0x72, 0x08, 0xE8, 0xB2,
        0xAF, 0x0C, 0x00, 0x8B, 0x43, 0x50, 0x0F, 0xBA,
        0xE0, 0x19, 0x72, 0x15, 0x83, 0x7B, 0x74, 0x02,
        0x75, 0x0F, 0x48, 0x8B, 0xD3, 0x40, 0x0F, 0xB6,
        0xCF, 0xE8, 0x07, 0x57, 0x0B, 0x00, 0x8B, 0x43,
        0x50, 0x0F, 0xBA, 0xE0, 0x14, 0x72, 0x0C, 0x48,
        0x8B, 0xD3, 0x40, 0x0F, 0xB6, 0xCF, 0xE8, 0xE2,
        0x05, 0x00, 0x00};
    // This witness is the safety boundary: near-room/static-grid/static-tile
    // calls occur before the separate ActiveRoom allocator at 0x326480.
    constexpr std::array<std::uint8_t, 76> buildRoomPipelineExpected{
        0x48, 0x83, 0x7A, 0x18, 0x00, 0x48, 0x8B, 0xDA,
        0x48, 0x8B, 0x82, 0x90, 0x00, 0x00, 0x00, 0x0F,
        0xB6, 0xF1, 0x48, 0x8B, 0xB8, 0xC8, 0x01, 0x00,
        0x00, 0x75, 0x05, 0xE8, 0xA1, 0x78, 0x03, 0x00,
        0x48, 0x8B, 0xD3, 0x40, 0x0F, 0xB6, 0xCE, 0xE8,
        0xC5, 0xA8, 0x0C, 0x00, 0x48, 0x8B, 0xD3, 0x40,
        0x0F, 0xB6, 0xCE, 0xE8, 0x19, 0xA9, 0x0C, 0x00,
        0x48, 0x8B, 0xD3, 0x48, 0x8B, 0xCF, 0xE8, 0x5E,
        0xD4, 0xFF, 0xFF, 0xFE, 0x87, 0x24, 0x01, 0x00,
        0x00, 0xFF, 0x47, 0x08};
    constexpr std::array<std::uint8_t, 31> buildNearRoomLinksExpected{
        0x88, 0x4C, 0x24, 0x08, 0x53, 0x56, 0x41, 0x55,
        0x48, 0x83, 0xEC, 0x70, 0x48, 0x89, 0x7C, 0x24,
        0x60, 0x48, 0x8B, 0xF2, 0x0F, 0xB6, 0xF9, 0x48,
        0x8B, 0xCA, 0xE8, 0x01, 0x0C, 0x00, 0x00};
    constexpr std::array<std::uint8_t, 30> addPresetUnitsExpected{
        0x40, 0x55, 0x56, 0x57, 0x41, 0x56, 0x41, 0x57,
        0x48, 0x8D, 0xAC, 0x24, 0x60, 0xFC, 0xFF, 0xFF,
        0x48, 0x81, 0xEC, 0xA0, 0x04, 0x00, 0x00, 0x48,
        0x8B, 0x05, 0xCA, 0xD1, 0x5E, 0x02};
    constexpr std::array<std::uint8_t, 31> initializeStaticRoomExpected{
        0x48, 0x89, 0x5C, 0x24, 0x08, 0x57, 0x48, 0x83,
        0xEC, 0x20, 0x0F, 0xB6, 0xF9, 0x48, 0x8B, 0xDA,
        0x48, 0x8D, 0x4A, 0x30, 0x8B, 0x52, 0x08, 0xE8,
        0x24, 0x38, 0xF7, 0xFF, 0x8B, 0x53, 0x74};
    constexpr std::array<std::uint8_t, 43> addStaticRoomTilesExpected{
        0x40, 0x53, 0x48, 0x83, 0xEC, 0x20, 0x48, 0x8B,
        0xDA, 0x8B, 0x52, 0x74, 0x83, 0xEA, 0x01, 0x74,
        0x1A, 0x83, 0xFA, 0x01, 0x75, 0x1D, 0x48, 0x8B,
        0xD3, 0xE8, 0x92, 0xAD, 0xFE, 0xFF, 0x81, 0x4B,
        0x50, 0x00, 0x00, 0x10, 0x00, 0x48, 0x83, 0xC4,
        0x20, 0x5B, 0xC3};
    constexpr std::array<std::uint8_t, 32> loadRoomTileLibrariesExpected{
        0x48, 0x89, 0x5C, 0x24, 0x18, 0x55, 0x56, 0x57,
        0x48, 0x81, 0xEC, 0x40, 0x01, 0x00, 0x00, 0x48,
        0x8B, 0x05, 0x42, 0x79, 0x5D, 0x02, 0x48, 0x33,
        0xC4, 0x48, 0x89, 0x84, 0x24, 0x30, 0x01, 0x00};
    constexpr std::array<std::uint8_t, 32> releaseStaticRoomExpected{
        0x48, 0x89, 0x5C, 0x24, 0x20, 0x55, 0x57, 0x41,
        0x54, 0x48, 0x83, 0xEC, 0x20, 0x48, 0x8B, 0x81,
        0x90, 0x00, 0x00, 0x00, 0x8B, 0xEA, 0x48, 0x8B,
        0xF9, 0x48, 0x8B, 0x98, 0xC8, 0x01, 0x00, 0x00};
    constexpr std::array<std::uint8_t, 43> releaseStaticRoomWitnessExpected{
        0x48, 0x8B, 0xCF, 0x4C, 0x89, 0x67, 0x58, 0xE8,
        0x88, 0xDB, 0xFE, 0xFF, 0x8B, 0x47, 0x50, 0x0F,
        0xBA, 0xE0, 0x14, 0x0F, 0x83, 0x47, 0x01, 0x00,
        0x00, 0x0F, 0xBA, 0xF8, 0x14, 0x4C, 0x89, 0x7C,
        0x24, 0x50, 0x89, 0x47, 0x50, 0xFF, 0x83, 0x50,
        0x08, 0x00, 0x00};
    constexpr std::array<std::uint8_t, 32> callbackExpected{
        0x48, 0x89, 0x5C, 0x24, 0x08, 0x48, 0x89, 0x74,
        0x24, 0x10, 0x57, 0x48, 0x83, 0xEC, 0x20, 0x48,
        0x8B, 0xF1, 0xE8, 0x79, 0x90, 0xFB, 0xFF, 0x8B,
        0xC8, 0xE8, 0x22, 0x82, 0xFC, 0xFF, 0x48, 0x85};
    // RBP is the client Drlg and R8 is a Level in this unique witness.
    // Both Drlg+0x830 reads index three per-difficulty LevelDef arrays.
    constexpr std::array<std::uint8_t, 59> clientDifficultyExpected{
        0x48, 0x8B, 0xEA, 0x49, 0x8B, 0xD8, 0x41, 0x8B,
        0x90, 0xF8, 0x01, 0x00, 0x00, 0x44, 0x0F, 0xB6,
        0xF1, 0xE8, 0x80, 0x4B, 0x00, 0x00, 0x44, 0x0F,
        0xB6, 0x8D, 0x30, 0x08, 0x00, 0x00, 0x45, 0x33,
        0xDB, 0x48, 0x8B, 0xF8, 0x41, 0x8B, 0xF3, 0x44,
        0x8B, 0x40, 0x2C, 0x46, 0x8B, 0x54, 0x88, 0x0C,
        0x44, 0x89, 0x53, 0x2C, 0x0F, 0xB6, 0x8D, 0x30,
        0x08, 0x00, 0x00};
    // DRLG allocation initializes {seed, 0x29A}, advances the native LCG once,
    // stores the original seed at +0x840 and its low result at +0x860.
    constexpr std::array<std::uint8_t, 129> seedInitializationExpected{
        0x8B, 0xD3, 0x48, 0x89, 0x87, 0x58, 0x08, 0x00,
        0x00, 0x44, 0x88, 0xAF, 0x70, 0x08, 0x00, 0x00,
        0xE8, 0x72, 0x02, 0x04, 0x00, 0x8B, 0x07, 0x4C,
        0x69, 0xC8, 0xC5, 0x90, 0xC6, 0x6A, 0x8B, 0x47,
        0x04, 0x4C, 0x03, 0xC8, 0x89, 0x9F, 0x40, 0x08,
        0x00, 0x00, 0x8B, 0x85, 0x28, 0x08, 0x00, 0x00,
        0x49, 0x8B, 0xD1, 0x89, 0x87, 0x10, 0x01, 0x00,
        0x00, 0x48, 0x8B, 0x85, 0x30, 0x08, 0x00, 0x00,
        0x48, 0x89, 0x87, 0x28, 0x01, 0x00, 0x00, 0x0F,
        0xB6, 0x85, 0x38, 0x08, 0x00, 0x00, 0x88, 0x87,
        0x30, 0x08, 0x00, 0x00, 0x48, 0x8B, 0x85, 0x40,
        0x08, 0x00, 0x00, 0x48, 0xC1, 0xEA, 0x20, 0x48,
        0x89, 0x87, 0x38, 0x08, 0x00, 0x00, 0x48, 0x8B,
        0x85, 0x48, 0x08, 0x00, 0x00, 0x48, 0x89, 0x87,
        0x78, 0x08, 0x00, 0x00, 0x89, 0x57, 0x04, 0x44,
        0x89, 0x0F, 0x44, 0x89, 0x8F, 0x60, 0x08, 0x00,
        0x00};
    const auto check = [](std::uintptr_t rva, const auto& expected) noexcept {
        return Context->CheckExpectedBytes(
            rva,
            expected.data(),
            static_cast<std::uint32_t>(expected.size()));
    };
    return check(GetLocalDataContextRva, localContextExpected)
        && check(GetLocalPlayerRva, localPlayerExpected)
        && check(GetLevelRva, getLevelExpected)
        && check(PathGetRoomRva, pathGetRoomExpected)
        && check(InitLevelRva, initLevelExpected)
        && check(CreateActiveRoomRva, createRoomExpected)
        && check(PrepareRoomWitnessRva, prepareRoomExpected)
        && check(BuildRoomPipelineWitnessRva, buildRoomPipelineExpected)
        && check(BuildNearRoomLinksRva, buildNearRoomLinksExpected)
        && check(AddPresetUnitsRva, addPresetUnitsExpected)
        && check(InitializeStaticRoomRva, initializeStaticRoomExpected)
        && check(AddStaticRoomTilesRva, addStaticRoomTilesExpected)
        && check(LoadRoomTileLibrariesRva, loadRoomTileLibrariesExpected)
        && check(ReleaseStaticRoomRva, releaseStaticRoomExpected)
        && check(
            ReleaseStaticRoomWitnessRva,
            releaseStaticRoomWitnessExpected)
        && check(StandardAutomapCallbackRva, callbackExpected)
        && check(
            ClientDrlgDifficultyWitnessRva,
            clientDifficultyExpected)
        && check(
            DrlgSeedInitializationWitnessRva,
            seedInitializationExpected);
}

} // namespace

auto InitializeRevealEngine(
        const D2RL::PluginContext* context,
        bool diagnostics) noexcept -> bool {
    if (!D2RL::HasContext(context) || context->exeBase == 0) return false;
    Context = context;
    Base = reinterpret_cast<std::uint8_t*>(context->exeBase);
    (void)diagnostics;
    RevealAllArmed.store(false, std::memory_order_release);
    ResetRevealSession();
    if (!ResolveCoreBridge()) {
        Context->LogError(
            "MapSense: D2RCore public reveal command bridge is unavailable.");
        Context = nullptr;
        Base = nullptr;
        return false;
    }
    if (!ValidateRuntime()) {
        Context->LogError(
            "MapSense: D2R automap native fingerprint or ABI mismatch; plugin refused.");
        Context = nullptr;
        Base = nullptr;
        return false;
    }

    GetLocalDataContext = At<GetLocalDataContextFn>(GetLocalDataContextRva);
    GetLocalPlayer = At<GetLocalPlayerFn>(GetLocalPlayerRva);
    InitLevel = At<InitLevelFn>(InitLevelRva);
    GetLevel = At<GetLevelFn>(GetLevelRva);
    CreateActiveRoom = At<CreateActiveRoomFn>(CreateActiveRoomRva);
    BuildNearRoomLinks = At<PrepareStaticRoomFn>(BuildNearRoomLinksRva);
    AddPresetUnits = At<PrepareStaticRoomFn>(AddPresetUnitsRva);
    InitializeStaticRoom = At<PrepareStaticRoomFn>(InitializeStaticRoomRva);
    AddStaticRoomTiles = At<PrepareStaticRoomFn>(AddStaticRoomTilesRva);
    LoadRoomTileLibraries = At<PrepareStaticRoomFn>(
        LoadRoomTileLibrariesRva);
    ReleaseStaticRoom = At<ReleaseStaticRoomFn>(ReleaseStaticRoomRva);
    PathGetRoom = At<PathGetRoomFn>(PathGetRoomRva);

    constexpr std::array<std::uint8_t, 14> hookExpected{
        0x48, 0x89, 0x6C, 0x24, 0x20, 0x56, 0x41,
        0x56, 0x41, 0x57, 0x48, 0x83, 0xEC, 0x20};
    if (!Context->InstallInlineHook(
            InitLevelRva,
            hookExpected.data(),
            static_cast<std::uint32_t>(hookExpected.size()),
            HookInitLevel,
            &OriginalInitLevel)) {
        Context->LogError(
            "MapSense: native level-initialization hook was refused.");
        ShutdownRevealEngine();
        return false;
    }

    Active.store(true, std::memory_order_release);
    return true;
}

void SetRevealLevelInitializedCallback(
        RevealLevelInitializedCallback callback,
        void* userData) noexcept {
    if (callback == nullptr) {
        LevelInitializedCallback.store(nullptr, std::memory_order_release);
        LevelInitializedUserData.store(nullptr, std::memory_order_release);
        return;
    }
    LevelInitializedUserData.store(userData, std::memory_order_release);
    LevelInitializedCallback.store(callback, std::memory_order_release);
}

void ShutdownRevealEngine() noexcept {
    Active.store(false, std::memory_order_release);
    SetRevealLevelInitializedCallback(nullptr, nullptr);
    RevealAllArmed.store(false, std::memory_order_release);
    ResetRevealSession();
    // D2RLoader owns the inline hook and restores it after the plugin unload
    // callback. Keep the trampoline and native addresses valid until the DLL
    // is actually unmapped so an already-running hook can safely finish.
}

void BeginRevealSession() noexcept {
    LevelsRevealed.store(0, std::memory_order_relaxed);
    RoomsRevealed.store(0, std::memory_order_relaxed);
    RevealFailures.store(0, std::memory_order_relaxed);
    TraversalLimits.store(0, std::memory_order_relaxed);
    StaticRoomCandidates.store(0U, std::memory_order_relaxed);
    StaticRoomsMaterialized.store(0U, std::memory_order_relaxed);
    StaticRoomsReleased.store(0U, std::memory_order_relaxed);
    StaticRoomFailures.store(0U, std::memory_order_relaxed);
}

void ResetRevealSession() noexcept {
    BeginRevealSession();
    ClientDrlgLockGuard lock;
    ActiveClientDrlg = nullptr;
    ActiveClientDataContext = 0;
}

auto RevealCurrentZone() noexcept -> RevealOutcome {
    if (!Active.load(std::memory_order_acquire)) {
        return RevealOutcome::Unavailable;
    }
    ClientLevelView current{};
    if (ResolveClientLevel(current)
        && RevealLevelUnchecked(current.dataContext, current.level)) {
        return RevealOutcome::Complete;
    }
    RevealFailures.fetch_add(1, std::memory_order_relaxed);
    return RevealOutcome::Unavailable;
}

auto ResolveCurrentClientLevelView(
        ClientLevelView& output) noexcept -> bool {
    output = {};
    if (!Active.load(std::memory_order_acquire)) return false;
    return ResolveClientLevel(output);
}

auto ResolveClientLevelById(
        const ClientLevelView& current,
        std::int32_t levelId,
        void*& outputLevel) noexcept -> bool {
    outputLevel = nullptr;
    if (!Active.load(std::memory_order_acquire)) return false;
    return ResolveClientLevelByIdUnchecked(current, levelId, outputLevel);
}

auto MaterializeClientRoom(
        std::uint8_t dataContext,
        void* drlgRoom) noexcept -> void* {
    if (!Active.load(std::memory_order_acquire)
        || CreateActiveRoom == nullptr || drlgRoom == nullptr) {
        return nullptr;
    }
    __try {
        return CreateActiveRoom(dataContext, drlgRoom);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

auto PrepareStaticClientRoom(
        std::uint8_t dataContext,
        void* drlgRoom,
        StaticClientRoomLease& lease) noexcept -> bool {
    lease = {};
    if (!Active.load(std::memory_order_acquire)
        || dataContext >= 8U || drlgRoom == nullptr
        || BuildNearRoomLinks == nullptr || AddPresetUnits == nullptr
        || InitializeStaticRoom == nullptr || AddStaticRoomTiles == nullptr
        || LoadRoomTileLibraries == nullptr || ReleaseStaticRoom == nullptr) {
        StaticRoomFailures.fetch_add(1U, std::memory_order_relaxed);
        return false;
    }
    StaticRoomCandidates.fetch_add(1U, std::memory_order_relaxed);
    __try {
        auto* const room = static_cast<std::uint8_t*>(drlgRoom);
        auto flags = *reinterpret_cast<std::uint32_t*>(
            room + RoomFlagsOffset);
        if ((flags & (StaticPoiRoomWarpMask | StaticPoiRoomWaypointMask))
                == 0U) {
            StaticRoomFailures.fetch_add(1U, std::memory_order_relaxed);
            return false;
        }
        if (*reinterpret_cast<void**>(room + RoomActiveRoomOffset) != nullptr
            || (flags & StaticPoiRoomHasRoomMask) != 0U) {
            return true;
        }

        if ((flags & 0x01000000U) == 0U) {
            LoadRoomTileLibraries(dataContext, room);
        }
        flags = *reinterpret_cast<std::uint32_t*>(room + RoomFlagsOffset);
        if ((flags & 0x02000000U) == 0U
            && *reinterpret_cast<std::int32_t*>(room + RoomTypeOffset) == 2) {
            AddPresetUnits(dataContext, room);
        }
        flags = *reinterpret_cast<std::uint32_t*>(room + RoomFlagsOffset);
        if ((flags & StaticPoiRoomWarpMask) == 0U) {
            if ((flags & StaticPoiRoomWaypointMask) == 0U
                || (flags & StaticPoiRoomPresetUnitsAddedMask) == 0U
                || *reinterpret_cast<void**>(
                    room + RoomPresetUnitOffset) == nullptr) {
                StaticRoomFailures.fetch_add(1U, std::memory_order_relaxed);
                return false;
            }
            return true;
        }
        if (*reinterpret_cast<void**>(room + RoomActiveRoomOffset) != nullptr
            || (flags & StaticPoiRoomHasRoomMask) != 0U) {
            StaticRoomFailures.fetch_add(1U, std::memory_order_relaxed);
            return false;
        }

        if (*reinterpret_cast<std::uint64_t*>(
                room + RoomNearCountOffset) == 0U) {
            BuildNearRoomLinks(dataContext, room);
        }
        InitializeStaticRoom(dataContext, room);
        AddStaticRoomTiles(dataContext, room);
        flags = *reinterpret_cast<std::uint32_t*>(room + RoomFlagsOffset);
        if (*reinterpret_cast<void**>(room + RoomActiveRoomOffset) != nullptr
            || (flags & StaticPoiRoomHasRoomMask) == 0U) {
            if (*reinterpret_cast<void**>(room + RoomActiveRoomOffset)
                    == nullptr
                && (flags & StaticPoiRoomHasRoomMask) != 0U) {
                ReleaseStaticRoom(room, 0);
            }
            StaticRoomFailures.fetch_add(1U, std::memory_order_relaxed);
            return false;
        }
        lease = {.room = room, .owned = true};
        StaticRoomsMaterialized.fetch_add(1U, std::memory_order_relaxed);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        StaticRoomFailures.fetch_add(1U, std::memory_order_relaxed);
        lease = {};
        return false;
    }
}

auto ReleaseStaticClientRoom(
        StaticClientRoomLease& lease) noexcept -> bool {
    if (!lease.owned || lease.room == nullptr || ReleaseStaticRoom == nullptr) {
        lease = {};
        return false;
    }
    auto* const room = static_cast<std::uint8_t*>(lease.room);
    lease = {};
    __try {
        const auto flags = *reinterpret_cast<std::uint32_t*>(
            room + RoomFlagsOffset);
        if (*reinterpret_cast<void**>(room + RoomActiveRoomOffset) != nullptr
            || (flags & StaticPoiRoomHasRoomMask) == 0U) {
            StaticRoomFailures.fetch_add(1U, std::memory_order_relaxed);
            return false;
        }
        ReleaseStaticRoom(room, 0);
        if (*reinterpret_cast<void**>(room + RoomActiveRoomOffset) != nullptr
            || (*reinterpret_cast<std::uint32_t*>(room + RoomFlagsOffset)
                & StaticPoiRoomHasRoomMask) != 0U) {
            StaticRoomFailures.fetch_add(1U, std::memory_order_relaxed);
            return false;
        }
        StaticRoomsReleased.fetch_add(1U, std::memory_order_relaxed);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        StaticRoomFailures.fetch_add(1U, std::memory_order_relaxed);
        return false;
    }
}

auto RevealCurrentAct() noexcept -> RevealOutcome {
    if (!Active.load(std::memory_order_acquire)) {
        return RevealOutcome::Unavailable;
    }
    if (SubmitNativeActReveal()) return RevealOutcome::Complete;
    RevealFailures.fetch_add(1U, std::memory_order_relaxed);
    return RevealOutcome::Unavailable;
}

auto ArmRevealAll() noexcept -> RevealOutcome {
    if (!Active.load(std::memory_order_acquire)) {
        return RevealOutcome::Unavailable;
    }
    if (RevealAllArmed.load(std::memory_order_acquire)) {
        return RevealOutcome::Armed;
    }
    if (RevealCurrentAct() == RevealOutcome::Unavailable) {
        return RevealOutcome::Unavailable;
    }
    RevealAllArmed.store(true, std::memory_order_release);
    return RevealOutcome::Armed;
}

auto ToggleRevealAll() noexcept -> RevealOutcome {
    if (!Active.load(std::memory_order_acquire)) {
        return RevealOutcome::Unavailable;
    }
    if (RevealAllArmed.exchange(true, std::memory_order_acq_rel)) {
        RevealAllArmed.store(false, std::memory_order_release);
        return RevealOutcome::Disarmed;
    }
    if (RevealCurrentAct() == RevealOutcome::Unavailable) {
        RevealAllArmed.store(false, std::memory_order_release);
        return RevealOutcome::Unavailable;
    }
    return RevealOutcome::Armed;
}

auto DisableRevealAll() noexcept -> RevealOutcome {
    RevealAllArmed.store(false, std::memory_order_release);
    return RevealOutcome::Disarmed;
}

auto IsRevealEngineActive() noexcept -> bool {
    return Active.load(std::memory_order_acquire);
}

auto IsRevealAllArmed() noexcept -> bool {
    return RevealAllArmed.load(std::memory_order_acquire);
}

auto GetRevealCounters() noexcept -> RevealCounters {
    return {
        .levels = LevelsRevealed.load(std::memory_order_relaxed),
        .rooms = RoomsRevealed.load(std::memory_order_relaxed),
        .failures = RevealFailures.load(std::memory_order_relaxed),
        .traversalLimits = TraversalLimits.load(std::memory_order_relaxed),
        .staticRoomCandidates = StaticRoomCandidates.load(
            std::memory_order_relaxed),
        .staticRoomsMaterialized = StaticRoomsMaterialized.load(
            std::memory_order_relaxed),
        .staticRoomsReleased = StaticRoomsReleased.load(
            std::memory_order_relaxed),
        .staticRoomFailures = StaticRoomFailures.load(
            std::memory_order_relaxed),
    };
}

} // namespace RuffnecKk::MapSense
