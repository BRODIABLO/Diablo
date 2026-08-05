#define NOMINMAX

#include <D2RLPlugin/api.h>

#include "repeatable-services-policy.h"

#include <Windows.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace RuffnecKk::RepeatableServices {
namespace {

constexpr std::uint32_t SupportedBuild = 92777;
constexpr wchar_t ConfigFileName[] = L"RepeatableServices.json";

constexpr std::uintptr_t NpcDialogCallbackRva = 0x4A9880;
constexpr std::uintptr_t AkaraCallbackRva = 0x4B2530;
constexpr std::uintptr_t ItemTransactionRva = 0x4FC230;
constexpr std::uintptr_t ItemServiceSlotRva = 0x1E58F0;

constexpr std::uintptr_t NpcDialogResolveCallRva = 0x4A9A5A;
constexpr std::uintptr_t AkaraResolveCallRva = 0x4B272B;
constexpr std::uintptr_t ItemTransactionResolveCallRva = 0x4FC5E7;
constexpr std::uintptr_t ItemFinalValidatorCallRva = 0x4FD410;
constexpr std::uintptr_t ItemPaymentDispatchRva = 0x4FD446;
constexpr std::uintptr_t ItemPaymentContinueRva = 0x4FD44D;
constexpr std::uintptr_t AkaraResetCallRva = 0x4B2A23;
constexpr std::uintptr_t NpcActionQuestSetCheckCallRva = 0x114346;
constexpr std::uintptr_t NpcActionQuestClearCheckCallRva = 0x114351;
constexpr std::uintptr_t AkaraGrantedCheckCallRva = 0x114C2E;
constexpr std::uintptr_t AkaraPendingCheckCallRva = 0x114C48;
constexpr std::uintptr_t NpcMenuAddEntryCallRva = 0x114D4D;

constexpr std::uintptr_t ResolveServiceRecordRva = 0x3971A0;
constexpr std::uintptr_t QuestFlagCheckRva = 0x325C50;
constexpr std::uintptr_t GetPlayerDataRva = 0x34B240;
constexpr std::uintptr_t GetStatRva = 0x2F5020;
constexpr std::uintptr_t ItemFinalValidatorRva = 0x472590;
constexpr std::uintptr_t ResetStatsAndSkillsRva = 0x580F20;
constexpr std::uintptr_t TryDeductGoldRva = 0x5416D0;
constexpr std::uintptr_t ClientGetDataContextRva = 0x8B2D0;
constexpr std::uintptr_t ClientGetLocalPlayerRva = 0x9A480;
constexpr std::uintptr_t LangGetStringRva = 0x5F4A50;
constexpr std::uintptr_t NpcMenuAddEntryRva = 0x1E9030;
constexpr std::uintptr_t ItemServiceStateRva = 0x2A96964;

constexpr std::size_t GameDifficultyOffset = 0x104;
constexpr std::size_t PlayerQuestFlagsOffset = 0x40;
constexpr std::size_t ServiceRecordSize = 17;
constexpr std::size_t ServiceSelectorOffset = 0x0B;
constexpr std::size_t ServiceQuestOffset = 0x10;
constexpr std::int32_t PlayerLevelStat = 12;
constexpr std::int32_t PlayerGoldStat = 14;
constexpr std::int32_t PlayerGoldBankStat = 15;
constexpr std::int32_t RewardGranted = 0;
constexpr std::int32_t RewardPending = 1;
constexpr std::int32_t CharsiQuest = 3;
constexpr std::int32_t LarzukQuest = 0x23;
constexpr std::int32_t AnyaQuest = 0x26;
constexpr std::int32_t AkaraQuest = 0x29;

constexpr std::uint16_t ImbueStringId = 4017;
constexpr std::uint16_t RespecStringId = 11168;
constexpr std::uint16_t SocketingStringId = 22748;
constexpr std::uint16_t PersonalizationStringId = 22749;

constexpr std::array<std::uint8_t, 24> NpcDialogCallbackExpected{
    0x40, 0x55, 0x53, 0x56, 0x57, 0x41, 0x55, 0x41,
    0x57, 0x48, 0x8D, 0x6C, 0x24, 0xD1, 0x48, 0x81,
    0xEC, 0xC8, 0x00, 0x00, 0x00, 0x48, 0x8B, 0x05,
};
constexpr std::array<std::uint8_t, 24> AkaraCallbackExpected{
    0x40, 0x55, 0x53, 0x56, 0x57, 0x41, 0x56, 0x48,
    0x8D, 0x6C, 0x24, 0xC9, 0x48, 0x81, 0xEC, 0xC0,
    0x00, 0x00, 0x00, 0x48, 0x8B, 0x05, 0x7E, 0x8D,
};
constexpr std::array<std::uint8_t, 24> ItemTransactionExpected{
    0x40, 0x55, 0x41, 0x54, 0x41, 0x56, 0x41, 0x57,
    0x48, 0x8D, 0xAC, 0x24, 0xE8, 0xFC, 0xFF, 0xFF,
    0x48, 0x81, 0xEC, 0x18, 0x04, 0x00, 0x00, 0x48,
};
constexpr std::array<std::uint8_t, 24> ItemServiceSlotExpected{
    0x40, 0x57, 0x48, 0x83, 0xEC, 0x30, 0x48, 0x8B,
    0xF9, 0x48, 0x85, 0xC9, 0x0F, 0x84, 0x68, 0x01,
    0x00, 0x00, 0x48, 0x89, 0x5C, 0x24, 0x40, 0xE8,
};
constexpr std::array<std::uint8_t, 24> ResolveServiceRecordExpected{
    0x45, 0x33, 0xC0, 0x4C, 0x8D, 0x15, 0xB6, 0x78,
    0xFE, 0x01, 0x41, 0x8B, 0xC8, 0x4D, 0x8B, 0xCA,
    0x4D, 0x85, 0xC0, 0x75, 0x1C, 0x41, 0x0F, 0xBF,
};
constexpr std::array<std::uint8_t, 24> QuestFlagCheckExpected{
    0x48, 0x89, 0x5C, 0x24, 0x08, 0x48, 0x89, 0x74,
    0x24, 0x18, 0x57, 0x48, 0x83, 0xEC, 0x20, 0x41,
    0x8B, 0xF0, 0x8B, 0xDA, 0x48, 0x8B, 0xF9, 0x48,
};
constexpr std::array<std::uint8_t, 24> GetPlayerDataExpected{
    0x40, 0x53, 0x48, 0x83, 0xEC, 0x20, 0x48, 0x8B,
    0xD9, 0x48, 0x85, 0xC9, 0x75, 0x13, 0x88, 0x4C,
    0x24, 0x30, 0x48, 0x8D, 0x4C, 0x24, 0x30, 0xE8,
};
constexpr std::array<std::uint8_t, 24> GetStatExpected{
    0x48, 0x89, 0x5C, 0x24, 0x10, 0x48, 0x89, 0x6C,
    0x24, 0x18, 0x48, 0x89, 0x74, 0x24, 0x20, 0x57,
    0x48, 0x83, 0xEC, 0x20, 0x41, 0x0F, 0xB7, 0xE8,
};
constexpr std::array<std::uint8_t, 24> ItemFinalValidatorExpected{
    0x40, 0x55, 0x53, 0x56, 0x57, 0x41, 0x54, 0x41,
    0x55, 0x41, 0x56, 0x48, 0x8D, 0xAC, 0x24, 0x80,
    0x56, 0xFF, 0xFF, 0xB8, 0x80, 0xAA, 0x00, 0x00,
};
constexpr std::array<std::uint8_t, 24> ResetExpected{
    0x48, 0x89, 0x5C, 0x24, 0x08, 0x57, 0x48, 0x83,
    0xEC, 0x20, 0x48, 0x8B, 0xFA, 0x48, 0x8B, 0xD9,
    0xE8, 0xBB, 0x51, 0xEB, 0xFF, 0x48, 0x8B, 0xD7,
};
constexpr std::array<std::uint8_t, 32> TryDeductGoldExpected{
    0x48, 0x89, 0x5C, 0x24, 0x18, 0x48, 0x89, 0x74,
    0x24, 0x20, 0x57, 0x48, 0x83, 0xEC, 0x20, 0x48,
    0x8B, 0xDA, 0x41, 0x8B, 0xF0, 0x45, 0x33, 0xC0,
    0x48, 0x8B, 0xCB, 0x41, 0x8D, 0x50, 0x0E, 0xE8,
};
constexpr std::array<std::uint8_t, 24> ClientGetDataExpected{
    0x8B, 0x05, 0x2E, 0x84, 0x99, 0x02, 0xC3, 0xCC,
    0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
    0x8B, 0x05, 0x76, 0x84, 0x99, 0x02, 0xC3, 0xCC,
};
constexpr std::array<std::uint8_t, 24> ClientGetPlayerExpected{
    0x48, 0x89, 0x5C, 0x24, 0x08, 0x57, 0x48, 0x83,
    0xEC, 0x20, 0x83, 0xF9, 0x08, 0x0F, 0x83, 0x85,
    0x00, 0x00, 0x00, 0x8B, 0xD9, 0x48, 0x89, 0x5C,
};
constexpr std::array<std::uint8_t, 24> LangGetStringExpected{
    0x80, 0x3D, 0x05, 0xAE, 0xCF, 0x02, 0x00, 0x48,
    0x8D, 0x05, 0xDA, 0x96, 0xDB, 0x01, 0x4C, 0x8D,
    0x05, 0x13, 0x97, 0xDB, 0x01, 0x0F, 0xB7, 0xD1,
};
constexpr std::array<std::uint8_t, 32> NpcMenuAddEntryExpected{
    0x48, 0x89, 0x5C, 0x24, 0x08, 0x48, 0x89, 0x6C,
    0x24, 0x10, 0x56, 0x57, 0x41, 0x56, 0x48, 0x83,
    0xEC, 0x20, 0x83, 0x79, 0x1C, 0x0A, 0x41, 0x8B,
    0xD9, 0x41, 0x8B, 0xE8, 0x4C, 0x8B, 0xF2, 0x48,
};

constexpr std::array<std::uint8_t, 5> NpcDialogResolveCallExpected{
    0xE8, 0x41, 0xD7, 0xEE, 0xFF,
};
constexpr std::array<std::uint8_t, 5> AkaraResolveCallExpected{
    0xE8, 0x70, 0x4A, 0xEE, 0xFF,
};
constexpr std::array<std::uint8_t, 5> ItemTransactionResolveCallExpected{
    0xE8, 0xB4, 0xAB, 0xE9, 0xFF,
};
constexpr std::array<std::uint8_t, 5> ItemFinalValidatorCallExpected{
    0xE8, 0x7B, 0x51, 0xF7, 0xFF,
};
constexpr std::array<std::uint8_t, 18> ItemPaymentDispatchExpected{
    0x0F, 0xB6, 0x48, 0x0B, 0x83, 0xE9, 0x01, 0x0F,
    0x84, 0x4F, 0x01, 0x00, 0x00, 0x83, 0xE9, 0x01,
    0x74, 0x6E,
};
constexpr std::array<std::uint8_t, 5> AkaraResetCallExpected{
    0xE8, 0xF8, 0xE4, 0x0C, 0x00,
};
constexpr std::array<std::uint8_t, 5> NpcActionQuestSetCheckCallExpected{
    0xE8, 0x05, 0x19, 0x21, 0x00,
};
constexpr std::array<std::uint8_t, 5> NpcActionQuestClearCheckCallExpected{
    0xE8, 0xFA, 0x18, 0x21, 0x00,
};
constexpr std::array<std::uint8_t, 5> AkaraGrantedCheckCallExpected{
    0xE8, 0x1D, 0x10, 0x21, 0x00,
};
constexpr std::array<std::uint8_t, 5> AkaraPendingCheckCallExpected{
    0xE8, 0x03, 0x10, 0x21, 0x00,
};
constexpr std::array<std::uint8_t, 5> NpcMenuAddEntryCallExpected{
    0xE8, 0xDE, 0x42, 0x0D, 0x00,
};

using NpcCallbackFn = std::int32_t(__fastcall*)(
    void*, void*, const std::uint8_t*, std::int32_t
) noexcept;
using ItemTransactionFn = std::int32_t(__fastcall*)(
    void*, void*, void*, std::uint8_t, void*
) noexcept;
using ItemServiceSlotFn = std::uint8_t(__fastcall*)(void*) noexcept;
using ResolveServiceRecordFn = std::uint8_t*(__fastcall*)(
    std::uint8_t, std::int32_t
) noexcept;
using QuestFlagCheckFn = std::int32_t(__fastcall*)(
    void*, std::int32_t, std::int32_t
) noexcept;
using GetPlayerDataFn = std::uint8_t*(__fastcall*)(void*) noexcept;
using GetStatFn = std::int32_t(__fastcall*)(
    void*, std::int32_t, std::uint32_t
) noexcept;
using ItemFinalValidatorFn = std::uint8_t(__fastcall*)(
    void*, std::int32_t, void*, std::int32_t,
    void*, std::uintptr_t, std::uintptr_t, std::uint8_t
) noexcept;
using ResetStatsAndSkillsFn = std::int32_t(__fastcall*)(void*, void*) noexcept;
using TryDeductGoldFn = std::int32_t(__fastcall*)(
    void*, void*, std::int32_t
) noexcept;
using ClientGetDataContextFn = std::int32_t(__fastcall*)() noexcept;
using ClientGetLocalPlayerFn = void*(__fastcall*)(std::int32_t) noexcept;
using LangGetStringFn = const char*(__fastcall*)(std::uint16_t) noexcept;
using NpcMenuAddEntryFn = std::uintptr_t(__fastcall*)(
    void*, const char*, std::int32_t, std::int32_t, void*, std::int32_t
) noexcept;

enum class ContextKind : std::uint8_t {
    None,
    NpcDialog,
    Akara,
    ItemTransaction,
};

struct InvocationContext {
    ContextKind kind{ContextKind::None};
    void* game{};
    void* player{};
    Service service{Service::Count};
    bool virtualized{};
    bool charged{};
    std::array<std::uint8_t, ServiceRecordSize> record{};
};

struct InvocationScope {
    InvocationContext previous;

