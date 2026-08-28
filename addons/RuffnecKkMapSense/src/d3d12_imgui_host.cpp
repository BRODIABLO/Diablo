#include "d3d12_imgui_host.hpp"

// The D3D12 interception and ImGui submission path is an adapted derivative
// of locbones/D2RHUD-2.4 at b9373f8508282948ceb3e2b56f892d9eba475744,
// used, modified, and redistributed with permission obtained 2026-08-16.

#include <MinHook.h>
#include <CommCtrl.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>
#include <windowsx.h>
#include <wrl/client.h>

#include <array>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <utility>
#include <vector>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND window,
    UINT message,
    WPARAM wParam,
    LPARAM lParam);

namespace RuffnecKk::MapSense {
namespace {
using Microsoft::WRL::ComPtr;

static_assert(
    IMGUI_VERSION_NUM
        == static_cast<int>(RuffnecKk::OverlayHost::ImGuiVersionNumber),
    "MapSense and overlay clients must use the governed ImGui commit.");

struct FrameContext {
    ComPtr<ID3D12CommandAllocator> allocator;
    ComPtr<ID3D12Resource> renderTarget;
    D3D12_CPU_DESCRIPTOR_HANDLE descriptor{};
    std::uint64_t fenceValue{};
};

using PresentFn = HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain3*, UINT, UINT);
using ResizeBuffersFn = HRESULT(STDMETHODCALLTYPE*)(
    IDXGISwapChain3*, UINT, UINT, UINT, DXGI_FORMAT, UINT);
using CreateSwapChainFn = HRESULT(STDMETHODCALLTYPE*)(
    IDXGIFactory*, IUnknown*, DXGI_SWAP_CHAIN_DESC*, IDXGISwapChain**);
using CreateSwapChainForHwndFn = HRESULT(STDMETHODCALLTYPE*)(
    IDXGIFactory2*,
    IUnknown*,
    HWND,
    const DXGI_SWAP_CHAIN_DESC1*,
    const DXGI_SWAP_CHAIN_FULLSCREEN_DESC*,
    IDXGIOutput*,
    IDXGISwapChain1**);
using CreateSwapChainForCoreWindowFn = HRESULT(STDMETHODCALLTYPE*)(
    IDXGIFactory2*,
    IUnknown*,
    IUnknown*,
    const DXGI_SWAP_CHAIN_DESC1*,
    IDXGIOutput*,
    IDXGISwapChain1**);
using CreateSwapChainForCompositionFn = HRESULT(STDMETHODCALLTYPE*)(
    IDXGIFactory2*,
    IUnknown*,
    const DXGI_SWAP_CHAIN_DESC1*,
    IDXGIOutput*,
    IDXGISwapChain1**);
using D3D12CreateDeviceFn = HRESULT(WINAPI*)(
    IUnknown*, D3D_FEATURE_LEVEL, REFIID, void**);

constexpr std::size_t MethodCount = 150;
constexpr std::size_t PresentMethod = 140;
constexpr std::size_t ResizeBuffersMethod = 145;
constexpr std::size_t FactoryMethodCount = 25;
constexpr std::size_t CreateSwapChainMethod = 10;
constexpr std::size_t CreateSwapChainForHwndMethod = 15;
constexpr std::size_t CreateSwapChainForCoreWindowMethod = 16;
constexpr std::size_t CreateSwapChainForCompositionMethod = 24;
constexpr DWORD FenceWaitMilliseconds = 5'000;
constexpr std::size_t MaximumExternalClients = 8;
constexpr std::size_t MaximumSwapChainQueueBindings = 16;

std::array<void*, MethodCount> Methods{};
std::array<void*, FactoryMethodCount> FactoryMethods{};
PresentFn OriginalPresent{};
ResizeBuffersFn OriginalResizeBuffers{};
CreateSwapChainFn OriginalCreateSwapChain{};
CreateSwapChainForHwndFn OriginalCreateSwapChainForHwnd{};
CreateSwapChainForCoreWindowFn OriginalCreateSwapChainForCoreWindow{};
CreateSwapChainForCompositionFn OriginalCreateSwapChainForComposition{};

// A recursive mutex prevents an unexpected synchronous window message (for
// example SetCapture/ReleaseCapture) from deadlocking the Present thread while
// still serializing ImGui backend access with the D2R window subclass.
std::recursive_mutex HostMutex;
D3D12ImGuiHostCallbacks Callbacks{};
bool Configured{};
bool HooksInstalled{};
bool FactoryHooksInstalled{};
bool MinHookReady{};
bool MinHookInitializedByHost{};
bool RendererInitialized{};
bool ImGuiContextCreated{};
bool ImGuiWin32Initialized{};
bool ImGuiDx12Initialized{};
bool ContextCallbacksActive{};
bool RendererResetPending{};
bool RendererPoisoned{};
HWND GameWindow{};
IDXGISwapChain3* ActiveSwapChain{};
DXGI_FORMAT BackBufferFormat{DXGI_FORMAT_R8G8B8A8_UNORM};

struct SwapChainQueueBinding {
    ComPtr<IUnknown> swapChainIdentity;
    ComPtr<ID3D12CommandQueue> commandQueue;
};

std::vector<SwapChainQueueBinding> SwapChainQueueBindings{};

// Process-lifetime storage avoids releasing D3D12 references during static
// destruction after D2R has already unloaded D3D12Core. Explicit shutdown and
// ResizeBuffers still release every live reference deterministically.
struct RendererStorage {
    ComPtr<ID3D12CommandQueue> commandQueue;
    ComPtr<ID3D12GraphicsCommandList> commandList;
    ComPtr<ID3D12DescriptorHeap> rtvHeap;
    ComPtr<ID3D12DescriptorHeap> srvHeap;
    ComPtr<ID3D12Fence> fence;
    HANDLE fenceEvent{};
    std::vector<FrameContext> frames;
};

RendererStorage* const ProcessRendererStorage = new RendererStorage{};
auto& CommandQueue = ProcessRendererStorage->commandQueue;
auto& CommandList = ProcessRendererStorage->commandList;
auto& RtvHeap = ProcessRendererStorage->rtvHeap;
auto& SrvHeap = ProcessRendererStorage->srvHeap;
auto& Fence = ProcessRendererStorage->fence;
auto& FenceEvent = ProcessRendererStorage->fenceEvent;
auto& Frames = ProcessRendererStorage->frames;
std::uint64_t NextFenceValue{1};
std::chrono::steady_clock::time_point LastFrameTime{};

std::atomic<ID3D12CommandQueue*> CapturedQueue{};
std::atomic<bool> UnboundSwapChainWarningLogged{};
std::atomic<bool> DeviceRemovalWarningLogged{};
std::atomic<bool> MenuOpen{};
std::atomic<bool> AwaitingFirstBounds{};
std::atomic<std::uint32_t> OwnedMouseButtons{};
std::atomic<std::uint32_t> CanceledMouseButtons{};
std::atomic<HWND> PublishedGameWindow{};
std::atomic<HWND> InputSubclassWindow{};
std::atomic<HWND> RequestedSubclassWindow{};
std::atomic<bool> InputSubclassInstalled{};
std::atomic<bool> InputInstallQueued{};
std::atomic<bool> InputQueueWarningLogged{};
std::atomic<std::uint32_t> ActiveHookCalls{};
std::atomic<bool> ClientsAccepting{true};

// BoundsVersion is a small seqlock. Present is the only writer; the window
// subclass may read from another thread without observing a torn rectangle.
std::atomic<std::uint64_t> BoundsVersion{};
std::atomic<bool> BoundsVisible{};
std::atomic<float> BoundsLeft{};
std::atomic<float> BoundsTop{};
std::atomic<float> BoundsRight{};
std::atomic<float> BoundsBottom{};

std::atomic<std::uint64_t> PresentCalls{};
std::atomic<std::uint64_t> ExactQueueBindings{};
std::atomic<std::uint64_t> RendererInitAttempts{};
std::atomic<std::uint64_t> RendererInitFailures{};
std::atomic<std::uint64_t> RenderedFrames{};
std::atomic<std::uint32_t> LastInitFailureStage{};
std::atomic<bool> HooksInstalledPublished{};
std::atomic<bool> RendererInitializedPublished{};
std::atomic<bool> InputSubclassInstalledPublished{};
std::atomic<bool> ConfiguredPublished{};
std::atomic<D3D12ImGuiLogCallback> InfoLogger{};
std::atomic<D3D12ImGuiLogCallback> WarningLogger{};

struct ExternalClientEntry {
    std::array<char, 64> owner{};
    RuffnecKk::OverlayHost::ContextCallbackV2 contextCreated{};
    RuffnecKk::OverlayHost::ContextCallbackV2 contextDestroying{};
    RuffnecKk::OverlayHost::HostStoppedCallbackV2 hostStopped{};
    RuffnecKk::OverlayHost::BeforeFrameCallbackV2 beforeFrame{};
    RuffnecKk::OverlayHost::RenderCallbackV2 render{};
    void* userData{};
};

std::mutex ExternalClientMutex;
std::array<ExternalClientEntry, MaximumExternalClients> ExternalClients{};

int SubclassIdentity{};

class HookCallGuard final {
public:
    HookCallGuard() noexcept {
        ActiveHookCalls.fetch_add(1U, std::memory_order_acq_rel);
    }

    ~HookCallGuard() {
        ActiveHookCalls.fetch_sub(1U, std::memory_order_acq_rel);
    }

