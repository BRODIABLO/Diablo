# ProgressiveAffixesPlugin autonome — D2R 3.2.92777

Dernière mise à jour : 11 août 2026

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
Magic, Rare et Crafted avec un TOML directement compréhensible par un joueur.
Le profil v0.2.1 conserve les seuils PD2 comme base, mais fait progresser les
Rare Jewels depuis leur distribution vanilla jusqu’au maximum :

- Magic : deux affixes garantis pour armes/armures à ilvl 65, jewels/jewelry à
  85 et charms à 90 ;
- Rare Jewels : chances 3/4 affixes de `50/50` à ilvl 1, `37.5/62.5` à 45,
  `25/75` à 65 et quatre garantis à 85 ;
- autres Rare : chances 3/4/5/6 de `12.5/37.5/37.5/12.5` à ilvl 1,
  `0/25/50/25` à 45, `0/0/50/50` à 65 et six garantis à 85 ;
- Crafted : chances 1/2/3/4 de `40/20/20/20` à ilvl 1, `0/60/20/20` à 31,
  `0/0/80/20` à 51 et quatre garantis à 71, en plus des propriétés fixes de la
  recette.

Le format joueur expose uniquement `[magic]`, `[rare_jewels]`,
`[regular_rare_items]` et `[crafted]`. Chaque ligne `from_level_N` contient des
pourcentages totalisant exactement 100, convertis en centièmes de pourcentage
sans arrondi flottant. Le parseur accepte toujours le format avancé v0.1.0 à
catégories ordonnées et poids entiers, mais refuse de mélanger les deux formats.

## Preuves natives gouvernées

Le gate obligatoire `npm.cmd run re:d2r32 -- status`, rejoué le 11 août 2026,
vérifie :

- image canonique SHA-256
  `CC59119DC2A6C7D43D088098FC162EAFA4AE1299B2079126AEF43C1ACA914715` ;
- image d’analyse SHA-256
  `673E8C0B2E89563E75525B24D137098EFD07B2DB4ED42ADEC56AA1ADDF0E63AB` ;
- index vérifié : 105 850 fonctions, 1 057 329 références, 53 patch-sites et
  372 connaissances.

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

La branche Rare Jewel native à `0x58BC65` calcule le compte avec un bit du seed,
soit trois ou quatre affixes à chances égales, sans progression selon l’ilvl.
Le premier palier v0.2.0 reproduit exactement ce comportement avant d’augmenter
progressivement la chance de quatre affixes.

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

Le relais du load prefix `0x442C78` est appelé avant que le prologue Magic
copie `RCX/RDX` vers `RDI/RSI`. Depuis la v0.2.1, il appelle donc le sélecteur
C++ avec un cadre Windows x64 complet, puis restaure exactement `RCX` et `RDX`
avant de rendre la main à D2R. Les relais suffix, Rare et Crafted restent
inchangés parce que leurs points d’injection n’exposent pas cette contrainte.

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

État du 11 août 2026 :

| Gate | Statut | Preuve |
|---|---|---|
| Configure/build Release x64 | passed | Visual Studio 2022, SDK D2RLoader épinglé `efcfaaa…970` |
| Warnings | passed | `/W4 /WX /permissive- /utf-8` |
| Politique TOML | passed | CTest `1/1`, format joueur, compatibilité v0.1, pourcentages exacts, seuils, distributions et refus invalides |
| Reproductibilité | passed | deux clean builds `/Brepro` identiques |
| DLL | passed | 161 280 octets, SHA-256 `6F70AB9EAA6238DAB8CE685B7C4C2E3E6A961326F71A82C566B86CFBD684DD0D` |
| TOML | passed | 1 145 octets, SHA-256 `2011929145203A6E2C2C06011376E774828C60676BD0311F70D1E5BE6D4AF41F` |
| Métadonnées | passed | v0.2.1, RuffnecKk, description courte, trois exports D2RLoader exacts |
| Hash build/package/BKVince | passed | DLL et TOML identiques dans les trois emplacements concernés |
| Cold start mod-local | passed | v0.2.1, build 92777, config mod-locale, `19/19` plugins, `15/15` patchsets et `24/24` tâches de démarrage |
| Cold start global + repli config | historical | v0.2.0 validée `[global]` avec `18/18` et `15/15`; non rejoué pour le hotfix v0.2.1 |
| Coexistence, configuration BKVince active | passed | cinq DLL PluginPack et tous les plugins actuellement chargés conservés, aucun composant retiré pour les deux cold starts |
| Coexistence PluginPack toutes fonctions | blocked | fonctions volontairement désactivées dans `D2RPlugins.json` et conflits full-stack préexistants `0x589736`, `0x314110`, `0x18885B/0x18887F` à fermer sans neutraliser de composant |
| Résolution runtime des ItemTypes | partial | le chemin de création d’un nouveau personnage passe; les six codes et les trois qualités ne sont pas encore couverts |
| Matrice gameplay | partial | création neuve et entrée en jeu sans crash; bornes, distributions et save/reload restent à exécuter |
| Hôte/joiner | not run | témoin multijoueur requis |

La création neuve ferme la régression de crash, mais pas la matrice statistique.

## Incident v0.2.0 et hotfix v0.2.1 — 11 août 2026

La création d’un Necromancer sous la v0.2.0 a produit une violation d’accès
`C0000005` dans `UNITS_GetUnitType`, pendant `Game::AddPlayerToGame`. Le rapport
`d2r-crash-report (2026_08_12 00_46_06 UTC).log`, SHA-256
`B84B217432ED082F1CF4F78DE9DE8098FB6B9FC7DDABCE0909362FCCE508EC46`,
contient deux adresses de retour dans `ProgressiveAffixesPlugin.dll`, après
`ITEMS_CheckItemTypeId` et `FindCategory`.

