# MeleeSplash.dll v0.1 public — D2R 3.2.92777

> **STATUT GOUVERNÉ — VERSION 0.1.0 LIVRÉE DEFAULT-OFF**
>
> Destination confirmée par Vincent le 9 août 2026 : plugin autonome permanent,
> public et générique pour D2RLoader. La version `0.1.0` compile de façon
> reproductible, sa qualification technique globale/mod-locale est consignée
> et son ZIP public strict est audité. Tous les critères de livraison demandés
> sont fermés. Le premier smoke a découvert un caller Fill manquant; la
> correction gouvernée admet maintenant les deux continuations melee exactes.
> Le smoke nominal passe avec un burst sur trois cibles secondaires. B, C et D
> passent ensuite avec les stats exacts 391/392. Le rollback legacy de G a
> exposé une assertion native; Vincent a donc retiré définitivement l'ancien
> splash BKVince au lieu de le conserver comme fallback.

Auteur : `RuffnecKk`

Configuration publique : `MeleeSplash.json`, **default-off**

Build exclusif initial : `D2R.exe 3.2.92777`

## Preuve statique du candidat — 9 août 2026

Deux builds propres Release x64 du même snapshot ont réussi avec `/W4 /WX` et
CTest `1/1`. Le build gouverné et le package staged sont byte-identiques :

```text
MeleeSplash.dll
199168 bytes
DBA0C40C191B2568A6B39D21324A45F770C1CBF8AD747B099AA3BCBEDEF8856C
```

L'inspection PE confirme une DLL AMD64, l'API D2RLoader v2, exactement les trois
exports attendus, la version `0.1.0`, l'auteur `RuffnecKk`, aucun chemin PDB et
aucune dépendance, chaîne ou ID BKVince/eezstreet obligatoire. Les configurations
générique reste default-off tandis que le profil BKVince est actif; le test ciblé des IDs 391/392,
properties 310/311, textes 65028/65029 et de la politique TSV/configuration
réussit.

L'audit d'écriture ne trouve aucun chevauchement exact avec les 139 sites du
manifeste PluginPack, les 57 sites de patches BKVince, l'observateur
FloatingDamage ou les addons du workspace dont les surfaces sont visibles.
Cette preuve ferme le prérequis statique pour le workspace courant; elle ne
qualifie ni une DLL black-box ou future, ni la pile runtime complète. Le cold
start et le ZIP sont fermés séparément ci-dessous; seule la matrice solo A–H
reste préparée sans résultat gameplay inféré.

## Qualification runtime technique — 9 août 2026

Trois cold starts frais de l'artefact initial
`D9A49607C0BA7EFF2E52200ED480EF8381739161F4E4B2BDE066D548554921B0`
ferment le chargement technique sous D2R 3.2.92777 :

- portée globale activée : MeleeSplash actif, `18/18` patches, `17/17`
  plugins actifs, `24/24`, zéro disabled/rejected/failed;
- portée mod-locale activée avec doublon global : la copie mod-locale prend
  correctement priorité, la copie globale est l'unique disabled attendu,
  `18/18` patches, 17 plugins effectifs actifs, `24/24`, zéro rejet/échec;
- état final mod-local default-off : `MeleeSplash.dll` charge sans installer de
  hook, `18/18` patches, `17/17` plugins actifs, `24/24`, zéro rejet/échec.

Le build corrigé `DBA0C40C...8856C` a ensuite passé un cold start mod-local
activé avec `18/18`, `17/17`, `24/24` et zéro disabled/rejected/failed avant le
smoke réussi. La DLL source, package, dépôt BKVince et runtime mod-local sont
byte-identiques sur ce hash corrigé.
La baseline technique default-off historique reste archivée. La configuration
BKVince gouvernée est désormais active et sans gate; les copies globales
temporaires de la DLL et du JSON restent retirées.

Le cold start final sur les sept tables où l'ancien splash est retiré passe
avec `18/18` patches, `17/17` plugins, `24/24` et zéro
disabled/rejected/failed. QtyTester sans stat 384 produit une capture
`gateSeen=false`, sans EventFunc20 ni ancienne assertion. La configuration
active silencieuse source/runtime vaut SHA-256
`0A7B1878C6D20CE3362F9B95055B7DBF9E56EDEC81C24001E2B88A400017802D`.

