# Readable Items / Clue Scrolls — D2R 3.2.92777

Dernière mise à jour : 24 juillet 2026

## Décision produit confirmée

Vincent a demandé le démarrage le 24 juillet 2026 après avoir retenu l’option B.
`ReadableItems.dll` demeure un plugin autonome : il n’a pas de catégorie ni de
DLL propriétaire dans le PluginPack et aucun merge dans une DLL d’eezstreet
n’est planifié.

Le livrable final devra rester hybride, attribué exactement à `RuffnecKk` et
installable soit globalement, soit dans le dossier d’un mod, sans
`ModScopedOnly`. Sa configuration autonome sera `ReadableItems.json`, en
anglais, recherchée d’abord dans le mod actif puis dans le dossier global. Il
n’utilisera pas de TOML et ne modifiera, ne liera ni ne redistribuera aucune DLL
d’eezstreet.

## Besoin joueur

- un objet configuré affiche `Right-click to read...` dans son tooltip;
- un clic droit ouvre un texte borné, enveloppé et défilant dans le style des
  dialogues de PNJ;
- lire ne consomme, ne déplace et ne modifie jamais l’objet;
- la fixture initiale utilise le code technique `dmy` uniquement pour tester le
  framework; le moddeur choisira ensuite son véritable objet, son titre et son
  texte;
- la phase 2 ajoutera un fichier audio optionnel par texte, avec arrêt,
  interruption et relecture déterministes.

## Faits vérifiés

### Données natives

- `misc.txt` conserve ses CRLF et son round-trip byte-exact avec
  `scripts/build-data/tsv.js`.
- Le record vanilla `bkd` (`Key to the Cairn Stones`) porte `useable=1`,
  `pSpell=14`, `spelldesc=1`, `spelldescstr=ReadScroll` et le son
  `item_scroll`.
- La localisation native `ReadScroll` vaut `Right Click to Read` en anglais et
  `Clic droit pour lire` en français.
- La documentation primaire D2R 3.2 identifie `pSpell=14` comme
  `SkillItemSendQuestInfo`: il envoie uniquement les informations de la quête
  du Scroll of Inifuss. Ce n’est pas une fonction générique pour afficher un
  texte arbitraire.

### Chemins natifs et collisions

- Le handler serveur d’utilisation d’objet est
  `D2GAME_HandleUseItemPacket` au RVA `0x4F40C0`. Son prologue strict commence
  par `40 55 53 56 57 41 54 41`; le GUID de l’objet source est lu à
  `packet+0x0A` et résolu par `SUNIT_GetServerUnit 0x48FE80`.
- `Transmogrify` possède déjà ce hook et délègue les objets non configurés à la
  fonction originale. `ReadableItems` ne peut donc pas installer un second
  hook concurrent lorsque les deux plugins sont présents.
- Le pipeline de tooltip au RVA `0x2BD480` appartient à
  `ExtendedItemStats` lorsqu’il est chargé; sinon `Transmogrify` peut le
  posséder. Readable Items devra fournir un transform déléguable et ne devenir
  propriétaire du hook que lorsqu’aucun propriétaire existant n’est actif.
- Le renderer Direct3D 12 de `FloatingDamage` n’accepte actuellement qu’un seul
  callback externe, déjà utilisé par `ExtendedItemStats`. Enregistrer un second
  callback ou poser un second hook Present écraserait un consommateur existant.
- L’audit du PluginPack officiel épinglé au commit
  `dc75b49ffbb67b887d7757ee00ee9a03bcde5d8a` n’a trouvé aucun framework de
  clic droit, de tooltip lisible ou de dialogue réutilisable.

## Hypothèses à tester

- Une primitive UI native du build 92777 peut peut-être afficher un panneau de
  texte sans interaction avec un véritable PNJ.
- À défaut, un renderer autonome peut coexister s’il dispose d’un protocole de
  délégation multi-consommateurs et d’un fallback qui ne double-hooke pas
  Direct3D 12.
- Le clic droit et le tooltip peuvent utiliser le même modèle de propriété
  dynamique : Readable Items possède le hook lorsqu’il est seul et enregistre
  ses callbacks auprès du propriétaire déjà chargé lorsqu’il coexiste.

Ces hypothèses ne sont pas encore des preuves et n’autorisent pas une DLL.

## Socle de configuration démarré

Le premier contrat, couvert par tests C++ purs, accepte :

```json
{
  "enabled": true,
  "tooltip": "Right-click to read...",
  "items": [
    {
      "code": "dmy",
      "title": "Clue Scroll Test",
      "text": "A bounded dummy message used only to validate Readable Items."
    }
  ]
}
```

Contraintes initiales : code ASCII visible de 1 à 4 octets, codes uniques,
maximum 256 objets, titre non vide de 128 octets maximum et texte non vide de
8 192 octets maximum. Une clé inconnue ou une configuration invalide est
refusée. Le champ audio ne sera ajouté qu’en phase 2 afin de ne pas annoncer une
fonction silencieusement ignorée.

## Prochain gate

Identifier et prouver sur 92777 une surface d’affichage client autonome : ABI,
fonction bornée, callers, signature stricte et absence de collision. Définir
ensuite le protocole explicite de délégation pour `0x4F40C0`, `0x2BD480` et le
renderer, avec fonctionnement autonome lorsque les autres plugins sont absents.

Seulement après ce gate : ajouter `plugin.cpp`, compiler une DLL Release et
déployer la fixture `dmy` dans le runtime de test.

## Matrice avant livraison

- configuration absente, désactivée, valide, invalide et dupliquée;
- fixture `dmy`, puis véritable code choisi par le moddeur;
- souris et manette;
- inventaire, Cube, stash personnel et stash partagé;
- texte court, long, wrapping, défilement, localisation, résolutions et
  échelles UI;
- ouvertures répétées, changement d’objet et fermeture;
- coexistence avec ExtendedItemStats, AdvancedItemTooltips et Transmogrify,
  sans hook concurrent;
- installation globale et mod-locale, cold start sans rejet ni échec;
- phase 2 séparée : audio absent, valide et invalide, synchronisation,
  interruption, changement d’objet et relecture.

## Livraison interdite à ce stade

Aucune DLL et aucun ZIP public ne sont autorisés tant que le gate de rendu et
de coexistence n’est pas fermé, puis validé en jeu.
