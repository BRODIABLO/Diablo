#define NOMINMAX
#include <D2RLPlugin/api.h>
#include <Windows.h>
#include <nlohmann/json.hpp>

#include "mass_id_policy.hpp"

#include <array>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace ruffneckk::mass_id;

constexpr std::uint32_t SupportedBuild = 92777;
constexpr wchar_t ConfigFileName[] = L"MassID.json";

constexpr std::uintptr_t SendTwentyOneBytePacketRva = 0x0EC820;
constexpr std::uintptr_t GetLocalDataContextRva = 0x08B2D0;
constexpr std::uintptr_t GetLocalPlayerRva = 0x09A480;
constexpr std::uintptr_t GetMouseStateRva = 0x14F210;
constexpr std::uintptr_t InventoryClickHandlerRva = 0x2C7540;
constexpr std::uintptr_t GetUnitStatRva = 0x2F5020;
constexpr std::uintptr_t GetUnitIdRva = 0x34A330;
constexpr std::uintptr_t GetUnitInventoryRva = 0x34A360;
constexpr std::uintptr_t GetUnitTypeRva = 0x34B9D0;
constexpr std::uintptr_t GetInventoryPageRva = 0x36CFE0;
constexpr std::uintptr_t CheckItemFlagRva = 0x36E2D0;
constexpr std::uintptr_t GetItemCodeRva = 0x36EF50;
constexpr std::uintptr_t GetCursorItemRva = 0x388A70;
constexpr std::uintptr_t GetFirstItemRva = 0x388C10;
constexpr std::uintptr_t GetNextItemRva = 0x38ABA0;
constexpr std::uintptr_t GetParentInventoryRva = 0x38AC50;
constexpr std::uintptr_t IdentifyStoredItemRva = 0x46EA70;
constexpr std::uintptr_t SynchronizeQuantityRva = 0x46F090;
constexpr std::uintptr_t ServerUnitRva = 0x48FE80;
constexpr std::uintptr_t CainIdentifyCallbackRva = 0x4AE280;
constexpr std::uintptr_t ResolveHoveredUnitRva = 0x2A7810;
constexpr std::uintptr_t ResolveHoveredUnitWitnessRva = 0x2A7820;
constexpr std::uintptr_t IsVirtualKeyDownRva = 0x120A100;

