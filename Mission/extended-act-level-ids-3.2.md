# Extended Act Level IDs — D2R 3.3.93847

Dernière mise à jour : 4 septembre 2026

Statut : **cache physique deux passes qualifié avec 1023 records; voyage
physique Harrogath ↔ Level 256 validé; correction Town Portal 2.1.1 avec
sidecars serveur/client et relay UI `0xC188E` implantée et qualifiée
statiquement; runtime 2.1.1 non exécuté et release bloquée**.
Vincent a donné `GO` le 4 septembre 2026 pour étendre le produit existant à la
limite native de 1023 records. La source 2.1.1 conserve le cache deux passes et
le codec room-visibility qualifiés, puis ajoute le contrat Town Portal
local/offline sous empreintes fail-closed. Le
diagnostic 2.0.1 a montré que `findRowById(256)` retourne `NotFound` malgré une
ligne physique valide; la 2.0.2 contourne cette limite par un cache strict en
deux passes. Son cold start exact de 1023 records sur Battle.net D2R 3.3.93847
et D2RLoader public 1.2.1 publie `RotW=1023` avec la pile complète. Une fixture
same-act relie maintenant Harrogath `109 / Act 4` au Level `256 / Act 4` :
Vincent apparaît normalement dans la zone et MapSense observe le graphe de
rooms du niveau 256 sans crash. Le retour répété est validé et la création d'un
Town Portal depuis Level 256 atteint le garde natif
`eLevelIdLocal <= 255` avant une troncature réelle vers
`ObjectData.InteractType:uint8`. Le census natif complet ferme maintenant la
création, l'opération, les paquets `0x51/0x60`, la compression/réactivation par
GUID et les suppressions. La 2.1.1 retient un sidecar éphémère par paire de
GUID côté serveur et un sidecar GUID/session borné côté client, sans `Unit*`
persistant; le multijoueur TCP reste bloqué faute d'énumération autoritaire de
tous les clients actifs. Le waypoint, automap visuel, save/reload et
host/joiner restent ouverts avant toute archive ou release. Les résultats
0.1.1 ci-dessous demeurent l'historique qualifié de la première fonction
`Levels.txt → Act`, pas une preuve gameplay complète du codec v2.

## Décision v2 — extension fonctionnelle à 1023 records — 4 septembre 2026

- Vincent a explicitement autorisé `GO implantation Extended Level IDs v2` le
  4 septembre 2026 après la revue read-only `$plugin-architect`.
- Le produit reste la DLL autonome RuffnecKk existante, hybride globale ou
  mod-locale, membre de la RuffnecKk D2RLoader Suite. La v2 conserve une seule
  identité de plugin, sa version indépendante et l'absence de configuration :
  aucun réglage moddeur réel distinct de la présence de la DLL n'est démontré.
- L'effet joueur retenu est de rendre fonctionnels jusqu'à 1023 records
  `Levels` compilés, soit les Level IDs contigus `0..1022`. L'ID `1023` et un
  1024e record restent hors contrat; la ligne de séparation texte `Expansion`
  n'est pas comptée comme un niveau jouable.
- La garde native `0x330446` accepte déjà au plus `0x3FF` records et ne sera
  pas patchée. La v2 doit plutôt supprimer les troncatures à huit bits prouvées
  dans les constructeurs serveur des paquets `0x07` et `0x08`, tout en
  conservant les chemins internes déjà en `int32`.
- Le mécanisme candidat retenu est un codec natif session-wide portant le Level
  ID sur 16 bits uniquement lorsque producteurs et consommateurs compatibles
  sont tous présents. Le protocole vanilla doit rester inchangé sur Battle.net
  et devant tout pair incompatible; aucune identité de build ou de canal ne
  peut sélectionner un profil natif.
- La v2 ne doit modifier aucun codec, taille de section ni schéma D2S/D2I, ne
  doit pas étendre le bitset de waypoints ni la représentation persistante des
  portal flags, et ne doit jamais exiger un convertisseur de sauvegarde. Toute
  preuve contraire arrête l'implantation avant release.
- Avant le premier nouveau hook, le reverse engineering doit gouverner les
  dispatchers et consommateurs client `0x07/0x08`, les handlers exacts des
  requêtes Town Portal/waypoint, la sélection atomique du protocole et les
  surfaces de persistance Automap/waypoint/portal/dernier niveau réellement
  nécessaires. Chaque RVA, signature, ABI et témoin de layout utilisé doit
  appartenir à l'empreinte fail-closed.
- L'incubation, les builds et les tests hors jeu sont autorisés. Tout arrêt,
  lancement, redémarrage, déploiement ou cold start du runtime attend une
  confirmation explicite séparée selon `d2r-runtime-validation`.

## Résultat d'implantation v2 — 4 septembre 2026

- `addons/ExtendedActLevelIds/src/plugin.cpp` porte maintenant cinq hooks : le
  résolveur `Levels.Act`, les constructeurs serveur des paquets room-in-sight
  `0x07` et room-out-of-sight `0x08`, puis leurs deux consommateurs client.
  Chaque RVA, signature, ABI et témoin de layout effectivement utilisé est
  couvert par l'empreinte fail-closed; aucun numéro de build ne choisit le
  chargement.
- Le cache refuse plus de 1023 lignes et exige `Id == index` pour chaque record,
  donc le contrat fonctionnel exact est `0..1022`. La garde native `<0x400` à
  `0x330446` est seulement vérifiée, jamais modifiée.
- Le protocole `0x07/0x08` vanilla reste byte-for-byte inchangé pour les IDs
  `0..255`. Pour `256..1022`, un codec v2 conserve les six octets, transporte
  les deux bits hauts de l'ID dans une partie réservée du WORD X et restaure X
  avant `DUNGEON_Set/UnsetClientIsInSight`. Le domaine démontré est X
  `0..8191`; BKVince reste actuellement sous cette borne.
- Le `NetworkServiceV1` impose le token exact `0x454C494456320001`. Le joueur
  local est identifié par `LocalPlayerReady`; un joiner doit en plus annoncer
  son GUID via le channel compatible. Un pair inconnu, absent ou incompatible
  reçoit zéro paquet étendu plutôt qu'un ID tronqué. Battle.net conserve le
  protocole vanilla et les IDs `>255` y échouent volontairement en mode fermé.
- Aucun fichier de configuration n'est ajouté. La DLL reste autonome, hybride
  globale/mod-locale et active par sa présence. Elle ne touche aucun codec ni
  layout D2S/D2I, et ne redimensionne ni waypoints ni portal flags.
- Les tests de codec couvrent les frontières `256/511/512/767/768/1021/1022`,
  les entrées invalides et la limite de records. Le premier build Release MSVC
  passe CTest `1/1`; reproductibilité, inspection PE et matrice runtime sont
  consignées séparément dans `addons/ExtendedActLevelIds/VALIDATION.md`.
- Les handlers D2R exacts Town Portal/waypoint et le test d'une véritable zone
  `>255` restent des gates de qualification. La v2 ne les hooke pas et ne
  revendique encore ni support gameplay complet ni release.

## Résultat runtime v2 — 4 septembre 2026

- Vincent a autorisé `GO runtime Extended Level IDs v2`. La DLL reproductible
  SHA-256
  `1874623DA1B4914BE465A430D117B19174D986CF9E326E057FFEB115AD10C508`
  a été déployée uniquement dans la portée mod-locale BKVince.
