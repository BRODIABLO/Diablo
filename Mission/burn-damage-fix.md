# BurnDamageFix — D2R 3.2.92777 et 3.3.93847

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
`MonsterDisplay.dll` et `BindAndSummon.dll` est un gate de livraison séparé pour
les builds `3.2.92777` et `3.3.93847`. Aucun plugin ni aucune fonction du
PluginPack ne peut être désactivé pour fabriquer un résultat passant.

Vincent a retenu l'option 1 et donné `GO overlay witness` le 25 août 2026.
BurnDamageFix doit observer, sans le forcer, le state natif `burning` (ID 115)
après une application Burn positive lorsque les diagnostics sont activés. Le
witness distingue `state active` de `state missing`; il ne crée aucun overlay,
ne retoggle aucun state et ne transforme pas un Burn annulé par immunité en
fausse anomalie.

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
  records. Le record Fire utilise les stats `39/40`, le type `fire`, l'index de
  réduction `2` et le log flag `8`. L'argument `dontAbsorb=1` conserve
  résistance, cap, difficulté, immunité et pierce tout en excluant Magic Damage
  Reduction et Fire Absorb.
- L'affirmation Discord d'environ « 200 DPS » n'est pas démontrée comme une
  constante exacte; la constante réellement prouvée est l'ID de stat `316`.

D2MOO est épinglé à
`19019806df7f3e877fa105b05395d1e3597e2316` et sert uniquement de preuve
sémantique. Aucune adresse, structure ou ABI 32 bits n'est transposée.

## Architecture implantée — 2.0.0

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

L'adresse `0x4523E0` est appelée vivante et n'est jamais validée contre ses
octets vanilla. Cette décision est essentielle : Monster Display peut déjà
avoir hooké l'entrée, ou la hooker plus tard, et le pointeur demeure l'adresse
du même code modifiable. Resistance Floor conserve aussi ses seams internes
`0x4524C4/0x4524E7` dans le trajet.

La configuration TOML stricte est indépendante :
`burn-damage-fix.toml`, `config_version=1`, master switch, normalisation,
résistance et diagnostics. L'ordre de résolution est mod actif, portée de la
DLL, puis global. Le SDK v3 épinglé est
`4933e2c42cb2592958cd0df3b6dc5003102252d1`.

Le plugin n'altère ni durée/ticks, ni sauvegardes, ni paquets, ni attribution de
kill/XP. Il accepte exactement `92777` et `93847` et refuse les autres builds.

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

## Résultats techniques — 2026-08-25

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

## Matrice runtime

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

## Gates restants

1. Corriger puis retester séparément le défaut préexistant de
   `RogueScoutMovement` sous 92777 sans le désactiver; les composants demandés
   Burn/Monster/Bind et les cinq eezstreet sont déjà PASS.
2. En jeu, comparer des applications Burn identiques contre résistance Fire
   négative, nulle, positive, cappée et immune, avec pierce, difficultés,
   réapplication, solo/hôte/joiner, mort, kill credit, XP, Save & Exit/reload.
3. Avec les diagnostics activés, provoquer un Burn positif et rapprocher
   `burning-state active/missing` du dommage observé et de l'overlay visible;
   aucun événement gameplay n'a été déclenché pendant les cold starts.
4. Confirmer que Fire Absorb et Magic Damage Reduction ne modifient pas Burn et
   que les dégâts Fire immédiats restent inchangés.
5. Valider l'arrêt propre après une partie réelle; les profils ont été fermés
   avant de charger les sauvegardes afin de ne pas accepter leurs avertissements
   de migration.
6. Surveiller la reproductibilité de l'unique crash graphique `dxgi.dll` lors
   des prochaines sessions diagnostics; deux relances identiques réussies ne
   suffisent pas à supprimer cette réserve.

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
