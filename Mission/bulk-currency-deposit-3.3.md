# Bulk Currency Deposit — D2R 3.3

Dernière mise à jour : 28 août 2026

## Décision et état

Vincent approuve le 25 août 2026 une évolution légère de l'intégration UI,
prévue pour `1.1.0`. Le plugin ne livre aucun MPQ compagnon. Quand il est actif,
sa commande privée et ses sprites virtuels restent disponibles même si
`inventory_button_enabled = false`; ce réglage contrôle uniquement l'injection
automatique dans `PlayerInventory`. Un mod peut ainsi ajouter un bouton éditable
à son propre layout de stash et déclencher la même action avec
`PanelManager:OpenPanel:RuffnecKkBulkCurrencyDeposit`.

Vincent approuve ensuite la localisation automatique du tooltip dans cette même
version `1.1.0`. La DLL enregistre une table de chaînes virtuelle possédée par le
plugin, sans MPQ compagnon, et le layout référence la clé
`@RuffnecKkBulkCurrencyDepositTooltip`. D2RCore sélectionne la langue active du
client parmi les treize locales D2R; le mod actif peut remplacer la ressource
par le même chemin virtuel s'il souhaite posséder sa formulation.

Le paquet test `1.1.0` préparé le 25 août 2026 est
`RuffnecKk-Bulk-Currency-Deposit-1.1.0-test.zip` (148 626 octets, SHA-256
`450C841C50C73FEBA4981CF8D9218B7C20B93CDBBB0D17B21CAA66AA0DC8E3C1`).
À la demande explicite de Vincent, cette archive de test non publique contient
la DLL de 322 560 octets
(`211082DD75A8F783CF7FCB60991C5218CDF1AFF2BBC0777CBEC7C9FA69F920EB`)
le TOML de 1 364 octets
(`209CE9411B1CAB61F5174030413CA530F7A734454608682C3430FFF851A43F2A`)
et le README de 3 203 octets
(`A1327537ECFF13DA9117657000E65EA2243BF247C21A93E83AE58871A7988516`).
Les trois entrées archivées sont byte-identiques à leurs sources. Cette
exception d'emballage sert uniquement au testeur et ne modifie pas la politique
des archives publiques. Au moment de l'emballage, aucun cold start ni test
gameplay `1.1.0` n'était revendiqué.

Vincent confirme ensuite que ce paquet test fonctionne et retient la version
`1.1.0` pour la prochaine release de la RuffnecKk D2RLoader Suite. Ce retour
confirme le comportement exercé par le testeur, sans fournir à lui seul une
matrice documentée par build, portée et scénario.

Vincent approuve le même jour l'adaptation à la politique de compatibilité plus
flexible de la Suite. Le build-name et la version rapportés par D2RLoader sont
désormais uniquement journalisés. L'ancien fingerprint natif demeure la gate
autoritaire : seize signatures strictes, puis le contrôle de propriété
`UI_IsStateOpen` par Diagnostics. Une identité inconnue peut donc charger si
tous ces témoins correspondent exactement; cette correspondance ne constitue
jamais une qualification implicite du runtime observé.

Deux builds Release x64 indépendants de cette candidate produisent une DLL
byte-identique de 322 560 octets, SHA-256
`3826595512F442B4DB89D9F4DD7D5079E3EDB267B5FBFE152725312E0B6E2118`.
Les deux exécutions du test de politique passent, la version PE et produit reste
`1.1.0`, et les trois exports D2RLoader attendus sont les seuls exports. Le ZIP
test fonctionnel reste volontairement byte-identique : son retour utilisateur
porte donc sur la DLL antérieure à cette migration de politique. Aucun cold
start ni test gameplay 3.2/3.3 n'est revendiqué pour la nouvelle DLL canonique.

Vincent approuve ensuite le remplacement du mold par défaut de la même version
`1.1.0` avec les sprites actualisés reçus le 25 août 2026. Le sprite normal
conserve le contrat `54 x 141`. Le low-end contient bien `27 x 71` pixels, mais
son header reçu déclare par erreur une hauteur de 70; l'intégration corrige
uniquement ce champ à 71 afin de conserver la dernière rangée opaque et un
fichier structurellement cohérent. Le layout, le sprite du bouton d'action, le
TOML et le comportement natif restent inchangés.