- Le runtime réellement testé est Battle.net D2R `3.3.93847`, Build Key
  `623f7a1f73eabb08ccb2b2046e3f9164`, sous la baseline publique promue
  D2RLoader `1.2.1`. Les hashes de `.build.info`, `D2R.exe`, `D2RLoader.exe`,
  de la source et de la copie runtime ont été relevés avant conclusion.
- Le plugin journalise `2.0.0`, accepte l'empreinte native complète et le canal
  privé, puis publie `Levels revision 1` avec `Classic=137`, `LoD=137` et
  `RotW=147`.
- La pile complète charge 38 plugins, applique 17 memory patches et atteint
  `24/24`. Les cinq plugins eezstreet restent actifs. La copie globale 1.0.0
  n'a pas été retirée : D2RLoader la saute explicitement parce que la v2
  mod-locale possède déjà l'identité, ce qui ferme le gate d'arbitrage.
- Les reçus et logs frais sont conservés sous
  `analysis-cache/extended-act-level-ids-v2/runtime/20260904-v2-cold-start/`.
- Ce cold start ne contenait aucun Level ID supérieur à 255. Le codec room
  visibility, une zone navigable, Town Portal, waypoint, automap, save/reload,
  hôte/joiner et pair incompatible restent donc `not run`, sans extrapolation.

## Gate capacité 1023 — résultat fail-closed du 4 septembre 2026

- Le fixture gouverné contient exactement 1023 records compilables, IDs
  contigus `0..1022`, 188 colonnes, CRLF et round-trip byte-exact. Son SHA-256
  source/runtime était
  `6D6756E03911C2BA531F007AE9E97EEC6E81C879800182964AF6BCB3E69C3FAF`.
- D2RLoader a compilé les 192 tables TXT, chargé 38 plugins et 17 memory
  patches, ignoré la copie globale dupliquée et atteint `24/24`. La DLL v2 a
  accepté son empreinte native et son canal privé.
- Le `TableView` RotW a passé les gardes initiales de service, révision,
  pointeur, nombre et taille de records. Un record physique a ensuite échoué
  l'identité `findRowById(Id) == même pointeur/même index`; la DLL a donc refusé
  tout le cache et conservé le résolveur original. Le log actuel ne nomme pas
  encore le premier ID fautif.
- Ce résultat invalide l'hypothèse selon laquelle le lookup logique
  `DataTableServiceV1` garantit directement toute la plage `0..1022`. Il ne
  justifie pas de supprimer silencieusement le garde-fou : il faut d'abord
  borner le premier échec, auditer l'implémentation du service et arbitrer entre
  table physique validée, évolution de service ou nouveau seam natif.
- Aucun personnage ni save n'a été chargé avec le fixture. Le processus de
  test a été arrêté, puis `levels.txt` source et runtime ont été restaurés
  byte-exact au SHA-256
  `A46B5438164ADB1FB9540890103594EA48A79AFA2478CB6865D2E6DB5795EB04`.
  Les reçus et logs sont conservés sous
  `analysis-cache/extended-act-level-ids-v2/runtime/20260904-capacity-1023/`.

## Gate diagnostique 2.0.1 — résultat runtime du 4 septembre 2026

- Vincent a autorisé `next gate go` pour identifier le premier échec du
  round-trip sans changer la décision fail-closed.
- Le diagnostic rapporte désormais le premier `rowIndex`, son `levelId`, le
  résultat numérique de `findRowById`, les valeurs retournées/attendues de
  révision, index et taille, puis leurs indicateurs de concordance. Le pointeur
  n'est jamais journalisé : seule son égalité est exposée par `rowMatch=0/1`.
- La condition et la sortie restent identiques : au premier écart, le cache de
  banque entier est refusé et le résolveur original demeure actif. Aucun hook,
  codec, token de compatibilité, appel de service, comportement de sauvegarde
  ou règle TSV n'a changé.
- Deux builds Release indépendants de 60 928 octets sont byte-identiques au
  SHA-256
  `A990980E762C8C0278DDCFE80F4BBCE6E5F042C859B9BC24B90B4C107D61F945`;
  chacun passe CTest `1/1` sans avertissement. Les métadonnées PE et PluginInfo
  portent `2.0.1`, `RuffnecKk`, et les trois exports D2RLoader restent seuls.
- Vincent a ensuite autorisé `GO runtime diagnostic 1023`. Le binaire exact
  `A990980E…D61F945` et le fixture exact `6D6756E0…3FAF` ont été déployés
  temporairement en portée mod-locale sous Battle.net D2R 3.3.93847 et
  D2RLoader public 1.2.1.
- La pile complète compile 192 TXT, charge 38 plugins et 17 patches, saute la
  copie globale plus ancienne et atteint `24/24` avec les cinq DLL eezstreet.
- Le premier échec est exactement `rowIndex=256 / levelId=256`.
  `findRowById` retourne `serviceResult=5`, soit `NotFound` dans le SDK; le
  `RowView` retourné reste nul (`revision=0`, `rowIndex=0`, `rowSize=0`,
  `rowMatch=0`). Le `getRow(256)` immédiatement précédent avait donc réussi,
  faute de quoi le diagnostic keyed n'aurait jamais été atteint.
- Le comportement fail-closed reste vert : cache RotW rejeté en entier,
  résolveur original conservé, aucun personnage ou save ouvert. Les records
  physiques 257..1022 ne sont pas encore prouvés individuellement, car la
  boucle s'arrête volontairement au premier lookup keyed refusé.
- Le processus a été arrêté. Le premier rollback combiné a restauré la table
  mais rencontré un verrou tardif sur la DLL; le retry borné, sans relance, a
  restauré uniquement cette DLL. Source et runtime portent finalement la table
  `A46B5438…795EB04` et la DLL 2.0.0 `1874623D…10C508`, byte-exact, sans
  processus Diablo restant.
- Les preuves fraîches sont conservées sous
  `analysis-cache/extended-act-level-ids-v2/runtime/20260904-capacity-1023-diagnostic-2.0.1/`.

### Décision d'architecture issue du diagnostic

- **Fait vérifié :** l'API expose un ID `uint32_t` et documente
  `findRowById` pour `Levels`, mais le fournisseur runtime retourne `NotFound`
  dès 256. Le chemin keyed actuel est donc réellement borné à la plage vanilla
  dans cette baseline, quelle qu'en soit la cause interne encore inconnue.
- **Fait vérifié :** `getTable` expose 1023 records et `getRow` réussit au moins
  jusqu'à l'index physique 256 avec le stride attendu. Le plugin impose déjà
  `Id == rowIndex`, unicité, plage `0..1022`, Act `0..4` et ancres vanilla.
- **Inconnu :** le run n'a pas balayé les lignes physiques 257..1022; il ne
  prouve donc pas encore que `getRow` les fournit toutes.
- **Recommandation :** ne pas ajouter de seam natif et ne pas attendre une
  extension D2RLoader pour poursuivre. Le prochain prototype doit valider les
  1023 lignes physiques dans une première passe, puis conserver le round-trip
  keyed seulement pour `0..255`. Cette équivalence est acceptable uniquement
  parce que le contrat du produit exige des IDs canoniques contigus égaux aux
  index physiques. Toute ligne physique absente ou invalide conserve le rejet
  complet du cache.
