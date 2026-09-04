#define NOMINMAX

#include <D2RLPlugin/api.h>

#include "extended_act_level_ids_policy.hpp"

#include <Windows.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <charconv>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace RuffnecKk::ExtendedActLevelIds {
namespace {

using namespace ruffneckk::extended_act_level_ids;

constexpr wchar_t SingletonName[] =
    L"Local\\RuffnecKk.ExtendedActLevelIds.Singleton";

constexpr std::uintptr_t ResolveActFromLevelIdRva = 0x326710;
constexpr std::uintptr_t LevelsCapacityGuardRva = 0x330446;
constexpr std::uintptr_t SendRoomInSightPacketRva = 0x47D2D0;
constexpr std::uintptr_t SendRoomOutOfSightPacketRva = 0x47EAF0;
constexpr std::uintptr_t SetClientInSightRva = 0x328680;
constexpr std::uintptr_t UnsetClientInSightRva = 0x328780;
constexpr std::uintptr_t D2ClientLayoutWitnessRva = 0x485B51;
constexpr std::size_t D2ClientPlayerIdOffset = 0x270;
constexpr std::uint32_t InvalidPlayerId = 0xFFFFFFFFU;
constexpr std::uint16_t CompatibilityHandshakeMessage = 1;
constexpr std::uint16_t NetworkLocalChannelId = 1;
constexpr std::uint64_t NetworkCompatibilityToken = 0x454C494456320001ULL;
constexpr auto ResolveActFromLevelIdExpected = std::to_array<std::uint8_t>({
    0x48,0x89,0x5C,0x24,0x08,0x48,0x89,0x74,
    0x24,0x10,0x57,0x48,0x83,0xEC,0x30,0x8B,
    0xF2,0x0F,0xB6,0xD9,0xE8,0xB7,0x9F,0xFD,
    0xFF,0x0F,0xB6,0xCB,0x48,0x8B,0xF8,0xE8,
    0x5C,0xA3,0xFD,0xFF,0x8B,0x98,0x08,0x01,
    0x00,0x00,0x83,0xEB,0x01,0x78,0x43,0x90,
});
constexpr auto LevelsCapacityGuardExpected = std::to_array<std::uint8_t>({
    0x48,0x81,0xBE,0x68,0x14,0x00,0x00,0x00,
    0x04,0x00,0x00,0x72,0x0F,0x48,0x8D,0x4C,
    0x24,0x40,0xE8,0x83,0x9F,0xFF,0xFF,0x84,
    0xC0,0x74,0x01,0xCC,0x4C,0x69,0x7F,0x08,
    0x8C,0x01,0x00,0x00,
});
constexpr auto SendRoomInSightPacketExpected = std::to_array<std::uint8_t>({
    0x40,0x57,0x48,0x83,0xEC,0x40,0xC6,0x44,
    0x24,0x20,0x07,0x48,0x8B,0xF9,0x66,0x44,
    0x89,0x44,0x24,0x21,0x66,0x44,0x89,0x4C,
    0x24,0x23,0x88,0x54,0x24,0x25,0x48,0x85,
    0xC9,
});
constexpr auto SendRoomOutOfSightPacketExpected = std::to_array<std::uint8_t>({
    0x40,0x57,0x48,0x83,0xEC,0x40,0xC6,0x44,
    0x24,0x20,0x08,0x48,0x8B,0xF9,0x66,0x44,
    0x89,0x44,0x24,0x21,0x66,0x44,0x89,0x4C,
    0x24,0x23,0x88,0x54,0x24,0x25,0x48,0x85,
    0xC9,
});
constexpr auto SetClientInSightExpected = std::to_array<std::uint8_t>({
    0x48,0x89,0x5C,0x24,0x08,0x48,0x89,0x6C,
    0x24,0x10,0x48,0x89,0x74,0x24,0x18,0x57,
    0x48,0x83,0xEC,0x30,0x41,0x8B,0xE9,0x41,
    0x8B,0xF8,0x48,0x8B,0xF2,0x0F,0xB6,0xD9,
});
constexpr auto UnsetClientInSightExpected = std::to_array<std::uint8_t>({
    0x48,0x89,0x5C,0x24,0x08,0x48,0x89,0x6C,
    0x24,0x10,0x48,0x89,0x74,0x24,0x18,0x57,
    0x48,0x83,0xEC,0x30,0x41,0x8B,0xE9,0x41,
    0x8B,0xD8,0x48,0x8B,0xF2,
});
constexpr auto D2ClientLayoutWitnessExpected = std::to_array<std::uint8_t>({
    0xF6,0x87,0xE0,0x04,0x00,0x00,0x01,0x74,
    0x09,0x48,0x8B,0x87,0x78,0x02,0x00,0x00,
    0xEB,0x2E,0x4C,0x39,0xA7,0x78,0x02,0x00,
    0x00,0x74,0x3F,0x44,0x8B,0x87,0x70,0x02,
    0x00,0x00,0x8B,0x97,0x6C,0x02,0x00,0x00,
    0x48,0x8B,0x8F,0xB0,0x02,0x00,0x00,0xE8,
    0xFB,0xA2,0x00,0x00,
});

using ResolveActFromLevelIdFn = std::uint8_t(__fastcall*)(
    std::uint8_t dataContext,
    std::int32_t levelId) noexcept;

using SendRoomVisibilityPacketFn = void(__fastcall*)(
    void* client,
    std::int32_t levelId,
    std::uint16_t x,
    std::uint16_t y) noexcept;

using UpdateClientSightFn = void(__fastcall*)(
    std::uint8_t dataContext,
    void* act,
    std::int32_t levelId,
    std::int32_t x,
    std::int32_t y,
    void* room) noexcept;

struct ActMap {
    std::uint64_t revision{};
    std::vector<ActEntry> entries;
};

struct CompatiblePeerEntry {
    D2RL::Network::PeerHandle peer{};
    std::uint32_t playerId{};
};

struct CompatiblePeerMap {
    std::vector<CompatiblePeerEntry> entries;
};

const D2RL::PluginContext* Context{};
const D2RL::DataTableServiceV1* DataTables{};
const D2RL::NetworkServiceV1* Network{};
D2RL::Network::ChannelHandle NetworkChannel{
    D2RL::Network::InvalidChannelHandle};
std::string RuntimeBuildName{"unknown"};
HANDLE SingletonHandle{};
ResolveActFromLevelIdFn OriginalResolveActFromLevelId{};
SendRoomVisibilityPacketFn OriginalSendRoomInSightPacket{};
SendRoomVisibilityPacketFn OriginalSendRoomOutOfSightPacket{};
UpdateClientSightFn OriginalSetClientInSight{};
UpdateClientSightFn OriginalUnsetClientInSight{};

std::atomic_bool Operational{};
std::atomic_bool CacheReady{};
std::atomic_uint64_t PublishedRevision{};
std::atomic_uint64_t ResolvedFromLevels{};
std::atomic_uint64_t OriginalFallbacks{};
std::atomic_uint64_t EncodedVisibilityPackets{};
std::atomic_uint64_t DecodedVisibilityPackets{};
std::atomic_uint64_t RefusedVisibilityPackets{};
std::atomic_uint64_t CompatiblePeerAnnouncements{};
std::atomic_uint64_t NetworkWarnings{};
std::atomic_uint32_t LocalPlayerId{InvalidPlayerId};
std::atomic_uint64_t SessionGeneration{};
std::atomic<std::shared_ptr<const ActMap>> ClassicCache{};
std::atomic<std::shared_ptr<const ActMap>> LodCache{};
std::atomic<std::shared_ptr<const ActMap>> RotwCache{};
std::atomic<std::shared_ptr<const CompatiblePeerMap>> CompatiblePeers{};

template <typename Value>
Value ReadValue(const void* base, std::size_t offset) noexcept {
    Value result{};
    if (base) {
        const auto* bytes = static_cast<const std::uint8_t*>(base);
        std::memcpy(&result, bytes + offset, sizeof(result));
    }
    return result;
}

const char* BankName(D2RL::DataTables::Bank bank) noexcept {
    switch (bank) {
    case D2RL::DataTables::Bank::Classic: return "Classic";
    case D2RL::DataTables::Bank::Lod: return "Lod";
    case D2RL::DataTables::Bank::Rotw: return "RotW";
    default: return "Unknown";
    }
}

D2RL::DataTables::Bank BankForContext(
        std::uint8_t dataContext) noexcept {
    switch (dataContext) {
    case 1: return D2RL::DataTables::Bank::Classic;
    case 2: return D2RL::DataTables::Bank::Lod;
    case 3: return D2RL::DataTables::Bank::Rotw;
    default: return static_cast<D2RL::DataTables::Bank>(0);
    }
}

std::shared_ptr<const ActMap> LoadCache(
        std::uint8_t dataContext) noexcept {
    switch (dataContext) {
    case 1: return ClassicCache.load(std::memory_order_acquire);
    case 2: return LodCache.load(std::memory_order_acquire);
    case 3: return RotwCache.load(std::memory_order_acquire);
    default: return {};
    }
}

void ResetCaches() noexcept {
    CacheReady.store(false, std::memory_order_release);
    ClassicCache.store({}, std::memory_order_release);
    LodCache.store({}, std::memory_order_release);
    RotwCache.store({}, std::memory_order_release);
    PublishedRevision.store(0, std::memory_order_release);
}

bool AcquireSingleton() noexcept {
    SingletonHandle = CreateMutexW(nullptr, FALSE, SingletonName);
    if (!SingletonHandle) {
        Context->LogError(
            "ExtendedActLevelIds: process singleton could not be created.");
        return false;
    }
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(SingletonHandle);
        SingletonHandle = nullptr;
        Context->LogError(
            "ExtendedActLevelIds: duplicate global/mod-local installation refused.");
        return false;
    }
    return true;
}

