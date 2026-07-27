# Mission — interdire l’éthéré par famille d’item types sous D2R 3.2

## Intention

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

1. Choisir au moins un code précis et, séparément, un parent pour la validation.
2. Matrice en jeu : type précis, parent, descendant non ciblé, normal/magic/
   rare/set/unique/crafted, drop naturel, vendeur, gamble, Cube forcé éthéré,
   `ALWAYSETH`, solo, hôte/joiner, sauvegarde et rechargement.
3. Vérifier que le taux BKVince de 6 % et les sets éthérés restent actifs pour
   les familles non exclues.
4. Générer chacun des sept uniques `indestruct`, vérifier qu'ils peuvent recevoir
   le drapeau éthéré tout en restant indestructibles, puis contrôler un arc ou
   une autre base réellement sans durabilité comme témoin négatif.
