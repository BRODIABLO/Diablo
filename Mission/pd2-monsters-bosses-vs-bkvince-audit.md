# Audit PD2 Monsters, Bosses et Prime Evil contre BKVince

Date de l'audit : **10 août 2026**
Mandant : **Vincent**
Statut : **audit uniquement — aucune implantation gameplay autorisée**
Cible BKVince auditée : dépôt `Diablo`, HEAD
`f38d73854b27376a4183a00529380f197b04100d`, tables actives relues dans
`data-BKVince/BKVince.mpq/data/global/excel/`.
Le HEAD du workspace a avancé dans une lane concurrente pendant la rédaction;
ce SHA et les hashes de §2 fixent volontairement la baseline de comparaison,
tandis que chaque row auditée a été recontrôlée comme inchangée dans la lecture
finale. Le rapport ne réattribue pas les edits concurrents.

## 0. Décision et frontière du mandat

Ce rapport établit la provenance, les valeurs effectives, les classifications
et les routes techniques qui doivent être fermées **avant** tout chantier
`BKVCombat.dll`. Il ne constitue ni une autorisation de merge ni une
spécification de DLL.

Pendant cet audit :

- aucun TXT, BIN, DS1, patch mémoire, plugin, DLL, profil runtime ou sauvegarde
  n'est modifié;
- aucun commit gameplay n'est produit;
- ni `BKVCombat.dll`, ni un plugin `MonsterRules` ne sont commencés;
- les extractions PD2 et calculs intermédiaires restent sous
  `analysis-cache/`, donc hors source distribuée;
- toute ligne provenant de Single Player Plus reste attribuée à **SP+** tant
  qu'une copie identique n'a pas été relue dans le MPQ core S13;
- une affirmation wiki n'est jamais promue en preuve TXT ou native par simple
  répétition.

### Verdict directeur

La famille de changements n'est pas un « monster merge » homogène. Elle se
divise en quatre couches indépendantes :

1. **données directement portables**, par exemple certains niveaux et
   pourcentages `monstats`;
2. **graphe économique à adapter**, parce que les Treasure Classes PD2 legacy
   et D2R 3.2 n'ont ni le même schéma ni le même comportement quest-drop;
3. **cartes**, notamment `MephComp.ds1` pour les Council Members;
4. **règles natives**, notamment le sélecteur Aura Enchanted, l'explosion
   retardée des Dolls et plusieurs immunités Prime Evil.

Deux « merges » demandés seraient déjà des régressions si les valeurs PD2
étaient copiées aveuglément : BKVince porte déjà Duriel au niveau 88 en Hell et
ses sept Tal Rasha's Tomb sont niveau 87, contre 82 dans le core PD2 S13.
BKVince configure aussi des multiplicateurs de 150 % contre les mercenaires et
200 % contre les pets. Sous la sémantique moteur héritée, le chemin PD2 donne
un total de 200 % contre hireling, mais **400 %** contre un autre monstre
`ISREVIVE` : le wiki sous-décrit les pets. Face à la configuration BKV, les
deltas projetés sont donc +33,33 % contre mercenaire et +100 % contre pet;
l'effectivité exacte des consommateurs 92777 reste à valider.

## 1. Hiérarchie de preuve employée

Les verdicts utilisent exclusivement les catégories exigées par le mandat :

| Catégorie | Critère d'admission |
|---|---|
| **Prouvé dans les données PD2** | cellule relue byte-exactement dans un fichier extrait directement du MPQ core S13 hashé |
| **Prouvé dans le code ou plugin PD2** | comportement identifié dans `ProjectDiablo.dll` S13 avec identité binaire, xrefs et contrôle de flux suffisants |
| **Documenté uniquement par le wiki** | texte wiki épinglé, sans corroboration directe dans les données ou le code disponibles |
| **Présent uniquement dans SP+** | différence absente du core S13 relu et présente dans le snapshot épinglé |
| **Inférence forte** | conclusion convergente de plusieurs preuves, mais sans observation du dernier handler effectif |
| **Inconnu** | sources disponibles insuffisantes pour trancher |
| **Contredit par les données** | valeur ou comportement incompatible avec la cellule core S13 actuelle |

Les verdicts fonctionnels sont : **vrai**, **vrai mais incomplet**,
**trompeur**, **faux** ou **non prouvable**. « Vrai dans S13 » ne signifie pas
nécessairement « vrai lors de la saison qui a introduit le changement ».

## 2. Sources, révisions et identités

### 2.1 Wiki officiel PD2 épinglé

