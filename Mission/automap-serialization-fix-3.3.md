# Automap Serialization Fix — D2R 3.3

Dernière mise à jour : 5 septembre 2026

## Reprise prioritaire — compatibilité avec la base MapSense 1.0.2

Vincent autorise le 5 septembre 2026 par `go` la réintégration ciblée du
témoin à deux états exacts dans la source produit MapSense, ainsi que son
suivi. Cette compatibilité précède la reprise des tests de dimensions des
grandes zones extérieures. Le candidat sera identifié MapSense `1.0.3` pour
ne pas confondre son binaire avec la version `1.0.2` publiée.

L'audit source retrouve l'adaptation dans
`addons/RuffnecKkMapSense/src/reveal_engine.{hpp,cpp}` et ses tests, tandis que
`suite:plugins/mapsense/src/reveal_engine.cpp` n'accepte que l'épilogue vanilla.
Automap Serialization Fix `0.1.0` conserve exclusivement les 13 octets à
`0xD7E3F`; MapSense reste consommateur read-only. Le report porte seulement sur
ce contrat, ses tests et les métadonnées du candidat, sans reprendre les
divergences GPS/mapgen ou les autres travaux du laboratoire.

Baseline : D2RLoader `1.2.1` public, PluginSDK API v3 minimal existant, corpus
natif commun vérifié pour Battle.net `3.3.93847` et `3.2.92777`. La release
Suite `1.3.3` publiée demeure inchangée. La qualification MapSense `1.0.0`
du 3 septembre ne vaut pas qualification de ce candidat.

Gates de reprise : tests exacts vanilla/corrigé et refus des altérations,
builds stricts reproductibles, identité DLL/package, puis pile complète dans
les deux ordres et portées, et aller-retour de layer avec payload >32 767
octets. Runtime du nouveau candidat : `not run`. Rollback : restaurer les
artefacts précédents depuis leur sauvegarde vérifiée; aucune migration de save.

### Preuves hors jeu du candidat 1.0.3

- Source autoritaire : `C:/Workspaces/RuffnecKk-D2RLoader-Suite/plugins/mapsense/`,
  HEAD d'audit `95ba7133a80ff7f5b818c1fc41d750a2cc9438ab`; report ciblé de
  l'adaptation du laboratoire, sans copie de son arbre divergent.
- Deux builds Release x64 MSVC `/W4 /WX` dans
  `analysis-cache/mapsense-1.0.3-automap-{a,b}` sont byte-identiques :
  `3 578 368` octets, SHA-256
  `D2F71263A3871EC3B4A356064603B601AF1A3E098C72A03CC47CDEC30BB4B965`.
- PE et PluginInfo `1.0.3`, auteur `RuffnecKk`, trois exports D2RLoader et
  export de coopération renderer préexistant conservés.
- CTest `2/2` dans chaque build : politiques et vraies polices Windows.
  Le contrat accepte les deux séquences complètes, refuse les `6 630`
  substitutions d'un octet et traite exactement les `8 192` combinaisons
  vanilla/corrigé, en refusant tout mélange distinct des deux états admis.
- Automap Serialization Fix reste byte-identique à
  `C7193FADB024236136E241B79164EFF4CF86C0ED5C0E7CA77454D1BD7CD8CE17`;
  son CTest existant passe `1/1`.
- `Test-Suite.ps1 -RequireAll`, `Test-NextRelease.ps1` avec le registre,
  schéma et allowlist `1.3.3` explicites, et `Test-SuiteGovernance.ps1` :
  `VALID`. Ces contrôles préservent la release publiée; le candidat ne devient
  ni publié ni package-ready par leur succès.
- ZIP de test local :
  `addons/RuffnecKkMapSense/package/RuffnecKk-mapsense-v1.0.3-automap-test-r1.zip`,
  SHA-256 `DD2C8CD0F51E0587FB72F5A50E7ADC01F4D132AA25E2A37688EB96A7D89B431D`.
  Trois fichiers seulement, hashes vérifiés après extraction : DLL candidate,
  helper publié `74CC1DACA28E836C53E10FDB43EE7B37883E73F0894A06E14AA2A8287B138A43`,
  TOML publié `C286358724195E17288B5949A83F69373C477D96A20273FE0C92B8912961D241`.
  README produit copié à côté du ZIP, exclu de l'archive.