Cette qualification prouve la coexistence avec la pile installée active, pas
avec toute DLL future ou toute combinaison de fonctionnalités. La matrice
formelle « toutes fonctionnalités PluginPack actives » demeure bloquée par trois
conflits antérieurs étrangers à MeleeSplash : ownership `0x589736`, composition
`0x314110` et rel32 `plugin-misc` `0x18885B/0x18887F`. Aucun composant de la pile
active n'a été retiré ou neutralisé pour ces essais; seule la copie globale
dupliquée de MeleeSplash a été shadowed automatiquement lors du test mod-local.

## Package public strict — 9 août 2026

`addons/MeleeSplash/MeleeSplash-0.1.0.zip` contient exactement deux entrées à
la racine :

| Entrée | Taille | SHA-256 |
|---|---:|---|
| `MeleeSplash.dll` | 199168 | `DBA0C40C191B2568A6B39D21324A45F770C1CBF8AD747B099AA3BCBEDEF8856C` |
| `MeleeSplash.json` | 601 | `6AA40B37051189ADE2CA5D60FE133765EE1D426E0B6DA5E2059B619E77030C20` |

Le JSON archivé conserve `enabled=false`. Aucun README, source, symbole, log,
fichier BKVince ou binaire tiers n'est inclus. SHA-256 du ZIP :

```text
F137F1B708A4C51C8A88EA68B49BFB85619F380693E63899B508EB1C342E35A9
```

## Décision active et frontière

La v0.1 implante uniquement le melee splash. Elle ne devient pas propriétaire
des formules générales Critical/Deadly, Crushing Blow, Open Wounds ou
résistances et ne sélectionne pas `Pd2CombatCore`. Elle doit déléguer aux
mécanismes actuellement autoritaires du jeu ou du mod hôte afin de consommer
leurs remplacements futurs. L'exception démontrée est Critical/Deadly : aucun
resolver callable n'existe sous 92777, donc la politique per-target utilise un
adaptateur isolé jusqu'à l'arrivée d'un provider commun explicite.

La conclusion MEC-01A `LIKELY_FRAGMENTED` autorise une composition de plusieurs
coutures internes strictement validées, pas la promotion d'une couture commune.
Chaque hook doit vérifier le build, la signature, les octets attendus, l'ABI et
la propriété unique du site, puis échouer fermé au moindre mismatch. Seul le
reverse engineering ciblé requis par une surface précise de la v0.1 est ouvert.

## Portée officielle v0.1

Supporté initialement :

- D2R 3.2.92777 sous D2RLoader;
- offline/local single-player;
- joueur contre monstres;
- attaque normale et attaques melee admissibles filtrables par skill ID;
- capture du skill ID réellement responsable du coup.

Hors portée ou non testé :

- multijoueur et PvP;
- mercenaires, summons et monstres comme attaquants;
- skills multi-hit non explicitement autorisés;
- hits utilisant la conversion physique native inline, rejetés fail-closed par
  la v0.1 tant qu'un adaptateur ciblé n'est pas prouvé;
- tout autre build D2R.

## Contrat gameplay v0.1

Le splash est serveur, instantané à 360 degrés, sans missile ni Next Hit Delay.
Il exclut la cible principale et partage un seul paquet offensif de base déjà
roulé, incluant les composantes physiques, magiques et élémentaires autorisées.
Pour chaque cible secondaire, il rejoue séparément la séquence actuellement
autoritaire Critical puis Deadly, recalcule résistances, réductions et absorb,
puis laisse Crushing Blow et Open Wounds recevoir eux aussi leurs propres jets.
Le leech splash vaut la moitié du leech normal.

Cette décision per-target prise par Vincent le 9 août 2026 remplace la demande
initiale de partager le résultat Critical/Deadly. La v0.1 isole ce replay dans
un adaptateur natif 92777 : elle ne devient pas propriétaire des futurs caps ou
multiplicateurs PD2, et devra consommer leur resolver autoritaire lorsqu'il sera
livré dans le lot Critical/Deadly suivant. D2R 92777 n'expose actuellement aucun
resolver callable et son bit aval ne distingue pas Critical de Deadly : ce futur
branchement exigera donc un provider commun explicite et une recompilation de la
DLL, pas une détection automatique prétendue par la v0.1.

