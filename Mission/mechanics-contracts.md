# Mechanics 2.0 — contrats natifs MEC-00/MEC-01

- Build cible : `D2R.exe 3.2.92777`
- Date d'ouverture : 8 août 2026
- Statut : `MEC-00 in_progress`; `MEC-01 blocked_by_cap90_runtime_gate`
- Source de conception : `Mission/pd2-game-mechanics-vs-bkvince-audit.md`
- Registre de preuve : `Mission/mechanics-native-proof-92777.md`
- Effet gameplay autorisé dans ce lot : **aucun**

> **Décision post-lot — 9 août 2026.** Les interdictions ci-dessous demeurent
> le contrat historique de MEC-00/MEC-01 et ne sont pas réécrites comme des
> preuves acquises. Vincent a ouvert séparément la mission publique autonome
> `MeleeSplash.dll` v0.1 pour D2R 3.2.92777 offline/local single-player,
> joueur contre monstres. L'ancien prototype reste non probant; la nouvelle
> implantation doit redémontrer chaque surface utilisée, peut composer plusieurs
> coutures strictement validées et fail-closed, mais ne sélectionne pas
> `Pd2CombatCore`. Multijoueur et PvP sont hors portée actuelle. Critical/Deadly,
> Crushing Blow/CBE, Open Wounds/DPS plat et résistances hybrides restent des
> lots généraux ultérieurs dont MeleeSplash consommera les formules autoritaires.
> La décision produit active roule toutefois Critical/Deadly, CB et OW
> indépendamment par cible splash; seul le paquet offensif pré-critique est
> partagé. L'adaptateur Critical/Deadly 92777 de la v0.1 est transitoire et ne
> préjuge pas du resolver PD2 général à venir.

## 1. Décision et frontière

MEC-00/MEC-01 établit la fondation technique d'un éventuel vrai melee splash
PD2 pour BKVince. Le lot ne sélectionne aucune DLL, aucun propriétaire futur et
aucune architecture `Pd2CombatCore`. Une couture commune ne devient admissible
qu'après preuve statique et témoin runtime read-only sur le build gouverné.

Sont interdits dans ce lot :

- toute modification de table TXT, formule, stat, sauvegarde ou configuration
  active;
- tout changement des dégâts, événements, jets RNG, cibles, statlists, HP,
  mana, stamina, modes, mort, packets ou kill credit;
- tout nouveau plugin ou DLL gameplay;
- tout build, déploiement ou test gameplay du prototype `MeleeSplash`;
- toute promotion d'une hypothèse dans `known-rvas.json` sans le dossier de
  preuve défini à la section 4.

Le prototype décrit par `Mission/melee-splash-3.2.md`, son code et son binaire
sont conservés byte-exactement comme matériel historique. Ils ne constituent
ni un oracle D2R, ni un témoin runtime, ni une preuve d'ABI. Leur manifeste de
quarantaine et leurs SHA-256 sont figés dans cette mission historique.

## 2. Autorité des sources

| Rang | Source | Usage autorisé |
|---:|---|---|
| 1 | Image et index vérifiés `D2R.exe 3.2.92777` | RVA, contrôle de flux, structures, ABI, signatures et ownership natif |
| 2 | Témoins runtime read-only du même build | ordre réel, valeurs, threads, lifetimes et autorité serveur |
| 3 | Tables et configurations BKVince gouvernées | données effectivement consommées; jamais preuve d'un handler |
| 4 | PD2 `Game Mechanics` révision 23934 | contrat de conception et vecteurs attendus |
| 5 | Binaire PD2 S13 épinglé | oracle comparatif seulement, avec path, hash et rapport indépendants du prototype |
| 6 | D2MOO 1.10f | guide sémantique; aucune adresse, structure ou ABI transposable |
| 7 | Prototype `MeleeSplash` | inventaire d'hypothèses à réfuter ou confirmer indépendamment |

En cas de divergence, le comportement 92777 observé fait autorité pour le
transport natif. La sémantique PD2 reste une décision produit ultérieure.

## 3. Vocabulaire et unités

