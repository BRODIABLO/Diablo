# Portage du Floating Damage D2RLAN — BKVince 3.2

## Intention

Implanter dans BKVince sous D2RLoader l'affichage **Floating Damage** fourni
par D2RLAN/D2RHUD 2.4, en conservant son comportement et tous ses paramètres
par défaut. La fonctionnalité doit être activée dans le preset initial du mod,
mais rester désactivable et configurable par le joueur.

La cible gouvernée est `D2R.exe 3.2.92777`. Les RVA et signatures du build
D2R 2.4 ne doivent jamais être réutilisés sans preuve sur cette cible.

## Référence fonctionnelle

La référence locale actuelle est l'implantation D2RHUD 2.4 :

- `C:/Workspaces/D2RHUD-2.4-TCP/d2rhud/plugin/FloatingDamage.h` ;
- `C:/Workspaces/D2RHUD-2.4-TCP/d2rhud/plugin/FloatingDamage.cpp` ;
- capture des dégâts dans
  `C:/Workspaces/D2RHUD-2.4-TCP/d2rhud/plugin/D2RHUD/D2RHUD.cpp` ;
- rendu et polices embarquées dans
  `C:/Workspaces/D2RHUD-2.4-TCP/d2rhud/D3D12Hook.cpp` ;
- preset TCP actif dans `C:/Games/D2RLAN/D2R/HUDConfig_TCP.json`.

Le portage doit afficher au-dessus des monstres les dégâts réellement observés
par le client local, gérer les coups normaux et critiques, agréger les impacts
rapides sur une même cible, afficher les ticks secondaires, répartir les nombres
pour limiter les chevauchements et proposer le compteur DPS glissant de la
référence.

## Paramètres par défaut à conserver

La configuration livrée doit reprendre exactement les valeurs logiques
ci-dessous. Si le format retenu est TOML, les clés, sections et commentaires du
fichier devront être rédigés en anglais.

### Activation et apparence

| Paramètre | Valeur |
|---|---:|
| Enabled | `true` |
| TextSize | `38.0` |
| CriticalHitSize | `48.0` |
| TextOutlineWidth | `1` |
| ShadowLeftRightOffset | `0.0` |
| ShadowUpDownOffset | `0.0` |
| MaxNumbersOnScreen | `160` |
| FontIndex | `0` — Exocet |
| ColorByDamageType | `false` |

### Animation

| Paramètre | Valeur |
|---|---:|
| DisplayTimeSeconds | `0.85` |
| CriticalDisplayTimeSeconds | `0.95` |
| FadeOutStart | `0.75` |
| SpawnSize | `0.01` |
| PopBounceSize | `1.75` |
| PopInTimeSeconds | `0.08` |
| SettleTimeSeconds | `0.12` |
| UpwardDriftSpeed | `45.0` |
| SidewaysSpread | `0.0` |
| SpawnHeightOffset | `0.0` |

### Agrégation et ticks

| Paramètre | Valeur |
|---|---:|
| EnableHitCombining | `true` |
| MaxCombinedHitSize | `999999` |
| CombineWindowMs | `500` |
| ExtendDisplayOnHitSeconds | `0.52` |
| HitPulseSize | `1.24` |
| HitPulseTimeSeconds | `0.13` |
| ShowTickPopups | `true` |
| TickPopupTimeSeconds | `0.70` |
| TickPopupSize | `0.60` |
| TickPopupTravel | `64.0` |
| TickPopupHeightOffset | `-28.0` |

### Répartition spatiale

| Paramètre | Valeur |
|---|---:|
| SpreadNumbersHorizontally | `true` |
| NumberOfColumns | `7` |
| ColumnSpacing | `40.0` |
| StackHeightStep | `24.0` |
| ColumnReuseTimeSeconds | `0.60` |
| MaxStackHeight | `96.0` |

### Compteur DPS et aperçu

| Paramètre | Valeur |
|---|---:|
| ShowDpsCounter | `true` |
| HorizontalPositionPercent | `2.0` |
| VerticalPositionPercent | `98.0` |
| DpsSampleTimeSeconds | `5.0` |
| PreviewNumberCount | `8` |
| PreviewSpread | `32.0` |

### Couleurs RGBA

| Usage | Valeur |
|---|---|
| Normal / Physical | `(0.92, 0.92, 0.88, 1.0)` |
| Critical | `(1.0, 0.84, 0.27, 1.0)` |
| Fire | `(1.0, 0.45, 0.12, 1.0)` |
| Lightning | `(1.0, 0.95, 0.35, 1.0)` |
| Cold | `(0.45, 0.78, 1.0, 1.0)` |
| Poison | `(0.35, 0.90, 0.30, 1.0)` |
| Magic | `(0.72, 0.45, 1.0, 1.0)` |
| Outline | `(0.16, 0.11, 0.03, 1.0)` |
| Shadow | `(0.16, 0.11, 0.02, 1.0)` |