Une garde thread-local marque toute résolution synthétique afin d'interdire la
récursion. La v0.1 ne répète ni durabilité ni thorns et n'ajoute pas stun,
knockback, blind, freeze, slow, Prevent Monster Heal, missiles secondaires ou
CTC offensifs génériques exclus. Les cibles sont dédupliquées par type/GUID;
attacker et primary sont exclus, aucun pointeur d'unité n'est conservé à long
terme et chaque cible est re-résolue avant application.

## Configuration et séparation BKVince

Le JSON public expose `enabled`, les modes `allEligibleMelee`, `whitelist` et
`blacklist`, le contrôle explicite de l'attaque normale, les listes de skill
IDs, le gate stat optionnel, les stats optionnels de rayon et de pourcentage de
dégâts, les rayons de base, le cap facultatif, le diagnostic et les overrides
par skill. Les valeurs invalides doivent être refusées proprement ou remplacées
par des valeurs sûres documentées. Le fichier générique livré utilise
`"enabled": false`; le profil BKVince séparé peut l'activer.

La DLL ne contient aucun nom, chemin ou ID BKVince obligatoire. Le profil
BKVince active toutes les attaques melee joueur admissibles sans gate, réserve
les IDs collision-safe de `inc_splash_radius` et
`item_melee_splash_damage_percent`, et n'emploie plus EventFunc20. Le vieux
montage est retiré des tables : stat 384/property 302, skills 430/432, missile
743 et state 242 conservent seulement les données minimales nécessaires à la
stabilité des IDs; les références Summon Splash et Titan's Echo sont retirées.
Désactiver ou retirer la DLL produit désormais « aucun splash », jamais un
retour au missile historique.

## Gates et livrables v0.1

- [x] source Release x64, tests automatisables et `MeleeSplash.dll` 0.1.0;
- [x] `MeleeSplash.json` générique default-off et exemple documenté;
- [x] README public, options, CHANGELOG et rapport court hooks/signatures;
- [x] package public strict limité à la DLL et au JSON;
- [x] profil BKVince séparé, deux stats/propriétés/textes collision-safe et IDs documentés;
- [x] vieux splash BKVince retiré avec tombstones ID-stables et migrateur idempotent;
- [x] audit statique de coexistence PluginPack, FloatingDamage et patches actifs;
- [x] cold starts techniques global, mod-local shadow et rollback default-off
  avec la pile actuellement installée;
- [x] smoke test solo A–H préparé : autorisé, exclu, rayon +20/+40, dégâts +50,
  DS/OW/CB 100 %, cible primaire unique, ancien missile absent et aucune récursion;
- [x] build, hashes, configuration et logs automatisables consignés sans inférer
  de résultat runtime voisin.

Le premier smoke solo du 9 août a ouvert un **NO-GO gameplay** puis fourni sa
cause exacte : Attack appelle Fill à `0x4300B6` et revient à `0x4300BB`, tandis
que le plugin n'admettait que `0x44B6A0`. Le contexte de 29 octets est unique et
la chaîne 92777 mène le même record vers Prepare/Allocate/Consume. Après
correction, le coup réussi journalise une capture puis un burst sur trois
secondaires. Chaque cible reçoit un roll Critical/Deadly et exactement un appel
CB et OW réussi; la primaire est rejetée de l'aire et la profondeur de récursion
revient à zéro. A, E, F, H et G-actif passent. B passe par exclusion de
l'attaque normale; C passe avec `inc_splash_radius=20/40` (rayons finaux 6/7)
et D avec `item_melee_splash_damage_percent=50` (150 %). Le retour default-off
de G a atteint l'ancien chemin puis déclenché l'assertion native
`ptSkill->nItemEffect != 0`; cette preuve a motivé son retrait définitif plutôt
qu'un correctif du montage obsolète. La compatibilité formelle de toutes
les fonctionnalités simultanément actives reste bloquée par la baseline
préexistante ci-dessus.

Le backlog suivant est verrouillé : Critical/Deadly, Crushing Blow/CBE, Open
Wounds/DPS plat, résistances hybrides, puis itemisation et équilibrage global.
Aucune de ces quatre refontes générales n'est implantée dans la v0.1.

## Archive historique — prototype du 8 août 2026

