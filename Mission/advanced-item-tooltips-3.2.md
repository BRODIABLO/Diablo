# Advanced Item Tooltips — D2R 3.2.92777

## Décision produit

La première phase du projet se concentre sur l'indication `Max Sockets: N` pour
les objets socketables. La phase suivante, approuvée par Vincent le 27 juillet
2026, ajoute les plages de rolls des propriétés et de la défense sans modifier
le comportement acquis de `Max Sockets`. L'affichage
`Sockets: courant / maximum` demeure hors périmètre.

La ligne utilise le marqueur UTF-8 privé du renderer D2R pour le blanc vanilla.
Sur une weapon, elle apparaît immédiatement sous le bloc de dommages; sur une
armor, elle apparaît immédiatement sous la ligne de défense. Depuis la révision
2.1.2 demandée par Vincent, un item qui porte déjà des sockets conserve aussi
la ligne `Max Sockets`; `Socketed (N)` ne la supprime plus.

## Implantation

Le plugin hybride `AdvancedItemTooltips.dll` 2.0.0 est attribué à `RuffnecKk`
et ne déclare pas `ModScopedOnly`. Il accepte uniquement le build 92777.

- `ITEMS_GetMaxSockets`, RVA `0x36EAD0`, calcule la capacité de l'objet concret,
  incluant son niveau, sa base et les limites de type. Le plugin ne duplique pas
  cette logique.
- L'état socketé courant n'intervient plus dans cette décision; seule une
  capacité maximale native nulle exclut l'ajout.
- Advanced Item Tooltips n'installe aucun hook. Il expose une fonction bornée
  qui retourne la ligne blanche et une fonction bornée qui résout son ancre.
- Transmogrify 1.2.5, déjà propriétaire du hook final strict à
  `ITEMS_BuildItemTooltip` (`0x2BD480`), appelle cette fonction puis insère la
  ligne juste avant la première ligne interne contenant `Damage:` ou `Defense:`.
  D2R assemble ce buffer du bas vers le haut : cette position correspond à la
  ligne immédiatement sous le dernier dommage visible ou sous la défense.
- Une capacité native nulle exclut l'objet de l'affichage.

## Validation attendue

- tests unitaires de placement sur tooltips blanc et magique;
- weapon socketable sans socket : ligne blanche sous tous les dommages;
- armor socketable sans socket : ligne blanche sous la défense;
- item déjà socketé : ligne `Max Sockets` présente;
- objet non socketable exclu;
- inventaire, coffre, cube, marchand et sol;
- souris et manette;
- refus sûr sur un build ou une signature incompatible.

## Validation du 22 juillet 2026

- compilation Release x64 réussie;
- tests unitaires réussis pour les tooltips blanc et magique, les fins de ligne
  CRLF et l'exclusion d'une capacité native nulle;
- DLL finale SHA-256
  `76676B83695F6AFD6BEE96159F2D39B7A7E333E2072E4E910284758888A3B605`;
- DLL Transmogrify 1.2.2 SHA-256
  `AC7CECE764072DA39CE494231F6AA5100B499E0897322261EB39AB77D8999EB5`;
- cold-start BKVince réussi : Advanced Item Tooltips 1.3.0 et Transmogrify
  1.2.1 actifs simultanément, sans échec de plugin ni avertissement
  d'intégration;
- partie DummyTester atteinte sans crash;
- la comparaison visuelle blanc/magique reste à effectuer avec deux objets
  socketables préparés dans le même inventaire.

La validation visuelle du Hand Axe a révélé que la version 1.1.0 affichait les
séquences de couleur littéralement et n'émettait aucun saut de ligne si le bloc
vanilla n'en possédait pas. Un essai 1.1.1 avec des octets `0xFF` bruts a rendu
la chaîne UTF-8 invalide, affiché `*c5` / `*c0` et déclenché la police de repli
du renderer. La version 1.1.2 retire donc tout code de couleur et garantit un
`0A` terminal avant la suite du tooltip.

La version 1.1.3 rétablit le gris socketing avec le format réellement consommé
par le renderer 3.2, déjà validé par Transmogrify : le marqueur UTF-8 privé
`EE 81 BE 35`, suivi du texte, puis `EE 81 BE 30` pour restaurer la couleur.

La version 1.2.0 a démontré que `ITEMS_GetStatsDescription` ne permet pas de
cibler la frontière du nom et que son reset gris vers blanc écrase l'état bleu
des modifiers suivants. La version 1.3.0 retire entièrement ce hook et ce reset;
l'intégration finale de Transmogrify 1.2.1 a d'abord supposé à tort que le
premier saut de ligne suivait le nom. La capture sur le sceptre magique a prouvé
que le buffer final est assemblé dans l'ordre inverse de l'affichage : cette
ancre plaçait `Max Sockets` juste avant le dernier affix visible. Transmogrify
1.2.2 insère désormais la ligne avant la dernière ligne logique du buffer — le
nom — sans modifier les octets vanilla des modifiers.

## Révision du 26 juillet 2026

