# Book of Lore — D2R 3.2

Mise à jour : 29 juillet 2026
Statut : actif — jalon `0.2.0` compilé, témoin client implanté mais non déployé

## Décision de destination

Vincent confirme le 28 juillet 2026 que Book of Lore est un **plugin autonome
permanent**. Il n'a aucune catégorie, DLL propriétaire future, clé ou trajectoire
de merge PluginPack. La cible demeure une `BookOfLore.dll` RuffnecKk hybride,
installable globalement ou dans un mod et compatible avec les cinq DLL du pack,
avec son propre `BookOfLore.toml` indépendant.

Vincent confirme le 29 juillet 2026 la migration de JSON vers TOML après revue
du cas d'usage réel. TOML rend les longs textes multilignes et les commentaires
plus conviviaux pour le moddeur, tout en représentant clairement le catalogue
avec `[[messages]]` et les filtres avec `[messages.filters]`. Le fichier est
recherché dans le mod actif avant le dossier global. Une configuration présente
mais invalide est refusée sans repli silencieux; l'absence de configuration
charge des valeurs intégrées désactivées. Le parseur `toml++` est épinglé à
`v3.4.0`; aucun parseur TOML ad hoc n'est maintenu dans le plugin.

## Objectif fonctionnel

Placer des livres interactifs dans les cartes. Lorsqu'un joueur ouvre un livre,
le serveur détermine les messages admissibles, en choisit un et conserve ce
choix pour ce joueur et cette instance de livre pendant la partie. Le client ne
reçoit que l'identité du message et substitue ses variables avant l'affichage.

Le comportement historique à préserver est le suivant :

- hors ville, difficulté, acte et zone sont des seuils minimaux;
- en ville, difficulté et acte sont exacts et la zone est ignorée;
- quête accomplie, classe et niveaux restent des contraintes strictes;
- un `max_level` inférieur à `min_level` est ignoré;
- un livre d'aventure conserve son choix pendant la partie seulement;
- un livre de ville choisit de nouveau à chaque lecture;
- `all_same` partage le premier message sélectionné avec les autres joueurs qui
  remplissent ses filtres;
- `##00` à `##06` représentent le nom, la classe, le niveau, la difficulté,
  l'acte, la zone et le titre du personnage.

Aucune persistance dans la sauvegarde personnage ou entre deux parties n'est
incluse dans le contrat actuel.

## Faits techniques vérifiés

- Le workbench canonique `D2R.exe 3.2.92777` est vérifié, avec image, index et
  projet Ghidra disponibles; aucun redump n'est nécessaire.
- La référence PluginPack est épinglée au commit
  `dc75b49ffbb67b887d7757ee00ee9a03bcde5d8a`.
- `objects.txt` BKVince contient `TowerTome` (`*ID=8`, `OperateFn=6`), ainsi que
  `LamEsensTome` et `ArcaneTome`. Le handler natif Tower Tome est maintenant
  prouvé; la propriété runtime de la table `OperateFn` reste inconnue.
- `0x528270`, actuellement identifié comme
  `OBJECTS_OperateFunction32_Bank`, n'est pas Tower Tome : sa signature courte
  apparaît à douze endroits et ne peut pas servir de signature Book of Lore.
- `0x4B2BE0`, `D2GAME_PACKETCALLBACK_InteractWithEntity`, est une réception
  réseau et n'a pas été observé pour l'ouverture locale normale d'un objet. Il
  ne constitue pas un hook d'activation local valide.
- D2MOO documente sémantiquement `OBJECTS_OperateFunction06_TowerTome` pour
  Diablo II 1.10f. Ses structures, adresses et ABI 32 bits ne sont pas
  transposables à D2R 3.2.
- Le handler Tower Tome 92777 est identifié à `0x5DC570`. Son ABI observée est
  `(operation, operateMode) -> int32`, avec `operation+0x00` jeu, `+0x08` objet,
  `+0x10` joueur et `+0x20` identifiant de classe objet. Le second argument est
  inutilisé par ce handler.
- Son contrôle est spécifique : modes d'animation 0 à 2, quête A1Q5 résolue
  par `QUESTS_GetQuestData 0x518220`, animation operating, événement ENDANIM,
  message 127, puis progression de l'état Tower Tome. La signature étendue est
  unique; le prologue court ne l'est pas.
- `QUESTS_SendScrollMessage 0x517E90` construit le paquet serveur `0x27` de
  0x28 octets à partir de `(game, player, unit, uint16 stringId)`. Tower Tome
  l'appelle directement à `0x5DC61E`.
- `CLIENT_HandleScrollMessagePacket27 0x19A630` lit le GUID objet et le
  `stringId` à `packet+0x0A`, puis appelle
  `CLIENT_ShowScrollMessageById 0x197BF0`. Cette dernière fonction résout le
  texte par `LANG_GetStringById 0x5F4A50` et construit l'UI native défilante.
  Les quatre entrées critiques possèdent des signatures strictes uniques.

## Jalons implantés — 0.1.0 et 0.2.0

Le scaffold `0.1.0` sous
`data-BKVince/d2rloader/plugins/BookOfLore-src/` fournit :

- le manifeste D2RLoader v2 et les trois exports attendus;
- l'auteur exact `RuffnecKk` et une description anglaise courte;
- un parseur TOML strict avec commentaires, textes multilignes, limites, clés inconnues et doublons
  refusés;
- la priorité mod-local puis globale, avec valeurs intégrées désactivées;
- les filtres historiques, la mémoire éphémère par livre/joueur, `all_same` et
  les substitutions `##00` à `##06`;
