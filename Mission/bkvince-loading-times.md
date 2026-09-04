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

## Jalon technique — A/B des plaques de PNJ (3 août 2026)

- L'ajout de `24` descripteurs conformes aux plaques `npcname_*.texture`
  supprime les `5` fallbacks de plaque observés dans le processus témoin Acte I,
  mais ne réduit pas le temps d'entrée chaud : médiane `3 298 ms` avec le cache
  d'origine contre `3 299 ms` avec le candidat; p95 `3 377 ms` contre
  `3 396 ms`. Le candidat est rejeté et le cache original est restauré.
- Les séries froides ordonnées donnent `4 120 ms` contre `2 966 ms`, mais ce
  delta n'est pas causal : un passage de contrôle du cache original effectué
  après le candidat atteint lui aussi `3 046 ms`. Le cache de fichiers de l'OS
  explique donc la baisse apparente des derniers processus froids.
- Un second candidat a réduit les `24` plaques de `1024×1024` à `512×512` : le
  volume passe de `96 MiB` à `24 MiB` et le rendu Warriv/Kashya reste lisible en
  jeu, mais la médiane chaude régresse à `3 570 ms` et le p95 à `8 600 ms`.
  Les `24` textures originales ont été restaurées byte-exactement en source et
  dans le profil actif.
- Désactiver temporairement ReShade ne produit pas non plus de gain : médiane
  chaude `3 323 ms`, p95 `3 405 ms`. Le `dxgi.dll` ReShade 6.8 original est
  restauré avec son SHA-256 `B2945C29…D08DA`; deux crashes au stade graphique
  lui restent corrélés et constituent une piste de stabilité distincte.
- État final : cache source/runtime original SHA-256 `CD52ECD…FB1`, plaques
  source/runtime à `100 664 352` octets, D2R fermé. Aucun candidat n'ayant passé
  le gate, aucune optimisation non démontrée n'est conservée.

## Gates

- [x] Baseline de cinq passages froids et cinq passages chauds pour chaque
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

Instrumenter l'entrée en partie pour séparer compilation des `50` tables TXT,
chargement de scène et travail de rendu. Le profil ne contient actuellement
aucun `.bin` Excel permettant de retirer `-txt` sans perdre les données BKVince :
prouver d'abord une chaîne de compilation `.txt` vers `.bin` reproductible et
byte-gouvernée, puis comparer `-txt` au profil binaire. Ne plus modifier de
texture tant qu'une trace n'attribue pas un coût mesurable à un asset précis.
