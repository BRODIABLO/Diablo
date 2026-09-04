#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace ruffneckk::scripted_ai::revive_v2 {

inline constexpr wchar_t ProviderModuleName[] =
    L"d2rl-ruffneckk-scripted-ai.dll";
inline constexpr char QueryExportName[] =
    "RuffnecKkScriptedAIQueryRevivePolicyV2";
inline constexpr std::uint32_t AbiVersion = 2U;
inline constexpr std::uint64_t InterfaceMagic = 0x3256495645524941ULL;
inline constexpr std::uint32_t CapabilityRequestNativeFollow = 1U << 0U;

enum class Result : std::uint32_t {
    DelegateNative = 0U,
    RequestNativeFollow = 1U,
    Unavailable = 2U,
    Error = 3U,
};

struct Context {
    std::uint32_t structSize{sizeof(Context)};
    std::uint32_t abiVersion{AbiVersion};
    void* game{};
    void* monster{};
    void* aiTickParam{};
    void* owner{};
    std::int32_t ownerDistance{-1};
    std::int32_t specialState{-1};
};

using EvaluateFunction = Result(__cdecl*)(const Context*) noexcept;

struct Interface {
    std::uint32_t structSize{sizeof(Interface)};
    std::uint32_t abiVersion{AbiVersion};
    std::uint64_t magic{InterfaceMagic};
    std::uint32_t capabilities{};
    std::uint32_t reserved{};
    EvaluateFunction evaluate{};
};

using QueryFunction = const Interface*(__cdecl*)(
    std::uint32_t requestedVersion,
    std::uint32_t callerInterfaceSize) noexcept;

[[nodiscard]] constexpr auto IsCompatible(
        const Interface* candidate) noexcept -> bool {
    return candidate != nullptr
        && candidate->structSize >= sizeof(Interface)
        && candidate->abiVersion == AbiVersion
        && candidate->magic == InterfaceMagic
        && (candidate->capabilities & CapabilityRequestNativeFollow) != 0U
        && candidate->evaluate != nullptr;
}

static_assert(std::is_standard_layout_v<Context>);
static_assert(std::is_trivially_copyable_v<Context>);
static_assert(std::is_standard_layout_v<Interface>);
static_assert(std::is_trivially_copyable_v<Interface>);
static_assert(offsetof(Context, structSize) == 0U);
static_assert(offsetof(Context, abiVersion) == 4U);
static_assert(sizeof(void*) != 8U || offsetof(Context, game) == 8U);
static_assert(sizeof(void*) != 8U || offsetof(Context, monster) == 16U);
static_assert(sizeof(void*) != 8U || offsetof(Context, aiTickParam) == 24U);
static_assert(sizeof(void*) != 8U || offsetof(Context, owner) == 32U);
static_assert(sizeof(void*) != 8U || offsetof(Context, ownerDistance) == 40U);
static_assert(sizeof(void*) != 8U || offsetof(Context, specialState) == 44U);
static_assert(offsetof(Interface, structSize) == 0U);
static_assert(offsetof(Interface, abiVersion) == 4U);
static_assert(offsetof(Interface, magic) == 8U);
static_assert(offsetof(Interface, capabilities) == 16U);
static_assert(offsetof(Interface, reserved) == 20U);
static_assert(sizeof(void*) != 8U || offsetof(Interface, evaluate) == 24U);
static_assert(sizeof(void*) != 8U || sizeof(Context) == 48U);
static_assert(sizeof(void*) != 8U || sizeof(Interface) == 32U);

} // namespace ruffneckk::scripted_ai::revive_v2

namespace ruffneckk::scripted_ai::revive_v3 {

inline constexpr wchar_t ProviderModuleName[] =
    L"d2rl-ruffneckk-scripted-ai.dll";
inline constexpr char QueryExportName[] =
    "RuffnecKkScriptedAIQueryReviveTacticsV3";
inline constexpr std::uint32_t AbiVersion = 3U;
inline constexpr std::uint64_t InterfaceMagic = 0x3356495645524941ULL;
inline constexpr std::uint32_t CapabilityTacticalActions = 1U << 0U;

enum class Result : std::uint32_t {
    DelegateNative = 0U,
    Handled = 1U,
    Unavailable = 2U,
    Error = 3U,
};

struct Context {
    std::uint32_t structSize{sizeof(Context)};
    std::uint32_t abiVersion{AbiVersion};
    void* game{};
    void* monster{};
    void* target{};
    void* owner{};
    const void* monStats{};
    std::int32_t ownerDistance{-1};
    std::int32_t targetDistance{-1};
    std::int32_t targetOwnerDistance{-1};
    std::int32_t specialState{-1};
    std::int32_t inCombat{};
};

using EvaluateFunction = Result(__cdecl*)(const Context*) noexcept;

struct Interface {
    std::uint32_t structSize{sizeof(Interface)};
    std::uint32_t abiVersion{AbiVersion};
    std::uint64_t magic{InterfaceMagic};
    std::uint32_t capabilities{};
    std::uint32_t reserved{};
    EvaluateFunction evaluate{};
};

using QueryFunction = const Interface*(__cdecl*)(
    std::uint32_t requestedVersion,
    std::uint32_t callerInterfaceSize) noexcept;

[[nodiscard]] constexpr auto IsCompatible(
        const Interface* candidate) noexcept -> bool {
    return candidate != nullptr
        && candidate->structSize >= sizeof(Interface)
        && candidate->abiVersion == AbiVersion
        && candidate->magic == InterfaceMagic
        && (candidate->capabilities & CapabilityTacticalActions) != 0U
        && candidate->evaluate != nullptr;
}

static_assert(std::is_standard_layout_v<Context>);
static_assert(std::is_trivially_copyable_v<Context>);
static_assert(std::is_standard_layout_v<Interface>);
static_assert(std::is_trivially_copyable_v<Interface>);
static_assert(offsetof(Context, structSize) == 0U);
static_assert(offsetof(Context, abiVersion) == 4U);
static_assert(sizeof(void*) != 8U || offsetof(Context, game) == 8U);
static_assert(sizeof(void*) != 8U || offsetof(Context, monster) == 16U);
static_assert(sizeof(void*) != 8U || offsetof(Context, target) == 24U);
static_assert(sizeof(void*) != 8U || offsetof(Context, owner) == 32U);
static_assert(sizeof(void*) != 8U || offsetof(Context, monStats) == 40U);
static_assert(sizeof(void*) != 8U || offsetof(Context, ownerDistance) == 48U);
static_assert(sizeof(void*) != 8U || offsetof(Context, targetDistance) == 52U);
static_assert(sizeof(void*) != 8U
    || offsetof(Context, targetOwnerDistance) == 56U);
static_assert(sizeof(void*) != 8U || offsetof(Context, specialState) == 60U);
static_assert(sizeof(void*) != 8U || offsetof(Context, inCombat) == 64U);
static_assert(offsetof(Interface, structSize) == 0U);
static_assert(offsetof(Interface, abiVersion) == 4U);
static_assert(offsetof(Interface, magic) == 8U);
static_assert(offsetof(Interface, capabilities) == 16U);
static_assert(offsetof(Interface, reserved) == 20U);
static_assert(sizeof(void*) != 8U || offsetof(Interface, evaluate) == 24U);
static_assert(sizeof(void*) != 8U || sizeof(Context) == 72U);
static_assert(sizeof(void*) != 8U || sizeof(Interface) == 32U);

} // namespace ruffneckk::scripted_ai::revive_v3
