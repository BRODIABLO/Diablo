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
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace ruffneckk::mass_id;

constexpr std::uint32_t SupportedBuild = 92777;
constexpr wchar_t ConfigFileName[] = L"MassID.json";

constexpr std::uintptr_t QueueOutgoingPacketRva = 0x0EE2A0;
constexpr std::uintptr_t GetLocalDataContextRva = 0x08B2D0;
constexpr std::uintptr_t GetLocalPlayerRva = 0x09A480;
constexpr std::uintptr_t TargetingPacketWorkerRva = 0x1C7A30;
constexpr std::uintptr_t IsVirtualKeyDownRva = 0x120A100;
constexpr std::uintptr_t LegacyDropAppenderCallRva = 0x2279BD;
constexpr std::uintptr_t ModernDropAppenderCallRva = 0x2C552D;
constexpr std::array<std::uintptr_t, 2> LegacyMoveAppenderCallRvas{
    0x2278DC,
    0x227936,
};
constexpr std::array<std::uintptr_t, 3> ModernMoveAppenderCallRvas{
    0x2C5241,
    0x2C528D,
    0x2C53AB,
};
constexpr std::uintptr_t AlternateMoveAppenderCallRva = 0x2CA2E0;
constexpr std::uintptr_t GetLocalizedStringByKeyRva = 0x5F4B90;
constexpr std::uintptr_t GetUnitStatRva = 0x2F5020;
constexpr std::uintptr_t CheckStateRva = 0x3351B0;
constexpr std::uintptr_t GetUnitIdRva = 0x34A330;
constexpr std::uintptr_t GetUnitInventoryRva = 0x34A360;
constexpr std::uintptr_t GetUnitTypeRva = 0x34B9D0;
constexpr std::uintptr_t GetInventoryPageRva = 0x36CFE0;
constexpr std::uintptr_t CheckItemFlagRva = 0x36E2D0;
constexpr std::uintptr_t SetItemFlagRva = 0x36D8F0;
constexpr std::uintptr_t GetItemSuffixIdRva = 0x36EDD0;
constexpr std::uintptr_t GetItemCodeRva = 0x36EF50;
constexpr std::uintptr_t GetCursorItemRva = 0x388A70;
constexpr std::uintptr_t GetFirstItemRva = 0x388C10;
constexpr std::uintptr_t GetNextItemRva = 0x38ABA0;
constexpr std::uintptr_t GetParentInventoryRva = 0x38AC50;
constexpr std::uintptr_t GetInventoryOwnerIdRva = 0x388BA0;
constexpr std::uintptr_t GetFirstCorpseRva = 0x388E00;
constexpr std::uintptr_t GetNextCorpseRva = 0x38CD70;
constexpr std::uintptr_t GetCorpseUnitIdRva = 0x2EF880;
constexpr std::uintptr_t IdentifyItemRva = 0x46E8C0;
constexpr std::uintptr_t SynchronizeQuantityRva = 0x46F090;
constexpr std::uintptr_t ServerUnitRva = 0x48FE80;
constexpr std::uintptr_t CainIdentifyCallbackRva = 0x4C6C90;
constexpr std::int32_t SharedStashProxyState = 0xBA;

