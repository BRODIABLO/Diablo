# Rework des Rogue Scouts de l’Acte I — BKVince 3.2

Dernière mise à jour : 6 août 2026

## Statut

Chantier actif en parallèle de la mission courante BaseMod. Vincent a confirmé
le 1 août 2026 l’adaptation originale des idées de TDE 3.1d pour les Rogue
Scouts de BKVince, sans branche Holy dans ce premier lot.

Vincent a retenu le 2 août 2026 un plugin autonome permanent pour le mouvement
walk/run. `RogueScoutMovement` demeure une DLL RuffnecKk hybride, installable
globalement ou sous un mod, avec une configuration TOML indépendante; aucune
DLL d'eezstreet n'est modifiée, liée ou redistribuée.

## Contrat joueur confirmé

- Les Rogues doivent utiliser la course pour leurs déplacements significatifs.
- Lorsqu’elles sont frappées en mêlée, elles lancent `Terror` selon la courbe
  TDE : 25 % au niveau 5 en Normal, 33 % au niveau 10 en Nightmare et 50 % au
  niveau 15 en Hell.
- Les deux factions conservent `Inner Sight` et reçoivent `Bow Mastery` selon
  la courbe TDE.
- La Rogue Fire progresse de `Fire Arrow` vers `Exploding Arrow`, puis
  `Immolation Arrow`.
- La Rogue Cold progresse de `Cold Arrow` vers `Ice Arrow`, puis
  `Freezing Arrow`.
- Les deux compétences avancées de chaque branche se débloquent ensemble au
  palier BKVince 36, au lieu d’être toutes actives dès le niveau 3 comme dans
  TDE; le palier 67 devient uniquement un rééquilibrage de puissance.
- Chaque Rogue invoque au maximum un familier distinct : `Fire Raven` ou
  `Cold Raven`. Le familier réutilise le modèle HD Raven existant, suit son
  propriétaire, fournit un rayon lumineux et ne doit pas entrer en collision
  avec les Ravens d’un joueur Druide.
- La faction Holy reste hors du premier lot et sera évaluée séparément. Aucun
  nouvel effet HD original n’est requis ici.

## Faits vérifiés

- BKVince utilise déjà `Velocity=11`, `Run=14` pour `roguehire`, contre
  `Velocity=11`, `Run=11` dans TDE et `7/11` dans vanilla 3.2. Le besoin restant
  concerne donc le choix du mode de déplacement, pas une simple hausse de
  vitesse.
- Le patch `hireling-ai-tuning.json` resserre déjà les distances de suivi et de
  rattrapage, augmente l’activité ranged et renforce le recul.
- Les lignes Expansion de `hireling.txt` possèdent déjà les paliers 3, 36 et 67
  nécessaires à la progression retenue.
- TDE met les six compétences à disposition dès le niveau 3. BKVince utilise
  une progression réelle : les deux flèches avancées portent `Chance=0`,
  `Level=0` et `LvlPerLvl=0` au palier 3, puis deviennent toutes deux actives au
  palier 36; elles ne peuvent ni être choisies ni fournir une synergie avant.
- Le proc TDE repose sur un stat événementiel `damagedinmelee`, distinct du
  `gethit-skill` vanilla qui accepte aussi les missiles.
- Le Raven TDE est un vrai summon limité à un, et non un projectile : pet type
  distinct, warp vers le propriétaire, attaque physique et élémentaire, lumière
  blanche de rayon 7.
- Le workbench D2R 3.2.92777 est vérifié. Les six patches IA ranged existants
  sont gouvernés. `D2GAME_PETAI_PetMove` est maintenant identifié à `0x5C1460`
  avec quatre xrefs, une signature unique et l'ABI
  `(game, owner, unit, motionType, run, velocityPercent, steps) -> int32`.
- Les motion types `0` et `1` couvrent respectivement le suivi proche et le
  rattrapage du propriétaire. Les autres valeurs servent notamment à
  l'errance, au warp et à l'espacement et ne doivent pas être modifiées.
