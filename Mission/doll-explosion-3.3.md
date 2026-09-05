# DollExplosion — explosion configurable des Stygian Dolls

Dernière mise à jour : 5 septembre 2026

## Statut

**P1 — implantation et qualification statiques terminées.** Les hooks, le TOML,
les tests et deux builds reproductibles sont fermés. La qualification runtime
P2, le packaging, la promotion Suite et toute release restent non exécutés.

## Décisions produit

- Le 4 septembre 2026, Vincent donne `GO` à la création d'un plugin consacré à
  l'explosion à la mort des Dolls.
- Le 5 septembre 2026, Vincent retient le séquencement **B** : ouvrir maintenant
  une incubation autonome, selon l'ordre preuve native → implantation →
  qualification, sans rouvrir la release `v1.3.2`.
- Le produit est une DLL indépendante de la **RuffnecKk D2RLoader Suite**,
  versionnée et archivée séparément. Elle ne sera jamais mergée, liée ou
  redistribuée dans une DLL d'eezstreet.
- L'installation doit fonctionner globalement et dans le dossier d'un mod,
  sans `ModScopedOnly` et avec la même empreinte native fail-closed.
- La version d'incubation prévue est `0.1.0`. Une future release Suite reste à
  décider après qualification; DollExplosion n'entre pas dans le registre
  `v1.3.3` pendant P0.
- Le fichier `ruffneckk-doll-explosion.toml` est justifié par trois réglages moddeur
  réels : délai, rayon et formule de dégâts. Son contenu et ses commentaires
  seront entièrement en anglais. La présence de la DLL constitue l'activation;
  aucun booléen `enabled` global n'est ajouté.

Description publique prévue :

> Delays Stygian Doll death blasts and makes their physical damage configurable.

Auteur des métadonnées : `RuffnecKk`.

## Faits vérifiés

### Modèle PD2

L'audit gouverné `Mission/pd2-monsters-bosses-vs-bkvince-audit.md` établit la
chaîne de données PD2 S13 suivante : propriété `death-skill` à 100 %, skill
`DollMeteor`, niveaux 1/2/10 selon la difficulté, rayon 4 et dégâts physiques
fixes 18–30 / 54–96 / 318–540. Le missile `dollmeteorcenter` porte `Range=25`
et `AlwaysExplode=1`.

La formule par défaut du plugin est donc :

| Difficulté | Dégâts physiques |
|---|---:|
| Normal | 18–30 |
| Nightmare | 54–96 |
| Hell | 318–540 |

Le délai de 25 frames est une dérivation forte de la donnée missile PD2, pas
une preuve issue du désassemblage de `ProjectDiablo.dll`. Aucun code ni asset
PD2 n'est copié.

### Vanilla D2R et D2MOO

Le corpus natif commun D2R 3.2.92777 / 3.3.93847 montre la branche vanilla qui
crée `monstercorpseexplode`, lit `MonsterCEDamagePercent`, tire un dommage
physique entre 60 % et 100 % du plafond calculé et appelle l'AoE native avec un
rayon 5 et les flags `0x581`. Les bornes exactes, l'ABI et les signatures de
cette route sont maintenant fermés statiquement dans le corpus gouverné.

D2MOO 1.10f confirme uniquement cette sémantique historique. Aucune adresse,
structure ou ABI 32 bits n'est transposée vers D2R. Le README créditera
explicitement D2MOO et distinguera le crédit de conception PD2.

### Cibles BKVince

Les sept Dolls classiques BKVince sont `bonefetish1..7`, MonStats `hcIdx`
212, 213, 214, 215, 216, 690 et 691. Elles ont `deathDmg` vide. La Doll custom
de Rift `bonefetish8`, ID 777, conserve `deathDmg=1` et reste exclue par défaut.
La documentation héritée qui parlait de « huit classiques » est corrigée sans
modifier les tables.

## Architecture retenue

La couture initialement envisagée à `0x44535F` est rejetée : les sept Dolls
classiques ont `deathDmg` vide et le guard natif à `0x445045` saute donc cette
route avant l'appel. Une DLL installée uniquement là serait inerte pour les
cibles qu'elle prétend corriger.

La DLL possède plutôt l'entrée complète du callback serveur de mort
`0x444F50`, d'ABI `(game, modeChange) -> int32`. Elle capture uniquement les
données minimales d'une cible configurée, appelle l'original exactement une
fois, puis planifie l'explosion après son retour. Elle ignore les unités en état
Revive 96 ainsi que toute cible dont le record compilé porte déjà `deathDmg`,
ce qui prévient la double explosion et conserve notamment le comportement de la
Doll Rift 777.