La preuve binaire gouvernée ferme la cause : le hook prefix à `0x442C78` était
placé avant `mov rsi, rdx` et `mov rdi, rcx`. Le relais v0.2.0 effectuait un
tail-jump vers un helper C++ autorisé par l’ABI Windows x64 à écraser les deux
registres volatils. D2R sauvegardait ensuite des arguments corrompus, jusqu’à
présenter le faux pointeur `0x8C001753B395D4B6` au test d’ItemType.

La v0.2.1 remplace uniquement ce relais par une séquence de 25 octets qui :

1. empile `RCX` et `RDX` ;
2. réserve `0x28` octets pour l’alignement et le shadow space ;
3. appelle le helper ;
4. restaure la pile, `RDX`, puis `RCX` ;
5. retourne au prologue natif.

Un test de non-régression vérifie octet par octet la séquence et le remplissage
`0xCC` du slot de 32 octets. Deux clean builds Release x64 passent CTest `1/1`
et produisent la même DLL. Le déploiement ciblé ne remplace que la DLL; le TOML
reste byte-identique.

Le cold start corrigé charge `ProgressiveAffixesPlugin 0.2.1 [mod]` avec
`19/19` plugins, `15/15` patchsets et `24/24` tâches de démarrage, sans rejet ni
échec. Aucun rapport de crash postérieur à 20:46:06 n’est apparu. Vincent a
ensuite créé un nouveau personnage et confirmé l’entrée en jeu sans crash. La
session D2R stable reste ouverte à sa demande implicite d’usage.

## Qualification runtime du 10 août 2026

La v0.2.0 remplace la DLL et le TOML v0.1.0 sans toucher aux autres composants.
Les trois patchsets force-max restent archivés hors des dossiers chargés. La DLL
et le TOML ont été synchronisés avec des hashes source/runtime identiques.

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
été retiré et le profil mod-local BKVince restauré byte-identique. Une seule
instance mod-locale stable a été relancée après la restauration.

Ces témoins prouvent les deux portées et la coexistence de chargement avec la
configuration active. Ils ne prouvent pas encore les distributions en jeu, la
résolution tardive des six ItemTypes, les fonctions PluginPack actuellement
désactivées, ni le multijoueur.

## Patch compagnon BKVince — niveau requis Crafted

Vincent confirme le 10 août 2026 que les objets Rare ne doivent pas servir
d'entrées aux recettes de crafting, puis autorise uniquement les phases de
preuve native, d'implantation du patch et de validation statique. Aucun fichier
TXT, aucune recette et aucune entrée `rar` ne sont ajoutés.

Le calcul 92777 est fermé statiquement dans `ITEMS_GetRequiredLevel 0x376DE0`.
Le chemin qualité Crafted `8` ajoute séparément un bonus fixe de `10`, puis `3`
par préfixe et par suffixe présents. Le chemin commun conserve ensuite les
exigences des affixes, socketables et skills, ajoute `item_levelreq=92`, borne
le résultat à zéro et applique l'ajustement dépendant du personnage.

Le patch BKVince distinct
`data-BKVince/d2rloader/patches/crafted-rare-level-requirements.json` neutralise
uniquement :

- `0x376EBA` — initialisation du bonus fixe `+10` ;
- `0x377089` — incrément `+3` pour un préfixe Crafted ;
- `0x377092` — incrément `+3` pour un suffixe Crafted.

Les signatures étendues de l'initialisation et de la boucle sont uniques dans
le `.text` 92777. Le patch ne chevauche aucune des six surfaces de
ProgressiveAffixesPlugin et ne modifie ni sa DLL, ni son TOML, ni son ZIP
public. Il s'agit d'une règle BKVince indépendante et réversible; retirer le
JSON restaure immédiatement la formule vanilla sans migration de sauvegarde.

La validation statique du 10 août 2026 confirme les octets `expected` sur
l'image canonique vérifiée, l'unicité des deux témoins `.text`, la cohérence
des 53 opérations contenues dans les 17 patchsets actifs et zéro chevauchement.
Les deux JSON sont valides, le self-test du workbench passe, l'index reconstruit
reconnaît les quatre entrées gouvernées et le cadastre est conforme à son
schéma. Aucun succès gameplay ou multijoueur n'est déduit de ces preuves
statiques.

## Rollback

Le rollback BKVince est atomique : retirer DLL et TOML, puis restaurer ensemble
les trois patchsets JSON précédents. Ne jamais laisser le plugin et un patchset
force-max actifs simultanément. Les objets générés avant rollback restent des
objets ordinaires et ne demandent aucune migration.

## Livraison publique

Allowlist stricte :

- `ProgressiveAffixesPlugin.dll` ;
- `ProgressiveAffixesPlugin.toml`.

Archive `ProgressiveAffixesPlugin-0.2.1.zip` : 70 144 octets, SHA-256
`F95D4BBDA94D296B3632E9F44B87B2B9137B4177079191D9CEC0B3D77DE6A82E`.
L'inspection confirme exactement deux entrées à la racine, la DLL et le TOML.

Le README, les sources, symboles, logs et preuves restent hors ZIP. L’archive ne
est validée pour le démarrage complet et la création d’un personnage. Les
distributions, les seuils, le multijoueur et la compatibilité universelle
PluginPack restent hors de cette conclusion.