Le mold normal intégré mesure 30 496 octets, SHA-256
`084679775E964EB7B87FC0B1E252BD5FAB09554E898E1D930C121B8AB29936F4`.
Le low-end corrigé mesure 7 708 octets, SHA-256
`2B0A792CC45DE8610EDCD260A7A63073A3076D3C8FDD2F3D425A62A3A2346815`.
Deux builds Release x64 indépendants produisent la même DLL de 322 560 octets,
SHA-256
`F288A6E8CBFFF934767D739F3A531F704A3BDC4B282D2EAB37372E56A65E2C92`.
Le test de politique passe dans les deux builds et vérifie maintenant le magic,
les dimensions, le nombre de frames et la taille byte-exacte des deux molds.
La DLL conserve la version `1.1.0`, l'auteur `RuffnecKk` et exactement les trois
exports D2RLoader attendus. Le ZIP test reste byte-identique, SHA-256
`450C841C50C73FEBA4981CF8D9218B7C20B93CDBBB0D17B21CAA66AA0DC8E3C1`;
aucun témoin visuel runtime n'est encore revendiqué pour le nouveau mold.

Vincent choisit ensuite la seconde paire de sprites reçue le 25 août 2026 comme
nouveau mold par défaut de `1.1.0`. Elle conserve les mêmes contrats de layout
`54 x 141` et low-end `27 x 71`. Comme le nouvel export low-end répète le header
erroné à 70 malgré ses 71 rangées stockées et sa dernière rangée opaque,
l'intégration applique de nouveau la correction minimale du champ hauteur. Les
tests structurels existants demeurent autoritaires; aucun changement de code
natif, de layout, de TOML ou de version n'est approuvé avec cette substitution.

La seconde variante intégrée mesure 30 496 octets pour le mold normal,
SHA-256
`1D538B74295588757E5DA0C1417F29A147CB7F44B80A041504806D35DBA339DD`,
et 7 708 octets pour le low-end corrigé, SHA-256
`39DFF56F0BF7CE2F51E9C277C1836F1406491718074AF917DDFED79E268029A3`.
Deux builds Release x64 indépendants produisent la même DLL de 322 560 octets,
SHA-256
`96C2859CD8EF80B3A383F2420C1DD56407A8E4BCBE0E1C099CE237B7B1F2BBDB`.
Le test de politique passe dans les deux builds; les ressources `1101` et
`1102` extraites de la DLL sont byte-identiques aux deux sources intégrées. La
version PE reste `1.1.0`, l'auteur demeure `RuffnecKk` et seuls les trois exports
D2RLoader attendus sont présents. Le ZIP test antérieur reste volontairement
byte-identique; aucun témoin visuel runtime n'est revendiqué pour cette seconde
variante.

La DLL canonique de cette seconde variante est ensuite déployée le 25 août 2026
dans le scope mod-local BKVince sous D2R `3.3.93847`. La source et la copie
runtime ont le même SHA-256
`96C2859CD8EF80B3A383F2420C1DD56407A8E4BCBE0E1C099CE237B7B1F2BBDB`.
Le TOML runtime personnalisé demeure byte-exactement inchangé, SHA-256
`137C2C53C234BFD773877F6956DD3737AAA479C3B06694E128161E18A2E7D316`.
Un cold start de la pile active avec `-txt -mod BKVince` charge 35 plugins et
18 memory patches, dont les cinq plugins eezstreet, puis atteint
`D2R startup complete`. Le log frais de Bulk Currency Deposit observe le
build-name `93847` et la version diagnostique `3.3.0`, accepte l'empreinte native,
confirme `buttonResources=ready` et injecte le bouton Inventory à `3,813` en
portée mod-locale. Aucun plugin n'est rejeté ou refusé. L'assertion TACT connue
est capturée et ignorée par le Loader après le startup complet; elle reste
externe à Currency. Ce cold start ne remplace pas le témoin visuel en jeu du
nouveau mold ni la qualification séparée sous D2R `3.2.92777`.

