#pragma once

#include <cstdint>

namespace RuffnecKk::BKVCombat {

inline constexpr char Version[] = "0.1.0";
inline constexpr std::uint32_t NativeApiVersion = 1;

enum class CriticalDeadlyOutcome : std::uint32_t {
    Unavailable = 0,
    None = 1,
    Critical = 2,
    Deadly = 3,
    AlreadyResolved = 4,
};

enum class NativeApiFlag : std::uint32_t {
    None = 0,
    SyntheticSecondary = 1U << 0U,
};

enum class NativeCapability : std::uint64_t {
    None = 0,
    CriticalDeadlyResolver = UINT64_C(1) << 0U,
};

using ResolveCriticalDeadlyFn = CriticalDeadlyOutcome(__fastcall*)(
    void* game,
    void* attacker,
    void* target,
    void* damage,
    std::int32_t damageMode,
    std::uint32_t flags) noexcept;
using GetActiveCapabilitiesFn = std::uint64_t(__cdecl*)() noexcept;

struct NativeApiV1 {
    std::uint32_t size;
    std::uint32_t version;
    std::uint64_t compiledCapabilities;
    ResolveCriticalDeadlyFn resolveCriticalDeadly;
    GetActiveCapabilitiesFn getActiveCapabilities;
};

} // namespace RuffnecKk::BKVCombat

#if defined(_WIN32)
extern "C" __declspec(dllexport)
auto __cdecl BKVCombat_GetApi(std::uint32_t requestedVersion) noexcept
    -> const RuffnecKk::BKVCombat::NativeApiV1*;
#endif