Un transporteur natif temporisé, invisible et identifié par un sidecar borné,
porte le délai. Une seconde couture sur le gestionnaire générique `0x466B40`
appelle d'abord l'original. Tant que celui-ci ne retourne pas 2, le missile et
son sidecar restent en attente. Lorsqu'il retourne 2 à l'expiration, la DLL
crée le visuel `monstercorpseexplode` ID 117, applique le dommage physique
configuré avec l'AoE native, puis laisse le dispatcher retirer le transporteur.
Toute unité ou tout missile non revendiqué délègue exactement à l'original.
Les événements Lifecycle `GameJoined` et `GameLeft` vident le sidecar afin
qu'un transporteur supprimé par la destruction complète d'une partie ne puisse
pas consommer un slot dans la session suivante.

Le transporteur retenu est `baalcorpseexplodedelay` (Missiles ID 587). Sa ligne
est byte-identique dans les tables vanilla 3.2, vanilla 3.3 et BKVince :
`pSrvDo=1`, aucun `pSrvHit`, `Range=10`, aucun CelFile et aucun dommage serveur.
Contrairement à `armageddoncontrol` ID 577, il ne peut donc pas produire un
effet offensif étranger si le sidecar est absent ou si la DLL est déchargée.
Avant chaque première utilisation d'un contexte de données, la DLL exige que le
record compilé conserve `pSrvDo=1` et `pSrvHit=0`, que la table native du
dispatcher associe encore l'entrée 1 au handler basique `0x455750`, puis que
l'instance créée soit un missile classe 587 avec une durée totale et un
compteur courant tous deux initialisés à 10. Toute divergence supprime proprement l'explosion
custom de l'événement; le transporteur lui-même demeure inoffensif.

L'analyse a prouvé :

1. le callback de mort `0x444F50`, son unité à `modeChange+8`, son guard
   `deathDmg` à `MonStats+0x3E` et sa route de création à onze arguments vers
   `0x4333F0`;
2. les helpers d'identité, de coordonnées, d'état Revive, de difficulté, de
   statistiques et de RNG consommés par le plugin;
3. les offsets d'instance missile `Unit+0x10`, durée totale `+0x10` et compteur
   courant flottant `+0x14`, avec témoins séparés et quatre getters/setters
   natifs fingerprintés pour régler les deux valeurs ensemble;
4. le dispatcher `pSrvDo` (`MissilesTxt+0x2C`) et son retrait natif quand le
   callback retourne 2;
5. l'ABI `(game, missile) -> int32` du gestionnaire `0x466B40`, dont les deux
   xrefs natifs sont inventoriés;
6. l'initialisation du record `D2Damage` de 0x180 octets, le physique à `+0x18`,
   le rayon et l'appel AoE `0x44A120`, puis le destructeur `0x4496E0`;
7. l'absence de propriétaire concurrent de ces deux coutures dans les sources
   RuffnecKk, les patches BKVince et la référence eezstreet épinglée.

Le plugin n'accrochera pas `MONSTERMODE_EventHandler 0x447420`, déjà possédé
par Burn Damage Fix dans la baseline auditée, et ne possédera pas
`srvdofunc169`.

Si l'une de ces surfaces reste fragmentée ou possède déjà un propriétaire non
composable, l'implantation s'arrête et revient devant Vincent au lieu d'ajouter
un second hook concurrent.

## Contrat TOML prévu

```toml
config_version = 1

[targets]
# Classic BKVince Dolls. The Rift Doll (777) keeps its existing behavior.
monstats_ids = [212, 213, 214, 215, 216, 690, 691]

[explosion]
# D2R runs at 25 simulation frames per second.
delay_frames = 25
radius = 4

[damage]
# Supported values: "fixed", "source_max_life_percent"
formula = "fixed"

[damage.fixed]
normal = [18, 30]
nightmare = [54, 96]
hell = [318, 540]

[damage.source_max_life_percent]
normal = [30, 50]
nightmare = [21, 35]
hell = [12, 20]

[diagnostics]
show_usage_counters = false
```

