# Vendor Stock Refresh — D2R 3.2

## Statut et séquencement

- Statut : **planifié — valeur et faisabilité à démontrer, aucune implantation**.
- Cible éventuelle : `D2R.exe 3.2.92777` sous D2RLoader.
- Vincent a retenu l’Option A le 26 juillet 2026 : entreprendre l’étude après
  Transmogrify puis Readable Items / Clue Scrolls, sans déplacer la priorité
  courante.
- Vincent a confirmé la catégorie future `items`, la DLL propriétaire
  `plugin-items.dll` et la clé prévue `items.vendorStockRefresh`.
- Pendant une éventuelle incubation, la DLL autonome serait
  `VendorStockRefresh.dll`, hybride globale/mod-locale et attribuée exactement à
  `RuffnecKk`.

## Intention joueur

Ajouter à l’interface des marchands normaux un bouton qui régénère leur stock
sans devoir quitter puis rouvrir la boutique, sur le modèle de l’action de
rafraîchissement disponible dans l’écran de gamble.

Le message source fourni le 26 juillet 2026 présente explicitement cette
fonctionnalité comme une idée dont l’utilité personnelle reste incertaine. Le
besoin est donc enregistré, mais sa valeur n’est pas encore démontrée.

## Faits vérifiés

- La référence officielle épinglée
  `eezstreet/D2RL-Plugins@dc75b49ffbb67b887d7757ee00ee9a03bcde5d8a`
  est propre et vérifiée.
- Son `README.md:28` attribue à `plugin-items.dll` le filtre de gamble et le
  vendor overhaul.
- `src/plugin-items/items-main.cpp:40-41` répertorie
  `D2GAME_NPC_FillStoreInventory` et `D2GAME_NPC_GenerateStoreItem`; le hook du
  remplissage commence à `items-main.cpp:287`, tandis que
  `items-main.cpp:518-533` charge les politiques du vendor overhaul.
- `src/plugin-shared/include/plugin-shared.h:368-403` décrit le proxy du cache
  d’objets marchand, l’entrée d’état par NPC, son timestamp de rafraîchissement
  et la chaîne de vendeurs portée par `D2GameStrc`.
- Le PluginPack existant contrôle la génération et la qualité des objets de
  boutique. Les recherches gouvernées `vendor`, `gamble` et `store` n’ont pas
  identifié de bouton de rafraîchissement manuel du stock normal.
- `GambleScreenLimit` ne fournit pas ce comportement : sa mission gouverne la
  limite de remplissage du seul écran de gamble et son patch minimal écrit une
  plage distincte dans `D2GAME_STORES_FillGamble`.

Ces faits justifient le propriétaire `items`; ils ne prouvent pas encore qu’un
rafraîchissement manuel est sûr ni que le bouton de gamble est réutilisable.

## Hypothèses à tester

- Le serveur pourrait invalider puis reconstruire uniquement la partie aléatoire
  du stock du marchand actif sans fermer la transaction.
- Le bouton de gamble pourrait fournir un modèle d’interface et de navigation,
  mais pas nécessairement le protocole du stock normal.
- La chaîne de vendeurs et son timestamp pourraient gouverner le cycle de
  rafraîchissement recherché; leur rôle exact doit être prouvé sur le build
  92777 avant toute écriture mémoire ou tout hook.

## Inconnues et décisions produit

- Portée du stock : joueur, marchand, acte ou partie entière.
- Sort des objets permanents, consommables, objets déjà achetés, objets de quête
  et entrées propres au vendor overhaul.
- Rafraîchissement gratuit et illimité, délai minimal, coût en or ou autre
  politique anti-spam.
- Marchands admissibles et exclusion explicite du gamble, du trade entre joueurs
  et des interfaces de récompense.
- Placement du bouton, libellé localisé, focus manette et comportement aux
  différentes résolutions et échelles d’interface.
- Comportement lorsqu’un objet est survolé, sélectionné ou en cours d’achat au
  moment de la demande.

## Architecture exigée avant prototype

- L’hôte demeure l’unique autorité : le client transmet une intention de
  rafraîchir le marchand actuellement ouvert; il ne génère ni ne choisit aucun
  objet.
- Le serveur revalide le joueur, le NPC, la session de boutique et la politique
  avant d’invalider le stock, puis renvoie un état complet cohérent au client.
- Toute erreur conserve le stock précédent; aucune opération partielle ne doit
  supprimer, dupliquer ou désynchroniser un objet.
- Le plugin d’incubation reste autonome, hybride et sans `ModScopedOnly`. Il ne
  modifie, ne lie ni ne redistribue aucune DLL d’eezstreet.
- Une éventuelle configuration utilise uniquement `VendorStockRefresh.json`, en
  anglais, recherchée d’abord dans le mod actif puis dans le dossier global du
  jeu. Aucun TOML n’est autorisé.
- Après validation, la fonctionnalité doit rejoindre `plugin-items.dll` et
  l’unique `D2RPlugins.json` sous `items.vendorStockRefresh`; la DLL et le JSON
  autonomes seront alors supprimés.
- Description prévue : `Refreshes a vendor's stock with one click.`

## Gates observables

1. **Valeur joueur** — reproduire le renouvellement vanilla du stock normal,
   mesurer le temps et le nombre de transitions nécessaires pour dix
   rafraîchissements, puis comparer avec la cible d’un clic par rafraîchissement.
   Ne poursuivre que si Vincent juge le gain matériel.
2. **Workbench 92777** — exécuter le gate `npm run re:d2r32 -- status`, puis
   prouver le bouton de gamble, le cycle normal d’ouverture/fermeture de boutique,
   l’invalidation du cache, la reconstruction du stock et les échanges réseau.
3. **Audit PluginPack** — inventorier dans `plugin-items` les fonctions,
   structures, callbacks, RVA et plages de hooks concernés; attribuer un
   propriétaire unique à chaque site canonique.
4. **Contrat produit** — fermer les décisions de coût/délai, marchands admis,
   stock permanent et comportement pendant une transaction.
5. **Prototype conditionnel** — seulement après les quatre gates précédents et
   une demande explicite d’implantation, créer la DLL/JSON autonomes avec build,
   signatures et ABI strictement contrôlés.
6. **Matrice runtime** — marchands de tous les actes, stock aléatoire et
   permanent, achats avant/après rafraîchissement, or insuffisant, souris,
   manette, résolutions, solo, hôte/joiner, nouvelle partie et retour au menu;
   zéro perte, duplication, objet fantôme, crash ou désynchronisation.
7. **Distribution** — portées globale et mod-locale, repli de configuration,
   coexistence avec les cinq DLL eezstreet, Release x64, exports D2RLoader,
   hashes source/runtime et ZIP public strict DLL + JSON seulement.

## Prochain gate

Après Transmogrify puis Readable Items / Clue Scrolls, mesurer la friction réelle
du renouvellement de stock normal et franchir le statut du workbench 92777 avant
toute analyse native ou implantation.

## Frontière Git

Le lot de planification comprend uniquement cette mission, son entrée ROADMAP,
son workstream et le nœud généré du cadastre. Aucun code, JSON runtime, DLL,
archive ou changement de priorité courante n’appartient à ce lot.
