# BKVince Hero Editor

Éditeur de sauvegarde D2R construit pour les données actuelles de BKVince. Le
projet reprend le workspace historique `apps/hero-editor/`, mais remplace son
prototype de forge d'objets par des tranches verticales d'édition complètes.

La première tranche permet de :

- ouvrir un D2S BKVince v105 ou créer un héros vierge parmi les classes que le
  codec sait réellement sérialiser ;
- placer automatiquement le pack de départ natif BKVince : huit charms dans la
  colonne gelée à droite de l'inventory (`mff`, `mfc`, puis six `mfd`), le
  Horadric Cube `box` en `(0,0)` et le Town Portal Scroll `tsc` en `(9,7)` ;
- modifier les écrans General et Stats ;
- déplacer les objets existants entre inventory, Cube et stash personnelle avec
  contrôles de limites et de chevauchement ;
- cliquer une case vide pour rechercher dans un catalogue unifié de 729 bases,
  215 Sets, 473 Uniques et 112 Runewords BKVince encodables, choisir une base
  compatible pour les recettes et placer de 1 à 20 copies atomiquement dans
  inventory, Cube ou stash personnelle ;
- afficher au hover ou au focus les propriétés réellement portées par le record
  et ouvrir l'éditeur d'un clic ;
- modifier une base compatible, la qualité Normal/Superior/Magic, l'identification,
  l'état éthéré et la quantité applicable dans une modale qui verrouille les
  conversions exigeant une reconstruction de record ;
- choisir les préfixes et suffixes Magic réellement spawnable pour le type et
  l'item level, puis compiler leurs déclarations `mod1..3` en propriétés
  numériques, paramétrées ou structurelles gouvernées; les fonctions 3, 10, 11,
  14, 19, 21 et 22 couvrent notamment skills élémentaires, skill tabs, procs,
  sockets, charges, bonus de classe Warlock et oskills ;
- ajouter manuellement une propriété gouvernée de `Properties.txt` à tout objet
  D2S complexe, sans forcer sa qualité, avec recherche et contrôles sémantiques
  adaptés (valeur, plage de dégâts, skill, classe, tab, proc, charges, durée,
  monstre ou sockets) ; les records compacts `simple_item` restent verrouillés car
  leur payload natif ne contient pas de liste d'attributs ;
- construire ou briser un runeword BKVince compatible depuis la modale : la
  recette remplit les sockets libres dans l'ordre exact des runes et compile les
  propriétés de la recette en mode minimum ou maximum ;
- éditer les 81 états de quête et les 117 waypoints des trois difficultés ;
- distribuer les ranks dans les 240 skills BKVince, positionnés depuis les vrais
  `SkillPage`, `SkillRow` et `SkillColumn`, avec prérequis, points disponibles et
  mode `Ignore Game Rules` ;
- consulter `Item Bonuses`, une synthèse live des propriétés réellement actives
  sur l'équipement joueur, groupées et formulées comme RuneWizard, puis ouvrir
  directement chaque objet source dans la modale partagée ;
- éditer un mercenaire existant — état vivant/mort, Name ID, type BKVince,
  expérience et équipement — parmi 33 définitions gouvernées de `Hireling.txt`,
  avec compétences et bonus live ;
- exporter un objet individuel canonique en `.d2i`, importer jusqu'à vingt
  objets atomiquement et sauvegarder/recharger des bundles nommés
  `.bkitems.json` liés par SHA-256 à l'ABI exacte des tables BKVince ;