`fixed` est le défaut PD2. `source_max_life_percent` offre une formule structurée et
bornée; aucun langage d'expressions arbitraires n'est prévu. Le dommage demeure
physique dans `0.1.0`. Le délai est borné à `0..32767` frames par la largeur
native prouvée et le rayon à `1..64`. Les minima doivent être inférieurs ou
égaux aux maxima et tout risque d'overflow fixed-point refuse le chargement.

La résolution cherche d'abord le TOML du mod actif, puis celui de la portée de
chargement, puis le TOML global, en dédupliquant les chemins identiques. Un
fichier absent utilise les valeurs PD2 embarquées et est matérialisé lorsque
possible. Le premier fichier présent mais invalide refuse le chargement; aucun
repli silencieux vers un autre fichier n'est permis.

## Gates P0 — preuve native fermée

- `npm run baseline:d2rloader -- status` : baseline promue seulement;
- `npm run re:d2r33 -- status` : image et index gouvernés vérifiés;
- `npm run ref:d2rlplugins -- status` : pin propre et exact;
- borner les deux coutures et leurs fonctions contenant chaque RVA;
- inventorier tous leurs xrefs et propriétaires connus;
- prouver ABI, structures, flags, RNG, fixed-point, rayon et lifecycle;
- construire une empreinte fail-closed sans build-name, canal, version ni hash
  global du PE comme décision de chargement;
- promouvoir les identifications stables dans `known-rvas.json` et garder les
  hypothèses fragmentées dans les findings de cette mission.

## Implantation P1 — terminée statiquement

- `addons/DollExplosion/src/CMakeLists.txt`;
- DLL D2RLoader x64, manifeste PluginSDK API 3 et trois exports attendus;
- parseur TOML strict, politique pure et tests unitaires;
- empreinte native et ownership avant le premier hook;
- deux hooks d'entrée D2RLoader avec trampolines originaux;
- `README.md`, `NATIVE-CONTRACT.md` et `VALIDATION.md`;
- configuration publique `ruffneckk-doll-explosion.toml`.

Le pin PluginSDK minimal sera choisi à partir des capabilities réellement
utilisées; aucune migration vers API v4 n'est présumée.

## Matrice de qualification

1. Deux builds Release x64 `/W4 /WX` reproductibles, CTest vert, version PE,
   manifeste et exports cohérents.
2. Tests de configuration absente, mod-locale, globale, priorité, clés
   inconnues, valeurs hors bornes, min/max inversés et overflow.
3. Empreinte complète valide sur identité diagnostique connue ou inconnue;
   refus avant hook pour correspondance absente, partielle ou ambiguë.
4. Les sept Dolls classiques, la Doll Rift 777 inchangée, Normal/Nightmare/Hell,
   limites intérieure/extérieure du rayon et délai frame-exact.
5. Mort melee, ranged, spell, DoT, thorns et splash; aucune double explosion;
   unités ressuscitées, corps consommé, Find Item et Redemption.
6. Résistance et réduction physiques, player count, solo et TCP/IP hôte/joiner,
   télégraphe et autorité serveur.
7. Portées globale et mod-locale, deux ordres pertinents, pile Suite complète et
   cinq plugins eezstreet avec toutes leurs fonctionnalités actives.
8. Save & Exit avec missiles en attente, nouvelle partie, fin de partie et
   unload sans état persistant ni crash.

Le runtime officiellement qualifié sera D2R 3.3.93847. Le build 3.2.92777 ne
sera couvert que si toutes les surfaces employées restent byte-identiques dans
le corpus gouverné. Steam 3.3.93787 demeure admissible mais non qualifié tant
que ses surfaces exactes ne sont pas prouvées.

## Livraison, promotion et rollback

Aucune promotion Suite, entrée de registre de release ou archive publique n'est
faite avant qualification. Une fois retenu, le plugin passera par la promotion
gouvernée vers le dépôt produit avant tout statut `package-ready`.

Le ZIP généré par l'agent contiendra uniquement la DLL et le TOML. Le README
restera à côté pour relecture humaine avant ajout manuel. Le rollback consiste
à retirer la DLL et son TOML; aucune table, sauvegarde ou donnée persistante
n'est migrée.

## Frontière Git

Cette mission couvre `Mission/doll-explosion-3.3.md`,
`addons/DollExplosion/**` et les preuves natives DollExplosion dédiées. La
ROADMAP, le registre des workstreams, le cadastre et les registres natifs sont
partagés. Préserver tous les changements étrangers; ne jamais commit ni push
sans demande explicite de Vincent.
