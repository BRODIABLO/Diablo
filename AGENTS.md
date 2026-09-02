Quand tu lis ceci, dis 'Je suis le gardien du Workspace RuffnecKk'

# Collaboration protocol

## Discussion before execution

- Default to discussion-first behavior for every non-trivial request.
- A request to explore, discuss, consider, compare, design, or plan is not
  authorization to implement.
- Do not modify files or run mutating commands until the user explicitly
  writes `GO`.

# Orientation des agents — Workspace Diablo RuffnecKk

N'interroge jamais spontanément le user sur l'ouverture de l'éditeur. Ouvre-le uniquement lorsqu'il le demande explicitement. L'éditeur déployé est accessible via diablo-tcp-admin.netlify.app (domaine personnalisé diablo.spheredi.com pas encore branché) ; si le user demande plutôt une exécution locale, fais les démarches nécessaires.

## Nature du dépôt

### Nomenclature autoritaire

| Terme | Signification |
|---|---|
| **Workspace RuffnecKk** | Laboratoire de plugins et de patches D2R 3.3 au service de la communauté Discord D2RLoader, et quartier général de BKVince |
| **RuffnecKk D2RLoader Suite** | Suite modulaire de plugins RuffnecKk autonomes, versionnés et configurés indépendamment, mais qualifiés ensemble contre une baseline D2RLoader/SDK et une matrice de coexistence communes |
| **BKVince** | Mod actuel pour D2R 3.3 sous D2RLoader, construit sur le squelette de BKDiablo puis enrichi par plusieurs mods provenant de Nexus Mods et par les idées propres de Vincent |
| **TCP** | Mod historique D2R 2.4, distinct de BKVince |
| **BK, BT, VNP** | Mods de référence distincts, jamais synonymes de BKVince |

Lorsque Vincent dit « mon mod » sans autre précision, interpréter **BKVince**. Ne jamais employer **TCP** comme synonyme du mod actuel; ne le mentionner que si Vincent le nomme explicitement ou si le travail concerne réellement `data-TCP/`.

Trois activités complémentaires cohabitent :

1. **Un laboratoire communautaire D2R 3.3 sous D2RLoader** — reverse engineering gouverné, plugins natifs autonomes et hybrides, patches mémoire JSON et add-ons publiables séparément de BKVince. Les travaux réutilisables ont vocation à servir la communauté Discord D2RLoader.
2. **Le quartier général de BKVince** — le mod actuel de Vincent, assemblé à partir du squelette de BKDiablo, d'apports sélectionnés provenant de plusieurs mods publiés sur Nexus Mods et de ses propres idées. Les sources tierces et leurs crédits restent explicitement distingués des créations RuffnecKk.
3. **Les données et l'outillage du workspace** — les tables `.txt` (TSV), les assets, les missions, le cadastre et une plateforme web en monorepo **npm + turbo** : un **Admin** pour éditer les tables et, à venir, un **Wiki** de comparaison.

Les `.txt` restent la source ; **pas de base de données**. Les dossiers `local/` et `hd/` de TCP et BK sont également versionnés, avec les binaires HD sous **Git LFS**. Stack : **Vite + React** (fronts), **Netlify** (hébergement en ligne : `diablo-tcp-admin.netlify.app`), git comme « base » (chaque édition = commit).

## Source de vérité : le cadastre

`ai-cartographie.json` (validé par `ai-cartographie.schema.json`) est la **carte gouvernée** du dépôt. Chaque zone porte un `role` et une politique `agentAccess`. **Vérifie ces accès avant de modifier quoi que ce soit.**

| Zone | Rôle | Accès |
|---|---|---|
| `data-TCP/` (`global`, `hd`, `local`) | mod historique D2R 2.4 | **modifiable** |
| `data-TCP/D2RLAN/` | profil local D2RLAN et intégration runtime de TCP | **modifiable** |
| `data-BKVince/` | quartier général et source de développement du mod actuel pour D2R 3.3 sous D2RLoader; squelette BKDiablo enrichi d'apports Nexus Mods et de créations propres | **modifiable** |
| `data-BK/`, `data-BT/` | mods de référence / inspiration; BKDiablo constitue le squelette historique de BKVince | **read-only** |
| `data-VNP/` | Mod Vanilla++ servant d'inspiration pour BKVince | **read-only** |
| `excel-vanilla2.4/` | données vanilla Diablo II 2.4 | **read-only** |
| `data-vanilla3.2/` | extraction locale CASCView de D2R 3.2 ; seul `data/data/global/excel` est versionné | **read-only** |
| `data-vanilla3.3/` | extraction directe du CASC officiel D2R 3.3.93847 ; seul `data/data/global/excel` est versionné | **read-only** |
| `Mission/` | besoins et intentions | modifiable |
| `addons/` | plugins, patches et add-ons autonomes destinés à une publication communautaire indépendante de BKVince | modifiable |
| `reverse-engineering/` | ateliers persistants et preuves natives pour le runtime courant D2R 3.3, avec provenance historique explicite des images binaires | modifiable |
| `apps/` | plateforme web (admin, wiki) | modifiable |
| `schemas/` | catalogue de schémas de colonnes (dérivé du guide TXT eezstreet/d2rdoc) | modifiable |
| `scripts/` | outillage (cadastre, validateur, TSV, dev-server) | modifiable |
| `.agents/skills/` | procédures spécialisées réutilisables des agents | modifiable |
| `guide/d2rdoc/` | guide TXT courant pour D2R 3.x/3.3 (`eezstreet/d2rdoc`) | **gitignoré — source primaire des schémas TXT** |
| `guide/legacy/` | ancien D2R Data Guide | **gitignoré — référence complémentaire pour assets et certains JSON, jamais normative pour les `.txt` 3.3** |

