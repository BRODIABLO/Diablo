#include "scripted_ai_sandbox.hpp"

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <string>
#include <unordered_set>

namespace ruffneckk::scripted_ai {
namespace {

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
    if (state->inThink
            && proposed > state->thinkStart
                + state->thinkGrowthLimit) {
        return nullptr;
    }

    auto* replacement = std::realloc(pointer, newSize);
    if (replacement == nullptr) return nullptr;
    state->used = proposed;
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
    allocator_.inThink = phase == ExecutionPhase::Think;
    allocator_.thinkStart = allocator_.used;
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
        lua_pop(state_, 1);
        if (!validKind) {
            error = "every behavior-tree node needs a canonical kind";
            return false;
        }

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
            if (key == "children") {
                if (valueType != LUA_TTABLE) {
                    lua_pop(state_, 2);
                    error = "behavior-tree children must be an array table";
                    return false;
                }
            } else if (valueType != LUA_TBOOLEAN && valueType != LUA_TNUMBER
                    && valueType != LUA_TSTRING) {
                lua_pop(state_, 2);
                error = "behavior-tree fields must be scalar except for children";
                return false;
            }
            lua_pop(state_, 1);
        }

        lua_getfield(state_, nodeIndex, "children");
        if (lua_isnil(state_, -1)) {
            lua_pop(state_, 1);
            return true;
        }
        if (!lua_istable(state_, -1)) {
            lua_pop(state_, 1);
            error = "behavior-tree children must be an array table";
            return false;
        }
        const auto childrenIndex = lua_absindex(state_, -1);
        const auto childCount = lua_rawlen(state_, childrenIndex);
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
