---
name: game-test
description: "Exécuter des scénarios rapides et sûrs dans Diablo II: Resurrected avec GameTestRunner, uniquement après invocation explicite `$game-test`. Utiliser ce skill pour confirmer un comportement impossible à prouver par le code seul, reproduire un bug, vérifier une interface, un tooltip, un hotkey, une animation ou un effet visuel, qualifier un cold start, détecter un crash de plugin ou vérifier la persistance après Save and Exit. Ne pas l'utiliser pour auditer des tables TXT, comparer du code, modifier de la documentation, lancer des tests unitaires ou traiter une tâche confirmable sans démarrer D2R."
---

# Game Test

## Respecter le périmètre

- Exécuter uniquement des scénarios hors ligne avec des personnages de test dédiés à BKVince.
- Garder le cœur de `GameTestRunner` générique; ne jamais y intégrer une logique propre à AdvancedTooltips, RemoteStash, FloatingDamage, PotionAutoPickUp ou à un autre plugin.
- Commencer par le plus petit trajet complet. Ne pas généraliser avant d'avoir obtenu et inspecté un run réel du smoke test.
- Utiliser aussi `d2r-runtime-validation` lorsqu'il faut déployer un build, synchroniser BKVince, gérer les processus ou qualifier le runtime.
- Ne jamais lancer deux scénarios D2R simultanément. En travail multi-agent, réserver le contrôle du PC, de D2R et du runner à un seul agent; limiter les autres agents à l'analyse statique.

## Répartir les responsabilités

Faire porter à Codex les décisions qui demandent du jugement :

- choisir le scénario et préparer le build;
- lancer le runner et inspecter `result.json`;
- analyser les captures demandées aux checkpoints;
- intervenir une seule fois face à un état imprévu;
- rendre le verdict et le rapport avec les preuves.

Faire porter à `GameTestRunner` le trajet déterministe :

- traiter l'action générique `launch_or_reuse_game`, activer D2R et vérifier sa géométrie;
- exécuter rapidement des blocs d'inputs AutoHotkey v2;
- appliquer les délais centralisés et attendre les états observables;
- capturer seulement les checkpoints et les échecs;
- arrêter proprement, journaliser les événements et produire le résultat structuré.

Réserver Computer Use à la calibration initiale, à l'analyse visuelle des checkpoints, à une récupération bornée et aux gestes non encore déterministes. Ne jamais demander au modèle de choisir chaque clic ou chaque touche du trajet normal.

## Préparer la configuration locale

1. Copier `tools/GameTestRunner/config.example.json` vers `tools/GameTestRunner/config.local.json`.
2. Renseigner les chemins locaux, le personnage de test, `runtimeModInfoPath`, `testSavePathName` et le profil d'interface.
3. Conserver `config.local.json`, les gabarits visuels et les détails de calibration hors de Git.
4. Installer AutoHotkey v2 et renseigner `autoHotkeyExecutable`.
5. Utiliser un mode de fenêtre et une taille de zone cliente correspondant exactement au profil choisi.

Le profil `1920x1080-windowed` de `config.example.json` est un squelette de
configuration, pas une calibration qualifiée. Le premier run réel de cette v1 a
été calibré localement sur une zone cliente `3840x2160`; toute autre taille exige
ses propres gabarits et un smoke de qualification.

Ne jamais inscrire un chemin local sensible dans le scénario, la fixture, le skill ou un fichier versionné.

## Calibrer une seule fois

Utiliser Computer Use une fois pour un couple résolution/mode d'affichage connu :

1. Ouvrir une session D2R hors ligne sur l'écran utile.
2. Mesurer la zone cliente de la fenêtre D2R, sans prendre l'écran entier comme référentiel.
3. Relever la zone de recherche du personnage, le bouton Play, la difficulté, Save and Exit et les repères des états.
4. Convertir chaque point en coordonnées normalisées dans `config.local.json`.
5. Créer localement les petits gabarits des sondes `CHARACTER_SELECT`, `IN_GAME`, `ESC_MENU` et `INVENTORY_OPEN`.
6. Vérifier la calibration par un run borné; ne pas recalibrer à chaque scénario.

