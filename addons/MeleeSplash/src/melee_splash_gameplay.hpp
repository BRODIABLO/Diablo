#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>

namespace RuffnecKk::MeleeSplash {

inline constexpr std::uint16_t CriticalDeadlyResultFlag = 0x2000;
// The native squared-distance helper adds dx² + dy² in signed 32-bit space.
// 32767 keeps the worst-case diagonal sum below INT32_MAX.
inline constexpr std::int32_t MaximumSafeNativeRadius = 32767;

struct NativeSeedPair {
    std::uint32_t low{};
    std::uint32_t high{};
};

struct Native92777CriticalChances {
    std::int32_t weaponMastery{};
    std::int32_t passiveCritical{};
    std::int32_t deadlyStrike{};
};

enum class Native92777CriticalOutcome {
    None,
    WeaponMastery,
    PassiveCritical,
    DeadlyStrike,
};

inline std::uint32_t RollNativePercent(NativeSeedPair& seed) noexcept {
    const auto next = static_cast<std::uint64_t>(seed.low)
            * UINT64_C(0x6AC690C5)
        + seed.high;
    seed.low = static_cast<std::uint32_t>(next);
    seed.high = static_cast<std::uint32_t>(next >> 32);
    return seed.low % 100U;
}

inline Native92777CriticalOutcome RollNative92777CriticalDeadly(
        NativeSeedPair& seed,
        const Native92777CriticalChances& chances,
        bool allowWeaponMastery = true) noexcept {
    const auto succeeds = [&](std::int32_t chance) noexcept {
        return chance > 0
            && static_cast<std::int32_t>(RollNativePercent(seed)) < chance;
    };
    if (allowWeaponMastery && succeeds(chances.weaponMastery)) {
        return Native92777CriticalOutcome::WeaponMastery;
    }
    if (succeeds(chances.passiveCritical)) {
        return Native92777CriticalOutcome::PassiveCritical;
    }
    if (succeeds(chances.deadlyStrike)) {
        return Native92777CriticalOutcome::DeadlyStrike;
    }
    return Native92777CriticalOutcome::None;
}

inline std::int32_t ApplyNative92777CriticalMultiplier(
        std::int32_t physical,
        Native92777CriticalOutcome outcome) noexcept {
    if (outcome == Native92777CriticalOutcome::None) return physical;
    // D2R 3.2.92777 uses `add eax,eax`: reproduce the native modulo-2^32
    // behavior without invoking signed-overflow undefined behavior in C++.
    const auto bits = static_cast<std::uint32_t>(physical);
    return static_cast<std::int32_t>(bits + bits);
}

inline std::int32_t ScaleFixedDamage(
        std::int32_t value, std::int32_t percent) noexcept {
    if (value <= 0 || percent <= 0) return 0;
    const auto scaled = static_cast<std::int64_t>(value) * percent / 100;
    return static_cast<std::int32_t>(std::min<std::int64_t>(
        scaled, std::numeric_limits<std::int32_t>::max()));
}

inline std::int32_t ClampNativeRadius(std::int32_t radius) noexcept {
    return std::clamp(radius, 0, MaximumSafeNativeRadius);
}

} // namespace RuffnecKk::MeleeSplash
