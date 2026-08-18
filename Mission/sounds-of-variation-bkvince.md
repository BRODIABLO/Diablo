# Sounds of Variation for BKVince

## Décision gouvernée

Vincent a retenu l'option A le 2 août 2026 : intégrer sélectivement **The Sounds of Variation 0.1g** dans BKVince 3.2, sans remplacer les tables courantes par les tables anciennes du mod source et sans versionner ses assets audio.

Cette mission est un workstream séparé. Elle ne remplace pas la mission active BaseMod.

## Sources locales vérifiées

| Archive Nexus locale | SHA-256 attendu |
|---|---|
| `The Sounds of Variation (Sounds)-287-v0-1g-1673569945.rar` | `3787E81A38BBC1959A2816EAA3E83B7B79F69E1931DD41095358631338E7699E` |
| `The Sounds of Variation - Sounds (FIX for no drops bug)-287-v0-1g-fix-1675802605.rar` | `537E238BE4289FA5DBBD36E7D32947A1743652C605DCFFEC5296C2E9C18E0E28` |

Les deux archives ont passé le test CRC de 7-Zip. L'archive principale contient 1 228 fichiers FLAC valides et aucun exécutable, DLL ou script. Le correctif contient uniquement `monstats.txt` et `sounds.txt`.

## Politique d'intégration

- `sounds.txt` : reconstruire les 38 groupes touchés dans l'ordre de l'auteur, ajouter 1 222 clés absentes et ne modifier sur les lignes BKVince existantes que `Group Size` et les `FileName` réellement fournis par l'archive.
- `monsounds.txt` : partir de la table vanilla 3.2 courante, appliquer le delta ciblé de `necromage` et ajouter `sk_mage_fire`, `sk_mage_cold`, `sk_mage_ltng` et `sk_mage_pois`.
- `monstats.txt` : modifier exclusivement `MonSound` et `UMonSound` sur les 27 IDs de mages squelettes qui référencent ces quatre profils.
- préserver les headers 3.2, les lignes propres à BKVince, le CRLF et le round-trip byte-exact.
- valider les deux SHA-256 avant toute extraction ou copie runtime.
- ne jamais copier les anciennes tables intégrales du mod source dans BKVince.

## Distribution et provenance

La page Nexus de l'auteur WyRuZzaH interdit la republication des assets sans autorisation. Le dépôt RuffnecKk étant public, les 1 228 FLAC restent hors de Git et sont installés depuis les archives Nexus détenues localement par Vincent. Une redistribution publique de ces sons demeure bloquée jusqu'à obtention d'une permission explicite de l'auteur. Les crédits tiers restent distincts des créations RuffnecKk.

## Gates

- [x] Fusion TSV idempotente, références résolues et round-trip byte-exact.
- [x] Archives, nombre de FLAC, signatures `fLaC` et hashes source/runtime vérifiés.
- [x] Trois tables synchronisées vers le profil BKVince actif avec hashes identiques.
- [x] Cold start D2RLoader 3.2 sans assertion, erreur de table ni erreur de chargement.
- [ ] Validation auditive en jeu des 38 familles, puis solo/hôte/joiner et sauvegarde/rechargement.
- [ ] Autorisation écrite de WyRuZzaH avant toute archive ou release publique contenant les FLAC.

## État au 2 août 2026

Intégration source/runtime terminée. Les 1 228 FLAC ont été installés localement sans écraser d'asset existant; les trois tables ont été synchronisées avec les SHA-256 `ABDCA1CB…7DEF8` (`monsounds.txt`), `2DEF9754…FD67` (`monstats.txt`) et `6A8CDEB7…0B8C` (`sounds.txt`). Le cold start a atteint le rendu de l'Acte I sans assertion, crash ni erreur de table ou de son. La validation auditive et la matrice multijoueur restent ouvertes.

