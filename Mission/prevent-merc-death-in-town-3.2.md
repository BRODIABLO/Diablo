# Prevent Merc Death in Town — D2R 3.2.92777

Dernière mise à jour : 24 juillet 2026

Statut : prototype autonome hybride `PreventMercDeathInTown.dll` 0.1.0 et JSON
implantés, compilés en Release et validés fonctionnellement par le testeur
externe le 27 juillet 2026. Le lot est prêt à être intégré dans
`plugin-misc.dll` sous `misc.preventMercDeathInTown`.

## Décisions confirmées

- Vincent a confirmé le 24 juillet 2026 que la mort d’un mercenaire en ville
  sous poison, Open Wounds ou autre dégénérescence persistante est un fait, et
  non un bug hypothétique ni une théorie à valider.
- Le nom exact de la mission est `Prevent Merc Death in Town`.
- Vincent avait retenu l’Option B, puis a demandé `Commence` le 24 juillet 2026,
  ce qui a autorisé l’implantation du prototype. La mission est maintenant mise
  en pause à son gate runtime pendant Repair Costs Cap.
- Vincent confirme la catégorie PluginPack `misc` le 24 juillet 2026, avec
  `plugin-misc.dll` comme DLL propriétaire future et
  `misc.preventMercDeathInTown` comme clé prévue.

## Résultat joueur attendu

Un mercenaire vivant qui entre ou se trouve dans une zone de ville ne peut pas
mourir à cause d’un dégât persistant encore actif. La protection ne doit pas
guérir arbitrairement le mercenaire, altérer le joueur, protéger les autres
serviteurs ou rester active après la sortie de ville.

## Fait établi et périmètre de preuve

- Fait produit : un mercenaire peut mourir en ville lorsqu’un état de poison,
  Open Wounds ou une autre dégénérescence continue d’infliger des dégâts.
- La phase de reproduction ne sert pas à décider si le fait existe; elle sert à
  construire des témoins déterministes, mesurer chaque famille de dégâts et
  produire une matrice de non-régression.
- Les mécanismes natifs exacts du build 92777, le chemin autoritaire du tick de
  dégâts, la détection de ville, les structures, l’ABI et le point
  d’intervention restent à prouver avant tout hook.

## Plan de match

1. Capturer des témoins déterministes pour poison, Open Wounds et les autres
   dégâts persistants pertinents, avec PV, zone et transition vers la ville.
2. Comparer le chemin du mercenaire à la protection déjà observée pour le
   joueur, sans supposer qu’ils partagent le même appel natif.
3. Exécuter le gate du workbench 92777, identifier le chemin serveur qui applique
   le tick létal et prouver fonction, callers, ABI, signatures et préconditions.
4. Auditer `plugin-misc` au commit officiel épinglé et attribuer un propriétaire
   unique à chaque hook ou structure partagée avant de coder.
5. Prototyper une protection minimale, autoritaire et strictement bornée au
   mercenaire vivant du joueur dans une zone de ville. La politique initiale à
   évaluer est un plancher de survie à 1 PV, sans guérison automatique.
6. Confirmer que la protection cesse hors ville et qu’elle ne touche ni joueur,
   mercenaire déjà mort, monstres, invocations, familiers ou autres unités.
7. Valider les transitions par portail et waypoint, les changements d’acte,
   solo, hôte/joiner, sauvegarde/rechargement et retour au combat.
8. Après validation fonctionnelle, appliquer les gates hybride global/mod-local,
   coexistence PluginPack, cold start, logs, hashes et archive publique stricte.

## Incubation envisagée

- Description anglaise prévue :
  `Prevents mercenaries from dying to lingering damage while in town.`
- La catégorie `misc` étant confirmée, conserver pendant l’incubation une DLL
  autonome hybride attribuée exactement à `RuffnecKk`, sans `ModScopedOnly`.
- Utiliser uniquement un JSON autonome compatible PluginPack si une option
  joueur réelle est justifiée; ne créer aucun TOML.
- Après un merge futur, intégrer la fonctionnalité à `plugin-misc.dll`, déplacer
  ses options sous `misc.preventMercDeathInTown`, puis supprimer la DLL et le
  JSON autonomes.

## Audit initial

- Le gate `npm.cmd run re:d2r32 -- status` est vert : images canonique et
  d’analyse, index SQLite et projet Ghidra persistant du build 92777 sont
  vérifiés; aucun redump ni réimport n’a été effectué.
- La référence officielle `D2RL-Plugins@dc75b49ffbb67b887d7757ee00ee9a03bcde5d8a`
  est propre et vérifiée. `plugin-misc` charge la section `misc` de l’unique
  JSON (`src/plugin-misc/misc-main.cpp:136-140`) et ne possède que les callsites
  `/players` `0x18885B`/`0x18887F` ainsi que le hook
  `GAME_GetPlayerCountBonus` `0x542F40` (`misc-main.cpp:5-40,142-158`). Aucun de
  ces sites ne chevauche le chemin périodique étudié autour de `0x448C21`.
- Le patch gouverné `Thorns and Burn Kill Credit` identifie déjà `0x448DCA` et
  `0x448DE5` dans la fonction `0x448C21-0x448E14`. Ce chemin applique un delta
  de PV, détecte le passage létal, puis parcourt les états poison `2`, Open
  Wounds `0x3E` et burning `0x73` afin d’en retrouver le propriétaire avant la
  logique de mort. Cette fonction constitue le point de départ prouvé, sans que
  son nom canonique ni son ABI complète soient encore promus.
