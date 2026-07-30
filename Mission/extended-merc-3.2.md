# ExtendedMerc — D2R 3.2.92777

Dernière mise à jour : 30 juillet 2026

Statut : incubation native active. Vincent a choisi le nom `ExtendedMerc`, la
voie du plugin autonome permanent et le séquencement A. Le trajet réseau, le
callback serveur, l'adoption d'un slot existant et les primitives natives de
création/attachement UI sont prouvés. Le probe 0.0.22 remplace la condition
expérimentale sur la classe d'objet par un masque fail-closed de BodyLocs : seul
`gloves=10` était activé pendant la matrice, et le BodyLoc natif de l'objet
restait exigé. L'équipement et le retrait ont réussi sans crash sur les
mercenaires des Actes I, II, III et V; l'Acte II a aussi conservé le glove et le
mercenaire après `Save and Exit` puis rechargement. La politique générique et la
persistance témoin sont donc démontrées. Restent à implanter la configuration
autonome des six slots, `boots=9`, la désactivation sûre, la navigation manette
et les matrices de portée et de multijoueur avant une release publique.

## Décisions confirmées

- Le plugin ne sera pas fusionné au PluginPack et ne reçoit donc ni catégorie,
  ni DLL propriétaire future, ni clé dans `D2RPlugins.json`.
- La future DLL est `ExtendedMerc.dll`, attribuée exactement à `RuffnecKk`.
  Elle devra être hybride : installable globalement ou dans un mod, sans
  `ModScopedOnly`.
- Son objectif joueur est : `Enables selected extra equipment slots for mercenaries.`
- Sa configuration restera indépendante du PluginPack. Le format JSON ou TOML
  sera retenu seulement après comparaison de la convivialité réelle pour un
  moddeur; son contenu et ses commentaires seront entièrement en anglais.
- Aucune DLL d'eezstreet ne sera modifiée, liée ou redistribuée. La coexistence
  avec les cinq DLL officielles est une gate du plugin autonome.

## Résultat produit attendu

Un moddeur active indépendamment les emplacements supplémentaires du
mercenaire : `amulet`, `rightRing`, `leftRing`, `belt`, `gloves` et `boots`.
Le comportement vanilla reste la valeur par défaut.

L'activation doit appliquer les règles d'équipement et présenter les slots sans
demander de modifier `itemtypes.txt`, `inventory.txt` ou les layouts Hireling.
Un layout déjà personnalisé pourra être adopté au runtime plutôt qu'écrasé.

## Faits vérifiés

- BKVince répartit aujourd'hui cette fonctionnalité entre la hiérarchie de
  types d'objets, les coordonnées d'inventaire et plusieurs layouts Hireling.
  Les widgets clavier et controller ne sont pas identiques, et les gants/bottes
  ne sont pas actuellement exposés par le layout BKVince.
- Le précédent D2Mod MercMod confirme le principe d'une politique native
  configurable par type de mercenaire, mais exigeait encore des modifications
  de panneau et d'inventaire : il ne satisfait pas le contrat d'ExtendedMerc.
- D2MOO 1.10f fournit une preuve sémantique que l'équipement du mercenaire passe
  par les BodyLocs et les transactions d'inventaire natives. Ce n'est ni une
  adresse, ni une structure, ni une ABI transposable vers D2R 3.2.92777.
- L'atelier D2R 3.2.92777 et la référence PluginPack épinglée ont été vérifiés.
  Le paquet client, le callback serveur, la hiérarchie runtime du panneau
  Hireling, le champ `isHireable`, la factory `InventorySlotWidget` et l'ajout
  natif d'un enfant UI sont maintenant identifiés sur le build 92777.

## Audit d'incubation — 29 juillet 2026

### Preuves obtenues

- `npm.cmd run re:d2r32 -- status` confirme l'image, l'index et les références
  du build 92777. D2MOO demeure une preuve sémantique utile, mais aucune de ses
  adresses, structures ou ABI n'est transférable.
