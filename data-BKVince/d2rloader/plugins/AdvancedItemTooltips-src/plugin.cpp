#define NOMINMAX
#include <D2RLPlugin/api.h>
#include "socket_tooltip.hpp"

#include <Windows.h>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace {
constexpr std::uint32_t SupportedBuild = 92777;
constexpr std::uintptr_t GetMaxSocketsRva = 0x36EAD0;
constexpr std::uintptr_t GetUnitStatRva = 0x2F5020;
constexpr std::int32_t SocketsStat = 194;

using GetMaxSocketsFn = std::uint8_t(__fastcall*)(void*) noexcept;
using GetUnitStatFn = std::int32_t(__fastcall*)(void*, std::int32_t, std::uint16_t) noexcept;

const D2RL::PluginContext* Context{};
std::uint8_t* Base{};
GetMaxSocketsFn GetMaxSockets{};
GetUnitStatFn GetUnitStat{};

constexpr D2RL::PluginInfo Info{
    .infoSize = D2RL::PluginInfoSize,
    .apiVersion = D2RL_PLUGIN_API_VERSION,
    .id = "advanced-item-tooltips",
    .name = "Advanced Item Tooltips",
    .version = "1.4.0",
    .author = "RuffnecKk",
    .description = "Shows maximum sockets below weapon damage or armor defense.",
    .flags = D2RL::PluginFlags::None,
};

template<class T>
T At(std::uintptr_t rva) noexcept {
    return reinterpret_cast<T>(Base + rva);
}

auto Status(
    D2R::Game::Client*,
    const D2RL::ConsoleCommandContext* command,
    void*
) noexcept -> D2RL::ConsoleCommandResult {
    if (!command || !command->plugin) return D2RL::ConsoleCommandResult::Failed;
    command->plugin->WriteConsoleMessage(
        "AdvancedItemTooltips 1.4.0: final-tooltip integration is available."
    );
    return D2RL::ConsoleCommandResult::Handled;
}
} // namespace

extern "C" __declspec(dllexport) std::size_t __cdecl AdvancedItemTooltipsBuildSocketLine(
    void* item,
    char* output,
    std::size_t outputCapacity
) noexcept {
    if (!item || !output || outputCapacity == 0 || !GetMaxSockets) return 0;
    try {
        const auto maximumSockets = static_cast<unsigned>(GetMaxSockets(item));
        const auto currentSockets = GetUnitStat
            ? GetUnitStat(item, SocketsStat, 0)
            : 0;
        const auto line = tcp::tooltips::FormatMaxSocketsLine(maximumSockets, currentSockets);
        if (line.empty() || line.size() + 1 > outputCapacity) return 0;
        std::memcpy(output, line.data(), line.size());
        output[line.size()] = '\0';
        return line.size();
    } catch (...) {
        return 0;
    }
}

extern "C" __declspec(dllexport) std::size_t __cdecl AdvancedItemTooltipsFindSocketLineInsertion(
    const char* tooltip,
    std::size_t tooltipLength
) noexcept {
    if (!tooltip || tooltipLength == 0 || tooltipLength > 16 * 1024) {
        return tcp::tooltips::NoSocketLineInsertion;
    }
    try {
        return tcp::tooltips::FindMaxSocketsInsertion(
            std::string_view(tooltip, tooltipLength)
        );
    } catch (...) {
        return tcp::tooltips::NoSocketLineInsertion;
    }
}

D2RL_PLUGIN_EXPORT auto D2RLoaderGetPluginInfo() noexcept -> const D2RL::PluginInfo* {
    return &Info;
}

D2RL_PLUGIN_EXPORT auto D2RLoaderLoadPlugin(const D2RL::PluginContext* context) noexcept -> bool {
    if (!context) return false;
    Context = context;
    Base = reinterpret_cast<std::uint8_t*>(GetModuleHandleW(nullptr));
    if (!Base) return false;
    if (context->modDataVersionBuild != 0 && context->modDataVersionBuild != SupportedBuild) {
        context->LogError("AdvancedItemTooltips: only D2R build 92777 is supported.");
        return false;
    }

    GetMaxSockets = At<GetMaxSocketsFn>(GetMaxSocketsRva);
    GetUnitStat = At<GetUnitStatFn>(GetUnitStatRva);
    if (!context->RegisterConsoleCommand(
            "advanced-item-tooltips",
            Status,
            "Show Advanced Item Tooltips integration status."
        )) {
        context->LogWarn("AdvancedItemTooltips: status command could not be registered.");
    }
    context->LogInfo("AdvancedItemTooltips 1.4.0 integration active for D2R 3.2.92777." );
    return true;
}

D2RL_PLUGIN_EXPORT void D2RLoaderUnloadPlugin() noexcept {
    GetMaxSockets = nullptr;
    GetUnitStat = nullptr;
    Context = nullptr;
}
