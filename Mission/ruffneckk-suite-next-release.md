# RuffnecKk D2RLoader Suite — prochaine release publique

Dernière mise à jour : 4 septembre 2026

## Statut final — hotfix 1.3.2 publié

Le 4 septembre 2026, Vincent autorise un hotfix rapide contre le D2RLoader
`1.2.1` public réellement distribué. Le candidat conserve le catalogue complet
de `v1.3.1` et met à jour uniquement MapSense `1.0.1`, Floating Damage `1.4.3`,
ISC12 `1.0.2` et Vendor Stock Refresh `2.0.2`.

Le conflit rapporté avec Bind And Summon `1.4.4-embedded110` est fermé par
architecture : MapSense ne pose plus et n'appelle plus le hook
`UNITS_GetClassId` à `0x349860`. Bind And Summon reste l'unique propriétaire;
MapSense lit le champ `D2UnitStrc+0x04` seulement après validation d'un témoin
de layout indépendant à `0x34B7B2`. La gouvernance épingle le DLL fourni à
168 448 octets, SHA-256
`8416C7908753ED3D35DB43BECE0CFAE9EF768F520C41ECEDFD570C8A4E6B70B0`, et
conserve ses onze hooks attestés.

Le vrai `D2RCore.dll` public vaut
`2130A98D0B879696116A7DDDE5C11AE8C91942B54B8276DB43E02074A715BBC8`.
ISC12 et Vendor ajoutent uniquement ses profils exacts complets; les profils
preview 10, 1.2 et 1.1 restent admissibles par leurs propres empreintes. Le
premier cold start public a correctement refusé ISC12 avant mutation parce que
la paire finale `WriteD2sFileWithEnvironment` / `CloseD2sFileWithEnvironment`
n'était pas encore incluse dans le sélecteur de génération. La correction
ajoute cette paire exacte, sans assouplir les empreintes ni sélectionner par
numéro de version.

Deux builds Release propres produisent les quatre DLL byte-identiques, les
quatorze tests ciblés passent dans chaque build et l'ownership natif est
`VALID`. Le second cold start sur
D2R `3.3.93847` et le D2RLoader `1.2.1-beta` public atteint `24/24`, avec 37
plugins chargés, 17 memory patches et seulement les deux échecs préexistants
Stash Search et Revive Overhaul. ISC12 `1.0.2` valide la paire publique, publie
152 empreintes / 43 sites / 129 mutations et atteint `SchemaReady=true`;
MapSense `1.0.1` installe ses hooks DirectX 12 sans collision avec Bind And
Summon, Floating Damage `1.4.3` rend sa première frame par l'hôte MapSense, et
Vendor Stock Refresh `2.0.2` valide son relais paquet. Aucun processus Diablo
ne reste actif. Le warning déjà connu d'atlas sprite MapSense laisse seulement
le generated underlay fail-closed; le gameplay visuel élargi n'a pas été rejoué.

Les DLL finales valent respectivement `ECFC729C…35D921` pour MapSense,
`1F6BAE0A…7FE2B` pour Floating Damage, `12F452FC…94E1C` pour Vendor et
`FE350F17…08F45` pour ISC12. Les quatre archives finales valent
`B27BAF8A…408C9F`, `0313D135…9FE41`, `A22FC56C…AD199E` et
`DB91B436…1966CAA`; la dernière est l'archive ISC12 revue par Vincent avec les
README ISC12 et D2R Save Converter insérés.

La Suite `v1.3.2` est publique depuis le 4 septembre 2026. Le tag et `main`
pointent sur le commit produit `3ee22db64b3f4d4738ad391c7a7241029a383f18`.
La release contient exactement 24 plugins, 17 patches et deux bundles, soit 43
assets; leurs 43 digests GitHub correspondent byte-exact au catalogue local.
Les deux constructions du catalogue complet sont identiques. Le bundle plugins
vaut `6282AF9A…B2E5F0C` et le bundle patches `288090AE…4547C8`. Le registre
privé est `published`, `releaseReady=true`, et son allowlist verrouille 68
entrées. `v1.3.1` demeure intacte comme rollback.

## Réorganisation post-publication