- `Velocity=11` est la base de déplacement de `roguehire`; l'argument natif
  `60` est un `velocitypercent` temporaire. Dans le chemin de rattrapage, zéro
  est remplacé par un bonus aléatoire de 50 à 64, de sorte qu'un simple patch de
  l'argument ne peut pas garantir une vitesse absolue de 11.
- `D2GAME_MONSTERMODE_SetVelocityParams 0x4473F0` écrit le pourcentage non nul
  à `aiParam+0x24`; zéro signifie « conserver ». Un override nul exact exige
  donc un clear borné de ce champ pendant le seul appel Rogue concerné.

## Architecture retenue

1. Appliquer une migration TSV reproductible et idempotente; refuser toute clé
   absente, dupliquée ou déjà occupée par un contenu différent.
2. Ajouter les définitions propres à BKVince pour `Bow Mastery`, `Fire Raven`,
   `Cold Raven` et leur pet type sans copier les identifiants numériques TDE.
3. Ajouter un proc mêlée-only de `Terror` sans modifier la compétence joueur.
4. Réaffecter les six slots de compétence des Rogues et conserver une vraie
   progression 3/36, avec un palier de puissance distinct à 67.
5. Équilibrer les dégâts sur la puissance BKVince actuelle aux niveaux 3, 36,
   67 et 90 avec la formule gouvernée de `hireling.txt`, le `HitShift` propre à
   chaque compétence et uniquement les synergies réellement apprises; la
   maîtrise fait partie du budget total de DPS.
6. Déployer uniquement les tables allowlistées, comparer leurs hashes et
   valider cold start puis gameplay.
7. Implanter `RogueScoutMovement` comme plugin autonome hybride. Hooker
   `D2GAME_PETAI_PetMove` seulement pour `roguehire=271`, les motion types `0/1`
   et une room valide; marcher en ville, courir ailleurs et préserver tous les
   mouvements de combat.
8. Convertir `town_velocity` et `outside_velocity` depuis les unités absolues de
   `monstats.txt` vers `velocitypercent`. Armer un second hook à `0x4473F0`
   uniquement par scope thread-local et pointeur `aiParam` correspondant afin
   que la valeur 11 efface l'override temporaire au lieu de déclencher le bonus
   aléatoire natif.
9. Refuser un build, une signature, une ABI ou une configuration présente mais
   invalide; prioriser la configuration mod-locale sur le repli global.

## Matrice de validation

| Domaine | Cas | Attendu | Statut |
|---|---|---|---|
| TSV | Round-trip et CRLF | byte-exact avant/après migration | PASS statique |
| Données | Références et IDs | aucune collision ou référence absente | PASS statique |
| Progression | Niveaux 3/36/67 | avancées verrouillées à 3, toutes actives à 36, puissance rééquilibrée à 67 | PASS statique |
| Terror | Normal/Nightmare/Hell | 25/33/50 %, mêlée seulement | PASS statique; combat runtime requis |
| Raven | Fire/Cold | un familier correct, dégâts et lumière cohérents | PASS statique; combat runtime requis |
| Cycle de vie | portail/waypoint/mort/résurrection | aucun doublon ni familier orphelin | not run |
| Coexistence | Druide Raven | pools indépendants | not run |
| Déplacement | suivi/recul/rattrapage | walk ville, run ailleurs sur les déplacements de suivi | PASS solo : campement puis Cold Plains; combat requis |
| Plugin | build, manifeste et tests | DLL hybride RuffnecKk, signatures strictes, politique ciblée | PASS build + runtime |
| Config | défauts et invalides | walk ville, run ailleurs, vitesses 11; invalides refusées | PASS runtime : mod-local, repli global et invalide |
| Coexistence native | cinq DLL eezstreet + ReviveOverhaul | aucun hook owner en collision | PASS source + cold start |
| Runtime | cold start BKVince | table chargée et frontend atteint | PARTIAL global : 24/24 et plugin 9/9; assertions BKVince préexistantes |
| Runtime | roster et Rogue existante | lignes version 100 visibles; niveau 98 et icônes cohérentes | PASS |
| Réseau | solo/hôte/joiner | propriété et comportement synchronisés | not run |

## Implantation data-first