- Le retrait moderne d'un objet équipé est maintenant localisé : le client
  émet le paquet `0x58` et le serveur le traite à `D2R.exe+0x4B4780`. Ce flux
  prouve une transaction d'inventaire native et sérialisée, mais il ne prouve
  pas l'équipement d'un objet sur un mercenaire.
- Le petit constructeur client à `D2R.exe+0xEC6B0` émet bien trois octets,
  sans appel observé avec l'opcode `0x61`. Un autre flux D2R observé commençant
  par `0x61` utilise `opcode | sous-opcode | longueur | payload variable`.
  Le paquet D2MOO legacy de trois octets ne doit donc pas être reproduit.
- `D2R.exe+0x4B6160` est le callback serveur de résurrection, pas celui du
  mercenaire. Le candidat `0x4AD650` rejette les tailles inférieures ou égales
  à trois et est exclu du chemin recherché.
- Les layouts BKVince confirment que les anneaux et l'amulette disposent déjà
  de widgets déclarés mais avec `isHireable: false`; gants et bottes n'ont pas
  de widget. Cette source reste uniquement un fixture de test : ExtendedMerc
  ne distribuera et ne remplacera aucun de ces layouts.
- L'audit du PluginPack épinglé relève 132 plages de hook sans chevauchement
  entre les cinq DLL. Aucune fonctionnalité mercenaire n'y est trouvée, mais
  ExtendedMerc devra exclure notamment `0x373890`, `0x2A7810`, `0x2A89C0`,
  `0x2BD480`, `0x2CACF0`, `0x0EE2A0`, `0x843D90` et `0x448C00`.
- Une trace runtime fraîche sur `QtyTester` a capturé quatre gestes vanilla au
  point d'entrée de la file client `0xEE2A0`. L'équipement du torse émet le
  paquet de 17 octets `0x51` sous la forme
  `{opcode, cursorItemId, mercId, equippedItemId, bodyLoc}`; le retrait inverse
  `cursorItemId` et `equippedItemId`. Les constructeurs observés sont les
  callsites `0x15C599` et `0x161875` vers le constructeur générique `0xEC7D0`.
- Le callback serveur exact est `0x4C0E20`, avec ABI
  `(game, player, packet, packetSize) -> int32`. Il exige `packetSize == 17`,
  résout le mercenaire, compare son identifiant avec le champ `packet+5`, lit
  les deux identifiants d'objet à `packet+1` et `packet+9`, puis transforme le
  `bodyLoc` de `packet+13` seulement lorsqu'il est inférieur à `11`. Son
  prologue strict de 29 octets est unique dans l'image 92777.
- Le succès autoritaire d'équipement est confirmé par une seconde sonde : le
  callback atteint `SUNIT_AttachSound 0x491960` depuis `0x4C1364` avec le son
  `0x5E`, le mercenaire comme unité et le joueur comme source. La stack remonte
  par le dispatcher indirect `0x4F2FA0`; CDB s'est ensuite détaché proprement.
- `0x34A330` retourne le `UnitId` de 32 bits stocké à `unit+0x08`, ou
  `0xFFFFFFFF` pour un pointeur nul. L'ancien nom gouverné
  `UNITS_GetItemCell` était trompeur : le callback l'appelle sur le mercenaire
  et compare le résultat au `mercId` du paquet.
- Une lecture non intrusive du panneau ouvert a relevé 42 enfants. Les slots
  vanilla `head`, `torso`, `right_arm` et `left_arm` sont `isHireable=1`; le
  `belt` BKVince l'est aussi. `slot_right_hand`, `slot_left_hand` et
  `slot_neck` existent déjà avec `isHireable=0`; `slot_gloves` et `slot_feet`
  sont absents. Tous les slots observés partagent le vtable
  `D2R.exe+0x1CF3F20`, le `location` est à `+0x5D8`, le rectangle local à
  `+0x70` et `isHireable` à `+0x638`.
- Le témoin d'adoption a modifié uniquement `slot_right_hand+0x638` sur
  l'instance runtime. Vincent a pu équiper un `ring`, puis a confirmé qu'il
  restait équipé. Cette observation prouve que l'adoption d'un widget existant
  suffit au trajet joueur constaté; elle ne remplace pas encore la matrice
  complète save/reload, host/joiner et désactivation.
