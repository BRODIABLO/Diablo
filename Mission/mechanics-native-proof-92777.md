# Mechanics 2.0 — preuves natives D2R 3.2.92777

- Lot : `MEC-00/MEC-01`
- Date d'ouverture : 8 août 2026
- Contrats : `Mission/mechanics-contracts.md`
- Build exclusif : `D2R.exe 3.2.92777`
- Verdict courant : **COUTURE AUTORITAIRE COMMUNE NON PROUVÉE**
- Témoins gameplay MEC-01A : **positifs dans leur portée exploratoire**
- Conclusion d'architecture MEC-01A : `LIKELY_FRAGMENTED`
- Mutation gameplay : **aucune**

> **Décision de production séparée — 9 août 2026.** `MeleeSplash.dll` v0.1 est
> désormais le chantier actif, limité à D2R 3.2.92777 offline/local
> single-player et joueur contre monstres. Cette décision ne modifie aucun grade
> du ledger : la couture commune reste `NON PROUVÉE`, l'ancien prototype reste
> non probant et aucun `Pd2CombatCore` n'est sélectionné. La v0.1 peut utiliser
> plusieurs coutures seulement après validation ciblée de leurs signatures,
> octets, ABI et propriété; multijoueur et PvP restent hors portée actuelle.
> Pour cette v0.1, Vincent a retenu un paquet offensif pré-critique partagé et
> des jets Critical/Deadly, CB et OW indépendants par cible. Cette politique de
> production ne change aucun grade de preuve MEC et ne constitue pas le futur
> resolver Critical/Deadly PD2.

## 1. Gate du workbench

Hashes gouvernés attendus :

| Artefact | SHA-256 attendu |
|---|---|
| image canonique | `CC59119DC2A6C7D43D088098FC162EAFA4AE1299B2079126AEF43C1ACA914715` |
| image d'analyse déterministe | `673E8C0B2E89563E75525B24D137098EFD07B2DB4ED42ADEC56AA1ADDF0E63AB` |

Commandes obligatoires :

```powershell
npm.cmd run re:d2r32 -- status
npm.cmd run re:d2r32 -- self-test
```

État initial du 8 août 2026 : `blocked`, car l'environnement Python local de
l'atelier n'était plus disponible. Le gate a ensuite été restauré par le
bootstrap gouverné, uniquement sous `analysis-cache/`.

État courant : **verified**.

```text
canonicalSize=32159440
canonicalSha256=CC59119DC2A6C7D43D088098FC162EAFA4AE1299B2079126AEF43C1ACA914715
canonicalVerified=true
analysisSize=32159440
analysisSha256=673E8C0B2E89563E75525B24D137098EFD07B2DB4ED42ADEC56AA1ADDF0E63AB
analysisVerified=true
indexVerified=true
functions=105850 refs=1057329 strings=59699 patch_sites=57
knowledge=352 json_refs=119
ghidraProjectPresent=true canonicalReady=true
selfTest=PASS
knownXref=0x5817CC->0x46F090
knownPatchSite=0x5817BD
```

L'environnement local utilise Python `3.13.13`, `pefile 2024.8.26` et la
distribution épinglée `capstone 5.0.9`. Le module expose toutefois
`capstone.__version__=5.0.7`; cet écart interne au wheel est consigné, sans
impact observé sur `status` ou `self-test`.

## 2. États de preuve

| État | Signification |
|---|---|
| `hypothesis` | candidat provenant d'une source secondaire ou d'un prototype; non gouverné |
| `partial` | rôle ou adresse prouvé, mais signature, ABI, structure, ownership ou runtime incomplet |
| `confirmed_static` | rôle, bornes, signature, callers/callees et ABI statiquement fermés |
| `confirmed_runtime` | preuve statique plus témoin runtime read-only reproductible |
| `rejected` | attribution contredite; conservée comme preuve négative |
| `unresolved` | aucune identification suffisamment solide |

Une entrée `known-rvas.json` exige au minimum `confirmed_static`. Les surfaces
dont la responsabilité dépend d'un ordre, thread, flag ou lifetime restent
incomplètes jusqu'à `confirmed_runtime`.

## 3. Registre des surfaces exigées

| ID | Surface | Candidat initial | État initial | Gate restant |
|---|---|---:|---|---|
| MEC-HIT-01 | couture serveur hit melee réussi / exécution events | `0x44CE80` | `confirmed_static` comme commit damage partagé, pas comme couture exclusivement melee | filtre `bMissile=0 + SUCCESS`, origine callsite et témoin runtime |
| MEC-DMG-01 | damage record | taille `0x180` | `confirmed_static` pour taille, copie/destruction et champs pertinents | lifetimes/aliasing runtime complets |
| MEC-CRIT-01 | résultat Critical/Deadly | `record+0x04 & 0x2000` | `confirmed_static` comme résultat partagé CS/DS | témoin d'ordre/consommation et impossibilité native de distinguer l'origine |
| MEC-EVT-15 | handler Open Wounds | `0x584170` | `confirmed_static` | ordre et lifecycle complet runtime |
| MEC-EVT-16 | handler Crushing Blow | `0x583150` | `confirmed_static` | vie/quantité et ordre runtime |
| MEC-EVT-20 | handler SkillOnAttackHitKill | `0x583B30` | `confirmed_static` | dispatch indirect exact et consumer runtime |
| MEC-EVT-14 | voisin ItemFreeze | `0x583580` | `confirmed_static` | témoin runtime de distinction |
| MEC-LEECH-01 | calcul life/mana leech | `0x450C90`, champs `+0x120/+0x124` | `confirmed_static` | valeurs pair/impair et crédit runtime |
| MEC-RNG-01 | RNG serveur events/crit | seed `0x34A1E0`, rolls inlinés | `confirmed_static` pour les chemins crit/leech étudiés; aucune primitive roll commune | ordre runtime inter-events |
| MEC-APPLY-01 | calcul total/résistances | `0x44DF10` | `confirmed_static` | ordre runtime et relation interposée avec owners actifs |
| MEC-APPLY-02 | events/HP | `0x44CE80` | `confirmed_static` | distinguer le hit melee autoritaire parmi 36 callers |
| MEC-APPLY-03 | finalisation/réactions | `0x44A9B0` | `confirmed_static` | ordre/mort/packets runtime |
| MEC-APPLY-04 | primitive générique d'application secondaire | chaîne 01→02→03 | primitive sûre `unresolved`; conclusion limitée `LIKELY_FRAGMENTED` | aucune chaîne native prouvée n'exclut events/leech/CB/OW tout en conservant HP/réactions/mort/packets |
| MEC-AREA-01 | énumération d'unités en zone | `0x4398B0` | noyau `confirmed_static`; contrat W07 global `partial` | writer de la room-list, membership adjacente, unicité/dédup et témoin frontière |
| MEC-AREA-02 | targetability/hostilité | `0x48E060` | `confirmed_static` | LOS/distance appartiennent à d'autres surfaces |
| MEC-OW-01 | lifecycle OW statlists | Event15 + helpers natifs | `partial` | ticks, teardown global, disconnect/end-game et stacks runtime |
| MEC-OWNER-01 | conflits PluginPack/FloatingDamage/patches | manifeste 139 sites + 19 patches + addons | `confirmed_static` pour les registres audités | inventaire exact du profil chargé MEC-01 |

Cette table est un backlog de preuve. Elle n'est pas une liste de fonctions
autorisées pour une implantation.

## 4. Candidats issus du prototype quarantiné

Les candidats ci-dessous sont inventoriés pour permettre leur vérification ou
leur rejet indépendant. Leur présence dans le code, la mission historique ou
le worktree de `known-rvas.json` n'est pas une preuve.