- Installation observée : `C:/Games/Diablo II Resurrected`, profil BKVince,
  MapSense `1.0.2` publié `FA8A2EE0...9EF724`, helper publié exact,
  aucun Automap Serialization Fix installé, aucun processus D2R actif.
  D2R et `.build.info` correspondent aux hashes gouvernés ci-dessous;
  D2RLoader public `1.2.1` vaut `27A79CCD...5C084` et D2RCore `2130A98D...5BBC8`.

Prochaine séquence proposée : quatre cold starts BKVince hors ligne,
deux ordres pour chaque portée globale/mod-locale, pile complète active;
le dernier couvre aussi le payload de `6 000` cellules tag-zéro, `36 000`
octets et le retour `layer 0 → 1 → 0`. Sauvegarder puis restaurer byte-exact
les DLL, configurations et saves affectées, ainsi que les noms temporaires
servant à contrôler l'ordre. Le harness diagnostique est temporaire.
Cette séquence runtime attend sa confirmation opérationnelle explicite.

## Décision autonome RuffnecKk Suite

Vincent autorise par `GO` le 3 septembre 2026 l'implantation d'une nouvelle
DLL D2RLoader autonome, `Automap Serialization Fix`, attribuée exactement à
`RuffnecKk` et membre indépendante de la RuffnecKk D2RLoader Suite. La DLL
reste hybride : le même binaire doit fonctionner depuis le dossier global
`<D2R>/d2rloader/plugins/` ou depuis le dossier mod-local
`<D2R>/mods/<mod>/d2rloader/plugins/`, sans `ModScopedOnly` et sans dépendance
à MapSense, BKVince ou une DLL d'eezstreet.

Le candidat initial porte la version autonome `0.1.0`. Aucun réglage moddeur
réel n'est démontré : la DLL est active par sa présence et n'ajoute donc aucun
fichier, parseur, chemin ou artefact de configuration.

Cette mission est distincte de la prochaine release publique Suite `1.3.0`.
Le plugin n'entre pas dans son registre verrouillé tant que ses gates source,
build, runtime, coexistence et packaging ne sont pas fermés et qu'une décision
de release séparée ne l'a pas retenu.

## Objectif joueur

Empêcher le crash de D2R lorsque le changement de layer automap sérialise plus
de 32 767 octets de cellules explorées. Le correctif ne change ni la taille des
niveaux, ni la génération DRLG, ni les coordonnées, ni le format des records,
ni les sauvegardes de personnage.

Description publique retenue :

> Prevents large explored maps from crashing during area transitions.

## Faits vérifiés

- Le workbench gouverné commun à D2R `3.2.92777` et Battle.net
  `3.3.93847` passe `npm run re:d2r33 -- status` le 3 septembre 2026 : images
  canonique/analyse et index sont vérifiés. Steam `3.3.93787` reste seulement
  admissible; aucune compatibilité n'est revendiquée sans preuve byte-exact de
  chaque surface employée.
- `AUTOMAP_SerializeCellTree` est à `D2R+0xD7CE0`. Il ignore une cellule lorsque
  le premier octet de sa clé à `node+0x20` est non nul, puis émet exactement
  trois `uint16` — frame, X et Y — pour chaque record tag-zéro.
- L'épilogue unique à `D2R+0xD7E3F` vaut
  `0F B7 4E 08 66 03 C9 0F BF C9 41 89 0F`. Il lit seulement 16 bits du nombre
  de mots, double en 16 bits, sign-étend le résultat et écrit la longueur dans
  le `uint32` fourni par l'appelant.
- Le crash MapSense observé comptait `7 556` records émis :
  `7556 * 3 * 2 = 45336 = 0xB118`. La sign-extension a produit
  `0xFFFFB118`, ensuite transmis à la copie optimisée où le crash est apparu à
  `D2R+0x12D399E`.
- Les quatre callsites gouvernés sont `0xD62CB`, `0xD6345`, `0xD63B5` et
  `0xD6425`. Ils sérialisent les quatre arbres de l'owner et fournissent chacun
  une destination de longueur 32 bits.
- La signature stricte des 13 octets originaux n'a qu'une occurrence dans
  `.text`.