- La tentative CDB suivante utilisait une condition invalide et a gelé le jeu
  au callback serveur. Cet incident est attribué à la sonde de diagnostic,
  pas à un hook ExtendedMerc ni au format de sauvegarde; CDB est exclu des
  validations suivantes.
- `UI_InventorySlotWidget_Factory` est prouvé à `0x2C9850`. Son ABI effective
  est `(typeDescriptorIgnored, const char* name, Widget* parent) -> Widget*` :
  elle alloue exactement `0x640` octets par l'allocateur UI, appelle le
  constructeur de base, pose le vtable `0x1CF3F20`, initialise la chaîne du
  fond et remet `isHireable` à zéro. Sa signature étendue de 37 octets est
  unique dans le `.text` 92777.
- Le slot se finalise par le vtable `+0x08`, soit `0x2CA970`; cette méthode
  charge le fond configuré puis délègue au finalizer de base. Les constructeurs
  natifs appellent ensuite `UI_Widget_AddChild 0x854DE0` avec ABI
  `(parent, child) -> void`. Cette fonction gère elle-même la croissance et
  l'ownership du tableau d'enfants à `parent+0x58/+0x60/+0x68`; son prologue
  strict de 18 octets est unique.
- Le champ hérité `cellSize` est un couple d'entiers à
  `InventoryItemWidget+0x5B8/+0x5BC`. Le constructeur `0x2A6FE0` l'initialise
  à zéro, puis les chemins de rendu `0x2A7574/0x2A75A3` et
  `0x2A7969/0x2A7971` consomment séparément ses deux composantes. Les layouts
  d'équipement 92777 renseignent tous ce champ; un slot créé au runtime doit
  donc le reprendre d'un slot hôte sain au lieu de supposer `98 × 98`.
- Le point post-dispatch `0xCE3B1` appartient au toggle d'interface natif et
  appelle `UI_DispatchMessage 0x843D90`; le retour est `0xCE3B6`. La signature
  de 23 octets englobant ce callsite est unique. Il permet une interception
  étroite après ouverture du panneau sans réclamer l'entrée globale du
  dispatcher déjà partagée par RemoteStash et le PluginPack.
- La référence PluginPack épinglée au commit `dc75b49` ne contient aucune
  référence textuelle à `InventorySlotWidget`, `HirelingInventoryPanel`,
  `0x2C9850`, `0x854DE0` ou `UI_FindChildWidgetByName`. L'absence de collision
  directe est donc établie pour la référence publique épinglée. Le manifeste
  exact de 132 sites aux checkpoints `4f8b276` et `5b56690` ne chevauche ni le
  callsite de production `0xCE3B1..0xCE3B5`, ni l'entrée jetable `0xCDE00`, ni
  les plages témoins de `0x2C9850`, `0x2CA970` et `0x854DE0`. Les autres
  plugins tiers restent gérés par validation stricte et refus fail-closed.
- Un probe jetable `ExtendedMercProbe 0.0.1` est compilé en Release uniquement
  sous `analysis-cache`. Il adopte les slots existants, cherche un rectangle
  sans collision d'équipement et ne crée que `slot_gloves` avec la séquence
  native prouvée. L'audit hors jeu l'a durci : il refuse désormais la création
  sans `cellSize` hôte sain, copie ce couple, en dérive le rectangle `2 × 2` et
  initialise aussi `PANEL\\gemsocket` avant le finalizer. Il ne détourne plus
  l'entrée complète `0xCDE00` : un relais local proche remplace uniquement le
  call `0xCE3B1`, délègue au dispatcher actuellement vivant puis agit après son
  retour. D2RLoader reste propriétaire de cette écriture et peut la refuser si
  un autre plugin possède déjà les cinq octets.
  Le premier démarrage l'a rejeté avant `D2RLoaderLoadPlugin` parce que la
  ressource-manifeste v2 manquait : aucun hook du probe n'a été installé. La
  ressource a été ajoutée et le build local corrigé portait le SHA-256
  `ECB258B6129816B09380272187F67228A0F2406693FFADA1DF526DBB97ADF668`.
