# BKVince — alignement data D2R 3.3.93847

Dernière mise à jour : 19 août 2026

## Décision

Vincent a retenu l’option A : établir d’abord une référence Vanilla 3.3
canonique, conserver intacte la référence 3.2.92777, puis porter uniquement les
deltas TXT explicitement acceptés dans BKVince. Ce chantier est un gate
environnemental temporaire avant la reprise de la matrice gameplay BKVCombat;
il ne remplace pas BKVCombat comme mission courante.

L’update officielle a installé `D2R.exe 3.3.93847`. D2RLoader 1.1.0-beta a
chargé la pile active mais a signalé que 3.3 sort de sa plage déclarée
`[3.0, 3.3)`, puis D2R a levé `BC_ASSERT: false` dans
`D2Common/src/Items/Items.cpp:1990` pendant la lecture du résumé du personnage
hors ligne. Vincent a choisi `Exit`; les sauvegardes BKVince originales sont
restées byte-exactes. Aucun succès de compatibilité binaire 3.3 n’est inféré de
ce démarrage.

## Référence Vanilla 3.3 gouvernée

`data-vanilla3.3/data/data/global/excel` provient directement du stockage CASC
officiel local, jamais du cache de compilation D2RLoader :

- version : `3.3.93847`;
- build key : `623f7a1f73eabb08ccb2b2046e3f9164`;
- SHA-256 `.build.info` :
  `2EBCAD0521DBF038D5A7FE5395E96B4BEF6D4F0774F7B1F840E03C3DE9CB067A`;
- SHA-256 `D2R.exe` :
  `E1F5436E3D9687F644EF16938B1B183D1FDEF434F18CF66D852CF68F48CC8936`;
- CascLib `1.50.0.206`, SHA-256
  `EAF1DE2512E9D0F7BACC27DC128C624FA971115D09145CDE46FE49062E8DA1A2`;
- `175824` entrées CASC énumérées, `361` fichiers Excel sélectionnés,
  `22995401` octets lus avec `CASC_STRICT_DATA_CHECK`;
- `361/361` tailles et CKey-MD5 concordantes, zéro entrée manquante;
- SHA-256 déterministe de l’arbre :
  `D0E3625EB96FAD6407CB9BE57D9C81250BA47C5223187DF39C26E1D551124360`.

Le manifeste exhaustif réside dans
`Mission/bkvince-d2r33-vanilla-manifest.json`. `data-vanilla3.2` demeure la
référence read-only canonique de 3.2.92777 et de toutes les preuves natives
92777.

## Audit Vanilla 3.2 → 3.3

Les headers des tables communes sont inchangés. Les changements TXT officiels
touchent seulement `cubemain`, `monstats`, `propertygroups`, `runes`,
`setitems`, `sets`, `skills`, `states`, `treasureclassex` et `uniqueitems`.
L’existence d’un delta officiel ne constitue pas une autorisation de merge.

## Allowlist approuvée

Le port BKVince est limité à trois tables et s’applique cellule par cellule :

1. `cubemain.txt` — six recettes de Sunder : `lvl 69 → 75`, localisées par
   leur `output` unique.
2. `monstats.txt` — `TreasureClassHerald(H)` devient
   `Act 3 (H) Herald C` pour `councilmember1`, `councilmember2` et
   `councilmember3`.
3. `treasureclassex.txt` :
   - ajouter les 13 nouvelles TC Shard officielles;
   - porter 56 cellules sur les 14 Citem;
   - porter 42 cellules sur les 14 Uitem;
   - rerouter `Item3` des 14 Herald Item;
   - porter les 84 bonus de qualité des 28 Herald Extra :
     `800/800/850 → 850/850/920` pour `- 1 Extra` et
     `800/800/850 → 870/870/950` pour `- 2 Extra`.

La forme BKVince sans guillemets `gld,mul=1280` est conservée dans la nouvelle
TC Citem, car elle est sémantiquement identique et cohérente avec les tables
locales.

## Valeurs BK préservées explicitement

- `All Acts Terrorize Consumable.NoDrop = 5` reste inchangé. En solo, la
  sous-table peut donc encore ne pas produire de Worldstone Shard; la garantie
  plus généreuse de Vanilla 3.3 est refusée par choix produit.