- utiliser un workspace unifié inspiré de RuneWizard : Equipment/Inventory,
  Stash, Cube/Belt, puis panneau Stats à navigation verticale ;
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
npm.cmd run extract:vanilla-ui -w apps/hero-editor
```

`npm run generate` régénère
`src/data/bkvince-constants.generated.js` depuis les tables TXT et chaînes
gouvernées de BKVince. Le générateur impose un aller-retour TSV byte-exact avant
d'utiliser les tables et produit aussi le catalogue d'objets. Il refuse les
headers requis absents, les clés gouvernées dupliquées et les références de base
ambiguës. Les IDs de préfixes et suffixes suivent l'ABI réelle du parseur D2S :
index initial à 1, lignes vides comptées et séparateur `Expansion` ignoré.

La commande `extract:vanilla-ui` lit les sprites d'items et les sept atlas de
skills vanilla depuis l'installation D2R locale de Vincent, puis les place
uniquement sous `analysis-cache`. Elle accepte
les paramètres PowerShell `-GameDataPath` et `-CascLibPath` si l'installation ou
CascLib se trouvent ailleurs. `generate-ui-assets.mjs` donne toujours priorité
aux overlays BKVince, puis produit les PNG versionnés et leur manifeste. Le
catalogue courant couvre les 800 bases avec 388 visuels distincts et 814 codes;
aucun asset ni code de RuneWizard n'est utilisé.

## Gate runtime et limite courante

La création, la modification General/Stats et le pack natif de dix objets de
départ ont réussi leur matrice load, save, reparse et reload dans D2R 3.2.92777.
Le patch
versionné de `@d2runewizard/d2s` conserve les quatre mots realm et le bit de
présence de quantité du format v105.

Le déplacement Inventory vers Cube a réussi deux cycles D2R sans perte d'objet
ni de realm data. Le témoin unifié `HEItemForge.d2s` ferme aussi deux cycles
D2R avec la potion changée en `hp2`, les deux Hand Axes, le Tome quantité 50 et
les huit charms BK : ses douze payloads items décodés restent identiques.

La suite locale passe `34/34`. Elle couvre maintenant les reconstructions Magic,
Set et Unique,
le filtrage d'affixes, les valeurs encodables, l'aller-retour `.d2i`, l'import
atomique, le bundle fingerprinté, le rejet d'octets traînants et un oracle Magic
éthéré natif dont la durabilité 28/28 reste byte-exact. Elle couvre aussi
l'écriture/relecture d'une quête, de Prison of Ice avec `consumed_scroll`, d'un
waypoint et d'un rank Warlock, suivie d'un export no-op byte-exact. Le patch de
`@d2runewizard/d2s` aligne aussi l'écriture de `act_v.introduced` sur l'offset
déjà utilisé par son lecteur.

Le témoin `HEMagicIO.d2s` ferme aussi deux cycles D2R avec dix items : huit
charms BK, un `cbw` Magic éthéré importé et un Hand Axe Magic construit. Le
payload items décodé conserve le SHA-256
`9A4536B1762C2F4F8E325A2A1E92C26344F57CEEB8F34FE6C6F92A49B1AFBADB`
et les deux sorties restent no-op byte-exactes dans l'éditeur. D2R ayant révélé
qu'il canonise les propriétés par ID croissant, l'éditeur applique désormais le
même ordre avant écriture et la suite le couvre explicitement.

Le témoin étendu `HEMagicIO.d2s` contient maintenant treize items et couvre les
fonctions paramétrées 10, 11, 21 et 22. Deux cycles sur le vrai profil
`D2RLoader.exe -mod BKVince -txt` conservent le payload décodé SHA-256
`C5CFADA6E62EE1D7C1896A707346BF4090C4EAF915096BCF96E9C9D285B1EB42`;
les deux sorties demeurent no-op byte-exactes.

Le témoin combiné `HEWorldState.d2s` ferme deux cycles sur le profil exact
`D2RLoader.exe -mod BKVince -txt`. Den of Evil, Prison of Ice et son
`consumed_scroll`, trois waypoints inter-actes, trois rangs Warlock, onze points
inutilisés et les huit charms BK conservent le même état gouverné SHA-256
`D16D7B5DBE472D196B20FC1857FA6657AAEFD09AB2EFEDD5B5AAC421651E92A2`
après les deux sauvegardes D2R; les deux sorties restent no-op byte-exactes dans
l'éditeur.

Le témoin `HEAffixOracle.d2s` ferme les derniers affixes Magic restants : cinq
skills élémentaires, deux suffixes de charges Teleport et les préfixes Mechanic,
Artisan et Jeweler. Les tooltips D2R affichent notamment `+1 to Lightning Skills`,
`Level 6 Teleport (52/52 Charges)` et `Socketed (2)`. Les dix objets gouvernés
conservent le SHA-256 sémantique
`E396066CE17F32E482B9EA3D88864CFA9A2EB45A0AC7D331D6882A131C6B965C`
pendant deux cycles `Save and Exit`; les deux sorties restent no-op byte-exactes.
Le compilateur atteint ainsi **567/567 préfixes** et **607/607 suffixes**
spawnable.

Les reconstructions Set et Unique sont maintenant ouvertes. Le compilateur
couvre **215/215 objets Set** et **473/473 objets Unique** sélectionnables aux
valeurs minimales et maximales. Les scalaires hors plage sont saturés à leur
borne native représentable, les durées élémentaires à leur maximum natif, tandis
que le booléen implicite `noheal=1` et le socket explicitement accordé par Static
Accumulator restent gouvernés. Wraithstep (ID `413`) expose ses trois arbres Warlock
exclusifs et Opalvein (ID `416`) ses six bonus de dégâts exclusifs. Le choix est
créé en roll **Perfect**, remplace uniquement la propriété placeholder absente de
`Properties.txt` et est ré-identifié depuis les attributs D2S natifs à la
réouverture; les six variantes d'Opalvein ne sont jamais additionnées.

Le témoin runtime `HEUmbralMax.d2s` ferme aussi le scalaire natif d'Umbral Disk.
Au niveau 99, l'écran du personnage affiche exactement **862 Defense** : 68
issus de 275 Dexterity, 15 issus de la défense parfaite de l'objet, puis 779
issus de `floor(63 × 99 / 8)`. Après `Save and Exit`, le fichier passe du
SHA-256 `DF5769CC7E195FD69C7BF71C47E6DD66874CC689DFF88DE9340F0AB9CBD4BA69`
à `09B55A61E773395FDB7D30C8AA44A08D9CF31E2BFC91C79C6E88EBAD440D0F82`,
reparse encore un unique stat 214 à `63` et reste no-op byte-exact. La
saturation au maximum natif est donc persistante et effective en gameplay.

Le témoin `HENamedItems.d2s` couvre deux objets Set et Annihilus pendant deux
cycles D2R avec le même SHA-256 sémantique gouverné
`F21D6B5B4426211E972E22A2BBA5FA41BCA258A1FB91A4C70BD6E0C09EA5386C`.
Le témoin corrigé `HENamedEdges.d2s` couvre Witherstring, Visceratuant, Tomb
Reaver et Titan's Echo. Deux cycles `Save and Exit` sur le vrai profil BKVince
conservent ses quatre objets, les huit charms BK et le SHA-256 sémantique
`B23644BD8E406BAACDDEE9E0606C95BFAFEAEEF37A5DA884459785EE45684718`;
les deux sorties sont no-op byte-exactes dans l'éditeur. Cette preuve a aussi
fixé l'encodage canonique de la durée de poison et confirmé que D2R ramène
`Static Accumulator` à `Socketed (0)`, donc cet unique demeure verrouillé.

Le générateur compile maintenant les cinq listes partielles de bonus Set depuis
`setitems.txt`. Les propriétés `aprop` de `add func=0` restent permanentes;
`add func=1/2` produit les cinq listes gouvernées par les bits 0 à 4 du masque
Set. Les masques non contigus `00100` de l'objet Set 77 et `10101` de l'objet
Set 136 sont conservés exactement. La modale permet de choisir chaque liste,
d'éditer ses valeurs minimales ou maximales et de la retirer.

La même modale insère désormais runes, gems et jewels dans un objet socketed.
Le codec crée les fillers `sock` dans l'ordre canonique, vérifie capacité et
nombre total, attribue des identifiants uniques et permet d'extraire le dernier
filler comme nouvel objet racine. Les témoins `HESetSockets.d2s` et
`HESockExtract.d2s` ont chacun traversé deux cycles `Save and Exit` sur le profil
BKVince exact. Leurs SHA-256 sémantiques restent respectivement
`DCF0333ADAFEA108FA0E172AFB95264229D8EB9E3E658B394EF1D5BA645FEFE0` et
`78673BB692E845D8BA5F0B346356F311FC729E2B24EE811AD92296D9597752E5`;
les quatre sorties reparsées restent no-op byte-exactes dans l'éditeur.

L'import direct dans les sockets accepte maintenant un ou plusieurs `.d2i` ou
un bundle `.bkitems.json`. L'opération est atomique : un item qui n'est pas un
filler BKVince ou un groupe plus grand que la capacité libre fait rejeter toute
la sélection. Les qualités complexes, propriétés, nouveaux IDs et realm data
sont conservés. Chaque filler peut aussi être extrait indépendamment; les
fillers suivants sont compactés et renumérotés dans l'ordre canonique.

Le témoin `HESockCompact.d2s` importe un Jewel Magic `+7 Strength` entre `r01`
et `r02` dans un Gothic Shield à trois sockets, puis extrait le Jewel central.
Deux cycles `Save and Exit` sur
`D2RLoader.exe -mod BKVince -txt -offline` conservent le Jewel dans
l'Inventory, `r01/r02` aux positions socket 0/1, les huit charms BK et le
SHA-256 sémantique
`DD908F00FF6B8EDC316017D860FFBA675ACA7EFDC02C23D5051D4A6B4B94240D`.
Les deux sorties D2R de 1 355 octets restent no-op byte-exactes dans l'éditeur.

Le log frais du dernier oracle confirme `D2RLoader.exe -mod BKVince -txt
-offline`, le build 3.2.92777, 18/18 patchsets appliqués et 12 plugins actifs.
Il ne contient aucun nouvel ID d'assertion par rapport aux cinq IDs du baseline
déjà documenté; le bruit CASC de fermeture reste présent.

Le catalogue gouverné expose maintenant les **112 runewords BKVince actifs**.
Le compilateur ferme **112/112 recettes** aux valeurs minimales et maximales,
remplit les fillers sans écraser une socket occupée et refuse une base ou une
séquence de runes incompatible. `Chaos` utilise une Katar compatible. Sa ligne
`rep-dur=100` dépasse le champ natif six bits : l'éditeur écrit donc une seule
occurrence sûre du stat 252 à `63`, sans élargir l'ABI d'`ItemStatCost.txt` ni
modifier `Runes.txt`. D2R réapplique ensuite la recette gouvernée en jeu, comme le
confirment les cycles de réparation mesurés. Les
runewords natifs Spirit, Exile et Dream sont reconnus dans `QtyTester.d2s`, avec
leurs noms et propriétés dans les tooltips.

Le témoin final `HERuneword.d2s` contient les huit charms BK et un Spirit Monarch
construit par l'éditeur. D2RLoader 3.2.92777 le charge, l'affiche dans la stash,
le sauvegarde et le recharge. Après la passe isolée, le fichier fait 1 388 octets,
porte le SHA-256
`E36923DFE1D93AB9ECB59C9174C7EC62EBB8D87D86A243616BFFC32510695832`,
conserve le runeword 155, `r07/r10/r09/r11` et ses sept groupes de propriétés,
puis reste no-op byte-exact dans l'éditeur. Le log isolé porte
`374484DF7AEF9E9F2CEBA4157A917C5724B6D8B7CFE41A59C326D193F00BCBEC` :
18/18 patchsets, 12 plugins actifs, 24/24 étapes startup, aucune assertion
`Items.cpp`; seul le doublon BKVince `Act5-Rifts`, extérieur au héros, demeure.

`Item Bonuses` et `Mercenary` sont maintenant des surfaces fonctionnelles. Le
résumé de bonus suit les placements et éditions en cours, regroupe les
propriétés Magic/runeword/socket et ouvre l'objet source d'un clic. Le
mercenaire expose les 33 types actuels de `Hireling.txt`, son en-tête D2S, ses
compétences gouvernées, son équipement étendu et la même modale d'objet que le
joueur. Un mercenaire absent peut maintenant être créé depuis les valeurs
gouvernées de la table, puis retiré avec restauration complète par Undo.

Le témoin `HEMercenary.d2s` a été produit depuis `QtyTester`, chargé avec
`D2RLoader.exe -mod BKVince -txt -offline`, puis sauvegardé et reparsé. Anor est
vivante au niveau 98; l'écran natif affiche Lionheart au torse et un
`Triumphant Ring of Nova Shield` dans le slot étendu 6. La sortie finale de
3 166 octets porte le SHA-256
`BC538674C25FDA25D3870C06C66EB90CF8B009A0D4D872A9B477B29D6A9C8723`
et reste no-op byte-exacte. Le cold start confirme le build 3.2.92777, 24/24,
18/18 patchsets et 12 plugins actifs sans rejet ni échec. L'ouverture de la
fiche mercenaire révèle deux assertions non bloquantes du profil BKVince : le
skill 443 n'a pas de `SkillDesc`; elles n'empêchent ni le chargement ni le
`Save and Exit` du témoin.

Le catalogue relie maintenant chacune des 800 bases à ses `BodyLoc1/BodyLoc2`
gouvernés par `ItemTypes.txt`. Les douze slots mercenaires acceptent un ajout ou
un import individuel par clic, le déplacement par drag-and-drop et le retrait
avec Undo. Le validateur refuse avant export les doublons de slot, une base
incompatible, un record hors bloc `jf` et un import multiple vers un seul slot.

Le témoin `HEMercAdd.d2s`, dérivé du précédent sans l'écraser, ajoute un `cap`
au slot tête 1 en conservant `scl` au torse 3 et `rin` au ring droit 6. Il a été
chargé avec `D2RLoader.exe -mod BKVince -txt -offline`; le log confirme
`mod="BKVince"`, le build 3.2.92777, 24/24, 18/18 patchsets et 12 plugins actifs.
Le menu conserve le branding historique BKDiablo du squelette, tandis que les
difficultés Pain/Agony/Insanity et le montage du profil prouvent BKVince. Le
panneau natif affiche le Cap, Lionheart et le ring; après `Save and Exit`, la
sortie de 3 202 octets porte
`C2834549FC805FCE1C51AE4928A7018F120D21EAA90F56F70ACF881C32D97536`,
reparse les trois slots et reste no-op byte-exacte. Les 21 sauvegardes originales
ont été restaurées; le témoin et ses fichiers auxiliaires sont conservés sous
`analysis-cache/hero-editor-runtime/postgame/`.

Le témoin `HEMercCreate.d2s` ferme la création d'un mercenaire absent. Le
générateur lit `Exp/Lvl`; le défaut type 0 devient une Rogue Acte I niveau 3,
Name ID 0 et 3 600 XP, avec un ID natif non nul dérivé du snapshot. BKVince
affiche Aliza vivante, `Level 3 Rogue` et `3,600 of 8,000`, puis `Save and Exit`
conserve exactement le header. La sortie de 1 229 octets porte le SHA-256
`657F7339184E2157AFD8B9AC5313E7056CAAED506C7318FAAC5BBFE16CC1F779`
et reste no-op byte-exacte. Le log frais
`30F02F855D43177AEF230E29CC2A048482B4110F3CD73F040240B9B9A549B2B8`
confirme `mod="BKVince"`, 3.2.92777, 18/18 patchsets, 12 plugins actifs et
24/24. Les 21 sauvegardes originales ont été restaurées.

`Chronicle` est maintenant une surface fonctionnelle adossée au vrai
`ModernSharedStashSoftCoreV2.d2i`. Le lecteur parcourt un nombre variable de
secteurs au lieu de supposer les six pages historiques de RuneWizard : le test
de compatibilité couvre explicitement les 1 001 pages du profil BKVince. Il
identifie l'enveloppe Chronicle finale, conserve toutes les pages de stockage
octet pour octet, préserve les IDs inconnus et refuse signature, version,
taille, secteur ou compteurs ambigus avant toute écriture.

L'interface expose les catalogues gouvernés Set, Unique et Runeword, la
recherche, l'ajout individuel ou par catégorie, le Monster ID, l'horodatage,
le retrait et l'historique Undo/Redo. Un export inchangé restitue le fichier
source byte-exact; un export modifié reconstruit uniquement le secteur Chronicle,
reparse le fichier complet et compare le snapshot demandé. Le témoin versionné
de 680 octets ouvre six pages et trois uniques; le parcours navigateur charge
ces trois découvertes, ajoute puis annule `The Gnasher`, modifie puis annule le
Monster ID de `Gravepalm`, sans erreur console. La suite passe **39/39** et le
build Vite est vert.

L'état sans fichier reprend maintenant exactement la hiérarchie RuneWizard :
le contenu commence par `Chronicle`, suivi du seul bouton `Load Shared Stash`.
L'ancien emblème, la grande carte vide et le texte technique sur le chemin D2I
ont été retirés; les informations BKVince détaillées n'apparaissent qu'après le
chargement réel du fichier.

`Demon` est maintenant une surface RuneWizard fonctionnelle pour les sauvegardes
qui contiennent déjà un bloc natif `lf`. Le générateur lit byte-exactement les
tables BKVince `monstats.txt`, `superuniques.txt` et `monumod.txt`, ainsi que la
localisation anglaise gouvernée, et expose **799 monstres**, **70 Super Uniques**
et **45 modificateurs**. Terrorized, Super Unique, l'identité du monstre et les
six premiers slots de mods sont éditables; les trois slots restants, les
difficultés, la zone, le niveau, les stats terminales et sept blocs opaques sont
préservés puis comparés après reparse.

Comme RuneWizard, l'éditeur ne crée pas artificiellement un démon lié lorsqu'il
n'en existe pas. Les sauvegardes BKVince locales auditées ne contiennent encore
aucun bloc réel : le test write/reparse utilise donc un payload synthétique
strictement borné et ne vaut pas preuve gameplay. Le navigateur couvre l'état
vide, puis Rakanishu Terrorized/Super Unique, six mods et Undo/Redo. La suite
passe **42/42** et le build Vite est vert.

La première verticale objets de la Shared Stash est maintenant fonctionnelle.
Le lecteur hydrate chaque secteur natif indépendamment, affiche les pages
ordinaires dans leur grille BKVince 16×13 et réutilise le même parcours
click-to-add/import, tooltip, Item Editor, sockets, runewords, propriétés,
suppression et Undo/Redo que les objets du personnage. L'export reconstruit
uniquement les secteurs modifiés, conserve les autres pages octet pour octet,
recombine Chronicle et inventaire puis reparse le fichier entier. Un test dédié
ferme aussi une édition Chronicle et une édition de page dans le même export.

La page stackable expose uniquement son compteur natif 8 bits, de 0 à 255. Les
records, propriétés, imports, suppressions et coordonnées superposées restent
verrouillés. Le témoin runtime porte une pile d'Ohm à `21` : BKVince la charge,
réécrit la Shared Stash lors de `Save and Exit`, puis l'éditeur reparse encore
`21`. Le stash original a ensuite été restauré avec ses 68 729 octets et son
SHA-256 `EF3AAE6E727924AE8071466C23C470579A9EFA3E27F1F21094D7D68CEE4D77BF`.

La Virtual Stash est maintenant une surface RuneWizard fonctionnelle, mais elle
reste volontairement distincte des formats physiques. La
[documentation publique de RuneWizard](https://d2runewizard.com/forum/Hero%20Editor/675dd8daacadd8cc6ac510af)
la décrit comme un stockage d'items additionnels qui ne sont pas
présents dans la stash physique; aucun secteur D2S/D2I imaginaire n'est donc
fabriqué. L'onglet ouvre une grille temporaire 16×13 dans la session du navigateur
et réutilise click-to-add, import groupé, tooltip, Item Editor complet,
déplacement, suppression et Undo/Redo. Son export `.bkitems.json` fingerprinté
reprend uniquement les items encore actifs, leurs modifications et leur position
courante. Recharger la page efface cet espace si le bundle n'a pas été exporté.

Le test dédié ajoute deux rings, modifie le premier, déplace le second, retire le
premier et reparse le bundle du record restant. La suite passe **47/47** et le
build Vite est vert. Le D2S du héros et le Shared Stash ne sont jamais modifiés
par ces opérations.

Les transferts directs entre Personal Stash, la page Shared Stash ordinaire
chargée, Virtual Stash et Trash sont maintenant atomiques. La destination est
validée et construite avant que la source soit retirée; un placement impossible
laisse donc les deux workspaces inchangés. Le record transféré reçoit un nouvel
ID sans collision et conserve qualité, affixes, propriétés Magic, sockets et
autres champs portables. Un Undo ou Redo lancé depuis l'un des deux workspaces
inverse les deux historiques ensemble.

Trash est un workspace récupérable de session, lui aussi sur une grille BKVince
16×13. Le bouton rouge de l'Item Editor y déplace l'objet, un objet supprimé peut
être restauré vers un stockage physique ou virtuel, `Empty Trash` se fait en une
seule action annulable et un bundle de récupération fingerprinté peut être
exporté. Le navigateur a couvert Personal → Virtual, Undo/Redo complet,
Virtual → Trash, Trash → Personal et Empty Trash → Undo. Le même parcours confirme
visuellement `mff`, `mfc` et six `mfd` aux huit lignes de la colonne 11 gelée.
La suite passe **48/48** et le build Vite est vert.

La preuve runtime d'une page ordinaire de Shared Stash est maintenant fermée.
L'éditeur a ajouté un Hand Axe item level 41 en `(0,0)` sur la page 1 d'une copie
du vrai fichier à 1 001 pages. D2RLoader lancé avec
`-mod BKVince -txt -offline` a affiché l'objet dans l'onglet Shared,
`PAGE 1 / 1000`, puis `Save and Exit` a conservé exactement le SHA-256
`482EAECDA5B60BC77ED87ABB42E059CCE1DCA1192ADA3EA959FD580331500208` :
les 1 001 secteurs sont restés identiques et le second export est byte-exact.
Le log frais confirme BKVince 3.2.92777, 18/18 patchsets, 12 plugins actifs,
zéro rejet, zéro échec et 24/24 étapes startup. Le stash actif a ensuite été
restauré byte pour byte à son SHA-256 original
`4461974E7FF5FE5A5DBF85E8B87AD24E56EED8BDECFE04BC1B630E8599E78318`,
et tous les fichiers du héros de test ont été retirés du profil actif après
archivage local.

Le prochain gate ferme la matrice de parité RuneWizard ligne par ligne, à partir
du rendu réel du shell, des grilles et des modales. Une première passe est déjà
fermée : shell utile de 760 px, stockage de 720 px en colonnes 264/280/90,
cellules adaptées séparément à Inventory, Stash, Cube et Belt, parcours unifié
`Load / create`, catalogue d'ajout compact de 600 px et Item Editor de 700 px en
deux colonnes avec aperçu vivant. Le navigateur ajoute deux Hand Axes, passe un
item Normal à Magic puis annule, et vérifie une mutation suivie d'Undo sur les
84 switches Quests, les 117 waypoints et les trois arbres de dix compétences du
Warlock. Les huit charms BK restent dans la colonne gelée droite. La suite reste
48/48 et le build Vite est vert.

La passe suivante recale aussi le point d'entrée lui-même. `Load / create` ouvre
désormais la modale RuneWizard de 800 px avec le titre `Load or Create a
Character`, le grand bouton pointillé `Load Character from Saved Games Folder`
et une grille 4×2 de cartes portrait pour les huit classes BKVince. Cliquer une
classe crée immédiatement le héros vierge sous son nom de classe; Name,
Hardcore et Ladder restent modifiables dans General. La section de builds
prédéfinis de RuneWizard est volontairement absente. Le navigateur crée les huit
classes et retrouve chaque fois exactement les huit charms BK de départ.

La passe papier-doll et forge groupée est maintenant fermée. Le générateur local
décode le panneau BKVince 1112×714 et 143 PNG distincts couvrant 149 codes
d'items; les huit charms BK utilisent leur vrai art, tandis que les bases sans
sprite versionné gardent un fallback explicite. La modale d'ajout offre les six
raccourcis RuneWizard — Worldstone Shards, Uber Ancients Materials, Warlord's
Glory Set, Cube, Organ Set et Key Set — avec quantités, retrait et ajout atomique.
Warlord's Glory produit les cinq vrais records Set 127–131 et compile leurs
propriétés gouvernées.

L'Item Editor affiche aussi `Make duplicates`, Ethereal, Identified et
Personalized dans sa rangée haute. Une personnalisation stocke 1–15 caractères
ASCII dans le payload natif; les copies reçoivent de nouveaux IDs et conservent
qualité, propriétés, sockets et nom. Le parcours navigateur `GroupUI` ajoute le
set complet, personnalise Warlord's Conquest avec `RuffnecKk`, crée deux copies
et confirme trois objets éditables. La suite passe **51/51** et le build Vite est
vert.

Les vrais tiers de base BKVince sont maintenant intégrés à l'Item Editor. Le
catalogue généré lit `normcode`, `ubercode`, `ultracode`, `minac` et `maxac`, puis
refuse toute référence absente ou famille D2S incohérente. `Downgrade` et
`Upgrade` suivent Normal → Exceptional → Elite en conservant qualité, identité
Set/Unique, propriétés, sockets, runeword et personnalisation. Item Level,
Defense et les deux valeurs de durabilité sont aussi éditables dans leurs plages
natives.

Le test write/reparse monte Warlord's Conquest de `hgl` à `xhg`, puis `uhg`, et
redescend jusqu'à `hgl` sans perdre le Set 127, ses propriétés, `RuffnecKk`, le
niveau 99 ou la durabilité 24/24. Le navigateur confirme 12 → 43 → 62 de défense,
puis 53 au retour sur War Gauntlets, avec l'aperçu Set intact. La suite passe
**52/52** et le build Vite est vert.

La couverture visuelle des items est maintenant complète : les 171 sprites
BKVince sont prioritaires, 330 sources vanilla locales complètent les trous et
388 PNG couvrent les 800 bases du catalogue. Les tooltips choisissent leur côté
selon la case occupée, tandis que l'Item Editor et ses champs s'empilent aux
seuils mobiles. La prochaine passe ferme les derniers détails responsive et
visuels. La page stackable reste verrouillée à son seul compteur 8 bits prouvé;
aucune autre mutation de son format n'est autorisée.

Les arbres de compétences utilisent désormais les vrais atlas `SpA1` des huit
classes. `SkillDesc.txt` gouverne la cellule paire inactive de chacun des 240
skills et la cellule impaire active; le générateur refuse toute cellule absente,
impaire ou hors de l'atlas de 60 frames. Les sept atlas vanilla sont extraits du
CASC local et le Warlock utilise son atlas BKVince. Les flèches sont construites
depuis les vrais `reqskill1..3`, y compris les liaisons latérales, et deviennent
dorées lorsque les deux rangs reliés sont investis. Le parcours navigateur a
confirmé Demon Skills, dix icônes, dix liaisons, puis Demonic Mastery et Summon
Goatman rang 1 avec leur état actif. La suite passe **53/53** et le build Vite
est vert.

L'écran Quests reprend maintenant la densité visuelle de la référence avec un
motif BKVince pour chacune des 27 quêtes, répété sur Normal, Nightmare et Hell.
Les motifs sont des sprites d'items déjà gouvernés et versionnés; aucun asset de
RuneWizard n'est copié. Leur état sombre/doré suit le switch de complétion. Le
bouton `Unlock Hell` complète exactement les 27 quêtes Normal et les 27 quêtes
Nightmare, sans changer Hell. La navigation Stats possède aussi huit pictogrammes
SVG RuffnecKk distincts. Le navigateur confirme 81 icônes chargées sans erreur,
54 switches après `Unlock Hell`, puis les 117 waypoints inchangés. La suite passe
**55/55** et le build Vite est vert.

Les fiches d’items reprennent maintenant les lignes RuneWizard depuis une copie
enrichie du record, sans muter le payload D2S : libellés Diablo localisés,
dégâts de base, exigences de force/dextérité/niveau, item level, propriétés,
capacité et contenu des sockets. Le tooltip de grille et l’aperçu de l’Item
Editor partagent exactement le même rendu; les fillers socketés possèdent leurs
miniatures et un tooltip trop haut devient défilable dans le viewport. Le
parcours navigateur confirme Warlord’s Conquest et Monarch, puis `Cancel
selection` rend immédiatement les cellules vides à `Click to add`. Spirit garde
ses quatre fillers dans le test dédié. La suite passe **56/56** et le build Vite
est vert.

Les trois fenêtres `Load / create`, `Choose items to add` et `Item Editor`
partagent maintenant la même gestion accessible : `Escape` ferme la fenêtre,
Tab reste dans le dialogue, le fond devient `inert`, le scroll de la page est
verrouillé et le focus revient exactement au bouton ou à la case qui avait
ouvert la fenêtre. Hors dialogue, `Escape` annule un déplacement d’item;
`Ctrl+Z`, `Ctrl+Shift+Z` et `Ctrl+Y` pilotent l’historique actif. Les cases déjà
occupées ne publient plus de faux boutons `Add item` sous le sprite : l’objet
visible est leur seule cible interactive. Les modales utilisent `100dvh`,
deviennent plein écran sous 520 px et masquent les tooltips hover persistants
sur écran tactile. Le parcours navigateur confirme fermeture, isolation,
restauration de focus, annulation, Undo/Redo et l’absence des fausses actions
sous les six Blank Charms gelés. La suite reste **56/56** et le build Vite est
vert.

La passe fine suivante rapproche encore l’Item Editor des captures RuneWizard.
`Quality` précède maintenant `Base item`; les champs Quantity et Runeword ne
sont rendus que lorsqu’ils s’appliquent. Chaque propriété apparaît sous sa
forme Diablo (`+15 to Strength`) dans une ligne compacte munie d’un crayon et
d’une corbeille; le crayon révèle ses valeurs gouvernées et `Add magic
attributes…` ouvre à la demande le choix rapide ou le compilateur
Properties.txt. Les bonus Set actifs affichent eux aussi leur libellé humain en
vert dans la liste et l’aperçu vivant.

Les sockets utilisent un sélecteur borné par la base et les fillers occupés.
Chaque filler possède son vrai sprite circulaire, `Extract` et une corbeille.
La nouvelle suppression enlève seulement le sous-record demandé, compacte
l’ordre `position_x`, brise une identité Runeword devenue invalide et reste
récupérable par Undo; elle ne fabrique aucun item racine. Le navigateur modifie
Warlord’s Conquest à +52 Strength, ajoute +20 Dexterity et son bonus Set +20%
Increased Attack Speed, puis insère/supprime/restaure une El Rune dans une Phase
Blade à trois sockets. Les tests verrouillent les libellés de listes Set et la
séquence insert/delete/extract; la suite reste **56/56** et le build Vite est
vert.

La composition supérieure de l’Item Editor suit maintenant plus précisément la
densité des captures RuneWizard. La modale desktop passe de 700 à 640 px; le
compteur de copies occupe la première ligne, les états Ethereal/Identified/
Personalized la seconde, puis Downgrade/Upgrade précède directement Quality et
Base item. L’aperçu reste à droite et le footer Delete/Download/Save demeure
fixe. Les seuils 760 et 520 px conservent respectivement la colonne unique et la
modale plein écran. Le navigateur confirme sur Warlord’s Conquest les deux sens
du changement de tier, le toggle Identified et `Make 2 duplicates`, sans erreur
console; la suite passe **74/74** et le build Vite est vert.

Un `.d2i` brut ne contient pas d’empreinte d’ABI; le bundle `.bkitems.json`
fingerprinté est le format recommandé pour transporter plusieurs items entre
versions.

La fenêtre `Choose items to add` possède maintenant une vraie file d’import :
les `.d2i` et `.bkitems.json` sélectionnés sont nommés, typés, mesurés et
retirables avant validation. Aucun inventaire ne change avant l’action explicite
`Import selected files`, et toute la sélection est refusée si un seul record
est invalide. Le codec contextualise les erreurs par fichier et par record,
refuse les extensions étrangères, JSON illisibles, empreintes BKVince
incompatibles, fichiers de plus de 16 MiB et sélections dépassant vingt records.
Le navigateur confirme qu’un Shared Stash présenté comme item reste dans la
fenêtre avec son erreur inline et sa file intacte, puis qu’un vrai
`Hand-Axe.d2i` ferme la fenêtre, ajoute exactement un Hand Axe au Personal Stash
et publie le succès atomique. La suite reste **56/56** et le build Vite est vert.

## Gate click-to-add de l'équipement joueur fermé

Les douze emplacements du papier-doll joueur suivent maintenant le même contrat
que les grilles et l'équipement mercenaire : une case vide ouvre `Choose items
to add`, filtré par le vrai `BodyLoc` BKVince, tandis qu'un item joueur déjà
sélectionné peut être placé directement dans un slot compatible. L'ajout depuis
le catalogue, l'import d'un `.d2i` ou d'un bundle, le déplacement, Undo/Redo et
l'export/reparse utilisent tous le bloc natif joueur. Une sélection provenant
de Virtual Stash, Shared Stash ou Trash ne peut jamais être interprétée comme un
index du héros : le clic ouvre plutôt un nouvel ajout joueur.

Le parcours navigateur crée une Amazon vierge, ouvre Head, ne montre que les 58
bases compatibles — dont Cap —, équipe le Cap puis confirme Undo et Redo. Le test
dédié ajoute un ring au bon slot, un Cap à Head, importe une amulette à Neck,
refuse ring→Torso et ring→Head, puis exporte et reparse les trois placements avec
des IDs uniques. La suite passe **57/57** et le build Vite est vert.

## Gate Belt RuneWizard fermé

La zone Belt est désormais une vraie grille 4×4 plutôt qu'un état `Empty belt`.
Elle affiche toujours les seize cases comme la référence, mais n'active que les
4, 8, 12 ou 16 slots réellement fournis par le belt équipé. Les positions D2S
restent les indices aplatis 0–15 et sont rendues du bas vers le haut comme dans
le panneau natif.

Le générateur lit sans modifier la colonne `Misc.txt:belt` et la référence de
layout `Armor.txt:belt`; vingt bases 1×1 sont compatibles. Catalogue, import
multiple, déplacement, collision, réduction de capacité, IDs et export/reparse
sont fail-closed. Le navigateur ajoute une Minor Healing Potion, confirme son
sprite et Undo/Redo, puis équipe un Sash et voit la capacité passer de 4 à 8.
Le clic sur la potion rouvre l'Item Editor complet avec le même aperçu vivant.
Les noms de potions dont les marqueurs internes ne laissaient que « 1 » utilisent
maintenant leur nom anglais gouverné. La suite passe **58/58** et le build Vite
est vert.

## Gate bandeaux d'or RuneWizard fermé

Equipment et Stash portent maintenant les deux capsules d'or visibles dans la
référence. Elles éditent directement `attributes.gold` et
`attributes.stashed_gold`; General reflète donc immédiatement la même valeur et
aucun état parallèle n'est créé. `Max` applique 10 000 par niveau jusqu'à
990 000 pour le héros et 2 500 000 pour le stash, tout en laissant le codec
préserver une valeur BKVince supérieure déjà présente.

Le navigateur passe le héros au niveau 99, obtient exactement 990 000 et
2 500 000 dans les capsules, retrouve les mêmes valeurs dans General, puis
confirme Undo/Redo sur l'or porté. Le test exporte et reparse simultanément les
stats D2S 14 et 15. La suite reste **58/58** et le build Vite est vert.

## Attributs des items complexes

Les propriétés gouvernées ne sont plus réservées aux qualités Magic, Set et
Unique. Tout record complexe peut ajouter, modifier ou retirer des attributs
sans changer sa qualité; les records compacts `simple_item` restent verrouillés
parce que leur format natif n'encode aucune liste de propriétés. Une Hand Axe
Normal conserve ainsi `+21 to Strength` après export/reparse v105. Le badge
`Click to add` est également rendu au-dessus des cellules voisines pour rester
net et complet.

## Portraits des huit classes

`extract-vanilla-ui-assets.ps1` extrait aussi les sept portraits de classes
vanilla 120×120 depuis le CASC D2R local. Le Warlock utilise un emblème généré
depuis son atlas de skills BKVince, faute de portrait dédié dans le mod. Les
assets versionnés vivent sous `public/ui/portraits` et le manifeste généré les
relie au header et à la fiche du personnage, avec les initiales comme fallback.
Aucun asset RuneWizard n'est copié. Le test valide les huit PNG et la suite
passe **60/60**.

## Qualités Rare et Crafted

L'Item Editor reconstruit maintenant les payloads Rare et Crafted du format
D2S v105. Les deux mots du nom proviennent des tables vanilla 3.2 gouvernées
`rareprefix.txt` et `raresuffix.txt`, lues sans modification avec un round-trip
TSV byte-exact. Les six IDs d'affixes natifs sont exposés comme trois paires
préfixe/suffixe et filtrés par les lignes BKVince `MagicPrefix.txt` et
`MagicSuffix.txt` qui portent `rare=1`, le type de base et l'item level.

Le compilateur applique les rolls minimum ou maximum de toutes les sélections,
refuse les groupes d'affixes en conflit et conserve l'édition manuelle des Item
Attributes. Un test crée une Hand Axe Rare puis Crafted nommée `Beast Bite`,
écrit `Jagged` #13 et `of Bashing` #12, exporte/reparse leurs propriétés et
confirme ensuite un no-op byte-exact. Le parcours navigateur couvre ajout depuis
le stash, édition, aperçu vivant, sauvegarde et réouverture. La suite passe
**61/61** et le build Vite est vert.

## Qualité Low

Les records complexes d'armure et d'arme peuvent aussi passer en qualité Low.
Le sélecteur propose les quatre lignes gouvernées de `LowQualityItems.txt` :
Crude, Cracked, Damaged et Low Quality. Leur ID 0–3 est écrit dans le champ D2S
3 bits, tandis que les bases Misc et les records compacts restent exclus.

Le test exporte/reparse les quatre variantes d'une Hand Axe et confirme un
second export byte-exact. Le navigateur choisit `Low Quality`, affiche
`Low Quality Hand Axe` dans l'aperçu, sauvegarde puis rouvre le même état. La
suite passe **62/62**.

## Variantes visuelles natives des items

L'Item Editor couvre aussi le choix d'apparence introduit par RuneWizard pour
les rings, amulets, charms et jewels. Le catalogue lit directement les tableaux
`ig` des constantes BKVince : neuf bases exposent 35 choix gouvernés, reposant
sur 23 images classiques distinctes.

Le script d'extraction récupère les DC6 et la palette `units` depuis
l'installation D2R locale de Vincent. Le générateur les convertit en PNG; aucun
code ni asset RuneWizard n'est repris. Le sélecteur affiche `Default` et chaque
variante native, met à jour l'aperçu et la grille, puis écrit le bit
`multiple_pictures` et le champ `picture_id` 3 bits du record D2S.

Le test choisit la cinquième apparence d'un Ring, exporte/reparse exactement
`multiple_pictures=1` et `picture_id=4`, refuse l'ID hors catalogue et ferme un
second export no-op byte-exact. Le parcours navigateur confirme les six
thumbnails, la persistance après `Save Changes` et réouverture, puis le rendu
mobile à 390×844 sans débordement. La suite passe **64/64** et le build Vite est
vert.

## Sélecteur visuel des propriétés

`Add magic attributes…` n'expose plus par défaut les centaines de stats natives
dans un grand `select`. Le parcours principal affiche maintenant les 236
déclarations `Properties.txt` supportées sous forme de liste sombre recherchable,
avec leur libellé Diablo, leur code gouverné et le nombre de groupes D2S qu'elles
produisent. Flèches haut/bas et Entrée sélectionnent une propriété; Échap ferme
seulement ce picker et conserve l'Item Editor. Les 390 stats numériques restent
disponibles dans une section avancée repliée.

Les propriétés sémantiques qui compilent plusieurs groupes sont également
présentées et éditées comme une seule ligne lorsqu'elles partagent réellement le
même rendu Diablo. `All Resistances +25` écrit ainsi les stats 39, 41, 43 et 45,
affiche une seule ligne, puis son champ partagé les porte ensemble à +40. Le
parcours navigateur confirme recherche, sélection clavier, Save Changes,
réouverture et mobile 390×844. Le test verrouille les métadonnées de `str`,
`res-all` et `dmg-elem`, les quatre résistances natives et le rendu sémantique
unique; la suite reste **64/64** et le build Vite est vert.

Le formulaire de propriétés n'expose plus les colonnes techniques
`Parameter / Minimum / Maximum`. Un schéma dérivé des 24 familles d'encodage
gouvernées produit les champs attendus pour chacune des 236 propriétés : une
valeur simple, une vraie plage de dégâts, un skill parmi 431 IDs nommés, un
skill tab, une classe, un monstre, une chance et un niveau de proc, des charges,
une durée ou un nombre de sockets. `oskill` affiche ainsi `Skill + Bonus` et les
trois `randclassskill*` affichent `Class + Bonus`. Les groupes paramétrés déjà
présents deviennent eux aussi éditables avec ces menus. Une matrice automatisée
construit, écrit et reparse un fichier D2S pour chacune des 236 propriétés; la
suite passe **70/70**. Le navigateur confirme `Whirlwind +7`, `Warlock +5`,
l'absence de placeholder `%+d` et l'absence de texte mojibake `â€¦`. Les valeurs
numériques utilisent un seul `input[type=number]` à `appearance: textfield`,
avec saisie directe et touches fléchées mais sans micro-chevrons navigateur
irréguliers ni bloc RuffnecKk ajouté.

Chaque formulaire compile aussi son état courant avant l'ajout et affiche le
texte final exact : `+7 to Whirlwind`, `+4 to Warlock Skills`, proc niveau/chance
et plages de dégâts ne restent plus sous la forme `+#`. L'action est désactivée
si le compilateur refuse la combinaison. Les neuf collisions de Monster ID sont
regroupées en 789 choix natifs uniques tout en conservant leurs aliases; le stat
155 affiche par exemple `100% Reanimate as: Goatman / Pit Lord` au lieu des
tokens `%0/%1` de la librairie amont. La matrice refuse désormais tout token non
résolu dans les aperçus des 236 propriétés et la suite passe **77/77**.

