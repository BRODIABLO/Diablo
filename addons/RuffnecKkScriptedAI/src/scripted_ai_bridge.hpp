#pragma once

#include "scripted_ai_limits.hpp"
#include "scripted_ai_sandbox.hpp"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace ruffneckk::scripted_ai {

inline constexpr std::size_t AiScriptNameCapacity = 65U;
inline constexpr std::size_t MaximumAiScriptRows = 4'096U;
inline constexpr std::size_t MaximumUniqueAiScripts = 1'024U;
inline constexpr std::size_t MaximumTotalAiScriptBytes = 16U * 1024U * 1024U;
inline constexpr std::uint16_t StockAiCount = 155U;
inline constexpr std::uint8_t ResolverTargetProfile = 2U;

struct AiScriptTableRow {
    std::uint32_t monStatsId{};
    char script[AiScriptNameCapacity]{};
    std::uint16_t fallbackAi{};
    std::uint8_t targetProfile{};
    std::uint8_t enabled{};
};

enum class ScriptBank : std::uint8_t {
    Base,
    Rotw,
};

struct TableRowsView {
    std::uint64_t revision{};
    std::span<const AiScriptTableRow> rows;
};

struct PreparedScript {
    std::string name;
    std::string source;
};

struct PreparedBinding {
    std::uint32_t monStatsId{};
    std::uint16_t fallbackAi{};
    std::uint8_t targetProfile{};
    std::size_t scriptIndex{};
};

struct PreparedBank {
    std::uint64_t revision{};
    std::vector<PreparedBinding> bindings;
};

struct PreparedBundle {
    std::uint64_t lifecycleRevision{};
    std::filesystem::path scriptRoot;
    std::vector<PreparedScript> scripts;
    std::array<PreparedBank, 2U> banks;
};

using SourceReader = std::function<bool(
    const std::filesystem::path& path,
    std::size_t maximumBytes,
    std::string& source,
    std::string& error)>;

[[nodiscard]] auto ReadScriptSource(
    const std::filesystem::path& path,
    std::size_t maximumBytes,
    std::string& source,
    std::string& error) -> bool;

[[nodiscard]] auto StagePreparedBundle(
    std::uint64_t lifecycleRevision,
    TableRowsView base,
    TableRowsView rotw,
    const std::filesystem::path& scriptRoot,
    const SandboxLimits& limits,
    const SourceReader& reader,
    std::string& error) -> std::shared_ptr<const PreparedBundle>;

class SessionGeneration final {
public:
    SessionGeneration(const SessionGeneration&) = delete;
    auto operator=(const SessionGeneration&) -> SessionGeneration& = delete;
    SessionGeneration(SessionGeneration&&) = delete;
    auto operator=(SessionGeneration&&) -> SessionGeneration& = delete;
    ~SessionGeneration() noexcept = default;

    [[nodiscard]] auto SessionId() const noexcept -> std::uint64_t;
    [[nodiscard]] auto LifecycleRevision() const noexcept -> std::uint64_t;
    [[nodiscard]] auto HasLuaVm() const noexcept -> bool;
    [[nodiscard]] auto ScriptCount() const noexcept -> std::size_t;
    [[nodiscard]] auto BindingCount(ScriptBank bank) const noexcept
        -> std::size_t;

private:
    friend auto CompileSessionGeneration(
        const PreparedBundle&,
        std::uint64_t,
        const SandboxLimits&,
        std::string&) -> std::shared_ptr<const SessionGeneration>;

    struct CompiledScript {
        std::string name;
        Sandbox::ScriptHandle handle{Sandbox::InvalidScriptHandle};
        TreeSummary summary;
    };

    SessionGeneration() = default;

    std::uint64_t sessionId_{};
    std::uint64_t lifecycleRevision_{};
    std::unique_ptr<Sandbox> sandbox_;
    std::vector<CompiledScript> scripts_;
    std::array<std::vector<PreparedBinding>, 2U> banks_;
};

[[nodiscard]] auto CompileSessionGeneration(
    const PreparedBundle& prepared,
    std::uint64_t sessionId,
    const SandboxLimits& limits,
    std::string& error) -> std::shared_ptr<const SessionGeneration>;

class BridgeCoordinator final {
public:
    explicit BridgeCoordinator(const SandboxLimits& limits) noexcept;

    void AnnounceGameJoined(std::uint64_t sessionId) noexcept;
    void AnnounceGameLeft(std::uint64_t sessionId) noexcept;

    // Data-table callback only. This stages copied source bytes without
    // touching Lua. ReconcileAuthoritativeSession performs the atomic compile
    // and publication later on a proven authoritative game thread.
    [[nodiscard]] auto PublishPrepared(
        std::shared_ptr<const PreparedBundle> candidate,
        std::string& error) -> bool;

    // Game-thread only. Calling this method proves that runOnGameThread
    // succeeded locally; remote clients never enter it.
    [[nodiscard]] auto ReconcileAuthoritativeSession(std::string& error)
        -> bool;

    // Game-thread only.
    void ResetGameThread() noexcept;

    [[nodiscard]] auto DesiredSession() const noexcept -> std::uint64_t;
    [[nodiscard]] auto Prepared() const noexcept
        -> std::shared_ptr<const PreparedBundle>;
    [[nodiscard]] auto ActiveFor(std::uint64_t sessionId) const noexcept
        -> std::shared_ptr<const SessionGeneration>;

private:
    SandboxLimits limits_;
    std::atomic<std::uint64_t> desiredSession_{};
    std::uint64_t authoritativeSession_{};
    std::shared_ptr<const PreparedBundle> pending_;
    std::shared_ptr<const PreparedBundle> prepared_;
    std::shared_ptr<const SessionGeneration> active_;
};

[[nodiscard]] constexpr auto BankIndex(ScriptBank bank) noexcept
        -> std::size_t {
    return bank == ScriptBank::Base ? 0U : 1U;
}

} // namespace ruffneckk::scripted_ai