Calculer chaque clic à partir du rectangle client courant. Refuser toute coordonnée calculée hors de ce rectangle.

## Isoler les sauvegardes et préparer la fixture

Considérer qu'une simple copie de personnage dans le savepath BKVince normal est insuffisante : D2R peut aussi modifier le shared stash. Échouer sans envoyer d'input tant que l'isolement du savepath de test n'est pas prouvé.

1. Arrêter la session concernée avant de changer le savepath runtime.
2. Lire `runtimeModInfoPath`, sauvegarder le fichier modinfo byte-exact, puis rediriger son savepath vers le dossier distinct nommé par `testSavePathName`.
3. Vérifier que `testSaveDirectory` correspond à ce savepath isolé, que son nom se termine par `GameTest` et non au savepath BKVince normal.
4. Exiger le marqueur d'appartenance créé par le runner avant toute suppression dans un dossier de test déjà présent; refuser un dossier non vide non marqué.
5. Inventorier et hasher les sauvegardes originales avant le scénario afin de confirmer ensuite qu'elles n'ont pas changé.
6. Vérifier que chaque fichier autorisé existe dans `fixtureSourceDirectory` et que son SHA-256 correspond au manifeste de fixture.
7. Ouvrir la fixture source en lecture seule, calculer son hash et copier uniquement les fichiers autorisés vers le dossier de test isolé.
8. Conserver l'ancienne copie de travail dans les artefacts lorsque `preserveWorkingCopy` vaut `true`.
9. Dans un bloc de finalisation garanti, restaurer le modinfo byte-exact, même après erreur ou arrêt.
10. Vérifier à nouveau le hash de la fixture source et des sauvegardes originales.
11. Ne supprimer ou remplacer que la copie de travail du personnage de test.

Refuser une fixture source située dans un dossier de sauvegarde utilisé par D2R. Ne jamais modifier une sauvegarde originale, un autre personnage ou un shared stash. Classer l'exécution `fail` si l'isolement, la restauration byte-exact du modinfo ou l'intégrité des sauvegardes originales ne peut pas être démontré.

## Exécuter le smoke test

