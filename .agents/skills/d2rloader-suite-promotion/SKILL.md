---
name: d2rloader-suite-promotion
description: Promouvoir de manière gouvernée un composant RuffnecKk depuis le laboratoire Diablo vers le dépôt public RuffnecKk D2RLoader Suite. Utiliser automatiquement dès qu'un plugin, patch ou outil qualifié doit entrer dans la Suite, rejoindre une prochaine release, changer de source autoritaire ou être synchronisé entre workspace et dépôt produit; ne pas utiliser pour l'incubation technique, l'admission d'une release D2RLoader/PluginSDK ni le simple déploiement runtime.
---

# Promotion vers la RuffnecKk D2RLoader Suite

## Déclencher automatiquement le gate

1. Appliquer ce skill sans attendre que Vincent le nomme lorsqu'un composant incubé est retenu pour la Suite ou une prochaine release.
2. Une demande d'analyse autorise seulement l'audit read-only. Exiger le `GO` du workspace avant toute copie, suppression ou modification de registre.
3. Ne pas confondre cette promotion avec `d2rloader-plugin-incubation`, `d2rloader-release-intake` ou `d2r-runtime-validation` : elle transfère l'autorité de source entre dépôts.
4. Résoudre les dépôts par `workspace-repositories.json` et ses variables d'environnement. Refuser un dépôt produit ou Governance situé sous `analysis-cache/`.

## Établir la transaction

1. Exécuter `npm run checkpoint` et lire `analysis-cache/checkpoint/state.json`.
2. Capturer pour `workspace`, `suite` et `suite-governance` : chemin, remote, branche, HEAD et statut Git. Préserver tous les changements étrangers au composant.
3. Identifier exactement :
   - la source d'incubation sous `addons/`;
   - la destination canonique `plugins/<slug>/`, `patches/<name>.json` ou `tools/<slug>/`;
   - l'entrée de registre concernée dans Governance;
   - la version, les hashes, la baseline SDK et les preuves de qualification.
4. Refuser la promotion si les gates d'incubation, de source ou de coexistence requis ne sont pas fermés.
5. Comparer les arbres source et destination fichier par fichier. Toute divergence préexistante doit être expliquée; ne jamais écraser aveuglément une source publique déjà différente.

## Promouvoir l'autorité

1. Copier uniquement les sources, manifests, tests et documentations publiables. Exclure systématiquement builds, caches, logs, preuves runtime, sauvegardes et ZIP.
2. Conserver les fichiers propres au dépôt public lorsqu'ils font partie de son outillage ou de son contrat de packaging.
3. Valider le composant dans le dépôt public avant de modifier son `sourceRef`.
4. Basculer alors le registre courant vers un `sourceRef` commençant par `suite:`. Une entrée `workspace:` ne peut jamais être `package-ready`.
5. Conserver l'origine d'incubation dans la décision ou la preuve de promotion : dépôt, chemin, commit source et empreinte de l'arbre promu.
6. Après le basculement, le dépôt public devient l'unique source active du composant livré. Le dossier `addons/` reste une provenance historique jusqu'à une suppression explicitement autorisée; ne plus y développer silencieusement la version promue.

## Fermer les gates de release

1. Vérifier que la version du composant correspond à ses métadonnées, son allowlist et au segment `v<version>` de son asset.
2. Exécuter les tests du composant, `scripts/Test-Suite.ps1`, `scripts/Test-NextRelease.ps1` et `scripts/releases/Test-SuiteGovernance.ps1` avec les chemins Governance explicites appropriés.
3. Exiger une allowlist complète et exactement concordante avant `package-ready`; un sous-dossier de hotfix ne vaut jamais catalogue complet.
4. Séparer les états `promoted`, `qualifying`, `package-ready`, `published` et `deployed`. Aucun état ne doit être déduit du suivant.
5. Ne jamais publier, taguer, committer ou pousser sans demande explicite de Vincent.
6. Ne jamais toucher au runtime dans ce workflow. Pour déployer ou valider en jeu, passer au skill `d2r-runtime-validation` et obtenir la confirmation opérationnelle requise.

## Rendre compte

Rapporter les trois états Git avant/après, les fichiers promus, les divergences résolues, les validations exécutées, la nouvelle source autoritaire et les gates encore ouverts. Rafraîchir le checkpoint après toute mutation.