| RVA | Nom revendiqué | Responsabilité revendiquée | État MEC courant |
|---:|---|---|---|
| `0x44CE80` | `SUNITDMG_ExecuteEvents` | commit damage/events partagé | `confirmed_static` |
| `0x583580` | `D2GAME_EventFunc14_ItemFreeze` | EventFunc14 | `confirmed_static` |
| `0x583B30` | `D2GAME_EventFunc20_SkillOnAttackHitKill` | EventFunc20 | `confirmed_static` |
| `0x4398B0` | `D2GAME_EnumerateUnitsInArea` | noyau d'énumération spatiale | `confirmed_static`; W07 global reste `partial` |
| `0x48E060` | `SUNIT_CanDamageTarget` | filtre attacker/candidate | `confirmed_static` |
| `0x4242B0` | `D2GAME_ResolveActiveWeaponForSkill` | arme locale de la source | `hypothesis` |
| `0x44C030` | `SUNITDMG_FillDamageValues` | weapon/item/all-components | `confirmed_static` |
| `0x437D00` | `D2GAME_RollElementalSkillDamage` | jet élémentaire skill | `hypothesis` |
| `0x438090` | `D2GAME_RollPhysicalSkillDamage` | jet physique skill | `hypothesis` |
| `0x44D710` | `SUNITDMG_ApplyDamageBonuses` | bonus de dégâts | `confirmed_static` |
| `0x44B930` | `SUNITDMG_ApplyBlockOrDodge` | défense par cible | `hypothesis` |
| `0x5889E0` | `SUNITDMG_ApplyMonsterCritical` | critical monstre | `confirmed_static` |
| `0x44DF10` | `SUNITDMG_CalculateTotalDamage` | résistance/absorb/total | `confirmed_static` |
| `0x44A9B0` | `SUNITDMG_FinalizeDamage` | réaction/mort/packets | `confirmed_static` |
| `0x4494B0` | `D2Damage_CopyConstructor` | copie profonde record | `confirmed_static` |
| `0x4496E0` | `D2Damage_Destructor` | destruction record | `confirmed_static` |

Neuf autres appels directs du prototype restent sans entrée gouvernée et
doivent être identifiés avant que son graphe puisse seulement servir de
comparaison : `0x34B720`, `0x3400A0`, `0x3B5160`, `0x385BE0`, `0x2F5940`,
`0x2F56A0`, `0x2F56E0`, `0x3385B0`, `0x338160`.

## 5. Preuves déjà gouvernées pertinentes

### MEC-RES-01 — resolver résistances et absorb

| Champ | Valeur |
|---|---|
| RVA | `0x4523E0` |
| Nom gouverné | `SUNITDMG_ApplyResistancesAndAbsorb` |
| État | `confirmed_static` préexistant; responsabilité à relier au pipeline MEC |
| Owners voisins | caps `0x4524D6/0x4524DE` par `plugin-items` |
| Limite | ne prouve ni hit melee, ni events, ni leech, ni primitive HP |

### MEC-OBS-01 — FloatingDamage

| Champ | Valeur |
|---|---|
| RVA | `0x427150` |
| Owner | `FloatingDamage` |
| Rôle connu | observation client post-résolution par composante |
| État | owner confirmé; aucune propriété de calcul ou d'application |
| Limite | trop tardif pour fermer seul origine CS/DS, CB/OW, leech ou RNG |

### MEC-STATLIST-BASE — helpers génériques

| RVA | Nom gouverné | Limite MEC |
|---:|---|---|
| `0x2F81A0` | `STATLIST_MergeStatLists` | helper générique, pas un consumer OW |
| `0x2F8120` | `STATLIST_GetOwner` | lecture d'owner, pas une preuve de création |
| `0x2F8290` | `STATLIST_ExpireUnitStatlist` | expiration générique, pas le lifecycle OW complet |

### MEC-DMG-01 — record `D2Damage`

Statut : `confirmed_static` pour la taille, la copie/destruction et les champs
consommés par le sous-graphe prouvé; `confirmed_runtime` reste requis pour les
lifetimes, aliases et dégâts imbriqués.

| Offset | Largeur / sens 92777 confirmé |
|---:|---|
| `+0x00` | hit flags `uint32` |
| `+0x04` | result flags `uint16`; `SUCCESS=0x0001`, résultat Critical/Deadly partagé `0x2000` |
| `+0x18` | physical |
| `+0x1C` | enhanced-damage percent |
| `+0x20/+0x24/+0x28` | fire / burn / burn length |
| `+0x2C/+0x30/+0x34` | lightning / magic / cold |
| `+0x38/+0x3C` | poison / poison length |
| `+0x40/+0x48/+0x50` | conteneur pointer / count / capacity; SBO `+0x58..+0x117`, 16 éléments de 12 octets |
| `+0x118/+0x11C` | cold length / freeze length |
| `+0x120/+0x124/+0x128` | life / mana / stamina leech |
| `+0x12C/+0x130/+0x134` | stun length / absorbed life / total damage |
| `+0x13C/+0x140` | pierce percent / damage rate |
| `+0x144/+0x148` | hit class / active-hit-class byte |
| `+0x149/+0x14C/+0x150` | conversion type / conversion percent / overlay |
| `+0x158` | sous-objet complexe possédé |

#### Copie profonde `0x4494B0`

- Statut : `confirmed_static`; bounds logiques `0x4494B0..0x4496DF`, `ret`
  `0x4496DE`.
- ABI : `D2Damage* (dst: RCX, const src: RDX)`, retour `dst` dans `RAX`.
- Signature unique, 32 octets :
  `48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 48 89 7C 24 20 41 56 48 83 EC 20 8B 02 4C 8D 41 58`.
- Callers : `0x44A2C8`, `0x44B37C`, `0x4508FC`, `0x4520C3`, `0x46463F`,
  `0x51E6C3`, `0x51E94D`, `0x51ED3A`, `0x52118E`, `0x523C89`, `0x55A3A8`,
  `0x56137B`, `0x5615D1`, `0x56E394`, `0x56E4EE`, `0x56E5E0`, `0x57008C`.
- Callees directs : `0x448FC0`, `0x12063C0` et un appel virtuel
  `[vtable+0x08]`.
- Taille `0x180` : les callsites `0x464610` et `0x56E360` placent le cookie
  exactement `0x180` octets après l'objet; le CFG copie les scalaires, le
  conteneur SBO puis construit le sous-objet `+0x158`.

#### Destructeur `0x4496E0`

- Statut : `confirmed_static`; bounds `0x4496E0..0x449752`, `ret 0x449751`.
- ABI : `void (D2Damage*: RCX)`.
- Signature unique, 32 octets :
  `48 89 5C 24 08 57 48 83 EC 20 F6 81 58 01 00 00 01 48 8B F9 74 0D 48 8B 89 58 01 00 00 48 83 E1 FE`.
- Callers indexés : `0x44BF0E`, `0x44BFEF`, `0x51E9CD`.
- Callees : `0x07F5F0`, `0x12063C0` et deux appels virtuels; destruction du
  sous-objet `+0x158`, puis libération du conteneur `+0x40` hors SBO.

### MEC-DMG-BUILD / MEC-CRIT-01 — construction, Critical et Deadly

`0x44C030` est `confirmed_static`, bounds `0x44C030..0x44CDC8`, `ret 0x44CDC7`.

- ABI : `void (game: RCX, attacker: RDX, defender: R8, damage: R9,
  mode: [rsp+0x20], SrcDam:uint8 [rsp+0x28])`; `0x44B69B` passe `mode=0`.
- Signature unique, 30 octets :
  `40 56 57 41 54 48 81 EC 10 05 00 00 48 8B 05 85 F2 57 02 48 33 C4 48 89 84 24 D0 04 00 00`.
- Callers : `0x42F067`, `0x4300B6`, `0x44B69B`, `0x56E408`, `0x572DCE`,
  `0x574029`, `0x5767AA`, `0x577E10`.
