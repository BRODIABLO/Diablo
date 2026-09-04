---
name: d2r-runtime-validation
description: Déployer un changement Diablo ou D2RLoader dans le profil runtime approprié, gérer les processus qui verrouillent les fichiers, collecter des logs frais et exécuter une matrice de validation technique et fonctionnelle. Utiliser ce skill pour synchroniser data-BKVince ou data-TCP vers un jeu installé, tester une DLL/configuration, réaliser un cold start ou confirmer un comportement en jeu.
---

# Validation runtime D2R

## Préparer une validation traçable

1. Exécuter `npm run baseline:d2rloader -- status`. Si le loader ciblé n'est ni la baseline promue ni un candidat enregistré, appliquer `d2rloader-release-intake` avant toute revendication de compatibilité.
2. Identifier la source gouvernée, le profil runtime exact, le build D2R, le canal Battle.net ou Steam et la portée globale ou mod-locale. Ne jamais supposer un chemin d'installation à partir d'un autre poste.
3. Définir avant copie la liste fermée des fichiers autorisés et les résultats attendus.
4. Relever les hashes source et l'état des logs avant le test. Ne pas mélanger des logs anciens avec le démarrage courant.
5. Établir la matrice fonctionnelle depuis la mission; séparer les gates statiques, cold-start, visuels, gameplay et multijoueur.

## Demander avant de contrôler les processus

1. Avant tout arrêt, lancement, redémarrage ou cold start de D2R/D2RLoader, présenter à Vincent le profil, l'action et la séquence bornée prévue, puis attendre sa confirmation explicite.
2. N'imposer aucune formule particulière : toute réponse claire comme `ok`, `go` ou `vas-y` autorise la séquence annoncée. Un accord donné auparavant pour implanter, construire, emballer, committer ou pousser ne vaut pas automatiquement autorisation de contrôler le runtime.
3. Observer d'abord les processus et leur ligne de commande. Si une instance est active, signaler qu'elle peut appartenir à une autre session de test et ne jamais la fermer avant confirmation.
4. Une confirmation couvre uniquement la séquence annoncée. Demander de nouveau avant tout arrêt, lancement ou redémarrage supplémentaire qui n'en faisait pas partie.
5. Sans confirmation, poursuivre uniquement les contrôles statiques, builds, tests hors jeu et plans non mutants; laisser les cases runtime `not run`.

## Détecter une dérive du build officiel

1. Avant toute copie ou relance, relever la version et la Build Key de `.build.info`, son SHA-256, puis la version et le SHA-256 de l'exécutable du jeu. Comparer ces preuves à la baseline gouvernée; un horodatage seul ne prouve pas un changement de build.
2. Après une opération Battle.net, distinguer une réparation ou réapplication du même build d'un nouveau build. Si les identifiants et hashes gouvernés restent identiques, conserver la baseline tout en revérifiant les fichiers runtime ciblés.
3. Lorsqu'une nouvelle version produit conserve une image native byte-identique prouvée, rattacher les preuves binaires au hash commun mais nommer la baseline, les logs, la documentation et les livraisons avec la nouvelle version officielle. Une provenance historique ne devient jamais le nom du runtime courant.
4. Si le build a changé, séparer les données softcodées des composants natifs. Un port TXT peut poursuivre sa propre matrice ciblée, mais aucun plugin, patch mémoire, RVA, signature ou ABI n'hérite automatiquement de sa compatibilité.
5. Suspendre toute revendication native sur un hash binaire différent jusqu'à l'obtention des preuves propres à cette image. Ne jamais convertir une validation data en compatibilité générale de type « version précédente+ ».

## Appliquer la qualification par corpus natif équivalent