Les deux dépôts de produit ne sont plus hébergés dans le cache jetable du
workspace. Le dépôt public autoritaire est
`C:\Workspaces\RuffnecKk-D2RLoader-Suite`; le dépôt privé de gouvernance est
`C:\Workspaces\RuffnecKk-D2RLoader-Suite-Governance`. Le fichier
`workspace-repositories.json` centralise ces relations et permet des overrides
par environnement. `npm run checkpoint` inventorie désormais les trois dépôts
et attribue leurs changements au workstream actif.

Le skill `d2rloader-suite-promotion` est implicitement applicable dès qu'un
composant incubé devient qualifié, entre dans la prochaine release ou change
de source autoritaire. Il promeut une copie filtrée dans le dépôt produit,
préserve la provenance dans le registre privé et refuse un état
`package-ready` ou `published` qui dépend encore d'une source `workspace:`.
Le registre `releases/1.3.2/next-release.json` référence maintenant les onze
composants concernés sous `suite:`; `promotion-ledger.json` conserve leurs
origines et empreintes d'arbre. D2R Save Converter est autonome sous
`tools/d2r-save-converter` et Shadow Master AI Fix sous `patches/`.

La synchronisation vers un jeu installé est séparée de la promotion de source.
`scripts/runtime/Sync-SuiteRelease.ps1` fournit Plan, Apply, Verify et Rollback
à partir de l'allowlist exacte; toute mutation runtime exige une confirmation
explicite, préserve les configurations par défaut et produit un reçu local.
Aucune synchronisation runtime n'a été exécutée pendant cette réorganisation.

Le contrôle statique complet des 24 plugins a mis au jour un écart réel dans
Resistance Floor `1.0.1` : l'asset déjà publié refuse encore les builds autres
que `92777` et `93847` par numéro. Le binaire publié de SHA-256
`871928BBC03B99164A545E0C2CB4287CBEDE424E24A1B1918052DB1612AABE31`
contient ces marqueurs. La source canonique publique a été corrigée pour ne
traiter le build-name qu'en diagnostic et pour décider exclusivement d'après
l'empreinte native fail-closed; son build Release et son test de politique
passent. Le tag et les 43 assets immuables de `v1.3.2` ne sont pas modifiés :
une future republication de ce correctif constitue un gate séparé.

## Statut final — hotfix 1.3.1 publié

La Suite `v1.3.1` est publique depuis le 3 septembre 2026. Elle pointe sur le
commit produit `c63ac0c`, contient exactement 24 plugins, 17 patches et deux
bundles (43 assets), et conserve `v1.3.0` intacte comme rollback. Les 43
digests GitHub correspondent byte-exact aux deux générations locales
reproductibles. Le registre privé final est au commit `47df4a4`.

Le cold start final sous D2R `3.3.93847` et D2RLoader `1.2.1-beta preview 10`
a atteint `24/24`; ISC12 `1.0.1`, Vendor Stock Refresh `2.0.1` et Remote Stash
`2.3.1` sont actifs. L'absence du fichier optionnel de skin MPQ de Remote Stash
retombe correctement sur le TOML D2RLoader. Les sections ci-dessous conservent
les décisions et preuves de préparation historiques.

## Décision et priorité

Vincent choisit le 2 septembre 2026 l'option **registre et outillage d'abord** :
le périmètre de la prochaine release doit être enregistré avant les migrations,
builds, qualifications et archives finales. Le 3 septembre, il élargit ce
périmètre à MapSense et Extended Act Level IDs, tous deux en `1.0.0`, et fixe
également ISC12 et D2R Save Converter à `1.0.0`.

La dernière release GitHub publique vérifiée est `v1.2.0`. Le prochain numéro
reste donc `1.3.0`; l'ancien candidat local non publié ne constitue pas une
release publique.

Le registre autoritaire est désormais
`releases/1.3.0/next-release.json` dans le dépôt privé versionné
`RuffDood/RuffnecKk-D2RLoader-Suite-Governance`. Il gouverne les décisions,
versions cibles, retraits, reports et gates. L'allowlist privée voisine conserve
un rôle distinct : chemins et SHA-256 exacts des artefacts finalisés. Le dépôt
produit public ne conserve que les données techniques reproductibles sous
`tests/data/compatibility/`; le packaging doit recevoir les trois chemins
privés explicitement et refuser toute absence ou divergence.

## Périmètre verrouillé