Le 10 août 2026, le rollback mercenaire demandé par Vincent restaure les
45 lignes Desert Mercenary du jalon pré-TDE `e67e66d2`. Le validateur
Mercenary Command exige de nouveau ces 45 profils et ne dépend plus de la
refonte Acte II ni du chantier Act I Rogue retiré.
La comparaison des assets JSON normalise leurs fins de ligne afin qu'un checkout
frais ne produise pas de faux positif. L'override HD `commandbar.json`, dont
l'identifiant d'entité BKVince avait été normalisé, est explicitement préservé;
toute autre divergence de contenu des assets Mercenary Command demeure refusée.
Les améliorations de comparaison JSON et de préservation de l'override HD
restent indépendantes de ce rollback.

## Correctif permanent du miroir HD — 4 août 2026

Après l'installation cumulative des mods de sons, Vincent a observé des sons
d'objets, de monstres et d'interface coupés ou silencieux. Le diagnostic a
montré que ce n'était pas une saturation du nombre de voix : 372 FLAC étaient
présents sous `data/global/sfx`, tandis que le moteur HD tentait notamment de
résoudre `data/hd/global/SFX/cursor/button.flac`.

Le correctif runtime a copié uniquement les 371 cibles HD absentes, conservé
une cible déjà byte-identique et refusé tout écrasement divergent. Les 372
paires ont ensuite été validées par SHA-256, avec zéro fichier manquant et zéro
divergence. Le cold start D2RLoader a franchi 24/24 étapes sans nouvelle erreur
`D2Sound`, puis Vincent a confirmé en jeu que le correctif fonctionnait.

Pour rendre ce comportement durable, 370 des 371 FLAC initialement manquants
sont intégrés à la source gouvernée sous
`data-BKVince/BKVince.mpq/data/hd/global/sfx/` et restent gérés par Git LFS. Ce
lot duplique seulement le placement HD des sons déjà sélectionnés par le pack
runtime; il n'écrase aucun asset HD existant et ne modifie aucune table TSV ni
le budget de voix audio.

Vincent a demandé le 4 août 2026 de conserver l'ancien son de pluie. L'override
`ambient/scene/rain2.flac` du miroir, SHA-256
`AA7B9C77B3DC7028BFF0744235B5C9AFAA3E75E65DC0F5D3AA564D4EC97A7328`, a donc
été retiré uniquement des chemins HD source et runtime. Le fichier global du
pack reste intact et le fallback retrouve l'asset vanilla 3.2, SHA-256
`C07E9A35A3608B026510A0D1E25B76DC8C2172A46D3D4641DC74DCB17EB0EE27`.
Le premier cold start de contrôle s'est interrompu à 15/24 sur une exception
graphique dans `dxgi.dll`, appelée depuis `plugin-items.dll`, avant le chargement
audio. Le second cold start a franchi 24/24 étapes avec 18/18 patches appliqués,
12 plugins actifs, zéro rejet/échec et aucune erreur `D2Sound` ou `rain2`.
L'audition du retour au son de pluie précédent reste à confirmer par Vincent.

## Exceptions audio vanilla personnalisées — 4 août 2026

Vincent demande de conserver les sons vanilla pour les interactions d'interface,
les boutons du menu principal, le cube, les waypoints et les petits pas de course
des monstres de type Fallen. La correction reste ciblée dans `sounds.txt` et ne
retire aucun son d'objet ou d'inventaire précédemment rétabli.

- Les cinq routes `cursor_pass`, `cursor_select`, `cursor_error`,
  `cursor_button_click` et `cursor_switch` retrouvent leurs redirects HD vanilla.
- Les deux événements de cube `cursor_convert_ready` et `cursor_convert_item`
  retrouvent leurs priorités vanilla; leurs redirects et fichiers étaient déjà
  ceux de D2R 3.2.
- L'ouverture et les trois boucles de waypoint retrouvent leurs valeurs vanilla
  de `LFEMix` et de priorité.
- `light_run_dirt_1` retrouve le redirect `light_run_dirt_hd1`, ce qui restaure
  les petits pas de course vanilla utilisés notamment par les Fallen sans
  désactiver leurs variations vocales de combat.