Vincent retient finalement le blanc vanilla et abandonne le placement sous le
nom. La version 1.4.0 prépare l'ancrage sous les dommages ou la défense et
refuse la ligne lorsque `STATLIST_GetUnitStat(item, 194, 0)` est positive. Les
tests statiques couvrent weapon à un dommage, throwing weapon à deux dommages,
armor, absence d'ancre, capacité nulle et item déjà socketé. Le build, le cold
start et les tests sont réussis. Les DLL déployées dans la source gouvernée,
le dossier global et le dossier mod-local portent respectivement les SHA-256
`069CD1A9C2DD3F73E124A8D4E67E0EFB93E5D584B03438FD3745CB7FD46C78F8`
pour Advanced Item Tooltips et
`EB1A0052D71EB4D640FC179636B09B217DAD52CDD2135965A5BD47E3BC468026`
pour Transmogrify. Le cold start BKVince charge 24 plugins actifs, avec 0 rejet,
0 échec et 20 memory patches appliqués sur 20. La validation visuelle finale
des trois cas — weapon, armor et base déjà socketée — reste le dernier gate en
jeu.

## Phase approuvée le 27 juillet 2026 — Affix Roll Ranges

Vincent retient le format SlashDiablo comme cible visuelle : la propriété
conserve sa couleur native et reçoit sur la même ligne une plage verte
`[minimum - maximum]`. Le tooltip doit s'adapter à la largeur et à la hauteur
ajoutées sans changer la police. Les objets d'armor affichent aussi la plage de
défense de base et les propriétés de défense variables.

Le calcul doit provenir des TXT du mod effectivement chargé, conformément à la
décision explicite de Vincent. La première implantation lit le dossier Excel
loose du `modDirectory` actif, comme le font déjà les plugins eezstreet qui
consomment des tables. Elle ne contient aucune donnée BKVince codée en dur et
échoue fermée lorsque ces TXT ne sont pas disponibles. Le support futur des
mods entièrement packés exigera une preuve séparée des tables compilées.

La plage d'une ligne est la somme des contributions de toutes les sources
réellement portées par l'objet, et non le meilleur affixe individuel possible.
Cela couvre notamment les doubles affixes, les propriétés fixes de craft,
l'Enhanced Damage et les dégâts plats minimum/maximum. Exemple obligatoire :
un craft donnant `5-10% Faster Cast Rate` et un affixe fixe de `10%` doit
afficher `[15 - 20]`.

Le séquencement retenu privilégie l'exactitude :

1. prouver sur le build 92777 les structures de l'objet et l'accès aux tables
   compilées du mod actif;
2. construire un évaluateur canonique des propriétés et additionner les plages
   des sources identifiées;
3. résoudre les propriétés fixes des crafted items, avec consensus par stat
   lorsque plusieurs recettes restent candidates;
4. traiter explicitement Enhanced Damage, dégâts plats et défense;
5. augmenter le tooltip final avant le fenêtrage d'Extended Item Stats, puis
   valider les contextes, résolutions, langues, souris et manette.

Le moteur échoue fermé : si la provenance reste ambiguë, si la valeur roulée
n'appartient pas à la plage calculée ou si une fonction de propriété n'est pas
supportée, il omet la plage et journalise le diagnostic. Aucune plage
approximative ne doit être présentée comme exacte. La fonctionnalité reste dans
la DLL autonome hybride `AdvancedItemTooltips.dll` pendant son incubation et
est destinée à la future `plugin-items.dll` sous configuration JSON.

## Implantation 2.0.0 du 27 juillet 2026

- `ItemData` 92777 est lu aux offsets prouvés de qualité, file index, préfixes,
  suffixes et automagic; les RVA runtime demeurent strictement verrouillées au
  build 92777;
- `properties.txt` est décodé uniquement pour les fonctions simples prouvées;
  les fonctions paramétrées ou multi-valeurs non modélisées sont omises;
- les contributions partageant une statistique canonique sont additionnées,
  notamment Enhanced Damage et les dégâts minimum/maximum;
- les propriétés fixes de `cubemain.txt` sont combinées aux affixes réels des
  crafted items; les recettes incompatibles avec le roll visible sont filtrées
  et les survivantes doivent produire exactement la même plage;
- le test réel BKVince confirme le gate obligatoire : suffixe `+10% FCR` plus
  craft `5-10% FCR` produit `[15 - 20]`;
- la plage utilise le vert privé D2R puis restaure le dernier marqueur couleur
  de la ligne, sans changer la police ni recolorer le modifier natif;
- la défense de base est ajoutée en blanc sous `Defense` seulement lorsqu'elle
  peut être reconstruite exactement; la présence d'Enhanced Defense la fait
  omettre pour cette tranche;
- Transmogrify demeure l'unique propriétaire du hook final et appelle l'export
  borné d'Advanced Item Tooltips avant Max Sockets et avant le fenêtrage
  d'Extended Item Stats;
- builds Debug/Release et tests unitaires réussis;
- déploiement mod-local vérifié par hashes : Advanced Item Tooltips
  `4B621A76916993C676FE07821A1D8810A848052F11EBD2E6FDEBA450BC5573A1` et
  Transmogrify
  `B004FF2DA3BD391F24EDE6405BCB0FDAD7927761958FAFFFC3FDDF32B2D85E4B`;