- L'audit du PluginPack épinglé ne trouve aucun propriétaire de `0xD7E3F` ni
  de `AUTOMAP_SerializeCellTree`. Dans le workspace, MapSense ne modifie pas la
  plage mais l'utilise comme témoin d'empreinte.

## Architecture retenue

Le plugin possède exclusivement la plage `D2R+0xD7E3F..0xD7E4B`. Après avoir
validé toutes les surfaces natives dont dépend l'interprétation, il remplace
les 13 octets par une séquence statique qui :

1. lit le compteur 32 bits de mots à `container+0x08`;
2. le double en 32 bits;
3. met la longueur à zéro si cette multiplication dépasse le champ `uint32`;
4. écrit la longueur 32 bits dans la destination de l'appelant.

Le format reste trois `uint16` par record et le seuil erroné de 5 461 records
disparaît. Un payload impossible à représenter dans le champ 32 bits échoue
fermement avec une longueur nulle au lieu de provoquer une copie hors limites.

La décision de chargement ne consulte jamais le build-name, le canal, la
version D2R ni le hash global du PE. Ces valeurs ne peuvent apparaître que dans
les diagnostics. Toute empreinte absente, partielle ou ambiguë refuse le plugin
avant la première écriture.

## Coexistence MapSense

MapSense 1.0.0 utilise désormais le tag natif restauré `1` pour ses cellules
synthétiques; elles restent visibles mais ne sont pas émises par le sidecar.
Il ne dépend donc plus de la limite 5 461 pour son propre atlas. Sa validation
native vérifie toutefois encore les 13 octets vanilla.

La coexistence retenue modifie uniquement ce témoin : MapSense accepte soit les
13 octets vanilla complets, soit les 13 octets corrigés complets. Aucun wildcard
n'est admis. Ainsi :

- MapSense chargé d'abord valide vanilla, puis le plugin devient propriétaire
  de la correction;
- le correctif chargé d'abord applique la séquence exacte, puis MapSense la
  reconnaît comme état légitime;
- l'absence du correctif laisse MapSense autonome et inchangé;
- une troisième séquence, un patch partiel ou une collision refuse le
  chargement avant mutation.

## Surfaces et fichiers prévus

- `addons/AutomapSerializationFix/` : source, tests, contrat natif, README et
  futur package autonome;
- `addons/RuffnecKkMapSense/src/reveal_engine.cpp` : acceptation fail-closed
  des deux états exacts du témoin, sans nouvelle dépendance runtime;
- `Mission/WORKSTREAMS.json` : ownership du nouveau chantier et déclaration de
  la plage native partagée avec MapSense comme consommateur;
- `ROADMAP.html` : tâche I7 distincte, non incluse automatiquement dans la
  release Suite 1.3.0;
- `ai-cartographie.json` : nouvelle zone structurale régénérée et annotée.

`known-rvas.json` et `findings.md` portent déjà les identifications stables
nécessaires. Ils ne seront modifiés que si l'implantation produit une preuve
native nouvelle.

## Plan de validation

1. Tests purs des seuils `5 461`, `5 462`, `7 556` et de la borne `uint32`.
2. Tests exacts des octets originaux/corrigés, du manifeste, des métadonnées,
   de l'absence de configuration et de toute allowlist de version/canal.
3. Build Release x64 strict `/W4 /WX`, trois exports D2RLoader, PE/PluginInfo
   `0.1.0`, puis deux builds reproductibles et SHA-256.
4. CTest du plugin et régression CTest de MapSense.
5. Cold starts Battle.net officiel courant avec pile complète, cinq DLL
   eezstreet et toutes les fonctionnalités actives : portée mod-locale, portée
   globale et deux ordres de chargement MapSense/correctif.
6. Reproduction gameplay sur un layer dépassant 32 767 octets sérialisés,
   changement aller/retour et vérification de la persistance automap sans crash.
7. Steam `3.3.93787` reste `not run` et non revendiqué tant que ses surfaces ne
   sont pas prouvées byte-exact ou qualifiées séparément. La compatibilité
   multijoueur reste un gate indépendant.
8. Le ZIP public éventuel contiendra seulement la DLL. Le README restera à
   côté du ZIP pour relecture humaine et créditera D2MOO comme référence
   sémantique, sans transposer d'adresse ou d'ABI 32 bits.

