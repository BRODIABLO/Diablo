# D2RMM Custom — intégration BKVince/D2RLoader

## Décision

Le fork `yinyin333333/d2rmm` est retenu comme **outil de composition local** pour
exécuter des mods D2RMM avec D2RLoader. Il n'est pas une nouvelle source de
vérité du mod.

- version installable retenue : `1.9.1`;
- commit épinglé : `634a39199fb819d2228e8aba7924e1515a291316`;
- archive officielle : `D2RMM.Custom.1.9.1.zip`;
- SHA-256 : `1D09425B4DCF69190D4F01459FD61C66C49E9D5B3D0FB534AEE1C724BA7A7DC6`;
- la branche amont `1.9.2` a été auditée, mais elle reste non taguée et non
  publiée au moment de l'intégration.

## Frontières de propriété

1. `data-BKVince/` reste la source gouvernée et versionnée du gameplay/runtime.
2. `C:/Games/Diablo II Resurrected/mods/BKVince/` reste le runtime validé.
3. D2RMM Custom compose ses mods dans une sortie locale. Toute modification à
   conserver doit être analysée puis rejouée dans `data-BKVince` avec les outils
   du dépôt; une sortie D2RMM ne doit jamais être recopiée aveuglément.
4. Les DLL, patches et configurations D2RLoader importés par D2RMM sont des
   paquets gérés séparément. Les extensions déjà versionnées sous
   `data-BKVince/d2rloader/` demeurent prioritaires.

## Capacités auditées

Le fork sait :

- lancer `D2RLoader.exe` et sélectionner le mod de sortie;
- lire et mettre à jour `d2rloader/config/d2rloader.toml` en préservant les
  commentaires et fins de ligne;
- produire `modinfo.json` et les prérequis de données attendus par D2RLoader;
- importer des DLL, patches JSON, dossiers ou ZIP en paquets avec inventaire et
  SHA-256;
- refuser les DLL dépourvues de `D2RLoaderGetPluginInfo` ou
  `D2RLoaderLoadPlugin`;
- éditer les JSON gérés et déployer atomiquement lors de `Install Mods`.

## Installation et réglages

Exécuter :

```powershell
powershell -ExecutionPolicy Bypass -File scripts/install-d2rmm-d2rloader.ps1
```

L'installateur conserve D2RMM 1.8.0, installe la version custom dans un dossier
séparé et migre uniquement son catalogue `mods/`.

Dans D2RMM Custom :

1. Game directory : `C:/Games/Diablo II Resurrected`.
2. Output mod name : `BKVince`.
3. Activer `Use D2RLoader`.
4. Vérifier que `default_mod` vaut `BKVince`.
5. Garder la normalisation CRLF activée pour les tables TXT.
6. Exécuter `Install Mods`, puis `Run D2R`.

## Validation obligatoire avant adoption d'un mod D2RMM

- comparer byte-à-byte la sortie aux sources gouvernées;
- classer chaque fichier en data, asset, patch ou plugin;
- rejouer les TSV avec `scripts/build-data/tsv.js`;
- tester un démarrage à froid sous D2RLoader et le build D2R ciblé;
- ne conserver dans Git que les changements explicitement validés.

## Adoption sélective — No Terror Zone Music 1.0 (prototype remplacé)

Décision de Vincent du 2 août 2026 : intégrer à BKVince l'effet data-only du mod
de NDState, dont le manifeste local crédite `salzgaard`, sans recopier le
`D2RMM.mpq` cumulatif. Ce dernier contenait aussi des versions divergentes de
`skills.txt` et `desecratedzones.json` et n'était donc pas une source acceptable.

Preuves statiques :

- la table du mod est byte-identique à la référence vanilla D2R 3.2 sauf la
  suppression de la clé stable `ESOUNDENVIRON_INHERIT_DESECRATED`;
- cette ligne sélectionnait `music_desecrated` avec
  `InheritEnvrionment = 1`; les 75 autres lignes et les 37 headers sont
  inchangés;
- la table gouvernée BKVince reste en CRLF avec saut final et passe un
  round-trip byte-exact via `scripts/build-data/tsv.js`;
- SHA-256 source gouvernée et runtime :
  `E2141C013B70762D2EBDCE693C1AB366D76CD22E713BE462F69D2871B8F637F1`.

Validation runtime du 2 août 2026 sur D2R `3.2.92777` et D2RLoader
`1.0.1-beta` :