- Une correction du service reste un problème réel à signaler, avec Extended
  Act Level IDs comme consommateur concret, mais elle n'est pas encore une
  dépendance démontrée : l'API `getRow` existante peut couvrir le besoin si le
  balayage `0..1022` passe. Aucun changement de comportement n'est implanté
  sans un nouveau `GO`.
- Vincent a autorisé l'implantation avec
  `GO implantation cache physique deux passes` le 4 septembre 2026. Le lot
  reste borné au cache : aucun nouveau seam natif, hook, protocole réseau,
  format de sauvegarde ou fichier de configuration n'est autorisé.

### Implantation du cache physique deux passes — 4 septembre 2026

- La version candidate devient `2.0.2`; le diagnostic `2.0.1` reste une preuve
  historique distincte et immuable.
- `BuildBankCache` balaie d'abord chaque ligne physique `0..rowCount-1` avec
  `getRow`, vérifie révision, pointeur, index, stride, `Id == rowIndex` et
  `Act 0..4`, puis copie seulement `{levelId, act}` dans le cache propriétaire.
- Une seconde passe réacquiert les lignes physiques et conserve l'identité
  `findRowById == même pointeur/index/révision/stride` pour au plus `0..255`.
  Aucun appel keyed n'est fait au-delà de la frontière runtime démontrée.
- Les trois banques restent publiées atomiquement seulement après leur succès
  commun; toute erreur réinitialise les caches et conserve le résolveur original.
- Deux builds Release indépendants sont byte-identiques : 61 440 octets,
  SHA-256 `FDAF884B69AB879D8F3946E2CF036FF25AF3C03C7CBE479803761A19B13A6EC1`.
  Les deux CTest passent `1/1` sans avertissement. Les métadonnées portent
  `RuffnecKk / 2.0.2`, les trois exports D2RLoader restent seuls et les imports
  demeurent limités aux runtimes Windows/MSVC attendus.
- Aucun RVA, octet `Expected`, hook, ABI, codec, canal privé, format D2S/D2I,
  configuration ou dépendance inter-plugin n'a changé. Le runtime n'a pas été
  touché pendant ce gate.
- Le gate runtime suivant exigeait le binaire exact 2.0.2 avec le fixture exact
  de 1023 records sous la pile complète, la publication
  `Classic=137 / LoD=137 / RotW=1023` et un rollback byte-exact. Il est fermé
  par le résultat ci-dessous.

### Runtime exact 1023 du cache deux passes — 4 septembre 2026

- Vincent a autorisé ce cold start avec `GO runtime 1023 cache deux passes`.
  Le profil réellement testé est BKVince mod-local avec `-mod BKVince -txt`
  sous Battle.net D2R `3.3.93847` et D2RLoader public `1.2.1`; les hashes du
  build installé concordent avec la baseline déjà gouvernée.
- La DLL exacte 2.0.2
  `FDAF884B69AB879D8F3946E2CF036FF25AF3C03C7CBE479803761A19B13A6EC1`
  et le fixture exact
  `6D6756E03911C2BA531F007AE9E97EEC6E81C879800182964AF6BCB3E69C3FAF`
  ont été déployés temporairement uniquement dans la portée mod-locale.
- La pile complète compile 192 tables TXT, charge 38 plugins, conserve les cinq
  DLL eezstreet, saute la copie globale dupliquée, applique 17 memory patches
  et atteint `24/24`, sans erreur fraîche.
- Extended Act Level IDs 2.0.2 accepte ses empreintes natives et son canal
  privé, puis publie atomiquement `Levels revision 1` avec `Classic=137`,
  `LoD=137` et `RotW=1023`. Le balayage physique `0..1022` et le round-trip
  keyed limité à `0..255` passent donc ensemble sur le runtime officiel.
- Aucun personnage ni save n'a été ouvert. Zone jouable `>255`, résolveur,
  room streaming, voyage retour, Town Portal, waypoint, automap, save/reload,
  hôte/joiner et pair incompatible restent `not run`.
- Après capture des logs, l'unique processus a été arrêté. La source et le
  runtime ont été restaurés byte-exact à la table normale
  `A46B5438…795EB04` et à la DLL mod-locale 2.0.0 `1874623D…10C508`; la copie
  globale 1.0.0 est restée intacte et aucun processus Diablo ne demeure.
- Les preuves, copies avant test et reçus de déploiement/rollback sont sous
  `analysis-cache/extended-act-level-ids-v2/runtime/20260904-capacity-1023-two-pass-2.0.2/`.
- **Prochain gate :** avec un personnage jetable et une véritable zone d'ID
  supérieur à 255, valider l'acte résolu, le room streaming, les voyages aller
  et retour, Town Portal, waypoint et automap. Save/reload et hôte/joiner
  restent des autorisations runtime distinctes.

### Gate gameplay Level 256 same-act — 4 septembre 2026

- Vincent a autorisé ce gate avec `go next gate`, puis a confirmé après le
  trajet Harrogath → `To Rift Level 7` : « j'apparais normalement dans le
  level ».
- La fixture gouvernée conserve 1023 records numériques contigus `0..1022`
  plus l'unique séparateur texte historique `Expansion`. Son SHA-256 est
  `5C9B604E0D9A595D8DDB24699D91AEF0C564E7956BE7DBAA7CC5B8B92928B2DF`.
  Par rapport à la fixture précédente, une seule cellule change : `Id=256`,
  `Act 0 → 4`. Harrogath `Id=109 / Act=4 / Vis0=256` et le Level 256
  `Act=4 / Vis0=109 / DrlgType=2` forment donc un lien same-act. Le preset
  `LevelId=256 / Def=1095` reste présent dans `lvlprest.txt`, SHA-256
  `4EF8B404DB5E54C0C676E7C57D290CF142ED5C8B158D9A11056FB7975869EC39`.
- La DLL candidate exacte reste la 2.0.2 de 61 440 octets, SHA-256
  `FDAF884B69AB879D8F3946E2CF036FF25AF3C03C7CBE479803761A19B13A6EC1`.
  Le cold start BKVince complet compile 192 tables, charge 38 plugins dont les
  cinq DLL eezstreet, applique 17 patches et atteint `24/24`; le cache publie
  `Classic=137 / LoD=137 / RotW=1023`.
- Après l'entrée, MapSense journalise `current-level=256`, `act=4`,
  `room-witness=35` et `external labels: PASS`. Le personnage et la scène sont
  rendus normalement; aucun nouveau rapport de crash D2RLoader n'est créé.
  Cela ferme l'entrée locale same-act et fournit un témoin runtime du graphe de
  rooms au-delà de 255. Les compteurs explicites du codec, le retour, Town
  Portal, waypoint, automap visuel, save/reload et le réseau restent ouverts.
- La session a modifié les fichiers ordinaires de position/session
  `QtyTester.d2s` et `QtyTester.d2rl`; aucune conclusion de format persistant
  n'en est déduite. Après capture, l'unique processus a été arrêté et les neuf
  fichiers `QtyTester` ont été restaurés à leurs SHA-256 pré-gate, 9/9 exacts.
- `levels.txt`, `lvlprest.txt` et la DLL ont aussi été restaurés byte-exact dans
  la source et le runtime aux hashes normaux `A46B5438…795EB04`,
  `AE719711…5EB04` et `1874623D…10C508`. Aucun processus Diablo ne demeure.
