# Repair Costs Cap — D2R 3.2.92777

Dernière mise à jour : 28 juillet 2026

Statut : le plafond à 100 gold et l'usure à 100 % ont été confirmés en jeu sur
le prototype autonome `RepairCostsCap.dll` 1.4.0. Vincent a ensuite autorisé le
28 juillet 2026 son intégration dans `plugin-items.dll`, sous la clé
`items.repairCostsCap`. Le port source intégré, le Release x64, les tests de
politique, les exports, la synchronisation par hash et le cold start
du port intégré passent. La DLL autonome est neutralisée de manière récupérable dans
le profil runtime afin qu'un seul propriétaire pose les hooks. Le gameplay du
port `RepairCostsCap` reste à confirmer avant de retirer les témoins autonomes.
`RepairCostsCap` demeure une fonctionnalité indépendante d’`EthItemRules`;
elles partagent seulement la DLL propriétaire `plugin-items.dll`.

## Décisions confirmées

- Vincent a confirmé le nom exact `Repair Costs Cap`, l’Option B, la catégorie
  `items`, puis l’évolution créative de la politique de prix le 24 juillet 2026.
- La DLL propriétaire est désormais `plugin-items.dll` et la clé dans l’unique
  `D2RPlugins.json` est `items.repairCostsCap`.
- L’incubation a été réalisée dans une DLL autonome attribuée exactement à
  `RuffnecKk`, sans modifier, lier ni redistribuer une DLL d’eezstreet; ce témoin
  est maintenant neutralisé dans le profil actif après le port autorisé.
- Le JSON autonome est en anglais, recherché d’abord dans le mod actif puis dans
  le dossier global du jeu. Aucun TOML n’est utilisé.
- Vincent a demandé le 26 juillet 2026 de remplacer la politique de prix
  détaillée par un contrat ultra simple : un unique `maximumGold` commun aux
  réparations individuelles et à `Repair All`, avec `0` pour rendre les deux
  actions gratuites. L’usure permanente configurable demeure disponible.

## Politique de réparation 1.4.0

Pour une réparation individuelle valide :

```text
minimum(prix vanilla, maximumGold)
```

Pour `Repair All`, chaque prix passe d’abord par ce même plafond, puis le total
final est lui aussi plafonné par la même valeur :

```text
minimum(somme des prix plafonnés par objet, maximumGold)
```

La politique ne peut donc jamais rendre une réparation plus chère que vanilla.
Les achats, ventes, résultats nuls, négatifs et sentinelles restent inchangés.
Une configuration absente conserve vanilla; une configuration présente mais
invalide ou contenant une clé inconnue fait refuser explicitement le plugin.

Configuration BKVince livrée par défaut :

```jsonc
{
  "enabled": false,
  "maximumGold": 2147483647,
  "durabilityWear": {
    "enabled": false,
    "chance": 0.0
  }
}
```

Effet concret : ces valeurs conservent intégralement les prix et les réparations
vanilla; aucun hook Repair Costs Cap n'est installé. Même si seul `enabled`
passe à `true`, `maximumGold: 2147483647` ne plafonne aucun prix et l’usure à
`0.0` ne retire rien. La valeur `0` rend les réparations individuelles et
`Repair All` gratuites; une autre valeur positive devient leur plafond commun; aucune
variation par difficulté ni aucun multiplicateur ne subsiste. L'usure est elle
aussi inactive par défaut. Lorsqu'elle est activée, chaque réparation physique
réussie donne à l'objet la probabilité configurée de perdre définitivement un point de
durabilité maximale, sans jamais descendre sous 1. Les réparations de charges
seules, les objets intacts, les objets sans durabilité et la génération du stock
marchand sont exclus. L’objet reste complètement réparé à son nouveau maximum.

## Télémétrie de session

La commande console `repair-costs-cap` affiche la configuration résolue,
`enabled`, l’unique `maximumGold`, la règle
d’usure et des compteurs séparés : évaluations par objet, ajustements, réduction
de devis, évaluations `Repair All`, plafonds globaux et réduction globale
additionnelle, réparations physiques réussies évaluées et points de durabilité
maximale perdus. Ces valeurs diagnostiquent les calculs client et serveur
répétés; elles ne prétendent pas être un historique persistant du wallet du
personnage.

## Preuves natives gouvernées

