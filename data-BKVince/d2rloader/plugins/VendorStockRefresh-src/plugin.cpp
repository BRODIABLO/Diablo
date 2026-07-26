#define NOMINMAX
#include <D2RLPlugin/api.h>
#include <nlohmann/json.hpp>
#include "vendor_refresh_policy.hpp"

#include <Windows.h>

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using ruffneckk::vendor_stock_refresh::RefreshActionForPanel;
using ruffneckk::vendor_stock_refresh::ShouldArmNormalRefresh;

constexpr std::uint32_t SupportedBuild = 92777;
constexpr wchar_t ConfigFileName[] = L"VendorStockRefresh.json";

constexpr std::uintptr_t SendVendorRefreshRva = 0x10F520;
constexpr std::uintptr_t IsGamblingRva = 0x10CAC0;
constexpr std::uintptr_t SendNineBytePacketRva = 0x0EC730;
constexpr std::uintptr_t CurrentNpcGuidRva = 0x2A4875C;
constexpr std::uintptr_t ConfigureVendorInteractionRva = 0x502F60;
constexpr std::uintptr_t GetPlayerDataRva = 0x34B240;
constexpr std::uintptr_t GetUnitClassIdRva = 0x349860;
constexpr std::uintptr_t GetVendorChainEntryRva = 0x502B70;

constexpr std::uintptr_t RefreshButtonEnableRva = 0x24137D;
constexpr std::uintptr_t RefreshButtonVisibilityRva = 0x241391;
constexpr std::uintptr_t RefreshInputGuardRva = 0x240E0D;

constexpr std::size_t PlayerDataVendorClassOffset = 0xFC;
constexpr std::size_t PlayerDataVendorModeOffset = 0x100;
constexpr std::size_t VendorEntryFilledOffset = 0x34;
constexpr std::size_t VendorEntryRefreshPendingOffset = 0x35;

constexpr std::array<std::uint8_t, 19> SendVendorRefreshExpected{
    0x44, 0x8B, 0x05, 0x35, 0x92, 0x93, 0x02, 0xBA,
    0x02, 0x00, 0x00, 0x00, 0xB1, 0x38, 0xE9, 0xFD,
    0xD1, 0xFD, 0xFF
};
constexpr std::array<std::uint8_t, 32> ConfigureVendorInteractionExpected{
    0x40, 0x56, 0x57, 0x41, 0x57, 0x48, 0x83, 0xEC,
    0x20, 0x48, 0x89, 0x6C, 0x24, 0x48, 0x41, 0x0F,
    0xB6, 0xF1, 0x4C, 0x89, 0x74, 0x24, 0x50, 0x49,
    0x8B, 0xE8, 0x4C, 0x8B, 0xF1, 0x4C, 0x8B, 0xFA
};
constexpr std::array<std::uint8_t, 7> IsGamblingExpected{
    0x8B, 0x05, 0x42, 0xBD, 0x93, 0x02, 0xC3
};
constexpr std::array<std::uint8_t, 48> SendNineBytePacketExpected{
    0x48, 0x83, 0xEC, 0x48, 0x48, 0x8B, 0x05, 0x8D,
    0xEB, 0x8D, 0x02, 0x48, 0x33, 0xC4, 0x48, 0x89,
    0x44, 0x24, 0x30, 0x88, 0x4C, 0x24, 0x20, 0x48,
    0x8D, 0x4C, 0x24, 0x20, 0x89, 0x54, 0x24, 0x21,
    0xBA, 0x09, 0x00, 0x00, 0x00, 0x44, 0x89, 0x44,
    0x24, 0x25, 0xE8, 0x41, 0x1B, 0x00, 0x00, 0x48
};
constexpr std::array<std::uint8_t, 24> GetPlayerDataExpected{
    0x40, 0x53, 0x48, 0x83, 0xEC, 0x20, 0x48, 0x8B,
    0xD9, 0x48, 0x85, 0xC9, 0x75, 0x13, 0x88, 0x4C,
    0x24, 0x30, 0x48, 0x8D, 0x4C, 0x24, 0x30, 0xE8
};
constexpr std::array<std::uint8_t, 24> GetUnitClassIdExpected{
    0x48, 0x83, 0xEC, 0x28, 0x48, 0x85, 0xC9, 0x75,
    0x1D, 0x88, 0x4C, 0x24, 0x30, 0x48, 0x8D, 0x4C,
    0x24, 0x30, 0xE8, 0x49, 0xCB, 0xFF, 0xFF, 0x84
};
constexpr std::array<std::uint8_t, 24> GetVendorChainEntryExpected{
    0x48, 0x89, 0x5C, 0x24, 0x08, 0x57, 0x48, 0x83,
    0xEC, 0x20, 0x48, 0x8B, 0xC2, 0x49, 0x8B, 0xF8,
    0x48, 0x8B, 0xD9, 0x48, 0x8D, 0x15, 0xD6, 0x6D
};