- Le premier déploiement fonctionnel a ensuite échoué fail-closed :
  `PatchCallRel32` ne pouvait pas encoder le relais alloué hors de la portée
  signée de 32 bits du callsite `0xCE3B1`. Le probe a donc encodé lui-même les
  cinq octets `E8 rel32`, après validation stricte de portée, puis les a confiés
  à `PatchBytes` afin que D2RLoader demeure propriétaire de l'écriture. Le build
  0.0.1 résultant (`6A635750D6C2ABBE5A391CD97E16301DAEA7D39A6ADFEF300C2832049C1983AF`)
  a passé le cold start avec 20/20 patches et
  `active=15 disabled=2 rejected=0 failed=0`.
- Ce build a créé `slot_gloves bodyLoc=10 rect=110,564,196,196 cell=98,98`,
  mais le widget demeurait invisible parce que ses méthodes virtuelles
  d'activation et d'affichage n'avaient pas été appelées. Un clic à son
  emplacement a ensuite reproduit un crash : `UI_HandleEquippedItemClick
  0x2CACF0` a appelé `UI_TOOLTIP_ResolveHoveredUnit 0x2A7810`, lequel a lu les
  valeurs par défaut `unitId=-1` et `unitType=6` laissées par la factory et a
  retourné une unité nulle avant `UNITS_GetInventory`.
- Le probe 0.0.2 copie maintenant un contexte mercenaire sain depuis un slot
  hôte (`unitType=1`, `unitId>=0`) dans `+0x5C4/+0x5C8`, puis appelle les entrées
  virtuelles 9 et 10 pour activer et afficher le widget. Son build Release
  (`52A8F23343EC2958FA0A0112D0E13EB75FF7A3B76DC0E7DA00A91273BBB658EA`)
  a passé le même cold start. Sur `QtyTester`, le slot a été visuellement
  confirmé et le clic vide exact qui faisait planter le jeu n'a produit ni gel,
  ni nouveau rapport de crash; le PID 45596 répondait toujours après le clic.
  L'instance a ensuite été sauvegardée et fermée proprement, et la DLL probe a
  été retirée du runtime. Aucun glove libre n'était disponible dans l'inventaire,
  donc l'équipement réel n'a pas été simulé.
- Un second passage complet avec le même probe 0.0.2 et `QtyTester` en difficulté
  Pain a acheté chez Charsi `Strong Leather Gloves of Self-Repair (12)` pour
  124 gold. Le widget `slot_gloves` a bien accepté le ciblage et affiché son état
  rouge, mais la transaction native a été refusée : le glove est resté sur le
  curseur au lieu d'être équipé. Il a été remis dans l'inventaire sans perte.
  Le ring droit du mercenaire est resté équipé et inchangé. Après save/exit et
  reload, le glove persistait dans l'inventaire joueur, le slot mercenaire était
  vide et le ring toujours présent. Aucun gel ni nouveau rapport de crash n'a été
  produit. La sauvegarde finale mesure 2857 octets
  (`A7BEB417EB7311F6433629233FE6CFD80336D6804CD4CDACC97B7AF2C4D29270`),
  contre 2818 octets pour le backup préalable
  (`C56CB58751A00252AC7C33A0A9D70422E5C6A1B4C75D6DD4D4349A27ADC888DE`).
  Le probe testé a été retiré du runtime puis archivé localement avec son hash
  inchangé `52A8F23343EC2958FA0A0112D0E13EB75FF7A3B76DC0E7DA00A91273BBB658EA`.
- La branche mercenaire de `UI_HandleEquippedItemClick 0x2CACF0` est maintenant
  délimitée sans réclamer l'entrée déjà possédée par EquippedItemToCube. Elle
  appelle successivement le contrôle BodyLoc `0x36BB10` au callsite unique
  `0x2CB0D9`, la compatibilité mercenaire `0x159E60` à `0x2CB0E8`, puis le
  contrôle commun des exigences `0x36BC50` à `0x2CB10D`. Le succès construit
  ensuite la commande avec `{bodyLoc, true}` par `0x15C220`; chaque refus
  rejoint l'avertissement UI sans émettre cette commande.