Le gate `npm.cmd run re:d2r32 -- status` est vert : image canonique SHA-256
`CC59119DC2A6C7D43D088098FC162EAFA4AE1299B2079126AEF43C1ACA914715`, image
d’analyse `673E8C0B2E89563E75525B24D137098EFD07B2DB4ED42ADEC56AA1ADDF0E63AB`,
index vérifié avec 105 850 fonctions et projet Ghidra persistant présent.

### Calcul commun par objet

- L’entrée publique `0x36F0B0` saute vers le corps `0x36F0C0`.
- Le prologue strict de 29 octets à `0x36F0C0` est unique :
  `4C 89 4C 24 20 44 89 44 24 18 48 89 54 24 10 48 89 4C 24 08 55 57 41 55 48 8D 6C 24 C9`.
- ABI x64 : `(player, item, difficulty, questFlags, vendorId,
  transactionType) -> int32`, avec `transactionType = 3` pour la réparation.
- Le devis client individuel appelle l’entrée à `0x10CD5F`; le serveur
  autoritaire recalcule le même coût à `0x540277` avant de vérifier l’or.
- Le handler individuel compare ensuite le coût au montant client, appelle le
  débit puis la réparation sans branche spéciale qui rejetterait zéro; le mode
  Normal gratuit conserve donc statiquement le chemin de réparation vanilla.
- Le hook applique l’unique plafond après le calcul vanilla; l’argument de
  difficulté reste transmis au
  calcul vanilla mais n’influence plus la politique du plugin.

### Total Repair All

- `0x375330` parcourt les emplacements `0..12`, appelle le calcul commun à
  `0x3754CB`, additionne les coûts et exécute un callback optionnel.
- Son prologue strict de 33 octets est unique :
  `41 54 41 55 41 56 41 57 48 81 EC 88 02 00 00 48 8B 05 82 5F 65 02 48 33 C4 48 89 84 24 50 02 00 00`.
- ABI x64 : `(game, player, vendorId, difficulty, questFlags, callback) ->
  int32`.
- Quatre callers sont prouvés : devis UI `0x10D270`, second chemin UI
  `0x241EC4`, premier passage serveur `0x53FF0E` et passage serveur avec callback
  `0x53FFDB`.
- Le second hook plafonne le total déjà ajusté; client et serveur partagent donc
  exactement le même résultat.

### Total nul et réparation effective

- Le handler serveur `0x53FE15` compare le total client avec son recalcul,
  vérifie l’or, débite, puis rappelle `0x375330` avec le callback de réparation.
- Vanilla saute le callback quand le total vaut zéro. Le site `0x53FF65` possède
  la signature unique
  `3B C7 0F 82 AD 00 00 00 85 FF 74 6F 48 8B 55 48`.
- Quand la politique est active, le plugin valide les 16 octets et remplace le
  seul déplacement `6F` par `21`. Un total nul produit par `maximumGold: 0`
  saute ainsi le débit mais rejoint le callback vanilla.

### Usure permanente après réparation

- La routine serveur commune `D2GAME_NPC_RepairItem` est prouvée à `0x53BB50`
  avec l’ABI x64 `(game, item, player) -> void` et un prologue strict unique de
  32 octets :
  `48 89 6C 24 18 48 89 74 24 20 57 48 83 EC 30 48 8B E9 49 8B F0 48 8B CA 48 8B FA E8 D0 EF E2 FF`.
- Le handler individuel l’appelle directement à `0x5402C7`. Le handler
  `Repair All` place à `0x53FFC1` le stub `0x5413F0`, lequel saute vers la même
  routine, comme callback par objet. Un seul hook serveur couvre donc les deux
  actions sans toucher aux devis UI.
- Le hook relève avant l’appel original la durabilité courante (stat 72), le
  maximum brut/sauvegardé via `STATLIST_GetUnitBaseStat 0x2F48C0` et le maximum
  effectif via `STATLIST_GetMaxDurabilityFromUnit 0x2F4B60`. Il laisse ensuite
  vanilla terminer, puis exige que la durabilité ait réellement atteint
  l’ancien maximum effectif. Une réparation de charges seule ou un objet intact
  ne peut donc pas déclencher l’usure.
- Le tirage utilise la seed native de l’objet à `0x34A1E0` et le RNG borné
  `0x153B00` sur 10 000 points de base. Les tests couvrent 0 %, la frontière
  exacte de 10 %, 100 %, les rolls hors plage et le plancher de durabilité 1.