- `Terror` est injecté par `monprop.txt` avec une propriété et un stat
  `damagedinmelee` propres à BKVince : 25/5, 33/10 et 50/15 selon la difficulté.
- Les paliers 3/36/67 sont conservés. Fire commence avec `Fire Arrow`, puis
  reçoit ensemble `Exploding Arrow` et `Immolation Arrow` au niveau 36; Cold
  commence avec `Cold Arrow`, puis reçoit ensemble `Ice Arrow` et
  `Freezing Arrow`. Le niveau 67 rééquilibre leurs niveaux sans nouvel unlock.
- `BKV Bow Mastery` reprend exactement la courbe TDE : à slvl 1/12/22/50,
  +28/116/196/420 % d’AR et +28/83/133/273 % de dégâts physiques.
- `BKV Fire Raven` et `BKV Cold Raven` utilisent deux monstres distincts et le
  pet type `bkvrogueraven`, plafonné à un familier, sans réutiliser le pool du
  Raven Druide.

## Budget de dégâts statique corrigé

Les valeurs ci-dessous sont des dégâts élémentaires par impact calculés depuis
les formules et synergies de `skills.txt`. Le calcul choisit le dernier palier
de recrutement inférieur ou égal au niveau courant, puis applique exactement :

`slvl = Level# + floor(LvlPerLvl# × (niveau courant − niveau du palier) / 32)`.

Il applique également le facteur `2^(HitShift−8)`; `Cold Arrow` utilise
`HitShift=7` et inflige donc la moitié des valeurs brutes de ses colonnes. Les
estimations excluent le physique de l’arc, Bow Mastery, l’équipement, les
résistances, la cadence réelle de l’IA et le sol brûlant d’Immolation Arrow.

| Niveau | Fire — attaques apprises | Cold — attaques apprises |
|---:|---|---|
| 3 | `Fire Arrow` 1–4 | `Cold Arrow` 3–4 |
| 36 | `Fire Arrow` 50–55; `Exploding` 74–99; `Immolation` 23–27 | `Cold Arrow` 50–52; `Ice` 82–90; `Freezing` 86–118 |
| 67 | `Fire Arrow` 183–205; `Exploding` 233–279; `Immolation` 57–62 | `Cold Arrow` 72–74; `Ice` 132–143; `Freezing` 163–198 |
| 90 | `Fire Arrow` 479–546; `Exploding` 380–444; `Immolation` 93–99 | `Cold Arrow` 149–159; `Ice` 265–287; `Freezing` 242–283 |

La branche Fire conserve ses niveaux 22/12/7 au palier 67. La branche Cold
utilise 15/9/7 au même palier : cela absorbe l’ajout de la synergie Ice au lieu
de doubler artificiellement sa puissance. À niveau 90, ses trois attaques sont
environ 8 à 18 % au-dessus de leurs équivalents actifs précédents, tandis que
Fire gagne surtout la variété, l’AoE et le sol brûlant. Bow Mastery atteint
alors slvl 29, soit +252 % d’AR et +168 % de dégâts physiques. Le Raven atteint
slvl 15 et demeure volontairement un compagnon d’appoint.

La migration recalcule et vérifie automatiquement ces huit snapshots. La
première estimation du 1 août extrapolait à tort la croissance depuis le niveau
zéro et omettait `HitShift`; les valeurs 1019–1394 et 525–754 sont retirées.

Le checkpoint conserve les réservations Readable Items `389/308` avant les IDs
Act I `390/309`, afin de préserver l’indexation physique de `itemstatcost.txt` et
`properties.txt`. Ces deux définitions restent inertes sans le plugin et la
recette Readable Items. Le validateur Storage Bag accepte donc des lignes tierces
après son bloc gouverné, tout en exigeant que ce bloc demeure complet, contigu et
byte-exact.

## Validation runtime

### 1 août 2026 — antérieure au correctif de dégâts

- Les neuf tables déployées (`hireling`, `itemstatcost`, `monprop`, `monstats`,
  `monstats2`, `pettype`, `properties`, `skills`, `states`) ont un SHA-256
  identique entre le dépôt et `mods/BKVince`.