void ReleaseSingleton() noexcept {
    if (SingletonHandle) {
        CloseHandle(SingletonHandle);
        SingletonHandle = nullptr;
    }
}

bool ValidateRuntime() noexcept {
    const auto* base = Context
        ? reinterpret_cast<const std::uint8_t*>(Context->exeBase)
        : nullptr;
    const auto matches = [base](
            std::uintptr_t rva,
            const auto& expected) noexcept {
        return base && std::memcmp(
            base + rva,
            expected.data(),
            expected.size()) == 0;
    };
    if (!matches(ResolveActFromLevelIdRva, ResolveActFromLevelIdExpected)) {
        Context->LogError(
            "ExtendedActLevelIds: central act resolver fingerprint mismatch; plugin refused.");
        return false;
    }
    if (!matches(LevelsCapacityGuardRva, LevelsCapacityGuardExpected)) {
        Context->LogError(
            "ExtendedActLevelIds: native 1023-record capacity witness mismatch; plugin refused.");
        return false;
    }
    if (!matches(SendRoomInSightPacketRva, SendRoomInSightPacketExpected)
            || !matches(
                SendRoomOutOfSightPacketRva,
                SendRoomOutOfSightPacketExpected)
            || !matches(SetClientInSightRva, SetClientInSightExpected)
            || !matches(UnsetClientInSightRva, UnsetClientInSightExpected)) {
        Context->LogError(
            "ExtendedActLevelIds: room-visibility codec fingerprint mismatch; plugin refused.");
        return false;
    }
    if (!matches(D2ClientLayoutWitnessRva, D2ClientLayoutWitnessExpected)) {
        Context->LogError(
            "ExtendedActLevelIds: D2Client player identity witness mismatch; plugin refused.");
        return false;
    }
    return true;
}