- Callees uniques : `0x0976E0`, `0x2F4190`, `0x2F48C0`, `0x2F5020`,
  `0x2F5C60`, `0x2F61F0`, `0x2F7D10`, `0x2F84B0`, `0x33D4F0`, `0x349860`,
  `0x34A0E0`, `0x34A1E0`, `0x34B9D0`, `0x373890`, `0x3AF190`, `0x3AF240`,
  `0x3AF2C0`, `0x4242B0`, `0x4242C0`, `0x424490`, `0x4491A0`, `0x449D40`,
  `0x449FB0`, `0x44D570`, `0x44D710`, `0x4501E0`, `0x5889E0`, `0x12D1150`.

Le bloc `0x44C2B4..0x44C3D9` ferme l'ordre Critical/Deadly natif : pour
`mode=0` et une arme active, chance de mastery calculée par `0x33D4F0`, puis
roll; à défaut, stat `337 passive_critical_strike`, nouveau roll; à défaut,
stat `141 item_deadlystrike`, nouveau roll. Le premier succès double seulement
le physique `+0x18`, pose `resultFlags|=0x2000` et court-circuite les jets
suivants. Le bit ne permet donc jamais de distinguer Critical de Deadly après
résolution. Les IDs 337 et 141 sont recoupés dans l'`ItemStatCost.txt` vanilla
3.2; aucune formule PD2 n'est transposée.

`0x33D4F0` reste `partial` comme identité globale à cause de son jump table,
mais son rôle local « chance critical de weapon mastery » est fermé par le
callsite `(attacker, activeWeapon, 0, 2)` et son consumer immédiat.

#### Bonus physiques `0x44D710`

- Statut : `confirmed_static`; bounds `0x44D710..0x44D9E5`, `ret 0x44D9E4`.
- ABI : `int32(attacker:RCX, getStats:EDX, item:R8, minDamage:R9D,
  maxDamage:[rsp+0x20], enhancedPct:[rsp+0x28], flatDamage:[rsp+0x30],
  SrcDam:uint8 [rsp+0x38])`.
- Signature unique, 31 octets :
  `48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 41 54 41 55 41 56 41 57 48 83 EC 30 BB 01 00`.
- Callers : `0x44C2AC`, `0x44DA86`, `0x55874E`, `0x564B86`, `0x5653EB`,
  `0x5656BF`.
- Callees : `0x2F5020`, `0x2F5C60`, `0x33D4F0`, `0x34A360`, `0x373490`,
  `0x3736E0`, `0x388910`, `0x4242B0`, `0x4501E0`.

Au call principal `0x44C2AC`, le moteur fournit attacker, `getStats=1`, item
null, min/max nuls, `damage+0x1C`, `damage+0x18` et SrcDam; `EAX` remplace
ensuite `damage+0x18`. Cette preuve ferme le rôle générique, pas la composition
Smite revendiquée historiquement par le prototype.

### MEC-MONCRIT — critical des monstres

`0x5889E0` est `confirmed_static`, bounds `0x5889E0..0x588AF4`, ABI
`void(attacker:RCX, defender:RDX, damage:R8)`.

- Signature unique, 32 octets :
  `48 89 6C 24 10 48 89 74 24 18 57 48 83 EC 20 49 8B F8 48 8B EA 48 8B F1 E8 D3 2F DC FF 83 F8 01`.
- Callers : `0x436F76`, `0x44CD88`, `0x46303A`.
- Callees : `0x0976E0`, `0x349020`, `0x349860`, `0x34A0E0`, `0x34A1E0`,
  `0x34B9D0`, `0x4916D0`.

Pour un attacker monstre, la chance vient de `MonStats+0xF8`; le succès double
physical, fire, lightning, magic, cold et poison. Il ne pose pas `0x2000` :
il tente un hit-class `0x10`, puis un overlay si nécessaire. Il ne doit donc
pas être confondu avec le résultat CS/DS joueur.

### MEC-PIPELINE-01 — sous-graphe autoritaire damage

#### Calcul total `0x44DF10`

- Statut : `confirmed_static`; bounds `0x44DF10..0x44ECE0`, `ret 0x44ECDF`.
- ABI : `void(game:RCX, attacker:RDX, defender:R8, damage:R9)`.
- Signature unique, 49 octets :
  `40 55 53 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 68 FD FF FF 48 81 EC 98 03 00 00 48 8B 05 95 D3 57 02 48 33 C4 48 89 85 80 02 00 00 4C 8B EA`.
- Callers : `0x447FC4`, `0x44B6AC`, `0x44CF93`, `0x5F2606`.
- Callees : `0x2F5020`, `0x2F5C60`, `0x300830`, `0x3351B0`, `0x34B9D0`,
  `0x34FB40`, `0x3AEFF0`, `0x3AF190`, `0x3AF240`, `0x3AF2C0`, `0x3B0250`,
  `0x4040B0`, `0x44D570`, `0x4523E0`, `0x12D1150`.
- Rôle : résistances, réductions, absorb et total final; il délègue au resolver
  gouverné `0x4523E0`.

#### Exécution events / commit damage `0x44CE80`

- Statut : `confirmed_static` comme couture de commit partagée; bounds logiques
  CFG `0x44CE80..0x44D569`, `ret 0x44D568`.
- ABI : `void(game:RCX, attacker:RDX, defender:R8, bMissile:R9D,
  damage:[caller rsp+0x20])`; le callee charge le cinquième argument dans `RDI`.
- Signature unique, 31 octets :
  `40 55 53 56 57 41 56 41 57 48 8D AC 24 E8 FE FF FF 48 81 EC 18 02 00 00 48 8B 05 29 E4 57 02`.
- Callers : `0x418885`, `0x436F95`, `0x4397A4`, `0x447FDF`, `0x44B3FA`,
  `0x4630AC`, `0x46465E`, `0x51E6E2`, `0x51E9B0`, `0x51EC38`, `0x51F9CF`,
  `0x5201DE`, `0x52081F`, `0x5211AE`, `0x523CA8`, `0x55294B`, `0x552BED`,
  `0x554B62`, `0x555E29`, `0x55A473`, `0x55BBEB`, `0x55EEBD`, `0x5613BC`,
  `0x561612`, `0x564839`, `0x56E5FF`, `0x570ED1`, `0x579C13`, `0x57A200`,
  `0x57C283`, `0x582A48`, `0x582C59`, `0x582E69`, `0x583079`, `0x583736`,
  `0x5F2621`.
- Callees uniques : `0x0976E0`, `0x0D1E20`, `0x213900`, `0x213A20`,
  `0x2F5020`, `0x2F7D10`, `0x300A90`, `0x349860`, `0x34B9D0`, `0x34C2C0`,
  `0x34DB60`, `0x4283F0`, `0x449760`, `0x44D570`, `0x44DF10`, `0x4509B0`,
  `0x450C90`, `0x451380`, `0x451570`, `0x451820`, `0x4518C0`, `0x451C30`,
  `0x48B720`, `0x48B890`, `0x48E060`, `0x4990C0`, `0x56C9E0`, `0x12063C0`,
  `0x12D1150`.

Le chemin générique `0x44B600` appelle FillDamageValues à `0x44B69B`, puis
CalculateTotalDamage à `0x44B6AC`. Le chemin melee appelle ExecuteEvents avec
`bMissile=0` à `0x44B3FA`, puis Finalize à `0x44B4F4`. Le chemin missile appelle
MonsterCritical à `0x436F76`, ExecuteEvents avec `bMissile=1` à `0x436F95`,
puis Finalize à `0x436FA6`; ExecuteEvents ne rappelle CalculateTotal que pour
`bMissile!=0`.

Conclusion statique : `0x44CE80` est une couture autoritaire commune de commit
post-hit, mais pas une couture exclusivement melee. Un observateur melee doit
au minimum prouver `bMissile=0`, `SUCCESS` et l'origine du callsite; la seule
adresse ne suffit pas à déclarer « hit melee réussi ».

#### Finalisation `0x44A9B0`

- Statut : `confirmed_static`; bounds `0x44A9B0..0x44B2A2`, `ret 0x44B2A1`.
- ABI : `void(game:RCX, attacker:RDX, defender:R8, damage:R9)`.
- Signature unique, 30 octets :
  `40 53 55 57 41 56 48 81 EC 88 00 00 00 49 8B D9 49 8B F8 4C 8B F2 48 8B E9 E8 92 36 04 00`.
