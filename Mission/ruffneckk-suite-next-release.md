# RuffnecKk D2RLoader Suite — prochaine release publique

Dernière mise à jour : 2 septembre 2026

## Décision et priorité

Vincent choisit le 2 septembre 2026 de ne plus retarder la mise à jour publique
de la Suite pour attendre MapSense. Il autorise l'option **registre et outillage
d'abord** : le périmètre de la prochaine release doit être enregistré avant les
migrations, builds, qualifications et archives finales.

La dernière release GitHub publique vérifiée est `v1.2.0`. Le prochain numéro
reste donc `1.3.0`; l'ancien candidat local non publié ne constitue pas une
release publique.

Le registre autoritaire est
`manifests/next-release.json` dans le dépôt produit
`RuffDood/RuffnecKk-D2RLoader-Suite`. Il gouverne les décisions, versions
cibles, retraits, reports et gates. `manifests/release-allowlist.json` conserve
un rôle distinct : chemins et SHA-256 exacts des artefacts finalisés. Le
packaging doit refuser toute divergence entre les deux.

## Périmètre verrouillé

Le catalogue visé contient **22 plugins, 17 memory patches, un outil autonome
et deux bundles**, soit **42 assets GitHub**.

Décisions explicites de Vincent :

- `Cast Triggers` entre en release comme `1.0.0`, et non `0.1.0`;
- le produit fonctionnel Armageddon devient
  `Armageddon-Hurricane CtC Fix 1.0.0`;
- `Resistance Floor 1.0.0` entre dans cette release;
- `Remote Stash 2.3.0` entre dans cette release;
- `Vendor Stock Refresh 2.0.0` entre dans cette release;
- `Shadow Master AI Fix` entre comme memory patch;
- `ISC12 0.2.1` entre comme plugin et son `ISC12 Save Converter 0.2.1`
  accompagne la même release comme EXE autonome;
- les deux presets `Ground Item Label Limit` sont retirés;
- `Normal Area Scaling` reste retiré conformément au candidat 1.3.0 existant;
- `Extended Act Level IDs` est reporté;
- la feature update MassID PluginSDK v4 est reportée; Vincent l'a nommée
  `1.2.0`, tandis que le candidat du dépôt est `2.1.0`, donc ce numéro devra
  être réconcilié avant une future promotion;
- MapSense reste hors release et ne la bloque pas.

Le composant MassID de base demeure dans la Suite et doit recevoir seulement
la migration de compatibilité nécessaire. Son prochain numéro de maintenance
reste volontairement non verrouillé dans le registre afin d'empêcher un
packaging prématuré.

## Contrat Steam

D2R `3.3.93787` Steam est admissible, mais pas encore qualifié. Aucun plugin ne
peut autoriser ou refuser son chargement d'après son canal, son build-name ou
son numéro de version. Chaque DLL doit valider avant mutation la totalité de ses
RVA, signatures, témoins de layout/ABI et plages possédées.

La release ne peut revendiquer Steam qu'après identification de l'exécutable,
logs frais de chaque empreinte et matrice runtime complète avec tous les
composants actifs de la Suite ainsi que les cinq plugins eezstreet. La
compatibilité multijoueur reste un gate distinct lorsqu'elle est revendiquée.

## Outillage gouverné

Le dépôt produit contient :

- `manifests/next-release.schema.json`, contrat lisible du registre;
- `scripts/Test-NextRelease.ps1`, qui dérive les comptes, vérifie les décisions,
  versions, gates, inclusions/retraits/reports et peut produire les notes;
- un gate dans `scripts/New-Release.ps1` qui exige `package-ready`, Steam
  qualifié et concordance exacte avec l'allowlist avant tout asset;
- un test CMake dédié afin que le registre reste validé même pendant la phase
  `scope-locked`.

Les configurations de plugin deviennent optionnelles par contrat : zéro ou une
configuration justifiée par composant. Un simple booléen `enabled` ne doit pas
forcer la création d'un fichier.

## Gates ouverts

1. Importer ou synchroniser dans le dépôt produit ISC12, le convertisseur,
   Cast Triggers, Armageddon-Hurricane CtC Fix, Resistance Floor, Shadow Master
   AI Fix, Bulk Currency Deposit 1.1.1, Burn Damage Fix 1.0.0 et Remote Stash
   2.3.0 sans perdre les changements locaux existants.
2. Verrouiller le numéro de maintenance MassID compatible avec cette release et
   confirmer que la feature PluginSDK v4 reportée n'y entre pas.
3. Fermer les gates source/build/package des nouveaux composants et les gates
   runtime exacts encore ouverts pour Vendor Stock Refresh et Remote Stash.
4. Qualifier les 22 plugins et 17 patches sur Steam avec la pile complète,
   puis consigner l'exécutable, les logs et les hashes réellement testés.
5. Promouvoir une allowlist de 42 assets exactement concordante, passer deux
   générations reproductibles, comparer les catalogues et inspecter chaque ZIP
   ou EXE.
6. Actualiser README et notes publiques, laisser chaque README humain à côté de
   son archive et hors des assets générés, puis obtenir l'autorisation séparée
   avant commit, push, tag ou publication GitHub.

## Rollback

Les releases et tags `v1.0.0`, `v1.1.0` et `v1.2.0` restent intacts. Tant que
le registre vaut `releaseReady=false`, aucun artefact 1.3.0 ne peut être généré
par le chemin canonique. Le rollback de cette gouvernance consiste à restaurer
les scripts et manifests du dépôt produit sans modifier les anciennes releases.
