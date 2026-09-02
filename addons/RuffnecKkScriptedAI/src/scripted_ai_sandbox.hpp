#pragma once

#include "scripted_ai_executor.hpp"
#include "scripted_ai_limits.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

struct lua_State;
struct lua_Debug;

namespace ruffneckk::scripted_ai {

enum class ExecutionPhase : std::uint8_t {
    Load,
    Think,
};

struct TreeSummary {
    std::size_t nodeCount{};
    std::size_t maximumDepth{};
    std::size_t maximumChildren{};
};

enum class SandboxTickStatus : std::uint8_t {
    Action,
    NoAction,
    ExplicitFallback,
    CapabilityFallback,
};

enum class SandboxTickFailure : std::uint8_t {
    None,
    LuaError,
    InstructionBudget,
    AllocationBudget,
    CapabilityError,
    StaleHandle,
    ProtocolError,
};

struct SandboxTickResult {
    SandboxTickStatus status{SandboxTickStatus::NoAction};
    SandboxTickFailure failure{SandboxTickFailure::None};
};

class Sandbox final {
public:
    using ScriptHandle = int;
    inline static constexpr ScriptHandle InvalidScriptHandle = -1;

    [[nodiscard]] static auto Create(
        const SandboxLimits& limits,
        std::string& error) noexcept -> std::unique_ptr<Sandbox>;

    ~Sandbox() noexcept;

    Sandbox(const Sandbox&) = delete;
    auto operator=(const Sandbox&) -> Sandbox& = delete;
    Sandbox(Sandbox&&) = delete;
    auto operator=(Sandbox&&) -> Sandbox& = delete;

    [[nodiscard]] auto LoadBehaviorTree(
        std::string_view source,
        TreeSummary& summary,
        std::string& error) -> bool;

    [[nodiscard]] auto CompileBehaviorTree(
        std::string_view source,
        ScriptHandle& handle,
        TreeSummary& summary,
        std::string& error) -> bool;

    void ReleaseBehaviorTree(ScriptHandle handle) noexcept;

    [[nodiscard]] auto EvaluateBehaviorTree(
        ScriptHandle handle,
        EphemeralThinkHandle& think,
        SandboxTickResult& result,
        std::string& error) -> bool;

    [[nodiscard]] auto ExecuteForTesting(
        std::string_view source,
        ExecutionPhase phase,
        std::string& error) -> bool;

    [[nodiscard]] auto HasGlobal(std::string_view name) const -> bool;
    [[nodiscard]] auto MemoryUsed() const noexcept -> std::size_t;

private:
    struct LuaThinkHandle;

    struct AllocatorState {
        std::size_t used{};
        std::size_t sessionLimit{};
        std::size_t thinkGrowthLimit{};
        std::size_t thinkStart{};
        std::size_t thinkAllocated{};
        bool inThink{};
    };

    explicit Sandbox(const SandboxLimits& limits) noexcept;

    [[nodiscard]] auto Initialize(std::string& error) -> bool;
    [[nodiscard]] auto ExecuteChunk(
        std::string_view source,
        ExecutionPhase phase,
        bool keepResults,
        std::string& error) -> bool;
    [[nodiscard]] auto ValidateTree(
        int index,
        TreeSummary& summary,
        std::string& error) -> bool;

    static auto Allocate(
        void* userData,
        void* pointer,
        std::size_t oldSize,
        std::size_t newSize) noexcept -> void*;
    static void InstructionHook(lua_State* state, lua_Debug* debug);
    static auto RunEvaluatorThunk(lua_State* state) -> int;
    static auto ThinkHandleDispatch(lua_State* state) -> int;

    SandboxLimits limits_;
    AllocatorState allocator_;
    lua_State* state_{};
    std::uint32_t remainingInstructions_{};
    ScriptHandle evaluatorHandle_{InvalidScriptHandle};
    ScriptHandle evaluatorRunnerHandle_{InvalidScriptHandle};
    LuaThinkHandle* activeLuaHandle_{};
};

} // namespace ruffneckk::scripted_ai