La source de synthèse est
[`Monsters`, oldid 23935](https://wiki.projectdiablo2.com/w/index.php?title=Monsters&oldid=23935),
révision du **20 juillet 2026 à 23:33:41 UTC**, pageid `2`, parent `23877`,
SHA-256 du wikitext
`10D8DBC06514F9A0B97345B8AF67B05B30187CF1C07CAAEBED30391DD9A2A770`.
Cette révision s'auto-déclare incomplète ou obsolète à partir des changements
de la saison 5 : elle est une source documentaire, pas une preuve d'exécution.

| Sujet | Section exacte de la révision 23935 |
|---|---|
| Aura Enchanted, Holy Shock, Dolls | [`Regular Monsters`](https://wiki.projectdiablo2.com/w/index.php?title=Monsters&oldid=23935#Regular_Monsters) |
| niveaux d'auras affichés | [`Monster Aura Stats`](https://wiki.projectdiablo2.com/w/index.php?title=Monsters&oldid=23935#Monster_Aura_Stats) |
| groupes et quatre règles Prime Evil | [`Prime Evils`](https://wiki.projectdiablo2.com/w/index.php?title=Monsters&oldid=23935#Prime_Evils) |
| Andariel | [`Andariel`](https://wiki.projectdiablo2.com/w/index.php?title=Monsters&oldid=23935#Andariel) |
| Duriel et Tal Rasha's Tomb | [`Duriel`](https://wiki.projectdiablo2.com/w/index.php?title=Monsters&oldid=23935#Duriel) |
| Mephisto et Council | [`Mephisto`](https://wiki.projectdiablo2.com/w/index.php?title=Monsters&oldid=23935#Mephisto) |
| Countess, Summoner, Nihlathak | [`Key Holders`](https://wiki.projectdiablo2.com/w/index.php?title=Monsters&oldid=23935#Key_Holders) |
| Ubers | [`Ubers`](https://wiki.projectdiablo2.com/w/index.php?title=Monsters&oldid=23935#Ubers) |
| Uber Tristram | [`Uber Tristram Prime Evils`](https://wiki.projectdiablo2.com/w/index.php?title=Monsters&oldid=23935#Uber_Tristram_Prime_Evils) |
| Diablo Clone | [`Diablo-Clone`](https://wiki.projectdiablo2.com/w/index.php?title=Monsters&oldid=23935#Diablo-Clone) |

Patch notes officielles utilisées comme documentation de changement :

| Page épinglée | Révision et date wiki | Passage pertinent |
|---|---|---|
| [`Patch:Season 1`](https://wiki.projectdiablo2.com/w/index.php?title=Patch%3ASeason_1&oldid=17689) | oldid `17689`, 12 oct. 2023 | entrées internes 8/10/19/28 nov. 2020 : Dolls et restauration du tag Prime Evil |
| [`Patch:Season 2`](https://wiki.projectdiablo2.com/w/index.php?title=Patch%3ASeason_2&oldid=17688#GENERAL_CHANGES) | oldid `17688`, 12 oct. 2023 | `GENERAL CHANGES` : Council déplacé, tag Prime Evil fonctionnel |
| [`Patch:Season 3`](https://wiki.projectdiablo2.com/w/index.php?title=Patch%3ASeason_3&oldid=17687) | oldid `17687`, 12 oct. 2023 | Holy Shock, Dolls, Ubers, Lower Resist, Static Field et Decrepify |
| [`Patch:Season 4`](https://wiki.projectdiablo2.com/w/index.php?title=Patch%3ASeason_4&oldid=17686#GENERAL_CHANGES) | oldid `17686`, 12 oct. 2023 | Andariel, Duriel, Tomb, DClone PLR/curse |
| [`Patch:Season 6`](https://wiki.projectdiablo2.com/w/index.php?title=Patch%3ASeason_6&oldid=17684#Paladin) | oldid `17684`, 12 oct. 2023 | exclusion Sanctuary des Prime Evils |
| [`Patch:Season 11`](https://wiki.projectdiablo2.com/w/index.php?title=Patch%3ASeason_11&oldid=21702#BUG_FIXES) | oldid `21702`, 29 nov. 2025 | correctifs ultérieurs consultés |

Révisions secondaires :

- [`Zones`, oldid 22824](https://wiki.projectdiablo2.com/w/index.php?title=Zones&oldid=22824#Zone_Levels),
  29 avril 2026;
- [`Offensive Auras`, oldid 23893](https://wiki.projectdiablo2.com/w/index.php?title=Offensive_Auras&oldid=23893),
  29 juin 2026;
- [`Curses`, oldid 23813](https://wiki.projectdiablo2.com/w/index.php?title=Curses&oldid=23813),
  21 juin 2026;
- [`Lightning Spells`, oldid 23708](https://wiki.projectdiablo2.com/w/index.php?title=Lightning_Spells&oldid=23708),
  4 juin 2026.

### 2.2 PD2 core S13

Source locale : `C:\Games\Diablo II pd2\ProjectD2`, saison 13
**Betrayal**. Les identités suivantes bornent toutes les affirmations « core
S13 » :

| Artefact | Octets | SHA-256 |
|---|---:|---|
| `ProjectDiablo.dll` | 4 312 576 | `538A77B7CCEF3D5334E56C4E9E57A4D8FC69A1E27C46BEB694C0DEDFCFBF9CB3` |
| `pd2data.mpq` | 2 157 546 | `196F9DA7F5A7EEAD7BC000137514E6B564E50E893CA89BB4711359D03C29CE63` |
| `pd2maps.mpq` | 4 881 728 | `5D39DBD10A24D17177D44007B9AEB094EDAF68A4FFB6913A833DDEA439848A5F` |
| `pd2assets.mpq` | 90 136 847 | `900522D3968752EF451E6BFAFD62FE4B2AEC2EC874C00E925D5A793A42B90831` |
| `pd2monchars.mpq` | 373 591 568 | `AE2ABADE85C5F393DB5D8089CB4B6566B2CFED2ADEA0F573BC0F4220EF733217` |
| `patch_d2.mpq` | 2 108 703 | `B976E7847DDA696F54198A9477819F96579C4894F54381C72AFB4937E919E0A9` |
| `PD2_EXT.dll` | 86 528 | `A47E6F026C867035EF5360E0F86061D0FA06746DFD249CE9E794DC6758766F49` |
| `BH.dll` | 1 423 360 | `CBA04FF9E6F944942D69E4B610EEB0A8434BC780D312970EA922877FD134AC74` |

Les tables citées comme core ont été extraites directement de
`pd2data.mpq`, puis lues avec `scripts/build-data/tsv.js`; chaque lecture a
passé un round-trip byte-exact. Principales identités :

| Table core S13 | Lignes de données × colonnes | SHA-256 |
|---|---:|---|
| `MonStats.txt` | 1241 × 255 | `1B3C08D28B4093D4C154798A5AE9E794F0ED08B1702EE2CB938B1BC5C2E4C5B2` |
| `MonStats2.txt` | 733 × 126 | `A0EC94D797AAC461E0AC0AE88E0B07F24EFB5EA7E8BF73515DB5D4BCF73BB9D6` |
| `MonLvl.txt` | 111 × 31 | `BDA243EFE1D1E9591F75B8CF97AC73F78FF691362381F35CC000233EB46C092A` |
| `SuperUniques.txt` | 68 × 21 | `1B9B8A3B949F03D2D08B94F46DC7FED5A00B27FEE90FE89B9ECF428EE80ADA48` |
| `TreasureClassEx.txt` | 1087 × 33 | `83EDCCF0101EC0142E265B9D2AED2CA1FDC2BB698F7A46C818E6C9ED2C753F4A` |
| `Levels.txt` | 203 × 185 | `FCAC65007E75698981312927C4539E494E098083DFDC5564735F26108189B712` |
| `MonUMod.txt` | 43 × 19 | `D515513E953C4DB98B39F1AF9CB6D58C972630A0F9A765D1381411128B9CDF0C` |
| `DifficultyLevels.txt` | 3 × 23 | `09FD2C8BAD2021D5E3DC7065F342BD49BD034F1082C528CA95F3C92EF83C1EDC` |

Le core ne fournit localement ni source C++ ni PDB. Les archives et modules
ci-dessus ont été inventoriés, mais les preuves natives positives de cet audit
proviennent du désassemblage de **ce** `ProjectDiablo.dll`; une recherche
infructueuse n'y prouve pas l'absence dans `PD2_EXT.dll`, `BH.dll` ou les DLL
vanilla. Aucune adresse D2MOO ou D2R n'est transposée.

### 2.3 Snapshot Single Player Plus

Snapshot :
`C:\Workspaces\PD2 Single PLayer\PD2-Single-Player-Plus-mod-main`.

- dépôt catalogué :
  [`Lukaszpg/PD2-Single-Player-Plus-mod`](https://github.com/Lukaszpg/PD2-Single-Player-Plus-mod);
- commit catalogué : `3debc6781f33c3c1474a995b80369a4e618cd386`;
- tree annoncé : `6f51e17e5f65abdd50b2fd33190c571fef296ccf`;
- 198 fichiers, dont 93 tables Excel, 90 BIN et `BH.dll`;
- **aucun DS1, AnimData, COF/DCC de monstre, `ProjectDiablo.dll`, source ou
  PDB**;
- le dossier n'a pas de `.git`; le commit est donc une provenance cataloguée,
  pas une égalité au tree distant reconstructible localement;
- la métadonnée interne `12.0.0a` ne concorde pas avec l'étiquette 13.0.2 du
  commit : les hashes, non le numéro marketing, gouvernent cette source.

Empreinte agrégée des 93 enfants immédiats `*.txt` de `data/global/excel` :
`AED5AC542E7B879FBF6BEB49F7F76A8ED40F5725DC830E82536CBA2A1C44A2B8`.
Elle est construite en triant les noms normalisés en minuscules, puis en hashant
en UTF-8 les 93 lignes `nom<TAB>SHA256_DES_OCTETS`, jointes par LF sans LF final.
`node scripts/audit-pd2-bkvince/audit.mjs` reproduit ce résultat (`PD2=93`,
`roundTrip=true`, EOL LF); l'empreinte ne couvre ni BIN, ni `BH.dll`, ni assets.

Le snapshot n'est pas une copie exacte du core S13. Exemple décisif :
`TreasureClassEx.txt` core et SP+ divergent sur les runes, essences, NoDrop,
objets SP+ et plusieurs TCs de boss. Dans `MonStats.txt`, la ligne
`uberdiablonew` a ses trois `TreasureClass1/2/3(H)` vides dans le core, mais
`UberDiablo` dans SP+. Ce drop est donc **présent uniquement dans SP+**.

### 2.4 BKVince et D2R 3.2.92777

Tables relues byte-exactement :

| Table BKVince | Lignes de données × colonnes | SHA-256 |
|---|---:|---|
| `monstats.txt` | 799 × 273 | `6DC2F8D715C2D2F9F727F3C3429D041D379237062FDA6E6629E20508862F647A` |
| `monstats2.txt` | 628 × 127 | `AAA34238086B8D11B02FCABEDF09324DD38B23AC6DDF44E6E81FAC11216E2764` |
| `monlvl.txt` | 127 × 31 | `AAA9CB26B31541AFA71AAC4A8DB4387E0CCC167131C4DD5A6A45250AE3B428F0` |
| `difficultylevels.txt` | 3 × 34 | `21B2EE3A73C226200EA7DE5340C6DBB4E9EFEE0EEAC9D167401DC9A378A78C8C` |
| `treasureclassex.txt` | 1416 × 39 | `2655A9DDADE428A678600D610D3FE631906D9E52614CAACD62CB3D576490A9B7` |
| `levels.txt` | 148 × 188 | `3A3D7C8649EF25FFE31829FDA7E829F370F0A673D136D6D2EB47B5C8F6C2F7C2` |

**Dérive concurrente observée.** La baseline de calcul est le fichier au HEAD
et au début d'audit, SHA `6DC2F8...F647A`. Pendant la rédaction, une autre lane
a successivement produit `A989AD4...71CC`, puis `53C7EED...11D4`, uniquement
sur `roguehire` et les deux rows Raven/tombstones en fin de table. Aucune row
Andariel/Duriel/Mephisto/Council/Uber/DClone/Doll/key-holder auditée n'a changé.
Le rapport conserve donc la baseline `6DC2F8...` et n'attribue ni n'écrase ces
edits concurrents.

`monmode.txt` et `lvltypes.txt` sont absents de l'overlay BKVince : leur valeur
effective vient de `data-vanilla3.2`. BKVince n'override ni
`Act3/Travincal/MephComp.ds1`, ni `Act2/Tomb/Duriel.ds1`, ni AnimData. Pour ces
ressources, sa baseline effective est D2R 3.2.

Plugins et patches qui modifient le résultat effectif :

- `StaticFieldRework.dll` est actif et appelle d'abord le Static Field natif,
  puis applique son debuff de résistance foudre;
- `enemy-resistance-affects-immunes.json` change le chemin de pierce;
- `hit-chance-bounds.json` remplace les bornes 5–95 % par 0–100 %;
- `ignore-target-defense-champions-uniques.json` étend ITD;
- les réglages `/players` retirent certains clamps et doivent être inclus dans
  tout test pN;
- `ReviveOverhaul.dll`, `MeleeSplash.dll` et le patch de kill credit rendent
  insuffisante toute conclusion d'ownership fondée uniquement sur les TXT.

Identités exactes de cette pile au moment de l'audit :

| artefact BKVince | octets | SHA-256 |
|---|---:|---|
| `d2rloader/plugins/StaticFieldRework.dll` | 74 752 | `9BCF9C96D21A21AD54BA66C8795B2900FC569348D7F30793359D1C6020DE8883` |
| `d2rloader/config/StaticFieldRework.toml` | 148 | `734271EB84151A443DCC81CBE0CB915F76F5EA82FEA3CEC1F5703581DAEFC708` |
| `d2rloader/plugins/ReviveOverhaul.dll` | 59 904 | `7A84CAE45C82D4B981AAF980AB38B80CF744F863FD53674C326F2DF63F59EDE2` |
| `d2rloader/config/ReviveOverhaul.toml` | 2 050 | `D11033EEC1A512705BFB9CBD60642F427ED5F1003C15B745886D66A0E3226C45` |
| `d2rloader/plugins/MeleeSplash.dll` | 199 168 | `DBA0C40C191B2568A6B39D21324A45F770C1CBF8AD747B099AA3BCBEDEF8856C` |
| `d2rloader/config/MeleeSplash.json` | 602 | `0A7B1878C6D20CE3362F9B95055B7DBF9E56EDEC81C24001E2B88A400017802D` |
| `patches/enemy-resistance-affects-immunes.json` | 402 | `3844D4BA9BBA7A5401247CF6DB201EA34931C14D01B30A67130C0B84036A3B70` |
| `patches/hit-chance-bounds.json` | 1 511 | `302317BE790D4C4F5CB463D69A25013071CA388B02B7ED51986C98B80E199C22` |
| `patches/ignore-target-defense-champions-uniques.json` | 362 | `4831786F90C471590010862DF18FBC8E88A7C02C5571CFA350411944F2090C13` |
| `patches/player-difficulty-overrides.json` | 1 801 | `83DB69A08143BB1951FEE051164E810D8D4936F237AB39B920B3568B9661F58B` |
| `patches/thorns-and-burn-kill-credit.json` | 1 138 | `AF35CC9093B52EFE5476C2D9E8D74EEE8B6F9043BFD5923DA368DC29862CFC79` |
| `patches/normal-area-level-scaling.json` | 391 | `7967DC5B919839FA16EF31F71EC74FE60CDA00158B4EB1367225AA9CFF87BC98` |
| `BKVince.mpq/D2RPlugins.json` | 15 777 | `7F3CE0442BF8DF3A4D308D1F8E1D3DBF9E7085021A6BB696B4BAA6C6E85F8C86` |

Les valeurs gouvernant les projections pN dans ce `D2RPlugins.json` sont
`playersCommandLimit=8`, `monsterHpPlayerCountCap=0` et
`monsterExperiencePlayerCountCap=0`; zéro désactive ces deux caps statistiques,
il ne signifie pas « player count zéro ». Les cinq DLL du PluginPack ont été
traitées comme couche active obligatoire pour les futurs tests, mais aucune de
leurs cellules/configurations n'est utilisée comme preuve numérique dans ce
rapport; une photographie hashée de toute la pile relève d'un futur cold start,
qui n'est ni exécuté ni revendiqué par cet audit-only.

Le patch `normal-area-level-scaling.json` gouverne spécifiquement le lookup
Normal BKV au RVA92777 `0x543D32` : il remplace
`45 85 FF 74 19` par `45 85 F6 7E 19` (`test r14d` puis `jle`) afin d'utiliser
le niveau de zone lorsqu'un `LevelId` de room est positif, et le niveau de base
sinon. C'est pourquoi la Countess BKV niveau brut8 consulte le niveau de zone7
en Normal; cette extension BKV ne provient pas de la règle D2MOO NM/H.

Workbench natif disponible : image D2R canonique
`CC59119DC2A6C7D43D088098FC162EAFA4AE1299B2079126AEF43C1ACA914715`,
image d'analyse
`673E8C0B2E89563E75525B24D137098EFD07B2DB4ED42ADEC56AA1ADDF0E63AB`,
build `3.2.92777`. Les RVA 1.10f ou PD2 ne lui sont jamais transposées.

### 2.5 Couverture des tables et ressources demandées

| source inspectée | usage ou résultat |
|---|---|
| `monstats.txt`, `monstats2.txt` | IDs, flags boss/prime/flying/noAura/deathDmg, niveaux, HP/AC/dégâts/résistances/block, collision/sélection |
| `monlvl.txt` | reconstruction HP/AC/dégâts/CTH par niveau; clamp au dernier record |
| `monumod.txt` | éligibilité/poids Aura, mods élémentaires et cap d'immunités |
| `monprop.txt`, `properties.txt`, `itemstatcost.txt`, `events.txt` | chaîne Doll `death-skill`; PLR/curse/absorb DClone |
| `superuniques.txt` | Countess/Summoner/Nihlathak, six Council et mods fixes |
| `monai.txt` et code AI | slots Uber Baal, catégories AI; la fréquence exacte de case8 reste inconnue |
| `monseq.txt`, `monmode.txt` | aucune seconde source de Doll retardée; BKV fallback D2R pour `monmode` |
| `monpreset.txt` | résolution des IDs locaux Bremm/Wyand/Maffer et Mephisto dans le DS1 |
| `difficultylevels.txt` | Static floors, ancien MonsterCE, multiplicateurs BKV player/merc/pet |
| `skills.txt`, `missiles.txt`, `states.txt` | Aura pool, DollMeteor, Conviction, Baal Lowres, curses, Sanctuary, Static |
| `treasureclassex.txt` | graphes récursifs, Picks/NoDrop/cap six, essences, keys et probabilités p1 |
| `levels.txt` | sept Tombes et Lair, densité et groupes de monstres |
| `lvlprest.txt`, `lvltypes.txt`, `lvlmaze.txt` | DS1 gouvernants, quatre orientations de tombe, géométrie et fallback BKV |
| AnimData/animations | SP+ et BKV n'en embarquent pas; D2R fallback `animdata.d2`, 586784 octets, SHA `61EA70EDE691E88A4BF375D872668D74D6CDCA4ED5235034188ED1EB0D1E8EE5`, et `eanimdata.d2`, 565984 octets, SHA `C910E23DA67DE84D5D0295159C2725778FCE53AF3A819B3D989E71005420113C`. La configuration du délai Doll S13 vient de `missile.Range=25`, non d'une animation; sa conversion effective en25f reste une inférence forte des handlers hérités. Aucune absence TXT n'est transformée en hardcode. |
| DS1 | `MephComp.ds1` extrait de `pd2maps.mpq` et comparé structurellement; tomb/lair presets inspectés; aucune carte BKV pertinente |
| patches/plugins PD2/SP+ | `ProjectDiablo.dll` S13 désassemblé pour les handlers ciblés; SP+ ne fournit que `BH.dll`, non utilisé comme preuve core |
| patches/plugins BKVince | StaticFieldRework, résistance/immunité, CTH/ITD, player count, ReviveOverhaul, MeleeSplash et kill credit pris en compte |
| workbench D2R 3.2.92777 | statut vérifié; handlers Static/curse/slow gouvernés relus; aucune RVA PD2 promue dans `known-rvas.json` |

#### 2.5.1 Manifeste cellulaire reproductible

Le tableau suivant ferme la provenance de **chaque table effectivement citée**.
La colonne BKV est la baseline du début d'audit; `fallback` signifie que
l'overlay BKV n'embarque pas la table et consomme la copie D2R 3.2.92777. Les
quatre astérisques signalent des edits concurrents intervenus après la lecture :
ils ne touchent aucune row utilisée dans ce rapport, qui reste calculé sur les
hashes indiqués.

| table | PD2 core S13 — rows×cols; SHA-256 | SP+ — rows×cols; SHA-256 | BKVince baseline/effective — rows×cols; SHA-256 |
|---|---|---|---|
| `monstats.txt` | 1241×255; `1B3C08D28B4093D4C154798A5AE9E794F0ED08B1702EE2CB938B1BC5C2E4C5B2` | 1242×255; `15DEFD1F57A060D8913C4A0CDBDABA5566DD296E8DF4F4525E2F949894DB17F5` | 799×273; `6DC2F8D715C2D2F9F727F3C3429D041D379237062FDA6E6629E20508862F647A`* |
| `monstats2.txt` | 733×126; `A0EC94D797AAC461E0AC0AE88E0B07F24EFB5EA7E8BF73515DB5D4BCF73BB9D6` | 733×126; `DBA5FB3A39DE77839794DA630020F5C67F0933D88D460C41B06A0692A156BEA4` | 628×127; `AAA34238086B8D11B02FCABEDF09324DD38B23AC6DDF44E6E81FAC11216E2764` |
| `monlvl.txt` | 111×31; `BDA243EFE1D1E9591F75B8CF97AC73F78FF691362381F35CC000233EB46C092A` | 111×31; `7624ABE33673F8BF4F243DE141FB387A1D78F1192466ACE49AC8B3C175FA4DF8` | 127×31; `AAA9CB26B31541AFA71AAC4A8DB4387E0CCC167131C4DD5A6A45250AE3B428F0` |
| `monumod.txt` | 43×19; `D515513E953C4DB98B39F1AF9CB6D58C972630A0F9A765D1381411128B9CDF0C` | 43×19; `94A595466ECD636EA9493E734D692FA718088DDEE3AF2D66671D42753FD7FBDF` | 45×18; `D3D17CFB905BCC138A30BED872D5DD5D219E61F0591A6FDAB5BB444012B52260` |
| `monprop.txt` | 66×92; `B0979AE1BB06D90A8B886A341CA9D0AEE9D7F79335D1CBE99B43E2508174C8E8` | 66×92; `4B12067264C23CCA8C9DFB08B2FE1F33FC9CD98379A6E63AC386F389545BF86B` | 15×92; `2C2D18FAEA482DFF2D86ACE9022214993941E7D1A3D1C31512A08C3F812B9A02`* |
| `superuniques.txt` | 68×21; `1B9B8A3B949F03D2D08B94F46DC7FED5A00B27FEE90FE89B9ECF428EE80ADA48` | 68×21; `FEDAA063116A0BB9D14E9D8C39106BB21178F7ED68AD204495ED35063C3964D6` | 70×23; `2D1E786D075EC65CBFEB34E4D8E89BDA1763965E52A200D7057E82D43ABCAE61` |
| `monai.txt` | 148×10; `0180650484293E4DFDB530BA67FC3090164BA938ACB1948393A3D1E46919A466` | 148×10; `1F41DD82FDCE182E8F3F6F5627B6CD0D21A8F510C68D507EB847BDF84F6799E8` | 155×10; `F473DD467D6C54A58DF63FBE3A2BC0FBBF468B7FD5F30EBCF43192E020ADEDF6` |
| `monseq.txt` | 1220×6; `F8171D14C19A39D4B81E015973F59C2EFCFC73A77485F5E93E4BB54470ECAE7C` | 1220×6; `F7075F1222D2B04BE524D7F6A79761323E2764F4F74D3FE1A2D9C899BB4E2E32` | 1030×6; `A48A884A654CAB7CAFE0BE2764FA8A77F428A9B48855D27461D133A1F553898C` |
| `monmode.txt` | 16×3; `2663C45B5CA74916DB0114335BABE7FC6DD9D357322892DF501F7850E4385B52` | 16×3; `7305927DBAE0377357AB106EA6FBAE4267796513DBF02908EB5DC2689A63F827` | fallback D2R 16×3; `2663C45B5CA74916DB0114335BABE7FC6DD9D357322892DF501F7850E4385B52` |
| `monpreset.txt` | 324×2; `A99ED04F9268144F184E19D58394F745A348AC0E1287B79927BC5E5A3097FEE8` | 324×2; `4D2C2BCB984F1041ABBD56054B2E6A954F91E74ABC21308D46EB9DA19219F5F6` | 236×2; `B261B1486839BD4E76B815B0364F863C7BC7DEF3607DF40DCBF7A21F57122DFF` |
| `difficultylevels.txt` | 3×23; `09FD2C8BAD2021D5E3DC7065F342BD49BD034F1082C528CA95F3C92EF83C1EDC` | 3×23; `5B4AF6E92C01D5AF403A13A5A1199B67B9B98CA93A11D6CC140C772273CCBFCA` | 3×34; `21B2EE3A73C226200EA7DE5340C6DBB4E9EFEE0EEAC9D167401DC9A378A78C8C` |
| `skills.txt` | 603×256; `19F8EFBAFBE5E2E58F0F010028EFC580A4A1EC8899E857094F2CD94B6DD806F4` | 603×256; `AEEFC3F2C0C80811D62FC1A17C3B031DE2164E5606BF9779F34024B35BC87B8B` | 449×322; `E32AE30F93D3BB18B514FE9487FCBCAAF0CDAA5C8C8CD03ABD7B51645ABA074F`* |
| `missiles.txt` | 1057×171; `056EB678514D8E1F6162EF915260C05A12B53639564D8A0A320DA0BC0DEE196E` | 1057×171; `39AF537714AABAE75ED11DE46B7BAC9DCE8635E16D8BE7A93767ACAF7B7F299D` | 759×172; `0A486649B245A2B6EFAEAD653EB58A95FFFB047AE595D1B2B9D185FF5D3AFB3B` |
| `states.txt` | 242×72; `78E8DE6BC46037B5B35F90299DD45DD1B744BB1D3DA6E06BBD81787066842828` | 242×72; `E7296F91AE7B02898D0356A91BE190F87CC41F820FED1EF9DC5D2F64DAC08DE2` | 247×75; `DA580CA0FDB0713FC62663602BB7BF81EB4D2AB0D422031901D676A2876E0940`* |
| `treasureclassex.txt` | 1087×33; `83EDCCF0101EC0142E265B9D2AED2CA1FDC2BB698F7A46C818E6C9ED2C753F4A` | 1089×33; `ADE002CD703F9BBC4E1065EA688D593472C54161BFD7E05E9FCF546ACD75E9A7` | 1416×39; `2655A9DDADE428A678600D610D3FE631906D9E52614CAACD62CB3D576490A9B7` |
| `levels.txt` | 203×185; `FCAC65007E75698981312927C4539E494E098083DFDC5564735F26108189B712` | 203×185; `FCC4E22F5AE6FA2A6B9C8876C9FC2406F33BC7C75CA37BFF0821F740179E45AF` | 148×188; `3A3D7C8649EF25FFE31829FDA7E829F370F0A673D136D6D2EB47B5C8F6C2F7C2` |
| `lvlprest.txt` | 1158×25; `3E0D3B7FFFC6D22E48D5FD4D271E347B70337294BEA0E15E89B181B36B21A801` | 1158×25; `530145BE8CA8CBFF854BE4B2E701E053BC0A25596F733E5FD671E2FBD3B1D526` | 1096×23; `0027E003C74D6073B325EF4C35C39620E33F7D48CC351643BC66ED1996EF319C` |
| `lvltypes.txt` | 47×37; `BB317366621D45CC71F5AB7F8547D144E94BE42078EEC1362D71D31404E50CAC` | 47×37; `E9C7AB953223A24CB4891D2EE8C46F24863EAC54E33A6FB2B86510C512EE1306` | fallback D2R 37×35; `578A75BEABF4A1C6CB11679A6F01008908CA419CCDDF8F0239384FA15081AD36` |
| `lvlmaze.txt` | 82×9; `83ED16BFC245B536E7D6005C11551454109C76E0E7E8D88AC41405D9B7072784` | 82×9; `02462C302FF7DAE04A31D3636A2950FD855325533A0585FE59D1F5DE4CE347F6` | 88×8; `0C22EE8F0435E10992DD6CE127086A6464057F5E2A2AC9882E9FA15C5FE45779` |
| `properties.txt` | 450×36; `9AFBA0A507D237DFB85A6FA93EDAC88E39276DE97F3CAC64805E368D96D582B5` | 450×36; `B31F6ACD99012D2AF02C233C64FBA600E3EEA27E4019386D210D6D4F83AE837A` | 312×38; `1908ADDFA2FB821ABC4ED7740D2E0ABEBEB48294BB88A40E6E1DCB3EBE2D5BF1` |
| `itemstatcost.txt` | 511×56; `023E47130E5CCE6FBE1E8046FA062793FB8CA6A640DB5D0E91224A0D66D1DBCE` | 511×56; `B2A70C4E258A8905A2997B666BB70CE9958699D1725CF4EB27C2B5795B04A6DC` | 393×52; `726773CA8A4BB702BA103E8EF41808CD7086476F617BC8B1D8F93D09040D5C95` |
| `events.txt` | 18×2; `04B553309FFF63CAC2044ACE77EDAD57FC71474CB9C618520033557575534F84` | 18×2; `08ADD426941956348CB7FDB72C23CE13885DB55066A25DA9F73386FE5D69958B` | fallback D2R 17×2; `320E880DBDFBC3C8DB714A56C15EF7FBFB48FF115BAD6CE0A6843BB192ECFC9C` |

### 2.6 Ledger natif borné par binaire

| binaire | RVA | identification | confiance/limite |
|---|---:|---|---|
| ProjectDiablo S13 | `0x2C62B0` | dispatcher Aura Enchanted, spécial Uber Meph/De Seis, niveau mlvl/7 cap13 | haute; signature d'entrée `55 8B EC 83 EC 0C A1 0C 80 39 10 33 C5 89 45 FC 57 8B 7D 0C 83 3F 01` |
| ProjectDiablo S13 | `0x126C80` | constructeur du vecteur de huit auras | haute |
| ProjectDiablo S13 | `0x2AFA40` | wrapper EventFunc19, refus item slow sur Prime | haute; signature `55 8B EC 51 53 8B D9 89 55 FC 56 57 85 DB` |
| ProjectDiablo S13 | `0x2BEF70`, `0x2BFE20` | doubles gardes Decrepify skill/state | haute |
| ProjectDiablo S13 | `0x26F680` | résistance physique Sanctuary, exclusion Prime | haute |
| ProjectDiablo S13 | `0x265480` | évaluation aurafilter et bit `IGNPRIME=0x40000` | moyenne-haute |
| ProjectDiablo S13 | `0x2785B0` | wrapper du prédicat `primeevil` | haute sémantique |
| D2R 3.2.92777 | `0x5849D0` | `D2GAME_EventFunc19_Slow`, caps mais aucun guard Prime | preuve gouvernée existante |
| D2R 3.2.92777 | `0x5546B0` | `SKILLS_SrvDoFunc20_StaticField` | preuve gouvernée; utilisé par StaticFieldRework |
| D2R 3.2.92777 | `0x55D6B0` | `SKILLS_SrvDoFunc30_Curse` | preuve gouvernée; second étage StaticFieldRework |

Les RVA ProjectDiablo appartiennent exclusivement au DLL S13 hashé et ne sont
jamais transposées au build92777. Elles restent des preuves d'audit en cache;
toute future promotion gouvernée exige une reproduction indépendante selon le
skill de reverse engineering. La garde exacte DClone/Static S13, le pool Aura
D2R et la fréquence AI Uber Baal restent inconnus.

## 3. Contrats de calcul effectif

### 3.1 Niveau, vie, défense et dégâts

Le contrat sémantique historique est corroboré par D2MOO épinglé au commit
`19019806df7f3e877fa105b05395d1e3597e2316` :

- `MonsterTbls.cpp:562-674` : si `noRatio=1`, les cellules `MonStats` sont des
  valeurs directes; sinon HP, AC et dégâts sont des pourcentages appliqués aux
  colonnes `MonLvl`;
- en Expansion, l'offset sélectionne `L-HP`, `L-AC`, `L-DM` et `L-TH`;
- `Monster.cpp:197-201` : le flag `boss` conserve le niveau de `MonStats`; les
  monstres ordinaires non-`noRatio` de Nightmare/Hell prennent le niveau de
  zone;
- `Monster.cpp:218-228` : la vie de base est tirée uniformément et
  inclusivement entre min et max, reçoit le bonus player-count, puis est
  stockée en fixed-point `×256`;
- `MonsterTbls.cpp:571-574` : un niveau supérieur à la dernière ligne
  `MonLvl` est clampé à cette dernière ligne dans la sémantique 1.10f. Toute
  affirmation identique pour PD2 ou D2R reste une **inférence** tant que son
  handler moderne n'est pas observé.

Formules employées pour les valeurs de tableau de ce rapport :

```text
HPmin_p1 = ratio(L-HP[difficulté, niveau effectif], MinHP[difficulté], 100)
HPmax_p1 = ratio(L-HP[difficulté, niveau effectif], MaxHP[difficulté], 100)
AC        = ratio(L-AC[difficulté, niveau effectif], AC[difficulté], 100)
A1min     = ratio(L-DM[difficulté, niveau effectif], A1MinD[difficulté], 100)
A1max     = ratio(L-DM[difficulté, niveau effectif], A1MaxD[difficulté], 100)
```

`ratio` suit la troncature entière du moteur. Les intervalles indiquent les
deux bornes avant roll. Les multiplicateurs de superunique/champion ne sont
ajoutés que lorsque le chemin de spawn les applique; un Act boss avec
`boss=1` n'est pas doublé silencieusement parce que son nom est unique.

Les valeurs A1/A2 de ce rapport sont **pré-critique**. La colonne `Crit` vaut5
sur les rows core demandées et10 sur leurs contreparties BKV. Dans la référence
D2MOO, le jet `<Crit` double tous les canaux de dégâts du hit, y compris dans
les callsites skill/missile qui invoquent ce handler; l'espérance d'un hit
éligible devient donc `moyenne_roll × (1 + Crit/100)`. Les espérances données
ci-dessous sont des estimations sémantiques 1.10f, pas la preuve directe du
handler 92777; les branches non-critique et critique restent indiquées pour ne
jamais appeler un simple intervalle A1 « dégâts finaux ».

Toutes les rows de boss calculées ici — Act bosses, trio Uber, DClone,
Countess, Summoner et Nihlathak — ont `NoRatio` vide, donc0, dans le core/SP+
et BKVince. `MonLvl` participe donc aux projections; toute exception aurait été
signalée. Les cellules et graphes sont des preuves data byte-exactes, tandis
que HP/AC/dégâts finaux, bonus UMod, chance de toucher et probabilités TC sont
des **valeurs dérivées sous le contrat D2MOO/évaluateur** : elles restent une
inférence forte de l'exécution moderne tant que le dernier handler PD2/D2R
n'est pas observé.

### 3.2 Chance de toucher témoin

Pour comparer la défense des Ubers, le témoin emploie la formule classique :

```text
factor = floor(100 × AR / (AR + défense))
CTH_pre = floor(2 × niveau_attaquant × factor /
                (niveau_attaquant + niveau_cible))
```

Ces **deux troncatures entières** précèdent les bornes du profil actif. Dans BKVince, le patch
`hit-chance-bounds` rend les bornes 0–100 %, et non 5–95 %. Le tableau de
défense n'inclut ni block, ni esquive, ni ITD, ni réduction d'AR, ni états.

### 3.3 Treasure Classes

Une égalité de « pool » exige la résolution du graphe complet : TC racine,
`Picks`, `NoDrop`, poids `Prob`, TCs imbriquées, qualité, limite de six drops,
quest/non-quest et drops forcés. Une TC parent de même nom ou un terminal
`Act X Equip` commun n'est pas une preuve de rendement identique.

Les probabilités de ce rapport sont p1, sans Magic Find, et séparent les
essences. Les TCs PD2 legacy ne sont jamais copiées positionnellement dans les
colonnes D2R `TreasureClass*`.

### 3.4 Durées, rayons et deltas

- 25 frames = 1 seconde;
- les rayons sont conservés dans l'unité de leur handler; une valeur de missile
  n'est pas appelée « tiles » sans preuve de conversion;
- `delta absolu = PD2 - BKVince`;
- `delta % = (PD2 / BKVince - 1) × 100`;
- si la baseline est absente ou nulle, le pourcentage est `n/a`, jamais infini;
- les multiplicateurs finaux sont comparés comme multiplicateurs :
  `150 % -> 200 %` vaut `+50 points`, soit `+33,33 %` de dégâts projetés si
  les deux moteurs consomment ces colonnes selon la sémantique héritée.

## 4. Format normalisé des fiches

Chaque fiche A à M ci-dessous contient les dix-sept champs obligatoires. Quand
une fiche couvre plusieurs phrases du wiki, le champ 1 les cite séparément et
les champs 3 à 10 rendent un verdict par sous-affirmation. Le libellé
`Inconnu` est un résultat d'audit valide : il interdit précisément de convertir
une absence TXT en pseudo-preuve de hardcode.

| Nº | Champ obligatoire |
|---:|---|
| 1 | affirmation exacte du wiki |
| 2 | page, oldid, section et date |
| 3 | preuve PD2 : fichier, ligne/row key, colonne, valeur |
| 4 | preuve SP+ séparée |
| 5 | état BKVince : fichier, row key, colonne, valeur |
| 6 | valeur effective PD2 |
| 7 | valeur effective BKVince |
| 8 | delta absolu |
| 9 | delta pourcentage |
| 10 | verdict fonctionnel et niveau de preuve |
| 11 | route technique |
| 12 | faisabilité D2R 3.2.92777 |
| 13 | dépendances |
| 14 | risques/effets secondaires |
| 15 | recommandation |
| 16 | fichiers BKVince éventuellement modifiés lors d'un futur mandat |
| 17 | tests requis lors d'un futur mandat |

## A. Aura Enchanted et Holy Shock

| Nº | Fiche d'audit |
|---:|---|
| 1 | **Affirmations wiki :** « Monsters can now spawn with Holy Shock » et l'aura reste « sub level 20 ». La même section donne `mlvl / 7` comme règle de niveau. |
| 2 | [`Monsters` oldid 23935, `Regular Monsters` et `Monster Aura Stats`](https://wiki.projectdiablo2.com/w/index.php?title=Monsters&oldid=23935#Regular_Monsters), 20 juillet 2026. L'introduction est aussi documentée dans [`Patch:Season 3` oldid 17687, `GENERAL CHANGES`](https://wiki.projectdiablo2.com/w/index.php?title=Patch%3ASeason_3&oldid=17687). |
| 3 | **Données core S13 :** `MonUMod.txt`, ligne 32, clé `aura`, `id=30`, `enabled=1`, `champion` vide, `upick/upick (N)/upick (H)=6/6/6`; `MonStats.noAura=1` seulement pour `boneprison1..4` lignes342–345, `bonewall`346 et `tcextradrop`738. **Code PD2 prouvé :** `ProjectDiablo.dll` RVA `0x2C62B0` dispatch le mod30 et le vecteur construit à RVA `0x126C80`; pool `[472,473,474,416,476,477,478,479]` = Mon Conviction, Mon Fanaticism, Mon Holy Shock, Mon Holy Freeze, Mon Holy Fire, Mon Might, Mon Concentration, Mon Vigor. Niveau=`clamp(floor(mlvl/7),1,13)`. |
| 4 | **SP+ :** mêmes cellules `MonUMod`, `MonStats` et `skills` pour ce sujet; aucune carte, AnimData ou routine du sélecteur n'est fournie. Il ne constitue donc pas une preuve native supplémentaire. |
| 5 | **BKVince :** `monumod.txt`, ligne 32, `aura`, `id=30`, `enabled=1`, `champion` vide, `upick=6/6/12`; le mod a donc deux fois le poids Hell du core. `monstats.txt.noAura=1` sur les cinq constructions osseuses, mais pas de `tcextradrop`. Dans `skills.txt`, le header est `*Id`, donc commentaire/non consommé : ligne120 `Holy Shock` porte `*Id=118`, ligne124 `Fanaticism` `*Id=122`, et ligne371 `MonHolyShock` répète `*Id=122` avec les calcs de Fanaticism. Cela prouve une row monster-only dupliquée/corrompue sémantiquement, **pas** une collision d'ID runtime; l'identité effective dépend de l'ordinal/position compilée à prouver. |
| 6 | **Valeur effective PD2 :** pool exact de huit auras ci-dessus. Niveau minimal1, maximal13; aucune lecture de difficulté, aucun seuil mlvl20 et aucun filtre d'aura supplémentaire dans le dispatcher. Uber Mephisto hcIdx704 est un cas spécial Conviction123 niveau20; Lord de Seis index SuperUnique37 reçoit Fanaticism473. Les uniques tirant le mod30, superuniques auxquels il est fixé et cas natifs sont candidats; les champions sont exclus par `champion` vide. L'éligibilité amont exhaustive des catégories reste la seule inconnue native. |
| 7 | **Valeur effective BKVince :** Aura Enchanted existe et est plus fréquemment tiré en Hell, mais le contenu exact de son pool D2R 3.2 reste natif. La row `MonHolyShock` sémantiquement fausse empêche de traiter le nom ou `*Id` comme identité runtime saine. |
| 8 | Niveau PD2 exact1–13; la différence au « strictement 19 max » attendu est `13-19=-6`. Poids Hell BKV/core : `12-6=+6`; pool BKV inconnu. |
| 9 | Le vrai cap PD2 est 31,58 % inférieur à19. Poids Hell BKV/core : +100 %. |
| 10 | **Vrai, mais la formulation est imprécise — prouvé dans le code/plugin PD2.** Holy Shock est dans le pool; « sub level20 » signifie en S13 **cap13**, non max19. Il peut aussi être attribué à un monstre de niveau inférieur à20. L'éligibilité amont complète reste non prouvée. |
| 11 | **Route : hybride.** Poids et exclusions sont softcode; choix d'aura, niveau et éventuel cap relèvent d'un sélecteur natif/hardcode-backed. |
| 12 | D2R 3.2.92777 peut porter poids/exclusions par TXT. Reproduire le pool/formule exige d'identifier puis adapter son sélecteur natif; aucune colonne TXT prouvée ne permet l'insertion. Les ordinals PD2 ne sont pas transposables et le header BKV `*Id` n'est pas une clé runtime. |
| 13 | Corriger ou réserver une row monster-only Holy Shock; prouver son ordinal/position compilée et le mapping consommé par le moteur; classifier uniques/superuniques; fermer le sélecteur92777 et ses exclusions de zone éventuelles. |
| 14 | Modifier le mauvais skill peut altérer Fanaticism joueur/monstre; un cap mal reproduit crée des auras ≥20; modifier `upick (H)` change toutes les auras, pas Holy Shock seulement; fixed mods et random mods peuvent se cumuler différemment. |
| 15 | **Reporter**, puis **adapter** seulement après preuve du sélecteur. Ne jamais « ajouter Holy Shock à un champ » sans consommateur démontré. |
| 16 | Futur possible : `monumod.txt`, `monstats.txt`, éventuellement une nouvelle ligne propre dans `skills.txt`; si le pool est natif, configuration/owner natif encore à décider. Aucun de ces fichiers n'est modifié ici. |
| 17 | Histogramme de 1 000+ spawns uniques par difficulté et zones, identification de l'aura affichée et de son niveau, seuils mlvl 19/20/132/133/140, exclusions `noAura`, champions, superuniques à mod fixe, sauvegarde/reload et host/joiner. |

### Conclusion de borne

Le code S13 tranche : division entière par7, plancher1 et plafond13. Le niveau
atteint13 dès `mlvl=91`, puis ne monte plus. Il n'existe ni test de difficulté,
ni garde exigeant `mlvl>=20`. Le pool historique D2MOO diffère, ce qui confirme
qu'il serait dangereux de transposer l'ancien sélecteur ou les IDs PD2 dans
D2R 3.2.

## B. Stygian Dolls et explosion à la mort

### Famille exacte

| Source | Lignes héritant de `BaseId=bonefetish1` | `deathDmg` |
|---|---|---|
| PD2 core S13 | `bonefetish1..5` lignes214–218; `bonefetish6..7`693–694; `bonefetish4Rat`977; `bonefetish4Rat2`985; `bonefetish7dungeon`1053; `KyovoshadBossMinion`1196. Tous portent `MonProp=stygian`, sauf `bonefetish7dungeon` qui porte `stygianDungeon`. | vide sur les onze |
| SP+ | mêmes onze lignes et mêmes cellules | vide |
| BKVince | `bonefetish1..7` lignes 214–218, 693–694 | vide |
| BKVince custom | `bonefetish8`, ligne 789, `BaseId=bonefetish1` | **1** |

| Nº | Fiche d'audit |
|---:|---|
| 1 | **Affirmations wiki :** petit délai avant explosion, rayon réduit, dégâts réduits. |
| 2 | [`Monsters` oldid 23935, `Regular Monsters`](https://wiki.projectdiablo2.com/w/index.php?title=Monsters&oldid=23935#Regular_Monsters), 20 juillet 2026. [`Patch:Season 1` oldid 17689](https://wiki.projectdiablo2.com/w/index.php?title=Patch%3ASeason_1&oldid=17689) documente `radius 5 -> 4`, un bug ayant divisé les dégâts par deux puis son correctif. [`Season 3` oldid 17687](https://wiki.projectdiablo2.com/w/index.php?title=Patch%3ASeason_3&oldid=17687) documente ensuite `+20 %` de dégâts : « reduced damage » est donc historiquement ambigu. |
| 3 | **Core S13 :** onze rows `BaseId=bonefetish1`, `AI=Fetish`, `deathDmg` vide; dix portent `MonProp=stygian` et `bonefetish7dungeon` porte `stygianDungeon`. `MonProp.txt` lignes29/52 : N/NM/H `prop1=death-skill`, `chance1=100`, `par1=363`, `min1=100`, `max1=1/2/10`. `Properties.txt` ligne263 route `death-skill` vers `item_skillondeath`; `ItemStatCost` ligne199 event `killed`, func30. `Skills` ligne365 `DollMeteor`, srvdofunc169, rayon `ln12=4`, physique18–30/54–96/318–540 aux niveaux1/2/10. `Missiles` ligne690 `dollmeteorcenter`, `pSrvDo=1`, `pSrvHit=14`, `Range=25`, `AlwaysExplode=1`. |
| 4 | **SP+ :** cellules identiques au core; aucune animation ni DLL ProjectDiablo. Le délai, le rayon et la formule ne peuvent pas être attribués au core depuis SP+ seul. |
| 5 | **BKVince :** les sept Dolls classiques ont aussi `deathDmg` vide : leur explosion vanilla est déjà neutralisée. `bonefetish8` conserve `deathDmg=1`. `difficultylevels.txt` vaut `MonsterCEDamagePercent=20/20/15`. |
| 6 | **PD2 configuré :** proc100 % à la mort, missile center `Range=25`, rayon **4**, dégâts physiques exacts N18–30/NM54–96/H318–540. Sous la sémantique D2MOO des funcs30/169/1/14, cela donne un impact serveur à **25 frames≈1 s**; l'exécution end-to-end de ces ordinals n'est pas directement prouvée dans le binaire PD2. Aucun terme HP, vie de base, mlvl ou players n'apparaît dans la formule data; la difficulté choisit seulement le niveau1/2/10. `CltParam1=59` et `dollmeteorexplode` sont visuels. |
| 7 | **BKVince configuré :** aucune route d'explosion data pour `bonefetish1..7`; `bonefetish8` porte `deathDmg=1`. Sous la sémantique historique, ce flag déclenche lors du mode de mort, crée `MonsterCorpseExplode`, prend le **maximum HP de base calculé**, applique `MonsterCEDamagePercent`, fixe le minimum à60 % du maximum, inflige du physique et emploie un rayon5. Cette effectivité doit encore être validée sur92777. |
| 8 | Classiques BKV→PD2 configuré : délai `aucun event→Range25`, projeté à25f sous sémantique héritée; rayon `0→4`; dégâts `0→18–30/54–96/318–540`. Face au vanilla historique : rayon `4-5=-1`. `bonefetish8` reste une route BKV immédiate distincte sans contrepartie row PD2. |
| 9 | Baseline classique BKV zéro : pourcentage non défini. Rayon PD2 vs vanilla historique : -20 %. Le delta de dégâts vs vanilla ne peut pas être réduit à un pourcentage : PD2 remplace une formule HP par des dégâts fixes. |
| 10 | **Vrai mais incomplet** : rayon et dégâts sont prouvés dans les données; le délai25f est documenté par le wiki et fortement inféré de `Range=25` sous les handlers hérités, sans preuve binaire PD2 end-to-end. « Dégâts réduits » reste **trompeur sans baseline** et S3 les a ensuite augmentés de20 %. |
| 11 | **Route : hybride softcode + handlers existants à confirmer.** MonProp→property→stat event→skill→missile porte la configuration; les funcs natives30/169/1/14 ont la sémantique requise dans D2MOO, mais leurs ordinals PD2/D2R doivent être prouvés. `deathDmg` doit rester vide. |
| 12 | D2R 3.2.92777 est potentiellement softcodable si ses eventfunc30 et srv/missile funcs compatibles sont prouvées; les ordinals ne sont pas transposables (`skill363` BKV est `ShowItems`). Sans compatibilité, adaptation native. |
| 13 | Classification exhaustive `BaseId`; preuve du handler 92777; AnimData réellement chargé; décision pour `bonefetish8`; propriété du corps, kill credit, player count et interaction avec Revive/MeleeSplash. |
| 14 | Double explosion si `deathDmg=1` coexiste; ordinal363 erroné; explosion après suppression; revive/re-kill, ownership, kill credit et consommation du cadavre non prouvés; VFX client dissocié du hit serveur. |
| 15 | **Adapter/reporter.** La configuration PD2 est démontrée et projette un danger absent des Dolls classiques BKV; décision gameplay et preuve des handlers 92777 requises. Traiter `bonefetish8` séparément. |
| 16 | Futur possible : `monstats.txt`, `skills.txt`, `missiles.txt`, `states.txt`, AnimData/COF si un event prouvé les consomme, ou propriétaire natif encore non décidé. |
| 17 | N/NM/H, p1/p8, onze rows (base + dix descendants), mort melee/ranged/spell/DoT/thorns/splash, délai frame par frame, rayon en subtiles, min/max sur ≥500 morts, corps consommé/non consommé, Revive, Redemption, Find Item, host/joiner, kill/XP/loot credit et absence de double event. |

### Configuration exacte et résultat projeté

La formule **configurée** PD2 S13 est entièrement reconstruite : dégâts
physiques fixes du skill aux niveaux1/2/10, sans terme HP ou player count dans
les calcs data. `MonsterCEDamagePercent=50/35/20` ne participe pas à cette
chaîne; l'utiliser comme formule PD2 actuelle serait une fausse preuve. La
sémantique héritée fait de la Doll morte la source, sans réattribuer le tueur,
mais ownership, kill credit et consommation du cadavre restent à confirmer
dans les handlers PD2/D2R modernes.

## C. Taxonomie Prime Evil

### C.1 Résolution des termes

Le wiki oldid 23935 énumère « all Act bosses, classic bosses, classic ubers,
map bosses, dungeon monsters and bosses, and new ubers and their minions ».
Une recherche exhaustive du wiki ne fournit aucune définition canonique de
**classic bosses** ni de **classic ubers**. Ces deux labels restent donc
**inconnus**; le rapport recommande de les remplacer par des ensembles nommés
sans ambiguïté (`ActBoss`, trio `Uber`, trio `MiniUber`, `DiabloClone`, etc.).
Ils ne sont jamais interprétés silencieusement depuis le lore.

Comptages exacts : core S13 `boss=1`: **115**, `primeevil=1`: **149**,
`SuperUniques`: **68**; SP+: 115/149/68; BKVince: 31/15/70. Le flag
`primeevil` inclut des boss, des minions d'encounter et des monstres de
donjon : il n'est synonyme ni de `boss`, ni de `superunique`, ni d'Act boss.

Les 15 rows BKV exactes sont : `andariel` 156/158, `duriel` 211/213,
`mephisto` 242/244, `diablo` 243/245, `diabloclone` 333/335, `baalcrab`
544/547, `baalclone` 570/573, `ubermephisto` 704/707, `uberdiablo` 705/708,
`uberandariel` 707/710, `uberduriel` 708/711, `uberbaal` 709/712,
`colossal1` 745/748, `colossal2` 746/749 et `colossal3` 747/750. La notation
est `hcIdx/ligne`.

### C.2 Tableau canonique des groupes demandés

`ID` désigne `hcIdx`; la ligne compte le header comme ligne 1.

| monstats key | ID PD2 | ID BKVince | nom/source | primeevil PD2 | boss | superunique | groupe souhaité | variantes/commentaire |
|---|---:|---:|---|---:|---:|---|---|---|
| `andariel` | 156 | 156 | Andariel, MonStats 158 | 0 | 1 | non | ActBoss | `uberandariel` 707 et `andarielMap` 1120 sont distincts et primeevil=1 |
| `duriel` | 211 | 211 | Duriel, 213 | 0 | 1 | non | ActBoss | `uberduriel` 708 et `durielMap` 1119 sont distincts et primeevil=1 |
| `mephisto` | 242 | 242 | Mephisto, 244 | 1 | 1 | non | ActBoss | `ubermephisto` 704, `mephistoMap` 1118 |
| `diablo` | 243 | 243 | Diablo, 245 | 1 | 1 | non | ActBoss | legacy `diabloclone` 333 prime=0 core; `uberdiablo` 705; `uberdiablonew` 789; `diabloMap` 1116 |
| `baalcrab` | 544 | 544 | Baal, 547 | 1 | 1 | non | ActBoss | `baalcrabstairs` 559 et `baalclone` 570 sont prime=0 core; `uberbaal` 709; `baalcrabMap` 1117 |
| `baalclone` | 570 | 570 | Baal Crab Clone, 573 | 0 | 1 | non | aucun groupe demandé démontré | **BKV `primeevil=1`**, contrairement au core; variante/clône à ne pas promouvoir silencieusement en ActBoss |
| `ubermephisto` | 704 | 704 | Uber Mephisto, 707 | 1 | 1 | non | Uber | même ID BKV |
| `uberdiablo` | 705 | 705 | Uber Diablo, 708 | 1 | 1 | non | Uber | même ID BKV |
| `uberbaal` | 709 | 709 | Uber Baal, 712 | 1 | 1 | non | Uber | même ID BKV |
| `uberizual` | 706 | 706 | Uber Izual, 709 | 1 | 1 | non | MiniUber | BKV `primeevil=0` |
| `uberandariel` | 707 | 707 | Lilith, 710 | 1 | 1 | non | MiniUber | BKV `primeevil=1` |
| `uberduriel` | 708 | 708 | Uber Duriel, 711 | 1 | 1 | non | MiniUber | BKV `primeevil=1` |
| `uberancientbarb1` | 989 | 745 | Uber Ancient 1, 992 | 1 | 1 | non | ColossalAncient | BKV `colossal1`, ligne 748 |
| `uberancientbarb2` | 990 | 746 | Uber Ancient 2, 993 | 1 | 1 | non | ColossalAncient | BKV `colossal2`, ligne 749 |
| `uberancientbarb3` | 991 | 747 | Uber Ancient 3, 994 | 1 | 1 | non | ColossalAncient | BKV `colossal3`, ligne 750 |
| `uberdiablonew` | 789 | 333 | DClone courant PD2, 792 | 1 | 1 | non | DiabloClone, ApexBoss | contrepartie BKV `diabloclone`, ligne 335; ne pas comparer à la row legacy PD2 |
| `rathmaBone` | 933 | — | Rathma, 936 | 1 | 1 | non | ApexBoss | forme principale bone |
| `rathmaPoison` | 934 | — | Mendeln, 937 | 1 | 1 | non | ApexBoss | forme principale poison |
| `Lucion` | 1112 | — | Lucion, 1115 | 1 | 1 | non | ApexBoss | contrôleurs/spawns 1113–1115 et 1140–1141 ne sont pas primeevil |

`superunique` est ici « présent comme entrée de `SuperUniques.txt` », pas
simplement « personnage nommé ». Aucun des rows principaux ci-dessus n'y est
référencé comme un SuperUnique classique; leurs spawns boss suivent leurs
propres placements/quests.

### C.3 Variantes exactes d'encounters Apex

- DClone : `ubertrappedsoul1..5`, IDs 790–794, lignes 793–797,
  `boss=1, primeevil=1`; `dcloneskele`, `dcloneskelearcher`, quatre
  `dcloneskmage_*` et `dclonebloodlord`, IDs 886–892, lignes 889–895,
  `boss=0, primeevil=1`.
- Rathma : `rathmaBoneClone`, `rathmaPoisonClone`, `rathmaVoidGolem`,
  `rathmaBloodGolem`, `rathmaTotem`, IDs 935–939, lignes 938–942; tous
  `primeevil=1`, le Void Golem seul a `boss=0`.
- Lucion : `LucionControl`, `LucionSpawn`, `LucionSpawnRanged`,
  `LucionSpawnTank`, `LucionSpawner`, IDs 1113–1115 et 1140–1141, lignes
  1116–1118 et 1143–1144, tous `boss=0, primeevil=0`.

### C.4 Fiche normalisée

| Nº | Fiche d'audit |
|---:|---|
| 1 | Le wiki affirme que le tag couvre les groupes cités plus haut; le mandat souhaite ActBoss, classic bosses/Ubers, tous Ubers/mini-Ubers, Colossal Ancients et DClone. |
| 2 | [`Monsters` oldid 23935, `Prime Evils`](https://wiki.projectdiablo2.com/w/index.php?title=Monsters&oldid=23935#Prime_Evils), 20 juillet 2026. |
| 3 | `MonStats.txt`, 149 rows `primeevil=1`, 115 rows `boss=1`; identités et lignes exactes dans les tableaux et l'annexe C.5. `SuperUniques.txt` contient 68 rows et constitue une classification séparée. |
| 4 | SP+ porte les mêmes 149 flags; cela confirme seulement le snapshot, indépendamment de l'extraction core qui fournit ici la preuve PD2. |
| 5 | BKVince `monstats.txt` ne porte que 15 flags : les cinq Act bosses, DClone, Baal Clone, cinq Ubers/mini-Ubers (trio Uber + Lilith/Duriel, sans Izual), et `colossal1..3`. |
| 6 | Valeur effective PD2 : `primeevil` est une appartenance à 149 rows, largement supérieure au groupe de boss demandé. |
| 7 | Valeur effective BKVince : 15 rows; Andariel/Duriel sont ajoutés par rapport au core, Uber Izual manque, et les familles map/dungeon/Apex PD2 n'existent généralement pas dans BKV. |
| 8 | `149 - 15 = +134` rows core; cette différence n'est pas une liste de merge, car beaucoup d'IDs n'existent pas dans BKVince. |
| 9 | Le core compte `893,33 %` de rows primeevil de plus que BKV (`134/15`). Ce pourcentage décrit les tables, pas la difficulté. |
| 10 | **Vrai mais incomplet** pour l'ampleur du tag; **non prouvable** pour les labels classic; **prouvé dans les données PD2** pour chaque flag. |
| 11 | **Softcode pour `primeevil`; sidecar de classification pour les groupes sans flag fiable.** |
| 12 | D2R 3.2 sait charger `boss` et `primeevil`; aucun champ natif existant ne distingue à lui seul ActBoss/Apex/Rift/Uber/MiniUber/DClone. |
| 13 | Définir la source canonique de classification avant toute règle de combat; mapper les IDs PD2/BKV; traiter les variantes custom et absentes. |
| 14 | Étendre `primeevil` en bloc appliquerait des règles de combat à des minions et donjons; utiliser `boss` seul inclurait des rows qui ne sont ni ActBoss ni Uber; une allowlist DLL dériverait avec les tables. |
| 15 | **Adapter :** garder `primeevil` comme axe `PrimeEvilRules`, mais créer ultérieurement une donnée de classification indépendante pour les groupes d'identité. |
| 16 | Futur possible : `monstats.txt` uniquement pour les flags réellement voulus; nouveau sidecar de classification si approuvé. Aucune DLL ne doit embarquer une liste de noms. |
| 17 | Validation de chaque ID au chargement, doublons/aliases, spawns quest/map, transformations/clones, rows absentes, hot reload, solo/host/joiner, consommateurs de classification et fail-closed sur clé inconnue. |

### C.5 Inventaire exhaustif des 149 rows `primeevil=1` du core S13

Cette annexe est incluse dans le rapport pour que la taxonomie reste autonome,
même si le cache d'analyse disparaît. Les cellules vides de `boss` valent0.
Le TSV source a 149 lignes × 7 colonnes, CRLF, SHA-256
`D939E8336AC98C712561453CF5C0CC2F025C4B1DA461BDDF40E96B1D0B93FE87`.

```tsv
Id	line	hcIdx	BaseId	boss	primeevil	NameStr
mephisto	244	242	mephisto	1	1	Mephisto
diablo	245	243	diablo	1	1	Diablo
baalcrab	547	544	baalcrab	1	1	Baal Crab
ubermephisto	707	704	mephisto	1	1	UberMephisto
uberdiablo	708	705	diablo	1	1	UberDiablo
uberizual	709	706	izual	1	1	UberIzual
uberandariel	710	707	andariel	1	1	Lilith
uberduriel	711	708	duriel	1	1	UberDuriel
uberbaal	712	709	baalcrab	1	1	Baal Crab
willowispboss	746	743	willowispboss	1	1	Wisp Boss
willowispminion	747	744	willowispminion	1	1	Wisp Minion1
megademonboss	749	746	megademonboss	1	1	Megademon Boss
fingermageboss	750	747	fingermageboss	1	1	Fingermage Boss
cantorboss	751	748	cantorboss	1	1	Cantor Boss
cantorbossbear	752	749	cantorbossbear	1	1	Bear
unravelerboss	753	750	unravelerboss	1	1	Unraveler Boss
griswoldmap	756	753	griswoldmap	1	1	StrAvunaos
griswoldgolem	757	754	griswoldgolem	1	1	BloodGolem
baalminionboss	758	755	baalminionboss	1	1	StrBelial
uberdiablonew	792	789	uberdiablonew	1	1	uberdiablonew
ubertrappedsoul1	793	790	trappedsoul1	1	1	TrappedSoul
ubertrappedsoul2	794	791	trappedsoul1	1	1	TrappedSoul
ubertrappedsoul3	795	792	trappedsoul1	1	1	TrappedSoul
ubertrappedsoul4	796	793	trappedsoul1	1	1	TrappedSoul
ubertrappedsoul5	797	794	trappedsoul1	1	1	TrappedSoul
willowispminion2	802	799	willowispminion2	1	1	Wisp Minion2
ArcaneBoss	803	800	succubuswitch1	1	1	ArcaneBoss
BastionBoss	812	809	skmage_cold1	1	1	BastionBoss
CowBoss	829	826	bloodlord1	1	1	CowBoss
reanimatedMaus	848	845	reanimatedhorde1		1	UnholyCorpse
pantherwomanMaus	849	846	pantherwoman1		1	Huntress
slingerMaus	850	847	slinger1		1	SpearCat
willowisp1Maus	851	848	willowisp1		1	Gloam
bloodlordMaus	852	849	bloodlord1		1	Blood Lord5
overseerMaus	853	850	overseer1		1	BloodBoss
succubusMaus	854	851	succubus1		1	Succubusexp
bigheadMaus	855	852	bighead1		1	Misshapen
IceBoss	864	861	overseer1	1	1	IceBoss
TombBoss	873	870	clawviper1	1	1	TombBoss
torajanBoss	882	879	maggotqueen5	1	1	torajanBoss
torajanBossPoisonEgg	883	880	maggotegg1	1	1	WorldKillerEgg
torajanBossMaggot	884	881	maggotbaby1	1	1	WorldKillerYoung
ThroneBoss	885	882	vampire1	1	1	ThroneBoss
SiegeBoss	886	883	hephasto	1	1	SiegeBoss
SewerBoss	887	884	councilmember1	1	1	SewerBoss
dcloneskele	889	886	skeleton1		1	BoneWarrior
dcloneskelearcher	890	887	sk_archer1		1	SkeletonArcher
dcloneskmage_fire	891	888	skmage_fire1		1	BurningDeadMage
dcloneskmage_ltng	892	889	skmage_ltng1		1	HorrorMage
dcloneskmage_cold	893	890	skmage_cold1		1	BoneMage
dcloneskmage_pois	894	891	skmage_pois1		1	HorrorMage
dclonebloodlord	895	892	bloodlord1		1	Blood Lord5
act2hireTraitorBoss	896	893	act2hireTraitorBoss	1	1	act2hireTraitorBoss
archerBoss	897	894	archerBoss	1	1	PandemoniumBoss
spiderboss	918	915	regurgitator1	1	1	spiderboss
siegebeast3Maus	919	916	siegebeast1		1	BloodBringer
vampire2Maus	920	917	vampire1		1	NightLord
summonerMap	922	919	summoner	1	1	SummonerMap
zombieNihlMinion	926	923	zombie1		1	HungryDead
chargerNihlMinion	927	924	reanimatedhorde1		1	DefiledWarrior
vampNihlMinion	928	925	vampire1		1	GhoulLord
skeleNihlMinion	929	926	skeleton1		1	Horror
archerNihlMinion	930	927	sk_archer1		1	HorrorArcher
mFireNihlMinion	931	928	skmage_fire1		1	HorrorMage
mLtngNihlMinion	932	929	skmage_ltng1		1	HorrorMage
mColdNihlMinion	933	930	skmage_cold1		1	HorrorMage
mPoisNihlMinion	934	931	skmage_pois1		1	HorrorMage
rathmaBone	936	933	nihlathak	1	1	Rathma
rathmaPoison	937	934	nihlathak	1	1	Mendeln
rathmaBoneClone	938	935	nihlathak	1	1	Rathma
rathmaPoisonClone	939	936	nihlathak	1	1	Mendeln
rathmaVoidGolem	940	937	bloodgolem		1	RathmaVoidGolem
rathmaBloodGolem	941	938	bloodgolem	1	1	RathmaBloodGolem
rathmaTotem	942	939	skmage_fire1	1	1	RathmaTotem
CanyonBoss	966	963	frozenhorror1	1	1	CanyonBoss
MarketBoss	967	964	blunderbore1	1	1	MarketBoss
fallenMarketBoss	968	965	fallen1	1	1	DevilkinSlave
spireFire	969	966	lightningspire	1	1	Mendeln
voidKnightPoison	971	968	doomknight3		1	VoidKnight
voidKnightFire	972	969	doomknight3		1	VoidKnight
overseerRat	974	971	overseer1		1	HellWhip
unraveler3Rat	975	972	unraveler1		1	Unraveler
skeleton4Rat	976	973	skeleton1		1	BurningDead
bonefetish4Rat	977	974	bonefetish1		1	Undead SoulKiller
mummy5Rat	978	975	mummy1		1	Cadaver
willowisp1Rat	979	976	willowisp1		1	Gloam
skmage_fire3Rat	980	977	skmage_fire1		1	BurningDeadMage
overseerRat2	982	979	overseer1		1	HellWhip
unraveler3Rat2	983	980	unraveler1		1	Unraveler
skeleton4Rat2	984	981	skeleton1		1	BurningDead
bonefetish4Rat2	985	982	bonefetish1		1	Undead SoulKiller
mummy5Rat2	986	983	mummy1		1	Cadaver
willowisp1Rat2	987	984	willowisp1		1	Gloam
skmage_fire3Rat2	988	985	skmage_fire1		1	BurningDeadMage
uberancientbarb1	992	989	ancientbarbboss1	1	1	UberAncient1
uberancientbarb2	993	990	ancientbarbboss2	1	1	UberAncient2
uberancientbarb3	994	991	ancientbarbboss3	1	1	UberAncient3
westmarchMapBoss	999	996	brute2	1	1	WestmarchBoss
doomknight3LibraryBoss	1000	997	doomknight3	1	1	LibraryBoss
leoricMapBoss	1001	998	skeleton1	1	1	LeoricBoss
siegebeastMapBoss	1003	1000	megademon1	1	1	SiegebeastMapBoss
siegebeastMapBossFallen	1004	1001	fallen1	1	1	SiegebeastMapBossFallen
AshenBoss	1028	1025	fingermage1	1	1	AshenBoss
lernaeanhydra2	1029	1026	tentaclehead2	1	1	CisternBoss
DungeonTest3	1030	1027	radament	1	1	Radament
lernaeanhydra1	1047	1044	tentaclehead1	1	1	CisternBoss
blunderbore1dungeon	1051	1048	blunderbore1		1	Blunderbore
batdemon3dungeon	1052	1049	batdemon1		1	ShriekingTerror
bonefetish7dungeon	1053	1050	bonefetish1		1	Undead SoulKiller
putriddefiler1dungeon	1054	1051	putriddefiler1		1	Putrid Defiler1
ZharTheMad	1061	1058	summoner	1	1	ZharTheMad
WarlordOfBlood	1063	1060	doomknight3	1	1	WarlordOfBlood
ZharMiniBossBigHead	1064	1061	bighead1	1	1	ZharMiniBossBigHead
ZharMiniBossBaboon	1065	1062	baboon1	1	1	ZharMiniBossBaboon
ZharMiniBossCantor	1066	1063	cantor1	1	1	ZharMiniBossCantor
GuardianOfFate	1067	1064	regurgitator1	1	1	GuardianOfFate
WarlordMiniBossShaman	1095	1092	fallenshaman1	1	1	DamnedCavesMiniBoss
WarlordMiniBossDefiler	1096	1093	putriddefiler1	1	1	WarlordOfBloodMiniBoss
Iskatu	1097	1094	iskatu	1	1	Iskatu
Rakanoth	1099	1096	rakanoth	1	1	Rakanoth
KanemithBoss	1106	1103	snowyeti1	1	1	KanemithBoss
RadamentBoss	1107	1104	radamentboss	1	1	RadamentBoss
SharpToothBoss	1108	1105	overseer1	1	1	SharpToothBoss
voidKnightCold	1109	1106	doomknight3		1	VoidKnight
voidKnightPhys	1110	1107	doomknight3		1	VoidKnight
SharpToothMinion	1111	1108	minion1		1	SharpToothMinion
Lucion	1115	1112	lucion	1	1	Lucion
diabloMap	1119	1116	diabloMap	1	1	Diablo
baalcrabMap	1120	1117	baalcrabMap	1	1	Baal Crab
mephistoMap	1121	1118	mephistoMap	1	1	Mephisto
durielMap	1122	1119	durielMap	1	1	Duriel
andarielMap	1123	1120	andarielMap	1	1	Andariel
DemonRoadBoss	1124	1121	deathmauler1	1	1	DemonRoadBoss
SkovosBoss	1125	1122	cantor1	1	1	SkovosBoss
ImperialPalaceMiniBoss	1151	1148	fallenshaman1	1	1	Kamyr
TortureHallsBoss	1152	1149	thornhulk1	1	1	TortureHallsBoss
ImperialPalaceBossMinion	1167	1164	councilmember1	1	1	ImperialAdvisor
ImperialPalaceBoss	1169	1166	councilmember1	1	1	ImperialAssassin
UrehRanger	1175	1172	sk_archer1	1	1	HorrorArcher
CityofUrehMiniBoss	1178	1175	CityofUrehMiniBoss	1	1	UrehMiniBoss
InvaderAmazon	1186	1183	InvaderAmazon	1	1	InvaderAmazon
InvaderAssassin	1187	1184	InvaderAssassin	1	1	InvaderAssassin
InvaderBarbarian	1188	1185	InvaderBarbarian	1	1	InvaderBarbarian
InvaderDruid	1189	1186	InvaderDruid	1	1	InvaderDruid
InvaderNecromancer	1190	1187	InvaderNecromancer	1	1	InvaderNecromancer
InvaderPaladin	1191	1188	InvaderPaladin	1	1	InvaderPaladin
InvaderSorceress	1192	1189	InvaderSorceress	1	1	InvaderSorceress
KyovoshadBoss	1195	1192	crownest1	1	1	KyovoshadBoss
NaKrulBoss	1197	1194	NaKrulBoss	1	1	NakrulBoss
```

## D. Buffs Prime Evil

### D.1 Dégâts contre mercenaires et minions

La formulation « 200% damage to minions and mercenaries » n'est pas une
description exacte du chemin S13. La routine sémantique héritée teste
l'attaquant `primeevil`, puis la nature du défenseur :

| Défenseur | condition | total PD2 projeté sous sémantique héritée | BKV `DifficultyLevels` configuré | delta projeté PD2−BKV |
|---|---|---:|---:|---:|
| mercenaire | hireling | **200 %=×2** | 150 %=×1,5 | +50 points, +33,33 % |
| monstre invoqué/réanimé | `UNITFLAG_ISREVIVE` | **400 %=×4** | 200 %=×2 | +200 points, +100 % |
| joueur | player | 100 % | 100 % | 0 |

Le 200 % mercenaire est un **multiplicateur total ×2**, pas un bonus de
`+200 %` donnant ×3. Pour les autres pets, le wiki sous-décrit le résultat :
le total est ×4. Le handler ne remonte pas un GUID d'ownership. Un golem,
revive, shadow, valkyrie ou summon standard est concerné s'il est une unité
monstre portant `ISREVIVE`. Les sentries Assassin 1.10f sont justement créées
comme pets avec ce flag et sont donc candidates au ×4 sous cette sémantique;
une trap moderne ne peut pas être exclue par son seul nom et dépend de son type
d'unité/flag. Un missile ou objet pur n'est pas lui-même un défenseur
`ISREVIVE`; un summon temporaire dépend du flag. Les coups, missiles, sorts et AoE dont le
damage record conserve le Prime Evil comme attaquant traversent le même point;
un serviteur appartenant au boss ne bénéficie pas automatiquement du bonus si
sa propre row n'est pas Prime. Le DoT dépend également de la source conservée.

La preuve du multiplicateur repose sur le chemin D2MOO corroborant le
prédicat/table S13; aucune routine de remplacement dans `ProjectDiablo.dll`
n'a été trouvée. Le niveau de preuve est donc **inférence forte, confiance
moyenne-haute**, non « prouvé byte-identique dans le code PD2 ».

### D.2 Immunité aux ralentissements

L'affirmation universelle n'est **pas démontrée** par le code directement
borné et elle rencontre un contre-exemple sous la sémantique héritée des
filtres : PD2 possède plusieurs gardes distinctes, pas un package unique
identifié :

| slow | preuve S13 | comportement Prime Evil | mécanisme |
|---|---|---|---|
| Slows Target by X% | `ProjectDiablo.dll` RVA `0x2AFA40`, wrapper EventFunc19 | bloqué | retour avant handler original |
| Decrepify skill87/state60 | gardes RVA `0x2BEF70` et `0x2BFE20` | bloqué | refus aux deux chemins skill/state |
| Holy Freeze 114/416/475 | `aurafilter=0x4A283`, bit `IGNPRIME=0x40000` | bloqué | filtre de cible, état non appliqué |
| chill par froid | `MonStats.ColdEffect` | **pas de test Prime global** | valeur propre à chaque row |
| Slow Movement17 | `aurafilter=0xC583`; le décodage hérité contient `IGNBOSS`, pas `IGNPRIME` | un prime non-boss reste éligible sous cette sémantique | contre-exemple data + sémantique héritée |
| slows custom | filtre/func propre | inconnu par défaut | aucune neutralisation générique |

Dans les trois chemins directement identifiés, PD2 refuse la cible ou
l'application; aucune preuve n'établit une valeur minimale, un nettoyage
périodique ou un simple recours à Cannot Be Frozen. Mouvement, attaque et
animation ne doivent donc être déclarés protégés que si leur mécanique aboutit
à l'un de ces chemins. Le scan n'est pas une preuve d'absence dans tous les
modules S13 : le verdict correct est « universel non prouvé », avec un
contre-exemple fort sous la sémantique héritée.

BKVince ne porte pas ce package. Le handler D2R 92777 gouverné
`D2GAME_EventFunc19_Slow`, RVA `0x5849D0`, n'appelle pas `primeevil`; il cape
l'item slow à50 pour unique/superunique/boss/hireling,75 pour la catégorie
intermédiaire et90 pour l'ordinaire. Copier `primeevil=1` ne crée donc aucune
immunité automatique.

### D.3 Dim Vision, Terror, Confuse et Attract

| skill | ID core | filtre | résultat sous sémantique héritée |
|---|---:|---:|---|
| Dim Vision | 71 | 2 | route de changement d'AI |
| Terror | 77 | 2 | route de changement d'AI |
| Confuse | 81 | 2 | route de changement d'AI |
| Attract | 86 | 2 | route de changement d'AI |

Sous la sémantique D2MOO héritée, `AIUTIL_CanUnitSwitchAi` refuse les unités
unique/superunique, les AI non interruptibles et `CanSwitchAI=false`. Aucun
RVA PD2 direct n'a été promu pour cette chaîne et le scan de
`ProjectDiablo.dll` ne suffit pas à prouver l'absence d'un autre guard. Le
résultat reste donc une **inférence forte** : les boss concernés sont
généralement protégés par leur classe/AI, tandis qu'un minion
`primeevil=1` ordinaire et switchable ne le serait pas par ce seul flag. Aucune
partie résiduelle de ces quatre skills n'est démontrée applicable lorsque le
switch AI est refusé. Verdict : le résultat peut être vrai pour les boss
nommés, mais la causalité « Prime Evil » du wiki est **non prouvée et
trompeuse comme spécification d'implantation**.

Aucun test direct de `MonStats.boss` ni de `primeevil` n'est prouvé dans ce
chemin. La protection observée/inférée vient du statut runtime
unique/superunique et/ou d'une AI non switchable; le flag `boss` ne doit donc
pas être crédité par simple corrélation.

### D.4 Sanctuary et résistance physique

Sanctuary skill119 possède l'état47 et `aurafilter=0xA783`; sa ligne softcode
porte aussi son dommage/bonus contre undead. La mécanique visée ici n'est ni
le tick de dégâts, ni le knockback, ni le bonus magique : c'est la règle native
qui traite la résistance physique comme zéro pour une cible undead.

`ProjectDiablo.dll` RVA `0x26F680` lit la résistance physique stat36, vérifie
l'aura Sanctuary sur l'attaquant, teste `primeevil`, puis le type undead. Il
force la résistance à zéro seulement si **undead && !primeevil**. L'exclusion
est donc **prouvée dans le code PD2**, et un simple changement de la ligne
Sanctuary ne reproduit pas nécessairement ce test sur D2R 3.2. Un hook du
resolver physique, ou une primitive native 92777 équivalente encore à prouver,
est nécessaire; le reste de la skill doit continuer à fonctionner.

### D.5 Fiche normalisée commune

| Nº | Fiche d'audit |
|---:|---|
| 1 | (a) 200 % de dégâts contre minions/mercs; (b) immunité à tous les slows; (c) immunité Dim Vision/Terror/Confuse/Attract; (d) Sanctuary ne supprime pas la résistance physique. |
| 2 | [`Monsters` oldid23935, `Prime Evils`](https://wiki.projectdiablo2.com/w/index.php?title=Monsters&oldid=23935#Prime_Evils), 20 juillet 2026. S1/S2 documentent le tag dégâts sans chiffre; S3 prouve Decrepify seulement; S6 oldid17684 documente Sanctuary. |
| 3 | `MonStats.primeevil`; code/RVA et filtres détaillés D.1–D.4. Le multiplicateur dégâts reste une inférence forte héritée; item slow, Decrepify et Sanctuary ont des preuves directes S13; Holy Freeze/Slow Movement reposent aussi sur le décodage hérité des filtres; les quatre AI curses n'ont aucun guard Prime directement prouvé. |
| 4 | SP+ a les mêmes flags/skills mais aucune DLL ProjectDiablo; il n'ajoute aucune preuve native. |
| 5 | BKV `DifficultyLevels` N/NM/H=`VSPlayer100/VSMerc150/VSPet200`; 15 flags prime. EventFunc19 92777 sans guard Prime. Skills/AI vanilla/BKV ne fournissent pas le package PD2. |
| 6 | PD2 : merc×2 et revive/pet×4 sous sémantique héritée; item slow/Decrep directement bloqués; Holy Freeze bloqué sous décodage de filtre; immunité universelle non prouvée; refus des AI curses par classe/AI inféré, pas causé par Prime; Sanctuary force la résistance physique à0 pour undead non-Prime et la conserve pour undead Prime. |
| 7 | BKV configure player×1/merc×1,5/pet×2 dans `DifficultyLevels`; l'effectivité exacte du consommateur 92777 reste à valider. Pas d'immunité Prime automatique prouvée; les quatre curses dépendent des filtres/AI existants; Sanctuary n'a pas l'exception PD2 démontrée. |
| 8 | Merc+0,5×; pet+2×. Les trois autres deltas sont booléens par handler, pas des pourcentages globaux. |
| 9 | Merc+33,33 %; pet+100 %. N/a pour les immunités. |
| 10 | **Dégâts : trompeur/incomplet. Slows : universel non prouvé et contre-exemple fort sous sémantique héritée. Quatre curses : causalité Prime non prouvée/trompeuse. Sanctuary : vrai, prouvé dans le code PD2.** |
| 11 | Dégâts/slow/curses/Sanctuary : **hardcode ou hardcode-backed**; filtres Holy Freeze partiellement softcode; flags/classification softcode. |
| 12 | Dégâts BKV déjà paramétrables via `DifficultyLevels`; l'identité exacte des cibles reste hardcode-backed. Les immunités nécessitent filtres et/ou hooks 92777 ciblés, jamais un seul flag magique. |
| 13 | Taxonomie stable, ownership/damage record, type/flags summons, preuve des handlers 92777, coexistence StaticField/MeleeSplash/ReviveOverhaul, politique de slows custom. |
| 14 | Pets surpunis ×4; summons opt-out oubliés; child damage mal attribué; états partiellement appliqués; rendre tous les prime rows immunes toucherait 149 rows PD2, dont des minions/donjons. |
| 15 | **Adapter** le multiplicateur après décision; **rejeter** l'énoncé « tous slows » comme spécification faute de preuve universelle; reproduire séparément les guards dont le niveau de preuve est explicite; **reporter** curses/Sanctuary jusqu'à preuve 92777. |
| 16 | Futur possible : `difficultylevels.txt`, `skills.txt`/filtres, sidecar de classification et propriétaire natif à décider; aucune modification ici. |
| 17 | Matrice attaquant boss/child, merc/revive/golem/shadow/valk/trap/temporaire, melee/missile/spell/AoE/DoT, chill/Decrep/HF/item/custom, quatre curses sur boss/superunique/prime-minion, Sanctuary undead prime/non-prime, solo/host/joiner. |

### D.6 Matrice de responsabilité

| Buff | obtenu par `primeevil` seul | autre TXT requis | map | code natif | état BKVince | faisabilité | futur propriétaire recommandé |
|---|---|---|---|---|---|---|---|
| dégâts merc/pets | non : handler doit lire le flag | `DifficultyLevels` pour multiplicateurs | non | identité cible/ownership | 150/200 configurés, effectivité à valider | élevée avec adaptation | règles de monstres/combat, à décider |
| item slow | non | non suffisant | non | oui | cap, pas immunité | moyenne après preuve | règles de statut |
| Decrepify | non | filtre possible mais non équivalent garanti | non | guards PD2 | non prouvé | moyenne | règles de skill/statut |
| Holy Freeze | non | aurafilter `IGNPRIME` | non | décodage filtre | non prouvé | élevée si bit92777 équivalent | skill/filter owner |
| chill | non | `ColdEffect` row par row | non | application froid | variable | élevée en data pour rows choisies | données monstres |
| 4 AI curses | aucun test Prime directement prouvé | filtres/AI | non | AI switch | protection de classe | ne pas imiter comme package Prime sans preuve | AI/skill owner |
| Sanctuary phys-res | non | aucun TXT seul prouvé | non | **oui** | exception absente/non prouvée | moyenne | resolver de résistance physique |

## E. Andariel en Hell

### Projections de combat p1 depuis les tables

| Source | Row/ligne | niveau | HP % / `L-HP` | vie p1 | AC % / `L-AC` | défense | A1 % | A1 pré-Crit | Crit | D/M/F/L/C/P | block |
|---|---|---:|---|---:|---|---:|---|---|---:|---|---:|
| PD2 core=SP+ | `andariel`, 158 | 85 | 1471 / 6182 | **90 937** | 110 / 1651 | 1816 | 200–240 | **192–230** | 5 % | 66/0/-50/66/66/66 | 40 |
| BKVince | `andariel`, 158 | 75 | 1193 / 5032 | **60 031** | 200 / 1475 | 2950 | 180–220 | **153–187** | 10 % | identiques | 40 |

Le delta brut HP est bien `1471/1193 - 1 = +23,30 %`, mais le niveau 85
sélectionne aussi un autre record `MonLvl`; le vrai delta p1 est
`90 937 - 60 031 = +30 906`, soit **+51,48 %**. Les pourcentages melee bruts
font `+11,11 %` au minimum et `+9,09 %` au maximum; les dégâts calculés font
respectivement **+39/+25,49 %** et **+43/+22,99 %** avant Crit. Les branches
critiques sont384–460 à5 % contre306–374 à10 %; l'espérance conditionnelle sur
un A1 qui touche vaut221,55 contre187,00, soit **+18,48 %**. La défense PD2 est
`-1 134/-38,44 %` contre BKVince. BKV est aussi plus mobile et réactif :
`Velocity/Run 10/10` contre8/8, `aidel(N/H)=4/0` contre11/9,
`aidist(H)=46` contre vide, et il ajoute `primeevil=1`. Une copie de row
complète ne représente donc pas le seul buff HP/melee du wiki.

### Chaînes de drop core S13, Hell p1

| TC/ligne | Picks/NoDrop | branches exactes | P(≥1 essence) |
|---|---|---|---:|
| `Andariel (H)`, 724 | 7/1 | gold 224; `Act 4 (H) Equip A` 387; Junk 305; Good 61; `tes` 20 | 11,4484 % |
| `Andarielq (H)`, 727 | 7/1 | Equip A 810; Good 127; `tes` 42 | 23,1327 % |
| `Mephisto (H)`, 742 | 7/0 | gold 75; **même** Equip A 787; Junk 75; Good 45; `ceh` 15 | 8,6943 % |
| `Mephistoq (H)`, 745 | 7/0 | Equip A 928; Good 53; `ceh` 17 | 9,7950 % |

Avec le cap de six drops, les formes fermées sont : Andariel non-quest
`1-(978/998)^7-(20/998)*(977/998)^6`, quest
`1-(938/980)^7-(42/980)*(937/980)^6`; Mephisto/Duriel/Baal non-quest
`1-(982/997)^6`, quest `1-(981/998)^6`. Elles séparent bien la cible directe
des autres branches consommant un slot.

Le terminal Equip est le même, mais Mephisto a zéro NoDrop, environ 79 % de
poids Equip par pick non-quest contre 39 % pour Andariel, et beaucoup moins de
gold/junk. « Same pool » est donc vrai pour la famille d'équipement, tandis que
« Mephisto has better probabilities » est aussi vrai. Les essences sont
explicitement séparées : `tes` pour Andariel, `ceh` pour Mephisto.

BKVince ne suit pas ce graphe legacy. `monstats.txt` ligne 158 pointe
`TreasureClass`, `TreasureClassUnique` **et** `TreasureClassQuest`, en Hell,
vers `Andarielq (H)`. Cette racine, ligne 791, a `Picks=-2` et appelle
déterministement `Andarielq Essence (H)` ligne 778 (`tes`, 1/1), puis
`Andarielq Items (H)` ligne 779. La probabilité effective BKVince p1 de
Twisted Essence est donc **100 %**. La famille items est Act 2, pas Act 4.
Le comportement D2R « permanent quest drop » est ainsi rendu indépendant du
dernier prédicat natif : toutes les colonnes utiles pointent déjà vers la TC q.

| Nº | Fiche d'audit |
|---:|---|
| 1 | Hell : niveau 85; même pool que Mephisto mais meilleures probabilités chez Mephisto; conserve Twisted Essence; +10 % melee; environ +23 % HP. |
| 2 | [`Monsters` oldid 23935, `Andariel`](https://wiki.projectdiablo2.com/w/index.php?title=Monsters&oldid=23935#Andariel), 20 juillet 2026; changements documentés par [`Season 4` oldid 17686](https://wiki.projectdiablo2.com/w/index.php?title=Patch%3ASeason_4&oldid=17686#GENERAL_CHANGES). |
| 3 | `MonStats.txt` ligne 158 : `Level(H)=85`, `MinHP/MaxHP(H)=1471`, `AC(H)=110`, `A1Min/Max(H)=200/240`, `Crit=5`, TC1–3=`Andariel (H)`, TC4=`Andarielq (H)`. `MonLvl.txt` niveau85 : `L-HP=6182`, `L-AC=1651`. TC rows 724/727 et Meph rows 742/745 détaillées ci-dessus. |
| 4 | SP+ possède les mêmes cellules de combat, mais ses TC globales divergent du core; pour les valeurs citées ici, la preuve est relue dans le core, pas transférée depuis SP+. |
| 5 | BKV ligne 158 : `Level(H)=75`, HP%=1193, AC%=200, A1=180/220, `Crit=10`, vitesse/AI/prime différents; `MonLvl`75 `L-HP=5032`, `L-AC=1475`; toutes les TC Andariel utiles=`Andarielq (H)`. |
| 6 | PD2 p1 : 90 937 HP, A1 pré-Crit192–230, branche384–460 à5 %, espérance221,55, défense1816; pool Act4 EquipA partagé avec Meph mais poids distincts; essence11,448/23,133 % nq/q. |
| 7 | BKV p1 : 60 031 HP, A1 pré-Crit153–187, branche306–374 à10 %, espérance187,00, défense2950; permanent q data-baké; `tes`100 %; pool Act2. |
| 8 | Niveau +10; HP+30 906; A1 pré-Crit+39/+43 et espérance+34,55; défense-1 134; essence PD2-76,867 points au mieux face au BKV effectif; vitesse/AI/prime sont des deltas catégoriels; pool Act4 contre Act2. |
| 9 | Niveau+13,33 %; HP+51,48 %; A1 pré-Crit+25,49/+22,99 %, espérance+18,48 %; défense-38,44 %; essence quest PD2-76,87 %. |
| 10 | **Vrai mais incomplet.** Les pourcentages wiki décrivent les cellules brutes, pas les valeurs dérivées. « Même pool » n'est pas « mêmes chances ». Les cellules/graphes sont prouvés dans les données PD2; combat et probabilités sont reconstruits sous les contrats §3. |
| 11 | **Softcode avec adaptation TC D2R.** |
| 12 | Niveau/HP/dégâts/défense sont directement faisables en TXT. Le graphe TC doit être reconstruit dans le schéma D2R sans casser le quest drop permanent. |
| 13 | Décider le pool voulu (Act4 PD2 ou identité BKV), le taux d'essence, la politique q permanente et l'économie globale. |
| 14 | Copier les TC core réduirait massivement l'essence BKV; copier seulement la TC parent perdrait les poids; niveau85 modifie XP, hit chance et tables liées au niveau en plus du combat. |
| 15 | **Adapter**, jamais merger tel quel : les stats peuvent être sélectionnées indépendamment; simuler et décider le graphe économique. |
| 16 | Futur possible : `monstats.txt`, `monlvl.txt` seulement si nécessaire, `treasureclassex.txt`; aucune modification ici. |
| 17 | p1/p3/p5/p8, quest jamais faite/pending/déjà faite, 100 000 kills simulés puis témoins runtime, HP min/max, melee rolls, defense/CTH, tes/équipement/Good/Junk/gold, MF, cap six drops, save/reload et host/joiner. |

## F. Duriel et Tal Rasha's Tomb

### Duriel Hell

| Source | niveau | HP p1 | défense | A1; A2 | résistances D/M/F/L/C/P | block |
|---|---:|---:|---:|---|---|---:|
| PD2 core=SP+ | 88 | 84 524 | 2044 | 140–190; 115–165 | 50/33/75/75/95/75 | 50 |
| BKVince | 88 | 84 524 | 3408 | identiques | identiques | 50 |

Le niveau 88, la vie et les dégâts sont **déjà présents** dans BKVince. Seule
la défense diverge ici : PD2 est `-1364/-40,02 %`. La note Season 4 disait
niveau85; la révision épinglée `Monsters` oldid23935 indique88 et la donnée
core S13 tranche en faveur de88.

Une copie de row complète ne serait pourtant pas un no-op. Le core emploie
`MonCharge/MonJab/MonSmite`, `Crit=5`, `Velocity/Run=10/15`, `aidel=15/15/15`,
`aidist(H)` vide et `primeevil=0`; BKV emploie `Charge/Jab/Smite`, `Crit=10`,
`Velocity/Run=12/18`, `aidel=15/8/0`, `aidist(H)=46` et `primeevil=1`.
`MonHolyFreeze` reste le quatrième skill dans les deux. Les cellules HP/dégâts
bruts sont présentes, pas l'ensemble du comportement.

Le graphe core est réellement aligné sur Baal :

- `Duriel (H)` ligne 894, `Picks=5`, appelle `Duriel (H) - Base` ligne 730;
  cette base a `Picks=7`, `NoDrop=0`, gold75, `Act 5 (H) Equip B`787,
  Junk75, Good45, `tes`15;
- `Baal (H)` ligne 782 a directement les mêmes Picks, NoDrop et poids, avec
  `fed` à la place de `tes`;
- les quest bases Duriel ligne 733 et Baal ligne 785 ont toutes deux
  Equip928, Good53, essence17, NoDrop0; la racine Durielq ligne897 ajoute le
  wrapper Picks5;
- le cap de six drops rend les probabilités d'essence Duriel et Baal
  identiques : **8,6943 % non-quest, 9,7950 % quest** en p1. L'essence reste
  `tes` pour Duriel.

BKVince est très différent : Duriel normal appelle une base déterministe
essence+items et donne `tes` à **100 %**; sa branche quest donne 40,6708 %.
Baal est à 100 %/9,1479 %. Les pools BKV sont Act3 pour Duriel et Act5 pour
Baal. Copier PD2 améliorerait le tier d'items de Duriel, mais nerferait très
fortement son essence.

### Les sept tombes et la chambre

| Zone/rows `Levels.txt` | PD2 core=SP+ | BKVince | D2R vanilla 3.2 |
|---|---:|---:|---:|
| `Tal Rasha's Tomb`, IDs 66–72, lignes 68–74 | **82 pour les sept** | **87 pour les sept** | 80 pour les sept |
| `Duriel's Lair`, ID73, ligne75 | 85 | 85 | 80 |

La vraie tombe n'est pas la seule row élevée. Le moteur choisit une staff tomb
et une boss tomb parmi 66–72, puis chaque labyrinthe conserve la même row
`Levels`; la chambre 73 est séparée. Les sept géométries ont six rooms de
16×16 et les quatre presets W/E/S/N communs; aucune différence DS1 n'est
requise pour appliquer le niveau.

Conséquences Hell, sous la sémantique de niveau classique : PD2 produit des
monstres ordinaires/champions/uniques **82/84/85**, BKVince **87/89/90**. Le
delta est -5 niveaux dans les trois catégories. En PD2, un normal H2H tombe sur
`Act 5 (H) H2H A`/EquipA (bucket max84), champion sur ChampB et unique sur
UniqueC (bucket87). En BKVince, les normaux non-Wraith atteignent déjà le
bucket87; les Wraith normals suivent leur TC Good/Magic propre. La baisse 87→82
réduit donc ilvl, XP, précision/défense via `MonLvl` et accès normal aux bases
haut tier. Duriel, `boss=1`, reste niveau88.

| Nº | Fiche d'audit |
|---:|---|
| 1 | Duriel niveau88 en Hell; même pool que Baal; conserve Twisted Essence; Tal Rasha's Tomb niveau82. |
| 2 | [`Monsters` oldid 23935, `Duriel`](https://wiki.projectdiablo2.com/w/index.php?title=Monsters&oldid=23935#Duriel), 20 juillet 2026; [`Season 4` oldid17686](https://wiki.projectdiablo2.com/w/index.php?title=Patch%3ASeason_4&oldid=17686#GENERAL_CHANGES) documente 85 et Tomb 80→82; [`Zones` oldid22824](https://wiki.projectdiablo2.com/w/index.php?title=Zones&oldid=22824#Zone_Levels) distingue Tomb82/Lair85. |
| 3 | `MonStats.txt` Duriel ligne213 `Level(H)=88`, HP%=1295, AC%=120, A1=140/190, A2=115/165; TC rows 730/733/894/897. `Levels.txt` lignes68–74 `MonLvl3Ex=82`, ligne75=85. |
| 4 | SP+ possède les mêmes stats/zones, mais ses poids TC sont différents et restent attribués SP+ seulement. |
| 5 | BKV Duriel ligne213 niveau88, HP%=1295, AC%=200 et mêmes dégâts bruts; `Charge/Jab/Smite` au lieu des versions `Mon*`, Crit10 vs5, vitesse12/18 vs10/15, AI delays/dist et `primeevil` différents; zone rows68–74 `MonLvlEx(H)=87`, lair85; TC regular/quest distinctes. |
| 6 | Duriel 84 524 HP p1, def2044; drops Act5 EquipB alignés Baal, essence tes8,694/9,795 %. Tombes toutes82 → mlvl82/84/85. |
| 7 | Duriel 84 524 HP, def3408; essence100/40,671 %, pool Act3; tombes toutes87 → mlvl87/89/90. |
| 8 | Duriel niveau/HP/dégâts bruts0; def -1364; skills/AI/Crit/vitesse/prime sont des deltas catégoriels distincts. Tombes -5 niveaux. Essence nq -91,306 points; q -30,876 points. |
| 9 | Duriel def -40,02 %; Tomb -5,75 %. Essence nq -91,31 %, q -75,92 % relativement à BKV; aucun pourcentage unique n'est valide pour les deltas comportementaux. |
| 10 | **Vrai** pour niveau88 core courant et sept tombes82; **vrai mais incomplet** pour même pool Baal; le niveau85 de la note S4 est historique et contredit par les données S13. |
| 11 | **Softcode** pour niveau/stats/TC; aucune map edit pour le niveau de zone. |
| 12 | Faisable D2R3.2 par TXT, mais la TC legacy doit être transposée sémantiquement et non copiée par colonnes. |
| 13 | Politique économique, essences, pools Act3/Act5, permanent/quest state, incidence sur bases TC87 et densité BKV supérieure. |
| 14 | Tomb82 serait une régression BKV; TC PD2 réduit l'essence; modifier le lair par erreur affecterait les trash mobs sans changer Duriel. |
| 15 | **Déjà présent** pour Duriel88/HP/dégâts; **rejeter** Tomb82; **adapter** défense et TC seulement après décision d'équilibrage. |
| 16 | Futur possible : `monstats.txt`, `treasureclassex.txt`, `levels.txt`; aucune DS1 nécessaire pour les niveaux. |
| 17 | Sept tombes vraie/fausses/boss tomb, Lair séparé, normal/champion/unique, ilvl et bases buckets84/87, XP/CTH, p1/p3/p5/p8, Duriel quest states, tes/Equip/Good/Junk, cap six, save/reload, host/joiner. |

## G. Mephisto et Council Members

### G.1 Flottaison au-dessus du blood moat

`MonStats.txt`, row `mephisto`, ligne 244 :

| Source | `flying` | `opendoors` |
|---|---:|---:|
| PD2 core=SP+ | 1 | 1 |
| BKVince | **1** | 1 |
| D2R 3.2 vanilla | vide | 1 |

`MonStats2.txt`, ligne 244, est identique core/BKV pour les cellules pertinentes :
`Height=3`, `OverlayHeight=4`, `PixHeight=112`, `SizeX=3`, `SizeY=3`,
`isSel=1`, `noSel` vide. Aucune divergence de sélection, taille ou collision
n'est donc nécessaire pour expliquer la faculté documentée. BKVince a déjà
repris `flying=1`; le changement est un **no-op de données**. Une validation
runtime resterait nécessaire pour prouver les effets précis du navmesh D2R
sur murs, portes, moat, téléportation AI et acquisition de cible.

### G.2 Preuve structurelle `MephComp.ds1`

| Carte | octets | SHA-256 |
|---|---:|---|
| PD2 core, `pd2maps.mpq:data/global/tiles/ACT3/Travincal/mephcomp.ds1` | 37 183 | `768F0A06831ECF09F88FEA99D29FDCFE6BC599A0759F0D872DF9530DDFFA5279` |
| D2R 3.2/BKV fallback, `act3/travincal/mephcomp.ds1` | 37 024 | `1DB568BC8183EFC862468647EF5245109FE396E2178D3749591E594A25750F93` |

Les deux DS1 v18 ont les mêmes dimensions 42×30, deux wall layers, deux floor
layers, 71 unités, et les hashes des **sept couches de tuiles sont identiques**.
Les 61 objets et leurs coordonnées sont aussi identiques. Seuls trois
placements de monstres changent; Mephisto reste `(40,65)` :

| MonPreset Act III | Boss | D2R/BKV `(x,y)` | PD2 `(x,y)` | distance à Meph D2R→PD2 |
|---:|---|---|---|---:|
| 35 | Bremm Sparkfist | (120,70) | (47,66) | 80,156 → 7,071 |
| 37 | Wyand Voidfinger | (103,18) | (78,48) | 78,600 → 41,629 |
| 38 | Maffer Dragonhand | (103,123) | (79,88) | 85,633 → 45,277 |

La différence est donc **prouvée comme map edit**, non hardcode. SP+ n'embarque
aucun DS1, mais hérite vraisemblablement du `pd2maps.mpq` d'une installation
PD2; cela n'est pas une preuve autonome du snapshot. BKVince n'a aucun override
de cette carte et hérite de D2R.

### G.3 Immunités Council

`MonStats.txt`, lignes 347–349, est identique core/BKV pour les résistances :

| classe | Res phys/magic/fire/light/cold/poison Hell | membres nommés |
|---|---|---|
| `councilmember1` | 50/0/**120**/33/33/33 | Travincal Ismail, Toorc |
| `councilmember2` | 50/0/33/**100**/33/33 | Durance Wyand; Travincal Geleb |
| `councilmember3` | 50/0/33/**100**/33/33 | Durance Bremm, Maffer |

Les trois Durance sont donc toujours lightning immune à la base; le statement
n'est pas une règle de tous les Council : Travincal n'en a qu'un sur trois.
`SuperUniques.txt` core lignes 28–33 fixe notamment Bremm Aura+Lightning,
Wyand ManaHit+Teleport, Maffer Strong+Fast. BKV ajoute un Mod3 Aura à plusieurs
d'entre eux, sans changer la conclusion de résistance.

Sous le handler classique, Fire/Cold/Lightning Enchanted ajoute +75 seulement
tant que le monstre a moins de deux immunités. Un Durance Council partant avec
une immunité lightning peut donc acquérir **au plus une immunité additionnelle**
et finir à deux; la confirmation du même cap dans le binaire PD2/D2R moderne
reste une inférence forte. Exemples Travincal : Geleb classe2 devient aussi
fire immune (`33+75=108`); Toorc classe1 est déjà fire immune et devient cold
immune (`33+75=108`). Bremm, déjà lightning100, monte à175 avec son mod fixe
Lightning sans créer un nouveau type d'immunité.

Cette équivalence ne vaut que pour les résistances demandées. La row complète
core→BKV diffère pour les trois classes : `Velocity 6/8/10→8/10/12`,
`Run 12→15`, `aidel(N) 13→6`, `aidel(H) 12→0`, `aidist(H) vide→46`,
`Skill1 MonHydra→Hydra`, `Crit 5→10` et `AC(H) 110→200`; `TransLvl` passe
aussi de0 à1/2 pour les classes2/3. Le placement PD2 n'est donc pas une preuve
que l'encounter complet serait un no-op statistique dans BKVince.

### G.4 Fiche normalisée

| Nº | Fiche d'audit |
|---:|---|
| 1 | Mephisto flotte au-dessus du blood moat; les Council Durance sont rapprochés; ils sont toujours lightning immune et peuvent obtenir d'autres immunités. |
| 2 | [`Monsters` oldid23935, `Mephisto`](https://wiki.projectdiablo2.com/w/index.php?title=Monsters&oldid=23935#Mephisto), 20 juillet 2026; déplacement documenté plus généralement dans [`Season2` oldid17688](https://wiki.projectdiablo2.com/w/index.php?title=Patch%3ASeason_2&oldid=17688#GENERAL_CHANGES); immunité ajoutée au wiki oldid21737. |
| 3 | `MonStats` Meph ligne244 `flying=1`; `MonStats2` ligne244 valeurs ci-dessus. DS1 core hash/coordonnées ci-dessus. Council rows347–349 résistances; `SuperUniques`28–33 mods fixes. |
| 4 | SP+ a les mêmes TXT mais aucun DS1; la carte n'est donc pas prouvée par SP+ seul. |
| 5 | BKV Meph `flying=1`; mêmes cellules MonStats2 pertinentes; aucun `mephcomp.ds1`; Council mêmes résistances et Mod3 supplémentaires sans résistance, mais vitesse/AI/skill/Crit/AC diffèrent comme détaillé en G.3. |
| 6 | Meph flying actif; Council à 7,071/41,629/45,277 unités de Meph; 3/3 Durance lightning immune, maximum inféré deux immunités totales. |
| 7 | Meph flying déjà actif; positions 80,156/78,600/85,633; 3/3 Durance également lightning immune. |
| 8 | Flying0; distances réduites de73,085/36,971/40,356; immunité base0. |
| 9 | Flying et immunité : 0 %. Distances : -91,178/-47,037/-47,127 %. |
| 10 | **Déjà présent** pour Meph flying et la LI de base des trois Durance; capacité/cap des immunités additionnelles cohérents sous sémantique héritée mais à valider92777; **vrai, prouvé par la carte PD2** pour le rapprochement. « Council » sans qualifier Durance serait trompeur. |
| 11 | Flying/résistances : **softcode**. Placement : **map edit**. Le comportement pathing final est hardcode-backed. |
| 12 | Tous réalisables en 3.2 : les données existent et un DS1 override ciblé suffit au placement; validation runtime obligatoire. |
| 13 | Résolveur MonPreset ActIII, priorité de chargement DS1, classification Durance vs Travincal, cap moderne d'immunités. |
| 14 | Meph peut traverser des limites non souhaitées, portes/murs, perdre/retarder son targeting; Council rapprochés peuvent aggro simultanément; modifier TravN par erreur toucherait Travincal. |
| 15 | **Déjà présent** pour flying/LI de base; **reporter** toute affirmation ferme sur le cap additionnel jusqu'au test92777; **map edit optionnel** à merger séparément pour les trois coordonnées; ne pas modifier Travincal. |
| 16 | Futur possible : seulement `data/global/tiles/act3/travincal/mephcomp.ds1` pour le placement; aucune table nécessaire pour les no-op. |
| 17 | Carte hashée, spawn exact des trois, aggro/retour/leash, Meph sur moat et pont, portes/murs/teleport/targeting, résistance avant/après deux mods, Durance/Travincal, plusieurs seed NM/H, host/joiner. |

## H. Uber Mephisto et Uber Baal

### H.1 Uber Mephisto — Conviction

`MonStats.txt` core ligne707, `ubermephisto`, `hcIdx=704`, ne contient pas
Conviction dans ses sept slots. Le dispatcher Aura Enchanted PD2 à RVA
`0x2C62B0` reconnaît explicitement hcIdx704 et impose **Conviction ID123,
niveau20**. Ce n'est donc pas un simple mod aléatoire.

| valeur à L20 | PD2 core `skills.txt` ligne125 | BKVince ligne125 |
|---|---|---|
| formule résistances | `-min(ln34,150)`, Params3/4=12/2 | même formule, Params3/4=30/5 |
| résultat F/C/L | **-50** | **-125** |
| poison/physique/magie | non affectés | non affectés |
| rayon | `ln12`, Params1/2=22/1 → **41** | `ln12*2`, 20/0 → **40** |
| défense | `-dm56`, Params5/6=10/75 → **-65 %** | Params5/6=40/100 → **-90 %** |

La réduction PD2 est donc réellement -50; elle ne provient pas de la formule
vanilla niveau20. Le delta face à BKV est `+75 points`, soit une magnitude
**60 % moins forte** : c'est un nerf important d'Uber Mephisto contre les
joueurs/mercenaires même si « niveau20 » paraît élevé. La plage 41 est l'unité
interne de la skill; l'approximation classique est 8,2 yards, sans promouvoir
cette conversion en preuve D2R.

La réduction de défense passe simultanément de -90 % BKV à -65 % PD2 :
`+25 points`, magnitude **27,78 % moins forte**. Ce second nerf augmente la
chance de toucher du joueur/mercenaire sous l'aura et doit être séparé de la
baisse des résistances F/C/L.

Changer la row `Conviction` BKV modifierait aussi la skill joueur. La route
sûre exige une variante monster-only propre et un dispatcher 92777 capable de
la sélectionner pour Uber Mephisto. Caps et overcap de résistance s'appliquent
ensuite au joueur; Sunders et le patch BKV de pierce sur les résistances des
**monstres** ne doivent pas être confondus avec cette aura ennemie.

### H.2 Uber Baal — Lower Resist

| Source | MonStats/skill | niveau | résistances | durée | rayon |
|---|---|---:|---|---:|---:|
| PD2 core | `uberbaal` ligne712 `Skill6=Baal Lowres`; skill ligne482 ID480 | 13 | F/L/C/P **-75** constants | `500` frames = **20 s** | `10+lvl/3+CurseMastery/3` = **14** |
| BKVince | `Skill6=Defense Curse`; skill ligne311, `*Id=309` commentaire | 3 | aucune; défense `-60 %` | `200+75×(3-1)=350f` = **14 s** | `3+(3-1)=5` |

La durée wiki « about 25 seconds » est **fausse pour S13** : 500 frames valent
20 secondes. `Baal Lowres` emploie `srvdofunc=30`, état `lowerresist`, filtre3,
et ne touche ni physique ni magie.

Sous la sémantique AI héritée, `BaalCrab`, branche case8, utilise Skill6 contre
une cible non-joueur ou lorsque sa vie maximale dépasse son mana maximal;
sinon elle choisit Skill7/Blood Mana. Aucun RVA PD2 direct n'est promu pour
cette sélection et sa fréquence temporelle n'a pas été reconstruite. BKVince
configure déjà une Skill6 : une nouvelle row Lower Resist et son assignment
pourraient suffire côté données **si** le handler92777 confirme cette
sémantique. L'ordinal réel de la row, la compatibilité des fonctions/états et
le comportement de Skill7 doivent être validés.

### H.3 Fiche normalisée

| Nº | Fiche d'audit |
|---:|---|
| 1 | Uber Mephisto utilise Conviction20, -50 au lieu de -125; Uber Baal lance sa propre Lower Resist, -75 pendant environ25s. |
| 2 | [`Monsters` oldid23935, `Uber Tristram Prime Evils`](https://wiki.projectdiablo2.com/w/index.php?title=Monsters&oldid=23935#Uber_Tristram_Prime_Evils), 20 juillet 2026; S3 oldid17687 documente -75 pour Baal. |
| 3 | Meph : MonStats707 + dispatcher RVA0x2C62B0 + Skills125 valeurs ci-dessus. Baal : MonStats712 Skill6/lvl13; Skills482 ID480, state/formules ci-dessus; sélection BaalCrab case8 seulement inférée de la sémantique héritée. |
| 4 | SP+ a les mêmes rows; la preuve native Meph vient du core DLL, pas du snapshot. La sélection AI de Baal n'est directement prouvée ni par SP+ ni par un RVA PD2 promu. |
| 5 | BKV Meph hcIdx704 et Conviction partagée row125 donne -125; BKV Baal Skill6 Defense Curse lvl3, row311, -60% def,14s, aucune résistance. |
| 6 | PD2 : Meph -50 F/C/L, défense-65 %, rayon41; Baal -75 F/L/C/P,20s, rayon14. |
| 7 | BKV : Meph -125 F/C/L, défense-90 %, rayon40; Baal defense-only -60 %,14s, rayon5. |
| 8 | Meph +75 points de résistance, +25 points de défense et rayon+1. Baal -75 points sur quatre résistances, durée+150f/+6s, rayon+9; effet défense supprimé. |
| 9 | Meph magnitudes résistance -60 % et défense -27,78 %, rayon+2,5 %. Baal durée+42,86 %, rayon+180 %; pourcentage de résistance n/a depuis zéro. |
| 10 | Meph : **vrai, prouvé code+données**. Baal row propre/-75/20s : **prouvé dans les données**; sélection effective : **inférence forte**; durée wiki25s : **faux**, S13 configuré=20s; fréquence : inconnue. |
| 11 | Meph : **hybride hardcode spécial + skill**. Baal : **softcode/hardcode-backed par AI existante**. |
| 12 | Meph nécessite de séparer player/monster skill et changer le sélecteur92777; modifier la row partagée est déconseillé. Baal semble faisable par nouvelle skill+slot après test des funcs. |
| 13 | Row keys et ordinals runtime gouvernés, dispatcher Aura92777, état Lower Resist, AI Skill6/7, caps/overcap, interaction curses et durée de boss. |
| 14 | Nerf accidentel de Conviction joueur; ordinal PD2 transposé; Baal spam/never-cast; remplacement de Defense Curse modifiant fortement CTH; conflits de groupe de curse. |
| 15 | **Adapter** les deux séparément. Meph est un nerf de boss assumé à décider; Baal ajoute un danger élémentaire majeur tout en retirant son debuff défense BKV. |
| 16 | Futur possible : `skills.txt`, `states.txt` si nouvel état nécessaire, `monstats.txt`; selector natif/configuration à décider pour Meph. |
| 17 | Meph niveau/rayon/-res F/C/L/poison/phys/mag, -défense65 exact, overcap/caps, joueur/merc/pet, skill joueur inchangée; Baal cast frequency, conditions HP/mana, -75 exact,20s, recast, curse conflicts, death/portal, solo/host/joiner. |

## I. Nihlathak

Pour `nihlathakboss`, hcIdx526, ligne529, les cellules HP, niveau, défense,
résistances et skills utilisées dans les calculs sont identiques core/SP+/BKV
en Normal et Nightmare; parmi ces cellules, seul le HP% Hell change. Les
valeurs finales incluent le bonus SuperUnique commun UMod2
(+300/+200/+100 %) puis le +3 niveaux d'unité. La row complète diffère sur la
vitesse, l'AI, `Crit` et le mod fixe, comme détaillé ci-dessous.

| difficulté | niveaux brut/lookup/unité | HP% core/BKV | `L-HP` | base p1 core/BKV | finale p1 core/BKV |
|---|---|---|---:|---|---|
| Normal | 65/65/68 | 191/191 | 477 | 911/911 | 3 644/3 644 |
| Nightmare | 70/70/73 | 191/191 | 1685 | 3 218/3 218 | 9 654/9 654 |
| Hell | 92/92/95 | **573/191** | 6987 | **40 035/13 345** | **80 070/26 690** |

En Hell, 40 035 est à la fois minimum, maximum et moyenne de la **base p1
avant UMod2**, puisque MinHP=MaxHP=573. Ce n'est pas la vie finale. Défense1951,
block50, drain100, dégâts A1/A2 nuls et résistances0/25/33/33/70/70 sont
identiques. Le player count multiplie ensuite les deux baselines selon le même
contrat; le ratio PD2/BKV reste3 tant qu'aucun cap externe ne sature.

Le reste de la row n'est pas identique : core→BKV donne `Velocity/Run 7/7→9/9`,
`aidel(N) 15→8`, `aidel(H) 15→0`, `aidist(H) vide→46` et `Crit 5→10`;
BKV ajoute en outre Aura Enchanted comme mod fixe. Les cinq skills nommés sont
identiques. Le constat « N/NM identiques » ci-dessous vise donc uniquement les
HP effectifs, pas la cadence, le déplacement ou le danger du combat.

La Treasure Class est indépendante : core `Nihlathak (H)` ligne779,
`Picks=5`, `NoDrop=2`, autres poids925, `pk3=24`, donne une Key of Destruction
à 11,9973 % p1. BKV ligne967 a `NoDrop=19`, autres poids41 et `pk1=2`, soit
15,1215 %, mais `pk1` est **Key of Terror** : même anomalie de code de clé que
le Summoner. Le merge HP ne doit ni masquer ni corriger implicitement ce graphe.

| Nº | Fiche d'audit |
|---:|---|
| 1 | Vie multipliée par trois; valeur40 035 HP. |
| 2 | [`Monsters` oldid23935, `Key Holders`](https://wiki.projectdiablo2.com/w/index.php?title=Monsters&oldid=23935#Key_Holders), 20 juillet 2026; aucune patch note chiffrée trouvée. |
| 3 | Core `MonStats`529 `MinHP/MaxHP(H)=573`, level92; `MonLvl`92 `L-HP=6987`; SuperUnique UMod2 Hell+100. |
| 4 | SP+ identique au core pour ces cellules. |
| 5 | BKV `MonStats`529 HP%=191; mêmes niveau/MonLvl/défense/résistances/skills, mais vitesse9/9 vs7/7, Crit10 vs5, delays/dist AI différents et Aura Enchanted fixe; TC pointe `pk1` au lieu de `pk3`. |
| 6 | N/NM finales3644/9654; Hell base40035, finale80070 p1. |
| 7 | HP N/NM identiques; Hell base13345, finale26690. Les différences de vitesse/Crit/AI/Aura persistent indépendamment de ce no-op HP. |
| 8 | HP Hell base+26690, finale+53380; HP N/NM0. Les autres cellules sont des deltas catégoriels. |
| 9 | HP Hell+200 %, donc ×3; HP N/NM0 %. Aucun pourcentage global de danger n'est inféré. |
| 10 | **Vrai mais incomplet.** Les cellules prouvent le ×3 Hell;40 035 et80 070 sont dérivés sous le contrat MonLvl/UMod §3 et ne doivent pas être confondus. |
| 11 | **Softcode.** |
| 12 | Faisable par `MinHP/MaxHP(H)` sans natif. |
| 13 | Conserver SuperUnique mods, Aura BKV, player-count caps et Corpse Explosion propre à Nihlathak séparés du HP. |
| 14 | Annoncer40035 comme vie finale sous-estime de moitié; multiplier après UMod2 une seconde fois donnerait ×6; durée du combat proche×3 si DPS inchangé. |
| 15 | **Adapter** : merge Hell possible, mais évaluer la combinaison avec Aura Enchanted BKV et son danger Corpse Explosion. |
| 16 | Futur possible : `monstats.txt` uniquement. |
| 17 | N/NM/H, p1/p8, min=max, vie avant/après UMod2, mods aléatoires, CE corps proches, Aura, key TC inchangée, save/reload, host/joiner. |

## J. Uber Mephisto, Uber Diablo et Uber Baal

### J.1 Projections Hell p1 depuis les tables

Les rows PD2 portent niveau120, mais la dernière row `MonLvl` est110 : les
ratios `L-HP/L-AC/L-DM=10000/2100/...` de110 sont utilisés tandis que l'unité
reste niveau120. BKV est niveau110 avec `L-HP=9057`.

Cette comparaison ne décrit **pas** le résultat d'un simple port de
`Level(H)=120` dans BKV. Son `monlvl.txt` possède127 rows et une row120 réelle :
`L-HP(H)=10207`, `L-AC(H)=2268`, `L-DM(H)=138`, `L-TH(H)=4738`, contre la
dernière row110 core PD2 `10000/2100/130/6532`. Copier les pourcentages PD2 dans
la row BKV ferait donc sélectionner des ratios différents. Pour reproduire les
valeurs S13, il faut recalibrer chaque pourcentage MonStats contre BKV120 ou
porter consciemment les ratios MonLvl; modifier la row globale120 toucherait
tous ses autres consommateurs.

| Uber | HP% PD2 | HP PD2 | HP BKV | delta HP | défense PD2/BKV | delta défense |
|---|---:|---:|---:|---|---:|---:|
| Mephisto | 5695 fixe | 569 500 | 588 705–597 762 | -19 205 à -28 262 (-3,26 à -4,73 %) | 1260/3347 | -62,35 % |
| Diablo | 6427 fixe | 642 700 | 588 705–597 762 | +44 938 à +53 995 (+7,52 à +9,17 %) | 1155/2928 | -60,55 % |
| Baal | 6336 fixe | 633 600 | 588 705–597 762 | +35 838 à +44 895 (+6,00 à +7,63 %) | 1176/3138 | -62,52 % |

Le wiki « +10 % HP, -25 % defense » est un résumé historique, pas le delta
S13→BKVince. Mephisto a même moins de vie. Les trois PD2 ont des résistances
uniformes 40/40/60/60/60/60, beaucoup plus faibles que BKV :

- Meph BKV 20/75/75/**110**/75/**110**;
- Diablo BKV 50/75/**110**/75/75/75;
- Baal BKV 50/75/75/75/**110**/75.

La couche offensive confirme que « +HP/-def » ne décrit pas tout :

| Boss | A1; A2 pré-Crit PD2 | A1; A2 pré-Crit BKV | Crit PD2/BKV | espérance A1; A2 PD2/BKV |
|---|---|---|---:|---|
| Uber Mephisto | 487–572; — | 468–550; — | 5/10 % | 555,98 / 559,90; — |
| Uber Diablo | 481–494; 143–299 | 462–475; 137–287 | 5/10 % | 511,88 / 515,35; 232,05 / 233,20 |
| Uber Baal | 650–715; 429–624 | 412–475; 206–300 | 5/10 % | 716,63 / 487,85; 552,83 / 278,30 |

Les deux premiers ont une espérance de hit presque identique malgré leurs rolls
PD2 plus hauts, car BKV critique deux fois plus souvent. Uber Baal reste au
contraire environ **+46,90 % A1** et **+98,64 % A2** dans cette estimation; sa
Lower Resist augmente encore le danger élémentaire. Fréquence AI, animation,
CTH du boss et résistance de la cible ne sont pas intégrées à ces espérances.

### J.2 Chance de toucher témoin

Valeurs avant block, formule entière et sans ITD :

| Uber/source | défense/niveau | attaquant90, AR5k/10k/20k | attaquant99, AR5k/10k/20k | block |
|---|---|---|---|---:|
| Meph PD2 |1260/120|67/75/80 %|71/79/84 %|50 %|
| Meph BKV |3347/110|53/66/76 %|55/70/80 %|50 %|
| Diablo PD2 |1155/120|69/76/80 %|73/80/84 %|50 %|
| Diablo BKV |2928/110|56/69/78 %|59/72/82 %|50 %|
| Baal PD2 |1176/120|68/76/80 %|72/80/84 %|55 %|
| Baal BKV |3138/110|54/68/77 %|57/72/81 %|55 %|

Au témoin niveau90/AR10k, la chance après block vaut Meph37,5 % contre33 %,
Diablo38 % contre34,5 %, Baal34,2 % contre30,6 %. Le proxy
`HP/chance-après-block` donne alors PD2 environ -15,5 % pour Meph, -1,7 % pour
Diablo et -4,4 % pour Baal : la défense plus basse compense plus que la hausse
de vie à ce point de mesure. Au niveau99/AR20k, la hausse HP peut reprendre le
dessus pour Diablo/Baal. Si le patch BKV ITD s'applique effectivement au boss,
la défense cesse d'être le facteur et le HP domine; ce cas doit être testé.

### J.3 Fiche normalisée

| Nº | Fiche d'audit |
|---:|---|
| 1 | Les trois Ubers ont10 % de vie supplémentaire et25 % de défense en moins. |
| 2 | [`Monsters` oldid23935, `Uber Tristram Prime Evils`](https://wiki.projectdiablo2.com/w/index.php?title=Monsters&oldid=23935#Uber_Tristram_Prime_Evils), 20 juillet 2026; S3 oldid17687 documente seulement -25 % défense. |
| 3 | Core `MonStats` lignes707/708/712 : level120, HP%=5695/6427/6336, AC%=60/55/56, Crit5; `MonLvl` s'arrête110, LHP10000/LAC2100/LDM130/LTH6532; résistances et dégâts ci-dessus. |
| 4 | SP+ combat identique pour ces rows. |
| 5 | BKV mêmes lignes : level110, HP%=6500–6600, AC%=160/140/150, Crit10; `MonLvl`110 LHP9057/LAC2092, mais la table continue jusqu'à126 et possède les ratios120 détaillés J.1; résistances/dégâts BKV ci-dessus. |
| 6 | HP/def/CTH PD2 des tableaux J.1/J.2. |
| 7 | HP/def/CTH BKV des tableaux J.1/J.2. |
| 8 | Meph HP négatif; Diablo/Baal positifs mais <10 % selon roll; défense -2087/-1773/-1962. |
| 9 | HP -4,73…+9,17 %; défense environ -61…-63 %, non -25 %. |
| 10 | **Contredit par les données comme description S13/BKV.** Le changement historique peut avoir été vrai face à une ancienne baseline, mais pas comme merge courant. |
| 11 | **Softcode**, avec règles hardcode-backed de niveau, hit, block et ITD. |
| 12 | Faisable TXT sans natif pour HP/AC, mais un port niveau120 sélectionne la row BKV120 existante : recalibrage MonStats ou politique MonLvl explicite obligatoire. |
| 13 | Politique niveau120 et consommateurs de MonLvl120, clamp propre au core, ITD patch, Crit, résistances, skills H, block et catégories Crushing Blow distinctes. |
| 14 | Copier le row entier retire des immunités BKV et change fortement le danger élémentaire; -def peut raccourcir le combat; valeurs fixes PD2 suppriment le roll HP BKV. |
| 15 | **Adapter**, boss par boss. Ne pas merger le slogan +10/-25. |
| 16 | Futur possible : `monstats.txt`; `monlvl.txt` seulement si un changement global120 est explicitement accepté, sinon recalibrer les pourcentages contre la row BKV120. |
| 17 | HP min/max p1/p8, niveau110/120, defense/CTH niveaux90/99 AR5k/10k/20k, blockable/nonblockable, ITD on/off, résistances/immunités, chaque skill boss, TTK build physique/élémental/minions, host/joiner. |

## K. Diablo Clone

### K.1 Row courante et statistiques

Le DClone S13 courant est **`uberdiablonew`**, hcIdx789, ligne792. La row
`diabloclone` ligne335 subsiste comme legacy et ne doit pas servir à comparer
l'encounter actuel. BKVince utilise précisément cette legacy, hcIdx333.

| valeur Hell p1 | PD2 `uberdiablonew` | BKV `diabloclone` | delta |
|---|---:|---:|---:|
| niveau |110|110|0|
| HP% / `L-HP` |10530/10000|6427/9057|—|
| vie de table |**1 053 000**|**582 093**|+470 907/+80,90 %|
| défense |2940|4184|-1244/-29,73 %|
| A1;A2 |481–494;143–299|125–237;137–287|+356/+257; +6/+12|
| `Crit` |5 %|10 %|-5 points|
| résistances D/M/F/L/C/P |30/15/30/30/30/30|50/50/95/95/95/95|nettement plus faibles PD2|
| block |**15 %**|**50 %**|-35 points/-70 %|
| drain |5|15|-10|
| `ColdEffect` N/NM/H |**0/0/0**|-10/-10/-10|immunité seulement PD2|

La colonne contrôlante est exactement `MonStats.ToBlock` :15 sur
`uberdiablonew` ligne792 et50 sur `diabloclone` BKV ligne335. C'est la chance
data avant que le runtime détermine si l'attaque considérée est blockable.

`10530` et `6427` sont des pourcentages de base MonStats, non des HP finaux.
Le block ne s'applique qu'aux attaques que le moteur marque blockables; sorts,
DoT et attaques explicitement unblockable n'en bénéficient pas. Pour un hit
blockable déjà arrivé au jet de block, le proxy HP/(1-block) vaut 1 238 824
PD2 contre 1 164 186 BKV : la baisse de block ramène le surcroît défensif de
80,9 % HP à environ **+6,4 %**, avant défense/CTH et résistances.

Les tiers/overrides externes de l'encounter PD2 ne sont pas inclus : la valeur
ci-dessus est la baseline de table p1, pas une promesse de vie finale de chaque
tier DClone.

Les A1/A2 sont pré-Crit. L'espérance sémantique A1 vaut511,88 PD2 contre199,10
BKV (**+157,09 %**), tandis qu'A2 vaut232,05 contre233,20 (-0,49 %). La
comparaison comportementale est encore plus éloignée qu'une row numérique :
PD2 emploie AI `Trap-Melee`, huit skills `UberDiab*`/`MonTeleport`,
`Velocity/Run=4/8`, `aidel=35/35/35`, `aidist=128`; BKV emploie AI `Diablo`,
ses huit skills `Diab*`/`PrimeFirewall`/`Diablogeddon`, vitesse9/9 et
`aidel(H)=0`, `aidist(H)=46`. Aucun merge de row entière n'est défendable.

### K.2 Poison et malédictions

`MonStats.MonProp=uberdiablonew` pointe `MonProp.txt` ligne31. En Hell :

- `curse-res`, chance100, min=max **95**;
- `res-pois-len`, chance100, min=max **50**;
- `abs-fire`, chance100, min=max8.

`res-pois-len` suit `Properties.txt` ligne90, `func1=1`, vers
`ItemStatCost.txt` ligne112, stat `item_poisonlengthresist` ID110. Sous la
formule poison héritée, la durée restante est
`floor(durée×(100-50)/100) = floor(durée/2)` : **PLR50 %, pas75 %**; une
durée d'une frame peut donc devenir0. Cette route est distincte de l'arrondi
des curses ci-dessous. Un scan des usages de hcIdx789 n'a trouvé aucune
surcharge +25. La patch note/wiki annonçant75 est donc
**contredite par les données S13 disponibles**; seule une couche de tier non
localisée pourrait encore l'expliquer.

Cette valeur50 est l'entrée de la formule data avant toute borne du handler.
Le cap positif, le traitement d'un éventuel overcap et l'ordre exact des
arrondis du runtime PD2/D2R moderne ne sont pas directement prouvés : le
rapport ne transforme donc pas la formule héritée en preuve d'un cap S13.

Sous la sémantique D2MOO, `curse-res=95` applique
`durée - floor(durée×95/100)`, donc laisse `ceil(5 %)` et au moins **1 frame**
pour toute durée positive; il ne bloque pas la curse comme une valeur≥100.
La chaîne data exacte est `Properties.txt` ligne334 `curse-res`, `func1=1` →
`ItemStatCost.txt` ligne111 `curse_resistance`, ID109. L'ordre d'arrondi moderne
reste à valider. BKVince n'attache aucun MonProp à sa row DClone, donc ces deux
réductions ne sont pas présentes par cette route.

### K.3 Chill et Static Field

- Chill : les trois colonnes `ColdEffect`, `ColdEffect(N)`, `ColdEffect(H)` PD2
  valent0; l'immunité est **softcode propre à la row**, sans avoir besoin du
  package Prime Evil. BKV vaut-10 dans les trois et n'est pas équivalent.
- Static : la skill PD2 ID42 utilise un custom `srvdofunc=160`; les planchers
  généraux sont55/70/85 %. Season3 patch4 documente que Static ne réduit plus
  DClone, donc le résultat attendu est un **blocage complet**, non un seuil
  relevé. La garde hcIdx789 exacte n'a pas été retrouvée : exception DClone vs
  filtre Prime global demeure inconnue.
- BKV : plancher60/60/60, `srvdofunc=20`, aurafilter sans `IGNBOSS` ni
  `IGNPRIME`. `StaticFieldRework` actif appelle d'abord le handler natif
  `0x5546B0`, puis son debuff de résistance via curse `0x55D6B0` lorsque le
  natif réussit; le plugin ne contient aucune garde boss/prime/DClone. BKV ne
  reproduit donc pas l'immunité PD2.

### K.4 Fiche normalisée

| Nº | Fiche d'audit |
|---:|---|
| 1 | DClone non chillable; aucun dégât Static; base health10530 vs6427 LoD; block15 vs50; PLR75; curse reduction95. |
| 2 | [`Monsters` oldid23935, `Diablo-Clone`](https://wiki.projectdiablo2.com/w/index.php?title=Monsters&oldid=23935#Diablo-Clone), 20 juillet2026; S3 oldid17687 Static/block; S4 oldid17686 PLR/curse; baseHP ajoutée oldid20896. |
| 3 | Core MonStats792 cellules K.1, dont `ToBlock=15`, Crit5, AI/skills/vitesse; MonProp31 cellules K.2; skills42 srvdo160; DifficultyLevels StaticMin55/70/85; patch note Static. |
| 4 | SP+ stats combat identiques, mais lui seul ajoute `TreasureClass=UberDiablo`; le core laisse les TC du DClone courant vides. Ne pas importer ce drop comme core. |
| 5 | BKV MonStats335, dont `ToBlock=50`, Crit10 et AI/skills/vitesse distincts, MonProp vide; Static fields/plugin actifs décrits K.3. |
| 6 | PD2 table p1=1 053 000, block15, PLR50, curse95, ColdEffect0; Static entièrement bloqué selon documentation, handler exact inconnu. |
| 7 | BKV p1=582 093, block50, aucune réduction MonProp, ColdEffect-10; Static jusqu'au floor60 puis debuff plugin si succès. |
| 8 | HP+470907; def-1244; block-35 points; PLR+50 points par data; curse+95; Static booléen; résistances PD2 -20/-35/-65×4. |
| 9 | HP+80,90 %; def-29,73 %; block-70 %; PLR/curse n/a depuis absence. |
| 10 | Chill/baseHP/block/curse : **vrai**. PLR75 : **contredit par les données**, valeur50. Static : **vrai documenté**, route précise non prouvée. |
| 11 | HP/def/block/cold/MonProp : **softcode**. Static : **hardcode/custom skill**, possiblement exception native. |
| 12 | Toutes les stats sont portables en TXT; Static nécessite une garde avant le handler natif **et** avant le second étage StaticFieldRework, ou une politique plugin coordonnée. |
| 13 | Mapping DClone current/legacy, tiers, plugin Static actif, poison/curse stat semantics, attaques blockables, classification Prime/DClone. |
| 14 | Comparer/copier la mauvaise row et son AI/skills; doubler une vie déjà tier-scalée; bloquer seulement le premier étage Static; rendre le player Static inutilisable; sous-estimer TTK à cause du block/résistances. |
| 15 | **Adapter** les stats individuellement; **rejeter PLR75** tant qu'une preuve de couche externe n'existe pas; **reporter Static** jusqu'à garde92777 et intégration plugin. |
| 16 | Futur possible : `monstats.txt`, `monprop.txt`, éventuellement `properties/itemstatcost` si la stat manque; owner Static/plugin à décider. |
| 17 | Row réellement spawnée, tiers, HP p1/p8, block melee/ranged/spells, chill/freeze, poison durée/arrondis, curses courtes/longues, Static avant/au floor avec plugin on/off sans le désactiver en qualification finale, host/joiner. |

## L. The Countess

Le core crée une row `countess` hcIdx734 ligne737; BKVince utilise
`corruptrogue3` hcIdx45 ligne47. Les finales incluent UMod2 et le +3 niveaux.

| diff. | niveau core/BKV (brut/lookup/unité) | HP% core/BKV | finale p1 core/BKV | défense core/BKV | A1 |
|---|---|---|---|---:|---|
| Normal |8/8/11 · 8/7/10|67–112 / identique|72–120 / 60–100|40/42|3–7 /2–5|
| Nightmare |39/42/45 identique|60–100 / identique|1311–2187 / identique|570/570|19–38|
| Hell |69/85/88 identique|**180–300 /60–100**|**22254–37092 /7418–12364**|1568/3302|52–105|

Le ×3 est exact en Hell seulement, cellule par cellule et sur les deux bornes.
Après le mod fixe Fire Enchanted, les résistances sont0/0/75/0/0/0 en N/NM
et20/20/**108**/33/**130**/20 en Hell; Countess est donc fire+cold immune en
Hell dans les deux mods. Le supplément de feu du mod fixe ne fait pas partie
de l'A1 physique affichée.

L'offense n'est cependant pas identique : core→BKV donne `Crit 5→10`,
`Velocity/Run 6/10→8/12` et `aidel N/NM/H 15/14/13→15/7/0`. À A1 pré-Crit
égal en NM/H, BKV frappe donc avec une chance critique supérieure et une
cadence/mobilité différente; le ×3 HP PD2 est un buff de durée, pas une copie
fidèle de danger offensif.

Sa TC spéciale reste indépendante du HP. En p1, le core donne Key of Terror
avec probabilité7,1448 %, BKV18,1724 %. La Countess tire ses runes avant ses
items; augmenter le player factor peut consommer le cap six et ne garantit pas
une hausse monotone de la clé.

| Nº | Fiche d'audit |
|---:|---|
| 1 | Vie multipliée par trois. |
| 2 | [`Monsters` oldid23935, `Key Holders`](https://wiki.projectdiablo2.com/w/index.php?title=Monsters&oldid=23935#Key_Holders), 20 juillet2026; aucune patch note chiffrée trouvée. |
| 3 | Core MonStats737 : Hell MinHP/MaxHP180/300; MonLvl85 LHP6182; SuperUnique class/mods et TC Countess. |
| 4 | SP+ mêmes HP; ses taux de clé/runes diffèrent et ne sont pas core. |
| 5 | BKV MonStats47 Hell60/100, mêmes niveaux; def3302; TC p1 key18,1724 %. |
| 6 | Hell base11127–18546, finale22254–37092; N/NM tableau; key7,1448 %. |
| 7 | Hell base3709–6182, finale7418–12364; key18,1724 %. |
| 8 | Finale Hell+14836 à+24728; def-1734; key-11,028 points. |
| 9 | HP+200 %; def-52,51 %; key-60,68 %. |
| 10 | **Vrai mais incomplet : cellules ×3 uniquement en Hell.** Les finales sont dérivées sous §3; Normal diffère légèrement par lookup BKV gouverné, NM est un no-op. |
| 11 | **Softcode.** |
| 12 | HP directement faisable par TXT, sans toucher la TC spéciale. |
| 13 | UMod2, area level Normal patch, immunités des mods fixes, économie key/runes et défense BKV. |
| 14 | Copier la row entière réduirait la défense et le taux de clé; appliquer ×3 après UMod2 doublerait le multiplicateur; fire damage non compté dans A1. |
| 15 | **Adapter** seulement les cellules HP Hell si ce TTK est désiré; conserver les TCs BKV jusqu'à audit économique séparé. |
| 16 | Futur possible : `monstats.txt` seulement. |
| 17 | N/NM/H p1/p8, bornes inclusives, UMod2, deux immunités, random mods, key/rune/item distribution et cap six, durée de combat, host/joiner. |

## M. The Summoner

`summoner`, hcIdx250, ligne252 :

| diff. | niveau brut/lookup/unité | HP% core/BKV/vanilla | base p1 core/BKV | finale p1 core/BKV | défense core/BKV |
|---|---|---|---|---|---:|
| Normal |18/18/21|1032–1376 /387–516 /129–172|660–880 /247–330|2640–3520 /988–1320|165/165|
| Nightmare |55/55/58|880–1120 /330–420 /110–140|10322–13137 /3870–4926|30966–39411 /11610–14778|642/642|
| Hell |80/80/83|**880–1120 /330–420 /110–140**|**49341–62798 /18503–23549**|**98682–125596 /37006–47098**|1328/3126|

Le core vaut bien ×8 contre vanilla, mais BKVince possède déjà ×3; le vrai
merge PD2/BKV est **×2,6667**, soit +166,67 %. La plage wiki49 336–62 792 est
une ancienne approximation de la **base avant UMod2**; S13 actuel donne
49 341–62 798, puis double en Hell. A1/A2=0, block18 et résistances
0/0/75/75/75/0 sont identiques.

Les skills ne sont pas identiques malgré les mêmes niveaux : core
`MonGlacialSpike6`, `MonFrostNova5`, `MonFireBall5`, `VampireFirewall7`,
`MonWeaken4`; BKV utilise les versions joueur `Glacial Spike`, `Frost Nova`,
`Fire Ball`, `VampireFirewall`, `Weaken`. Core a Strong+Fast comme mods fixes;
BKV ajoute Aura Enchanted. Ces écarts peuvent dominer le danger offensif.
Core→BKV donne aussi `Crit 5→10`, `Velocity/Run 5/5→7/7` et
`aidel N/NM/H 15/13/10→15/6/0`. Même avec A1/A2 nuls, le handler critique peut
concerner des callsites skill/missile; la fréquence AI exacte reste à mesurer.

La TC BKV contient une anomalie indépendante : Summoner et Nihlathak pointent
tous deux `pk1` (Key of Terror) au lieu de `pk2`/`pk3`; le core cible bien
`pk2` pour Summoner. Taux p1 : core Hate Key12,8268 %, BKV Terror Key16,3552 %.
Le HP ne corrige pas cette anomalie.

| Nº | Fiche d'audit |
|---:|---|
| 1 | Vie multipliée par huit; plage49 336–62 792. |
| 2 | [`Monsters` oldid23935, `Key Holders`](https://wiki.projectdiablo2.com/w/index.php?title=Monsters&oldid=23935#Key_Holders), 20 juillet2026; aucune patch note chiffrée trouvée. |
| 3 | Core MonStats252 HP%=1032–1376 N,880–1120 NM/H; MonLvl values64/1173/5607; skill slots monster-only; SuperUnique UMod2. |
| 4 | SP+ combat identique; key weights SP+ distincts. |
| 5 | BKV MonStats252 HP%=387–516 N,330–420 NM/H; def3126 H; player skill names; Aura fixe; TC `pk1`. |
| 6 | Finales2640–3520/30966–39411/98682–125596; base Hell49341–62798; def1328. |
| 7 | Finales988–1320/11610–14778/37006–47098; def3126. |
| 8 | Hell finale+61676 à+78498; def-1798; base wiki actuelle+5/+6. |
| 9 | HP+166,67 % face BKV; def-57,52 %. |
| 10 | **Vrai face vanilla, trompeur comme delta BKV; plage wiki légèrement obsolète et non finale.** Les cellules HP sont prouvées dans les données PD2; les finales sont dérivées sous §3. |
| 11 | **Softcode avec adaptation des identités de skill.** |
| 12 | HP/def/slots faisables TXT; ne pas porter les ordinals ni remplacer les skills BKV sans comparaison formule par formule. |
| 13 | UMod2, skills monster/player, Aura Enchanted, key TC incorrecte BKV, random mods et player count. |
| 14 | TTK ×2,667 seulement à DPS constant; -def augmente la fiabilité des coups; retirer Aura peut réduire le danger; mauvaise clé demeure; plage affichée sous-estime×2. |
| 15 | **Adapter** : décider HP, défense et skills séparément; corriger la TC key dans un futur mandat économique distinct. |
| 16 | Futur possible : `monstats.txt`; éventuellement `skills.txt` seulement après audit des versions; `treasureclassex.txt` pour la key, hors merge HP. |
| 17 | N/NM/H p1/p8, base/finale UMod2, defense/CTH, chaque sort/dégât/cadence, Aura on/off, random mods, bonne clé et taux, durée combat, host/joiner. |

## N. Analyse des vrais buffs et nerfs

Les durées ci-dessous sont des **directions de TTK**, pas des chronométrages :
elles supposent un DPS constant sauf lorsqu'un témoin CTH explicite est donné.
Les skills, déplacements et immunités peuvent dominer ces ratios.

| cible/changement | vie/défense/résistances | attaque/skills/block | niveau/drop | TTK et danger joueur | mercenaires/summons | verdict gameplay face à BKVince |
|---|---|---|---|---|---|---|
| Andariel | HP+51,48 %, def-38,44 %, rés/block identiques | A1 pré-Crit+23–25 %, espérance+18,48 %; core plus lent, Crit5 vs10 | lvl75→85; pool Act4 mais essence100→11/23 % | barre beaucoup plus longue, mais BKV reste plus mobile/réactif; meilleure CTH joueur compense partiellement | PD2 core Andy n'est pas prime, BKV Andy l'est | **buff combat net probable**, comportement hybride à éviter, pas « +23/+10 » seulement |
| Duriel | HP/dégâts bruts/rés0; def-40 % | block identique; versions `Mon*`, Crit5, vitesse/AI plus lentes et prime0 contre skills joueur, Crit10, vitesse/AI BKV et prime1 | lvl88 no-op; Act3→Act5, essence100/40,7→8,7/9,8 % | plus facile à toucher et moins critique/mobile : **nerf combat probable**, skill formulas à comparer | PD2 core Duriel n'est pas prime, BKV l'est | ne merger que cellules approuvées; drop meilleur tier mais essence nerfée |
| sept Tombes | mobs -5 niveaux | dégâts/HP/def suivent MonLvl inférieur | bases normales bucket87→84 pour non-Wraith; ilvl/XP baissent | monstres ordinaires/champ/unique82/84/85 au lieu de87/89/90 | summons du joueur profitent aussi de cibles plus faibles | **régression BKV**, rejeter82 |
| Mephisto base | flying no-op | comportement moat déjà data-baké | TC non auditée comme merge ici | aucun gain du flag; pathing à tester | prime dans les deux | **déjà présent** |
| Council Durance | résistances/LI de base déjà présentes; vitesse/AI/skill/Crit/AC diffèrent | trois spawns beaucoup plus proches | graphes de drop hors conclusion de ce lot carte | aggro groupé, pression immédiate supérieure | merc/summons exposés simultanément | **buff encounter par carte**, isolable; ne pas qualifier la row complète de no-op |
| Uber Mephisto | HP-3,3 à-4,7 %, def-62 %, résistances bien moindres | A1 moyen Crit presque identique; Conviction résistances -125→-50 et défense -90→-65; CTH joueur monte | lvl110→120, mais ratios BKV120 différents si porté | moins de HP, plus facile à toucher, aura beaucoup moins punitive | éventuel ×4 Prime sur pets peut compenser | **nerf majeur du boss** hors règle pets |
| Uber Diablo | HP+7,5 à9,2 %, def-60,6 %, résistances bien moindres | A1/A2 moyens Crit quasi identiques; block identique; CTH joueur monte | lvl120 avec recalibrage requis | au témoin90/10k, TTK physique≈-1,7 %; haut AR peut devenir légèrement plus long | règles Prime augmenteraient le danger pets | **déplacement de difficulté**, souvent plus facile élémentaire |
| Uber Baal | HP+6,0 à7,6 %, def-62,5 %, résistances moindres | A1 moyen+46,9 %, A2+98,6 %; Lower Resist -75/20s remplace Defense Curse | lvl120 avec recalibrage requis | hits joueur plus fiables, mais melee/LR augmentent fortement le danger reçu | LR touche aussi pets éligibles; ×4 Prime possible | **mixte : boss offensivement buffé, mais plus facile à toucher/tuer** |
| DClone | HP+80,9 %, def-29,7 %, résistances -20 à-65 points | A1 moyen+157 %, A2 quasi égal; block50→15, chill/Static immune, curse95, PLR50; AI/skills entièrement distincts | TC core vide; TC SP+ exclue | HP/(1-block) seulement+6,4 % avant CTH; Static/curse rallongent certains builds, A1 punit davantage | prime; pets potentiellement très punis | **nouvel encounter à adapter**, pas une row/stat isolée; plus facile aux hits/éléments |
| Nihlathak | HP Hell×3; core plus lent, Crit5 et AI différente | exposition prolongée à CE; BKV Crit10/vitesse9/Aura fixe absents core | key core pk3 vs anomalie BKV pk1 | TTK HP≈×3 à DPS constant, mais cadence/danger ne suivent pas ce seul ratio | compagnons restent plus longtemps près des corps, tandis que l'offense BKV peut être plus vive | **buff de durée Hell**, danger offensif mixte; N/NM no-op HP seulement |
| Countess | HP Hell×3, def-52,5 %, doubles immunités identiques | A1 brut égal H mais core Crit5/vitesse/AI moindres; Fire Enchanted | key7,14 % vs18,17 % BKV | TTK augmente moins que×3 si AR bas; aucun buff HP NM, Normal diffère | combat prolongé sous mods random, offense BKV plus vive | **buff HP Hell, baisse défense/drop et offense partiellement nerfée** |
| Summoner | HP×2,667 face BKV, def-57,5 %, résistances identiques | versions monster skills core; Crit5/vitesse/AI plus faibles; BKV Aura fixe | mauvaise key pk1 BKV indépendante | plus longue barre, mais bien plus facile à toucher; danger spell dépend des skills/cadence | Aura/curse/AoE modifient pets | **buff de durée probable, danger offensif mixte à adapter** |
| Dolls classiques | BKV aucune explosion configurée sur les sept rows | PD2 Range25 projeté à25f sous handlers hérités, r4, phys18–30/54–96/318–540 | n/a | nouveau spike vraisemblablement retardé et télégraphié | source/ownership modernes à confirmer; summons potentiellement touchables | **réintroduction probable d'un danger**, pas un nerf BKV; handler gate requis |
| Aura Enchanted | poids BKV Hell×2, pool inconnu | PD2 pool8, aura lvl1–13 | dépend mlvl | Holy Shock ajoute danger de proximité | auras touchent groupe/pets selon cible | **non comparable avant sélecteur92777** |
| Prime : dégâts compagnons | aucune hausse de HP | PD2 total×2 hireling/×4 `ISREVIVE` sous sémantique héritée; BKV configure×1,5/×2 | n/a | aucun delta direct contre le joueur | delta théorique +33,33 % merc et +100 % pets face à la configuration BKV, à valider sur92777 | **buff Prime probable contre compagnons**, ownership et consommateurs natifs à prouver avant adaptation |
| Prime : item slow/Decrepify/Holy Freeze | stats de base inchangées | item slow/Decrep directement protégés; Holy Freeze sous décodage hérité; Slow Movement donne un contre-exemple fort | n/a | boss plus stable dans les chemins confirmés | compagnons perdent une partie de leur contrôle | **buff défensif partiel**, immunité universelle non prouvée |
| Prime : Dim Vision/Terror/Confuse/Attract | aucune | protection par classe/AI switch inférée; aucun guard `primeevil` directement prouvé | n/a | aucun effet Prime additionnel démontré | prime-minions ordinaires ne doivent pas être supposés immunes | **causalité non prouvée**; ne pas créer une nouvelle immunité sous prétexte du wiki |
| Prime : Sanctuary | résistance physique d'un undead Prime n'est pas forcée à zéro | dégâts/aura/bonus undead inchangés; seule la suppression de résistance est exclue | n/a | builds Sanctuary physiques perdent leur bypass | mêmes limites pour attaques de compagnons sous Sanctuary | **buff défensif ciblé PD2**, absent/non prouvé BKV, route native |

Deux conséquences transversales empêchent un verdict purement numérique :

1. BKVince marque Andariel et Duriel `primeevil=1`, contrairement au core PD2.
   Conserver ce flag tout en important les stats PD2 crée un hybride plus
   dangereux pour les compagnons que le boss PD2 correspondant.
2. Les résistances BKV des Ubers sont beaucoup plus hautes que celles du core.
   Un HP supérieur dans PD2 ne signifie donc pas automatiquement un TTK
   supérieur pour un build élémentaire.

## O. Classification canonique à fournir au futur combat

### O.1 Contrat recommandé

| classification | source canonique recommandée | pourquoi |
|---|---|---|
| `Ordinary` | flags runtime d'unité, absence champion/unique/super/boss | état de spawn, pas identité MonStats |
| `Champion` | type/UMod runtime champion | un même row peut spawn ordinary ou champion |
| `Unique` | type/UMod runtime unique | distinct de `boss` et SuperUnique |
| `Superunique` | flag/index runtime + `SuperUniques.txt` | identifie la variante effectivement spawnée |
| `ActBoss` | **sidecar TXT par `MonStats.Id`**, résolu en ID runtime au chargement | aucun champ existant ne distingue exactement les cinq |
| `ApexBoss` | sidecar | DClone/Rathma/Lucion sont des familles d'encounter, pas un flag natif |
| `PrimeEvilRules` | **`MonStats.primeevil`, après politique et migration explicites des flags BKV** | flag natif exact, mais les 15 rows BKV incluent Baal Clone et omettent Uber Izual face au core; l'état courant ne doit pas être canonisé silencieusement |
| `RiftBoss` | inconnu, puis sidecar seulement après définition | `TreasureClassHerald` n'est pas une identité de boss |
| `ColossalAncient` | sidecar avec mapping PD2/BKV | IDs/keys diffèrent entre engines |
| `Uber` | sidecar | trio d'identité exact, non déductible de `boss+prime` |
| `MiniUber` | sidecar | trio distinct; Uber Izual prouve que `primeevil` BKV ne suffit pas |
| `DiabloClone` | sidecar | row courante PD2 et legacy BKV portent des IDs différents |

Le sidecar proposé est une **recommandation de source de données**, pas un
fichier créé par cet audit. Il devrait être keyed par `MonStats.Id`, puis
résoudre et mettre en cache l'ID numérique de la table active au chargement.
Ainsi, la DLL consommatrice demande un bitset de classification et ne compile
aucune allowlist de noms. Une clé absente doit fail-closed et produire un log,
pas être devinée par le lore.

Avant de rendre `PrimeEvilRules` stable, une politique BKV doit décider
explicitement du cas Uber Izual (`primeevil=0` BKV,1 core), de Baal Clone
(`1` BKV,0 core) et d'Andariel/Duriel (`1` BKV,0 core). Réutiliser la colonne
est recommandé; sanctuariser ses quinze valeurs actuelles sans décision ne
l'est pas.

`ApexBoss` doit désigner uniquement l'unité principale de l'encounter :
`uberdiablonew`, `rathmaBone`, `rathmaPoison` et `Lucion` dans la taxonomie S13
observée. Les contrôleurs, spawners et minions de ces familles peuvent conserver
`PrimeEvilRules`, mais ne deviennent pas `ApexBoss`; si un futur besoin exige de
les regrouper, il faut un bit séparé `EncounterMember`. `RiftBoss` reste
**explicitement non résolu** dans cet audit : aucun bit ne doit être émis avant
qu'une définition produit et ses rows canoniques soient approuvées. Le contrat O
est donc complet pour les sources démontrées, mais volontairement partiel sur
`RiftBoss`.

### O.2 Règles de séparation

- `boss` reste utile comme classe moteur, mais ne signifie pas ActBoss : le
  core a115 rows boss.
- `primeevil` doit rester séparé de `boss` : Andariel/Duriel core prouvent
  `boss=1, prime=0`; de nombreux minions prouvent `boss=0, prime=1`.
- `CrushingBlowTier` est un axe de politique de combat séparé. Il peut grouper
  ordinary/superunique/Prime/map boss selon une formule sans redéfinir leur
  identité canonique.
- Les variantes BKV custom rendent le sidecar nécessaire; ne pas détourner
  une colonne de Treasure Class, `NameStr` ou un flag de collision.
- `ConfigurableCharsiReward` montre déjà qu'une DLL peut lire dynamiquement
  `boss=1` dans la table active; son allowlist vanilla de secours est un témoin
  à ne pas reproduire pour les nouvelles classifications.

### O.3 API conceptuelle, sans implantation

```text
MonsterClassifications resolve(monstatsId, runtimeFlags, superuniqueIndex)
  dynamic: Ordinary | Champion | Unique | Superunique
  identity: ActBoss | ApexBoss | RiftBoss | ColossalAncient |
            Uber | MiniUber | DiabloClone
  rules: PrimeEvilRules
  policy (separate query): CrushingBlowTier
```

Le futur `BKVCombat.dll` ne doit démarrer qu'après gel de ce contrat et ne doit
pas en être le propriétaire : il le consomme. L'audit ne choisit pas encore si
le futur owner natif sera `BKVCombat.dll`, un éventuel MonsterRules ou une
autre façade partagée.

## P. Matrice finale de faisabilité

Catégories : **1** merge direct sûr; **2** softcode avec adaptation BKV;
**3** modification de carte; **4** hybride TXT+natif; **5** logique native
séparée; **6** déjà présent; **7** wiki trompeur/non prouvé; **8** déconseillé.

| Changement | confirmé PD2 | déjà BKVince | softcode | map edit | natif | dépendance | risque | recommandation |
|---|---|---|---|---|---|---|---|---|
| Holy Shock Aura Enchanted, pool8, lvl1–13 | code S13 | aura oui, pool inconnu | poids/exclusions | non | sélecteur | row keys + ordinals + RE92777 | haut | **4 — reporter puis adapter** |
| Dolls Range25→25f/r4/dégâts fixes | data S13 + sémantique handlers héritée | explosions classiques désactivées en data | chaîne MonProp/skill/missile | non | funcs à prouver | ordinals/event/owner | haut | **4 — décision gameplay puis adaptation** |
| `primeevil` comme axe de règles | 149 rows data | 15 rows | oui | non | consommateurs | contrat O | moyen | **2 — conserver axe, pas copier 149 rows** |
| taxonomies Act/Apex/Rift/Colossal/Uber/Mini/DClone | groupes partiels | non structurées | sidecar futur | non | loader requis | définition Rift + politique flags | moyen | **4 — hybride sidecar+loader, Rift reporté** |
| dégâts Prime merc×2/pet×4 | inférence forte | 1,5/2 configurés via DifficultyLevels; effectivité à valider | multiplicateurs | non | target flags/ownership | classification | haut | **4 — adapter après tests** |
| item Slow Target immune | code S13 | non, seulement caps | non | non | oui | EventFunc19 | haut | **5 — logique ciblée** |
| Decrepify immune | code S13 direct | non prouvé | non suffisant | non | guards | états/RE92777 | haut | **5 — logique native ciblée** |
| Holy Freeze immune | filtre S13 + décodage hérité | non prouvé | filtre HF partiel | non | équivalence bit à confirmer | aurafilter92777 | haut | **4 — hybride après preuve du bit** |
| « tous les slows » | universel non prouvé; contre-exemple fort sous sémantique héritée | non | non | non | aucun package identifié | — | haut | **7 — rejeter l'énoncé comme spécification** |
| Dim/Terror/Confuse/Attract par Prime | causalité Prime non prouvée; protection classe/AI inférée | classe/AI protège certains | filtres | non | AI switch | catégorie unique/SU | moyen | **7 — ne pas implanter comme règle Prime** |
| Sanctuary conserve phys-res Prime | code S13 | absent/non prouvé | non suffisant | non | resolver phys | RE92777 | haut | **5 — reporter** |
| Andariel lvl85/HP/melee | données | non | oui | non | non | choix balance | moyen | **2 — adapter cellules choisies** |
| Andariel pool Meph | graphe core | non; BKV q/tes100 % | oui, graphe | non | quest predicate hardcode-backed | économie | haut | **2 — simuler/adapt­er** |
| Duriel lvl88/HP/A1-A2 bruts | données | **oui** | n/a | non | non | skills/AI/Crit/prime distincts | faible pour ces cellules seules | **6 — ne rien faire sur ces cellules** |
| Duriel défense basse | données | non | oui | non | non | CTH | moyen | **2 — décision séparée** |
| Duriel pool Baal | graphe core | non | oui, graphe | non | quest hardcode-backed | essences/Act tiers | haut | **2 — adapter, pas copier** |
| sept Tombes lvl82 | données | BKV87 | oui | non | non | ilvl/TC/XP | haut | **8 — rejeter comme régression** |
| Mephisto flying | données | **oui** | n/a | non | pathing backé | runtime | faible | **6 — déjà présent** |
| Council rapprochés | DS1 core | non | non | **oui** | non | MonPreset IDs | moyen | **3 — map edit ciblé** |
| Durance Council lightning immune | données | **oui** | n/a | non | cap immunity backé | UMods | faible | **6 — déjà présent** |
| Council acquiert une immunité additionnelle via UMods | résistances/mods data; cap de deux inféré du handler hérité | mêmes résistances, mods fixes différents | résistances/UMods | non | cap unique à confirmer | handler92777 + seeds | moyen | **4 — données présentes, valider le cap natif** |
| Uber Meph Conviction20 -50 | code+data | level20 mais -125 | skill partiel | non | dispatcher | skill monster-only | haut | **4 — adapter** |
| Uber Baal LR -75/20s | données; sélection AI inférée | Defense Curse | skill+slot | non | AI à confirmer | état/Skill6/RE92777 | haut | **4 — hybride conditionnel après test AI** |
| Nihlathak HP×3 Hell | données | non | oui | non | non | Aura/CE | moyen | **2 — merge sélectif possible** |
| slogan Ubers +10HP/-25def | contredit comme delta S13/BKV courant | valeurs différentes | n/a | non | non | baseline historique inconnue | haut | **7 — rejeter le slogan comme merge** |
| recalibrage S13 des trois Ubers, boss par boss | données S13 exactes | non | MonStats | non | lookup niveau hardcode-backed | row BKV MonLvl120/ITD/res/Crit | haut | **2 — softcode avec adaptation BKV** |
| DClone HP10530/block15/cold0/curse95 | données | non | oui | non | non | row current/legacy | haut | **2 — adapter individuellement** |
| DClone PLR75 | **contredit, data=50** | absent | n/a | non | override non trouvé | tiers éventuels | moyen | **7 — rejeter75** |
| DClone PLR50 S13 | données MonProp; cap runtime inconnu | absent | oui | non | stat handler hardcode-backed | cap/arrondis92777 | moyen | **2 — adapter seulement après politique PLR** |
| DClone immune Static | wiki fort/custom skill | non, plugin actif | non | non | oui | StaticFieldRework | très haut | **5 — reporter** |
| Countess HP×3 Hell | données | non | oui | non | non | def/key TC | moyen | **2 — cellules HP seulement** |
| Summoner HP×8 vanilla/×2,667 BKV | données | BKV déjà×3 vanilla | oui | non | non | skills/aura/key | moyen-haut | **2 — adapter** |
| clés Summoner/Nihl `pk1` BKV | anomalie comparative | oui | oui | non | non | économie séparée | haut | **2 — correctif économique distinct après audit** |

Aucun changement demandé ne mérite un « merge direct sûr » aveugle : les seuls
lots à faible risque sont déjà présents. La carte Council est le changement le
plus isolable; les HP Nihl/Countess sont les softcodes les plus simples, mais
restent des décisions d'équilibrage et non des corrections techniques.

## Q. Ordre d'implantation recommandé

L'ordre proposé par le mandat est globalement correct, avec deux ajustements :
les anomalies de baseline doivent être gelées avant calcul, et les Treasure
Classes doivent recevoir leur propre gate économique avant les skills.

1. **Geler la baseline et le contrat de classification.** Épingler le SHA de
   baseline approuvé et revalider les rows après chaque lane concurrente;
   adopter les sources O pour Ordinary…DClone;
   définir RiftBoss ou le laisser explicitement inconnu; séparer
   `PrimeEvilRules` de `CrushingBlowTier`.
2. **Décider les deltas numériques, row par row.** Andariel, Nihlathak,
   Countess, Summoner, Ubers et DClone; enregistrer les no-op Duriel/Meph/
   Council LI et rejeter Tomb82. Pour tout Uber porté à120, recalculer contre
   la row BKV `MonLvl120` au lieu de supposer le clamp PD2 à110. Ne modifier
   que les cellules approuvées.
3. **Traiter niveau de zone et économie comme lot indépendant.** Simuler les
   graphes TC D2R avec cap six, quest state, p1–p8, essences et keys; corriger
   éventuellement l'anomalie `pk1` séparément. Ne pas transposer la disposition
   des colonnes legacy.
4. **Stabiliser les identités de skills de boss.** Créer, si approuvé, des
   row keys monster-only propres avec ordinals runtime gouvernés; Uber Baal
   Lower Resist avant Uber Mephisto, car Baal peut réutiliser Skill6 alors que
   Meph exige le dispatcher.
5. **Importer la carte Council comme lot autonome.** Un seul DS1, trois
   coordonnées, hashes de couches inchangés, avec rollback par retrait du seul
   override.
6. **Fermer les chaînes Aura Enchanted et Dolls sur 92777.** Identifier le pool
   D2R, les funcs event/skill/missile compatibles, les ordinals et l'ownership;
   seulement ensuite décider si TXT suffit ou si un adaptateur natif minimal
   est requis.
7. **Implanter les règles Prime softcodables séparément.** Flags choisis,
   `ColdEffect`, filtres démontrés et multiplicateurs `DifficultyLevels`, sans
   prétendre obtenir toutes les règles par le flag seul.
8. **Implanter les règles Prime natives une par une.** Item slow, Decrepify,
   Sanctuary, ownership des dégâts, puis DClone Static en coordination avec
   `StaticFieldRework`. Les quatre AI curses ne deviennent pas une règle Prime
   sans nouvelle preuve/volonté produit.
9. **Seulement alors ouvrir le chantier combat.** Le futur consommateur demande
   la classification stable et les politiques validées; il ne contient ni
   liste de noms ni taxonomie implicite. Le choix entre `BKVCombat.dll` et un
   autre propriétaire demeure hors mandat et devra repasser par le gate
   d'incubation plugin.

### Gates de sortie avant toute DLL de combat

- source PD2 core/SP+ explicitement séparée pour chaque cellule;
- aucune collision de row key ni d'ordinal runtime skill/state/missile/property;
- sidecar/classification validé contre toutes les rows custom actives;
- simulations TC reproductibles et politique d'essence/key approuvée;
- carte isolée et réversible;
- preuves 92777 signatures/xrefs/ABI pour chaque handler natif réellement
  retenu;
- tests solo et host/joiner, les cinq DLL du PluginPack chargées et **toutes les
  fonctionnalités du pack activées**, sans désactiver aucun autre plugin actif;
- sauvegarde/rechargement et rollback byte-exact vérifié séparément pour chaque
  lot de données, carte ou logique native;
- aucune assertion « wiki » utilisée comme substitut d'une preuve données/code.

## Crédits et attribution des sources

- **Project Diablo 2 Team** : données core S13, `ProjectDiablo.dll`, cartes,
  wiki officiel et patch notes épinglés;
- **Lukaszpg** :
  [`PD2-Single-Player-Plus-mod`](https://github.com/Lukaszpg/PD2-Single-Player-Plus-mod),
  snapshot SP+ épinglé et toujours distingué du core;
- **D2MOO / The Phrozen Keep** :
  [référence sémantique 1.10f](https://github.com/ThePhrozenKeep/D2MOO/tree/19019806df7f3e877fa105b05395d1e3597e2316),
  jamais utilisée comme preuve d'adresse ou d'ABI PD2/D2R;
- **RuffnecKk** : workspace BKVince, workbench 3.2.92777, scripts gouvernés et
  consolidation de l'audit.