constexpr std::array<std::uint8_t, 32> SendTwentyOneBytePacketExpected{
    0x48, 0x83, 0xEC, 0x48, 0x48, 0x8B, 0x05, 0x9D,
    0xEA, 0x8D, 0x02, 0x48, 0x33, 0xC4, 0x48, 0x89,
    0x44, 0x24, 0x38, 0x8B, 0x44, 0x24, 0x70, 0x89,
    0x44, 0x24, 0x2D, 0x8B, 0x44, 0x24, 0x78, 0x88,
};
constexpr std::array<std::uint8_t, 32> InventoryClickHandlerExpected{
    0x40, 0x55, 0x56, 0x48, 0x8D, 0xAC, 0x24, 0x48,
    0xFD, 0xFF, 0xFF, 0x48, 0x81, 0xEC, 0xB8, 0x03,
    0x00, 0x00, 0x48, 0x8B, 0x05, 0x6F, 0x3D, 0x70,
    0x02, 0x48, 0x33, 0xC4, 0x48, 0x89, 0x85, 0x80,
};
constexpr std::array<std::uint8_t, 32> CainIdentifyCallbackExpected{
    0x40, 0x53, 0x55, 0x56, 0x57, 0x48, 0x81, 0xEC,
    0xB8, 0x00, 0x00, 0x00, 0x48, 0x8B, 0x05, 0x35,
    0xD0, 0x51, 0x02, 0x48, 0x33, 0xC4, 0x48, 0x89,
    0x84, 0x24, 0xA0, 0x00, 0x00, 0x00, 0x49, 0x63,
};
constexpr std::array<std::uint8_t, 32> IdentifyStoredItemExpected{
    0x48, 0x89, 0x5C, 0x24, 0x08, 0x48, 0x89, 0x74,
    0x24, 0x10, 0x57, 0x48, 0x83, 0xEC, 0x20, 0x48,
    0x8B, 0xFA, 0x49, 0x8B, 0xD8, 0xBA, 0x10, 0x00,
    0x00, 0x00, 0x48, 0x8B, 0xF1, 0x48, 0x8B, 0xCF,
};
constexpr std::array<std::uint8_t, 32> SynchronizeQuantityExpected{
    0x48, 0x89, 0x5C, 0x24, 0x08, 0x48, 0x89, 0x6C,
    0x24, 0x10, 0x56, 0x57, 0x41, 0x54, 0x41, 0x56,
    0x41, 0x57, 0x48, 0x83, 0xEC, 0x30, 0x49, 0x8B,
    0xF8, 0x48, 0x8B, 0xDA, 0x45, 0x33, 0xC0, 0x48,
};
constexpr std::array<std::uint8_t, 32> GetLocalDataContextExpected{
    0x8B, 0x05, 0x2E, 0x84, 0x99, 0x02, 0xC3, 0xCC,
    0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
    0x8B, 0x05, 0x76, 0x84, 0x99, 0x02, 0xC3, 0xCC,
    0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
};
constexpr std::array<std::uint8_t, 32> GetLocalPlayerExpected{
    0x48, 0x89, 0x5C, 0x24, 0x08, 0x57, 0x48, 0x83,
    0xEC, 0x20, 0x83, 0xF9, 0x08, 0x0F, 0x83, 0x85,
    0x00, 0x00, 0x00, 0x8B, 0xD9, 0x48, 0x89, 0x5C,
    0x24, 0x38, 0x48, 0x83, 0xFB, 0x08, 0x72, 0x19,
};
constexpr std::array<std::uint8_t, 16> GetMouseStateExpected{
    0x8B, 0x05, 0xF2, 0x94, 0x93, 0x02, 0xC3, 0xCC,
    0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
};
constexpr std::array<std::uint8_t, 21> IsVirtualKeyDownExpected{
    0x48, 0x83, 0xEC, 0x28, 0xFF, 0x15, 0x86, 0x6E,
    0xAA, 0x00, 0xC1, 0xE8, 0x0F, 0x83, 0xE0, 0x01,
    0x48, 0x83, 0xC4, 0x28, 0xC3,
};
constexpr std::array<std::uint8_t, 16> GetUnitStatExpected{
    0x48, 0x89, 0x5C, 0x24, 0x10, 0x48, 0x89, 0x6C,
    0x24, 0x18, 0x48, 0x89, 0x74, 0x24, 0x20, 0x57,
};
constexpr std::array<std::uint8_t, 32> GetUnitIdExpected{
    0x48, 0x83, 0xEC, 0x28, 0x48, 0x85, 0xC9, 0x75,
    0x1D, 0x88, 0x4C, 0x24, 0x30, 0x48, 0x8D, 0x4C,
    0x24, 0x30, 0xE8, 0x39, 0xCA, 0xFF, 0xFF, 0x84,
    0xC0, 0x74, 0x01, 0xCC, 0xB8, 0xFF, 0xFF, 0xFF,
};
constexpr std::array<std::uint8_t, 32> GetUnitInventoryExpected{
    0x48, 0x89, 0x5C, 0x24, 0x18, 0x56, 0x48, 0x83,
    0xEC, 0x20, 0x48, 0x8B, 0xF1, 0x48, 0x85, 0xC9,
    0x75, 0x13, 0x88, 0x4C, 0x24, 0x30, 0x48, 0x8D,
    0x4C, 0x24, 0x30, 0xE8, 0x70, 0xCC, 0xFF, 0xFF,
};
constexpr std::array<std::uint8_t, 32> GetUnitTypeExpected{
    0x48, 0x83, 0xEC, 0x28, 0x48, 0x85, 0xC9, 0x75,
    0x1D, 0x88, 0x4C, 0x24, 0x30, 0x48, 0x8D, 0x4C,
    0x24, 0x30, 0xE8, 0x39, 0x9E, 0xFF, 0xFF, 0x84,
    0xC0, 0x74, 0x01, 0xCC, 0xB8, 0x06, 0x00, 0x00,
};
constexpr std::array<std::uint8_t, 15> GetInventoryPageExpected{
    0x40, 0x53, 0x48, 0x83, 0xEC, 0x20, 0x48, 0x8B,
    0xD9, 0x48, 0x85, 0xC9, 0x74, 0x0A, 0xE8,
};
constexpr std::array<std::uint8_t, 16> CheckItemFlagExpected{
    0x48, 0x89, 0x5C, 0x24, 0x10, 0x57, 0x48, 0x83,
    0xEC, 0x20, 0x8B, 0xFA, 0x48, 0x8B, 0xD9, 0x48,
};
constexpr std::array<std::uint8_t, 16> ResolveHoveredUnitWitness{
    0x10, 0x83, 0xB9, 0xC8, 0x05, 0x00, 0x00, 0x06,
    0x75, 0x07, 0x33, 0xC0, 0x48, 0x83, 0xC4, 0x28,
};
constexpr std::array<std::uint8_t, 16> GetCursorItemExpected{
    0x40, 0x53, 0x48, 0x83, 0xEC, 0x20, 0x48, 0x8B,
    0xD9, 0x48, 0x85, 0xC9, 0x75, 0x1B, 0x88, 0x4C,
};
constexpr std::array<std::uint8_t, 32> GetItemCodeExpected{
    0x48, 0x89, 0x5C, 0x24, 0x10, 0x57, 0x48, 0x83,
    0xEC, 0x20, 0x48, 0x8B, 0xF9, 0x48, 0x85, 0xC9,
    0x75, 0x13, 0x88, 0x4C, 0x24, 0x30, 0x48, 0x8D,
    0x4C, 0x24, 0x30, 0xE8, 0x80, 0x83, 0xFF, 0xFF,
};
constexpr std::array<std::uint8_t, 32> GetFirstItemExpected{
    0x40, 0x53, 0x48, 0x83, 0xEC, 0x20, 0x48, 0x8B,
    0xD9, 0x48, 0x85, 0xC9, 0x74, 0x2E, 0x81, 0x39,
    0x04, 0x03, 0x02, 0x01, 0x74, 0x1C, 0x48, 0x8D,
    0x4C, 0x24, 0x30, 0xC6, 0x44, 0x24, 0x30, 0x00,
};
constexpr std::array<std::uint8_t, 32> GetNextItemExpected{
    0x40, 0x53, 0x48, 0x83, 0xEC, 0x20, 0x48, 0x8B,
    0xD9, 0x48, 0x85, 0xC9, 0x75, 0x10, 0x88, 0x4C,
    0x24, 0x30, 0x48, 0x8D, 0x4C, 0x24, 0x30, 0xE8,
    0x84, 0x98, 0xFF, 0xFF, 0xEB, 0x67, 0xE8, 0x0D,
};
constexpr std::array<std::uint8_t, 32> GetParentInventoryExpected{
    0x40, 0x53, 0x48, 0x83, 0xEC, 0x20, 0x48, 0x8B,
    0xD9, 0x48, 0x85, 0xC9, 0x74, 0x0A, 0xE8, 0x6D,
    0x0D, 0xFC, 0xFF, 0x83, 0xF8, 0x04, 0x74, 0x19,
    0x48, 0x8D, 0x4C, 0x24, 0x30, 0xC6, 0x44, 0x24,
};
constexpr std::array<std::uint8_t, 32> ServerUnitExpected{
    0x48, 0x89, 0x5C, 0x24, 0x08, 0x48, 0x89, 0x74,
    0x24, 0x18, 0x57, 0x48, 0x83, 0xEC, 0x20, 0x41,
    0x8B, 0xD8, 0x8B, 0xF2, 0x48, 0x8B, 0xF9, 0x48,
    0x85, 0xC9, 0x75, 0x13, 0x88, 0x4C, 0x24, 0x38,
};