- cold start réussi : Advanced Item Tooltips 2.0.0 charge son catalogue sans
  avertissement, Transmogrify 1.2.5 accepte ses trois hooks, et D2RLoader
  termine avec `scanned=27 active=25 disabled=2 rejected=0 failed=0` et
  `scanned=20 applied=20 disabled=0 failed=0` pour les memory patches.

Les gates restants sont la validation visuelle des familles représentatives et
la matrice souris/manette/contextes; le cold start ne les remplace pas.

## Correction 2.0.1 du 27 juillet 2026

La validation visuelle de Vincent a confirmé la plage de défense de base, mais
a invalidé toutes les plages d'affixes. Le témoin runtime sur
`Expert's Great Maul of Slaughter` a isolé la cause sans approximation :
`ItemData` porte les IDs compilés `mp=1275` et `ms=201`, tandis que le catalogue
brut les associait aux lignes `1277` et `202`. Le compilateur TXT ne compte pas
la ligne nommée `Expansion` de `magicprefix.txt` et `magicsuffix.txt`; le lecteur
du plugin la comptait et décalait chaque affixe suivant. Le garde-fou de roll
refusait ensuite correctement d'afficher une plage incohérente.

La version 2.0.1 omet uniquement ce séparateur lors de la construction des IDs,
conserve toutes les lignes vides qui occupent réellement un record et ajoute un
test reprenant exactement le maul : le suffixe `of Slaughter`, roll 18, produit
`[15 - 20]`, tandis que le préfixe fixe `Expert's +1` demeure sans plage. La
trace temporaire ayant fourni la preuve a été retirée du binaire final.

La couleur des plages passe du vert set-item `2` au vert étendu sombre `U` avec
le marqueur privé D2R, puis restaure la couleur native de l'affixe. Le build
Release et les deux suites de tests passent. La DLL source, globale et mod-locale
porte le SHA-256
`9B461284B7A964EC51E68C023972C0D3343836682B15E4AABEDCDD26756136CA`.
Le cold start BKVince charge Advanced Item Tooltips 2.0.1 et Transmogrify 1.3.1,
neutralise les doublons globaux attendus et termine avec
`active=25 rejected=0 failed=0` ainsi que `applied=20 failed=0` pour les patches.
Le nouveau rendu sombre et la plage d'un affixe post-`Expansion` restent à
confirmer visuellement par Vincent.

## Correction 2.0.2 du 27 juillet 2026

La validation visuelle suivante confirme que la correction d'indexation 2.0.1
fonctionne : `Expert's Great Maul of Slaughter` affiche désormais exactement
`[15 - 20]`. Elle invalide toutefois le code couleur étendu `U`, rendu cyan dans
D2R 3.2 et non vert sombre.

La version 2.0.2 utilise le code `:` que la source primaire de SlashDiablo
déclare explicitement `DARK_GREEN` (palette `0x76`), encodé avec le marqueur
privé D2R `EE 81 BE 3A`. Le code de restauration de la couleur native de
l'affixe demeure inchangé. Le build Release et les deux suites de tests passent.
La DLL source, gouvernée, globale et mod-locale porte le SHA-256
`FA473D7442915C1F0B57B4FD6F5E8A1B470E0F67EF142476D33205145CCE1B7E`.

Le cold start BKVince charge Advanced Item Tooltips 2.0.2 et Transmogrify 1.3.1,
neutralise les deux doublons globaux attendus et termine avec
`scanned=27 active=25 disabled=2 rejected=0 failed=0` ainsi que
`scanned=20 applied=20 disabled=0 failed=0` pour les patches. Seule la
confirmation visuelle finale du vert sombre reste ouverte avant de poursuivre
la matrice runtime.

## Correction 2.0.3 du 27 juillet 2026

Vincent confirme visuellement que le code SlashDiablo `DARK_GREEN` `:` produit
la couleur souhaitée. Le même test révèle une régression distincte sur les rares :
les lignes annotées ou situées au-dessus d'une annotation peuvent perdre leur
bleu natif et être rendues en blanc.

La cause est l'état de couleur hérité du renderer. D2R stocke et traite le
tooltip final du bas vers le haut; une seule ligne peut porter le marqueur bleu
et les lignes suivantes l'héritent. Le lecteur reconnaissait seulement le
marqueur privé `EE 81 BE`, pas les encodages natifs `FF 63` et `C3 BF 63`, et
réinitialisait chaque ligne sans marqueur explicite au blanc.

La version 2.0.3 reconnaît les trois encodages et propage l'état de couleur dans
l'ordre réel du buffer. Un test reproduit le rare montré par Vincent : marqueur
bleu sur `+32 to Attack Rating against Undead`, puis lignes héritées `+51%
Damage to Undead`, `+1 to Maximum Damage` et `+19% Enhanced Damage`. Toutes les
plages restaurent maintenant le bleu, y compris après les lignes sans marqueur,
tandis que le vert sombre validé reste inchangé.

Le build Release et les deux suites de tests passent. Les DLL build, gouvernée,
globale et mod-locale sont byte-identiques avec le SHA-256
`7E02214E0D9BF13F273AE963CA29D2AC59E21A2D0FCDCB2D4200EFBFCEB07254`.
Le cold start BKVince charge Advanced Item Tooltips 2.0.3 et Transmogrify 1.3.1,
neutralise les deux doublons globaux attendus et termine avec
`scanned=27 active=25 disabled=2 rejected=0 failed=0` ainsi que
`scanned=20 applied=20 disabled=0 failed=0` pour les patches. Le retest visuel
du rare reste ouvert; les validations magic et couleur sombre demeurent acquises.

