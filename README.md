# Workspace RuffnecKk — laboratoire D2RLoader 3.2 et quartier général de BKVince

Ce dépôt réunit trois dimensions complémentaires :

1. Un **laboratoire de plugins et de patches pour D2R 3.2**, au service de la communauté Discord D2RLoader : reverse engineering gouverné, DLL autonomes et hybrides, patches mémoire JSON et add-ons publiables indépendamment de BKVince.
2. Le **quartier général de BKVince**, le mod actuel de Vincent sous D2RLoader 3.2. BKVince est un assemblage dirigé par Vincent : son squelette provient de **BKDiablo**, puis il est enrichi par des apports sélectionnés de plusieurs mods publiés sur **Nexus Mods** et par ses propres idées. Les sources tierces et leurs crédits restent distingués des créations RuffnecKk.
3. Les **données et l'outillage** qui soutiennent ces travaux : tables et assets des mods, atelier de reverse engineering, missions gouvernées, éditeur web et futur wiki.

Les `.txt` restent la **source de vérité** ; pas de base de données — git est la « base ».

## Structure

- `addons/` — plugins, patches et add-ons autonomes préparés pour une publication communautaire indépendante de BKVince
- `reverse-engineering/` — ateliers persistants, manifestes et preuves natives pour D2R 3.2
- `data-BKVince/` — quartier général et source gouvernée du mod actuel; squelette BKDiablo enrichi d'apports Nexus Mods et de créations propres, synchronisé vers le runtime seulement par lots validés
- `data-TCP/` — mod historique D2R 2.4, distinct de BKVince
  - `global/excel/` — tables de gameplay (`.txt`) : armor, weapons, hireling, runes, setitems…
  - `local/lng` — chaînes localisées, **versionnées**
  - `hd/` — assets HD **versionnés via Git LFS**
  - `D2RLAN/` — profil launcher local et intégration runtime de TCP, **modifiable mais non versionné**
- `data-BK/`, `data-BT/` — mods externes de **référence** (lecture seule); BKDiablo constitue le squelette historique de BKVince; `local/` et `hd/` de BK sont versionnés, tandis que `hd/` de BT reste local
- `data-VNP/` — Mod Vanilla++ servant d'inspiration pour BKVince (**lecture seule**, hors Comparateur); seuls `global/`, `local/` et `hd/` sont versionnés
- `excel-vanilla2.4/` — tables du jeu de base D2 2.4 (référence, lecture seule)
- `data-vanilla3.2/` — extraction CASCView du jeu de base D2R 3.2 (référence, lecture seule) ; seul `data/data/global/excel/` est versionné, les autres branches demeurent locales
- `data-vanilla3.3/` — extraction directe du CASC officiel D2R 3.3.93847 (référence retail courante, lecture seule) ; seul `data/data/global/excel/` est versionné et `data-vanilla3.2/` reste intact pour les preuves 92777
- `apps/admin/` — **éditeur web** des tables (Vite + React)
- `schemas/` — schémas de colonnes des tables BKVince 3.2 (typage, descriptions et validation de l'éditeur), générés depuis le guide TXT courant
- `scripts/` — `dev-server.js` (API locale de lecture/écriture des `.txt`), `build-data/` (parseur/écrivain TSV), `generate-architecture.ps1` (cadastre), `validate-cartographie/` (validateur), `publish-tcp.ps1`
- `tools/` — utilitaires tiers locaux de modding; la documentation et la provenance sont versionnées, tandis que les binaires fournisseurs restent ignorés
- `guide/d2rdoc/` — clone local non versionné de [`eezstreet/d2rdoc`](https://eezstreet.github.io/d2rdoc/), référence primaire des `.txt` D2R 3.2 et des descriptions de headers
- `guide/legacy/` — ancien D2R Data Guide, conservé localement uniquement pour les assets et certains JSON
- `wiki-template/` — références pour le futur wiki (dont l'index du wiki BT)
- `Mission/` — besoins, décisions et preuves des chantiers du workspace
- `ai-cartographie.json` (+ `.schema.json`) — **cadastre** gouverné du dépôt (arbre + rôles + accès agents)
- `AGENTS.md`, `CLAUDE.md` — orientation des agents

## Démarrer l'éditeur

```powershell
npm install
npm run dev
```

Lance le serveur local (port 4000, lit/écrit les `.txt`) et l'éditeur sur **http://localhost:5173**. Clique une cellule pour l'éditer ; « Sauvegarder » réécrit le `.txt` en **préservant son format exact** (colonnes, CRLF, encodage).

## Guide TXT D2R 3.2

Le guide [`eezstreet/d2rdoc`](https://eezstreet.github.io/d2rdoc/) est la référence primaire pour les tables `.txt` 3.2. Son clone demeure local et gitignoré :

```powershell
git clone https://github.com/eezstreet/d2rdoc.git guide/d2rdoc   # première installation
git -C guide/d2rdoc pull --ff-only                               # mises à jour suivantes
npm run generate:schemas                                         # régénère les tooltips/schémas BKVince
```

L'ancien guide peut être conservé sous `guide/legacy/` pour les assets et certains JSON, mais il n'est plus normatif pour les headers `.txt` 3.2.

## Cadastre

`ai-cartographie.json` décrit tout l'arbre du dépôt et annote chaque zone d'un **rôle** et d'une **politique d'accès** pour les agents. Après toute modification **structurelle** :

```powershell
powershell -File scripts/generate-architecture.ps1     # régénère
node scripts/validate-cartographie/validate.mjs        # valide -> VALID
```

## Atelier de données

Les dossiers `global/`, `local/` et `hd/` de TCP et BK sont versionnés. Les formats HD binaires et les quatre JSON de plus de 10 MiB sont stockés via **Git LFS** ; les autres JSON restent des fichiers Git ordinaires. Le profil local modifiable `data-TCP/D2RLAN/`, les assets HD de BT, les backups et les réglages utilisateur restent hors Git. Pour la référence read-only VNP, `global/`, `local/` et `hd/` restent également versionnés ; `D2RLAN/` et les assets propres à `VNP/` restent ignorés.

## Prérequis

- **Git LFS** — requis pour cloner et publier les assets HD TCP/BK (`git lfs install` une fois par poste)
- **Node.js** — l'éditeur et les scripts JS
- **PowerShell** (5.1 convient) — les scripts `.ps1`

## Conventions

- Items désignés par leur terme **anglais** : `ring`, `belt`, `amulet`, `gem`, `rune`…
- Code en **UTF-8 sans BOM** ; les tables `.txt` D2R en **CRLF** (épinglé par `.gitattributes`) — l'éditeur préserve le format à la sauvegarde.