    HookCallGuard(const HookCallGuard&) = delete;
    auto operator=(const HookCallGuard&) -> HookCallGuard& = delete;
};

constexpr std::uint32_t LeftButtonBit = 1U << 0U;
constexpr std::uint32_t RightButtonBit = 1U << 1U;
constexpr std::uint32_t MiddleButtonBit = 1U << 2U;
constexpr std::uint32_t XButton1Bit = 1U << 3U;
constexpr std::uint32_t XButton2Bit = 1U << 4U;

void PublishPanelBounds(const D3D12ImGuiPanelBounds& bounds) noexcept {
    BoundsVersion.fetch_add(1, std::memory_order_acq_rel);
    BoundsVisible.store(bounds.visible, std::memory_order_relaxed);
    BoundsLeft.store(bounds.left, std::memory_order_relaxed);
    BoundsTop.store(bounds.top, std::memory_order_relaxed);
    BoundsRight.store(bounds.right, std::memory_order_relaxed);
    BoundsBottom.store(bounds.bottom, std::memory_order_relaxed);
    BoundsVersion.fetch_add(1, std::memory_order_release);
}

auto CopyExternalClients() noexcept
    -> std::array<ExternalClientEntry, MaximumExternalClients> {
    std::scoped_lock lock(ExternalClientMutex);
    return ExternalClients;
}

auto HasExternalFrameClient(
    const std::array<ExternalClientEntry, MaximumExternalClients>& clients)
        noexcept -> bool {
    for (const auto& client : clients) {
        if (client.render != nullptr || client.beforeFrame != nullptr)
            return true;
    }
    return false;
}

auto MakeExternalFrameContextLocked() noexcept
    -> RuffnecKk::OverlayHost::FrameContextV2 {
    RECT clientRect{};
    if (GameWindow != nullptr) GetClientRect(GameWindow, &clientRect);
    return {
        .structSize = RuffnecKk::OverlayHost::FrameContextV2Size,
        .version = RuffnecKk::OverlayHost::ApiVersion2,
        .imguiContext = ImGuiContextCreated
            ? ImGui::GetCurrentContext()
            : nullptr,
        .window = GameWindow,
        .displayWidth = static_cast<float>(clientRect.right - clientRect.left),
        .displayHeight = static_cast<float>(clientRect.bottom - clientRect.top),
    };
}

void NotifyContextCreatedLocked(
    const std::array<ExternalClientEntry, MaximumExternalClients>& clients)
        noexcept {
    const auto frame = MakeExternalFrameContextLocked();
    for (const auto& client : clients) {
        if (client.contextCreated != nullptr)
            client.contextCreated(&frame, client.userData);
    }
    ContextCallbacksActive = true;
}

void NotifyContextDestroyingLocked(
    const std::array<ExternalClientEntry, MaximumExternalClients>& clients)
        noexcept {
    if (!ContextCallbacksActive || !ImGuiContextCreated) return;
    ContextCallbacksActive = false;
    const auto frame = MakeExternalFrameContextLocked();
    for (const auto& client : clients) {
        if (client.contextDestroying != nullptr)
            client.contextDestroying(&frame, client.userData);
    }
}

auto ReadPanelBounds() noexcept -> D3D12ImGuiPanelBounds {
    D3D12ImGuiPanelBounds bounds{};
    for (;;) {
        const std::uint64_t before = BoundsVersion.load(
            std::memory_order_acquire);
        if ((before & 1U) != 0U) continue;
        bounds.visible = BoundsVisible.load(std::memory_order_relaxed);
        bounds.left = BoundsLeft.load(std::memory_order_relaxed);
        bounds.top = BoundsTop.load(std::memory_order_relaxed);
        bounds.right = BoundsRight.load(std::memory_order_relaxed);
        bounds.bottom = BoundsBottom.load(std::memory_order_relaxed);
        const std::uint64_t after = BoundsVersion.load(
            std::memory_order_acquire);
        if (before == after) return bounds;
    }
}

void LogInfo(const char* message) noexcept {
    if (const auto logger = InfoLogger.load(std::memory_order_acquire))
        logger(message);
}

void LogWarning(const char* message) noexcept {
    if (const auto logger = WarningLogger.load(std::memory_order_acquire))
        logger(message);
}

auto SubclassId() noexcept -> UINT_PTR {
    return reinterpret_cast<UINT_PTR>(&SubclassIdentity);
}

auto MousePointFromMessage(
    HWND window,
    UINT message,
    LPARAM lParam) noexcept -> POINT {
    POINT point{};
    if (message == WM_MOUSEWHEEL || message == WM_MOUSEHWHEEL) {
        point.x = GET_X_LPARAM(lParam);
        point.y = GET_Y_LPARAM(lParam);
        ScreenToClient(window, &point);
        return point;
    }
    if (message >= WM_MOUSEFIRST && message <= WM_MOUSELAST) {
        point.x = GET_X_LPARAM(lParam);
        point.y = GET_Y_LPARAM(lParam);
        return point;
    }
    if (GetCursorPos(&point)) ScreenToClient(window, &point);
    return point;
}

auto PointInsidePublishedPanel(POINT point) noexcept -> bool {
    if (!MenuOpen.load(std::memory_order_acquire)) return false;
    if (AwaitingFirstBounds.load(std::memory_order_acquire)) return true;
    const auto bounds = ReadPanelBounds();
    return bounds.visible
        && static_cast<float>(point.x) >= bounds.left
        && static_cast<float>(point.x) < bounds.right
        && static_cast<float>(point.y) >= bounds.top
        && static_cast<float>(point.y) < bounds.bottom;
}

auto ButtonBitFromMessage(UINT message, WPARAM wParam) noexcept
    -> std::uint32_t {
    switch (message) {
    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
    case WM_LBUTTONUP:
        return LeftButtonBit;
    case WM_RBUTTONDOWN:
    case WM_RBUTTONDBLCLK:
    case WM_RBUTTONUP:
        return RightButtonBit;
    case WM_MBUTTONDOWN:
    case WM_MBUTTONDBLCLK:
    case WM_MBUTTONUP:
        return MiddleButtonBit;
    case WM_XBUTTONDOWN:
    case WM_XBUTTONDBLCLK:
    case WM_XBUTTONUP:
        return GET_XBUTTON_WPARAM(wParam) == XBUTTON1
            ? XButton1Bit
            : XButton2Bit;
    default:
        return 0U;
    }
}

auto IsButtonDownMessage(UINT message) noexcept -> bool {
    switch (message) {
    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONDBLCLK:
    case WM_XBUTTONDOWN:
    case WM_XBUTTONDBLCLK:
        return true;
    default:
        return false;
    }
}

auto IsButtonUpMessage(UINT message) noexcept -> bool {
    switch (message) {
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
    case WM_XBUTTONUP:
        return true;
    default:
        return false;
    }
}

void FeedImGuiWin32Message(
    HWND window,
    UINT message,
    WPARAM wParam,
    LPARAM lParam) noexcept {
    // Defense in depth: the window subclass normally rejects native automap
    // Tab messages before reaching this helper. Keep the backend boundary
    // closed as well so a later input branch cannot accidentally regress it.
    if (!ShouldForwardWin32MessageToImGui(message, wParam)) return;
    std::scoped_lock lock(HostMutex);
    if (!RendererInitialized || !ImGuiContextCreated) return;
    ImGui_ImplWin32_WndProcHandler(window, message, wParam, lParam);
}

auto RemoveInputSubclassMessage() noexcept -> UINT {
    static const UINT message = RegisterWindowMessageW(
        L"RuffnecKk.MapSense.RemoveInputSubclass.v1");
    return message;
}

auto ConsumedButtonResult(UINT message) noexcept -> LRESULT {
    return message == WM_XBUTTONDOWN
            || message == WM_XBUTTONDBLCLK
            || message == WM_XBUTTONUP
        ? TRUE
        : 0;
}

auto CancelOwnedMouseButtons() noexcept -> bool {
    const std::uint32_t owned = OwnedMouseButtons.exchange(
        0U, std::memory_order_acq_rel);
    if (owned == 0U) return false;
    CanceledMouseButtons.fetch_or(owned, std::memory_order_acq_rel);

    std::scoped_lock lock(HostMutex);
    if (ImGuiContextCreated) {
        ImGuiIO& io = ImGui::GetIO();
        if ((owned & LeftButtonBit) != 0U) io.AddMouseButtonEvent(0, false);
        if ((owned & RightButtonBit) != 0U) io.AddMouseButtonEvent(1, false);
        if ((owned & MiddleButtonBit) != 0U) io.AddMouseButtonEvent(2, false);
        if ((owned & XButton1Bit) != 0U) io.AddMouseButtonEvent(3, false);
        if ((owned & XButton2Bit) != 0U) io.AddMouseButtonEvent(4, false);
    }
    return true;
}

auto CALLBACK HostWindowSubclassProc(
    HWND window,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    UINT_PTR,
    DWORD_PTR) noexcept -> LRESULT {
    const UINT removeMessage = RemoveInputSubclassMessage();
    if (removeMessage != 0U
        && message == removeMessage
        && wParam == SubclassId()) {
        const bool releasedOwnedButton = CancelOwnedMouseButtons();
        if (releasedOwnedButton && GetCapture() == window) ReleaseCapture();
        const BOOL removed = RemoveWindowSubclass(
            window, HostWindowSubclassProc, SubclassId());
        if (removed) {
            InputSubclassWindow.store(nullptr, std::memory_order_release);
            InputSubclassInstalled.store(false, std::memory_order_release);
            InputSubclassInstalledPublished.store(
                false, std::memory_order_release);
        }
        return removed ? TRUE : FALSE;
    }

    if (IsInitialOwnedOverlayDismissalMessage(message, wParam, lParam)) {
        // Present owns the same recursive mutex for its complete ImGui frame.
        // Waiting here guarantees the invalidation finishes after any frame
        // already being submitted and before D2R processes Tab or Escape.
        std::scoped_lock lock(HostMutex);
        if (Callbacks.ownedOverlayDismissal != nullptr) {
            Callbacks.ownedOverlayDismissal(Callbacks.userData);
        }
    }

    if (!ShouldForwardWin32MessageToImGui(message, wParam)) {
        // Never consume Tab and never feed it to ImGui. D2R remains the sole
        // owner of its native automap key, including key-up, system-key and
        // translated character messages.
        LogInfo(
            "MapSense diagnostic input: native Tab message bypassed ImGui and was forwarded to D2R.");
        return DefSubclassProc(window, message, wParam, lParam);
    }

    const std::uint32_t ownedBefore = OwnedMouseButtons.load(
        std::memory_order_acquire);
    const bool hasOwnedButton = ownedBefore != 0U;
    const POINT point = MousePointFromMessage(window, message, lParam);
    const bool insidePanel = PointInsidePublishedPanel(point);

    const std::uint32_t buttonBit = ButtonBitFromMessage(message, wParam);
    const std::uint32_t canceledBefore = CanceledMouseButtons.load(
        std::memory_order_acquire);
    if (buttonBit != 0U && (canceledBefore & buttonBit) != 0U) {
        if (IsButtonUpMessage(message)) {
            CanceledMouseButtons.fetch_and(
                ~buttonBit, std::memory_order_acq_rel);
            return ConsumedButtonResult(message);
        }
        // A new down means the canceled press was released while D2R was not
        // delivering messages (for example across GameLeft). Do not discard
        // the first legitimate click of the next session.
        if (IsButtonDownMessage(message)) {
            CanceledMouseButtons.fetch_and(
                ~buttonBit, std::memory_order_acq_rel);
        }
    }

    if (buttonBit != 0U && IsButtonDownMessage(message)) {
        if (insidePanel || hasOwnedButton) {
            OwnedMouseButtons.fetch_or(buttonBit, std::memory_order_acq_rel);
            FeedImGuiWin32Message(window, message, wParam, lParam);
            SetCapture(window);
            return ConsumedButtonResult(message);
        }
        return DefSubclassProc(window, message, wParam, lParam);
    }

    if (buttonBit != 0U && IsButtonUpMessage(message)) {
        if ((ownedBefore & buttonBit) != 0U) {
            FeedImGuiWin32Message(window, message, wParam, lParam);
            const std::uint32_t remaining = OwnedMouseButtons.fetch_and(
                ~buttonBit, std::memory_order_acq_rel) & ~buttonBit;
            if (remaining == 0U && GetCapture() == window) ReleaseCapture();
            return ConsumedButtonResult(message);
        }
        return DefSubclassProc(window, message, wParam, lParam);
    }

    switch (message) {
    case WM_MOUSEMOVE:
        if (MenuOpen.load(std::memory_order_acquire) || hasOwnedButton)
            FeedImGuiWin32Message(window, message, wParam, lParam);
        if (insidePanel || hasOwnedButton) return 0;
        break;
    case WM_MOUSEWHEEL:
    case WM_MOUSEHWHEEL:
        if (insidePanel || hasOwnedButton) {
            FeedImGuiWin32Message(window, message, wParam, lParam);
            return 0;
        }
        break;
    case WM_INPUT:
        if (insidePanel || hasOwnedButton) {
            RAWINPUTHEADER header{};
            UINT headerSize = sizeof(header);
            const UINT copied = GetRawInputData(
                reinterpret_cast<HRAWINPUT>(lParam),
                RID_HEADER,
                &header,
                &headerSize,
                sizeof(RAWINPUTHEADER));
            if (copied == sizeof(header)
                && header.dwType == RIM_TYPEMOUSE) {
                // Foreground raw input still needs DefWindowProc cleanup, but
                // bypasses D2R and every later subclass while the panel owns
                // the pointer. Raw keyboard packets remain game-owned.
                return DefWindowProcW(window, message, wParam, lParam);
            }
        }
        break;
    case WM_MOUSELEAVE:
        if (MenuOpen.load(std::memory_order_acquire) || hasOwnedButton)
            FeedImGuiWin32Message(window, message, wParam, lParam);
        break;
    case WM_CAPTURECHANGED:
        if (reinterpret_cast<HWND>(lParam) != window)
            CancelOwnedMouseButtons();
        FeedImGuiWin32Message(window, message, wParam, lParam);
        break;
    case WM_KILLFOCUS:
        CancelOwnedMouseButtons();
        FeedImGuiWin32Message(window, message, wParam, lParam);
        break;
    default:
        // Keyboard and non-input messages remain D2R-owned. Feeding them to
        // the backend keeps ImGui modifiers/focus correct without consuming
        // the game's message. Native automap Tab messages already returned
        // above and can never reach this backend.
        if (MenuOpen.load(std::memory_order_acquire))
            FeedImGuiWin32Message(window, message, wParam, lParam);
        break;
    }
    return DefSubclassProc(window, message, wParam, lParam);
}

void InstallInputSubclassOnUiThread(void*) noexcept {
    const HWND window = RequestedSubclassWindow.exchange(
        nullptr, std::memory_order_acq_rel);
    InputInstallQueued.store(false, std::memory_order_release);
    if (window == nullptr || !IsWindow(window)) return;

    DWORD processId{};
    const DWORD ownerThreadId = GetWindowThreadProcessId(window, &processId);
    if (ownerThreadId == 0U
        || ownerThreadId != GetCurrentThreadId()
        || processId != GetCurrentProcessId()) {
        LogWarning(
            "MapSense: refused to subclass D2R from a non-owner UI thread.");
        return;
    }

    std::scoped_lock lock(HostMutex);
    if (!RendererInitialized
        || RendererResetPending
        || GameWindow != window) {
        return;
    }
    if (InputSubclassInstalled.load(std::memory_order_acquire)) {
        if (InputSubclassWindow.load(std::memory_order_acquire) != window) {
            LogWarning(
                "MapSense: refused to replace an input subclass owned by another D2R window.");
        }
        return;
    }
    if (!SetWindowSubclass(
            window, HostWindowSubclassProc, SubclassId(), 0U)) {
        LogWarning("MapSense: D2R input subclass installation failed.");
        return;
    }
    InputSubclassWindow.store(window, std::memory_order_release);
    InputSubclassInstalled.store(true, std::memory_order_release);
    InputSubclassInstalledPublished.store(true, std::memory_order_release);
    AwaitingFirstBounds.store(
        MenuOpen.load(std::memory_order_acquire),
        std::memory_order_release);
    InputQueueWarningLogged.store(false, std::memory_order_release);
}

auto QueueInputSubclassInstallLocked() noexcept -> bool {
    if (GameWindow == nullptr || RendererResetPending) return false;
    if (InputSubclassInstalled.load(std::memory_order_acquire)
        && InputSubclassWindow.load(std::memory_order_acquire)
            == GameWindow) {
        return true;
    }
    if (InputInstallQueued.load(std::memory_order_acquire)) return true;
    if (Callbacks.queueUiTask == nullptr) return false;

    RequestedSubclassWindow.store(GameWindow, std::memory_order_release);
    InputInstallQueued.store(true, std::memory_order_release);
    if (Callbacks.queueUiTask(
            InstallInputSubclassOnUiThread,
            nullptr,
            Callbacks.uiDispatcherUserData)) {
        return true;
    }
    RequestedSubclassWindow.store(nullptr, std::memory_order_release);
    InputInstallQueued.store(false, std::memory_order_release);
    return false;
}

auto RemoveInputSubclassSynchronously() noexcept -> bool {
    RequestedSubclassWindow.store(nullptr, std::memory_order_release);
    InputInstallQueued.store(false, std::memory_order_release);
    const HWND window = InputSubclassWindow.load(std::memory_order_acquire);
    if (!InputSubclassInstalled.load(std::memory_order_acquire)
        || window == nullptr) {
        return true;
    }
    if (!IsWindow(window)) {
        CancelOwnedMouseButtons();
        InputSubclassWindow.store(nullptr, std::memory_order_release);
        InputSubclassInstalled.store(false, std::memory_order_release);
        InputSubclassInstalledPublished.store(
            false, std::memory_order_release);
        return true;
    }

    const UINT message = RemoveInputSubclassMessage();
    DWORD processId{};
    const DWORD ownerThreadId = GetWindowThreadProcessId(window, &processId);
    if (message == 0U
        || ownerThreadId == 0U
        || processId != GetCurrentProcessId()) {
        return false;
    }

    LRESULT result{};
    if (ownerThreadId == GetCurrentThreadId()) {
        result = SendMessageW(window, message, SubclassId(), 0);
    }
    else {
        DWORD_PTR sendResult{};
        if (SendMessageTimeoutW(
                window,
                message,
                SubclassId(),
                0,
                SMTO_ABORTIFHUNG | SMTO_BLOCK,
                FenceWaitMilliseconds,
                &sendResult) == 0) {
            // Unload safety is stricter than responsiveness: once the
            // bounded attempt expires, wait for the owner thread rather than
            // leave a callback pointing into a DLL that may be unloaded.
            result = SendMessageW(window, message, SubclassId(), 0);
        }
        else {
            result = static_cast<LRESULT>(sendResult);
        }
    }
    (void)result;
    if (!InputSubclassInstalled.load(std::memory_order_acquire)) return true;
    if (!IsWindow(window)) {
        CancelOwnedMouseButtons();
        InputSubclassWindow.store(nullptr, std::memory_order_release);
        InputSubclassInstalled.store(false, std::memory_order_release);
        InputSubclassInstalledPublished.store(
            false, std::memory_order_release);
        return true;
    }
    return false;
}

auto PinHostModuleForResidualSubclass() noexcept -> bool {
    HMODULE module{};
    return GetModuleHandleExW(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS
            | GET_MODULE_HANDLE_EX_FLAG_PIN,
        reinterpret_cast<LPCWSTR>(static_cast<void*>(&SubclassIdentity)),
        &module) != FALSE;
}

auto WaitForFenceValueLocked(std::uint64_t value) noexcept -> bool {
    if (value == 0U || !Fence || !FenceEvent) return true;
    if (Fence->GetCompletedValue() >= value) return true;
    if (FAILED(Fence->SetEventOnCompletion(value, FenceEvent))) return false;
    return WaitForSingleObject(FenceEvent, FenceWaitMilliseconds)
        == WAIT_OBJECT_0;
}

auto WaitForGpuIdleLocked() noexcept -> bool {
    if (!CommandQueue || !Fence || !FenceEvent) return true;
    const std::uint64_t value = NextFenceValue++;
    if (FAILED(CommandQueue->Signal(Fence.Get(), value))) return false;
    return WaitForFenceValueLocked(value);
}

void ResetRendererStateLocked(bool clearInputSubclassState = true) noexcept {
    RendererInitialized = false;
    RendererInitializedPublished.store(false, std::memory_order_release);
    const bool gpuIdle = WaitForGpuIdleLocked();
    if (!gpuIdle)
        LogWarning("MapSense: timed out while waiting for submitted GPU work.");

    NotifyContextDestroyingLocked(CopyExternalClients());
    if (ImGuiDx12Initialized) {
        ImGui_ImplDX12_Shutdown();
        ImGuiDx12Initialized = false;
    }
    if (ImGuiWin32Initialized) {
        ImGui_ImplWin32_Shutdown();
        ImGuiWin32Initialized = false;
    }
    if (ImGuiContextCreated) {
        ImGui::DestroyContext();
        ImGuiContextCreated = false;
    }

    ContextCallbacksActive = false;
    RequestedSubclassWindow.store(nullptr, std::memory_order_release);
    InputInstallQueued.store(false, std::memory_order_release);
    if (clearInputSubclassState) {
        InputSubclassWindow.store(nullptr, std::memory_order_release);
        InputSubclassInstalled.store(false, std::memory_order_release);
        InputSubclassInstalledPublished.store(
            false, std::memory_order_release);
    }
    GameWindow = nullptr;
    PublishedGameWindow.store(nullptr, std::memory_order_release);
    ActiveSwapChain = nullptr;
    Frames.clear();
    CommandList.Reset();
    RtvHeap.Reset();
    SrvHeap.Reset();
    Fence.Reset();
    if (FenceEvent) {
        CloseHandle(FenceEvent);
        FenceEvent = nullptr;
    }
    NextFenceValue = 1U;
    LastFrameTime = {};
    RendererResetPending = false;
    PublishPanelBounds({});
}

auto FailRendererInitialization(
    std::uint32_t stage,
    const char* message) noexcept -> bool {
    RendererInitFailures.fetch_add(1, std::memory_order_relaxed);
    LastInitFailureStage.store(stage, std::memory_order_relaxed);
    LogWarning(message);
    ResetRendererStateLocked();
    return false;
}

auto SameComIdentity(IUnknown* left, IUnknown* right) noexcept -> bool {
    if (left == nullptr || right == nullptr) return false;
    ComPtr<IUnknown> leftIdentity;
    ComPtr<IUnknown> rightIdentity;
    return SUCCEEDED(left->QueryInterface(IID_PPV_ARGS(&leftIdentity)))
        && SUCCEEDED(right->QueryInterface(IID_PPV_ARGS(&rightIdentity)))
        && leftIdentity.Get() == rightIdentity.Get();
}

void RecordExactSwapChainQueue(
    IUnknown* queueCandidate,
    IUnknown* swapChainCandidate) noexcept {
    if (queueCandidate == nullptr || swapChainCandidate == nullptr) return;

    ComPtr<ID3D12CommandQueue> queue;
    ComPtr<IUnknown> swapChainIdentity;
    if (FAILED(queueCandidate->QueryInterface(IID_PPV_ARGS(&queue)))
        || queue->GetDesc().Type != D3D12_COMMAND_LIST_TYPE_DIRECT
        || FAILED(swapChainCandidate->QueryInterface(
            IID_PPV_ARGS(&swapChainIdentity)))) {
        return;
    }

    std::scoped_lock lock(HostMutex);
    for (auto& binding : SwapChainQueueBindings) {
        if (!SameComIdentity(
                binding.swapChainIdentity.Get(), swapChainIdentity.Get())) {
            continue;
        }
        binding.commandQueue = queue;
        return;
    }
    if (SwapChainQueueBindings.size() >= MaximumSwapChainQueueBindings) {
        LogWarning(
            "MapSense: refused a swap-chain command queue because the exact-binding registry is full.");
        return;
    }
    SwapChainQueueBindings.push_back(SwapChainQueueBinding{
        .swapChainIdentity = std::move(swapChainIdentity),
        .commandQueue = std::move(queue),
    });
    ExactQueueBindings.fetch_add(1U, std::memory_order_relaxed);
    LogInfo(
        "MapSense: recorded the exact DirectX 12 command queue supplied at swap-chain creation.");
}

auto SelectExactSwapChainQueueLocked(IDXGISwapChain3* swapChain) noexcept
    -> bool {
    if (swapChain == nullptr || RendererPoisoned) return false;
    if (CommandQueue && ActiveSwapChain == swapChain) return true;

    for (const auto& binding : SwapChainQueueBindings) {
        if (!SameComIdentity(binding.swapChainIdentity.Get(), swapChain))
            continue;
        if (RendererInitialized && CommandQueue
            && !SameComIdentity(CommandQueue.Get(), binding.commandQueue.Get())) {
            RendererPoisoned = true;
            RendererInitializedPublished.store(
                false, std::memory_order_release);
            LogWarning(
                "MapSense: renderer disabled because an initialized swap chain changed command queues.");
            return false;
        }
        CommandQueue = binding.commandQueue;
        CapturedQueue.store(CommandQueue.Get(), std::memory_order_release);
        UnboundSwapChainWarningLogged.store(false, std::memory_order_release);
        return true;
    }

    if (!UnboundSwapChainWarningLogged.exchange(
            true, std::memory_order_acq_rel)) {
        LogWarning(
            "MapSense: GPU rendering is fail-closed because Present has no exact swap-chain command-queue binding.");
    }
    return false;
}

auto STDMETHODCALLTYPE HookCreateSwapChain(
    IDXGIFactory* factory,
    IUnknown* queue,
    DXGI_SWAP_CHAIN_DESC* description,
    IDXGISwapChain** swapChain) noexcept -> HRESULT {
    HookCallGuard hookCall;
    CreateSwapChainFn original{};
    {
        std::scoped_lock lock(HostMutex);
        original = OriginalCreateSwapChain;
    }
    const HRESULT result = original != nullptr
        ? original(factory, queue, description, swapChain)
        : DXGI_ERROR_INVALID_CALL;
    if (SUCCEEDED(result) && swapChain != nullptr && *swapChain != nullptr)
        RecordExactSwapChainQueue(queue, *swapChain);
    return result;
}

auto STDMETHODCALLTYPE HookCreateSwapChainForHwnd(
    IDXGIFactory2* factory,
    IUnknown* queue,
    HWND window,
    const DXGI_SWAP_CHAIN_DESC1* description,
    const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* fullscreenDescription,
    IDXGIOutput* restrictToOutput,
    IDXGISwapChain1** swapChain) noexcept -> HRESULT {
    HookCallGuard hookCall;
    CreateSwapChainForHwndFn original{};
    {
        std::scoped_lock lock(HostMutex);
        original = OriginalCreateSwapChainForHwnd;
    }
    const HRESULT result = original != nullptr
        ? original(
            factory,
            queue,
            window,
            description,
            fullscreenDescription,
            restrictToOutput,
            swapChain)
        : DXGI_ERROR_INVALID_CALL;
    if (SUCCEEDED(result) && swapChain != nullptr && *swapChain != nullptr)
        RecordExactSwapChainQueue(queue, *swapChain);
    return result;
}

auto STDMETHODCALLTYPE HookCreateSwapChainForCoreWindow(
    IDXGIFactory2* factory,
    IUnknown* queue,
    IUnknown* window,
    const DXGI_SWAP_CHAIN_DESC1* description,
    IDXGIOutput* restrictToOutput,
    IDXGISwapChain1** swapChain) noexcept -> HRESULT {
    HookCallGuard hookCall;
    CreateSwapChainForCoreWindowFn original{};
    {
        std::scoped_lock lock(HostMutex);
        original = OriginalCreateSwapChainForCoreWindow;
    }
    const HRESULT result = original != nullptr
        ? original(
            factory,
            queue,
            window,
            description,
            restrictToOutput,
            swapChain)
        : DXGI_ERROR_INVALID_CALL;
    if (SUCCEEDED(result) && swapChain != nullptr && *swapChain != nullptr)
        RecordExactSwapChainQueue(queue, *swapChain);
    return result;
}

auto STDMETHODCALLTYPE HookCreateSwapChainForComposition(
    IDXGIFactory2* factory,
    IUnknown* queue,
    const DXGI_SWAP_CHAIN_DESC1* description,
    IDXGIOutput* restrictToOutput,
    IDXGISwapChain1** swapChain) noexcept -> HRESULT {
    HookCallGuard hookCall;
    CreateSwapChainForCompositionFn original{};
    {
        std::scoped_lock lock(HostMutex);
        original = OriginalCreateSwapChainForComposition;
    }
    const HRESULT result = original != nullptr
        ? original(
            factory,
            queue,
            description,
            restrictToOutput,
            swapChain)
        : DXGI_ERROR_INVALID_CALL;
    if (SUCCEEDED(result) && swapChain != nullptr && *swapChain != nullptr)
        RecordExactSwapChainQueue(queue, *swapChain);
    return result;
}

auto InitializeRenderer(
    IDXGISwapChain3* swapChain,
    const std::array<ExternalClientEntry, MaximumExternalClients>&
        externalClients) noexcept -> bool {
    RendererInitAttempts.fetch_add(1, std::memory_order_relaxed);
    try {
        ComPtr<ID3D12Device> device;
        if (!swapChain
            || FAILED(swapChain->GetDevice(IID_PPV_ARGS(&device)))) {
            return FailRendererInitialization(
                1,
                "MapSense: renderer initialization failed at swap-chain device lookup.");
        }

        DXGI_SWAP_CHAIN_DESC swapDesc{};
        if (FAILED(swapChain->GetDesc(&swapDesc))
            || swapDesc.BufferCount == 0U
            || !swapDesc.OutputWindow) {
            return FailRendererInitialization(
                2,
                "MapSense: renderer initialization rejected a swap chain without D2R's OutputWindow.");
        }

        ComPtr<ID3D12Device> queueDevice;
        if (!CommandQueue
            || FAILED(CommandQueue->GetDevice(IID_PPV_ARGS(&queueDevice)))
            || !SameComIdentity(queueDevice.Get(), device.Get())) {
            RendererPoisoned = true;
            return FailRendererInitialization(
                3,
                "MapSense: exact swap-chain command queue does not belong to the swap-chain device; GPU rendering is disabled.");
        }

        GameWindow = swapDesc.OutputWindow;
        PublishedGameWindow.store(GameWindow, std::memory_order_release);
        ActiveSwapChain = swapChain;
        BackBufferFormat = swapDesc.BufferDesc.Format == DXGI_FORMAT_UNKNOWN
            ? DXGI_FORMAT_R8G8B8A8_UNORM
            : swapDesc.BufferDesc.Format;

        D3D12_DESCRIPTOR_HEAP_DESC srvDesc{};
        srvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        srvDesc.NumDescriptors = 1U;
        srvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        if (FAILED(device->CreateDescriptorHeap(
                &srvDesc,
                IID_PPV_ARGS(&SrvHeap)))) {
            return FailRendererInitialization(
                4,
                "MapSense: renderer initialization failed at SRV heap creation.");
        }

        D3D12_DESCRIPTOR_HEAP_DESC rtvDesc{};
        rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvDesc.NumDescriptors = swapDesc.BufferCount;
        if (FAILED(device->CreateDescriptorHeap(
                &rtvDesc,
                IID_PPV_ARGS(&RtvHeap)))) {
            return FailRendererInitialization(
                5,
                "MapSense: renderer initialization failed at RTV heap creation.");
        }

        Frames.clear();
        Frames.resize(swapDesc.BufferCount);
        auto descriptor = RtvHeap->GetCPUDescriptorHandleForHeapStart();
        const UINT descriptorSize = device->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        for (UINT index = 0U; index < swapDesc.BufferCount; ++index) {
            auto& frame = Frames[index];
            if (FAILED(device->CreateCommandAllocator(
                    D3D12_COMMAND_LIST_TYPE_DIRECT,
                    IID_PPV_ARGS(&frame.allocator)))) {
                return FailRendererInitialization(
                    6,
                    "MapSense: renderer initialization failed at command allocator creation.");
            }
            if (FAILED(swapChain->GetBuffer(
                    index,
                    IID_PPV_ARGS(&frame.renderTarget)))) {
                return FailRendererInitialization(
                    7,
                    "MapSense: renderer initialization failed at back-buffer lookup.");
            }
            frame.descriptor = descriptor;
            device->CreateRenderTargetView(
                frame.renderTarget.Get(), nullptr, descriptor);
            descriptor.ptr += descriptorSize;
        }

        if (FAILED(device->CreateCommandList(
                0U,
                D3D12_COMMAND_LIST_TYPE_DIRECT,
                Frames[0].allocator.Get(),
                nullptr,
                IID_PPV_ARGS(&CommandList)))) {
            return FailRendererInitialization(
                8,
                "MapSense: renderer initialization failed at command-list creation.");
        }
        if (FAILED(CommandList->Close())) {
            return FailRendererInitialization(
                9,
                "MapSense: renderer initialization failed while closing the command list.");
        }
        if (FAILED(device->CreateFence(
                0U,
                D3D12_FENCE_FLAG_NONE,
                IID_PPV_ARGS(&Fence)))) {
            return FailRendererInitialization(
                10,
                "MapSense: renderer initialization failed at fence creation.");
        }
        FenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        if (!FenceEvent) {
            return FailRendererInitialization(
                11,
                "MapSense: renderer initialization failed at fence-event creation.");
        }

        IMGUI_CHECKVERSION();
        if (ImGui::CreateContext() == nullptr) {
            return FailRendererInitialization(
                12,
                "MapSense: renderer initialization failed at ImGui context creation.");
        }
        ImGuiContextCreated = true;
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = nullptr;
        io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;
        // D2R remains the sole cursor owner. MapSense consumes panel mouse
        // input without asking the Win32 backend to show a second OS cursor.
        io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
        ImFont* const mapSenseDefaultFont = io.Fonts->AddFontDefault();
        if (mapSenseDefaultFont == nullptr) {
            return FailRendererInitialization(
                13,
                "MapSense: renderer initialization failed while adding its default font.");
        }
        io.FontDefault = mapSenseDefaultFont;
        ImGui::StyleColorsDark();
        if (!ImGui_ImplWin32_Init(GameWindow)) {
            return FailRendererInitialization(
                14,
                "MapSense: renderer initialization failed at ImGui Win32 startup.");
        }
        ImGuiWin32Initialized = true;
        // MapSense owns the first/default atlas font. Optional clients attach
        // their fonts before the DX12 backend creates the shared font texture.
        NotifyContextCreatedLocked(externalClients);
        if (!ImGui_ImplDX12_Init(
                device.Get(),
                swapDesc.BufferCount,
                BackBufferFormat,
                SrvHeap.Get(),
                SrvHeap->GetCPUDescriptorHandleForHeapStart(),
                SrvHeap->GetGPUDescriptorHandleForHeapStart())) {
            return FailRendererInitialization(
                15,
                "MapSense: renderer initialization failed at ImGui DirectX 12 startup.");
        }
        ImGuiDx12Initialized = true;
        if (!ImGui_ImplDX12_CreateDeviceObjects()) {
            return FailRendererInitialization(
                16,
                "MapSense: renderer initialization failed while creating ImGui device objects.");
        }

        LastFrameTime = std::chrono::steady_clock::now();
        RendererInitialized = true;
        RendererInitializedPublished.store(true, std::memory_order_release);
        if (!QueueInputSubclassInstallLocked()
            && !InputQueueWarningLogged.exchange(
                true, std::memory_order_acq_rel)) {
            LogWarning(
                "MapSense: input subclass installation could not be queued on D2R's UI thread.");
        }
        LogInfo("MapSense: in-frame D3D12/ImGui host initialized on D2R's window.");
        return true;
    }
    catch (...) {
        return FailRendererInitialization(
            17,
            "MapSense: renderer initialization failed with an unexpected exception.");
    }
}

auto RenderPanelFrame(
    IDXGISwapChain3* swapChain,
    const std::array<ExternalClientEntry, MaximumExternalClients>&
        externalClients) noexcept -> bool {
    const UINT frameIndex = swapChain->GetCurrentBackBufferIndex();
    if (frameIndex >= Frames.size()) return false;
    auto& frame = Frames[frameIndex];
    if (!WaitForFenceValueLocked(frame.fenceValue)) {
        RendererPoisoned = true;
        RendererInitializedPublished.store(false, std::memory_order_release);
        LogWarning(
            "MapSense: GPU rendering disabled because submitted work did not reach its fence safely.");
        return false;
    }

    const auto now = std::chrono::steady_clock::now();
    const float deltaSeconds = LastFrameTime.time_since_epoch().count() == 0
        ? (1.0F / 60.0F)
        : std::chrono::duration<float>(now - LastFrameTime).count();
    LastFrameTime = now;

    auto externalFrame = MakeExternalFrameContextLocked();
    for (const auto& client : externalClients) {
        if (client.beforeFrame != nullptr)
            client.beforeFrame(&externalFrame, client.userData);
    }

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGuiIO& io = ImGui::GetIO();
    io.DeltaTime = deltaSeconds > 0.0F && deltaSeconds <= 0.1F
        ? deltaSeconds
        : (1.0F / 60.0F);
    ImGui::NewFrame();

    D3D12ImGuiPanelBounds bounds{};
    bool open = MenuOpen.load(std::memory_order_acquire);
    const bool inputReady = InputSubclassInstalled.load(
        std::memory_order_acquire);
    if (open && inputReady && Callbacks.drawPanel)
        bounds = Callbacks.drawPanel(&open, Callbacks.userData);
    if (!open) bounds = {};
    MenuOpen.store(open, std::memory_order_release);
    PublishPanelBounds(bounds);
    AwaitingFirstBounds.store(
        open && !inputReady, std::memory_order_release);

    if (Callbacks.drawOwnedOverlay != nullptr)
        Callbacks.drawOwnedOverlay(Callbacks.userData);

    externalFrame.displayWidth = io.DisplaySize.x;
    externalFrame.displayHeight = io.DisplaySize.y;
    for (const auto& client : externalClients) {
        if (client.render != nullptr)
            client.render(&externalFrame, client.userData);
    }

    ImGui::Render();
    ImDrawData* const drawData = ImGui::GetDrawData();
    if (drawData == nullptr
        || !ShouldSubmitD3D12DrawData(
            drawData->CmdListsCount, drawData->TotalVtxCount)) {
        return true;
    }

    HRESULT result = frame.allocator->Reset();
    if (FAILED(result)) {
        RendererPoisoned = true;
        RendererInitializedPublished.store(false, std::memory_order_release);
        LogWarning(
            "MapSense: GPU rendering disabled after command-allocator reset failed.");
        return false;
    }
    result = CommandList->Reset(frame.allocator.Get(), nullptr);
    if (FAILED(result)) {
        RendererPoisoned = true;
        RendererInitializedPublished.store(false, std::memory_order_release);
        LogWarning(
            "MapSense: GPU rendering disabled after command-list reset failed.");
        return false;
    }

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = frame.renderTarget.Get();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    CommandList->ResourceBarrier(1U, &barrier);
    CommandList->OMSetRenderTargets(
        1U, &frame.descriptor, FALSE, nullptr);
    ID3D12DescriptorHeap* heaps[]{SrvHeap.Get()};
    CommandList->SetDescriptorHeaps(1U, heaps);
    ImGui_ImplDX12_RenderDrawData(drawData, CommandList.Get());
    const auto oldState = barrier.Transition.StateBefore;
    barrier.Transition.StateBefore = barrier.Transition.StateAfter;
    barrier.Transition.StateAfter = oldState;
    CommandList->ResourceBarrier(1U, &barrier);
    result = CommandList->Close();
    if (FAILED(result)) {
        RendererPoisoned = true;
        RendererInitializedPublished.store(false, std::memory_order_release);
        LogWarning(
            "MapSense: GPU rendering disabled after command-list close failed.");
        return false;
    }

    ID3D12CommandList* commandLists[]{CommandList.Get()};
    CommandQueue->ExecuteCommandLists(1U, commandLists);
    const std::uint64_t fenceValue = NextFenceValue++;
    result = CommandQueue->Signal(Fence.Get(), fenceValue);
    if (FAILED(result)) {
        RendererPoisoned = true;
        RendererInitializedPublished.store(false, std::memory_order_release);
        LogWarning(
            "MapSense: GPU rendering disabled after command-queue signaling failed.");
        return false;
    }
    frame.fenceValue = fenceValue;
    RenderedFrames.fetch_add(1U, std::memory_order_relaxed);
    return true;
}

auto IsDeviceRemovalResult(HRESULT result) noexcept -> bool {
    return result == DXGI_ERROR_DEVICE_REMOVED
        || result == DXGI_ERROR_DEVICE_HUNG
        || result == DXGI_ERROR_DEVICE_RESET
        || result == DXGI_ERROR_DRIVER_INTERNAL_ERROR;
}

void RecordDeviceRemovalLocked(
    IDXGISwapChain3* swapChain,
    HRESULT presentResult) noexcept {
    RendererPoisoned = true;
    RendererResetPending = true;
    RendererInitializedPublished.store(false, std::memory_order_release);
    if (DeviceRemovalWarningLogged.exchange(true, std::memory_order_acq_rel))
        return;

    HRESULT deviceReason = presentResult;
    ComPtr<ID3D12Device> device;
    if (swapChain != nullptr
        && SUCCEEDED(swapChain->GetDevice(IID_PPV_ARGS(&device)))) {
        deviceReason = device->GetDeviceRemovedReason();
    }
    char message[256]{};
    const int written = std::snprintf(
        message,
        sizeof(message),
        "MapSense: Present reported device removal (present=0x%08lX, reason=0x%08lX); all further plugin GPU submissions are blocked.",
        static_cast<unsigned long>(
            static_cast<std::uint32_t>(presentResult)),
        static_cast<unsigned long>(
            static_cast<std::uint32_t>(deviceReason)));
    if (written > 0) LogWarning(message);
}

auto STDMETHODCALLTYPE HookPresent(
    IDXGISwapChain3* swapChain,
    UINT syncInterval,
    UINT flags) noexcept -> HRESULT {
    HookCallGuard hookCall;
    PresentCalls.fetch_add(1U, std::memory_order_relaxed);
    PresentFn original{};
    {
        std::scoped_lock lock(HostMutex);
        original = OriginalPresent;
        if (original != nullptr
            && !RendererPoisoned
            && !RendererResetPending) {
            // Registration and rendering share HostMutex, so this is the one
            // stable client snapshot used for this complete Present pass.
            const auto externalClients = CopyExternalClients();
            const bool hasExternalClient = HasExternalFrameClient(
                externalClients);
            const bool wantsOwnedOverlay
                = Callbacks.wantsOwnedOverlay != nullptr
                && Callbacks.wantsOwnedOverlay(Callbacks.userData);
            const bool wantsFrame = MenuOpen.load(std::memory_order_acquire)
                || OwnedMouseButtons.load(std::memory_order_acquire) != 0U
                || wantsOwnedOverlay
                || hasExternalClient;
            if (wantsFrame
                && (!RendererInitialized
                    || ActiveSwapChain == swapChain)
                && SelectExactSwapChainQueueLocked(swapChain)) {
                if (!RendererInitialized) {
                    InitializeRenderer(swapChain, externalClients);
                }
                if (RendererInitialized) {
                    if (!InputSubclassInstalled.load(
                            std::memory_order_acquire)
                        && !QueueInputSubclassInstallLocked()
                        && !InputQueueWarningLogged.exchange(
                            true, std::memory_order_acq_rel)) {
                        LogWarning(
                            "MapSense: input subclass installation retry could not be queued.");
                    }
                    // Client overlays remain live while the MapSense settings
                    // panel is collapsed.
                    RenderPanelFrame(swapChain, externalClients);
                }
            }
        }
    }
    // Never hold HostMutex across D2R/DXGI code. Shutdown disables hooks and
    // waits for HookCallGuard before removing the trampoline behind this copy.
    const HRESULT result = original != nullptr
        ? original(swapChain, syncInterval, flags)
        : DXGI_ERROR_INVALID_CALL;
    if (IsDeviceRemovalResult(result)) {
        std::scoped_lock lock(HostMutex);
        RecordDeviceRemovalLocked(swapChain, result);
    }
    return result;
}

auto STDMETHODCALLTYPE HookResizeBuffers(
    IDXGISwapChain3* swapChain,
    UINT bufferCount,
    UINT width,
    UINT height,
    DXGI_FORMAT format,
    UINT flags) noexcept -> HRESULT {
    HookCallGuard hookCall;
    ResizeBuffersFn original{};
    bool resetRenderer{};
    {
        std::scoped_lock lock(HostMutex);
        original = OriginalResizeBuffers;
        resetRenderer = RendererInitialized && ActiveSwapChain == swapChain;
        if (resetRenderer) {
            RendererResetPending = true;
            RendererInitializedPublished.store(
                false, std::memory_order_release);
        }
    }
    if (resetRenderer && !RemoveInputSubclassSynchronously()) {
        std::scoped_lock lock(HostMutex);
        RendererResetPending = false;
        RendererInitializedPublished.store(
            RendererInitialized, std::memory_order_release);
        LogWarning(
            "MapSense: ResizeBuffers was deferred because the D2R input subclass could not be removed safely.");
        return DXGI_ERROR_INVALID_CALL;
    }
    if (resetRenderer) {
        std::scoped_lock lock(HostMutex);
        if (RendererInitialized && ActiveSwapChain == swapChain)
            ResetRendererStateLocked();
    }
    return original != nullptr
        ? original(
            swapChain, bufferCount, width, height, format, flags)
        : DXGI_ERROR_INVALID_CALL;
}

auto ResolveD3D12CreateDevice() noexcept -> D3D12CreateDeviceFn {
    HMODULE d3d12Module = GetModuleHandleW(L"d3d12.dll");
    if (!d3d12Module) d3d12Module = LoadLibraryW(L"d3d12.dll");
    return d3d12Module
        ? reinterpret_cast<D3D12CreateDeviceFn>(
            GetProcAddress(d3d12Module, "D3D12CreateDevice"))
        : nullptr;
}

auto BuildFactoryMethodTable() noexcept -> bool {
    ComPtr<IDXGIFactory2> factory;
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) return false;
    std::memcpy(
        FactoryMethods.data(),
        *reinterpret_cast<void***>(factory.Get()),
        FactoryMethodCount * sizeof(void*));
    return true;
}