## Recherche visuelle des items

`Choose items to add` utilise maintenant les vrais sprites gouvernés au lieu de
faire porter l'identité visuelle par les seuls codes `RIN`, `HAX`, etc. Chaque
résultat montre l'image, le nom, la source, les dimensions et le type de record;
le code reste un repère secondaire compact. La recherche se pilote aussi avec
Flèches haut/bas et Entrée.

Une carte `Selected item` confirme explicitement l'objet, son sprite et sa
destination avant l'ajout. Les files Worldstone, Uber Ancients, Warlord's Glory,
Cube, Organ et Key montrent elles aussi le vrai sprite de chaque entrée avec
leurs contrôles de quantité et de retrait. Le test navigateur ajoute Ring Mail
par clavier puis les trois organes comme groupe atomique, et confirme le même
rendu à 390×844. Un test de manifeste exige désormais un PNG gouverné pour
chaque entrée quick-add; la suite passe **65/65** et le build Vite est vert.

## Catalogue unifié Bases, Sets, Uniques et Runewords

La recherche libre de `Choose items to add` ne s'arrête plus aux bases. Elle
mélange les **729 bases ajoutables**, les **215/215 Sets**, les **473/473
Uniques encodables** et les **112/112 Runewords compilables** issus des TXT
BKVince. Les filtres Bases, Sets, Uniques et Runewords complètent Armor,
Weapons et Misc; chaque résultat conserve son sprite et affiche un chemin de
catalogue comparable à RuneWizard.

