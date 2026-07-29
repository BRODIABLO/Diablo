#define NOMINMAX
#include <Windows.h>
#include <D2RLPlugin/api.h>

#include "book_of_lore_config.hpp"
#include "book_of_lore_state.hpp"

#include <array>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace ruffneckk::book_of_lore;

constexpr std::uint32_t SupportedBuild = 92777;
constexpr std::uintptr_t ClientPacket27Rva = 0x19A630;
constexpr std::uintptr_t ShowScrollMessageRva = 0x197BF0;
constexpr std::uintptr_t ScrollLocalizationCallRva = 0x197C5F;
constexpr std::uintptr_t GetLocalizedStringRva = 0x5F4A50;
constexpr std::uint16_t TowerTomeStringId = 127;

constexpr std::array<std::uint8_t, 32> ClientPacket27Expected{
    0x48, 0x89, 0x5C, 0x24, 0x10, 0x57, 0x48, 0x83,
    0xEC, 0x50, 0x48, 0x8B, 0x05, 0x87, 0x0C, 0x83,
    0x02, 0x48, 0x33, 0xC4, 0x48, 0x89, 0x44, 0x24,
    0x40, 0x0F, 0xB6, 0x41, 0x01, 0x48, 0x8B, 0xD9,
};
constexpr std::array<std::uint8_t, 64> ShowScrollMessageExpected{
    0x48, 0x89, 0x5C, 0x24, 0x10, 0x48, 0x89, 0x74,
    0x24, 0x18, 0x48, 0x89, 0x7C, 0x24, 0x20, 0x55,
    0x48, 0x8D, 0xAC, 0x24, 0xE0, 0xFD, 0xFF, 0xFF,
    0x48, 0x81, 0xEC, 0x20, 0x03, 0x00, 0x00, 0x48,
    0x8B, 0x05, 0xB2, 0x36, 0x83, 0x02, 0x48, 0x33,
    0xC4, 0x48, 0x89, 0x85, 0x10, 0x02, 0x00, 0x00,
    0x0F, 0xB7, 0xF9, 0xE8, 0xA8, 0x36, 0xEF, 0xFF,
    0x8B, 0xD8, 0xE8, 0x11, 0x59, 0xEF, 0xFF, 0x33,
};
constexpr std::array<std::uint8_t, 5> ScrollLocalizationCallExpected{
    0xE8, 0xEC, 0xCD, 0x45, 0x00,
};
constexpr std::array<std::uint8_t, 14> GetLocalizedStringExpected{
    0x80, 0x3D, 0x05, 0xAE, 0xCF, 0x02, 0x00,
    0x48, 0x8D, 0x05, 0xDA, 0x96, 0xDB, 0x01,
};

using ClientPacket27Fn = void(__fastcall*)(const std::uint8_t* packet) noexcept;
using GetLocalizedStringFn = const char*(__fastcall*)(std::uint16_t stringId) noexcept;

const D2RL::PluginContext* Context{};
std::uint8_t* Base{};
Config Settings{};
SessionSelections Selections{};
std::string LoadedConfigPath{"built-in defaults"};
std::vector<std::string> ScrollTexts{};
ClientPacket27Fn OriginalClientPacket27{};
GetLocalizedStringFn GetLocalizedString{};
void* LocalizationRelay{};
std::uint64_t LocalizationRelayRva{};
thread_local const std::string* ActiveScrollText{};
std::atomic<std::uint64_t> PrototypeMessagesSeen{};
std::atomic<std::uint64_t> PrototypeSubstitutions{};

constexpr D2RL::PluginInfo Info{
    .infoSize = D2RL::PluginInfoSize,
    .apiVersion = D2RL_PLUGIN_API_VERSION,
    .id = "book-of-lore",
    .name = "Book of Lore",
    .version = "0.2.0",
    .author = "RuffnecKk",
    .description = "Reveals configured lore when a world book is opened.",
    .flags = D2RL::PluginFlags::NativeHooks,
};

template<std::size_t Size>
bool Matches(
        std::uintptr_t rva,
        const std::array<std::uint8_t, Size>& expected) noexcept {
    return Base
        && std::memcmp(Base + rva, expected.data(), expected.size()) == 0;
}

