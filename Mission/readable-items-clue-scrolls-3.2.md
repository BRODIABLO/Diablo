# Readable Items / Clue Scrolls — D2R 3.2.92777

Dernière mise à jour : 27 juillet 2026

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
- la version 0.4.0 accepte un fichier WAV ou FLAC optionnel par texte, avec
  arrêt, interruption et relecture déterministes.

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
      "text": "A bounded dummy message used only to validate Readable Items.",
      "audioFile": "data/global/sfx/item/readable_items_test.flac"
    }
  ]
}
```

Contraintes initiales : code ASCII visible de 1 à 4 octets, codes uniques,
maximum 256 objets, titre non vide de 128 octets maximum et texte non vide de
8 192 octets maximum. Une clé inconnue ou une configuration invalide est
refusée. `audioFile` est optionnel; lorsqu'il est présent, il doit être un chemin
relatif sûr de 260 octets maximum vers un `.wav` ou un `.flac`. Les chemins absolus, les
composants `..`, les extensions étrangères et les caractères Windows interdits
sont refusés avant le chargement du plugin.

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

## Témoin mod-local 0.3.0 prêt à tester

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

Vincent a ensuite signalé que les clics sur `Close` et la scrollbar atteignaient
encore Diablo comme commandes de déplacement. La version 0.2.2 ferme ce défaut
sans ajouter de hook D2R ni de second hook Windows : le hook souris bas niveau
déjà possédé par Extended Item Stats délègue chaque événement à l'export
`ReadableItemsHandleMouseInput`. Readable Items publie ses zones interactives en
coordonnées écran, traite lui-même Close, flèches, piste, drag et fast-forward,
puis consomme le bouton bas et le relâchement sur tout le panneau. Les mouvements
de souris restent transmis pendant le drag afin de ne pas figer le curseur.

Preuves techniques du 26 juillet 2026 :

- Readable Items 0.2.2 : 2/2 tests Release, dont le padding ItemsTxt exact,
  la révélation progressive, le suivi/repli manuel et le déplacement absolu
  borné employé par le drag, avec conversion piste-vers-offset testée aux deux
  bornes, au milieu et sur une piste dégénérée;
- Transmogrify : 1/1 test Release après ajout de la délégation;
- ExtendedItemStats 0.3.16 : 2/2 tests Release après ajout de la délégation
  d'entrée souris;
- manifeste v2 et sept exports présents dans `ReadableItems.dll`;
- hashes source/runtime identiques pour les quatre fichiers déployés;
- cold start 0.2.2 : 20/20 patchsets, 24 plugins actifs, zéro rejet, zéro échec et
  séquence D2R complète jusqu'à 24/24;
- aucune erreur fraîche dans Readable Items, Transmogrify, ExtendedItemStats ou
  FloatingDamage.

SHA-256 :

- `ReadableItems.dll` 0.2.2 : `C96BE644BC472952A8DDF89D28E25709EFEE47CEF892A0E479AF58BEAE1E55A7`;
- `ReadableItems.json` : `355D2BAAD8E8F83E357CA0A47CA255DD2FA3FB53CB70EC716743A373B7992FED`;
- `Transmogrify.dll` hôte : `EB1A0052D71EB4D640FC179636B09B217DAD52CDD2135965A5BD47E3BC468026`;
- `ExtendedItemStats.dll` 0.3.16 hôte : `CC3506166A6D57971815A2CAFF81E1C448F0E1246E96177FABC67FE14B0D700C`.

Vincent confirme le 27 juillet 2026 que le témoin 0.2.2 fonctionne très bien.
Cette observation ferme le retest nominal du panneau, de son rythme, de la
scrollbar, du bouton `Close` et de la capture souris demandée avant la phase
audio; les scénarios étendus de conteneurs, manette et multijoueur restent
distincts.

### Audio autonome 0.3.0

Vincent demande ensuite un fichier audio propre au parchemin, sans faire parler
Charsi ni dépendre d'un PNJ. La version 0.3.0 lit donc directement un WAV depuis
`items[].audioFile`, relativement au dossier du `ReadableItems.json` réellement
sélectionné. Elle ne modifie pas `sounds.txt`, n'appelle aucune primitive sonore
92777 et n'ajoute aucun hook : un moteur XAudio2 privé reçoit le PCM du fichier.

Le contrat initial accepte uniquement un RIFF/WAVE PCM non compressé, 16 bits,
mono ou stéréo, de 8 à 192 kHz, avec un plafond de 64 MiB et des champs RIFF,
débit, alignement et données cohérents. Le son démarre avec l'ouverture du
lecteur. Il s'arrête sur `Close`, Escape, la commande console `close`,
l'ouverture d'un autre objet ou le déchargement du plugin; une réouverture
repart du début. Un fichier absent ou illisible produit un warning et laisse le
texte entièrement utilisable.

Le témoin demandé est
`BKVince.mpq/data/global/sfx/item/readable_items_test.wav`. Il prononce clairement
« Readable Items audio test. If you hear this message, custom scroll audio is
working. » et ses propriétés sont vérifiées automatiquement : PCM 16 bits,
deux canaux et 48 000 Hz.

Preuves techniques du 27 juillet 2026 :

- build Release x64 réussi, manifeste v2 et sept exports attendus présents;
- 2/2 tests CTest passent : contrat JSON, chemins audio sûrs, cycles du lecteur
  et inspection réelle du WAV livré en stéréo 48 kHz;
- cadastre régénéré avec le nouveau header et le WAV, puis validé `VALID`;
- DLL, JSON et WAV synchronisés seuls dans le profil BKVince avec SHA-256
  source/runtime identiques;
- cold start : Readable Items 0.3.0 accepté, `20/20` patchsets,
  `scanned=26 active=24 disabled=2 rejected=0 failed=0` et démarrage `24/24`;
- aucun échec frais Readable Items au chargement. L'audition et les arrêts en
  jeu restent à confirmer par Vincent.

SHA-256 0.3.0 :

- `ReadableItems.dll` : `CBFA4A6883BCEF7DBE7C27F1480D53A552977AB7021A032ECF341E9359528FF2`;
- `ReadableItems.json` : `DE609CEA30C9DF8A448DE358C81EE560CB309A00BCEBE3E0EFCEC884335E6D63`;
- `readable_items_test.wav` : `4218F785E4B9E78405B3F4B3C048DCEA5E6C4941227749262C97666DA5D15A7D`.

Vincent confirme ensuite le 27 juillet 2026 que le témoin WAV est clairement
audible en jeu. Cette observation ferme l'audition nominale 0.3.0 et motive le
passage demandé à un format sans perte plus compact.

### Décodage FLAC autonome 0.4.0

La version 0.4.0 conserve le chemin WAV 16 bits et ajoute les fichiers FLAC
natifs dans `items[].audioFile`. Le décodeur `dr_flac` 0.13.4 de David Reid est
lié statiquement sous MIT-0 depuis le commit épinglé
`34a89ffe6bfc4d78db6888fef76cd408dba18185`; aucune DLL additionnelle n'est
requise et aucune primitive audio ou table `sounds.txt` de D2R n'est modifiée.

Le plugin lit le FLAC depuis la mémoire, exige un ou deux canaux et une fréquence
de 8 à 192 kHz, puis remet à XAudio2 un PCM signé 32 bits afin de conserver une
source FLAC allant jusqu'à 32 bits. Le fichier compressé reste borné à 64 MiB et
le PCM décodé à 128 MiB afin de refuser les fichiers dont l'expansion serait
excessive. Les chemins absolus, traversées, extensions inconnues, FLAC tronqués,
canaux ou fréquences hors contrat sont refusés sans empêcher l'ouverture du
texte.

Le témoin 0.4.0 est
`BKVince.mpq/data/global/sfx/item/readable_items_test.flac`. Il est la conversion
lossless du WAV déjà entendu par Vincent : stéréo, 48 000 Hz, source 16 bits,
7,5 secondes et 159 046 octets au lieu de 1 439 950 octets. La configuration
mod-locale pointe maintenant exclusivement vers ce FLAC.

Preuves techniques du 27 juillet 2026 :

- workbench D2R 3.2.92777 vérifié; aucun hook, RVA ou ABI n'a changé;
- référence PluginPack `dc75b49ffbb67b887d7757ee00ee9a03bcde5d8a`
  vérifiée et aucun framework audio concurrent trouvé;
- build Release x64 réussi, manifeste v2 et sept exports attendus présents;
- 2/2 tests CTest passent, dont acceptation `.flac`, rejet d'un FLAC tronqué et
  décodage réel du témoin livré en stéréo 48 kHz avec échantillons non vides;
- DLL, JSON et FLAC synchronisés seuls dans le profil BKVince avec hashes
  source/runtime identiques;
- cold start 0.4.0 : `20/20` patchsets,
  `scanned=26 active=24 disabled=2 rejected=0 failed=0` et démarrage `24/24`;
- le log frais sélectionne le JSON mod-local et annonce
  `optional WAV/FLAC audio enabled`; un clic droit a ensuite produit
  `audio started` sur `readable_items_test.flac` sans erreur. L'audition FLAC
  reste à confirmer explicitement par Vincent.

SHA-256 0.4.0 :

- `ReadableItems.dll` : `AEAEFE4B78BE598C354037B03F0C49520B7D77B633F83E6483E4331EDE075A44`;
- `ReadableItems.json` : `4E37AA9FCF44D95260D8A5A3F05E8CA482EE36FCF0743C9517CD88B3AF73054F`;
- `readable_items_test.flac` : `EF9BEDB1141A21FCA891889E0A06F96DEC8AC006DAE49F06CE1F58B13278F15B`.

### Activation `pSpell` 0.5.0

Vincent confirme le 27 juillet 2026 que les acquis du lecteur et du FLAC sont
conservés, mais corrige le contrat demandé : l'activation doit être configurée
comme un `pSpell`, et non seulement par la présence d'un code dans le JSON.

La version 0.5.0 réserve donc la sentinelle privée `pSpell=-2`. Le record compilé
est lu à l'offset prouvé `ItemsTxt+0x94`; le code reste lu à `+0x80`. Readable
Items ne prend la commande que lorsque cette sentinelle est présente. Le JSON
fournit ensuite le titre, le texte et l'audio optionnel, mais ne peut plus
transformer à lui seul un objet vanilla en objet lisible. Une sentinelle sans
entrée JSON est consommée sûrement et journalisée au lieu d'être transmise au
dispatcher vanilla. Tous les autres `pSpell` sont délégués inchangés.

Ce contrat reste un `pSpell` virtuel possédé par la DLL autonome : il n'étend
pas la table native D2R avec un handler 16 non prouvé. `Transmogrify 1.3.1`
conserve l'unique hook autoritaire `0x4F40C0` et appelle l'export
`ReadableItemsHandleUseItem` avant le handler original; aucun hook D2R
supplémentaire n'est installé.

Le Town Portal Scroll `tsc` n'est plus une fixture Readable Items. Il conserve
son `pSpell=2` vanilla et doit créer un portail normalement. Un témoin dédié
`rds` (`Clue Scroll Test`) est ajouté en dernière ligne de `misc.txt`, sans
déplacer les 277 Class IDs existants. Il copie l'art du parchemin, porte
`useable=1`, `pSpell=-2`, `spawnable=0`, `PermStoreItem=1` et est disponible
chez les vendeurs qui proposaient le témoin `tsc`. La chaîne localisée `rds`
utilise l'ID 74077. Le script idempotent
`scripts/migrate-bkvince/add-readable-items-test-witness.js` applique et vérifie
la ligne avec `scripts/build-data/tsv.js`, CRLF et round-trip byte-exact.

Preuves techniques du 27 juillet 2026 :

- workbench 92777 et référence PluginPack épinglée vérifiés;
- mutation `misc.txt` limitée à une ligne terminale et `item-names.json` à une
  entrée; vérification idempotente `VALID (check)`;
- build Release x64 réussi et 2/2 tests CTest verts, incluant la sentinelle
  exacte et le témoin JSON `rds` avec décodage du FLAC réel;
- `npm run verify:data` entièrement vert; le validateur Storage Bag exige
  toujours son bloc gouverné contigu et byte-identique, mais accepte désormais
  une extension étrangère après ce bloc afin de préserver tous les Class IDs;
- DLL, JSON, `misc.txt`, `item-names.json` et FLAC synchronisés par allowlist
  dans le profil BKVince avec hashes source/runtime identiques;
- cold start 0.5.0 : plugin accepté, `20/20` patchsets,
  `scanned=26 active=24 disabled=2 rejected=0 failed=0` et démarrage `24/24`;
- le chargement des tables gameplay et des chaînes atteint 23/24 sans erreur.

SHA-256 0.5.0 :

- `ReadableItems.dll` : `C5494C5B0D2EC764B7804CC95209E970FBA97A31DE092FB0D3313803CBA7EAB1`;
- `ReadableItems.json` : `E5A5A1C22CF1C83A08E7A806494B3B242A48B8A10A6927CC659AC6D4F4AB674A`;
- `misc.txt` : `68D2160859916F9AB579CB66B0FDBA2E98CCD0C3B557A3EA21F2F53AF72D8DE2`;
- `item-names.json` : `965DB141502B983063949144C30DB508F26D4A1F431BDFE3F2F662555F63F8ED`;
- `readable_items_test.flac` : `EF9BEDB1141A21FCA891889E0A06F96DEC8AC006DAE49F06CE1F58B13278F15B`.

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
| Tooltip du témoin `rds` | `Right-click to read...` | cold start 0.5.0 pass; gameplay not run |
| Clic droit `rds`, `pSpell=-2` | lecteur ouvert, objet intact | cold start 0.5.0 pass; gameplay not run |
| Clic droit `tsc`, `pSpell=2` | comportement vanilla, création d'un portail | not run |
| Présentation PNJ | panneau supérieur, cadre doré, police Formal, sans UI technique | pass visuel en 0.2.0 |
| Rythme de reveal 0.2.2 | environ 18 caractères/s, proche de Charsi | pass confirmé par Vincent |
| Piste et drag de scrollbar 0.2.2 | clic et déplacement proportionnel bornés | pass confirmé par Vincent |
| Flèches, Up/Down et Page Up/Page Down | défilement borné | not run |
| Clic/Space/Enter | révèle immédiatement tout le texte | not run |
| Bouton `Close` | ferme immédiatement le dialogue | pass confirmé par Vincent en 0.2.2 |
| Capture souris du panneau | Close, texte, flèches, piste et thumb ne déplacent jamais le personnage | pass confirmé par Vincent en 0.2.2 |
| Audio WAV 0.3.0 stéréo 48 kHz | message anglais clairement audible à l'ouverture | pass confirmé par Vincent |
| Audio FLAC 0.4.0 stéréo 48 kHz | même message lossless clairement audible à l'ouverture | cold start pass; audition not run |
| Interruption audio | Close et Escape coupent immédiatement le message | not run |
| Relecture audio | chaque réouverture repart du début | not run |
| Audio absent/invalide | texte lisible et warning sans crash | politique statique pass; runtime not run |
| Escape puis réouverture | fermeture puis reprise progressive ligne 1 | not run |
| Quantité et position | strictement inchangées | not run |
| Transmogrify et ExtendedItemStats | comportements existants préservés | not run |
| Cube, stash personnel et partagé | même comportement | not run |
| Manette, hôte et joiner | comportement cohérent | not run |

## Prochain gate

Vincent teste maintenant le témoin dédié `rds` avec la 0.5.0 : acheter `Clue
Scroll Test` chez Akara, confirmer le tooltip et l'ouverture par `pSpell=-2`,
entendre le FLAC, fermer pendant sa lecture avec `Close`, puis rouvrir pour
confirmer le redémarrage au début. Un vrai `tsc` doit ensuite créer un portail
normalement. Après ce gate `pSpell` et audio, implanter le fallback
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
- audio : absent, valide et invalide, interruption, changement d’objet et
  relecture; le contrat et le témoin valide sont implantés, la matrice runtime
  reste ouverte.

## Livraison interdite à ce stade

La DLL 0.5.0 est uniquement un témoin local. Aucun ZIP public ni déclaration de
livraison ne sont autorisés avant validation en jeu, fallback autonome et
matrice de coexistence fermée.