Le 28 août 2026, les deux mêmes molds sont intégrés localement au MPQ compagnon
de Dimentio Charm Inventory `0.19.0` afin d'aligner visuellement les boutons des
deux plugins. Le candidat de 9 587 401 octets, SHA-256
`A463A548E5DEF0913AB35EF9B524B1F941D57C516C651A73029DD9F754DF3F62`,
diffère du MPQ original dans exactement deux de ses dix-huit membres :
`button_mold.sprite` vaut
`1D538B74295588757E5DA0C1417F29A147CB7F44B80A041504806D35DBA339DD`
et `button_mold.lowend.sprite` vaut
`39DFF56F0BF7CE2F51E9C277C1836F1406491718074AF917DDFED79E268029A3`.
La DLL Dimentio reste byte-identique, SHA-256
`CFC57AA34780E056987926CDA9AEBAD7476F7491464FFBFC9E816ECBE4309A1B`.
Un cold start de la pile complète atteint `D2R startup complete` avec 36
plugins chargés; l'échec Revive Overhaul observé est antérieur au changement et
hors de ce lot. Vincent valide ensuite en jeu le rendu comme très propre. Le
MPQ tiers reconstruit demeure local et n'est pas redistribué; les deux sources
autorisées et leurs hashes suffisent à reproduire précisément ce delta visuel.

Vincent a approuvé l'implantation puis le rebranding complet le 20 août 2026.
Le produit est une DLL autonome de la RuffnecKk D2RLoader Suite, hybride
globale/mod-locale et attribuée exactement à `RuffnecKk`.

- Nom public : **Bulk Currency Deposit**.
- DLL distribuée : `d2rl-ruffneckk-bulk-currency-deposit.dll`.
- Configuration : `ruffneckk-bulk-currency-deposit.toml`.
- Identifiant et commande : `bulk-currency-deposit`.
- Version publique préparée : `1.0.0`.
- Intégration publique préparée : RuffnecKk D2RLoader Suite `1.2.0`, désormais
  composée de 17 plugins.
- Description DLL exacte : `Auto transfers all your stackable currency items
  into their respective stash slots.`
- Interaction : raccourci configurable et bouton Inventory facultatif, désactivé
  par défaut.

Vincent approuve le 20 août 2026 le lot de finition `1.0.0-rc.2`, confirme que
la fonctionnalité est prête pour une release publique `1.0.0`, puis autorise sa
préparation par `GO`. Le raccourci est une action du menu Controls natif de
D2RLoader, sous la catégorie `RuffnecKk Suite`, avec le binding primaire
`SHIFT+D`, aucun binding secondaire et l'identifiant logique stable
`bulk-currency-deposit`. Le TOML ne possède plus de hotkey; effacer les
deux bindings dans le Loader désactive le raccourci. Le bouton Inventory reste
facultatif et désactivé par défaut. Son atlas doit conserver l'ordre natif des
quatre frames normal, disabled, pressed et hovered, avec `hoveredFrame = 3`,
au lieu de mirrorer naïvement toute la bande fournie par Charm Inventory. Dans
le même lot, Floating Damage migre son toggle
`SHIFT+Z` vers le menu natif et Transmute Hotkey quitte la Suite parce que
D2RLoader expose déjà `d2rloader/cube_transmute`.

Vincent a aussi approuvé l'ajout de cette mission à l'Incrément 7 comme court
lot borné. RuffnecKk MapSense reste la priorité du projet avant et après ce lot.

## Besoin joueur

Une seule action doit transférer les runes, gems et monnaies empilables depuis
l'inventaire vers les emplacements choisis par le système Advanced Stash du mod
actif. Le plugin ne doit ni maintenir une table d'items BKVince, ni connaître le
nom des onglets, ni imposer un layout de stash.

Le bouton demandé ensuite doit exécuter la même action que le hotkey sans
devenir obligatoire. Le plugin ne doit imposer aucun MPQ compagnon : un moddeur
qui veut rattacher le bouton au stash peut ajouter un simple enfant JSON dans
son layout loose-file, avec des coordonnées qu'il possède lui-même.

## Faits vérifiés

- `AdvancedStashStackable` est une colonne native de `misc.txt` sous D2R 3.3.
- La vraie condition runtime est plus forte que la seule colonne TXT :
  `UI_CanDepositToAdvancedStash` à `0x15A0B0` vérifie qu'une destination est
  enregistrée pour la classe d'item dans le registre Advanced Stash actif.
- Le transfert natif résout le proxy de destination à `0x46DA50`, puis appelle
  `CLIENT_TransferItemToInventoryPage` à `0x15F8B0` vers la page `4`.
- Les handlers Ctrl-clic natifs appellent la chaîne UI
  `0x2AAAA0 -> 0x15F8B0 -> 0x1A0780`. `0x15F8B0` calcule le placement et émet
  le paquet client normal `0x54`; le serveur reste l'autorité de la mutation.
  Bulk Currency Deposit reproduit donc cette demande cliente sur
  `ThreadService::runOnUiThread`, et non une mutation ItemService sur
  `runOnGameThread`, qui serait indisponible chez un joiner TCP/IP.
