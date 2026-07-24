# Repair Costs Cap — D2R 3.2.92777

Dernière mise à jour : 24 juillet 2026

Statut : mission planifiée et mise en pause après son audit initial. L’instruction
ultérieure `Commence` de Vincent a promu Prevent Merc Death in Town comme priorité
courante. Aucun prototype, aucune configuration, aucune DLL et aucune archive
publique n’existent encore.

## Décisions confirmées

- Vincent a confirmé le 24 juillet 2026 le nom exact `Repair Costs Cap`.
- Vincent a d’abord retenu l’Option B pour rendre cette mission immédiatement
  prioritaire. Son instruction ultérieure `Commence` sur Prevent Merc Death in
  Town la remet en pause sans perdre son audit ni son prochain gate de preuve.
- La catégorie PluginPack future est `items`, avec `plugin-items.dll` comme DLL
  propriétaire et `items.repairCostsCap` comme clé prévue dans l’unique
  `D2RPlugins.json`.
- Pendant l’incubation, la fonctionnalité restera dans une DLL autonome hybride
  `RepairCostsCap.dll`, attribuée exactement à `RuffnecKk`, sans modifier, lier
  ni redistribuer une DLL d’eezstreet.
- La configuration autonome sera un JSON anglais compatible avec le lecteur du
  PluginPack, recherché d’abord dans le mod actif puis dans le dossier global du
  jeu. Aucun TOML ne sera créé.

## Résultat joueur attendu

Les réparations proposées par les PNJ peuvent être limitées à un montant maximal
configurable. La valeur `0` représente le mode gratuit. Le plafond s’applique au
coût calculé par le jeu sans rendre réparables les objets normalement exclus ni
modifier leur durabilité, leurs charges ou leur état autrement que par le chemin
de réparation vanilla.

Contrat prévu pour le premier prototype, à confirmer contre le chemin natif :

- plafond appliqué à chaque objet après le calcul vanilla et ses modificateurs;
- coût de `Repair All` égal à la somme des coûts individuels déjà plafonnés;
- configuration absente ou fonctionnalité désactivée : comportement vanilla;
- entier `maximumGold >= 0`, avec `0` pour gratuit;
- configuration présente mais invalide : refus explicite, sans valeur implicite.

La valeur initiale recommandée hors mode gratuit reste à choisir après mesure de
coûts réels représentatifs; aucune priorité produit n’est déduite d’un nombre
arbitraire.

## Friction observée et couverture actuelle

- La capture fournie le 24 juillet 2026 demande explicitement de rendre les
  réparations chez les PNJ gratuites ou plafonnées.
- `npc.txt` expose `rep mult`; le schéma 3.2 documente le calcul
  `[cost] * [rep mult] / 1024`, ensuite influencé par la durabilité et les
  charges. BKVince et la référence vanilla 3.2 possèdent chacun 17 lignes avec
  `rep mult = 128` et les multiplicateurs de quête présents à `1024`.
- Un multiplicateur nul pourrait fournir une réparation gratuite par les
  données, mais cette conséquence reste une hypothèse runtime à tester. Les
  tables n’exposent aucun plafond absolu configurable.
- L’Option B reste donc justifiée uniquement par le besoin vérifiable de proposer
  un vrai plafond réutilisable; la gratuité seule ne justifierait pas une DLL.

## Audit initial gouverné

- Le gate `npm.cmd run re:d2r32 -- status` est vert : image canonique
  SHA-256 `CC59119D…14715`, image d’analyse `673E8C0B…E63AB`, index SQLite
  vérifié avec 105 850 fonctions et projet Ghidra persistant présent. Aucun
  redump ni réimport n’a été effectué.
- Les recherches gouvernées `known repair` et `known vendor` ne retournent
  encore aucune identification stable; `known cost` ne fournit aucun chemin de
  réparation pertinent. Aucun RVA, hook ou ABI de réparation n’est donc déclaré.
- La référence officielle
  `D2RL-Plugins@dc75b49ffbb67b887d7757ee00ee9a03bcde5d8a` est propre, épinglée
  et vérifiée. Sa recherche `repair` retourne zéro résultat.
- `plugin-items.dll` est le propriétaire logique existant des fonctions objets,
  plafonds d’or et vendeurs (`README.md:28`). Son module comprend
  `items-main.cpp`, `items-private.h`, `plugin.rc` et `CMakeLists.txt`.