struct Config {
    bool enabled{true};
    bool freeIdentification{false};
};

using SendTwentyOneBytePacketFn = void(__fastcall*)(
    std::uint8_t, std::uint32_t, std::uint32_t, std::uint32_t,
    std::uint32_t, std::uint32_t) noexcept;
using InventoryClickHandlerFn = void(__fastcall*)(void*, void*) noexcept;
using CainIdentifyCallbackFn = std::int32_t(__fastcall*)(
    void*, void*, const std::uint8_t*, std::int32_t) noexcept;
using GetLocalDataContextFn = std::int32_t(__fastcall*)() noexcept;
using GetLocalPlayerFn = void*(__fastcall*)(std::int32_t) noexcept;
using GetMouseStateFn = std::int32_t(__fastcall*)() noexcept;
using IsVirtualKeyDownFn = std::int32_t(__fastcall*)(std::int32_t) noexcept;
using ResolveHoveredUnitFn = void*(__fastcall*)(void*) noexcept;
using ResolveClickedItemFn = void*(__fastcall*)(void*, void*) noexcept;
using GetUnitInventoryFn = void*(__fastcall*)(void*) noexcept;
using GetCursorItemFn = void*(__fastcall*)(void*) noexcept;
using GetParentInventoryFn = void*(__fastcall*)(void*) noexcept;
using GetUnitTypeFn = std::int32_t(__fastcall*)(void*) noexcept;
using GetInventoryPageFn = std::uint8_t(__fastcall*)(void*) noexcept;
using GetItemCodeFn = std::uint32_t(__fastcall*)(void*) noexcept;
using GetUnitIdFn = std::int32_t(__fastcall*)(void*) noexcept;
using GetServerUnitFn = void*(__fastcall*)(void*, std::int32_t, std::int32_t) noexcept;
using GetFirstItemFn = void*(__fastcall*)(void*) noexcept;
using GetNextItemFn = void*(__fastcall*)(void*) noexcept;
using CheckItemFlagFn = std::int32_t(__fastcall*)(
    void*, std::uint32_t) noexcept;
