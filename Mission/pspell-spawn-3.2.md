# PSpell Spawn — D2R 3.2

Mise à jour : 30 juillet 2026
Statut : planifié — aucune implantation autorisée avant le séquencement et les preuves natives

## Décisions confirmées

Vincent confirme le 29 juillet 2026 que PSpell Spawn sera un **plugin autonome
permanent**. Il ne possède aucune catégorie PluginPack, DLL propriétaire future,
clé de merge ou trajectoire de merge planifiée.

La cible produit est une `PSpellSpawn.dll` attribuée exactement à `RuffnecKk`,
hybride globale/mod-locale, compatible avec les cinq DLL du PluginPack et dotée
d'une configuration indépendante de `D2RPlugins.json`. Le choix JSON ou TOML
reste ouvert jusqu'à une comparaison ergonomique du vrai catalogue de spawns;
le contenu et les commentaires devront être entièrement en anglais, avec
recherche dans le mod actif avant repli global et refus explicite d'une
configuration présente mais invalide.

Vincent retient l'**Option A** le 30 juillet 2026. Le chantier commencera après :

1. les cinq lots de smoke tests gameplay et la revue eezstreet du PluginPack;
2. la stabilisation du fallback autonome et du propriétaire de hook de
   Readable Items;
3. la preuve puis le gel de l'ABI du PSpell Framework / UseSkill, propriétaire
   unique du dispatcher `pSpell` et du registre partagé;
4. l'incubation D2RedPortal déjà planifiée, premier consommateur externe qui
   doit éprouver l'inscription, la consommation et l'absence de collision.

PSpell Spawn précède ensuite toute future façade inventaire `pSpell` de Book of
Lore; le noyau Tower Tome actuel conserve son trajet distinct. La mission active
demeure `Mission/eezstreet-pluginpack-integration.md` jusqu'à fermeture de son
gate.

## Besoin fonctionnel

Permettre à un moddeur d'associer un item utilisable à une définition de
rencontre. Lors de l'utilisation, le serveur valide le joueur, l'item, la zone
et la configuration, crée les monstres sur des cellules libres, puis consomme
l'item seulement selon la politique de succès configurée.

Le premier périmètre recommandé reste volontairement borné : monstres normaux
explicitement nommés, quantité plafonnée, zones autorisées, villes refusées,
RNG serveur et consommation sur succès. Les packs champions/uniques et les
SuperUniques constituent des phases distinctes après preuve de leurs helpers
d'initialisation, règles de quête, drops et unicité par partie.

## Faits vérifiés

- L'archive historique `D2Spawn.rar` contient deux DLL PE32 x86 de 4 Ko,
  `D2Spawn.txt` et un README, mais aucun source ou licence permettant un port
  binaire. `D2Spawn.dll` et `NewTxt.dll` ne seront ni repris ni redistribués.
- Le comportement historique choisit un `pSpell`, utilise `misc.txt.calc1`
  comme index de table et demande des SuperUniques, packs aléatoires ou
  monstres précis avec quantités et restrictions de niveau.
- Le workbench gouverné cible `D2R.exe 3.2.92777`; aucune adresse ou ABI LoD
  1.10 n'est transposable.
- `D2ItemsTxt.pSpell` est situé à `+0x94` dans la structure partagée prouvée.
- Les données vanilla 3.2 et BKVince utilisent déjà les valeurs positives
  `1` à `15`; le `pSpell=13` historique appartient notamment au Token of
  Absolution.
- Le témoin Readable Items a prouvé que `pSpell=-2` est rejeté côté client
  avant émission du paquet, tandis que `pSpell=14` plus un code configuré
  atteint le trajet serveur sans détourner les autres items.
- `D2GAME_HandleUseItemPacket` au RVA `0x4F40C0` et
  `SUNIT_GetServerUnit` au RVA `0x48FE80` sont bornés dans la mission Readable
  Items. Transmogrify possède actuellement le premier hook et délègue à
  Readable Items; un second hook concurrent est interdit.
- Aucune primitive complète de création de monstre 92777 n'est encore promue
  dans `known-rvas.json`.

## Hypothèses à tester

- Le PSpell Framework pourrait transporter ce consommateur sans exiger un
  emplacement positif exclusif; `pSpell=16` reste toutefois un témoin utile à
  tester et non une réservation acquise.
- Un helper serveur de haut niveau devrait pouvoir créer un monstre avec ses
  statistiques, son IA et sa synchronisation réseau sans appel direct fragile
  à un allocateur d'unité bas niveau.