- `Sunder Charms` conserve les poids BK/BKVince
  `20/20/18/12/10/10`; la distribution Vanilla
  `30/30/20/12/6/3`, orientée Cold/Fire, n’est pas importée.
- Les distributions d’équipement, de runes, de gemmes, les wrappers de boss et
  toutes les autres personnalisations BK/BKVince restent intactes.

## Exclusions fermes

Aucun port de :

- `skills.txt`;
- `uniqueitems.txt`;
- buffs Angelic dans `setitems.txt` ou `sets.txt`;
- `runes.txt:lastLadderSeason`;
- `propertygroups.txt`;
- `states.txt`;
- les 24 changements officiels de boss dans `treasureclassex.txt`.

La valeur Angelical Raiment déjà présente dans BKVince n’est ni ajoutée ni
retirée par ce chantier.

## Validation et compatibilité

### Résultat technique du 19 août 2026

- migration gouvernée : **205/205** cellules existantes et **13/13** lignes TC
  ajoutées; `--check` idempotent, CRLF, final EOL, absence de BOM et round-trip
  byte-exact **PASS**;
- exclusions : hashes BKVince exacts pour `skills`, `uniqueitems`, `setitems`,
  `sets`, `runes` et `states`; `propertygroups` toujours hérité;
- valeurs produit : `All Acts Terrorize Consumable.NoDrop=5` et poids Sunder
  `20/20/18/12/10/10` confirmés après écriture;
- cadastre régénéré : `data-vanilla3.3` canonique read-only et validation
  **VALID**;
- hashes source/runtime identiques :
  - `cubemain.txt` :
    `2554AC1E45EC2A4679F0952A9BF3A6B8446AA82E40ADBAE091A8BA6C3B7EE344`;
  - `monstats.txt` :
    `A9196B151BE9C3E92BB6F1B3868CAA312FCA06A9B4992E7C47C6C5FD3520B3B1`;
  - `treasureclassex.txt` :
    `2BEC4A22F2FEA2C8C16AF2F850B129B04A8DEFDDC02C2D8CB96BDE378D2461AC`;
- cold start `3.3.93847` avec pile complète : **27 plugins**, **17 memory
  patches**, **24/24**, **188 tables** recompilées depuis TXT; aucune récidive
  de `D2Common/src/Items/Items.cpp:1990`; Vincent atteint le jeu et confirme
  « ça marche »;
- D2RLoader conserve son avertissement de plage `[3.0, 3.3)` et capture une
  assertion TACT récurrente `type_ops.cpp:47`, déjà ignorée par
  `ignored_asserts.txt`; ce signal est distinct de l’assertion Items corrigée.

Les constantes Hero Editor sont courantes et le build Vite passe. Sa suite
locale passe **84/85** : l’unique attente obsolète `19` contre `20` beltables
provient du témoin Readable Items `rds`, extérieur à ce port. `verify:data`
s’arrête également sur l’asset Mercenary Command déjà divergent. Le garde-fou
global read-only refuse, comme prévu, le nouveau snapshot tant qu’il demeure
non commité; aucune de ces trois limites n’invalide les validations ciblées
ci-dessus.

### Gates encore ouverts

- les observations gameplay Council, Herald Extra, Worldstone Shards et Sunder
  Charms restent **NOT RUN**;
- 149 fichiers actifs BKVince ont été hashés avant et après le démarrage : 147
  restent byte-exacts; `QtyTester.d2s` et `QtyTester.d2rl` ont été réécrits à
  taille identique pendant l’entrée réussie de Vincent. Aucune corruption n’est
  inférée, mais le gate « 149/149 inchangés » n’est pas revendiqué;
- un port TXT réussi ne rend pas automatiquement compatibles avec 3.3 les DLL,
  RVA ou memory patches prouvés uniquement sur 3.2.92777. Leur documentation ne
  passera à « 3.2+ » qu’après preuve build/signature/runtime propre à chaque
  composant.

Le blocage de démarrage qui suspendait BKVCombat est fermé; le chantier data
reste ouvert uniquement pour les observations gameplay ci-dessus et la
qualification native séparée.

## Rollback

Le rollback retire les 13 TC ajoutées et restaure exclusivement les 205
cellules BKVince pré-port depuis les témoins hashés. `data-vanilla3.3` reste une
référence read-only; `data-vanilla3.2` n’est jamais modifiée. Aucun commit ni
push n’est effectué sans demande explicite de Vincent.