constexpr std::array<std::uint8_t, 20> RefreshButtonEnableExpected{
    0x83, 0xBB, 0x68, 0x01, 0x00, 0x00, 0x02, 0x48,
    0x8B, 0xC8, 0x4C, 0x8B, 0x00, 0x0F, 0x94, 0xC2,
    0x41, 0xFF, 0x50, 0x48
};
constexpr std::array<std::uint8_t, 20> RefreshButtonEnableReplacement{
    0x83, 0xBB, 0x68, 0x01, 0x00, 0x00, 0x01, 0x48,
    0x8B, 0xC8, 0x4C, 0x8B, 0x00, 0x0F, 0x95, 0xC2,
    0x41, 0xFF, 0x50, 0x48
};
constexpr std::array<std::uint8_t, 20> RefreshButtonVisibilityExpected{
    0x83, 0xBB, 0x68, 0x01, 0x00, 0x00, 0x02, 0x48,
    0x8B, 0xCF, 0x4C, 0x8B, 0x07, 0x0F, 0x94, 0xC2,
    0x41, 0xFF, 0x50, 0x50
};
constexpr std::array<std::uint8_t, 20> RefreshButtonVisibilityReplacement{
    0x83, 0xBB, 0x68, 0x01, 0x00, 0x00, 0x01, 0x48,
    0x8B, 0xCF, 0x4C, 0x8B, 0x07, 0x0F, 0x95, 0xC2,
    0x41, 0xFF, 0x50, 0x50
};
constexpr std::array<std::uint8_t, 23> RefreshInputGuardExpected{
    0x83, 0xBE, 0x68, 0x01, 0x00, 0x00, 0x02, 0x0F,
    0x85, 0x6E, 0x03, 0x00, 0x00, 0x48, 0x8B, 0xCE,
    0xE8, 0xEE, 0x71, 0x61, 0x00, 0x84, 0xC0
};
constexpr std::array<std::uint8_t, 23> RefreshInputGuardReplacement{
    0x83, 0xBE, 0x68, 0x01, 0x00, 0x00, 0x01, 0x0F,
    0x84, 0x6E, 0x03, 0x00, 0x00, 0x48, 0x8B, 0xCE,
    0xE8, 0xEE, 0x71, 0x61, 0x00, 0x84, 0xC0
};

struct Config {
    bool enabled{true};
};

using SendVendorRefreshFn = void(__fastcall*)() noexcept;
using IsGamblingFn = std::int32_t(__fastcall*)() noexcept;
using SendNineBytePacketFn = void(__fastcall*)(
    std::uint8_t opcode,
    std::uint32_t action,
    std::uint32_t npcGuid
) noexcept;
using ConfigureVendorInteractionFn = void(__fastcall*)(
    void* game,
    void* npc,
    void* player,
    std::uint8_t mode
) noexcept;
using GetPlayerDataFn = void*(__fastcall*)(void* player) noexcept;
using GetUnitClassIdFn = std::int32_t(__fastcall*)(void* unit) noexcept;
using GetVendorChainEntryFn = void*(__fastcall*)(
    void* game,
    void* npc,
    std::int32_t* indexOut
) noexcept;

const D2RL::PluginContext* Context{};
std::uint8_t* Base{};
Config Settings{};
std::string LoadedConfigPath{"built-in defaults"};
SendVendorRefreshFn OriginalSendVendorRefresh{};
ConfigureVendorInteractionFn OriginalConfigureVendorInteraction{};
IsGamblingFn IsGambling{};
SendNineBytePacketFn SendNineBytePacket{};
GetPlayerDataFn GetPlayerData{};
GetUnitClassIdFn GetUnitClassId{};
GetVendorChainEntryFn GetVendorChainEntry{};
std::atomic<std::uint64_t> NormalRequestsSent{};
std::atomic<std::uint64_t> NormalRefreshesArmed{};
std::atomic<std::uint64_t> RejectedNormalRequests{};

constexpr D2RL::PluginInfo Info{
    .infoSize = D2RL::PluginInfoSize,
    .apiVersion = D2RL_PLUGIN_API_VERSION,
    .id = "vendor-stock-refresh",
    .name = "Vendor Stock Refresh",
    .version = "0.1.0",
    .author = "RuffnecKk",
    .description = "Refreshes a vendor's stock with one click.",
    .flags = D2RL::PluginFlags::NativeHooks,
};

template<class T>
T At(std::uintptr_t rva) noexcept {
    return reinterpret_cast<T>(Base + rva);
}

template<std::size_t Size>
bool Matches(std::uintptr_t rva, const std::array<std::uint8_t, Size>& expected) noexcept {
    return std::memcmp(Base + rva, expected.data(), Size) == 0;
}