auto BuildMethodTableWithoutWindow() noexcept -> bool {
    const auto createDevice = ResolveD3D12CreateDevice();
    if (!createDevice) return false;

    ComPtr<IDXGIFactory2> factory;
    ComPtr<ID3D12Device> device;
    ComPtr<ID3D12CommandQueue> queue;
    ComPtr<ID3D12CommandAllocator> allocator;
    ComPtr<ID3D12GraphicsCommandList> commandList;
    ComPtr<IDXGISwapChain1> swapChain1;
    ComPtr<IDXGISwapChain3> swapChain3;

    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) return false;
    if (FAILED(createDevice(
            nullptr,
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&device)))) {
        return false;
    }
    D3D12_COMMAND_QUEUE_DESC queueDesc{};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    if (FAILED(device->CreateCommandQueue(
            &queueDesc,
            IID_PPV_ARGS(&queue)))) {
        return false;
    }
    if (FAILED(device->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(&allocator)))) {
        return false;
    }
    if (FAILED(device->CreateCommandList(
            0U,
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            allocator.Get(),
            nullptr,
            IID_PPV_ARGS(&commandList)))) {
        return false;
    }

    DXGI_SWAP_CHAIN_DESC1 swapDesc{};
    swapDesc.Width = 100U;
    swapDesc.Height = 100U;
    swapDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapDesc.SampleDesc.Count = 1U;
    swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapDesc.BufferCount = 2U;
    swapDesc.Scaling = DXGI_SCALING_STRETCH;
    swapDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    swapDesc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;
    const HRESULT swapChainResult
        = FactoryHooksInstalled
            && OriginalCreateSwapChainForComposition != nullptr
        ? OriginalCreateSwapChainForComposition(
            factory.Get(), queue.Get(), &swapDesc, nullptr, &swapChain1)
        : factory->CreateSwapChainForComposition(
            queue.Get(), &swapDesc, nullptr, &swapChain1);
    if (FAILED(swapChainResult)) {
        return false;
    }
    if (FAILED(swapChain1.As(&swapChain3))) return false;

    std::memcpy(
        Methods.data(),
        *reinterpret_cast<void***>(device.Get()),
        44U * sizeof(void*));
    std::memcpy(
        Methods.data() + 44U,
        *reinterpret_cast<void***>(queue.Get()),
        19U * sizeof(void*));
    std::memcpy(
        Methods.data() + 63U,
        *reinterpret_cast<void***>(allocator.Get()),
        9U * sizeof(void*));
    std::memcpy(
        Methods.data() + 72U,
        *reinterpret_cast<void***>(commandList.Get()),
        60U * sizeof(void*));
    std::memcpy(
        Methods.data() + 132U,
        *reinterpret_cast<void***>(swapChain3.Get()),
        18U * sizeof(void*));
    return true;
}

