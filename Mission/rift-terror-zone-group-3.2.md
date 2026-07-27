# Rift Terror Zone Group — D2R 3.2

Dernière mise à jour : 27 juillet 2026

Statut : correctif data autonome déployé, validé techniquement puis déclaré
terminé par Vincent le 27 juillet 2026. La carte a été retirée de la ROADMAP
active; la mission demeure comme preuve historique.

## Besoin joueur

Lorsque `Act5-Rifts` est sélectionné comme zone désacralisée, afficher un nom de
groupe explicite au lieu d’une chaîne de repli incorrecte, sans modifier la
sélection, les niveaux, les monstres ni les bonus de Terrozone.

## Faits vérifiés

- `desecratedzones.json` référence correctement les Level IDs `138–146` sous
  deux zones `Act5-Rifts`, régulière et manuelle.
- Les neuf lignes correspondantes de `levels.txt` utilisent toutes
  `LevelGroup = Act 5 - Rift`.
- BKVince ne fournit aucun `levelgroups.txt` et la table vanilla 3.2 ne contient
  pas `Act 5 - Rift`; les Level IDs existent, mais leur nom de groupe ne peut
  donc pas être résolu pour le message de Terrozone.
- Le guide d2rdoc 3.2 confirme que `LevelGroup` condense les noms des niveaux
  dans les messages de zones désacralisées et que `NameString` fournit le nom
  affiché du groupe.
- La table vanilla fournit déjà `Act 1 - Moo Moo Farm`, mais sa chaîne de groupe
  `MooMooFarmGroup` emploie le gag coloré « a non-existent level »; Vincent a
  explicitement demandé le 27 juillet 2026 que la Terrozone Cow affiche plutôt
  son vrai nom.

## Décision

- Ajouter une copie gouvernée de `levelgroups.txt` vanilla 3.2 à BKVince, avec
  une seule ligne supplémentaire `Act 5 - Rift`.
- Laisser `ParentLevelGroupId` vide, comme les autres zones spéciales Acte 5,
  afin que le groupe Rift reste affichable indépendamment du groupe global
  `Act 5`.
- Utiliser `NameString = RiftGroup` et la chaîne anglaise `The Rifts`.
- Faire reprendre à `MooMooFarmGroup` les traductions gouvernées de la chaîne
  existante `Moo Moo Farm`, soit `The Secret Cow Level` en anglais, sans code de
  couleur embarqué.
- Ne modifier ni les neuf lignes Rift de `levels.txt`, ni leurs identifiants dans
  `desecratedzones.json`.
- Retirer uniquement la virgule JSON invalide déjà observée dans la définition
  manuelle du premier Rift.

## Matrice de validation

| Domaine | Cas | Attendu | Statut | Preuve |
|---|---|---|---|---|
| TSV | `levelgroups.txt` | CRLF et round-trip byte-exact | passed | `test:bkvince-rift-levelgroups`: 101 lignes vanilla + 1 groupe Rift, réexécution sans diff |
| Références | Level IDs `138–146` | `Act 5 - Rift` résolu une fois | passed | neuf IDs Rift validés et 279 références de zones résolues |
| Localisation | `RiftGroup` / ID `73040` | unique, `enUS = The Rifts` | passed | clé et ID uniques validés par le test ciblé |
| Localisation Cow | `MooMooFarmGroup` / ID `27338` | mêmes traductions que `Moo Moo Farm`, sans code couleur | passed | `enUS = The Secret Cow Level`, toutes les langues comparées par le test ciblé |
| JSON | `desecratedzones.json` | parse strict et deux groupes Rift complets | passed | deux entrées `Act5-Rifts` validées par parse strict |
| Déploiement | trois fichiers runtime | hashes source/runtime identiques | passed | `analysis-cache/runtime-sync/20260727-140212759-apply.json`, 3/3 hashes identiques |
| Déploiement Cow | `levels.json` uniquement | hash source/runtime identique | passed | `analysis-cache/runtime-sync/20260727-140717271-apply.json`, SHA-256 `28319E9A…259756` |
| Chargement | D2RLoader + BKVince `-txt` | cold start sans nouvelle assertion | passed | une instance relancée, 18 logs et 58 lignes frais à 10:07, zéro erreur |
| Suite data | `npm run verify:data` | tous les validateurs gouvernés réussissent | passed | suite complète verte le 27 juillet 2026, avertissements Cube historiques seulement |
| Visuel | sélection `Act5-Rifts` | ligne violette `THE RIFTS` | passed | validation de clôture de Vincent, 27 juillet 2026 |
| Visuel | sélection Cow Level | ligne violette `THE SECRET COW LEVEL` | passed | validation de clôture de Vincent, 27 juillet 2026 |
| Réseau | hôte/joiner | même nom et même sélection | not run | observation en jeu |

## Clôture

Aucun gate supplémentaire n’est requis pour la clôture demandée. Le contrôle
hôte/joiner reste explicitement documenté comme `not run` et ne constitue pas
une validation acquise.