constexpr std::array<std::uint8_t, 32> QueueOutgoingPacketExpected{
    0x48, 0x89, 0x5C, 0x24, 0x18, 0x55, 0x56, 0x57,
    0x48, 0x81, 0xEC, 0x30, 0x02, 0x00, 0x00, 0x48,
    0x8B, 0x05, 0x12, 0xD0, 0x8D, 0x02, 0x48, 0x33,
    0xC4, 0x48, 0x89, 0x84, 0x24, 0x20, 0x02, 0x00,
};
constexpr std::array<std::uint8_t, 35> TargetingPacketWorkerExpected{
    0x40, 0x53, 0x48, 0x81, 0xEC, 0xB0, 0x00, 0x00,
    0x00, 0x48, 0x8B, 0x05, 0x88, 0x38, 0x80, 0x02,
    0x48, 0x33, 0xC4, 0x48, 0x89, 0x84, 0x24, 0x90,
    0x00, 0x00, 0x00, 0x48, 0x8B, 0xD9, 0xE8, 0x8D,
    0x77, 0xF8, 0xFF,
};
constexpr std::array<std::uint8_t, 21> IsVirtualKeyDownExpected{
    0x48, 0x83, 0xEC, 0x28, 0xFF, 0x15, 0x86, 0x6E,
    0xAA, 0x00, 0xC1, 0xE8, 0x0F, 0x83, 0xE0, 0x01,
    0x48, 0x83, 0xC4, 0x28, 0xC3,
};
constexpr std::array<std::uint8_t, 5> LegacyDropAppenderCallExpected{
    0xE8, 0xCE, 0xD1, 0x3C, 0x00,
};
constexpr std::array<std::uint8_t, 5> ModernDropAppenderCallExpected{
    0xE8, 0x5E, 0xF6, 0x32, 0x00,
};
constexpr std::array<std::array<std::uint8_t, 5>, 2>
        LegacyMoveAppenderCallsExpected{{
    {0xE8, 0xAF, 0xD2, 0x3C, 0x00},
    {0xE8, 0x55, 0xD2, 0x3C, 0x00},
}};
constexpr std::array<std::array<std::uint8_t, 5>, 3>
        ModernMoveAppenderCallsExpected{{
    {0xE8, 0x4A, 0xF9, 0x32, 0x00},
    {0xE8, 0xFE, 0xF8, 0x32, 0x00},
    {0xE8, 0xE0, 0xF7, 0x32, 0x00},
}};
constexpr std::array<std::uint8_t, 5> AlternateMoveAppenderCallExpected{
    0xE8, 0xAB, 0xA8, 0x32, 0x00,
};
constexpr std::array<std::uint8_t, 29> GetLocalizedStringByKeyExpected{
    0x4C, 0x8B, 0xDC, 0x55, 0x53, 0x57, 0x49, 0x8D,
    0x6B, 0xA1, 0x48, 0x81, 0xEC, 0xB0, 0x00, 0x00,
    0x00, 0x48, 0x8B, 0x05, 0x20, 0x67, 0x3D, 0x02,
    0x48, 0x33, 0xC4, 0x48, 0x89,
};
constexpr std::array<std::uint8_t, 32> CainIdentifyCallbackExpected{
    0x40, 0x55, 0x53, 0x56, 0x57, 0x48, 0x8D, 0xAC,
    0x24, 0x18, 0xB0, 0xFF, 0xFF, 0xB8, 0xE8, 0x50,
    0x00, 0x00, 0xE8, 0x39, 0xA4, 0xE0, 0x00, 0x48,
    0x2B, 0xE0, 0x48, 0x8B, 0x05, 0x17, 0x46, 0x50,
};
constexpr std::array<std::uint8_t, 32> IdentifyItemExpected{
    0x48, 0x89, 0x6C, 0x24, 0x20, 0x41, 0x54, 0x41,
    0x56, 0x41, 0x57, 0x48, 0x83, 0xEC, 0x50, 0x49,
    0x8B, 0xE8, 0x45, 0x0F, 0xB6, 0xE1, 0x4C, 0x8B,
    0xF2, 0x4C, 0x8D, 0x0D, 0x30, 0xD7, 0x8A, 0x01,
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
constexpr std::array<std::uint8_t, 16> GetUnitStatExpected{
    0x48, 0x89, 0x5C, 0x24, 0x10, 0x48, 0x89, 0x6C,
    0x24, 0x18, 0x48, 0x89, 0x74, 0x24, 0x20, 0x57,
};
constexpr std::array<std::uint8_t, 32> CheckStateExpected{
    0x48, 0x89, 0x5C, 0x24, 0x08, 0x48, 0x89, 0x74,
    0x24, 0x10, 0x57, 0x48, 0x83, 0xEC, 0x20, 0x8B,
    0xDA, 0x48, 0x8B, 0xF1, 0xE8, 0x07, 0x68, 0x01,
    0x00, 0x85, 0xC0, 0x74, 0x0E, 0x83, 0xE8, 0x01,
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
constexpr std::array<std::uint8_t, 16> SetItemFlagExpected{
    0x48, 0x89, 0x5C, 0x24, 0x10, 0x48, 0x89, 0x74,
    0x24, 0x18, 0x57, 0x48, 0x83, 0xEC, 0x20, 0x41,
};
constexpr std::array<std::uint8_t, 32> GetItemSuffixIdExpected{
    0x48, 0x89, 0x5C, 0x24, 0x08, 0x57, 0x48, 0x83,
    0xEC, 0x20, 0xBA, 0x12, 0x00, 0x00, 0x00, 0x48,
    0x8B, 0xD9, 0xE8, 0xA9, 0x4A, 0x00, 0x00, 0x85,
    0xC0, 0x0F, 0x84, 0x9C, 0x00, 0x00, 0x00, 0x48,
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
constexpr std::array<std::uint8_t, 32> GetInventoryOwnerIdExpected{
    0x40, 0x53, 0x48, 0x83, 0xEC, 0x20, 0x48, 0x8B,
    0xD9, 0x48, 0x85, 0xC9, 0x75, 0x1E, 0x88, 0x4C,
    0x24, 0x30, 0x48, 0x8D, 0x4C, 0x24, 0x30, 0xE8,
    0xB4, 0xC2, 0xFF, 0xFF, 0x84, 0xC0, 0x74, 0x30,
};
constexpr std::array<std::uint8_t, 16> GetFirstCorpseExpected{
    0x48, 0x8B, 0x41, 0x68, 0xC3, 0xCC, 0xCC, 0xCC,
    0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
};
constexpr std::array<std::uint8_t, 16> GetNextCorpseExpected{
    0x48, 0x8B, 0x41, 0x10, 0xC3, 0xCC, 0xCC, 0xCC,
    0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
};
constexpr std::array<std::uint8_t, 16> GetCorpseUnitIdExpected{
    0x8B, 0x01, 0xC3, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
    0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
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

struct GameStringView {
    const char* data{};
    std::size_t size{};
};

struct TooltipLocale {
    std::string_view defenseFingerprint;
    std::string_view massIdText;
};

// D2R's thirteen shipped locales, selected from the same native defense
// fingerprint used by AdvancedItemTooltips. Only MassID's invented label lives
// in this table; the game remains the authority for the active language.
constexpr std::array TooltipLocales{
    TooltipLocale{"Defense: %d", "Shift + Right Click to Mass ID"},
    TooltipLocale{"防禦：%d", "Shift + 右鍵點擊以批量鑑定"},
    TooltipLocale{"Verteidigung: %d", "Umschalt + Rechtsklick für Massenidentifizierung"},
    TooltipLocale{"Defensa: %d", "Mayús + clic derecho para identificar todo"},
    TooltipLocale{"Défense : %d", "Maj + clic droit pour tout identifier"},
    TooltipLocale{"Difesa: %d", "Maiusc + clic destro per identificare tutto"},
    TooltipLocale{"방어력: %d", "Shift + 오른쪽 클릭으로 모두 감정"},
    TooltipLocale{"Obrona: %d", "Shift + prawy przycisk, aby zidentyfikować wszystko"},
    TooltipLocale{"Defensa: %d", "Mayús + clic derecho para identificar todo"},
    TooltipLocale{"防御力: %d", "Shift + 右クリックですべて鑑定"},
    TooltipLocale{"Defesa: %d", "Shift + clique direito para identificar tudo"},
    TooltipLocale{"Защита: %d", "Shift + ПКМ, чтобы опознать всё"},
    TooltipLocale{"防御: %d", "Shift + 右键点击以批量辨识"},
};

using QueueOutgoingPacketFn = void(__fastcall*)(
    const std::uint8_t*, std::int32_t) noexcept;
using TargetingPacketWorkerFn = void(__fastcall*)(
    const std::uint8_t*) noexcept;
using CainIdentifyCallbackFn = std::int32_t(__fastcall*)(
    void*, void*, const std::uint8_t*, std::int32_t) noexcept;
using GetLocalDataContextFn = std::int32_t(__fastcall*)() noexcept;
using GetLocalPlayerFn = void*(__fastcall*)(std::int32_t) noexcept;
using IsVirtualKeyDownFn = std::int32_t(__fastcall*)(std::int32_t) noexcept;
using GetLocalizedStringByKeyFn = const char*(__fastcall*)(
    const GameStringView*) noexcept;
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
using GetInventoryOwnerIdFn = std::int32_t(__fastcall*)(void*) noexcept;
using GetFirstCorpseFn = void*(__fastcall*)(void*) noexcept;
using GetNextCorpseFn = void*(__fastcall*)(void*) noexcept;
using GetCorpseUnitIdFn = std::int32_t(__fastcall*)(void*) noexcept;
using CheckItemFlagFn = std::int32_t(__fastcall*)(
    void*, std::uint32_t) noexcept;
using SetItemFlagFn = void(__fastcall*)(
    void*, std::uint32_t, std::int32_t) noexcept;
using GetItemSuffixIdFn = std::uint16_t(__fastcall*)(void*) noexcept;
using GetUnitStatFn = std::int32_t(__fastcall*)(
    void*, std::int32_t, std::int32_t) noexcept;
using CheckStateFn = std::int32_t(__fastcall*)(
    void*, std::int32_t) noexcept;
using IdentifyItemFn = void(__fastcall*)(
    void*, void*, void*, std::uint8_t) noexcept;
using SynchronizeQuantityFn = void(__fastcall*)(
    void*, void*, void*, std::int32_t) noexcept;

const D2RL::PluginContext* Context{};
std::uint8_t* Base{};
Config Settings{};
std::string LoadedConfigPath{"built-in defaults"};

QueueOutgoingPacketFn QueueOutgoingPacket{};
TargetingPacketWorkerFn OriginalTargetingPacketWorker{};
CainIdentifyCallbackFn OriginalCainIdentifyCallback{};
GetLocalDataContextFn GetLocalDataContext{};
GetLocalPlayerFn GetLocalPlayer{};
IsVirtualKeyDownFn IsVirtualKeyDown{};
GetLocalizedStringByKeyFn GetLocalizedStringByKey{};
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
GetInventoryOwnerIdFn GetInventoryOwnerId{};
GetFirstCorpseFn GetFirstCorpse{};
GetNextCorpseFn GetNextCorpse{};
GetCorpseUnitIdFn GetCorpseUnitId{};
CheckItemFlagFn CheckItemFlag{};
SetItemFlagFn SetItemFlag{};
GetItemSuffixIdFn GetItemSuffixId{};
GetUnitStatFn GetUnitStat{};
CheckStateFn CheckState{};
IdentifyItemFn IdentifyItem{};
SynchronizeQuantityFn SynchronizeQuantity{};

std::atomic<std::uint64_t> RequestsSent{};
std::atomic<std::uint64_t> GesturesObserved{};
std::atomic<std::uint64_t> TargetingWorkersObserved{};
std::atomic<std::uint64_t> RequestsAccepted{};
std::atomic<std::uint64_t> RequestsRejected{};
std::atomic<std::uint64_t> ItemsIdentified{};
std::atomic<std::uint64_t> ChargesConsumed{};
std::atomic<std::uint32_t> HoveredIdentifyTomeGuid{};
std::atomic<std::uint64_t> HoveredIdentifyTomeTick{};
std::atomic<std::uint32_t> PendingMassIdGuid{};
std::atomic_bool SuppressRightButtonUp{};
std::atomic_bool PluginActive{};
void* TooltipRelayPage{};
bool TooltipCallSitesInstalled{};
HWND GameWindow{};
WNDPROC OriginalWindowProc{};

constexpr D2RL::PluginInfo Info{
    .infoSize = D2RL::PluginInfoSize,
    .apiVersion = D2RL_PLUGIN_API_VERSION,
    .id = "mass-id",
    .name = "MassID",
    .version = "0.2.9",
    .author = "RuffnecKk",
    .description = "Identifies inventory, Cube and stash items from an Identify Tome.",
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

bool LoadConfig() noexcept {
    Settings = {};
    LoadedConfigPath = "built-in defaults";

    std::filesystem::path activeModConfigDirectory;
    std::filesystem::path scopeConfigDirectory;
    if (Context && Context->activeMod && Context->activeMod[0] != '\0'
            && Context->modSupportDirectory
            && Context->modSupportDirectory[0] != L'\0') {
        activeModConfigDirectory =
            std::filesystem::path(Context->modSupportDirectory) / L"config";
    }
    if (Context && Context->pluginConfigPath
            && Context->pluginConfigPath[0] != L'\0') {
        scopeConfigDirectory =
            std::filesystem::path(Context->pluginConfigPath).parent_path();
    }
    std::error_code currentPathError;
    const auto currentPath = std::filesystem::current_path(currentPathError);
    const auto globalConfigDirectory = currentPathError
        ? std::filesystem::path{}
        : currentPath / L"d2rloader" / L"config";
    const auto candidates = BuildConfigCandidates(
        activeModConfigDirectory,
        scopeConfigDirectory,
        globalConfigDirectory,
        ConfigFileName);

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
            char message[320]{};
            std::snprintf(
                message,
                sizeof(message),
                "MassID: signature mismatch for %s at RVA 0x%llX.",
                label,
                static_cast<unsigned long long>(rva));
            Context->LogError(message);
        }
    };

    // This is a composable live entry that MassID calls but does not hook.
    // A plugin loaded earlier may legitimately own its prologue.
    if (!IsExecutableAddress(Base + QueueOutgoingPacketRva)) {
        valid = false;
        if (Context) {
            Context->LogError(
                "MassID: CLIENT_QueueOutgoingPacket is not executable.");
        }
    }
    check(TargetingPacketWorkerRva, TargetingPacketWorkerExpected,
        "CLIENT_ProcessTargetingPacket");
    check(IsVirtualKeyDownRva, IsVirtualKeyDownExpected,
        "INPUT_IsVirtualKeyDownAsync");
    check(LegacyDropAppenderCallRva, LegacyDropAppenderCallExpected,
        "UI_LegacyInventoryTooltipAppenderDrop call");
    check(ModernDropAppenderCallRva, ModernDropAppenderCallExpected,
        "UI_ModernInventoryTooltipAppenderDrop call");
    check(LegacyMoveAppenderCallRvas[0], LegacyMoveAppenderCallsExpected[0],
        "UI_LegacyInventoryTooltipAppenderMove call A");
    check(LegacyMoveAppenderCallRvas[1], LegacyMoveAppenderCallsExpected[1],
        "UI_LegacyInventoryTooltipAppenderMove call B");
    check(ModernMoveAppenderCallRvas[0], ModernMoveAppenderCallsExpected[0],
        "UI_ModernInventoryTooltipAppenderMove call A");
    check(ModernMoveAppenderCallRvas[1], ModernMoveAppenderCallsExpected[1],
        "UI_ModernInventoryTooltipAppenderMove call B");
    check(ModernMoveAppenderCallRvas[2], ModernMoveAppenderCallsExpected[2],
        "UI_ModernInventoryTooltipAppenderMove call C");
    check(AlternateMoveAppenderCallRva, AlternateMoveAppenderCallExpected,
        "UI_AlternateInventoryTooltipAppenderMove call");
    check(CainIdentifyCallbackRva, CainIdentifyCallbackExpected,
        "D2GAME_PACKETCALLBACK_Rcv0x34_IdentifyItemsWithNpc");
    check(IdentifyItemRva, IdentifyItemExpected,
        "D2GAME_ITEMS_Identify");
    check(SynchronizeQuantityRva, SynchronizeQuantityExpected,
        "SynchronizeItemAndBoundSkillQuantity");
    check(GetLocalDataContextRva, GetLocalDataContextExpected,
        "CLIENT_GetLocalDataContext");
    check(GetLocalPlayerRva, GetLocalPlayerExpected,
        "CLIENT_GetLocalPlayer");
    check(GetLocalizedStringByKeyRva, GetLocalizedStringByKeyExpected,
        "LOCALIZATION_GetStringByKey");
    check(GetUnitStatRva, GetUnitStatExpected, "STATLIST_GetUnitStat");
    check(CheckStateRva, CheckStateExpected, "STATES_CheckState");
    check(GetUnitIdRva, GetUnitIdExpected, "UNITS_GetUnitId");
    check(GetUnitInventoryRva, GetUnitInventoryExpected, "UNITS_GetInventory");
    check(GetUnitTypeRva, GetUnitTypeExpected, "UNITS_GetUnitType");
    check(GetInventoryPageRva, GetInventoryPageExpected, "ITEMS_GetInvPage");
    check(CheckItemFlagRva, CheckItemFlagExpected, "ITEMS_CheckItemFlag");
    check(SetItemFlagRva, SetItemFlagExpected, "ITEMS_SetItemFlag");
    check(GetItemSuffixIdRva, GetItemSuffixIdExpected,
        "ITEMS_GetSuffixId");
    check(GetItemCodeRva, GetItemCodeExpected, "ITEMS_GetItemCode");
    check(GetCursorItemRva, GetCursorItemExpected, "INVENTORY_GetCursorItem");
    check(GetFirstItemRva, GetFirstItemExpected, "INVENTORY_GetFirstItem");
    check(GetNextItemRva, GetNextItemExpected, "INVENTORY_GetNextItem");
    check(GetParentInventoryRva, GetParentInventoryExpected,
        "INVENTORY_GetParentInventory");
    check(GetInventoryOwnerIdRva, GetInventoryOwnerIdExpected,
        "INVENTORY_GetOwnerId");
    check(GetFirstCorpseRva, GetFirstCorpseExpected,
        "INVENTORY_GetFirstCorpse");
    check(GetNextCorpseRva, GetNextCorpseExpected,
        "INVENTORY_GetNextCorpse");
    check(GetCorpseUnitIdRva, GetCorpseUnitIdExpected,
        "INVENTORY_GetUnitGUIDFromCorpse");
    check(ServerUnitRva, ServerUnitExpected, "SUNIT_GetServerUnit");
    return valid;
}

bool CaptureMassIdRequest(void* item, const char* path) noexcept {
    GesturesObserved.fetch_add(1, std::memory_order_relaxed);
    void* localPlayer = GetLocalPlayer(GetLocalDataContext());
    void* inventory = localPlayer ? GetUnitInventory(localPlayer) : nullptr;
    const bool cursorEmpty = inventory && GetCursorItem(inventory) == nullptr;
    const auto unitType = item ? GetUnitType(item) : -1;
    const auto itemCode = item ? GetItemCode(item) : 0;
    if (item && ShouldCaptureGesture(
            Settings.enabled, true, true, cursorEmpty, unitType, itemCode)) {
        const auto tomeGuid = static_cast<std::uint32_t>(GetUnitId(item));
        const auto packet = MakeRequest(tomeGuid);
        QueueOutgoingPacket(
            packet.data(), static_cast<std::int32_t>(packet.size()));
        RequestsSent.fetch_add(1, std::memory_order_relaxed);
        if (Context) {
            char message[192]{};
            std::snprintf(
                message,
                sizeof(message),
                "MassID: %s Shift-right-click captured; queued bytes=34 %08X %08X %08X 00000000 00000000; Tome GUID %u.",
                path,
                tomeGuid,
                RequestMarker,
                RequestGuard,
                tomeGuid);
            Context->LogInfo(message);
        }
        return true;
    }
    if (Context) {
        char message[256]{};
        std::snprintf(
            message,
            sizeof(message),
            "MassID: %s Shift-right-click ignored; item=%p; cursorEmpty=%s; unitType=%d; itemCode=0x%08X.",
            path,
            item,
            cursorEmpty ? "true" : "false",
            unitType,
            itemCode);
        Context->LogInfo(message);
    }
    return false;
}

void* FindIdentifyTomeForPacket(const std::uint8_t* packet) noexcept {
    if (!packet) return nullptr;
    void* localPlayer = GetLocalPlayer(GetLocalDataContext());
    void* inventory = localPlayer ? GetUnitInventory(localPlayer) : nullptr;
    if (!inventory) return nullptr;
    const auto suffixId = static_cast<std::uint16_t>(
        packet[1] | (static_cast<std::uint16_t>(packet[2]) << 8));
    for (void* item = GetFirstItem(inventory); item; item = GetNextItem(item)) {
        if (GetUnitType(item) == 4
                && GetParentInventory(item) == inventory
                && GetItemCode(item) == IdentifyTomeCode
                && GetItemSuffixId(item) == suffixId
                && IsSupportedInventoryPage(GetInventoryPage(item))) {
            return item;
        }
    }
    return nullptr;
}

void* FindIdentifyTomeByGuid(std::uint32_t tomeGuid) noexcept {
    if (tomeGuid == 0) return nullptr;
    void* localPlayer = GetLocalPlayer(GetLocalDataContext());
    void* inventory = localPlayer ? GetUnitInventory(localPlayer) : nullptr;
    if (!inventory) return nullptr;
    for (void* item = GetFirstItem(inventory); item; item = GetNextItem(item)) {
        if (GetUnitType(item) == 4
                && GetParentInventory(item) == inventory
                && GetItemCode(item) == IdentifyTomeCode
                && static_cast<std::uint32_t>(GetUnitId(item)) == tomeGuid
                && IsSupportedInventoryPage(GetInventoryPage(item))) {
            return item;
        }
    }
    return nullptr;
}

void __fastcall HookTargetingPacketWorker(
        const std::uint8_t* packet) noexcept {
    const bool nativeShift = IsVirtualKeyDown(VK_SHIFT) != 0;
    const bool win32Shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    void* tome = FindIdentifyTomeForPacket(packet);
    TargetingWorkersObserved.fetch_add(1, std::memory_order_relaxed);
    if (Context) {
        char message[224]{};
        std::snprintf(
            message,
            sizeof(message),
            "MassID: targeting worker observed; tome=%p; nativeShift=%s; win32Shift=%s.",
            tome,
            nativeShift ? "true" : "false",
            win32Shift ? "true" : "false");
        Context->LogInfo(message);
    }
    if (Settings.enabled && (nativeShift || win32Shift)
            && CaptureMassIdRequest(tome, "targeting worker")) {
        return;
    }
    OriginalTargetingPacketWorker(packet);
}

LRESULT CALLBACK MassIDWindowProc(
        HWND window, UINT message, WPARAM wParam, LPARAM lParam) noexcept {
    if (PluginActive.load(std::memory_order_acquire)
            && Settings.enabled && message == WM_RBUTTONDOWN) {
        const bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0
            || (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
        if (shift) {
            const auto now = GetTickCount64();
            const auto hoveredAt = HoveredIdentifyTomeTick.load(
                std::memory_order_acquire);
            const auto tomeGuid = HoveredIdentifyTomeGuid.load(
                std::memory_order_acquire);
            const auto age = now >= hoveredAt ? now - hoveredAt
                                               : UINT64_MAX;
            if (Context) {
                char logMessage[192]{};
                std::snprintf(
                    logMessage,
                    sizeof(logMessage),
                    "MassID: window Shift-right-button observed; hoveredGuid=%u; ageMs=%llu.",
                    tomeGuid,
                    static_cast<unsigned long long>(age));
                Context->LogInfo(logMessage);
            }
            if (tomeGuid != 0 && age <= 1500) {
                PendingMassIdGuid.store(tomeGuid, std::memory_order_release);
                SuppressRightButtonUp.store(true, std::memory_order_release);
                return 0;
            }
        }
    }
    if (message == WM_RBUTTONUP
            && SuppressRightButtonUp.exchange(
                false, std::memory_order_acq_rel)) {
        return 0;
    }
    return OriginalWindowProc
        ? CallWindowProcW(OriginalWindowProc, window, message, wParam, lParam)
        : DefWindowProcW(window, message, wParam, lParam);
}

BOOL CALLBACK FindGameWindowCallback(HWND window, LPARAM state) noexcept {
    DWORD processId{};
    GetWindowThreadProcessId(window, &processId);
    if (processId != GetCurrentProcessId()
            || GetWindow(window, GW_OWNER) != nullptr
            || !IsWindowVisible(window)) {
        return TRUE;
    }
    *reinterpret_cast<HWND*>(state) = window;
    return FALSE;
}

bool TryInstallGameWindowHook() noexcept {
    if (GameWindow && OriginalWindowProc && IsWindow(GameWindow)) return true;
    HWND window{};
    EnumWindows(FindGameWindowCallback, reinterpret_cast<LPARAM>(&window));
    if (!window) return false;
    SetLastError(ERROR_SUCCESS);
    const auto previous = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(
        window,
        GWLP_WNDPROC,
        reinterpret_cast<LONG_PTR>(&MassIDWindowProc)));
    if (!previous && GetLastError() != ERROR_SUCCESS) return false;
    GameWindow = window;
    OriginalWindowProc = previous;
    if (Context) {
        char message[160]{};
        std::snprintf(
            message,
            sizeof(message),
            "MassID: game window input hook installed; hwnd=%p.",
            window);
        Context->LogInfo(message);
    }
    return true;
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
        void* inventoryActor,
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
            IdentifyItem(game, inventoryActor, item, 1);
            if (CheckItemFlag(item, IdentifiedItemFlag) != 0) {
                ++identified;
            }
        }
        item = next;
    }
    return identified;
}

struct SharedIdentifyResult {
    std::int32_t identified{};
    std::int32_t containers{};
};

SharedIdentifyResult IdentifySharedStashes(
        void* game,
        void* player,
        void* playerInventory,
        std::int32_t budget) noexcept {
    SharedIdentifyResult result{};
    if (!game || !player || !playerInventory || budget <= 0) return result;

    const auto playerGuid = GetUnitId(player);
    for (void* record = GetFirstCorpse(playerInventory);
            record && result.identified < budget;
            record = GetNextCorpse(record)) {
        const auto proxyGuid = GetCorpseUnitId(record);
        void* proxy = GetServerUnit(game, 0, proxyGuid);
        if (!proxy || proxy == player
                || CheckState(proxy, SharedStashProxyState) == 0) {
            continue;
        }

        void* proxyInventory = GetUnitInventory(proxy);
        if (!proxyInventory
                || GetInventoryOwnerId(proxyInventory) != playerGuid) {
            continue;
        }

        ++result.containers;
        // ITEMS_SendItemUpdate has a dedicated state-0xBA route for shared
        // stash proxy players.  The proxy must remain the identification actor;
        // using the main player makes the client deserialize the update into
        // personal inventory and leaves a frozen ghost until the next reload.
        result.identified += IdentifyPage(
            game,
            proxy,
            proxyInventory,
            StashPage,
            budget - result.identified);
    }
    return result;
}

std::int32_t __fastcall HookCainIdentifyCallback(
        void* game,
        void* player,
        const std::uint8_t* packet,
        std::int32_t packetSize) noexcept {
    if (Context) {
        const auto field = [packet, packetSize](std::size_t offset) noexcept {
            return packet && packetSize >= static_cast<std::int32_t>(offset + 4)
                ? ReadU32(packet, offset)
                : UINT32_MAX;
        };
        char message[256]{};
        std::snprintf(
            message,
            sizeof(message),
            "MassID: server opcode-0x34 callback observed; size=%d; opcode=0x%02X; fields=%08X %08X %08X %08X %08X.",
            packetSize,
            packet && packetSize > 0 ? packet[0] : 0,
            field(1), field(5), field(9), field(13), field(17));
        Context->LogInfo(message);
    }
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
        if (Context) {
            char message[160]{};
            std::snprintf(
                message,
                sizeof(message),
                "MassID: rejected request for invalid Tome GUID %u.",
                static_cast<std::uint32_t>(tomeGuid));
            Context->LogWarn(message);
        }
        return 0;
    }
    SetItemFlag(tome, 0x00000004u, 0);

    const auto quantity = GetUnitStat(
        tome, static_cast<std::int32_t>(QuantityStat), 0);
    const auto budget = IdentificationBudget(
        Settings.freeIdentification, quantity);

    std::int32_t inventoryIdentified{};
    std::int32_t cubeIdentified{};
    std::int32_t personalStashIdentified{};
    SharedIdentifyResult sharedStash{};
    if (budget > 0) {
        inventoryIdentified = IdentifyPage(
            game, player, inventory, InventoryPage, budget);
        if (inventoryIdentified < budget) {
            cubeIdentified = IdentifyPage(
                game,
                player,
                inventory,
                CubePage,
                budget - inventoryIdentified);
        }
        if (inventoryIdentified + cubeIdentified < budget) {
            personalStashIdentified = IdentifyPage(
                game,
                player,
                inventory,
                StashPage,
                budget - inventoryIdentified - cubeIdentified);
        }
        const auto mainInventoryIdentified = inventoryIdentified
            + cubeIdentified
            + personalStashIdentified;
        if (mainInventoryIdentified < budget) {
            sharedStash = IdentifySharedStashes(
                game,
                player,
                inventory,
                budget - mainInventoryIdentified);
        }
    }
    const auto identified = inventoryIdentified
        + cubeIdentified
        + personalStashIdentified
        + sharedStash.identified;

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
    if (Context) {
        char message[384]{};
        std::snprintf(
            message,
            sizeof(message),
            "MassID: accepted Tome GUID %u; quantity=%d; identified=%d (inventory=%d; cube=%d; personalStash=%d; sharedStash=%d; sharedContainers=%d); consumed=%d; freeIdentification=%s.",
            static_cast<std::uint32_t>(tomeGuid),
            quantity,
            identified,
            inventoryIdentified,
            cubeIdentified,
            personalStashIdentified,
            sharedStash.identified,
            sharedStash.containers,
            Settings.freeIdentification ? 0 : identified,
            Settings.freeIdentification ? "true" : "false");
        Context->LogInfo(message);
    }
    return 0;
}