- Le cold start atteint la sélection des personnages puis une partie BKVince;
  les logs frais D2RLoader ne contiennent aucune nouvelle erreur, exception ou
  référence invalide.
- Le roster de Kashya affiche les lignes Expansion version 100 au niveau 98 et
  leurs profils de compétences alternés.
- La Rogue existante Amplisa charge au niveau 98, accompagne le joueur et son
  panneau présente le profil de compétences attendu sans crash.
- Une tentative runtime temporaire avec un prix de recrutement nul a été
  refusée par le jeu; cette donnée de diagnostic n’a jamais été écrite dans le
  dépôt et le runtime officiel a été restauré puis revalidé par hash.
- Aucun remplacement de mercenaire ni aucune modification volontaire de
  sauvegarde n’a été effectué. Le summon effectif des deux Ravens, `Terror`, la
  mort/résurrection et la coexistence Druide restent donc des gates de combat.

### 2 août 2026 — progression et dégâts corrigés

- `hireling.txt` source et runtime portent tous deux le SHA-256
  `B0AC97C7…1652`; le rapport gouverné le plus récent est
  `analysis-cache/runtime-sync/20260802-111739858-apply.json`. Cette version
  déverrouille ensemble les deux flèches avancées au niveau 36; le niveau 67
  ne déverrouille plus aucune compétence.
- Le cold start charge `19/19` memory patches, `8/8` plugins, zéro rejet et
  atteint les étapes `24/24`; le frontend et la liste des personnages sont
  visibles et fonctionnels.
- Quatre assertions fraîches surviennent ensuite dans
  `D2Common/src/Items/Items.cpp` (`ptStats`, `ptItemStats` et classe d’objet),
  depuis le caller `D2RLoader.exe+0x7600A`. Elles ne désignent ni
  `hireling.txt` ni une compétence, mais leur causalité n’est pas encore
  isolée; le cold start reste donc `PARTIAL`, pas un PASS propre.
- Un second cold start, rapport
  `analysis-cache/runtime-sync/20260802-103307722-apply.json`, reproduit les
  quatre mêmes assertions après sélection de `QtyTester` au lieu de la
  sauvegarde invalide `BerserkBarb`; l’hypothèse d’un incident propre au
  personnage sélectionné est donc écartée.
- Le cold start du nouveau palier 36 reproduit encore exactement ces quatre
  assertions item après `24/24`, sans nouvelle erreur liée à `hireling.txt` ou
  aux compétences. Le chargement de la nouvelle table est donc prouvé, mais
  son comportement en combat demeure à observer.
- Aucun personnage n’a été ouvert et aucune sauvegarde n’a été modifiée durant
  cette validation. Les dégâts réels, les trois paliers et la branche Cold
  corrigée restent à observer en combat.

### 6 août 2026 — descriptions du panneau mercenaire

- L’ouverture de l’inventaire d’une Rogue Ice a produit un `assert exit` dans
  `HirelingInventoryPanel.cpp:890` : `Skill has no associated description!`,
  avec l’index interne `444`, soit `BKV Cold Raven`.
- L’audit exhaustif des compétences référencées par `hireling.txt` a trouvé six
  références invalides. `BKV Bow Mastery`, `BKV Fire Raven` et
  `BKV Cold Raven` reçoivent trois descriptions BKVince originales;
  `BKV Desert Smite`, `BKV Desert Mastery` et `BKV Desert Sacrifice`
  réutilisent respectivement `smite`, `pole arm mastery` et `sacrifice`.
- TDE 3.1d ne fournit pas de correctif copiable : `RavenRF`, `RavenRC`,
  `Bow Mastery` et `Mizan Mastery` y ont eux aussi un `skilldesc` vide. Aucun
  texte ni aucune ligne `skilldesc.txt` de TDE n’a donc été importé.
- Le validateur de démarrage contrôle désormais les 38 compétences distinctes
  utilisées par les mercenaires et refuse toute référence vide ou absente.
  `npm run verify:data` est vert.