using GetUnitStatFn = std::int32_t(__fastcall*)(
    void*, std::int32_t, std::int32_t) noexcept;
using IdentifyStoredItemFn = void(__fastcall*)(void*, void*, void*) noexcept;
using SynchronizeQuantityFn = void(__fastcall*)(
    void*, void*, void*, std::int32_t) noexcept;

const D2RL::PluginContext* Context{};
std::uint8_t* Base{};
Config Settings{};
std::string LoadedConfigPath{"built-in defaults"};

SendTwentyOneBytePacketFn SendTwentyOneBytePacket{};
InventoryClickHandlerFn OriginalInventoryClickHandler{};
CainIdentifyCallbackFn OriginalCainIdentifyCallback{};
GetLocalDataContextFn GetLocalDataContext{};
GetLocalPlayerFn GetLocalPlayer{};
GetMouseStateFn GetMouseState{};
IsVirtualKeyDownFn IsVirtualKeyDown{};
ResolveHoveredUnitFn ResolveHoveredUnit{};
GetUnitInventoryFn GetUnitInventory{};
GetCursorItemFn GetCursorItem{};
GetParentInventoryFn GetParentInventory{};
GetUnitTypeFn GetUnitType{};
GetInventoryPageFn GetInventoryPage{};
GetItemCodeFn GetItemCode{};
GetUnitIdFn GetUnitId{};
GetServerUnitFn GetServerUnit{};
GetFirstItemFn GetFirstItem{};
GetNextItemFn GetNextItem{};
CheckItemFlagFn CheckItemFlag{};
GetUnitStatFn GetUnitStat{};
IdentifyStoredItemFn IdentifyStoredItem{};
SynchronizeQuantityFn SynchronizeQuantity{};

std::atomic<std::uint64_t> RequestsSent{};
std::atomic<std::uint64_t> RequestsAccepted{};
std::atomic<std::uint64_t> RequestsRejected{};
std::atomic<std::uint64_t> ItemsIdentified{};
std::atomic<std::uint64_t> ChargesConsumed{};