- `0x36BB10` lit les deux BodyLoc compilés du type d'objet et accepte l'un ou
  l'autre. La table vanilla 3.2, lue en round-trip byte-exact, confirme que la
  ligne `Gloves` est l'ItemType `16`, hérite de `Armor` et porte `glov` dans
  `BodyLoc1/BodyLoc2`; le BodyLoc `10` du widget créé est donc cohérent. Le probe
  0.0.7 le confirme au runtime pour le glove témoin : le callsite `0x2CB0D9`
  reçoit `bodyLoc=10` et retourne `1`. Le contrôle suivant `0x159E60` retourne
  toutefois `0` nativement pour ce même objet de classe `334` — comme pour le
  Leather Armor de classe `314` observé séparément — et rejoint le refus UI.
- Le callback serveur mercenaire `0x4C0E20` rappelle le même validateur
  `0x36BC50` au callsite `0x4C1031` après avoir résolu l'objet du paquet et le
  mercenaire. Son témoin de 18 octets
  `E8 1A AC EA FF 85 C0 75 37 4C 8B 45 9F 8D 50 5D 48 8B` est unique dans le
  `.text` 92777. Un correctif limité au client serait donc insuffisant si ce
  contrôle commun est celui qui refuse le glove; les deux côtés doivent être
  observés avant toute extension.
- Le probe 0.0.6 a instrumenté les trois appels client. Plusieurs instances D2R
  ont cependant été fermées puis recréées par une autre session pendant les
  gestes de contrôle : les PID et heures de démarrage ont changé, aucun
  événement de crash ni aucune ligne de validation n'a été produit. Ce passage
  est invalide et ne doit pas être attribué au plugin. Le probe 0.0.7 corrigé
  expose exactement les trois exports v2, porte les métadonnées `0.0.7` et le
  SHA-256
  `2A8B170B400863A914F499E7EB7FEC158CFBF23952BBF1FDD3F0E86ECA6DAA3`.
- Le probe 0.0.8 a remplacé temporairement et uniquement le résultat du callsite
  `0x2CB0E8` pour la classe exacte `334`. La compatibilité est alors passée à
  `1`; les contrôles d'exigences client `0x2CB10D` et serveur `0x4C1031` ont tous
  deux retourné `1`. Le serveur a néanmoins refusé la transaction sans crash ni
  perte, ce qui exclut ces exigences comme cause immédiate du refus témoin. Ce
  contournement expérimental n'est pas une politique de production.
- Les probes 0.0.9 à 0.0.13 ont ensuite suivi la même transaction côté serveur.
  Le builder appelé à `0x4716A7` produit un état valide. Dans le validateur
  `0x473ED0` appelé à `0x471757`, les quatre contrôles initiaux `0x36E240`,
  `0x36E2D0`, `0x34FC20` et `0x34FC60` retournent `0`, les gardes `0x385550` et
  `0x474700` retournent `1`, et `0x46E2B0` renvoie un record dont le premier
  pointeur est nul. Le chemin exact atteint ensuite `0x472D20`.
- Les journaux 0.0.14 à 0.0.17 contiennent plusieurs états de placement pour le
  même objet de classe `334`. La corrélation stricte entre l'entrée et la sortie
  de `placement-server-first` distingue la transaction autoritaire refusée :
  ses quatre valeurs 32 bits sont `{1, 255, 10, 10}` aux offsets
  `+0x00/+0x04/+0x08/+0x0C`. L'état `{4, 255, 0, 0}` apparaît plus tard hors de
  cette enveloppe et ne doit pas être attribué au refus serveur. La synthèse
  antérieure `{1, -1, 10, 10}` avait également interprété `255` comme `-1`.
