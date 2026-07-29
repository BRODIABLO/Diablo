#define NOMINMAX
#include <D2RLPlugin/api.h>
#include "remote_stash_layout_policy.hpp"

#include <Windows.h>

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <intrin.h>
#include <mutex>
#include <unordered_map>

namespace {
using ruffneckk::remote_stash::HasUsableSize;
using ruffneckk::remote_stash::PlaceDesktopFooterLeft;
using ruffneckk::remote_stash::WidgetRect;

constexpr std::uint32_t SupportedBuild = 92777;

constexpr std::uintptr_t ConfigurePlayerInventoryRva = 0x22BA70;
constexpr std::uintptr_t DispatchUiMessageRva = 0x843D90;
constexpr std::uintptr_t QueueOutgoingPacketRva = 0xEE2A0;
constexpr std::uintptr_t RemoveItemHandlerRva = 0x4AA100;
constexpr std::uintptr_t InsertItemHandlerRva = 0x4BFF30;
constexpr std::uintptr_t SharedDepositHandlerRva = 0x4C5570;
constexpr std::uintptr_t SharedWithdrawalHandlerRva = 0x4C6480;
constexpr std::uintptr_t ValidateItemPacketStateRva = 0x474700;
constexpr std::uintptr_t GoldButtonHandlerRva = 0x4BA580;
constexpr std::uintptr_t GoldRangeGateRva = 0x4BA617;
constexpr std::uintptr_t GoldRangeBypassRva = 0x4BA6A1;
constexpr std::uintptr_t SendServerUiRva = 0x480650;
constexpr std::uintptr_t GetClientFromPlayerRva = 0x48FDE0;
constexpr std::uintptr_t IsRoomInTownRva = 0x2F0750;
constexpr std::uintptr_t TransferItemToInventoryPageRva = 0x15F8B0;
constexpr std::uintptr_t GetUiStateRva = 0xCE500;
constexpr std::uintptr_t FindWidgetRva = 0x856220;
constexpr std::uintptr_t GetWidgetRectRva = 0x8562A0;

// These two return addresses are the only client stash lifecycle checks that
// may observe the synthetic remote session as a town-backed stash session.
constexpr std::uintptr_t ClientStashTownCheckReturnRva = 0x259137;
constexpr std::uintptr_t ClientStashCleanupTownCheckReturnRva = 0x25A122;
constexpr std::uintptr_t QuickMoveItemInitTownCheckReturnRva = 0xFEE3B;
constexpr std::uintptr_t QuickMoveItemPlacementTownCheckReturnRva = 0xFF1DC;
constexpr std::uintptr_t QuickMoveStashUiStateReturnRva = 0x15F982;
constexpr std::uintptr_t QuickMoveToStashItemPacketStateReturnRva = 0x473F80;
constexpr std::uintptr_t QuickMoveFromStashItemPacketStateReturnRva = 0x474257;
constexpr std::size_t QuickMoveInventoryPageCallerStackOffset = 0x30;
constexpr std::size_t QuickMoveDestinationKindCallerStackOffset = 0x59;
constexpr std::size_t WidgetRectOffset = 0x70;
constexpr std::size_t ButtonOnClickMessageOffset = 0x558;

constexpr std::array<std::uint8_t, 32> ConfigurePlayerInventoryExpected{
    0x4C, 0x8B, 0xDC, 0x49, 0x89, 0x5B, 0x20, 0x55,
    0x56, 0x57, 0x41, 0x55, 0x41, 0x56, 0x48, 0x8D,
    0x6C, 0x24, 0x90, 0x48, 0x81, 0xEC, 0x70, 0x01,
    0x00, 0x00, 0x48, 0x8B, 0x05, 0x37, 0xF8, 0x79
};
constexpr std::array<std::uint8_t, 32> DispatchUiMessageExpected{
    0x40, 0x53, 0x56, 0x57, 0x48, 0x83, 0xEC, 0x20,
    0x4C, 0x89, 0x7C, 0x24, 0x58, 0x4C, 0x8B, 0xF9,
    0xE8, 0x7B, 0x1C, 0xA6, 0x00, 0x0F, 0xB6, 0x90,
    0x18, 0x01, 0x00, 0x00, 0x84, 0xD2, 0x74, 0x6F
};
constexpr std::array<std::uint8_t, 32> QueueOutgoingPacketExpected{
    0x48, 0x89, 0x5C, 0x24, 0x18, 0x55, 0x56, 0x57,
    0x48, 0x81, 0xEC, 0x30, 0x02, 0x00, 0x00, 0x48,
    0x8B, 0x05, 0x12, 0xD0, 0x8D, 0x02, 0x48, 0x33,
    0xC4, 0x48, 0x89, 0x84, 0x24, 0x20, 0x02, 0x00
};
constexpr std::array<std::uint8_t, 32> InsertItemHandlerExpected{
    0x40, 0x55, 0x53, 0x56, 0x57, 0x41, 0x56, 0x48,
    0x8D, 0x6C, 0x24, 0xF0, 0x48, 0x81, 0xEC, 0x10,
    0x01, 0x00, 0x00, 0x48, 0x8B, 0x05, 0x7E, 0xB3,
    0x50, 0x02, 0x48, 0x33, 0xC4, 0x48, 0x89, 0x45
};
constexpr std::array<std::uint8_t, 32> RemoveItemHandlerExpected{
    0x40, 0x55, 0x53, 0x56, 0x57, 0x41, 0x56, 0x48,
    0x8D, 0x6C, 0x24, 0xF0, 0x48, 0x81, 0xEC, 0x10,
    0x01, 0x00, 0x00, 0x48, 0x8B, 0x05, 0xAE, 0x11,
    0x52, 0x02, 0x48, 0x33, 0xC4, 0x48, 0x89, 0x45
};
constexpr std::array<std::uint8_t, 32> SharedDepositHandlerExpected{
    0x40, 0x55, 0x53, 0x56, 0x57, 0x41, 0x55, 0x48,
    0x8D, 0x6C, 0x24, 0xC9, 0x48, 0x81, 0xEC, 0xF0,
    0x00, 0x00, 0x00, 0x48, 0x8B, 0x05, 0x3E, 0x5D,
    0x50, 0x02, 0x48, 0x33, 0xC4, 0x48, 0x89, 0x45
};
constexpr std::array<std::uint8_t, 32> SharedWithdrawalHandlerExpected{
    0x40, 0x55, 0x53, 0x56, 0x57, 0x41, 0x55, 0x48,
    0x8D, 0x6C, 0x24, 0xC9, 0x48, 0x81, 0xEC, 0xF0,
    0x00, 0x00, 0x00, 0x48, 0x8B, 0x05, 0x2E, 0x4E,
    0x50, 0x02, 0x48, 0x33, 0xC4, 0x48, 0x89, 0x45
};
constexpr std::array<std::uint8_t, 32> ValidateItemPacketStateExpected{
    0x48, 0x89, 0x5C, 0x24, 0x08, 0x48, 0x89, 0x74,
    0x24, 0x10, 0x57, 0x48, 0x83, 0xEC, 0x20, 0x48,
    0x8B, 0xD9, 0x41, 0x0F, 0xB6, 0xF0, 0x48, 0x8B,
    0x09, 0x48, 0x8B, 0xFA, 0xE8, 0xAF, 0x72, 0xED
};
constexpr std::array<std::uint8_t, 32> GoldButtonHandlerExpected{
    0x40, 0x55, 0x53, 0x56, 0x57, 0x41, 0x57, 0x48,
    0x8D, 0x6C, 0x24, 0xC9, 0x48, 0x81, 0xEC, 0x00,
    0x01, 0x00, 0x00, 0x48, 0x8B, 0x05, 0x2E, 0x0D,
    0x51, 0x02, 0x48, 0x33, 0xC4, 0x48, 0x89, 0x45
};
constexpr std::array<std::uint8_t, 15> GoldRangeGateExpected{
    0x44, 0x39, 0x7D, 0xBF, 0x0F, 0x86, 0x39, 0x0E,
    0x00, 0x00, 0x48, 0x85, 0xFF, 0x75, 0x7B
};
constexpr std::array<std::uint8_t, 16> SendServerUiExpected{
    0x40, 0x57, 0x48, 0x83, 0xEC, 0x40, 0xC6, 0x44,
    0x24, 0x50, 0x77, 0x48, 0x8B, 0xF9, 0x88, 0x54
};
constexpr std::array<std::uint8_t, 16> GetClientFromPlayerExpected{
    0x40, 0x53, 0x48, 0x83, 0xEC, 0x20, 0x48, 0x8B,
    0xD9, 0x48, 0x85, 0xC9, 0x74, 0x1E, 0xE8, 0xDD
};
constexpr std::array<std::uint8_t, 16> IsRoomInTownExpected{
    0x48, 0x83, 0xEC, 0x28, 0x48, 0x85, 0xC9, 0x75,
    0x07, 0x33, 0xC0, 0x48, 0x83, 0xC4, 0x28, 0xC3
};
constexpr std::array<std::uint8_t, 16> TransferItemToInventoryPageExpected{
    0x40, 0x55, 0x53, 0x56, 0x57, 0x41, 0x54, 0x41,
    0x55, 0x41, 0x56, 0x41, 0x57, 0x48, 0x8D, 0xAC
};
constexpr std::array<std::uint8_t, 15> GetUiStateExpected{
    0x48, 0x63, 0xC1, 0x48, 0x8D, 0x0D, 0x96, 0xC8,
    0x95, 0x02, 0x0F, 0xB6, 0x04, 0x08, 0xC3
};
constexpr std::array<std::uint8_t, 32> FindWidgetExpected{
    0x48, 0x89, 0x5C, 0x24, 0x10, 0x48, 0x89, 0x74,
    0x24, 0x18, 0x57, 0x48, 0x83, 0xEC, 0x20, 0x48,
    0x8B, 0x59, 0x58, 0x48, 0x8B, 0xF2, 0x48, 0x8B,
    0x41, 0x60, 0x48, 0x8D, 0x3C, 0xC3, 0x48, 0x3B
};
constexpr std::array<std::uint8_t, 32> GetWidgetRectExpected{
    0x40, 0x53, 0x48, 0x83, 0xEC, 0x30, 0x80, 0x79,
    0x52, 0x00, 0x48, 0x8B, 0xDA, 0x74, 0x2A, 0x48,
    0x8B, 0x49, 0x30, 0x48, 0x8D, 0x54, 0x24, 0x20,
    0xE8, 0xE3, 0xFF, 0xFF, 0xFF, 0x33, 0xC0, 0x48
};

using ConfigurePlayerInventoryFn = void(__fastcall*)(void* panel) noexcept;
using DispatchUiMessageFn = void(__fastcall*)(void* message) noexcept;
using QueueOutgoingPacketFn = void(__fastcall*)(const std::uint8_t* packet, std::int32_t size) noexcept;
using ServerPacketHandlerFn = std::int32_t(__fastcall*)(
    void* game,
    void* player,
    const std::uint8_t* packet,
    std::int32_t size
) noexcept;
using ValidateItemPacketStateFn = bool(__fastcall*)(
    void* transaction,
    const void* packetState,
    bool bypassStashProximity
) noexcept;
using SendServerUiFn = void(__fastcall*)(void* client, std::uint8_t action) noexcept;
using GetClientFromPlayerFn = void*(__fastcall*)(void* player) noexcept;
using IsRoomInTownFn = std::int32_t(__fastcall*)(void* room) noexcept;
using TransferItemToInventoryPageFn = bool(__fastcall*)(
    void* item,
    void* destinationUnit,
    std::uint8_t inventoryPage,
    std::uint8_t destinationKind,
    bool transferMode,
    void* placementOut
) noexcept;
using GetUiStateFn = std::uint8_t(__fastcall*)(std::int32_t state) noexcept;
using FindWidgetFn = void*(__fastcall*)(void* panel, const char* name) noexcept;
using GetWidgetRectFn = WidgetRect*(__fastcall*)(
    void* widget,
    WidgetRect* rectOut
) noexcept;
using SetWidgetBoolFn = void(__fastcall*)(void* widget, bool value) noexcept;
using UiMessageInterceptorFn = bool(__fastcall*)(void* message) noexcept;
using RegisterUiMessageInterceptorFn = bool(__cdecl*)(UiMessageInterceptorFn) noexcept;
using UnregisterUiMessageInterceptorFn = void(__cdecl*)(UiMessageInterceptorFn) noexcept;

constexpr std::array<const wchar_t*, 2> UiMessageBrokerModules{
    L"plugin-skills.dll",
    L"BulkSkillPointAllocation.dll",
};
constexpr char RegisterUiMessageInterceptorExport[] =
    "RuffneckkRegisterUiMessageInterceptor";
constexpr char UnregisterUiMessageInterceptorExport[] =
    "RuffneckkUnregisterUiMessageInterceptor";

const D2RL::PluginContext* Context{};
std::uint8_t* Base{};
ConfigurePlayerInventoryFn OriginalConfigurePlayerInventory{};
DispatchUiMessageFn OriginalDispatchUiMessage{};
QueueOutgoingPacketFn QueueOutgoingPacket{};
ServerPacketHandlerFn OriginalRemoveItemHandler{};
ServerPacketHandlerFn OriginalInsertItemHandler{};
ServerPacketHandlerFn OriginalSharedDepositHandler{};
ServerPacketHandlerFn OriginalSharedWithdrawalHandler{};
ValidateItemPacketStateFn OriginalValidateItemPacketState{};
ServerPacketHandlerFn OriginalGoldButtonHandler{};
SendServerUiFn SendServerUi{};
GetClientFromPlayerFn GetClientFromPlayer{};
IsRoomInTownFn OriginalIsRoomInTown{};
TransferItemToInventoryPageFn OriginalTransferItemToInventoryPage{};
GetUiStateFn OriginalGetUiState{};
FindWidgetFn FindWidget{};
GetWidgetRectFn GetWidgetRect{};

std::atomic<std::uint64_t> DynamicPlacements{};
std::atomic<std::uint64_t> PlacementFailures{};
std::atomic<std::uint64_t> OpenRequests{};
std::atomic<std::uint64_t> ServerSessionsOpened{};
std::atomic<std::uint64_t> RemoteItemOperations{};
std::atomic<std::uint64_t> RemoteItemFailures{};
std::atomic<std::uint64_t> RemoteStashProximityBypasses{};
std::atomic<std::uint64_t> RemoteTownBypasses{};
std::atomic<std::uint64_t> RemoteSharedTransferOperations{};
std::atomic<std::uint64_t> RemoteSharedTransferFailures{};
std::atomic<std::uint64_t> RemoteGoldTransactions{};
std::atomic<std::uint64_t> RemoteGoldFailures{};
std::atomic<std::uint64_t> RemoteQuickMoveUiBypasses{};
std::atomic<std::uint64_t> RemoteQuickMoveWithdrawalDeadline{};
constexpr std::size_t TownProbeCapacity = 256;
std::array<std::atomic<std::uintptr_t>, TownProbeCapacity> TownProbeCallsites{};
std::atomic_bool PlacementSuccessReported{};
std::atomic_bool PlacementFailureReported{};
std::atomic<void*> InventoryPanel{};
std::atomic<void*> RemoteStashButton{};
std::atomic_bool UsingUiMessageBroker{};
UnregisterUiMessageInterceptorFn BrokerUnregister{};
std::atomic<UiMessageInterceptorFn> ExternalUiMessageInterceptor{};
std::atomic_bool BrokerReady{};

struct RemoteSession {
    void* game{};
};

std::mutex RemoteSessionsMutex;
std::unordered_map<void*, RemoteSession> RemoteSessions;
std::atomic_bool RemoteClientSessionActive{};
thread_local bool RemoteItemScope{};
thread_local bool RemoteGoldScope{};
void* GoldRangeStub{};
void* GoldRangeTrampoline{};

constexpr std::array<std::uint8_t, 17> RemoteOpenRequest{
    0x18,
    0x52, 0x53, 0x54, 0x41,
    0x53, 0x48, 0x52, 0x55,
    0x46, 0x46, 0x4E, 0x45,
    0x43, 0x4B, 0x4B, 0x21,
};
constexpr std::int32_t CloseStashAction = 18;
constexpr std::int32_t WithdrawStashGoldAction = 19;
constexpr std::int32_t DepositStashGoldAction = 20;
constexpr std::uint8_t OpenStashUiAction = 0x10;

constexpr D2RL::PluginInfo Info{
    .infoSize = D2RL::PluginInfoSize,
    .apiVersion = D2RL_PLUGIN_API_VERSION,
    .id = "remote-stash",
    .name = "Remote Stash",
    .version = "0.2.24",
    .author = "RuffnecKk",
    .description = "Opens the player stash from the inventory screen.",
    .flags = D2RL::PluginFlags::NativeHooks,
};

template<class T>
T At(std::uintptr_t rva) noexcept {
    return reinterpret_cast<T>(Base + rva);
}

template<std::size_t Size>
bool Matches(
    std::uintptr_t rva,
    const std::array<std::uint8_t, Size>& expected
) noexcept {
    return std::memcmp(Base + rva, expected.data(), Size) == 0;
}

bool IsExecutableAddress(const void* address) noexcept {
    if (!address) return false;
    MEMORY_BASIC_INFORMATION region{};
    if (VirtualQuery(address, &region, sizeof(region)) == 0
        || region.State != MEM_COMMIT) {
        return false;
    }
    const auto protection = region.Protect & 0xFF;
    return protection == PAGE_EXECUTE
        || protection == PAGE_EXECUTE_READ
        || protection == PAGE_EXECUTE_READWRITE
        || protection == PAGE_EXECUTE_WRITECOPY;
}

bool ValidateRuntime() noexcept {
    return Matches(ConfigurePlayerInventoryRva, ConfigurePlayerInventoryExpected)
        && Matches(QueueOutgoingPacketRva, QueueOutgoingPacketExpected)
        && Matches(RemoveItemHandlerRva, RemoveItemHandlerExpected)
        && Matches(InsertItemHandlerRva, InsertItemHandlerExpected)
        && Matches(SharedDepositHandlerRva, SharedDepositHandlerExpected)
        && Matches(SharedWithdrawalHandlerRva, SharedWithdrawalHandlerExpected)
        && Matches(ValidateItemPacketStateRva, ValidateItemPacketStateExpected)
        && Matches(GoldButtonHandlerRva, GoldButtonHandlerExpected)
        && Matches(GoldRangeGateRva, GoldRangeGateExpected)
        && Matches(SendServerUiRva, SendServerUiExpected)
        && Matches(GetClientFromPlayerRva, GetClientFromPlayerExpected)
        && Matches(IsRoomInTownRva, IsRoomInTownExpected)
        && Matches(
            TransferItemToInventoryPageRva,
            TransferItemToInventoryPageExpected
        )
        && Matches(GetUiStateRva, GetUiStateExpected)
        && Matches(FindWidgetRva, FindWidgetExpected)
        && Matches(GetWidgetRectRva, GetWidgetRectExpected);
}

bool IsRemoteOpenRequest(
    const std::uint8_t* packet,
    std::int32_t size
) noexcept {
    return packet
        && size == static_cast<std::int32_t>(RemoteOpenRequest.size())
        && std::memcmp(packet, RemoteOpenRequest.data(), RemoteOpenRequest.size()) == 0;
}

bool HasRemoteSession(void* game, void* player) noexcept {
    if (!game || !player) return false;
    try {
        const std::lock_guard lock(RemoteSessionsMutex);
        const auto found = RemoteSessions.find(player);
        return found != RemoteSessions.end() && found->second.game == game;
    } catch (...) {
        return false;
    }
}

void CloseRemoteSession(void* game, void* player) noexcept {
    if (!game || !player) return;
    try {
        const std::lock_guard lock(RemoteSessionsMutex);
        const auto found = RemoteSessions.find(player);
        if (found != RemoteSessions.end() && found->second.game == game) {
            RemoteSessions.erase(found);
        }
    } catch (...) {
    }
}

std::size_t RemoteSessionCount() noexcept {
    try {
        const std::lock_guard lock(RemoteSessionsMutex);
        return RemoteSessions.size();
    } catch (...) {
        return 0;
    }
}

bool TrySendOpenStashUi(void* player) noexcept {
    if (!player || !GetClientFromPlayer || !SendServerUi) return false;
    __try {
        auto* client = GetClientFromPlayer(player);
        if (!client) return false;
        SendServerUi(client, OpenStashUiAction);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool OpenRemoteSession(void* game, void* player) noexcept {
    if (!game || !player) return false;
    try {
        const std::lock_guard lock(RemoteSessionsMutex);
        RemoteSessions.insert_or_assign(player, RemoteSession{game});
    } catch (...) {
        return false;
    }

    if (!TrySendOpenStashUi(player)) {
        CloseRemoteSession(game, player);
        return false;
    }

    const auto count = ServerSessionsOpened.fetch_add(1, std::memory_order_relaxed) + 1;
    if (Context) {
        char message[190]{};
        std::snprintf(
            message,
            sizeof(message),
            "RemoteStash: authoritative server session opened (sessionsOpened=%llu).",
            static_cast<unsigned long long>(count)
        );
        Context->LogInfo(message);
    }
    return true;
}

void ReportRemoteItemOperation(
    const char* operation,
    const std::uint8_t* packet,
    std::int32_t size,
    std::int32_t result,
    std::uint64_t proximityBypassesBefore
) noexcept {
    RemoteItemOperations.fetch_add(1, std::memory_order_relaxed);
    if (result != 0) {
        RemoteItemFailures.fetch_add(1, std::memory_order_relaxed);
    }
    if (!Context) return;

    std::uint32_t itemGuid{};
    std::int32_t page{-1};
    if (packet && size >= 5) {
        std::memcpy(&itemGuid, packet + 1, sizeof(itemGuid));
    }
    if (packet && size >= 17) {
        std::memcpy(&page, packet + 13, sizeof(page));
    }
    const auto proximityBypassesAfter =
        RemoteStashProximityBypasses.load(std::memory_order_relaxed);
    char message[240]{};
    std::snprintf(
        message,
        sizeof(message),
        "RemoteStash: remote %s forwarded (opcode=0x%02X, item=%u, page=%d, "
        "result=%d, scopedStashProximityBypasses=%llu).",
        operation,
        packet && size > 0 ? packet[0] : 0,
        itemGuid,
        page,
        result,
        static_cast<unsigned long long>(
            proximityBypassesAfter - proximityBypassesBefore
        )
    );
    Context->LogInfo(message);
}

std::int32_t __fastcall HookInsertItemHandler(
    void* game,
    void* player,
    const std::uint8_t* packet,
    std::int32_t size
) noexcept {
    if (IsRemoteOpenRequest(packet, size)) {
        if (OpenRemoteSession(game, player)) return 0;
        if (Context) {
            Context->LogError("RemoteStash: server could not open the remote session.");
        }
        return 1;
    }

    const auto remote = HasRemoteSession(game, player);
    const auto bypassesBefore =
        RemoteStashProximityBypasses.load(std::memory_order_relaxed);
    const auto previousScope = RemoteItemScope;
    RemoteItemScope = remote;
    const auto result = OriginalInsertItemHandler(game, player, packet, size);
    RemoteItemScope = previousScope;
    if (remote) {
        ReportRemoteItemOperation("insert", packet, size, result, bypassesBefore);
    }
    return result;
}

std::int32_t __fastcall HookRemoveItemHandler(
    void* game,
    void* player,
    const std::uint8_t* packet,
    std::int32_t size
) noexcept {
    const auto remote = HasRemoteSession(game, player);
    const auto bypassesBefore =
        RemoteStashProximityBypasses.load(std::memory_order_relaxed);
    const auto previousScope = RemoteItemScope;
    RemoteItemScope = remote;
    const auto result = OriginalRemoveItemHandler(game, player, packet, size);
    RemoteItemScope = previousScope;
    if (remote) {
        ReportRemoteItemOperation("remove", packet, size, result, bypassesBefore);
    }
    return result;
}

std::int32_t ForwardSharedTransfer(
    const char* operation,
    ServerPacketHandlerFn original,
    void* game,
    void* player,
    const std::uint8_t* packet,
    std::int32_t size
) noexcept {
    const auto remote = HasRemoteSession(game, player);
    const auto bypassesBefore =
        RemoteStashProximityBypasses.load(std::memory_order_relaxed);
    const auto previousScope = RemoteItemScope;
    RemoteItemScope = remote;
    const auto result = original(game, player, packet, size);
    RemoteItemScope = previousScope;
    if (!remote) return result;

    RemoteSharedTransferOperations.fetch_add(1, std::memory_order_relaxed);
    if (result != 0) {
        RemoteSharedTransferFailures.fetch_add(1, std::memory_order_relaxed);
    }
    if (Context) {
        std::uint32_t itemGuid{};
        std::uint32_t sharedUnitId{};
        std::uint32_t position{};
        if (packet && size >= 13) {
            std::memcpy(&itemGuid, packet + 1, sizeof(itemGuid));
            std::memcpy(&sharedUnitId, packet + 5, sizeof(sharedUnitId));
            std::memcpy(&position, packet + 9, sizeof(position));
        }
        const auto bypassesAfter =
            RemoteStashProximityBypasses.load(std::memory_order_relaxed);
        char message[260]{};
        std::snprintf(
            message,
            sizeof(message),
            "RemoteStash: shared %s forwarded (opcode=0x%02X, size=%d, "
            "item=%u, sharedUnit=%u, position=0x%08X, result=%d, "
            "scopedStashProximityBypasses=%llu).",
            operation,
            packet && size > 0 ? packet[0] : 0,
            size,
            itemGuid,
            sharedUnitId,
            position,
            result,
            static_cast<unsigned long long>(bypassesAfter - bypassesBefore)
        );
        Context->LogInfo(message);
    }
    return result;
}

std::int32_t __fastcall HookSharedDepositHandler(
    void* game,
    void* player,
    const std::uint8_t* packet,
    std::int32_t size
) noexcept {
    return ForwardSharedTransfer(
        "deposit",
        OriginalSharedDepositHandler,
        game,
        player,
        packet,
        size
    );
}

std::int32_t __fastcall HookSharedWithdrawalHandler(
    void* game,
    void* player,
    const std::uint8_t* packet,
    std::int32_t size
) noexcept {
    return ForwardSharedTransfer(
        "withdrawal",
        OriginalSharedWithdrawalHandler,
        game,
        player,
        packet,
        size
    );
}

bool __fastcall HookValidateItemPacketState(
    void* transaction,
    const void* packetState,
    bool bypassStashProximity
) noexcept {
    const auto returnAddress = reinterpret_cast<std::uintptr_t>(_ReturnAddress());
    bool remoteStashState{};
    std::int32_t page{-1};
    if (packetState) {
        __try {
            std::memcpy(
                &page,
                static_cast<const std::uint8_t*>(packetState) + 4,
                sizeof(page)
            );
            remoteStashState = page == 4
                && (RemoteItemScope
                    || (RemoteClientSessionActive.load(std::memory_order_acquire)
                        && (returnAddress == reinterpret_cast<std::uintptr_t>(
                                Base + QuickMoveToStashItemPacketStateReturnRva
                            )
                            || returnAddress == reinterpret_cast<std::uintptr_t>(
                                Base + QuickMoveFromStashItemPacketStateReturnRva
                            ))));
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            page = -1;
            remoteStashState = false;
        }
    }

    if (remoteStashState && !bypassStashProximity) {
        RemoteStashProximityBypasses.fetch_add(1, std::memory_order_relaxed);
        bypassStashProximity = true;
    }

    const auto result = OriginalValidateItemPacketState(
        transaction,
        packetState,
        bypassStashProximity
    );
    if (page == 4
        && !RemoteItemScope
        && RemoteClientSessionActive.load(std::memory_order_acquire)
        && Context
        && Base) {
        char message[230]{};
        std::snprintf(
            message,
            sizeof(message),
            "RemoteStash: unscoped page-4 validator probe caller=0x%llX bypass=%d result=%d.",
            static_cast<unsigned long long>(
                returnAddress - reinterpret_cast<std::uintptr_t>(Base)
            ),
            bypassStashProximity ? 1 : 0,
            result ? 1 : 0
        );
        Context->LogInfo(message);
    }
    return result;
}

bool IsRemoteClientStashTownCallsite(std::uintptr_t returnAddress) noexcept {
    if (!RemoteClientSessionActive.load(std::memory_order_acquire)) return false;
    if (returnAddress == reinterpret_cast<std::uintptr_t>(
            Base + ClientStashTownCheckReturnRva
        )
        || returnAddress == reinterpret_cast<std::uintptr_t>(
            Base + ClientStashCleanupTownCheckReturnRva
        )) {
        return true;
    }

    const auto quickMoveWithdrawalActive = GetTickCount64()
        <= RemoteQuickMoveWithdrawalDeadline.load(std::memory_order_acquire);
    return quickMoveWithdrawalActive
        && (returnAddress == reinterpret_cast<std::uintptr_t>(
                Base + QuickMoveItemInitTownCheckReturnRva
            )
            || returnAddress == reinterpret_cast<std::uintptr_t>(
                Base + QuickMoveItemPlacementTownCheckReturnRva
            ));
}

std::int32_t __fastcall HookIsRoomInTown(void* room) noexcept {
    const auto returnAddress = reinterpret_cast<std::uintptr_t>(_ReturnAddress());
    if (IsRemoteClientStashTownCallsite(returnAddress)) {
        RemoteTownBypasses.fetch_add(1, std::memory_order_relaxed);
        return 1;
    }

    const auto result = OriginalIsRoomInTown(room);
    if (!RemoteClientSessionActive.load(std::memory_order_acquire) || result != 0 || !Base) {
        return result;
    }

    for (auto& observed : TownProbeCallsites) {
        const auto known = observed.load(std::memory_order_acquire);
        if (known == returnAddress) break;
        if (known != 0) continue;

        std::uintptr_t empty{};
        if (!observed.compare_exchange_strong(
                empty,
                returnAddress,
                std::memory_order_acq_rel,
                std::memory_order_acquire
            )) {
            continue;
        }

        if (Context) {
            char message[180]{};
            std::snprintf(
                message,
                sizeof(message),
                "RemoteStash: quick-move town probe observed rejected callsite rva=0x%llX.",
                static_cast<unsigned long long>(returnAddress - reinterpret_cast<std::uintptr_t>(Base))
            );
            Context->LogInfo(message);
        }
        break;
    }
    return result;
}

struct UnitProbe {
    std::uint32_t type{0xFFFFFFFF};
    std::uint32_t classId{0xFFFFFFFF};
    std::uint32_t unitId{0xFFFFFFFF};
};

UnitProbe ProbeUnit(const void* unit) noexcept {
    UnitProbe probe{};
    if (!unit) return probe;
    __try {
        const auto* bytes = static_cast<const std::uint8_t*>(unit);
        std::memcpy(&probe.type, bytes, sizeof(probe.type));
        std::memcpy(&probe.classId, bytes + 4, sizeof(probe.classId));
        std::memcpy(&probe.unitId, bytes + 8, sizeof(probe.unitId));
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return UnitProbe{};
    }
    return probe;
}

bool __fastcall HookTransferItemToInventoryPage(
    void* item,
    void* destinationUnit,
    std::uint8_t inventoryPage,
    std::uint8_t destinationKind,
    bool transferMode,
    void* placementOut
) noexcept {
    const auto remoteWithdrawal =
        RemoteClientSessionActive.load(std::memory_order_acquire)
        && inventoryPage == 0
        && destinationKind == 4;
    if (remoteWithdrawal) {
        RemoteQuickMoveWithdrawalDeadline.store(
            GetTickCount64() + 1000,
            std::memory_order_release
        );
    }
    const auto result = OriginalTransferItemToInventoryPage(
        item,
        destinationUnit,
        inventoryPage,
        destinationKind,
        transferMode,
        placementOut
    );
    if (!RemoteClientSessionActive.load(std::memory_order_acquire) || !Context) {
        return result;
    }

    const auto itemProbe = ProbeUnit(item);
    const auto destinationProbe = ProbeUnit(destinationUnit);
    char message[360]{};
    std::snprintf(
        message,
        sizeof(message),
        "RemoteStash: quick-move transfer probe result=%d item=%p(type=%u,class=%u,id=%u) "
        "destination=%p(type=%u,class=%u,id=%u) page=%u kind=%u mode=%d placement=%p.",
        result ? 1 : 0,
        item,
        itemProbe.type,
        itemProbe.classId,
        itemProbe.unitId,
        destinationUnit,
        destinationProbe.type,
        destinationProbe.classId,
        destinationProbe.unitId,
        static_cast<unsigned>(inventoryPage),
        static_cast<unsigned>(destinationKind),
        transferMode ? 1 : 0,
        placementOut
    );
    Context->LogInfo(message);
    return result;
}

std::uint8_t __fastcall HookGetUiState(std::int32_t state) noexcept {
    const auto* returnSlot = static_cast<const std::uint8_t*>(_AddressOfReturnAddress());
    const auto returnAddress = reinterpret_cast<std::uintptr_t>(_ReturnAddress());
    std::uint8_t inventoryPage = 0xFF;
    std::uint8_t destinationKind = 0xFF;
    if (returnAddress == reinterpret_cast<std::uintptr_t>(
            Base + QuickMoveStashUiStateReturnRva
        )) {
        __try {
            inventoryPage = returnSlot[QuickMoveInventoryPageCallerStackOffset];
            destinationKind = returnSlot[QuickMoveDestinationKindCallerStackOffset];
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            inventoryPage = 0xFF;
            destinationKind = 0xFF;
        }
    }
    const auto result = OriginalGetUiState(state);
    if (state == 0x16
        && RemoteClientSessionActive.load(std::memory_order_acquire)
        && returnAddress == reinterpret_cast<std::uintptr_t>(
            Base + QuickMoveStashUiStateReturnRva
        )) {
        const auto count = RemoteQuickMoveUiBypasses.fetch_add(
            1,
            std::memory_order_relaxed
        ) + 1;
        if (Context) {
            char message[240]{};
            std::snprintf(
                message,
                sizeof(message),
                "RemoteStash: native quick-move UI state=%u page=%u kind=%u observations=%llu.",
                static_cast<unsigned>(result),
                static_cast<unsigned>(inventoryPage),
                static_cast<unsigned>(destinationKind),
                static_cast<unsigned long long>(count)
            );
            Context->LogInfo(message);
        }
        return result;
    }
    return result;
}

std::int32_t PacketAction(const std::uint8_t* packet, std::int32_t size) noexcept {
    if (!packet || size < 17) return -1;
    std::int32_t action{};
    std::memcpy(&action, packet + 13, sizeof(action));
    return action;
}

bool IsStashButtonAction(std::int32_t action) noexcept {
    return action >= CloseStashAction && action <= DepositStashGoldAction;
}

bool ShouldBypassGoldRange() noexcept {
    return RemoteGoldScope;
}

std::int32_t __fastcall HookGoldButtonHandler(
    void* game,
    void* player,
    const std::uint8_t* packet,
    std::int32_t size
) noexcept {
    const auto action = PacketAction(packet, size);
    const auto remote = IsStashButtonAction(action) && HasRemoteSession(game, player);
    const auto previousScope = RemoteGoldScope;
    RemoteGoldScope = remote;
    const auto result = OriginalGoldButtonHandler(game, player, packet, size);
    RemoteGoldScope = previousScope;

    if (!remote) return result;
    if (action == WithdrawStashGoldAction || action == DepositStashGoldAction) {
        RemoteGoldTransactions.fetch_add(1, std::memory_order_relaxed);
        if (result != 0) {
            RemoteGoldFailures.fetch_add(1, std::memory_order_relaxed);
        }
    }
    if (action == CloseStashAction) {
        CloseRemoteSession(game, player);
        RemoteClientSessionActive.store(false, std::memory_order_release);
    }
    return result;
}

bool CreateGoldRangeStub() noexcept {
    constexpr std::size_t StubSize = 50;
    constexpr std::size_t HelperAddressOffset = 6;
    constexpr std::size_t BypassAddressOffset = 26;
    constexpr std::size_t TrampolineAddressOffset = 39;

    GoldRangeStub = VirtualAlloc(
        nullptr,
        StubSize,
        MEM_RESERVE | MEM_COMMIT,
        PAGE_EXECUTE_READWRITE
    );
    if (!GoldRangeStub) return false;

    std::array<std::uint8_t, StubSize> stub{
        0x48, 0x83, 0xEC, 0x20,
        0x48, 0xB8, 0, 0, 0, 0, 0, 0, 0, 0,
        0xFF, 0xD0,
        0x48, 0x83, 0xC4, 0x20,
        0x84, 0xC0,
        0x74, 0x0D,
        0x49, 0xBB, 0, 0, 0, 0, 0, 0, 0, 0,
        0x41, 0xFF, 0xE3,
        0x49, 0xBB, 0, 0, 0, 0, 0, 0, 0, 0,
        0x41, 0xFF, 0xE3,
    };
    const auto helper = reinterpret_cast<std::uintptr_t>(&ShouldBypassGoldRange);
    const auto bypass = reinterpret_cast<std::uintptr_t>(Base + GoldRangeBypassRva);
    std::memcpy(stub.data() + HelperAddressOffset, &helper, sizeof(helper));
    std::memcpy(stub.data() + BypassAddressOffset, &bypass, sizeof(bypass));
    std::memcpy(GoldRangeStub, stub.data(), stub.size());

    if (!Context->InstallInlineHook(
            GoldRangeGateRva,
            GoldRangeGateExpected.data(),
            static_cast<std::uint32_t>(GoldRangeGateExpected.size()),
            GoldRangeStub,
            &GoldRangeTrampoline
        ) || !GoldRangeTrampoline) {
        VirtualFree(GoldRangeStub, 0, MEM_RELEASE);
        GoldRangeStub = nullptr;
        GoldRangeTrampoline = nullptr;
        return false;
    }

    const auto trampoline = reinterpret_cast<std::uintptr_t>(GoldRangeTrampoline);
    std::memcpy(
        static_cast<std::uint8_t*>(GoldRangeStub) + TrampolineAddressOffset,
        &trampoline,
        sizeof(trampoline)
    );
    FlushInstructionCache(GetCurrentProcess(), GoldRangeStub, StubSize);

    DWORD previousProtection{};
    if (!VirtualProtect(GoldRangeStub, StubSize, PAGE_EXECUTE_READ, &previousProtection)) {
        return false;
    }
    return true;
}

void* FindNamedWidget(void* panel, const char* name) noexcept {
    if (!panel || !name) return nullptr;
    __try {
        return FindWidget(panel, name);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

bool ReadWidgetRect(void* widget, WidgetRect& rect) noexcept {
    if (!widget) return false;
    __try {
        WidgetRect current{};
        if (GetWidgetRect(widget, &current) != &current) return false;
        rect = current;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool WriteWidgetPosition(
    void* widget,
    std::int32_t x,
    std::int32_t y
) noexcept {
    if (!widget) return false;
    __try {
        auto* rect = reinterpret_cast<WidgetRect*>(
            static_cast<std::uint8_t*>(widget) + WidgetRectOffset
        );
        rect->x = x;
        rect->y = y;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

void SetWidgetState(void* widget, bool value) noexcept {
    if (!widget) return;
    __try {
        auto** vtable = *reinterpret_cast<void***>(widget);
        auto setEnabled = reinterpret_cast<SetWidgetBoolFn>(vtable[9]);
        auto setVisible = reinterpret_cast<SetWidgetBoolFn>(vtable[10]);
        setEnabled(widget, value);
        setVisible(widget, value);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        if (Context) {
            Context->LogError("RemoteStash: button state update failed.");
        }
    }
}

void ReportPlacementFailure(const char* reason) noexcept {
    PlacementFailures.fetch_add(1, std::memory_order_relaxed);
    if (!Context || PlacementFailureReported.exchange(true, std::memory_order_relaxed)) {
        return;
    }
    char message[240]{};
    std::snprintf(
        message,
        sizeof(message),
        "RemoteStash: dynamic placement failed (%s); button stays hidden.",
        reason
    );
    Context->LogWarn(message);
}

void __fastcall HookConfigurePlayerInventory(void* panel) noexcept {
    OriginalConfigurePlayerInventory(panel);
    InventoryPanel.store(nullptr, std::memory_order_release);
    RemoteStashButton.store(nullptr, std::memory_order_release);
    if (!panel) return;

    auto* button = FindNamedWidget(panel, "remote_stash");
    if (!button) {
        ReportPlacementFailure("remote_stash was not found");
        return;
    }

    WidgetRect panelRect{};
    WidgetRect gridRect{};
    WidgetRect goldButtonRect{};
    WidgetRect goldAmountRect{};
    WidgetRect buttonRect{};
    const auto panelOk = ReadWidgetRect(panel, panelRect);
    const auto gridOk = ReadWidgetRect(FindNamedWidget(panel, "grid"), gridRect);
    const auto goldButtonOk = ReadWidgetRect(
        FindNamedWidget(panel, "gold_button"),
        goldButtonRect
    );
    const auto goldAmountOk = ReadWidgetRect(
        FindNamedWidget(panel, "gold_amount"),
        goldAmountRect
    );
    const auto buttonOk = ReadWidgetRect(button, buttonRect);

    if (!panelOk || !HasUsableSize(panelRect)) {
        SetWidgetState(button, false);
        ReportPlacementFailure("panel rectangle is unavailable");
        return;
    }
    if (!gridOk || !buttonOk || (!goldButtonOk && !goldAmountOk)) {
        SetWidgetState(button, false);
        ReportPlacementFailure("required runtime rectangles are unavailable");
        return;
    }

    // Child rectangles are panel-local even when the top-level panel is screen-anchored.
    panelRect.x = 0;
    panelRect.y = 0;
    const auto placement = PlaceDesktopFooterLeft(
        panelRect,
        gridRect,
        goldButtonRect,
        goldAmountRect,
        buttonRect
    );
    if (!placement.valid
        || !WriteWidgetPosition(button, placement.rect.x, placement.rect.y)) {
        SetWidgetState(button, false);
        ReportPlacementFailure("computed position is unsafe for this layout");
        return;
    }

    SetWidgetState(button, true);
    InventoryPanel.store(panel, std::memory_order_release);
    RemoteStashButton.store(button, std::memory_order_release);
    DynamicPlacements.fetch_add(1, std::memory_order_relaxed);
    if (Context && !PlacementSuccessReported.exchange(true, std::memory_order_relaxed)) {
        char message[220]{};
        std::snprintf(
            message,
            sizeof(message),
            "RemoteStash: button placed at %d,%d from grid %d,%d and footer %d,%d.",
            placement.rect.x,
            placement.rect.y,
            gridRect.x,
            gridRect.y,
            goldButtonRect.x,
            goldButtonRect.y
        );
        Context->LogInfo(message);
    }
}

bool IsCurrentRemoteStashMessage(void* message) noexcept {
    if (!message) return false;
    const auto inventoryPanel = InventoryPanel.load(std::memory_order_acquire);
    const auto remoteStashButton = RemoteStashButton.load(std::memory_order_acquire);
    if (!inventoryPanel || !remoteStashButton) return false;

    const auto* expectedMessage = static_cast<const std::uint8_t*>(remoteStashButton)
        + ButtonOnClickMessageOffset;
    if (message != expectedMessage) return false;

    __try {
        return FindWidget(inventoryPanel, "remote_stash") == remoteStashButton;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool __fastcall InterceptUiMessage(void* message) noexcept {
    if (!IsCurrentRemoteStashMessage(message)) return false;

    __try {
        RemoteClientSessionActive.store(true, std::memory_order_release);
        QueueOutgoingPacket(
            RemoteOpenRequest.data(),
            static_cast<std::int32_t>(RemoteOpenRequest.size())
        );
        const auto count = OpenRequests.fetch_add(1, std::memory_order_relaxed) + 1;
        if (Context) {
            char message[190]{};
            std::snprintf(
                message,
                sizeof(message),
                "RemoteStash: remote button message consumed; server open request "
                "queued (requests=%llu).",
                static_cast<unsigned long long>(count)
            );
            Context->LogInfo(message);
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        RemoteClientSessionActive.store(false, std::memory_order_release);
        if (Context) {
            Context->LogError("RemoteStash: server open request could not be queued.");
        }
    }
    return true;
}

bool TryExternalUiMessageInterceptor(void* message) noexcept {
    const auto interceptor = ExternalUiMessageInterceptor.load(std::memory_order_acquire);
    if (!interceptor) return false;
    __try {
        return interceptor(message);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool __fastcall InterceptUiMessageChain(void* message) noexcept {
    return TryExternalUiMessageInterceptor(message) || InterceptUiMessage(message);
}

void __fastcall HookDispatchUiMessage(void* message) noexcept {
    if (InterceptUiMessageChain(message)) return;
    OriginalDispatchUiMessage(message);
}

auto Status(D2R::Game::Client*, const D2RL::ConsoleCommandContext* command, void*) noexcept
    -> D2RL::ConsoleCommandResult {
    if (!command || !command->plugin) return D2RL::ConsoleCommandResult::Failed;
    char message[640]{};
    std::snprintf(
        message,
        sizeof(message),
        "RemoteStash 0.2.24: placements=%llu; placementFailures=%llu; "
        "clientOpenRequests=%llu; sessionsOpened=%llu; activeSessions=%llu; "
        "remoteItemOps=%llu; remoteItemFailures=%llu; remoteGoldOps=%llu; "
        "remoteGoldFailures=%llu; stashProximityBypasses=%llu; "
        "clientTownBypasses=%llu; sharedTransferOps=%llu; "
        "sharedTransferFailures=%llu; quickMoveUiBypasses=%llu.",
        static_cast<unsigned long long>(DynamicPlacements.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(PlacementFailures.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(OpenRequests.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(ServerSessionsOpened.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(RemoteSessionCount()),
        static_cast<unsigned long long>(RemoteItemOperations.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(RemoteItemFailures.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(RemoteGoldTransactions.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(RemoteGoldFailures.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(
            RemoteStashProximityBypasses.load(std::memory_order_relaxed)
        ),
        static_cast<unsigned long long>(RemoteTownBypasses.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(
            RemoteSharedTransferOperations.load(std::memory_order_relaxed)
        ),
        static_cast<unsigned long long>(
            RemoteSharedTransferFailures.load(std::memory_order_relaxed)
        ),
        static_cast<unsigned long long>(
            RemoteQuickMoveUiBypasses.load(std::memory_order_relaxed)
        )
    );
    command->plugin->WriteConsoleMessage(message);
    return D2RL::ConsoleCommandResult::Handled;
}
} // namespace

extern "C" __declspec(dllexport) bool __cdecl RuffneckkRegisterUiMessageInterceptor(
    UiMessageInterceptorFn interceptor
) noexcept {
    if (!interceptor || !BrokerReady.load(std::memory_order_acquire)) return false;
    UiMessageInterceptorFn expected{};
    return ExternalUiMessageInterceptor.compare_exchange_strong(
        expected,
        interceptor,
        std::memory_order_acq_rel,
        std::memory_order_acquire
    );
}

extern "C" __declspec(dllexport) void __cdecl RuffneckkUnregisterUiMessageInterceptor(
    UiMessageInterceptorFn interceptor
) noexcept {
    if (!interceptor) return;
    auto expected = interceptor;
    ExternalUiMessageInterceptor.compare_exchange_strong(
        expected,
        nullptr,
        std::memory_order_acq_rel,
        std::memory_order_acquire
    );
}

D2RL_PLUGIN_EXPORT auto D2RLoaderGetPluginInfo() noexcept -> const D2RL::PluginInfo* {
    return &Info;
}

D2RL_PLUGIN_EXPORT auto D2RLoaderLoadPlugin(const D2RL::PluginContext* context) noexcept
    -> bool {
    if (!context) return false;
    Context = context;
    Base = reinterpret_cast<std::uint8_t*>(GetModuleHandleW(nullptr));
    DynamicPlacements.store(0, std::memory_order_relaxed);
    PlacementFailures.store(0, std::memory_order_relaxed);
    OpenRequests.store(0, std::memory_order_relaxed);
    ServerSessionsOpened.store(0, std::memory_order_relaxed);
    RemoteItemOperations.store(0, std::memory_order_relaxed);
    RemoteItemFailures.store(0, std::memory_order_relaxed);
    RemoteStashProximityBypasses.store(0, std::memory_order_relaxed);
    RemoteTownBypasses.store(0, std::memory_order_relaxed);
    RemoteSharedTransferOperations.store(0, std::memory_order_relaxed);
    RemoteSharedTransferFailures.store(0, std::memory_order_relaxed);
    RemoteGoldTransactions.store(0, std::memory_order_relaxed);
    RemoteGoldFailures.store(0, std::memory_order_relaxed);
    RemoteQuickMoveUiBypasses.store(0, std::memory_order_relaxed);
    for (auto& observed : TownProbeCallsites) {
        observed.store(0, std::memory_order_relaxed);
    }
    PlacementSuccessReported.store(false, std::memory_order_relaxed);
    PlacementFailureReported.store(false, std::memory_order_relaxed);
    InventoryPanel.store(nullptr, std::memory_order_relaxed);
    RemoteStashButton.store(nullptr, std::memory_order_relaxed);
    UsingUiMessageBroker.store(false, std::memory_order_relaxed);
    BrokerUnregister = nullptr;
    ExternalUiMessageInterceptor.store(nullptr, std::memory_order_relaxed);
    BrokerReady.store(false, std::memory_order_relaxed);
    RemoteClientSessionActive.store(false, std::memory_order_relaxed);
    RemoteItemScope = false;
    RemoteGoldScope = false;
    GoldRangeStub = nullptr;
    GoldRangeTrampoline = nullptr;
    try {
        const std::lock_guard lock(RemoteSessionsMutex);
        RemoteSessions.clear();
    } catch (...) {
        return false;
    }

    if (!Base) return false;
    if (context->modDataVersionBuild != 0
        && context->modDataVersionBuild != SupportedBuild) {
        context->LogError("RemoteStash: only D2R build 92777 is supported.");
        return false;
    }
    if (!ValidateRuntime()) {
        context->LogError("RemoteStash: 92777 native signature mismatch; plugin refused.");
        return false;
    }

    FindWidget = At<FindWidgetFn>(FindWidgetRva);
    GetWidgetRect = At<GetWidgetRectFn>(GetWidgetRectRva);
    SendServerUi = At<SendServerUiFn>(SendServerUiRva);
    GetClientFromPlayer = At<GetClientFromPlayerFn>(GetClientFromPlayerRva);
    QueueOutgoingPacket = At<QueueOutgoingPacketFn>(QueueOutgoingPacketRva);
    for (const auto* brokerName : UiMessageBrokerModules) {
        const auto brokerModule = GetModuleHandleW(brokerName);
        if (!brokerModule) continue;
        const auto registerBroker = reinterpret_cast<RegisterUiMessageInterceptorFn>(
            GetProcAddress(brokerModule, RegisterUiMessageInterceptorExport)
        );
        const auto unregisterBroker = reinterpret_cast<UnregisterUiMessageInterceptorFn>(
            GetProcAddress(brokerModule, UnregisterUiMessageInterceptorExport)
        );
        if (registerBroker && unregisterBroker && registerBroker(InterceptUiMessageChain)) {
            BrokerUnregister = unregisterBroker;
            UsingUiMessageBroker.store(true, std::memory_order_release);
            break;
        }
    }
    if (!UsingUiMessageBroker.load(std::memory_order_acquire)
        && !Matches(DispatchUiMessageRva, DispatchUiMessageExpected)) {
        context->LogError(
            "RemoteStash: UI-message dispatcher is already owned and exposes no "
            "compatible broker; plugin refused."
        );
        return false;
    }

    if (!context->InstallInlineHook(
            ConfigurePlayerInventoryRva,
            ConfigurePlayerInventoryExpected.data(),
            static_cast<std::uint32_t>(ConfigurePlayerInventoryExpected.size()),
            HookConfigurePlayerInventory,
            &OriginalConfigurePlayerInventory
        )) {
        if (BrokerUnregister) BrokerUnregister(InterceptUiMessageChain);
        BrokerUnregister = nullptr;
        UsingUiMessageBroker.store(false, std::memory_order_release);
        context->LogError("RemoteStash: inventory-panel hook failed.");
        return false;
    }
    if (!UsingUiMessageBroker.load(std::memory_order_acquire)
        && !context->InstallInlineHook(
            DispatchUiMessageRva,
            DispatchUiMessageExpected.data(),
            static_cast<std::uint32_t>(DispatchUiMessageExpected.size()),
            HookDispatchUiMessage,
            &OriginalDispatchUiMessage
        )) {
        context->LogError("RemoteStash: UI-message dispatch hook failed.");
        return false;
    }
    BrokerReady.store(true, std::memory_order_release);

    if (!CreateGoldRangeStub()) {
        context->LogError("RemoteStash: scoped stash-range hook failed.");
        return false;
    }
    if (!context->InstallInlineHook(
            IsRoomInTownRva,
            IsRoomInTownExpected.data(),
            static_cast<std::uint32_t>(IsRoomInTownExpected.size()),
            HookIsRoomInTown,
            &OriginalIsRoomInTown
        )) {
        context->LogError("RemoteStash: scoped town-state hook failed.");
        return false;
    }
    if (!context->InstallInlineHook(
            TransferItemToInventoryPageRva,
            TransferItemToInventoryPageExpected.data(),
            static_cast<std::uint32_t>(TransferItemToInventoryPageExpected.size()),
            HookTransferItemToInventoryPage,
            &OriginalTransferItemToInventoryPage
        )) {
        context->LogError("RemoteStash: quick-move transfer probe hook failed.");
        return false;
    }
    if (!context->InstallInlineHook(
            GetUiStateRva,
            GetUiStateExpected.data(),
            static_cast<std::uint32_t>(GetUiStateExpected.size()),
            HookGetUiState,
            &OriginalGetUiState
        )) {
        context->LogError("RemoteStash: scoped quick-move UI-state hook failed.");
        return false;
    }
    if (!context->InstallInlineHook(
            GoldButtonHandlerRva,
            GoldButtonHandlerExpected.data(),
            static_cast<std::uint32_t>(GoldButtonHandlerExpected.size()),
            HookGoldButtonHandler,
            &OriginalGoldButtonHandler
        )) {
        context->LogError("RemoteStash: stash gold-button hook failed.");
        return false;
    }
    if (!context->InstallInlineHook(
            RemoveItemHandlerRva,
            RemoveItemHandlerExpected.data(),
            static_cast<std::uint32_t>(RemoveItemHandlerExpected.size()),
            HookRemoveItemHandler,
            &OriginalRemoveItemHandler
        )) {
        context->LogError("RemoteStash: stash item-removal hook failed.");
        return false;
    }
    if (!context->InstallInlineHook(
            ValidateItemPacketStateRva,
            ValidateItemPacketStateExpected.data(),
            static_cast<std::uint32_t>(ValidateItemPacketStateExpected.size()),
            HookValidateItemPacketState,
            &OriginalValidateItemPacketState
        )) {
        context->LogError("RemoteStash: scoped stash-proximity validator hook failed.");
        return false;
    }
    if (!context->InstallInlineHook(
            SharedDepositHandlerRva,
            SharedDepositHandlerExpected.data(),
            static_cast<std::uint32_t>(SharedDepositHandlerExpected.size()),
            HookSharedDepositHandler,
            &OriginalSharedDepositHandler
        )) {
        context->LogError("RemoteStash: shared-deposit handler hook failed.");
        return false;
    }
    if (!context->InstallInlineHook(
            SharedWithdrawalHandlerRva,
            SharedWithdrawalHandlerExpected.data(),
            static_cast<std::uint32_t>(SharedWithdrawalHandlerExpected.size()),
            HookSharedWithdrawalHandler,
            &OriginalSharedWithdrawalHandler
        )) {
        context->LogError("RemoteStash: shared-withdrawal handler hook failed.");
        return false;
    }
    if (!context->InstallInlineHook(
            InsertItemHandlerRva,
            InsertItemHandlerExpected.data(),
            static_cast<std::uint32_t>(InsertItemHandlerExpected.size()),
            HookInsertItemHandler,
            &OriginalInsertItemHandler
        )) {
        context->LogError("RemoteStash: authoritative open-request hook failed.");
        return false;
    }

    if (!context->RegisterConsoleCommand(
            "remote-stash",
            Status,
            "Show Remote Stash status and counters."
        )) {
        context->LogWarn("RemoteStash: status command could not be registered.");
    }

    context->LogInfo(
        UsingUiMessageBroker.load(std::memory_order_acquire)
            ? "RemoteStash 0.2.24 diagnostic active for D2R 3.2.92777; the existing dynamic "
              "desktop placement uses the shared PluginPack UI dispatcher and remote "
              "stash sessions bypass native proximity checks."
            : "RemoteStash 0.2.24 diagnostic active for D2R 3.2.92777; the existing dynamic "
              "desktop placement uses the standalone UI dispatcher and remote stash "
              "sessions bypass native proximity checks."
    );
    return true;
}

D2RL_PLUGIN_EXPORT void D2RLoaderUnloadPlugin() noexcept {
    BrokerReady.store(false, std::memory_order_release);
    ExternalUiMessageInterceptor.store(nullptr, std::memory_order_release);
    if (UsingUiMessageBroker.exchange(false, std::memory_order_acq_rel)) {
        if (BrokerUnregister) {
            BrokerUnregister(InterceptUiMessageChain);
        }
    }
    BrokerUnregister = nullptr;
    RemoteClientSessionActive.store(false, std::memory_order_release);
    RemoteItemScope = false;
    RemoteGoldScope = false;
    try {
        const std::lock_guard lock(RemoteSessionsMutex);
        RemoteSessions.clear();
    } catch (...) {
    }
    Context = nullptr;
}