Les douze polices embarquées de la référence sont : Exocet, Akaya Telivagala,
ReggaeOne, SansitaSwashed, DM Mono, Girassol, Turret Road, Literata, Zilla Slab,
Aref Ruqaa, Formal 436 et PoE. Exocet reste le choix initial.

## Contraintes d'implantation

1. Commencer par `npm run re:d2r32 -- status`, puis réutiliser le workbench
   persistant et le projet Ghidra gouverné pour toute identification native.
2. Auditer séparément la capture des dégâts, l'attribution à la cible, la
   projection monde-écran, le rendu D3D12/ImGui, les polices et la persistance
   de configuration. Ne pas présumer que l'API plugin D2RLoader expose toutes
   ces surfaces.
3. Distribuer un composant installable soit globalement dans
   `<D2R>/d2rloader/`, soit par mod dans `<D2R>/mods/<mod>/d2rloader/`. Toute
   interception native doit être verrouillée au build et aux signatures
   attendues, puis refuser proprement un binaire incompatible.
4. Le système reste visuel et client-only : il ne doit modifier ni les dégâts,
   ni les paquets de combat, ni l'attribution des kills, ni l'expérience, ni le
   loot, ni la simulation serveur.
5. Éviter le double comptage des dégâts périodiques, des messages de combat et
   des impacts rapides. Les dégâts affichés et le DPS doivent provenir d'un
   même événement logique dédupliqué.
6. Fournir une commande ou une interface permettant l'activation, le réglage,
   la prévisualisation, la restauration des valeurs par défaut et la sauvegarde
   persistante de la configuration.

## Gate de validation

- compilation Release x64, exports et chargement à froid sans erreur ;
- contrôle strict du build 92777 et des signatures utilisées ;
- comparaison visuelle avec la référence D2RLAN aux réglages par défaut ;
- coups normaux et critiques, mêlée, ranged, sorts, missiles et dégâts de zone ;
- dégâts physiques, feu, foudre, froid, poison et magie ;
- dégâts périodiques, impacts très rapides, agrégation par cible, plafond de
  nombres, colonnes, fade, ticks secondaires et compteur DPS ;
- monstres normaux, champions, uniques, superuniques, boss et groupes nombreux ;
- résolution, mode fenêtré/plein écran, redimensionnement, différentes échelles
  d'interface, souris et manette ;
- joueur seul, mercenaire, summons et parties solo, hôte et joiner ;
- sauvegarde/rechargement des paramètres, restauration des valeurs par défaut
  et désactivation complète ;
- absence de crash, fuite, baisse durable de performances, double comptage,
  désynchronisation et modification du gameplay.

## Implantation livrée — 19 juillet 2026

Le portage est livré comme plugin natif D2RLoader installable globalement ou
par mod :

- `data-BKVince/d2rloader/plugins/FloatingDamage.dll` est le binaire Release
  x64 distribué ; ses sources reproductibles sont conservées dans
  `data-BKVince/d2rloader/plugins/FloatingDamage-src/` ;
- `data-BKVince/d2rloader/config/floating-damage.toml` contient toutes les
  valeurs par défaut ci-dessus, en anglais, avec la fonctionnalité activée ;
- la capture client intercepte `DAMAGE_LogResolvedType` à la RVA `0x427150`,
  appelée depuis `SUNITDMG_ApplyResistancesAndAbsorb` à la RVA `0x4523E0` ;
  le hook exige le build `92777` et une signature stricte unique de 26 octets ;
- l'original est toujours appelé avec ses arguments inchangés. Le plugin ne
  modifie aucune valeur de dégâts ni aucun état de gameplay ;
- l'overlay privé DirectX 12/ImGui embarque les douze polices D2RLAN et porte
  le rendu, l'agrégation, les ticks, les colonnes, les animations et le DPS ;
- la commande D2RLoader
  `floating-damage [status|on|off|toggle|preview|reload|reset]` permet le
  contrôle, la prévisualisation et la persistance de la configuration.

La compilation Release x64 et les exports ont réussi. Un démarrage à froid
réel sous BKVince a chargé le hook et l'overlay ; le compteur `DPS 0` a été
constaté visuellement au menu puis en jeu. Le premier test d'arrêt a révélé une
libération trop tardive des références GPU pendant l'extinction de D3D12Core.
Le stockage de ces références est désormais borné à la vie du processus, tout
en restant explicitement nettoyé lors d'un unload normal. Le cycle final
démarrage/fermeture s'est achevé sans nouveau rapport de crash et le log a
confirmé l'exécution propre de `FloatingDamage stopped`.

Le smoke test final n'a pas produit d'attaque réussie (`captured=0`,
`displayed=0`) : la preuve du point de capture repose donc sur l'analyse native
persistante et sa signature stricte, tandis que la matrice de combat étendue du
gate ci-dessus reste une campagne de non-régression à rejouer lors des futures
évolutions de D2RLoader, du mod ou du build du jeu.

