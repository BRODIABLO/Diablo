# Extended Act Level IDs — D2R 3.3.93847

Dernière mise à jour : 1er septembre 2026

Statut : **implantation et qualification technique terminées**. Vincent a
donné `GO` le 1er septembre 2026 pour le plan autonome fondé sur
`Levels.txt → Act`. Le résolveur hors plage est prouvé au runtime; seule la
matrice d'une véritable nouvelle zone jouable reste ouverte avant release.

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
- Le contrat Suite actuel exige une configuration indépendante. Le candidat
  utilise donc `ruffneckk-extended-act-level-ids.json` avec le seul booléen
  anglais `enabled`; l'absence du fichier active le comportement retenu et un
  fichier présent mais invalide refuse le chargement avant le premier hook.
- La baseline d'incubation est D2R `3.3.93847`, D2RLoader `1.2.0-beta` et
  PluginSDK API v4 au commit
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
- PluginSDK API v4 expose officiellement `DataTableServiceV1`,
  `TableId::Levels`, `findRowById` et l'événement `DataTablesLoaded`. Un plugin
  peut donc copier les actes dans une mémoire dont il reste propriétaire au
  chargement des tables, sans conserver un pointeur de ligne éphémère.
- La référence D2RLPlugins épinglée est propre au commit
  `dc75b49ffbb67b887d7757ee00ee9a03bcde5d8a`; la baseline SDK retenue reste le
  commit `4933e2c42cb2592958cd0df3b6dc5003102252d1`.

## Résultat livré et preuves — 1er septembre 2026

- Le produit autonome réside sous `addons/ExtendedActLevelIds/`; son binaire
  livré à BKVince est
  `data-BKVince/d2rloader/plugins/d2rl-ruffneckk-extended-act-level-ids.dll`
  et sa configuration indépendante est
  `data-BKVince/d2rloader/config/ruffneckk-extended-act-level-ids.json`.
- La DLL finale porte le SHA-256
  `5A61181135A18039E3362ECDBD2A8972155C904A6B7B09AABC5611BE49E1FCD4`.
  Deux builds Release propres sont byte-identiques; CTest passe `1/1`; les
  trois exports D2RLoader, les métadonnées PE `RuffnecKk / 0.1.0` et les
  dépendances MSVC attendues sont vérifiés.
- Le cache immuable est reconstruit après `DataTablesLoaded` pour Classic,
  LoD et RotW. Chaque ligne physique doit réussir un aller-retour
  `findRowById(Id)` vers le même pointeur et le même index; les ancres vanilla
  `1/40/75/103/109`, la taille `0x18C`, `Act=+0x0D`, les actes `0..4` et
  l'unicité des IDs sont exigés avant publication.
- La matrice du hash final passe avec la pile complète : portée mod-locale,
  portée globale, arbitrage du doublon, `enabled=false`, JSON invalide et
  restauration mod-locale. Les cold starts valides chargent 37 plugins et 17
  memory patches jusqu'à `24/24`; le JSON invalide refuse seulement cette DLL
  et D2R termine quand même son démarrage.
- Un fixture TSV temporaire a ajouté `Id=147 / Act=0` hors des sources
  livrées. D2R l'a compilé (`RotW=148`) et l'entrée native hookée a journalisé
  `Level Id 147 resolved to Act index 0 (Act 1), data context 3,
  source=Levels.txt`. Le fixture a ensuite été retiré et le `levels.txt`
  runtime restauré byte-exact au hash
  `A46B5438164ADB1FB9540890103594EA48A79AFA2478CB6865D2E6DB5795EB04`.
- Les preuves locales résident sous
  `analysis-cache/extended-act-level-ids-product/evidence/`, notamment
  `20260901-1947-functional-fixture/` et
  `20260901-1949-final-matrix/`.

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
3. **Passé** — implanter la DLL autonome, sa configuration JSON stricte, le cache publié
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
