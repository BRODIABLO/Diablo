# Remodelage des modèles de personnages D2R

## Décision

Le 29 juillet 2026, Vincent retient une première voie de remodelage conservant
la structure du modèle D2R original. Le premier témoin sera un morph facial
localisé sur une tête joueur LOD0. Une tête générée par IA peut servir de cible
de sculpture, mais sa topologie ne devient pas directement un asset du jeu.

Le chantier ne revendique pas de pipeline générique `DAE/FBX -> GR2`. Carbon
est retenu comme lecteur/importeur de référence et D2R GPS comme second
convertisseur `GR2 -> model`, à comparer avec MoPaH/d2rpp. Les outils tiers et
leurs binaires restent locaux sous `analysis-cache`; seuls leur provenance,
leurs versions, leurs hashes et leur procédure de récupération sont versionnés
dans `scripts/d2r-assets/THIRD_PARTY_TOOLS.md`.

## Faits vérifiés

- La vidéo *Diablo 2 Resurrected 3D Model Export Guide* montre l'extraction
  `.model -> DAE` et l'import Blender, mais pas le retour vers GR2 ou D2R.
- Le pipeline local a déjà décompressé un modèle D2R, repéré dynamiquement ses
  tampons, modifié indices ou positions, recalculé le CRC et produit un modèle
  chargé par le jeu sans reconstruire les métadonnées Granny.
- Le remodelage de la tenue LOD0 de la Sorcière a été approuvé visuellement.
- L'essai de queue-de-cheval a chargé en jeu, mais son résultat visuel a été
  abandonné; cet échec artistique n'invalide pas la chirurgie binaire.
- Carbon au commit
  `cd00830df8d892249bf10fd066d28613ea65d396` lit le témoin de tête de la
  Sorcière : Granny v7, 8 sections, 8 meshes, 40 matériaux et 16 textures.
- D2R GPS Fix1 est un exécutable x64 PyInstaller non signé avec un payload
  `d2rpp`; sa publication revendique `GR2 -> model`, pas `DAE/FBX -> GR2`.
- Le mesh facial principal inspecté possède 8 219 sommets, 43 758 indices,
  quatre influences par sommet et des os dédiés aux yeux, sourcils, joues,
  nez, mâchoire et bouche. Aucun morph target facial n'a été trouvé.

## Hypothèse à tester

Un morph localisé devrait conserver les animations faciales si le pipeline ne
modifie ni topologie, ni ordre des sommets, ni poids, ni indices d'os, ni
squelette. Cette conclusion demeure une hypothèse tant qu'un témoin facial
édité n'a pas été exercé dans le runtime 3.2.92777.

Les normales et tangentes existantes peuvent suffire pour un déplacement
faible. Un changement plus important peut exiger leur recalcul et leur
réinjection avec la même identité de sommet.

## Premier témoin gouverné

1. Choisir une tête joueur LOD0 et une région visible mais limitée, par exemple
   le menton, le nez ou une pommette.
2. Produire et vérifier un no-op avant toute mutation.
3. Conserver exactement les nombres de sommets et d'indices, la topologie,
   l'ordre des sommets, les UV, les poids, le squelette, les matériaux et les
   données étendues Granny.
4. Conserver un identifiant de sommet original stable durant le passage dans
   Blender; ne jamais supposer que l'ordre DAE est préservé.
5. Réinjecter seulement les positions prouvées dans le conteneur décompressé
   original, recalculer le CRC et vérifier une différence binaire bornée.
6. Ajouter les normales et tangentes seulement si le témoin de positions met en
   évidence un défaut d'éclairage.
7. Convertir le même GR2 avec MoPaH/d2rpp et D2R GPS, comparer les structures et
   refuser toute divergence non expliquée avant le runtime.

## Gates runtime encore ouverts

- chargement à l'écran de sélection et en partie;
- idle, marche, course, attaque, sorts, dégâts et mort;
- clignement, bouche et déformations du rig facial;
- couture tête-cou, cheveux, diadèmes, casques et armures;
- éclairages variés et absence d'artefacts de normales/tangentes;
- inventaire exact des LOD et variantes réellement référencés, puis absence de
  popping lors de leur propagation;
- rollback byte-exact vers le modèle original;
- solo, hôte et joiner, sans attribuer à l'asset client une autorité gameplay.

## Prochain gate

Produire un no-op puis un morph facial LOD0 localisé conservant tous les
invariants, comparer les sorties MoPaH/d2rpp et D2R GPS dans un environnement
isolé, puis seulement déployer le modèle pour la matrice runtime.