Le catalogue visé contient **24 plugins, 17 memory patches et deux bundles**,
soit **43 assets GitHub**. D2R Save Converter demeure un outil hors-jeu
versionné `1.0.0`, mais il est inclus uniquement dans le ZIP individuel ISC12
et ne possède plus son propre asset GitHub.

Décisions explicites de Vincent :

- `Cast Triggers` entre en release comme `1.0.0`, et non `0.1.0`;
- le produit fonctionnel Armageddon devient
  `Armageddon-Hurricane CtC Fix 1.0.0`;
- `Resistance Floor 1.0.0` entre dans cette release;
- `MapSense 1.0.0` entre dans cette release; son candidat `0.13.41` doit être
  reversionné, rebâti et requalifié sous cette identité publique;
- `Remote Stash 2.3.0` entre avec ses corrections d'incohérences et de
  comportement ainsi qu'avec la personnalisation du bouton depuis le MPQ du
  mod actif;
- `Vendor Stock Refresh 2.0.0` entre avec sa compatibilité D2RLoader 1.2 et
  1.2.1;
- `Shadow Master AI Fix` entre comme memory patch;
- `ISC12 1.0.0` entre comme plugin et son ZIP individuel contient
  `D2R Save Converter 1.0.0` ainsi que les README d'ISC12 et du Converter;
- `Extended Act Level IDs 1.0.0` entre comme plugin;
- les deux presets `Ground Item Label Limit` 64 et 128 sont retirés parce que
  D2RLoader 1.2 fournit désormais la fonction native équivalente;
- `Normal Area Scaling` reste retiré parce que Yinyin possède un patch
  fonctionnel et que celui de RuffnecKk ne fonctionnait apparemment pas;
- la feature update MassID PluginSDK v4 est reportée; Vincent l'a nommée
  `1.2.0`, tandis que le candidat du dépôt est `2.1.0`, donc ce numéro devra
  être réconcilié avant une future promotion.

Le composant MassID de base demeure dans la Suite et doit recevoir seulement
la migration de compatibilité nécessaire. Son prochain numéro de maintenance
reste volontairement non verrouillé dans le registre afin d'empêcher un
packaging prématuré.

Les seuls artefacts publics Ground Item Label Limit des releases précédentes
étaient les deux memory patches JSON 64 et 128. Aucune instruction de migration
ne doit demander le retrait d'une `GroundItemLabelLimit.dll` inexistante dans
ces releases.

## Compatibilité Steam

D2R `3.3.93787` Steam n'est pas testé pour cette release. Vincent décide le
3 septembre 2026 que ce test ne bloque ni le packaging ni la publication. Aucun
plugin ne peut autoriser ou refuser son chargement d'après son canal, son
build-name ou son numéro de version. Chaque DLL conserve sa validation native
fail-closed avant mutation afin d'éviter un hook sur des octets incompatibles.

Les notes publiques indiquent simplement que Steam n'a pas été testé. Aucune
qualification Steam ou multijoueur n'est revendiquée.

## Outillage gouverné

Le dépôt privé contient le schéma, le registre et l'allowlist propres à la
release. Le dépôt produit public contient :

- `scripts/Test-NextRelease.ps1`, qui exige des chemins externes explicites,
  dérive les comptes, vérifie les décisions, versions, gates,
  inclusions/retraits/reports et peut produire les notes;
- un gate dans `scripts/New-Release.ps1` qui exige le plan, le schéma et
  l'allowlist privés, puis refuse le packaging avant `package-ready` ou en cas
  de divergence;
- des tests CMake publics autonomes fondés sur les catalogues techniques sous
  `tests/data/compatibility/`, plus les tests de release activés seulement
  lorsque les trois chemins privés sont configurés.

Les configurations de plugin deviennent optionnelles par contrat : zéro ou une
configuration justifiée par composant. Un simple booléen `enabled` ne doit pas
forcer la création d'un fichier.

## Gates ouverts

### Compatibilité ISC12 avec D2RLoader 1.2.1

Vincent autorise le 3 septembre 2026 l'adaptation du candidat public
`ISC12 1.0.0` à `D2RLoader 1.2.1-beta preview 10`. La version publique reste
`1.0.0` puisqu'elle n'a pas encore été publiée. L'implantation doit ajouter une
nouvelle empreinte native complète des fournisseurs `D2RCore` utilisés par
ISC12, conserver les générations déjà admises et sélectionner une génération
uniquement par ses octets et témoins ABI, jamais par le numéro ou le canal du
runtime. Le convertisseur autonome reste `1.0.0` et ne dépend pas de
`D2RCore.dll`.

