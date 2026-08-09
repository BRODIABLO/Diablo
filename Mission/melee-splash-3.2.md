# Melee Splash natif — D2R 3.2.92777

> **STATUT GOUVERNÉ — PROTOTYPE GAMEPLAY EN QUARANTAINE, NON PROBANT**
>
> Ce document et l'implantation qu'il décrit sont gelés comme inventaire
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

Statut gouverné courant : **QUARANTAINE MEC-00/MEC-01 — NON PROBANT**

Date de décision : 8 août 2026
Destination historique : plugin autonome permanent `MeleeSplash.dll`
Auteur : `RuffnecKk`
État historique : implantation native compilée; qualification runtime suspendue

## Manifeste byte-exact de quarantaine

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

## Mission

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

## Sémantique PD2 S13 prouvée

L'oracle local est `ProjectDiablo.dll` S13, SHA-256
`538a77b7ccef3d5334e56c4e9e57a4d8fc69a1e27c46beb694c0dedfcfbf9cb3`.
Les rapports locaux non versionnés se trouvent sous
`analysis-cache/pd2-s13-melee-splash/`.

- Le trigger est un événement `domeleedamage`; aucun missile ne distribue les
  dégâts.
- Le centre est la cible primaire, qui est explicitement exclue.
- Chaque cible secondaire reconstruit et résout ses dégâts indépendamment,
  incluant RNG, critical/deadly, crushing blow, open wounds et résistances.
- Aucun jet d'attack rating secondaire n'a lieu; block/dodge/avoid reste par
  cible.
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

## BKVince avant remplacement

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

## Couture et ABI D2R 92777

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

## Chaîne secondaire implantée

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

## Spécificités de namespace BKVince

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

## Validation

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

## Rollback

Retirer uniquement `MeleeSplash.dll` du dossier D2RLoader concerné après arrêt
des processus. Aucun TXT n'est migré et aucune sauvegarde n'est modifiée par
l'installation. Le retrait rétablit automatiquement l'ancien EventFunc20 du
stat 384, puisque la neutralisation existe uniquement dans les hooks de la DLL.
