# Advanced Item Tooltips — D2R 3.2.92777

## Décision produit

La première phase du projet se concentre sur l'indication `Max Sockets: N` pour
les objets socketables. La phase suivante, approuvée par Vincent le 27 juillet
2026, ajoute les plages de rolls des propriétés et de la défense sans modifier
le comportement acquis de `Max Sockets`. L'affichage
`Sockets: courant / maximum` demeure hors périmètre.

La ligne utilise le marqueur UTF-8 privé du renderer D2R pour le blanc vanilla.
Sur une weapon, elle apparaît immédiatement sous le bloc de dommages; sur une
armor, elle apparaît immédiatement sous la ligne de défense. Un item qui porte
déjà des sockets — notamment une base naturellement affichée `Socketed (N)` —
n'affiche aucune ligne `Max Sockets` supplémentaire.

## Implantation

Le plugin hybride `AdvancedItemTooltips.dll` 2.0.0 est attribué à `RuffnecKk`
et ne déclare pas `ModScopedOnly`. Il accepte uniquement le build 92777.

- `ITEMS_GetMaxSockets`, RVA `0x36EAD0`, calcule la capacité de l'objet concret,
  incluant son niveau, sa base et les limites de type. Le plugin ne duplique pas
  cette logique.
- `STATLIST_GetUnitStat`, RVA `0x2F5020`, lit la stat socket `194`; une valeur
  positive supprime entièrement l'ajout.
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
- item déjà socketé : ligne absente;
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