auto CreateAndEnableHook(
    std::size_t methodIndex,
    void* detour,
    void** original) noexcept -> bool {
    if (methodIndex >= Methods.size() || !Methods[methodIndex]) return false;
    const MH_STATUS created = MH_CreateHook(
        Methods[methodIndex], detour, original);
    if (created != MH_OK) return false;
    if (MH_EnableHook(Methods[methodIndex]) == MH_OK) return true;
    MH_RemoveHook(Methods[methodIndex]);
    if (original) *original = nullptr;
    return false;
}

auto CreateAndEnableFactoryHook(
    std::size_t methodIndex,
    void* detour,
    void** original) noexcept -> bool {
    if (methodIndex >= FactoryMethods.size()
        || !FactoryMethods[methodIndex]) {
        return false;
    }
    const MH_STATUS created = MH_CreateHook(
        FactoryMethods[methodIndex], detour, original);
    if (created != MH_OK) return false;
    if (MH_EnableHook(FactoryMethods[methodIndex]) == MH_OK) return true;
    MH_RemoveHook(FactoryMethods[methodIndex]);
    if (original) *original = nullptr;
    return false;
}

void RemoveOwnedHook(std::size_t methodIndex) noexcept {
    if (methodIndex >= Methods.size() || !Methods[methodIndex]) return;
    MH_DisableHook(Methods[methodIndex]);
    MH_RemoveHook(Methods[methodIndex]);
}