## Évolution 1.2.0 — 11 août 2026

La présentation a été adaptée aux résolutions modernes sans modifier le point
de capture natif ni la simulation de combat :

- la notation compacte commence à `1,000` (`1k`, `1.3k`, `1m`, `1.3m`, puis
  `b` et `t`) et utilise un point comme séparateur décimal ;
- les tailles TOML restent des pixels de référence 4K/2160p. Le rendu applique
  un facteur fondé sur la hauteur d'affichage : `0.333` à 720p, `0.5` à 1080p,
  `0.667` à 1440p et `1.0` à 2160p. Les écrans ultrawide suivent leur hauteur,
  pas leur ratio ;
- `CTRL+SHIFT+D` bascule l'affichage pour la session par un simple appui. La
  liaison est configurable avec lettres, chiffres, `F1`–`F24`, touches de
  navigation/édition, ponctuation, `MOUSE3`–`MOUSE5` et modificateurs exacts
  `CTRL`, `SHIFT` et `ALT` ;
- le plugin sonde l'état de la touche uniquement lorsque la fenêtre du jeu a le
  focus. Il n'intercepte ni ne consomme le message d'entrée : la même touche
  continue donc d'être reçue par D2R et par le chat ;
- l'état d'activation partagé avec le hook de dégâts est atomique et la
  désactivation demande au thread de rendu de vider les nombres et le DPS, ce
  qui évite les accès concurrents avec l'overlay.

La DLL Release x64 finale porte le SHA-256
`E398E3355A6060BA73184DB4C012CA3A52A05C97E0E8F9E7D7BD2C80CFA9D97F`.
La configuration distribuée porte le SHA-256
`4CF259813621B9184DC3DFDE20357452D9765E1E4C60148FE8D5007751EFC3E3`.
L'archive publique stricte contient uniquement `FloatingDamage.dll` et
`floating-damage.toml`; son SHA-256 est
`6A6BD72236B2D7EA0F185FDE57A3354F8BCB4CDC33D31DD453F751F6DDA424E9`.

La compilation, l'architecture x64, les exports et `git diff --check` passent.
Le cold start final avec la pile complète atteint `15/15` patchsets, `19/19`
plugins et `24/24` étapes ; FloatingDamage 1.2.0 installe son hook, initialise
l'overlay DX12, puis journalise un arrêt propre sans nouveau rapport de crash.
La taille visuelle aux différentes résolutions, le basculement réel du hotkey
et les seuils `k`/`m` restent à constater en jeu : ils ne sont pas revendiqués
comme tests fonctionnels réussis par cette validation automatisée.

## Évolution 1.2.1 — 12 août 2026

Un rapport vidéo a montré un coup physique pur retirant `4` HP visibles à un
zombie sans résistance alors que le popup affichait `3`. La cause était le
décalage fixe 8.8 appliqué séparément à chaque composante par l'ancien hook :
une valeur comme `992/256 = 3.875` était tronquée à `3`, même lorsque les bornes
entières des HP passaient de `662` à `658` et rendaient une perte visible de
`4`.

La correction finale ne fait ni `ceil`, ni arrondi arbitraire. Elle redirige
uniquement l'appel de cinq octets à `0x44D093`, couture unique où
`SUNITDMG_ExecuteEvents` transmet au setter natif les HP principaux du monstre.
Le contexte strict de 21 octets à `0x44D083`, ainsi que les signatures uniques
de `STATLIST_GetUnitStat` (`0x2F5020`) et `STATLIST_SetUnitStat` (`0x2F7D10`),
sont validés avant le patch. Un relais privé proche conserve `attacker` et
`D2Damage`, appelle le setter original avec ses quatre arguments inchangés,
puis affiche `(oldHp >> 8) - (newHp >> 8)`. Cette couture étroite ne prend pas
possession de l'entrée `SUNITDMG_ExecuteEvents`, qui reste détenue par
MeleeSplash dans la pile complète.

Une première tentative autour de `SUNITDMG_FinalizeDamage` a été rejetée au
runtime : le contrat gouverné confirme que la mutation HP précède cette étape,
et le test a produit `captured=0`. La DLL 1.2.0 a été restaurée immédiatement,
puis remplacée seulement après implantation du callsite autoritaire ci-dessus.

La projection ne remplace plus un attaquant absent par la cible elle-même. Les
deux chemins doivent avoir une position valide et l'ancre calculée doit se
trouver dans l'affichage courant; une mort hors écran est donc omise au lieu de
faire apparaître son nombre au centre, au-dessus du joueur.

Les assertions de compilation couvrent le cas juste sous `4.0` et le report du
résidu entre deux coups. Le cold start visible final a chargé `16/16` patchsets
et `19/19` plugins, sans désactivation, rejet ni échec. Vincent a confirmé le
retour de `DPS 0`; l'overlay et le hotkey étaient donc actifs. Le premier coup
observé a journalisé `fixed=169472->168480; popup=4`, soit exactement
`992/256 = 3.875`, puis la mise en file du popup a été confirmée. La garde de
mort hors écran reste à rejouer volontairement en gameplay avant de revendiquer
ce cas comme test fonctionnel réussi.

