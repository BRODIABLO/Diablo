#include "scripted_ai_sandbox.hpp"

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <limits>
#include <string>
#include <unordered_set>
#include <utility>

namespace ruffneckk::scripted_ai {
namespace {

constexpr char ThinkHandleMetatable[] =
    "RuffnecKk.ScriptedAI.EphemeralThinkHandle";

constexpr int EvaluatorFailure = 0;
constexpr int EvaluatorAction = 1;
constexpr int EvaluatorFallback = 2;
constexpr int EvaluatorSuccess = 3;
constexpr int EvaluatorCapabilityFallback = 4;

constexpr int LuaCapabilityRejected = 0;
constexpr int LuaCapabilityAccepted = 1;
constexpr int LuaCapabilityFallback = 2;

constexpr char TrustedEvaluatorSource[] = R"lua(
return function(root, m)
    local FAILURE = 0
    local ACTION = 1
    local FALLBACK = 2
    local SUCCESS = 3
    local CAPABILITY_FALLBACK = 4

    local function action(result)
        if result == 1 then
            return ACTION
        end
        if result == 2 then
            return CAPABILITY_FALLBACK
        end
        return FAILURE
    end

    local function tick(node)
        local kind = node.kind
        if kind == "selector" then
            for index = 1, #node.children do
                local result = tick(node.children[index])
                if result == ACTION or result == FALLBACK
                        or result == CAPABILITY_FALLBACK then
                    return result
                end
                if result == SUCCESS then
                    return SUCCESS
                end
            end
            return FAILURE
        end
        if kind == "sequence" then
            for index = 1, #node.children do
                local result = tick(node.children[index])
                if result == FAILURE then
                    return FAILURE
                end
                if result == ACTION or result == FALLBACK
                        or result == CAPABILITY_FALLBACK then
                    return result
                end
            end
            return SUCCESS
        end
        if kind == "has_target" then
            return m:hasTarget() and SUCCESS or FAILURE
        end
        if kind == "in_combat" then
            return m:inCombat() and SUCCESS or FAILURE
        end
        if kind == "target_distance_lte" then
            return m:targetDistance() <= node.distance and SUCCESS or FAILURE
        end
        if kind == "target_distance_gte" then
            return m:targetDistance() >= node.distance and SUCCESS or FAILURE
        end
        if kind == "has_owner" then
            return m:hasOwner() and SUCCESS or FAILURE
        end
        if kind == "owner_distance_lte" then
            return m:ownerDistance() <= node.distance and SUCCESS or FAILURE
        end
        if kind == "owner_distance_gte" then
            return m:ownerDistance() >= node.distance and SUCCESS or FAILURE
        end
        if kind == "target_owner_distance_lte" then
            return m:targetOwnerDistance() <= node.distance and SUCCESS or FAILURE
        end
        if kind == "target_owner_distance_gte" then
            return m:targetOwnerDistance() >= node.distance and SUCCESS or FAILURE
        end
        if kind == "is_melee" then
            return m:isMelee() and SUCCESS or FAILURE
        end
        if kind == "is_ranged" then
            return m:isRanged() and SUCCESS or FAILURE
        end
        if kind == "is_caster" then
            return m:isCaster() and SUCCESS or FAILURE
        end
        if kind == "last_action_attack" then
            return m:lastActionWasAttack() and SUCCESS or FAILURE
        end
        if kind == "last_action_retreat" then
            return m:lastActionWasRetreat() and SUCCESS or FAILURE
        end
        if kind == "last_action_cast" then
            return m:lastActionWasCast() and SUCCESS or FAILURE
        end
        if kind == "has_preferred_skill" then
            return m:hasPreferredSkill() and SUCCESS or FAILURE
        end
        if kind == "idle" then
            return action(m:idle(node.frames or 1))
        end
        if kind == "wander" then
            return action(m:wander(node.radius or 5))
        end
        if kind == "attack" then
            return action(m:attackTarget())
        end
        if kind == "chase" then
            return action(m:chaseTarget())
        end
        if kind == "retreat" then
            return action(m:retreatFromTarget(node.distance or 6))
        end
        if kind == "cast" then
            return action(m:castOnTarget(node.skill))
        end
        if kind == "cast_preferred" then
            return action(m:castPreferredSkill())
        end
        if kind == "follow_owner" then
            return action(m:followOwner())
        end
        if kind == "fallback" then
            return FALLBACK
        end
        error("unsupported Scripted AI node kind")
    end