void RemoveOwnedFactoryHook(std::size_t methodIndex) noexcept {
    if (methodIndex >= FactoryMethods.size()
        || !FactoryMethods[methodIndex]) {
        return;
    }
    MH_DisableHook(FactoryMethods[methodIndex]);
    MH_RemoveHook(FactoryMethods[methodIndex]);
}

void WaitForActiveHookCalls() noexcept {
    while (ActiveHookCalls.load(std::memory_order_acquire) != 0U)
        Sleep(1U);
}

auto AttachClientToLiveContextLocked(
    const ExternalClientEntry& incoming,
    const ExternalClientEntry* replaced) noexcept -> bool {
    if (!ContextCallbacksActive || !ImGuiContextCreated) return true;
    const bool contextChanged = replaced == nullptr
        || replaced->contextCreated != incoming.contextCreated
        || replaced->contextDestroying != incoming.contextDestroying
        || replaced->userData != incoming.userData;
    if (!contextChanged) return true;
    if (!WaitForGpuIdleLocked()) {
        LogWarning(
            "MapSense: a late overlay client could not join while GPU work was active.");
        return false;
    }

    if (ImGuiDx12Initialized) ImGui_ImplDX12_InvalidateDeviceObjects();
    const auto frame = MakeExternalFrameContextLocked();
    if (replaced != nullptr && replaced->contextDestroying != nullptr)
        replaced->contextDestroying(&frame, replaced->userData);
    if (incoming.contextCreated != nullptr)
        incoming.contextCreated(&frame, incoming.userData);

    if (!ImGuiDx12Initialized || ImGui_ImplDX12_CreateDeviceObjects())
        return true;

    // Restore the previous client lifecycle if the shared atlas could not be
    // uploaded. Registration remains transactional from the registry's view.
    if (incoming.contextDestroying != nullptr)
        incoming.contextDestroying(&frame, incoming.userData);
    if (replaced != nullptr && replaced->contextCreated != nullptr)
        replaced->contextCreated(&frame, replaced->userData);
    if (ImGuiDx12Initialized
        && !ImGui_ImplDX12_CreateDeviceObjects()) {
        LogWarning(
            "MapSense: the ImGui device objects could not be restored after a client registration failure.");
    }
    return false;
}
} // namespace

