#pragma once

// The D3D12 interception and ImGui submission path is an adapted derivative
// of locbones/D2RHUD-2.4 at b9373f8508282948ceb3e2b56f892d9eba475744.
// locbones authorized its use, modification, and redistribution on
// 2026-08-16. No upstream license is claimed or applied by this notice.

#include <Windows.h>

#include "overlay_host_api.hpp"

#include <cstdint>

namespace RuffnecKk::MapSense {

// Tab is owned exclusively by D2R's native automap. This policy is evaluated
// by the actual D2R window subclass before the ImGui Win32 backend sees a
// message. Shift/Ctrl do not change wParam for Tab, so the same rule covers
// every modifier combination. A false result must still be forwarded to D2R
// through DefSubclassProc.
[[nodiscard]] constexpr auto ShouldForwardWin32MessageToImGui(
        UINT message,
        WPARAM wParam) noexcept -> bool {
    switch (message) {
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
            return wParam != static_cast<WPARAM>(VK_TAB);
        case WM_CHAR:
        case WM_SYSCHAR:
            return wParam != static_cast<WPARAM>(L'\t');
        default:
            return true;
    }
}

// Tab toggles the native automap and Escape transfers the screen to or from
// D2R's pause menu. Either initial key-down must discard MapSense-owned
// automap pixels before D2R processes the transition. Bit 30 is the Win32
// previous-key-state bit, so holding either key cannot repeatedly invalidate.
[[nodiscard]] constexpr auto IsInitialOwnedOverlayDismissalMessage(
        UINT message,
        WPARAM wParam,
        LPARAM lParam) noexcept -> bool {
    constexpr auto PreviousKeyStateMask = std::uint64_t{1} << 30U;
    return (message == WM_KEYDOWN || message == WM_SYSKEYDOWN)
        && (wParam == static_cast<WPARAM>(VK_TAB)
            || wParam == static_cast<WPARAM>(VK_ESCAPE))
        && (static_cast<std::uint64_t>(lParam) & PreviousKeyStateMask) == 0U;
}

// A registered client keeps the CPU-side ImGui frame alive, but an empty
// frame must never reach D2R's GPU queue. This keeps the shared host dormant
// between visible MapSense/Floating Damage draws.
[[nodiscard]] constexpr auto ShouldSubmitD3D12DrawData(
        int commandListCount,
        int vertexCount) noexcept -> bool {
    return commandListCount > 0 && vertexCount > 0;
}

// Client-space bounds of the one interactive MapSense surface drawn during
// the current ImGui frame. The host publishes these through atomics for the
// D2R window subclass; no separate overlay HWND is created.
struct D3D12ImGuiPanelBounds {
    bool visible{};
    float left{};
    float top{};
    float right{};
    float bottom{};
};

// The callback runs on D2R's Present thread between ImGui::NewFrame() and
// ImGui::Render(). It may change *open (for example, from a close button) and
// must return the complete interactive bounds of the rendered surface.
using D3D12ImGuiDrawPanelCallback = D3D12ImGuiPanelBounds (*)(
    bool* open,
    void* userData) noexcept;
// Draws MapSense-owned, non-interactive additions into the current ImGui
// frame. Unlike drawPanel, this callback never publishes input bounds.
using D3D12ImGuiDrawOwnedOverlayCallback = void (*)(
    void* userData) noexcept;
// Returns true when MapSense has an owned visual ready and Present must submit
// a frame even while the settings panel is closed and no external client is
// registered.
using D3D12ImGuiWantsOwnedOverlayCallback = bool (*)(
    void* userData) noexcept;
using D3D12ImGuiOwnedOverlayDismissalCallback = void (*)(
    void* userData) noexcept;
using D3D12ImGuiLogCallback = void (*)(const char* message) noexcept;
using D3D12ImGuiUiTaskCallback = void (*)(void* taskUserData) noexcept;
using D3D12ImGuiQueueUiTaskCallback = bool (*)(
    D3D12ImGuiUiTaskCallback task,
    void* taskUserData,
    void* dispatcherUserData) noexcept;

struct D3D12ImGuiHostCallbacks {
    D3D12ImGuiDrawPanelCallback drawPanel{};
    D3D12ImGuiDrawOwnedOverlayCallback drawOwnedOverlay{};
    D3D12ImGuiWantsOwnedOverlayCallback wantsOwnedOverlay{};
    D3D12ImGuiOwnedOverlayDismissalCallback ownedOverlayDismissal{};
    void* userData{};
    // The dispatcher must enqueue task(taskUserData) through D2RLoader's
    // runOnUiThread service. It must not call SetWindowSubclass itself.
    D3D12ImGuiQueueUiTaskCallback queueUiTask{};
    void* uiDispatcherUserData{};
    D3D12ImGuiLogCallback info{};
    D3D12ImGuiLogCallback warning{};
};

struct D3D12ImGuiHostStatus {
    std::uint64_t presentCalls{};
    // Retained field name for internal diagnostic compatibility. Each count
    // now represents an exact CreateSwapChain binding, never a queue guess.
    std::uint64_t directQueueCaptures{};
    std::uint64_t rendererInitAttempts{};
    std::uint64_t rendererInitFailures{};
    std::uint64_t renderedFrames{};
    std::uint32_t lastInitFailureStage{};
    bool configured{};
    bool hooksInstalled{};
    bool commandQueueReady{};
    bool rendererInitialized{};
    bool inputSubclassInstalled{};
    bool menuOpen{};
    HWND gameWindow{};
};

// Stores the callbacks and attempts to install the process-wide D3D12 hooks.
// It is safe to call this repeatedly from a short-lived plugin retry worker;
// rendering itself always occurs synchronously inside D2R's Present call.
[[nodiscard]] auto InitializeD3D12ImGuiHost(
    D3D12ImGuiHostCallbacks callbacks) noexcept -> bool;

// Retries only hook installation after InitializeD3D12ImGuiHost has supplied
// the callbacks. This never starts a render thread and never creates an HWND.
[[nodiscard]] auto TryInstallD3D12ImGuiHooks() noexcept -> bool;

// Removes the input subclass, waits for submitted MapSense GPU work, releases
// the ImGui/D3D12 resources, and removes only the hooks owned by this host.
void ShutdownD3D12ImGuiHost() noexcept;

// Full host-owner shutdown. Registered clients first receive
// contextDestroying while the ImGui context is valid. hostStopped is invoked
// only after the renderer resources and all owned hooks are gone, allowing an
// autonomous client to claim the D3D12 methods without racing MapSense.
void ShutdownD3D12ImGuiHostAndNotifyClients() noexcept;

void ToggleD3D12ImGuiMenu() noexcept;
void SetD3D12ImGuiMenuOpen(bool open) noexcept;
[[nodiscard]] auto IsD3D12ImGuiMenuOpen() noexcept -> bool;
[[nodiscard]] auto GetD3D12ImGuiHostStatus() noexcept
    -> D3D12ImGuiHostStatus;

// Versioned optional inter-DLL contract. Registration is synchronous with
// Present so a client may safely unregister before its DLL unloads.
[[nodiscard]] auto RegisterD3D12ImGuiClientV2(
    const RuffnecKk::OverlayHost::ClientV2* client) noexcept -> bool;
[[nodiscard]] auto UnregisterD3D12ImGuiClientV2(
    const char* owner) noexcept -> bool;
void ClearD3D12ImGuiClientsV2() noexcept;

} // namespace RuffnecKk::MapSense