Le changement porte sur 15 cellules réparties sur 13 lignes. Le round-trip reste
byte-exact en CRLF; `sounds.txt` passe de
`FCD6E738855602EE51E8A07C85F4A16DDAE3A1824F88DF83DC04E3FE2D287D8E` à
`37DEF08860C3F304E110932CDA7201CA880ADE91230CD087CBB5DB8C05023ED9`.
Le déploiement runtime est volontairement limité à ces 15 cellules afin de ne
pas embarquer le prototype Magic/Guided Arrow encore source-only. Le candidat et
le runtime partagent le SHA-256
`8A8EB4F7A8210EC95EAB4431AEBF169A22045AD30C8858DC42C6D7C99700D114`; la
sauvegarde et le rapport récupérables sont sous
`analysis-cache/runtime-sync/audio-preferences-20260804-133247/`.

Le cold start exact `D2RLoader.exe -mod BKVince -txt` franchit 24/24 étapes,
charge la table audio, applique 18/18 patches et active 12 plugins sur 13, avec
zéro rejet, zéro échec et aucune erreur `D2Sound`, son ou audio. Les assertions
capturées après le frontend concernent les tables d'items et de zones, pas cette
correction. L'audition en jeu reste à confirmer par Vincent sur le menu principal,
le cube, les waypoints, les pas des Fallen et la non-régression des sons d'items.

### Voyage au waypoint, amulets et rings vanilla — 4 août 2026

Vincent précise que les nouveaux sons de voyage au waypoint, d'amulet et de ring
ne lui conviennent pas. L'audit confirme que ces routes avaient été intégrées
comme remplacements : leurs redirects vanilla avaient été supprimés, au lieu
de conserver l'original dans un groupe de variantes.

- `item_amulet` et `item_ring` retrouvent leurs redirects HD et leurs priorités
  vanilla; `item_amulet_hd` et `item_ring_hd` retrouvent aussi leur priorité
  vanilla.
- La route d'entrée/transition `player_townportal_enter` retrouve le redirect,
  le `LFEMix` et la priorité vanilla; `player_townportal_enter_hd` retrouve son
  `LFEMix` vanilla.
- Les autres familles d'items et les autres variantes audio restent inchangées.

Ce second affinage modifie 10 cellules sur 6 lignes. Le `sounds.txt` gouverné
reste byte-exact en CRLF et passe au SHA-256
`EAB0E6F9C47283AC9FCF495896E2E44AA60B22418A808026E4C58A1C66588C2C`.
Le déploiement runtime ciblé conserve les prototypes source-only et porte le
SHA-256 `2B4C4F262626427167E44EF3F9EB6BEB36BD7F0B1B98BDBD8332E6C360D3987B`;
la sauvegarde et le rapport sont sous
`analysis-cache/runtime-sync/audio-vanilla-exclusions-20260804-134627/`.
Conformément à la demande de Vincent, aucun redémarrage n'est effectué. Le cold
start et l'audition du voyage au waypoint, des amulets et des rings restent
`not run` jusqu'à son autorisation explicite.

### Horadric Cube : son d'item vanilla ciblé — 4 août 2026

Le retour utilisateur montre que les routes `cursor_convert_*` corrigées plus
haut couvrent la transmutation, mais pas le son propre au Horadric Cube. La ligne
`box` de `misc.txt` utilisait encore `dropsound=item_rare`; cette route reste
volontairement personnalisée pour les autres rare items et charge le fichier
mod-local `item/rare.flac`.

Le Cube reçoit donc directement `dropsound=item_rare_hd`. Cette exception
réutilise le son HD vanilla depuis les archives du jeu sans retirer les variantes
des autres rare items. Une seule cellule change dans `misc.txt`, dont le SHA-256
passe de `CFC3E004B43F587578BC7EB4E8B4BB8B43FE5FD88AA7B03E5439D198BC1A112F`
à `63CA8FD1B608804AF6BCDA92C632C005B3BDF0FA903E2D738967441CAEE0C5EF`.