auto InitializeD3D12ImGuiHost(
    D3D12ImGuiHostCallbacks callbacks) noexcept -> bool {
    const bool configured = callbacks.drawPanel != nullptr
        && callbacks.ownedOverlayDismissal != nullptr
        && callbacks.queueUiTask != nullptr;
    {
        std::scoped_lock lock(HostMutex);
        Callbacks = callbacks;
        Configured = configured;
        ClientsAccepting.store(true, std::memory_order_release);
        ConfiguredPublished.store(Configured, std::memory_order_release);
        InfoLogger.store(callbacks.info, std::memory_order_release);
        WarningLogger.store(callbacks.warning, std::memory_order_release);
    }
    if (!configured) return false;
    return TryInstallD3D12ImGuiHooks();
}

auto TryInstallD3D12ImGuiHooks() noexcept -> bool {
    std::scoped_lock lock(HostMutex);
    if (HooksInstalled) return true;
    if (!Configured) return false;

    if (!MinHookReady) {
        const MH_STATUS initialized = MH_Initialize();
        if (initialized != MH_OK
            && initialized != MH_ERROR_ALREADY_INITIALIZED) {
            return false;
        }
        MinHookReady = true;
        MinHookInitializedByHost = initialized == MH_OK;
    }

    const auto rollbackFactoryHooks = []() noexcept -> bool {
        if (OriginalCreateSwapChainForComposition != nullptr)
            RemoveOwnedFactoryHook(CreateSwapChainForCompositionMethod);
        if (OriginalCreateSwapChainForCoreWindow != nullptr)
            RemoveOwnedFactoryHook(CreateSwapChainForCoreWindowMethod);
        if (OriginalCreateSwapChainForHwnd != nullptr)
            RemoveOwnedFactoryHook(CreateSwapChainForHwndMethod);
        if (OriginalCreateSwapChain != nullptr)
            RemoveOwnedFactoryHook(CreateSwapChainMethod);
        OriginalResizeBuffers = nullptr;
        OriginalPresent = nullptr;
        OriginalCreateSwapChainForComposition = nullptr;
        OriginalCreateSwapChainForCoreWindow = nullptr;
        OriginalCreateSwapChainForHwnd = nullptr;
        OriginalCreateSwapChain = nullptr;
        FactoryHooksInstalled = false;
        FactoryMethods = {};
        if (MinHookInitializedByHost) MH_Uninitialize();
        MinHookReady = false;
        MinHookInitializedByHost = false;
        return false;
    };

    if (!FactoryHooksInstalled) {
        FactoryMethods = {};
        if (!BuildFactoryMethodTable()
            || !CreateAndEnableFactoryHook(
                CreateSwapChainMethod,
                reinterpret_cast<void*>(HookCreateSwapChain),
                reinterpret_cast<void**>(&OriginalCreateSwapChain))
            || !CreateAndEnableFactoryHook(
                CreateSwapChainForHwndMethod,
                reinterpret_cast<void*>(HookCreateSwapChainForHwnd),
                reinterpret_cast<void**>(&OriginalCreateSwapChainForHwnd))
            || !CreateAndEnableFactoryHook(
                CreateSwapChainForCoreWindowMethod,
                reinterpret_cast<void*>(HookCreateSwapChainForCoreWindow),
                reinterpret_cast<void**>(
                    &OriginalCreateSwapChainForCoreWindow))
            || !CreateAndEnableFactoryHook(
                CreateSwapChainForCompositionMethod,
                reinterpret_cast<void*>(HookCreateSwapChainForComposition),
                reinterpret_cast<void**>(
                    &OriginalCreateSwapChainForComposition))) {
            return rollbackFactoryHooks();
        }
        FactoryHooksInstalled = true;
        LogInfo(
            "MapSense: early DXGI swap-chain ownership hooks installed before D2R graphics initialization.");
    }

    Methods = {};
    if (!BuildMethodTableWithoutWindow()) {
        LogWarning(
            "MapSense: D3D12 Present method discovery is waiting while early swap-chain ownership hooks remain active.");
        return false;
    }

    const auto rollbackRenderHooks = []() noexcept -> bool {
        if (OriginalResizeBuffers != nullptr)
            RemoveOwnedHook(ResizeBuffersMethod);
        if (OriginalPresent != nullptr) RemoveOwnedHook(PresentMethod);
        OriginalResizeBuffers = nullptr;
        OriginalPresent = nullptr;
        return false;
    };
    if (!CreateAndEnableHook(
            PresentMethod,
            reinterpret_cast<void*>(HookPresent),
            reinterpret_cast<void**>(&OriginalPresent))
        || !CreateAndEnableHook(
            ResizeBuffersMethod,
            reinterpret_cast<void*>(HookResizeBuffers),
            reinterpret_cast<void**>(&OriginalResizeBuffers))) {
        return rollbackRenderHooks();
    }

    HooksInstalled = true;
    HooksInstalledPublished.store(true, std::memory_order_release);
    LogInfo(
        "MapSense: fail-closed D3D12 hooks installed with exact swap-chain command-queue ownership.");
    return true;
}