Les identités nommées ne sont pas des coquilles visuelles : leur base canonique,
leur qualité, leur ID et leurs propriétés gouvernées sont écrits en une seule
mutation atomique, avec rolls minimum ou maximum. Les Runewords proposent
uniquement des bases acceptées par leurs types et leur nombre de sockets, puis
insèrent la séquence exacte de runes. `Call to Arms (CTA) (Flail)` choisit ainsi
Flail par défaut; `Plague (PB)` choisit Phase Blade tout en laissant sélectionner
une autre base réellement compatible.

Le test `CatalogHero` exporte et reparse Annihilus, Warlord's Conquest, CTA et
Plague, vérifie leurs IDs, propriétés, bases et fillers, ferme un second export
byte-exact et refuse les couples base/identité invalides. Dans le navigateur,
`call` mélange CTA et les Sets Orphan's Call, tandis que `plague` propose Plague,
Plague Bearer et Hellplague. Le rendu desktop et mobile 390×844 est validé; la
suite passe **66/66** et le build Vite de production est vert.

## Drag-and-drop RuneWizard des items

Les items se déplacent maintenant directement par drag-and-drop entre les
grilles visibles : Inventory, Personal Stash, Cube, Belt, pages ordinaires de
Shared Stash, Virtual Stash et Trash. Les douze slots d'équipement joueur sont
aussi des destinations depuis chacun de ces workspaces, avec validation du vrai
`BodyLoc`. Sur écran tactile ou
au stylet, un appui maintenu pendant 220 ms démarre le même geste; les cellules
compatibles prennent un contour doré et la destination survolée est mise en
évidence.