En clair : `addons/`, `reverse-engineering/` et les procédures natives portent le laboratoire communautaire; `data-BKVince/` est le quartier général du mod actuel; `data-TCP/` demeure la source historique 2.4 et n'est jamais un synonyme de BKVince. `apps/`, `schemas/` et `scripts/` fournissent la plateforme et l'outillage communs.

## Conventions

- **Items en anglais** : `ring`, `belt`, `amulet`, `gem`, `rune`, `charm`…
- **Configuration TOML** : le contenu et les commentaires des fichiers `.toml` doivent toujours être rédigés en anglais — jamais en français.
- **Auteur des patchs et plugins** : utiliser exactement `RuffnecKk` dans les métadonnées d’auteur — jamais `TCP`. Conserver séparément les crédits tiers déjà présents.
- **README des releases publiques** : lors de la création d’un ZIP de release publique, créer ou actualiser le README et le déposer à côté du ZIP dans le dossier de livraison, mais ne jamais l’inclure dans l’archive générée par l’agent. Vincent doit pouvoir le relire et le modifier humainement avant de l’ajouter lui-même au ZIP final.
- **Rétention des releases publiques** : les ZIP générés sous `addons/` restent locaux et gitignorés; les artefacts distribués sont publiés comme assets des GitHub Releases du dépôt produit approprié, principalement `RuffDood/RuffnecKk-D2RLoader-Suite`. Conserver les anciennes GitHub Releases et leurs tags comme canaux de rollback sans recopier leurs binaires dans le dépôt principal.
- **Crédit D2MOO** : dès qu’un plugin a nécessité des connaissances acquises grâce à D2MOO, créditer explicitement D2MOO dans le README du plugin. Conserver ce README avec la documentation du projet et à côté du ZIP généré par l’agent, afin que Vincent puisse le réviser avant de l’ajouter lui-même au ZIP final.
- **Description des plugins** : rédiger en anglais une seule phrase courte, idéalement moins de 100 caractères, commençant par un verbe au présent et décrivant d’abord l’effet visible pour le joueur. Ne pas y répéter le build D2R, le mode de chargement ni les détails internes (`RVA`, hooks, ABI, identifiants de statistiques) ; conserver ces précisions dans le README et les logs.
- **Plugins D2RLoader hybrides** : toute nouvelle DLL doit pouvoir être installée indifféremment dans le dossier global `<D2R>/d2rloader/plugins/` ou dans le dossier d’un mod `<D2R>/mods/<mod>/d2rloader/plugins/`. Ne déclare pas `ModScopedOnly`; conserve les mêmes contrôles stricts de build, de signatures et d’ABI dans les deux portées.
- **Gate absolu pour chaque nouveau plugin** : lorsqu'une nouvelle DLL est retenue et avant toute implantation, utiliser le skill `d2rloader-plugin-incubation`. Ne pas activer son workflow opérationnel pendant une comparaison exploratoire où le mécanisme reste ouvert. Tout nouveau plugin est automatiquement une DLL autonome RuffnecKk membre de la **RuffnecKk D2RLoader Suite**; ne plus demander de choisir entre autonome et merge, ne proposer aucune catégorie, DLL propriétaire ou clé de merge PluginPack, et ne planifier aucun merge futur dans une DLL d’eezstreet.
- **Contrat RuffnecKk Suite** : chaque nouvelle DLL conserve sa version, ses métadonnées, son archive et sa configuration indépendante en JSON ou en TOML lorsque TOML est plus convivial. Elle doit utiliser la baseline D2RLoader/SDK gouvernée courante, rester hybride globale/mod-locale, désigner un propriétaire unique pour chaque hook ou contrat partagé et être qualifiée avec tous les composants actifs de la Suite ainsi qu’avec les cinq plugins eezstreet. Toute coopération inter-DLL doit être versionnée, tolérer l’absence du fournisseur et refuser proprement une ABI incompatible. Ne jamais modifier, lier ni redistribuer une DLL d’eezstreet.
- **Documentation des `.txt`** : `https://eezstreet.github.io/d2rdoc/` est la référence primaire pour les tables du runtime courant D2R 3.3 et les descriptions de headers de l’Éditeur. L’ancien guide ne tranche plus une question concernant un header `.txt` 3.3; il reste utilisable pour les assets et certains JSON.
- **Encodage & intégrité des `.txt`** : UTF-8 sans BOM pour le code ; les tables `.txt` D2R restent en **CRLF**. Toute réécriture doit utiliser le skill `diablo-tsv` et `scripts/build-data/tsv.js`, avec un round-trip **byte-exact** obligatoire.
- **Assets versionnés** : `data-TCP/hd`, `data-TCP/local`, `data-BK/hd` et `data-BK/local` sont dans Git. Les formats HD binaires de TCP/BK passent par Git LFS ; les backups `*.bak` restent exclus.
- **Git** : ne change jamais de branche et ne commit ni ne push jamais de ta propre initiative. Une demande explicite de l’utilisateur courant suffit; aucune formule `GO` dédiée ni identité particulière n’est requise.
- **Runtime Diablo** : utiliser le skill `d2r-runtime-validation`. Si des fichiers sont verrouillés, fermer soi-même les instances concernées; ne jamais demander à Vincent de fermer le jeu. Relancer ensuite une seule instance si la validation l’exige.
- **Compatibilité des plugins** : ne désactiver aucun plugin installé ni aucune fonctionnalité du PluginPack pendant un cold start ou un test déclaré de compatibilité. La matrice de qualification doit utiliser la pile complète active et activer toutes les fonctionnalités du pack; un démarrage obtenu en retirant, neutralisant ou laissant désactivé un composant ne prouve aucune compatibilité. Une isolation temporaire est permise uniquement comme diagnostic explicitement étiqueté, puis la pile complète doit être restaurée et retestée avant toute conclusion ou livraison.

