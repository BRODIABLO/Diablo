# BurnDamageFix — empreinte native sans allowlist de version D2R

## Décision produit — 2026-08-25

Vincent autorise par `GO` le renommage permanent de `BurnFireResistance` vers
`BurnDamageFix` et l'extension du plugin autonome existant. La même DLL doit
normaliser toute production Burn générique au seam interne gouverné `0x44CB32`,
puis conserver la résolution Fire Resistance composable à `0x451380` sans
prendre possession de l'entrée partagée `0x4523E0`.

Le plugin reste une DLL autonome, versionnée indépendamment, hybride
globale/mod-locale et membre de la RuffnecKk D2RLoader Suite.
`BurnFireResistance.dll` et `BurnDamageFix.dll` ne doivent jamais être chargées
ensemble. La coexistence avec la Suite active complète, les cinq DLL eezstreet,
`MonsterDisplay.dll` et `BindAndSummon.dll` est un gate de livraison. La matrice
runtime complète vise la version officielle courante 3.3.93847; 3.2.92777 est
couverte seulement lorsque le corpus prouve byte-exact toutes les surfaces
natives employées. Ces noms qualifient les preuves et ne limitent jamais le
chargement. Aucun plugin ni aucune fonction du PluginPack ne peut être désactivé
pour fabriquer un résultat passant.

Vincent a retenu l'option 1 et donné `GO overlay witness` le 25 août 2026.
BurnDamageFix doit observer, sans le forcer, le state natif `burning` (ID 115)
après une application Burn positive lorsque les diagnostics sont activés. Le
witness distingue `state active` de `state missing`; il ne crée aucun overlay,
ne retoggle aucun state et ne transforme pas un Burn annulé par immunité en
fausse anomalie.

## Décision corrective 2.1.0 — 2026-08-26

Vincent donne `GO` pour corriger dans une seule passe les deux anomalies
confirmées pendant le témoin BKVince : un monstre Fire Immune ne doit plus
recevoir le DoT Burn, et un effet `fire_hit` doit être rejoué sur l'unité tant
que son vrai state `burning` reste actif. L'effet est strictement visuel : il ne
produit aucun dommage, n'allonge pas le Burn et ne modifie ni kill credit ni XP.

Le rejeu visuel ne peut être armé qu'après une résolution Burn positive. Un
Burn annulé par Fire Resistance ou immunité ne doit créer ni state suivi ni
overlay synthétique. Le seam périodique retenu est désormais prouvé et implanté
sur les dispatchers joueur `0x42CE30` et monstre `0x447420`; son comportement et
sa coexistence restent à qualifier au runtime avec la pile complète.

## Décision corrective 2.2.0 — 2026-08-26

Après le témoin BKVince 2.1, Vincent confirme que le `fire_hit` périodique est
le meilleur visuel courant, mais demande de retirer la flamme native
`burning` 224 qui reste au sol derrière la cible. La DLL doit réaliser ce
remplacement automatiquement pour toutes les installations, sans distribuer ni
réécrire `states.txt`; BKVince doit suivre exactement le même chemin que les
autres utilisateurs du plugin.

Le `GO` retient une mutation process-local de la ligne compilée `burning` 115,
réutilise le hook Burn existant `0x451380` et n'ajoute aucun hook exécutable. Le
DoT, Fire Resistance/immunité/pierce, kill credit, XP, Floating Damage et le
replay `fire_hit` doivent rester inchangés. Monster Display et Bind And Summon
restent des gates de coexistence runtime, mais aucun nouveau seam partagé avec
eux n'est introduit.

## But

Réparer les deux défauts Burn rapportés par Necrolis : la production générique
ne doit plus ajouter le nombre `316` comme dommage plat, et le Burn appliqué
doit respecter la Fire Resistance native. Le patch indépendant Thorns/Burn de
kill credit et d'expérience reste inchangé.

## Faits natifs vérifiés

- Le corpus natif commun aux deux cibles est vérifié par l'atelier persistant;
  aucune nouvelle image n'a été dumpée.