## Correction 2.0.4 et audit exhaustif des uniques du 27 juillet 2026

Vincent confirme que la correction 2.0.3 conserve maintenant le bleu natif du
rare. Le test visuel suivant sur `The Gnasher`, unique `*ID=0`, ne montre
toutefois aucune plage pour son `+199% Enhanced Damage`, attendu à
`[180 - 200]` selon le `uniqueitems.txt` du mod chargé.

La cause est une collision entre un vrai ID zéro et les lignes de section. Le
lecteur convertissait un `*ID` vide en zéro; chacune des six lignes sans ID
(`Expansion`, `Armor`, `Elite Uniques`, `Rings`, `Class Specific` et
`Warlock Class Pack`) remplaçait donc le record de The Gnasher. Le même défaut
pouvait vider `Civerb's Ward`, premier record de `setitems.txt`.

La version 2.0.4 ignore uniquement les lignes dont `*ID` est vide, refuse un ID
malformé et conserve explicitement le véritable ID zéro. La régression exacte
de The Gnasher exige désormais `+199% Enhanced Damage [180 - 200]` et vérifie
qu'une propriété fixe comme Increased Attack Speed ne reçoit aucune plage.

Le contrôle ne se limite pas à ce témoin : le test Release audite les 502 IDs
explicites de `uniqueitems.txt`, leurs six sections vides, l'absence de doublon
et le maximum 506 avec ses cinq gaps gouvernés. Pour chacun des 490 records
portant une propriété scalaire décodable, le candidat résolu est comparé clé par
clé et borne par borne au TXT; 367 de ces records possèdent au moins un roll
variable. Les 12 autres sont des placeholders sans propriété décodable, sauf
`Titan's Echo` qui ne porte que le paramètre multi-valeur fixe `splash`.

Les deux suites de tests passent. Les DLL build, gouvernée, globale et mod-locale
sont byte-identiques avec le SHA-256
`63AE116734611DFD42D6F7AC5F2BF7A6BDDA1F9D4DFBA63C3C548A8FD3CB5F1B`.
Le cold start BKVince charge Advanced Item Tooltips 2.0.4 et Transmogrify 1.3.1,
neutralise les deux doublons globaux attendus et termine avec
`scanned=27 active=25 disabled=2 rejected=0 failed=0` ainsi que
`scanned=20 applied=20 disabled=0 failed=0` pour les patches. L'audit statique
de tous les uniques est acquis; le rendu de The Gnasher 2.0.4 reste à confirmer
visuellement, puis le gate crafted demeure ouvert.

## Extension 2.1.0 aux runewords du 27 juillet 2026

La version 2.1.0 étend le calcul aux 113 lignes actives de `runes.txt`, sans
liste de runewords codée en dur. L'identité est prise sur l'objet concret : le
flag runeword `0x04000000` de `ItemData+0x18` déclenche le résolveur natif
`ITEMS_GetRunesTxtRecordFromItem` au RVA `0x372260`; l'identifiant de chaîne du
record à `+0x46` est comparé aux clés `Name` du TXT par les résolveurs de langue
du mod actif. Les trois entrées natives sont verrouillées par signatures 92777.

Le champ `ItemData+0x48`, qui porte normalement `magicPrefix[0]`, contient
l'identité du runeword lorsque ce flag est actif. Il est donc toujours exclu de
la lecture des affixes, même si la localisation du runeword échoue. Cette règle
fail-closed empêche un identifiant de chaîne d'être interprété comme un préfixe.

Pour chaque runeword résolu, le catalogue additionne les propriétés T1 du
`runes.txt` et les bonus propres à chacune de ses runes dans `gems.txt`. Le
groupe weapon, armor ou shield est choisi depuis la hiérarchie effective de
`itemtypes.txt`. Les propriétés composites `res-all` et `all-stats` sont
décomposées vers leurs statistiques constitutives lorsque des bonus individuels
s'y ajoutent, tout en conservant leur ligne groupée lorsqu'elle existe.

Le test de double empilement utilise Call to Arms : son Enhanced Damage
`200-240` s'ajoute au `+50%` fixe d'Ohm et doit produire `[250 - 290]`. Ancients'
Pledge vérifie séparément les quatre résistances finales de shield
(`48/48/43/48`) et confirme qu'aucun suffixe n'est ajouté puisqu'elles sont
fixes. Une aura variable de Crescent Moon vérifie `[14 - 16]`; Bone vérifie le
FCR `15-20` et l'exclusion du couple chance/niveau de `gethit-skill`.

Les couples T1 qui ne représentent pas un roll scalaire sont omis : chance et
niveau des skills déclenchés, minimum/maximum des dégâts élémentaires, charges
et niveau des charged skills, ainsi que les formules dépendantes du niveau ou
du temps non reconstructibles exactement. Les propriétés fixes restent
affichées sans plage. L'audit Release couvre l'unicité et le chargement des 113
clés actives en plus des 502 IDs uniques déjà gouvernés; les deux suites passent.