- L'inventaire joueur est parcouru avec `INVENTORY_GetFirstItem` et
  `INVENTORY_GetNextItem`; l'appartenance courante est revalidée avec
  `INVENTORY_GetParentInventory`.
- Le corpus gouverné D2R 3.3.93847, son index et ses signatures passent
  `npm run re:d2r33 -- status` le 20 août 2026.
- Le SDK v3 épinglé au commit
  `4933e2c42cb2592958cd0df3b6dc5003102252d1` fournit les services Input,
  Thread, Panel, Resource et SharedEvent nécessaires au binding, au relais UI
  et au bouton sans MPQ compagnon.
- Remote Stash 2.0.x prouve le modèle de child layout `PlayerInventory`, des
  ressources virtuelles embarquées et du message UI privé.
- Remote Stash possède déjà le hook natif de configuration de l'inventaire à
  `0x22BA70`. Bulk Currency Deposit ne prend pas ce hook : il utilise des
  coordonnées TOML fixes et modifiables, ce qui évite un second propriétaire.
- La validation de `UI_IsStateOpen` à `0xCE500` accepte soit la signature
  vanilla, soit exactement un inline hook suivi par Diagnostics et possédé par
  `ruffneckk-remote-stash`. Toute autre modification, pluralité de propriétaires
  ou absence de preuve Diagnostics est refusée fail-closed.
- Le visuel du bouton Charm Inventory fourni par Dimentio est réutilisable ici
  avec sa permission, selon l'autorisation transmise par Vincent.

## Architecture retenue

1. Le raccourci est enregistré auprès de l'Input Service SDK v3 avec le binding
   par défaut `SHIFT+D`. Son callback ne dépose rien directement : il publie
   seulement une demande atomique pour le worker de cadence.
2. Quand le plugin est actif, il interroge SharedEvent et Resource, enregistre
   ses quatre sprites virtuels et conserve sa commande privée disponible pour
   un bouton fourni par le mod. Lorsque `inventory_button_enabled = true`, il
   interroge en plus Panel et enregistre son propre enfant `PlayerInventory`.
3. Le bouton émet
   `PanelManager:OpenPanel:RuffnecKkBulkCurrencyDeposit`. Un listener privé de
   priorité 10000 reconnaît exactement ce triplet, publie la même demande
   atomique que l'action Controls et consomme le message avant le PanelManager
   normal. Il ne parcourt ni ne transfère aucun item dans le callback SharedEvent.
4. Le worker de cadence remet ensuite la demande commune au moteur de dépôt sur
   le thread UI. Une deuxième demande est refusée pendant un batch actif.
5. Le moteur exige le stash ouvert, un joueur local et son inventaire valide.
   Il sélectionne seulement les items de page principale acceptés par le
   registre Advanced Stash natif et par les filtres TOML facultatifs.
6. La queue conserve GUID et item code, jamais un pointeur vivant. Chaque étape
   retrouve et revalide l'item et la destination avant le transfert.
7. Un seul item est transféré par étape UI avec un délai par défaut de 100 ms.
   Le worker de cadence délègue chaque étape à `ThreadService::runOnUiThread`;
   il n'installe aucun hook Windows ni dispatcher UI privé. Fermer le stash ou
   perdre un état requis annule le reste du batch.
8. Le plugin n'édite aucun save, quantité, table TXT, tab de stash ou layout du
   mod actif. Il ne possède aucun hook natif d'interface et n'exige pas Remote
   Stash.

## Contrat TOML

La configuration et tous ses commentaires sont en anglais.

- `deposit.enabled` : master switch, défaut `true`.
- `deposit.inventory_button_enabled` : injecte le bouton possédé par le plugin
  dans `PlayerInventory`, défaut `false`. Avec `false`, la commande privée et
  les sprites restent disponibles pour un bouton externe fourni par le mod.
- `deposit.item_delay_ms` : `50..1000`, défaut testé `100`. Les grandes valeurs
  ralentissent seulement le lot; le joueur devrait conserver `100`.