La DLL Release x64 finale porte le SHA-256
`D3F83F6BC24BA91BB4373BEEFA1A1C1CAF7A64790025B629CD0965AB713F6136`.
La configuration inchangée porte le SHA-256
`4CF259813621B9184DC3DFDE20357452D9765E1E4C60148FE8D5007751EFC3E3`.
L'archive publique stricte contient uniquement `FloatingDamage.dll` et
`floating-damage.toml`, conserve son README révisable à côté et porte le
SHA-256 `534DDC608F2EADED011BF4524176EEABE32DE6DA79A27397B75AA1723F0B109F`.

## Évolution 1.2.2 — 12 août 2026

Un rapport utilisateur couvrant les summons de Necromancer, Assassin et Amazon
a révélé que les dégâts de leurs unités apparaissaient autour du personnage au
lieu du monstre touché. La projection 1.2.1 utilisait l'attaquant comme origine
présumée de la caméra. Cette hypothèse n'était correcte que lorsque le joueur
frappait lui-même : avec un mercenaire, un Revive, une invocation, un piège, un
missile ou toute autre source distante, le déplacement cible-attaquant était
recentré à tort sur le personnage.

La version 1.2.2 rend la projection entièrement indépendante de la source des
dégâts et du skill employé. `CLIENT_GetLocalDataContext` (`0x8B2D0`) et
`CLIENT_GetLocalPlayer` (`0x9A480`) résolvent à chaque événement le joueur local
qui porte réellement la caméra; les deux signatures strictes sont uniques dans
l'image 92777. La position du popup est désormais calculée entre ce joueur
local et le monstre dont les HP viennent d'être commit. Aucun type de pet,
classe, skill, attaque mêlée/ranged, sort ou missile n'est listé ou filtré.

La cible doit toujours être `UNIT_MONSTER`; un joueur ne peut donc pas produire
un popup de dégâts reçus sur le personnage. Une cible projetée hors des bornes
de l'affichage reste omise. Cette même règle couvre les morts hors écran sans
réintroduire le fallback centré de l'ancienne implantation.

Le build Release x64 et les huit exports attendus passent. Les signatures des
deux lookups client ont chacune exactement un témoin sous 92777. Le cold start
visible final charge Floating Damage 1.2.2 en portée globale, applique `16/16`
patchsets, active `19/19` plugins sans désactivation, rejet ni échec et atteint
`24/24`. Une lecture mémoire du processus répondant confirme que l'appel HP à
`0x44D093` vise le relais privé `0x143EB0000`, et non plus le setter vanilla à
`0x1402F7D10`; le hook est donc réellement installé. Le placement visuel avec
mercenaire, Revive, réanimé, invocation, piège, ranged et tous les skills reste
à constater en gameplay et n'est pas revendiqué comme réussi par le cold start.

La DLL source, gouvernée et runtime porte le SHA-256
`959F9B50E66AC9BBB3AC1E383BC8355E62D7EBBBBD448605E28D6753A7B21650`.
La configuration inchangée porte le SHA-256
`4CF259813621B9184DC3DFDE20357452D9765E1E4C60148FE8D5007751EFC3E3`.
L'archive publique stricte contient uniquement `FloatingDamage.dll` et
`floating-damage.toml`, conserve son README révisable à côté et porte le
SHA-256 `1D524B7716B6CAD0E55C97F7760FA0D8D13E0E188EA2A780A5E8A4C8B55F9C50`.

## Évolution 1.2.3 — 12 août 2026

Un rapport utilisateur a montré que les nombres déjà créés conservaient leur
position en pixels. Quand le joueur et la caméra se déplaçaient, ces pixels
restaient fixes dans l'overlay et le popup semblait donc suivre l'écran au lieu
de rester à l'endroit où le monstre avait été touché ou tué.

La version 1.2.3 a tenté de remplacer cette ancre d'écran persistante par une
ancre monde.
Au commit des HP, le plugin copie seulement les coordonnées X/Y du monstre avec
son identifiant; aucun pointeur `Unit` n'est conservé après le hook. À chaque
image de rendu, la position du joueur local qui porte la caméra est lue une
seule fois, puis toutes les ancres actives sont reprojetées vers l'écran. Un
L'intention était qu'un popup se déplace à l'opposé de la caméra et reste
visuellement attaché au lieu du hit ou de la mort. S'il sort de l'affichage, il
est masqué sans fallback sur le joueur; s'il redevient visible avant son
expiration, sa position est resynchronisée immédiatement.

Le mécanisme reste entièrement indépendant de l'attaquant et du skill. Les
dommages de joueur, mercenaire, summon, Revive, réanimé, piège, missile, attaque
de mêlée ou ranged suivent tous la même cible `UNIT_MONSTER`. Les previews du
menu restent volontairement ancrées à l'écran puisqu'elles ne correspondent à
aucune unité du monde.

