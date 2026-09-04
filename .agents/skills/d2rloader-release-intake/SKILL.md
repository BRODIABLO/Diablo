---
name: d2rloader-release-intake
description: Auditer et promouvoir une nouvelle baseline D2RLoader/PluginSDK à partir de sa release exacte, de ses artefacts, de ses contrats et de sa qualification. Utiliser automatiquement lorsque Vincent annonce qu'une version D2RLoader ou PluginSDK est sortie, demande d'adapter la Suite à cette version ou veut connaître la baseline courante; ne pas l'utiliser pour implanter un plugin ni pour surveiller périodiquement les releases.
---

# Admission d'une release D2RLoader

## Réagir à l'annonce sans surinterpréter

1. Commencer par `npm run baseline:d2rloader -- status` et lire `reverse-engineering/d2rloader-baselines.json`.
2. Une annonce seule déclenche l'identification et l'analyse read-only. Elle ne prouve ni l'authenticité de la release, ni sa compatibilité, ni son statut de baseline promue.
3. Respecter le gate `GO` d'`AGENTS.md` avant de créer un candidat, télécharger ou extraire des artefacts, modifier le registre, adapter du code ou déployer dans un runtime.
4. Ne jamais réécrire les autres skills pour y copier un numéro de version. Ils consomment la baseline promue et les candidats depuis le registre.
5. Ne pas créer de surveillance périodique : l'annonce de Vincent est le déclencheur attendu, sauf demande explicite d'un monitor.

## Vérifier la provenance exacte

1. Confirmer la version, le canal, la date, la page de release et les assets auprès de la source officielle D2RLoader. Ne pas conclure depuis un nom de fichier, une archive relayée ou un message Discord seul.
2. Distinguer release publique, preview, beta, nightly et build local. Le PE peut conserver un suffixe différent du nom public; consigner les deux sans les harmoniser artificiellement.
3. Identifier séparément la release D2RLoader, `D2RLoader.exe`, `D2RCore.dll`, `d2rloader.mpq`, le tag/commit PluginSDK et la version d'API exposée.
4. Lorsque les checksums officiels n'existent pas, le dire et conserver les SHA-256 calculés comme identité locale de l'artefact inspecté, pas comme attestation de publication.

## Créer et auditer le candidat

Après `GO` :

1. Créer le candidat avec `npm run baseline:d2rloader -- announce <version> --source-url <url> --channel <canal>`.
2. Conserver les artefacts sous `analysis-cache/d2rloader-release-intake/<id>/`; ne jamais versionner les binaires du loader dans ce dépôt.
3. Capturer l'ensemble exact avec `npm run baseline:d2rloader -- capture <id> --artifact-dir <dossier>`.
4. Lire [references/release-intake-checklist.md](references/release-intake-checklist.md), puis auditer les deltas d'exports, manifeste, API/ABI, services, lifecycle, threads, configuration, ordre de chargement, patches/hooks et providers `D2RCore`.
5. Enregistrer dans le candidat les pins SDK exacts, le contrat observé et la matrice d'impact des composants. Choisir le SDK minimal couvrant les capabilities réellement consommées; ne pas migrer toutes les DLL vers le dernier SDK par principe.
6. Faire passer chaque gate avec une preuve précise via `npm run baseline:d2rloader -- gate <id> <gate> passed --evidence <preuve>`. Ne jamais marquer `passed` sur la seule foi du numéro de version ou d'une déclaration de compatibilité générale.

## Router les conséquences

1. Si une nouvelle capability ou un nouveau service apparaît, appliquer `d2rloader-service-governance` seulement pour les consommateurs réels; une nouveauté non consommée reste livrée mais non adoptée.
2. Si une surface D2R.exe, un RVA, une signature, un layout ou une ABI native change, appliquer `d2r33-reverse-engineering` avant toute adaptation.
3. Si une DLL doit être créée ou adaptée, utiliser `d2rloader-plugin-incubation` pour son implantation et sa qualification; l'admission de la baseline ne remplace pas ce gate.
4. Si la release rend une fonctionnalité RuffnecKk native ou redondante, classer le composant `superseded-by-loader` et préparer une migration ou un retrait explicite au lieu de conserver deux propriétaires.
5. Pour toute qualification en jeu, utiliser `d2r-runtime-validation` et obtenir sa confirmation spécifique avant d'arrêter ou lancer un processus.

## Promouvoir sans perdre l'historique

1. La baseline précédente reste autoritaire pendant tout l'audit. Un candidat peut informer un design futur, mais ses propriétés restent provisoires.
2. Exiger `passed` pour la provenance, l'intégrité des artefacts, le SDK, les contrats, la compatibilité statique, la qualification runtime et la coexistence pile complète.
3. Garder le multijoueur comme gate indépendant : `not-run` interdit toute revendication réseau, sans empêcher à lui seul la promotion d'une baseline locale correctement qualifiée.
4. Promouvoir seulement avec `npm run baseline:d2rloader -- promote <id>`. La commande doit refuser un artefact absent, un pin SDK absent, un contrat non relu ou un gate requis ouvert.
5. Conserver l'ancienne baseline en `superseded`; ne supprimer ni ses hashes, ni ses preuves, ni les anciennes releases de la Suite.
6. Après promotion, exécuter `npm run test:d2rloader-baseline` et vérifier que `plugin-architect`, l'incubation et la validation runtime lisent tous le même pointeur promu.

## Maintenir la frontière de compatibilité

- La version et le canal D2RLoader sont des métadonnées de diagnostic et de gouvernance.
- Aucune DLL RuffnecKk ne peut les utiliser pour autoriser, refuser ou sélectionner un profil natif.
- Le chargement reste gouverné par l'API réellement exposée, les capabilities nécessaires et les empreintes natives fail-closed.
- Une release annoncée, auditée ou promue ne prouve pas la compatibilité multijoueur ni celle d'un plugin qui n'a pas été testé sur les surfaces qu'il consomme.