bool QueryServices(
        const D2RL::LifecycleServiceV1*& lifecycle) noexcept {
    const D2RL::DataTableServiceV1* tables{};
    if (Context->QueryService(
            D2RL::ServiceId::DataTable,
            D2RL::DataTableServiceV1Version,
            &tables) != D2RL::ServiceQueryResult::Success
            || !D2RL::HasDataTableServiceV1Field(
                tables,
                D2RL::DataTableServiceV1RequiredSize)) {
        Context->LogError(
            "ExtendedActLevelIds: DataTableServiceV1 is unavailable or incompatible.");
        return false;
    }
    DataTables = tables;

    const D2RL::LifecycleServiceV1* lifecycleService{};
    if (Context->QueryService(
            D2RL::ServiceId::Lifecycle,
            D2RL::LifecycleServiceV1Version,
            &lifecycleService) != D2RL::ServiceQueryResult::Success
            || !D2RL::HasLifecycleServiceV1Field(
                lifecycleService,
                D2RL::LifecycleServiceV1RequiredSize)) {
        Context->LogError(
            "ExtendedActLevelIds: LifecycleServiceV1 is unavailable or incompatible.");
        return false;
    }
    lifecycle = lifecycleService;

    const D2RL::NetworkServiceV1* networkService{};
    if (Context->QueryService(
            D2RL::ServiceId::Network,
            D2RL::NetworkServiceV1Version,
            &networkService) != D2RL::ServiceQueryResult::Success
            || !D2RL::HasNetworkServiceV1Field(
                networkService,
                D2RL::NetworkServiceV1RequiredSize)) {
        Context->LogError(
            "ExtendedActLevelIds: NetworkServiceV1 is unavailable or incompatible.");
        return false;
    }
    Network = networkService;
    return true;
}

std::shared_ptr<const ActMap> BuildBankCache(
        const D2RL::PluginContext* context,
        D2RL::DataTables::Bank bank,
        std::uint64_t revision,
        std::string& error) {
    D2RL::DataTables::TableView table{
        .structSize = D2RL::DataTables::TableViewSize,
    };
    const auto tableResult = DataTables->getTable(
        context,
        bank,
        D2RL::DataTables::TableId::Levels,
        &table);
    if (tableResult != D2RL::DataTables::Result::Success
            || table.revision != revision
            || table.rows == nullptr
            || !HasValidLevelRecordCount(table.rowCount)
            || table.rowSize != LevelsRowSize) {
        error = "invalid Levels table view";
        return {};
    }

    auto map = std::make_shared<ActMap>();
    map->revision = revision;
    map->entries.reserve(table.rowCount);
    for (std::uint32_t rowIndex = 0; rowIndex < table.rowCount; ++rowIndex) {
        D2RL::DataTables::RowView physical{
            .structSize = D2RL::DataTables::RowViewSize,
        };
        if (DataTables->getRow(
                context,
                bank,
                D2RL::DataTables::TableId::Levels,
                rowIndex,
                &physical) != D2RL::DataTables::Result::Success
                || physical.revision != revision
                || physical.row == nullptr
                || physical.rowIndex != rowIndex
                || physical.rowSize != LevelsRowSize) {
            error = "physical Levels row lookup failed";
            return {};
        }

        const auto levelId = ReadValue<std::int32_t>(
            physical.row,
            LevelsIdOffset);
        const auto act = ReadValue<std::uint8_t>(
            physical.row,
            LevelsActOffset);
        if (!IsCanonicalLevelId(levelId, rowIndex)
                || act > MaximumAct) {
            error = "Levels rows must use contiguous Id values 0..1022 and Act values 0..4";
            return {};
        }

        D2RL::DataTables::RowView keyed{
            .structSize = D2RL::DataTables::RowViewSize,
        };
        if (DataTables->findRowById(
                context,
                bank,
                D2RL::DataTables::TableId::Levels,
                static_cast<std::uint32_t>(levelId),
                &keyed) != D2RL::DataTables::Result::Success
                || keyed.revision != revision
                || keyed.row != physical.row
                || keyed.rowIndex != rowIndex
                || keyed.rowSize != LevelsRowSize) {
            error = "Levels Id layout failed the service round-trip";
            return {};
        }
        map->entries.push_back({levelId, act});
    }

    std::sort(map->entries.begin(), map->entries.end());
    if (std::adjacent_find(
            map->entries.begin(),
            map->entries.end(),
            [](const ActEntry& left, const ActEntry& right) {
                return left.levelId == right.levelId;
            }) != map->entries.end()) {
        error = "Levels contains duplicate Id values";
        return {};
    }
    if (!HasValidAnchorActs(std::span<const ActEntry>(map->entries))) {
        error = "Levels Act layout failed the anchor check";
        return {};
    }
    return map;
}

