# Mission — bloc EthItemRules unifié sous D2R 3.2

## Intention

Le 28 juillet 2026, Vincent confirme l’intégration future dans la catégorie
`items`, avec `plugin-items.dll` comme DLL propriétaire. Il clarifie ensuite que
`Exclude ItemTypes from Rolling Ethereal` et `Ethereal Item Rules` ne sont pas
deux fonctionnalités finales : elles forment ensemble un seul bloc
`EthItemRules` sous l’unique clé `items.etherealItemRules` de
`D2RPlugins.json`. L’exclusion des familles devient un champ interne du même
bloc que le taux éthéré, les sets éthérés et l’admissibilité des objets
indestructibles. La clé `items.etherealExclusions` ne fait pas partie du contrat
final.

Le séquencement comporte deux paliers techniques. Les deux implémentations sont
d'abord réunies dans le témoin RuffnecKk autonome `EtherealItemRules 0.1.0` : le
hook d'exclusion des familles et les quatre opérations d'`Ethereal Item Rules`
y partagent le contrôle du build 92777 et une configuration JSON. Après ses
preuves de compilation, signatures et cold start, Vincent précise que ce témoin
ne doit pas être traité comme un composant spécial ni comme un gate gameplay
avant le pack. Il reste disponible uniquement pour comparer un comportement en
cas d'écart.

Le bloc unique `items.etherealItemRules` est donc porté directement dans
`plugin-items.dll`, propriétaire final ordinaire du sous-système. Le pack final
ne distribue ni `EtherealItemRules.dll`, ni identifiant
`ethereal-item-rules`, ni configuration autonome. Cette décision conserve
l’Option A — fusionner les deux apports avant leur intégration — et interdit de
les représenter comme deux plugins, deux composants ou deux blocs de
configuration.

Permettre à BKVince de déclarer des codes de `itemtypes.txt` qui ne doivent
jamais devenir éthérés. La politique doit être configurable sans recompilation,
comprendre l’héritage natif des types et ne modifier aucun type non sélectionné.

## Contrat fonctionnel

- configuration mod-locale dans
  `d2rloader/config/no-ethereal-item-types.toml` ;
- liste de codes de la colonne `Code` de `itemtypes.txt` ;
- un parent tel que `armo` couvre ses descendants selon la LUT d’équivalence du
  jeu, tandis qu’un type précis tel que `belt` ne couvre que sa famille ;
- l’interdiction est absolue : jet naturel, set autorisé par le patch BKVince,
  sortie Cube forcée éthérée et autre drapeau `ALWAYSETH` ;
- les types absents ou invalides ne doivent jamais être interprétés comme un
  autre type ; le statut runtime expose les résolutions manquantes ;
- refus sûr sur tout build autre que `D2R.exe 3.2.92777` ou toute signature
  incompatible.

## Implantation prouvée

La routine éthérée 92777 teste successivement `ITEMTYPE_WEAPON` puis
`ITEMTYPE_ANY_ARMOR` par `ITEMS_CheckItemTypeId` avant le jet et avant les
drapeaux forcés. Les deux appels reviennent aux RVA `0x004432DA` et
`0x004432E9`. Le helper partagé se trouve au RVA `0x00373890` avec la signature
initiale stricte :

```text
48 89 5C 24 10 48 89 6C 24 18 48 89 74 24 20
```

Le plugin n’altère le résultat du helper qu’à ces deux retours. Les codes sont
résolus dans le conteneur runtime `itemtypes` (`data tables + 0x1348`, compteur
`+0x1350`, stride `0xE8`) puis testés avec le helper original, ce qui conserve
exactement les équivalences du jeu.

## État technique au 19 juillet 2026

- `NoEtherealItemTypes.dll` 1.0.0 compilée en Release x64, taille 36 864 octets,
  SHA-256 `3034D37C2E4D9F3E4BC98B2CDE0E198AD75B6ECBF1A77244E05696E05F4F059B` ;
- tests unitaires de normalisation, padding, résolution et bornes réussis ;
- exports `D2RLoaderGetPluginInfo`, `D2RLoaderLoadPlugin` et
  `D2RLoaderUnloadPlugin` présents, manifeste v2 accepté ;