Le candidat runtime byte-exact est prêt sous
`analysis-cache/runtime-sync/audio-cube-vanilla-20260804-141054/` avec le même
SHA-256 que la source. Le jeu étant actif pendant les tests de Vincent, ni le
déploiement runtime ni le redémarrage ne sont effectués. Ces deux gates ainsi que
l'audition restent `not run` jusqu'à son autorisation explicite.

### Identification d'objet vanilla — 4 août 2026

Vincent refuse également les nouveaux sons joués lors de l'identification. Les
routes `cursor_identify_ready` et `cursor_identify_item` chargeaient directement
les fichiers mod-locaux `identifyready.flac` et `identify.flac` parce que leurs
redirects avaient été supprimés.

Les deux routes retrouvent leurs redirects et priorités exacts de D2R 3.2 : le
curseur d'identification et l'application sur l'objet utilisent de nouveau leurs
groupes HD vanilla, dont les dix lignes membres étaient déjà intactes. Quatre
cellules changent sur deux lignes; le SHA-256 gouverné de `sounds.txt` devient
`4527BA4A91A99810727CBCCDABD7B230BF891094E8C4388CFFC6EE359C504563`.

Le paquet runtime combiné Cube + identification est prêt sous
`analysis-cache/runtime-sync/audio-pending-cube-identify-20260804-141425/`.
Le jeu reste actif et aucun déploiement ni redémarrage n'est effectué sans
l'autorisation explicite de Vincent; le cold start et l'audition restent `not run`.

### Town Portals vanilla — 4 août 2026

Vincent demande ensuite le retour vanilla de toute la famille des Town Portals.
La route d'entrée `player_townportal_enter` et sa variante HD avaient déjà été
restaurées. Les routes restantes retrouvent maintenant leurs valeurs D2R 3.2 :

- `player_townportal_cast` retrouve son redirect, son `LFEMix` et sa priorité;
- `object_townportal` et `object_townportal_hd` retrouvent leur mix et leur
  priorité;
- `player_townportal_loop_hd` retrouve son mix et sa priorité;
- les trois membres `player_townportal_cast_hd1..3` retrouvent leur `LFEMix`.

Cette correction porte sur 12 cellules de 7 lignes et n'affecte ni les portails
de quête/Baal ni la compétence Teleport de la Sorceress. Le SHA-256 gouverné de
`sounds.txt` devient
`B107AEC50592A215CB769516007C2061547DB34416451E560C8D3E1FE540DE5E`.

Le nouveau paquet runtime combiné Cube + identification + Town Portals est prêt
sous
`analysis-cache/runtime-sync/audio-pending-cube-identify-townportal-20260804-141637/`.
Le jeu reste actif : déploiement, redémarrage, cold start et audition demeurent
`not run` jusqu'à l'autorisation explicite de Vincent.

### Déploiement combiné autorisé — 4 août 2026, 14:21

Après le `go` explicite de Vincent, les candidats combinés Cube + identification
+ Town Portals sont copiés dans le runtime BKVince, puis une seule instance est
relancée avec `D2RLoader.exe -mod BKVince -txt`.

- `sounds.txt` runtime correspond exactement au candidat, SHA-256
  `78B5752B363615E165A7DE651736B6C62D12E90D8EEA144A68AE9F14F79C1B1E`.
- `misc.txt` runtime correspond exactement au candidat, SHA-256
  `63CA8FD1B608804AF6BCDA92C632C005B3BDF0FA903E2D738967441CAEE0C5EF`.
- Le build `3.2.92777` franchit 24/24 étapes, applique 18/18 patches, charge la
  table audio et ne produit aucune erreur `D2Sound`, son ou audio.
- `plugin-items.dll` refuse indépendamment son chargement après un mismatch
  `check-expected` au RVA `0x2BD480`; le bilan est donc 11 plugins actifs,
  1 désactivé, 0 rejeté et 1 en échec. Cet incident précède le chargement audio
  et ne modifie pas les deux tables déployées.