1. Traiter D2R `3.2.92777`, Battle.net `3.3.93847` et Steam `3.3.93787` comme des candidats admissibles, jamais comme une allowlist, puis identifier la version officielle courante qui recevra la matrice runtime complète.
2. Déployer et tester l'artefact sur ce runtime, puis prouver son SHA-256 dans le build, le package et chaque portée testée.
3. Exiger des tests de politique qui interdisent toute allowlist runtime de build-name, de canal ou de version, ainsi que toute comparaison autorisante ou bloquante avec `92777`, `93847`, `93787` ou un autre numéro. Vérifier plutôt que l'empreinte native complète accepte ses témoins exacts et refuse toute différence avant le premier hook; les identifiants restent diagnostiques.
4. Le corpus gouverné commun couvre déjà `92777` et `93847`. Steam `93787` peut hériter sans matrice runtime dupliquée uniquement lorsque son exécutable identifié et les preuves gouvernées établissent byte-exact chaque RVA, signature, témoin de layout/ABI et plage utilisée.
5. Exécuter une qualification séparée dès qu'une surface native, l'environnement D2RLoader ou un comportement observé diffère. Toujours nommer le runtime testé, les builds couverts uniquement par équivalence et les candidats seulement admissibles.
6. Tester séparément la compatibilité multijoueur entre deux builds; une empreinte native acceptée de part et d'autre ne prouve ni protocole, ni négociation, ni autorité réseau compatibles.

## Libérer et synchroniser les fichiers

1. Si Diablo, D2RLoader ou le profil ciblé verrouille un fichier, arrêter la procédure et demander confirmation avant de contrôler les processus. Après confirmation, fermer soi-même uniquement les instances annoncées; ne pas demander à Vincent de fermer le jeu.
2. Vérifier que les processus sont réellement terminés avant toute copie.
3. Pour les données du mod, utiliser `scripts/runtime/Sync-BKVince.ps1`. Pour une release de la RuffnecKk Suite, utiliser `scripts/runtime/Sync-SuiteRelease.ps1` avec l'allowlist Governance exacte, le staging d'artefacts et la portée explicite; ne jamais remplacer l'un de ces contrats par l'autre.
4. En mode Suite, exécuter d'abord `Plan` sans contrôle de processus. `Apply` et `Rollback` exigent la confirmation opérationnelle de Vincent et le switch `-ConfirmRuntimeControl`; conserver les configurations joueur par défaut et n'utiliser `-OverwriteConfiguration` que si le remplacement exact a été annoncé et autorisé.
5. Copier uniquement les cibles gouvernées prévues. Refuser une synchronisation large qui emporterait des changements non liés, tout doublon global/mod-local et toute source dont le SHA-256 diffère de l'allowlist.
6. Conserver le reçu et les sauvegardes sous `analysis-cache/runtime-deployments/suite/`, puis exécuter `Verify`. Utiliser le reçu exact pour tout rollback; ne jamais reconstruire une restauration de mémoire.
7. Recalculer les SHA-256 dans le runtime et exiger leur égalité avec les sources.
8. Pour un plugin hybride, tester explicitement les portées requises et vérifier la neutralisation attendue des doublons par identifiant.

## Effectuer le cold start

1. Vérifier qu'une confirmation explicite couvre ce cold start précis. Sans elle, ne lancer ni relancer aucune instance.
2. Relancer une seule instance si la validation autorisée l'exige, avec le profil et les arguments établis par la mission ou la configuration locale.
3. Collecter uniquement les nouvelles lignes de log ou les fichiers dont l'horodatage appartient au test.
4. Vérifier l'identité de build journalisée, l'acceptation de l'empreinte native, la version du plugin, la résolution de configuration, les signatures, hooks et patches, les compteurs de plugins actifs/rejetés/échoués et la fin complète du démarrage.
5. Distinguer les erreurs causées par le changement des incidents déjà connus et documentés. Ne pas masquer un nouvel échec sous un bruit historique.

## Fermer la matrice fonctionnelle

1. Tester les valeurs par défaut, bornes, valeurs invalides et repli de configuration.
2. Tester les actions joueur, transitions, réouvertures, sauvegardes/reprises et périphériques pertinents.
3. Couvrir solo, hôte et client lorsque la fonctionnalité peut toucher le réseau ou l'état partagé.
4. Noter chaque case `passed`, `failed`, `blocked` ou `not run`, avec preuve et date. Ne jamais transformer `not run` en succès implicite.
5. Mettre à jour la mission et la ROADMAP avec les preuves obtenues et les gates restants; ne déclarer la livraison fonctionnelle qu'après observation en jeu.

Lire [references/validation-matrix.md](references/validation-matrix.md) pour la séquence processus-déploiement-logs et le format de matrice recommandé.