Le clic reste volontairement disponible : il ouvre toujours l'Item Editor, puis
une cellule libre peut encore servir au placement accessible. Les transferts
entre workspaces conservent la destination exacte (conteneur et coordonnées) et
restent target-first : collision, débordement, item incompatible avec le Belt ou
mauvais slot d'équipement ne retirent jamais la source. La page Shared Stash
stackable demeure verrouillée au compteur prouvé et n'accepte aucun drag.

Une cible Trash globale reprend maintenant le bouton carré de RuneWizard sous
la ceinture. Un clic ouvre la grille de récupération; un drag souris, tactile ou
stylet y déplace l'objet vers la première cellule libre et le badge indique le
nombre de records récupérables. Les transferts successifs sont auto-placés sans
collision, restent target-first et participent au même Undo/Redo composé que la
source. Le test navigateur ajoute un Cap, le déplace vers Trash, le restaure par
Undo puis rejoue le transfert par Redo; la suite passe **74/74**.

Le test de transfert couvre Physical → Virtual → Trash → Cube en `(2,2)`
→ Virtual → Right hand. Il conserve propriétés Magic et nouveaux IDs,
reparse `location_id=1/equipped_id=4`, et prouve qu'un essai incompatible vers
Head laisse source et cible inchangées. Le parcours navigateur confirme aussi le
clic Stash → Cube, l'ouverture de l'Item Editor, une source Virtual
`draggable`, les douze cibles Equipment typées et l'absence de débordement au
viewport 390 px. La suite reste **66/66** et le build Vite de production est
vert.

