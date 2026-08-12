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