> **Supersession du 9 août 2026 :** les passages historiques ci-dessous qui
> promettent le retour automatique d'EventFunc20 ne gouvernent plus BKVince.
> L'ancien montage est retiré; ils sont conservés uniquement comme trace du
> prototype et de la décision remplacée.

Le bloc ci-dessous conserve le prototype précédent comme inventaire
d'hypothèses non probantes. Ses hashes décrivent la baseline au moment du gel;
ils ne sont pas les hashes attendus de la nouvelle v0.1 et tout drift doit être
traité comme une nouvelle implantation à prouver.

> **STATUT HISTORIQUE — PROTOTYPE GAMEPLAY EN QUARANTAINE, NON PROBANT**
>
> Ce prototype et l'implantation qu'il décrit sont gelés comme inventaire
> d'hypothèses pour MEC-00/MEC-01. Aucune architecture melee splash commune,
> aucun `Pd2CombatCore` et aucune destination de plugin ne sont sélectionnés.
> Jusqu'à la clôture probatoire de MEC-00/MEC-01, aucun build, déploiement,
> test gameplay, publication ni promotion dans `known-rvas.json` provenant de
> ce prototype n'est autorisé. Ses RVA, signatures, layouts, offsets, callers,
> callees et ABI doivent tous être revérifiés indépendamment dans l'atelier
> gouverné D2R 3.2.92777. Le cold start consigné ci-dessous prouve seulement le
> chargement de la pile observée; il ne prouve ni la correction fonctionnelle,
> ni l'autorité serveur, ni la viabilité d'une couture commune.
>
> Les mentions historiques « Destination » et « État » ci-dessous décrivent
> l'intention et l'avancement antérieurs du prototype; elles ne constituent
> plus des décisions d'architecture ou de livraison actives.

Statut gouverné de la baseline historique : **QUARANTAINE MEC-00/MEC-01 — NON PROBANT**

Date de décision : 8 août 2026
Destination historique : plugin autonome permanent `MeleeSplash.dll`
Auteur : `RuffnecKk`
État historique : implantation native compilée; qualification runtime suspendue

### Manifeste byte-exact de quarantaine

Les hashes suivants figent les artefacts gameplay au moment du gel. Tout drift
interdit leur emploi comme matériel de comparaison avant une nouvelle décision
explicite. Le présent document n'est pas auto-hashé, car l'ajout du manifeste
modifierait nécessairement son propre hash.

| Artefact | SHA-256 |
|---|---|
| `addons/MeleeSplash/README.md` | `BE2EBF53EB0F9A8824C4909435B09C041C57FAF323B8DC0AE0F787D0708BE595` |
| `addons/MeleeSplash/src/CMakeLists.txt` | `CBC016500C41EAE801D4F237A71C9DD83A33D93E9A87FB2FD4C3A98102EEAAAD` |
| `addons/MeleeSplash/src/plugin.cpp` | `0AFD26024281C31694149D91A13E45040A55640FAC5D436623852784D89A1B17` |
| `addons/MeleeSplash/src/plugin.rc` | `D2AEEABAC2687C56690E64A2677B0B1A211A74F524A922A89A9AC6595C141C9C` |
| `data-BKVince/d2rloader/plugins/MeleeSplash.dll` | `6960BEB1D5F480E77DE05F18D312536F9FE165AE3A3C61FD6CDAD4AE5D664B07` |

### Mission historique

Remplacer le missile de dégâts melee splash de BKVince par une reproduction
serveur native du comportement Project Diablo 2 Season 13. Le stat BKVince
`item_splashonhit` ID 384 reste uniquement le gate de distribution. La skill
430 et le missile 743 ne doivent jamais appartenir au nouveau mécanisme de
dégâts.

Au moment du prototype, la DLL était conçue comme toujours active, sans
`enabled`, sans configuration, sans solution softcode, sans coefficient de
remplacement et sans fallback gameplay. Elle est
hybride globale/mod-locale, ne déclare pas `ModScopedOnly` et ne modifie, ne lie
ni ne redistribue aucune DLL eezstreet. Vincent a confirmé la destination
autonome et l'absence de configuration dans la conversation historique; cette
décision est désormais supersédée par la quarantaine MEC.

### Sémantique PD2 S13 relevée historiquement

L'oracle local est `ProjectDiablo.dll` S13, SHA-256
`538a77b7ccef3d5334e56c4e9e57a4d8fc69a1e27c46beb694c0dedfcfbf9cb3`.
Les rapports locaux non versionnés se trouvent sous
`analysis-cache/pd2-s13-melee-splash/`.