| Domaine | Résultat | Statut |
|---|---|---|
| Déploiement ciblé | Un seul fichier copié, hashes source/runtime identiques | passed |
| Chargement BKVince | Mod build 92777 et `savepath BKVince/` acceptés | passed |
| Table sonore | Étape `Loading sound data tables` atteinte | passed |
| Extensions | 18/18 patchsets et 11/11 plugins actifs, zéro rejet/échec | passed |
| Cold start | Initialisation D2R `24/24` atteinte | passed |
| Musique en Terror Zone | Musique normale de la zone audible | not run |

Les quatre assertions d'items déjà connues surviennent après le frontend et ne
concernaient pas cette table. Cette conclusion du 2 août est remplacée par
l'incident et la preuve native ci-dessous.

### Correctif hybride du 12 août 2026

Le prototype data-only a provoqué l'assertion
`BC_ASSERT: !SoundGetNumSoundEnviron()` dans
`D2Common/src/DataTbls/SoundTbls.cpp:228` lors d'une transition de jeu. Le cold
start initial ne couvrait pas l'accès tardif à l'environnement réservé aux
Terror Zones.

Preuves natives gouvernées pour D2R `3.2.92777` :

- `SOUNDENVIRON_GetRecord` à `0x3B0BA0` reçoit l'index en `ECX`, lit le nombre
  de lignes compilées à `DataTables+0x508` et déclenche son assertion si
  `index >= count`;
- avec la ligne supprimée, le moteur demandait l'index stable `75` alors que
  le compteur valait également `75`, ce qui reproduit exactement le chemin
  d'échec à `0x3B0C82`;
- dans le résolveur d'environnement hérité, l'instruction unique
  `89 05 1F B5 88 02` à `0x20C4EF` copie seulement le champ `Song` de la ligne
  héritée dans l'environnement actif. Les dix autres champs copiés par cette
  branche restent indépendants.

Implantation retenue :

- restauration byte-exacte de la ligne vanilla
  `ESOUNDENVIRON_INHERIT_DESECRATED` comme 76e ligne de `soundenviron.txt`;
- patch strict `preserve-terror-zone-area-music.json`, limité aux six octets
  de la copie `Song` à `0x20C4EF` et gardé par les octets attendus;
- conservation du morceau normal déjà hérité de la zone, tout en conservant
  les ambiances et événements propres aux environnements terrorisés.

Intégrité statique :

- table de 76 lignes et 37 colonnes, CRLF, saut final et round-trip byte-exact
  via `scripts/build-data/tsv.js`;
- SHA-256 de `soundenviron.txt` :
  `F7C4F82380D239A82DCE54E2987B8F624933C16A4FB100150B7CEC7243A00F78`;
- SHA-256 du patch JSON :
  `4E23E7E032A176D18B2FEB2BF9F45C187EADFC992ABAEF7CDCCFA43B8C50D9A6`;
- la signature stricte du site `0x20C4EF` possède une seule occurrence dans
  la section `.text` du build 92777.

| Domaine | Résultat attendu | Statut |
|---|---|---|
| Déploiement ciblé | Table et patch copiés avec hashes identiques | passed |
| Chargement du patch | Site `0x20C4EF` accepté, aucun mismatch | passed |
| Table sonore | Étape `Loading sound data tables` franchie sans assertion | passed |
| Cold start | `16/16` patchsets, `19/19` plugins et D2R `24/24` | passed |
| Musique en Terror Zone | Musique normale de la zone audible sans assertion | not run |

Le cold start du 12 août 2026 a utilisé D2RLoader `1.0.1-beta`, D2R
`3.2.92777`, les extensions globales et mod-locales actives, sans désactivation :
`scanned=16 applied=16 disabled=0 failed=0` pour les patchsets et
`scanned=19 active=19 disabled=0 rejected=0 failed=0` pour les plugins. Les
24 étapes se terminent en 5,610 secondes, sans ligne `[ERROR]` ni assertion.
Le journal frais est
`C:\Games\Diablo II Resurrected\d2rloader\logs\d2rloader.log`, SHA-256
`84D39CD903E04D7A66B073C5F3763099AEDCDC3DF67700DB1EE177204077C2CB`.

La validation fonctionnelle reste volontairement ouverte : elle exige une
entrée réelle dans une Terror Zone pour confirmer auditivement le morceau de
la zone et l'absence d'assertion pendant la transition tardive.