La qualification runtime sur D2R officiel `3.3.93847` a révélé que le premier
rebuild du cache compilateur de D2RLoader 1.2.1 produit quatre captures
ItemStatCost avant la publication RotW, contre deux avec un cache valide. ISC12
conserve maintenant jusqu'à huit captures bornées. Les cinq tests ISC12
passent. Un cold start avec le cache `3.3.0` absent a publié le schéma de 400
lignes avec `G0-builds=4` et atteint le démarrage `24/24`; le cold start suivant
a validé le cache, publié le même schéma avec `G0-builds=2` et atteint de nouveau
`24/24`. Le candidat déployé mesure 334 848 octets et porte le SHA-256
`59C46E13158C1E03D54BC378AF27E78333FD692F6D648B9AA200EE25DC3B7767`.

### Compatibilité Vendor Stock Refresh avec D2RLoader 1.2.1

Vincent autorise le 3 septembre 2026 l'adaptation de
`Vendor Stock Refresh 2.0.0` à `D2RLoader 1.2.1-beta preview 10`. La version
publique reste `2.0.0`. Le plugin conserve le profil D2RCore 1.2 déjà admis et
ajoute une empreinte complète du nouveau provider
`SendClientGameplayPacket` : corps, PDATA/unwind, export, slot de forwarding et
destination native. La sélection repose uniquement sur les octets et témoins
ABI observés; aucun numéro de version, canal ou build-name ne décide du
chargement.

L'audit statique confirme byte-exact les deux profils du provider
`SendClientGameplayPacket` : D2RCore 1.2 à `0x630D90`, taille `0x19A`, et
D2RCore 1.2.1 preview 10 à `0x693E70`, taille `0x170`. Pour chacun, l'export,
le corps SHA-256, PDATA/unwind, FuncInfo et le forwarding witness correspondent
au binaire source. Le build Release final de Vendor Stock Refresh 2.0.0, incluant
la section réservée `[d2rl]`, mesure 48 640 octets et porte le SHA-256
`A6E67D11B2B675F2B31F38D1A2F98C45915956838C1852F73603D638D3CB306D`;
les 36 tests de la Suite passent après rebuild complet. Le cold start pile
complète sous D2RLoader 1.2.1 preview 10 vérifie le relais paquet de ce binaire,
le charge et atteint le démarrage `24/24`; le démarrage natif 1.2.1 est fermé.
Le clic du bouton chez un marchand reste une vérification fonctionnelle
manuelle.

1. Importer ou synchroniser dans le dépôt produit ISC12 1.0.0, le convertisseur
   1.0.0, Cast Triggers 1.0.0, Armageddon-Hurricane CtC Fix 1.0.0, Resistance
   Floor 1.0.0, MapSense 1.0.0, Extended Act Level IDs 1.0.0, Shadow Master AI
   Fix, Bulk Currency Deposit 1.1.1, Burn Damage Fix 1.0.0 et Remote Stash 2.3.0
   sans perdre les changements locaux existants.
2. Verrouiller le numéro de maintenance MassID compatible avec cette release et
   confirmer que la feature PluginSDK v4 reportée n'y entre pas.
3. Fermer les gates source/build/package des nouveaux composants.
4. Promouvoir une allowlist de 43 assets exactement concordante, passer deux
   générations reproductibles, comparer les catalogues et inspecter chaque ZIP
   ou JSON.
5. Actualiser le README central et les notes publiques avec une formulation
   courte orientée joueur, puis obtenir l'autorisation séparée avant commit,
   push, tag ou publication GitHub.

## Préparation vérifiée le 3 septembre 2026

Les versions publiques `1.0.0` de Cast Triggers, Armageddon-Hurricane CtC Fix,
Resistance Floor, MapSense, Extended Act Level IDs et ISC12 ont été rebâties en
Release. Tous leurs tests CTest passent et leurs DLL exportent seulement les
trois points d'entrée D2RLoader attendus. Le convertisseur hors-jeu `1.0.0`
passe ses 64 tests et sa génération EXE.

SHA-256 des candidats rebâtis :