- Dans ce chemin, `0x34AB60` est un getter unique du champ `unit+0x0C`; la
  séquence compare sa valeur à `0` puis `12`. Cela ne prouve ni la ville ni le
  statut de mercenaire et ne doit pas être utilisé comme tel sans xrefs et
  preuves supplémentaires.
- La branche unique `0x448CEC` prouve la faiblesse du garde de ville : pour un
  `HPREGEN` négatif, elle teste les PV courants contre `0x100` avant d’ajouter le
  delta. Si les PV courants valent au moins 1 mais que le tick les fait passer
  directement sous 1, le garde room/town n’est jamais consulté et la valeur
  résultante est ramenée à zéro. La signature de 46 octets couvrant test du
  delta, seuil `0x100`, résolution room/town et addition ne possède qu’un match
  dans l’image 92777.
- La correspondance sémantique D2MOO est exacte dans
  `source/D2Game/src/MONSTER/MonsterMode.cpp:575-617` :
  `D2GAME_MONSTER_ApplyStatRegen` teste également `current HP < 256` et la ville
  avant de calculer `new HP`, puis ramène tout résultat inférieur à 1 à zéro.
  Le correctif doit donc protéger le résultat projeté du mercenaire en ville,
  sans neutraliser globalement son `HPREGEN` négatif.
- D2MOO épinglé fournit seulement des références sémantiques :
  `DUNGEON_IsTownLevelId` apparaît notamment dans
  `source/D2Common/src/D2Dungeon.cpp:985-987`, tandis que le calcul Open Wounds
  est documenté dans `source/D2Game/src/SKILLS/SkillItem.cpp:1296-1343`. Aucune
  adresse, structure ou ABI 1.10f n’est transposée vers 92777.
- L’entrée réelle est `0x448C00`, avec l’ABI x64
  `(game, unit, a3, a4) -> void`. Le prototype valide son prologue strict de 21
  octets avant d’installer son hook; tous les cas hors cible appellent le
  trampoline vanilla.
- La classification native des mercenaires est fermée par deux preuves :
  D2MOO `MONSTERS_GetHirelingTypeId` classe seulement les cinq familles, et les
  tables BKVince 3.2 byte-exactes donnent `classId` 271, 338, 359, 560 et 561.
- `0x34B440` est promu comme `UNITS_GetRoom`; `0x2F0750` est promu comme
  `DUNGEON_IsRoomInTown`. Ce dernier suit `room+0x18`, puis retourne le champ
  `Town` du record de niveau via `0x360FC0` (`level record + 0x1F8`).
- Le prototype recalcule le `HPREGEN` effectif comme vanilla, teste les PV
  projetés, et n’intercepte que le tick négatif létal d’un des cinq mercenaires
  dans une room de ville. Il appelle ensuite `EVENT_SetEvent` `0x48B720` pour
  reprogrammer `STATREGEN` à `gameFrame + 1`; il ne guérit pas et ne fige pas
  l’expiration de l’effet persistant.
- Le build Release gouverné passe avec le SHA-256
  `D49166D33B2DEA7BCECE0F972692802AE8BC4D48A1B15FF811FA2A10329C8980`.
  Les trois exports requis, la section ressource, le binaire x64 et les
  métadonnées RuffnecKk 0.1.0 sont contrôlés sur l’artefact synchronisé.
- Le paquet de test externe
  `addons/PreventMercDeathInTown/PreventMercDeathInTown-0.1.0-test.zip` contient
  uniquement `PreventMercDeathInTown.dll` et `PreventMercDeathInTown.json` à sa
  racine. SHA-256 du ZIP :
  `152C54B88360A2B655837F8D88714CBB443ADD9D4DC3A4F3EA1DB48202EE636F`.
  La DLL distribuée est byte-identique au build Release et au dépôt, SHA-256
  `D49166D33B2DEA7BCECE0F972692802AE8BC4D48A1B15FF811FA2A10329C8980`;
  le JSON porte `27358CD08B39917A924115EF6BA3F5D34C14EC2F6E53A1D204C01BCE7C3F7D8C`.
  README, sources, symboles, TOML, logs, preuves et DLL eezstreet sont exclus.
- Vincent confirme le 27 juillet 2026 que le test externe fonctionne. Cette
  preuve ferme le gate fonctionnel du prototype distribué et autorise la
  préparation du merge PluginPack; elle ne remplace pas les contrôles de build,
  configuration et coexistence propres à la future DLL fusionnée.

## Gates observables

- propriétaire futur `misc` / `plugin-misc.dll` confirmé explicitement par Vincent;
- workbench 92777 et référence PluginPack épinglée vérifiés;
- poison, Open Wounds et autres dégâts persistants couverts par des témoins;
- chemin autoritaire, détection de ville, ABI et signatures strictes prouvés;
- mercenaire maintenu vivant en ville sans guérison ou immunité hors périmètre;
- sortie de ville, portail, waypoint et changement d’acte sans protection résiduelle;
- joueur, autres serviteurs et mercenaires morts inchangés;
- solo, hôte/joiner, sauvegarde/rechargement et coexistence validés;
- portées globale et mod-locale, cold start et hashes conformes avant livraison.

## Prochain gate

Intégrer la fonctionnalité dans `plugin-misc.dll` sous la clé
`misc.preventMercDeathInTown`, préserver le crédit RuffnecKk sans remplacer les
métadonnées eezstreet, supprimer ensuite la DLL et le JSON autonomes, puis
recompiler et valider le PluginPack fusionné avec cold start, logs frais et
coexistence des autres options `misc`.