- `source` : unité ayant produit l'attaque ou la compétence.
- `attacker` : unité attribuée comme attaquant au pipeline de dégâts.
- `primary` : cible du hit melee initial.
- `secondary` : cible potentielle d'un futur splash; inexistante dans ce lot.
- `damage record` : objet natif transportant valeurs, flags et états du hit.
- `successful melee hit` : hit serveur ayant franchi le résultat d'attaque
  requis par le moteur; la position exacte de ce gate reste à prouver.
- `offensive result` : résultat déjà choisi par la source, dont le résultat
  Critical/Deadly si le moteur l'a résolu.
- `per-target result` : défense, résistance, absorb, CB, OW, leech, HP et mort
  qui peuvent dépendre de la cible.

Les dégâts par frame, HP, mana, stamina et autres valeurs natives en fixe sont
conservés dans leur unité brute. Aucun observateur ne convertit en flottant sur
le hot path. Toute division ou multiplication documente les troncatures
intermédiaires et distingue entier signé, non signé et fixe `1/256`.

## 4. Dossier minimal d'une preuve native

Une surface ne peut passer à `confirmed` que si le registre 92777 contient :

1. le `status` du workbench avec hashes canonique et d'analyse vérifiés;
2. les bornes de fonction et son rôle démontré par contrôle de flux;
3. une signature suffisamment stricte, ses octets attendus et son unicité;
4. les callers et callees matériels pour la responsabilité revendiquée;
5. l'ABI x64 : arguments, retour, registres préservés, stack et ownership;
6. chaque champ de structure utilisé, avec largeur, sens et lifetime;
7. la plage exacte qui serait observée et l'audit de collisions;
8. un témoin runtime lorsque l'ordre, le thread, la valeur ou le lifecycle ne
   peut pas être fermé statiquement;
9. les limites et preuves négatives qui empêchent de généraliser le résultat.

Une adresse seulement plausible reste `hypothesis`. Une fonction prouvée mais
dont l'ABI est incomplète reste `partial`. Une contradiction est `rejected` et
reste consignée pour empêcher sa réintroduction.

## 5. Contrats de surface

### MCT-HIT — couture serveur d'un hit melee réussi

Entrées minimales attendues : `game`, `source/attacker`, `primary`, contexte
melee et pointeur vers le damage record vivant.

Invariants :

- exécution sur le chemin serveur autoritaire;
- distinction démontrée entre melee, missile et dommages périodiques;
- hit déjà accepté selon le contrat natif réellement observé;
- aucun HP, mort ou packet ajouté par l'observateur;
- portée joueur, mercenaire, summon et monstre explicitement mesurée;
- dual wield, Smite, kicks et skills multi-hit décrits comme branches propres
  lorsqu'ils ne partagent pas la couture.
- récursion, dégâts imbriqués, réentrance et concurrence de threads mesurés;
- aucune invocation canonique future ne peut revenir récursivement dans son
  propre point d'entrée sans un contrat explicite et borné.

Gate : callers, callees, ABI, ordre relativement à Critical/Deadly, events,
leech, résistances, HP et mort, plus témoin runtime sur un hit et un miss.

### MCT-DAMAGE — damage record

Chaque champ reçoit un identifiant sémantique indépendant de son offset :

| Champ logique | Questions obligatoires |
|---|---|
| résultat/flags | largeur, bits stables, moment de pose/effacement, CS/DS distinguables ou non |
| physique | unité fixe, valeur avant/après défense et résistances |
| feu/froid/foudre/poison/magique | unité, durée éventuelle et phase d'agrégation |
| life/mana/stamina leech | valeur, unité, source du calcul, point de consommation |
| CB/OW/knockback/slow | chance, résultat, state/event et ownership |
| source skill/weapon | pointeur, ID, layer, durée de vie et règles local/global |
| bookkeeping | constructeurs, copie, destructeur, pointeurs internes et aliasing |

La taille `0x180` et les offsets actuellement présents dans le prototype sont
des hypothèses tant que constructeurs, destructeur et consommateurs 92777 ne
les ont pas prouvés champ par champ. Aucun `memcpy` ou accès runtime n'est
autorisé par ce document.

### MCT-CRIT — résultat Critical/Deadly Strike

Le contrat doit fermer séparément :