    return tick(root)
end
)lua";

enum class HandleOperation : int {
    HasTarget,
    InCombat,
    TargetDistance,
    HasOwner,
    OwnerDistance,
    TargetOwnerDistance,
    IsMelee,
    IsRanged,
    IsCaster,
    LastActionWasAttack,
    LastActionWasRetreat,
    LastActionWasCast,
    HasPreferredSkill,
    Idle,
    Wander,
    AttackTarget,
    ChaseTarget,
    RetreatFromTarget,
    CastOnTarget,
    CastPreferredSkill,
    FollowOwner,
    ToString,
};

[[nodiscard]] auto IsCompositeKind(std::string_view kind) noexcept -> bool {
    return kind == "selector" || kind == "sequence";
}

[[nodiscard]] auto IsKnownLeafKind(std::string_view kind) noexcept -> bool {
    return kind == "has_target" || kind == "in_combat"
        || kind == "target_distance_lte" || kind == "target_distance_gte"
        || kind == "has_owner" || kind == "owner_distance_lte"
        || kind == "owner_distance_gte"
        || kind == "target_owner_distance_lte"
        || kind == "target_owner_distance_gte"
        || kind == "is_melee" || kind == "is_ranged"
        || kind == "is_caster" || kind == "last_action_attack"
        || kind == "last_action_retreat" || kind == "last_action_cast"
        || kind == "has_preferred_skill"
        || kind == "idle" || kind == "wander" || kind == "attack"
        || kind == "chase" || kind == "retreat" || kind == "cast"
        || kind == "cast_preferred"
        || kind == "follow_owner" || kind == "fallback";
}

[[nodiscard]] auto IsAllowedNodeField(
        std::string_view kind,
        std::string_view field) noexcept -> bool {
    if (field == "kind") return true;
    if (IsCompositeKind(kind)) return field == "children";
    if (kind == "target_distance_lte" || kind == "target_distance_gte"
            || kind == "owner_distance_lte"
            || kind == "owner_distance_gte"
            || kind == "target_owner_distance_lte"
            || kind == "target_owner_distance_gte"
            || kind == "retreat") {
        return field == "distance";
    }
    if (kind == "idle") return field == "frames";
    if (kind == "wander") return field == "radius";
    if (kind == "cast") return field == "skill";
    return false;
}

[[nodiscard]] auto ScalarRange(
        std::string_view kind,
        std::string_view field,
        lua_Integer& minimum,
        lua_Integer& maximum) noexcept -> bool {
    minimum = 0;
    maximum = 0;
    if (field == "distance") {
        minimum = (kind == "retreat") ? 1 : 0;
        maximum = 255;
        return true;
    }
    if (field == "frames" || field == "radius") {
        minimum = 1;
        maximum = 255;
        return true;
    }
    if (field == "skill") {
        minimum = 0;
        maximum = 65'535;
        return true;
    }
    return false;
}

void RemoveGlobal(lua_State* state, const char* name) {
    lua_pushnil(state);
    lua_setglobal(state, name);
}

void RemoveTableField(lua_State* state, const char* table, const char* field) {
    lua_getglobal(state, table);
    if (lua_istable(state, -1)) {
        lua_pushnil(state);
        lua_setfield(state, -2, field);
    }
    lua_pop(state, 1);
}

[[nodiscard]] auto StackError(lua_State* state) -> std::string {
    const auto* message = lua_tostring(state, -1);
    return message != nullptr ? std::string(message)
                              : std::string("unknown Lua failure");
}

[[nodiscard]] auto IsCanonicalKind(std::string_view value) noexcept -> bool {
    if (value.empty() || value.size() > 32U) return false;
    return std::all_of(value.begin(), value.end(), [](const char character) {
        return (character >= 'a' && character <= 'z')
            || (character >= '0' && character <= '9')
            || character == '_';
    });
}

void CopyGlobal(lua_State* state, int environment, const char* name) {
    environment = lua_absindex(state, environment);
    lua_getglobal(state, name);
    lua_setfield(state, environment, name);
}

void CloneGlobalTable(lua_State* state, int environment, const char* name) {
    environment = lua_absindex(state, environment);
    lua_getglobal(state, name);
    if (!lua_istable(state, -1)) {
        lua_pop(state, 1);
        return;
    }
    const auto source = lua_absindex(state, -1);
    lua_newtable(state);
    const auto clone = lua_absindex(state, -1);
    lua_pushnil(state);
    while (lua_next(state, source) != 0) {
        lua_pushvalue(state, -2);
        lua_insert(state, -2);
        lua_settable(state, clone);
    }
    lua_setfield(state, environment, name);
    lua_pop(state, 1);
}

void InstallChunkEnvironment(lua_State* state, int chunk) {
    chunk = lua_absindex(state, chunk);
    lua_newtable(state);
    const auto environment = lua_absindex(state, -1);
    for (const auto* name : {
            "assert", "error", "ipairs", "next", "pairs", "rawequal",
            "rawget", "rawlen", "select", "tonumber", "tostring", "type"}) {
        CopyGlobal(state, environment, name);
    }
    CloneGlobalTable(state, environment, LUA_MATHLIBNAME);
    CloneGlobalTable(state, environment, LUA_TABLIBNAME);
    lua_pushvalue(state, environment);
    lua_setfield(state, environment, "_G");
    lua_pushvalue(state, environment);
    (void)lua_setupvalue(state, chunk, 1);
    lua_pop(state, 1);
}

int RetainBehaviorTree(lua_State* state) {
    luaL_checktype(state, 1, LUA_TTABLE);
    lua_pushvalue(state, 1);
    const auto reference = luaL_ref(state, LUA_REGISTRYINDEX);
    lua_pushinteger(state, static_cast<lua_Integer>(reference));
    return 1;
}

} // namespace

struct Sandbox::LuaThinkHandle {
    EphemeralThinkHandle* handle{};
    std::uint64_t sessionGeneration{};
    std::uint64_t thinkToken{};
};

