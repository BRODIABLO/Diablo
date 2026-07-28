# Equipped Item to Cube — D2R 3.2

## Statut et propriété

- Statut : **prototype autonome implanté et chargé à froid; validation manuelle joueur restante**.
- Cible : `D2R.exe 3.2.92777` sous D2RLoader.
- Catégorie confirmée par Vincent le 27 juillet 2026 : `misc`.
- Propriétaire futur : `plugin-misc.dll`.
- Clé future : `misc.equippedItemToCube`.
- Prototype : `EquippedItemToCube.dll` `0.1.0`, autonome, hybride globale/mod-locale,
  auteur `RuffnecKk`, JSON anglais et aucun TOML.

## Intention joueur

Rétablir `Ctrl + clic gauche` sur un objet équipé afin de le déplacer directement
dans le Horadric Cube ouvert, comme l’action encore annoncée par le tooltip, sans
transit manuel par l’inventaire.

## Faits vérifiés

- Le tooltip du build actuel affiche encore `Ctrl + Left Click to Move` sur un
  objet équipé lorsque le Cube est ouvert.
- Le workbench gouverné du build `92777` est vérifié : image canonique SHA-256
  `CC59119DC2A6C7D43D088098FC162EAFA4AE1299B2079126AEF43C1ACA914715`,
  image d’analyse SHA-256
  `673E8C0B2E89563E75525B24D137098EFD07B2DB4ED42ADEC56AA1ADDF0E63AB`,
  index SQLite et projet Ghidra persistants présents.
- Le helper `0x15A170` reconnaît la page native `3` comme le Horadric Cube.
- Le helper central `0x15A280` conserve les contrôles de panneau Cube, refuse le
  Cube lui-même et délègue la recherche de destination au chemin natif.
- Le handler d’objet équipé/hoveré commence à `0x228AC0`. Le build 3.2 ajoute un
  contrôle client à `0x228B91`, puis la branche de sortie `JE 0x228CF3` à
  `0x228B98`, juste avant le test natif de `Ctrl` à `0x228B9E`.
- Le chemin correspondant du dump D2R 2.4 ne contient pas cette sortie
  supplémentaire; les deux autres chemins de déplacement vers le Cube ne la
  contiennent pas non plus.
- Le prototype remplace uniquement les six octets de la branche
  `0x228B98..0x228B9D` par des NOP. Le contrôle appelé conserve ses effets de
  bord; la détection de `Ctrl`, les contrôles de place, la transaction native et
  les validations serveur restent inchangés.
- Contexte strict de 47 octets exigé à `0x228B81` :
  `48 8B CF E8 D7 E7 F9 FF 85 C0 0F 85 62 01 00 00 E8 6A A7 FB FF 84 C0 0F 84 55 01 00 00 B9 11 00 00 00 E8 58 15 FE 00 85 C0 0F 84 EF 00 00 00`.
- Audit de collision : CubeQuickMove possède `0x4BBA73`, eezstreet
  `plugin-misc.dll` possède `0x542F40`, et ce prototype possède uniquement
  `0x228B98..0x228B9D`.

## Implantation

- Identifiant : `equipped-item-to-cube`.
- Description : `Restores Ctrl-click moves from equipped slots to the Horadric Cube.`
- Configuration stricte : `EquippedItemToCube.json`, clé booléenne `enabled`
  uniquement, recherchée d’abord dans le mod actif puis dans la portée globale.
- Le build autre que `92777`, un contexte binaire différent, une branche déjà
  modifiée ou un JSON invalide entraînent un refus fermé.
- La DLL ne modifie, ne lie et ne redistribue aucune DLL d’eezstreet; elle ne
  déclare pas `ModScopedOnly`.
- Archive publique candidate : DLL + JSON uniquement; le README reste hors ZIP.

## Preuves de build et runtime

- Release x64 : succès.
- Test de politique : `1/1` réussi.
- Exports présents : `D2RLoaderGetPluginInfo`, `D2RLoaderLoadPlugin`,
  `D2RLoaderUnloadPlugin`.
- DLL source/runtime/package SHA-256 :
  `4FCEAB81E680095CBA53AD359FC5A18FFA3A2E8A2CE643A4DB527779AFA4F009`.
- ZIP SHA-256 :
  `073C96B6D2CB5A7D4E05006AC63B4713F28BBDC6266DE1EF0A0869A33549DC21`.
- Cold start mod-local final du 27 juillet 2026 : plugin `0.1.0` chargé avec
  auteur `RuffnecKk`, configuration mod-locale lue, `20/20` patchsets appliqués,
  `30` plugins scannés, `28` actifs, `2` désactivés, zéro rejet/échec et startup
  `24/24`.
- Le hash du DLL déployé est identique au hash source/package ci-dessus.

## Validation fonctionnelle

Les tentatives d’automatisation ont confirmé l’ouverture du Cube, les objets
équipés et le tooltip, mais l’environnement de contrôle ne peut pas maintenir un
vrai état `Ctrl` pendant son clic injecté dans D2R. Aucun succès fonctionnel n’est
donc revendiqué sur cette base. Un axe déjà présent dans le Cube ne constitue pas
une preuve attribuable au prototype.

Gates manuels encore ouverts :

1. confirmer un déplacement réel par `Ctrl + clic gauche` depuis chaque slot
   équipé et les deux weapon sets;
2. vérifier Cube absent, fermé, plein et fragmenté, inventaire plein et objets de
   tailles variées;
3. vérifier recalcul des statistiques, sauvegarde/rechargement et absence de
   perte, duplication ou objet fantôme;
4. couvrir souris/manette, solo, hôte/joiner et client sans plugin;
5. répéter le cold start et les cas fonctionnels en portée globale.

## Promotion future

Après fermeture de la matrice manuelle, porter la fonctionnalité et ses gardes
dans `plugin-misc.dll` sous `misc.equippedItemToCube`, compiler le pack complet,
répéter les non-régressions et retirer l’autonome seulement après validation du
binaire fusionné.

## Prochain gate

Vincent confirme en jeu le déplacement d’un objet équipé vers le Cube ouvert,
puis les refus Cube absent/plein. En cas d’échec, capturer un log frais et tracer
le premier point où le handler `0x228AC0` diverge après la branche neutralisée.

## Frontière Git

Le prototype reste autonome et ne modifie aucune DLL d’eezstreet. La demande
explicite `commit push go` autorise le checkpoint de cette tâche uniquement; les
changements concurrents du workspace restent exclus.