- Les fixtures, logs avant/après, témoins MapSense, copies de sauvegarde et
  reçus de déploiement/rollback sont conservés sous
  `analysis-cache/extended-act-level-ids-v2/runtime/20260904-gameplay-level-256-same-act-2.0.2/`.
- **Prochain gate recommandé :** rejouer la même fixture et valider d'abord le
  retour Level 256 → Harrogath puis Town Portal aller/retour. Le waypoint,
  save/reload, manette et hôte/joiner restent ensuite des gates séparés.

### Gate retour physique et Town Portal Level 256 — 4 septembre 2026

- Vincent a autorisé `retour Level 256 → Harrogath, puis Town Portal
  aller/retour` avec `Go`. La DLL 2.0.2 et les deux fixtures exactes du gate
  same-act ont été redéployées dans le profil BKVince mod-local après backup
  des tables, de la DLL et des neuf fichiers `QtyTester`.
- Le cold start complet reste vert : 192 tables TXT, 38 plugins, les cinq DLL
  eezstreet, 17 patches et startup `24/24`. Extended Act Level IDs publie
  `Classic=137 / LoD=137 / RotW=1023`.
- La chronologie MapSense ferme sans ambiguïté le voyage physique. Elle observe
  `109 → 256 → 109 → 256 → 109 → 256`, toujours Act 4; chaque observation du
  Level 256 porte `room-witness=35` et `external labels: PASS`. Le retour
  Level 256 → Harrogath est donc **PASS**, y compris deux répétitions avant le
  test du portail.
- À `12:03:39.462`, la création d'un Town Portal depuis Level 256 déclenche
  `BC_ASSERT: eLevelIdLocal <= 255` dans
  `D2Game\src\Skills\Skills.cpp:4120`. La pile fraîche revient par
  `0x436075 → 0x432F27 → 0x46FD81 → 0x581965 → 0x4F52CB → 0x4C144C →
  0x4F30BD`. Le test s'arrête avant l'entrée dans le portail; aucun aller ou
  retour Town Portal n'est revendiqué.
- L'analyse du corpus commun identifie `D2GAME_CreateLinkPortal 0x435DD0` et
  son témoin strict unique `0x436061`. Après avoir obtenu le Level ID complet
  de la room source, le code compare `EAX` à `0xFF`, assert, puis exécute
  `movzx edx,dil` avant `UNITS_SetObjectInteractType 0x34E9D0`. Ce setter écrit
  un byte à `ObjectData+0x08`; ignorer ou NOPer l'assertion stockerait donc
  Level 256 comme `0`.
- La largeur huit bits est également prouvée chez le consommateur
  `SUNIT_GetPortalOwner 0x490070`, dans le producteur serveur du paquet portail
  `0x60` à `0x47F650` et dans son consommateur client à `0x1CB1C0`. La v2 ne
  couvre actuellement que les paquets room-visibility `0x07/0x08`; le Town
  Portal exige un sidecar/session et un codec portail distincts, pas la
  suppression d'un assert.
- Le processus a été arrêté sans Continue. Le rollback combiné a rencontré un
  verrou DLL tardif après avoir restauré les deux tables; son retry borné a
  restauré uniquement la DLL. Source/runtime reviennent byte-exact aux hashes
  normaux `A46B5438…795EB04`, `AE719711…5EB04` et
  `1874623D…10C508`. Les neuf fichiers `QtyTester` concordent 9/9 avec le
  backup pré-gate et aucun processus ne demeure.
- La capture, les logs, les sauvegardes avant/assertion, les reçus et le rapport
  sont conservés sous
  `analysis-cache/extended-act-level-ids-v2/runtime/20260904-gameplay-level-256-return-portal-2.0.2/`.
- **Décision de gate :** voyage physique qualifié; Town Portal **bloqué** et
  release toujours interdite. Aucun nouveau hook, codec, sidecar ou changement
  de sauvegarde n'est implanté sans nouvelle autorisation.
- **Prochain gate recommandé :** reverse engineering strictement read-only du
  pipeline portail complet — créations/destructions, interaction/téléport,
  lifecycle/GUID, paquet initial `0x51`, paquet d'état `0x60`, autorité
  host/joiner et fail-closed incompatible/Battle.net — puis choix explicite du
  contrat sidecar avant code. Waypoint, save/reload, manette et réseau runtime
  restent séparés.

### Gate reverse engineering Town Portal 1023 — 4 septembre 2026

- Vincent a autorisé `GO reverse engineering Town Portal 1023`. Le workbench
  natif commun 92777/93847, son image et son index ont été validés avant un
  census strictement read-only; aucun code, build, déploiement, runtime ou save
  n'a été touché.
- La création centrale passe par `D2GAME_CreatePortalObject 0x432CE0`, puis
  `D2GAME_CreateLinkPortal 0x435DD0`. Le premier centralise quatorze callsites;
  le second a seulement deux appels directs et conserve le full destination
  ID jusqu'à ce que le Level ID de la room source soit asserté puis réduit au
  low byte à `0x436061`.
- L'usage est centralisé par `OBJECTS_OperateFunction15_Portal 0x58F680` :
  permissions, propriétaire distant, records de niveau, changement de room,
  déplacement, puis suppression des deux moitiés si le joueur possède le
  portail. `D2GAME_RemovePlayerPortal 0x4C8650` fournit l'autre suppression
  autoritaire de la paire.
- Le paquet initial `0x51`, produit à `0x47CE40` et consommé à `0x129D70`,
  transporte `InteractType` sur un byte à `+0x0D`. Le paquet d'état `0x60`,
  produit à `0x47F620` et consommé à `0x1CB1C0`, porte deux témoins huit bits :
  destination à `+2` et Level ID de la room courante du joueur propriétaire à
  `+0x0B`; les coordonnées propriétaire restent des words à `+7/+9`. Le gate
  runtime 2.1.0 ci-dessous prouve que ces champs sont indépendants.
- La compression inactive est explicitement spéciale pour les classes portail
  `59/60` dans `SUNITINACTIVE_CompressUnitIfNeeded 0x504260`. Le nœud de
  `SUNITINACTIVE_CompressInactiveUnit 0x5045D0` conserve le GUID à `+0x20`
  mais seulement le low byte à `+0x28`; la restauration `0x503790` recrée une
  nouvelle `Unit*` avec ce même GUID. Le sidecar doit donc être indexé par
  `{génération de session, Game*, GUID}`, survivre à la compression et vérifier
  classe, paire et low-byte contre la réutilisation d'un GUID périmé.
- Aucun retarget D2R par opcode `0x45` n'est démontré dans le corpus statique;
  la sémantique D2MOO ne peut pas être promue sans preuve séparée de la table de
  dispatch vivante. Aucun autre plugin recensé ne possède les seams portail
  retenues.
- L'option « ignorer l'assertion » est rejetée car `256` deviendrait `0`.
  L'élargissement global d'`ObjectData`, des nœuds inactifs et des paquets est
  rejeté à cause des ABI partagées et de la rupture réseau. L'architecture
  retenue est un sidecar atomique par paire de GUID/session, limité d'abord au
  Town Portal dynamique classe `59`, avec hooks centraux, scopes TLS étroits,
  validation fail-closed et codecs `0x51/0x60` sans changement de taille.
