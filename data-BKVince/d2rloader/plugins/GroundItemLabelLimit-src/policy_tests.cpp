#include "config_policy.hpp"

#include <cassert>
#include <cstdint>
#include <stdexcept>

namespace {
bool Rejects(const nlohmann::json& document) {
    try {
        (void)ruffneckk::ground_item_label_limit::ParseConfig(document);
        return false;
    } catch (const std::exception&) {
        return true;
    }
}
} // namespace

int main() {
    using namespace ruffneckk::ground_item_label_limit;

    assert(!IsSupportedLimit(32));
    assert(!IsSupportedLimit(63));
    assert(IsSupportedLimit(64));
    assert(!IsSupportedLimit(65));
    assert(!IsSupportedLimit(127));
    assert(IsSupportedLimit(128));
    assert(!IsSupportedLimit(129));
    assert(LabelArrayByteOffset(32) == 0x2880);
    assert(LabelArrayByteOffset(64) == 0x5100);
    assert(LabelArrayByteOffset(128) == 0xA200);

    const auto defaults = ParseConfig(nlohmann::json::object());
    assert(defaults.enabled && defaults.limit == 64);

    const auto configured64 = ParseConfig(nlohmann::json::parse(R"json(
        {
            // Comments are accepted by the runtime reader.
            "enabled": true,
            "limit": 64
        }
    )json", nullptr, true, true));
    assert(configured64.enabled && configured64.limit == 64);

    const auto configured128 = ParseConfig({{"enabled", true}, {"limit", 128}});
    assert(configured128.enabled && configured128.limit == 128);

    const auto disabled = ParseConfig({{"enabled", false}, {"limit", 128}});
    assert(!disabled.enabled && disabled.limit == 128);

    assert(Rejects(nlohmann::json::array()));
    assert(Rejects({{"limit", 32}}));
    assert(Rejects({{"limit", 63}}));
    assert(Rejects({{"limit", 65}}));
    assert(Rejects({{"limit", 127}}));
    assert(Rejects({{"limit", 129}}));
    assert(Rejects({{"limit", "64"}}));
    assert(Rejects({{"enabled", 1}}));
    assert(Rejects({{"extra", true}}));
}