## Recherche d'items par propriété

La recherche du catalogue reprend aussi le parcours documenté par RuneWizard :
un item peut être trouvé par une propriété visible, même si son nom ne contient
pas la requête. L'index ne repose pas sur des mots-clés maintenus à la main : il
est construit une fois depuis les propriétés Sets, Uniques et Runewords que le
codec BKVince compile réellement. Codes `Properties.txt`, descriptions Diablo,
skills, procs, sockets et drapeau Ethereal restent ainsi cohérents avec l'item
qui sera écrit.

Chaque proposition explique le match sur une ligne dorée. `teleport` retourne
huit résultats et affiche notamment `+1 to Teleport` sous Enigma; `Cannot Be
Frozen` retrouve Raven Frost et `All Skills` retrouve Annihilus. Le parcours
navigateur sélectionne Enigma au clavier, l'ajoute dans le Personal Stash et
confirme le message `1× Enigma added atomically`. La même recherche à 390×844
reste contenue dans un viewport de 390 px. La suite reste **66/66** et le build
Vite de production est vert.

## Panier libre d'items

`Choose items to add` permet maintenant de composer un lot arbitraire avant de
toucher la sauvegarde. Chaque sélection conserve indépendamment sa base, son
item level, sa quantité de copies ou de stack, son identité Set/Unique/Runeword
et ses rolls perfect aux maxima gouvernés. Les lignes identiques sont fusionnées,
restent ajustables ou supprimables et le panier est borné à vingt records.

Le placement est une transaction unique : tout le panier doit tenir dans la
destination, sinon aucun record n'est ajouté. Le test codec exporte et reparse
deux Annihilus, Warlord's Conquest et Call to Arms sur Flail, puis vérifie qu'un
lot trop grand pour le Cube laisse le snapshot original intact. Le navigateur
confirme les mêmes quatre objets dans le Personal Stash, un Undo qui retire le
lot complet, un Redo qui le restaure et un rendu mobile sans débordement à
390×844. La suite passe **67/67** et le build Vite de production est vert.

## Empty personal stash et seuil 320 px

Le bouton `Empty personal stash` retire uniquement les records actifs du
Personal Stash dans un snapshot joueur atomique. Inventory, Equipment, Belt,
Cube et les huit charms BK gelés ne sont pas touchés. Une seule commande Undo
restaure la stash complète et Redo réapplique le vidage.

Le test codec place deux Hand Axes dans la stash et un Cap dans le Cube, puis
exporte et reparse le résultat en vérifiant que seul le conteneur stash est
vide. Dans le navigateur, deux Annihilus sont retirés tandis que le Cap reste
dans le Cube; Undo restaure les deux Annihilus et Redo les retire à nouveau.
Le shell et la modale d'ajout suivent désormais la largeur réelle du viewport :
à la largeur nominale 320 px, `clientWidth=scrollWidth=304` dans le shell chargé
et aucun débordement horizontal n'apparaît. La suite passe **68/68** et le build
Vite de production est vert.

## Sprites de recherche contenus et création Perfect

Les sprites du catalogue sont maintenant confinés dans une surface intérieure
de 35×35 px avec `object-fit: contain` et deux niveaux d'`overflow: hidden`.
Les armes hautes ne peuvent plus traverser les lignes voisines : Dacian Falx,
Dagger, Calamity et Call to Arms restent intégralement dans leur vignette en
desktop comme à 319 px, sans débordement horizontal.

Tout objet forgé par `Choose items to add` utilise désormais les meilleurs rolls
possibles. Une armure reçoit le maximum de sa plage de défense; Sets, Uniques,
Runewords et groupes rapides compilent leurs propriétés avec les valeurs
maximales dans les limites du format. L'interface affiche
`Perfect · Maximum native save-compatible values` et ne propose
plus de création minimum. Les imports `.d2i` et bundles restent fidèles à leurs
records d'origine. Le navigateur confirme Annihilus à +20 all attributes,
+20 all resistances et +15% XP ainsi qu'un Cap à 5 de défense sur 3–5. Le
paper-doll affiche aussi des silhouettes assombries issues des sprites gouvernés
dans ses douze slots vides; elles disparaissent sous l'objet équipé et ne
bloquent ni clic, ni drag, ni Undo. La suite passe **69/69** et le build Vite de
production est vert.

## Arbres Skills natifs comme RuneWizard

La surface Skills ne redessine plus les cases et les flèches avec du CSS/SVG.
L'extracteur CASC récupère les huit atlases Blizzard
`Spells/Skill_Trees/*Skilltree.sprite`; le générateur produit leurs trois frames
895×1169, soit **24 fonds natifs** versionnés sous `public/ui/skill-trees/`. Les
deux colonnes transparentes de chaque frame source 897×1169 sont retirées sans
rééchantillonnage. Aucun
asset RuneWizard n'est copié. Amazon, Sorceress, Necromancer, Paladin,
Barbarian, Druid, Assassin et Warlock utilisent donc les vrais cadres, onglets,
cases de rang et liaisons de leur arbre en jeu.

La scène suit maintenant la géométrie RuneWizard observée directement : largeur
maximale 420 px, ratio 895/1169, en-tête 24 px dans un contenu espacé de 16 px,
onglets sur les 7,5 % supérieurs
et grille 3×6 avec les mêmes paddings et gaps proportionnels. Les icônes occupent
environ 64×64 px et les rangs 24×24 px. Les atlases Blizzard 7920×130 sont
maintenant compilés en **240 images individuelles 130×130**, une par skill
gouverné. Chaque nœud affiche donc un vrai `<img>` comme RuneWizard, sans grande
bande de 7 800 px déplacée par une transformation fractionnaire. Les deux
colonnes transparentes de chaque frame source restent retirées. Le chiffre se
place en bas à droite dans la petite case du fond, reste vide à zéro et reçoit le
halo doré seulement après investissement. Une compétence verrouillée suit le
même `brightness(.35) grayscale(1)` que la référence; les trois boutons
d'onglet sont transparents, car leur chrome vient du fond natif.

La passe de netteté conserve les fonds à leur résolution source Blizzard
complète au lieu de les réduire à 447×584 puis de les agrandir dans le navigateur.
Le compteur reprend exactement le texte rouge 16 px de la référence, le switch
`Ignore Game Rules` mesure 32×16 px avec libellé 12 px, le halo investi passe à
4 px et les infobulles ajoutées localement ont été retirées : RuneWizard ne fait
qu'éclaircir le bouton au survol.