- Le trigger est un événement `domeleedamage`; aucun missile ne distribue les
  dégâts.
- Le centre est la cible primaire, qui est explicitement exclue.
- Chaque cible secondaire reconstruit et résout ses dégâts indépendamment,
  incluant RNG, critical/deadly, crushing blow, open wounds et résistances.
- Aucun jet d'attack rating secondaire n'a lieu. Le comportement S13 historique
  de block/dodge/avoid par cible reste une observation de l'oracle, mais ne fait
  pas partie de la portée publique v0.1 retenue ci-dessus : le succès du hit
  primaire est partagé et seules résistances, réductions et absorb sont
  recalculés pour chaque cible secondaire.
- Le rayon S13 installé vaut 5, moins 1 pour une arme active normal-tier.
  BKVince n'a pas le stat PD2 478, donc aucun terme softcode correspondant
  n'existe à importer.
- Il n'y a ni ligne de vue ni plafond de cibles ni coefficient de dégâts.
- Le life leech est divisé par deux, avec minimum 1 lorsqu'il était non nul;
  mana et stamina leech ne le sont pas ici.
- Knockback, open wounds, crushing blow et slow restent éligibles. EventFunc14
  item-freeze et tous les EventFunc20 sont filtrés pendant le secondaire.
- Smite possède une construction shield damage dédiée. Dragon Talon, Dragon
  Tail et Dragon Flight repartent du record de combat primaire.

### BKVince avant remplacement

- `itemstatcost.txt`: `item_splashonhit`, ID 384, `domeleedamage`, EventFunc20;
- `properties.txt`: propriété `splash` vers le stat 384;
- `skills.txt`: `Splash` ID 430, SrvDoFunc 8, missile
  `proc_splashdamage`;
- `missiles.txt`: missile 743, `SrcDamage=64`, donc 50 %;
- `Summon Splash` ID 432 distribue également le stat 384.

L'implantation intercepte exclusivement le mot haut ID 384 du token EventFunc20
`(statId << 16) | layer`, relit la valeur sur le layer reçu et ne rappelle
jamais l'EventFunc20 original pour ce stat. Le stat 379 `hit_skill_splash` est
distinct et reste entièrement délégué à D2R.

### Couture et ABI revendiquées par le prototype

Les trois hooks propriétaires sont :

| RVA | Rôle | ABI |
|---:|---|---|
| `0x44CE80` | exécution serveur des événements de dégâts | `(game, attacker, defender, bMissile, damage)` |
| `0x583580` | EventFunc14 | sept arguments d'événement |
| `0x583B30` | EventFunc20 | sept arguments d'événement; token stat/layer en arg6 |

Les signatures strictes d'entrée sont uniques dans l'image gouvernée 92777.
Les plages sont libres contre les 139 sites du manifeste PluginPack audité et
les 57 patches JSON actifs de BKVince. La DLL pose ses hooks en pass-through,
puis publie son état opérationnel seulement lorsque les trois sont installés.

Primitives natives principales appelées par la composition :

| RVA | Responsabilité |
|---:|---|
| `0x4398B0` | énumération spatiale serveur, ABI dix arguments |
| `0x48E060` | permission de dégâts attacker/candidate |
| `0x44C030` | construction weapon/item/all-components |
| `0x437D00` | roll élémentaire de la source skill |
| `0x438090` | roll physique de la source skill |
| `0x44B930` | block/dodge/avoid par cible |
| `0x5889E0` | critical monstre post-défense observé dans PD2 |
| `0x44DF10` | résistances, immunités, pierce, absorb et total |
| `0x44CE80` | dégâts/événements mêlée et HP |
| `0x44A9B0` | modes, réactions, mort et packets |
| `0x4494B0` / `0x4496E0` | copie profonde et destruction `D2Damage` |

`D2Damage` mesure `0x180` octets sous 92777. Une instance fraîche respecte son
état interne natif; une kick utilise obligatoirement le copy constructor. Aucun
`memcpy` de ce record complexe n'est permis.

### Chaîne secondaire historique

1. EventFunc20 constate le stat 384 sur le layer courant et marque le trigger
   TLS avant l'énumération.