- les producteurs Critical Strike et Deadly Strike;
- leur ordre de jet, leurs caps et multiplicateurs natifs;
- le périmètre physique et l'arme/statlist réellement consultée;
- le bit ou champ de résultat, son moment de pose et sa persistance;
- la différence entre « coup critique affiché » et origine CS/DS;
- le comportement joueur, mercenaire, summon et monstre.

Un flag observé en aval peut prouver « multiplicateur critique appliqué » sans
prouver s'il provient de Critical ou Deadly. Cette distinction reste alors
`unresolved`.

### MCT-EVENT — dispatcher et handlers 15, 16 et 20

Pour le dispatcher puis chacun des handlers :

| Event | Sémantique data connue | Preuve native attendue |
|---:|---|---|
| 15 | Open Wounds / `item_openwounds` | entrée réelle, token/stat/layer, RNG, statlist créée ou rafraîchie |
| 16 | Crushing Blow / `item_crushingblow` | entrée réelle, RNG, catégorie de cible, vie lue et dégâts produits |
| 20 | event générique, dont splash BKVince ID 384 | entrée réelle, token/stat/layer, dispatch et consumers exacts |

Le lot doit aussi distinguer EventFunc14, déjà candidat dans le prototype, des
trois événements exigés. Un voisinage numérique ou un tableau supposé ne prouve
aucune identité de handler.

### MCT-LEECH — calcul life/mana leech

Le point natif doit exposer ou permettre de reconstruire : dégâts physiques
effectivement infligés, pourcentage de leech, réduction attaque/skill,
difficulté, Drain Effectiveness et toutes les troncatures.

Le contrat sépare :

- calcul de la quantité;
- écriture dans le damage record;
- crédit effectif de vie/mana;
- plafonds ou refus de cible;
- life, mana et stamina;
- source primaire et éventuelle cible secondaire.

L'offset `+0x120` et la règle « life uniquement divisé par deux » du prototype
restent des hypothèses. Aucun demi-leech splash n'est implanté dans ce lot.

### MCT-RNG — RNG serveur

Pour chaque jet, consigner owner de seed, ABI, borne, convention inclusive ou
exclusive, unité/thread associé, consommation en cas d'échec et ordre relatif.

Un observateur peut lire avant/après; il ne peut jamais appeler la primitive
RNG, modifier la seed, réordonner l'énumération ou ajouter un jet. La preuve doit
fermer au minimum Critical/Deadly, events 15/16/20 et tout filtre aléatoire de
cible rencontré.

### MCT-APPLY — primitive native d'application des dégâts

La primitive recherchée doit être distinguée des helpers qui ne font que
calculer, résister, afficher ou finaliser une réaction. Le dossier précise si
elle couvre :

- résistances, immunités, pierce, absorb et réductions;
- mutation HP et flags létaux;
- réactions, modes et mort;
- packets, attribution et kill credit;
- on-hit/events, afin d'éviter leur double exécution.

FloatingDamage à `0x427150` est un observateur post-résolution connu et ne peut
pas être renommé « primitive d'application » sans preuve contraire.

MEC-01A ferme positivement la chaîne melee canonique de préparation/queue puis
consommation. Elle mêle toutefois Fill/CS-DS, défenses, events/leech/HP,
durabilité, thorns, réactions, mort, packets et lifecycle du combat node. Aucune
primitive secondaire sûre n'est prouvée; la disposition limitée est donc
`LIKELY_FRAGMENTED`, sans sélection d'une architecture custom.

### MCT-AREA — unités, zone et rooms adjacentes

Le contrat doit prouver l'ABI de l'énumérateur et de son callback, l'unité des
coordonnées et du rayon, la room d'origine, les rooms adjacentes réellement
visitées, l'ordre d'énumération et ses conséquences RNG.

Les filtres sont documentés séparément : type d'unité, alignement/hostilité,
targetability, vie/mode, portée, collision, ligne de vue, primaire exclue et
doublons aux frontières de rooms. Les témoins couvrent aussi obstacles,
mort/despawn pendant l'énumération et déduplication d'une unité visible depuis
plusieurs rooms. Aucun masque numérique du prototype n'est accepté sans
décomposition 92777.

MEC-01A confirme statiquement le noyau `0x4398B0`, son ABI, son callback, son
ordre et ses filtres. Le contrat global reste `partial` tant que le writer de la
room-list, sa membership adjacente et ses invariants d'unicité/dédup ne sont pas
fermés.

