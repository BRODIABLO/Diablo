# Mission courante

Dernière mise à jour : 10 août 2026

## Priorité active

[BKVCombat 0.1 — Release 1 Damage Core](bkvcombat-0.1.md)

État : **build/package et coexistence current-stack terminés; gameplay NOT RUN**.
Vincent a confirmé une DLL RuffnecKk autonome permanente, hybride
global/mod-local et compatible par contrat avec PluginPack et les autres plugins
du workspace. La configuration reste `default-off` et chaque politique peut être
activée indépendamment.

La DLL candidate de 215 552 octets vaut SHA-256
`3EFCEB7374E26207FE603FF5AC43DAFBC8246E85C37426B62D0AEF1F38663D50`; le build
Release x64 et CTest `1/1` passent. La base courante descend du Monster Merge.
Crushing Blow applique la priorité `MajorBoss > PrimeEvil > Elite > Ordinary`;
le marqueur Herald/Ascendant appartient à `Elite` et le profil BKVince réserve
la CBE à la stat `393`. Open Wounds supporte trois stacks, puis les rafraîchit
tous au cap dans le contrat offline/local mono-source. Life/Mana Steal conserve
et valide le baseline natif sans hook général.

Le ZIP public strict vaut SHA-256
`A6E89B7B4B8723704A44F95386AB841A6ABD4AD9C2C27003EE61A4B90331BE24`.
Les cold starts default-off, policies-on et ordre inverse exact atteignent
`19/19` plugins, `15/15` patchsets et `24/24`; le profil final est mod-local,
default-off et aucun processus D2R ne reste actif. La matrice universelle reste
NO-GO parce que plusieurs fonctionnalités PluginPack sont désactivées dans la
baseline et que les incidents render préexistants `dxgi/plugin-items` et
`PopcornUber` ne sont pas fermés.

## Prochain gate

Exécuter la matrice solo : Critical/Deadly, les quatre classes CB,
ranged/player-count/CBE, les trois stacks OW, Life/Mana Steal et Life Tap. Le
premier hit doit aussi prouver la négociation lazy MeleeSplash→BKVCombat et
l’absence de double CB/OW. Reprendre la matrice universelle seulement après
fermeture des fonctions PluginPack désactivées et des incidents render, sans
neutraliser de composant tiers. Aucun succès gameplay n’est encore revendiqué.

## Frontière Git active

La mission active couvre `addons/BKVCombat/**`, son profil BKVince, les données
CBE, les preuves natives, la mission, la ROADMAP, le workstream et le cadastre.
Les autres changements du workspace restent hors périmètre et sont préservés.
Aucun commit ni push n’est effectué sans demande explicite de Vincent.

## Priorité précédente conservée — ProgressiveAffixesPlugin

[ProgressiveAffixesPlugin autonome — D2R 3.2.92777](progressive-affixes-plugin-3.2.md)

État : **TOML joueur v0.2.0 et double cold start terminés, qualification gameplay ouverte**.
Vincent avait placé le plugin en option B dans la phase itemisation, puis a
donné le 10 août 2026 la directive explicite `Implemente`. Cette directive ouvre
le lot immédiatement sans modifier la destination confirmée : plugin autonome
permanent RuffnecKk, hybride global/mod-local, TOML indépendant et aucune
mutation d’une DLL eezstreet.

La DLL Release x64 et le TOML joueur PD2-inspired sont présents dans `addons/` et dans le profil
source BKVince. Les trois anciens patchsets force Magic/Rare/Crafted sont retirés
atomiquement. Deux clean builds `/Brepro` produisent la même DLL de 161 280
octets, SHA-256
`F88386D2839E996880F1C9EFBEE7891E8CF4CCADCAC109C65EE2F8B70671FC7C`.
La suite CTest passe `1/1`; les métadonnées, trois exports D2RLoader et hashes
build/package/BKVince sont cohérents.

Le TOML expose maintenant quatre sections simples et des pourcentages lisibles.
Les Rare Jewels suivent `50/50`, `37.5/62.5`, `25/75`, puis `0/100` pour trois
ou quatre affixes aux ilvl 1/45/65/85. Le format avancé v0.1.0 reste accepté
sans pouvoir être mélangé au format joueur.

Les trois patchsets runtime obsolètes ont été archivés hors des dossiers actifs.
Deux cold starts frais passent sur le build 92777 : la portée mod-locale puis la
portée globale avec son propre repli de configuration chargent chacune
ProgressiveAffixesPlugin 0.2.0. Chaque démarrage conserve la pile de production
et termine avec `18/18` plugins actifs, `15/15` patchsets appliqués, zéro rejet
et zéro échec. La portée globale de test a été retirée et le profil BKVince
mod-local restauré avec les hashes source/runtime exacts; une seule instance
mod-locale stable a été relancée.

### Gates encore ouverts de ProgressiveAffixesPlugin

Produire les témoins gameplay qui déclenchent la résolution tardive des codes
`weap`, `armo`, `jewl`, `ring`, `amul`, `char`, puis vérifier les bornes et les
distributions réelles.

La qualification complète exige ensuite :

- toutes les fonctionnalités du PluginPack actives et aucun composant
  neutralisé ;
- deux ordres pertinents pour toute surface composable ;
- bornes Magic 64/65, jewelry 84/85 et charms 89/90 ;
- distributions Rare aux ilvl 1/45/65/85, y compris la progression 3/4 des Rare Jewels ;
- distributions Crafted aux ilvl 1/31/51/71 ;
- sauvegarde/rechargement et hôte/joiner.

Les conflits full-stack préexistants `0x589736`, `0x314110` et
`0x18885B/0x18887F` restent étrangers au plugin et bloquent toute affirmation de
compatibilité universelle tant qu’ils ne sont pas fermés sans désactiver de
fonctionnalité.

### Frontière Git historique de ProgressiveAffixesPlugin

La mission couvre `addons/ProgressiveAffixesPlugin/**`, son profil BKVince, la
mission, les preuves RE, la ROADMAP, le cadastre et l’archive publique stricte.
Les autres changements du workspace restent hors périmètre et sont préservés.
Aucun commit ni push n’est effectué sans demande explicite de Vincent.
