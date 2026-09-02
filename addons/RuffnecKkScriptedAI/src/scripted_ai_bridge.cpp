#include "scripted_ai_bridge.hpp"

#include <algorithm>
#include <cctype>
#include <cwctype>
#include <exception>
#include <fstream>
#include <iterator>
#include <limits>
#include <system_error>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

namespace ruffneckk::scripted_ai {
namespace {

[[nodiscard]] auto ExtractScriptName(
        const AiScriptTableRow& row,
        std::string& name,
        std::string& error) -> bool {
    const auto terminator = std::find(
        std::begin(row.script),
        std::end(row.script),
        '\0');
    if (terminator == std::end(row.script)) {
        error = "AIScript.Script is not NUL-terminated";
        return false;
    }
    name.assign(std::begin(row.script), terminator);
    if (!std::all_of(name.begin(), name.end(), [](unsigned char value) {
            return value >= 0x20U && value <= 0x7eU;
        })) {
        error = "AIScript.Script must contain printable ASCII only";
        return false;
    }
    return true;
}

[[nodiscard]] auto IsSafeRelativeScriptName(
        std::string_view name,
        std::string& error) -> bool {
    if (name.empty()) {
        error = "enabled AIScript rows require a script name";
        return false;
    }
    if (name.find('\\') != name.npos || name.find(':') != name.npos) {
        error = "AIScript.Script must use a relative forward-slash path";
        return false;
    }
    const std::filesystem::path relative(name);
    if (relative.is_absolute() || relative.has_root_name()
            || relative.has_root_directory()) {
        error = "AIScript.Script cannot be absolute";
        return false;
    }
    for (const auto& component : relative) {
        if (component.empty() || component == L"." || component == L"..") {
            error = "AIScript.Script cannot contain empty, dot, or parent components";
            return false;
        }
    }
    auto extension = relative.extension().string();
    std::transform(
        extension.begin(),
        extension.end(),
        extension.begin(),
        [](unsigned char value) { return static_cast<char>(std::tolower(value)); });
    if (extension != ".lua") {
        error = "AIScript.Script must name a .lua source";
        return false;
    }
    return true;
}

[[nodiscard]] auto EqualPathComponent(
        const std::filesystem::path& left,
        const std::filesystem::path& right) -> bool {
#ifdef _WIN32
    auto leftText = left.native();
    auto rightText = right.native();
    std::transform(
        leftText.begin(), leftText.end(), leftText.begin(),
        [](wchar_t value) { return static_cast<wchar_t>(std::towlower(value)); });
    std::transform(
        rightText.begin(), rightText.end(), rightText.begin(),
        [](wchar_t value) { return static_cast<wchar_t>(std::towlower(value)); });
    return leftText == rightText;
#else
    return left == right;
#endif
}

[[nodiscard]] auto IsStrictDescendant(
        const std::filesystem::path& root,
        const std::filesystem::path& candidate) -> bool {
    auto rootIt = root.begin();
    auto candidateIt = candidate.begin();
    for (; rootIt != root.end(); ++rootIt, ++candidateIt) {
        if (candidateIt == candidate.end()
                || !EqualPathComponent(*rootIt, *candidateIt)) {
            return false;
        }
    }
    return candidateIt != candidate.end();
}

[[nodiscard]] auto ResolveScriptPath(
        const std::filesystem::path& canonicalRoot,
        std::string_view name,
        std::filesystem::path& resolved,
        std::string& error) -> bool {
    if (!IsSafeRelativeScriptName(name, error)) return false;
    std::error_code canonicalError;
    resolved = std::filesystem::weakly_canonical(
        canonicalRoot / std::filesystem::path(name),
        canonicalError);
    if (canonicalError) {
        error = "AIScript.Script could not be canonicalized: "
            + canonicalError.message();
        return false;
    }
    if (!IsStrictDescendant(canonicalRoot, resolved)) {
        error = "AIScript.Script escapes the configured script root";
        return false;
    }
    std::error_code statusError;
    if (!std::filesystem::is_regular_file(resolved, statusError)
            || statusError) {
        error = "AIScript.Script does not resolve to a regular file";
        return false;
    }
    return true;
}

[[nodiscard]] auto PathKey(const std::filesystem::path& path) -> std::wstring {
    auto key = path.generic_wstring();
#ifdef _WIN32
    std::transform(
        key.begin(), key.end(), key.begin(),
        [](wchar_t value) { return static_cast<wchar_t>(std::towlower(value)); });
#endif
    return key;
}

[[nodiscard]] auto ValidateAndStageBank(
        TableRowsView view,
        const std::filesystem::path& canonicalRoot,
        const SandboxLimits& limits,
        const SourceReader& reader,
        std::unordered_map<std::wstring, std::size_t>& scriptIndices,
        std::size_t& totalSourceBytes,
        PreparedBundle& candidate,
        PreparedBank& bank,
        std::string& error) -> bool {
    if (view.rows.size() > MaximumAiScriptRows) {
        error = "AIScript bank exceeds the compiled row limit";
        return false;
    }
    bank.revision = view.revision;
    std::unordered_set<std::uint32_t> monsters;
    monsters.reserve(view.rows.size());
    bank.bindings.reserve(view.rows.size());

    for (const auto& row : view.rows) {
        std::string scriptName;
        if (!ExtractScriptName(row, scriptName, error)) return false;
        if (row.enabled > 1U) {
            error = "AIScript.Enabled must be 0 or 1";
            return false;
        }
        if (row.enabled == 0U) continue;
        if (row.fallbackAi >= StockAiCount) {
            error = "AIScript.FallbackAi must name a stock AI below 155";
            return false;
        }
        if (row.targetProfile != ResolverTargetProfile) {
            error = "AIScript.TargetProfile must be 2 for the governed resolver bridge";
            return false;
        }
        if (!monsters.insert(row.monStatsId).second) {
            error = "AIScript contains duplicate enabled MonStatsId rows in one bank";
            return false;
        }

        std::filesystem::path resolved;
        if (!ResolveScriptPath(canonicalRoot, scriptName, resolved, error)) {
            return false;
        }
        const auto key = PathKey(resolved);
        auto found = scriptIndices.find(key);
        std::size_t scriptIndex{};
        if (found == scriptIndices.end()) {
            if (candidate.scripts.size() >= MaximumUniqueAiScripts) {
                error = "AIScript exceeds the compiled unique-script limit";
                return false;
            }
            std::string source;
            if (!reader(resolved, limits.maxSourceBytes, source, error)) {
                return false;
            }
            if (source.find('\0') != source.npos) {
                error = "Lua source contains a NUL byte";
                return false;
            }
            if (source.size() > MaximumTotalAiScriptBytes - std::min(
                    MaximumTotalAiScriptBytes,
                    totalSourceBytes)) {
                error = "AIScript sources exceed the compiled aggregate byte limit";
                return false;
            }
            totalSourceBytes += source.size();
            scriptIndex = candidate.scripts.size();
            candidate.scripts.push_back({
                .name = scriptName,
                .source = std::move(source),
            });
            scriptIndices.emplace(key, scriptIndex);
        } else {
            scriptIndex = found->second;
        }
        bank.bindings.push_back({
            .monStatsId = row.monStatsId,
            .fallbackAi = row.fallbackAi,
            .targetProfile = row.targetProfile,
            .scriptIndex = scriptIndex,
        });
    }
    std::sort(
        bank.bindings.begin(),
        bank.bindings.end(),
        [](const PreparedBinding& left, const PreparedBinding& right) {
            return left.monStatsId < right.monStatsId;
        });
    return true;
}

} // namespace

static_assert(std::is_standard_layout_v<AiScriptTableRow>);
static_assert(std::is_trivially_copyable_v<AiScriptTableRow>);
static_assert(offsetof(AiScriptTableRow, monStatsId) == 0U);
static_assert(offsetof(AiScriptTableRow, script) == 4U);
static_assert(offsetof(AiScriptTableRow, fallbackAi) == 70U);
static_assert(offsetof(AiScriptTableRow, targetProfile) == 72U);
static_assert(offsetof(AiScriptTableRow, enabled) == 73U);
static_assert(sizeof(AiScriptTableRow) == 76U);

auto ReadScriptSource(
        const std::filesystem::path& path,
        std::size_t maximumBytes,
        std::string& source,
        std::string& error) -> bool {
    try {
        const auto size = std::filesystem::file_size(path);
        if (size > maximumBytes) {
            error = "Lua source exceeds max_source_bytes";
            return false;
        }
        std::ifstream input(path, std::ios::binary);
        if (!input.is_open()) {
            error = "Lua source cannot be opened";
            return false;
        }
        source.assign(
            std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>());
        if (!input.eof() && input.fail()) {
            error = "Lua source could not be read completely";
            return false;
        }
        if (source.size() != size) {
            error = "Lua source changed while it was being copied";
            return false;
        }
        error.clear();
        return true;
    } catch (const std::exception& exception) {
        error = exception.what();
        return false;
    } catch (...) {
        error = "unknown Lua source read failure";
        return false;
    }
}

auto StagePreparedBundle(
        std::uint64_t lifecycleRevision,
        TableRowsView base,
        TableRowsView rotw,
        const std::filesystem::path& scriptRoot,
        const SandboxLimits& limits,
        const SourceReader& reader,
        std::string& error) -> std::shared_ptr<const PreparedBundle> {
    try {
        if (!IsWithinHardLimits(limits)) {
            error = "sandbox limits exceed or violate compiled hard limits";
            return {};
        }
        if (!reader) {
            error = "AIScript source reader is unavailable";
            return {};
        }
        if (base.rows.size() + rotw.rows.size() > MaximumAiScriptRows) {
            error = "AIScript Base and RotW rows exceed the compiled aggregate limit";
            return {};
        }

        auto candidate = std::make_shared<PreparedBundle>();
        candidate->lifecycleRevision = lifecycleRevision;

        const auto hasEnabledRow = [](std::span<const AiScriptTableRow> rows) {
            return std::any_of(rows.begin(), rows.end(), [](const auto& row) {
                return row.enabled != 0U;
            });
        };
        std::filesystem::path canonicalRoot = scriptRoot;
        if (hasEnabledRow(base.rows) || hasEnabledRow(rotw.rows)) {
            std::error_code rootError;
            canonicalRoot = std::filesystem::weakly_canonical(
                scriptRoot,
                rootError);
            if (rootError) {
                error = "configured script root could not be canonicalized: "
                    + rootError.message();
                return {};
            }
            std::error_code statusError;
            if (!std::filesystem::is_directory(canonicalRoot, statusError)
                    || statusError) {
                error = "configured script root is not a directory";
                return {};
            }
        }
        candidate->scriptRoot = canonicalRoot;

        std::unordered_map<std::wstring, std::size_t> scriptIndices;
        std::size_t totalSourceBytes{};
        if (!ValidateAndStageBank(
                base,
                canonicalRoot,
                limits,
                reader,
                scriptIndices,
                totalSourceBytes,
                *candidate,
                candidate->banks[BankIndex(ScriptBank::Base)],
                error)
                || !ValidateAndStageBank(
                    rotw,
                    canonicalRoot,
                    limits,
                    reader,
                    scriptIndices,
                    totalSourceBytes,
                    *candidate,
                    candidate->banks[BankIndex(ScriptBank::Rotw)],
                    error)) {
            return {};
        }
        error.clear();
        return candidate;
    } catch (const std::exception& exception) {
        error = exception.what();
        return {};
    } catch (...) {
        error = "unknown AIScript staging failure";
        return {};
    }
}

auto SessionGeneration::SessionId() const noexcept -> std::uint64_t {
    return sessionId_;
}

auto SessionGeneration::LifecycleRevision() const noexcept -> std::uint64_t {
    return lifecycleRevision_;
}

auto SessionGeneration::HasLuaVm() const noexcept -> bool {
    return sandbox_ != nullptr;
}

auto SessionGeneration::ScriptCount() const noexcept -> std::size_t {
    return scripts_.size();
}

auto SessionGeneration::BindingCount(ScriptBank bank) const noexcept
        -> std::size_t {
    return banks_[BankIndex(bank)].size();
}

auto SessionGeneration::InspectBinding(
        ScriptBank bank,
        std::uint32_t monStatsId) const noexcept -> BindingRuntimeState {
    const auto& bindings = banks_[BankIndex(bank)];
    const auto binding = std::lower_bound(
        bindings.begin(),
        bindings.end(),
        monStatsId,
        [](const PreparedBinding& candidate, std::uint32_t id) {
            return candidate.monStatsId < id;
        });
    if (binding == bindings.end() || binding->monStatsId != monStatsId) {
        return {};
    }
    BindingRuntimeState state{
        .bound = true,
        .scriptReady = binding->scriptIndex < scripts_.size()
            && sandbox_ != nullptr,
        .quarantined = false,
        .fallbackAi = binding->fallbackAi,
    };
    if (binding->scriptIndex < scripts_.size()) {
        state.quarantined = scripts_[binding->scriptIndex].quarantined;
    }
    return state;
}

auto SessionGeneration::EvaluateThink(
        ScriptBank bank,
        std::uint32_t monStatsId,
        std::uint64_t sessionId,
        std::uint64_t thinkToken,
        ThinkSnapshot snapshot,
        ThinkCapabilities& capabilities,
        ThinkTiming timing,
        std::string& error) const -> ThinkDecision {
    ThinkDecision decision{};
    const auto& bindings = banks_[BankIndex(bank)];
    const auto binding = std::lower_bound(
        bindings.begin(),
        bindings.end(),
        monStatsId,
        [](const PreparedBinding& candidate, std::uint32_t id) {
            return candidate.monStatsId < id;
        });
    if (binding == bindings.end() || binding->monStatsId != monStatsId) {
        decision.fallbackReason = FallbackReason::MissingBinding;
        error.clear();
        return decision;
    }
    decision.fallbackAi = binding->fallbackAi;
    if (sessionId == 0U || sessionId != sessionId_ || thinkToken == 0U) {
        decision.fallbackReason = FallbackReason::StaleHandle;
        error = "think session or token does not match the active generation";
        return decision;
    }
    if (binding->scriptIndex >= scripts_.size() || sandbox_ == nullptr) {
        decision.fallbackReason = FallbackReason::InternalError;
        error = "compiled Scripted AI binding is incomplete";
        return decision;
    }

    const auto& script = scripts_[binding->scriptIndex];
    decision.scriptErrors = script.errors;
    decision.slowStrikes = script.slowStrikes;
    decision.quarantined = script.quarantined;
    if (script.quarantined) {
        decision.fallbackReason = FallbackReason::Quarantined;
        error.clear();
        return decision;
    }

    EphemeralThinkHandle think(
        sessionId,
        thinkToken,
        snapshot,
        capabilities,
        timing);
    const auto started = ReadMicroseconds(timing);
    SandboxTickResult tick{};
    const auto accepted = sandbox_->EvaluateBehaviorTree(
        script.handle,
        think,
        tick,
        error);
    const auto finished = ReadMicroseconds(timing);
    const auto wallMicroseconds = finished >= started
        ? finished - started
        : 0U;
    decision.luaMicroseconds = wallMicroseconds
        - std::min(wallMicroseconds, think.CapabilityMicroseconds());
    decision.enteredLua = true;
    decision.handleInvalidated = !think.IsValid();

    if (!accepted) {
        switch (tick.failure) {
        case SandboxTickFailure::InstructionBudget:
            decision.fallbackReason = FallbackReason::InstructionBudget;
            break;
        case SandboxTickFailure::AllocationBudget:
            decision.fallbackReason = FallbackReason::AllocationBudget;
            break;
        case SandboxTickFailure::CapabilityError:
            decision.fallbackReason = FallbackReason::CapabilityError;
            break;
        case SandboxTickFailure::StaleHandle:
            decision.fallbackReason = FallbackReason::StaleHandle;
            break;
        case SandboxTickFailure::LuaError:
            decision.fallbackReason = FallbackReason::LuaError;
            break;
        case SandboxTickFailure::ProtocolError:
        case SandboxTickFailure::None:
            decision.fallbackReason = FallbackReason::InternalError;
            break;
        }
        if (script.errors < std::numeric_limits<std::uint32_t>::max()) {
            ++script.errors;
        }
    } else if (tick.status == SandboxTickStatus::Action) {
        decision.disposition = ThinkDisposition::Action;
        decision.fallbackReason = FallbackReason::None;
        decision.action = think.CommittedAction();
        error.clear();
    } else if (tick.status == SandboxTickStatus::ExplicitFallback) {
        decision.fallbackReason = FallbackReason::ExplicitFallback;
        error.clear();
    } else if (tick.status == SandboxTickStatus::CapabilityFallback) {
        decision.fallbackReason = FallbackReason::CapabilityFallbackScheduled;
        decision.fallbackScheduled = true;
        error.clear();
    } else {
        decision.fallbackReason = think.ActionAttempts() == 0U
            ? FallbackReason::NoAction
            : FallbackReason::CapabilityRejected;
        error.clear();
    }

    if (decision.luaMicroseconds > limits_.slowThinkMicroseconds
            && script.slowStrikes
                < std::numeric_limits<std::uint32_t>::max()) {
        ++script.slowStrikes;
    }
    if (script.errors >= limits_.maxScriptErrorsPerSession
            || script.slowStrikes >= limits_.maxSlowStrikesPerSession) {
        script.quarantined = true;
    }
    decision.scriptErrors = script.errors;
    decision.slowStrikes = script.slowStrikes;
    decision.quarantined = script.quarantined;
    return decision;
}

auto CompileSessionGeneration(
        const PreparedBundle& prepared,
        std::uint64_t sessionId,
        const SandboxLimits& limits,
        std::string& error) -> std::shared_ptr<const SessionGeneration> {
    try {
        if (sessionId == 0U) {
            error = "session generation zero cannot own a Lua VM";
            return {};
        }
        auto candidate = std::shared_ptr<SessionGeneration>(
            new SessionGeneration());
        candidate->sessionId_ = sessionId;
        candidate->lifecycleRevision_ = prepared.lifecycleRevision;
        candidate->limits_ = limits;
        candidate->banks_[BankIndex(ScriptBank::Base)] =
            prepared.banks[BankIndex(ScriptBank::Base)].bindings;
        candidate->banks_[BankIndex(ScriptBank::Rotw)] =
            prepared.banks[BankIndex(ScriptBank::Rotw)].bindings;

        if (prepared.scripts.empty()) {
            error.clear();
            return candidate;
        }
        candidate->sandbox_ = Sandbox::Create(limits, error);
        if (!candidate->sandbox_) return {};
        candidate->scripts_.reserve(prepared.scripts.size());
        for (const auto& script : prepared.scripts) {
            Sandbox::ScriptHandle handle = Sandbox::InvalidScriptHandle;
            TreeSummary summary{};
            if (!candidate->sandbox_->CompileBehaviorTree(
                    script.source,
                    handle,
                    summary,
                    error)) {
                error = script.name + ": " + error;
                return {};
            }
            candidate->scripts_.push_back({
                .name = script.name,
                .handle = handle,
                .summary = summary,
            });
        }
        error.clear();
        return candidate;
    } catch (const std::exception& exception) {
        error = exception.what();
        return {};
    } catch (...) {
        error = "unknown immutable-generation compilation failure";
        return {};
    }
}

BridgeCoordinator::BridgeCoordinator(const SandboxLimits& limits) noexcept
    : limits_(limits) {}

void BridgeCoordinator::AnnounceGameJoined(std::uint64_t sessionId) noexcept {
    if (sessionId != 0U) {
        desiredSession_.store(sessionId, std::memory_order_release);
    }
}

void BridgeCoordinator::AnnounceGameLeft(std::uint64_t sessionId) noexcept {
    (void)sessionId;
    desiredSession_.store(0U, std::memory_order_release);
}

auto BridgeCoordinator::PublishPrepared(
        std::shared_ptr<const PreparedBundle> candidate,
        std::string& error) -> bool {
    if (!candidate) {
        error = "prepared AIScript candidate is null";
        return false;
    }
    pending_ = std::move(candidate);
    error.clear();
    return true;
}

auto BridgeCoordinator::ReconcileAuthoritativeSession(std::string& error)
        -> bool {
    const auto desired = desiredSession_.load(std::memory_order_acquire);
    if (desired == 0U) {
        active_.reset();
        authoritativeSession_ = 0U;
        error.clear();
        return true;
    }

    authoritativeSession_ = desired;
    const auto candidate = pending_ ? pending_ : prepared_;
    if (!candidate) {
        if (active_ && active_->SessionId() != desired) active_.reset();
        error = "no prepared AIScript table snapshot is available";
        return false;
    }
    if (!pending_ && active_ && active_->SessionId() == desired
            && active_->LifecycleRevision()
                == candidate->lifecycleRevision) {
        error.clear();
        return true;
    }

    auto compiled = CompileSessionGeneration(
        *candidate,
        desired,
        limits_,
        error);
    if (!compiled) {
        pending_.reset();
        if (active_ && active_->SessionId() != desired) active_.reset();
        return false;
    }
    if (pending_) {
        prepared_ = std::move(pending_);
    }
    active_ = std::move(compiled);
    error.clear();
    return true;
}

void BridgeCoordinator::ResetGameThread() noexcept {
    active_.reset();
    pending_.reset();
    prepared_.reset();
    authoritativeSession_ = 0U;
    desiredSession_.store(0U, std::memory_order_release);
}

auto BridgeCoordinator::DesiredSession() const noexcept -> std::uint64_t {
    return desiredSession_.load(std::memory_order_acquire);
}

auto BridgeCoordinator::Prepared() const noexcept
        -> std::shared_ptr<const PreparedBundle> {
    return prepared_;
}

auto BridgeCoordinator::ActiveFor(std::uint64_t sessionId) const noexcept
        -> std::shared_ptr<const SessionGeneration> {
    if (sessionId == 0U
            || desiredSession_.load(std::memory_order_acquire) != sessionId
            || authoritativeSession_ != sessionId
            || !active_
            || active_->SessionId() != sessionId) {
        return {};
    }
    return active_;
}

} // namespace ruffneckk::scripted_ai