### MCT-OW — lifecycle des statlists Open Wounds

Le registre doit suivre : création, owner/source, target, state, stats, durée,
tick, refresh ou stack, expiration, mort, despawn, changement de room,
déconnexion, fin de game et réutilisation de GUID.

Les cas obligatoires incluent première application, refresh, deuxième et
troisième stacks, quatrième application, deux attaquants concurrents et
teardown après déconnexion ou fin de game.

Les helpers déjà gouvernés de statlists peuvent guider la recherche, mais ne
prouvent pas le consommateur Open Wounds. Le patch kill-credit aux sites
`0x448DCA/0x448DE5` est un owner voisin, pas une preuve de la formule OW.

### MCT-OWNER — propriété et coexistence

Avant toute conclusion, chaque plage est croisée contre :

- les écritures du PluginPack épinglé réellement livré;
- les patches JSON BKVince actifs;
- les hooks de FloatingDamage et des addons RuffnecKk;
- les trois sites revendiqués par le prototype quarantiné;
- les callsites et trampolines, pas seulement les entrées de fonction.

Un site possède un seul owner d'écriture. Un observateur externe n'acquiert
aucune propriété. Aucune DLL eezstreet n'est modifiée, liée ou redistribuée.

## 6. Propriétaires déjà réservés

| Surface | Owner actuel | Conséquence MEC |
|---|---|---|
| `0x427150` | FloatingDamage | observer seulement; aucun second detour |
| `0x4524D6/0x4524DE` | `plugin-items` | caps physiques/élémentaires hors propriété MEC |
| `0x441B10` | `plugin-items/items.itemDurability`, réservé mais désactivé | aucun hook concurrent; le standalone historique n'est pas l'owner runtime courant |
| `0x448DCA/0x448DE5` | patch kill credit Poison/OW/Burning | conserver le patch dans la baseline observée |
| `0x44F8F1` | patch enemy resistance BKVince | baseline explicite; traitement atomique dans un futur lot résistance |
| `0x44CE80/0x583580/0x583B30` | revendiqués par le prototype quarantiné | hypothèses; aucun nouveau hook ni promotion avant preuve |

La matrice complète, y compris les non-chevauchements prouvés, appartient au
registre natif et doit citer le manifeste exact.

## 7. Contrat des témoins runtime MEC-01

MEC-01 commence seulement après fermeture du gate runtime du cap élémentaire
90. Les captures utilisent en priorité un debugger externe et des breakpoints
matériels tournants. Elles peuvent modifier les registres de debug du thread,
mais jamais le code, les données gameplay ou l'ordre des appels.

Chaque témoin consigne :

- build, hash, profil et pile de plugins/patches;
- heure de début et logs frais;
- scénario, attaquant, cible et valeurs contrôlées;
- thread, caller, registres, stack et mémoire lue;
- compte d'occurrences et corrélation du même damage record;
- résultat brut avant toute conversion;
- limites, perturbation de timing et statut `passed/failed/blocked/not run`.

La baseline qualifiée fige les SHA-256 et versions de `D2RPlugins.json`, des
cinq DLL PluginPack, de FloatingDamage, du manifeste de 139 écritures et des
patches BKVince actifs. Elle prouve explicitement que `MeleeSplash.dll` n'est
pas chargé tout en gardant toutes les autres fonctionnalités actives. Une
isolation sans cet inventaire est seulement diagnostique.

Les scénarios couvrent hit/miss/block/kill, CS/DS, events 15/16/20, leech pair
et impair, room courante/adjacente, player/merc/summon/monster, OW
création-expiration-teardown, solo/hôte/joiner. Le prototype `MeleeSplash` ne
peut pas être chargé dans un témoin qualifié du pipeline D2R natif.

Si les breakpoints externes ne suffisent pas, une évolution observer-only du
propriétaire existant est une décision séparée. Toute nouvelle DLL déclenche le
gate d'incubation avant le premier changement de code.

### 7.1 MEC-01A exploratoire du 8 août 2026

Une courte session solo a été exécutée pendant que le gate cap 90 demeurait
ouvert. Elle est conservée comme **preuve exploratoire**, pas comme témoin MEC-01
qualifié. Les résultats utilisent les libellés `passed exploratory`, `observed`
et `not run`; ils ne changent aucun des états de preuve natifs définis plus haut.