void __cdecl OnDataTablesLoaded(
        const D2RL::PluginContext* context,
        const D2RL::Lifecycle::DataTablesLoadedEvent* event,
        void*) noexcept {
    CacheReady.store(false, std::memory_order_release);
    try {
        if (!context
                || !DataTables
                || !D2RL::Lifecycle::HasDataTablesLoadedEventField(
                    event,
                    D2RL::Lifecycle::DataTablesLoadedEventRequiredSize)) {
            ResetCaches();
            if (context) context->LogError(
                "ExtendedActLevelIds: invalid DataTablesLoaded event; original resolver retained.");
            return;
        }

        std::array<std::shared_ptr<const ActMap>, 3> maps;
        constexpr std::array<D2RL::DataTables::Bank, 3> banks{
            D2RL::DataTables::Bank::Classic,
            D2RL::DataTables::Bank::Lod,
            D2RL::DataTables::Bank::Rotw,
        };
        for (std::size_t index = 0; index < banks.size(); ++index) {
            std::string error;
            maps[index] = BuildBankCache(
                context,
                banks[index],
                event->revision,
                error);
            if (!maps[index]) {
                ResetCaches();
                const auto message = std::string(
                    "ExtendedActLevelIds: ") + BankName(banks[index])
                    + " cache rejected (" + error
                    + "); original resolver retained.";
                context->LogError(message.c_str());
                return;
            }
        }

        ClassicCache.store(maps[0], std::memory_order_release);
        LodCache.store(maps[1], std::memory_order_release);
        RotwCache.store(maps[2], std::memory_order_release);
        PublishedRevision.store(event->revision, std::memory_order_release);
        CacheReady.store(true, std::memory_order_release);

        char message[256]{};
        std::snprintf(
            message,
            sizeof(message),
            "ExtendedActLevelIds: Levels revision %llu accepted; rows Classic=%zu, LoD=%zu, RotW=%zu.",
            static_cast<unsigned long long>(event->revision),
            maps[0]->entries.size(),
            maps[1]->entries.size(),
            maps[2]->entries.size());
        context->LogInfo(message);
    } catch (const std::exception& exception) {
        ResetCaches();
        const auto message = std::string(
            "ExtendedActLevelIds: cache build failed (")
            + exception.what() + "); original resolver retained.";
        context->LogError(message.c_str());
    } catch (...) {
        ResetCaches();
        context->LogError(
            "ExtendedActLevelIds: cache build failed; original resolver retained.");
    }
}

std::uint8_t __fastcall HookResolveActFromLevelId(
        std::uint8_t dataContext,
        std::int32_t levelId) noexcept {
    if (Operational.load(std::memory_order_acquire)
            && CacheReady.load(std::memory_order_acquire)
            && IsSupportedDataContext(dataContext)
            && BankForContext(dataContext)
                != static_cast<D2RL::DataTables::Bank>(0)) {
        const auto cache = LoadCache(dataContext);
        if (cache) {
            const auto act = FindAct(
                std::span<const ActEntry>(cache->entries),
                levelId);
            if (act) {
                ResolvedFromLevels.fetch_add(1, std::memory_order_relaxed);
                return *act;
            }
        }
    }
    OriginalFallbacks.fetch_add(1, std::memory_order_relaxed);
    return OriginalResolveActFromLevelId(dataContext, levelId);
}

bool IsKnownExtendedLevelId(std::int32_t levelId) noexcept {
    if (levelId <= MaximumVanillaNetworkLevelId
            || levelId > MaximumLevelId
            || !CacheReady.load(std::memory_order_acquire)) {
        return false;
    }
    for (std::uint8_t dataContext = MinimumDataContext;
            dataContext <= MaximumDataContext;
            ++dataContext) {
        const auto cache = LoadCache(dataContext);
        if (cache && FindAct(
                std::span<const ActEntry>(cache->entries),
                levelId)) {
            return true;
        }
    }
    return false;
}

bool IsCompatiblePlayer(std::uint32_t playerId) noexcept {
    if (playerId == InvalidPlayerId) return false;
    const auto localPlayerId = LocalPlayerId.load(std::memory_order_acquire);
    if (localPlayerId != InvalidPlayerId && localPlayerId == playerId) {
        return true;
    }
    const auto peers = CompatiblePeers.load(std::memory_order_acquire);
    return peers && std::any_of(
        peers->entries.begin(),
        peers->entries.end(),
        [playerId](const CompatiblePeerEntry& entry) {
            return entry.playerId == playerId;
        });
}

std::size_t CompatiblePeerCount() noexcept {
    const auto peers = CompatiblePeers.load(std::memory_order_acquire);
    return peers ? peers->entries.size() : 0;
}

void PublishCompatiblePeer(
        D2RL::Network::PeerHandle peer,
        std::uint32_t playerId) noexcept {
    if (peer == D2RL::Network::InvalidPeerHandle) return;
    try {
        auto current = CompatiblePeers.load(std::memory_order_acquire);
        for (;;) {
            auto mutableNext = std::make_shared<CompatiblePeerMap>();
            if (current) mutableNext->entries = current->entries;
            std::erase_if(
                mutableNext->entries,
                [peer](const CompatiblePeerEntry& entry) {
                    return entry.peer == peer;
                });
            mutableNext->entries.push_back({peer, playerId});
            std::shared_ptr<const CompatiblePeerMap> next{mutableNext};
            if (CompatiblePeers.compare_exchange_weak(
                    current,
                    next,
                    std::memory_order_release,
                    std::memory_order_acquire)) {
                CompatiblePeerAnnouncements.fetch_add(
                    1,
                    std::memory_order_relaxed);
                return;
            }
        }
    } catch (...) {
        if (Context) Context->LogError(
            "ExtendedActLevelIds: compatible peer map update failed.");
    }
}

void RemoveCompatiblePeer(D2RL::Network::PeerHandle peer) noexcept {
    if (peer == D2RL::Network::InvalidPeerHandle) return;
    try {
        auto current = CompatiblePeers.load(std::memory_order_acquire);
        while (current) {
            auto mutableNext = std::make_shared<CompatiblePeerMap>();
            mutableNext->entries = current->entries;
            std::erase_if(
                mutableNext->entries,
                [peer](const CompatiblePeerEntry& entry) {
                    return entry.peer == peer;
                });
            std::shared_ptr<const CompatiblePeerMap> next =
                mutableNext->entries.empty()
                    ? std::shared_ptr<const CompatiblePeerMap>{}
                    : std::shared_ptr<const CompatiblePeerMap>{mutableNext};
            if (CompatiblePeers.compare_exchange_weak(
                    current,
                    next,
                    std::memory_order_release,
                    std::memory_order_acquire)) {
                return;
            }
        }
    } catch (...) {
        if (Context) Context->LogError(
            "ExtendedActLevelIds: compatible peer removal failed.");
    }
}