- Dans le producteur générique à `0x44CB32`, la séquence unique
  `81 C3 3C 01 00 00 41 0F 48 DE` additionne littéralement `316` après avoir
  calculé le dommage existant et avancé le seed natif. `316` et `317` sont les
  IDs `burningmin` et `burningmax`, pas des valeurs de dommage.
- Le seam expose l'attaquant dans `RSI`, le numérateur Burn existant dans
  `EBX` et la valeur aléatoire native déjà avancée dans `R8D`. Le relais peut
  donc reproduire le jet natif sans faire avancer le seed une seconde fois.
- Le helper natif de production missile à `0x465799/0x465B40` et la référence
  D2MOO épinglée prouvent l'ordre : lire max/min, appliquer
  `passive_fire_mastery` (`329`), puis tirer `min + random(max-min)` avec borne
  maximum exclusive.
- Burn est appliqué par `SUNITDMG_ApplyBurnDamage` à `0x451380`, puis stocké
  comme `STAT_HPREGEN` négatif; les ticks ultérieurs ne traversent donc pas la
  table élémentaire habituelle.
- Lorsque dommage et durée sont positifs, le chemin natif appelle
  `STATES_ToggleState 0x3354C0` avec le state `burning` 115. Dans les tables
  vanilla 3.2, vanilla 3.3 et BKVince, ce state référence l'overlay `burning`
  224 et l'asset `Expansion\\On_Fire`; l'overlay n'est donc pas absent des
  données actuelles.
- `SUNITDMG_ApplyResistancesAndAbsorb` à `0x4523E0` reçoit la table de douze
  records. Le record Fire D2R de `0x40` octets utilise résistance `39`, maximum
  `40`, pierce `333`, pierce d'immunité `189`, absorb `%/plat` `142/143`, index
  de réduction `2`, flag `+0x28=1` et log flag `8`.
- Le troisième argument historique nommé `dontAbsorb` ne signifie pas seulement
  « ignorer absorb » : avec la valeur `1`, le témoin unique `0x45251F` applique
  `min(résistance,0)`. La 2.0.0 supprimait donc par erreur toute résistance
  positive et toute immunité. La correction appelle le résolveur avec `0`, met
  les deux stats absorb à `-1` et conserve explicitement le slot MDR `2` à zéro.
- L'affirmation Discord d'environ « 200 DPS » n'est pas démontrée comme une
  constante exacte; la constante réellement prouvée est l'ID de stat `316`.

D2MOO est épinglé à
`19019806df7f3e877fa105b05395d1e3597e2316` et sert uniquement de preuve
sémantique. Aucune adresse, structure ou ABI 32 bits n'est transposée.

## Architecture implantée — candidate 2.2.0

La 2.2 conserve toute l'architecture mécanique et visuelle de la 2.1, puis
remplace seulement la source du visuel stationnaire :

1. Le compilateur natif prouve des records StatesTxt de stride `0x44`, leur
   vecteur à `DataTables+0x290`, leur count à `+0x298`, et le premier overlay
   word à `record+0x02`. L'initialiseur D2R de ces records prouve `0xFFFF` comme
   sentinelle vide. Toutes ces séquences font partie de l'empreinte fail-closed.
   Le témoin de stride s'arrête au préfixe unique de 22 octets qui encode
   `0x44`, avant le `CALL` de compilation que D2RLoader peut légitimement
   rediriger; Burn Damage Fix n'utilise ni ne revendique la cible de ce `CALL`.
2. Juste avant le trampoline original de `0x451380`, la DLL résout le contexte
   du défenseur par `GetItemDataContext 0x34A0E0` et
   `GetDataTablesForContext 0x300A90`, vérifie count, base, stride, id 115,
   alignement et page writable, puis exécute un compare/exchange atomique
   strict `224 -> 0xFFFF`.
3. Un row déjà vide est accepté. Tout autre id d'overlay est classé custom et
   préservé; un layout ou accès invalide échoue seulement cette suppression et
   laisse les mécaniques Burn actives. Le statut `burn-damage-fix` expose les
   compteurs `removed/already-none/custom/fail/restored`.
