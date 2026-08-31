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
#include <string_view>

namespace RuffnecKk::ISC12 {
namespace {

using namespace ruffneckk::isc12;

constexpr wchar_t ConfigFileName[] = L"ruffneckk-isc12.toml";
constexpr wchar_t ProcessMutexNameFormat[] =
    L"Local\\RuffnecKk.ISC12.%lu";

const D2RL::PluginContext* Context{};
const D2RL::DiagnosticsServiceV1* DiagnosticsService{};
const D2RL::LifecycleServiceV1* LifecycleService{};
const D2RL::DataTableServiceV1* DataTableService{};
std::atomic<D2RL::Lifecycle::ListenerHandle> DataTablesLoadedListenerHandle{
    D2RL::Lifecycle::InvalidHandle};
std::uint8_t* Base{};
std::size_t ImageSize{};
Config Settings{};
std::string LoadedConfigPath{"embedded defaults"};
std::string RuntimeBuild{"<unavailable>"};
HANDLE ProcessMutex{};

enum class SchemaLifecycleState : std::uint8_t {
    Inactive,
    Registered,
    Active,
    Stopping,
};

std::atomic<SchemaLifecycleState> SchemaLifecycle{
    SchemaLifecycleState::Inactive};

struct InitialLoadPublicationState {
    std::atomic<DWORD> threadId{};
    std::atomic_bool active{};
};

InitialLoadPublicationState InitialLoadPublication{};

auto ValidateInitialLoadPublication(void*) noexcept -> bool {
    return InitialLoadPublication.active.load(std::memory_order_acquire)
        && InitialLoadPublication.threadId.load(std::memory_order_acquire)
            == GetCurrentThreadId();
}

class InitialLoadPublicationWindow final {
public:
    InitialLoadPublicationWindow() noexcept {
        bool expected = false;
        acquired_ = InitialLoadPublication.active.compare_exchange_strong(
            expected, true, std::memory_order_acq_rel);
        if (!acquired_) return;
        InitialLoadPublication.threadId.store(
            GetCurrentThreadId(), std::memory_order_release);
        lease_ = NativePublicationLeaseView::ForInitialLoad(
            nullptr, &ValidateInitialLoadPublication);
    }

    InitialLoadPublicationWindow(const InitialLoadPublicationWindow&) = delete;
    auto operator=(const InitialLoadPublicationWindow&)
        -> InitialLoadPublicationWindow& = delete;

    ~InitialLoadPublicationWindow() noexcept {
        if (!acquired_) return;
        InitialLoadPublication.active.store(false, std::memory_order_release);
        InitialLoadPublication.threadId.store(0, std::memory_order_release);
    }