- Le module charge déjà `vendorOverhaul` depuis la section items
  (`src/plugin-items/items-main.cpp:518`) et hooke notamment le remplissage du
  stock vendeur à `0x53C9F0` ainsi que le gamble à `0x541880`. Ces chemins ne
  prouvent pas le calcul de réparation et ne seront ni réutilisés ni supposés
  compatibles sans audit de leurs plages.
- Les structures partagées exposent `D2ItemsTxt::dwCost` à `+0xE8`,
  `NpcItemCacheEntry`, `VendorChainEntry` et les champs vendeurs de `D2GameStrc`
  (`src/plugin-shared/include/plugin-shared.h:136,368-413`). Ces layouts sont
  des éléments à auditer, pas une preuve du calcul recherché.
- La référence sémantique D2MOO épinglée
  `@19019806df7f3e877fa105b05395d1e3597e2316` ne fournit pas de chemin de
  réparation exploitable par sa recherche initiale. Aucune adresse, structure
  ou ABI 1.10f ne sera transposée vers D2R 3.2.

## Hypothèses à tester

- Un calculateur commun pourrait produire le devis client et le prélèvement
  serveur; il peut aussi exister deux chemins distincts qui doivent appliquer le
  même plafond pour éviter un affichage ou un paiement divergent.
- `Repair All` pourrait sommer des coûts unitaires ou suivre une branche dédiée.
- Les réparations de durabilité, de charges de skills et les objets combinant les
  deux peuvent partager seulement une partie de la formule.
- Les multiplicateurs de quête et les minima natifs peuvent être appliqués avant
  ou après le meilleur point d’intervention.

## Plan de match

1. Partir du workbench vérifié et identifier le producteur du coût affiché pour
   une réparation individuelle, puis ses callers, son ABI et ses octets stricts.
2. Identifier séparément le chemin serveur qui vérifie l’or, le prélève et
   applique la réparation; prouver si `Repair All` réutilise le même calcul.
3. Cartographier les structures et paramètres réellement nécessaires, puis
   comparer chaque plage de hook aux hooks et patches actuels de `plugin-items`.
4. Écrire d’abord une politique pure et testable : disabled, gratuit, plafond
   inférieur ou supérieur au coût vanilla, bornes entières et erreur de config.
5. Incuber ensuite `RepairCostsCap.dll` avec signatures complètes, garde stricte
   du build 92777, trois exports D2RLoader v2 et description anglaise prévue :
   `Caps NPC repair costs at the configured amount.`
6. Valider techniquement, synchroniser par hashes puis exécuter la matrice
   fonctionnelle avant toute archive ou intégration à `plugin-items.dll`.

## Gates observables

- fonctions client et serveur bornées, rôles, callers, ABI, signatures et plage
  exacte de hook prouvés sur 92777;
- propriétaire unique de chaque hook et absence de collision PluginPack;
- configuration absente, désactivée, gratuite (`0`), plafonds `1`, inférieur,
  égal et supérieur au coût vanilla, ainsi que JSON invalide;
- réparation individuelle et `Repair All`, devis affiché et or réellement
  prélevé identiques;
- objets intacts, endommagés, cassés, à charges, combinant charges et durabilité,
  éthérés et non réparables;
- vendeurs des cinq actes, trois difficultés, avant/après multiplicateurs de
  quête, or suffisant et insuffisant;
- souris/manette, solo, hôte/joiner, sauvegarde/rechargement et transitions de
  partie sans désynchronisation, perte d’or incorrecte, duplication ni crash;
- portées globale et mod-locale, priorité du JSON, coexistence avec les cinq DLL
  eezstreet, `rejected=0`, `failed=0` et cold start complet;
- ZIP public limité à la DLL autonome et son JSON indispensable, avec contenu et
  SHA-256 inspectés seulement après validation runtime.

## Prochain gate

Identifier et prouver sur le build 92777 le calcul du devis de réparation côté
client et le chemin serveur qui vérifie/prélève l’or puis répare l’objet, y
compris la branche `Repair All`; borner fonctions, callers, ABI, signatures et
structures, puis auditer leurs plages contre `plugin-items` avant tout hook.
