#include "router.hpp"
#include <cassert>
using namespace ruffneckk::potion_auto_pickup;
int main() {
    static_assert(Classify("hp5").family == Family::Healing);
    static_assert(Classify("mp4").tier == 4);
    static_assert(Classify("rvl").family == Family::Rejuvenation);
    static_assert(PackItemCode("hp2") == 0x20327068);
    static_assert(PackItemCode("mp2") == 0x2032706D);
    static_assert(PackItemCode("rvs") == 0x20737672);

    RoutingToken routingToken{610};
    assert(routingToken.Matches(610));
    assert(!routingToken.Matches(605));
    routingToken.Reset();
    assert(!routingToken.Matches(610));

    Policy everyHealing{};
    everyHealing.enabled=true;
    for(std::uint8_t tier=1;tier<=5;++tier) everyHealing.tiers[tier]=true;
    everyHealing.overflowTiers[1]=everyHealing.overflowTiers[3]=everyHealing.overflowTiers[5]=true;
    for(const auto code:{"hp1","hp2","hp3","hp4","hp5"}) {
        const auto item=Classify(code);
        assert(everyHealing.Accepts(item));
        assert(everyHealing.AllowsOverflow(item)==(item.tier%2==1));
    }
    assert(!everyHealing.Accepts(Classify("mp1")));

    Policy everyMana{};
    everyMana.enabled=true;
    for(std::uint8_t tier=1;tier<=5;++tier) {
        everyMana.tiers[tier]=true;
        everyMana.overflowTiers[tier]=true;
    }
    for(const auto code:{"mp1","mp2","mp3","mp4","mp5"}) {
        const auto item=Classify(code);
        assert(everyMana.Accepts(item));
        assert(everyMana.AllowsOverflow(item));
    }

    Policy everyRejuvenation{};
    everyRejuvenation.enabled=true;
    everyRejuvenation.tiers[1]=everyRejuvenation.tiers[2]=true;
    everyRejuvenation.overflowTiers[1]=everyRejuvenation.overflowTiers[2]=true;
    assert(everyRejuvenation.Accepts(Classify("rvs")));
    assert(everyRejuvenation.Accepts(Classify("rvl")));
    assert(everyRejuvenation.AllowsOverflow(Classify("rvs")));
    assert(everyRejuvenation.AllowsOverflow(Classify("rvl")));

    Policy healing{};
    healing.enabled=true;
    healing.tiers[2]=healing.tiers[3]=true;
    healing.columns={1,2,0,0};
    healing.columnCount=2;
    healing.overflowTiers[3]=true;

    std::array<BeltSlot,16> belt{};
    auto routed=Route(healing,Classify("hp2"),belt,16,true);
    assert(routed.destination==Destination::Column1 && routed.beltSlot==0);

    belt[0]={true,Family::Healing};
    routed=Route(healing,Classify("hp3"),belt,16,true);
    assert(routed.destination==Destination::Column1 && routed.beltSlot==4);

    belt[4]=belt[8]=belt[12]={true,Family::Healing};
    belt[1]={true,Family::Mana};
    routed=Route(healing,Classify("hp2"),belt,16,true);
    assert(routed.destination==Destination::Ground);
    routed=Route(healing,Classify("hp3"),belt,16,true);
    assert(routed.destination==Destination::Inventory);
    routed=Route(healing,Classify("hp3"),belt,16,false);
    assert(routed.destination==Destination::Ground);

    Policy rejuvenation{};
    rejuvenation.enabled=true;
    rejuvenation.tiers[1]=rejuvenation.tiers[2]=true;
    rejuvenation.columns={4,0,0,0};
    rejuvenation.columnCount=1;
    rejuvenation.overflowTiers[1]=rejuvenation.overflowTiers[2]=true;
    belt={};
    routed=Route(rejuvenation,Classify("rvl"),belt,8,true);
    assert(routed.destination==Destination::Column4 && routed.beltSlot==3);
    belt[3]=belt[7]={true,Family::Rejuvenation};
    routed=Route(rejuvenation,Classify("rvs"),belt,8,true);
    assert(routed.destination==Destination::Inventory);

    Policy inventoryOnly=everyMana;
    inventoryOnly.columnCount=0;
    belt={};
    routed=Route(inventoryOnly,Classify("mp5"),belt,16,true);
    assert(routed.destination==Destination::Inventory);
}
