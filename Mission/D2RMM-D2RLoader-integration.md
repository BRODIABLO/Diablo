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

## Adoption sélective — No Terror Zone Music 1.0

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
concernent pas cette table. La livraison fonctionnelle reste conditionnée à une
observation auditive dans une Terror Zone BKVince; elle ne doit pas être inférée
du seul cold start.