- `deposit.include_item_codes` : allow-list exacte facultative.
- `deposit.exclude_item_codes` : deny-list appliquée ensuite.
- `button.x` / `button.y` : coordonnées Inventory, défaut `3,813`, directement
  sous le bouton Charm Inventory standard de Dimentio à `3,672`.

Il n'existe ni option `consume`, ni option TOML de hotkey. Le binding appartient
à D2RLoader et persiste uniquement dans son fichier `input-bindings.toml`, que
le plugin et son archive ne modifient jamais. Une requête acceptée retourne
`Handled`. Un TOML présent mais invalide refuse le chargement avant toute
installation d'input, listener, ressource ou action de dépôt.

La recherche suit le support config du mod actif, le scope du plugin, puis le
fallback global. Si aucun fichier n'existe, le plugin tente de matérialiser le
TOML RuffnecKk dans son scope de configuration, sinon utilise les mêmes défauts
anglais embarqués.

## Compatibilité et autorité

- Le mod actif reste l'unique autorité de ses monnaies, de son registre
  Advanced Stash et de ses slots. Les items custom fonctionnent sans ajout au
  plugin lorsqu'ils sont enregistrés correctement par le mod.
- Les filtres TOML peuvent seulement réduire l'ensemble natif; ils ne peuvent
  pas forcer un item sans destination.
- Remote Stash n'est ni modifié, ni appelé, ni requis. Sa version 2.0.x sert de
  référence technique SDK et son propriétaire de hook est reconnu strictement
  par Diagnostics lorsqu'il est présent. Les deux boutons ont un emplacement et
  une action privée distincts; aucune ABI Remote Stash n'est consommée.
- Le plugin ne modifie, lie ou redistribue aucune DLL eezstreet.
- Le bouton est `KeyboardMouseOnly` pour la première release. La manette ne
  reçoit ni bouton artificiel ni route concurrente.
- Le raccourci natif est clavier seulement, avec zéro ou un modificateur selon
  le contrat de l'Input Service SDK v3. Les boutons souris ne sont pas proposés.

## Preuves gameplay déjà fournies par Vincent

Vincent a validé le moteur de dépôt antérieur au rebranding dans BKVince :

- runes, gems et monnaies custom routées automatiquement dans deux tabs custom;
- items custom couverts par le registre Advanced Stash sans table interne au
  plugin;
- items ordinaires non stackables laissés dans l'inventaire;
- Remote Stash utilisé pendant la matrice;
- sauvegarde et rechargement après dépôt;
- interruption d'un dépôt;
- aucune perte, duplication ou erreur de placement observée.

Ces preuves démontrent le design général dans un mod fortement personnalisé.
Elles ne sont pas transformées en claim universel pour tous les mods, tous les
plugins optionnels ou tous les rôles multijoueur.

## Fichiers gouvernés

- `Mission/bulk-currency-deposit-3.3.md`
- `addons/BulkCurrencyDeposit/src/`
- `addons/BulkCurrencyDeposit/assets/`
- `addons/BulkCurrencyDeposit/ruffneckk-bulk-currency-deposit.toml`
- `addons/BulkCurrencyDeposit/README.md`
- `addons/BulkCurrencyDeposit/d2rl-ruffneckk-bulk-currency-deposit.dll`
- `addons/BulkCurrencyDeposit/RuffnecKk-Bulk-Currency-Deposit-1.0.0.zip`
- `addons/BulkCurrencyDeposit/RuffnecKk-Bulk-Currency-Deposit-1.1.0-test.zip`
- témoins runtime mod-locaux du même nom sous `data-BKVince/d2rloader/`.

Le README minimal reste uniquement dans le dépôt pour les crédits et n'est pas
un asset de release. Les ZIP publics contiennent uniquement la DLL et sa
configuration. D2MOO y demeure crédité pour les connaissances natives et
Dimentio pour les sprites réutilisés avec permission.

## Gates de la release publique 1.0.0

1. **Preuve native : fermée.** Fonctions, signatures, call shape, chaîne UI,
   paquet `0x54` et corpus 3.3.93847 sont gouvernés. L'exception UI par rapport
   au conseil ItemService générique est explicitement documentée.
2. **Tests et reproductibilité : fermés.** Deux builds Release x64 propres de
   la Suite, avec warnings-as-errors, produisent 17 DLL byte-identiques et passent
   chacun `CTest 25/25`. Les politiques couvrent InputService `SHIFT+D`,
   ThreadService UI-only, TOML strict, callbacks, Diagnostics, assets, ownership
   natif et absence de hook Windows.
