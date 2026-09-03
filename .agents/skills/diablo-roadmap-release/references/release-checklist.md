# Checklist mission, archive et livraison

## Fraîcheur de mission

- La priorité courante de la ROADMAP correspond au contexte le plus récent.
- La mission distingue faits prouvés, hypothèses et gates non exécutés.
- Les décisions de Vincent et leurs dates sont consignées.
- Les versions, chemins, hashes et résultats de cold start correspondent aux artefacts présents.
- Toute case encore ouverte reste visible; aucune réussite n'est déduite d'un test voisin.

## Archive publique

- Le registre gouverné de prochaine release est valide; chaque ajout, retrait, renommage, report et changement de version y est consigné avec sa décision et ses gates.
- Les comptes plugins, patches, outils, bundles et assets totaux sont dérivés du registre et concordent avec l'allowlist finale.
- `releaseReady=true` et `package-ready` ne sont définis qu'après fermeture de tous les gates requis, notamment Steam lorsqu'il fait partie de la revendication.
- L'allowlist finale correspond exactement aux composants, versions et assets inclus dans le registre; aucun composant retiré ou reporté ne subsiste.
- Une allowlist exacte a été définie avant création.
- Le README a été créé ou actualisé et placé à côté du ZIP dans le dossier de livraison pour la relecture et la modification humaines de Vincent.
- Le README n'est pas inclus dans le ZIP généré par l'agent; Vincent l'ajoutera lui-même au ZIP final après sa révision.
- Les fichiers proviennent du build et de la configuration réellement validés.
- La liste des entrées du ZIP a été inspectée après création.
- Aucun README, source, symbole, log ou fichier de preuve interdit n'est inclus dans le ZIP généré par l'agent; pour un plugin incubé, un fichier de configuration indépendant JSON ou TOML accompagne la DLL seulement si le contrat confirmé en a réellement besoin.
- Aucune DLL tierce n'est redistribuée sans autorisation et crédits appropriés.
- Le SHA-256 du ZIP et des artefacts distribués est consigné dans la mission.

## Contrôles dépôt

```powershell
git status --short --branch
git diff --check
git diff --stat
node scripts/validate-cartographie/validate.mjs
```

Ajouter les tests/builds spécifiques au composant. Si des fichiers ou dossiers ont été ajoutés, supprimés ou renommés, régénérer le cadastre avant son validateur.

## Gate Git

- Ne pas changer de branche, committer ou pousser de sa propre initiative.
- Une demande explicite de l’utilisateur courant suffit; aucune formule `GO`
  dédiée ni identité particulière n’est requise.
- Autoriser ensemble les actions clairement demandées ensemble, par exemple
  « commit ces changements puis push ».
- Une demande de livraison ou d’archive ne constitue pas automatiquement une
  demande de commit ou de push.