Le jeu reste ouvert au frontend. L'audition du Cube, de l'identification et des
Town Portals est `not run` jusqu'au retour de Vincent.

Retour en jeu de Vincent : le son vanilla d'identification et le son original du
Horadric Cube sont confirmés `passed`. L'audition des Town Portals n'est pas
explicitement confirmée et reste `not run`.

## Priorité générale d'Enhanced Effects — 5 août 2026

Vincent décide que tout effet audio réellement fourni par **Enhanced Effects
and Sounds** doit conserver la priorité sur les packs audio intégrés ensuite.
L'audit historique distingue les superpositions compatibles des remplacements :

- **The Sounds of Variation** conserve ses groupes étendus et ses variantes,
  puisqu'ils n'écrasent aucun asset Enhanced aux chemins concernés;
- les lignes `custom_d2pack_*` restent disponibles dans la table, mais ne
  remplacent plus une route dont le groupe HD contient des assets Enhanced;
- les exceptions vanilla explicitement choisies par Vincent pour l'interface,
  le cube, l'identification, les amulets, les rings, les waypoints, les Town
  Portals et les pas des Fallen demeurent inchangées.

Quatorze redirects sont restaurés vers leurs groupes HD Enhanced : swings 1H,
bows, crossbows, punch, claw, blunt, blade thrust, blade swing, arrow, bash,
ainsi que Cold Arrow, Fire Arrow, Magic Arrow et Multiple Shot. Les 53
classifications `hit class` d'Enhanced dans `weapons.txt` étaient déjà intactes,
et les assets Enhanced vérifiés restent byte-identiques entre la source et le
runtime actuellement installé.

Le changement source porte uniquement sur 14 cellules `Redirect` de
`sounds.txt`, dont le SHA-256 passe de
`B107AEC50592A215CB769516007C2061547DB34416451E560C8D3E1FE540DE5E` à
`533FBEA427B1CD98F0FA139A026A2B39F7602A4465347DA539B063824FB2AAE1`.
Le déploiement runtime, le redémarrage et l'audition restent `not run` jusqu'à
l'autorisation explicite de Vincent.

Vincent étend ensuite cette priorité aux deux derniers conflits directs entre
Sounds of Variation et D2Pack. `weapon_bow_draw_1` retrouve le redirect
`weapon_bow_draw_hd_1`, rendant ses 11 variantes accessibles, et
`necromancer_corpseexp_1` retrouve `necromancer_corpseexp_hd7`, rendant ses 13
variantes accessibles. Ces deux cellules supplémentaires font passer le
SHA-256 de `sounds.txt` à
`D1CEDC15CE63CBC3BF2529918874AF0E69E984A4887F85686E15470BF005C382`.
Le runtime et l'audition restent `not run`.

La synchronisation runtime est autorisée ensuite sans relance. Un candidat
ciblé est construit depuis le `sounds.txt` runtime afin de conserver les trois
lignes Magic/Guided Arrow encore source-only, puis les 16 redirects décidés y
sont appliqués. Le runtime correspond byte-exactement au candidat, SHA-256
`73DA51D3B5D747112C6652AEB83481E7E45979CA80D8712EE7006A07D96F02DA`;
la version précédente `78B5752B363615E165A7DE651736B6C62D12E90D8EEA144A68AE9F14F79C1B1E`
et le candidat sont conservés sous
`analysis-cache/runtime-sync/audio-enhanced-variation-priority-20260805-100324/`.
Aucune instance D2R ou D2RLoader n'était active avant ou après la copie. Aucun
redémarrage n'est effectué; cold start et audition restent `not run`.

## Prototype séparé — Magic Arrow et Guided Arrow