Les DLL build, gouvernée, globale et mod-locale sont byte-identiques avec le
SHA-256
`C990BC272D2E889846BDB013B053FECFA6DD2AD4954C7182025ACFD1822BFEC2`.
Le cold start BKVince charge Advanced Item Tooltips 2.1.0 et Transmogrify 1.3.1,
neutralise les deux doublons globaux attendus, atteint `24/24` et termine avec
`scanned=27 active=25 disabled=2 rejected=0 failed=0` ainsi que
`scanned=20 applied=20 disabled=0 failed=0` pour les patches. Le gate restant
est visuel : confirmer au moins un runeword variable, un fixe et un cas agrégé
tel que Call to Arms dans le tooltip final.

## Correctif 2.1.1 des cumuls crafted du 27 juillet 2026

Le calcul des crafts ne choisit plus une variante recette par recette pour
chaque ligne indépendante. Toutes les statistiques modélisées du tooltip sont
d'abord confrontées à chaque candidat issu de `cubemain.txt`; une recette est
écartée si elle ne contient pas une ligne reconnue ou si sa plage ne peut pas
produire la valeur affichée. Les plages ligne par ligne ne sont calculées
qu'après cette sélection globale et demeurent omises si les candidats survivants
ne s'accordent pas exactement.

Ce changement couvre notamment les douze recettes d'amulette BKVince. Le FCR
`15` identifie les variantes Caster, tandis que `Regenerate Mana 5%` et
`+3% to Experience Gained` distinguent le craft Caster standard de l'Ascended
Caster, qui donne lui aussi `5-10% FCR` mais pas `10-20 Mana`. Sur une amulette
portant le préfixe Great Wyrm's `61-90 Mana`, le cumul standard produit donc
exactement `+83 to Mana [71 - 110]` et conserve `FCR [15 - 20]`,
`Regenerate Mana [4 - 10]` et `Experience Gained [1 - 5]`.

Une régression synthétique reproduit l'ambiguïté `61-90` contre `71-110`; une
seconde charge les vrais TXT BKVince et vérifie l'objet complet. Les deux suites
Release passent, puis l'audit étendu des 502 uniques et des 113 runewords sort
avec le code 0. Les DLL build, gouvernée, globale et mod-locale sont
byte-identiques avec le SHA-256
`B5AC7A1672E0FAB195B90369FC25D86693FCD49920BEDD028936F7FF1604327F`.
Le cold start BKVince charge Advanced Item Tooltips 2.1.1, atteint `24/24` et
termine avec `scanned=27 active=25 disabled=2 rejected=0 failed=0` ainsi que
`scanned=20 applied=20 disabled=0 failed=0` pour les patches. Le gate restant
est la confirmation visuelle de `[71 - 110]` sur l'amulette crafted témoin.

## Révision 2.1.2 de la règle des objets socketés du 27 juillet 2026

À la demande de Vincent, la présence de sockets ne supprime plus la ligne
`Max Sockets`. L'export ne lit plus la stat socket courante pour décider de
l'affichage : toute capacité non nulle produite par `ITEMS_GetMaxSockets`
génère désormais la ligne blanche, y compris lorsque le tooltip contient déjà
`Socketed (N)`. Une régression exige explicitement `Max Sockets: 3` avec trois
sockets courants.

Les deux suites Release et le test sur le catalogue BKVince passent. Les DLL
build, gouvernée, globale et mod-locale sont byte-identiques avec le SHA-256
`7E0BA479C99EAEDBB5A13EA5E25AE2867B198E301F3DC6E41E803891C8D9953A`.
Le cold start charge Advanced Item Tooltips 2.1.2, neutralise le doublon global
attendu, atteint `24/24` et termine avec
`scanned=27 active=25 disabled=2 rejected=0 failed=0` ainsi que
`scanned=20 applied=20 disabled=0 failed=0`. Le gate restant est visuel :
confirmer simultanément `Max Sockets` et les plages sur un rare ou crafted déjà
socketé.

## Correctif 2.1.3 des noms rares et crafted du 27 juillet 2026

Le témoin runtime `Stone Razor` a conservé tous ses IDs après socketing :
`magicPrefix=1389,980,1349` et `magicSuffix=745,742,195`. L'instabilité venait
des deux champs séparés `rarePrefix=172` et `rareSuffix=7`, utilisés uniquement
pour composer le nom généré. Le catalogue les interprétait aussi comme des
records de `magicprefix.txt` et `magicsuffix.txt`. Par collision, l'ID de nom
172 devenait le préfixe statistique `Forked` et ajoutait fictivement
`+9-10 Maximum Damage` au vrai `+1`; le filtre rejetait alors le candidat entier.

La version 2.1.3 ne calcule plus les propriétés qu'à partir des trois slots
`magicPrefix`, des trois slots `magicSuffix`, de l'automagic et des sources
unique, set, craft ou runeword appropriées. Les mots du nom rare/crafted ne
participent jamais aux statistiques. Une régression construite avec les six IDs
exacts du `Stone Razor`, ses dégâts élémentaires, `Socketed (2)` et les deux IDs
de nom exige désormais `Enhanced Damage [10 - 20]`,
`Damage to Undead [25 - 75]` et
`Attack Rating against Undead [25 - 75]`.

