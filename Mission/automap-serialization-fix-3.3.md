# Automap Serialization Fix — D2R 3.3

Dernière mise à jour : 3 septembre 2026

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

## Gate courant

Le candidat config-free `0.1.0` est implanté. Le build Release x64 strict
`/W4 /WX`, CTest du plugin, CTest de MapSense, les trois exports D2RLoader,
les métadonnées PE/PluginInfo et deux builds reproductibles passent. L'artefact
statique mesure `25 600` octets et porte le SHA-256
`C7193FADB024236136E241B79164EFF4CF86C0ED5C0E7CA77454D1BD7CD8CE17`.

La matrice de cold start et de coexistence est fermée sur Battle.net
`3.3.93847`. Le prochain gate est une preuve gameplay sur un payload supérieur
à 32 767 octets : changement de layer sans crash, retour sur le layer et
persistance de l'automap. Steam `3.3.93787` et la compatibilité multijoueur
croisée restent `not run` et non revendiqués. Aucun ZIP ni ajout au registre
Suite `1.3.0` n'est autorisé avant une décision de release séparée.