constexpr D2RL::PluginInfo Info{
    .infoSize = D2RL::PluginInfoSize,
    .apiVersion = D2RL_PLUGIN_API_VERSION,
    .id = "mass-id",
    .name = "MassID",
    .version = "0.1.0",
    .author = "RuffnecKk",
    .description = "Identifies inventory and Cube items from an Identify Tome.",
    .flags = D2RL::PluginFlags::NativeHooks,
};

template<class T>
T At(std::uintptr_t rva) noexcept {
    return reinterpret_cast<T>(Base + rva);
}

template<std::size_t Size>
bool Matches(
        std::uintptr_t rva,
        const std::array<std::uint8_t, Size>& expected) noexcept {
    return Base && std::memcmp(Base + rva, expected.data(), Size) == 0;
}

bool LoadConfig() noexcept {
    Settings = {};
    LoadedConfigPath = "built-in defaults";

    std::vector<std::filesystem::path> candidates;
    if (Context && Context->modDirectory && Context->modDirectory[0] != L'\0') {
        candidates.emplace_back(
            std::filesystem::path(Context->modDirectory) / ConfigFileName);
    }
    candidates.emplace_back(ConfigFileName);

    for (const auto& path : candidates) {
        std::error_code error;
        if (!std::filesystem::is_regular_file(path, error)) continue;

        try {
            std::ifstream input(path);
            if (!input.is_open()) {
                throw std::runtime_error("configuration file cannot be opened");
            }
            const auto config = nlohmann::json::parse(input, nullptr, true, true);
            if (!config.is_object()) {
                throw std::invalid_argument("configuration root must be an object");
            }
            for (const auto& [key, value] : config.items()) {
                (void)value;
                if (key != "enabled" && key != "freeIdentification") {
                    throw std::invalid_argument("unknown setting: " + key);
                }
            }
            if (config.contains("enabled") && !config.at("enabled").is_boolean()) {
                throw std::invalid_argument("enabled must be a boolean");
            }
            if (config.contains("freeIdentification")
                    && !config.at("freeIdentification").is_boolean()) {
                throw std::invalid_argument(
                    "freeIdentification must be a boolean");
            }
            Settings.enabled = config.value("enabled", true);
            Settings.freeIdentification = config.value(
                "freeIdentification", false);
            LoadedConfigPath = path.string();
            return true;
        } catch (const std::exception& exception) {
            if (Context) {
                const auto message = std::string("MassID: invalid ")
                    + path.string() + " (" + exception.what() + ").";
                Context->LogError(message.c_str());
            }
            return false;
        }
    }
    return true;
}