- Le codec peut réutiliser le marqueur coordonnée de la v2 pour porter les deux
  bits hauts uniquement entre pairs compatibles. La création, la résolution
  du propriétaire et l'opération doivent cependant refuser tout lookup sidecar
  incohérent; aucun repli vers le low byte n'est admis pour un portail étendu.
- Le canal privé actuel sait identifier les pairs compatibles, mais
  `NetworkServiceV1` n'énumère pas tous les clients actifs. L'implantation peut
  viser un premier contrat local/offline; toute revendication TCP hôte/joiner
  exige d'abord un census natif de la liste clients ou un service SDK gouverné.
  Battle.net et toute partie à compatibilité non prouvée doivent refuser un
  portail `>255` avant la mutation serveur.
- Cet état portail est éphémère et ne touche aucun codec D2S/D2I : aucune
  migration de sauvegarde n'est requise. Le gate reverse engineering est
  **PASS**; implantation, runtime et release restent **non autorisés** sans des
  gates `GO` séparés.

### Gate implantation Town Portal 1023 local/offline — 4 septembre 2026

- Vincent a autorisé `GO implantation Town Portal 1023 local/offline`.
  Extended Act Level IDs passe à `2.1.0`; aucun déploiement, lancement du jeu,
  save, archive, commit ou push n'appartient à ce gate.
- Le hook `D2GAME_CreateLinkPortal 0x435DD0` capture la paire dynamique classe
  `59`. Le sidecar copy-on-write enregistre pour chaque moitié la génération,
  le `Game*`, son GUID, le GUID réciproque, le full destination ID et le low
  byte natif. Les lookups refusent toute paire périmée, non réciproque ou
  incohérente.
- Le getter `DUNGEON_GetLevelIdFromRoom 0x2EFC10` présente le low byte au code
  stock uniquement pendant cette création et seulement au retour exact
  `0x43605F`. Le garde et le setter byte restent intacts; aucun assert n'est
  NOPé et aucune structure native n'est élargie. La création étendue exige
  aussi le propriétaire `LocalPlayerReady` et un contrat de session sain; tout
  prérequis manquant ou échec de publication du sidecar refuse avant l'appel
  natif non scopé et empoisonne le trafic portail classe `59` de cette session.
- `SUNIT_GetPortalOwner 0x490070` réinjecte le full ID dans le résolveur d'acte
  sous TLS, puis vérifie le GUID owner réellement obtenu. L'opération
  `0x58F680` n'utilise ensuite le full ID qu'après cette validation et seulement
  pour l'identité `LocalPlayerReady`.
- L'audit de coexistence a rejeté les hooks globaux envisagés sur
  `DATATBLS_GetLevelsTxtRecord 0x32C4A0` et
  `DATATBLS_GetLevelDefRecord 0x32C200`, car MapSense vérifie et appelle
  directement `0x32C200`. Les entrées restent byte-exactes; seuls les deux
  calls portail uniques `0x58F819` et `0x58F8EE` sont redirigés par des relays
  proches.
- Les paquets `0x51` et `0x60` gardent leurs tailles stock. Le full ID est
  marqué dans X uniquement pour le client local, puis décodé dans une copie
  avec X restauré avant le handler original. Les opérations et paquets
  distants sont refusés; TCP hôte/joiner et Battle.net ne sont pas revendiqués.
- Les treize hooks, deux callsites et témoins ABI/layout ont chacun une seule
  occurrence exacte dans le corpus commun. Deux builds Release propres passent
  `CTest 1/1` sans warning et sont byte-identiques : 78 336 octets, SHA-256
  `1803A73E0894C2A8916DD5BD32793525E4F795B13652CFC488786247AB6045B6`.
  Les métadonnées sont `RuffnecKk / 2.1.0`; les exports restent les trois points
  D2RLoader et les dépendances Windows/MSVC attendues.
- **Verdict : implantation statique PASS.** Le prochain gate recommandé est le
  runtime local/offline exact : création depuis Level 256, traversée Level 256
  → Harrogath, retour Harrogath → Level 256, nettoyage de la paire et lecture
  des nouveaux compteurs. Save/reload et réseau restent séparés.

### Gate runtime Town Portal 1023 local/offline 2.1.0 — 4 septembre 2026

- Vincent a autorisé `Go runtime town portal 123 local - offline`, corrigé
  immédiatement en `1023`, puis a confirmé la séquence présentée avec `go`.
  La DLL exacte 2.1.0 `1803A73E…AB6045B6` et les fixtures Level 256/1023 ont
  été déployées temporairement dans le profil BKVince mod-local.
- Le cold start officiel Battle.net D2R `3.3.93847` sous D2RLoader public
  `1.2.1` est **PASS** avec la pile complète : 192 tables TXT, 38 plugins, les
  cinq DLL eezstreet, 17 patches et startup `24/24`. Le plugin accepte ses
  empreintes et publie `Classic=137 / LoD=137 / RotW=1023`.
- La création du portail depuis Level 256 et le premier trajet vers Harrogath
  sont **PASS**. Vincent signale ensuite, avant de tenter de reprendre le
  portail, `BC_ASSERT: eLevelId > 0 && eLevelId <
  DataTablesGetNumLevels(ver)` dans `D2Common/src/DataTbls/LvlTbls.cpp:284`.
  Le retour Harrogath → Level 256, le nettoyage et les compteurs finaux sont
  donc **NOT RUN**.
- La pile native place le retour de `DATATBLS_GetLevelsTxtRecord 0x32C4A0` à
  `0xC1893`. Le callsite client exact `0xC188E` récupère auparavant
  `UNITS_GetObjectInteractType`, zéro-étend son byte puis demande le record
  `Levels`; pour la destination 256, le client stock reçoit donc `0`.
- Le log frais précède l'assertion par
  `extended Town Portal state packet refused outside the local codec
  contract`. Le caller `0x5388CA` prouve que l'argument R8B du sender `0x60`
  est le low byte du niveau de la room du **joueur propriétaire**, alors que
  le paquet `+2` vient de l'`InteractType` destination du portail. Ces deux
  valeurs ne sont pas un invariant d'égalité. Les gardes 2.1.0
  `linkedLevelId == endpoint.nativeLowLevelId` et client
  `packet[0x0B] == packet[2]` sont donc invalides.
- Les handlers clients 2.1.0 décodent le full ID et restaurent X dans une copie
  du paquet, mais ne publient aucun sidecar client. Le consommateur UI
  `0xC188E` reste ainsi hors des scopes serveur et déclenche l'assertion sur le
  low byte tronqué. Hooker globalement `0x32C4A0` demeure rejeté; MapSense et
  les 43 autres callsites doivent conserver l'entrée partagée byte-exacte.
- Le jeu a été fermé sans `Continue`. Les neuf fichiers `QtyTester` sont
  restaurés 9/9 à leurs hashes pré-gate; les deux tables et la DLL sont
  restaurées byte-exact dans la source et le runtime, la copie globale est
  restée intacte et aucun processus Diablo ne demeure.
- Les preuves fraîches sont sous
  `analysis-cache/extended-act-level-ids-v2/runtime/20260904-town-portal-1023-local-offline-2.1.0/`.
  Le sous-dossier `historical-crash-not-current` contient uniquement deux
  rapports anciens relabellisés; aucun crash report frais n'a été généré par
  l'assertion courante.