2. L'énumérateur part de la cible primaire avec le masque structurel `0x583`;
   le callback applique `CanDamageTarget` avec le véritable attaquant.
3. Le callback construit un `D2Damage` neuf, ou copie profondément le combat
   primaire pour une kick.
4. Ordre exact : `FillDamageValues`, roll élémentaire, roll physique.
5. Le life leech est divisé par deux.
6. `ApplyBlockOrDodge(game, attacker, defender, 1, 1)` est résolu par cible;
   toute défense enlève `SUCCESS` et produit les flags PD2 correspondants.
7. Le critical monstre post-défense est appelé même après une défense.
8. Sur succès uniquement : `CalculateTotalDamage`, puis
   `ExecuteEvents(..., bMissile=0)` sous filtre TLS.
9. `FinalizeDamage` est toujours appelé afin de produire les réactions natives.
10. Le destructeur natif libère exactement une fois le record.

La chaîne n'alloue aucun combat secondaire, ne refait pas la portée ou l'AR,
ne déclenche pas l'usure, les thorns ou les événements skill-on-attack et ne
pré-calcule pas `WILLDIE`. `ExecuteEvents` décide la mort après la mutation
réelle des HP.

### Spécificités historiques de namespace BKVince

La whitelist de formule ED conserve les IDs vanilla identiques à PD2. Les IDs
PD2 custom 378/380 ne sont pas transposés : BKVince les attribue à Blood Oath
et Blood Boil, et appliquer leurs formules par simple égalité numérique serait
une collision de namespace, pas une reproduction de Joust/Blade Dance.

Le code ne conserve pas la skill missile 430 comme source cachée. Pour un joueur,
la source est sa skill réellement utilisée. Pour un attaquant non joueur ou un
contexte sans skill utilisable, la ligne vanilla neutre `Attack` ID 0 alimente
les deux rollers natifs : ses champs de dégâts sont vides comme ceux de la skill
interne PD2, mais elle conserve l'ordre RNG et le marquage source du `D2Damage`
sans dépendre d'une ligne softcode de proc. Cette traduction respecte la
contrainte selon laquelle seul le stat 384 de l'ancien montage peut survivre.

### Validation historique

| Gate | État | Preuve attendue |
|---|---|---|
| Release x64 `/W4 /WX` | passed | compilation MSVC sans avertissement |
| exports/manifest/version | passed | 3 exports exacts, manifeste v2, version 1.0.0 |
| imports | passed | aucune DLL eezstreet, aucun fichier de config |
| cold start pile complète | passed | hooks `0x44CE80/0x583580/0x583B30`; 17 actifs, 0 désactivé/rejeté/échoué; 18/18 patches |
| neutralisation missile | not run | aucune création/trajectoire du missile 743 |
| dégâts secondaires natifs | not run | HP, réactions, morts et compteurs cohérents |
| filtres/procs | not run | freeze/CTC supprimés; CB/OW/KB/slow conservés |
| rayon normal/excep/elite | not run | respectivement 4/5/5 |
| Smite et kicks | not run | aucun double combat/usure; dégâts reconstruits |
| solo/hôte/client | not run | autorité serveur et packets cohérents |

Un cold start ne constitue pas à lui seul une preuve fonctionnelle. Aucune
archive publique ne sera déclarée livrable tant que les cases gameplay
pertinentes n'auront pas été observées.

Cold start du 8 août 2026 à 20:22, profil mod-local BKVince installé sous
`C:/Games/Diablo II Resurrected/mods/BKVince/`. Le build, la copie gouvernée et
la copie runtime avaient le même SHA-256
`6960beb1d5f480e77de05f18d312536f9fe165ae3a3c61fd6cdad4ae5d664b07`.
Le log frais `d2rloader.log` prouve les trois installations de hooks, puis
`Plugin loading complete: scanned=17 active=17 disabled=0 rejected=0 failed=0`
et `Memory patch loading complete: scanned=18 applied=18 disabled=0 failed=0`.

### Rollback historique

Retirer uniquement `MeleeSplash.dll` du dossier D2RLoader concerné après arrêt
des processus. Aucun TXT n'est migré et aucune sauvegarde n'est modifiée par
l'installation. Le retrait rétablit automatiquement l'ancien EventFunc20 du
stat 384, puisque la neutralisation existe uniquement dans les hooks de la DLL.