- Les trois fichiers du correctif ont été redéployés le 6 août dans le profil
  BKVince avec des SHA-256 source/runtime identiques : `skills.txt`
  `46906B94…E022B3`, `skilldesc.txt` `66EDF1D4…5FE548` et `skills.json`
  `D77D0A1D…748C9`.
- Le cold start frais accepte D2R `3.2.92777`, applique `18/18` patchsets,
  atteint `24/24` et ne produit aucun nouveau crash. Il reste `PARTIAL` :
  `RogueScoutMovement.dll` refuse sa signature attendue à `0x349860`, incident
  déjà observé depuis le 5 août et distinct des trois tables redéployées
  (`13/14` plugins actifs, zéro rejet, un échec).
- La réouverture manuelle de l’inventaire de la Rogue Ice reste `not run`; le
  jeu est laissé ouvert avec `-mod BKVince -txt` pour cette observation.

### 6 août 2026 — assertions du cycle Fire/Cold Raven

- Le premier rapport, `d2r-crash-report (2026_08_07 00_30_02 UTC).log`, capture
  à 20:29:50 l’assertion `D2Common/src/Stats/States.cpp:46` :
  `nShift >= 0 && nShift < DataTablesGetNumStates(UnitGetGameVersion(hUnit))`.
  La Rogue Ice utilise `BKV Cold Raven` dans son slot 5 avec le même mode
  d’animation que ses tirs; le crash pouvait donc visuellement sembler provenir
  d’une flèche.
- Le workbench vérifié du build `3.2.92777` prouve que le chemin natif teste
  `D2SkillsTxt+0x2D8` (`aitype`) à `1`, charge ensuite le signed word
  `D2SkillsTxt+0x0A0` (`aurastate`) et appelle `STATES_CheckState`. Les deux
  Ravens BKVince étaient les seules compétences `aitype=1` sans `aurastate`
  résolu. Le premier correctif a supprimé cette incohérence, mais il conservait
  le summon générique `srvdofunc=119` et ne constituait donc qu’un correctif
  partiel.
- Le test en combat a ensuite produit
  `d2r-crash-report (2026_08_07 00_46_35 UTC).log`, assertion
  `D2Common/src/Units/Units.cpp:4666` :
  `ptUnit->eType == UNIT_PLAYER`. Le Raven avait bien été créé. La backtrace
  remonte de `0x5C12E1` vers le helper de déplacement `0x5BD1E0`, lequel appelle
  à `0x5BD23C` la routine `0x34B240` qui exige les données d’un joueur. Avec
  `srvdofunc=119`, le propriétaire direct du familier était la mercenaire
  `UNIT_MONSTER`, d’où cette seconde assertion.
- L’architecture TDE complète n’est pas un simple summon 119 : `RavenRF` et
  `RavenRC` utilisent `srvstfunc=28`, `srvdofunc=44`, l’état `rogueraven` et une
  compétence d’aura auxiliaire exécutée par `srvdofunc=65`. La documentation
  D2R 3.2 confirme respectivement le démarrage Blade Shield, l’invocation Blade
  Sentinel et l’aura Might. D2MOO, au commit gouverné
  `19019806df7f3e877fa105b05395d1e3597e2316`, montre dans
  `SKILLS_SrvDo044_BladeSentinel` que la chaîne des propriétaires monstres est
  remontée avant la création du familier; le joueur devient donc son
  propriétaire effectif. Cette preuve est sémantique et reste soumise au test
  runtime D2R 3.2.
- La migration BKVince reproduit désormais ce contrat sans importer d’IDs TDE :
  les deux Ravens utilisent `srvstfunc=28`, `srvdofunc=44`, le missile
  `blade shield attachment`, `aurastate=bkvrogueraven`, `auralencalc=125`,
  `aitype=1`, `pettype=bkvrogueraven` et `petmax=1`. Le nouvel état gouverné
  `bkvrogueraven` occupe l’ID libre `244`. La compétence auxiliaire
  `BKV Rogue Raven Aura` utilise `srvdofunc=65`, le même état en aura source et
  cible, `aurafilter=65539`, `aurarangecalc=1024`, `aura=1` et `perdelay=50`.