4. Au déchargement, `224` n'est restauré que si la DLL avait elle-même modifié
   cette cellule exacte et qu'elle vaut encore `0xFFFF`. Une modification
   tierce arrivée ensuite n'est jamais écrasée. Aucune donnée persistante,
   sauvegarde ou table source n'est modifiée.
5. `overlay.suppress_native_burning=true` est inclus dans le TOML 2.2, mais la
   clé est optionnelle en lecture et vaut `true` par défaut pour les TOML
   `config_version=2` existants. Elle n'est effective que lorsque le replay
   `fire_hit` est actif, afin de ne jamais retirer le seul visuel sans
   remplacement.

Deux builds Release x64 propres de la candidate corrigée sont byte-identiques :
193 536 octets, SHA-256
`5A7B4D3304CE1802DAB56EEB2787AFC16DB066F42EB5DE9BD2E999AA2F8F6752`.
Les deux CTest passent `1/1`; `/W4 /WX`, métadonnées 2.2.0, configuration
embarquée, x64, ASLR/NX, dépendances runtime Microsoft et exactement les trois
exports D2RLoader sont validés. La DLL ne contient aucune allowlist texte
`92777/93847`.

La première candidate 2.2, SHA-256
`9F3E02924E19B84CBC15F2E9E9541D4E3024218F457EA4EE4F8B5C6DF619707E`,
refusait proprement son chargement parce que son témoin de stride incluait le
`CALL` suivant, déjà redirigé par l'intégration compilateur de D2RLoader. Le
témoin corrigé conserve les 22 octets uniques qui prouvent réellement le stride
`0x44` sans revendiquer ce `CALL`. La candidate corrigée est déployée dans le
runtime mod-local BKVince avec le TOML 2.1 existant laissé intact afin de valider
la valeur par défaut de migration.

Le cold start officiel D2R 3.3.93847 du 27 août 2026 atteint `24/24` avec Burn
Damage Fix 2.2.0 actif, Monster Display 1.5.8-altf4-teardown-fix, Bind And Summon
1.4.3-embedded110, Floating Damage 1.4.2, la Suite et les cinq plugins eezstreet.
Aucun nouveau crash n'est créé. Le seul plugin refusé est le défaut préexistant
Fourth Skill Tree Framework, hors du périmètre Burn Damage Fix.

Le témoin gameplay BKVince du même démarrage valide le chemin nominal complet :
DoT actif, replay `fire_hit` continu et attaché à une cible mobile, grosse flamme
native stationnaire absente, nombres périodiques Floating Damage actifs, kill
credit/XP conservé et aucun crash. Le statut après impact rapporte
`native-burning=on/1/0/0/0/0`, `resolved=28/0`, `burning-state=28/0` et
`overlay-replay=317/303096/0`. Le log frais confirme à `07:57:17` la suppression
unique de l'overlay natif `224` dans le contexte data actif; le nombre de fichiers
de crash reste inchangé à 49 avant et après le test.

Un Save & Exit suivi d'une nouvelle partie dans le même processus répète le test
avec succès et la flamme native demeure absente. Ce témoin valide la transition
partie -> frontend -> nouvelle partie; il ne constitue pas un déchargement de la
DLL, puisque D2RLoader et la mutation process-local restent actifs jusqu'à la
fermeture complète du processus.

## Release candidate Suite 2.2.0 — 2026-08-27

Vincent demande explicitement de retirer toute restriction de version D2R et de
préparer la release pour la RuffnecKk D2RLoader Suite. Le code ne compare aucun
`build-name` et ne contient aucun numéro de build cible; le nom observé reste
uniquement journalisé. Le contrôle `apiVersion` de D2RLoader demeure obligatoire
comme garde-fou d'ABI et ne filtre aucune version du jeu. CTest lit désormais le
source du plugin et échoue si une allowlist `92777/93847`, un prédicat
`IsSupportedBuild`, un message `Unsupported D2R build` ou une comparaison de
`RuntimeBuild` réapparaît.

Les deux rebuilds Release x64 après ajout de ce test passent `1/1`, restent
byte-identiques et conservent le SHA-256 du binaire validé en jeu. Le binaire est
promu dans `package/` et l'archive locale gitignorée est préparée avec une
allowlist de deux entrées seulement :