- Callers : `0x418898`, `0x436FA6`, `0x447FF2`, `0x44B4F4`, `0x4630CE`,
  `0x464672`, `0x48E9D0`, `0x51E6F6`, `0x51E9C3`, `0x51EC4B`, `0x51F9E2`,
  `0x5201F1`, `0x52087B`, `0x5211C3`, `0x523CBC`, `0x552C00`, `0x554B75`,
  `0x555E3C`, `0x55A487`, `0x55BBFE`, `0x55EED0`, `0x5613CF`, `0x561625`,
  `0x56484C`, `0x56E613`, `0x570FFE`, `0x579C3F`, `0x582A5B`, `0x582C6C`,
  `0x582E7C`, `0x58308C`, `0x5F2634`.
- Callees uniques : `0x097790`, `0x1A2D30`, `0x2F5020`, `0x2F5940`,
  `0x2F5DF0`, `0x2F7D10`, `0x300830`, `0x3351B0`, `0x33CC00`, `0x33DD40`,
  `0x33EC20`, `0x341A20`, `0x341A30`, `0x349860`, `0x34A330`, `0x34AB60`,
  `0x34B9D0`, `0x34C2C0`, `0x34CE40`, `0x34E340`, `0x34EEE0`, `0x42D020`,
  `0x42D2C0`, `0x4471E0`, `0x4475C0`, `0x449C40`, `0x44DB20`, `0x44FBA0`,
  `0x451E50`, `0x451FB0`, `0x48B230`, `0x48E060`, `0x491960`, `0x498EE0`,
  `0x543900`, `0x588B50`, `0x588CE0`.

#### Préparation et queue du combat `0x44B600` / `0x4507B0`

`0x44B600`, nom conservateur `SUNITDMG_PrepareAndQueueCombatRecord`, est
`confirmed_static` sur le CFG logique `0x44B600..0x44B71B` :

- ABI `void(game:RCX, attacker:RDX, defender:R8, damage:R9, SrcDam:uint8
  [caller rsp+0x20])`;
- signature unique, 33 octets :
  `48 89 5C 24 20 55 56 41 56 48 83 EC 30 49 8B D9 49 8B F0 48 8B EA 4C 8B F1 48 85 D2 74 05 4D 85 C0`;
- 31 callers directs;
- Fill à `0x44B69B`, CalculateTotal à `0x44B6AC`, calcul WILLDIE, puis
  caller unique du linker `0x4507B0` à `0x44B703`.

`0x4507B0`, `SUNITDMG_AllocateAndLinkCombatRecord`, est `confirmed_static` sur
`0x4507B0..0x4509A3` :

- ABI `void(game:RCX, attacker:RDX, defender:R8, const damage:R9)`;
- signature unique, 32 octets :
  `40 55 57 41 56 41 57 48 81 EC C8 01 00 00 48 8B 05 03 AB 57 02 48 33 C4 48 89 84 24 B0 01 00 00`;
- caller direct unique `0x44B703`;
- allocation d'un node `0x1A0`, stockage type/GUID source et cible, copie profonde
  du record à `node+0x18`, puis lien dans `attacker+0x108` via `node+0x198`.

Cette paire prépare un record **propre à la cible** et le met en attente. Elle
n'applique aucun HP et peut rejouer Fill/CS/DS selon ses flags; elle n'est donc
pas une primitive secondaire.

#### Consommation melee `0x44B2B0`

`SUNITDMG_ConsumeMeleeCombatRecord` est `confirmed_static` sur le CFG logique
`0x44B2B0..0x44B5F5` :

- ABI `bool/uint8(game:RCX, attacker:RDX, defender:R8, rangeBonus:R9D)`;
- signature unique, 34 octets :
  `40 53 55 56 57 41 54 41 56 48 81 EC C8 01 00 00 48 8B 05 01 00 58 02 48 33 C4 48 89 84 24 B0 01 00 00`;
- 36 callers directs;
- recherche du node par type/GUID, copie profonde à `0x44B37C`, validation de
  portée, ExecuteEvents melee à `0x44B3FA`, effets melee supplémentaires,
  events 7/3, chemin thorns, Finalize à `0x44B4F4`, puis unlink/free du node.

La paire `0x44B600 → node → 0x44B2B0` est donc la chaîne melee canonique
complète et sa largeur est positivement prouvée. Dans ExecuteEvents, le leech est
appelé à `0x44D038`, la mutation HP est inline à `0x44D06C..0x44D093`, tandis
que Finalize porte séparément réactions, mort et packets. Aucun callee prouvé ne
fournit « HP + statuts + kill sans events/leech ».

Le pattern AoE natif le plus proche, callback Mind Blast
`0x56E590..0x56E695`, copie un record, appelle ExecuteEvents avec `bMissile=1`
et Finalize, puis le détruit. Il conserve events/leech et les canaux missile; il
ne constitue donc pas une primitive melee-splash sûre.

Verdict MEC-APPLY-04 : la primitive sûre reste `unresolved`, mais les preuves
positives suffisent à la conclusion limitée `LIKELY_FRAGMENTED`. Une composition
Calculate → Execute(0) → Finalize serait une architecture custom à filtrer et
qualifier, pas un contrat natif. La couture commune autorisant `Pd2CombatCore`
reste `NON PROUVÉE`.

### MEC-LEECH-01 — point unique life/mana leech

`0x450C90` est `confirmed_static`, bounds `0x450C90..0x451380`, ABI
`void(game:RCX, attacker:RDX, defender:R8, damage:R9)`.

- Signature unique, 32 octets :
  `40 55 56 57 41 56 48 83 EC 58 41 83 B9 20 01 00 00 00 49 8B F1 49 8B F8 4C 8B F2 48 8B E9 75 0E`.
- Caller unique : `0x44D038`, dans ExecuteEvents.
- Callees uniques : `0x2F34F0`, `0x2F4D20`, `0x2F4E20`, `0x2F5020`,
  `0x2F7D10`, `0x300830`, `0x33A1F0`, `0x349020`, `0x349860`, `0x34A1E0`,
  `0x34AB60`, `0x34B9D0`, `0x3AF240`, `0x4468D0`, `0x446DB0`, `0x451820`,
  `0x48FF00`, `0x48FF40`.

FillDamageValues place les pourcentages bruts : stat 60 life à `+0x120` si le
hit flag `0x4` est absent, stat 62 mana à `+0x124` si `0x8` est absent. Le
consumer prend le Drain de la cible, les diviseurs de difficulté joueur, la
base physique et, sous hit flag `0x40`, les composantes élémentaires. Il
applique deux troncatures en pourcentage puis une division signée après le
shift interne. Il réécrit `+0x120/+0x124` avec les pourcentages ajustés natifs,
puis calcule séparément les montants effectifs, les cappe au manque, crédite les
stats natives et soustrait ensuite le mana effectif à la cible. Les champs du
record ne sont donc pas un témoin direct des montants crédités. Le roll final
choisit seulement l'overlay visuel et ne décide pas la valeur de leech.

### MEC-RNG-01 — seed et rolls inlinés

`0x34A1E0` est `confirmed_static` comme accesseur du seed d'unité et
explicitement `rejected` comme prétendue primitive callable de roll.

- Bounds `0x34A1E0..0x34A215`; ABI `seed_pair* (unit:RCX) -> RAX`, retour de
  `unit+0x28`.
- Signature unique, 32 octets :
  `40 53 48 83 EC 20 48 8B D9 48 85 C9 75 1D 88 4C 24 30 48 8D 4C 24 30 E8 94 BB FF FF 84 C0 74 01`.
- Plus de 100 xrefs globaux; callsites MEC : `0x44C2E1`, `0x44C339`,
  `0x44C38D`, `0x588A50`, `0x451319`.

Sur ces chemins, le roll est inliné :