Les deux suites Release et le catalogue BKVince passent. Les DLL build,
gouvernée, globale et mod-locale sont byte-identiques avec le SHA-256
`525AEB0103210E79BBBFFD8E62BA487C62DEF036A3C8553B24C9C791D7BB8A17`.
Le cold start charge Advanced Item Tooltips 2.1.3, atteint `24/24` et termine
avec `scanned=27 active=25 disabled=2 rejected=0 failed=0` ainsi que
`scanned=20 applied=20 disabled=0 failed=0`. Le gate restant est la confirmation
visuelle du `Stone Razor` corrigé.

## Correctifs 2.1.4-2.1.8 des runewords du 28 juillet 2026

Le catalogue BKVince calculait déjà correctement Stone sur une torso armor :
le `350-400% Enhanced Defense` du runeword s'additionne au `+50%` fixe de Pul
pour produire `[400 - 450]`. Les deux statistiques localisées sous le même
libellé Strength restent distinctes et donnent respectivement `[15 - 20]` et
`[10 - 15]`; Vitality donne `[15 - 20]`.

La 2.1.4 a d'abord durci le résolveur runtime avec un repli strict sur le titre
localisé déjà rendu. Le retest Stone a prouvé que cette correction était
insuffisante. La cause réelle se trouvait dans les champs d'affixes : seul
`magicPrefix[0]`, explicitement réutilisé pour l'identité du runeword, était
ignoré. Or un objet blanc devenu runeword ne possède aucun affixe magique et
les cinq autres emplacements peuvent eux aussi contenir un payload runtime.
Leur décodage produisait des plages fictives et faisait rejeter le candidat
Stone entier.

