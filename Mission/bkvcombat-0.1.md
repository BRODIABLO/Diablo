# BKVCombat 0.1 — Release 1 Damage Core

Dernière mise à jour : 15 août 2026

## Décision et état

Vincent a confirmé `BKVCombat.dll` comme plugin **autonome permanent** RuffnecKk,
hybride global/mod-local, avec configuration JSON indépendante. La Release 1
regroupe six politiques activables séparément : Critical Strike, Deadly Strike,
Crushing Blow, Life Steal, Mana Steal et Open Wounds. Les résistances, les drops
et le véritable melee splash restent hors de ce lot.

État actuel : **code, build, package et coexistence current-stack terminés;
gameplay NOT RUN**. Le plugin reste `default-off`. Deux ordres de chargement
atteignent `19/19` plugins, `15/15` patchsets et `24/24`, mais la compatibilité
universelle demeure refusée : plusieurs fonctions PluginPack sont désactivées
dans la baseline et deux incidents render préexistants restent visibles.

La base Git constatée avant cette gouvernance est le HEAD
`756df5f53109729f16643b36aa459fead4cdbf94`, descendant du jalon Monster Merge
`8bbb252e86ed3d9c21a3a7f77644b296a6979c15`. Les classifications de combat sont
donc consommées après le Monster Merge; BKVCombat ne reprend pas la propriété
des règles universelles de ralentissement décidées dans ce merge.

## Contrat Release 1

- Critical Strike : cap 75 %, multiplicateur physique ×2,0.
- Deadly Strike : cap 75 %, multiplicateur physique ×1,5, testé seulement si
  Critical échoue.
- Crushing Blow : classification data-driven selon la priorité
  `MajorBoss > PrimeEvil > Elite > Ordinary`, scaling par le nombre de joueurs,
  pénalité ranged et résistance physique. Les Heralds et Ascendants, reconnus
  par le marqueur runtime Herald/ghostly actuel, appartiennent à `Elite`.
- Crushing Blow Efficiency : stat public configurable; le profil BKVince
  réserve définitivement `item_crushingblow_efficiency` à l’ID `393`, sans cap
  supérieur inventé.
- Open Wounds : durée de cinq secondes, maximum de trois stacks; au cap, les
  trois stacks sont rafraîchis. Le contrat initial est offline/local et
  mono-source par cible, avec résistance physique et quart des dégâts contre
  mercenaires ou pets.
- Life Steal et Mana Steal : conservation du baseline natif autoritaire
  équivalent au modèle PD2 pour difficultés et `Drain`; BKVCombat le valide
  mais n’installe aucun hook général de leech.

Les fractions Crushing Blow sont :

| Classe | Melee | Ranged |
|---|---:|---:|
| Ordinary | 1/6 | 1/9 |
| Elite | 1/8 | 1/12 |
| PrimeEvil | 1/16 | 1/24 |
| MajorBoss | 1/20 | 1/30 |

Les dix identités `MajorBoss` résident dans le JSON et sont vérifiées contre
la table active. Aucune allowlist C++ de noms de boss n’est autorisée.

## Preuves acquises

- build Release x64 avec avertissements traités comme erreurs : **PASS**;
- tests automatisés CTest : **1/1 PASS**;
- DLL candidate : 215 552 octets, SHA-256
  `3EFCEB7374E26207FE603FF5AC43DAFBC8246E85C37426B62D0AEF1F38663D50`;
- ZIP public strict DLL+JSON : SHA-256
  `A6E89B7B4B8723704A44F95386AB841A6ABD4AD9C2C27003EE61A4B90331BE24`;
- contrats, signatures 92777, propriétaires de hooks et coexistence statique
  consignés sous `addons/BKVCombat/` et dans les preuves natives gouvernées;
- cold starts default-off, policies-on et ordre inverse exact : **PASS
  technique current-stack**;
- négociation lazy MeleeSplash→BKVCombat et témoins gameplay : **NOT RUN**.

## Prérequis GameTestRunner retenu

Vincent a retenu l’option A le 15 août 2026 : fermer d’abord le trajet vertical
repo-scoped `game-test` / `GameTestRunner`, puis seulement ouvrir la matrice
gameplay BKVCombat. Son premier gate est le scénario générique
`smoke-launch-save-exit` sur une copie isolée du personnage de test niveau 99
`QtyTester`. Il doit produire une capture d’inventaire et un `result.json`,
confirmer le retour à la sélection après `Save and Exit`, refuser tout bloc
d’inputs hors de la fenêtre D2R et conserver les sauvegardes originales
byte-exactes. Un échec volontaire et le hotkey d’arrêt `Pause` font partie du
gate.

Ce prérequis est **fermé par preuves réelles le 15 août 2026**. Le run
`20260815-114640043` passe 10/10 étapes en 31,7 s sous le profil local
3840×2160, produit
la capture d’inventaire, exécute `Save and Exit` et confirme
`CHARACTER_SELECT`; le modinfo, les sauvegardes originales et la fixture source
restent byte-exacts. L’échec volontaire `20260815-114719747` capture puis refuse
`open_inventory` hors `IN_GAME` sans envoyer d’input, et le run
`20260815-114810431` prouve l’arrêt `Pause` avec code AHK 130 et cleanup complet.
La v1 ferme encore toute session existante avant de rediriger le savepath et
referme D2R avant restauration; la réutilisation sûre reste un lot ultérieur.

Ce smoke ne vaut **aucune preuve des politiques de combat**. L’intégration future
au BKVince Hero Editor est limitée à des recettes de fixtures versionnées
appliquées par son codec à des copies de travail; elle n’est pas construite dans
le lot actuel.

## Prochain gate

1. Vérifier en solo avec `GameTestRunner` les caps et multiplicateurs
   Critical/Deadly, les quatre
   classes CB, le player-count, ranged, CBE `0/+100`, les trois stacks OW et leur
   rafraîchissement, ainsi que Life/Mana Steal et Life Tap.
2. Confirmer au premier hit la négociation lazy MeleeSplash→BKVCombat,
   l’absence de double application et l’échec fermé
   sur configuration, build ou signature incompatibles.
3. Reprendre séparément la matrice universelle seulement lorsque toutes les
   fonctions PluginPack peuvent être actives ensemble et que les incidents
   `dxgi/plugin-items` et `PopcornUber` sont fermés sans neutraliser un tiers.

Le multijoueur et le PvP restent non qualifiés pour cette version. Aucun résultat
runtime ou gameplay ne doit être déduit des seuls tests statiques.

## Suites planifiées

- Release 2 : Attack Engine — sélection dual wield par IAS−WSM, cadence,
  breakpoints et exceptions propres aux skills après preuve native ciblée.
- Release 3 : véritable melee splash PD2 natif.

## Rollback et frontière Git

La configuration livrée reste désactivée. Le rollback supporté est à froid :
désactiver ou retirer le plugin avant le prochain démarrage; aucun hot-unpatch
n’est revendiqué.

Le workstream couvre `addons/BKVCombat/**`, son profil BKVince, ses lignes de
stats/propriétés/localisation, ses preuves natives, cette mission, la ROADMAP et
les entrées de cadastre correspondantes. Les changements concurrents restent
hors périmètre. Aucun commit ni push n’est autorisé sans demande explicite de
Vincent.