bool ValidateRuntime() noexcept {
    bool valid = Base != nullptr;
    const auto check = [&valid](
            std::uintptr_t rva,
            const auto& expected,
            const char* label) noexcept {
        if (Matches(rva, expected)) return;
        valid = false;
        if (Context) {
            char message[192]{};
            std::snprintf(
                message,
                sizeof(message),
                "MassID: signature mismatch for %s at RVA 0x%llX.",
                label,
                static_cast<unsigned long long>(rva));
            Context->LogError(message);
        }
    };

    check(SendTwentyOneBytePacketRva, SendTwentyOneBytePacketExpected,
        "CLIENT_SendTwentyOneByteCommandPacket");
    check(InventoryClickHandlerRva, InventoryClickHandlerExpected,
        "UI_InventorySlotWidget_HandleClick");
    check(CainIdentifyCallbackRva, CainIdentifyCallbackExpected,
        "D2GAME_PACKETCALLBACK_Rcv0x34_IdentifyItemsWithNpc");
    check(IdentifyStoredItemRva, IdentifyStoredItemExpected,
        "D2GAME_ITEMS_IdentifyStoredItem");
    check(SynchronizeQuantityRva, SynchronizeQuantityExpected,
        "SynchronizeItemAndBoundSkillQuantity");
    check(GetLocalDataContextRva, GetLocalDataContextExpected,
        "CLIENT_GetLocalDataContext");
    check(GetLocalPlayerRva, GetLocalPlayerExpected,
        "CLIENT_GetLocalPlayer");
    check(GetMouseStateRva, GetMouseStateExpected, "INPUT_GetMouseState");
    check(IsVirtualKeyDownRva, IsVirtualKeyDownExpected,
        "INPUT_IsVirtualKeyDownAsync");
    check(GetUnitStatRva, GetUnitStatExpected, "STATLIST_GetUnitStat");
    check(GetUnitIdRva, GetUnitIdExpected, "UNITS_GetUnitId");
    check(GetUnitInventoryRva, GetUnitInventoryExpected, "UNITS_GetInventory");
    check(GetUnitTypeRva, GetUnitTypeExpected, "UNITS_GetUnitType");
    check(GetInventoryPageRva, GetInventoryPageExpected, "ITEMS_GetInvPage");
    check(CheckItemFlagRva, CheckItemFlagExpected, "ITEMS_CheckItemFlag");
    check(ResolveHoveredUnitWitnessRva, ResolveHoveredUnitWitness,
        "UI_TOOLTIP_ResolveHoveredUnit witness");
    check(GetItemCodeRva, GetItemCodeExpected, "ITEMS_GetItemCode");
    check(GetCursorItemRva, GetCursorItemExpected, "INVENTORY_GetCursorItem");
    check(GetFirstItemRva, GetFirstItemExpected, "INVENTORY_GetFirstItem");
    check(GetNextItemRva, GetNextItemExpected, "INVENTORY_GetNextItem");
    check(GetParentInventoryRva, GetParentInventoryExpected,
        "INVENTORY_GetParentInventory");
    check(ServerUnitRva, ServerUnitExpected, "SUNIT_GetServerUnit");
    return valid;
}

void* ResolveClickedItem(void* widget, void* eventState) noexcept {
    if (!widget || !eventState) return nullptr;
    auto* vtable = *reinterpret_cast<void***>(widget);
    if (!vtable) return nullptr;
    const auto resolver = reinterpret_cast<ResolveClickedItemFn>(
        vtable[0xC8 / sizeof(void*)]);
    return resolver ? resolver(widget, eventState) : nullptr;
}

void __fastcall HookInventoryClickHandler(
        void* widget, void* eventState) noexcept {
    if (Settings.enabled && widget && eventState
            && IsVirtualKeyDown(VK_SHIFT)
            && IsRightClickState(GetMouseState())) {
        void* localPlayer = GetLocalPlayer(GetLocalDataContext());
        void* owner = ResolveHoveredUnit(widget);
        void* inventory = localPlayer
            ? GetUnitInventory(localPlayer)
            : nullptr;
        void* item = inventory && GetCursorItem(inventory) == nullptr
            ? ResolveClickedItem(widget, eventState)
            : nullptr;

        const bool owned = item
            && GetParentInventory(item) == inventory;
        if (item && ShouldCaptureGesture(
                Settings.enabled,
                true,
                true,
                inventory && GetCursorItem(inventory) == nullptr,
                owner == localPlayer,
                owned,
                GetUnitType(item),
                GetItemCode(item),
                GetInventoryPage(item))) {
            const auto tomeGuid = static_cast<std::uint32_t>(
                GetUnitId(item));
            SendTwentyOneBytePacket(
                CainIdentifyOpcode,
                tomeGuid,
                RequestMarker,
                RequestGuard,
                0,
                0);
            RequestsSent.fetch_add(1, std::memory_order_relaxed);
            return;
        }
    }
    OriginalInventoryClickHandler(widget, eventState);
}

bool IsOwnedIdentifyTome(
        void* player, void* inventory, void* tome) noexcept {
    return player
        && inventory
        && tome
        && GetUnitType(tome) == 4
        && GetParentInventory(tome) == inventory
        && GetItemCode(tome) == IdentifyTomeCode
        && IsSupportedInventoryPage(GetInventoryPage(tome));
}

