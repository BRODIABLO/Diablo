# Readable Items / Clue Scrolls — D2R 3.2.92777

Dernière mise à jour : 26 juillet 2026

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

Ces hypothèses n’autorisent toujours pas une release publique. Vincent a toutefois
autorisé explicitement le 26 juillet 2026 une version test : elle doit rester un
témoin mod-local délégué et ne vaut pas encore preuve du fallback autonome final.

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

Le 26 juillet 2026, Vincent a explicitement démarré ce chantier en parallèle de
Transmogrify. Le noyau C++ du lecteur couvre désormais l’ouverture d’un objet
valide, la copie sûre du titre et du texte, le défilement par lignes avec
bornage, le redimensionnement du viewport, la remise à zéro lors d’un changement
d’objet et le nettoyage complet à la fermeture. Les deux tests Release passent,
y compris la configuration autonome livrée et sa fixture `dmy`.

La version 0.2.1 mesure désormais le wrapping dans la surface cliente active et
sélectionne la police `Formal 436` déjà embarquée par l’hôte de rendu, avec un
repli sur la police courante lorsque cet index n’existe pas. L’état pur couvre
aussi la révélation progressive, le suivi automatique de la dernière ligne et
la suspension de ce suivi pendant une relecture manuelle.

## Témoin mod-local 0.2.1 prêt à retester

Le témoin demandé par Vincent le 26 juillet 2026 est compilé et synchronisé dans
le runtime BKVince. `ReadableItems.dll` demeure une DLL distincte attribuée à
`RuffnecKk` et sans `ModScopedOnly`. Elle n’installe aucun nouveau hook D2R :

- `ExtendedItemStats` appelle son transform de tooltip puis son renderer;
- `Transmogrify`, propriétaire de `0x4F40C0`, lui délègue d’abord les objets
  configurés et conserve son comportement pour tous les autres;
- `FloatingDamage` reste le propriétaire Direct3D 12 existant; Readable Items
  est dessiné à travers le callback déjà possédé par `ExtendedItemStats`.

Cette chaîne ferme la collision pour le témoin combiné, mais ne constitue pas
encore le fallback autonome lorsque ces DLL sont absentes. Elle ne modifie, ne
lie ni ne redistribue aucune DLL d’eezstreet.

La configuration conserve `dmy` pour les tests purs et ajoute `tsc` comme témoin
réellement achetable : un Scroll of Town Portal libre. Son texte long permet de
tester wrapping et défilement; le clic droit doit être intercepté avant la
consommation vanilla.

Le premier essai visuel de Vincent a échoué au tooltip le 26 juillet 2026 : la
capture montre le `tsc` bleu survolé sans `Right-click to read...`. La cause est
prouvée dans `D2RL-Plugins@dc75b49`,
`src/plugin-shared/include/plugin-shared.h:530-537` : un code item compilé de
trois caractères est complété par un espace ASCII. La version 0.1.0 produisait
`tsc\0` (`0x00637374`) au lieu de `tsc ` (`0x20637374`) et ne pouvait donc pas
retrouver l’entrée. La version 0.1.1 initialise désormais les quatre octets avec
des espaces avant de recopier le code JSON; des tests exacts couvrent désormais
les codes de un, trois et quatre caractères.

Le second essai de Vincent a confirmé que le tooltip et le clic droit ouvrent
bien le lecteur, mais a invalidé la présentation 0.1.1 : le grand panneau
technique centré, son titre, ses instructions et son compteur ne correspondent
pas au texte défilant d’un dialogue de PNJ montré dans la capture de référence.
La version 0.2.0 remplace cette liste par un panneau supérieur noir translucide,
un double cadre doré, une police narrative blanche, une révélation progressive,
un suivi automatique, des flèches et une barre de défilement dorées. Un clic
dans le texte ou Space/Enter révèle tout; Escape ferme; Up/Down et Page Up/Page
Down permettent la relecture.

La capture suivante de Vincent confirme le 26 juillet le panneau supérieur et
la police, mais invalide deux interactions 0.2.0 : le curseur de scrollbar ne se
laisse pas manipuler et aucun bouton souris ne ferme le dialogue. Vincent juge
aussi les 38 caractères par seconde trop rapides par rapport à Charsi. La 0.2.1
ralentit donc le reveal à 18 caractères par seconde, accepte le clic direct dans
la piste et le glisser-déposer du curseur, puis ajoute un bouton doré libellé
exactement `Close`, sans chevrons, qui ferme immédiatement le lecteur.