void ResetNetworkSession(std::uint64_t sessionGeneration = 0) noexcept {
    CompatiblePeers.store({}, std::memory_order_release);
    LocalPlayerId.store(InvalidPlayerId, std::memory_order_release);
    SessionGeneration.store(sessionGeneration, std::memory_order_release);
}

void WarnNetworkUnavailableOnce(const char* message) noexcept {
    if (NetworkWarnings.fetch_add(1, std::memory_order_relaxed) == 0
            && Context) {
        Context->LogWarn(message);
    }
}

void SendCompatibilityHandshake(
        const D2RL::PluginContext* context) noexcept {
    if (!context
            || !Network
            || NetworkChannel == D2RL::Network::InvalidChannelHandle) {
        return;
    }
    const auto playerId = LocalPlayerId.load(std::memory_order_acquire);
    if (playerId == InvalidPlayerId) return;
    const auto result = Network->sendToHost(
        context,
        NetworkChannel,
        CompatibilityHandshakeMessage,
        &playerId,
        sizeof(playerId));
    if (result != D2RL::Network::Result::Success
            && result != D2RL::Network::Result::WrongRole
            && result != D2RL::Network::Result::NotConnected) {
        WarnNetworkUnavailableOnce(
            "ExtendedActLevelIds: compatibility handshake is unavailable; Level IDs above 255 will fail closed.");
    }
}

void ConnectOrAnnounce(const D2RL::PluginContext* context) noexcept {
    if (!context
            || !Network
            || NetworkChannel == D2RL::Network::InvalidChannelHandle) {
        return;
    }
    D2RL::Network::ChannelInfo info{
        .structSize = D2RL::Network::ChannelInfoSize,
    };
    if (Network->getChannelInfo(
            context,
            NetworkChannel,
            &info) == D2RL::Network::Result::Success
            && info.connectionState
                == D2RL::Network::ConnectionState::Connected) {
        SendCompatibilityHandshake(context);
        return;
    }
    const auto result = Network->connectToHost(context, NetworkChannel);
    if (result != D2RL::Network::Result::Success
            && result != D2RL::Network::Result::Busy
            && result != D2RL::Network::Result::NotConnected) {
        WarnNetworkUnavailableOnce(
            "ExtendedActLevelIds: no compatible private game channel; Level IDs above 255 will fail closed.");
    }
}

void __cdecl OnHostCompatibilityMessage(
        const D2RL::PluginContext* context,
        D2RL::Network::ChannelHandle channel,
        D2RL::Network::PeerHandle peer,
        std::uint16_t messageId,
        const void* data,
        std::uint32_t size,
        void*) noexcept {
    if (!context
            || channel != NetworkChannel
            || messageId != CompatibilityHandshakeMessage
            || !data
            || size != sizeof(std::uint32_t)) {
        return;
    }
    std::uint32_t playerId{};
    std::memcpy(&playerId, data, sizeof(playerId));
    PublishCompatiblePeer(peer, playerId);
}

void __cdecl OnClientCompatibilityMessage(
        const D2RL::PluginContext*,
        D2RL::Network::ChannelHandle,
        std::uint16_t,
        const void*,
        std::uint32_t,
        void*) noexcept {
}

void __cdecl OnNetworkConnectionState(
        const D2RL::PluginContext* context,
        const D2RL::Network::ConnectionEvent* event,
        void*) noexcept {
    if (!context
            || !D2RL::Network::HasConnectionEventField(
                event,
                D2RL::Network::ConnectionEventRequiredSize)
            || event->channel != NetworkChannel) {
        return;
    }
    if (event->state == D2RL::Network::ConnectionState::Connected) {
        SendCompatibilityHandshake(context);
    } else if (event->state
            == D2RL::Network::ConnectionState::Disconnected
            || event->state == D2RL::Network::ConnectionState::Rejected) {
        RemoveCompatiblePeer(event->peer);
    }
}

void __cdecl OnGameplayEvent(
        const D2RL::PluginContext* context,
        const D2RL::Lifecycle::GameplayEvent* event,
        void*) noexcept {
    if (!context
            || !D2RL::Lifecycle::HasGameplayEventField(
                event,
                D2RL::Lifecycle::GameplayEventRequiredSize)) {
        return;
    }
    if (event->kind == D2RL::Lifecycle::GameplayEventKind::GameLeft) {
        ResetNetworkSession();
        return;
    }
    if (event->sessionGeneration
            != SessionGeneration.load(std::memory_order_acquire)) {
        ResetNetworkSession(event->sessionGeneration);
    }
    if (event->kind
            == D2RL::Lifecycle::GameplayEventKind::LocalPlayerReady) {
        LocalPlayerId.store(event->playerId, std::memory_order_release);
    }
    if (event->kind
                == D2RL::Lifecycle::GameplayEventKind::LocalPlayerReady
            || event->kind == D2RL::Lifecycle::GameplayEventKind::ActChanged
            || event->kind == D2RL::Lifecycle::GameplayEventKind::LevelChanged
            || event->kind
                == D2RL::Lifecycle::GameplayEventKind::PlayerResurrected) {
        ConnectOrAnnounce(context);
    }
}

void RefuseVisibilityPacketOnce(const char* reason) noexcept {
    if (RefusedVisibilityPackets.fetch_add(1, std::memory_order_relaxed) == 0
            && Context) {
        Context->LogError(reason);
    }
}