std::int32_t IdentifyPage(
        void* game,
        void* player,
        void* inventory,
        std::uint8_t page,
        std::int32_t budget) noexcept {
    std::int32_t identified{};
    for (void* item = GetFirstItem(inventory);
            item && identified < budget;) {
        void* next = GetNextItem(item);
        if (GetUnitType(item) == 4
                && GetParentInventory(item) == inventory
                && GetInventoryPage(item) == page
                && CheckItemFlag(item, IdentifiedItemFlag) == 0) {
            IdentifyStoredItem(game, item, player);
            if (CheckItemFlag(item, IdentifiedItemFlag) != 0) {
                ++identified;
            }
        }
        item = next;
    }
    return identified;
}

std::int32_t __fastcall HookCainIdentifyCallback(
        void* game,
        void* player,
        const std::uint8_t* packet,
        std::int32_t packetSize) noexcept {
    if (!IsPrivateRequest(packet, packetSize)) {
        return OriginalCainIdentifyCallback(
            game, player, packet, packetSize);
    }

    void* inventory = player
        ? GetUnitInventory(player)
        : nullptr;
    const auto tomeGuid = static_cast<std::int32_t>(ReadU32(packet, 1));
    void* tome = game ? GetServerUnit(game, 4, tomeGuid) : nullptr;
    if (!IsOwnedIdentifyTome(player, inventory, tome)) {
        RequestsRejected.fetch_add(1, std::memory_order_relaxed);
        return 0;
    }

    const auto quantity = GetUnitStat(
        tome, static_cast<std::int32_t>(QuantityStat), 0);
    const auto budget = IdentificationBudget(
        Settings.freeIdentification, quantity);

    std::int32_t identified{};
    if (budget > 0) {
        identified += IdentifyPage(
            game, player, inventory, InventoryPage, budget);
        if (identified < budget) {
            identified += IdentifyPage(
                game, player, inventory, CubePage, budget - identified);
        }
    }

    if (!Settings.freeIdentification && identified > 0) {
        SynchronizeQuantity(game, player, tome, -identified);
        ChargesConsumed.fetch_add(
            static_cast<std::uint64_t>(identified),
            std::memory_order_relaxed);
    }
    RequestsAccepted.fetch_add(1, std::memory_order_relaxed);
    ItemsIdentified.fetch_add(
        static_cast<std::uint64_t>(identified),
        std::memory_order_relaxed);
    return 0;
}

bool InstallHooks() noexcept {
    if (!Context->InstallInlineHook(
            InventoryClickHandlerRva,
            InventoryClickHandlerExpected.data(),
            static_cast<std::uint32_t>(InventoryClickHandlerExpected.size()),
            HookInventoryClickHandler,
            &OriginalInventoryClickHandler)) {
        Context->LogError("MassID: inventory click hook refused.");
        return false;
    }
    if (!Context->InstallInlineHook(
            CainIdentifyCallbackRva,
            CainIdentifyCallbackExpected.data(),
            static_cast<std::uint32_t>(CainIdentifyCallbackExpected.size()),
            HookCainIdentifyCallback,
            &OriginalCainIdentifyCallback)) {
        Context->LogError("MassID: Cain identify callback hook refused.");
        return false;
    }
    return true;
}

auto Status(
        D2R::Game::Client*,
        const D2RL::ConsoleCommandContext* command,
        void*) noexcept -> D2RL::ConsoleCommandResult {
    if (!command || !command->plugin) {
        return D2RL::ConsoleCommandResult::Failed;
    }
    char message[480]{};
    std::snprintf(
        message,
        sizeof(message),
        "MassID 0.1.0: enabled=%s; freeIdentification=%s; sent=%llu; accepted=%llu; rejected=%llu; identified=%llu; consumed=%llu; JSON=%s; future owner=items.massIdentify.",
        Settings.enabled ? "true" : "false",
        Settings.freeIdentification ? "true" : "false",
        static_cast<unsigned long long>(RequestsSent.load()),
        static_cast<unsigned long long>(RequestsAccepted.load()),
        static_cast<unsigned long long>(RequestsRejected.load()),
        static_cast<unsigned long long>(ItemsIdentified.load()),
        static_cast<unsigned long long>(ChargesConsumed.load()),
        LoadedConfigPath.c_str());
    command->plugin->WriteConsoleMessage(message);
    return D2RL::ConsoleCommandResult::Handled;
}
} // namespace