- DLL et TOML source/runtime byte-identiques ;
- archive partageable `addons/NoEtherealItemTypes/NoEtherealItemTypes.zip`,
  SHA-256 `3A6E46EC8D0ADC9EAAF6BAC915BFAF66E4D789E21976EC613F385D8789371B12`,
  vérifiée avec exactement la DLL et le TOML à la racine, sans README ni
  sources ; les commentaires du TOML public sont intégralement en anglais ;
- cold start sous D2RLoader 1.0.1-beta et build 92777 : hook installé à
  `0x373890`, `20/20 patches`, `8/8 plugins`, `24/24` étapes de démarrage ;
- l’assertion RapidJSON tardive déjà connue se reproduit après le frontend et
  reste indépendante de ce plugin ;
- liste `item_types` volontairement vide tant que Vincent n’a pas choisi les
  familles à activer : le plugin est chargé mais ne change encore aucun drop.

## Distribution hybride 1.1.0 (20 juillet 2026)

La DLL ne declare plus `PluginFlags::ModScopedOnly` et conserve
`PluginFlags::NativeHooks`. Elle fonctionne depuis le dossier global
`d2rloader/plugins` comme depuis le dossier local d'un mod. Dans les deux cas,
le TOML est resolu par le contexte D2RLoader correspondant.

- SHA-256 DLL Release x64 : `56CD7A727196051225F1E53D13CF0923615DF435F41C049CAA0B99C771E5883F`
- SHA-256 ZIP : `7FFC14307093EFDF7C8D15CBF8B39A7AB51F539E63E2C804B97C8140847F00D0`
- archive verifiee : DLL et TOML uniquement a la racine
- compilation Release, test de politique et exports D2RLoader valides

## Évolution — objets indestructibles éthérés (26 juillet 2026)

Le patch JSON BKVince `ethereal-item-rules.json` conserve son jet configuré de
6 % et accepte désormais les objets qui possèdent la statistique
`item_indestructible`, sans rendre admissibles les bases réellement dépourvues
de durabilité.

La routine d'éthéréalité 92777 appelle `ITEMS_HasDurability` au RVA `0x373540`
depuis `0x4432F4`. La référence sémantique D2MOO épinglée au commit
`19019806df7f3e877fa105b05395d1e3597e2316` confirme que ce helper refuse un
objet lorsque `STAT_ITEM_INDESCTRUCTIBLE` est positif. Le désassemblage 92777
prouve le même veto : après les contrôles `items.txt` et la durabilité maximale,
le helper lit la stat `0x98` par `STATLIST_GetUnitStat` au RVA `0x2F5020` et ne
retourne vrai que lorsque sa valeur est inférieure ou égale à zéro.

Le premier appel, antérieur au jet aléatoire, est redirigé vers un helper de
67 octets au RVA `0x46D840`. Cette plage était composée uniquement de `INT3`,
ne possédait aucune xref dans l'index gouverné et se situe entre le `RET` à
`0x46D83C` et le code suivant à `0x46D8F0`. Le helper :

1. réutilise `ITEMS_HasDurability` et conserve immédiatement son résultat vrai ;
2. si le résultat est faux, exige `item_indestructible` (`0x98`) strictement
   positif ;
3. exige aussi une durabilité maximale effective strictement positive via
   `STATLIST_GetMaxDurabilityFromUnit` au RVA `0x2F4B60` ;
4. laisse intact le second appel vanilla à `ITEMS_HasDurability` au RVA
   `0x443507`, après l'application du drapeau éthéré, afin de ne pas réécrire ou
   réduire inutilement la durabilité d'un objet indestructible.

Les tables BKVince ont été lues par le parseur TSV gouverné avec round-trip
byte-exact et CRLF confirmé. Sept uniques portent actuellement la propriété
`indestruct` — `Ethereal Edge`, `Ghostflame`, `Shadowkiller`, `Corrupted
Harlequin Crest`, `The Corrupted Grandfather`, `Corrupted Arkaine's Valor` et
`Wind God Fist` — et leurs sept bases ont une durabilité non nulle.