```text
new64 = uint32(low) * 0x6AC690C5 + uint32(high)
low   = uint32(new64)
high  = uint32(new64 >> 32)
roll  = low % 100          # ou low & 1 pour l'overlay leech
```

Il n'existe donc aucune primitive RNG commune démontrée à appeler ou hooker.
La propriété RNG appartient au seed de l'attacker et à chaque séquence inline;
l'ordre inter-events exige encore MEC-01.

### MEC-EVENT — dispatcher natif et ABI réelle

La référence D2MOO est épinglée au commit
`19019806df7f3e877fa105b05395d1e3597e2316` et n'est utilisée que comme guide
sémantique. Aucun RVA, layout ou ABI legacy n'est transposé.

`0x44CE80` appelle le wrapper event `0x44D570` pour les événements 6, 5, puis
2/1 sur les branches qualifiées. Le wrapper copie le sous-objet
`D2Damage+0x158` et appelle le dispatcher `0x5881E0`.

#### Dispatcher `0x5881E0`

- Statut : `confirmed_static`; bounds `0x5881E0..0x58844B`, `ret 0x58844A`.
- Signature unique, 16 octets :
  `4C 89 4C 24 20 4C 89 44 24 18 89 54 24 10 48 89`.
- Callers : `0x44583E`, `0x448EA4`, `0x448F36`, `0x44D5F2`, `0x44F411`,
  `0x44F640`, `0x463CA0`, `0x463DC3`.
- Node : event byte `+0x00`, flags busy/deferred-delete `+0x02`, payloads
  `uint32 +0x0C/+0x10/+0x14`, callable `+0x18`, callback `+0x20`, liens
  `prev/next +0x28/+0x30`; liste depuis `attacker+0xE0`.
- Le callback direct est invoqué à `0x58839D`, puis la suppression différée
  est traitée sans invalider le parcours.

ABI callback D2R 92777, retour `int32 EAX` :

| Position | Transport |
|---:|---|
| 1 | `RCX=game` |
| 2 | `EDX=event` |
| 3 | `R8=attacker` |
| 4 | `R9=target` |
| 5 | stack : `D2Damage*` |
| 6/7/8 | stack : node payloads `+0x0C/+0x10/+0x14` |
| 9 | stack : pointeur auxiliaire vers la copie de node `+0x18` |

Le préfixe legacy à sept arguments est compatible sémantiquement mais n'est
pas l'ABI exacte D2R. Aucun pointeur/table statique vers les quatre handlers
ci-dessous n'a été retrouvé et leurs xrefs directs sont nuls, comme attendu
pour un dispatch indirect. L'association des numéros 14/15/16/20 repose sur
l'isomorphisme complet de leur comportement avec la référence épinglée; cette
limite devra être corrélée runtime et ne doit jamais être reformulée comme une
table native retrouvée.

#### EventFunc14 ItemFreeze `0x583580`

- Bounds `0x583580..0x5837EC`; padding à partir de `0x5837EC`.
- Signature unique, 32 octets :
  `40 55 53 56 57 41 54 41 56 41 57 48 8D AC 24 40 FF FF FF 48 81 EC C0 01 00 00 48 8B 05 27 7D 44`.
- ABI neuf slots; consomme game/event/attacker/target et arg6 packed-stat.
- Callees probants : `0x2F5C60` layered stat chance, `0x2F5020` niveaux,
  `0x34A1E0` attacker seed, `0x44CE80` damage/events.
- Statut : rôle/bounds/ABI `confirmed_static`; association numérique à
  corréler par MEC-01.

#### EventFunc15 Open Wounds `0x584170`

- Bounds `0x584170..0x584316`; padding `0x584316..0x58431F`.
- Signature unique, 24 octets :
  `48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 48 83 EC 50 49 8B F9 49`.
- ABI neuf slots; consomme game/event/attacker/target et arg6 packed-stat.
- Callees matériels : layered stat `0x2F5C60`, seed `0x34A1E0`, stat level
  `0x2F5020`, unit type `0x34B9D0`, monster flag `0x38E870`, helper
  curse/statlist `0x433D20`.
- RNG : LCG attacker seed gouverné, modulo 100, succès si roll inférieur à la
  chance layered.
- Formule native : `40` au niveau 1, puis `9L+31` jusqu'à 15, `18L-104`
  jusqu'à 30, `27L-374` jusqu'à 45, `36L-779` jusqu'à 60, puis `45L-1319`;
  cible player `/4`, encore `/2` pour event 6, monstre flag 12 `/2`. La valeur
  négative est fournie au helper avec state `62`.

Le tuple `{skillId=0, skillLevel=1, duration=200, statId=7}` provient d'une
constante runtime à `0x1D3EBD0`, dans la BSS nulle de l'image canonique. D2MOO
et tout le CFG D2R concordent, mais ces quatre valeurs restent `pending` jusqu'à
lecture mémoire MEC-01; elles ne sont pas présentées comme octets image.

#### EventFunc16 Crushing Blow `0x583150`

- Bounds `0x583150..0x583276`; padding `0x583276..0x58327F`.
- Signature unique, 32 octets :
  `48 89 6C 24 10 48 89 74 24 18 57 41 56 41 57 48 83 EC 60 49 8B F1 49 8B F8 44 8B FA 4C 8B F1 4D`.
- ABI neuf slots; consomme game/event/attacker/target, arg5 damage et arg6
  packed-stat.
- Callees : chance `0x2F5C60`, seed `0x34A1E0`, core CB `0x581BD0`.

Le handler construit `{damage, divisorDefault=4, divisorPlayer=10,
divisorHireling=10, event==6}`. Le core `0x581BD0..0x581EC7` possède une
signature unique de 24 octets
`48 89 54 24 10 48 89 4C 24 08 53 55 56 57 41 56 41 57 48 83 EC 48 41 8B`
et deux callers (`0x51D950`, `0x583257`). Il lit HP stat 6, physical resist
stat 36 capé à 100 et monster-player-count stat 100, écrit HP par `0x2F7D10`,
pose `resultFlags|=2` si lethal et choisit l'overlay 147 si la réduction est
positive.

#### EventFunc20 SkillOnAttackHitKill `0x583B30`

- Bounds `0x583B30..0x583D1E`; padding `0x583D1E..0x583D1F`.
- Signature unique, 33 octets :
  `48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 48 89 7C 24 20 41 54 41 56 41 57 48 83 EC 40 49 8B F1`.
- ABI neuf slots; consomme game/event/attacker/target, arg5 damage et arg6
  packed-stat.
- Callees/consumers : chance `0x2F5C60`, seed `0x34A1E0`, data context,
  skill lookup `0x097790`, cast target `0x5896E0` ou cast position `0x589820`.
- Si damage existe, le handler exige `[damage+0] & 0x20`, décode chance,
  skill id et level, puis consomme le même LCG/modulo 100.

`0x5837F0` est EventFunc21 SkillOnGetHit; `0x5839A0/0x583A70` traitent
mana/heal-after-kill. Ces voisins empêchent de réintroduire l'ancien découpage
fondé sur les seules partitions PData.

### MEC-OW-01 — lifecycle curse/statlist observé

Le helper générique `0x433D20..0x4341DC` est `confirmed_static` avec 32 callers
et signature unique de 24 octets :
`48 89 5C 24 20 55 56 57 41 54 41 55 41 56 41 57 48 83 EC 50 48 8B 69 08`.

Layout args : `source +0`, `target +8`, `skillId +0x10`, `skillLevel +0x14`,
`duration +0x18`, `statId +0x1C`, `statValue +0x20`, `state +0x24`, callback
optionnel `+0x28`.

Flux prouvé : lookup curse par `0x2F5810` ou state par `0x2F5940`; refresh
expiry `0x2F7AB0` et event type 12 `0x48B720` si même state/skill/level; refus
d'un niveau inférieur; unlink/free `0x2F7920/0x2F4180` en cas de remplacement;
activation state `0x3354C0`; allocation `0x2F7300`; setters state/skill/level/
stat `0x2F72F0/0x2F79B0/0x2F7A00/0x2F7C00`; post list `0x2F2CB0`; callback
`0x2F7A50`.