- Après un succès, `0x43EB30` réduit d’un point le maximum brut/sauvegardé (stat
  73), puis `0x2F4B60` recalcule le maximum effectif et `0x43EB30` aligne la
  durabilité courante (stat 72) sur celui-ci. L’objet reste donc plein, la perte
  persiste et les modificateurs de durabilité en pourcentage restent cohérents.
- Les signatures des helpers `0x2F5020`, `0x2F48C0`, `0x2F4B60`, `0x34A1E0`,
  `0x153B00`, `0x48FDE0` et `0x43EB30` sont validées avant l’installation du
  hook. `Durability Resistance` pouvant déjà poser son saut à l’entrée de
  `0x2F48C0`, Repair Costs Cap signe les 50 octets intacts et uniques à partir de
  `0x2F48C5`; son appel traverse le hook existant, dont la garde par adresse de
  retour laisse cette lecture brute inchangée. Toute divergence fait refuser le
  plugin.

La référence D2MOO épinglée
`19019806df7f3e877fa105b05395d1e3597e2316` a servi uniquement d’ancre
sémantique. Aucune adresse, structure ni ABI 1.10f n’a été transposée.

## Audit de propriété et de collision

La référence officielle
`D2RL-Plugins@dc75b49ffbb67b887d7757ee00ee9a03bcde5d8a` est propre et épinglée;
sa recherche `repair` retourne zéro résultat. Les sites actuels de
`plugin-items` comprennent notamment `0x53C9F0`, `0x540EA0` et `0x541880` et ne
recouvrent aucune des quatre plages. `ForceLarzukSockets` hooke `0x375560`, après la
fin du prologue `0x375330..0x375350`. Depuis le port autorisé du 28 juillet,
`plugin-items.dll` est le propriétaire unique des trois hooks et du patch dans
le profil actif. La DLL autonome est conservée comme témoin source/runtime
désactivé jusqu'à la validation gameplay d'équivalence.

## Build, cold start et ZIP de test 1.4.0

- source : `data-BKVince/d2rloader/plugins/RepairCostsCap-src/`;
- configuration : `data-BKVince/BKVince.mpq/RepairCostsCap.json`;
- artefact : `data-BKVince/d2rloader/plugins/RepairCostsCap.dll`;
- description :
  `Controls NPC repair prices and optional permanent durability wear.`;
- trois exports D2RLoader v2, architecture x64, ressource manifeste,
  `NativeHooks` et métadonnées Windows 1.4.0 confirmés;
- tests Release : `repair-costs-cap-policy`, 1/1 vert;
- SHA-256 DLL source/runtime :
  `75FF5ADD222319CF6418DA8FEC39E453C57D922BBF4BF6C880668F051A3CAAB2`;
- SHA-256 JSON source/runtime :
  `E931EF486690FB55A202F738177C0A939329CBC4000A4C6C95F5F17F4E969CA1`;
- configuration du cold start autonome historique résolue avec `maximumGold=0`
  commun aux réparations individuelles et à `Repair All`, et usure à 10 %;
- hooks acceptés à `0x36F0C0`, `0x375330` et `0x53BB50`, puis chargement complet
  confirmant l’acceptation du patch conditionnel et des signatures helpers;
- patchsets : `scanned=20 applied=20 disabled=0 failed=0`;
- plugins : `scanned=22 active=22 disabled=0 rejected=0 failed=0`;
- démarrage D2R complet jusqu’à `24/24`, sans erreur fraîche du plugin.
- ZIP local de test :
  `analysis-cache/test-packages/RepairCostsCap-1.4.0-simple-cap-test.zip`;
- contenu inspecté : uniquement `RepairCostsCap.dll` et
  `RepairCostsCap.json` à la racine;
- SHA-256 ZIP :
  `C8F949435719EEC255D4B178D3FF39C9EFB39E9C41832593841DA88CFEA65831`.

## Intégration dans plugin-items — 28 juillet 2026

- propriétaire confirmé une seconde fois par Vincent : catégorie `items`, DLL
  `plugin-items.dll`, clé `items.repairCostsCap`;
- trois fichiers internes attribués à `RuffnecKk` sont compilés par le target
  existant : `repair-costs-cap.cpp`, `repair-costs-cap.h` et
  `repair-costs-cap-policy.h`;