Sandbox::Sandbox(const SandboxLimits& limits) noexcept
    : limits_(limits) {
    allocator_.sessionLimit = limits.sessionHeapBytes;
    allocator_.thinkGrowthLimit = limits.perThinkHeapGrowthBytes;
}

Sandbox::~Sandbox() noexcept {
    if (state_ != nullptr) {
        lua_close(state_);
        state_ = nullptr;
    }
}

auto Sandbox::Create(
        const SandboxLimits& limits,
        std::string& error) noexcept -> std::unique_ptr<Sandbox> {
    try {
        if (!IsWithinHardLimits(limits)) {
            error = "sandbox limits exceed or violate compiled hard limits";
            return {};
        }
        auto sandbox = std::unique_ptr<Sandbox>(new Sandbox(limits));
        if (!sandbox->Initialize(error)) return {};
        error.clear();
        return sandbox;
    } catch (const std::exception& exception) {
        error = exception.what();
        return {};
    } catch (...) {
        error = "unknown sandbox construction failure";
        return {};
    }
}

auto Sandbox::Initialize(std::string& error) -> bool {
    state_ = lua_newstate(Allocate, &allocator_);
    if (state_ == nullptr) {
        error = "Lua state allocation failed within the session heap budget";
        return false;
    }
    *static_cast<Sandbox**>(lua_getextraspace(state_)) = this;

    luaL_requiref(state_, LUA_GNAME, luaopen_base, 1);
    lua_pop(state_, 1);
    luaL_requiref(state_, LUA_TABLIBNAME, luaopen_table, 1);
    lua_pop(state_, 1);
    luaL_requiref(state_, LUA_MATHLIBNAME, luaopen_math, 1);
    lua_pop(state_, 1);

    RemoveGlobal(state_, "collectgarbage");
    RemoveGlobal(state_, "dofile");
    RemoveGlobal(state_, "load");
    RemoveGlobal(state_, "loadfile");
    RemoveGlobal(state_, "pcall");
    RemoveGlobal(state_, "print");
    RemoveGlobal(state_, "require");
    RemoveGlobal(state_, "package");
    RemoveGlobal(state_, "io");
    RemoveGlobal(state_, "os");
    RemoveGlobal(state_, "debug");
    RemoveGlobal(state_, "coroutine");
    RemoveGlobal(state_, "ffi");
    RemoveGlobal(state_, "string");
    RemoveGlobal(state_, "utf8");
    RemoveGlobal(state_, "warn");
    RemoveGlobal(state_, "xpcall");
    RemoveTableField(state_, LUA_MATHLIBNAME, "random");
    RemoveTableField(state_, LUA_MATHLIBNAME, "randomseed");

    if (luaL_newmetatable(state_, ThinkHandleMetatable) == 0) {
        lua_pop(state_, 1);
        error = "ephemeral think-handle metatable already exists";
        return false;
    }
    lua_newtable(state_);
    constexpr std::array methods{
        std::pair{"hasTarget", HandleOperation::HasTarget},
        std::pair{"inCombat", HandleOperation::InCombat},
        std::pair{"targetDistance", HandleOperation::TargetDistance},
        std::pair{"hasOwner", HandleOperation::HasOwner},
        std::pair{"ownerDistance", HandleOperation::OwnerDistance},
        std::pair{"targetOwnerDistance", HandleOperation::TargetOwnerDistance},
        std::pair{"isMelee", HandleOperation::IsMelee},
        std::pair{"isRanged", HandleOperation::IsRanged},
        std::pair{"isCaster", HandleOperation::IsCaster},
        std::pair{"lastActionWasAttack", HandleOperation::LastActionWasAttack},
        std::pair{"lastActionWasRetreat", HandleOperation::LastActionWasRetreat},
        std::pair{"lastActionWasCast", HandleOperation::LastActionWasCast},
        std::pair{"hasPreferredSkill", HandleOperation::HasPreferredSkill},
        std::pair{"idle", HandleOperation::Idle},
        std::pair{"wander", HandleOperation::Wander},
        std::pair{"attackTarget", HandleOperation::AttackTarget},
        std::pair{"chaseTarget", HandleOperation::ChaseTarget},
        std::pair{"retreatFromTarget", HandleOperation::RetreatFromTarget},
        std::pair{"castOnTarget", HandleOperation::CastOnTarget},
        std::pair{"castPreferredSkill", HandleOperation::CastPreferredSkill},
        std::pair{"followOwner", HandleOperation::FollowOwner},
    };
    for (const auto& [name, operation] : methods) {
        lua_pushinteger(state_, static_cast<lua_Integer>(operation));
        lua_pushcclosure(state_, ThinkHandleDispatch, 1);
        lua_setfield(state_, -2, name);
    }
    lua_setfield(state_, -2, "__index");
    lua_pushinteger(
        state_, static_cast<lua_Integer>(HandleOperation::ToString));
    lua_pushcclosure(state_, ThinkHandleDispatch, 1);
    lua_setfield(state_, -2, "__tostring");
    lua_pushboolean(state_, 0);
    lua_setfield(state_, -2, "__metatable");
    lua_pop(state_, 1);

    if (luaL_loadbufferx(
            state_,
            TrustedEvaluatorSource,
            sizeof(TrustedEvaluatorSource) - 1U,
            "@ruffneckk-scripted-ai-evaluator",
            "t") != LUA_OK
            || lua_pcall(state_, 0, 1, 0) != LUA_OK) {
        error = "trusted behavior-tree evaluator failed to initialize: "
            + StackError(state_);
        lua_settop(state_, 0);
        return false;
    }
    if (!lua_isfunction(state_, -1)) {
        error = "trusted behavior-tree evaluator did not return a function";
        lua_settop(state_, 0);
        return false;
    }
    evaluatorHandle_ = luaL_ref(state_, LUA_REGISTRYINDEX);
    lua_pushcfunction(state_, RunEvaluatorThunk);
    evaluatorRunnerHandle_ = luaL_ref(state_, LUA_REGISTRYINDEX);
    error.clear();
    return true;
}