## Rollback

Retirer la DLL restaure le comportement vanilla au prochain démarrage. Pour un
rollback de développement, restaurer séparément le témoin MapSense à son état
mono-empreinte et supprimer uniquement le dossier autonome, sa mission et son
entrée de workstream/ROADMAP. Aucun fichier de sauvegarde ou de données de mod
n'est migré par ce chantier.

## Preuves runtime du 3 septembre 2026

La baseline officielle testée est Battle.net D2R `3.3.93847`, Build Key
`623f7a1f73eabb08ccb2b2046e3f9164`. Le SHA-256 de `.build.info` vaut
`2EBCAD0521DBF038D5A7FE5395E96B4BEF6D4F0774F7B1F840E03C3DE9CB067A`,
celui de `D2R.exe`
`E1F5436E3D9687F644EF16938B1B183D1FDEF434F18CF66D852CF68F48CC8936`
et celui de D2RLoader `1.2.0-beta`
`651FA9EB33083088349224B1624819F63ED79596F808950CF6468B5D82F7132E`.

Le premier démarrage pré-final a prouvé le chargement du correctif, mais a
révélé que le témoin alternatif MapSense journalisait à tort l'échec de l'état
vanilla avant d'accepter l'état corrigé. Cette preuve est conservée comme
superseded. Le contrôle MapSense compare maintenant silencieusement les deux
séquences complètes et n'émet une erreur que si aucune ne correspond. Deux
builds propres du candidat MapSense final sont byte-identiques; son build, son
package et l'artefact testé mesurent `3 553 280` octets et portent le SHA-256
`2B71748E53084FDE72E36731293251C9776072F690AA7224F4E579AE7CC624A1`.
CTest MapSense passe `1/1`.

Trois cold starts du candidat final ferment la matrice de chargement :

1. correctif mod-local chargé avant MapSense mod-local : `PASS`;
2. MapSense mod-local chargé avant le correctif mod-local : `PASS`;
3. MapSense mod-local chargé avant le correctif global : `PASS`.

Chaque démarrage atteint `D2R startup complete` avec `39` plugins, `17`
memory patches, les cinq DLL eezstreet et zéro erreur fraîche du loader, du
correctif ou du témoin de sérialisation MapSense. Le même correctif de `25 600`
octets et SHA-256
`C7193FADB024236136E241B79164EFF4CF86C0ED5C0E7CA77454D1BD7CD8CE17`
est utilisé dans les trois cas. Les journaux sont conservés localement sous
`analysis-cache/runtime-validation/automap-serialization-fix-20260903/`.

Après la matrice, le profil est restauré à son état initial : MapSense
`0.13.41`, SHA-256
`25B18515A47B121C4F5905E8D16A8FA8560370519028A6BA31C11E35A8D9E24A`,
`14` DLL mod-locales et `24` globales, aucun correctif installé et aucun
processus D2R/D2RLoader actif.

## Qualification D2RLoader 1.2.1 preview 10

Vincent confirme par `GO` le 3 septembre 2026 la matrice bornée de
compatibilité avec le ZIP fourni `D2RLoader-1.2.1-beta-preview.10.zip`, de
SHA-256
`D61230B250A0A3D94DD80EB2511822642F1ACA0353E981FCCB6214FA6243FEB3`.
Le runtime contenait déjà les trois binaires exacts du ZIP :

- `D2RLoader.exe` `1.2.1-beta+preview.10`, SHA-256
  `F566D30ED5D41C7079D21E82BA9A2129EC8B0DA5472B50996D0D185D2D7BB4AF`;
- `D2RCore.dll` `1.2.1-beta+preview.10`, SHA-256
  `667241D494F6A73E940E9EE89544872482B6083A08060BB539CFCDE5FADE7125`;
- `d2rloader.mpq`, SHA-256
  `2130F4FF3FED8E78C92D6E547BAB4F445A02B24E3F0A074679E0EDCE6E9E6008`.

Le TOML runtime personnalisé différait volontairement du fichier stock et a
été conservé byte-exact. Deux cold starts avec la pile complète ont ensuite
chargé le même correctif `0.1.0` de SHA-256
`C7193FADB024236136E241B79164EFF4CF86C0ED5C0E7CA77454D1BD7CD8CE17`
et MapSense `1.0.0` de SHA-256
`2B71748E53084FDE72E36731293251C9776072F690AA7224F4E579AE7CC624A1` :