void SendRoomVisibilityPacket(
        SendRoomVisibilityPacketFn original,
        void* client,
        std::int32_t levelId,
        std::uint16_t x,
        std::uint16_t y) noexcept {
    if (!original) return;
    if (levelId <= MaximumVanillaNetworkLevelId) {
        original(client, levelId, x, y);
        return;
    }
    if (!Operational.load(std::memory_order_acquire)
            || !client
            || !IsKnownExtendedLevelId(levelId)) {
        RefuseVisibilityPacketOnce(
            "ExtendedActLevelIds: unknown extended Level ID refused before packet encoding.");
        return;
    }
    const auto playerId = ReadValue<std::uint32_t>(
        client,
        D2ClientPlayerIdOffset);
    if (!IsCompatiblePlayer(playerId)) {
        RefuseVisibilityPacketOnce(
            "ExtendedActLevelIds: extended Level ID refused for a peer without the compatible v2 channel.");
        return;
    }
    const auto encoded = EncodeLevelCoordinate(levelId, x);
    if (!encoded) {
        RefuseVisibilityPacketOnce(
            "ExtendedActLevelIds: extended Level ID or room X coordinate exceeds the v2 codec contract.");
        return;
    }
    EncodedVisibilityPackets.fetch_add(1, std::memory_order_relaxed);
    original(client, levelId, encoded->x, y);
}

void __fastcall HookSendRoomInSightPacket(
        void* client,
        std::int32_t levelId,
        std::uint16_t x,
        std::uint16_t y) noexcept {
    SendRoomVisibilityPacket(
        OriginalSendRoomInSightPacket,
        client,
        levelId,
        x,
        y);
}

void __fastcall HookSendRoomOutOfSightPacket(
        void* client,
        std::int32_t levelId,
        std::uint16_t x,
        std::uint16_t y) noexcept {
    SendRoomVisibilityPacket(
        OriginalSendRoomOutOfSightPacket,
        client,
        levelId,
        x,
        y);
}

void UpdateClientSight(
        UpdateClientSightFn original,
        std::uint8_t dataContext,
        void* act,
        std::int32_t levelId,
        std::int32_t x,
        std::int32_t y,
        void* room) noexcept {
    if (!original) return;
    if ((static_cast<std::uint32_t>(x) & CodecMarkerMask) == 0) {
        original(dataContext, act, levelId, x, y, room);
        return;
    }
    const auto decoded = DecodeLevelCoordinate(levelId, x);
    if (!Operational.load(std::memory_order_acquire)
            || !decoded
            || !IsKnownExtendedLevelId(decoded->levelId)) {
        RefuseVisibilityPacketOnce(
            "ExtendedActLevelIds: malformed or unknown v2 room-visibility packet refused.");
        return;
    }
    DecodedVisibilityPackets.fetch_add(1, std::memory_order_relaxed);
    original(
        dataContext,
        act,
        decoded->levelId,
        decoded->x,
        y,
        room);
}

void __fastcall HookSetClientInSight(
        std::uint8_t dataContext,
        void* act,
        std::int32_t levelId,
        std::int32_t x,
        std::int32_t y,
        void* room) noexcept {
    UpdateClientSight(
        OriginalSetClientInSight,
        dataContext,
        act,
        levelId,
        x,
        y,
        room);
}

void __fastcall HookUnsetClientInSight(
        std::uint8_t dataContext,
        void* act,
        std::int32_t levelId,
        std::int32_t x,
        std::int32_t y,
        void* room) noexcept {
    UpdateClientSight(
        OriginalUnsetClientInSight,
        dataContext,
        act,
        levelId,
        x,
        y,
        room);
}

std::string_view Trim(std::string_view text) noexcept {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos) return {};
    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

bool ParseInteger(
        std::string_view text,
        std::int32_t& value) noexcept {
    text = Trim(text);
    if (text.empty()) return false;
    const auto parsed = std::from_chars(
        text.data(),
        text.data() + text.size(),
        value);
    return parsed.ec == std::errc{}
        && parsed.ptr == text.data() + text.size();
}

auto ResolveProbe(
        const D2RL::ConsoleCommandContext* command,
        std::string_view arguments) noexcept
        -> D2RL::ConsoleCommandResult {
    constexpr std::string_view verb{"resolve"};
    arguments = Trim(arguments);
    if (!arguments.starts_with(verb)
            || (arguments.size() > verb.size()
                && arguments[verb.size()] != ' ')) {
        return D2RL::ConsoleCommandResult::InvalidArguments;
    }
    arguments = Trim(arguments.substr(verb.size()));
    const auto separator = arguments.find_first_of(" \t");
    const auto levelText = separator == std::string_view::npos
        ? arguments
        : arguments.substr(0, separator);
    const auto contextText = separator == std::string_view::npos
        ? std::string_view{}
        : Trim(arguments.substr(separator + 1));

    std::int32_t levelId{};
    std::int32_t parsedContext{3};
    if (!ParseInteger(levelText, levelId)
            || (!contextText.empty()
                && !ParseInteger(contextText, parsedContext))
            || parsedContext < MinimumDataContext
            || parsedContext > MaximumDataContext
            || levelId < 0
            || levelId > MaximumLevelId) {
        return D2RL::ConsoleCommandResult::InvalidArguments;
    }
    if (!Operational.load(std::memory_order_acquire)
            || !CacheReady.load(std::memory_order_acquire)
            || !Context
            || !Context->exeBase) {
        command->plugin->WriteConsoleMessage(
            "Extended Act Level IDs: resolver is not operational.",
            D2RL::ConsoleMessageKind::Error);
        return D2RL::ConsoleCommandResult::Failed;
    }

    const auto dataContext = static_cast<std::uint8_t>(parsedContext);
    const auto cache = LoadCache(dataContext);
    const auto cachedAct = cache
        ? FindAct(std::span<const ActEntry>(cache->entries), levelId)
        : std::nullopt;
    const auto resolver = reinterpret_cast<ResolveActFromLevelIdFn>(
        Context->exeBase + ResolveActFromLevelIdRva);
    const auto resolvedAct = resolver(dataContext, levelId);

    char message[256]{};
    std::snprintf(
        message,
        sizeof(message),
        "Extended Act Level IDs: Level Id %d resolved to Act index %u (Act %u), data context %u, source=%s.",
        levelId,
        static_cast<unsigned>(resolvedAct),
        static_cast<unsigned>(resolvedAct) + 1,
        static_cast<unsigned>(dataContext),
        cachedAct ? "Levels.txt" : "original resolver");
    Context->LogInfo(message);
    command->plugin->WriteConsoleMessage(message);
    return D2RL::ConsoleCommandResult::Handled;
}