- Le probe 0.0.16 filtrait déjà exactement `{1, 255, 10, 10}`. Dans l'enveloppe
  autoritaire du glove, il observe `0x472B10=0`, puis `0x472D20=0`,
  `0x473ED0=0` et `ITEMS_PlaceItemForPlayer 0x471500=0`; `0x46E450`,
  `0x4730E0`, `0x4734F0` et le validateur final `0x46E050` ne sont pas atteints.
  `0x472B10` est donc le premier prédicat négatif exact maintenant prouvé.
- Le probe 0.0.17 a filtré `{4, 255, 0, 0}` et confirme que cet autre état passe
  `0x472D20`, `0x4734F0`, `0x473ED0` et `0x46E050`. Cette passe négative est
  conservée pour expliquer la distinction, mais elle ne remplace pas la preuve
  autoritaire 0.0.16. Son build Release porte le SHA-256
  `6BA96501241772514B38787754ED996329B35157CBD3626F4035918B5C8F92B7`.
- Le probe 0.0.18 décompose `0x472B10` sans en modifier le résultat. Pour la
  transaction autoritaire `{1, 255, 10, 10}`, le glove est identifié
  (`flag 16 -> 16`), sa classe est valide (`0`), il n'est pas brisé
  (`flag 256 -> 0`), la restriction spéciale retourne `0`, la politique de
  classe cible retourne `1`, puis les tests génériques `Armor` (`3`) et `Helm`
  (`37`) retournent tous deux `0`. Le refus survient donc après ces sept gardes.
  Le build Release porte le SHA-256
  `723E7D78C61B8686797986A4223DEEC9CE49607E5AD85BC6746F6AF5D9499B94`.
- Le probe 0.0.19 instrumente ensuite les appels uniques `0x472C61`,
  `0x472CBA`, `0x472CD4` et `0x472CED`. `0x3AEB70` résout sur la Rogue le couple
  autorisé `{47, 56}`; les deux appels `ITEMS_CheckItemTypeId` retournent `0`
  pour le glove. La table vanilla 3.2 `itemtypes.txt`, lue byte-exact, identifie
  `47` comme `Missile Weapon`, `56` comme `Missile` et `16` comme `Gloves` avec
  BodyLoc `glov`. Le couple compilé de la cible, et non le BodyLoc du widget ni
  les exigences du glove, est donc la cause directe et partagée du refus. Le
  build Release porte le SHA-256
  `EB2CF688D99D465356979CCC58D44BF1A4DFBA3F0A4B809A927464EFDC4255D7`.
- Le probe 0.0.20 remplace uniquement le résultat négatif du dernier test de ce
  couple : callsite client `0x159FC8` dans `0x159E60`, puis callsite serveur
  `0x472CED` dans `0x472B10`. L'admission est armée seulement pendant une
  transaction dont le BodyLoc demandé vaut `10` et seulement si
  `ITEMS_CanEquipAtBodyLocation(item, 10)` réussit; toutes les gardes natives
  précédentes, les exigences et les validateurs de placement restent actifs.
  Le build Release de 32 768 octets porte le SHA-256
  `7F93D0A67C42FD635DD175BECEF071A3A0BA7A39F8D7CB68E7481B58692287AA`.
- La transaction runtime 0.0.20 prouve le chemin complet. Le client journalise
  `client-equip-target-type-b-override=1`, puis la compatibilité vaut `1`. Pour
  l'état serveur exact `{1, 255, 10, 10}`, les événements `#11639` à `#11670`
  montrent le couple `{47, 56}`, l'override final à `1`, puis
  `0x472B10=1`, `0x46E450=1`, `0x4730E0=1`, `0x472D20=1`, `0x4734F0=1`, le
  validateur d'objet `=1`, le validateur final `=1` et
  `placement-server-first=1`. Visuellement, le glove quitte l'inventaire,
  apparaît dans le slot d'Amplisa et applique son effet; le clic de retrait le
  remet ensuite dans sa cellule d'origine et restaure l'état précédent.
