# BKVince — Réduction des temps de chargement

## Décision gouvernée

Vincent a confirmé le 2 août 2026 que ce chantier avance **en parallèle** de
la mission BaseMod 3.2. Il ne remplace pas la priorité désignée par
`Mission/CURRENT.md` et commence par un lot transversal borné de mesure et de
correction des assets, sans nouveau plugin ni changement gameplay.

## Objectif

Réduire les temps de démarrage, d'entrée en partie et de transition entre les
actes de BKVince, tout en préservant le rendu approuvé, les sauvegardes,
l'autorité hôte/joiner et la compatibilité D2RLoader 3.2 build 92777.

Le chantier doit attribuer chaque gain à une cause mesurée. Une optimisation
n'est conservée que si elle réduit la médiane d'au moins `10 %` ou `300 ms`, ne
dégrade pas le 95e percentile et ne crée aucune assertion, erreur de chargement
ou régression visuelle.

## Faits vérifiés au 2 août 2026

- le profil runtime actif contient environ `739 MiB` et `2 788` fichiers sous
  `BKVince.mpq`, dont `241 MiB` de sprites, `208 MiB` de textures et `126 MiB`
  de FLAC ;
- le dernier démarrage D2R mesuré atteint `24/24` en `3 766 ms`, tandis que
  l'entrée en partie à Harrogath est rapportée à `6 543 ms` ;
- le profil contient `210` textures mod-locales ; une vérification lexicale de
  leurs chemins dans `texture_desc_cache.json` en retrouve `44` et en laisse
  `166` à auditer, représentant environ `154 MiB` ;
- les 24 plaques `npcname_*.texture` pèsent exactement `96 MiB`, soit `4 MiB`
  chacune, et leurs chemins ne sont pas retrouvés dans le cache actuel ;
- la dernière session contient 18 chemins de textures distincts passés par un
  fallback annoncé comme potentiellement bloquant, 25 chemins avec échec
  d'allocation de texture et 20 chemins de modèles invalides ;
- les `18` patches sont lus en quelques dizaines de millisecondes, alors que le
  chargement des `11` plugins et de leurs hooks représente environ `1,5 s`
  avant l'étape `01/24` ; cette durée concerne le lancement et ne prouve pas la
  cause des 6,543 secondes d'entrée en partie ;
- les journaux de preuve restent locaux sous
  `C:/Games/Diablo II Resurrected/D2RLoader/logs/d2rloader.log` et
  `C:/Games/Diablo II Resurrected/blz-log.txt` jusqu'à leur collecte par le
  workflow runtime gouverné.

## Hypothèses à tester

1. Le cache de textures incomplet ou périmé provoque des chemins de chargement
   synchrones et contribue aux temps d'entrée dans les zones.
2. Les plaques de PNJ non compressées ou surdimensionnées augmentent
   inutilement les lectures et la pression du gestionnaire de textures.
3. Les dépendances de modèles ou matériaux invalides provoquent des recherches
   et replis répétitifs.
4. ReShade, les FLAC, le mode `-txt` et les extensions D2RLoader peuvent ajouter
   des coûts secondaires, mais aucun n'est encore démontré comme cause
   prioritaire des transitions lentes.
5. La suppression du préchargement CASC peut déplacer du temps du démarrage vers
   l'entrée en partie ; aucun préchargeur natif n'est justifié sans une matrice
   A/B préalable.

## Séquencement retenu — lot transversal parallèle

### 1. Baseline reproductible

- mesurer séparément D2RLoader → `01/24`, `01/24` → `24/24`, entrée en partie
  et transitions Acte I ↔ Acte V ;
- effectuer cinq passages froids et cinq passages chauds avec le même
  personnage, la même résolution et les mêmes réglages ;
- séparer les séries solo, hôte et joiner ;
- consigner médiane, 95e percentile, erreurs fraîches et hashes du profil.

### 2. Intégrité des assets et du cache

- établir la couverture sémantique des 210 textures, pas seulement la présence
  textuelle de leur chemin ;
- reconstruire ou corriger `texture_desc_cache.json` à partir des assets
  réellement livrés ;
- ajouter un validateur reproductible qui refuse les textures BKVince sans
  métadonnées attendues ;
- éliminer les références invalides propres au mod sans masquer les erreurs
  vanilla ou tierces.

### 3. Plaques de noms de PNJ

- comparer les fichiers actuels avec une variante correctement compressée et
  munie de mipmaps ;
- tester d'abord `512×512`, puis `256×256` seulement si la comparaison 4K reste
  visuellement équivalente ;
- conserver un rollback atomique de la famille et une preuve source/runtime par
  SHA-256.

### 4. Matrice secondaire

Seulement après les trois premières étapes, mesurer séparément ReShade,
Sounds of Variation, les extensions D2RLoader et un éventuel profil de jeu sans
`-txt`. Toute évolution de préchargement natif reste hors périmètre tant qu'un
goulot d'étranglement résiduel n'est pas démontré.

## Gates

- [ ] Baseline de cinq passages froids et cinq passages chauds pour chaque
  scénario retenu.
- [ ] Couverture sémantique complète des textures mod-locales et validateur
  reproductible.
- [ ] Zéro fallback bloquant, erreur d'allocation ou dépendance invalide causé
  par un asset BKVince dans les scènes témoins.
- [ ] Comparaison visuelle approuvée des plaques de PNJ à la résolution de jeu
  réelle.
- [ ] Gain médian d'au moins `10 %` ou `300 ms`, sans régression du 95e
  percentile.
- [ ] Cold start `24/24`, plugins et patches sans rejet ni échec, hashes
  source/runtime identiques.
- [ ] Entrée en partie et transitions validées en solo, hôte et joiner.
- [ ] Aucune migration de sauvegarde, mutation gameplay ou nouvelle autorité
  réseau.
- [ ] Les assertions de zone, niveau ou warp observées séparément ne sont pas
  attribuées aux temps de chargement sans preuve causale.

## Prochain gate

Capturer la baseline reproductible froid/chaud du démarrage, de l'entrée en
partie et des transitions d'acte, puis auditer les 210 textures mod-locales
contre `texture_desc_cache.json` avant toute modification d'asset.