La compilation Release x64 et les huit exports attendus passent. Le cold start
visible avec la pile complète charge Floating Damage 1.2.3, applique `16/16`
patchsets, active `19/19` plugins sans désactivation, rejet ni échec et atteint
`24/24`. Une lecture mémoire du processus répondant confirme que l'appel HP à
`0x44D093` vise le relais privé `0x143EB0000` et non le setter vanilla
`0x1402F7D10`. Le test visuel consistant à tuer un monstre puis à déplacer le
personnage pendant la durée du popup reste à confirmer en gameplay et n'est pas
revendiqué comme réussi par le cold start.

Le test gameplay ultérieur de Vincent a invalidé le résultat fonctionnel de
1.2.3 : à 2560×1440, les nombres dérivaient encore avec les mouvements du
personnage au lieu de rester aux coordonnées du hit. Le cold start avait prouvé
le chargement et le hook, pas l'exactitude de la projection.

La DLL source, gouvernée et runtime porte le SHA-256
`965173A639DCCB3DABC47D1920F08BB02C65211FF34EC262E8B1D5CD7D60C707`.
La configuration inchangée porte le SHA-256
`4CF259813621B9184DC3DFDE20357452D9765E1E4C60148FE8D5007751EFC3E3`.
L'archive publique stricte contient uniquement `FloatingDamage.dll` et
`floating-damage.toml`, conserve son README révisable à côté et porte le
SHA-256 `3A03FEE425C38B6B9C1D4F03340CA288F5AE4BE4A5AE513C802E2831060B30F3`.

## Évolution 1.2.4 — 12 août 2026

Le diagnostic runtime a montré que la scène D2R exposait des métriques internes
de `3840×2160` tandis que l'overlay ImGui actif mesurait `2560×1440`. La version
1.2.3 avait bien copié les coordonnées monde du monstre, mais elle appliquait
encore à chaque image ses pas isométriques et ses décalages d'ancrage 4K sans
les convertir dans l'espace pixel de l'overlay. À 1440p, le déplacement du
popup était donc 1,5 fois trop grand et produisait la dérive observée.

La version 1.2.4 conserve l'ancre monde et la reprojection par rapport au joueur
local, mais multiplie maintenant les pas isométriques horizontaux et verticaux,
ainsi que les décalages d'ancrage, par `displayHeight / 2160`. Le facteur est le
même que celui déjà utilisé pour la taille des chiffres : `0.333` à 720p,
`0.5` à 1080p, `0.667` à 1440p et `1.0` à 2160p. Une résolution ultrawide suit
sa hauteur; sa largeur ne fait que déplacer le centre horizontal et ne déforme
pas la géométrie.

Les assertions de compilation couvrent les projections 1080p, 1440p, 4K et
3440×1440. Le build Release x64 et les huit exports attendus passent. Le test
gameplay « frapper ou tuer, puis marcher pendant la durée du popup », ainsi que
le cas des dégâts de mercenaires, summons, Revives, réanimés, pièges, missiles
et autres sources distantes, restent à exécuter et ne sont pas encore
revendiqués comme réussis.

La DLL Release x64 gouvernée porte le SHA-256
`160F3B8C361DEC44B1D5AF3124F19131A42E128973449F480A0BDB416EDD3A7F`.
La configuration inchangée porte le SHA-256
`4CF259813621B9184DC3DFDE20357452D9765E1E4C60148FE8D5007751EFC3E3`.
L'archive publique stricte contient uniquement `FloatingDamage.dll` et
`floating-damage.toml`, conserve son README révisable à côté et porte le
SHA-256 `728EC40C2DEFD58A4E357710269FD813D47823123D265BFE6733D1639302A963`.
La DLL runtime porte le même hash que la DLL gouvernée. Le cold start visible de
la pile complète charge Floating Damage 1.2.4 en portée globale, applique
`17/17` patchsets, active `19/19` plugins sans désactivation, rejet ni échec et
atteint `24/24`. L'overlay DirectX 12 est installé. Une lecture mémoire du
processus répondant confirme que l'appel HP à `0x44D093` vise le relais privé
`0x143EB0000`, et non le setter vanilla `0x1402F7D10`. Le jeu reste ouvert pour
le test gameplay de l'ancrage; aucun succès visuel n'est revendiqué avant cette
validation humaine.

Le test gameplay immédiat de Vincent invalide également l'ancrage fonctionnel
de 1.2.4 : les chiffres bougent encore avec le déplacement au lieu de rester
exactement attachés au lieu du hit ou de la mort. La mise à l'échelle 1440p
corrigeait une incohérence réelle, mais pas la cause fondamentale. Le plugin
utilise toujours la position monde du joueur local comme approximation de la
caméra; D2R applique son propre suivi et son transform de scène. Une nouvelle
version ne doit plus modifier empiriquement les coefficients : elle doit
s'appuyer sur une primitive native gouvernée de coordonnées client/projection,
ou sur un témoin exact du pipeline de rendu. Les cold starts 1.2.3/1.2.4
prouvent seulement le chargement, les hooks et la stabilité de la pile.