- L'ABI versionnée du PSpell Framework peut accueillir PSpell Spawn comme
  consommateur délégué sans modifier ni lier une DLL d'eezstreet.
- Une consommation après succès peut être rendue atomique ou, à défaut,
  gouvernée par un seuil explicite de spawns réussis.

Ces hypothèses ne constituent pas des preuves et n'autorisent aucune
implantation.

## Inconnues bloquantes

- mode de transport final attribué par le registre; acceptation client et paquet
  de `pSpell=16` encore inconnus mais non requis si le registre virtualise un
  transporteur déjà prouvé;
- RVA, signature, ABI et contrat du helper de spawn de monstres 92777;
- recherche de cellule libre, initialisation de niveau, IA, drops et réseau;
- helpers distincts requis pour un pack champion/unique ou un SuperUnique;
- consommation/suppression native d'un item et synchronisation de quantité;
- comportement en cas de succès partiel, zone invalide ou absence de cellule;
- coexistence exacte avec Transmogrify, Readable Items, D2RedPortal et les cinq
  DLL eezstreet;
- politique multijoueur, sauvegarde/rechargement et prévention de l'abus
  XP/loot.

## Architecture recommandée à prouver

1. Enregistrer le consommateur auprès de l'ABI versionnée du PSpell Framework,
   sans poser un second hook sur le dispatcher ni sur `0x4F40C0`.
2. Résoudre l'item par code et exiger une entrée de configuration exacte.
3. Valider côté serveur propriétaire, difficulté, niveau, zone, plafond par
   activation et plafond par partie.
4. Résoudre les monstres par identifiants textuels stables des tables actives,
   jamais par intervalles de `hcIdx` fragiles.
5. Chercher des positions libres autour du joueur et appeler uniquement un
   helper natif de haut niveau dont le contrat 92777 est prouvé.
6. Consommer l'item selon la politique de succès, puis laisser le runtime
   synchroniser les unités et la quantité aux clients.
7. Déléguer intégralement tout item non configuré au comportement original.

Les monstres créés sont des unités temporaires de la partie et ne doivent pas
être persistés dans la sauvegarde. La consommation de l'item, elle, doit rester
cohérente après sauvegarde/rechargement. En TCP/IP, l'hôte reste autoritaire et
les clients doivent charger des données de mod compatibles.

## Gates avant code

1. Exécuter `npm.cmd run re:d2r32 -- status` et refuser toute image non vérifiée.
2. Vérifier la référence PluginPack épinglée et inventorier les cinq DLL.
3. Vérifier l'ABI PSpell Framework gelée et prouver l'inscription du sélecteur
   PSpell Spawn; tester `pSpell=16` comme témoin sans en faire une dépendance
   silencieuse ni réutiliser `14` hors du contrat du registre.
4. Identifier le helper de spawn, ses callers, ses octets attendus, son ABI et
   toutes les structures lues ou écrites.
5. Prouver séparément placement libre, initialisation et consommation.
6. Conserver Transmogrify comme propriétaire unique de `0x4F40C0`, le PSpell
   Framework comme propriétaire du dispatcher plus étroit et PSpell Spawn comme
   simple consommateur enregistré.
7. Définir le contrat minimal et comparer concrètement JSON et TOML avant de
   choisir le format permanent.

## Gates d'implantation et de livraison

- manifeste v2, trois exports, auteur `RuffnecKk` et description anglaise courte;
- build Release x64 strictement limité à 92777 avec signatures complètes;
- configuration absente, valide et invalide; priorité mod actif puis repli global;
- monstres normaux, quantités bornées, positions libres/occupées, villes et
  zones autorisées/interdites;
- consommation réussie, échec total, succès partiel et répétitions;
- non-régression de tous les `pSpell` vanilla et des items non configurés;
- solo, hôte, joiner, sauvegarde/rechargement et absence de duplication;
- portées globale et mod-locale, coexistence avec PSpell Framework,
  Transmogrify, Readable Items, D2RedPortal et les cinq DLL du PluginPack;
- hashes identiques build/dépôt/runtime et cold starts sans rejet ni échec;
- ZIP public limité à `PSpellSpawn.dll` et son unique configuration indépendante.

## Prochain gate

Attendre la fermeture de la revue PluginPack, la stabilisation de Readable
Items, le gel de l'ABI PSpell Framework et l'incubation D2RedPortal. Reprendre
ensuite par les statuts gouvernés du workbench et de la référence PluginPack,
puis prouver l'inscription, le helper de spawn et la consommation avant toute
création de DLL ou configuration.