auto Status(
        D2R::Game::Client*,
        const D2RL::ConsoleCommandContext* command,
        void*) noexcept -> D2RL::ConsoleCommandResult {
    if (!command || !command->plugin) {
        return D2RL::ConsoleCommandResult::Failed;
    }
    const auto arguments = command->args
        ? Trim(std::string_view(command->args, command->argsLength))
        : std::string_view{};
    if (!arguments.empty()) return ResolveProbe(command, arguments);
    const auto classic = ClassicCache.load(std::memory_order_acquire);
    const auto lod = LodCache.load(std::memory_order_acquire);
    const auto rotw = RotwCache.load(std::memory_order_acquire);
    char message[1024]{};
    std::snprintf(
        message,
        sizeof(message),
        "Extended Act Level IDs 2.0.0: %s; cache=%s; revision=%llu; rows=%zu/%zu/%zu; compatible peers=%zu; encoded=%llu; decoded=%llu; refused=%llu; Levels resolutions=%llu; original fallbacks=%llu; build=%s.",
        Operational.load(std::memory_order_acquire) ? "active" : "inactive",
        CacheReady.load(std::memory_order_acquire) ? "ready" : "not ready",
        static_cast<unsigned long long>(
            PublishedRevision.load(std::memory_order_acquire)),
        classic ? classic->entries.size() : 0,
        lod ? lod->entries.size() : 0,
        rotw ? rotw->entries.size() : 0,
        CompatiblePeerCount(),
        static_cast<unsigned long long>(
            EncodedVisibilityPackets.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(
            DecodedVisibilityPackets.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(
            RefusedVisibilityPackets.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(
            ResolvedFromLevels.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(
            OriginalFallbacks.load(std::memory_order_relaxed)),
        RuntimeBuildName.c_str());
    command->plugin->WriteConsoleMessage(message);
    return D2RL::ConsoleCommandResult::Handled;
}

void ResetState() noexcept {
    Operational.store(false, std::memory_order_release);
    ResetCaches();
    ResetNetworkSession();
    ResolvedFromLevels.store(0, std::memory_order_relaxed);
    OriginalFallbacks.store(0, std::memory_order_relaxed);
    EncodedVisibilityPackets.store(0, std::memory_order_relaxed);
    DecodedVisibilityPackets.store(0, std::memory_order_relaxed);
    RefusedVisibilityPackets.store(0, std::memory_order_relaxed);
    CompatiblePeerAnnouncements.store(0, std::memory_order_relaxed);
    NetworkWarnings.store(0, std::memory_order_relaxed);
    DataTables = nullptr;
    Network = nullptr;
    NetworkChannel = D2RL::Network::InvalidChannelHandle;
    OriginalResolveActFromLevelId = nullptr;
    OriginalSendRoomInSightPacket = nullptr;
    OriginalSendRoomOutOfSightPacket = nullptr;
    OriginalSetClientInSight = nullptr;
    OriginalUnsetClientInSight = nullptr;
}

} // namespace
} // namespace RuffnecKk::ExtendedActLevelIds

namespace {

constexpr D2RL::PluginInfo Info{
    .infoSize = D2RL::PluginInfoSize,
    .apiVersion = D2RL_PLUGIN_API_VERSION,
    .id = "ruffneckk-extended-act-level-ids",
    .name = "Extended Act Level IDs",
    .version = "2.0.0",
    .author = "RuffnecKk",
    .description = "Extends functional level IDs to the native 1023-record limit.",
    .flags = D2RL::PluginFlags::Shared | D2RL::PluginFlags::NativeHooks,
};

} // namespace

D2RL_PLUGIN_EXPORT auto D2RLoaderGetPluginInfo() noexcept
        -> const D2RL::PluginInfo* {
    return &Info;
}

D2RL_PLUGIN_EXPORT auto D2RLoaderLoadPlugin(
        const D2RL::PluginContext* context) noexcept -> bool {
    using namespace RuffnecKk::ExtendedActLevelIds;
    Context = context;
    ResetState();
    if (!Context || !Context->exeBase) return false;
    if (!AcquireSingleton()) return false;

    const auto* runtimeBuild = D2RL::GetBuildName(Context);
    RuntimeBuildName = runtimeBuild ? runtimeBuild : "unknown";
    if (!Context->RegisterConsoleCommand(
            "extended-act-level-ids",
            Status,
            "Show status or resolve a cached Level ID.")) {
        Context->LogWarn(
            "ExtendedActLevelIds: status command could not be registered.");
    }
    if (!ValidateRuntime()) {
        ReleaseSingleton();
        return false;
    }

    const D2RL::LifecycleServiceV1* lifecycle{};
    if (!QueryServices(lifecycle)) {
        ReleaseSingleton();
        return false;
    }
    const D2RL::Network::ChannelRegistration channelRegistration{
        .structSize = D2RL::Network::ChannelRegistrationSize,
        .flags = 0,
        .localChannelId = NetworkLocalChannelId,
        .reserved = 0,
        .reserved2 = 0,
        .compatibilityToken = NetworkCompatibilityToken,
        .hostMessage = OnHostCompatibilityMessage,
        .clientMessage = OnClientCompatibilityMessage,
        .connectionState = OnNetworkConnectionState,
        .userData = nullptr,
    };
    if (Network->registerChannel(
            Context,
            &channelRegistration,
            &NetworkChannel) != D2RL::Network::Result::Success
            || NetworkChannel == D2RL::Network::InvalidChannelHandle) {
        Context->LogError(
            "ExtendedActLevelIds: v2 compatibility channel registration failed.");
        ReleaseSingleton();
        return false;
    }
    const D2RL::Lifecycle::DataTablesLoadedListener listener{
        .structSize = D2RL::Lifecycle::DataTablesLoadedListenerSize,
        .flags = 0,
        .callback = OnDataTablesLoaded,
        .userData = nullptr,
    };
    D2RL::Lifecycle::ListenerHandle listenerHandle{
        D2RL::Lifecycle::InvalidHandle};
    if (lifecycle->registerDataTablesLoadedListener(
            Context,
            &listener,
            &listenerHandle) != D2RL::Lifecycle::Result::Success
            || listenerHandle == D2RL::Lifecycle::InvalidHandle) {
        Context->LogError(
            "ExtendedActLevelIds: DataTablesLoaded listener registration failed.");
        (void)Network->unregisterChannel(Context, NetworkChannel);
        NetworkChannel = D2RL::Network::InvalidChannelHandle;
        ReleaseSingleton();
        return false;
    }

    constexpr std::array gameplayKinds{
        D2RL::Lifecycle::GameplayEventKind::GameJoined,
        D2RL::Lifecycle::GameplayEventKind::GameLeft,
        D2RL::Lifecycle::GameplayEventKind::LocalPlayerReady,
        D2RL::Lifecycle::GameplayEventKind::ActChanged,
        D2RL::Lifecycle::GameplayEventKind::LevelChanged,
        D2RL::Lifecycle::GameplayEventKind::PlayerResurrected,
    };
    for (const auto kind : gameplayKinds) {
        const D2RL::Lifecycle::GameplayEventListener gameplayListener{
            .structSize = D2RL::Lifecycle::GameplayEventListenerSize,
            .flags = 0,
            .kind = kind,
            .reserved = 0,
            .callback = OnGameplayEvent,
            .userData = nullptr,
        };
        D2RL::Lifecycle::ListenerHandle gameplayHandle{
            D2RL::Lifecycle::InvalidHandle};
        if (lifecycle->registerGameplayEventListener(
                Context,
                &gameplayListener,
                &gameplayHandle) != D2RL::Lifecycle::Result::Success
                || gameplayHandle == D2RL::Lifecycle::InvalidHandle) {
            Context->LogError(
                "ExtendedActLevelIds: gameplay listener registration failed.");
            (void)Network->unregisterChannel(Context, NetworkChannel);
            NetworkChannel = D2RL::Network::InvalidChannelHandle;
            ReleaseSingleton();
            return false;
        }
    }

    if (!Context->InstallInlineHook(
            ResolveActFromLevelIdRva,
            ResolveActFromLevelIdExpected.data(),
            static_cast<std::uint32_t>(
                ResolveActFromLevelIdExpected.size()),
            HookResolveActFromLevelId,
            &OriginalResolveActFromLevelId)) {
        Context->LogError(
            "ExtendedActLevelIds: central resolver hook is already owned or unavailable.");
        (void)Network->unregisterChannel(Context, NetworkChannel);
        NetworkChannel = D2RL::Network::InvalidChannelHandle;
        ReleaseSingleton();
        return false;
    }

    if (!Context->InstallInlineHook(
            SendRoomInSightPacketRva,
            SendRoomInSightPacketExpected.data(),
            static_cast<std::uint32_t>(
                SendRoomInSightPacketExpected.size()),
            HookSendRoomInSightPacket,
            &OriginalSendRoomInSightPacket)
            || !Context->InstallInlineHook(
                SendRoomOutOfSightPacketRva,
                SendRoomOutOfSightPacketExpected.data(),
                static_cast<std::uint32_t>(
                    SendRoomOutOfSightPacketExpected.size()),
                HookSendRoomOutOfSightPacket,
                &OriginalSendRoomOutOfSightPacket)
            || !Context->InstallInlineHook(
                SetClientInSightRva,
                SetClientInSightExpected.data(),
                static_cast<std::uint32_t>(
                    SetClientInSightExpected.size()),
                HookSetClientInSight,
                &OriginalSetClientInSight)
            || !Context->InstallInlineHook(
                UnsetClientInSightRva,
                UnsetClientInSightExpected.data(),
                static_cast<std::uint32_t>(
                    UnsetClientInSightExpected.size()),
                HookUnsetClientInSight,
                &OriginalUnsetClientInSight)) {
        Context->LogError(
            "ExtendedActLevelIds: a v2 room-visibility hook is already owned or unavailable.");
        (void)Network->unregisterChannel(Context, NetworkChannel);
        NetworkChannel = D2RL::Network::InvalidChannelHandle;
        ReleaseSingleton();
        return false;
    }

    Operational.store(true, std::memory_order_release);
    const auto message = std::string(
        "Extended Act Level IDs 2.0.0 by RuffnecKk active; native fingerprints and private compatibility channel accepted; build=")
        + RuntimeBuildName + ".";
    Context->LogInfo(message.c_str());
    return true;
}

D2RL_PLUGIN_EXPORT void D2RLoaderUnloadPlugin() noexcept {
    using namespace RuffnecKk::ExtendedActLevelIds;
    Operational.store(false, std::memory_order_release);
    if (Network
            && NetworkChannel != D2RL::Network::InvalidChannelHandle) {
        (void)Network->unregisterChannel(Context, NetworkChannel);
    }
    ResetState();
    ReleaseSingleton();
}
