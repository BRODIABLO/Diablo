# ProgressiveAffixesPlugin autonome — D2R 3.2.92777

Dernière mise à jour : 10 août 2026

## Décision produit et destination

Vincent a confirmé un **plugin autonome permanent** compatible avec le
PluginPack et les plugins du workspace, puis a demandé son implantation le
10 août 2026. Cette directive d’exécution explicite ouvre le chantier malgré le
placement initial de l’option B après les quatre lots Mechanics prioritaires.

Le produit est `ProgressiveAffixesPlugin.dll`, auteur exact `RuffnecKk`, avec
une configuration indépendante `ProgressiveAffixesPlugin.toml`. La DLL reste
hybride global/mod-local, ne déclare pas `ModScopedOnly`, n’utilise pas
`D2RPlugins.json` et ne modifie, lie ou redistribue aucune DLL eezstreet.

Description publique :
`Increases generated item affix counts as item levels rise.`

## Objectif joueur

Rendre configurable la progression du nombre d’affixes générés pour les objets
Magic, Rare et Crafted, par catégories ordonnées de codes `itemtypes.txt`. Le
TOML livré reproduit par défaut la progression Project Diablo 2 :

- Magic : deux affixes garantis pour armes/armures à ilvl 65, jewels/jewelry à
  85 et charms à 90 ;
- Rare jewels : quatre affixes garantis ;
- autres Rare : poids `1/3/3/1` pour 3/4/5/6 à ilvl 1, `0/1/2/1` à 45,
  `0/0/1/1` à 65 et six garantis à 85 ;
- Crafted : poids `2/1/1/1` pour 1/2/3/4 à ilvl 1, `0/3/1/1` à 31,
  `0/0/4/1` à 51 et quatre garantis à 71, en plus des propriétés fixes de la
  recette.

Les catégories sont évaluées dans l’ordre de déclaration. Un fallback `*`
termine chaque qualité configurée. Les poids sont des entiers exacts, sans
arrondi flottant.

## Preuves natives gouvernées

Le gate obligatoire `npm.cmd run re:d2r32 -- status` du 10 août 2026 vérifie :

- image canonique SHA-256
  `CC59119DC2A6C7D43D088098FC162EAFA4AE1299B2079126AEF43C1ACA914715` ;
- image d’analyse SHA-256
  `673E8C0B2E89563E75525B24D137098EFD07B2DB4ED42ADEC56AA1ADDF0E63AB` ;
- index vérifié : 105 850 fonctions, 1 057 329 références, 57 patch-sites et
  352 connaissances.

La référence PluginPack est épinglée et propre au commit
`eezstreet/D2RL-Plugins@dc75b49ffbb67b887d7757ee00ee9a03bcde5d8a`.
Les recherches `affix`, `442C78` et `58BCAA` y retournent zéro résultat : aucune
des cinq DLL ne possède les surfaces ci-dessous.

| Surface 92777 | Preuve et rôle | Propriétaire ProgressiveAffixesPlugin |
|---|---|---|
| `0x442C60` | générateur Magic `(itemWrapper, generation)` ; le wrapper contient l’item à `+0x00` | les loads de décision `0x442C78` et `0x442CDC` seulement |
| `0x58A120` | générateur Crafted `(item, generation)` ; ilvl à `generation+0x18` | call RNG `0x58A21B` et clamp `0x58A220` |
| `0x58BBA0` | générateur Rare `(item, generation)` ; seed native et ilvl conservés | branche jewel `0x58BC65` et bloc de sélection `0x58BC90..0x58BCAD` |
| `0x153B00` | avance un seed et retourne `[0, borne)` | appelé par les sélecteurs pondérés Rare/Crafted |
| `0x367160` | avance un seed et retourne un index power-of-two | repli Rare vanilla uniquement |
| `0x373890` | `ITEMS_CheckItemTypeId(item, typeId)` via la LUT d’héritage native | lecture seulement, aucune propriété du prologue |
| `0x300A90` / `0x34A0E0` | tables compilées par contexte et contexte de l’item | résolution tardive stricte des codes TOML |

Les signatures exactes des fonctions, prologues, calls et blocs écrits sont
vérifiées avant toute allocation ou écriture. Un ancien patch force-max, un
autre propriétaire ou un build différent provoque un refus fail-closed.

## Architecture implantée

La DLL n’implémente pas la génération d’affixes. Quatre relais alloués à portée
`rel32` appellent des sélecteurs C++ puis rendent la main aux générateurs
92777 :

1. les deux relais Magic substituent uniquement les valeurs minimum prefix et
   suffix lorsque le palier exige deux affixes ;
2. le relais Rare choisit directement un compte 3–6 avec un unique avancement
   du seed, puis rejoint la boucle native à `0x58BCAE` ;
3. le relais Crafted choisit directement un compte 1–4 avec un unique
   avancement du seed ; le clamp vanilla est neutralisé parce que sa politique
   est désormais entièrement portée par le TOML ;
4. D2R choisit ensuite les affixes légaux, applique les limites trois prefixes /
   trois suffixes, écrit les stats, sérialise l’objet et assure la synchronisation.

La résolution des codes d’ItemType réutilise les records compilés actifs
(`dataTables+0x1348`, compteur `+0x1350`, stride `0xE8`) et la LUT native. Une
configuration absente laisse le plugin chargé mais désactivé. Une configuration
présente invalide est refusée avant les patches ; un code absent de la table
active refuse la sélection progressive au runtime et écrit une erreur explicite.