std::string_view CurrentMassIdTooltipText() noexcept {
    constexpr std::string_view key = "ItemStats1h";
    if (!GetLocalizedStringByKey) return TooltipLocales.front().massIdText;
    const GameStringView view{key.data(), key.size()};
    const auto* defense = GetLocalizedStringByKey(&view);
    if (!defense || defense[0] == '\0') return TooltipLocales.front().massIdText;
    for (const auto& locale : TooltipLocales) {
        if (locale.defenseFingerprint == defense) return locale.massIdText;
    }
    return TooltipLocales.front().massIdText;
}

const char* __fastcall HookTooltipAppender(
        const GameStringView* key, void* item) noexcept {
    const auto* original = GetLocalizedStringByKey(key);
    TryInstallGameWindowHook();
    if (!Settings.enabled || !original || !item
            || GetUnitType(item) != 4
            || GetItemCode(item) != IdentifyTomeCode) {
        HoveredIdentifyTomeGuid.store(0, std::memory_order_release);
        HoveredIdentifyTomeTick.store(0, std::memory_order_release);
        return original;
    }
    HoveredIdentifyTomeGuid.store(
        static_cast<std::uint32_t>(GetUnitId(item)),
        std::memory_order_release);
    HoveredIdentifyTomeTick.store(GetTickCount64(), std::memory_order_release);
    const auto itemGuid = static_cast<std::uint32_t>(GetUnitId(item));
    const auto pendingGuid = PendingMassIdGuid.exchange(
        0, std::memory_order_acq_rel);
    if (pendingGuid != 0) {
        if (pendingGuid == itemGuid) {
            CaptureMassIdRequest(item, "deferred game-thread input");
        } else if (Context) {
            char message[160]{};
            std::snprintf(
                message,
                sizeof(message),
                "MassID: deferred request discarded; pendingGuid=%u; renderedGuid=%u.",
                pendingGuid,
                itemGuid);
            Context->LogWarn(message);
        }
    }
    try {
        thread_local std::string enhanced;
        enhanced = AddMassIdTooltipLine(
            original, CurrentMassIdTooltipText());
        return enhanced.c_str();
    } catch (...) {
        if (Context) {
            Context->LogError(
                "MassID: Identify Tome tooltip-appender transform failed safely.");
        }
        return original;
    }
}