Ce petit lot du 4 août 2026 est distinct de **The Sounds of Variation**. Les
trois FLAC proviennent du post Inven
[`메아리 타격 효과음_매직에로우`](https://www.inven.co.kr/board/diablo2/5842/7410)
de **Koo3869**, qui demande explicitement de conserver la source lors du
partage. L'archive locale `c2517874081.7z` porte le SHA-256
`0E6D9419E3D76EF996E7C2113D4E2724367AFCEE14C29DFB1ADDA70DE92F2607`.

- Les trois fichiers 48 kHz/24 bits sont renommés sous `skill/amazon` et
  versionnés par Git LFS avec leurs hashes source exacts.
- `Magic Arrow` et `Guided Arrow` utilisent le groupe isolé
  `custom_bkvince_magic_guided_arrow_cast_1`; leurs sons de projectile natifs
  restent actifs.
- Aucun fichier ni identifiant Warlock n'est modifié et `Silent.flac` est exclu.
- Le snapshot exact de l'index passe `verify:data`, le cadastre et les contrôles
  ciblés TSV/références. L'audition en jeu reste volontairement ouverte.

## Normalisation perceptuelle des ambiances et des objets — 8 août 2026

Vincent signale que plusieurs ambiances du D2Pack dominent la musique et que
certaines manipulations d'objet sont anormalement fortes. L'audit confirme que
ce problème est distinct des 38 groupes de Sounds of Variation : l'intégration
du D2Pack avait changé les routes audio sans recalibrer `Volume Min` et
`Volume Max` pour le niveau propre des nouveaux FLAC.

Les trois manifestes locaux du D2Pack recensent 371 assets. Les 370 assets
encore installés sont décodés avec Xiph FLAC 1.5.0, dont l'archive officielle
porte le SHA-256 déjà gouverné
`53F1500F0D6E7C61379D7FEE50D4A9F7F504C650009506D9BA015530D76C0DDE`.
`rain2.flac`, supprimé intentionnellement lors du retour à la pluie vanilla,
reste exclu. L'analyse PCM mesure le RMS et le peak sans modifier les fichiers :

- les 18 ambiances custom vont d'environ `-40.36` à `-14.18 dBFS RMS`;
- les manipulations d'objet actives vont d'environ `-28.13` à
  `-11.66 dBFS RMS`;
- `cathedral`, `sewer`, `mesa`, `metalshield`, `quiver`, `sword` et plusieurs
  armures sont des outliers mesurés, pas une simple impression de volume;
- les footsteps, le combat, les skills, les sons restaurés vanilla et les
  routes prioritaires Enhanced Effects ne sont pas retouchés.

La correction applique uniquement une atténuation dans `sounds.txt`, sans
réencoder de FLAC et sans augmenter les sons déjà faibles. Le plafond retenu
est d'environ `-24 dBFS RMS effectif` pour les ambiances continues et
`-20 dBFS RMS effectif` pour les manipulations d'objet. Une correction
inférieure à 1 dB est ignorée afin d'éviter un churn sans gain audible.

Le changement porte exactement sur 32 cellules `Volume Min`/`Volume Max` de
16 lignes : 9 ambiances et 7 manipulations d'objet. Les headers, les 13 242
lignes, les redirects et les FLAC restent inchangés. Le round-trip TSV demeure
byte-exact en CRLF. Le SHA-256 gouverné de `sounds.txt` passe de
`D1CEDC15CE63CBC3BF2529918874AF0E69E984A4887F85686E15470BF005C382` à
`07141226C0D0C841AA3EBB17CC811E6B3FBFC94C89422D4804A96288BC2ACE8D`.

La synchronisation ciblée du 8 août 2026 remplace uniquement le `sounds.txt` du
profil actif `mods/BKVince`. La copie runtime et la source gouvernée sont
byte-identiques, SHA-256
`07141226C0D0C841AA3EBB17CC811E6B3FBFC94C89422D4804A96288BC2ACE8D`.
La version runtime précédente, SHA-256
`73DA51D3B5D747112C6652AEB83481E7E45979CA80D8712EE7006A07D96F02DA`,
est conservée sous
`analysis-cache/runtime-sync-backups/20260808-130310708/`. Une instance
`D2RLoader` a été fermée avant la copie et n'a pas été relancée. Le cold start
et l'audition comparative restent `not run`; ils constituent le gate suivant
avant de déclarer l'équilibre subjectif validé en jeu.

### Remplacement par l'étalonnage HD natif — 8 août 2026

Le retour runtime de Vincent invalide la première passe pour les ambiances :
malgré le bon hash runtime et un lancement ultérieur avec
`D2RLoader.exe -mod BKVince -txt`, aucune baisse audible n'est observée. Cette
première méthode n'avait modifié que 9 boucles et avait conservé notamment les
ambiances Desert, Harem, Jungle, Lava, Town 3 et Wilderness à leurs anciens
volumes. Son plafond RMS n'est donc plus la référence retenue pour les scènes.

L'audit du routage `soundenviron.txt` montre 126 références HD Day/Night vers
19 boucles custom actives. Pour chacune, `Volume Min` et `Volume Max` héritent
désormais exactement des anciennes cibles HD que le D2Pack avait déconnectées.
Les FLAC custom et les redirects vides sont conservés. `scene_rain` reste exclue
parce que son asset custom avait déjà été retiré pour restaurer la pluie
vanilla. Les 7 corrections d'objets de la première passe restent inchangées.

Le changement porte sur 38 cellules de 19 lignes. Les 13 242 lignes, les
headers et le CRLF restent byte-exacts; `verify:data` et le validateur ciblé
des 126 références passent. Le SHA-256 de `sounds.txt` devient
`528F1D402733BF5EF575D32E2E04A333ED1380176311A4FEF31118449FBF25BE`.

L'autosync ciblé remplace uniquement le `sounds.txt` du profil actif BKVince.
La source et le runtime sont byte-identiques au nouveau SHA-256. La version
runtime précédente `07141226C0D0C841AA3EBB17CC811E6B3FBFC94C89422D4804A96288BC2ACE8D`
est conservée sous
`analysis-cache/runtime-sync-backups/20260808-134736263-hd-ambient/`.
`D2RLoader` était terminé côté noyau mais son PID restait publié par CIM;
la copie de repli n'a été effectuée qu'après confirmation de zéro processus
vivant. Aucun redémarrage n'est effectué. Le cold start et l'audition de cette
nouvelle correction restent `not run`.

## Menu principal Classic — 18 août 2026

Le thème Lord of Destruction du premier groupe du jukebox est remplacé par le
thème Diablo II Classic déjà livré par D2R. Le changement modifie uniquement
les deux cellules `FileName` de `jukebox_music_group1` et
`jukebox_music_group_hd1` dans `sounds.txt` : `introedit.flac` devient
`common\\options.flac` et `introedit_hd.flac` devient
`common\\options_hd.flac`. Les redirects, groupes, volumes et fades restent
inchangés; aucun FLAC Blizzard n'est copié ni ajouté au dépôt.

La table conserve ses 13 242 lignes, ses headers, son EOL final et un
round-trip byte-exact intégralement CRLF. Le SHA-256 gouverné et runtime de
`sounds.txt` est
`094E5CBA9CCF2B101F3A3CD82E3A629963717860AD0998D500A330479AB48BFC`.
La copie a remplacé uniquement le `sounds.txt` du profil actif
`mods/BKVince`, après fermeture complète de D2RLoader et vérification de zéro
processus D2R vivant.

Le cold start frais du profil BKVince build 92777 passe avec la pile complète :
27 plugins chargés, 17 memory patches appliqués, table audio chargée, 188
tables compilées depuis TXT et 24/24 étapes terminées. Le menu principal est
atteint et confirmé visuellement. `verify:data` reste bloqué par l'écart
préexistant et hors périmètre de
`hd/global/ui/spells/submenu/skillicon.lowend.sprite` dans le contrôle
Mercenary Command; les validations antérieures et le contrôle TSV ciblé
passent. L'audition humaine du thème Classic dans les modes Resurrected et
Legacy reste `not run` et constitue le seul gate fonctionnel de ce lot.