- Le validateur refuse maintenant tout `aitype=1` sans état résolu et verrouille
  explicitement le contrat Blade Sentinel, l’état et la compétence auxiliaire.
  `test:bkvince-act1-rogue`, `test:bkvince-startup-refs`,
  `test:bkvince-mercenary-command`, les contrôles de syntaxe et la suite complète
  `verify:data` passent. Les round-trips de `skills.txt` (449 lignes) et
  `states.txt` (245 lignes) sont byte-exact avec CRLF préservés.
- Les deux tables redéployées sont identiques à la source : `skills.txt`
  `4FB3EA8EB98C8429179867E398B7FAD2B853CF2FDBEF0121103970148D317507` et
  `states.txt`
  `BBADCF47A7AE1675D42D4164FCB2184896B797925840ABB11FD65E3A837CF555`.
  Le cold start frais accepte le build `92777`, applique `18/18` patchsets,
  charge `13/13` plugins sans rejet ni échec — dont `RogueScoutMovement` — et
  atteint `24/24`. Aucun nouveau rapport de crash n’est présent au moment du
  contrôle; le test Fire/Cold Raven en combat reste toutefois `not run` jusqu’à
  observation directe du familier en mouvement et en attaque.
- `BindAndSummon.dll` est absent de la pile actuellement installée; ce cold
  start ne constitue donc pas une preuve de coexistence avec ce plugin.

### 7 août 2026 — Raven serveur présent, rendu client absent

- L’observation gameplay corrige l’interprétation précédente : le Raven issu du
  cycle TDE était bien créé côté serveur et attaquait les monstres. L’absence à
  l’écran était donc un défaut de rendu client, pas un échec d’invocation.
- Les deux classes `bkvfireraven` et `bkvcoldraven` utilisaient
  `MonStatsEx=bkvrogueraven`. La recherche dans tout `data-BKVince` ne trouve
  aucune ressource HD ni aucun mapping portant cette identité, seulement les
  cinq références TXT qui la définissaient. Le client recevait donc une unité
  serveur fonctionnelle sans identité graphique HD résoluble.
- Le prototype de diagnostic qui a remplacé temporairement le cycle TDE par
  `srvdofunc=114` et l’IA native `Raven` était fondé sur une mauvaise attribution
  de l’assertion `LvlTbls`. Il a introduit l’assertion distincte
  `D2Game/src/Unit/SUnitMsg.cpp:459` dans le rapport
  `d2r-crash-report (2026_08_07 01_21_48 UTC).log`.
- Le workbench 92777 situe cette assertion à `0x539D36`, dans le sérialiseur des
  stats d’une unité au moment où ses états sont envoyés au client. La référence
  sémantique D2MOO montre que `SKILLS_SrvDo114_Raven` applique les passives du
  skill au pet; combiné à l’état d’aura requis par l’IA du mercenaire, ce
  prototype faisait entrer les dégâts élémentaires signés dans ce trajet
  réseau. Il est retiré intégralement plutôt que masqué par un clamp arbitraire.
- Le correctif revient au cycle TDE déjà observé fonctionnel : `srvstfunc=28`,
  `srvdofunc=44`, état `bkvrogueraven`, helper
  `BKV Rogue Raven Aura` en `srvdofunc=65` et IA `NecroPet`. Les deux monstres
  réutilisent désormais `MonStatsEx=druidhawk`, soit l’identité graphique Raven
  vanilla existante, tout en conservant leurs classes et leur pet type propres.
- La ligne `monstats2=bkvrogueraven`, qui ne fournissait qu’un halo blanc sur une
  identité HD inexistante, est supprimée. Le halo reste volontairement hors de
  ce correctif de stabilité et devra revenir par un effet visuel valide.
- `LvlTbls.cpp:284` est explicitement hors de ce diagnostic et traité dans une
  autre tâche. Aucun fichier de niveau n’est modifié ici.
- La migration est idempotente; `test:bkvince-act1-rogue`,
  `test:bkvince-startup-refs`, `test:bkvince-mercenary-command` et la suite
  complète `verify:data` passent. La visibilité et l’absence d’assertion en
  combat restent à confirmer après le redéploiement runtime.