- D2R a été fermé proprement après `Save and Exit`. Le journal 0.0.20 archivé
  localement porte le SHA-256
  `98BB099CF1E62DFF578CC6D11AB886716C3025CA8E3F8F108D6C865B716083D0`.
  La DLL et le journal ont été retirés du runtime; les huit fichiers protégés de
  `QtyTester` ont été restaurés byte-exact, zéro mismatch, et le `.d2s` retrouve
  `A7BEB417EB7311F6433629233FE6CFD80336D6804CD4CDACC97B7AF2C4D29270`.
- Le probe 0.0.22 généralise l'admission aux seuls bits d'un masque de BodyLocs,
  sans classe d'objet hardcodée. La matrice utilisait `0x00000400`, soit
  uniquement `gloves=10`, et conservait le contrôle
  `ITEMS_CanEquipAtBodyLocation`. Son build Release de 27 648 octets porte le
  SHA-256
  `4127B1A917B4D5C815712A814F0447391BA21285B7BCCB6F629F3AE26C15B725`.
  Le cold start a chargé 20/20 patchsets, 17 plugins scannés, 15 actifs, deux
  désactivés, zéro rejet, zéro échec et un démarrage 24/24.
- La matrice runtime 0.0.22 couvre tous les actes possédant un mercenaire :
  Amplisa (Acte I), Alhizeer (Acte II), Barani (Acte III) et Klisk (Acte V) ont
  chacun équipé puis retiré le glove du slot dynamique sans crash. Avec
  Alhizeer, `Save and Exit` puis rechargement en Pain ont conservé le mercenaire
  et le glove équipé avant son retrait. Il n'existe pas de mercenaire d'Acte IV.
  Cette matrice démontre la politique commune; elle ne justifie pas de répéter
  chaque futur slot sur chacun des quatre actes.
- La DLL, le journal et les sources exactes sont archivés localement sous
  `analysis-cache` avec un manifeste SHA-256; ils ont été retirés du runtime.
  Le journal porte
  `4F353432E5667D7BC574941CBA13177457E2A33CC38C72F6BEBE5318D1AD4D5A`.
  Le ring et l'armure protégés sont restés intacts, puis les huit fichiers de
  `QtyTester` ont été restaurés byte-exact à leurs hashes de départ, dont le
  `.d2s` `A7BEB417EB7311F6433629233FE6CFD80336D6804CD4CDACC97B7AF2C4D29270`.

### Conclusion d'audit

Le modèle « cases à cocher par slot » reste retenu. Le trajet réseau, le callback
serveur, l'adoption runtime d'un `ring` existant et la création visible d'un
`slot_gloves` absent sont prouvés. Les BodyLocs vanilla 1 à 8 correspondent aux
huit slots déjà observés; D2MOO épinglé et les layouts 92777 corroborent
`feet=9` et `gloves=10`, sans transférer aucune adresse legacy. Le contexte de
widget requis par le hit-test est prouvé par le crash 0.0.1 et le succès 0.0.2.
Les probes 0.0.18 à 0.0.20 isolent la comparaison au couple d'ItemTypes compilé
de la classe mercenaire et démontrent le point d'extension minimal. Le probe
0.0.22 généralise ensuite ce point à un masque de BodyLocs sans classe d'objet
hardcodée. Toutes les autres gardes restent natives; le glove s'équipe et se
retire sur les quatre actes de mercenaires, et la sauvegarde/rechargement est
confirmée avec l'Acte II. La prochaine implantation peut donc devenir un
prototype versionné configurable. Une release publique reste toutefois bloquée
par les six interrupteurs autonomes, la désactivation sûre, la manette, les deux
portées, le multijoueur et la coexistence finale.

## Hypothèses à tester

- La politique générique prouvée avec `gloves=10` peut piloter les six BodyLocs
  configurés sans modifier la hiérarchie globale des ItemTypes ni accepter un
  objet dont le BodyLoc natif ne correspond pas.
- Le panneau Hireling peut créer dynamiquement les slots absents avec la
  séquence native factory/finalizer/AddChild et reconstruire la navigation
  controller sans remplacer de fichier layout.