bool LoadConfig() noexcept {
    Settings = {};
    LoadedConfigPath = "built-in defaults";

    std::vector<std::filesystem::path> candidates;
    if (Context && Context->modDirectory && Context->modDirectory[0] != L'\0') {
        candidates.emplace_back(std::filesystem::path(Context->modDirectory) / ConfigFileName);
    }
    candidates.emplace_back(ConfigFileName);

    bool malformedConfigFound{};
    for (const auto& path : candidates) {
        std::error_code error;
        if (!std::filesystem::is_regular_file(path, error)) continue;

        try {
            std::ifstream input(path);
            if (!input.is_open()) continue;
            const auto config = nlohmann::json::parse(input, nullptr, true, true);
            if (!config.is_object()) {
                throw std::invalid_argument("configuration root must be an object");
            }
            for (const auto& [key, value] : config.items()) {
                (void)value;
                if (key != "enabled") {
                    throw std::invalid_argument("unknown setting: " + key);
                }
            }
            if (config.contains("enabled") && !config.at("enabled").is_boolean()) {
                throw std::invalid_argument("enabled must be a boolean");
            }
            Settings.enabled = config.value("enabled", true);
            LoadedConfigPath = path.string();
            return true;
        } catch (const std::exception& exception) {
            malformedConfigFound = true;
            if (Context) {
                const auto message = std::string("VendorStockRefresh: invalid ")
                    + path.string() + " (" + exception.what() + ").";
                Context->LogError(message.c_str());
            }
        }
    }
    return !malformedConfigFound;
}

bool ValidateRuntime() noexcept {
    return Matches(IsGamblingRva, IsGamblingExpected)
        && Matches(SendNineBytePacketRva, SendNineBytePacketExpected)
        && Matches(GetPlayerDataRva, GetPlayerDataExpected)
        && Matches(GetUnitClassIdRva, GetUnitClassIdExpected)
        && Matches(GetVendorChainEntryRva, GetVendorChainEntryExpected);
}

void __fastcall HookSendVendorRefresh() noexcept {
    const auto isGambling = IsGambling() != 0;
    const auto action = RefreshActionForPanel(isGambling);
    const auto npcGuid = *At<const std::uint32_t*>(CurrentNpcGuidRva);
    if (!isGambling) {
        NormalRequestsSent.fetch_add(1, std::memory_order_relaxed);
    }
    SendNineBytePacket(0x38, action, npcGuid);
}

