# Extended Act Level IDs — D2R 3.3.93847

Dernière mise à jour : 4 septembre 2026

Statut : **v2 implantée et validée hors jeu; qualification runtime ouverte**.
Vincent a donné `GO` le 4 septembre 2026 pour étendre le produit existant à la
limite native de 1023 records. La source 2.0.0, ses cinq hooks fail-closed et
son codec réseau compatible sont compilés; les preuves statiques sont
gouvernées. Aucun binaire v2 n'a été déployé ni exécuté dans D2R. La matrice
d'une véritable zone jouable d'ID supérieur à 255, Town Portal, waypoint,
save/reload et host/joiner reste ouverte avant toute archive ou release. Les
résultats 0.1.1 ci-dessous demeurent l'historique qualifié de la première
fonction `Levels.txt → Act`, pas une preuve runtime de la v2.

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

- Le hook central couvre les 113 appels directs recensés et le fixture 147
  prouve la correction du résolveur lui-même. Une vraie zone jouable doit
  encore vérifier qu'aucun consommateur critique ne recalcule directement
  l'acte depuis les plages `ActInfo`.
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
6. **Partiel** — le fixture technique `Id=147 / Act=0`, sa compilation et sa
   résolution centrale vers l'acte 1 sont passés et restaurés. Reste à créer
   une zone réellement jouable, puis valider génération, transitions aller/retour,
   town/start, waypoints, automap, quêtes, portails, sauvegarde/rechargement,
   souris/manette, solo, hôte et joiner.
7. **Bloqué par le gate 6** — générer le ZIP public uniquement après les gates; conserver le README
   anglais créditant D2MOO à côté du ZIP et hors de l'archive.

## Prochain gate

Auteur une nouvelle zone de test réellement navigable avec un ID supérieur à
146 et `Act=0`, puis exécuter la matrice transitions/town/waypoint/automap/
quêtes/portails/save-reload/souris-manette/solo-hôte-joiner. La release et son
ZIP restent volontairement absents tant que cette matrice n'est pas fermée.
