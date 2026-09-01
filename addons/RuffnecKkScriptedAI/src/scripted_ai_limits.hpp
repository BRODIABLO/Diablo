#pragma once

#include <cstddef>
#include <cstdint>

namespace ruffneckk::scripted_ai {

struct SandboxLimits {
    std::size_t maxSourceBytes{256U * 1024U};
    std::size_t maxTreeNodes{256U};
    std::size_t maxTreeDepth{32U};
    std::size_t maxChildrenPerNode{32U};
    std::size_t sessionHeapBytes{16U * 1024U * 1024U};
    std::size_t perThinkHeapGrowthBytes{64U * 1024U};
    std::uint32_t maxInstructionsPerThink{25'000U};
    std::uint32_t instructionHookInterval{500U};
    std::uint32_t maxLoadInstructions{250'000U};
    std::uint32_t maxScriptErrorsPerSession{3U};
    std::uint32_t maxSlowStrikesPerSession{3U};
    std::uint32_t slowThinkMicroseconds{2'000U};
    std::uint32_t detailedLogIntervalMilliseconds{5'000U};
};

inline constexpr SandboxLimits HardSandboxLimits{};
inline constexpr std::uint32_t AcceptanceP99Microseconds = 50U;
inline constexpr std::uint32_t AcceptanceBudgetMicrosecondsPer40Ms = 2'000U;

[[nodiscard]] constexpr auto IsWithinHardLimits(
        const SandboxLimits& value) noexcept -> bool {
    return value.maxSourceBytes >= 1'024U
        && value.maxSourceBytes <= HardSandboxLimits.maxSourceBytes
        && value.maxTreeNodes >= 1U
        && value.maxTreeNodes <= HardSandboxLimits.maxTreeNodes
        && value.maxTreeDepth >= 1U
        && value.maxTreeDepth <= HardSandboxLimits.maxTreeDepth
        && value.maxChildrenPerNode >= 1U
        && value.maxChildrenPerNode
            <= HardSandboxLimits.maxChildrenPerNode
        && value.sessionHeapBytes >= 1024U * 1024U
        && value.sessionHeapBytes <= HardSandboxLimits.sessionHeapBytes
        && value.perThinkHeapGrowthBytes >= 4U * 1024U
        && value.perThinkHeapGrowthBytes
            <= HardSandboxLimits.perThinkHeapGrowthBytes
        && value.maxInstructionsPerThink >= 500U
        && value.maxInstructionsPerThink
            <= HardSandboxLimits.maxInstructionsPerThink
        && value.instructionHookInterval >= 50U
        && value.instructionHookInterval
            <= HardSandboxLimits.instructionHookInterval
        && value.maxInstructionsPerThink % value.instructionHookInterval == 0U
        && value.maxLoadInstructions >= value.maxInstructionsPerThink
        && value.maxLoadInstructions <= HardSandboxLimits.maxLoadInstructions
        && value.maxLoadInstructions % value.instructionHookInterval == 0U
        && value.maxScriptErrorsPerSession >= 1U
        && value.maxScriptErrorsPerSession
            <= HardSandboxLimits.maxScriptErrorsPerSession
        && value.maxSlowStrikesPerSession >= 1U
        && value.maxSlowStrikesPerSession
            <= HardSandboxLimits.maxSlowStrikesPerSession
        && value.slowThinkMicroseconds >= 100U
        && value.slowThinkMicroseconds
            <= HardSandboxLimits.slowThinkMicroseconds
        && value.detailedLogIntervalMilliseconds >= 1'000U
        && value.detailedLogIntervalMilliseconds
            <= HardSandboxLimits.detailedLogIntervalMilliseconds;
}

} // namespace ruffneckk::scripted_ai