## Évolution 1.2.5 — 12 août 2026

La version 1.2.5 supprime entièrement la projection isométrique empirique des
versions 1.2.3/1.2.4. Le hook HP ne capture plus la position monde du monstre ni
la position du joueur local : il place seulement le type `UNIT_MONSTER` et
l'identifiant client de la cible dans la file d'événements.

À chaque passe d'overlay, le plugin résout temporairement cet identifiant avec
`CLIENT_GetUnitByIdAndType 0x9A5D0`, lit le contexte actif à `+0x20` de
`RENDER_GetThreadContextRoot 0x685750`, puis appelle la primitive native
`RENDER_ProjectUnitToScreen 0x76A7D0`. La sortie de deux `float` est exprimée
dans les dimensions UI natives retournées par `UI_GetNativeWidth 0x7F510` et
`UI_GetNativeHeight 0x7F4A0`; elle est convertie vers les dimensions réelles de
l'overlay ImGui. Une unité absente, une projection native refusée ou une
coordonnée hors écran masque le popup sans fallback sur le joueur.

Les six xrefs directs de la projection établissent l'ABI
`(renderContext, Unit*, floatXYOut, bool) -> bool`. Le caller `0x198F75` passe
`R9B=1`, teste `AL`, convertit les deux floats et les borne avec les getters de
dimensions natifs. Les signatures strictes de la projection (61 octets), du
contexte renderer (37 octets) et du lookup client (32 octets) sont chacune
uniques sous le build 92777 et ont été promues dans `known-rvas.json`.

Le build Release x64 passe et expose toujours les huit exports attendus. La DLL
candidate source et gouvernée porte le SHA-256
`3791AF01E86B6A079AC13AE7C48E5C5893D1C5C745D863C595B627C33230C4EC`.
La DLL runtime globale porte le même SHA-256 que la source gouvernée. Le cold
start visible avec la pile complète charge Floating Damage 1.2.5, installe son
overlay DirectX 12, applique `17/17` patchsets, active `19/19` plugins sans
désactivation, rejet ni échec et atteint `24/24`. L'acceptation du plugin après
`InstallDamageHook` prouve également que les signatures HP, lookup client,
contexte renderer, projection et dimensions natives ont toutes été acceptées.
Le processus D2RLoader répond et reste ouvert pour le témoin gameplay. Aucun
succès fonctionnel n'est revendiqué avant le test « frapper ou tuer, puis
marcher » et les cas mercenaire/summon/Revive/réanimé/piège/missile/ranged.

Le test runtime a ensuite invalidé l'architecture 1.2.5. Le hook HP capturait
correctement les dégâts et plaçait le popup dans la file, mais le log n'a jamais
atteint le témoin de projection native. `RENDER_GetThreadContextRoot` dépend du
TLS du thread de rendu : l'appeler depuis la passe DirectX/ImGui du plugin ne
fournit pas le contexte que les callers natifs de D2R utilisent. La DLL était
donc chargée, mais ses popups restaient fonctionnellement muets.

## Évolution 1.2.6 — 12 août 2026

La version 1.2.6 conserve l'identité de la cible capturée par le hook HP, mais
déplace l'observation de sa projection sur le thread qui la possède réellement.
Un hook MinHook étroit sur `RENDER_ProjectUnitToScreen 0x76A7D0` appelle toujours
l'original sans modifier ses arguments ni son résultat, puis copie uniquement
les coordonnées réussies des `UNIT_MONSTER` dans un cache atomique fixe indexé
par `(unitType, unitId)`. Aucun pointeur `Unit` n'est conservé et le hook de
rendu n'alloue pas de mémoire.

La passe overlay ne rappelle plus aucune primitive du renderer. Elle consulte
le cache, exige une entrée âgée d'au plus 250 ms, convertit les dimensions UI
natives mémorisées vers l'affichage ImGui et applique un gate strict au viewport.
Elle préfère la projection native avec hauteur d'unité et replie seulement sur
la projection native de base du même monstre. Elle ne replie jamais sur le
joueur, l'attaquant, le mercenaire ou une invocation. Un événement dont la
projection n'a pas encore été observée est retenu au plus 150 ms pour permettre
au prochain passage natif de le renseigner; un monstre hors écran reste omis.
Les dégâts du joueur, des mercenaires, summons, Revives, réanimés, pièges,
missiles, attaques ranged et autres skills suivent tous cette même clé cible.

L'audit de coexistence compte six xrefs natifs vers la projection. Sa signature
stricte de 61 octets reste unique sous le build 92777; aucune référence à cette
entrée n'existe dans la source épinglée du PluginPack, et le log de la pile
installée ne déclare aucun autre propriétaire de `0x76A7D0`. La création du
hook échoue fermement en cas de collision au lieu de chaîner un propriétaire
inconnu.