void* AllocateNear(void* hint, std::size_t size) noexcept {
    SYSTEM_INFO systemInfo{};
    GetSystemInfo(&systemInfo);
    const auto granularity = static_cast<std::uintptr_t>(
        systemInfo.dwAllocationGranularity
    );
    const auto base = reinterpret_cast<std::uintptr_t>(hint)
        & ~(granularity - 1);

    for (std::uintptr_t delta = granularity;
         delta < 0x70000000ULL;
         delta += granularity) {
        if (base > std::numeric_limits<std::uintptr_t>::max() - delta) break;
        if (auto* memory = VirtualAlloc(
                reinterpret_cast<void*>(base + delta),
                size,
                MEM_COMMIT | MEM_RESERVE,
                PAGE_EXECUTE_READWRITE
            )) {
            return memory;
        }
    }
    return nullptr;
}

const char* __fastcall ResolveScrollLocalization(
        std::uint16_t stringId) noexcept {
    if (stringId == TowerTomeStringId && ActiveScrollText) {
        PrototypeSubstitutions.fetch_add(1, std::memory_order_relaxed);
        return ActiveScrollText->c_str();
    }
    return GetLocalizedString ? GetLocalizedString(stringId) : nullptr;
}

class ScopedScrollText final {
public:
    explicit ScopedScrollText(const std::string* text) noexcept
        : previous_(ActiveScrollText) {
        ActiveScrollText = text;
    }

    ~ScopedScrollText() {
        ActiveScrollText = previous_;
    }

    ScopedScrollText(const ScopedScrollText&) = delete;
    ScopedScrollText& operator=(const ScopedScrollText&) = delete;

private:
    const std::string* previous_{};
};

void __fastcall HandleClientPacket27(const std::uint8_t* packet) noexcept {
    if (!OriginalClientPacket27) return;
    if (ScrollTexts.empty()
        || !IsObjectScrollMessage(packet, TowerTomeStringId)) {
        OriginalClientPacket27(packet);
        return;
    }

    PrototypeMessagesSeen.fetch_add(1, std::memory_order_relaxed);
    const ScopedScrollText scope(&ScrollTexts.front());
    OriginalClientPacket27(packet);
}

bool InstallLocalizationCallRedirect() noexcept {
    constexpr std::size_t RelaySize = 14;
    LocalizationRelay = AllocateNear(Base + ScrollLocalizationCallRva, RelaySize);
    if (!LocalizationRelay) return false;

    auto* relay = static_cast<std::uint8_t*>(LocalizationRelay);
    relay[0] = 0xFF;
    relay[1] = 0x25;
    relay[2] = 0x00;
    relay[3] = 0x00;
    relay[4] = 0x00;
    relay[5] = 0x00;
    const auto target = reinterpret_cast<std::uint64_t>(ResolveScrollLocalization);
    std::memcpy(relay + 6, &target, sizeof(target));
    FlushInstructionCache(GetCurrentProcess(), LocalizationRelay, RelaySize);

    DWORD previousProtection{};
    if (!VirtualProtect(
            LocalizationRelay,
            RelaySize,
            PAGE_EXECUTE_READ,
            &previousProtection
        )) {
        VirtualFree(LocalizationRelay, 0, MEM_RELEASE);
        LocalizationRelay = nullptr;
        return false;
    }

    const auto relayAddress = reinterpret_cast<std::uintptr_t>(LocalizationRelay);
    const auto baseAddress = reinterpret_cast<std::uintptr_t>(Base);
    if (relayAddress < baseAddress) {
        VirtualFree(LocalizationRelay, 0, MEM_RELEASE);
        LocalizationRelay = nullptr;
        return false;
    }
    LocalizationRelayRva = relayAddress - baseAddress;
    if (!Context->PatchCallRel32(
        ScrollLocalizationCallRva,
        ScrollLocalizationCallExpected.data(),
        static_cast<std::uint32_t>(ScrollLocalizationCallExpected.size()),
        LocalizationRelayRva
    )) {
        VirtualFree(LocalizationRelay, 0, MEM_RELEASE);
        LocalizationRelay = nullptr;
        LocalizationRelayRva = 0;
        return false;
    }
    return true;
}

bool InstallPrototypeHooks() noexcept {
    if (!Matches(ClientPacket27Rva, ClientPacket27Expected)
        || !Matches(ShowScrollMessageRva, ShowScrollMessageExpected)
        || !Matches(ScrollLocalizationCallRva, ScrollLocalizationCallExpected)
        || !Matches(GetLocalizedStringRva, GetLocalizedStringExpected)) {
        Context->LogError(
            "BookOfLore: 92777 packet or scroll-localization signature mismatch; plugin refused."
        );
        return false;
    }

    GetLocalizedString = reinterpret_cast<GetLocalizedStringFn>(
        Base + GetLocalizedStringRva
    );
    if (!InstallLocalizationCallRedirect()) {
        Context->LogError("BookOfLore: native scroll localization redirect failed.");
        return false;
    }
    if (!Context->InstallInlineHook(
            ClientPacket27Rva,
            ClientPacket27Expected.data(),
            static_cast<std::uint32_t>(ClientPacket27Expected.size()),
            HandleClientPacket27,
            &OriginalClientPacket27
        )) {
        Context->LogError("BookOfLore: packet-0x27 hook failed.");
        return false;
    }
    return true;
}