- `BurnDamageFix.dll` — 193 536 octets — SHA-256
  `5A7B4D3304CE1802DAB56EEB2787AFC16DB066F42EB5DE9BD2E999AA2F8F6752`;
- `burn-damage-fix.toml` — 965 octets — SHA-256
  `8C2831F4A1BE9647757DDBDE7C9FE089FFF7008181DB9A0B5AD45FA4E9BB17C9`;
- `BurnDamageFix-2.2.0.zip` — SHA-256
  `DA90303028A3A2D1030A8935B042151F4DA209F3075FDA008D335E36425B6FA8`.

Le README anglais, avec crédit D2MOO explicite, demeure à côté du ZIP et n'est
pas inclus dans l'archive générée. La candidate est préparée pour relecture;
aucune GitHub Release n'est créée par cette demande.

## Architecture implantée — fondation 2.1.0

La candidate 2.1.0 conserve les deux seams 2.0.0 et ajoute les contrats
suivants :

1. Le hook `0x451380` construit le record Fire complet de `0x40` octets et
   appelle le résolveur vivant `0x4523E0` avec le troisième argument `0`.
   Résistance positive, cap et immunité restent donc actifs; les stats Fire
   pierce et `item_pierce_fire_immunity` restent natives. Fire Absorb est exclu
   par les sentinelles `-1/-1` et MDR par `reductions[2]=0`.
2. Une résolution qui échoue ou retourne zéro ne traverse pas le trampoline
   d'application Burn. Le comportement est fail-closed : aucune immunité ne
   retombe silencieusement sur le dommage non résolu.
3. Après une application positive confirmant le state `burning` 115, le plugin
   appelle `UNITS_SetOverlay 0x349020` avec `fire_hit` 81. Les dispatchers
   d'événements joueur `0x42CE30` et monstre `0x447420` répètent ce visuel avant
   le callback original, seulement pour l'événement stat-regen 3, à la cadence
   `Game.frame % repeat_frames == 0`, si le state existe encore et si
   `SUNIT_IsDead 0x34C2C0` retourne zéro. Aucun pointeur, GUID, thread ou état
   parallèle n'est conservé.
4. L'entrée partagée `0x4523E0` doit être vanilla exacte et sans propriétaire,
   ou porter exactement un inline hook suivi par DiagnosticsService dont le
   propriétaire est `monsterdisplay`. Dans les deux cas, les témoins internes
   de record, résistance, pierce, réduction et absorb restent stricts. Sans
   DiagnosticsService, seule l'entrée vanilla exacte est acceptée.
5. Le numéro de build n'est plus une allowlist. Chaque RVA, signature, helper,
   layout et ownership utilisé par une fonctionnalité activée appartient à son
   empreinte fail-closed. Les noms `92777/93847` sont seulement diagnostiques;
   la matrice runtime complète porte sur la version officielle courante et la
   couverture 92777 est héritée seulement par équivalence native byte-exact.

Le TOML courant passe à `config_version=2` et ajoute `[overlay]` avec
`enabled=true` et `repeat_frames=10` par défaut (`1..250`, soit 0,4 seconde de
temps serveur). `fire_hit` porte huit frames à `AnimRate=16`, que la référence
d2rdoc définit comme seize frames d'animation par seconde. Les TOML version 1
restent acceptés mais gardent le replay désactivé jusqu'à leur migration. Le
replay visuel est refusé si la résolution Fire Resistance est désactivée.

`UNITS_SetOverlay` alloue/réutilise une stat-list portant le flag interne `0x80`,
mais écrit l'ID visuel dans `unit_dooverlay` stat `178`; le témoin natif unique
`0x34916C` le prouve et les tables BKVince/vanilla 3.3 confirment les IDs. Cette
stat conserve le dernier overlay direct écrit, pas la durée de son animation :
elle ne peut donc pas servir de verrou d'activité. Le canal direct demeure
last-write-wins. BurnDamageFix compte une valeur étrangère puis restaure
`fire_hit` à sa cadence; les mécaniques Crushing Blow de BKVCombat restent
inchangées, mais l'arbitrage visuel doit être qualifié en jeu.