- **Verdict : FAIL.** La 2.1.0 ne doit pas être redéployée. Le prochain gate
  recommandé est un census strictement read-only du sidecar client alimenté
  par `0x51/0x60` et des consommateurs portail, en commençant par le callsite
  UI `0xC188E`; aucune implantation ni nouvelle relance n'est autorisée par ce
  résultat.

## Décision de reprise autoritaire — 1er septembre 2026

- Le produit devient définitivement une DLL autonome RuffnecKk membre de la
  RuffnecKk D2RLoader Suite. L'ancienne destination `plugin-levels.dll`, la
  catégorie `levels` et la clé `levels.extendedActLevelIds` sont supersédées;
  aucun merge présent ou futur dans une DLL d'eezstreet n'est planifié.
- La DLL porte le nom `d2rl-ruffneckk-extended-act-level-ids.dll`, l'auteur
  exact `RuffnecKk` et la description courte
  `Allows new levels to belong to any act.`. Elle reste hybride globale ou
  mod-locale et ne déclare pas `ModScopedOnly`.
- Le mécanisme retenu hooke le résolveur central d'acte, recherche la ligne
  `Levels` autoritaire par Level ID et retourne son champ `Act` lorsque la
  valeur est comprise entre `0` et `4`. Une ligne absente ou invalide reprend
  le résultat original au lieu d'inventer un acte.
- La configuration JSON `enabled` de `0.1.0` est supersédée. Elle n'exposait
  aucun réglage moddeur réel et dupliquait la présence de la DLL; `0.1.1` retire
  donc le fichier, le parseur, la dépendance `nlohmann/json`, les chemins de
  recherche et la branche `enabled`. La présence de la DLL active le plugin;
  son retrait le désactive au prochain démarrage.
- La baseline d'incubation est D2R `3.3.93847`, D2RLoader `1.2.0-beta` et
  PluginSDK API v3 au commit
  `4933e2c42cb2592958cd0df3b6dc5003102252d1`. Le build `3.2.92777` est couvert
  seulement par l'équivalence byte-exact gouvernée de toutes les surfaces
  natives effectivement utilisées.

## Historique de planification supersédé

Les décisions ci-dessous expliquent l'ordre de travail de juillet. Elles ne
réouvrent ni une destination PluginPack ni la pause levée par le `GO` du
1er septembre 2026.

- Vincent a confirmé la catégorie `levels` le 26 juillet 2026.
- La destination future est `plugin-levels.dll` et la clé prévue dans l’unique
  `D2RPlugins.json` est `levels.extendedActLevelIds`.
- Pendant l’incubation, conserver une DLL autonome hybride
  `ExtendedActLevelIds.dll`, attribuée exactement à `RuffnecKk`, sans TOML et
  sans modifier, lier ni redistribuer une DLL d’eezstreet.
- Le 30 juillet 2026, Vincent retient l’Option B du chantier Waypoint Expansion.
  Elle remplace l’ancien démarrage après RemoteStash pour l’ordre de travail :
  terminer les cinq lots gameplay PluginPack et la revue eezstreet, partager
  ensuite la recherche `ActInfo`/waypoints entre les deux missions, implanter
  Waypoint Expansion en premier et conserver cette mission en pause jusqu’à une
  décision distincte.
- Description anglaise prévue : `Allows new levels to belong to any act.`

## Besoin joueur et moddeur

Permettre à de nouveaux Level IDs d’appartenir aux actes 1 à 4 au lieu de
forcer toutes les nouvelles lignes à utiliser l’acte 5, sans casser la
génération des niveaux, les transitions, les waypoints, l’automap, les quêtes,
la sauvegarde ni la synchronisation client/serveur.

## Faits vérifiés

- Le workbench commun 92777/93847 est prêt; ses images canonique et d'analyse
  sont vérifiées par hash. Le runtime réellement ciblé et à qualifier est
  D2R `3.3.93847`.
- Le résolveur central à `D2R.exe+0x326710` a l'ABI observée
  `(uint8 dataContext dans CL, int32 levelId dans EDX) -> uint8 act`. Il charge
  la table compilée `ActInfo`, itère à rebours ses cinq bornes de début et
  renvoie l'index de plage; plus de cent appels directs en dépendent.
- D2MOO 1.10f confirme la sémantique historique :
  `DRLG_GetActNoFromLevelId` compare le Level ID aux seuils fixes
  `{1, 40, 75, 103, 109, 1024}` et porte déjà le commentaire d'amélioration
  proposant une recherche dans `Levels.txt`. Cette source éclaire le sens du
  code, mais aucune adresse, structure ni ABI 32 bits n'est transposée.
- D2R 3.3 fournit `actinfo.txt`, avec cinq lignes et notamment les références
  `classlevelrangestart` et `classlevelrangeend`; ces champs décrivent encore
  des plages contiguës par acte, pas l'appartenance arbitraire de chaque ligne.
- Dans les données vanilla 3.3, les Level IDs sont contigus par acte : acte 1
  `0–39`, acte 2 `40–74`, acte 3 `75–102`, acte 4 `103–108` et acte 5 à partir
  de `109`.
- BKVince ajoute les Rift Levels `138–146` avec `Act = 4`, soit l’acte 5. La
  friction est donc observée dans le mod actuel.
- La table BKVince `levels.txt` passe le lecteur TSV gouverné en CRLF avec un
  round-trip byte-exact; son champ `Act` autoritaire vaut actuellement `0..4`.
- PluginSDK API v3 expose officiellement `DataTableServiceV1`,
  `TableId::Levels`, `findRowById` et l'événement `DataTablesLoaded`. Un plugin
  peut donc copier les actes dans une mémoire dont il reste propriétaire au
  chargement des tables, sans conserver un pointeur de ligne éphémère.
- La référence D2RLPlugins épinglée est propre au commit
  `dc75b49ffbb67b887d7757ee00ee9a03bcde5d8a`; la baseline SDK retenue reste le
  commit `4933e2c42cb2592958cd0df3b6dc5003102252d1`.

## Résultat livré et preuves — 2 septembre 2026

- Le produit autonome réside sous `addons/ExtendedActLevelIds/`; son binaire
  livré à BKVince est
  `data-BKVince/d2rloader/plugins/d2rl-ruffneckk-extended-act-level-ids.dll`.
  Aucun fichier de configuration n'appartient désormais au produit.
- La DLL finale porte le SHA-256
  `16805ACA4207015729516687D1A869226569A64BCCEC0BED318E453CD08E7775`
  pour 48 640 octets.
  Deux builds Release propres sont byte-identiques; CTest passe `1/1`; les
  trois exports D2RLoader, les métadonnées PE `RuffnecKk / 0.1.1` et les
  dépendances MSVC attendues sont vérifiés.
- Le cache immuable est reconstruit après `DataTablesLoaded` pour Classic,
  LoD et RotW. Chaque ligne physique doit réussir un aller-retour
  `findRowById(Id)` vers le même pointeur et le même index; les ancres vanilla
  `1/40/75/103/109`, la taille `0x18C`, `Act=+0x0D`, les actes `0..4` et
  l'unicité des IDs sont exigés avant publication.
- La DLL seule passe les cold starts mod-local puis global sur le runtime
  officiel `3.3.93847`, avec la pile complète courante : 38 plugins, 17 memory
  patches et `24/24`. L'arbitrage du doublon avait été prouvé sur `0.1.0` mais
  n'a pas été rejoué sur `0.1.1`; il reste `not run` pour cette version.