void* AllocateNear(void* hint, std::size_t size) noexcept {
    SYSTEM_INFO systemInfo{};
    GetSystemInfo(&systemInfo);
    const auto granularity = static_cast<std::uintptr_t>(
        systemInfo.dwAllocationGranularity);
    const auto base = reinterpret_cast<std::uintptr_t>(hint)
        & ~(granularity - 1);
    for (std::uintptr_t delta = granularity;
            delta < 0x70000000ULL; delta += granularity) {
        if (base > std::numeric_limits<std::uintptr_t>::max() - delta) break;
        if (auto* memory = VirtualAlloc(
                reinterpret_cast<void*>(base + delta), size,
                MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE)) {
            return memory;
        }
    }
    return nullptr;
}

bool WriteTooltipRelay(
        std::uint8_t* destination,
        std::span<const std::uint8_t> itemMove) noexcept {
    if (!destination || itemMove.empty()) return false;
    std::memcpy(destination, itemMove.data(), itemMove.size());
    auto* jump = destination + itemMove.size();
    jump[0] = 0xFF;
    jump[1] = 0x25;
    jump[2] = jump[3] = jump[4] = jump[5] = 0x00;
    const auto target = reinterpret_cast<std::uint64_t>(
        &HookTooltipAppender);
    std::memcpy(jump + 6, &target, sizeof(target));
    return true;
}

