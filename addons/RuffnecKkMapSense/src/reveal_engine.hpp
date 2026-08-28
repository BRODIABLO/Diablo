#pragma once

#include <cstdint>

namespace D2RL {
struct PluginContext;
}

namespace RuffnecKk::MapSense {

enum class RevealOutcome : std::uint32_t {
    Complete,
    Accepted,
    Unavailable,
    Armed,
    Disarmed,
};

struct RevealCounters {
    std::uint64_t levels{};
    std::uint64_t rooms{};
    std::uint64_t actRequests{};
    std::uint64_t rejectedActRequests{};
    std::uint64_t failures{};
    std::uint64_t traversalLimits{};
};

// Borrowed native view resolved from the client DRLG captured by the existing
// InitLevel hook. Pointers are valid only during the synchronous UI-thread
// operation that requested the view; callers must never retain them.
struct ClientLevelView final {
    std::uint8_t dataContext{};
    std::int32_t levelId{};
    void* activeRoom{};
    void* drlg{};
    void* level{};
};

auto InitializeRevealEngine(
    const D2RL::PluginContext* context,
    bool diagnostics) noexcept -> bool;
void ShutdownRevealEngine() noexcept;
void BeginRevealSession() noexcept;
void ResetRevealSession() noexcept;

auto RevealCurrentZone() noexcept -> RevealOutcome;
auto RevealCurrentAct() noexcept -> RevealOutcome;
auto ToggleRevealAll() noexcept -> RevealOutcome;
auto DisableRevealAll() noexcept -> RevealOutcome;

// Navigation and Reveal must inspect the same client-side generated map.
// These helpers centralize the governed DRLG_GetLevel/DRLG_InitLevel and
// DRLGROOM_CreateActiveRoom calls already owned and fingerprinted here.
[[nodiscard]] auto ResolveCurrentClientLevelView(
    ClientLevelView& output) noexcept -> bool;
[[nodiscard]] auto ResolveClientLevelById(
    const ClientLevelView& current,
    std::int32_t levelId,
    void*& outputLevel) noexcept -> bool;
[[nodiscard]] auto MaterializeClientRoom(
    std::uint8_t dataContext,
    void* drlgRoom) noexcept -> void*;

auto IsRevealEngineActive() noexcept -> bool;
auto IsRevealAllArmed() noexcept -> bool;
auto GetRevealCounters() noexcept -> RevealCounters;

} // namespace RuffnecKk::MapSense