auto Sandbox::Allocate(
        void* userData,
        void* pointer,
        std::size_t oldSize,
        std::size_t newSize) noexcept -> void* {
    auto* state = static_cast<AllocatorState*>(userData);
    const auto accountedOld = pointer != nullptr ? oldSize : 0U;
    if (newSize == 0U) {
        std::free(pointer);
        state->used -= std::min(state->used, accountedOld);
        return nullptr;
    }

    const auto reduced = state->used - std::min(state->used, accountedOld);
    if (newSize > state->sessionLimit - std::min(
            state->sessionLimit,
            reduced)) {
        return nullptr;
    }
    const auto proposed = reduced + newSize;
    if (proposed > state->sessionLimit) return nullptr;
    const auto grossGrowth = newSize > accountedOld
        ? newSize - accountedOld
        : 0U;
    if (state->inThink
            && grossGrowth > state->thinkGrowthLimit - std::min(
                state->thinkGrowthLimit,
                state->thinkAllocated)) {
        return nullptr;
    }
    if (state->inThink
            && proposed > state->thinkStart
                + state->thinkGrowthLimit) {
        return nullptr;
    }

    auto* replacement = std::realloc(pointer, newSize);
    if (replacement == nullptr) return nullptr;
    state->used = proposed;
    if (state->inThink) state->thinkAllocated += grossGrowth;
    return replacement;
}

void Sandbox::InstructionHook(lua_State* state, lua_Debug*) {
    auto* sandbox = *static_cast<Sandbox**>(lua_getextraspace(state));
    if (sandbox == nullptr) {
        luaL_error(state, "Scripted AI instruction guard is unavailable");
        return;
    }
    const auto interval = sandbox->limits_.instructionHookInterval;
    if (sandbox->remainingInstructions_ <= interval) {
        sandbox->remainingInstructions_ = 0U;
        luaL_error(state, "Scripted AI instruction budget exceeded");
        return;
    }
    sandbox->remainingInstructions_ -= interval;
}