void __fastcall HookConfigureVendorInteraction(
    void* game,
    void* npc,
    void* player,
    std::uint8_t requestedMode
) noexcept {
    bool armed{};
    if (Settings.enabled && requestedMode == ruffneckk::vendor_stock_refresh::NormalVendorMode) {
        __try {
            auto* playerData = static_cast<std::uint8_t*>(GetPlayerData(player));
            const auto currentMode = playerData
                ? playerData[PlayerDataVendorModeOffset]
                : std::uint8_t{};
            const auto currentVendorClass = playerData
                ? *reinterpret_cast<const std::int32_t*>(
                    playerData + PlayerDataVendorClassOffset)
                : -1;
            const auto targetVendorClass = GetUnitClassId(npc);
            std::int32_t vendorIndex{-1};
            auto* vendorEntry = static_cast<std::uint8_t*>(
                GetVendorChainEntry(game, npc, &vendorIndex)
            );
            const auto inventoryFilled = vendorEntry
                && vendorEntry[VendorEntryFilledOffset] != 0;

            if (ShouldArmNormalRefresh(
                    true,
                    requestedMode,
                    currentMode,
                    currentVendorClass,
                    targetVendorClass,
                    vendorEntry != nullptr && vendorIndex >= 0,
                    inventoryFilled
                )) {
                vendorEntry[VendorEntryRefreshPendingOffset] = 1;
                NormalRefreshesArmed.fetch_add(1, std::memory_order_relaxed);
                armed = true;
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            armed = false;
        }

        if (!armed) {
            RejectedNormalRequests.fetch_add(1, std::memory_order_relaxed);
        }
    }

    OriginalConfigureVendorInteraction(game, npc, player, requestedMode);
}

bool InstallUiPatches() noexcept {
    if (!Context->PatchBytes(
            RefreshButtonEnableRva,
            RefreshButtonEnableExpected.data(),
            static_cast<std::uint32_t>(RefreshButtonEnableExpected.size()),
            RefreshButtonEnableReplacement.data(),
            static_cast<std::uint32_t>(RefreshButtonEnableReplacement.size())
        )) {
        Context->LogError("VendorStockRefresh: refresh-button enable signature mismatch.");
        return false;
    }
    if (!Context->PatchBytes(
            RefreshButtonVisibilityRva,
            RefreshButtonVisibilityExpected.data(),
            static_cast<std::uint32_t>(RefreshButtonVisibilityExpected.size()),
            RefreshButtonVisibilityReplacement.data(),
            static_cast<std::uint32_t>(RefreshButtonVisibilityReplacement.size())
        )) {
        Context->LogError("VendorStockRefresh: refresh-button visibility signature mismatch.");
        return false;
    }
    if (!Context->PatchBytes(
            RefreshInputGuardRva,
            RefreshInputGuardExpected.data(),
            static_cast<std::uint32_t>(RefreshInputGuardExpected.size()),
            RefreshInputGuardReplacement.data(),
            static_cast<std::uint32_t>(RefreshInputGuardReplacement.size())
        )) {
        Context->LogError("VendorStockRefresh: refresh-input guard signature mismatch.");
        return false;
    }
    return true;
}

auto Status(D2R::Game::Client*, const D2RL::ConsoleCommandContext* command, void*) noexcept
    -> D2RL::ConsoleCommandResult {
    if (!command || !command->plugin) return D2RL::ConsoleCommandResult::Failed;
    char message[420]{};
    std::snprintf(
        message,
        sizeof(message),
        "VendorStockRefresh 0.1.0: enabled=%s; JSON config=%s; normal requests=%llu; armed=%llu; rejected=%llu.",
        Settings.enabled ? "true" : "false",
        LoadedConfigPath.c_str(),
        static_cast<unsigned long long>(NormalRequestsSent.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(NormalRefreshesArmed.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(RejectedNormalRequests.load(std::memory_order_relaxed))
    );
    command->plugin->WriteConsoleMessage(message);
    return D2RL::ConsoleCommandResult::Handled;
}
} // namespace

D2RL_PLUGIN_EXPORT auto D2RLoaderGetPluginInfo() noexcept -> const D2RL::PluginInfo* {
    return &Info;
}

D2RL_PLUGIN_EXPORT auto D2RLoaderLoadPlugin(const D2RL::PluginContext* context) noexcept -> bool {
    if (!context) return false;
    Context = context;
    Base = reinterpret_cast<std::uint8_t*>(GetModuleHandleW(nullptr));
    NormalRequestsSent.store(0, std::memory_order_relaxed);
    NormalRefreshesArmed.store(0, std::memory_order_relaxed);
    RejectedNormalRequests.store(0, std::memory_order_relaxed);

    if (!Base || !LoadConfig()) return false;
    if (context->modDataVersionBuild != 0 && context->modDataVersionBuild != SupportedBuild) {
        context->LogError("VendorStockRefresh: only D2R build 92777 is supported.");
        return false;
    }

    IsGambling = At<IsGamblingFn>(IsGamblingRva);
    SendNineBytePacket = At<SendNineBytePacketFn>(SendNineBytePacketRva);
    GetPlayerData = At<GetPlayerDataFn>(GetPlayerDataRva);
    GetUnitClassId = At<GetUnitClassIdFn>(GetUnitClassIdRva);
    GetVendorChainEntry = At<GetVendorChainEntryFn>(GetVendorChainEntryRva);

    if (Settings.enabled) {
        if (!ValidateRuntime()) {
            context->LogError("VendorStockRefresh: 92777 helper signature mismatch; plugin refused.");
            return false;
        }
        if (!context->InstallInlineHook(
                SendVendorRefreshRva,
                SendVendorRefreshExpected.data(),
                static_cast<std::uint32_t>(SendVendorRefreshExpected.size()),
                HookSendVendorRefresh,
                &OriginalSendVendorRefresh
            )) {
            context->LogError("VendorStockRefresh: client refresh-sender hook failed.");
            return false;
        }
        if (!context->InstallInlineHook(
                ConfigureVendorInteractionRva,
                ConfigureVendorInteractionExpected.data(),
                static_cast<std::uint32_t>(ConfigureVendorInteractionExpected.size()),
                HookConfigureVendorInteraction,
                &OriginalConfigureVendorInteraction
            )) {
            context->LogError("VendorStockRefresh: server vendor-session hook failed.");
            return false;
        }
        if (!InstallUiPatches()) return false;
    }

    if (!context->RegisterConsoleCommand(
            "vendor-stock-refresh",
            Status,
            "Show vendor stock refresh status and counters."
        )) {
        context->LogWarn("VendorStockRefresh: status command could not be registered.");
    }

    const auto state = Settings.enabled ? "active" : "disabled";
    const auto message = std::string("VendorStockRefresh 0.1.0 ") + state
        + " for D2R 3.2.92777; native normal-vendor refresh uses opcode 0x38"
        + " (JSON config: " + LoadedConfigPath + ").";
    context->LogInfo(message.c_str());
    return true;
}

D2RL_PLUGIN_EXPORT void D2RLoaderUnloadPlugin() noexcept {
    Context = nullptr;
}