Le build Release x64 passe et expose les huit exports attendus. Les DLL source,
gouvernée et runtime portent toutes le SHA-256
`97C7A5024A0F44CF23F6902E8462E53FBB77F58E6F749E05711CB684D4EBC867`.
Le cold start visible avec la pile complète charge Floating Damage 1.2.6,
installe son overlay DirectX 12, applique `17/17` patchsets, active `19/19`
plugins sans désactivation, rejet ni échec et atteint `24/24`. Le processus
D2RLoader répond et reste ouvert. La présence visuelle, l'ancrage pendant un
déplacement et les cas de dégâts déportés restent soumis au témoin gameplay de
Vincent avant toute mise à jour du ZIP public.

Le témoin gameplay de Vincent invalide finalement 1.2.6. Les popups visibles
disparaissent beaucoup plus tôt que les `0,85/0,95 s` configurées, et une
attaque de groupe ne montre les dégâts que sur la cible principale attaquée.
La capture HP reste multi-cibles : chaque commit place son propre identifiant
de monstre dans la file. La régression se trouve dans le fournisseur de
coordonnées. Un événement sans entrée de cache est abandonné après 150 ms et
une entrée déjà utilisée est refusée après 250 ms. Or les six callsites natifs
sont des consommateurs conditionnels d'interface; ils ne constituent pas un
flux exhaustif de toutes les unités endommagées. Le compteur `displayed` de
1.2.6 signifiait en outre « queued » et ne prouvait pas un rendu effectif.

## Évolution 1.2.7 — 12 août 2026

La version 1.2.7 conserve l'unique hook MinHook sur
`RENDER_ProjectUnitToScreen 0x76A7D0`, mais cesse d'utiliser ses arguments
naturels comme liste passive de cibles. Chaque commit HP positif enregistre
immédiatement `(UNIT_MONSTER, unitId)` dans un registre atomique borné de 1 024
entrées. La passe overlay renouvelle la demande tant que le popup existe.

À l'entrée suivante de la projection native, donc avec le `renderContext` du
thread choisi par D2R, le hook appelle d'abord l'original sans modification,
puis traite au plus une fois par 4 ms toutes les demandes actives. Chaque cible
est résolue temporairement avec `CLIENT_GetUnitByIdAndType 0x9A5D0`, projetée
avec hauteur d'unité puis, seulement si nécessaire, avec sa base. Le pointeur
client n'est jamais conservé. Les résultats réussis publient leurs coordonnées
dans le cache atomique; un lookup ou une projection refusée invalide la cible
au lieu de réutiliser des pixels périmés. Les slots de demande sont dédupliqués
et expirent 300 ms après le dernier renouvellement.

Cette architecture ne dépend ni de la cible sélectionnée ni de l'attaquant.
Tous les monstres d'une attaque de zone, d'un projectile, d'un piège, d'un
mercenaire, d'un summon, d'un Revive ou d'un réanimé reçoivent une demande
indépendante. La durée configurée reste portée par le nombre actif; la fraîcheur
du cache sert seulement à refuser une coordonnée réellement non renouvelée.
Aucun fallback sur le joueur ou l'attaquant n'est réintroduit.

Le build Release x64 passe et conserve les huit exports attendus. Les DLL de
build, gouvernée et runtime globale portent le même SHA-256
`C85F9AED20F50D1359BB4D7D3F00438169BC43A9FF130451FEC8A9F6D19B0FE4`.
La configuration globale reste inchangée au SHA-256
`4CF259813621B9184DC3DFDE20357452D9765E1E4C60148FE8D5007751EFC3E3`.
Le cold start visible avec la pile complète charge Floating Damage 1.2.7,
installe l'overlay, applique `17/17` patchsets, active `19/19` plugins sans
désactivation, rejet ni échec et atteint `24/24`. Le processus répond.

Les témoins gameplay « plusieurs monstres simultanés », durée normale/critique,
déplacement de la caméra, mort de la cible, hors-écran et dégâts de minions sont
`not run` : le contrôleur Windows a échoué deux fois avec `EPERM` sur ses propres
fichiers Codex. Le ZIP public reste volontairement inchangé jusqu'à observation
en jeu de ces cas.

Le témoin gameplay de Vincent invalide ensuite 1.2.7. Lorsqu'il déplace le
personnage et la caméra, les chiffres disparaissent encore prématurément et une
seule cible conserve son popup au lieu du groupe. Le registre HP reste
multi-cibles, mais son traitement demeure déclenché depuis le hook de
`RENDER_ProjectUnitToScreen`. Cette entrée n'est appelée que par six
consommateurs UI spécialisés; le registre ne possède donc toujours pas de
rendez-vous garanti à chaque image. La cible principale continue d'être
projetée par l'UI native, alors que les autres demandes cessent d'être
rafraîchies ou sont invalidées au premier refus du contexte graphique.