    [[nodiscard]] auto Lease() const noexcept
            -> const NativePublicationLeaseView& {
        return lease_;
    }

private:
    bool acquired_{};
    NativePublicationLeaseView lease_{};
};

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

auto QuerySchemaLifecycleServices() noexcept -> bool {
    LifecycleService = nullptr;
    DataTableService = nullptr;
    const auto lifecycleResult = Context->QueryService(
        D2RL::ServiceId::Lifecycle,
        D2RL::LifecycleServiceV1Version,
        &LifecycleService);
    if (lifecycleResult != D2RL::ServiceQueryResult::Success
            || !D2RL::HasLifecycleServiceV1Field(
                LifecycleService,
                D2RL::LifecycleServiceV1RequiredSize)
            || !LifecycleService->registerDataTablesLoadedListener
            || !LifecycleService->unregisterDataTablesLoadedListener) {
        Context->LogError(
            "ISC12: LifecycleService v1 cannot provide the authoritative "
            "DataTablesLoaded boundary; plugin refused before native writes.");
        LifecycleService = nullptr;
        return false;
    }

    const auto dataTableResult = Context->QueryService(
        D2RL::ServiceId::DataTable,
        D2RL::DataTableServiceV1Version,
        &DataTableService);
    if (dataTableResult != D2RL::ServiceQueryResult::Success
            || !D2RL::HasDataTableServiceV1Field(
                DataTableService,
                D2RL::DataTableServiceV1RequiredSize)
            || !DataTableService->getTable) {
        Context->LogError(
            "ISC12: DataTableService v1 cannot identify the authoritative "
            "RotW ItemStatCost table; plugin refused before native writes.");
        LifecycleService = nullptr;
        DataTableService = nullptr;
        return false;
    }
    return true;
}

[[noreturn]] auto FailSchemaLifecycle(const char* reason) noexcept -> void {
    if (Context && reason) Context->LogError(reason);
    FailClosedNativePublication(
        reason ? reason : "authoritative schema lifecycle failed");
}

void __cdecl OnDataTablesLoaded(
        const D2RL::PluginContext* context,
        const D2RL::Lifecycle::DataTablesLoadedEvent* event,
        void* userData) noexcept {
    const auto lifecycle = SchemaLifecycle.load(std::memory_order_acquire);
    // unregisterDataTablesLoadedListener waits for an in-flight callback.
    // Stopping is therefore a normal shutdown edge, not native corruption.
    if (lifecycle == SchemaLifecycleState::Stopping
            || lifecycle == SchemaLifecycleState::Inactive) {
        return;
    }

    const auto* callbackContext = Context;
    const auto* dataTableService = DataTableService;
    if (lifecycle != SchemaLifecycleState::Active
            || !callbackContext || context != callbackContext
            || userData != &SchemaLifecycle
            || !dataTableService
            || !D2RL::Lifecycle::HasDataTablesLoadedEventField(
                event,
                D2RL::Lifecycle::DataTablesLoadedEventRequiredSize)
            || event->revision == 0) {
        FailSchemaLifecycle(
            "ISC12: invalid DataTablesLoaded callback contract; "
            "loader stopped fail-closed.");
    }

    D2RL::DataTables::TableView view{
        .structSize = D2RL::DataTables::TableViewSize,
    };
    const auto tableResult = dataTableService->getTable(
        callbackContext,
        D2RL::DataTables::Bank::Rotw,
        D2RL::DataTables::TableId::ItemStatCost,
        &view);
    if (tableResult != D2RL::DataTables::Result::Success
            || !D2RL::DataTables::HasTableViewField(
                &view, D2RL::DataTables::TableViewRequiredSize)
            || view.tableId != D2RL::DataTables::TableId::ItemStatCost
            || view.bank != D2RL::DataTables::Bank::Rotw
            || view.revision != event->revision
            || !view.rows
            || view.rowCount == 0
            || view.rowSize != CompiledItemStatRecordStride) {
        FailSchemaLifecycle(
            "ISC12: authoritative RotW ItemStatCost TableView is unavailable "
            "or ABI-incompatible; loader stopped fail-closed.");
    }

    std::string finalizationError;
    const auto finalization = FinalizePublishedSchemaSnapshot(
        view.rows,
        view.rowCount,
        view.rowSize,
        view.revision,
        finalizationError);
    if (finalization != NativeSchemaFinalizeResult::Published
            && finalization
                != NativeSchemaFinalizeResult::AcceptedExisting) {
        char failure[512]{};
        std::snprintf(
            failure,
            sizeof(failure),
            "ISC12: authoritative RotW ItemStatCost schema finalization "
            "failed (revision=%llu; rows=%u; reason=%s); loader stopped "
            "fail-closed.",
            static_cast<unsigned long long>(view.revision),
            view.rowCount,
            finalizationError.empty()
                ? "unavailable" : finalizationError.c_str());
        FailSchemaLifecycle(failure);
    }

    if (Settings.diagnostics) {
        const auto runtimeStatus = GetLoaderRuntimeStatus();
        char message[320]{};
        std::snprintf(
            message,
            sizeof(message),
            "ISC12 diagnostics: authoritative RotW ItemStatCost schema %s "
            "at DataTablesLoaded revision %llu; rows=%u; G0-builds=%llu; "
            "SchemaReady=true.",
            finalization == NativeSchemaFinalizeResult::Published
                ? "published" : "revalidated",
            static_cast<unsigned long long>(view.revision),
            view.rowCount,
            static_cast<unsigned long long>(runtimeStatus.buildCalls));
        callbackContext->LogInfo(message);
    }
}

auto RegisterSchemaLifecycleListener() noexcept -> bool {
    if (!Context || !LifecycleService || !DataTableService
            || DataTablesLoadedListenerHandle.load(std::memory_order_acquire)
                != D2RL::Lifecycle::InvalidHandle
            || SchemaLifecycle.load(std::memory_order_acquire)
                != SchemaLifecycleState::Inactive) {
        return false;
    }
    const D2RL::Lifecycle::DataTablesLoadedListener listener{
        .structSize = D2RL::Lifecycle::DataTablesLoadedListenerSize,
        .flags = 0,
        .callback = &OnDataTablesLoaded,
        .userData = &SchemaLifecycle,
    };
    D2RL::Lifecycle::ListenerHandle handle{
        D2RL::Lifecycle::InvalidHandle};
    const auto result = LifecycleService->registerDataTablesLoadedListener(
        Context, &listener, &handle);
    if (result != D2RL::Lifecycle::Result::Success
            || handle == D2RL::Lifecycle::InvalidHandle) {
        Context->LogError(
            "ISC12: DataTablesLoaded listener registration failed; plugin "
            "refused before native writes.");
        return false;
    }
    DataTablesLoadedListenerHandle.store(handle, std::memory_order_release);
    SchemaLifecycle.store(
        SchemaLifecycleState::Registered, std::memory_order_release);
    return true;
}

auto UnregisterSchemaLifecycleListener() noexcept -> void {
    SchemaLifecycle.store(
        SchemaLifecycleState::Stopping, std::memory_order_release);
    const auto handle = DataTablesLoadedListenerHandle.load(
        std::memory_order_acquire);
    if (handle == D2RL::Lifecycle::InvalidHandle
            || !LifecycleService || !Context) {
        DataTablesLoadedListenerHandle.store(
            D2RL::Lifecycle::InvalidHandle, std::memory_order_release);
        SchemaLifecycle.store(
            SchemaLifecycleState::Inactive, std::memory_order_release);
        return;
    }
    const auto result =
        LifecycleService->unregisterDataTablesLoadedListener(Context, handle);
    if (result != D2RL::Lifecycle::Result::Success
            && result != D2RL::Lifecycle::Result::NotFound) {
        Context->LogWarn(
            "ISC12: explicit DataTablesLoaded listener removal failed; "
            "D2RLoader unload cleanup remains authoritative.");
    }
    DataTablesLoadedListenerHandle.store(
        D2RL::Lifecycle::InvalidHandle, std::memory_order_release);
    SchemaLifecycle.store(
        SchemaLifecycleState::Inactive, std::memory_order_release);
}

auto QueryOwnership(
        const NativePattern& pattern,
        D2RL::Diagnostics::HookStatus& status) noexcept -> bool {
    if (!DiagnosticsService) return false;
    const D2RL::Diagnostics::HookQuery query{
        .structSize = D2RL::Diagnostics::HookQuerySize,
        .flags = 0,
        .rva = pattern.rva,
        .expected = pattern.bytes.data(),
        .expectedSize = static_cast<std::uint32_t>(pattern.bytes.size()),
        .reserved = 0,
    };
    status = {.structSize = D2RL::Diagnostics::HookStatusSize};
    const auto result = DiagnosticsService->queryHookStatus(
        Context, &query, &status);
    return result == D2RL::Diagnostics::Result::Success
        && status.structSize >= D2RL::Diagnostics::HookStatusRequiredSize
        && status.rva == pattern.rva
        && status.size == pattern.bytes.size();
}

auto ValidateOwnership(const NativePattern& pattern) noexcept -> bool {
    if (!DiagnosticsService) return true;
    D2RL::Diagnostics::HookStatus status{};
    return QueryOwnership(pattern, status)
        && status.state == D2RL::Diagnostics::ModificationState::Unchanged
        && status.ownerCount == 0;
}

auto ValidateFoundationFingerprint() noexcept -> bool {
    for (const auto& pattern : FoundationPatterns) {
        const auto compileProvider = pattern.rva == LoaderCompileCallRva
            ? InspectLoaderCompileProviderContract(Base, ImageSize)
            : LoaderCompileProviderKind::Invalid;
        const bool acceptedD2RCoreProvider =
            compileProvider
                == LoaderCompileProviderKind::D2RCoreLoadExcelTable;
        const auto playerSaveWriterProvider =
            pattern.rva == PlayerSaveStatWriterCallRva
                ? InspectPlayerSaveStatWriterProviderContract(Base, ImageSize)
                : PlayerSaveStatWriterProviderKind::Invalid;
        const bool acceptedD2RCoreSaveWriter =
            playerSaveWriterProvider
                == PlayerSaveStatWriterProviderKind::
                    D2RCoreWritePlayerSaveStatId;
        const auto itemSaveWriterProvider =
            pattern.rva == ItemSaveStatWriterCallRva
                ? InspectItemSaveStatWriterProviderContract(Base, ImageSize)
                : ItemSaveStatWriterProviderKind::Invalid;
        const bool acceptedD2RCoreItemWriter =
            itemSaveWriterProvider
                == ItemSaveStatWriterProviderKind::
                    D2RCoreWriteItemSaveStatId;
        const auto playerSaveProvider =
            pattern.rva == PlayerSaveDynamicCapacityRva
                    || pattern.rva == PlayerSaveDynamicCallRva
                ? InspectPlayerSaveProviderContract(Base, ImageSize)
                : PlayerSaveProviderKind::Invalid;
        const bool acceptedD2RCorePlayerSave =
            playerSaveProvider
                == PlayerSaveProviderKind::
                    D2RCoreWritePlayerSaveWithEnvironmentCapture;
        const auto d2sItemReadProvider =
            pattern.rva == D2SContainerVersionForwardRva
                ? InspectD2SItemReadProviderContract(Base, ImageSize)
                : D2SItemReadProviderKind::Invalid;
        const bool acceptedD2RCoreItemReader =
            d2sItemReadProvider
                == D2SItemReadProviderKind::D2RCoreReadItemsByVersion;
        const auto d2sSaveIoProvider =
            pattern.rva == D2SSaveWriterProviderCallRva
                    || pattern.rva == D2SSaveCloseProviderCallRva
                ? InspectD2SSaveIoProviderContract(Base, ImageSize)
                : D2SSaveIoProviderKind::Invalid;
        const bool acceptedD2RCoreSaveIo =
            d2sSaveIoProvider
                == D2SSaveIoProviderKind::
                    D2RCoreWriteAndCloseWithEnvironment;
        if (!Matches(pattern)
                && !acceptedD2RCoreProvider
                && !acceptedD2RCoreSaveWriter
                && !acceptedD2RCoreItemWriter
                && !acceptedD2RCorePlayerSave
                && !acceptedD2RCoreItemReader
                && !acceptedD2RCoreSaveIo) {
            char message[512]{};
            D2RL::Diagnostics::HookStatus status{};
            if (QueryOwnership(pattern, status)) {
                const auto ownerEnd = std::find(
                    std::begin(status.ownerPluginId),
                    std::end(status.ownerPluginId),
                    '\0');
                const std::string_view owner{
                    status.ownerPluginId,
                    static_cast<std::size_t>(
                        ownerEnd - std::begin(status.ownerPluginId))};
                std::snprintf(
                    message,
                    sizeof(message),
                    "ISC12: native foundation mismatch at %s (RVA 0x%llX; state=%u; kind=%u; owners=%u; owner=%.*s); plugin refused.",
                    pattern.id,
                    static_cast<unsigned long long>(pattern.rva),
                    static_cast<unsigned>(status.state),
                    static_cast<unsigned>(status.kind),
                    status.ownerCount,
                    static_cast<int>(owner.size()),
                    owner.data());
            } else {
                std::snprintf(
                    message,
                    sizeof(message),
                    "ISC12: native foundation mismatch at %s (RVA 0x%llX; ownership unavailable); plugin refused.",
                    pattern.id,
                    static_cast<unsigned long long>(pattern.rva));
            }
            Context->LogError(message);
            return false;
        }
        if (acceptedD2RCoreProvider) {
            Context->LogInfo(
                "ISC12: verified the loader.compile-call relay to "
                "D2RCore!LoadExcelTable and its native compiler forwarder; "
                "the ItemStatCost compiler contract is preserved.");
            continue;
        }
        if (acceptedD2RCoreSaveWriter) {
            Context->LogInfo(
                "ISC12: verified the codec.g3-writer-id relay to "
                "D2RCore!WritePlayerSaveStatId and its native bit-writer "
                "forwarder; ISC12 will preserve the provider and widen only "
                "the stat-ID width argument.");
            continue;
        }
        if (acceptedD2RCoreItemWriter) {
            Context->LogInfo(
                "ISC12: verified the codec.g1-bounded-writer relay to "
                "D2RCore!WriteItemSaveStatId and its native bit-writer "
                "forwarder; ISC12 will preserve the provider while publishing "
                "the bounded 12-bit writer body.");
            continue;
        }
        if (acceptedD2RCorePlayerSave) {
            if (pattern.rva == PlayerSaveDynamicCapacityRva) {
                Context->LogInfo(
                    "ISC12: verified D2RLoader's 0xFFFF dynamic save buffer "
                    "and relay to "
                    "D2RCore!WritePlayerSaveWithEnvironmentCapture, including "
                    "its full provider body, unwind contract and native "
                    "D2S serializer forwarder; ISC12 will preserve both.");
            }
            continue;
        }
        if (acceptedD2RCoreItemReader) {
            Context->LogInfo(
                "ISC12: verified the schema.d2s-container-version relay to "
                "D2RCore!ReadItemsByVersion, including its full provider "
                "body, unwind metadata and native item-reader forwarder; "
                "the D2S container version remains unchanged.");
            continue;
        }
        if (acceptedD2RCoreSaveIo) {
            if (pattern.rva == D2SSaveWriterProviderCallRva) {
                Context->LogInfo(
                    "ISC12: verified D2RCore's paired "
                    "WriteD2sFileWithEnvironment and "
                    "CloseD2sFileWithEnvironment providers, their complete "
                    "bodies, unwind contracts and native file-I/O "
                    "forwarders. Non-managed stores retain the pair; ISC12 "
                    "envelope saves use the atomic managed path and keep "
                    "environment-sidecar composition as an open gate.");
            }
            continue;
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
    SchemaLifecycle.store(
        SchemaLifecycleState::Inactive, std::memory_order_release);
    DiagnosticsService = nullptr;
    LifecycleService = nullptr;
    DataTableService = nullptr;
    DataTablesLoadedListenerHandle.store(
        D2RL::Lifecycle::InvalidHandle, std::memory_order_release);
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
    // D2RLoader's official patching model applies native changes directly
    // during this synchronous initial-load callback. Keep the authority
    // narrower than the export itself: same thread, active stack lifetime,
    // and no reuse after readiness or return.
    InitialLoadPublicationWindow publicationWindow;
    const auto& publicationLease = publicationWindow.Lease();
    if (!publicationLease.IsHeld()) {
        context->LogError(
            "ISC12: initial-load native publication window is unavailable; "
            "plugin refused with zero native writes.");
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
            || !QuerySchemaLifecycleServices()
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
    if (!RegisterSchemaLifecycleListener()) {
        ShutdownLoaderExtension();
        ReleaseProcessMutex();
        return false;
    }
    PublicationCoordinatorCallbacks publicationCallbacks{};
    if (!TryGetPreparedPublicationCallbacks(publicationCallbacks)) {
        context->LogError(
            "ISC12: canonical publication adapters are unavailable; plugin "
            "refused before the first native write.");
        UnregisterSchemaLifecycleListener();
        ShutdownLoaderExtension();
        ReleaseProcessMutex();
        return false;
    }

    PublicationCoordinator publicationCoordinator;
    auto publicationStatus = publicationCoordinator.Publish(
        publicationLease, publicationCallbacks);
    if (publicationStatus == PublicationCoordinatorStatus::QuiescenceRequired
            || publicationStatus
                == PublicationCoordinatorStatus::RejectedBeforeMutation
            || publicationStatus
                == PublicationCoordinatorStatus::ReservedWithoutMutation) {
        context->LogError(
            "ISC12: canonical publication was rejected before any native "
            "mutation; plugin refused.");
        UnregisterSchemaLifecycleListener();
        ShutdownLoaderExtension();
        ReleaseProcessMutex();
        return false;
    }
    if (publicationStatus == PublicationCoordinatorStatus::Poisoned) {
        FailClosedNativePublication(
            "canonical publication became indeterminate after native mutation");
    }
    if (publicationStatus
            == PublicationCoordinatorStatus::CommittedPendingReadiness) {
        if (!publicationLease.IsHeld()) {
            static_cast<void>(
                publicationCoordinator.PoisonBeforeStartupReadiness());
            FailClosedNativePublication(
                "initial-load window ended before readiness publication");
        }
        publicationStatus =
            publicationCoordinator.PublishReadinessAfterStartupCommit();
    }
    if (publicationStatus != PublicationCoordinatorStatus::Active) {
        static_cast<void>(
            publicationCoordinator.PoisonBeforeStartupReadiness());
        FailClosedNativePublication(
            "canonical publication did not reach active readiness");
    }

    const auto runtimeStatus = GetLoaderRuntimeStatus();
    if (!runtimeStatus.tailPatchInstalled
            || !runtimeStatus.capPatchInstalled
            || !runtimeStatus.persistenceReaderPatchInstalled
            || !runtimeStatus.persistenceWriterPatchInstalled
            || !runtimeStatus.operational
            || !runtimeStatus.persistenceCodecReady
            || !runtimeStatus.itemTransportReady
            || runtimeStatus.coldRestartRequired) {
        FailClosedNativePublication(
            "canonical publication postconditions are incomplete");
    }

    auto expectedLifecycle = SchemaLifecycleState::Registered;
    if (!SchemaLifecycle.compare_exchange_strong(
            expectedLifecycle,
            SchemaLifecycleState::Active,
            std::memory_order_acq_rel)) {
        FailClosedNativePublication(
            "schema lifecycle listener did not reach active state");
    }
    char message[384]{};
    std::snprintf(
        message,
        sizeof(message),
        "ISC12 0.2.0 active for observed D2R %s; max-stat-id=%u; foundation-patterns=%zu; codec-sites=%zu; codec-mutations=%zu; scope=%s; TOML=%s.",
        RuntimeBuild.c_str(),
        static_cast<unsigned>(MaximumStatId),
        FoundationPatterns.size(),
        PreparedCodecMutableSiteCount,
        PreparedCodecMutationCount,
        context->loadScope == D2RL::LoadScope::Mod ? "mod-local" : "global",
        LoadedConfigPath.c_str());
    context->LogInfo(message);
    if (Settings.diagnostics) {
        context->LogInfo(
            "ISC12 diagnostics: G0/G10/G9/G1-G4 are active; specialized stat packets G5-G8 remain outside this publication.");
    }
    return true;
}

D2RL_PLUGIN_EXPORT void D2RLoaderUnloadPlugin() noexcept {
    UnregisterSchemaLifecycleListener();
    ShutdownLoaderExtension();
    ReleaseProcessMutex();
    LifecycleService = nullptr;
    DataTableService = nullptr;
}

} // namespace RuffnecKk::ISC12