    InvocationScope(ContextKind kind, void* game, void* player) noexcept
        : previous(CurrentInvocation) {
        CurrentInvocation = {};
        CurrentInvocation.kind = kind;
        CurrentInvocation.game = game;
        CurrentInvocation.player = player;
    }

    ~InvocationScope() noexcept {
        CurrentInvocation = previous;
    }

    static thread_local InvocationContext CurrentInvocation;
};

thread_local InvocationContext InvocationScope::CurrentInvocation{};
thread_local std::array<char, 512> LabelBuffer{};
thread_local std::uint32_t ClientRepeatServices{};
thread_local Service ClientItemService{Service::Count};

#define CurrentInvocation InvocationScope::CurrentInvocation

const D2RL::PluginContext* Context{};
std::uint8_t* Base{};
Config Settings{};
std::string LoadedConfigPath{"built-in disabled defaults"};
std::atomic<bool> Operational{};
void* RelayPage{};
void* ItemPaymentDispatchRelay{};

NpcCallbackFn OriginalNpcDialogCallback{};
NpcCallbackFn OriginalAkaraCallback{};
ItemTransactionFn OriginalItemTransaction{};
ItemServiceSlotFn OriginalItemServiceSlot{};
ResolveServiceRecordFn ResolveServiceRecord{};
QuestFlagCheckFn QuestFlagCheck{};
GetPlayerDataFn GetPlayerData{};
GetStatFn GetStat{};
ItemFinalValidatorFn ItemFinalValidator{};
ResetStatsAndSkillsFn ResetStatsAndSkills{};
TryDeductGoldFn TryDeductGold{};
ClientGetDataContextFn ClientGetDataContext{};
ClientGetLocalPlayerFn ClientGetLocalPlayer{};
LangGetStringFn LangGetString{};
NpcMenuAddEntryFn NpcMenuAddEntry{};

template <typename Fn>
Fn At(std::uintptr_t rva) noexcept {
    return reinterpret_cast<Fn>(Base + rva);
}

std::vector<std::filesystem::path> ConfigCandidates() {
    std::vector<std::filesystem::path> candidates;
    if (Context && Context->pluginConfigPath
            && Context->pluginConfigPath[0] != L'\0') {
        const auto scopeConfigPath =
            std::filesystem::path(Context->pluginConfigPath);
        candidates.emplace_back(scopeConfigPath.parent_path() / ConfigFileName);
    }
    std::error_code currentPathError;
    const auto currentPath = std::filesystem::current_path(currentPathError);
    if (!currentPathError) {
        candidates.emplace_back(
            currentPath / L"d2rloader" / L"config" / ConfigFileName);
    }
    if (Context && Context->modDirectory && Context->modDirectory[0] != L'\0') {
        const auto modDirectory = std::filesystem::path(Context->modDirectory);
        if (Context->activeMod && Context->activeMod[0] != '\0') {
            candidates.emplace_back(
                modDirectory / (std::string(Context->activeMod) + ".mpq") / ConfigFileName
            );
        }
        candidates.emplace_back(modDirectory / ConfigFileName);
    }
    candidates.emplace_back(ConfigFileName);
    return candidates;
}

bool LoadConfig() noexcept {
    Settings = {};
    LoadedConfigPath = "built-in disabled defaults";
    for (const auto& path : ConfigCandidates()) {
        std::error_code error;
        if (!std::filesystem::is_regular_file(path, error)) continue;
        try {
            std::ifstream input(path);
            if (!input.is_open()) {
                throw std::invalid_argument("configuration file could not be opened");
            }
            const auto root = nlohmann::json::parse(input, nullptr, true, true);
            Settings = ParseConfig(root);
            LoadedConfigPath = path.string();
            return true;
        } catch (const std::exception& exception) {
            const auto message = std::string("RepeatableServices: invalid ")
                + path.string() + " (" + exception.what() + ").";
            Context->LogError(message.c_str());
            return false;
        }
    }
    return true;
}

Service ServiceFromSelector(std::uint8_t selector) noexcept {
    switch (selector) {
    case 1: return Service::Imbue;
    case 2: return Service::Socketing;
    case 4: return Service::Personalization;
    default: return Service::Count;
    }
}

std::int32_t QuestForService(Service service) noexcept {
    switch (service) {
    case Service::Imbue: return CharsiQuest;
    case Service::Socketing: return LarzukQuest;
    case Service::Personalization: return AnyaQuest;
    case Service::Respec: return AkaraQuest;
    default: return -1;
    }
}

Service ServiceFromQuest(std::int32_t quest) noexcept {
    switch (quest) {
    case CharsiQuest: return Service::Imbue;
    case LarzukQuest: return Service::Socketing;
    case AnyaQuest: return Service::Personalization;
    default: return Service::Count;
    }
}

void* ServerQuestFlags(void* game, void* player) noexcept {
    if (!game || !player) return nullptr;
    auto* playerData = GetPlayerData(player);
    if (!playerData) return nullptr;
    const auto difficulty = *(static_cast<const std::uint8_t*>(game) + GameDifficultyOffset);
    if (difficulty > 2) return nullptr;
    void* flags{};
    std::memcpy(
        &flags,
        playerData + PlayerQuestFlagsOffset + static_cast<std::size_t>(difficulty) * sizeof(void*),
        sizeof(flags)
    );
    return flags;
}

bool RewardConsumed(void* flags, std::int32_t quest) noexcept {
    return flags
        && QuestFlagCheck(flags, quest, RewardGranted) != 0
        && QuestFlagCheck(flags, quest, RewardPending) == 0;
}

bool ServerRewardConsumed(void* game, void* player, Service service) noexcept {
    return RewardConsumed(ServerQuestFlags(game, player), QuestForService(service));
}

bool ClientRewardConsumed(Service service) noexcept {
    const auto bit = std::uint32_t{1} << static_cast<std::uint32_t>(service);
    return (ClientRepeatServices & bit) != 0;
}

void SetClientRewardConsumed(Service service, bool consumed) noexcept {
    if (service == Service::Count) return;
    const auto bit = std::uint32_t{1} << static_cast<std::uint32_t>(service);
    if (consumed) {
        ClientRepeatServices |= bit;
    } else {
        ClientRepeatServices &= ~bit;
    }
}

std::uint8_t* VirtualizeItemServiceRecord(std::uint8_t* record) noexcept {
    if (!Operational.load(std::memory_order_acquire)
        || !record
        || CurrentInvocation.kind == ContextKind::None) {
        return record;
    }
    const auto service = ServiceFromSelector(record[ServiceSelectorOffset]);
    if (service == Service::Count) {
        return record;
    }
    const auto active = IsActive(For(Settings, service));
    const auto consumed = ServerRewardConsumed(CurrentInvocation.game, CurrentInvocation.player, service);
    if (Settings.diagnostics && CurrentInvocation.kind == ContextKind::ItemTransaction) {
        char message[256]{};
        std::snprintf(
            message,
            sizeof(message),
            "RepeatableServices: observed %.*s transaction record; selector=%u, questByte=%u, active=%u, consumed=%u.",
            static_cast<int>(ServiceName(service).size()),
            ServiceName(service).data(),
            static_cast<unsigned>(record[ServiceSelectorOffset]),
            static_cast<unsigned>(record[ServiceQuestOffset]),
            active ? 1u : 0u,
            consumed ? 1u : 0u
        );
        Context->LogInfo(message);
    }
    if (!active || !consumed) return record;
    std::memcpy(CurrentInvocation.record.data(), record, CurrentInvocation.record.size());
    CurrentInvocation.record[ServiceQuestOffset] = 0;
    CurrentInvocation.service = service;
    CurrentInvocation.virtualized = true;
    return CurrentInvocation.record.data();
}

std::uint8_t* VirtualizeAkaraRecord(std::uint8_t* record) noexcept {
    const auto& config = For(Settings, Service::Respec);
    if (!Operational.load(std::memory_order_acquire)
        || !record
        || CurrentInvocation.kind != ContextKind::Akara
        || !IsActive(config)
        || !ServerRewardConsumed(CurrentInvocation.game, CurrentInvocation.player, Service::Respec)) {
        return record;
    }
    std::memcpy(CurrentInvocation.record.data(), record, CurrentInvocation.record.size());
    CurrentInvocation.record[ServiceQuestOffset] = 0;
    CurrentInvocation.service = Service::Respec;
    CurrentInvocation.virtualized = true;
    return CurrentInvocation.record.data();
}

std::uint32_t InvocationPrice(Service service) noexcept {
    const auto level = CurrentInvocation.player
        ? GetStat(CurrentInvocation.player, PlayerLevelStat, 0)
        : 0;
    return Price(For(Settings, service), level);
}

std::uint64_t PlayerAvailableGold(void* player) noexcept {
    if (!player) return 0;
    return AvailableGold(
        GetStat(player, PlayerGoldStat, 0),
        GetStat(player, PlayerGoldBankStat, 0)
    );
}

bool PlayerCanAfford(void* player, Service service) noexcept {
    const auto& config = For(Settings, service);
    if (config.mode != Mode::Paid) return true;
    const auto level = player ? GetStat(player, PlayerLevelStat, 0) : 0;
    return PlayerAvailableGold(player) >= Price(config, level);
}

void LogInsufficientFunds(Service service, std::uint32_t price, const char* stage) noexcept {
    if (!Settings.diagnostics) return;
    char message[320]{};
    std::snprintf(
        message,
        sizeof(message),
        "RepeatableServices: blocked %.*s repeat before %s; price=%u and funds are insufficient.",
        static_cast<int>(ServiceName(service).size()),
        ServiceName(service).data(),
        stage,
        price
    );
    Context->LogInfo(message);
}

bool ChargeInvocation() noexcept {
    if (!CurrentInvocation.virtualized || CurrentInvocation.service == Service::Count) return true;
    const auto& config = For(Settings, CurrentInvocation.service);
    if (config.mode != Mode::Paid || CurrentInvocation.charged) return true;
    const auto price = InvocationPrice(CurrentInvocation.service);
    if (price != 0 && TryDeductGold(
            CurrentInvocation.game,
            CurrentInvocation.player,
            static_cast<std::int32_t>(price)
        ) == 0) {
        CurrentInvocation.charged = true;
        Context->LogError(
            "RepeatableServices: authoritative payment failed after an accepted preflight; "
            "the service is allowed without charge to preserve the item and release the UI."
        );
        return true;
    }
    CurrentInvocation.charged = true;
    if (Settings.diagnostics) {
        char message[256]{};
        std::snprintf(
            message,
            sizeof(message),
            "RepeatableServices: accepted %.*s repeat; price=%u.",
            static_cast<int>(ServiceName(CurrentInvocation.service).size()),
            ServiceName(CurrentInvocation.service).data(),
            price
        );
        Context->LogInfo(message);
    }
    return true;
}

std::int32_t __fastcall HookNpcDialogCallback(
    void* game,
    void* player,
    const std::uint8_t* packet,
    std::int32_t packetSize
) noexcept {
    if (!Operational.load(std::memory_order_acquire)) {
        return OriginalNpcDialogCallback(game, player, packet, packetSize);
    }
    InvocationScope scope(ContextKind::NpcDialog, game, player);
    return OriginalNpcDialogCallback(game, player, packet, packetSize);
}

std::int32_t __fastcall HookAkaraCallback(
    void* game,
    void* player,
    const std::uint8_t* packet,
    std::int32_t packetSize
) noexcept {
    if (!Operational.load(std::memory_order_acquire)) {
        return OriginalAkaraCallback(game, player, packet, packetSize);
    }
    InvocationScope scope(ContextKind::Akara, game, player);
    return OriginalAkaraCallback(game, player, packet, packetSize);
}

std::int32_t __fastcall HookItemTransaction(
    void* game,
    void* player,
    void* packetState,
    std::uint8_t phase,
    void* context
) noexcept {
    if (!Operational.load(std::memory_order_acquire) || phase != 0) {
        return OriginalItemTransaction(game, player, packetState, phase, context);
    }
    InvocationScope scope(ContextKind::ItemTransaction, game, player);
    return OriginalItemTransaction(game, player, packetState, phase, context);
}

std::uint8_t* __fastcall HookNpcDialogResolve(
    std::uint8_t dataContext,
    std::int32_t npcClassId
) noexcept {
    return VirtualizeItemServiceRecord(ResolveServiceRecord(dataContext, npcClassId));
}

std::uint8_t* __fastcall HookAkaraResolve(
    std::uint8_t dataContext,
    std::int32_t npcClassId
) noexcept {
    auto* record = ResolveServiceRecord(dataContext, npcClassId);
    const auto repeat = Operational.load(std::memory_order_acquire)
        && record
        && IsActive(For(Settings, Service::Respec))
        && ServerRewardConsumed(
            CurrentInvocation.game,
            CurrentInvocation.player,
            Service::Respec
        );
    if (repeat && !PlayerCanAfford(CurrentInvocation.player, Service::Respec)) {
        const auto price = InvocationPrice(Service::Respec);
        LogInsufficientFunds(Service::Respec, price, "Akara reset virtualization");
        return record;
    }
    return VirtualizeAkaraRecord(record);
}

std::uint8_t* __fastcall HookItemTransactionResolve(
    std::uint8_t dataContext,
    std::int32_t npcClassId
) noexcept {
    auto* record = ResolveServiceRecord(dataContext, npcClassId);
    if (!Operational.load(std::memory_order_acquire) || !record) return record;
    const auto service = ServiceFromSelector(record[ServiceSelectorOffset]);
    const auto repeat = service != Service::Count
        && IsActive(For(Settings, service))
        && ServerRewardConsumed(CurrentInvocation.game, CurrentInvocation.player, service);
    if (repeat && !PlayerCanAfford(CurrentInvocation.player, service)) {
        const auto price = InvocationPrice(service);
        LogInsufficientFunds(service, price, "record virtualization");
        return record;
    }
    return VirtualizeItemServiceRecord(record);
}

std::uint8_t __fastcall HookItemFinalValidator(
    void* player,
    std::int32_t argument2,
    void* packetState,
    std::int32_t argument4,
    void* argument5,
    std::uintptr_t argument6,
    std::uintptr_t argument7,
    std::uint8_t argument8
) noexcept {
    const auto valid = ItemFinalValidator(
        player,
        argument2,
        packetState,
        argument4,
        argument5,
        argument6,
        argument7,
        argument8
    );
    if (!valid || !Operational.load(std::memory_order_acquire)) return valid;
    return ChargeInvocation() ? valid : 0;
}

std::int32_t __fastcall HookAkaraReset(void* game, void* player) noexcept {
    if (Operational.load(std::memory_order_acquire)
        && CurrentInvocation.kind == ContextKind::Akara
        && CurrentInvocation.virtualized
        && !ChargeInvocation()) {
        return 0;
    }
    return ResetStatsAndSkills(game, player);
}

std::int32_t __fastcall HookNpcActionQuestFlagCheck(
    void* flags,
    std::int32_t quest,
    std::int32_t flag
) noexcept {
    const auto result = QuestFlagCheck(flags, quest, flag);
    const auto service = ServiceFromQuest(quest);
    if (service == Service::Count || flag != RewardPending) {
        return result;
    }
    const auto repeat = Operational.load(std::memory_order_acquire)
        && IsActive(For(Settings, service))
        && result == 0
        && QuestFlagCheck(flags, quest, RewardGranted) != 0;
    SetClientRewardConsumed(service, repeat);
    return repeat ? 1 : result;
}

std::int32_t __fastcall HookAkaraGrantedCheck(
    void* flags,
    std::int32_t quest,
    std::int32_t flag
) noexcept {
    const auto result = QuestFlagCheck(flags, quest, flag);
    if (quest != AkaraQuest || flag != RewardGranted) {
        return result;
    }
    const auto repeat = Operational.load(std::memory_order_acquire)
        && IsActive(For(Settings, Service::Respec))
        && result != 0
        && QuestFlagCheck(flags, quest, RewardPending) == 0;
    SetClientRewardConsumed(Service::Respec, repeat);
    return repeat ? 0 : result;
}

std::int32_t __fastcall HookAkaraPendingCheck(
    void* flags,
    std::int32_t quest,
    std::int32_t flag
) noexcept {
    const auto result = QuestFlagCheck(flags, quest, flag);
    if (quest != AkaraQuest || flag != RewardPending) {
        return result;
    }
    const auto repeat = Operational.load(std::memory_order_acquire)
        && IsActive(For(Settings, Service::Respec))
        && result == 0
        && QuestFlagCheck(flags, quest, RewardGranted) != 0;
    SetClientRewardConsumed(Service::Respec, repeat);
    return repeat ? 1 : result;
}

Service ServiceFromStringId(std::uint16_t stringId) noexcept {
    switch (stringId) {
    case RespecStringId: return Service::Respec;
    case ImbueStringId: return Service::Imbue;
    case SocketingStringId: return Service::Socketing;
    case PersonalizationStringId: return Service::Personalization;
    default: return Service::Count;
    }
}

Service ServiceFromMenuLabel(const char* label) noexcept {
    if (!label) return Service::Count;
    constexpr std::array<std::uint16_t, 4> ids{
        RespecStringId,
        ImbueStringId,
        SocketingStringId,
        PersonalizationStringId,
    };
    for (const auto id : ids) {
        const auto* localized = LangGetString(id);
        if (localized && std::strcmp(label, localized) == 0) {
            return ServiceFromStringId(id);
        }
    }
    return Service::Count;
}

const char* PricedMenuLabel(
    Service service,
    const char* original
) noexcept {
    const auto& config = For(Settings, service);
    if (config.mode == Mode::Free) {
        _snprintf_s(LabelBuffer.data(), LabelBuffer.size(), _TRUNCATE, "%s [Free]", original);
        return LabelBuffer.data();
    }
    auto* player = ClientGetLocalPlayer(ClientGetDataContext());
    const auto level = player ? GetStat(player, PlayerLevelStat, 0) : 0;
    const auto price = Price(config, level);
    _snprintf_s(
        LabelBuffer.data(),
        LabelBuffer.size(),
        _TRUNCATE,
        "%s [%u gold]",
        original,
        price
    );
    return LabelBuffer.data();
}

std::uint8_t __fastcall HookItemServiceSlot(void* item) noexcept {
    const auto state = *reinterpret_cast<volatile std::int32_t*>(Base + ItemServiceStateRva);
    const auto service = ClientItemService;
    const auto repeat = Operational.load(std::memory_order_acquire)
        && state == 1
        && service != Service::Count
        && IsActive(For(Settings, service))
        && ClientRewardConsumed(service);
    auto* player = repeat ? ClientGetLocalPlayer(ClientGetDataContext()) : nullptr;
    const auto level = player ? GetStat(player, PlayerLevelStat, 0) : 0;
    const auto carriedGold = player ? GetStat(player, PlayerGoldStat, 0) : 0;
    const auto personalStashGold = player ? GetStat(player, PlayerGoldBankStat, 0) : 0;
    if (service != Service::Count && ShouldBlockItemDeposit(
            state == 1,
            repeat,
            For(Settings, service),
            level,
            carriedGold,
            personalStashGold
        )) {
        LogInsufficientFunds(service, Price(For(Settings, service), level), "client item-slot capture");
        return 0;
    }
    return OriginalItemServiceSlot(item);
}

std::uintptr_t __fastcall HookNpcMenuAddEntry(
    void* menu,
    const char* label,
    std::int32_t style,
    std::int32_t enabled,
    void* callback,
    std::int32_t closeOnSelect
) noexcept {
    const auto service = ServiceFromMenuLabel(label);
    if (service != Service::Count) {
        ClientItemService = service == Service::Respec ? Service::Count : service;
    }
    const auto repeat = Operational.load(std::memory_order_acquire)
        && service != Service::Count
        && IsActive(For(Settings, service))
        && ClientRewardConsumed(service);
    const auto* displayed = repeat ? PricedMenuLabel(service, label) : label;
    if (Settings.diagnostics && service != Service::Count) {
        char message[256]{};
        std::snprintf(
            message,
            sizeof(message),
            "RepeatableServices: menu %.*s label; repeat=%d, decorated=%d.",
            static_cast<int>(ServiceName(service).size()),
            ServiceName(service).data(),
            ClientRewardConsumed(service) ? 1 : 0,
            repeat ? 1 : 0
        );
        Context->LogInfo(message);
    }
    return NpcMenuAddEntry(
        menu,
        displayed,
        style,
        enabled,
        callback,
        closeOnSelect
    );
}

template <std::size_t Size>
bool Matches(std::uintptr_t rva, const std::array<std::uint8_t, Size>& expected) noexcept {
    return Context->CheckExpectedBytes(
        rva,
        expected.data(),
        static_cast<std::uint32_t>(expected.size())
    );
}

bool ValidateSignatures() noexcept {
    const auto valid = Matches(NpcDialogCallbackRva, NpcDialogCallbackExpected)
        && Matches(AkaraCallbackRva, AkaraCallbackExpected)
        && Matches(ItemTransactionRva, ItemTransactionExpected)
        && Matches(ItemServiceSlotRva, ItemServiceSlotExpected)
        && Matches(ResolveServiceRecordRva, ResolveServiceRecordExpected)
        && Matches(QuestFlagCheckRva, QuestFlagCheckExpected)
        && Matches(GetPlayerDataRva, GetPlayerDataExpected)
        && Matches(GetStatRva, GetStatExpected)
        && Matches(ItemFinalValidatorRva, ItemFinalValidatorExpected)
        && Matches(ResetStatsAndSkillsRva, ResetExpected)
        && Matches(TryDeductGoldRva, TryDeductGoldExpected)
        && Matches(ClientGetDataContextRva, ClientGetDataExpected)
        && Matches(ClientGetLocalPlayerRva, ClientGetPlayerExpected)
        && Matches(LangGetStringRva, LangGetStringExpected)
        && Matches(NpcMenuAddEntryRva, NpcMenuAddEntryExpected)
        && Matches(NpcDialogResolveCallRva, NpcDialogResolveCallExpected)
        && Matches(AkaraResolveCallRva, AkaraResolveCallExpected)
        && Matches(ItemTransactionResolveCallRva, ItemTransactionResolveCallExpected)
        && Matches(ItemFinalValidatorCallRva, ItemFinalValidatorCallExpected)
        && Matches(ItemPaymentDispatchRva, ItemPaymentDispatchExpected)
        && Matches(AkaraResetCallRva, AkaraResetCallExpected)
        && Matches(NpcActionQuestSetCheckCallRva, NpcActionQuestSetCheckCallExpected)
        && Matches(NpcActionQuestClearCheckCallRva, NpcActionQuestClearCheckCallExpected)
        && Matches(AkaraGrantedCheckCallRva, AkaraGrantedCheckCallExpected)
        && Matches(AkaraPendingCheckCallRva, AkaraPendingCheckCallExpected)
        && Matches(NpcMenuAddEntryCallRva, NpcMenuAddEntryCallExpected);
    if (!valid) {
        Context->LogError(
            "RepeatableServices: D2R 3.2.92777 signature mismatch or hook collision; plugin refused."
        );
    }
    return valid;
}

struct CallPatch {
    std::uintptr_t rva;
    const std::uint8_t* expected;
    std::uint32_t expectedSize;
    void* target;
    const char* name;
};

std::array<CallPatch, 10> CallPatches() noexcept {
    return {{
        {NpcDialogResolveCallRva, NpcDialogResolveCallExpected.data(), 5,
            reinterpret_cast<void*>(&HookNpcDialogResolve), "NPC dialog record"},
        {AkaraResolveCallRva, AkaraResolveCallExpected.data(), 5,
            reinterpret_cast<void*>(&HookAkaraResolve), "Akara record"},
        {ItemTransactionResolveCallRva, ItemTransactionResolveCallExpected.data(), 5,
            reinterpret_cast<void*>(&HookItemTransactionResolve), "item transaction record"},
        {ItemFinalValidatorCallRva, ItemFinalValidatorCallExpected.data(), 5,
            reinterpret_cast<void*>(&HookItemFinalValidator), "item payment"},
        {AkaraResetCallRva, AkaraResetCallExpected.data(), 5,
            reinterpret_cast<void*>(&HookAkaraReset), "Akara payment"},
        {NpcActionQuestSetCheckCallRva, NpcActionQuestSetCheckCallExpected.data(), 5,
            reinterpret_cast<void*>(&HookNpcActionQuestFlagCheck), "NPC action set gate"},
        {NpcActionQuestClearCheckCallRva, NpcActionQuestClearCheckCallExpected.data(), 5,
            reinterpret_cast<void*>(&HookNpcActionQuestFlagCheck), "NPC action clear gate"},
        {AkaraGrantedCheckCallRva, AkaraGrantedCheckCallExpected.data(), 5,
            reinterpret_cast<void*>(&HookAkaraGrantedCheck), "Akara granted gate"},
        {AkaraPendingCheckCallRva, AkaraPendingCheckCallExpected.data(), 5,
            reinterpret_cast<void*>(&HookAkaraPendingCheck), "Akara pending gate"},
        {NpcMenuAddEntryCallRva, NpcMenuAddEntryCallExpected.data(), 5,
            reinterpret_cast<void*>(&HookNpcMenuAddEntry), "menu price label"},
    }};
}

void* AllocateRelayPage(const std::array<CallPatch, 10>& patches) noexcept {
    SYSTEM_INFO systemInfo{};
    GetSystemInfo(&systemInfo);
    const auto minimumAddress = reinterpret_cast<std::uintptr_t>(systemInfo.lpMinimumApplicationAddress);
    const auto maximumAddress = reinterpret_cast<std::uintptr_t>(systemInfo.lpMaximumApplicationAddress);
    constexpr auto maximumDistance = static_cast<std::uintptr_t>(std::numeric_limits<std::int32_t>::max());

    std::uintptr_t minimumAfter = std::numeric_limits<std::uintptr_t>::max();
    std::uintptr_t maximumAfter{};
    for (const auto& patch : patches) {
        const auto after = reinterpret_cast<std::uintptr_t>(Base + patch.rva + 5);
        minimumAfter = std::min(minimumAfter, after);
        maximumAfter = std::max(maximumAfter, after);
    }
    const auto lowerBound = std::max(
        minimumAddress,
        maximumAfter > maximumDistance ? maximumAfter - maximumDistance : minimumAddress
    );
    const auto upperBound = std::min(
        maximumAddress,
        minimumAfter < maximumAddress - maximumDistance
            ? minimumAfter + maximumDistance
            : maximumAddress
    );
    const auto granularity = static_cast<std::uintptr_t>(systemInfo.dwAllocationGranularity);
    for (auto cursor = lowerBound; cursor < upperBound;) {
        MEMORY_BASIC_INFORMATION region{};
        if (VirtualQuery(reinterpret_cast<void*>(cursor), &region, sizeof(region)) == 0) break;
        const auto regionBase = reinterpret_cast<std::uintptr_t>(region.BaseAddress);
        const auto regionEnd = regionBase + region.RegionSize;
        if (region.State == MEM_FREE) {
            const auto first = std::max(regionBase, lowerBound);
            const auto candidate = (first + granularity - 1) & ~(granularity - 1);
            if (candidate < upperBound && candidate + systemInfo.dwPageSize <= regionEnd) {
                if (auto* page = VirtualAlloc(
                        reinterpret_cast<void*>(candidate),
                        systemInfo.dwPageSize,
                        MEM_RESERVE | MEM_COMMIT,
                        PAGE_READWRITE
                    )) {
                    return page;
                }
            }
        }
        if (regionEnd <= cursor) break;
        cursor = regionEnd;
    }
    return nullptr;
}

bool CreateCallRelays(const std::array<CallPatch, 10>& patches) noexcept {
    constexpr std::size_t relayStride = 16;
    constexpr std::size_t paymentRelayOffset = 10 * relayStride;
    constexpr std::size_t paymentRelayCapacity = 256;
    RelayPage = AllocateRelayPage(patches);
    if (!RelayPage) {
        Context->LogError("RepeatableServices: could not allocate rel32 relays near D2R.exe.");
        return false;
    }
    auto* bytes = static_cast<std::uint8_t*>(RelayPage);
    for (std::size_t index = 0; index < patches.size(); ++index) {
        auto* relay = bytes + index * relayStride;
        relay[0] = 0x48;
        relay[1] = 0xB8;
        const auto target = reinterpret_cast<std::uintptr_t>(patches[index].target);
        std::memcpy(relay + 2, &target, sizeof(target));
        relay[10] = 0xFF;
        relay[11] = 0xE0;
        std::memset(relay + 12, 0xCC, relayStride - 12);
    }

    ItemPaymentDispatchRelay = bytes + paymentRelayOffset;
    auto* cursor = static_cast<std::uint8_t*>(ItemPaymentDispatchRelay);
    const auto* paymentRelayEnd = cursor + paymentRelayCapacity;
    const auto emit = [&cursor](std::initializer_list<std::uint8_t> sequence) noexcept {
        for (const auto byte : sequence) *cursor++ = byte;
    };
    const auto emitAddress = [&cursor](std::uintptr_t address) noexcept {
        std::memcpy(cursor, &address, sizeof(address));
        cursor += sizeof(address);
    };

    // Preserve every volatile integer and vector register before calling C++
    // from this mid-function seam. Seven pushes plus 0x88 bytes retain Windows
    // x64 call alignment and provide the mandatory 32-byte shadow space.
    emit({0x50, 0x51, 0x52, 0x41, 0x50, 0x41, 0x51, 0x41, 0x52, 0x41, 0x53});
    emit({0x48, 0x81, 0xEC, 0x88, 0x00, 0x00, 0x00});
    emit({0xF3, 0x0F, 0x7F, 0x44, 0x24, 0x20});
    emit({0xF3, 0x0F, 0x7F, 0x4C, 0x24, 0x30});
    emit({0xF3, 0x0F, 0x7F, 0x54, 0x24, 0x40});
    emit({0xF3, 0x0F, 0x7F, 0x5C, 0x24, 0x50});
    emit({0xF3, 0x0F, 0x7F, 0x64, 0x24, 0x60});
    emit({0xF3, 0x0F, 0x7F, 0x6C, 0x24, 0x70});
    emit({0x48, 0xB8});
    emitAddress(reinterpret_cast<std::uintptr_t>(&ChargeInvocation));
    emit({0xFF, 0xD0});
    emit({0xF3, 0x0F, 0x6F, 0x44, 0x24, 0x20});
    emit({0xF3, 0x0F, 0x6F, 0x4C, 0x24, 0x30});
    emit({0xF3, 0x0F, 0x6F, 0x54, 0x24, 0x40});
    emit({0xF3, 0x0F, 0x6F, 0x5C, 0x24, 0x50});
    emit({0xF3, 0x0F, 0x6F, 0x64, 0x24, 0x60});
    emit({0xF3, 0x0F, 0x6F, 0x6C, 0x24, 0x70});
    emit({0x48, 0x81, 0xC4, 0x88, 0x00, 0x00, 0x00});
    emit({0x41, 0x5B, 0x41, 0x5A, 0x41, 0x59, 0x41, 0x58, 0x5A, 0x59, 0x58});

    // Affordability is rejected before record virtualization. At this late
    // post-validation seam the transaction must always continue so an item can
    // never be stranded in the NPC service UI by a payment anomaly.
    emit({0x0F, 0xB6, 0x48, 0x0B, 0x83, 0xE9, 0x01});
    emit({0xFF, 0x25, 0x00, 0x00, 0x00, 0x00});
    emitAddress(reinterpret_cast<std::uintptr_t>(Base + ItemPaymentContinueRva));

    if (cursor > paymentRelayEnd) {
        Context->LogError("RepeatableServices: item payment relay exceeded its fixed capacity.");
        return false;
    }
    std::memset(cursor, 0xCC, static_cast<std::size_t>(paymentRelayEnd - cursor));
    DWORD oldProtection{};
    if (!VirtualProtect(
            RelayPage,
            paymentRelayOffset + paymentRelayCapacity,
            PAGE_EXECUTE_READ,
            &oldProtection
        )) {
        Context->LogError("RepeatableServices: could not protect the call relay page.");
        return false;
    }
    FlushInstructionCache(
        GetCurrentProcess(),
        RelayPage,
        paymentRelayOffset + paymentRelayCapacity
    );
    return true;
}

bool InstallHooks() noexcept {
    const auto patches = CallPatches();
    if (!CreateCallRelays(patches)) return false;
    constexpr std::size_t relayStride = 16;
    auto* relayBytes = static_cast<std::uint8_t*>(RelayPage);
    for (std::size_t index = 0; index < patches.size(); ++index) {
        const auto* relay = relayBytes + index * relayStride;
        const auto callAfter = Base + patches[index].rva + 5;
        const auto distance = reinterpret_cast<std::intptr_t>(relay)
            - reinterpret_cast<std::intptr_t>(callAfter);
        if (distance < std::numeric_limits<std::int32_t>::min()
            || distance > std::numeric_limits<std::int32_t>::max()) {
            Context->LogError("RepeatableServices: an allocated relay is outside rel32 range.");
            return false;
        }
        std::array<std::uint8_t, 5> call{0xE8, 0, 0, 0, 0};
        const auto displacement = static_cast<std::int32_t>(distance);
        std::memcpy(call.data() + 1, &displacement, sizeof(displacement));
        if (!Context->PatchBytes(
                patches[index].rva,
                patches[index].expected,
                patches[index].expectedSize,
                call.data(),
                static_cast<std::uint32_t>(call.size())
            )) {
            const auto message = std::string("RepeatableServices: failed to install ")
                + patches[index].name + " call hook; feature remains inactive.";
            Context->LogError(message.c_str());
            return false;
        }
    }

    const auto paymentAfter = Base + ItemPaymentDispatchRva + 5;
    const auto paymentDistance = reinterpret_cast<std::intptr_t>(ItemPaymentDispatchRelay)
        - reinterpret_cast<std::intptr_t>(paymentAfter);
    if (paymentDistance < std::numeric_limits<std::int32_t>::min()
        || paymentDistance > std::numeric_limits<std::int32_t>::max()) {
        Context->LogError("RepeatableServices: item payment dispatch relay is outside rel32 range.");
        return false;
    }
    std::array<std::uint8_t, 7> paymentJump{0xE9, 0, 0, 0, 0, 0x90, 0x90};
    const auto paymentDisplacement = static_cast<std::int32_t>(paymentDistance);
    std::memcpy(paymentJump.data() + 1, &paymentDisplacement, sizeof(paymentDisplacement));
    if (!Context->PatchBytes(
            ItemPaymentDispatchRva,
            ItemPaymentDispatchExpected.data(),
            static_cast<std::uint32_t>(paymentJump.size()),
            paymentJump.data(),
            static_cast<std::uint32_t>(paymentJump.size())
        )) {
        Context->LogError(
            "RepeatableServices: failed to install universal item payment dispatch; feature remains inactive."
        );
        return false;
    }

    return Context->InstallInlineHook(
            NpcDialogCallbackRva,
            NpcDialogCallbackExpected.data(),
            static_cast<std::uint32_t>(NpcDialogCallbackExpected.size()),
            HookNpcDialogCallback,
            &OriginalNpcDialogCallback
        )
        && Context->InstallInlineHook(
            AkaraCallbackRva,
            AkaraCallbackExpected.data(),
            static_cast<std::uint32_t>(AkaraCallbackExpected.size()),
            HookAkaraCallback,
            &OriginalAkaraCallback
        )
        && Context->InstallInlineHook(
            ItemTransactionRva,
            ItemTransactionExpected.data(),
            static_cast<std::uint32_t>(ItemTransactionExpected.size()),
            HookItemTransaction,
            &OriginalItemTransaction
        )
        && Context->InstallInlineHook(
            ItemServiceSlotRva,
            ItemServiceSlotExpected.data(),
            static_cast<std::uint32_t>(ItemServiceSlotExpected.size()),
            HookItemServiceSlot,
            &OriginalItemServiceSlot
        );
}

void ResolveNativeFunctions() noexcept {
    ResolveServiceRecord = At<ResolveServiceRecordFn>(ResolveServiceRecordRva);
    QuestFlagCheck = At<QuestFlagCheckFn>(QuestFlagCheckRva);
    GetPlayerData = At<GetPlayerDataFn>(GetPlayerDataRva);
    GetStat = At<GetStatFn>(GetStatRva);
    ItemFinalValidator = At<ItemFinalValidatorFn>(ItemFinalValidatorRva);
    ResetStatsAndSkills = At<ResetStatsAndSkillsFn>(ResetStatsAndSkillsRva);
    TryDeductGold = At<TryDeductGoldFn>(TryDeductGoldRva);
    ClientGetDataContext = At<ClientGetDataContextFn>(ClientGetDataContextRva);
    ClientGetLocalPlayer = At<ClientGetLocalPlayerFn>(ClientGetLocalPlayerRva);
    LangGetString = At<LangGetStringFn>(LangGetStringRva);
    NpcMenuAddEntry = At<NpcMenuAddEntryFn>(NpcMenuAddEntryRva);
}

} // namespace
} // namespace RuffnecKk::RepeatableServices