D2RL_PLUGIN_EXPORT auto D2RLoaderGetPluginInfo() noexcept
        -> const D2RL::PluginInfo* {
    return &Info;
}

D2RL_PLUGIN_EXPORT auto D2RLoaderLoadPlugin(
        const D2RL::PluginContext* context) noexcept -> bool {
    if (!context) return false;
    Context = context;
    Base = reinterpret_cast<std::uint8_t*>(context->exeBase);

    if (!LoadConfig()) {
        context->LogError("MassID: configuration is invalid.");
        return false;
    }
    if (context->modDataVersionBuild != 0
            && context->modDataVersionBuild != SupportedBuild) {
        context->LogError("MassID: only D2R build 92777 is supported.");
        return false;
    }
    if (!ValidateRuntime()) {
        context->LogError(
            "MassID: 92777 runtime signature mismatch; plugin refused.");
        return false;
    }

    SendTwentyOneBytePacket = At<SendTwentyOneBytePacketFn>(
        SendTwentyOneBytePacketRva);
    GetLocalDataContext = At<GetLocalDataContextFn>(GetLocalDataContextRva);
    GetLocalPlayer = At<GetLocalPlayerFn>(GetLocalPlayerRva);
    GetMouseState = At<GetMouseStateFn>(GetMouseStateRva);
    IsVirtualKeyDown = At<IsVirtualKeyDownFn>(IsVirtualKeyDownRva);
    ResolveHoveredUnit = At<ResolveHoveredUnitFn>(ResolveHoveredUnitRva);
    GetUnitInventory = At<GetUnitInventoryFn>(GetUnitInventoryRva);
    GetCursorItem = At<GetCursorItemFn>(GetCursorItemRva);
    GetParentInventory = At<GetParentInventoryFn>(GetParentInventoryRva);
    GetUnitType = At<GetUnitTypeFn>(GetUnitTypeRva);
    GetInventoryPage = At<GetInventoryPageFn>(GetInventoryPageRva);
    GetItemCode = At<GetItemCodeFn>(GetItemCodeRva);
    GetUnitId = At<GetUnitIdFn>(GetUnitIdRva);
    GetServerUnit = At<GetServerUnitFn>(ServerUnitRva);
    GetFirstItem = At<GetFirstItemFn>(GetFirstItemRva);
    GetNextItem = At<GetNextItemFn>(GetNextItemRva);
    CheckItemFlag = At<CheckItemFlagFn>(CheckItemFlagRva);
    GetUnitStat = At<GetUnitStatFn>(GetUnitStatRva);
    IdentifyStoredItem = At<IdentifyStoredItemFn>(IdentifyStoredItemRva);
    SynchronizeQuantity = At<SynchronizeQuantityFn>(SynchronizeQuantityRva);

    if (Settings.enabled && !InstallHooks()) return false;

    if (!context->RegisterConsoleCommand(
            "mass-id", Status, "Show MassID status and counters.")) {
        context->LogWarn("MassID: status command could not be registered.");
    }

    char message[480]{};
    std::snprintf(
        message,
        sizeof(message),
        "MassID 0.1.0 %s for D2R 3.2.92777; Shift-right-click identification %s; freeIdentification=%s (JSON: %s; future owner: items.massIdentify).",
        Settings.enabled ? "active" : "disabled",
        Settings.enabled ? "installed" : "left unchanged",
        Settings.freeIdentification ? "true" : "false",
        LoadedConfigPath.c_str());
    context->LogInfo(message);
    return true;
}

D2RL_PLUGIN_EXPORT void D2RLoaderUnloadPlugin() noexcept {
    Context = nullptr;
    Base = nullptr;
}