La version 2.1.5 ignore donc les six slots `magicPrefix/magicSuffix` lorsqu'un
item porte `IFLAG_RUNEWORD`. L'automagic propre à la base demeure traité
séparément. Le repli de titre 2.1.4 reste fail-closed : il ne s'active que pour
un item marqué runeword, exige une ligne de titre exacte (avec le suffixe
optionnel de niveau d'item) et n'utilise que les noms localisés uniques.

Le second retest Stone a isolé une collision distincte dans la validation du
tooltip complet : `Required Strength: 110` satisfaisait l'ancre textuelle du
modifier Strength, puis sa valeur 110 hors de `[15 - 20]` faisait rejeter le
candidat entier. La 2.1.6 exclut explicitement les lignes de métadonnées
`Required ...` du matching des propriétés. La régression Stone contient
désormais les lignes réelles Defense, Max Sockets, Durability, Required
Strength et Required Level, en plus de tous ses modifiers.

La 2.1.7 généralise ce gate aux métadonnées structurelles : Defense, dommages
one-hand/two-hand/throw, Durability, Chance to Block, Required, Item/Affix
Level, Base Defense, Max Sockets, Socketed, coût et quantité. Un audit parcourt
les 113 runewords actifs dans les trois catégories weapon, torso armor et
shield. Pour chaque plage variable décodée, il compare le résultat avec et sans
chaque ligne structurelle; plus de 500 comparaisons sont exécutées. Cet audit a
notamment détecté puis fermé une collision indépendante avec `Base Defense`.

Le test visuel Dream helmet a validé FHR, résistances et Magic Find, mais a
révélé que le gate `Defense` ne distinguait pas l'en-tête `Defense: 254` du
modifier signé `+199 Defense`; la normalisation textuelle réduisait les deux à
`defense`. La 2.1.8 réserve désormais l'exclusion à la ligne non signée. Une
régression complète Dream Crown exige `+199 Defense [150 - 220]` tout en
laissant le `+50% Enhanced Defense` fixe de Pul sans plage. Spirit shield a
simultanément confirmé visuellement Mana `[89 - 112]` et Magic Absorb `[3 - 8]`.

Une régression reproduit le Stone de la capture sur Gothic Plate, injecte six
IDs d'affixes parasites et exige les quatre plages ED, Strength, Strength
percent et Vitality. Les deux suites
Release et l'audit du catalogue BKVince passent. Les DLL build, gouvernée,
globale et mod-locale sont byte-identiques avec le SHA-256
`0C23F9370563F8C54F81ADF7F200948A93C05DE93A9459457E043205D4003D4B`.
Le cold start BKVince charge Advanced Item Tooltips 2.1.8. Les validations en
jeu sont acquises pour Stone torso armor (`[400 - 450]`, les deux Strength et
Vitality), Spirit shield (Mana et Magic Absorb) et Dream helmet (`+199 Defense
[150 - 220]`, FHR, résistances et Magic Find). Le prochain domaine distinct à
étudier est l'affichage des staffmods hardcodés, qui ne proviennent pas des
affixes et propriétés TXT déjà couverts.

## Extension 2.1.9 aux auto-affixes et objets superior du 28 juillet 2026

La capture `Sup. Crown` ne provenait pas d'`automagic.txt`, mais du record 8 de
`qualityitems.txt`. Le champ `fileIndex` d'un objet de qualité 3 sélectionne ce
record, qui fournit `ac% 5-25` et `red-dmg 1-2`. Le catalogue charge désormais
les records superior dans leur ordre physique compilé et exige sur le témoin
`+5% Enhanced Defense [5 - 25]` et `Damage Reduced by 2 [1 - 2]`.

Le chemin distinct `automagic.txt` reste actif et reçoit une régression dédiée.
Le test calcule l'ID runtime unifié suffixes + prefixes + automagic depuis les
tables BKVince chargées, résout `Armor_fhr` sur une Quilted Armor et exige
`Faster Hit Recovery [5 - 10]`. Les deux suites Release passent. Les DLL build,
gouvernée, globale et mod-locale sont byte-identiques avec le SHA-256
`95A39443D933560C1A518B13E8DD9752CF5B445FE78C3DB6FA2A81EBC1D3BB74`.
Le cold start BKVince charge Advanced Item Tooltips 2.1.9; le gate restant est
la confirmation visuelle du Superior Crown.

## Correctifs 2.1.10-2.1.12 des superior, sets et socket fillers du 28 juillet 2026

Les captures runtime ont prouvé que `qualityitems.txt` ne peut pas recevoir un
offset global : un Axe porte `fileIndex=0`, un Leather Armor ED 47 porte
`fileIndex=2`, tandis que les captures Crown et Scale Mail exigent aussi
l'interprétation voisine. Le catalogue conserve donc les deux interprétations
`fileIndex` et `fileIndex+1` comme candidats, puis le tooltip complet ne retient
que celle capable de reproduire toutes les propriétés visibles. La régression
exacte du Leather Armor ancien, avec `+47% Enhanced Defense`, automagic
`Increase Maximum Mana 4%` et Ethereal, exige respectivement `[5 - 50]` et
`[1 - 8]`; aucun respawn de l'objet n'est nécessaire puisque le calcul se fait
à l'affichage.

La validation d'un set complet a révélé qu'une ligne ajoutée par une source
externe au record de l'objet pouvait éliminer le candidat entier. Une ligne
n'est désormais utilisée comme preuve de provenance que si au moins un
candidat sait reproduire sa valeur finale. Les bonus de set complet et les
propriétés encore non modélisées d'un socket filler ne suppriment plus les
plages indépendantes. Vincent a confirmé visuellement les deux armes complètes
de Bul-Kathos avec leurs plages ED, attribut et résistances intactes.

La 2.1.12 modélise ensuite les fillers au lieu de se contenter de tolérer leurs
effets. Les fonctions 92777 gouvernées `UNITS_GetInventory` (`0x34A360`),
`INVENTORY_GetFirstItem` (`0x388C10`) et `INVENTORY_GetNextItem` (`0x38ABA0`)
parcourent au plus six objets du socket inventory sous signatures strictes. Les
affixes magic, rare et unique de chaque jewel sont résolus puis fusionnés avec
le candidat parent avant le consensus. Les gems et runes libres utilisent les
colonnes weapon, helm/armor ou shield de `gems.txt` selon la hiérarchie réelle
du parent. Un runeword est exclu de ce second parcours, car ses runes sont déjà
additionnées depuis `runes.txt` et `gems.txt`; cette garde interdit tout double
comptage.

Le témoin table-driven reproduit exactement Stone Razor avec un rare jewel
portant Ruby, Vermillion, of Burning et of Hope. Il exige ED `[41 - 60]`,
Maximum Damage `[12 - 16]`, Fire Damage `[11 - 25] to [31 - 50]`, Life
`[9 - 20]`, tout en conservant les deux plages Undead `[25 - 75]`. Des tests
distincts couvrent Ohm, Ral, Perfect Ruby, Um sur armor et shield, ainsi que
plusieurs fillers cumulés. Les lignes `Adds X-Y Damage` comparent désormais les
deux valeurs visibles séparément; les calculs de poison dépendants de la durée
restent volontairement fail-closed.

Les deux suites Debug passent contre les TXT BKVince actifs. Les DLL build,
gouvernée, globale et mod-locale sont byte-identiques avec le SHA-256
`D40306836B61791969A826073DD00BBFD2AF63873DD0195E619CF7A80D7E704C`.
Le cold start BKVince charge Advanced Item Tooltips 2.1.12 mod-local, neutralise
le doublon global attendu, atteint `24/24` et termine avec
`scanned=29 active=27 disabled=2 rejected=0 failed=0` ainsi que
`scanned=20 applied=20 disabled=0 failed=0` pour les patches. Le gate encore
ouvert est le témoin visuel sur le Stone Razor déjà socketé et sur le Superior
Leather Armor existant; le cold start et les tests statiques ne les remplacent
pas.

Vincent a fermé ces derniers gates visuels le 28 juillet 2026. Le Stone Razor
déjà socketé affiche ED `[41 - 60]`, Maximum Damage `[12 - 16]`, Fire Damage
`[11 - 25] to [31 - 50]` et Life `[9 - 20]`, tout en conservant les deux
plages Undead `[25 - 75]`. Le Superior Leather Armor éthéré existant affiche
Enhanced Defense `[5 - 50]` et Maximum Mana `[1 - 8]`. Le Superior Axe éthéré
portant `fileIndex=0` affiche Target Defense `[5 - 30]`. Ces observations
valident en jeu la matrice personnelle BKVince 2.1.12 pour les cumuls
parent + jewel, les dégâts composés, les objets superior, automagic et
éthérés. Aucun respawn n'a été nécessaire.

Ce jalon ne transforme pas en succès implicite les domaines volontairement
hors matrice : les staffmods hardcodés et les calculs poison dépendants de la
durée restent fail-closed. La compatibilité publique avec les tables d'autres
mods demeure un gate de publication séparé de cette validation BKVince.

Un dernier témoin Stone Razor socketé ferme aussi le cas composé des dégâts
physiques plats. Un jewel portant simultanément Minimum Damage et Maximum
Damage produit une seule ligne native `Adds 9-19 Damage`, mais le plugin
conserve deux composantes distinctes : `[5 - 10] to [18 - 25]`. La borne
Maximum Damage inclut correctement le `+1` déjà porté par l'arme. Sur le même
objet, ED `[41 - 60]`, Fire Damage `[22 - 50] to [62 - 100]`, Life `[9 - 20]`,
All Resistances `[5 - 10]` et les deux plages Undead `[25 - 75]` demeurent
indépendants. Cette observation confirme en jeu que la fusion parent + jewel
n'écrase ni ne mélange les composantes minimum et maximum d'une propriété
compound.

## Couleur blanche des propriétés de runes du 28 juillet 2026

Le tooltip Ohm a révélé une alternance bleue/blanche qui ne provenait ni de la
DLL ni du moteur de plages. Les quatre chaînes BKVince `GemXp1` à `GemXp4`
réinitialisaient chaque en-tête en blanc avec `ÿc0`, puis forçaient les
propriétés suivantes en bleu avec `ÿc3`. Les catalogues courant et legacy
conservent maintenant `ÿc0` après `Helms:`, `Shields:`, `Weapons:` et `Armor:`
pour que tout le bloc de propriétés de la rune demeure blanc en anglais. Les
autres locales et les couleurs normales des affixes d'items ne sont pas
modifiées.

Les deux JSON source et runtime sont valides et byte-identiques : SHA-256
`04B065254D4F5199C46E95A8767DCABDF33E1D1FD2096413E24E98CB34586706` pour
`strings/item-gems.json` et
`EAC8446709FE4494A504D00B33D0562FE61BAE3DD5AFDF38B43F03A3461ABC75` pour
`strings-legacy/item-gems.json`. Le cold start BKVince recharge les tables de
chaînes, atteint `24/24` et termine avec
`scanned=29 active=27 disabled=2 rejected=0 failed=0` ainsi que
`scanned=20 applied=20 disabled=0 failed=0` pour les patches. Le témoin visuel
Ohm en blanc reste à confirmer en jeu.

## Pipeline autonome 2.2.0 du 30 juillet 2026

Advanced Item Tooltips ne dépend plus de Transmogrify ni d'une DLL du
PluginPack pour atteindre le tooltip final. L'atelier persistant 92777 confirme
sept xrefs directs vers `ITEMS_BuildItemTooltip` (`0x2BD480`) aux call-sites
`0x2291DC`, `0x2BCEE9`, `0x2C8C02`, `0x2CB32E`, `0x2CE716`, `0x87E882` et
`0x150C377`. La 2.2.0 valide les cinq octets exacts de chaque `CALL rel32`, les
redirige vers un relais privé proche et appelle d'abord le constructeur vivant
de D2R avant d'appliquer la transformation idempotente des ranges et de
`Max Sockets`. Le prologue strict du constructeur reste intact : un autre
plugin peut donc le hooker sans collision avec Advanced Item Tooltips.

Le binaire n'importe et ne résout ni Transmogrify, ni ExtendedItemStats, ni
`plugin-items.dll`, ni aucune autre DLL du pack. `dumpbin /dependents` ne montre
que les runtimes Windows/MSVC; les six exports publics historiques demeurent
présents. Les builds Release et Debug passent chacun les deux suites de tests.
Le SHA-256 Release autonome est
`8970908514F62B98F41205FDD3601B2B305B1D363AB1FCD32C6044751036863B`.

Un cold start avec Advanced Item Tooltips comme seule DLL active charge la
2.2.0 avec `7/7 call-sites`. Un second cold start avec seulement
AdvancedItemTooltips et Transmogrify conserve les deux propriétaires sans
collision : Advanced Item Tooltips possède ses call-sites et Transmogrify
possède le prologue natif. Avant cette isolation, Vincent a aussi confirmé en
jeu les ranges et `Max Sockets` avec Transmogrify absent et la DLL PluginPack de
travail encore présente. La matrice prouve donc l'autonomie fonctionnelle et la
coexistence sans transformer le PluginPack en dépendance.

Le build `plugin-items.dll` alors en cours de test tentait indépendamment de
reprendre le prologue déjà possédé par Transmogrify. Ce témoin a été exclu de la
matrice finale, puis les cinq DLL de travail ont été sauvegardées byte-exact
avant l'isolation. Ce conflit appartient au chantier PluginPack en cours et
n'est ni causé ni masqué par Advanced Item Tooltips.
Après l'isolation, les cinq DLL PluginPack ont été restaurées byte-identiques
dans le profil BKVince et le jeu a été laissé fermé pour la reprise de leurs
propres tests.
