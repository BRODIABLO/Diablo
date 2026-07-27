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

namespace {
using ruffneckk::remote_stash::HasUsableSize;
using ruffneckk::remote_stash::PlaceDesktopFooterLeft;
using ruffneckk::remote_stash::WidgetRect;

constexpr std::uint32_t SupportedBuild = 92777;

constexpr std::uintptr_t ConfigurePlayerInventoryRva = 0x22BA70;
constexpr std::uintptr_t DispatchUiMessageRva = 0x843D90;
constexpr std::uintptr_t ClientUiPacket77Rva = 0x12DBC0;
constexpr std::uintptr_t ClientUiPacket77TargetRva = 0x1F0AB0;
constexpr std::uintptr_t FindWidgetRva = 0x856220;
constexpr std::uintptr_t GetWidgetRectRva = 0x8562A0;

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
constexpr std::array<std::uint8_t, 16> ClientUiPacket77Expected{
    0xE9, 0xEB, 0x2E, 0x0C, 0x00, 0xCC, 0xCC, 0xCC,
    0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC
};
constexpr std::array<std::uint8_t, 32> ClientUiPacket77TargetExpected{
    0x48, 0x83, 0xEC, 0x28, 0x0F, 0xB6, 0x41, 0x01,
    0x3C, 0x10, 0x74, 0x31, 0x3C, 0x14, 0x75, 0x67,
    0xB9, 0x18, 0x00, 0x00, 0x00, 0xE8, 0x36, 0xDA,
    0xED, 0xFF, 0x84, 0xC0, 0x74, 0x0A, 0x33, 0xD2
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
using ClientUiPacket77Fn = void(__fastcall*)(const std::uint8_t* packet) noexcept;
using FindWidgetFn = void*(__fastcall*)(void* panel, const char* name) noexcept;
using GetWidgetRectFn = WidgetRect*(__fastcall*)(
    void* widget,
    WidgetRect* rectOut
) noexcept;
using SetWidgetBoolFn = void(__fastcall*)(void* widget, bool value) noexcept;
using UiMessageInterceptorFn = bool(__fastcall*)(void* message) noexcept;
using RegisterUiMessageInterceptorFn = bool(__cdecl*)(UiMessageInterceptorFn) noexcept;
using UnregisterUiMessageInterceptorFn = void(__cdecl*)(UiMessageInterceptorFn) noexcept;

constexpr wchar_t UiMessageBrokerModule[] = L"BulkSkillPointAllocation.dll";
constexpr char RegisterUiMessageInterceptorExport[] =
    "RuffneckkRegisterUiMessageInterceptor";
constexpr char UnregisterUiMessageInterceptorExport[] =
    "RuffneckkUnregisterUiMessageInterceptor";

const D2RL::PluginContext* Context{};
std::uint8_t* Base{};
ConfigurePlayerInventoryFn OriginalConfigurePlayerInventory{};
DispatchUiMessageFn OriginalDispatchUiMessage{};
ClientUiPacket77Fn ClientUiPacket77{};
FindWidgetFn FindWidget{};
GetWidgetRectFn GetWidgetRect{};

std::atomic<std::uint64_t> DynamicPlacements{};
std::atomic<std::uint64_t> PlacementFailures{};
std::atomic<std::uint64_t> OpenRequests{};
std::atomic_bool PlacementSuccessReported{};
std::atomic_bool PlacementFailureReported{};
std::atomic<void*> DiagnosticPanel{};
std::atomic<void*> DiagnosticButton{};
std::atomic_bool UsingUiMessageBroker{};
UnregisterUiMessageInterceptorFn BrokerUnregister{};

constexpr D2RL::PluginInfo Info{
    .infoSize = D2RL::PluginInfoSize,
    .apiVersion = D2RL_PLUGIN_API_VERSION,
    .id = "remote-stash",
    .name = "Remote Stash",
    .version = "0.1.5",
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

bool ValidateRuntime() noexcept {
    return Matches(ConfigurePlayerInventoryRva, ConfigurePlayerInventoryExpected)
        && Matches(ClientUiPacket77Rva, ClientUiPacket77Expected)
        && Matches(ClientUiPacket77TargetRva, ClientUiPacket77TargetExpected)
        && Matches(FindWidgetRva, FindWidgetExpected)
        && Matches(GetWidgetRectRva, GetWidgetRectExpected);
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
    DiagnosticPanel.store(nullptr, std::memory_order_release);
    DiagnosticButton.store(nullptr, std::memory_order_release);
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
    DiagnosticPanel.store(panel, std::memory_order_release);
    DiagnosticButton.store(button, std::memory_order_release);
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
    const auto diagnosticPanel = DiagnosticPanel.load(std::memory_order_acquire);
    const auto diagnosticButton = DiagnosticButton.load(std::memory_order_acquire);
    if (!diagnosticPanel || !diagnosticButton) return false;

    const auto* expectedMessage = static_cast<const std::uint8_t*>(diagnosticButton)
        + ButtonOnClickMessageOffset;
    if (message != expectedMessage) return false;

    __try {
        return FindWidget(diagnosticPanel, "remote_stash") == diagnosticButton;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool __fastcall InterceptUiMessage(void* message) noexcept {
    if (!IsCurrentRemoteStashMessage(message)) return false;

    constexpr std::array<std::uint8_t, 2> packet{0x77, 0x10};
    __try {
        ClientUiPacket77(packet.data());
        const auto count = OpenRequests.fetch_add(1, std::memory_order_relaxed) + 1;
        if (Context) {
            char message[190]{};
            std::snprintf(
                message,
                sizeof(message),
                "RemoteStash: remote button message consumed; native client open-stash "
                "UI action dispatched (requests=%llu).",
                static_cast<unsigned long long>(count)
            );
            Context->LogInfo(message);
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        if (Context) {
            Context->LogError("RemoteStash: native client open-stash UI action failed.");
        }
    }
    return true;
}

void __fastcall HookDispatchUiMessage(void* message) noexcept {
    if (InterceptUiMessage(message)) return;
    OriginalDispatchUiMessage(message);
}

auto Status(D2R::Game::Client*, const D2RL::ConsoleCommandContext* command, void*) noexcept
    -> D2RL::ConsoleCommandResult {
    if (!command || !command->plugin) return D2RL::ConsoleCommandResult::Failed;
    char message[320]{};
    std::snprintf(
        message,
        sizeof(message),
        "RemoteStash 0.1.5: placements=%llu; placementFailures=%llu; clientOpenRequests=%llu; serverBankSession=false (diagnostic prototype).",
        static_cast<unsigned long long>(DynamicPlacements.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(PlacementFailures.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(OpenRequests.load(std::memory_order_relaxed))
    );
    command->plugin->WriteConsoleMessage(message);
    return D2RL::ConsoleCommandResult::Handled;
}
} // namespace

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
    PlacementSuccessReported.store(false, std::memory_order_relaxed);
    PlacementFailureReported.store(false, std::memory_order_relaxed);
    DiagnosticPanel.store(nullptr, std::memory_order_relaxed);
    DiagnosticButton.store(nullptr, std::memory_order_relaxed);
    UsingUiMessageBroker.store(false, std::memory_order_relaxed);
    BrokerUnregister = nullptr;

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
    ClientUiPacket77 = At<ClientUiPacket77Fn>(ClientUiPacket77Rva);

    const auto brokerModule = GetModuleHandleW(UiMessageBrokerModule);
    const auto registerBroker = brokerModule
        ? reinterpret_cast<RegisterUiMessageInterceptorFn>(
            GetProcAddress(brokerModule, RegisterUiMessageInterceptorExport)
        )
        : nullptr;
    const auto unregisterBroker = brokerModule
        ? reinterpret_cast<UnregisterUiMessageInterceptorFn>(
            GetProcAddress(brokerModule, UnregisterUiMessageInterceptorExport)
        )
        : nullptr;
    if (registerBroker && unregisterBroker && registerBroker(InterceptUiMessage)) {
        BrokerUnregister = unregisterBroker;
        UsingUiMessageBroker.store(true, std::memory_order_release);
    } else if (!Matches(DispatchUiMessageRva, DispatchUiMessageExpected)) {
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
        if (BrokerUnregister) BrokerUnregister(InterceptUiMessage);
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

    if (!context->RegisterConsoleCommand(
            "remote-stash",
            Status,
            "Show Remote Stash prototype status and counters."
        )) {
        context->LogWarn("RemoteStash: status command could not be registered.");
    }

    context->LogInfo(
        UsingUiMessageBroker.load(std::memory_order_acquire)
            ? "RemoteStash 0.1.5 active for D2R 3.2.92777; dynamic desktop placement "
              "and local native open-stash UI action use the shared plugin-misc UI "
              "dispatcher (no server bank session yet)."
            : "RemoteStash 0.1.5 active for D2R 3.2.92777; dynamic desktop placement "
              "and local native open-stash UI action use the standalone UI dispatcher "
              "(no server bank session yet)."
    );
    return true;
}

D2RL_PLUGIN_EXPORT void D2RLoaderUnloadPlugin() noexcept {
    if (UsingUiMessageBroker.exchange(false, std::memory_order_acq_rel)) {
        const auto brokerModule = GetModuleHandleW(UiMessageBrokerModule);
        if (brokerModule && BrokerUnregister) {
            BrokerUnregister(InterceptUiMessage);
        }
    }
    BrokerUnregister = nullptr;
    Context = nullptr;
}