Le build Release x64, les tests de politique/configuration et le témoin gameplay
BKVince 93847 ont validé la fondation 2.1 le 26 août 2026 : résistance et
immunité Fire, DoT, kill credit, XP, Floating Damage périodique, `fire_hit`
continu et réémis sur une cible mobile, sans double popup ni crash dans le
témoin final. Le défaut restant est précisément l'overlay natif `burning` 224
stationnaire que la candidate 2.2 remplace process-localement.

## Architecture historique — 2.0.0

`BurnDamageFix.dll` possède deux seams stricts :

1. Un mid-hook rel32 de six octets à `0x44CB32` remplace le `ADD EBX,316`. Un
   relais MASM transmet l'attaquant, le dommage existant et le random courant au
   normalisateur, recrée le flag de signe consommé par le `CMOVS` vanilla et
   reprend à `0x44CB38`.
2. Un inline hook à `0x451380` reconstruit le contexte et le record Fire exact,
   résout la quantité Burn, puis appelle le trampoline original.

Lorsque `diagnostics.enabled=true`, le second hook appelle ensuite le prédicat
natif emprunté `STATES_CheckState 0x3351B0` sur le défenseur et le state 115,
uniquement si le Burn résolu et sa durée sont positifs. Les compteurs
`burning-state active/missing` sont passifs : aucun toggle, overlay ou dommage
n'est fabriqué par le témoin.

Dans cette version historique, l'adresse `0x4523E0` était appelée vivante sans
validation de son entrée. La 2.1.0 remplace cette confiance ouverte par le
contrat d'ownership suivi décrit plus haut, tout en laissant Resistance Floor
conserver ses seams internes `0x4524C4/0x4524E7` dans le trajet.

La configuration TOML stricte est indépendante :
`burn-damage-fix.toml`, `config_version=1`, master switch, normalisation,
résistance et diagnostics. L'ordre de résolution est mod actif, portée de la
DLL, puis global. Le SDK v3 épinglé est
`4933e2c42cb2592958cd0df3b6dc5003102252d1`.

Le plugin 2.0.0 n'altérait ni durée/ticks, ni sauvegardes, ni paquets, ni
attribution de kill/XP. Son ancienne allowlist acceptait exactement `92777` et
`93847`; ce comportement n'est pas conservé par la 2.1.0.

## Audit de propriété et coexistence

- Scan de toutes les DLL actives : aucun témoin exact de `0x44CB32` et aucune
  référence brute à `0x451380` hors BurnDamageFix.
- `monsterdisplay.dll` 214 016 octets, SHA-256
  `0E66049A6E359A58CD926A00B8316401B04F4C7756DA3655053C1278173E5AA0` :
  une référence à `0x4523E0`, zéro référence aux deux seams possédés.
- `BindAndSummon.dll` du profil 3.3, 166 400 octets, SHA-256
  `6385F1B767E5F5705E8D920A3C3465A9E4127361B2A0B6FAC391A8AF6912B40C` :
  zéro référence aux trois sites.
- Le profil isolé 92777 porte une autre build de Bind And Summon, 196 096
  octets, SHA-256
  `AEEABBE6371510859E2CA1117BBE116B98691FA3F45FC4A9BF616B43F7A6EB7A`;
  son scan et ses cold starts propres ne trouvent aucun overlap.
- Melee Splash possède l'entrée externe `0x44C030`, sans overlap de bytes avec
  le seam interne `0x44CB32`.
- Le PluginPack épinglé
  `dc75b49ffbb67b887d7757ee00ee9a03bcde5d8a` ne contient aucun propriétaire
  des deux seams Burn. Aucun binaire eezstreet n'est modifié, lié ou redistribué.

Le détail d'ownership et d'ordre de chargement est gouverné dans
`addons/BurnDamageFix/NATIVE-HOOKS.md`.

## Résultats techniques historiques 2.0.0 — 2026-08-25

- Build Release x64 propre : PASS.
- Tests CTest de politique/config/build/math/rel32 : `1/1` PASS.
- Reproductibilité `/Brepro` : deux builds propres byte-identiques.
- DLL : 164 864 octets, SHA-256
  `56555AA60FC284CA6A691526827124E9BFA7E9D8C6312C09523AC7AFD522C48B`.