bool InstallTooltipCallSites() noexcept {
    constexpr std::size_t RelayStride = 32;
    constexpr std::size_t RelayBytes = RelayStride * 3;
    constexpr std::array<std::uint8_t, 3> LegacyItemMove{
        0x4C, 0x89, 0xEA, // mov rdx, r13
    };
    constexpr std::array<std::uint8_t, 4> ModernItemMove{
        0x48, 0x8B, 0x55, 0x88, // mov rdx, [rbp-0x78]
    };
    constexpr std::array<std::uint8_t, 3> ModernMoveItemMove{
        0x4C, 0x89, 0xE2, // mov rdx, r12
    };

    TooltipRelayPage = AllocateNear(
        Base + LegacyDropAppenderCallRva, RelayBytes);
    if (!TooltipRelayPage) return false;
    auto* relays = static_cast<std::uint8_t*>(TooltipRelayPage);
    if (!WriteTooltipRelay(relays, LegacyItemMove)
            || !WriteTooltipRelay(relays + RelayStride, ModernItemMove)
            || !WriteTooltipRelay(
                relays + RelayStride * 2, ModernMoveItemMove)) {
        return false;
    }
    DWORD previousProtection{};
    if (!VirtualProtect(
            relays, RelayBytes, PAGE_EXECUTE_READ, &previousProtection)) {
        return false;
    }
    FlushInstructionCache(GetCurrentProcess(), relays, RelayBytes);

    const auto relayAddress = reinterpret_cast<std::uintptr_t>(relays);
    const auto baseAddress = reinterpret_cast<std::uintptr_t>(Base);
    if (relayAddress < baseAddress) return false;
    const auto relayRva = relayAddress - baseAddress;
    bool installed = Context->PatchCallRel32(
            LegacyDropAppenderCallRva,
            LegacyDropAppenderCallExpected.data(),
            static_cast<std::uint32_t>(LegacyDropAppenderCallExpected.size()),
            relayRva,
            static_cast<std::uint32_t>(LegacyDropAppenderCallExpected.size()))
        && Context->PatchCallRel32(
            ModernDropAppenderCallRva,
            ModernDropAppenderCallExpected.data(),
            static_cast<std::uint32_t>(ModernDropAppenderCallExpected.size()),
            relayRva + RelayStride,
            static_cast<std::uint32_t>(ModernDropAppenderCallExpected.size()));
    for (std::size_t index = 0;
            installed && index < LegacyMoveAppenderCallRvas.size(); ++index) {
        installed = Context->PatchCallRel32(
            LegacyMoveAppenderCallRvas[index],
            LegacyMoveAppenderCallsExpected[index].data(),
            static_cast<std::uint32_t>(
                LegacyMoveAppenderCallsExpected[index].size()),
            relayRva,
            static_cast<std::uint32_t>(
                LegacyMoveAppenderCallsExpected[index].size()));
    }
    for (std::size_t index = 0;
            installed && index < ModernMoveAppenderCallRvas.size(); ++index) {
        installed = Context->PatchCallRel32(
            ModernMoveAppenderCallRvas[index],
            ModernMoveAppenderCallsExpected[index].data(),
            static_cast<std::uint32_t>(
                ModernMoveAppenderCallsExpected[index].size()),
            relayRva + RelayStride * 2,
            static_cast<std::uint32_t>(
                ModernMoveAppenderCallsExpected[index].size()));
    }
    return installed && Context->PatchCallRel32(
        AlternateMoveAppenderCallRva,
        AlternateMoveAppenderCallExpected.data(),
        static_cast<std::uint32_t>(
            AlternateMoveAppenderCallExpected.size()),
        relayRva,
        static_cast<std::uint32_t>(
            AlternateMoveAppenderCallExpected.size()));
}

