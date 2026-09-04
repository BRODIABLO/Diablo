---
name: diablo-roadmap-release
description: Maintenir la mission courante et ROADMAP.html, séquencer une nouvelle tâche significative, préparer une archive publique et exécuter les contrôles avant livraison. Utiliser ce skill lorsqu'un chantier Diablo émerge, lorsqu'une tâche ou un commit est terminé, lorsqu'une mission doit refléter des preuves/gates à jour, ou avant de livrer un addon, un ZIP ou une release.
---

# Roadmap et livraison Diablo

## Retrouver la mission courante

1. Lire les incréments `EN COURS`, les notes de priorité et les gates ouverts dans `ROADMAP.html`.
2. Croiser ces informations avec les fichiers `Mission/`, le contexte de conversation et les changements Git récents.
3. Préférer la priorité explicite la plus récente. Si plusieurs missions restent réellement concurrentes, valider inline le prochain pas avec l'utilisateur.
4. Au début et à la fin d'une tâche, vérifier que la mission et la ROADMAP décrivent encore le même état.

## Traiter une nouvelle tâche significative

1. Proposer à l'utilisateur de l'ajouter à la ROADMAP.
2. Ne pas modifier la ROADMAP tant que l'utilisateur n'a pas confirmé cet ajout.
3. Après confirmation, analyser le placement selon la valeur métier et l'efficacité d'avancement pour le projet humain.
4. Produire deux séquencements logiques et cohérents, avec leurs dépendances et compromis, puis demander lequel retenir.
5. Insérer seulement l'option choisie, avec un identifiant stable, un propriétaire, des gates observables et le bon statut.

## Tenir les preuves à jour

1. Consigner dans la mission les décisions produit, preuves techniques, hashes, validations runtime et cases encore ouvertes.
2. Marquer un jalon livré seulement lorsque ses gates requis sont réellement fermés. Distinguer `livré techniquement`, `validé en jeu`, `abandonné` et `bloqué`.
3. Actualiser la date, la note de priorité et le résumé de bas de page de la ROADMAP sans écraser les changements concurrents.
4. Après une modification structurelle, exécuter le workflow cadastre du skill `diablo-tsv`.

## Gouverner la prochaine release de la Suite

1. Résoudre le dépôt privé Governance par `workspace-repositories.json`, puis mettre à jour son registre versionné `releases/<version>/next-release.json` dès qu'une décision ajoute, retire, renomme, reporte ou change la version d'un composant. Conserver la formulation de la décision, le statut des gates et une note de release; ne pas attendre la création des ZIP.
2. Lorsqu'un composant inclus pointe encore vers `workspace:`, appliquer automatiquement le skill `d2rloader-suite-promotion` avant d'autoriser `package-ready`. Le dépôt public Suite est l'unique source de packaging d'un composant promu.
3. Traiter ce registre comme source de vérité du périmètre et des comptes dérivés. L'allowlist de release reste la source de vérité des chemins, versions finales et SHA-256 des artefacts, mais elle doit correspondre exactement au registre avant packaging.
4. Distinguer pour chaque composant les gates source, build, runtime Battle.net, runtime Steam et packaging. Une ancienne preuve ou un candidat admissible ne ferme pas le gate de l'artefact final.
5. Ne mettre `releaseReady=true` et le statut `package-ready` qu'après fermeture de tous les gates requis. Le générateur doit refuser une allowlist divergente, une version non verrouillée, un composant reporté présent ou un compte d'assets écrit en dur qui ne correspond plus aux entrées.
6. Générer ou actualiser les notes lisibles depuis les décisions enregistrées, puis vérifier manuellement leur qualité. Le changelog ne remplace pas le registre gouverné.

## Préparer une livraison

1. Valider le registre de prochaine release, puis définir une allowlist explicite et exactement concordante du contenu public. Pour un plugin incubé, appliquer le skill `d2rloader-plugin-incubation`.
2. Créer ou actualiser le README de la release et le déposer à côté du ZIP dans le dossier de livraison. Ne jamais l'inclure dans l'archive générée par l'agent : Vincent le relit et le modifie humainement avant de l'ajouter lui-même au ZIP final.
3. Construire l'archive depuis des artefacts validés, inspecter ses entrées réelles et vérifier l'absence de README, sources, secrets, logs ou preuves non destinés au public. Autoriser un compagnon runtime seulement lorsqu'il appartient au plugin, est indispensable et figure exactement dans le registre et l'allowlist.
4. Calculer les SHA-256 des artefacts et prouver leur égalité avec les fichiers testés dans le runtime.
5. Exécuter les tests ciblés, le build, la validation du cadastre si nécessaire, le cold start et la matrice fonctionnelle requise.
6. Garder les ZIP de travail sous `addons/` locaux et gitignorés. Publier les artefacts validés comme assets de la GitHub Release du dépôt produit approprié, vérifier leur digest distant, et conserver les anciennes releases/tags sans recopier leurs binaires dans le dépôt principal.
7. Examiner `git status`, `git diff --check` et le diff complet. Ne changer jamais de branche et ne jamais commit/push de sa propre initiative; une demande explicite de l’utilisateur courant suffit, sans formule `GO` dédiée ni identité particulière.
8. Rappeler à Vincent de commit et push par petits lots cohérents avec des messages clairs lorsque le travail est prêt.

Lire [references/release-checklist.md](references/release-checklist.md) pour le contrôle final d'archive, de mission, de ROADMAP et de Git.