Le callback par défaut `0x436240..0x4362DC` a la signature unique
`48 89 5C 24 08 57 48 83 EC 20 48 8B CA 41 8B F8 48 8B DA E8 A8 9C 05 00`.
Il désenregistre l'événement par `0x588670`, respecte dead/stay-death, désactive
l'état et rafraîchit animation/passifs/player.

Cette chaîne prouve création, refresh, remplacement, expiration et callback
génériques. Elle ne ferme pas encore les stacks OW à 1/2/3/4 applications, deux
attaquants, disconnect, fin de game, GUID reuse ni la constante BSS; le verdict
lifecycle OW reste donc `partial` jusqu'à MEC-01.

### MEC-AREA-02 — prédicat natif de permission de dégâts

| Champ | Preuve 92777 |
|---|---|
| Statut | `confirmed_static` |
| RVA / bounds | entrée `0x48E060`; corps délimité manuellement jusqu'au `ret` à `0x48E247`, prochain prologue à `0x48E250` |
| Rôle | filtre serveur entre attacker et candidate : owners/minions, town, rooms et relation hostile |
| Signature | `40 53 56 57 48 83 EC 20 4C 89 74 24 50 49 8B F8 4C 8B F1`, unique dans `.text` |
| Callers | `0x44CEC1` dans l'exécuteur damage/events; `0x44A9C9` dans la finalisation damage |
| ABI | `int32 (game: RCX, attacker: RDX, candidate: R8)`; retour booléen dans `EAX` |
| Callees matériels | `UNITS_GetUnitType`, résolution owner/minion, `UNITS_GetRoom`, `DUNGEON_IsRoomInTown`, puis prédicat de relation native |
| Limites | ne prouve ni distance, ni masque d'aura, ni LOS; ce filtre doit être appelé avec l'attacker, pas avec l'origine spatiale si celle-ci est la primary |
| Action RVA | admissible après retrait de la provenance prototype; source remplacée par ce ledger |

Les deux callers préservent directement leurs trois premiers arguments avant
l'appel : `0x44CE80` reçoit `game/attacker/defender/...`, et `0x44A9B0` reçoit
`game/attacker/defender/damage`. Le test du retour précède leurs autres étapes
de damage, ce qui ferme le sens du booléen sans utiliser le prototype.

### MEC-AREA-01 — énumérateur spatial

| Champ | Preuve 92777 |
|---|---|
| Statut | noyau `confirmed_static`; MEC-W07 global `partial` |
| RVA / bounds | `0x4398B0..0x439C04` end-exclusive; les entrées `.pdata` fragmentent artificiellement ce CFG logique |
| Signature | `44 89 4C 24 20 55 57 41 56 41 57 48 8B EC 48 83 EC 78`, unique dans `.text` |
| Callers directs | `0x42F23B`, `0x43287D`, `0x4329BC`, `0x43329A`, `0x4333AB`, `0x435733`, `0x43585F`, `0x43792E` |
| ABI | `void(game RCX, origin RDX, descriptor R8, radius R9D, mask arg5, callback arg6, userCtx arg7, specialMonsterFilter arg8, sourceFile arg9, sourceLine arg10)`; le corps consomme les huit premiers arguments |
| Descriptor | `{room* +0, x:int32 +8, y:int32 +0x0C}`; game/origin/callback non nuls, rayon strictement positif; masque nul remplacé par `0x583` |
| Callback | `int32 callback(localCtx*:RCX, candidate*:RDX)` à `0x439B94`; `localCtx={game +0, origin +8, countNonZero +0x10, userCtx +0x18}`; un retour non nul incrémente le compteur sans interrompre le parcours |
| Ordre | tableau de rooms dans son ordre brut, puis chaîne d'unités de chaque room; le next est lu avant filtres/callback; aucun tri ni set `seen` local |
| Aire | broad phase carré/rectangle de room, puis `distance² <= radius²`; l'origine seule est explicitement exclue |
| Collision | gate ray/collision optionnel si `mask & 0x200`; le masque par défaut `0x583` n'active pas ce gate; il n'existe donc pas de LOS implicite par défaut |
| Limites | le writer de la room-list, sa membership adjacente exacte, l'unicité des rooms/unités, la dédup et le comportement runtime de frontière restent ouverts |
| Action RVA | promotion du noyau seulement; ne pas reformuler cette promotion comme fermeture de MEC-W07 |

Helpers natifs fermés :

- `0x2EFDE0..0x2EFDED`, `DUNGEON_GetRoomListAndCount` : écrit le pointeur
  stocké à `room+0` et le count `room+0x40`; signature unique
  `8B 41 40 41 89 00 48 8B 01 48 89 02 C3`;
- `0x2EFD90..0x2EFD98` : retourne le premier unit à `room+0xA8`;
- `0x34B4A0` : retourne le next à `unit+0x160`;
- `0x3256E0..0x325760` : broad phase inclusive entre le carré de recherche et
  le rectangle de room;
- `0x3412C0..0x3412D3` : calcule la distance euclidienne au carré.

Le corps et l'ABI de `0x2EFDE0` sont isomorphes à l'accesseur legacy de rooms
adjacentes, mais l'image 92777 ne livre ici que l'accesseur d'une liste déjà
construite. Sans son writer/builder natif, « rooms adjacentes » demeure une
identification sémantique haute confiance et non un invariant 92777 prouvé.
De même, `0x4398B0` ne déduplique ni room ni GUID : le modèle normal d'une unité
dans une seule room active reste une hypothèse tant que les structures amont ne
sont pas fermées.

## 6. Ledger détaillé à remplir

Chaque entrée confirmée doit utiliser ce format; les sorties brutes restent
sous `analysis-cache/mechanics-92777/`.

```text
Proof ID:
Status:
RVA / bounds:
Role:
Expected bytes / mask:
Signature uniqueness:
Callers:
Callees:
ABI:
Structure fields:
Ownership / overlaps:
Static commands:
Runtime witness:
Negative evidence / limits:
known-rvas action:
```

## 7. Ordre de recherche MEC-00

1. `known` sur damage, event, statlist, RNG, leech, critical et les RVA
   candidates;
2. constructeurs/destructeur du damage record et xrefs de ses champs;
3. callers de `0x44CE80`, puis remontée jusqu'au résultat de hit;
4. table/dispatcher et identités EventFunc 14, 15, 16 et 20;
5. producteurs/consommateurs du flag Critical/Deadly;
6. producteurs et crédit effectif life/mana leech;
7. primitives RNG appelées sur ces chemins;
8. séparation calculate/resist/HP/finalize/death/packet;
9. énumération de zone, callback, rooms adjacentes et ordre;
10. xrefs de l'état OW et suivi complet des statlists;
11. audit de propriété sur entrées, callsites et plages attendues;
12. promotions stables seulement après revue croisée.

## 8. Témoins MEC-01 read-only

### 8.1 Stratégie

Approche initiale : debugger externe avec breakpoints matériels tournants. Aucun
octet de code D2R ou de DLL n'est remplacé. Aucun trampoline n'est installé.
Les registres de debug sont le seul état technique modifié par l'attachement;
les captures ne modifient aucun argument, retour ou champ gameplay.

FloatingDamage peut servir de corrélation post-résolution avec son owner actuel,
sans second hook. Le prototype `MeleeSplash` est exclu de tout témoin qualifié.

### 8.2 Artefact minimal par témoin

| Champ | Contenu |
|---|---|
| Witness ID | `MEC-Wxx` stable |
| build/hash | image exacte observée |
| stack | plugins, versions, patches actifs et owners |
| scenario | action et valeurs contrôlées |
| breakpoints | adresses et conditions |
| capture | thread, caller, registres, stack, mémoire brute |
| correlation | GUIDs et pointeur damage record |
| result | `passed`, `failed`, `blocked` ou `not run` |
| limits | perturbations et ce que le témoin ne prouve pas |