L'audit direct du site RuneWizard a aussi corrigé la sémantique. Le compteur
`Unused Skill Points` est informatif et ne change pas lorsque le niveau d'une
compétence est édité. Les prérequis déterminent l'état visuel verrouillé; le
niveau requis est vérifié au clic. `Ignore Game Rules` retire ces deux gardes et
conserve les niveaux raw 0–255. Le bouton `Reset skills` propre à l'ancienne
interprétation a été retiré de la surface visible.

Un clic augmente; `Shift`-clic ou clic droit diminue; Undo/Redo couvre les deux
sens. Le navigateur confirme Amazon avec Jab 0→1, compteur stable à 0, Power
Strike visuellement déverrouillé mais refusé au niveau 1, Undo/Redo et retour à
zéro. En mode libre, Lightning Fury ferme 0→1→0. Les trois fonds Amazon changent
avec leur onglet et le Warlock affiche `Chaos Skills / Eldritch Skills / Demon
Skills` sur son atlas natif. La suite passe **77/77** et le build Vite de
production est vert.

La surface Stats suit aussi la croissance desktop réelle de RuneWizard. À partir
de 960 px de viewport, le cadre passe de sa composition compacte 664 px à
**884 px**, la navigation de 132×42 px à **176×56 px** et ses libellés de 11 px
à **16 px**. Le contenu Skills devient ainsi 707 px de large autour de la scène
native 420 px, exactement comme la référence mesurée, sans agrandir ni
rééchantillonner les fonds ou les icônes Blizzard.

## General compact comme RuneWizard

La page General n'est plus une succession de cinq panneaux techniques. Le titre
interne redondant, la classe read-only, les bornes numériques, le bloc d'export,
le niveau et les champs d'or dupliqués ont été retirés de cette surface. Les
capsules Equipment et Stash demeurent l'unique endroit où modifier l'or.

La fiche suit maintenant la référence : nom sur 300 px, rangée `Level / Map
Seed`, puis trois colonnes `Strength/Dexterity/Energy/Vitality`,
`Experience/Unused Stats/Unused Skill Points` et
`Current Hp/Max Hp/Current Mana/Max Mana/Max Stamina`. Les champs ont une hauteur
de 22 px; Expansion, Hardcore, Died et Ladder utilisent une rangée de switches
compacts, suivie d'une seule note de préservation violette.

Le navigateur mesure un contenu de 538 px, une fiche de 471 px et trois colonnes
sans débordement. Il confirme un seul Level, aucun hint de bornes, Strength
30 → 41, Expansion true → false, les deux Undo, les deux Redo puis le retour au
snapshot initial, avec zéro erreur console. La suite reste **74/74** et le build
Vite de production est vert.

## Chrome Quests et Waypoints fidèle aux captures

Quests n'utilise plus le titre/description interne ni une toolbar horizontale.
La surface suit la séquence de la référence : `Unlock Hell`, note violette avec
icône d'information, `Complete All / Reset All`, marge de respiration, puis
Normal et ses cinq actes en trois colonnes. Les difficultés suivantes ne sont
plus séparées par une ligne artificielle.

Waypoints retire le titre interne, la note technique et les boutons globaux qui
n'apparaissent pas dans la capture RuneWizard. `Normal` commence directement en
haut du contenu, suivi des cinq actes et de leurs 39 switches; Nightmare et Hell
gardent la même grille, soit 117 bits éditables individuellement.

Le navigateur confirme 84 switches Quests : `Unlock Hell` active exactement 54
états, `Complete All` 81 et `Reset All` zéro, avec Undo/Redo du snapshot complet.
Waypoints confirme 117 switches, Rogue Encampment false → true → Undo → Redo →
false restauré, zéro note/action globale résiduelle et zéro erreur console. La
suite reste **74/74** et le build Vite de production est vert.

## Item Bonuses fidèle à RuneWizard

La surface technique précédente est retirée. `Item Bonuses` reprend maintenant
la composition exacte de RuneWizard : aucun titre interne, aucun compteur de
records et aucune liste de sources; le contenu commence directement par les
quatre colonnes `Attributes`, `Resistances`, `Breakpoints` et `Misc`. Le cadre
mesure 664×337 px avec la navigation 132 px et les huit rangées 42 px. Les
libellés, espacements, pourcentages et le `+0` de `All Skills` suivent la
référence; sous 700 px, les quatre groupes se replient proprement en deux
colonnes sans débordement.

Les dix-neuf valeurs ne sont pas décoratives. Elles agrègent en direct le
payload effectif de tout l'équipement joueur, y compris les attributs de
Runeword et les fillers socketés. Les groupes natifs `all attributes` et `all
resistances` alimentent chacun leurs quatre valeurs, tandis que Life, Mana et
les résistances par niveau sont recalculées au niveau courant. Le test équipe
Harlequin Crest, Mara's Kaleidoscope, The Gnasher, Arachnid Mesh, Sandstorm
Trek et Chance Guards : il obtient 40/25/25/25, 148 Life/Mana, 30/30/30/100 de
résistances, FCR 20, FHR 20, FBR 15, IAS 25, +6 All Skills, 90 MF et 200 GF,
puis exporte, reparse et ferme un no-op byte-exact.

Dans le navigateur, Harlequin Crest Perfect passe les quatre attributs à 2,
Physical Res à 10%, All Skills à +2 et Magic Find à 50%. Le changement de
niveau 1→99 recalcule Life/Mana de 1 à 148; deux Undo puis deux Redo prouvent
l'ajout et le calcul par niveau avant restauration du héros vierge. La suite
passe **76/76** et le build Vite de production est vert.

## Fenêtre Choose items to add compacte comme RuneWizard

La fenêtre d'ajout reprend maintenant la hiérarchie directe de la référence :
le titre ouvre immédiatement sur la recherche et les six groupes rapides. Le
contexte technique de destination reste porté par l'accessibilité, mais ne
charge plus visuellement l'en-tête. Lorsqu'un groupe est actif, les filtres du
catalogue et le champ global `Item level` disparaissent au profit d'un panier
compact : lignes de 38 px, sprites contenus dans 30 px, quantités et vraie icône
de corbeille.

L'action principale est placée avant l'import portable et reste visible pendant
le défilement. Le navigateur mesure la modale Warlord à 600×532 px, ses cinq
lignes à 38 px et confirme `Add 5 items`, puis 6 après avoir doublé Warlord's
Conquest. L'ajout produit six vrais records et Undo/Redo restaure exactement
0 → 6 → 0. La recherche `call to arms` retourne une seule proposition, crée CTA
sur la base compatible Flail `FLA · 2×3`, puis Undo rend la case disponible sans
laisser de sélection `Unknown item`. La suite reste **74/74** et le build Vite de
production est vert.

## Workspace Equipment / Stash / Cube / Belt recalé

Le workspace supérieur reprend maintenant les proportions de la capture
RuneWizard sans réduire les conteneurs BKVince. Le header chargé mesure 56 px et
ses actions 28 px. Comme dans RuneWizard, il ne contient plus l'export : la
sélection fixe `BKVince v105` et l'action `Save to file` sont alignées sous le
panneau Stats, après la note de compatibilité au même emplacement que la
référence. Les confirmations ne repoussent plus tout le contenu : elles
apparaissent comme un toast fixe, non bloquant, qui disparaît après 2,8 secondes.

Les trois colonnes mesurent respectivement 252, 262 et 84 px avec un écart de
24 px. Le paper-doll occupe 252×184 px, exactement aligné avec la capture, puis
l'inventory conserve sa vraie géométrie BK 11×8 et sa colonne gelée de charms.
Personal Stash reste 16×13, Cube 6×6 et Belt 4×4; seules leurs cellules sont
recalées à 15, 12 et 18 px pour préserver le langage visuel compact. La rangée
de stash revient aux deux actions visibles de la référence : `Empty personal
stash` et `Load Shared Stash`.

Le navigateur ajoute un Cap Perfect dans Personal Stash, le déplace vers Trash,
prouve Undo/Redo puis restaure le témoin vierge. La capsule Equipment applique
990000 avec `Max` et Undo revient à zéro. À 390×844, Equipment mesure 252 px,
Stash 262 px et `clientWidth=scrollWidth=375`; aucune erreur ou alerte console
n'est émise. La suite reste **74/74** et le build Vite de production est vert.

Une création ou l'ouverture d'un nouveau D2S ramène maintenant explicitement le
workspace à `scrollY=0` après le premier rendu. Le header sticky de 56 px ne peut
donc plus masquer Equipment, Stash et Cube au premier affichage; le navigateur
mesure leurs titres à 75 px, sous le bord inférieur du header à 57 px.

## Item Editor aligné sur la fenêtre RuneWizard

La modale desktop mesure maintenant 600 px et ouvre directement sur le parcours
utile de la référence, sans titre technique ni texte de sérialisation visible.
Quantité/duplication, Ethereal/Identified/Personalized et Downgrade/Upgrade
précèdent une composition 305/195 px : contrôles à gauche, aperçu Diablo vivant
à droite. Quality/Base item, identité Set ou Unique, Item Level et Defense sont
suivis immédiatement de `Magic Attributes`, `Item Set Bonuses` et `Socketed`.

Les durabilités, compilateurs de rolls et transferts restent disponibles sous
des volets repliés après le parcours principal. Une liste Set inactive n'expose
plus ses codes techniques : `Add set attributes…` applique directement son roll
Perfect. Les descriptions répétitives des champs sont masquées, le footer reste
sticky et ses actions reprennent les tons gris/rouge de RuneWizard.