bool LoadConfig() noexcept {
    try {
        std::optional<std::filesystem::path> modDirectory;
        if (Context && Context->modDirectory && Context->modDirectory[0] != L'\0') {
            modDirectory = std::filesystem::path(Context->modDirectory);
        }
        auto loaded = LoadConfiguration(modDirectory);
        Settings = std::move(loaded.config);
        LoadedConfigPath = loaded.path ? loaded.path->string() : "built-in defaults";
        ScrollTexts.clear();
        ScrollTexts.reserve(Settings.messages.size());
        for (const auto& message : Settings.messages) {
            ScrollTexts.push_back(BuildScrollText(message));
        }
        if (Settings.enabled && ScrollTexts.empty()) {
            throw std::invalid_argument(
                "enabled configuration requires at least one message"
            );
        }
        return true;
    } catch (const std::exception& exception) {
        const auto message = std::string("BookOfLore: ") + exception.what() + ".";
        if (Context) Context->LogError(message.c_str());
        return false;
    }
}

auto Status(
        D2R::Game::Client*,
        const D2RL::ConsoleCommandContext* command,
        void*) noexcept -> D2RL::ConsoleCommandResult {
    if (!command || !command->plugin) return D2RL::ConsoleCommandResult::Failed;
    char message[384]{};
    std::snprintf(
        message,
        sizeof(message),
        "BookOfLore 0.2.0: TOML config=%s; enabled=%s; messages=%zu; prototype packet27=%llu; substitutions=%llu; server selection=not installed.",
        LoadedConfigPath.c_str(),
        Settings.enabled ? "true" : "false",
        Settings.messages.size(),
        static_cast<unsigned long long>(
            PrototypeMessagesSeen.load(std::memory_order_relaxed)
        ),
        static_cast<unsigned long long>(
            PrototypeSubstitutions.load(std::memory_order_relaxed)
        )
    );
    command->plugin->WriteConsoleMessage(message);
    return D2RL::ConsoleCommandResult::Handled;
}
} // namespace

D2RL_PLUGIN_EXPORT auto D2RLoaderGetPluginInfo() noexcept -> const D2RL::PluginInfo* {
    return &Info;
}

D2RL_PLUGIN_EXPORT auto D2RLoaderLoadPlugin(
        const D2RL::PluginContext* context) noexcept -> bool {
    if (!context) return false;
    Context = context;
    Base = reinterpret_cast<std::uint8_t*>(context->exeBase);
    Selections.Clear();
    PrototypeMessagesSeen.store(0, std::memory_order_relaxed);
    PrototypeSubstitutions.store(0, std::memory_order_relaxed);
    if (!LoadConfig()) return false;
    if (!Base) {
        context->LogError("BookOfLore: D2R executable base is unavailable.");
        return false;
    }
    if (context->modDataVersionBuild != 0
        && context->modDataVersionBuild != SupportedBuild) {
        context->LogError("BookOfLore: only D2R build 92777 is supported.");
        return false;
    }
    if (Settings.enabled && !InstallPrototypeHooks()) return false;

    if (!context->RegisterConsoleCommand(
            "book-of-lore",
            Status,
            "Show Book of Lore configuration and implementation status."
        )) {
        context->LogWarn("BookOfLore: status command could not be registered.");
    }
    context->LogInfo(Settings.enabled
        ? "BookOfLore 0.2.0 enabled its fail-closed Tower Tome client witness; server selection is not installed."
        : "BookOfLore 0.2.0 loaded with runtime hooks disabled by configuration."
    );
    return true;
}

D2RL_PLUGIN_EXPORT void D2RLoaderUnloadPlugin() noexcept {
    Selections.Clear();
    ScrollTexts.clear();
    ActiveScrollText = nullptr;
    OriginalClientPacket27 = nullptr;
    GetLocalizedString = nullptr;
    // D2RLoader restores registered patches. Keep the relay allocated until
    // process exit so an in-flight scroll call can never target freed code.
    LocalizationRelay = nullptr;
    LocalizationRelayRva = 0;
    Base = nullptr;
    Context = nullptr;
}