Validation technique : image canonique 92777 vérifiée, 61/61 sites des 20
patchsets conformes aux octets `expected`, zéro chevauchement. Le cold start
frais du 26 juillet utilisait un JSON source/runtime byte-identique, SHA-256
`2F7C30BBB926EB994B4BABB98D058C35E5E7BDE2515D3856E8ADFEBBF6BB83A4`.
La clarification player-friendly ajoutée ensuite aux descriptions porte le
SHA-256 source actuel à
`51548C9BA14E9143F95C571457BB95E75EB25FE5C6DF57F54D5429F721C7CD6A`,
sans modifier les quatre opérations du patch. Le cold start accepte le build
92777, charge
`Ethereal Item Rules` avec quatre patches, puis termine avec
`scanned=20 applied=20 disabled=0 failed=0`, 22/22 plugins actifs sans rejet ni
échec et les 24/24 étapes de démarrage. La relecture du processus confirme les
quatre sites actifs : taux `06` à `0x4434DF`, NOP set à `0x443315`, redirection
`E8 47 A5 02 00` à `0x4432F4` et helper exact de 67 octets à `0x46D840`.
La validation gameplay reste distincte et ouverte.

## Validation requise

### Prototype autonome unifié 0.1.0 — 28 juillet 2026

`EtherealItemRules` a réuni les deux témoins dans une DLL RuffnecKk hybride
unique, sans TOML et sans liaison ni redistribution d’une DLL d’eezstreet. Son
prototype 0.1.0 exposait encore deux objets JSON, `etherealExclusions` et
`etherealItemRules`; cette représentation est désormais explicitement rejetée.
Le prochain port doit exposer un seul bloc `etherealItemRules`, désactivé par
défaut, contenant le taux vanilla `5`, les autorisations set/indestructible à
`false` et une liste vide des ItemTypes exclus. Aucun gameplay final ne doit être
validé contre l’ancien contrat à deux blocs.

- Release x64 : 142 336 octets, SHA-256
  `BAF958001ED43624F98C04672D4B844B6D2D0FEBE9B0EA1A04DF37EF76859174` ;
- JSON vanilla : SHA-256
  `22E0A07F5654FDE49F2693E6627BC649F963BDCBABC2A14879F8947B280ADA80` ;
- test de politique `1/1`, manifeste API v2, PE32+ x64 et trois exports
  D2RLoader attendus ;
- audits du build 92777 et du PluginPack officiel épinglé `dc75b49` fermés :
  aucun propriétaire existant dans `plugin-items`, signatures strictes
  préflightées avant toute mutation ;
- cold start actif mod-local puis global avec les anciens propriétaires
  neutralisés : hook `0x373890`, taux `06`, six NOP set, call
  `E8 47 A5 02 00` et helper exact de 67 octets observés dans le processus ;
  dans les deux portées, `scanned=29 active=27 disabled=2 rejected=0 failed=0`,
  `scanned=19 applied=19 disabled=0 failed=0` et démarrage `24/24` ;
- cold start final avec le JSON livré vanilla et les témoins restaurés : zéro
  ligne de hook pour `ethereal-item-rules`, une ligne pour l'ancien témoin,
  `scanned=30 active=28 disabled=2 rejected=0 failed=0`, 20/20 patchsets et
  démarrage `24/24` ;
- JSON présent mais invalide (`chancePercent=101`) refusé avec l'erreur attendue,
  `failed=1` pour cette case négative et démarrage `24/24`; le JSON vanilla est
  ensuite restauré par son SHA-256 exact ;
- runtime final : nouvelle DLL mod-locale et JSON vanilla byte-identiques aux
  sources, trois anciens témoins restaurés avec leurs hashes exacts, aucune
  copie globale et zéro processus D2R restant ;
- rapport local :
  `analysis-cache/ethereal-item-rules-runtime-validation/20260728-unified-prototype/report.json`.

La validation technique autonome est acquise. Les comportements en jeu, le
multijoueur et la persistance restent ouverts; aucun ZIP public n'est produit à
ce stade.

### Port conjoint dans `plugin-items.dll` — 28 juillet 2026

Le module `plugin-items` contient maintenant les deux politiques dans des
sources internes et demeure l'unique propriétaire runtime. Le manifeste passe
de 57 à 62 sites uniques. Les cinq DLL Release x64 compilent et les trois CTest
passent. Le `D2RPlugins.json` joueur conserve vanilla par défaut : sections
désactivées, taux 5 %, sets et indestructibles à `false`; le défaut actif
historique `skills.selfHealParams=true` est également corrigé à `false`.