Depuis la racine du dépôt, lancer exactement :

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/GameTestRunner/Invoke-GameTest.ps1 -Scenario smoke-launch-save-exit
```

Laisser le scénario versionné exprimer seulement des actions nommées de haut niveau. Centraliser touches, coordonnées, délais, timeouts et sondes dans le profil local. Ne pas introduire un langage de programmation dans le JSON.

Attendre de `smoke-launch-save-exit` le trajet suivant : lancer ou réutiliser D2R, activer la fenêtre, sélectionner la copie isolée de `QtyTester`, entrer hors ligne, confirmer `IN_GAME`, ouvrir et confirmer l'inventaire, capturer `inventory-open`, fermer l'inventaire, faire Save and Exit, puis confirmer `CHARACTER_SELECT`.

Limite sûre de la v1 : même si l'action conserve son nom générique
`launch_or_reuse_game`, l'orchestrateur ferme actuellement toute session D2R,
redirige le savepath, lance une session isolée, puis ferme D2R avant de restaurer
le modinfo byte-exact. La réutilisation d'une session déjà ouverte et sa
conservation entre scénarios viendront seulement lorsqu'elles pourront préserver
le même contrat d'isolement.

## Appliquer les garde-fous d'inputs

Avant chaque bloc d'inputs :

1. Confirmer qu'un processus D2R autorisé existe.
2. Activer exclusivement la fenêtre dont le titre correspond à D2R.
3. Confirmer qu'elle est réellement au premier plan.
4. Comparer son rectangle client au profil avec la tolérance configurée.
5. Valider que chaque point d'entrée reste dans le rectangle client.
6. Annuler le bloc entier si un contrôle échoue.

Ne jamais utiliser `BlockInput`. `Pause` est le hotkey d'arrêt immédiat fixe de
la v1. Ne jamais envoyer volontairement d'inputs à Battle.net, un navigateur,
Discord, un terminal, un éditeur ou toute autre application. Ne jamais entrer
dans une partie en ligne.

## Reconnaître les états et rendre le verdict

Limiter la v1 aux états suivants :

- `NOT_RUNNING`
- `CHARACTER_SELECT`
- `LOADING`
- `IN_GAME`
- `ESC_MENU`
- `FAILED`

Utiliser d'abord le processus, le titre et le rectangle de la fenêtre; employer ensuite seulement les sondes visuelles ciblées. Considérer `INVENTORY_OPEN` comme une assertion visuelle, pas comme un nouvel état global.

Attribuer les statuts ainsi :

- `pass` : confirmer réellement toutes les transitions et assertions du scénario;
- `fail` : constater l'échec d'une étape ou d'une assertion;
- `inconclusive` : terminer le trajet sans pouvoir confirmer le résultat.

Ne jamais déclarer `pass` parce que D2R n'a simplement pas crashé. Traiter une capture graphique ambiguë comme `inconclusive` jusqu'à son analyse par Codex.

## Inspecter les artefacts

Lire le `result.json` écrit sous `game-tests/artifacts/<date>-<scenario>/` et vérifier au minimum :

- `scenarioId`, `status`, `startedAt` et `completedAt`;
- `stepsCompleted`, `failedStep` et le dernier état reconnu;
- `processAlive`, `recoveryAttempted` et `notes`;
- les captures, logs PowerShell, sorties AutoHotkey, dernières actions et timings.

À l'échec, exiger une capture de la fenêtre D2R avant récupération et conserver le résultat de cette récupération. Garder `game-tests/artifacts/` ignoré par Git, sauf `.gitkeep`.

Sur l'installation AHK/GDI+ qualifiée, le code brut du processus AHK peut être
normalisé à `0` après une capture d'échec alors que son événement JSONL est
explicitement `failed`. Le verdict et le code de sortie de
`Invoke-GameTest.ps1` sont dérivés de la séquence JSONL complète; considérer
`result.json` et ces événements comme autoritaires, jamais le seul code enfant.

## Borner la récupération

- Capturer l'état avant toute tentative de récupération.
- Autoriser au plus une récupération raisonnable par scénario.
- Arrêter rapidement si le processus disparaît, si le focus se perd, si la géométrie diverge ou si l'état reste incohérent.
- Exiger la création du flux JSONL pendant `timeoutsMs.runnerStartup`; tuer uniquement l'enfant AHK silencieux à l'expiration, puis laisser le finalizer restaurer D2R et le modinfo.
- Ne jamais relancer indéfiniment un scénario.
- Vérifier d'abord `config.local.json`, AutoHotkey v2, les chemins de fixture, l'isolement du savepath et les gabarits locaux.
- Recalibrer seulement si la taille cliente, le mode d'affichage ou l'interface ont réellement changé.
- Classer `inconclusive` plutôt que d'inventer une confirmation lorsque la sonde visuelle est ambiguë.

## Étendre seulement après le smoke

Ajouter ensuite, dans cet ordre et sans les implémenter dans la v1 : `inventory-hover-capture`, `hotkey-open-ui`, `combat-action-capture`, puis `spawn-and-interact` uniquement lorsqu'une préparation déterministe existe.

Le dossier de sauvegarde isolé peut encore conserver son shared stash entre les
runs. Le smoke n'en dépend pas; avant des scénarios déterministes sur les objets,
ajouter une politique explicite de fixture/restauration du shared stash plutôt
que de le supprimer implicitement.

### Futur préparateur Hero Editor

Utiliser à terme le codec du Hero Editor comme préparateur déterministe de fixtures : appliquer une recette versionnée à une copie de la fixture, exporter puis reparser la copie, et vérifier checksum et hash avant de la déposer dans `testSaveDirectory`. Ne pas automatiser l'interface du Hero Editor par défaut. Ne jamais ouvrir en écriture la fixture source ni une sauvegarde live; réserver l'automatisation visuelle à une opération absente du codec.

### Futur GameTestBridge

Ne pas construire `GameTestBridge` dans cette version. Envisager plus tard un plugin local de test pour préparer monstres, objets, positions, skills, points de vie et captures de dégâts. Imposer un build de test, le mode hors ligne et un refus explicite de toute partie en ligne; retirer ou désactiver le bridge dans les releases publiques.