- Cast Triggers : `FCC8993377349C8FC4B7CE8C7C48C78092F39E19E7A800FA611B1AEF2E7ED343`;
- Armageddon-Hurricane CtC Fix : `FBF449062EF06D1693B9E268C6EA2637757BFC472CCA648C7BEF27AD57C3A56D`;
- Resistance Floor : `D4014A5A0185FC83256EF51F13AD1CA0B9E98E278A1D1E2F1369FDA468A1C844`;
- MapSense : `A5A441EF434A8901153388A1158D2E3BA4E4BD7C17C6F07FFB0234F0FEECC753`;
- Extended Act Level IDs : `979DC79C6CF791B58728178B48B44B9F8A141528E7BE51F038DDC8CF2F24F8F0`;
- ISC12 : `59C46E13158C1E03D54BC378AF27E78333FD692F6D648B9AA200EE25DC3B7767`;
- D2R Save Converter : `5BC08306B1A165641383D991AC430CF135126E5FB034667B34DE2A00FB95FD48`.

Vincent choisit le 3 septembre 2026 de supprimer l'asset EXE séparé du
Converter. Le ZIP individuel ISC12 devient l'unique téléchargement public de
l'outil et doit contenir :

```text
plugins/d2rl-ruffneckk-isc12.dll
README.md
D2R Save Converter/D2RSaveConverter.exe
D2R Save Converter/README.md
```

Le candidat local `addons/ISC12/RuffnecKk-isc12-v1.0.0.zip` a été généré le
3 septembre 2026 avec exactement ces quatre entrées. Sa taille est de
`95034618` octets et son SHA-256 est
`EFCAB702CDD58F24CF008028D3E4C817F9AA55B7CEECAFDD06498A2AE36827BA`; chaque
entrée extraite correspondait alors byte-exact à sa source gouvernée. Cette
archive est maintenant obsolète depuis le correctif de compatibilité 1.2.1
d'ISC12 et doit être régénérée avant publication.

Le Converter et son README sont exclus du bundle `All Plugins`. Son README
public a été ramené à un guide joueur sans référence à BKVince; il conserve les
avertissements de sauvegarde, les formats 9-bit/12-bit, la sélection des données
de mod, les limites de conversion et les consignes de chargement. Son SHA-256
est `A18BB77D7244D69BF504278B4159D938FB79D4163B3C9B806C15A9C8B9EF83A8` dans
le dépôt produit; la copie de travail du projet Converter a le même contenu
normalisé en LF.

Le package source de Cast Triggers contient désormais sa DLL `1.0.0` exacte.
Son README public fournit également le parcours moddeur complet : réservation
des IDs, rows ItemStatCost/Properties, tooltips JSON, liaison TOML, exemples,
validation et dépannage. Vincent a relu et modifié ce README, SHA-256
`D48016BF499B95FFB7038039FFEF4B885975C5DC99A9386526156FA417B69252`, puis a
demandé le 3 septembre 2026 qu'il soit inclus comme `README.md` dans le ZIP
individuel Cast Triggers 1.0.0. Le registre et les validateurs rendent cette
exception obligatoire; le README reste exclu du bundle `All Plugins`.

Le package MapSense utilise désormais le nom canonique
`d2rl-ruffneckk-mapsense.dll`; l'ancien `RuffnecKkMapSense.dll` a été sorti du
package et conservé localement comme rollback dans `analysis-cache/`. Son
README public et le `THIRD_PARTY_NOTICES.md` central créditent maintenant
libd2, Joffreybesos/PrimeMH, Dear ImGui, MinHook, toml++, D2MOO et D2RMH. Les
textes de licence libd2 et toml++ sont conservés byte-exact avec leur SHA-256;
la limite explicite de la licence libd2 sur les données dérivées de Diablo reste
un gate de provenance distinct.

`RuffnecKkMapSenseMapgen.exe` demeure un composant runtime obligatoire lancé
automatiquement par MapSense. L'ancien
`RuffnecKkMapSenseAutomapSprites.msp` a été déplacé hors de `package/` vers
`test-fixtures/`; il reste disponible pour CTest, mais ne fait pas partie des
fichiers publics.

