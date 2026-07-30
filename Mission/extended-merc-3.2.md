# ExtendedMerc — D2R 3.2.92777

Dernière mise à jour : 29 juillet 2026

Statut : incubation native active. Vincent a choisi le nom `ExtendedMerc`, la
voie du plugin autonome permanent et le séquencement A. Le trajet réseau, le
callback serveur, l'adoption d'un slot existant et les primitives natives de
création/attachement UI sont maintenant prouvés. Un premier `slot_gloves`
absent a été créé, affiché et manipulé sans crash au runtime. Le test avec un
glove réel prouve toutefois que le contrôleur natif refuse encore la transaction
vers ce nouveau widget; son extension ciblée est désormais le gate avant
l'implantation de production.

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

### Conclusion d'audit

Le modèle « cases à cocher par slot » reste retenu. Le trajet réseau, le callback
serveur, l'adoption runtime d'un `ring` existant et la création visible d'un
`slot_gloves` absent sont prouvés. Les BodyLocs vanilla 1 à 8 correspondent aux
huit slots déjà observés; D2MOO épinglé et les layouts 92777 corroborent
`feet=9` et `gloves=10`, sans transférer aucune adresse legacy. Le contexte de
widget requis par le hit-test est maintenant prouvé par le crash 0.0.1 et le
succès 0.0.2. Le test avec un objet réel démontre que la création UI seule ne
suffit pas : le contrôleur de transaction rejette actuellement le BodyLoc 10 du
nouveau widget. Il faut donc identifier puis étendre fail-closed ce chemin natif
avant de pouvoir prouver équipement, retrait et restauration après sauvegarde.
Il ne faut créer ni DLL de production, ni configuration, ni archive ExtendedMerc
avant cette preuve.

## Hypothèses à tester

- Une extension ciblée du contrôleur de transaction peut accepter les BodyLocs
  supplémentaires activés sans modifier la hiérarchie globale des ItemTypes.
- Le panneau Hireling peut créer dynamiquement les slots absents avec la
  séquence native factory/finalizer/AddChild et reconstruire la navigation
  controller sans remplacer de fichier layout.
- La sérialisation native des objets équipés du mercenaire suffit tant que le
  plugin n'introduit aucun format de sauvegarde parallèle.

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
  un glove réel est refusée sans crash; il reste à identifier le contrôleur qui
  décide ce refus, à étendre seulement les slots activés, puis à valider une
  transaction d'équipement complète dans le slot créé.
- Auditer le commit PluginPack épinglé, les cinq DLL et chaque plage de hook ou
  structure partagée. Toute collision doit avoir un propriétaire unique avant
  l'écriture de code.
- Le slot absent témoin `gloves` est maintenant créé sans table ni layout,
  visible et cliquable à vide à la souris. Il reste à prouver équipement/retrait,
  navigation manette et reconstruction répétée du panneau. L'adoption d'un
  `ring` existant est déjà confirmée; `belt` est déjà exposé dans le fixture
  BKVince et n'est plus un témoin suffisant de la factory.
- Prouver la sauvegarde/rechargement, la désactivation avec slot occupé, la
  désinstallation après vidage, solo, hôte/joiner et l'absence de perte,
  duplication, objet fantôme, crash ou désynchronisation.
- Valider séparément les portées globale et mod-locale, le repli de
  configuration, la configuration absente ou invalide, le cold start et les
  hashes.
- Avant publication, limiter le ZIP public à la DLL autonome et à son unique
  configuration indispensable, puis inspecter les entrées et calculer le
  SHA-256.

## Prochain gate

Tracer le refus observé entre le drop sur `slot_gloves` et la construction ou
l'émission de la transaction native, identifier le contrôleur/validateur exact
sur le build 92777, puis étendre fail-closed uniquement les BodyLocs activés.
Redéployer ensuite le probe et valider la transaction complète : équipement,
retrait, fermeture et réouverture du panneau, sauvegarde/rechargement puis
navigation controller. Ne modifier aucune table, aucun layout ni le format de
sauvegarde. Une fois ce témoin vert, figer le hook, la navigation adaptative et
la configuration autonome de production.
