#include "reveal_engine.hpp"

#include <D2RLPlugin/api.h>
#include <D2RLPlugin/core_exports.h>

#include <Windows.h>

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace RuffnecKk::MapSense {
namespace {

constexpr std::uintptr_t GetLocalDataContextRva = 0x08B2D0;
constexpr std::uintptr_t GetLocalPlayerRva = 0x09A480;
constexpr std::uintptr_t InitLevelRva = 0x3271C0;
constexpr std::uintptr_t GetLevelRva = 0x3267C0;
constexpr std::uintptr_t CreateActiveRoomRva = 0x3289A0;
constexpr std::uintptr_t PathGetRoomRva = 0x341C30;
constexpr std::uintptr_t StandardAutomapCallbackRva = 0x0D2240;
constexpr char SupportedCoreVersion[] = "1.1.0-beta";

constexpr std::size_t UnitTypeOffset = 0x00;
constexpr std::size_t UnitDynamicPathOffset = 0x38;
constexpr std::size_t ActiveRoomDrlgRoomOffset = 0x18;
constexpr std::size_t DrlgRoomLevelOffset = 0x90;
constexpr std::size_t LevelFirstRoomOffset = 0x10;
constexpr std::size_t LevelDrlgOffset = 0x1C8;
constexpr std::size_t LevelIdOffset = 0x1F8;
constexpr std::size_t RoomNextOffset = 0x48;
constexpr std::size_t DrlgAutomapCallbackOffset = 0x838;
constexpr std::uint32_t MaximumRoomsPerLevel = 4096;

using GetLocalDataContextFn = std::int32_t(__fastcall*)() noexcept;
using GetLocalPlayerFn = void*(__fastcall*)(std::int32_t) noexcept;
using InitLevelFn = void(__fastcall*)(std::uint8_t, void*);
using GetLevelFn = void*(__fastcall*)(std::uint8_t, void*, std::uint32_t);
using CreateActiveRoomFn = void*(__fastcall*)(std::uint8_t, void*);
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
PathGetRoomFn PathGetRoom{};
D2RL::CoreExports::IsInGameFn IsInGame{};
D2RL::CoreExports::ExecuteConsoleCommandFn ExecuteConsoleCommand{};

std::atomic_bool Active{};
std::atomic_bool RevealAllArmed{};
std::atomic_bool Diagnostics{};
void* ActiveClientDrlg{};
std::uint8_t ActiveClientDataContext{};
std::atomic_flag ClientDrlgLock = ATOMIC_FLAG_INIT;
std::atomic_uint64_t LevelsRevealed{};
std::atomic_uint64_t RoomsRevealed{};
std::atomic_uint64_t ActRequests{};
std::atomic_uint64_t RejectedActRequests{};
std::atomic_uint64_t RevealFailures{};
std::atomic_uint64_t TraversalLimits{};

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

        auto* const currentLevel = static_cast<std::uint8_t*>(GetLevel(
            clientDataContext, clientDrlg, currentLevelId));
        if (currentLevel == nullptr) return false;
        if (*reinterpret_cast<void**>(
                currentLevel + LevelFirstRoomOffset) == nullptr) {
            InitLevel(clientDataContext, currentLevel);
        }
        output = {
            .dataContext = clientDataContext,
            .levelId = static_cast<std::int32_t>(currentLevelId),
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

auto SubmitNativeActReveal() noexcept -> bool {
    ActRequests.fetch_add(1, std::memory_order_relaxed);
    const bool inGame = IsInGame != nullptr && IsInGame();
    const bool accepted = inGame && ExecuteConsoleCommand != nullptr
        && ExecuteConsoleCommand("revealmap");
    if (Diagnostics.load(std::memory_order_acquire) && Context != nullptr) {
        if (accepted) {
            Context->LogInfo(
                "MapSense diagnostics: D2RCore accepted the revealmap request.");
        } else {
            Context->LogWarn(
                "MapSense diagnostics: D2RCore rejected or could not dispatch revealmap.");
        }
    }
    if (!accepted) {
        RejectedActRequests.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    return true;
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
}

auto ResolveCoreBridge() noexcept -> bool {
    const HMODULE core = GetModuleHandleA(D2RL::CoreExports::CoreDllName);
    if (core == nullptr) return false;

    const auto version = reinterpret_cast<D2RL::CoreExports::VersionFn>(
        GetProcAddress(core, D2RL::CoreExports::VersionInfo.name));
    IsInGame = reinterpret_cast<D2RL::CoreExports::IsInGameFn>(
        GetProcAddress(core, D2RL::CoreExports::IsInGameInfo.name));
    ExecuteConsoleCommand =
        reinterpret_cast<D2RL::CoreExports::ExecuteConsoleCommandFn>(
            GetProcAddress(
                core,
                D2RL::CoreExports::ExecuteConsoleCommandInfo.name));
    if (version == nullptr || IsInGame == nullptr
        || ExecuteConsoleCommand == nullptr) {
        return false;
    }
    const char* const activeVersion = version();
    return activeVersion != nullptr
        && std::strcmp(activeVersion, SupportedCoreVersion) == 0;
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
    constexpr std::array<std::uint8_t, 32> callbackExpected{
        0x48, 0x89, 0x5C, 0x24, 0x08, 0x48, 0x89, 0x74,
        0x24, 0x10, 0x57, 0x48, 0x83, 0xEC, 0x20, 0x48,
        0x8B, 0xF1, 0xE8, 0x79, 0x90, 0xFB, 0xFF, 0x8B,
        0xC8, 0xE8, 0x22, 0x82, 0xFC, 0xFF, 0x48, 0x85};
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
        && check(StandardAutomapCallbackRva, callbackExpected);
}

} // namespace

auto InitializeRevealEngine(
        const D2RL::PluginContext* context,
        bool diagnostics) noexcept -> bool {
    if (!D2RL::HasContext(context) || context->exeBase == 0) return false;
    Context = context;
    Base = reinterpret_cast<std::uint8_t*>(context->exeBase);
    Diagnostics.store(diagnostics, std::memory_order_release);
    ResetRevealSession();
    if (!ResolveCoreBridge()) {
        Context->LogError(
            "MapSense: D2RCore 1.1.0-beta reveal command bridge is unavailable.");
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

void ShutdownRevealEngine() noexcept {
    Active.store(false, std::memory_order_release);
    ResetRevealSession();
    // D2RLoader owns the inline hook and restores it after the plugin unload
    // callback. Keep the trampoline and native addresses valid until the DLL
    // is actually unmapped so an already-running hook can safely finish.
}

void BeginRevealSession() noexcept {
    RevealAllArmed.store(false, std::memory_order_release);
    LevelsRevealed.store(0, std::memory_order_relaxed);
    RoomsRevealed.store(0, std::memory_order_relaxed);
    ActRequests.store(0, std::memory_order_relaxed);
    RejectedActRequests.store(0, std::memory_order_relaxed);
    RevealFailures.store(0, std::memory_order_relaxed);
    TraversalLimits.store(0, std::memory_order_relaxed);
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

auto RevealCurrentAct() noexcept -> RevealOutcome {
    if (!Active.load(std::memory_order_acquire)) {
        return RevealOutcome::Unavailable;
    }
    if (SubmitNativeActReveal()) {
        return RevealOutcome::Accepted;
    }
    RevealFailures.fetch_add(1, std::memory_order_relaxed);
    return RevealOutcome::Unavailable;
}

auto ToggleRevealAll() noexcept -> RevealOutcome {
    if (!Active.load(std::memory_order_acquire)) {
        return RevealOutcome::Unavailable;
    }
    if (RevealAllArmed.exchange(true, std::memory_order_acq_rel)) {
        RevealAllArmed.store(false, std::memory_order_release);
        return RevealOutcome::Disarmed;
    }
    const auto current = RevealCurrentAct();
    if (current == RevealOutcome::Unavailable) {
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
        .actRequests = ActRequests.load(std::memory_order_relaxed),
        .rejectedActRequests =
            RejectedActRequests.load(std::memory_order_relaxed),
        .failures = RevealFailures.load(std::memory_order_relaxed),
        .traversalLimits = TraversalLimits.load(std::memory_order_relaxed),
    };
}

} // namespace RuffnecKk::MapSense