3. **Build de livraison : fermé.** La source principale vaut SHA-256
   `035F829F478A1CA1FE0C11222703BC14AE40512BF94B287AD088981AC559D359`.
   La DLL de 321 024 octets vaut
   `52B54990773485F98F04F7EC50711F18075D7B5A87785A602E30C400232A2EC5`,
   porte `1.0.0`, expose exactement les trois exports D2RLoader et embarque le
   manifeste API v3, le TOML et les quatre sprites.
4. **Distribution : fermée.** Deux générations produisent 38/38 assets GitHub
   byte-identiques : 17 ZIPs de plugins, 19 patches et 2 bundles. Aucun README
   n'est inclus. Le ZIP individuel Suite vaut
   `F97E4D807FFAA03C5F5765AA44A4E53F86B12C0EB0CD9C2B14C914D8AE350F9B`;
   le ZIP autonome à racine plate vaut
   `B39A9F519504EE150583BF9B1135D762A0B664C90E1CC53B8D035E32DDAD615B`.
   Le TOML public vaut
   `747DEA10790FA5F403B81DE74DA9693ECB72C41235FB5E764DADEF90042FBE2A`;
   la variante BKVince avec bouton activé vaut
   `137C2C53C234BFD773877F6956DD3737AAA479C3B06694E128161E18A2E7D316`.
5. **Portées runtime : fermées pour le chargement.** Le même hash final charge
   sous D2R 3.3.93847 en mod-local BKVince avec bouton activé à `3,813`, puis en
   global seul avec le bouton public désactivé. La copie globale de test a été
   retirée et la configuration mod-locale restaurée byte-exactement.
6. **Gameplay et décision produit : fermés pour la publication.** Vincent a
   validé le routing natif et custom multi-tabs, hotkey, bouton, filtres,
   interruption et save/reload sans perte ni duplication, puis a explicitement
   demandé la release publique. Le dernier témoin RC.2 dépose 11/11 candidats;
   le rebranding `1.0.0` change les identifiants et l'emballage, sans modifier
   le moteur natif accepté.
7. **Coexistence de chargement : fermée.** Le cold start restauré charge Bulk Currency Deposit
   avec MapSense, Remote Stash, Charm Inventory, les cinq DLL eezstreet et les
   plugins yinyin actifs : 29 plugins, 17 patches et `D2R startup complete`.
   L'assertion TACT connue et ignorée du Loader reste externe à Currency.
8. **Limites non revendiquées.** Les rôles TCP/IP host et joiner n'ont pas été
   exercés séparément. Ils restent un suivi post-release facultatif et ne sont
   pas déclarés réussis.
9. **Dépôt et publication : release publique fermée.** Le
   cadastre régénéré, les deux ZIP Bulk et les validateurs de la Suite sont
   valides. Le `npm run verify` global demeure non propre sur les 716 fichiers
   préexistants non assignés à un workstream; le validateur ZIP global relève
   également huit anciennes archives non suivies et hors politique. Bulk
   Currency Deposit possède bien ses 17 fichiers assignés, aucun overlap et
   deux archives conformes sans README. Le commit Suite `15c2808` est poussé
   sur `main`; le tag `v1.2.0` pointe sur ce même commit. La release GitHub
   publique contient exactement les 38 assets validés, avec les mêmes noms et
   tailles que le lot local et aucun README :
   <https://github.com/RuffDood/RuffnecKk-D2RLoader-Suite/releases/tag/v1.2.0>.

## Rollback

Retirer la DLL et son TOML du scope choisi, puis redémarrer D2R. Le plugin ne
crée aucune migration, aucun format de sauvegarde et aucune donnée persistante.
Les artefacts pré-rebranding ont été déplacés de façon récupérable sous
`analysis-cache/bulk-currency-deposit-retired-artifacts-20260820/`, et les
anciens fichiers runtime sous
`analysis-cache/runtime-deployments/20260820-bulk-currency-deposit-1.0.0/`;
ils ne doivent pas cohabiter au runtime avec le binaire renommé.

## Prochain gate

Bulk Currency Deposit `1.0.0` est publié comme 17e plugin du catalogue et des
bundles publics de la Suite `1.2.0`. Aucun gate requis ne reste ouvert. Les
rôles TCP/IP host/joiner demeurent un suivi post-release facultatif non
revendiqué; MapSense redevient la priorité.