- la commande console `book-of-lore status`;
- aucun hook D3D12, Present, Windows, souris ou clavier.

Le jalon `0.2.0` ajoute un témoin client fail-closed, installé uniquement si
`enabled=true` :

- hook inline strict de `CLIENT_HandleScrollMessagePacket27 0x19A630`;
- interception limitée au type objet `2`, menu ordinaire `0`, au moins un
  message et identifiant Tower Tome vanilla `127`;
- portée thread-local active seulement pendant le traitement original du paquet;
- redirection du seul appel interne `0x197C5F` de l'UI scroll vers un relais
  proche, sans hook global de `LANG_GetStringById`;
- substitution par le premier message configuré, titre et texte séparés par une
  ligne vide;
- signatures exactes du handler paquet, de l'UI scroll, du callsite et de la
  primitive de localisation avant toute mutation;
- compteurs observables dans `book-of-lore status`.

Ce témoin ne réalise pas encore la sélection serveur. Les filtres, l'historique
par partie, `all_same`, les variables `##00` à `##06` et `scroll_speed` sont
testés comme politique mais ne sont pas encore actifs en jeu.

La configuration témoin `data-BKVince/BKVince.mpq/BookOfLore.toml` reste
explicitement `enabled=false`. Elle prouve le contrat de données sans annoncer
un comportement en jeu encore absent.

Preuves du 29 juillet 2026 :

- compilation Release x64 réussie par l'outillage natif gouverné;
- `3/3` tests CTest réussis : parseur/repli, configuration livrée et état de
  sélection;
- artefact `0.2.0` local SHA-256
  `09FE20D16DE70A84BD48DEF8106D45ECE2E867981736CB2687C4A3E60AD557DD`;
- les exports `D2RLoaderGetPluginInfo`, `D2RLoaderLoadPlugin` et
  `D2RLoaderUnloadPlugin` sont présents dans la DLL PE32+ x64;
- aucune DLL copiée dans le dépôt ou dans un runtime et aucun ZIP créé.

## Propriété des surfaces et coexistence

`Transmogrify` possède déjà le hook d'utilisation d'objet d'inventaire
`0x4F40C0`. `ReadableItems` lui délègue son chemin `pSpell=14`; il ne s'agit pas
d'un mécanisme pour un livre placé dans le monde. `ExtendedItemStats` et
`FloatingDamage` possèdent actuellement la chaîne de rendu/entrée qui relaie
spécifiquement Readable Items. Book of Lore ne doit ajouter aucun second hook
concurrent sur ces surfaces.

Les cinq DLL eezstreet ne fournissent pas de bus générique d'activation d'objet,
de rendu ou d'entrée. La coexistence statique est favorable, mais les portées
globale et mod-locale devront être prouvées par cold starts et matrice gameplay
après implantation du chemin runtime.

## Architecture runtime cible démontrée

Le trajet peut conserver l'UI native et éviter tout hook concurrent de rendu ou
d'entrée :

1. un hook serveur limité à `OBJECTS_OperateFunction06_TowerTome` sélectionne
   le message admissible et transmet un identifiant privé dans le champ 16 bits
   du paquet vanilla `0x27`;
2. le hook client du seul handler `0x27` reconnaît cet identifiant, arme une
   portée thread-local et laisse le chemin original continuer;
3. dans cette portée seulement, le callsite interne `0x197C5F` de l'UI scroll
   est redirigé vers une résolution conditionnelle; `LANG_GetStringById` reste
   intact pour tous les autres consommateurs;
4. `CLIENT_ShowScrollMessageById` construit ensuite le panneau natif, sans
   Present, D3D12, souris, clavier ni panneau propriétaire.

Cette architecture préserve le choix autoritaire de l'hôte et transporte une
identité de message compacte aux joiners. Elle exige que chaque pair charge le
même catalogue autonome; un identifiant inconnu doit échouer fermé et revenir au
comportement vanilla, jamais afficher un texte arbitraire.

Le témoin `0.2.0` implante déjà les étapes client 2 à 4 avec l'identifiant
vanilla `127` et le premier texte du catalogue. Il sert uniquement à prouver
l'affichage natif avant d'ajouter l'étape serveur et les identifiants privés.

## Prochain gate

Déployer temporairement le témoin `0.2.0` avec un catalogue minimal et
`enabled=true`, puis prouver qu'une Tower Tome affiche le premier texte configuré
dans l'UI native sans altérer les autres textes localisés. Conserver la DLL et la
configuration publique désactivées tant que cette preuve n'est pas obtenue.

Ensuite, résoudre la propriété exacte de la table runtime `OperateFn`, qui ne
contient aucun pointeur brut ni xref statique vers `0x5DC570` dans l'image
hydratée, et implanter la sélection serveur fail-closed. Journaliser l'identifiant
choisi côté serveur puis reçu côté client avant d'activer les filtres complets.

Après ce témoin :

1. valider les filtres, l'historique de partie et `all_same`;
2. valider solo, hôte/joiner et le refus des catalogues incompatibles;
3. valider les portées globale/mod-locale avec les cinq DLL du PluginPack;
4. seulement ensuite construire une archive publique limitée à
   `BookOfLore.dll` et `BookOfLore.toml`.

## Crédits historiques

- Myhrginoc — Diablo II Messaging System, D2Extra, tutoriel et kit historiques;
- SVR — conversion D2Mod et correctif d'acte;
- afj666 — Custom TBL Plugin utilisé par l'implantation D2Mod.

Ces crédits restent séparés de l'auteur du port D2R, `RuffnecKk`. Aucun binaire
legacy ni DLL d'eezstreet ne sera modifié, lié ou redistribué.