void ShutdownD3D12ImGuiHost() noexcept {
    {
        std::scoped_lock lock(HostMutex);
        if (HooksInstalled) {
            // Disable first so no new hook entry can race resource teardown.
            MH_DisableHook(Methods[ResizeBuffersMethod]);
            MH_DisableHook(Methods[PresentMethod]);
            HooksInstalledPublished.store(false, std::memory_order_release);
        }
        if (FactoryHooksInstalled) {
            MH_DisableHook(
                FactoryMethods[CreateSwapChainForCompositionMethod]);
            MH_DisableHook(
                FactoryMethods[CreateSwapChainForCoreWindowMethod]);
            MH_DisableHook(FactoryMethods[CreateSwapChainForHwndMethod]);
            MH_DisableHook(FactoryMethods[CreateSwapChainMethod]);
        }
        RendererResetPending = true;
        MenuOpen.store(false, std::memory_order_release);
        AwaitingFirstBounds.store(false, std::memory_order_release);
        PublishPanelBounds({});
    }

    WaitForActiveHookCalls();
    const bool inputSubclassRemoved = RemoveInputSubclassSynchronously();
    if (!inputSubclassRemoved) {
        if (!PinHostModuleForResidualSubclass()) {
            LogWarning(
                "MapSense: waiting for the D2R input subclass to acknowledge removal before unload.");
            while (!RemoveInputSubclassSynchronously()) Sleep(1U);
        }
        else {
            LogWarning(
                "MapSense: D2R retained the input subclass; the host module was pinned for process-lifetime safety.");
        }
    }

    {
        std::scoped_lock lock(HostMutex);
        ResetRendererStateLocked(
            inputSubclassRemoved
                || !InputSubclassInstalled.load(std::memory_order_acquire));
        CommandQueue.Reset();
        CapturedQueue.store(nullptr, std::memory_order_release);
        SwapChainQueueBindings.clear();
        if (HooksInstalled) {
            MH_RemoveHook(Methods[ResizeBuffersMethod]);
            MH_RemoveHook(Methods[PresentMethod]);
        }
        if (FactoryHooksInstalled) {
            MH_RemoveHook(
                FactoryMethods[CreateSwapChainForCompositionMethod]);
            MH_RemoveHook(
                FactoryMethods[CreateSwapChainForCoreWindowMethod]);
            MH_RemoveHook(FactoryMethods[CreateSwapChainForHwndMethod]);
            MH_RemoveHook(FactoryMethods[CreateSwapChainMethod]);
        }
        if (MinHookInitializedByHost) MH_Uninitialize();
        MinHookReady = false;
        MinHookInitializedByHost = false;
        HooksInstalled = false;
        FactoryHooksInstalled = false;
        HooksInstalledPublished.store(false, std::memory_order_release);
        OriginalResizeBuffers = nullptr;
        OriginalPresent = nullptr;
        OriginalCreateSwapChainForComposition = nullptr;
        OriginalCreateSwapChainForCoreWindow = nullptr;
        OriginalCreateSwapChainForHwnd = nullptr;
        OriginalCreateSwapChain = nullptr;
        Methods = {};
        FactoryMethods = {};
        RendererPoisoned = false;
        UnboundSwapChainWarningLogged.store(false, std::memory_order_release);
        DeviceRemovalWarningLogged.store(false, std::memory_order_release);
        MenuOpen.store(false, std::memory_order_release);
        AwaitingFirstBounds.store(false, std::memory_order_release);
        PublishPanelBounds({});
        Callbacks = {};
        Configured = false;
        ConfiguredPublished.store(false, std::memory_order_release);
        InfoLogger.store(nullptr, std::memory_order_release);
        WarningLogger.store(nullptr, std::memory_order_release);
    }
}