La session a observé un miss et un hit acceptés, un Crushing Blow de 25 % sur
une cible à 100 000 HP, le résultat Critical/Deadly aval partagé et la transition
visuelle Open Wounds gris vers rouge puis rouge vers gris. Elle n'a capturé ni
thread, caller, registres, stack, seed, pointeur `D2Damage`, handler ou statlist.

La reprise statique post-session corrige toutefois l'exigence de corréler une
adresse `D2Damage` littéralement identique sur les quatre étapes. Le chemin
melee copie en profondeur le record conservé, de `combatNode+0x18` vers un record
local, par `D2Damage_CopyConstructor` au callsite `0x44B37C`. `ExecuteEvents`
(`0x44B3FA`) et `FinalizeDamage` (`0x44B4F4`) reçoivent cette copie. Un futur
témoin doit donc prouver l'identité **logique** et la frontière de copie; exiger
`pointer(Fill) == pointer(Execute)` serait un faux gate.

Les artefacts bruts, le manifeste des fichiers runtime et la matrice détaillée
sont sous
`analysis-cache/mechanics-92777/20260808-223638-mec01a-exploratory/`. Le crash
de fin de session est un assert TACT/CURL récurrent déjà observé deux fois avant
MEC-01A; il n'est pas attribué au combat sans preuve supplémentaire.

Conséquences gouvernées :

- aucune promotion `confirmed_runtime`;
- témoins gameplay réduits : positifs pour hit/miss, CB, résultat critique aval
  et application/retrait visuels OW;
- conclusion limitée d'architecture MEC-01A : `LIKELY_FRAGMENTED`; la chaîne
  melee canonique est prouvée, mais ses responsabilités sont trop larges pour
  une cible splash et aucune primitive secondaire sûre n'est prouvée;
- verdict Mechanics 2.0 maintenu à `NON PROUVÉE`;
- aucun choix de `Pd2CombatCore`, de DLL ou d'architecture.

## 8. Gate d'une couture autoritaire commune

Une couture commune est `PROUVÉE` seulement si un même contrat stable :

1. est exécuté par le serveur pour un hit melee réussi;
2. transporte `game`, source/attacker, primary et damage record vivant;
3. expose le résultat offensif, dont Critical/Deadly, sans reroll implicite;
4. précède les mutations par cible ou permet d'appeler la sous-chaîne native
   canonique sans doubler events, leech, HP, mort ou packets;
5. préserve strictement l'ordre et l'ownership RNG;
6. couvre les sources requises ou expose des branches explicitement prouvées;
7. ne chevauche aucun owner actuel;
8. supporte sans ambiguïté récursion, dégâts imbriqués, réentrance et
   concurrence, ou démontre leur exclusion native;
9. possède signatures, ABI et témoins reproductibles.

Sinon le verdict est `FRAGMENTÉE` ou `NON PROUVÉE`. Même un verdict `PROUVÉE`
n'autorise pas automatiquement `Pd2CombatCore`; il ouvre seulement une future
décision d'architecture.

## 9. Divergences sémantiques à ne pas figer par accident

Le wiki/audit, le binaire PD2 S13 et le prototype ne concordent pas encore sur
les nouveaux jets par cible, le partage du crit, knockback/slow, les rayons,
les bonus de rayon, le type de leech réduit et plusieurs skills spéciaux.

MEC-00/MEC-01 prouve le transport D2R 92777. Il n'implante ni ne tranche ces
choix PD2. Crushing Blow, Open Wounds et résistances conservent leurs formules
BKVince natives; aucun retune de ces mécaniques n'appartient à ce lot.

## 10. Rollback

Le rollback documentaire retire les deux documents MEC et leurs fragments de
gouvernance. Les promotions ciblées de `known-rvas.json` et conclusions de
`findings.md` sont retirées seulement si leur preuve est invalidée; une preuve
native stable reste utile indépendamment d'une future architecture. Aucune
table, configuration, DLL, sauvegarde ou installation runtime n'a à être
restaurée. Les preuves locales sous `analysis-cache/` sont non versionnées et
le prototype quarantiné reste préservé.