- `plugin-items.dll` : SHA-256
  `FB4AD6015DFCEE02FFECEFBF2056155F4EB1E7E8CAB9A54AE13DC0B0BBBC823D` ;
- checkpoint PluginPack : `a51c865` (`Port ethereal rules into plugin-items
  prototype`), poussé et synchronisé à `0/0` sur
  `RuffDood/D2RL-Plugins:codex/pluginpack-foundation` ;
- JSON joueur : SHA-256
  `68889F5D3BA02FAE6D3CA251172BBA50AE35B8C1815F4D2CB784F470CAAD10B7` ;
- cold start vanilla : cinq plugins eezstreet actifs, aucun hook ethereal,
  `scanned=28 active=26 disabled=2 rejected=0 failed=0`, startup `24/24` ;
- cold start actif avec `belt`, `armo`, taux 6 %, sets et indestructibles :
  hook `0x373890` installé par `eezstreet-plugin-items`, tous les patches
  acceptés, aucun propriétaire autonome chargé, même résumé sans échec et
  startup `24/24` ;
- rollback : les cinq anciennes DLL du pack, le JSON BKVince, les deux DLL
  témoins et le patch JSON ont tous été restaurés par SHA-256 après un retry de
  verrou tardif; aucun processus D2R/D2RLoader ne reste actif ;
- preuves locales :
  `analysis-cache/pluginpack-foundation-runtime-validation/20260728-170819582/`.

Cette validation prouve l'intégration technique et l'absence de double
propriétaire. Elle ne remplace pas l'observation gameplay du module fusionné.

1. ✅ Construire et cold-starter d'abord le composant autonome commun avec les
   anciens propriétaires désactivés, puis vérifier que son hook et ses quatre
   opérations sont les seuls sites actifs de ce sous-système — acquis dans les
   deux portées le 28 juillet 2026.
2. ✅ Porter les deux politiques sous `plugin-items.dll`, compiler le pack et
   cold-starter les profils vanilla et actif sans les anciens propriétaires —
   acquis le 28 juillet 2026.
3. Choisir au moins un code précis et, séparément, un parent pour la validation
   du module fusionné.
4. Matrice en jeu : type précis, parent, descendant non ciblé, normal/magic/
   rare/set/unique/crafted, drop naturel, vendeur, gamble, Cube forcé éthéré,
   `ALWAYSETH`, solo, hôte/joiner, sauvegarde et rechargement.
5. Vérifier que le taux BKVince de 6 % et les sets éthérés restent actifs pour
   les familles non exclues.
6. Générer chacun des sept uniques `indestruct`, vérifier qu'ils peuvent recevoir
   le drapeau éthéré tout en restant indestructibles, puis contrôler un arc ou
   une autre base réellement sans durabilité comme témoin négatif.
7. En cas d'écart, rejouer uniquement le cas fautif avec le témoin autonome;
   retirer les artefacts autonomes du lot final après équivalence fusionnée.

## Validation gameplay intégrée — 30 juillet 2026

Avec `items.etherealItemRules.enabled=true`, `chancePercent=100` et
`allowSetItems=true`, Vincent confirme en jeu que les objets admissibles
apparaissent tous éthérés et que les objets de set peuvent eux aussi apparaître
éthérés. Ces deux cas du module intégré sont `passed`.

L'ancien memory patch BKVince `ethereal-item-rules.json`, devenu un second
propriétaire inutile du même comportement, a été retiré de la source gouvernée
et du profil runtime à la demande de Vincent. Les autres memory patches, dont
Infinite Larzuk, demeurent inchangés. Les exclusions par ItemType,
l'indestructible, le multijoueur et la persistance restent `not run`.

## Promotion RuffnecKk Suite 1.0.0 — 18 août 2026

Vincent confirme qu'Ethereal Item Rules n'est plus un prototype et approuve sa
promotion de `0.1.1` à `1.0.0`. La politique et la configuration restent
inchangées. Les builds Debug et Release passent `25/25` tests; deux builds
Release indépendants concordent byte-for-byte, et le runtime live charge
`Ethereal Item Rules 1.0.0` dans la pile complète à `26` plugins,
`19` patches et `24/24`.