- les métadonnées de `eezstreet-plugin-items` 2.0.1 et le crédit d'eezstreet
  restent inchangés; RuffnecKk est crédité dans le source, le log et la mission;
- le modèle `D2RPlugins.json` du pack expose la fonctionnalité désactivée par
  défaut avec `maximumGold=2147483647`, usure désactivée et `chance=0.0`; le
  cold start actif a temporairement exercé `maximumGold=100` avec
  usure à `1.0`, puis BKVince a été restauré à `enabled=false`,
  `durabilityWear.enabled=false`;
- le port est implanté comme module indépendant de `plugin-items.dll`, au
  checkpoint poussé `387dff8`; il ne partage avec `EthItemRules` ni bloc JSON,
  ni drapeau d’activation, ni logique fonctionnelle;
  l’ancien patch séparé, devenu non applicable sur cette tête, a été retiré de
  `data-BKVince` et archivé sous les preuves runtime;
- les cinq targets du PluginPack compilent en Release x64, les quatre CTest sont
  verts et le manifeste valide 66 sites uniques; `plugin-items.dll` porte le
  SHA-256
  `508D5A77F155D74C02B45C054F642A7CAA3B165287CE28694279561DD9F50EE0`;
- le JSON autonome témoin est lui aussi restauré au défaut vanilla et synchronisé
  source/runtime au SHA-256
  `2A81D81017F593267704270F2DC122E0CB5401CA2CB532A69CE86CB13C9E981C`;
- cold start conjoint actif : `EthItemRules` utilise son bloc JSON unique,
  `RepairCostsCap` installe les hooks `0x36F0C0`, `0x375330` et `0x53BB50`, les
  patches directs sont acceptés et `eezstreet-plugin-items` reste actif;
- cold start vanilla : aucun hook `EthItemRules` ni Repair Costs Cap n'est posé;
  les deux cas atteignent `24/24` avec
  `scanned=28 active=26 disabled=2 rejected=0 failed=0`;
- `RepairCostsCap.dll` n'est plus chargée dans le profil actif : elle est
  renommée `RepairCostsCap.dll.standalone-disabled`, son journal reste inchangé,
  et l'instance de validation est ensuite fermée;
- les cinq fichiers runtime temporairement touchés ont été restaurés à leur
  SHA-256 d’origine et aucun processus D2R n’est resté ouvert; rapport :
  `analysis-cache/pluginpack-foundation-runtime-validation/20260728-181150128/report.json`;
- le source, la DLL et le JSON autonomes versionnés restent temporairement des
  témoins de comparaison; ils seront retirés seulement après équivalence en jeu.

## Gates gameplay du port intégré encore ouverts

- `maximumGold=0` en Normal, Nightmare et Hell : réparation individuelle et
  `Repair All`, devis zéro, aucun gold retiré et objets réellement réparés;
- `maximumGold=5000` : plafond de 5 000 pour un objet réparé seul et pour le
  total de `Repair All`, avec concordance du devis, du débit et de l’état réparé;
- configuration absente, désactivée, bornes `0`/`1`, JSON invalide et repli
  global;
- objets intacts, endommagés, cassés, à charges, combinant charges et durabilité,
  éthérés et non réparables;
- usure à 0 %, 10 % et 100 %, réparation individuelle et `Repair All`, perte
  brute exactement égale à un point, plancher 1, affichage immédiat, sauvegarde
  et rechargement persistants; contrôler séparément un objet ordinaire et un
  objet portant un modificateur de durabilité en pourcentage;
- vendeurs des cinq actes, multiplicateurs de quête, or suffisant/insuffisant,
  souris/manette, solo, hôte/joiner, sauvegarde/rechargement et transitions;
- portée globale et neutralisation du doublon par identifiant.

## Prochain gate

Le défaut vanilla est maintenant restauré. Pour fermer l'équivalence du port,
activer temporairement `items.repairCostsCap`, régler `maximumGold=100`, activer
l'usure à `chance=1.0`, puis réparer un objet endommagé seul et plusieurs objets
avec `Repair All`. Chaque action doit coûter au plus 100 gold et chaque objet
physiquement réparé doit perdre exactement un point de durabilité maximale tout
en restant plein. Après sauvegarde/rechargement, confirmer la persistance, puis
remettre `enabled=false` avant la livraison finale.