namespace {

constexpr D2RL::PluginInfo Info{
    .infoSize = D2RL::PluginInfoSize,
    .apiVersion = D2RL_PLUGIN_API_VERSION,
    .id = "ruffneckk-repeatable-services",
    .name = "Repeatable Services",
    .version = "0.1.9",
    .author = "RuffnecKk",
    .description = "Repeats completed quest services for free or level-scaled gold.",
    .flags = D2RL::PluginFlags::NativeHooks,
};

} // namespace

D2RL_PLUGIN_EXPORT auto D2RLoaderGetPluginInfo() noexcept -> const D2RL::PluginInfo* {
    return &Info;
}

D2RL_PLUGIN_EXPORT auto D2RLoaderLoadPlugin(const D2RL::PluginContext* context) noexcept -> bool {
    using namespace RuffnecKk::RepeatableServices;
    Context = context;
    Base = context ? reinterpret_cast<std::uint8_t*>(context->exeBase) : nullptr;
    if (!Context || !Base) return false;
    if (context->modDataVersionBuild != 0 && context->modDataVersionBuild != SupportedBuild) {
        context->LogError("RepeatableServices: supports only D2R 3.2 build 92777.");
        return false;
    }
    if (!LoadConfig()) return false;
    if (!AnyActive(Settings)) {
        const auto message = std::string("RepeatableServices 0.1.9 by RuffnecKk loaded disabled; config=")
            + LoadedConfigPath + ".";
        context->LogInfo(message.c_str());
        return true;
    }
    ResolveNativeFunctions();
    if (!ValidateSignatures()) return false;
    if (!InstallHooks()) {
        Operational.store(false, std::memory_order_release);
        context->LogError(
            "RepeatableServices: hook installation was incomplete; the loaded DLL is retained as an inactive safety barrier."
        );
        return true;
    }
    Operational.store(true, std::memory_order_release);
    const auto message = std::string("RepeatableServices 0.1.9 by RuffnecKk active; config=")
        + LoadedConfigPath + "; first rewards remain native.";
    context->LogInfo(message.c_str());
    return true;
}

D2RL_PLUGIN_EXPORT auto D2RLoaderUnloadPlugin() noexcept -> void {
    RuffnecKk::RepeatableServices::Operational.store(false, std::memory_order_release);
}