L'archive `D2RMM Custom 1.9.6` inspectée le 3 septembre 2026 porte le SHA-256
`E7BFFFB81B69AB4C278A01ABE8AD6AA5A177531FDC64A2D1110D03760BE8E311`.
Son importeur accepte les ZIPs D2RLoader et route les entrées explicites
`plugins/`, `config/` et `patches/`. L'audit de la Suite confirme que les deux
configurations plugin JSON, Bulk Skill Point Allocation et Larzuk Sockets,
sont déjà sous `config/`; les TOML et memory patches utilisent également leurs
racines canoniques. Aucun autre plugin ne possède un compagnon runtime externe
obligatoire.

Le vieux sous-arbre de travail `addons/RemoteStash/package/d2rloader/` contient
encore un `RemoteStash.json` et des snippets de merge, mais ils ne sont pas
distribués par l'archive publique actuelle : celle-ci contient uniquement la DLL
sous `plugins/` et le TOML sous `config/`. Ils ne constituent donc pas un second
cas de pathing D2RMM pour la prochaine release.

MapSense constitue l'unique exception : son générateur SHA-256
`7E176D2317EEAD60D1DBD543CC375B0B8BEEFA452DC0246642F4F8B7247E2120` doit être
archivé comme `plugins/RuffnecKkMapSenseMapgen.exe` à côté de la DLL. Le
registre gouverne désormais ce contrat et les validateurs refusent les DLL,
compagnons EXE, configurations ou bundles qui ne respectent pas les chemins
importables par D2RMM Custom. Cette preuve est statique; l'import UI puis le
lancement en jeu restent à rejouer sur l'archive finale avant publication.

La qualification Steam n'est plus un gate de release. Le packaging final de la
Suite reste ouvert tant que MassID, l'allowlist exacte et les archives
reproductibles ne sont pas fermés.

## Séparation publique/privée vérifiée le 3 septembre 2026

Le dépôt privé de gouvernance est établi sur `main` au commit `01eb929` avec les
trois fichiers copiés byte-exact avant leur retrait public. Le dépôt produit
public est synchronisé sur `main` au commit `661ed43` : le dossier racine
`manifests/` a disparu, les catalogues techniques ont été déplacés sans
modification sous `tests/data/compatibility/`, et les scripts refusent les
entrées de release privées absentes. Les quatre tests CTest publics autonomes
passent. Le registre privé reste volontairement `scope-locked` et l'allowlist
actuelle est rejetée comme obsolète jusqu'à sa future promotion.

## Métadonnées de vérification multijoueur — 3 septembre 2026

Les configurations publiées de Bulk Currency Deposit, Burn Damage Fix,
Charm Aura Trigger Fix, Cube Quick Move, Enhanced Damage Min/Max Fix,
Item Durability, MassID, Potion Auto Pickup, Remote Stash, Repair Costs Cap,
Resistance Floor et Vendor Stock Refresh déclarent désormais la section
réservée D2RLoader suivante :

```toml
[d2rl]
match = true
```

D2RLoader peut ainsi inclure leurs réglages dans sa vérification multijoueur.
Cette métadonnée reste optionnelle pour le chargement : une ancienne
configuration sans `[d2rl]` demeure acceptée par le plugin, mais ne fournit pas
au Loader cette attestation. Les parseurs ignorent la section réservée au
Loader tout en continuant de refuser les sections et réglages inconnus dans
leur propre espace de configuration.

La validation Release ciblée rebâtit les 12 DLL et leurs 12 exécutables de
test. Les 12 tests de politique passent; chacun accepte la configuration
publiée avec la section réservée, et les cas historiques sans `[d2rl]`
continuent également de passer. L'audit statique confirme exactement une
section `[d2rl]` et un `match = true` dans chacun des 12 TOML. Les contrôles
globaux `ruffneckk-suite-source-policy` et de propriété des écritures natives
passent aussi. Aucun test runtime en jeu n'est requis pour ce changement de
métadonnée et de tolérance de parseur.

La même validation a corrigé l'option de compilation de Bulk Currency Deposit :
ses cibles DLL et test activent maintenant `/EHsc`, requis par leur parseur
toml++ et déjà employé par les autres plugins toml++ de la Suite.

## Rollback

Les releases et tags `v1.0.0`, `v1.1.0` et `v1.2.0` restent intacts. Tant que
le registre vaut `releaseReady=false`, aucun artefact 1.3.0 ne peut être généré
par le chemin canonique. Le rollback consiste à rétablir le commit public
précédent sans supprimer le dépôt privé ni modifier les anciennes releases.
