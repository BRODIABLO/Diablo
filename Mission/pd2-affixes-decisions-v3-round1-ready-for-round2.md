# PD2 Affixes — Round 1 prêt pour Round 2

- Preview uniquement : `ready=false`; aucune application gameplay.
- Résultat : 1 383 cellules, 7 lignes ajoutées, 249 lignes rejetées, 655 occurrences auditées, 296 décisions incomplètes et 1 conflit.
- Delta : `+15` cellules contre 1 368, car Toxic devient projetable avec ses 7 changements et Pestilent avec ses 8 changements après remédiation poison exacte.
- Seul conflit : `magicsuffix.txt:569` — Iron Maiden Proc, skill PD2 `444`; dépendance approuvée, implantation différée sur la ligne BKVince réservée `444`.
- Sérialisation : toutes les tables restent sous 2 047 lignes compilées et le catalogue unifié reste sous 65 535.
- Préservé : tous les choix MaxLevel, les 83 remédiations conservatrices, les trois overrides ItemType nommés, les 248 exclusions maps, `of Decay` et toutes les autres décisions produit.
- Validation : 49/49 tests affixes passent. Le contrôle `pd2-affixes-review.mjs --check` reste volontairement ouvert, car le moteur technique 61a rend le rapport principal stale et ce lot interdit sa régénération ainsi que tout changement de `comparisonHash`.