## Skills spécialisés

Les procédures répétables résident sous `.agents/skills/`. Utilise le skill correspondant dès que son domaine est engagé :

- `diablo-tsv` — cadastre, schémas, tables TXT, CRLF et round-trip byte-exact;
- `plugin-architect` — revue d'architecture explicite, approfondie et strictement read-only avant le choix d'un mécanisme;
- `d2rloader-service-governance` — gate automatique « no consumer, no service » pour auditer, prioriser, formuler et mesurer les demandes de services D2RLoader/PluginSDK;
- `d2r33-reverse-engineering` — preuves natives du runtime courant D2R 3.3.93847, fonctions, xrefs, signatures, ABI et RVA;
- `d2rloader-plugin-incubation` — implantation d'une DLL retenue, autonomie RuffnecKk Suite, audit SDK/ABI/hooks, coexistence complète, configuration indépendante, crédits et ZIP;
- `d2r-runtime-validation` — arrêt/relance du jeu, synchronisation, hashes, logs et matrice de validation;
- `diablo-roadmap-release` — mission courante, séquencement ROADMAP, archive publique et contrôles de livraison.

## Atelier persistant de reverse engineering D2R 3.3

Pour tout memory patch ou plugin natif ciblant le runtime courant `D2R.exe 3.3.93847`, utiliser obligatoirement le skill `d2r33-reverse-engineering` et commencer par `npm run re:d2r33 -- status`. L'identité binaire utile entre 3.2.92777 et 3.3.93847 est établie : `reverse-engineering/d2r-3.2.92777/` reste le corpus natif gouverné commun et ses RVA, signatures, ABI, index et preuves sont réutilisables. Les nouvelles instructions, sorties et livraisons nomment toujours la cible courante 3.3.93847; le chemin historique décrit seulement la provenance du corpus. Si l’image et l’index sont vérifiés, ne pas redumper ni réimporter le binaire. Les contenus de `analysis-cache/` restent locaux, gitignorés et jamais commités. Toute identification stable enrichit `known-rvas.json` avec sa source et sa confiance. Un workbench distinct n'est créé que pour une future image native utile réellement différente, jamais sur la seule différence de hash du PE retail protégé. D2MOO reste une référence sémantique 1.10f : aucune adresse, structure ou ABI 32 bits n’est transposable directement.

## Développement de la plateforme

- `npm install` puis `npm run dev` : lance le **dev-server** local (`scripts/dev-server.js`, port 4000, lit/écrit les `.txt`) et l'**admin** Vite (port 5173).
- L'admin édite les tables typées via `schemas/*.json`. Ces schémas sont régénérés avec `npm run generate:schemas` depuis les headers réels de BKVince 3.3 et les définitions structurées du clone local `guide/d2rdoc/`; TCP 2.4 et `guide/legacy/` ne servent que de replis. En dev, l'admin écrit les `.txt` en direct ; en production (`diablo-tcp-admin.netlify.app`), l'écriture passe par des **commits via l'API GitHub**.