- Le redéploiement ciblé porte des hashes source/runtime identiques :
  `skills.txt` `4FB3EA8E…17507`, `monstats.txt` `FE4015AF…C7407` et
  `monstats2.txt` `AAA34238…E2764`. Le cold start accepte le build `92777`,
  applique `18/18` patchsets, charge `13/13` plugins sans rejet ni échec — dont
  `RogueScoutMovement` — et atteint `24/24`. Aucun nouveau rapport de crash
  n’est créé avant le test gameplay; le jeu reste ouvert pour ce témoin.

## Implantation native autonome — 2 août 2026

- Destination confirmée par Vincent : plugin autonome permanent, sans future
  clé de merge PluginPack.
- `RogueScoutMovement 0.1.0` cible D2R `3.2.92777`, porte l'auteur exact
  `RuffnecKk`, ne déclare pas `ModScopedOnly` et conserve uniquement le flag
  `NativeHooks`. Sa description visible est : « Makes Act I Rogue Scouts walk
  in town and run while following elsewhere. »
- Le TOML indépendant expose `walk_in_town`, `run_outside_town`,
  `town_velocity=11`, `outside_velocity=11` et les diagnostics. Les vitesses
  valides vont de 3 à 24; les sections, clés, doublons, booléens et entiers
  inconnus ou invalides provoquent un refus de chargement.
- La résolution cherche d'abord
  `<D2R>/mods/<mod>/d2rloader/config/rogue-scout-movement.toml`, puis le chemin
  de configuration du scope chargé, puis le repli global
  `<D2R>/d2rloader/config/rogue-scout-movement.toml`.
- Le build Release MSVC 19.44 réussit. Le test de politique
  `rogue-scout-movement-policy` passe `1/1`; il couvre la classe Rogue, les deux
  motion types, la ville, l'extérieur, les opt-outs, les autres mercenaires,
  les mouvements de combat et la conversion 11/12/10.
- DLL finale construite : SHA-256
  `F94340D3CD82E528B2EBDD727744A6EED1D2FCB7A14712BFD9048BE3A1997F01`,
  86 016 octets. Le binaire du dépôt et celui déployé sont identiques.
- Le rapport gouverné
  `analysis-cache/runtime-sync/20260802-194340534-apply.json` confirme le
  déploiement du binaire final et du TOML dans le profil BKVince, l'arrêt d'une
  instance verrouillante, la relance d'une seule instance et un résultat
  `PASS`.
- Le cold start final installe les deux hooks à `0x4473F0` et `0x5C1460`,
  applique `19/19` patchsets, charge `9/9` plugins sans rejet ni échec et atteint
  `24/24`. Les assertions observées après le frontend concernent les incidents
  BKVince déjà connus dans Items, la zone `Act5-Rifts` dupliquée et un level ID;
  aucune assertion, erreur ou exception ne désigne le plugin Rogue.
- La matrice de configuration passe en portée mod-locale, en repli global et
  avec une configuration présente mais invalide. Cette dernière est refusée
  avant l'installation des hooks (`8` plugins actifs, `1` échec attendu), puis
  le TOML mod-local valide est restauré et le repli global temporaire supprimé.
- En solo avec Amplisa, le suivi a été exercé dans le Rogue Encampment, puis
  après waypoint dans les Cold Plains. La Rogue rejoint le joueur dans les deux
  zones, la partie demeure stable et la sortie sauvegardée revient au frontend.
  La sélection walk/run et la vitesse absolue 11 sont imposées par les branches
  natives testées; une capture fixe ne permet toutefois pas de mesurer la
  cadence d'animation image par image.

## Gate immédiat

Le plugin autonome et sa configuration sont implantés, déployés et validés en
solo pour les transitions ville/hors-ville. Il reste à confirmer que les modes
de combat, recul, warp et résurrection demeurent inchangés, puis à exécuter la
matrice hôte/joiner. Les assertions BKVince post-frontend et les gates de combat
Fire/Cold, `Terror`, Ravens et coexistence Druide restent ouverts et distincts.