- La sérialisation native a conservé le glove équipé sur l'Acte II. Elle doit
  encore être vérifiée avec plusieurs slots occupés, en multijoueur et pendant
  le cycle de désactivation, sans format de sauvegarde parallèle.

## Architecture visée

1. Politique serveur ou autoritaire : conserver toutes les validations vanilla
   (propriété, distance, exigences, restrictions propres à l'acte) et étendre
   seulement l'éligibilité des slots activés.
2. Interface client : adopter les widgets existants, créer seulement les slots
   manquants et reconstruire la navigation sans écrire de layout sur disque.
3. Cycle de vie : un slot désactivé mais occupé reste visible en récupération
   seulement; le retrait est autorisé, le nouvel équipement est refusé, puis le
   slot disparaît une fois vide. Aucun objet n'est supprimé ou déplacé
   silencieusement.

## Gates avant implantation

- Le paquet client `0x51`, le callback serveur `0x4C0E20`, son ABI, sa taille de
  17 octets et sa signature unique sont prouvés. La factory, le finalizer et
  l'attachement UI sont prouvés statiquement et au runtime. La transaction avec
  un glove réel est maintenant acceptée par le prototype ciblé. Les preuves
  0.0.18/0.0.19 identifient le couple natif Rogue `{47, 56}` comme cause du
  refus, et 0.0.20 fait passer uniquement le dernier test d'ItemType aux
  callsites client `0x159FC8` et serveur `0x472CED` lorsque le BodyLoc `10` est
  demandé et validé. `0x472B10`, `0x472D20`, `0x473ED0` et
  `ITEMS_PlaceItemForPlayer 0x471500` retournent alors tous `1`; équipement et
  retrait sont confirmés. Le probe 0.0.22 généralise cette règle à un masque de
  BodyLocs sans condition hardcodée sur la classe d'objet; la matrice A1/A2/A3/A5
  et la sauvegarde/rechargement A2 sont vertes.
- Auditer le commit PluginPack épinglé, les cinq DLL et chaque plage de hook ou
  structure partagée. Toute collision doit avoir un propriétaire unique avant
  l'écriture de code.
- Le slot absent témoin `gloves` est maintenant créé sans table ni layout,
  visible et cliquable à la souris; équipement et retrait sont prouvés avec un
  glove réel. Il reste à prouver navigation manette, fermeture/réouverture et
  reconstruction répétée du panneau. L'adoption d'un `ring` existant est déjà
  confirmée; `belt` est déjà exposé dans le fixture BKVince et n'est plus un
  témoin suffisant de la factory.
- La sauvegarde/rechargement avec le glove équipé est prouvée sur l'Acte II. Il
  reste à tester plusieurs slots occupés, la désactivation avec récupération
  seulement, la désinstallation après vidage, solo, hôte/joiner et l'absence de
  perte, duplication, objet fantôme, crash ou désynchronisation.
- Valider séparément les portées globale et mod-locale, le repli de
  configuration, la configuration absente ou invalide, le cold start et les
  hashes.
- Avant publication, limiter le ZIP public à la DLL autonome et à son unique
  configuration indispensable, puis inspecter les entrées et calculer le
  SHA-256.

## Prochain gate

Versionner le prototype autonome et figer une configuration indépendante avec
six booléens : `amulet`, `rightRing`, `leftRing`, `belt`, `boots` et `gloves`.
Construire le masque fail-closed uniquement depuis ces valeurs, adopter les
widgets existants et créer seulement les slots absents, dont `boots=9` et
`gloves=10`, sans table ni layout. Une configuration présente mais invalide doit
faire refuser le plugin; l'absence de configuration doit conserver le vanilla.

Valider ensuite les six slots dans une seule session préparée, puis la
fermeture/réouverture du panneau, plusieurs slots occupés au rechargement, la
désactivation en récupération seulement, le vidage puis la disparition et la
navigation controller. La couverture A1/A2/A3/A5 déjà verte ne sera pas répétée
pour chaque slot. Fermer enfin les portées globale/mod-locale, le solo et
hôte/joiner et la coexistence PluginPack avant de produire le ZIP public.