- Audit PE : x64, trois exports D2RLoader requis, aucune dépendance envers une
  DLL eezstreet, Monster Display ou Bind And Summon.
- ZIP candidat : exactement `BurnDamageFix.dll` et
  `burn-damage-fix.toml`, SHA-256
  `4C4C118D40656CADB395F41B671EE47C2820D2D0FB3C60E7F3B2E55C4B3196F0`.

## Matrice runtime historique 2.0.0

| Build / portée | Suite complète | 5 eezstreet | Monster Display | Bind And Summon | Résultat |
|---|---:|---:|---:|---:|---|
| 3.2.92777 mod-local | présente; 1 défaut préexistant | toutes actives | PASS | PASS 1.2.3 | 24/24, 30 chargés, 19 patches; seul `RogueScoutMovement` échoue |
| 3.2.92777 global | présente; 1 défaut préexistant | toutes actives | PASS | PASS 1.2.3 | 24/24, 30 chargés, 19 patches; même défaut Rogue |
| 3.3.93847 mod-local | PASS | toutes actives | PASS | PASS 1.4.3 | 24/24, 34 chargés, 18 patches, aucun échec |
| 3.3.93847 global | PASS | toutes actives | PASS | PASS 1.4.3 | 24/24, 34 chargés, 18 patches, aucun échec |

Sur les deux builds, les ordres Burn → Bind/Monster, Bind → Burn → Monster et
Monster → Burn atteignent le démarrage complet. L'installation simultanée dans
les deux portées charge la version mod-locale et neutralise proprement le
duplicata global par `mod version already active`.

D2RLoader 1.1.0-beta avertit encore que 3.3 est hors de sa plage déclarée
`[3.0,3.3)`, mais il poursuit : le log frais reconnaît `93847`, charge la pile
et atteint 24/24. Le profil BKVince signale aussi que ses données portent encore
la version 92777; ces avertissements sont des réserves de baseline, pas des
échecs de chargement de BurnDamageFix.

Le witness a aussi été chargé avec `diagnostics.enabled=true` et toutes les
fonctions eezstreet actives. Une première tentative 93847 a produit une access
violation dans `dxgi.dll+0x38B1C1` pendant l'initialisation graphique 15/24,
avant tout événement Burn et sans frame BurnDamageFix dans le rapport. Le dump,
le rapport et les logs sont conservés sous `analysis-cache`. Deux cold starts
strictement identiques immédiatement suivants ont atteint 24/24 avec 34 plugins,
18 patches et aucun échec. L'incident est donc non reproductible à ce stade et
ne démontre pas une faute du witness, mais il reste une réserve runtime au lieu
d'être effacé du dossier de preuve. Le fichier eezstreet maximal temporaire a
été retiré du jeu installé après la matrice; BurnDamageFix demeure mod-local
avec ses diagnostics activés pour le prochain témoin gameplay.

## Témoin gameplay BKVince historique 2.0.0 — 2026-08-26

Le laboratoire a été exécuté directement dans le profil BKVince 3.3.93847 avec
Fire Ball niveau 2 convertie en Burn, `36..52` dégâts, `ELen=500` frames et
`HitShift=7`. Le testeur a confirmé : DoT oui, kill credit/XP oui, visuel oui,
mais visuel restant au point d'impact lorsque la cible se déplace. Le statut
frais du plugin après deux applications était : `production=0/0`, `resolved=2`
et `burning-state=2/0 active/missing`.

Ces compteurs prouvent que les deux applications ont traversé le résolveur et
que le state natif 115 était actif. `production=0/0` est attendu pour ce témoin
Fire Ball : ce producteur ne traverse pas le seam générique `0x44CB32`.