bool InstallHooks() noexcept {
    if (!Context->InstallInlineHook(
            TargetingPacketWorkerRva,
            TargetingPacketWorkerExpected.data(),
            static_cast<std::uint32_t>(
                TargetingPacketWorkerExpected.size()),
            HookTargetingPacketWorker,
            &OriginalTargetingPacketWorker)) {
        Context->LogError("MassID: targeting packet worker hook refused.");
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
    TooltipCallSitesInstalled = InstallTooltipCallSites();
    if (!TooltipCallSitesInstalled) {
        Context->LogError(
            "MassID: tooltip appender call-sites are unavailable.");
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
        "MassID 0.2.9: enabled=%s; freeIdentification=%s; windowInput=%s; pendingGuid=%u; targetingWorkers=%llu; gestures=%llu; sent=%llu; accepted=%llu; rejected=%llu; identified=%llu; consumed=%llu; tooltip=%s; JSON=%s.",
        Settings.enabled ? "true" : "false",
        Settings.freeIdentification ? "true" : "false",
        GameWindow && OriginalWindowProc ? "installed" : "pending",
        PendingMassIdGuid.load(std::memory_order_acquire),
        static_cast<unsigned long long>(TargetingWorkersObserved.load()),
        static_cast<unsigned long long>(GesturesObserved.load()),
        static_cast<unsigned long long>(RequestsSent.load()),
        static_cast<unsigned long long>(RequestsAccepted.load()),
        static_cast<unsigned long long>(RequestsRejected.load()),
        static_cast<unsigned long long>(ItemsIdentified.load()),
        static_cast<unsigned long long>(ChargesConsumed.load()),
        TooltipCallSitesInstalled ? "drop-and-move-appenders" : "unavailable",
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

    QueueOutgoingPacket = At<QueueOutgoingPacketFn>(QueueOutgoingPacketRva);
    GetLocalDataContext = At<GetLocalDataContextFn>(GetLocalDataContextRva);
    GetLocalPlayer = At<GetLocalPlayerFn>(GetLocalPlayerRva);
    IsVirtualKeyDown = At<IsVirtualKeyDownFn>(IsVirtualKeyDownRva);
    GetLocalizedStringByKey = At<GetLocalizedStringByKeyFn>(
        GetLocalizedStringByKeyRva);
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
    GetInventoryOwnerId = At<GetInventoryOwnerIdFn>(GetInventoryOwnerIdRva);
    GetFirstCorpse = At<GetFirstCorpseFn>(GetFirstCorpseRva);
    GetNextCorpse = At<GetNextCorpseFn>(GetNextCorpseRva);
    GetCorpseUnitId = At<GetCorpseUnitIdFn>(GetCorpseUnitIdRva);
    CheckItemFlag = At<CheckItemFlagFn>(CheckItemFlagRva);
    SetItemFlag = At<SetItemFlagFn>(SetItemFlagRva);
    GetItemSuffixId = At<GetItemSuffixIdFn>(GetItemSuffixIdRva);
    GetUnitStat = At<GetUnitStatFn>(GetUnitStatRva);
    CheckState = At<CheckStateFn>(CheckStateRva);
    IdentifyItem = At<IdentifyItemFn>(IdentifyItemRva);
    SynchronizeQuantity = At<SynchronizeQuantityFn>(SynchronizeQuantityRva);

    if (Settings.enabled && !InstallHooks()) return false;
    PluginActive.store(true, std::memory_order_release);
    const bool windowInputInstalled = !Settings.enabled
        || TryInstallGameWindowHook();
    if (Settings.enabled && !windowInputInstalled) {
        context->LogWarn(
            "MassID: game window input hook pending until the Tome tooltip is rendered.");
    }

    if (!context->RegisterConsoleCommand(
            "mass-id", Status, "Show MassID status and counters.")) {
        context->LogWarn("MassID: status command could not be registered.");
    }

    char message[480]{};
    std::snprintf(
        message,
        sizeof(message),
        "MassID 0.2.9 %s for D2R 3.2.92777; window capture=%s; direct packed request dispatch=tooltip-frame; server 0x34 routing=corrected; target containers=inventory,cube,personal-stash,shared-stash; shared update actor=state-0xBA proxy; targeting-worker fallback=%s; localized tooltip=%s; freeIdentification=%s (JSON: %s).",
        Settings.enabled ? "active" : "disabled",
        Settings.enabled
            ? (windowInputInstalled ? "installed" : "pending")
            : "left unchanged",
        Settings.enabled ? "installed" : "left unchanged",
        TooltipCallSitesInstalled ? "drop-and-move-appenders" : "unavailable",
        Settings.freeIdentification ? "true" : "false",
        LoadedConfigPath.c_str());
    context->LogInfo(message);
    return true;
}

D2RL_PLUGIN_EXPORT void D2RLoaderUnloadPlugin() noexcept {
    PluginActive.store(false, std::memory_order_release);
    if (GameWindow && OriginalWindowProc && IsWindow(GameWindow)
            && reinterpret_cast<WNDPROC>(GetWindowLongPtrW(
                GameWindow, GWLP_WNDPROC)) == &MassIDWindowProc) {
        SetWindowLongPtrW(
            GameWindow,
            GWLP_WNDPROC,
            reinterpret_cast<LONG_PTR>(OriginalWindowProc));
    }
    TooltipCallSitesInstalled = false;
    Context = nullptr;
    Base = nullptr;
}