Le navigateur modifie Warlord's Conquest de +15 à +52 Strength, ferme puis
rouvre l'objet, confirme la valeur, restaure +15 par Undo et rejoue +52 par Redo
avant de nettoyer le groupe de test. La suite reste **74/74** et le build Vite de
production est vert.

## Pack de départ BKVince complet

La création ne se limite plus aux huit charms historiques. Elle reproduit les
dix records observés dans un personnage créé par BKVince et déclarés par
`CharStats.txt` : `mff`, `mfc`, six `mfd`, `box` et `tsc`. Les huit charms
occupent les lignes 1 à 8 de la colonne 11 gelée, le Horadric Cube occupe le
bloc 2×2 en haut à gauche et le Town Portal Scroll est placé en colonne 10,
ligne 8. La colonne gelée est maintenant soulignée visuellement dans la grille,
sans inventer une restriction D2S que les sources ne démontrent pas.

Le navigateur confirme les dix sprites et les cellules libres exactes. Le
témoin `HEStarterTen.d2s`, créé par l'éditeur comme Amazon niveau 1, est chargé
par `D2RLoader.exe -mod BKVince -txt -offline`; l'inventaire en jeu montre le
Cube, le scroll et les huit charms aux positions attendues. Après `Save and
Exit`, le fichier reste à 1 283 octets, checksum valide, contient exactement les
dix mêmes records et se réexporte no-op byte-exact. La suite passe **77/77** et
le build Vite de production est vert.

## Tooltips de grille aux bords

Le message `Click to add` s'aligne désormais vers l'intérieur de la grille
lorsque la case visée touche un bord horizontal ou vertical. Les cellules
portent leur ancrage `start`, `center` ou `end`, commun à Inventory, Personal
Stash, Cube et Belt; le texte ne peut donc plus être rogné par le conteneur.

Le navigateur ouvre puis ferme la fenêtre d'ajout depuis la colonne 10 de
l'Inventory et confirme le retour de focus avec le libellé complet visible. Un
audit côte à côte de l'onglet Skills contre RuneWizard confirme par ailleurs
les mêmes proportions de cadre, navigation, scène native et onglets : aucune
retouche non démontrée n'a été ajoutée. La suite reste **77/77** et le build
Vite de production est vert.

## Créations Perfect au niveau 99

Tout item créé par le catalogue, un groupe rapide ou directement dans un slot
d'équipement part maintenant de `Item Level 99`, comme RuneWizard. Ce défaut
est défini par le codec et partagé par l'interface; il ne modifie jamais le
niveau d'un item chargé ou importé. Les rolls gouvernés restent au maximum
natif et la défense de base reste à sa valeur maximale.

La comparaison vivante crée Harlequin Crest sur deux héros niveau 1. RuneWizard
et BKVince affichent tous deux `Item Level: 99`, `Defense: 141` et les mêmes
rolls Perfect. Après fermeture de l'éditeur, BKVince efface aussi la sélection
temporaire et revient directement aux grilles, sans la barre `Selected item`
absente de RuneWizard. La suite reste **77/77** et le build Vite de production
est vert.

## Item Editor responsive aux dimensions RuneWizard

À viewport 1 280×720, la fenêtre RuneWizard vivante mesure 800 px et non 600 px.
L'Item Editor BKVince adopte maintenant exactement cette largeur avec une
colonne de contrôles de 400 px, un aperçu flexible de 320 px et un écart
de 16 px. Sous 820 px, la fenêtre retrouve le format compact de 600 px et la
composition 305/195 px observée dans les captures étroites; l'empilement sur une
seule colonne est réservé au vrai mobile sous 520 px.

Le tooltip suit aussi la ponctuation de la référence :
`[click to edit]/[drag to move]`. La comparaison DOM mesure BKVince à
800×648 px et RuneWizard à 800×647,5 px au même viewport. Les deux fenêtres ont
la même composition utile et le même footer sticky. Harlequin Crest reprend
aussi la teinte native `cgrn` sous la forme exacte `Cyan Green tint`, avec la
pastille `rgb(0, 255, 127)`, puis le nom, la base, le niveau d'objet et les
propriétés aux mêmes coordonnées à moins de 0,3 px. L'ordre de l'aperçu suit
celui des attributs éditables, et la barre `Selected item` reste masquée pendant
l'édition. La suite passe **78/78** et le build Vite de production est vert.

## Propriétés Based on Character Level

Les 37 stats dont `ItemStatCost` fournit la seconde ligne gouvernée
`(Based on Character Level)` conservent maintenant cette information dans les
tooltips, les lignes de Magic Attributes et l'aperçu de l'Item Editor. La valeur
visible reste calculée au niveau courant : Harlequin Crest affiche `+1 to Life`
au niveau 1 et `+148 to Life` au niveau 99, avec le suffixe explicatif dans les
deux cas.

Le dashboard Item Bonuses continue volontairement d'agréger les valeurs
effectives sans modifier son modèle numérique. Un test parcourt les 37 stats
gouvernées, puis le scénario d'équipement Harlequin confirme les deux niveaux,
le total Life/Mana 148 à 99 et le round-trip no-op. La suite passe **78/78** et
le build Vite de production est vert.

## Parité visuelle du panneau Skills

Le panneau Skills desktop est maintenant mesuré directement contre RuneWizard
sur la même Amazone et au même viewport. La colonne de navigation mesure 175 px,
le contenu utile 708 px, le fond natif 420×548,6 px et son origine est identique
à moins de 0,1 px. Le padding de 16 px, la toolbar de 24 px et le toggle
`Ignore Game Rules` de 32×16 px reproduisent aussi la référence.

La pile typographique est exactement `Montserrat, Verdana, sans-serif`, comme
RuneWizard. Dans l'environnement Windows observé, le repli Verdana produit les
largeurs de texte de référence : 116 px pour `Ignore Game Rules` et 189 px pour
`0 skill choice remaining`. Les icônes de navigation font désormais 24 px, le
gap 8 px et `Item Bonuses` reste sur une seule ligne. Le navigateur confirme
également ajout par clic et retrait par clic droit d'un rang avec
`Ignore Game Rules`.

La passe de netteté compare maintenant les trois arbres Amazon nœud par nœud.
Les 30 couples nom/fichier correspondent exactement à RuneWizard (`08.png`
pour Jab, `16.png` pour Power Strike, etc.), les trois fonds changent avec le
même onglet et le delta maximal des rectangles relatifs est **0 px**. Le halo
investi est appliqué directement à l'image comme dans la référence, et le filtre
verrouillé calculé est exactement `brightness(0.35) grayscale(1)`. La suite
reste **78/78** et le build Vite est vert.

## Parité visuelle et fonctionnelle de Quests / Waypoints

Les deux panneaux desktop sont maintenant mesurés directement contre
RuneWizard sur la même Amazone niveau 1 et au même viewport 1 280×720. Quests
reprend le padding 16 px, les boutons 13,2/16 px, la note 676×87 px, les titres
20/30 px, les actes 16/24 px, les rangées 23,49 px et les toggles 32×16 px.
Sa hauteur diffère de seulement **0,08 px** de la référence, à vide comme après
`Complete All`.

`Complete All` active les 81 quêtes et les trois récompenses
`consumed_scroll`; `Reset All`, `Unlock Hell`, les cases individuelles et
Undo/Redo restent des mutations atomiques. Les libellés visibles suivent la
ponctuation de la référence, tandis que les pictogrammes proviennent toujours
des assets gouvernés BKVince.

L'audit vivant corrige aussi une conclusion prise depuis une capture tronquée :
Waypoints possède bien `Unlock All / Reset All` dans RuneWizard. BKVince expose
maintenant ces actions et reproduit exactement une hauteur de contenu de
2 001,354 px, des grilles de 539,792 px, des cartes de 257,899 px, des rangées
de 17,995 px et des toggles 32×16 px. Les 117 waypoints passent 0→117, puis
Undo/Redo restaure les deux états. Comme RuneWizard, un héros neuf niveau 1
commence avec les trois `Rogue Encampement` actifs. La suite passe **80/80** et
le build Vite de production est vert.

## Parité visuelle Item Bonuses / Chronicle / Mercenary

L'audit vivant reprend la même Amazone niveau 1 et le même viewport 1 280×720
dans les deux éditeurs. `Item Bonuses` ferme un contenu utile de
676,007×290,772 px : les quatre colonnes ont les mêmes hauteurs que RuneWizard,
un gap de 48 px, des titres 16/24 px, des métriques 13,2/19,8 px et les mêmes
séparations de groupes. Le cadre complet ne diffère que de 0,07 px de hauteur,
attribuable à l'arrondi du navigateur.

L'état vide de `Chronicle` mesure exactement 676,007×77,101 px. Son titre
20/30 px et son bouton `Load Shared Stash` de 146,927×31,102 px correspondent à
la référence; aucun placeholder technique ne revient dans cet état.

Le chrome vide de `Mercenary` reprend le titre 20/30 px et le switch 32×16 px.
Le libellé `died` mesure exactement 17,994791 px dans les deux éditeurs.
BKVince conserve volontairement le bouton `Create mercenary`, absent de
RuneWizard, afin de permettre la création du record natif nécessaire aux tests
de plugins. Le navigateur prouve création, Name ID 0→321, Undo→0, Redo→321,
suppression, restauration et retour final à l'état sans mercenaire. La suite
reste **80/80**, le build Vite de production, `git diff --check` et le cadastre
sont verts.