- Un fixture TSV temporaire a ajouté `Id=147 / Act=0` hors des sources
  livrées. D2R l'a compilé (`RotW=148`) et l'entrée native hookée a journalisé
  `Level Id 147 resolved to Act index 0 (Act 1), data context 3,
  source=Levels.txt`. Le fixture a ensuite été retiré et le `levels.txt`
  runtime restauré byte-exact au hash
  `A46B5438164ADB1FB9540890103594EA48A79AFA2478CB6865D2E6DB5795EB04`.
- Les preuves locales résident sous
  `analysis-cache/extended-act-level-ids-product/evidence/`, notamment
  `20260901-1947-functional-fixture/` et
  `20260902-0851-v0.1.1-configless-matrix/`. Le fixture exact `Id=147` demeure
  une preuve `0.1.0`; son code de résolution est inchangé, mais son rejeu
  `0.1.1` attend une future autorisation de lancement.

## Handoff de test externe — 2 septembre 2026

- Vincent a autorisé la préparation d'un ZIP destiné à des testeurs externes.
  Cette archive reste explicitement une build de test et ne ferme aucun gate de
  zone jouable ni de release publique.
- L'archive locale gitignorée
  `addons/ExtendedActLevelIds/RuffnecKk-Extended-Act-Level-IDs-0.1.1-test.zip`
  contient uniquement, à sa racine, la DLL autonome.
  Le README anglais créditant D2MOO demeure à côté du ZIP et n'est pas inclus.
- Le SHA-256 du ZIP est
  `370FB1934ED6EA68CF26E9FB703A9294ABF7D7E320A5C9FE7470F6F5673DC874`.
  Son unique entrée a été relue depuis l'archive : DLL de 48 640 octets,
  SHA-256 `16805ACA4207015729516687D1A869226569A64BCCEC0BED318E453CD08E7775`.

## Risques résiduels à tester

- Le hook central couvre les 113 appels directs recensés; la fixture jouable
  Level 256 prouve maintenant l'acte, le graphe de rooms et les transitions
  physiques same-act. D'autres consommateurs spécialisés restent néanmoins à
  qualifier séparément, notamment waypoint, automap visuel et quêtes.
- Town Portal ne dépend pas seulement du résolveur d'acte. Sa construction,
  son propriétaire et son paquet d'état reposent sur
  `ObjectData.InteractType:uint8`; sans sidecar gouverné, un ID 256 est rejeté
  puis serait tronqué à zéro. NOPer le garde est explicitement interdit.
- Des appels précoces peuvent précéder la première publication du cache
  `Levels`; le code reprend alors l'original. Le test d'une zone réelle doit
  confirmer que ce repli ne crée pas de divergence observable pendant les
  transitions et la synchronisation.

## Gates observables

1. **Passé** — construire une sonde temporaire sans hook produit : obtenir les trois vues
   `Levels`, leurs tailles, révisions et candidats `Act`, puis corréler les
   banques au contexte natif sur D2R `3.3.93847`.
2. **Passé** — promouvoir dans `known-rvas.json` et `findings.md` le résolveur `0x326710`,
   son ABI, sa signature unique, son corps utile et le layout `Levels.Act`
   uniquement après cette preuve.
3. **Passé** — implanter la DLL autonome sans configuration, le cache publié
   après `DataTablesLoaded`, le fallback original et les tests unitaires des
   chemins valides, absents, précoces et invalides.
4. **Passé** — produire deux builds Release indépendants et comparer les DLL après
   normalisation; vérifier exports, métadonnées, dépendances, signature native
   exhaustive et absence d'allowlist de version.
5. **Passé** — déployer sur une allowlist fermée dans la portée mod-locale puis globale,
   avec la pile D2RLoader complète, tous les plugins eezstreet et RuffnecKk
   actifs; exécuter les cold starts et collecter des logs frais.
6. **Partiel** — la fixture Level 256 Act 4 compile 1023 records, entre en jeu,
   rend son graphe de rooms et passe les transitions physiques répétées avec
   Harrogath. Le census puis l'implantation statique `2.1.0` du sidecar serveur
   sont passés; son runtime passe la création et Level 256 → Harrogath, puis
   échoue avant le retour sur le lookup client tronqué. Le census du sidecar
   client est maintenant passé, mais sa correction n'est pas implantée.
   Waypoints, automap visuel, quêtes, sauvegarde/rechargement, souris/manette,
   solo réseau, hôte et joiner restent ouverts.
7. **Bloqué par le gate 6** — générer le ZIP public uniquement après les gates; conserver le README
   anglais créditant D2MOO à côté du ZIP et hors de l'archive.

## 4 septembre 2026 — reverse engineering du sidecar client Town Portal

- Vincent a autorisé le census read-only exact des handlers clients `0x51`,
  `0x60` et du callsite UI `0xC188E`. Le workbench commun 92777/93847, son
  image, son index et les références D2MOO/D2RL-Plugins épinglées ont été
  revérifiés. Aucun code de plugin, table, profil runtime, processus ou save
  n'a été modifié pendant ce gate.
- Le census des 28 lecteurs directs de
  `UNITS_GetObjectInteractType 0x34AD40` ne trouve que trois chemins clients.
  `0x9A39E` teste le bit de signe pour le comportement de verrouillage d'un
  objet; `0x1CB118` transmet la valeur au résolveur de shrine; seul `0xC1882`
  l'envoie à `DATATBLS_GetLevelsTxtRecord 0x32C4A0`. Le callsite UI
  `0xC188E` est donc l'unique consommateur client `Levels` directement
  alimenté par ce byte dans le corpus gouverné.
- La fonction UI réelle commence à `0xC17E0` avec l'ABI observée
  `(outputString, Unit*)`. Elle conserve `Unit*` dans le registre non volatil
  RSI, récupère la classe et le contexte de données, exige le bit de sous-classe
  `ObjectsTxt+0x127 & 4`, puis appelle le getter byte et le lookup `Levels`.
  Son contexte de 27 octets à `0xC187F` n'a qu'une occurrence. Un record valide
  fournit la clé de nom à `Levels+0xFD`; un retour nul emprunte déjà le fallback
  stock `LANG_GetStringById(0x150D)`. Le relay recommandé peut donc ajouter
  `R8=RSI` et tail-jumper vers un résolveur C++ sans toucher l'entrée partagée
  `0x32C4A0` ni ses 43 autres callsites.
- `CLIENT_HandlePacket0x51_ObjectSpawn 0x129D70` lit le GUID à `+2`, la classe
  à `+6`, X/Y à `+8/+0x0A` et l'`InteractType` à `+0x0D`, puis appelle
  `CLIENT_CreateObjectFromPacket 0x99510` à `0x129DD8`. Ce helper termine la
  création native avant d'écrire le byte réseau par
  `UNITS_SetObjectInteractType` à `0x99582`. La future interception doit donc
  évincer toute ancienne entrée du même GUID pour chaque spawn classe `59`,
  restaurer X dans une copie marquée, déléguer au handler stock, puis publier
  le full ID décodé.
- `CLIENT_HandlePacket0x60_PortalState 0x1CB1C0` résout le GUID `+3` avec
  `CLIENT_GetUnitByIdAndType 0x9A5D0` et le type objet `2`, écrit la destination
  low-byte `+2`, puis conserve séparément les coordonnées du propriétaire
  `+7/+9` et son niveau de room `+0x0B`. Le wrapper corrigé doit restaurer
  seulement X, appeler le handler stock, résoudre immédiatement l'unité vivante
  et exiger type `2`, classe `59`, GUID et low byte concordants avant de
  rafraîchir le sidecar. `packet[0x0B]` ne participe jamais à ce contrat.