Le premier lancement 0.2.1 a ensuite produit une régression distincte : aucun
panneau ne s’ouvrait et le log Readable Items ne recevait aucun événement
`opened`. Le diagnostic a prouvé que le runtime mod-local avait été remplacé en
parallèle par `Transmogrify 1.2.3`, SHA-256 `1A46F477...71CF438`, sans les chaînes
`ReadableItemsHandleUseItem` ni `ReadableItemsTransformTooltip`. La DLL gouvernée
`Transmogrify 1.3.1`, SHA-256 `EB1A0052...68026`, contient les deux délégations;
elle a été resynchronisée byte-exact dans le runtime et le cold start conjoint
0.2.1/1.3.1 est vert. Le comportement gameplay reste à retester.

Preuves techniques du 26 juillet 2026 :

- Readable Items 0.2.1 : 2/2 tests Release, dont le padding ItemsTxt exact,
  la révélation progressive, le suivi/repli manuel et le déplacement absolu
  borné employé par le drag;
- Transmogrify : 1/1 test Release après ajout de la délégation;
- ExtendedItemStats : 2/2 tests Release après ajout du transform et du rendu;
- manifeste v2 et six exports présents dans `ReadableItems.dll`;
- hashes source/runtime identiques pour les quatre fichiers déployés;
- cold start 0.2.1 : 20/20 patchsets, 24 plugins actifs, zéro rejet, zéro échec et
  séquence D2R complète jusqu’à 24/24;
- aucune erreur fraîche dans Readable Items, Transmogrify, ExtendedItemStats ou
  FloatingDamage.

SHA-256 :

- `ReadableItems.dll` 0.2.1 : `DC3C1EE68839A363CA8195766617AA89E6A8708B4F64823FAC355D45D6A5D45F`;
- `ReadableItems.json` : `355D2BAAD8E8F83E357CA0A47CA255DD2FA3FB53CB70EC716743A373B7992FED`;
- `Transmogrify.dll` hôte : `EB1A0052D71EB4D640FC179636B09B217DAD52CDD2135965A5BD47E3BC468026`;
- `ExtendedItemStats.dll` hôte : `13B3B67BB69ACFA63947E7A225F7E2A83A88774B2C86E39726DDFA3D9566F341`.

Pour rendre le checkpoint Readable Items autonome sans absorber le chantier
pondéré 1.3.1, le commit embarque aussi un hôte minimal Transmogrify 1.2.4 bâti
depuis `HEAD` avec uniquement les délégations tooltip et clic droit. Son SHA-256
est `1BC2CB95F8F1613E22B3BEE3885FBEA2EBE3F3F6E7AAB1F6CAFB238AF87EFD3B`.
Le runtime de test reste volontairement sur le témoin 1.3.1 ci-dessus; les deux
versions exposent les mêmes deux appels attendus par Readable Items.

Matrice fonctionnelle actuelle :

| Cas | Attendu | Statut |
|---|---|---|
| Console `readable-items preview` | panneau visible sans objet | not run |
| Tooltip d’un `tsc` libre | `Right-click to read...` | pass visuel en 0.1.1 |
| Clic droit `tsc` | lecteur ouvert, objet intact | failed avec hôte 1.2.3 sans délégation; hôte 1.3.1 restauré, à retester |
| Présentation PNJ | panneau supérieur, cadre doré, police Formal, sans UI technique | pass visuel en 0.2.0 |
| Rythme de reveal 0.2.1 | environ 18 caractères/s, proche de Charsi | not run |
| Piste et drag de scrollbar 0.2.1 | clic et déplacement proportionnel bornés | failed en 0.2.0; correction à retester |
| Flèches, Up/Down et Page Up/Page Down | défilement borné | not run |
| Clic/Space/Enter | révèle immédiatement tout le texte | not run |
| Bouton `Close` | ferme immédiatement le dialogue | absent en 0.2.0; correction à retester |
| Escape puis réouverture | fermeture puis reprise progressive ligne 1 | not run |
| Quantité et position | strictement inchangées | not run |
| Transmogrify et ExtendedItemStats | comportements existants préservés | not run |
| Cube, stash personnel et partagé | même comportement | not run |
| Manette, hôte et joiner | comportement cohérent | not run |

## Prochain gate

Vincent reteste maintenant le témoin `tsc` avec la 0.2.1 et valide le rythme
ralenti, le clic dans la piste, le drag complet du curseur, le bouton `Close`,
le suivi automatique, le reveal immédiat, la fermeture/réouverture et
l’intégrité de l’objet. Si le témoin devient vert, implanter le fallback
autonome pour tooltip, clic droit et rendu lorsque les trois hôtes sont absents,
puis couvrir les autres conteneurs et la manette.

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

La DLL 0.2.1 est uniquement un témoin local. Aucun ZIP public ni déclaration de
livraison ne sont autorisés avant validation en jeu, fallback autonome et
matrice de coexistence fermée.
