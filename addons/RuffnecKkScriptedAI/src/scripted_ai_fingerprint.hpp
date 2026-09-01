#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace ruffneckk::scripted_ai {

struct NativeWindow {
    std::string_view name;
    std::uintptr_t rva;
    std::size_t size;
    std::string_view expectedHex;
    std::string_view expectedSha256;
    bool hookTarget;
};

inline constexpr std::size_t NativeWindowCount = 22U;
inline constexpr std::uintptr_t ResolverHookRva = 0x4A36C0U;

[[nodiscard]] auto NativeFingerprint() noexcept
    -> const std::array<NativeWindow, NativeWindowCount>&;

[[nodiscard]] auto DecodeNativeWindow(
    const NativeWindow& window,
    std::vector<std::uint8_t>& output,
    std::string& error) -> bool;

using NativeCheckCallback = bool(*)(
    void* userData,
    std::uintptr_t rva,
    std::span<const std::uint8_t> expected) noexcept;

struct FingerprintValidationResult {
    bool accepted{};
    std::size_t failedIndex{NativeWindowCount};
    std::string error;
};

[[nodiscard]] auto ValidateNativeFingerprint(
    NativeCheckCallback callback,
    void* userData) -> FingerprintValidationResult;

} // namespace ruffneckk::scripted_ai