- Le census des 30 écritures directes de
  `UNITS_SetObjectInteractType 0x34E9D0` ne révèle que trois écritures clientes :
  la création réseau `0x99582`, l'état `0x60` à `0x1CB1E9` et l'initialisation
  générique d'objet à `0x1CB5D3`. Cette dernière appartient au constructeur
  commençant à `0x1CB410` et précède l'écriture explicite du paquet dans
  `CLIENT_CreateObjectFromPacket`; elle n'impose pas un troisième hook.
- Le sidecar client retenu est séparé de la paire serveur et contient seulement
  `{sessionGeneration, portalGuid, destinationLevelId, nativeLowLevelId}`.
  Sa publication copy-on-write reste bornée à 1024 entrées, ses lectures sont
  lock-free, toute allocation ou incohérence empoisonne le contrat portail de
  la session, et `GameJoined`/`GameLeft` effacent tout. Aucun `Unit*` n'est
  conservé : `CLIENT_GetUnitByIdAndType` sert uniquement à une revalidation
  immédiate.
- Au callsite UI, un portail classe `59` avec une entrée saine et concordante
  appelle le lookup stock avec le full ID, après validation du record dans le
  contexte courant. Une entrée présente mais incohérente, un contrat empoisonné
  ou le cas `low=0` sans sidecar retourne `nullptr` afin d'emprunter le fallback
  stock plutôt que d'asserter. Une classe non portail, ou un portail vanilla
  non marqué avec low byte non nul et sans sidecar, garde exactement le lookup
  original.
- L'architecture globale sur `0x32C4A0` reste rejetée : elle modifierait 44
  callsites et entrerait en conflit avec MapSense. Élargir `ObjectData` ou les
  paquets reste rejeté pour incompatibilité ABI/protocole. Masquer seulement
  l'assertion reste rejeté parce que `256 -> 0` ne restaure aucune identité.
  Le sidecar client et le relay exact `0xC188E` sont le mécanisme le plus étroit,
  réversible et compatible avec les hooks `0x51/0x60` déjà possédés par le
  plugin.
- D2MOO au commit `19019806df7f3e877fa105b05395d1e3597e2316` confirme
  seulement la sémantique historique du paquet `0x51`, du paquet `0x60` et de
  `SUNIT_GetPortalOwner`; aucune adresse ni ABI 32 bits n'est transposée. La
  référence D2RL-Plugins épinglée à
  `dc75b49ffbb67b887d7757ee00ee9a03bcde5d8a` ne contient aucun propriétaire
  de `DATATBLS_GetLevelsTxtRecord`, `0xC188E` ou d'un hook portail concurrent.
- Verdict du gate : **PASS statique**. Il ferme le mécanisme client
  local/offline, mais ne corrige pas la DLL 2.1.0 et ne qualifie aucun runtime,
  réseau ou save. Le retrait des faux invariants `0x60`, l'implantation du
  sidecar client et le relay UI exigent un nouveau `GO`.

## Autorisation d'implantation du sidecar client — 4 septembre 2026

- Vincent a donné explicitement
  `GO implantation sidecar client Town Portal 0x51/0x60 + UI 0xC188E local/offline`.
- Le lot reste la DLL autonome RuffnecKk Extended Act Level IDs, hybride
  globale/mod-locale et sans configuration. Il est strictement borné au
  sidecar client, à la correction des deux faux invariants `0x60`, au relay UI
  exact et à leurs tests/builds statiques.
- Ce `GO` n'autorise ni déploiement, ni lancement du jeu, ni accès aux saves,
  ni qualification TCP/Battle.net, ni packaging, ni commit/push.

## Résultat de l'implantation du sidecar client — 4 septembre 2026

- Extended Act Level IDs passe à `2.1.1`. Les comparaisons fautives entre le
  champ destination `0x60 +2` et le low byte de room propriétaire en R8B / à
  `+0x0B` sont retirées; ce dernier champ reste transmis sans altération.
- Un cache client immutable distinct publie
  `{sessionGeneration, portalGuid, destinationLevelId, nativeLowLevelId}`
  après délégation des copies `0x51/0x60` au handler stock. Il est copy-on-write,
  lu sans verrou, borné à 1024 entrées, évince tout GUID classe `59` réutilisé,
  se vide au changement de session et ne conserve jamais de `Unit*`.
- Le wrapper `0x60` re-résout immédiatement le portail avec
  `CLIENT_GetUnitByIdAndType 0x9A5D0` et exige type `2`, classe `59`, GUID et
  low byte concordants avant publication. Allocation, overflow, paquet marqué
  malformé ou incohérence vivante empoisonnent le contrat portail de la
  session.
- Le troisième relay proche cible uniquement le call UI `0xC188E`; ses octets
  `49 89 F0` transmettent `R8=RSI`, donc le `Unit*` vivant, avant le saut vers
  le résolveur client. L'entrée partagée `0x32C4A0` et ses 43 autres callsites
  restent inchangés. Le lookup UI valide génération, GUID, type/classe, low
  byte, contexte et record complet; sinon il renvoie `nullptr` vers le fallback
  stock au lieu d'asserter.
- Les nouvelles empreintes exactes de `0x9A5D0`, `0xC187F` et `0xC188E` ont
  chacune une seule occurrence dans le corpus commun 92777/93847 et sont
  vérifiées fail-closed au chargement.
- Les tests couvrent récupération des IDs `256` et `1022`, repli vanilla,
  `low=0` sans sidecar, mismatch, génération périmée, réutilisation/éviction de
  GUID, capacité exacte 1024, overflow, purge de session et passthrough hors
  portail. Ils interdisent aussi le retour des deux faux invariants `0x60`.
- Deux builds Release propres MSVC 19.44 / Windows SDK 10.0.26100.0 passent
  `CTest 1/1` sans warning et sont byte-identiques : 88 576 octets, SHA-256
  `DC1DEFC82D8B62F3F33859D2D57B621F8802CFEFAE14644D5B5884179E5887F6`.
  Les métadonnées portent `RuffnecKk / 2.1.1`, avec exactement les trois
  exports D2RLoader et les dépendances Windows/MSVC attendues.
- Les candidats restent locaux sous
  `analysis-cache/extended-act-level-ids-v2/town-portal-client-final-a/Release/`
  et `town-portal-client-final-b/Release/`. Aucun déploiement, processus D2R,
  table, save, ZIP, commit ou push n'a été touché. La ROADMAP reste inchangée
  faute de confirmation spécifique.

## Prochain gate

Après un `GO` runtime séparé, déployer temporairement la candidate 2.1.1 et la
fixture Level 256 dans le profil BKVince mod-local complet : créer le portail
depuis Level 256, traverser vers Harrogath, vérifier le label sans assertion,
reprendre le portail vers Level 256, nettoyer la paire et capturer les compteurs
client publication/éviction/full lookup/fallback/refus. Restaurer ensuite DLL,
tables et personnage byte-exact. Save/reload, waypoint, automap et tous les
rôles réseau restent des gates distincts; la release et son ZIP restent
bloqués.