void ShutdownD3D12ImGuiHostAndNotifyClients() noexcept {
    ClientsAccepting.store(false, std::memory_order_release);
    ShutdownD3D12ImGuiHost();

    std::array<ExternalClientEntry, MaximumExternalClients> clients{};
    {
        std::scoped_lock lock(ExternalClientMutex);
        clients = ExternalClients;
        ExternalClients = {};
    }
    // Renderer resources and all MapSense hooks are already gone. A client
    // may now safely install its autonomous host from this callback.
    for (const auto& client : clients) {
        if (client.hostStopped != nullptr)
            client.hostStopped(client.userData);
    }
}

void ToggleD3D12ImGuiMenu() noexcept {
    std::scoped_lock lock(HostMutex);
    bool wasOpen = MenuOpen.load(std::memory_order_acquire);
    while (!MenuOpen.compare_exchange_weak(
        wasOpen,
        !wasOpen,
        std::memory_order_acq_rel,
        std::memory_order_acquire)) {
    }
    const bool isOpen = !wasOpen;
    AwaitingFirstBounds.store(isOpen, std::memory_order_release);
    if (!isOpen) PublishPanelBounds({});
}

void SetD3D12ImGuiMenuOpen(bool open) noexcept {
    // RenderPanelFrame reads, lets the panel mutate, and republishes this state
    // while holding HostMutex. Serialize lifecycle changes with that complete
    // transition so a late Present cannot undo GameLeft or LocalPlayerReady.
    std::scoped_lock lock(HostMutex);
    const bool changed = MenuOpen.exchange(open, std::memory_order_acq_rel)
        != open;
    if (changed && open)
        AwaitingFirstBounds.store(true, std::memory_order_release);
    if (!open) {
        const bool releasedOwnedButton = CancelOwnedMouseButtons();
        const HWND window = PublishedGameWindow.load(std::memory_order_acquire);
        if (releasedOwnedButton
            && window != nullptr
            && GetCapture() == window) {
            ReleaseCapture();
        }
        AwaitingFirstBounds.store(false, std::memory_order_release);
        PublishPanelBounds({});
    }
}

auto IsD3D12ImGuiMenuOpen() noexcept -> bool {
    return MenuOpen.load(std::memory_order_acquire);
}

auto GetD3D12ImGuiHostStatus() noexcept -> D3D12ImGuiHostStatus {
    return D3D12ImGuiHostStatus{
        .presentCalls = PresentCalls.load(std::memory_order_relaxed),
        .directQueueCaptures = ExactQueueBindings.load(
            std::memory_order_relaxed),
        .rendererInitAttempts = RendererInitAttempts.load(
            std::memory_order_relaxed),
        .rendererInitFailures = RendererInitFailures.load(
            std::memory_order_relaxed),
        .renderedFrames = RenderedFrames.load(std::memory_order_relaxed),
        .lastInitFailureStage = LastInitFailureStage.load(
            std::memory_order_relaxed),
        .configured = ConfiguredPublished.load(std::memory_order_acquire),
        .hooksInstalled = HooksInstalledPublished.load(
            std::memory_order_acquire),
        .commandQueueReady = CapturedQueue.load(
            std::memory_order_acquire) != nullptr,
        .rendererInitialized = RendererInitializedPublished.load(
            std::memory_order_acquire),
        .inputSubclassInstalled = InputSubclassInstalledPublished.load(
            std::memory_order_acquire),
        .menuOpen = MenuOpen.load(std::memory_order_acquire),
        .gameWindow = PublishedGameWindow.load(std::memory_order_acquire),
    };
}

auto RegisterD3D12ImGuiClientV2(
        const RuffnecKk::OverlayHost::ClientV2* client) noexcept -> bool {
    if (client == nullptr
        || client->structSize < RuffnecKk::OverlayHost::ClientV2Size
        || client->version != RuffnecKk::OverlayHost::ApiVersion2
        || client->imguiAbiFingerprint
            != RuffnecKk::OverlayHost::ImGuiAbiFingerprint
        || client->owner == nullptr
        || client->owner[0] == '\0'
        || strnlen_s(client->owner, 64U) >= 64U
        || (client->contextCreated == nullptr
            && client->contextDestroying == nullptr
            && client->hostStopped == nullptr
            && client->beforeFrame == nullptr
            && client->render == nullptr)
        || !ClientsAccepting.load(std::memory_order_acquire)) {
        return false;
    }

    std::scoped_lock hostLock(HostMutex);
    if (!ClientsAccepting.load(std::memory_order_acquire)) return false;

    ExternalClientEntry incoming{};
    strncpy_s(
        incoming.owner.data(),
        incoming.owner.size(),
        client->owner,
        _TRUNCATE);
    incoming.contextCreated = client->contextCreated;
    incoming.contextDestroying = client->contextDestroying;
    incoming.hostStopped = client->hostStopped;
    incoming.beforeFrame = client->beforeFrame;
    incoming.render = client->render;
    incoming.userData = client->userData;

    std::size_t selected = MaximumExternalClients;
    ExternalClientEntry replaced{};
    bool replacing{};
    {
        std::scoped_lock registryLock(ExternalClientMutex);
        for (std::size_t index = 0U;
             index < ExternalClients.size();
             ++index) {
            const auto& entry = ExternalClients[index];
            if (entry.owner[0] != '\0'
                && std::strcmp(entry.owner.data(), client->owner) == 0) {
                selected = index;
                replaced = entry;
                replacing = true;
                break;
            }
            if (entry.owner[0] == '\0'
                && selected == MaximumExternalClients) {
                selected = index;
            }
        }
    }
    if (selected == MaximumExternalClients) return false;
    if (!AttachClientToLiveContextLocked(
            incoming, replacing ? &replaced : nullptr)) {
        return false;
    }
    {
        std::scoped_lock registryLock(ExternalClientMutex);
        ExternalClients[selected] = incoming;
    }
    return true;
}

auto UnregisterD3D12ImGuiClientV2(const char* owner) noexcept -> bool {
    if (owner == nullptr || owner[0] == '\0') return false;
    std::scoped_lock hostLock(HostMutex);
    std::size_t selected = MaximumExternalClients;
    ExternalClientEntry removed{};
    {
        std::scoped_lock registryLock(ExternalClientMutex);
        for (std::size_t index = 0U;
             index < ExternalClients.size();
             ++index) {
            const auto& entry = ExternalClients[index];
            if (entry.owner[0] != '\0'
                && std::strcmp(entry.owner.data(), owner) == 0) {
                selected = index;
                removed = entry;
                break;
            }
        }
    }
    if (selected == MaximumExternalClients) return true;

    if (ContextCallbacksActive && ImGuiContextCreated) {
        if (!WaitForGpuIdleLocked()) {
            LogWarning(
                "MapSense: GPU idle wait failed while unregistering an overlay client.");
        }
        const auto frame = MakeExternalFrameContextLocked();
        if (removed.contextDestroying != nullptr)
            removed.contextDestroying(&frame, removed.userData);
    }
    {
        std::scoped_lock registryLock(ExternalClientMutex);
        ExternalClients[selected] = {};
    }
    return true;
}

void ClearD3D12ImGuiClientsV2() noexcept {
    std::scoped_lock hostLock(HostMutex);
    const auto clients = CopyExternalClients();
    if (ContextCallbacksActive && ImGuiContextCreated) {
        if (!WaitForGpuIdleLocked()) {
            LogWarning(
                "MapSense: GPU idle wait failed while clearing overlay clients.");
        }
        const auto frame = MakeExternalFrameContextLocked();
        for (const auto& client : clients) {
            if (client.contextDestroying != nullptr)
                client.contextDestroying(&frame, client.userData);
        }
    }
    std::scoped_lock registryLock(ExternalClientMutex);
    ExternalClients = {};
}

} // namespace RuffnecKk::MapSense
