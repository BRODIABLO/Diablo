#include <D2RLPlugin/api.h>
#include "router.hpp"
#include <Windows.h>
#include <algorithm>
#include <array>
#include <cctype>
#include <climits>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

namespace {
using namespace ruffneckk::potion_auto_pickup;

constexpr std::uintptr_t GetGameRva=0x34B440, EnumerateRva=0x2EFDE0;
constexpr std::uintptr_t FirstUnitRva=0x2EFD90, NextUnitRva=0x34B4A0, UnitTypeRva=0x34B9D0;
constexpr std::uintptr_t UnitIdRva=0x34A330, UnitModeRva=0x34AB60, UnitDistanceRva=0x325140;
constexpr std::uintptr_t UnitCollisionRva=0x350550, PickupRva=0x471950;
constexpr std::uintptr_t GetInventoryRva=0x34A360, ResolveOccupancyGridRva=0x38B070;
constexpr std::uintptr_t GetBeltTypeRva=0x349720, GetFreeBeltSlotRva=0x3862D0;
constexpr std::uintptr_t BodyGridInfoRva=0x237B620, BeltGridInfoRva=0x237B638;
constexpr std::uintptr_t ServerPacketTableRva=0x1D2A790;
constexpr std::uint32_t ItemType=4, GroundMode=3, PickupCollisionMask=0x804;
constexpr std::uint32_t SupportedBuild=92777;
constexpr std::uint8_t FirstTriggerOpcode=0x01, LastTriggerOpcode=0x12;
constexpr std::uint8_t BeltBodySlot=8;

constexpr std::array<std::uintptr_t,LastTriggerOpcode+1> TriggerHandlerRvas{
    0,
    0x4AC050,0x4ACE20,0x4ACE40,0x4ACE60,0x4ACE80,0x4ACF80,
    0x4AD030,0x4AD0E0,0x4AD100,0x4AD120,0x4AD140,0x4AD230,
    0x4AD330,0x4AD3E0,0x4AD490,0x4AD4B0,0x4AD4D0,0x4AD4F0
};
constexpr std::array<std::uint8_t,32> GetFreeBeltSlotExpected{
    0x40,0x53,0x55,0x56,0x57,0x41,0x54,0x41,
    0x56,0x41,0x57,0x48,0x81,0xEC,0x70,0x01,
    0x00,0x00,0x48,0x8B,0x05,0xDF,0x4F,0x64,
    0x02,0x48,0x33,0xC4,0x48,0x89,0x84,0x24
};
constexpr std::array<std::uint8_t,32> ResolveOccupancyGridExpected{
    0x4C,0x8B,0xDC,0x49,0x89,0x5B,0x20,0x57,
    0x48,0x83,0xEC,0x30,0x49,0x8B,0xF8,0x48,
    0x39,0x51,0x28,0x0F,0x86,0x01,0x01,0x00,
    0x00,0x49,0x89,0x73,0x18,0x48,0x8D,0x71
};
constexpr std::array<std::uint8_t,32> GetInventoryExpected{
    0x48,0x89,0x5C,0x24,0x18,0x56,0x48,0x83,
    0xEC,0x20,0x48,0x8B,0xF1,0x48,0x85,0xC9,
    0x75,0x13,0x88,0x4C,0x24,0x30,0x48,0x8D,
    0x4C,0x24,0x30,0xE8,0x70,0xCC,0xFF,0xFF
};
constexpr std::array<std::uint8_t,32> GetBeltTypeExpected{
    0x48,0x89,0x5C,0x24,0x10,0x57,0x48,0x83,
    0xEC,0x20,0x48,0x8B,0xD9,0x48,0x85,0xC9,
    0x75,0x15,0x88,0x4C,0x24,0x30,0x48,0x8D,
    0x4C,0x24,0x30,0xE8,0xE0,0xC0,0xFF,0xFF
};

// BKVince 3.2 combined Weapons (307) + Armor (218) + Misc row indices.
struct PotionClass { std::uint32_t id; std::string_view code; Family family; std::uint8_t tier; };
constexpr std::array<PotionClass,12> PotionClasses{
    PotionClass{604,"hp1",Family::Healing,1}, PotionClass{605,"hp2",Family::Healing,2},
    PotionClass{606,"hp3",Family::Healing,3}, PotionClass{607,"hp4",Family::Healing,4}, PotionClass{608,"hp5",Family::Healing,5},
    PotionClass{609,"mp1",Family::Mana,1}, PotionClass{610,"mp2",Family::Mana,2}, PotionClass{611,"mp3",Family::Mana,3},
    PotionClass{612,"mp4",Family::Mana,4}, PotionClass{613,"mp5",Family::Mana,5},
    PotionClass{532,"rvs",Family::Rejuvenation,1}, PotionClass{533,"rvl",Family::Rejuvenation,2},
};

struct FamilyConfig {
    Policy policy{};
    bool legacyOverflow{};
    bool explicitOverflowTiers{};
    std::array<std::uint8_t,5> tierPriority{};
    std::uint8_t tierPriorityCount{};
};
struct Config {
    bool enabled=true, diagnostics=false;
    std::uint32_t distance=4, interval=3;
    FamilyConfig healing{}, mana{}, rejuvenation{};
    std::array<Family,3> familyPriority{Family::Rejuvenation,Family::Healing,Family::Mana};
    std::uint8_t familyPriorityCount=3;
};

using TriggerFn=std::int64_t(__fastcall*)(void*,void*,void*,std::int32_t);
using GetGameFn=void*(__fastcall*)(void*);
using EnumerateFn=void(__fastcall*)(void*,void***,std::uint32_t*);
using UnitFn=void*(__fastcall*)(void*);
using UnitIntFn=std::uint32_t(__fastcall*)(void*);
using UnitPairFn=std::int32_t(__fastcall*)(void*,void*);
using CollisionFn=std::int32_t(__fastcall*)(void*,void*,std::uint32_t);
using PickupFn=bool(__fastcall*)(void*,std::uint32_t,bool,std::uint32_t,bool,bool);
using GetInventoryFn=void*(__fastcall*)(void*);
using GetBeltTypeFn=std::int32_t(__fastcall*)(void*);
using ResolveOccupancyGridFn=void*(__fastcall*)(void*,std::uint64_t,const void*);
using GetFreeBeltSlotFn=std::int32_t(__fastcall*)(void*,void*,std::int32_t*,bool);

const D2RL::PluginContext* Context{};
std::uint8_t* Base{};
std::array<TriggerFn,LastTriggerOpcode+1> OriginalTriggers{};
GetFreeBeltSlotFn OriginalGetFreeBeltSlot{};
GetGameFn GetGame{}; EnumerateFn Enumerate{}; UnitFn FirstUnit{},NextUnit{};
UnitIntFn UnitType{},UnitId{},UnitMode{}; UnitPairFn UnitDistance{}; CollisionFn UnitCollision{}; PickupFn Pickup{};
GetInventoryFn GetInventory{}; GetBeltTypeFn GetBeltType{}; ResolveOccupancyGridFn ResolveOccupancyGrid{};
Config Settings{};
thread_local bool Inside{};
thread_local std::uint32_t TriggerCounter{};
thread_local void* ForcedInventory{};
thread_local void* ForcedItem{};
thread_local std::int32_t ForcedBeltSlot{-1};
thread_local bool ForceInventoryOverflow{};
thread_local bool LoggedScanException{};

constexpr D2RL::PluginInfo Info{
    .infoSize=D2RL::PluginInfoSize, .apiVersion=D2RL_PLUGIN_API_VERSION,
    .id="potion-auto-pickup", .name="PotionAutoPickup", .version="1.1.1",
    .author="RuffnecKk", .description="Automatically picks up configured potions when belt and inventory rules allow.",
    .flags=D2RL::PluginFlags::NativeHooks,
};

std::string Trim(std::string text) {
    const auto first=text.find_first_not_of(" \t\r\n"); if(first==std::string::npos) return {};
    const auto last=text.find_last_not_of(" \t\r\n"); return text.substr(first,last-first+1);
}
bool BoolValue(std::string_view value,bool fallback) { return value=="true" ? true : value=="false" ? false : fallback; }
std::uint32_t UIntValue(std::string_view value,std::uint32_t fallback) {
    try { return static_cast<std::uint32_t>(std::stoul(std::string(value))); } catch(...) { return fallback; }
}
FamilyConfig* Section(std::string_view section) {
    if(section=="healing") return &Settings.healing; if(section=="mana") return &Settings.mana;
    if(section=="rejuvenation") return &Settings.rejuvenation; return nullptr;
}
Family SectionFamily(std::string_view section) {
    if(section=="healing") return Family::Healing;
    if(section=="mana") return Family::Mana;
    if(section=="rejuvenation") return Family::Rejuvenation;
    return Family::Unknown;
}
template<class Visitor>
void ForEachString(std::string_view value,Visitor visitor) {
    std::size_t position=0;
    while((position=value.find('"',position))!=std::string_view::npos) {
        const auto end=value.find('"',position+1);
        if(end==std::string_view::npos) return;
        visitor(value.substr(position+1,end-position-1));
        position=end+1;
    }
}
const PotionClass* ClassifyCode(std::string_view code,Family family) {
    for(const auto& potion:PotionClasses) {
        if(potion.family==family && potion.code==code) return &potion;
    }
    return nullptr;
}
void ParseTierSet(
    Family family,
    std::string_view value,
    std::array<bool,6>& output) {
    output.fill(false);
    ForEachString(value,[&](std::string_view code) {
        if(const auto* potion=ClassifyCode(code,family)) output[potion->tier]=true;
    });
}
void ParseColumns(FamilyConfig& config,std::string_view value) {
    config.policy.columns.fill(0);
    config.policy.columnCount=0;
    std::array<bool,4> seen{};
    for(const char character:value) {
        if(character<'1' || character>'4') continue;
        const auto column=static_cast<std::uint8_t>(character-'0');
        if(seen[column-1]) continue;
        seen[column-1]=true;
        config.policy.columns[config.policy.columnCount++]=column;
    }
}
void ParseTierPriority(FamilyConfig& config,Family family,std::string_view value) {
    config.tierPriority.fill(0);
    config.tierPriorityCount=0;
    std::array<bool,6> seen{};
    ForEachString(value,[&](std::string_view code) {
        const auto* potion=ClassifyCode(code,family);
        if(!potion || seen[potion->tier] || config.tierPriorityCount>=config.tierPriority.size()) return;
        seen[potion->tier]=true;
        config.tierPriority[config.tierPriorityCount++]=potion->tier;
    });
}
void ParseFamilyPriority(std::string_view value) {
    Settings.familyPriorityCount=0;
    std::array<bool,3> seen{};
    ForEachString(value,[&](std::string_view name) {
        Family family=Family::Unknown;
        std::size_t index=0;
        if(name=="healing") { family=Family::Healing; index=0; }
        else if(name=="mana") { family=Family::Mana; index=1; }
        else if(name=="rejuvenation") { family=Family::Rejuvenation; index=2; }
        if(family==Family::Unknown || seen[index] || Settings.familyPriorityCount>=Settings.familyPriority.size()) return;
        seen[index]=true;
        Settings.familyPriority[Settings.familyPriorityCount++]=family;
    });
}
void SetDefaults() {
    Settings={};
    auto prepare=[](FamilyConfig& config) {
        config.policy.enabled=true;
    };
    prepare(Settings.healing);
    Settings.healing.policy.tiers[4]=Settings.healing.policy.tiers[5]=true;
    Settings.healing.policy.columns={1,0,0,0};
    Settings.healing.policy.columnCount=1;
    Settings.healing.tierPriority={5,4,0,0,0};
    Settings.healing.tierPriorityCount=2;

    prepare(Settings.mana);
    Settings.mana.policy.tiers[4]=Settings.mana.policy.tiers[5]=true;
    Settings.mana.policy.columns={2,0,0,0};
    Settings.mana.policy.columnCount=1;
    Settings.mana.tierPriority={5,4,0,0,0};
    Settings.mana.tierPriorityCount=2;

    prepare(Settings.rejuvenation);
    Settings.rejuvenation.policy.tiers[1]=Settings.rejuvenation.policy.tiers[2]=true;
    Settings.rejuvenation.policy.columns={3,4,0,0};
    Settings.rejuvenation.policy.columnCount=2;
    Settings.rejuvenation.legacyOverflow=true;
    Settings.rejuvenation.tierPriority={2,1,0,0,0};
    Settings.rejuvenation.tierPriorityCount=2;
}
void FinalizeOverflow(FamilyConfig& config) {
    if(config.explicitOverflowTiers) return;
    for(std::size_t tier=0;tier<config.policy.overflowTiers.size();++tier) {
        config.policy.overflowTiers[tier]=config.legacyOverflow && config.policy.tiers[tier];
    }
}
bool LoadConfig() {
    if(!Context->pluginConfigPath) return false;
    const auto path=std::filesystem::path(Context->pluginConfigPath).parent_path()/L"PotionAutoPickup.toml";
    std::ifstream stream(path,std::ios::binary); if(!stream) return false;
    const std::string input((std::istreambuf_iterator<char>(stream)),std::istreambuf_iterator<char>());
    SetDefaults();
    std::string section; std::size_t start=0;
    while(start<input.size()) {
        auto end=input.find('\n',start); auto line=Trim(input.substr(start,end-start)); start=end==std::string::npos?input.size():end+1;
        if(auto hash=line.find('#'); hash!=std::string::npos) line=Trim(line.substr(0,hash)); if(line.empty()) continue;
        if(line.front()=='[' && line.back()==']') { section=line.substr(1,line.size()-2); continue; }
        const auto equal=line.find('='); if(equal==std::string::npos) continue;
        const auto key=Trim(line.substr(0,equal)), value=Trim(line.substr(equal+1));
        if(section.empty()) {
            if(key=="enabled") Settings.enabled=BoolValue(value,Settings.enabled);
            else if(key=="pickup_distance") Settings.distance=std::clamp(UIntValue(value,4u),1u,4u);
            else if(key=="minimum_interval_actions" || key=="minimum_interval_frames") Settings.interval=std::clamp(UIntValue(value,3u),1u,25u);
            else if(key=="family_priority") ParseFamilyPriority(value);
        } else if(auto* family=Section(section)) {
            const auto familyId=SectionFamily(section);
            if(key=="enabled") family->policy.enabled=BoolValue(value,family->policy.enabled);
            else if(key=="tiers") ParseTierSet(familyId,value,family->policy.tiers);
            else if(key=="columns") ParseColumns(*family,value);
            else if(key=="overflow_to_inventory") family->legacyOverflow=BoolValue(value,family->legacyOverflow);
            else if(key=="overflow_tiers") {
                ParseTierSet(familyId,value,family->policy.overflowTiers);
                family->explicitOverflowTiers=true;
            } else if(key=="tier_priority") ParseTierPriority(*family,familyId,value);
        } else if(section=="diagnostics" && key=="enabled") Settings.diagnostics=BoolValue(value,Settings.diagnostics);
    }
    FinalizeOverflow(Settings.healing);
    FinalizeOverflow(Settings.mana);
    FinalizeOverflow(Settings.rejuvenation);
    return true;
}

const PotionClass* ClassifyId(std::uint32_t id) { for(const auto& p:PotionClasses) if(p.id==id) return &p; return nullptr; }
const FamilyConfig& FamilySettings(Family family) {
    if(family==Family::Healing) return Settings.healing; if(family==Family::Mana) return Settings.mana; return Settings.rejuvenation;
}
bool Accepted(const PotionClass& potion) {
    return FamilySettings(potion.family).policy.Accepts({potion.code,potion.family,potion.tier});
}
std::uint8_t FamilyRank(Family family) {
    for(std::uint8_t index=0;index<Settings.familyPriorityCount;++index) {
        if(Settings.familyPriority[index]==family) return index;
    }
    return static_cast<std::uint8_t>(Settings.familyPriorityCount+static_cast<std::uint8_t>(family));
}
std::uint8_t TierRank(const FamilyConfig& config,std::uint8_t tier) {
    for(std::uint8_t index=0;index<config.tierPriorityCount;++index) {
        if(config.tierPriority[index]==tier) return index;
    }
    return static_cast<std::uint8_t>(config.tierPriorityCount+5-tier);
}
bool ReadBeltState(
    void* inventory,
    std::array<BeltSlot,16>& slots,
    std::uint8_t& capacity) noexcept {
    __try {
        if(!inventory || !GetBeltType || !ResolveOccupancyGrid) return false;
        auto* bodyGrid=static_cast<std::uint8_t*>(ResolveOccupancyGrid(
            inventory,0,Base+BodyGridInfoRva));
        if(!bodyGrid) return false;
        auto** bodyItems=*reinterpret_cast<void***>(bodyGrid+0x18);
        if(!bodyItems) return false;
        void* belt=bodyItems[BeltBodySlot];
        const auto beltType=belt?GetBeltType(belt):2;
        switch(beltType) {
        case 0: case 5: capacity=12; break;
        case 1: case 4: capacity=8; break;
        case 2: capacity=4; break;
        case 3: case 6: capacity=16; break;
        default: return false;
        }
        auto* beltGrid=static_cast<std::uint8_t*>(ResolveOccupancyGrid(
            inventory,1,Base+BeltGridInfoRva));
        if(!beltGrid) return false;
        const auto cells=static_cast<std::uint32_t>(beltGrid[0x10])*beltGrid[0x11];
        if(cells<capacity || cells>slots.size()) return false;
        auto** items=*reinterpret_cast<void***>(beltGrid+0x18);
        if(!items) return false;
        for(std::uint8_t index=0;index<capacity;++index) {
            if(!items[index]) continue;
            slots[index].occupied=true;
            const auto classId=*reinterpret_cast<std::uint32_t*>(
                static_cast<std::uint8_t*>(items[index])+4);
            if(const auto* potion=ClassifyId(classId)) slots[index].family=potion->family;
        }
        return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}
std::string JoinCodes(Family family,const std::array<bool,6>& enabled) {
    std::string output;
    for(const auto& potion:PotionClasses) {
        if(potion.family!=family || !enabled[potion.tier]) continue;
        if(!output.empty()) output.push_back(',');
        output.append(potion.code);
    }
    return output.empty()?"none":output;
}
std::string JoinColumns(const Policy& policy) {
    std::string output;
    for(std::uint8_t index=0;index<policy.columnCount;++index) {
        if(!output.empty()) output.push_back(',');
        output.append(std::to_string(policy.columns[index]));
    }
    return output.empty()?"none":output;
}
std::string FamilySummary(std::string_view name,Family family,const FamilyConfig& config) {
    std::string output(name);
    output.append(" tiers=").append(JoinCodes(family,config.policy.tiers));
    output.append(" columns=").append(JoinColumns(config.policy));
    output.append(" overflow=").append(JoinCodes(family,config.policy.overflowTiers));
    return output;
}
std::string Summary() {
    std::string output="PotionAutoPickup 1.1.1 active; triggers=0x01-0x12; ";
    output.append(FamilySummary("healing",Family::Healing,Settings.healing)).append("; ");
    output.append(FamilySummary("mana",Family::Mana,Settings.mana)).append("; ");
    output.append(FamilySummary("rejuvenation",Family::Rejuvenation,Settings.rejuvenation)).append(".");
    return output;
}
std::int32_t __fastcall HookGetFreeBeltSlot(
    void* inventory,
    void* item,
    std::int32_t* freeSlot,
    bool allowAnyBeltable) {
    if(Inside && inventory==ForcedInventory && item==ForcedItem) {
        if(ForceInventoryOverflow) {
            if(freeSlot) *freeSlot=-1;
            return 0;
        }
        if(freeSlot && ForcedBeltSlot>=0 && ForcedBeltSlot<16) {
            *freeSlot=ForcedBeltSlot;
            return 1;
        }
        return 0;
    }
    return OriginalGetFreeBeltSlot(inventory,item,freeSlot,allowAnyBeltable);
}

void ResetRoutingScope() noexcept {
    ForceInventoryOverflow=false;
    ForcedBeltSlot=-1;
    ForcedItem=nullptr;
    ForcedInventory=nullptr;
    Inside=false;
}
void ScanUnsafe(void* player) {
    if(!Settings.enabled || Inside || !player || (++TriggerCounter%Settings.interval)!=0) return;
    void* game=GetGame(player); if(!game) return;
    void* inventory=GetInventory(player); if(!inventory) return;
    std::array<BeltSlot,16> belt{};
    std::uint8_t beltCapacity{};
    if(!ReadBeltState(inventory,belt,beltCapacity)) return;
    void** buckets=nullptr; std::uint32_t count=0; Enumerate(game,&buckets,&count); if(!buckets || !count || count>4096) return;
    void* best=nullptr; const PotionClass* bestPotion=nullptr; std::int32_t bestDistance=INT_MAX,bestBeltSlot=-1;
    bool bestOverflow{};
    std::uint8_t bestFamilyRank=UINT8_MAX,bestTierRank=UINT8_MAX;
    for(std::uint32_t i=0;i<count;i++) for(void* unit=FirstUnit(buckets[i]);unit;unit=NextUnit(unit)) {
        if(UnitType(unit)!=ItemType || UnitMode(unit)!=GroundMode) continue;
        const auto* potion=ClassifyId(*reinterpret_cast<std::uint32_t*>(static_cast<std::uint8_t*>(unit)+4));
        if(!potion || !Accepted(*potion) || UnitCollision(player,unit,PickupCollisionMask)!=0) continue;
        const auto distance=UnitDistance(player,unit); if(distance<0 || static_cast<std::uint32_t>(distance)>Settings.distance) continue;
        const auto& family=FamilySettings(potion->family);
        const Item item{potion->code,potion->family,potion->tier};
        const auto beltSlot=ChooseBeltSlot(family.policy,item,belt,beltCapacity);
        const bool overflow=beltSlot<0 && family.policy.AllowsOverflow(item);
        if(beltSlot<0 && !overflow) continue;
        const auto familyRank=FamilyRank(potion->family),tierRank=TierRank(family,potion->tier);
        const bool better=!best
            || familyRank<bestFamilyRank
            || (familyRank==bestFamilyRank && tierRank<bestTierRank)
            || (familyRank==bestFamilyRank && tierRank==bestTierRank && distance<bestDistance);
        if(better) {
            best=unit; bestPotion=potion; bestDistance=distance; bestBeltSlot=beltSlot;
            bestOverflow=overflow; bestFamilyRank=familyRank; bestTierRank=tierRank;
        }
    }
    if(!best || !bestPotion) return;
    Inside=true; ForcedInventory=inventory; ForcedItem=best;
    ForcedBeltSlot=bestBeltSlot; ForceInventoryOverflow=bestOverflow;
    // Same server pickup routine and flags used by vanilla automatic gold pickup.
    Pickup(player,UnitId(best),true,Settings.distance,true,false);
    ResetRoutingScope();
}

std::uint32_t ScanProtected(void* player) noexcept {
    __try {
        ScanUnsafe(player);
        return 0;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        ResetRoutingScope();
        return GetExceptionCode();
    }
}
void Scan(void* player) {
    const auto exception=ScanProtected(player);
    if(exception && !LoggedScanException && Context) {
        LoggedScanException=true;
        Context->LogWarn("PotionAutoPickup: one automatic scan was skipped after a structured exception.");
    }
}
std::int64_t __fastcall HookTrigger(void* game,void* player,void* packet,std::int32_t size) {
    const auto opcode=packet && size>0
        ? *static_cast<const std::uint8_t*>(packet)
        : static_cast<std::uint8_t>(0);
    if(opcode<FirstTriggerOpcode || opcode>LastTriggerOpcode || !OriginalTriggers[opcode]) return 1;
    const auto result=OriginalTriggers[opcode](game,player,packet,size);
    Scan(player);
    return result;
}
auto Status(D2R::Game::Client*,const D2RL::ConsoleCommandContext* command,void*) noexcept -> D2RL::ConsoleCommandResult {
    if(!command || !command->plugin) return D2RL::ConsoleCommandResult::Failed;
    const auto summary=Summary();
    command->plugin->WriteConsoleMessage(summary.c_str());
    return D2RL::ConsoleCommandResult::Handled;
}
template<class T> T At(std::uintptr_t rva) { return reinterpret_cast<T>(Base+rva); }
bool ValidateTriggerTable() {
    for(std::uint8_t opcode=FirstTriggerOpcode;opcode<=LastTriggerOpcode;++opcode) {
        const auto expected=reinterpret_cast<std::uintptr_t>(Base+TriggerHandlerRvas[opcode]);
        if(!Context->CheckExpectedBytes(
                ServerPacketTableRva+static_cast<std::uintptr_t>(opcode)*sizeof(std::uintptr_t),
                &expected,
                static_cast<std::uint32_t>(sizeof(expected)))) return false;
    }
    return true;
}
bool InstallTriggerTablePatches() {
    const auto replacement=static_cast<std::uint64_t>(
        reinterpret_cast<std::uintptr_t>(&HookTrigger));
    for(std::uint8_t opcode=FirstTriggerOpcode;opcode<=LastTriggerOpcode;++opcode) {
        const auto expected=reinterpret_cast<std::uintptr_t>(Base+TriggerHandlerRvas[opcode]);
        OriginalTriggers[opcode]=reinterpret_cast<TriggerFn>(expected);
        if(!Context->PatchWriteU64(
                ServerPacketTableRva+static_cast<std::uintptr_t>(opcode)*sizeof(std::uintptr_t),
                &expected,
                static_cast<std::uint32_t>(sizeof(expected)),
                replacement)) return false;
    }
    return true;
}
}

D2RL_PLUGIN_EXPORT auto D2RLoaderGetPluginInfo() noexcept -> const D2RL::PluginInfo* { return &Info; }
D2RL_PLUGIN_EXPORT auto D2RLoaderLoadPlugin(const D2RL::PluginContext* context) noexcept -> bool {
    if(!context) return false; Context=context;
    Base=reinterpret_cast<std::uint8_t*>(GetModuleHandleW(nullptr)); if(!Base || !LoadConfig()) return false;
    if(context->modDataVersionBuild!=0 && context->modDataVersionBuild!=SupportedBuild) {
        context->LogError("PotionAutoPickup: only D2R build 92777 is supported.");
        return false;
    }
    GetGame=At<GetGameFn>(GetGameRva); Enumerate=At<EnumerateFn>(EnumerateRva); FirstUnit=At<UnitFn>(FirstUnitRva); NextUnit=At<UnitFn>(NextUnitRva);
    UnitType=At<UnitIntFn>(UnitTypeRva); UnitId=At<UnitIntFn>(UnitIdRva); UnitMode=At<UnitIntFn>(UnitModeRva); UnitDistance=At<UnitPairFn>(UnitDistanceRva);
    UnitCollision=At<CollisionFn>(UnitCollisionRva); Pickup=At<PickupFn>(PickupRva);
    GetInventory=At<GetInventoryFn>(GetInventoryRva);
    GetBeltType=At<GetBeltTypeFn>(GetBeltTypeRva);
    ResolveOccupancyGrid=At<ResolveOccupancyGridFn>(ResolveOccupancyGridRva);

    if(!ValidateTriggerTable()
        || !context->CheckExpectedBytes(GetFreeBeltSlotRva,GetFreeBeltSlotExpected.data(),static_cast<std::uint32_t>(GetFreeBeltSlotExpected.size()))
        || !context->CheckExpectedBytes(ResolveOccupancyGridRva,ResolveOccupancyGridExpected.data(),static_cast<std::uint32_t>(ResolveOccupancyGridExpected.size()))
        || !context->CheckExpectedBytes(GetInventoryRva,GetInventoryExpected.data(),static_cast<std::uint32_t>(GetInventoryExpected.size()))
        || !context->CheckExpectedBytes(GetBeltTypeRva,GetBeltTypeExpected.data(),static_cast<std::uint32_t>(GetBeltTypeExpected.size()))) {
        context->LogError("PotionAutoPickup: D2R 3.2.92777 runtime signature mismatch; hooks refused.");
        return false;
    }
    if(!Settings.enabled) {
        context->LogInfo("PotionAutoPickup 1.1.1 disabled by configuration.");
        return true;
    }
    if(!context->InstallInlineHook(
            GetFreeBeltSlotRva,
            GetFreeBeltSlotExpected.data(),
            static_cast<std::uint32_t>(GetFreeBeltSlotExpected.size()),
            HookGetFreeBeltSlot,
            &OriginalGetFreeBeltSlot)) {
        context->LogError("PotionAutoPickup: free-belt-slot hook installation failed.");
        return false;
    }
    if(!InstallTriggerTablePatches()) {
        context->LogError("PotionAutoPickup: player-action trigger table patch failed.");
        return false;
    }
    if(!context->RegisterConsoleCommand("potion-auto-pickup",Status,"Show PotionAutoPickup native-hook status.")) {
        context->LogError("PotionAutoPickup: console command registration failed."); return false;
    }
    const auto summary=Summary();
    context->LogInfo(summary.c_str());
    return true;
}
D2RL_PLUGIN_EXPORT void D2RLoaderUnloadPlugin() noexcept {
    OriginalTriggers.fill(nullptr);
    OriginalGetFreeBeltSlot=nullptr;
    Context=nullptr;
}