## Intégration BKVince

Le lot source remplace atomiquement :

- `force-magic-prefix-suffix.json` ;
- `force-rare-affixes.json` ;
- `force-crafted-affixes.json` ;

par :

- `data-BKVince/d2rloader/plugins/ProgressiveAffixesPlugin.dll` ;
- `data-BKVince/d2rloader/config/ProgressiveAffixesPlugin.toml`.

Le changement ne modifie aucune table gameplay ni aucun format sérialisé. Les
objets et sauvegardes existants restent valides ; seule la génération future
change.

## Validation technique

État du 10 août 2026 :

| Gate | Statut | Preuve |
|---|---|---|
| Configure/build Release x64 | passed | Visual Studio 2022, SDK D2RLoader épinglé `efcfaaa…970` |
| Warnings | passed | `/W4 /WX /permissive- /utf-8` |
| Politique TOML | passed | CTest `1/1`, seuils, distributions et refus invalides |
| Reproductibilité | passed | deux clean builds `/Brepro` identiques |
| DLL | passed | 137 216 octets, SHA-256 `92481CF78DD3BAD2DAA33212A0561CD65CFAAFB786565BB6F62F527725EB9A8F` |
| TOML | passed | SHA-256 `0C3B2617592F6871DF2A737D352108D702A1F14834AB9BDE720EA95A399A17C1` |
| Métadonnées | passed | v0.1.0, RuffnecKk, description courte, trois exports D2RLoader exacts |
| Hash build/package/BKVince | passed | DLL et TOML identiques dans les trois emplacements concernés |
| Cold start mod-local | passed | build 92777, config mod-locale, `18/18` plugins et `15/15` patchsets, zéro rejet/échec |
| Cold start global + repli config | passed | DLL `[global]`, config globale explicite, mêmes totaux `18/18` et `15/15` |
| Coexistence, configuration BKVince active | passed | cinq DLL PluginPack et tous les plugins actuellement chargés conservés, aucun composant retiré pour les deux cold starts |
| Coexistence PluginPack toutes fonctions | blocked | fonctions volontairement désactivées dans `D2RPlugins.json` et conflits full-stack préexistants `0x589736`, `0x314110`, `0x18885B/0x18887F` à fermer sans neutraliser de composant |
| Résolution runtime des ItemTypes | not run | la résolution tardive exige une génération Magic/Rare/Crafted en jeu |
| Matrice gameplay | not run | bornes Magic/Rare/Crafted, distributions, save/reload |
| Hôte/joiner | not run | témoin multijoueur requis |

La compilation et les tests statiques ne ferment pas les cases gameplay.

## Qualification runtime du 10 août 2026

Le profil installé a été qualifié sans processus Diablo préexistant. Les trois
patchsets force-max actifs en portée globale ont été déplacés hors des dossiers
chargés vers un backup récupérable sous `analysis-cache/runtime-sync-backups/`,
puis la DLL et le TOML ont été synchronisés avec des hashes source/runtime
identiques.

Le cold start mod-local `D2RLoader.exe -mod BKVince -txt` donne :

- build-name `92777`, build-comments `3.2.0 RC 5` ;
- `ProgressiveAffixesPlugin.dll [mod]` et log RuffnecKk actif avec le TOML sous
  `mods/BKVince/d2rloader/config/` ;
- memory patches `scanned=15 applied=15 disabled=0 failed=0` ;
- plugins `scanned=18 active=18 disabled=0 rejected=0 failed=0` ;
- processus encore stable après l'échantillon de démarrage, puis arrêté par le
  protocole de validation.

Le second cold start déplace temporairement et exclusivement la même DLL et le
même TOML vers la portée globale. Le loader rapporte
`ProgressiveAffixesPlugin.dll [global]`, le log sélectionne
`<D2R>/d2rloader/config/ProgressiveAffixesPlugin.toml`, et les totaux restent
`18/18` plugins et `15/15` patchsets sans rejet ni échec. Le test global a ensuite
été archivé et le profil mod-local BKVince restauré byte-identique. Aucun
processus Diablo ne reste ouvert.

Ces témoins prouvent les deux portées et la coexistence de chargement avec la
configuration active. Ils ne prouvent pas encore les distributions en jeu, la
résolution tardive des six ItemTypes, les fonctions PluginPack actuellement
désactivées, ni le multijoueur.

## Rollback

Le rollback BKVince est atomique : retirer DLL et TOML, puis restaurer ensemble
les trois patchsets JSON précédents. Ne jamais laisser le plugin et un patchset
force-max actifs simultanément. Les objets générés avant rollback restent des
objets ordinaires et ne demandent aucune migration.

## Livraison publique

Allowlist stricte :

- `ProgressiveAffixesPlugin.dll` ;
- `ProgressiveAffixesPlugin.toml`.

Archive `ProgressiveAffixesPlugin-0.1.0.zip` : 58 833 octets, SHA-256
`A08741294D62FE8BB62C0CD6E18874F165DE6C1B2E60E8E54423AEFB564B24E4`.
L'inspection confirme exactement deux entrées à la racine, la DLL et le TOML.

Le README, les sources, symboles, logs et preuves restent hors ZIP. L’archive ne
peut être déclarée validée en jeu tant que les gates runtime et fonctionnels
restent ouverts.