## Évolution 1.2.8 — 13 août 2026

La version 1.2.8 retire entièrement le hook MinHook de
`RENDER_ProjectUnitToScreen 0x76A7D0`. La fonction reste vérifiée par sa
signature stricte et appelée directement, mais elle n'est plus utilisée comme
signal de cadence.

Le nouveau rendez-vous est `CLIENT_UpdateCameraOffsets 0xB9B90`. Cette fonction
résout le joueur local, lit ses coordonnées client, soustrait la demi-largeur et
la demi-hauteur natives puis applique le déplacement caméra optionnel. Son
unique appel direct, à `0x93D79`, appartient à la passe client principale. Sa
signature stricte de 33 octets est unique sous 92777. Le PluginPack épinglé au
commit `dc75b49ffbb67b887d7757ee00ee9a03bcde5d8a` ne contient aucune référence à
ce RVA, et les logs installés ne déclarent aucun propriétaire concurrent.

Le hook appelle d'abord la mise à jour caméra originale sans modifier son
contrat. Il récupère ensuite la racine TLS vérifiée de
`RENDER_GetThreadContextRoot 0x685750`, lit le contexte actif à `+0x20`, puis
traite toutes les demandes de monstres encore actives. Chaque cible est résolue
temporairement par identifiant, projetée avec les offsets caméra de l'image
courante et publiée dans le cache atomique. Aucun pointeur `Unit`, aucun pixel
périmé et aucun fallback sur le joueur ne sont conservés.

Le build Release x64 passe et conserve les huit exports attendus. Les DLL de
build, gouvernée et runtime globale portent le SHA-256
`7EDF436590C352C3066D4AFC7BDD7F5C39D528B437129269AA17947FE354F441`.
Le cold start visible avec la pile complète charge Floating Damage 1.2.8,
installe l'overlay, applique `17/17` patchsets, active `19/19` plugins sans
désactivation, rejet ni échec et atteint `24/24`. Aucun nouveau rapport de
crash n'est créé et le processus répond.

Le rendez-vous caméra ne s'exécute pas au frontend. Les témoins gameplay
« groupe puis déplacement », durée complète, mort, hors-écran et minions sont
donc encore `not run`; le contrôleur Windows échoue toujours avec `EPERM` après
sa récupération prescrite. Le ZIP public reste inchangé jusqu'à validation.

Le premier témoin gameplay 1.2.8 invalide aussi son overlay : Vincent ne voit
plus `DPS` en bas à gauche, même si la DLL charge, que la capture HP fonctionne
et que le log annonce les hooks DirectX. Cette annonce ne prouvait alors que
la création des hooks, sans témoin d'un appel `Present`, d'une queue DirectX,
de l'initialisation ImGui ou d'une frame soumise.

## Évolution 1.2.9 — 13 août 2026

La version 1.2.9 instrumente sans spam les quatre jalons du renderer : premier
`Present`, capture de la queue DirectX 12, initialisation ImGui et première
frame soumise. Le cold start initial reproduit l'échec : aucun de ces jalons ne
s'exécute alors que l'ancien message « hooks installed » est présent.

L'audit du binaire runtime `plugin-items.dll` montre qu'il embarque
`ExtendedItemStats 0.3.17`, Dear ImGui, `D3D12CreateDevice` et son renderer de
repli. Il ne contient aucun pont `FloatingDamageRegisterExternalOverlay` dans
ce binaire actif. Les deux plugins créaient donc leur probe D3D12 presque
simultanément et le dernier detour écrit devenait le seul propriétaire visible;
le résultat dépendait du timing du cold start.

Floating Damage détecte maintenant les deux exports ExtendedItemStats de
`plugin-items.dll`, attend que son renderer de repli s'installe, puis pose son
propre hook en dernier afin de former une chaîne déterministe. Aucun plugin du
pack n'est modifié, désactivé ou retiré. Le cold start complet applique
`17/17` patchsets, active `19/19` plugins avec `disabled=0`, `rejected=0` et
`failed=0`, puis journalise successivement la queue DirectX, `Present`, ImGui
et la première frame GPU.

Vincent confirme ensuite en jeu que `DPS` est de nouveau visible en bas à
gauche, que les popups restent attachés aux monstres touchés et que plusieurs
monstres d'un groupe conservent simultanément leurs chiffres. Les gates
overlay/DPS, ancrage du test courant et groupe multi-cibles sont `passed`.

Les DLL Release de build, gouvernée et runtime globale sont byte-identiques au
SHA-256
`76FCCFED9AE7E7B5AC01D613AD49DD9D1BBF43CE587D0E553C341F7379F89C95`.
La configuration globale reste inchangée au SHA-256
`4CF259813621B9184DC3DFDE20357452D9765E1E4C60148FE8D5007751EFC3E3`.
Les gates mort, hors-écran et dégâts de minions restent ouverts; le ZIP public
demeure inchangé.