1. correctif mod-local : `PASS`;
2. correctif global avec MapSense mod-local : `PASS`.

Les deux démarrages atteignent `D2R startup complete` avec `39 plugins
loaded`, `1 global duplicate skipped`, `17 memory patches`, les cinq DLL
eezstreet et aucune erreur fraîche du loader, du correctif ou du témoin
MapSense. Les preuves sont conservées localement sous
`analysis-cache/runtime-validation/automap-serialization-fix-d2rloader-1.2.1-preview10-20260903/`.

Après la matrice, l'état plugin antérieur est restauré exactement : MapSense
`0.13.41` au SHA-256
`25B18515A47B121C4F5905E8D16A8FA8560370519028A6BA31C11E35A8D9E24A`,
`14` DLL mod-locales, `25` DLL globales, aucun correctif installé et aucun
processus D2R/D2RLoader. D2RLoader 1.2.1 preview 10 reste la baseline déjà
installée; sa configuration personnalisée n'a pas changé.

## Gate courant

Le candidat config-free `0.1.0` est implanté. Le build Release x64 strict
`/W4 /WX`, CTest du plugin, CTest de MapSense, les trois exports D2RLoader,
les métadonnées PE/PluginInfo et deux builds reproductibles passent. L'artefact
statique mesure `25 600` octets et porte le SHA-256
`C7193FADB024236136E241B79164EFF4CF86C0ED5C0E7CA77454D1BD7CD8CE17`.

La matrice de cold start, de coexistence et le gate gameplay sont fermés sur
Battle.net `3.3.93847` avec D2RLoader `1.2.0-beta` et
`1.2.1-beta+preview.10`. Le témoin gameplay a sérialisé un arbre de `6 000`
cellules tag-zéro, soit `36 000` octets, puis a traversé
`layer 0 → layer 1 → layer 0` sans crash. Au retour, les `6 000/6 000` clés
étaient restaurées avec le tag natif `1` et aucune avec le tag `0`.

Le release candidate config-free
`AutomapSerializationFix-0.1.0-rc.1.zip` contient seulement la DLL testée à la
racine. Il mesure `11 465` octets et porte le SHA-256
`5D76BC7B9FC66D61CB5D843D187E076C2F480C40FC279352C5F9793235778131`.
Le README reste à côté du ZIP et n'y est pas inclus. Steam `3.3.93787` et la
compatibilité multijoueur croisée restent `not run` et non revendiqués; le RC
n'ajoute pas automatiquement le plugin au registre Suite `1.3.0`.

## Preuve gameplay déterministe et restauration

Le 3 septembre 2026, un harnais diagnostique local et gitignoré a exigé
l'empreinte exacte du correctif de release avant de charger. Sous la pile
complète, D2RLoader a rapporté `40 plugins loaded`, `1 global duplicate
skipped`, `17 memory patches`, les cinq DLL eezstreet et `D2R startup
complete`. Le harnais a ensuite fait croître l'arbre floor du layer `0` de
`2 654` à `8 654` nœuds avec exactement `6 000` clés tag-zéro et un payload
attendu de `36 000` octets.

Le contrôle Rogue Encampment → Blood Moor est resté volontairement sur le
layer `0` et a retrouvé les clés encore tag-zéro. Le vrai aller-retour par le
Den of Evil a forcé `0 → 1 → 0`; la vérification finale a donné
`found=6000/6000`, `tags=0:0,1:6000,other:0`. Le sidecar témoin
`Helena.ma0` est passé de `50 310` à `86 348` octets, SHA-256 final
`D0D2A0184E08CC2DC51468859AE98A1415ABC860AF3C26333DF169DD68EC4EB2`.
Les captures, le sidecar et les logs sont conservés sous
`analysis-cache/runtime-validation/automap-serialization-fix-gameplay-20260903-220747/`.

Après capture, les six fichiers de la famille `Helena` ont été restaurés à
leurs hashes initiaux, l'inventaire original de `39` DLL a été retrouvé, les
deux DLL de test ont été sorties du runtime et aucun processus D2R/D2RLoader
n'est resté actif.
