# BKVince Hero Editor

Éditeur de sauvegarde D2R construit pour les données actuelles de BKVince. Le
projet reprend le workspace historique `apps/hero-editor/`, mais remplace son
prototype de forge d'objets par des tranches verticales d'édition complètes.

La première tranche permet de :

- ouvrir un D2S BKVince v105 ou créer un héros vierge parmi les classes que le
  codec sait réellement sérialiser ;
- modifier les écrans General et Stats ;
- annuler et rétablir les modifications ;
- télécharger une copie après validation de la taille, du checksum et de la
  relecture du fichier ;
- restituer les octets source sans réécriture lorsqu'aucun champ n'a changé.

Les presets de personnages et builds prédéfinis sont volontairement exclus.
Les sauvegardes restent dans le navigateur et ne sont envoyées à aucun serveur.

## Commandes

Depuis la racine du workspace :

```powershell
npm.cmd run dev -w apps/hero-editor
npm.cmd test -w apps/hero-editor
npm.cmd run build -w apps/hero-editor
```

`npm run generate` régénère
`src/data/bkvince-constants.generated.js` depuis les tables TXT et chaînes
gouvernées de BKVince. Le générateur impose un aller-retour TSV byte-exact avant
d'utiliser les tables.

## Limite du gate courant

Les contrôles automatiques prouvent la structure, la taille, le checksum, la
relecture et la préservation no-op. La validation runtime D2R 3.2.92777 d'un
personnage vierge puis d'un personnage modifié reste obligatoire avant de
présenter les exports édités comme définitivement compatibles en jeu.