L'observation visuelle ne qualifie toutefois pas encore l'overlay d'état 224.
Fire Ball crée séparément le missile client `fireexplosion2` par
`CltHitSubMissile1`; celui-ci est une explosion au point d'impact avec
`Range=12`. Le testeur a précisé que ce feu dure environ deux secondes, et non
les vingt secondes du DoT. Cette durée confirme fortement qu'il observait
l'explosion Fire Ball plutôt que l'overlay Burn persistant; le résultat
fonctionnel est donc reclassé `overlay persistant=non` et
`overlay follows target=N/A`.
Les lignes `burning` de `states.txt` et `overlay.txt` sont identiques dans
BKVince, vanilla 3.2 et vanilla 3.3. `overlay.txt` ne contient aucun champ
d'attachement : les mêmes champs servent à Fade, Conviction et aux malédictions
qui sont rendues sur l'unité. La correction n'est donc pas démontrée comme une
simple cellule `overlay.txt`.

Les A/B visuels suivants ont ensuite été exécutés : Decrepify était visible et
suivait l'unité; Fire Golem ne produisait aucun visuel; Enchant était un effet
one-shot; l'effet Holy Fire était continu mais ses particules restaient environ
cinq secondes à leur point d'émission au lieu de suivre la cible. Le testeur a
finalement retenu le principe de répéter directement l'effet de réception de
dégâts Fire (`fire_hit`) sur l'unité pendant le vrai state Burn. Une session de
contrôle a crashé pendant les tirs, puis un retest d'environ dix Fire Ball avec
Decrepify visible et DoT actif n'a pas reproduit le crash.

Le même témoin a révélé l'anomalie prioritaire : le Burn affectait encore des
monstres Fire Immune. La preuve native a ensuite attribué cette faute au
troisième argument `1` du résolveur 2.0.0, qui écrasait les résistances
positives à zéro. Ces essais motivent les deux changements 2.1.0; ils ne
qualifient pas encore la nouvelle DLL.

## Gates restants

1. La portée mod-locale 3.3.93847 et la coexistence de démarrage complète sont
   validées. Qualifier encore l'installation globale et l'ordre inverse
   MonsterDisplay/BurnDamageFix. La couverture 3.2.92777 ne demande pas une
   seconde matrice runtime tant que le corpus gouverné prouve byte-exact toutes
   les surfaces natives utilisées.
2. En jeu, comparer résistance Fire négative, nulle, positive, cappée et immune.
   Fire Immune sans sunder/pierce doit recevoir zéro Burn; le pierce d'immunité
   légitime doit rester natif. Confirmer aussi l'exclusion MDR/Fire Absorb.
3. La suppression de `Expansion\\On_Fire` et le replay mobile `fire_hit` sont
   validés sur Fire Ball. Tester encore malédiction, aura, Crushing Blow
   BKVCombat et réapplication intensive.
4. Save & Exit/reload dans le même processus est validé. Tester encore les rôles
   hôte et joiner lorsque Burn doit être qualifié en multijoueur.
5. Tester `suppress_native_burning=false`, un row déjà `0xFFFF`, puis un id
   custom afin de prouver respectivement le rollback fonctionnel, le no-op et la
   préservation tierce. Vérifier aussi la restauration conditionnelle lors d'un
   déchargement propre.

Le défaut préexistant `RogueScoutMovement` sous 92777, l'échec courant de Fourth
Skill Tree Framework et le crash graphique `dxgi.dll` non reproduit restent des
incidents de pile suivis séparément. Ils ne sont pas attribués à BurnDamageFix,
mais empêchent de présenter leur démarrage comme une pile Suite entièrement
verte. L'archive candidate peut être inspectée; sa publication reste distincte
de ces gates de qualification.

## Historique 1.0.0

Le prédécesseur `BurnFireResistance 1.0.0` ne hookait que `0x451380` et avait
réussi les cold starts 92777 mod-local/global avec Monster Display. Son premier
prototype, propriétaire de `0x4523E0`, empêchait Monster Display de charger et a
été rejeté. Cette preuve A/B motive le contrat live-resolver de 2.0.0; elle ne
qualifie pas le nouveau seam `0x44CB32` ni les builds actuelles des autres DLL.

## Rollback

Fermer D2R puis retirer `BurnDamageFix.dll` et son TOML. Ne restaurer
`BurnFireResistance.dll` que si l'ancien comportement résistance-only est
explicitement voulu, et jamais en parallèle. Aucun save ni fichier data ne
demande de migration; le patch kill-credit peut rester actif.
