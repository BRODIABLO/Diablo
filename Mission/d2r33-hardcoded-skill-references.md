# D2R 3.3 Hardcoded Skill References

| ID | Skill | Native reference |
|---:|---|---|
| 0 | Attack | `SKILLS_GetSkillById(skillId=0)` at `0xF9FAC`, `0xFA654`, `0x217570`, `0x2176B0`, `0x21787D`, `0x217AB2`, `0x22F20D`, `0x438DE2`, `0x4AD1E6`, `0x4AD54D`, `0x4AD5FD`, `0x4FDDE6`, `0x4FDFB0`, `0x591FFE`, `0x5CD6D3`, `0x5CED16`, `0x5CF1B5`, `0x5CF274`.<br>`DATATBLS_GetSkillsTxtRecordForContext(skillId=0)` at `0xFF232`.<br>`SKILLS_GetHighestLevelSkillFromUnitAndId(skillId=0)` at `0x1007E8`, `0x1008D6`, `0x102A03`.<br>`SKILLS_SetLeftActiveSkill(skillId=0)` at `0x239C45`, `0x33FF0F`, `0x3403BA`, `0x5CD70D`, `0x5CED50`.<br>`SKILLS_SetRightActiveSkill(skillId=0)` at `0x33E31D`, `0x33FF29`, `0x3403E3`, `0x5CD71D`, `0x5CED60`.<br>`D2GAME_AssignSkill(skillId=0)` at `0x4702C9`, `0x4702E9`, `0x475F74`, `0x475F94`, `0x483FAD`, `0x48406E`, `0x4849DC`, `0x484A9D`.<br>`SKILLS_AssignSkill(skillId=0)` at `0x5CD6FD`, `0x5CED40`, `0x5CF264`. |
| 1 | Kick | `SKILLS_GetHighestLevelSkillFromUnitAndId(skillId=1)` at `0xFA5C9`; `SKILLS_GetSkillById(skillId=1)` at `0x4B14C0`, `0x58E8E8`. |
| 2 | Throw | `D2GAME_AssignSkill(skillId=2)` at `0x474E4E`. |
| 4 | Left Hand Throw | `D2GAME_AssignSkill(skillId=4)` at `0x474F57`. |
| 5 | Left Hand Swing | `SKILLS_GetSkillById(skillId=5)` at `0x217AD5`, `0x4FC1FB`. |
| 6 | Magic Arrow | `SKILLS_GetSkillById(skillId=6)` at `0x2605D1`, `0x27818B`. |
| 27 | Immolation Arrow | Baal AI compares its current skill ID with `27` at `0x5AB4D5` and `0x5AB54C`. |
| 41 | Inferno | Compares the current `skillId` with `41` at `0x5BF31B`, then passes it to `SKILLS_GetSkillById` at `0x5BF32B`. |
| 51 | Fire Wall | Baal AI compares its current skill ID with `51` at `0x5AB4A9` and `0x5AB520`. |
| 56 | Meteor | Baal AI compares its current skill ID with `56` at `0x5AB4A4` and `0x5AB51B`. |
| 59 | Blizzard | Baal AI compares its current skill ID with `59` at `0x5AB49F` and `0x5AB516`. |
| 64 | Frozen Orb | `SKILLS_SetRightActiveSkill(skillId=64)` at `0x959EB`. |
| 90 | IronGolem | `SKILLS_GetHighestLevelSkillFromUnitAndId(skillId=90)` at `0x53329C`. |
| 117 | Holy Shield | `SKILLS_GetSkillById(skillId=117)` at `0x283B44`; `SKILLS_GetHighestLevelSkillFromUnitAndId(skillId=117)` at `0x2C0BCF`, `0x2C0E5C`. |
| 123 | Conviction | `D2GAME_SetSkills(skillId=123)` at `0x495D58`; `D2GAME_AssignSkill(skillId=123)` at `0x495D78`. |
| 124 | Redemption | `SKILLS_ApplyRedemptionEffect(skillId=124)` at `0x4648B7`. |
| 130 | Howl | `AIUTIL_ApplyTerrorCurseState(skillId=130)` at `0x583DCF`. |
| 137 | Taunt | `DATATBLS_GetSkillsTxtRecordForContext(skillId=137)` at `0x57B8AC`. |
| 143 | Leap Attack | `DATATBLS_GetSkillsTxtRecordForContext(skillId=143)` at `0x45E13C`. |
| 158 | SkeletonRaise | `SKILLS_GetSkillById(skillId=158)` at `0x5848FA`. |
| 164 | AndrialSpray | The monster-spawn handler compares its `skillId` with `164` at `0x5A7694`. |
| 167 | Nest | `AITACTICS_UseSequenceSkill(skillId=167)` at `0x5BDD7A`; the monster-spawn handler also compares its `skillId` with `167` at `0x5A75C8`. |
| 184 | MonTeleport | `SKILLS_AssignSkill(skillId=184)` at `0x1DEFFF`; `SKILLS_GetHighestLevelSkillFromUnitAndId(skillId=184)` at `0x1DF00F`, `0x495C97`; `D2GAME_SetSkills(skillId=184)` at `0x495C87`; `AITACTICS_UseSkill(skillId=184)` at `0x4A364D`; the monster-spawn handler also compares its `skillId` with `184` at `0x5A76F2`. |
| 199 | DiabPrison | The monster-spawn handler compares its `skillId` with `199` at `0x5A76B5`. |
| 200 | PoisonBallTrap | `DATATBLS_GetSkillsTxtRecordForContext(skillId=200)` at `0x22219D`, `0x28B9E7`. |
| 203 | DesertTurret | The monster-spawn handler compares its `skillId` with `203` at `0x5A78E8`. |
| 216 | QueenDeath | `SKILLS_GetHighestLevelSkillFromUnitAndId(skillId=216)` at `0x1A525D`. |
| 219 | Scroll of Townportal | `SKILLS_GetSkillById(skillId=219)` at `0x117DA1`, `0x2391BE`. |
| 220 | Book of Townportal | `SKILLS_GetSkillById(skillId=220)` at `0x2391E4`. |
| 268 | Shadow Warrior | `SKILLS_GetHighestLevelSkillFromUnitAndId(skillId=268)` at `0x49FF7C`. |
| 284 | Baal Taunt | `AITACTICS_UseSkill(skillId=284)` at `0x5C6D9B`. |
| 285 | Baal Corpse Explode | `AITACTICS_UseSkill(skillId=285)` at `0x5C71C9`. |
| 286 | Baal Monster Spawn | `SKILLS_AssignSkill(skillId=286)` at `0x5C732C`; `SKILLS_GetHighestLevelSkillFromUnitAndId(skillId=286)` at `0x5C733C`; `AITACTICS_UseSkill(skillId=286)` at `0x5C73FA`. |
| 292 | Teleport 2 | `AITACTICS_UseSkill(skillId=292)` at `0x5CC078`. |
| 293 | Self-resurrect | `AITACTICS_UseSkill(skillId=293)` at `0x4A1B5C`. |
| 294 | Vine Attack | `SKILLS_GetHighestLevelSkillFromUnitAndId(skillId=294)` at `0x2B8A51`, `0x2B8A99`; `SKILLS_AssignSkill(skillId=294)` at `0x2B8A89`. |
| 300 | Impregnate | `AITACTICS_UseSkill(skillId=300)` at `0x5CC9D4`. |
| 302 | MinionSpawner | `AITACTICS_UseSkill(skillId=302)` at `0x5C6AC2`. |
| 315 | Baal Tentacle | `AITACTICS_UseSkill(skillId=315)` at `0x5AAA6F`. |
| 316 | Baal Nova | `SKILLS_AssignSkill(skillId=316)` at `0x5AAA9C`; `AITACTICS_UseSkill(skillId=316)` at `0x5AAAC4`. |
| 317 | Baal Inferno | `AITACTICS_UseSkill(skillId=317)` at `0x5AAB37`. |
| 318 | Baal Cold Missiles | `AITACTICS_UseSkill(skillId=318)` at `0x5AAAF1`. |
| 382 | Bind Demon | `SKILLS_GetHighestLevelSkillFromUnitAndId(skillId=382)` at `0x531814`. |
| 388 | Echoing Strike | The skill-record ID returned by the lookup called at `0x4C28C6` is compared with `388` at `0x4C28FD`. |
| 392 | Mirrored Blades | The code derives ID `392` at `0x4C2904`, then passes it to `SKILLS_GetSkillById` at `0x4C2910`. |