## Workflow cadastre

Après toute modification **structurelle** — ajout, suppression ou renommage de fichier ou dossier — appliquer le workflow cadastre du skill `diablo-tsv` : régénérer `ai-cartographie.json`, enrichir les annotations de toute zone signifiante et exiger `VALID` de `node scripts/validate-cartographie/validate.mjs`.

## Suivi de la ROADMAP

`ROADMAP.html` est le tableau de bord du projet. Utiliser le skill `diablo-roadmap-release` pour toute tâche significative ou livraison. Toujours proposer l’ajout d’une nouvelle tâche et attendre la confirmation avant d’éditer la ROADMAP; après confirmation, proposer deux séquencements fondés sur la valeur métier et l’efficacité d’avancement. Quand une tâche ou un commit est fait, vérifier la fraîcheur de la mission et de la ROADMAP.

## Mission courante

Cherche et trouve la mission courante avec le skill `diablo-roadmap-release`. Si le contexte ou la ROADMAP ne sont pas assez clairs, valide inline avec l'utilisateur le prochain pas.

## Profondeur des réponses et phase de conception

Lorsqu’un utilisateur explore une idée, pose une question technique, compare des
approches ou évalue une architecture, produire par défaut une réponse approfondie
orientée vers la décision.

Ne pas confondre un effort de raisonnement élevé avec une réponse concise.
Exposer dans la réponse finale les conclusions utiles du raisonnement.

Sauf demande explicite de brièveté :

1. reformuler précisément le besoin;
2. inspecter le dépôt et les sources gouvernées avant toute affirmation technique;
3. distinguer explicitement :
   - les faits vérifiés;
   - les hypothèses à tester;
   - les inconnues;
   - les recommandations démontrées;
4. présenter les approches réellement viables;
5. expliquer leur fonctionnement étape par étape;
6. comparer leurs avantages, inconvénients, risques et prérequis;
7. couvrir, lorsque pertinent :
   - le runtime;
   - la persistance et les sauvegardes;
   - le multijoueur et l’autorité serveur/client;
   - la compatibilité;
   - la migration;
   - la maintenance;
8. recommander une approche et justifier ce choix;
9. identifier les décisions utilisateur et les preuves techniques requises avant
   l’implantation.

Préférer une analyse Markdown structurée à une réponse conversationnelle courte.

Lorsque l’utilisateur est encore en phase d’exploration, de clarification ou de
choix architectural, ne modifier aucun fichier et ne commencer aucune
implantation. Une formulation comme « étudions cette idée », « comment cela
fonctionnerait-il ? » ou « quelles sont les options ? » constitue une demande
d’analyse, pas une autorisation d’implanter.

Lorsqu’une demande d’implantation est explicite, ne pas répéter inutilement
l’analyse complète si les décisions et les gates requis sont déjà fermés.
Rappeler brièvement l’architecture retenue, puis exécuter le travail conformément
aux procédures du workspace.

## Gate anti-sauce

Avant de recommander un nouvel outil, une automatisation ou une évolution d'architecture :

1. identifier une friction réellement observée et citer sa preuve concrète;
2. auditer ce que l'outillage actuel couvre déjà;
3. distinguer explicitement **fait vérifié**, **hypothèse à tester**, **simple idée** et **recommandation démontrée**;
4. annoncer un gain vérifiable et la manière de le mesurer;
5. si le besoin ou le gain n'est pas démontré, conclure honnêtement qu'aucune amélioration n'est justifiée pour le moment.

Ne jamais présenter une possibilité plausible comme une priorité établie. Préférer ne rien construire à ajouter une couche sans problème mesuré.

## Hygiène Git

- **Reprise automatique** : au début de chaque intervention, exécuter soi-même `npm run checkpoint`, puis lire `analysis-cache/checkpoint/state.json`. Après toute mutation, le rafraîchir avant la réponse finale. Ne jamais demander à l'utilisateur de résumer l'état du dépôt si ce diagnostic peut le reconstruire.
- **Rituel conservé** : la préparation du checkpoint est automatique, mais commit et push exigent toujours la demande explicite de l'utilisateur; `commit push go` reste la formule habituelle.

- **Vincent** (humain) : rappelle-lui régulièrement de commit et push son travail — grain fin, messages clairs.
- **Toi (agent)** : ne commit ni ne push **jamais** de ta propre initiative. Agis uniquement après une demande explicite de l’utilisateur courant, formulée naturellement ou sous forme de `GO` (cf. § Conventions).