### 8.3 Matrice

| ID | Scénario | Attendu | Statut |
|---|---|---|---|
| MEC-W01 | miss puis hit melee | distinguer gate hit et chemin events | `not run` |
| MEC-W02 | CS seul, DS seul, aucun | producteur, flag et ordre RNG | `not run` |
| MEC-W03 | EventFunc15 OW | handler, RNG et création statlist | `not run` |
| MEC-W04 | EventFunc16 CB | handler, vie lue et quantité produite | `not run` |
| MEC-W05 | EventFunc20 ID 384 sans prototype | consommateur BKVince historique et token | `not run` |
| MEC-W06 | life/mana leech pair/impair | étapes et troncatures | `not run` |
| MEC-W07 | room courante/adjacente, obstacle, frontière, mort pendant parcours | callback, LOS/collision, dédup et ordre | `not run` |
| MEC-W08 | OW 1/2/3/4 applications, refresh, deux attaquants, expiration/mort/despawn/disconnect/end-game/GUID reuse | stacks et teardown statlists | `not run` |
| MEC-W09 | player/merc/summon/monster | portée de la couture | `not run` |
| MEC-W10 | dual wield, Smite, kicks, Zeal, Fury, Whirlwind et autres multi-hit | branches ou couture commune | `not run` |
| MEC-W11 | solo/hôte/joiner | autorité et cohérence réseau | `not run` |
| MEC-W12 | dégâts imbriqués, récursion, réentrance et threads concurrents | appel canonique non récursif ou exclusions prouvées | `not run` |

MEC-01 reste bloqué tant que le gate gameplay du cap élémentaire 90 n'est pas
fermé. Un cold start ne remplace aucun de ces témoins.

### 8.4 MEC-01A — session exploratoire solo du 8 août 2026

Cette session n'a pas contourné le gate précédent. Elle a utilisé le Combat Log
et les écrans de debug intégrés à D2RLoader, sans debugger externe, hook,
trampoline, DLL, table, configuration ou formule supplémentaire. Son dossier
est
`analysis-cache/mechanics-92777/20260808-223638-mec01a-exploratory/`.

Baseline observée : D2R `3.2.92777`, D2RLoader `1.0.1-beta`, BKVince solo local,
16 plugins et 18 patches. L'inventaire des 162 modules ne contenait pas
`MeleeSplash.dll`; le prototype se trouvait seulement sous `paused-plugins`
avec SHA-256
`6960BEB1D5F480E77DE05F18D312536F9FE165AE3A3C61FD6CDAD4AE5D664B07`.
Les hashes complets sont dans `runtime-manifest.json`.

Le témoin était `QtyTester` avec un Fleshripper modifié. Les captures Character
Stats prouvent `item_openwounds=100`, `item_crushingblow=100`,
`item_deadlystrike=100` et `lifedrainmindam=50`. Blade Mastery niveau 20 ajoute
une source Critical distincte; le rendu aval ne peut donc pas attribuer un hit
précis à Critical ou Deadly.

Le Combat Log du hit propre contient aussi `PLen=75` et `Pois=42`, composante
runtime non expliquée par les stats sérialisées inspectées. Elle est conservée
comme facteur non contrôlé : elle n'annule pas les témoins CB/OW, mais interdit
d'utiliser ce hit comme preuve d'une formule purement physique.

| Surface | Statut exploratoire | Observation | Limite décisive |
|---|---|---|---|
| MEC-W01 | `passed exploratory` | deux rolls rejetés `78/69`, `97/69`, puis hits acceptés `10/69` et `22/73` | aucun caller/thread/`bMissile`/SUCCESS natif/record corrélé |
| MEC-W02 | `observed` | DS 100 et popup FloatingDamage or/jaune | le bit partagé ne distingue pas mastery CS de DS; branches et seed non tracées |
| MEC-W03 | `passed exploratory` | Misshapen identique gris vers rouge après hit, puis rouge vers gris après `monfreeze=0` | EventFunc15, state 62, statlist, durée, callback, RNG et refresh non tracés |
| MEC-W04 | `passed exploratory` | roll CB `95<100`, montant 25 000 sur 100 000 HP, puis HP 75 000 et 71 496 | handler/core, seed, diviseur interne et ordre natif non tracés |
| MEC-W05/W06/W08 complet/W10/W12 | `not run` | aucun nouveau témoin | contrats inchangés |
| MEC-W07 | `not run` (runtime) | reprise statique : noyau `0x4398B0` fermé, ordre et callback prouvés | room-list adjacente, unicité/dédup et frontière runtime restent `partial` |
| MEC-APPLY-04 | `not run` (runtime) | reprise statique : chaîne melee canonique prouvée trop large | primitive secondaire sûre toujours `unresolved`; disposition `LIKELY_FRAGMENTED` |
| MEC-W09/W11 | `observed` | joueur vers monstre, solo local | autres sources et réseau non couverts |

Le Combat Log intégré suffit à ces observations gameplay, mais pas à un
témoin natif du pipeline. Il n'expose ni thread, caller, `bMissile`, pointeur
`D2Damage`, `resultFlags`, ni les entrées Fill/Calculate/Execute/Finalize. Les
chaînes locales `attackinfo` et `attacklog` ne révèlent aucun format plus riche.

Le hit propre est conservé byte-exactement dans
`capture-01-equipped-hit.txt`; la capture brute bruitée dans
`capture-00-noisy-combat-log.txt`; les stats et l'état OW rouge dans les PNG du
même dossier. L'observation du retrait rouge vers gris vient du compte rendu
explicite de l'opérateur et ne possède pas de capture d'écran.

À `2026-08-09 03:14:03Z`, la session a fini sur un `assert exit`
`package_cache\\tact\\3.5.20\\src\\download\\download_curl.cpp:810`, message
`BC_VERIFY: iter`. La pile contient D2RCore, D2RLoader, KERNEL32 et ntdll, sans
DLL gameplay. Deux rapports du 7 août ont la même identité, le même caller et la
même pile. Le dossier conserve les trois rapports. La classification est donc
**assert TACT/CURL récurrent incident**, sans preuve d'un lien direct avec OW,
`monfreeze`, FloatingDamage, PluginPack ou MEC-01A.

Conclusion limitée : les témoins gameplay sont positifs dans leur portée et ne
doivent pas être décrits comme insuffisants. C'est la **preuve d'architecture**
qui reste incomplète : la session ne ferme ni l'ordre natif, ni une application
secondaire sûre, ni l'aire, ni la réentrance. La conclusion d'architecture
MEC-01A vaut donc `LIKELY_FRAGMENTED`; le verdict global demeure
`NON PROUVÉE` et aucune architecture n'est sélectionnée.

### 8.5 MEC-W01 — identité logique du record et frontière de copie

La preuve statique ferme une correction importante du cahier MEC-01A : le chemin
melee ne transporte pas la même **adresse** `D2Damage` jusqu'au commit.

- `0x44B600` conserve le record amont `P` en `RBX`, l'envoie à Fill à
  `0x44B69B`, puis à CalculateTotalDamage à `0x44B6AC` et à l'enqueue
  `0x4507B0` à `0x44B703`;
- `0x4508FC` copie `P` vers un temporaire, puis `0x450908` transfère ce record
  dans le combat node à `node+0x18`;
- `0x44B37C` appelle `D2Damage_CopyConstructor(LOCAL, node+0x18)`;
- `0x44B3E1` remplace seulement `LOCAL+0x00` par `0x20` avant les events;
- `0x44B3FA` passe `bMissile=0` et `LOCAL` à ExecuteEvents;
- `0x44B4F4` passe le même `LOCAL` à FinalizeDamage.