auto Sandbox::ThinkHandleDispatch(lua_State* state) -> int {
    const auto operation = static_cast<HandleOperation>(
        lua_tointeger(state, lua_upvalueindex(1)));
    if (operation == HandleOperation::ToString) {
        lua_pushliteral(state, "ScriptedAIThinkHandle");
        return 1;
    }

    auto* slot = static_cast<LuaThinkHandle*>(
        luaL_checkudata(state, 1, ThinkHandleMetatable));
    auto* handle = slot != nullptr ? slot->handle : nullptr;
    const auto valid = handle != nullptr && handle->IsValid()
        && slot->sessionGeneration == handle->SessionGeneration()
        && slot->thinkToken == handle->ThinkToken();
    const auto snapshot = valid ? handle->Snapshot() : ThinkSnapshot{};

    switch (operation) {
    case HandleOperation::HasTarget:
        lua_pushboolean(state, valid && snapshot.hasTarget);
        return 1;
    case HandleOperation::InCombat:
        lua_pushboolean(state, valid && snapshot.inCombat);
        return 1;
    case HandleOperation::TargetDistance:
        lua_pushinteger(
            state,
            valid ? static_cast<lua_Integer>(snapshot.targetDistance) : 0);
        return 1;
    case HandleOperation::HasOwner:
        lua_pushboolean(state, valid && snapshot.hasOwner);
        return 1;
    case HandleOperation::OwnerDistance:
        lua_pushinteger(
            state,
            valid ? static_cast<lua_Integer>(snapshot.ownerDistance) : 0);
        return 1;
    case HandleOperation::TargetOwnerDistance:
        lua_pushinteger(
            state,
            valid
                ? static_cast<lua_Integer>(snapshot.targetOwnerDistance)
                : 0);
        return 1;
    case HandleOperation::IsMelee:
        lua_pushboolean(
            state,
            valid && snapshot.tacticalProfile
                == TacticalProfile::MeleeVanguard);
        return 1;
    case HandleOperation::IsRanged:
        lua_pushboolean(
            state,
            valid && snapshot.tacticalProfile
                == TacticalProfile::RangedSkirmisher);
        return 1;
    case HandleOperation::IsCaster:
        lua_pushboolean(
            state,
            valid && snapshot.tacticalProfile
                == TacticalProfile::CasterArtillery);
        return 1;
    case HandleOperation::LastActionWasAttack:
        lua_pushboolean(
            state,
            valid && snapshot.hasLastAction
                && snapshot.lastAction == ActionKind::AttackTarget);
        return 1;
    case HandleOperation::LastActionWasRetreat:
        lua_pushboolean(
            state,
            valid && snapshot.hasLastAction
                && snapshot.lastAction == ActionKind::RetreatFromTarget);
        return 1;
    case HandleOperation::LastActionWasCast:
        lua_pushboolean(
            state,
            valid && snapshot.hasLastAction
                && snapshot.lastAction == ActionKind::CastOnTarget);
        return 1;
    case HandleOperation::HasPreferredSkill:
        lua_pushboolean(state, valid && snapshot.hasPreferredSkill);
        return 1;
    case HandleOperation::ToString:
        break;
    default:
        break;
    }

    const auto boundedArgument = [&](int index,
                                     lua_Integer minimum,
                                     lua_Integer maximum,
                                     const char* label) -> std::uint32_t {
        const auto value = luaL_checkinteger(state, index);
        luaL_argcheck(
            state,
            value >= minimum && value <= maximum,
            index,
            label);
        return static_cast<std::uint32_t>(value);
    };

    ActionIntent intent{};
    switch (operation) {
    case HandleOperation::Idle:
        intent = {
            .kind = ActionKind::Idle,
            .argument = boundedArgument(2, 1, 255, "frames must be 1..255"),
        };
        break;
    case HandleOperation::Wander:
        intent = {
            .kind = ActionKind::Wander,
            .argument = boundedArgument(2, 1, 255, "radius must be 1..255"),
        };
        break;
    case HandleOperation::AttackTarget:
        intent = {.kind = ActionKind::AttackTarget, .argument = 0U};
        break;
    case HandleOperation::ChaseTarget:
        intent = {.kind = ActionKind::ChaseTarget, .argument = 0U};
        break;
    case HandleOperation::RetreatFromTarget:
        intent = {
            .kind = ActionKind::RetreatFromTarget,
            .argument = boundedArgument(
                2, 1, 255, "distance must be 1..255"),
        };
        break;
    case HandleOperation::CastOnTarget:
        intent = {
            .kind = ActionKind::CastOnTarget,
            .argument = boundedArgument(
                2, 0, 65'535, "skill must be 0..65535"),
        };
        break;
    case HandleOperation::CastPreferredSkill:
        if (!valid || !snapshot.hasPreferredSkill) {
            lua_pushinteger(state, LuaCapabilityRejected);
            return 1;
        }
        intent = {
            .kind = ActionKind::CastOnTarget,
            .argument = snapshot.preferredSkill,
        };
        break;
    case HandleOperation::FollowOwner:
        intent = {.kind = ActionKind::FollowOwner, .argument = 0U};
        break;
    case HandleOperation::HasTarget:
    case HandleOperation::InCombat:
    case HandleOperation::TargetDistance:
    case HandleOperation::HasOwner:
    case HandleOperation::OwnerDistance:
    case HandleOperation::TargetOwnerDistance:
    case HandleOperation::IsMelee:
    case HandleOperation::IsRanged:
    case HandleOperation::IsCaster:
    case HandleOperation::LastActionWasAttack:
    case HandleOperation::LastActionWasRetreat:
    case HandleOperation::LastActionWasCast:
    case HandleOperation::HasPreferredSkill:
    case HandleOperation::ToString:
        return luaL_error(state, "invalid Scripted AI think-handle operation");
    }

    if (!valid) {
        if (handle != nullptr) (void)handle->TryAction(intent);
        lua_pushinteger(state, LuaCapabilityRejected);
        return 1;
    }
    const auto result = handle->TryAction(intent);
    if (result == CapabilityResult::Error) {
        return luaL_error(state, "Scripted AI capability failed");
    }
    const auto luaResult = result == CapabilityResult::Accepted
        ? LuaCapabilityAccepted
        : result == CapabilityResult::FallbackScheduled
            ? LuaCapabilityFallback
            : LuaCapabilityRejected;
    lua_pushinteger(state, luaResult);
    return 1;
}

auto Sandbox::RunEvaluatorThunk(lua_State* state) -> int {
    auto* sandbox = *static_cast<Sandbox**>(lua_getextraspace(state));
    if (sandbox == nullptr
            || sandbox->evaluatorHandle_ == InvalidScriptHandle) {
        return luaL_error(
            state, "trusted Scripted AI evaluator is unavailable");
    }
    sandbox->allocator_.inThink = true;
    sandbox->allocator_.thinkStart = sandbox->allocator_.used;
    sandbox->allocator_.thinkAllocated = 0U;
    const auto treeHandle = static_cast<ScriptHandle>(
        luaL_checkinteger(state, 1));
    auto* think = static_cast<EphemeralThinkHandle*>(
        lua_touserdata(state, 2));
    if (treeHandle < 0 || think == nullptr) {
        return luaL_error(state, "invalid Scripted AI evaluation request");
    }

    lua_rawgeti(state, LUA_REGISTRYINDEX, sandbox->evaluatorHandle_);
    if (!lua_isfunction(state, -1)) {
        return luaL_error(state, "trusted Scripted AI evaluator was lost");
    }
    lua_rawgeti(state, LUA_REGISTRYINDEX, treeHandle);
    if (!lua_istable(state, -1)) {
        return luaL_error(state, "retained Scripted AI tree was lost");
    }
    auto* slot = static_cast<LuaThinkHandle*>(
        lua_newuserdatauv(state, sizeof(LuaThinkHandle), 0));
    *slot = {
        .handle = think,
        .sessionGeneration = think->SessionGeneration(),
        .thinkToken = think->ThinkToken(),
    };
    sandbox->activeLuaHandle_ = slot;
    luaL_getmetatable(state, ThinkHandleMetatable);
    lua_setmetatable(state, -2);
    lua_call(state, 2, 1);
    return 1;
}

auto Sandbox::ExecuteChunk(
        std::string_view source,
        ExecutionPhase phase,
        bool keepResults,
        std::string& error) -> bool {
    if (state_ == nullptr) {
        error = "Lua state is unavailable";
        return false;
    }
    if (source.size() > limits_.maxSourceBytes) {
        error = "Lua source exceeds max_source_bytes";
        return false;
    }

    lua_settop(state_, 0);
    allocator_.inThink = false;
    allocator_.thinkStart = allocator_.used;
    allocator_.thinkAllocated = 0U;
    const auto status = luaL_loadbufferx(
        state_,
        source.data(),
        source.size(),
        "@ruffneckk-scripted-ai",
        "t");
    if (status != LUA_OK) {
        error = StackError(state_);
        lua_settop(state_, 0);
        allocator_.inThink = false;
        return false;
    }
    InstallChunkEnvironment(state_, -1);

    allocator_.inThink = phase == ExecutionPhase::Think;
    allocator_.thinkStart = allocator_.used;
    allocator_.thinkAllocated = 0U;

    remainingInstructions_ = phase == ExecutionPhase::Think
        ? limits_.maxInstructionsPerThink
        : limits_.maxLoadInstructions;
    lua_sethook(
        state_,
        InstructionHook,
        LUA_MASKCOUNT,
        static_cast<int>(limits_.instructionHookInterval));
    const auto callStatus = lua_pcall(state_, 0, LUA_MULTRET, 0);
    lua_sethook(state_, nullptr, 0, 0);
    allocator_.inThink = false;
    if (callStatus != LUA_OK) {
        error = StackError(state_);
        lua_settop(state_, 0);
        return false;
    }
    if (!keepResults) lua_settop(state_, 0);
    error.clear();
    return true;
}

auto Sandbox::ExecuteForTesting(
        std::string_view source,
        ExecutionPhase phase,
        std::string& error) -> bool {
    return ExecuteChunk(source, phase, false, error);
}

auto Sandbox::ValidateTree(
        int index,
        TreeSummary& summary,
        std::string& error) -> bool {
    std::unordered_set<const void*> seen;
    const auto validate = [&](const auto& self, int nodeIndex,
                              std::size_t depth) -> bool {
        nodeIndex = lua_absindex(state_, nodeIndex);
        if (!lua_istable(state_, nodeIndex)) {
            error = "behavior-tree nodes must be plain tables";
            return false;
        }
        if (lua_getmetatable(state_, nodeIndex) != 0) {
            lua_pop(state_, 1);
            error = "behavior-tree nodes cannot have metatables";
            return false;
        }
        const auto* identity = lua_topointer(state_, nodeIndex);
        if (identity == nullptr || !seen.insert(identity).second) {
            error = "behavior tree contains a cycle or shared node";
            return false;
        }
        ++summary.nodeCount;
        summary.maximumDepth = std::max(summary.maximumDepth, depth);
        if (summary.nodeCount > limits_.maxTreeNodes) {
            error = "behavior tree exceeds max_tree_nodes";
            return false;
        }
        if (depth > limits_.maxTreeDepth) {
            error = "behavior tree exceeds max_tree_depth";
            return false;
        }

        lua_getfield(state_, nodeIndex, "kind");
        std::size_t kindLength{};
        const auto* kind = lua_tolstring(state_, -1, &kindLength);
        const auto validKind = kind != nullptr
            && IsCanonicalKind(std::string_view(kind, kindLength));
        const std::string kindValue = validKind
            ? std::string(kind, kindLength)
            : std::string{};
        lua_pop(state_, 1);
        if (!validKind) {
            error = "every behavior-tree node needs a canonical kind";
            return false;
        }
        if (!IsCompositeKind(kindValue) && !IsKnownLeafKind(kindValue)) {
            error = "behavior-tree node kind is not part of the V1 allowlist";
            return false;
        }

        bool sawRequiredParameter{};
        lua_pushnil(state_);
        while (lua_next(state_, nodeIndex) != 0) {
            if (lua_type(state_, -2) != LUA_TSTRING) {
                lua_pop(state_, 2);
                error = "behavior-tree node keys must be strings";
                return false;
            }
            std::size_t keyLength{};
            const auto* keyText = lua_tolstring(state_, -2, &keyLength);
            const std::string_view key(keyText, keyLength);
            const auto valueType = lua_type(state_, -1);
            if (!IsAllowedNodeField(kindValue, key)) {
                lua_pop(state_, 2);
                error = "behavior-tree node contains a field outside its V1 contract";
                return false;
            }
            if (key == "children") {
                if (valueType != LUA_TTABLE) {
                    lua_pop(state_, 2);
                    error = "behavior-tree children must be an array table";
                    return false;
                }
            } else if (key == "kind") {
                if (valueType != LUA_TSTRING) {
                    lua_pop(state_, 2);
                    error = "behavior-tree kind must be a string";
                    return false;
                }
            } else {
                lua_Integer minimum{};
                lua_Integer maximum{};
                if (!ScalarRange(kindValue, key, minimum, maximum)
                        || !lua_isinteger(state_, -1)) {
                    lua_pop(state_, 2);
                    error = "behavior-tree parameters must be bounded integers";
                    return false;
                }
                const auto value = lua_tointeger(state_, -1);
                if (value < minimum || value > maximum) {
                    lua_pop(state_, 2);
                    error = "behavior-tree parameter is outside its V1 range";
                    return false;
                }
                sawRequiredParameter = true;
            }
            lua_pop(state_, 1);
        }

        if ((kindValue == "target_distance_lte"
                || kindValue == "target_distance_gte"
                || kindValue == "owner_distance_lte"
                || kindValue == "owner_distance_gte"
                || kindValue == "target_owner_distance_lte"
                || kindValue == "target_owner_distance_gte"
                || kindValue == "cast")
                && !sawRequiredParameter) {
            error = "behavior-tree node is missing its required V1 parameter";
            return false;
        }

        lua_getfield(state_, nodeIndex, "children");
        if (lua_isnil(state_, -1)) {
            lua_pop(state_, 1);
            if (IsCompositeKind(kindValue)) {
                error = "selector and sequence nodes require children";
                return false;
            }
            return true;
        }
        if (!IsCompositeKind(kindValue)) {
            lua_pop(state_, 1);
            error = "behavior-tree leaves cannot contain children";
            return false;
        }
        if (!lua_istable(state_, -1)) {
            lua_pop(state_, 1);
            error = "behavior-tree children must be an array table";
            return false;
        }
        const auto childrenIndex = lua_absindex(state_, -1);
        const auto childCount = lua_rawlen(state_, childrenIndex);
        if (childCount == 0U) {
            lua_pop(state_, 1);
            error = "selector and sequence nodes require at least one child";
            return false;
        }
        summary.maximumChildren = std::max(
            summary.maximumChildren,
            childCount);
        if (childCount > limits_.maxChildrenPerNode) {
            lua_pop(state_, 1);
            error = "behavior tree exceeds max_children_per_node";
            return false;
        }

        std::size_t observedChildren{};
        lua_pushnil(state_);
        while (lua_next(state_, childrenIndex) != 0) {
            if (!lua_isinteger(state_, -2)) {
                lua_pop(state_, 3);
                error = "behavior-tree children must use contiguous integer keys";
                return false;
            }
            const auto key = lua_tointeger(state_, -2);
            if (key < 1 || static_cast<std::size_t>(key) > childCount
                    || !lua_istable(state_, -1)) {
                lua_pop(state_, 3);
                error = "behavior-tree children must be a contiguous node array";
                return false;
            }
            ++observedChildren;
            lua_pop(state_, 1);
        }
        if (observedChildren != childCount) {
            lua_pop(state_, 1);
            error = "behavior-tree children contain a hole or duplicate index";
            return false;
        }

        for (std::size_t child = 1U; child <= childCount; ++child) {
            lua_rawgeti(state_, childrenIndex, static_cast<lua_Integer>(child));
            const auto accepted = self(self, -1, depth + 1U);
            lua_pop(state_, 1);
            if (!accepted) {
                lua_pop(state_, 1);
                return false;
            }
        }
        lua_pop(state_, 1);
        return true;
    };
    return validate(validate, index, 1U);
}

auto Sandbox::LoadBehaviorTree(
        std::string_view source,
        TreeSummary& summary,
        std::string& error) -> bool {
    ScriptHandle handle = InvalidScriptHandle;
    if (!CompileBehaviorTree(source, handle, summary, error)) return false;
    ReleaseBehaviorTree(handle);
    return true;
}

auto Sandbox::CompileBehaviorTree(
        std::string_view source,
        ScriptHandle& handle,
        TreeSummary& summary,
        std::string& error) -> bool {
    handle = InvalidScriptHandle;
    summary = {};
    if (!ExecuteChunk(source, ExecutionPhase::Load, true, error)) return false;
    if (lua_gettop(state_) != 1 || !lua_istable(state_, 1)) {
        error = "a Scripted AI source must return exactly one behavior-tree table";
        lua_settop(state_, 0);
        return false;
    }
    const auto accepted = ValidateTree(1, summary, error);
    if (accepted) {
        lua_pushcfunction(state_, RetainBehaviorTree);
        lua_pushvalue(state_, 1);
        if (lua_pcall(state_, 1, 1, 0) != LUA_OK) {
            error = StackError(state_);
            lua_settop(state_, 0);
            return false;
        }
        handle = static_cast<ScriptHandle>(lua_tointeger(state_, -1));
    }
    lua_settop(state_, 0);
    return accepted;
}

void Sandbox::ReleaseBehaviorTree(ScriptHandle handle) noexcept {
    if (state_ != nullptr && handle >= 0) {
        luaL_unref(state_, LUA_REGISTRYINDEX, handle);
    }
}

auto Sandbox::EvaluateBehaviorTree(
        ScriptHandle handle,
        EphemeralThinkHandle& think,
        SandboxTickResult& result,
        std::string& error) -> bool {
    result = {};
    if (state_ == nullptr || evaluatorHandle_ == InvalidScriptHandle
            || evaluatorRunnerHandle_ == InvalidScriptHandle || handle < 0) {
        result.failure = SandboxTickFailure::ProtocolError;
        error = "Scripted AI evaluator state is unavailable";
        think.Invalidate();
        return false;
    }
    if (!think.IsValid()) {
        result.failure = SandboxTickFailure::StaleHandle;
        error = "Scripted AI think handle is stale";
        think.Invalidate();
        return false;
    }

    lua_settop(state_, 0);
    allocator_.inThink = false;
    remainingInstructions_ = limits_.maxInstructionsPerThink;
    activeLuaHandle_ = nullptr;
    lua_sethook(
        state_,
        InstructionHook,
        LUA_MASKCOUNT,
        static_cast<int>(limits_.instructionHookInterval));

    lua_rawgeti(state_, LUA_REGISTRYINDEX, evaluatorRunnerHandle_);
    lua_pushinteger(state_, static_cast<lua_Integer>(handle));
    lua_pushlightuserdata(state_, &think);
    const auto callStatus = lua_pcall(state_, 2, 1, 0);
    lua_sethook(state_, nullptr, 0, 0);
    allocator_.inThink = false;
    if (activeLuaHandle_ != nullptr) {
        activeLuaHandle_->handle = nullptr;
        activeLuaHandle_ = nullptr;
    }
    think.Invalidate();

    if (callStatus != LUA_OK) {
        error = StackError(state_);
        if (callStatus == LUA_ERRMEM) {
            result.failure = SandboxTickFailure::AllocationBudget;
        } else if (think.HadCapabilityError()) {
            result.failure = SandboxTickFailure::CapabilityError;
        } else if (think.HadStaleAccess()) {
            result.failure = SandboxTickFailure::StaleHandle;
        } else if (error.find("instruction budget exceeded")
                != error.npos) {
            result.failure = SandboxTickFailure::InstructionBudget;
        } else {
            result.failure = SandboxTickFailure::LuaError;
        }
        lua_settop(state_, 0);
        return false;
    }
    if (!lua_isinteger(state_, -1)) {
        result.failure = SandboxTickFailure::ProtocolError;
        error = "trusted Scripted AI evaluator returned a non-integer result";
        lua_settop(state_, 0);
        return false;
    }

    const auto evaluatorResult = static_cast<int>(lua_tointeger(state_, -1));
    lua_settop(state_, 0);
    if (think.HadSecondActionAttempt()) {
        result.failure = SandboxTickFailure::ProtocolError;
        error = "trusted Scripted AI evaluator attempted a second action";
        return false;
    }
    if (think.HadCapabilityError()) {
        result.failure = SandboxTickFailure::CapabilityError;
        error = "Scripted AI capability failed without a Lua error";
        return false;
    }

    switch (evaluatorResult) {
    case EvaluatorAction:
        if (!think.HasCommittedAction()) {
            result.failure = SandboxTickFailure::ProtocolError;
            error = "trusted Scripted AI evaluator reported an uncommitted action";
            return false;
        }
        result.status = SandboxTickStatus::Action;
        break;
    case EvaluatorFallback:
        if (think.HasCommittedAction() || think.HasScheduledFallback()) {
            result.failure = SandboxTickFailure::ProtocolError;
            error = "trusted Scripted AI evaluator discarded a terminal capability result";
            return false;
        }
        result.status = SandboxTickStatus::ExplicitFallback;
        break;
    case EvaluatorCapabilityFallback:
        if (think.HasCommittedAction() || !think.HasScheduledFallback()) {
            result.failure = SandboxTickFailure::ProtocolError;
            error = "trusted Scripted AI evaluator reported an invalid capability fallback";
            return false;
        }
        result.status = SandboxTickStatus::CapabilityFallback;
        break;
    case EvaluatorFailure:
    case EvaluatorSuccess:
        if (think.HasCommittedAction() || think.HasScheduledFallback()) {
            result.failure = SandboxTickFailure::ProtocolError;
            error = "trusted Scripted AI evaluator lost a terminal capability result";
            return false;
        }
        result.status = SandboxTickStatus::NoAction;
        break;
    default:
        result.failure = SandboxTickFailure::ProtocolError;
        error = "trusted Scripted AI evaluator returned an unknown result";
        return false;
    }

    result.failure = SandboxTickFailure::None;
    error.clear();
    return true;
}

auto Sandbox::HasGlobal(std::string_view name) const -> bool {
    if (state_ == nullptr || name.find('\0') != name.npos) return false;
    const std::string ownedName(name);
    lua_getglobal(state_, ownedName.c_str());
    const auto present = !lua_isnil(state_, -1);
    lua_pop(state_, 1);
    return present;
}

auto Sandbox::MemoryUsed() const noexcept -> std::size_t {
    return allocator_.used;
}

} // namespace ruffneckk::scripted_ai
