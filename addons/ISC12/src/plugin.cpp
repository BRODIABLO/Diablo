#include <D2RLPlugin/api.h>

#include "isc12_config.hpp"
#include "isc12_contract.hpp"
#include "isc12_loader.hpp"
#include "isc12_native_sites.hpp"

#include <Windows.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cwchar>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>

namespace RuffnecKk::ISC12 {
namespace {

using namespace ruffneckk::isc12;

constexpr wchar_t ConfigFileName[] = L"ruffneckk-isc12.toml";
constexpr wchar_t ProcessMutexNameFormat[] =
    L"Local\\RuffnecKk.ISC12.%lu";

const D2RL::PluginContext* Context{};
const D2RL::DiagnosticsServiceV1* DiagnosticsService{};
std::uint8_t* Base{};
std::size_t ImageSize{};
Config Settings{};
std::string LoadedConfigPath{"embedded defaults"};
std::string RuntimeBuild{"<unavailable>"};
HANDLE ProcessMutex{};
std::atomic_bool Operational{};

constexpr D2RL::PluginInfo Info{
    .infoSize = D2RL::PluginInfoSize,
    .apiVersion = D2RL_PLUGIN_API_VERSION,
    .id = "ruffneckk-isc12",
    .name = "ISC12",
    .version = "0.2.0",
    .author = "RuffnecKk",
    .description = "Supports up to 4,095 item stat definitions for overhaul mods.",
    .flags = D2RL::PluginFlags::Shared | D2RL::PluginFlags::NativeHooks,
};

auto ReleaseProcessMutex() noexcept -> void {
    if (!ProcessMutex) return;
    CloseHandle(ProcessMutex);
    ProcessMutex = nullptr;
}

auto AcquireProcessMutex() noexcept -> bool {
    std::array<wchar_t, 96> mutexName{};
    const auto nameLength = std::swprintf(
        mutexName.data(),
        mutexName.size(),
        ProcessMutexNameFormat,
        static_cast<unsigned long>(GetCurrentProcessId()));
    if (nameLength <= 0
            || static_cast<std::size_t>(nameLength) >= mutexName.size()) {
        Context->LogError(
            "ISC12: process mutex name creation failed; plugin refused.");
        return false;
    }
    ProcessMutex = CreateMutexW(nullptr, FALSE, mutexName.data());
    if (!ProcessMutex) {
        Context->LogError("ISC12: process mutex creation failed; plugin refused.");
        return false;
    }
    if (GetLastError() != ERROR_ALREADY_EXISTS) return true;
    Context->LogError(
        "ISC12: another global or mod-local instance already owns this process; plugin refused.");
    ReleaseProcessMutex();
    return false;
}

auto IsExecutableProtection(DWORD value) noexcept -> bool {
    const auto protection = value & 0xFFU;
    return protection == PAGE_EXECUTE
        || protection == PAGE_EXECUTE_READ
        || protection == PAGE_EXECUTE_READWRITE
        || protection == PAGE_EXECUTE_WRITECOPY;
}

auto IsExecutableRange(const void* address, std::size_t size) noexcept -> bool {
    if (!address || size == 0) return false;
    const auto start = reinterpret_cast<std::uintptr_t>(address);
    if (start > std::numeric_limits<std::uintptr_t>::max() - size) return false;
    const auto end = start + size;
    auto cursor = start;
    while (cursor < end) {
        MEMORY_BASIC_INFORMATION info{};
        if (VirtualQuery(
                reinterpret_cast<const void*>(cursor),
                &info,
                sizeof(info)) != sizeof(info)
                || info.State != MEM_COMMIT
                || (info.Protect & (PAGE_GUARD | PAGE_NOACCESS)) != 0
                || !IsExecutableProtection(info.Protect)) {
            return false;
        }
        const auto regionStart = reinterpret_cast<std::uintptr_t>(
            info.BaseAddress);
        if (regionStart > std::numeric_limits<std::uintptr_t>::max()
                - info.RegionSize) {
            return false;
        }
        const auto regionEnd = regionStart + info.RegionSize;
        if (regionEnd <= cursor) return false;
        cursor = std::min(regionEnd, end);
    }
    return true;
}

auto InitializeImageBounds() noexcept -> bool {
    if (!Base) return false;
    __try {
        const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(Base);
        if (dos->e_magic != IMAGE_DOS_SIGNATURE
                || dos->e_lfanew <= 0 || dos->e_lfanew > 0x1000) {
            return false;
        }
        const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(
            Base + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE
                || nt->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64
                || nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC
                || nt->OptionalHeader.SizeOfImage == 0) {
            return false;
        }
        ImageSize = nt->OptionalHeader.SizeOfImage;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        ImageSize = 0;
        return false;
    }
}

auto Matches(const NativePattern& pattern) noexcept -> bool {
    if (!Base || pattern.bytes.empty()
            || pattern.bytes.size() != pattern.mask.size()
            || pattern.rva > ImageSize
            || pattern.bytes.size() > ImageSize - pattern.rva
            || !IsExecutableRange(Base + pattern.rva, pattern.bytes.size())) {
        return false;
    }
    __try {
        for (std::size_t index{}; index < pattern.bytes.size(); ++index) {
            if (pattern.mask[index] != 0
                    && Base[pattern.rva + index] != pattern.bytes[index]) {
                return false;
            }
        }
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

auto QueryDiagnosticsService() noexcept -> bool {
    const auto result = Context->QueryService(
        D2RL::ServiceId::Diagnostics,
        D2RL::DiagnosticsServiceV1Version,
        &DiagnosticsService);
    if (result != D2RL::ServiceQueryResult::Success) {
        DiagnosticsService = nullptr;
        Context->LogWarn(
            "ISC12: DiagnosticsService v1 is unavailable; strict byte fingerprints remain mandatory.");
        return true;
    }
    if (!D2RL::HasDiagnosticsServiceV1Field(
            DiagnosticsService,
            D2RL::DiagnosticsServiceV1RequiredSize)
            || DiagnosticsService->queryHookStatus == nullptr) {
        Context->LogError(
            "ISC12: DiagnosticsService v1 returned an invalid contract; plugin refused.");
        DiagnosticsService = nullptr;
        return false;
    }
    return true;
}

auto ValidateOwnership(const NativePattern& pattern) noexcept -> bool {
    if (!DiagnosticsService) return true;
    const D2RL::Diagnostics::HookQuery query{
        .structSize = D2RL::Diagnostics::HookQuerySize,
        .flags = 0,
        .rva = pattern.rva,
        .expected = pattern.bytes.data(),
        .expectedSize = static_cast<std::uint32_t>(pattern.bytes.size()),
        .reserved = 0,
    };
    D2RL::Diagnostics::HookStatus status{
        .structSize = D2RL::Diagnostics::HookStatusSize,
    };
    const auto result = DiagnosticsService->queryHookStatus(
        Context, &query, &status);
    return result == D2RL::Diagnostics::Result::Success
        && status.structSize >= D2RL::Diagnostics::HookStatusRequiredSize
        && status.state == D2RL::Diagnostics::ModificationState::Unchanged
        && status.ownerCount == 0;
}

auto ValidateFoundationFingerprint() noexcept -> bool {
    for (const auto& pattern : FoundationPatterns) {
        if (!Matches(pattern)) {
            char message[256]{};
            std::snprintf(
                message,
                sizeof(message),
                "ISC12: native foundation mismatch at %s (RVA 0x%llX); plugin refused.",
                pattern.id,
                static_cast<unsigned long long>(pattern.rva));
            Context->LogError(message);
            return false;
        }
        if (!ValidateOwnership(pattern)) {
            char message[256]{};
            std::snprintf(
                message,
                sizeof(message),
                "ISC12: native foundation surface %s is already owned; plugin refused.",
                pattern.id);
            Context->LogError(message);
            return false;
        }
    }
    return true;
}

auto ConfigCandidates() -> std::vector<std::filesystem::path> {
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
    return BuildConfigCandidates(
        activeModConfigDirectory,
        scopeConfigDirectory,
        globalConfigDirectory,
        ConfigFileName);
}

auto LoadConfig() noexcept -> bool {
    Settings = {};
    LoadedConfigPath = "embedded defaults";
    for (const auto& path : ConfigCandidates()) {
        std::error_code statusError;
        const auto status = std::filesystem::status(path, statusError);
        if (statusError || !std::filesystem::exists(status)) continue;
        if (!std::filesystem::is_regular_file(status)) {
            Context->LogError(
                "ISC12: configuration path exists but is not a regular file; plugin refused.");
            return false;
        }
        try {
            std::ifstream input(path, std::ios::binary);
            if (!input.is_open()) {
                throw std::runtime_error("configuration file cannot be opened");
            }
            const std::string text{
                std::istreambuf_iterator<char>(input),
                std::istreambuf_iterator<char>()};
            Config parsed{};
            std::string error;
            if (!ParseToml(text, parsed, error)) {
                throw std::invalid_argument(error);
            }
            Settings = parsed;
            LoadedConfigPath = path.string();
            return true;
        } catch (const std::exception& exception) {
            const auto message = std::string("ISC12: invalid ")
                + path.string() + " (" + exception.what() + ").";
            Context->LogError(message.c_str());
            return false;
        }
    }
    Context->LogWarn(
        "ISC12: no TOML was found; embedded defaults are active.");
    return true;
}

} // namespace

D2RL_PLUGIN_EXPORT auto D2RLoaderGetPluginInfo() noexcept
        -> const D2RL::PluginInfo* {
    return &Info;
}

D2RL_PLUGIN_EXPORT auto D2RLoaderLoadPlugin(
        const D2RL::PluginContext* context) noexcept -> bool {
    if (!D2RL::HasContext(context)
            || context->apiVersion != D2RL_PLUGIN_API_VERSION) {
        return false;
    }
    Context = context;
    Base = reinterpret_cast<std::uint8_t*>(context->exeBase);
    Operational.store(false, std::memory_order_release);
    DiagnosticsService = nullptr;
    ImageSize = 0;
    if (!Base || !LoadConfig()) return false;

    const auto* buildName = D2RL::GetBuildName(context);
    RuntimeBuild = buildName && buildName[0] != '\0'
        ? buildName : "<unavailable>";
    if (!Settings.enabled) {
        context->LogInfo(
            "ISC12 0.2.0 by RuffnecKk loaded disabled; hooks=0; writes=0.");
        return true;
    }
    NativePublicationQuiescenceLease publicationLease;
    if (!publicationLease.IsHeld()) {
        context->LogError(
            "ISC12: enabled=true refused with zero native writes because the "
            "pinned D2RLoader SDK exposes no loader-owned native publication "
            "quiescence service.");
        return false;
    }
    if (!AcquireProcessMutex()) return false;

    char identity[256]{};
    std::snprintf(
        identity,
        sizeof(identity),
        "ISC12: observed D2R build-name=%s; validating the native foundation without a version allowlist.",
        RuntimeBuild.c_str());
    context->LogInfo(identity);

    if (!InitializeImageBounds() || !QueryDiagnosticsService()
            || !ValidateFoundationFingerprint()) {
        ReleaseProcessMutex();
        return false;
    }

    std::string loaderError;
    if (!PrepareLoaderExtension(
            context,
            Base,
            ImageSize,
            Settings.diagnostics,
            loaderError)) {
        const auto message = std::string("ISC12: loader preparation failed (")
            + loaderError + ").";
        context->LogError(message.c_str());
        ShutdownLoaderExtension();
        ReleaseProcessMutex();
        return false;
    }
    const auto installResult =
        InstallLoaderExtension(publicationLease, loaderError);
    if (installResult == LoaderInstallResult::QuiescenceRequired) {
        const auto message = std::string(
            "ISC12: loader publication refused before the first native write (")
            + loaderError + ").";
        context->LogError(message.c_str());
        ShutdownLoaderExtension();
        ReleaseProcessMutex();
        return false;
    }
    if (installResult == LoaderInstallResult::FailedBeforeMutation) {
        const auto message = std::string("ISC12: loader install failed (")
            + loaderError + ").";
        context->LogError(message.c_str());
        ShutdownLoaderExtension();
        ReleaseProcessMutex();
        return false;
    }
    if (installResult
            == LoaderInstallResult::PartialCommitColdRestartRequired) {
        const auto message = std::string(
            "ISC12: loader entered partial-commit-cold-restart-required (")
            + loaderError
            + "); the cap is treated as potentially modified, the guarded tail "
              "remains resident, and a cold restart is mandatory.";
        context->LogError(message.c_str());
        Operational.store(false, std::memory_order_release);
        return true;
    }

    Operational.store(true, std::memory_order_release);
    char message[384]{};
    std::snprintf(
        message,
        sizeof(message),
        "ISC12 0.2.0 loader active for observed D2R %s; max-stat-id=%u; sites=%zu; hooks=%zu; writes=%zu; scope=%s; TOML=%s.",
        RuntimeBuild.c_str(),
        static_cast<unsigned>(MaximumStatId),
        FoundationPatterns.size(),
        InstalledHookCount,
        InstalledPatchCount,
        context->loadScope == D2RL::LoadScope::Mod ? "mod-local" : "global",
        LoadedConfigPath.c_str());
    context->LogInfo(message);
    if (Settings.diagnostics) {
        context->LogInfo(
            "ISC12 diagnostics: the guarded loader/DescFunc transaction is active; save and network codecs remain disabled.");
    }
    return true;
}

D2RL_PLUGIN_EXPORT void D2RLoaderUnloadPlugin() noexcept {
    Operational.store(false, std::memory_order_release);
    ShutdownLoaderExtension();
    ReleaseProcessMutex();
}

} // namespace RuffnecKk::ISC12