Le record amont et le record local sont donc deux objets reliés par une copie
profonde gouvernée. Un futur témoin debugger doit corréler leurs champs, les GUIDs
et le callsite de copie; il ne doit pas exiger l'égalité des pointeurs. Quatre
breakpoints hardware sur Fill, Calculate, Execute et Finalize suffisent à une
première vague. Ils doivent être log-only, filtrés sur le couple de GUIDs et
consigner thread, caller, `P`, `node+0x18`, `LOCAL`, `bMissile`, SUCCESS, flags,
physical, total et leech. La comparaison porte sur les champs scalaires stables,
pas sur `+0x00`, les pointeurs du conteneur `+0x40` ou le sous-objet `+0x158`.

Le critère d'acceptation est Fill → Calculate sur le même `P`, puis Execute →
Finalize sur le même `LOCAL`, même thread/couple, retour melee exact,
`bMissile=0`, `SUCCESS=1` et fingerprints `P≈node+0x18≈LOCAL`. Un miss du même
couple ne doit produire aucune séquence pipeline dans la fenêtre bornée. Un
second passage sur les trois copies n'est requis qu'en cas de divergence. Aucun
observer ou nouveau hook n'est justifié à ce stade; ce témoin reste `not run`.

## 9. Matrice de propriété initiale

| Surface | Owner connu | État |
|---|---|---|
| `0x427150` | FloatingDamage | réservé |
| `0x4524D6` | `plugin-items/items.physResistCap` | réservé; option désactivée |
| `0x4524DE` | `plugin-items/items.elementalResistCap` | owner actif, maximum 90 |
| `0x4506A1` | `plugin-items/items.absorbCap` | réservé; option désactivée |
| `0x441B10` | `plugin-items/items.itemDurability`, désactivé | réservé; standalone historique non owner runtime courant |
| `0x448DCA/0x448DE5` | patch kill credit Poison/OW/Burning | réservé |
| `0x44F8F1` | patch enemy resistance BKVince | réservé |
| `0x44CE80/0x583580/0x583B30` | prototype quarantiné uniquement | aucune propriété de production admise |
| autres candidats MEC | à comparer au manifeste exact | `unresolved` |

L'absence d'une chaîne RVA dans les sources n'est pas une preuve suffisante de
non-chevauchement. Le manifeste exact doit produire plages, longueurs et octets.

### 9.1 Baseline statique épinglée

| Artefact | Version / état | SHA-256 |
|---|---|---|
| manifeste PluginPack | branche `codex/pluginpack-foundation`, commit `7540a2a28a01118b43bf344fc587eed1215ddc4d`, build `3.2.92777`, 139 sites | `A4FE83AFB0EFD8C9D98E7176C0961EFBE89057CE9F5D2947659A30E8B351BF8B` |
| `D2RPlugins.json` | configuration source auditée | `7F3CE0442BF8DF3A4D308D1F8E1D3DBF9E7085021A6BB696B4BAA6C6E85F8C86` |
| `plugin-items.dll` | source PluginPack épinglée | `57FCB38A5374F1C199862165420EBF6A4D82205D496F8E8CDF72B010CB05DFCD` |
| `plugin-levels.dll` | source PluginPack épinglée | `4DCFB19EBE9AE9AE2B4B13224CECDE43EA8641253592EE51A117B06BDD170348` |
| `plugin-misc.dll` | source PluginPack épinglée | `B6D0AAF7DC566B0DFF263B1C8E2FA0B597628B0E56D706492832384F71B27AB1` |
| `plugin-quests.dll` | source PluginPack épinglée | `BDDDCFD07149D6D56F1D4B9068EC01993DB3E9FF620A716B90EF1FA91EFB04B1` |
| `plugin-skills.dll` | source PluginPack épinglée | `5261FA145C36E9280599479FEC16C435656F4039DFA5CC15D72406DACA1F0C8E` |
| `FloatingDamage.dll` | owner observer actuel | `C27A8626362101ED30A9D7F6988B085FF7DD75D45E0C84DDAE80672AD257CD29` |
| `enemy-resistance-affects-immunes.json` | patch BKVince actif | `3844D4BA9BBA7A5401247CF6DB201EA34931C14D01B30A67130C0B84036A3B70` |
| `thorns-and-burn-kill-credit.json` | patch BKVince actif | `AF35CC9093B52EFE5476C2D9E8D74EEE8B6F9043BFD5923DA368DC29862CFC79` |
| `allow-ctc-while-uninterruptible.json` | patch BKVince actif | `3C0F328196722099B2717886D9A11548EE089A74D37725ACBC14FDBDB82832A4` |
| `Disable Run Defense and Block Penalties.json` | patch BKVince actif | `E7C069290174F34CD6DF005DB8F244EA1BC552623B52EB67771A74FC12DE6BD4` |

Ces hashes décrivent la source auditée; MEC-01 devra refaire un inventaire du
profil effectivement chargé et rapprocher chaque DLL runtime de sa source.

### 9.2 Résultat de l'audit des écritures

La comparaison exhaustive des 139 sites du manifeste PluginPack avec les 19
fichiers de patches BKVince trouve une seule collision physique exacte :
`0x589736`. Le site est réservé par `plugin-skills`, mais
`skills.whirlwindCtC=false`; le patch BKVince
`allow-ctc-while-uninterruptible` en est donc l'owner runtime actuel avec
`0x58986B`. Toute preuve EventFunc20 conserve cette baseline.

Les entrées `0x44CE80`, `0x583580` et `0x583B30` ne chevauchent aucun site de
ces deux registres, mais sont occupées par le prototype quarantiné dans le
profil source. Elles ne sont ni libres ni utilisables pour MEC-01 tant que
l'inventaire runtime n'a pas prouvé que `MeleeSplash.dll` n'est pas chargé.

Les candidats `0x44C030`, `0x44DF10`, `0x44A9B0` et `0x4398B0` n'ont aucun
chevauchement dans les registres d'écritures audités. Cette preuve signifie
uniquement « aucun owner déclaré dans ces registres »; elle ne prouve pas leur
identité, leurs bornes ni une liberté globale.

Le patch `Disable Run Defense and Block Penalties` possède `0x44B7E1` et
`0x44B9B9`; les bounds de hit chance possèdent `0x44BD45/0x44BD57`. Toute
future couture d'un hit réussi doit observer le pipeline déjà patché après ces
politiques, ou déléguer exactement au code natif ainsi modifié.

## 10. Règle de conclusion

Le verdict demeure **NON PROUVÉE** jusqu'à fermeture de MEC-HIT, MEC-DMG,
MEC-CRIT, MEC-EVT-15/16/20, MEC-LEECH, MEC-RNG, MEC-APPLY, MEC-AREA, MEC-OW et
MEC-OWNER.

Conclusion explicite au terme de cette passe MEC-00 : une couture autoritaire
commune de **commit post-hit** existe statiquement à `0x44CE80`, mais la couture
autoritaire commune requise pour construire un vrai melee splash n'existe pas
encore comme contrat prouvé. Elle n'est pas exclusivement melee, ne fournit pas
à elle seule le gate SUCCESS, l'aire, le lifecycle OW complet ni une primitive
d'application secondaire non récursive. Le verdict architectural reste donc
**COUTURE AUTORITAIRE COMMUNE NON PROUVÉE**.

La conclusion **limitée MEC-01A** est toutefois `LIKELY_FRAGMENTED`, et non
`INSUFFICIENT_EVIDENCE` : la preuve positive de la queue/consommation melee, de
la mutation HP inline dans ExecuteEvents et de la finalisation séparée montre
que les responsabilités nécessaires au splash ne sont pas offertes par une
primitive native unique et sûre. Cela ne promeut pas encore le verdict formel
global à `FRAGMENTÉE`, lequel exige la fermeture des autres contrats et la preuve
qu'aucune couture autorisée commune ne peut les composer.

Valeurs finales autorisées :

- `PROUVÉE` : couture serveur commune stable et sans conflit;
- `FRAGMENTÉE` : plusieurs coutures/branches obligatoires;
- `NON PROUVÉE` : preuve insuffisante ou contradiction non résolue.

Une conclusion `PROUVÉE` est nécessaire mais non suffisante pour sélectionner
une architecture commune. Aucun `Pd2CombatCore` n'est choisi dans ce lot.
