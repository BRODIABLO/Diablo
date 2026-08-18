# Require Item Display for Pickup — D2R 3.2

Dernière mise à jour : 18 août 2026

## Décision produit

Vincent retient l’idée issue du catalogue TDE consistant à empêcher les
ramassages accidentels lorsque les étiquettes des objets au sol ne sont pas
affichées.

La destination PluginPack choisie le 1 août 2026 est supersédée par la règle
Suite confirmée par Vincent le 18 août 2026. La fonctionnalité devient un
plugin RuffnecKk autonome permanent, membre de la RuffnecKk D2RLoader Suite;
aucune catégorie, DLL propriétaire, clé de merge ou intégration future dans
une DLL d’eezstreet ne reste planifiée.

La future DLL doit rester hybride, installable globalement ou dans un mod,
avec sa propre version, son archive et une configuration JSON ou TOML
indépendante de `D2RPlugins.json`. Le format sera choisi selon la convivialité
réelle du contrat. Aucune DLL d’eezstreet ne doit être modifiée, liée ou
redistribuée.

Description joueur retenue :

> Requires item labels to be visible before ground items can be picked up.

## Contrat fonctionnel à fermer

- Le clic destiné au déplacement ou au combat ne ramasse aucun objet lorsque
  les étiquettes d’objets sont masquées.
- Le ramassage manuel reste possible lorsque l’état natif « Show Items » est
  actif, que cet état provienne du mode maintenir, basculer ou affichage
  permanent.
- Les ramassages automatiques déjà gouvernés, notamment l’or et
  PotionAutoPickup, ne doivent pas être bloqués par une règle destinée au clic
  manuel.
- Les objets de quête, interactions obligatoires et cas d’inventaire plein
  doivent conserver une issue explicite et testée.
- Le comportement manette doit être défini depuis les événements natifs réels;
  aucune équivalence avec la souris n’est présumée.
- Toute mutation d’inventaire demeure autoritaire côté serveur en solo, chez
  l’hôte et chez le joiner.

## Faits vérifiés

- Le candidat du catalogue est `alt-required-ground-pickup`, classé deuxième
  parmi les inspirations natives/hybrides.
- Le 1 août 2026, Vincent avait confirmé successivement le merge PluginPack, la
  catégorie `items`, la DLL propriétaire `plugin-items.dll`, la clé
  `items.requireItemDisplayForPickup` et l’Option A de séquencement; cette
  destination est conservée comme historique et annulée prospectivement par la
  décision Suite du 18 août 2026.
- MassID demeure un voisin logique pour mutualiser l’audit des hooks, contrats
  inter-DLL et matrices de coexistence de la Suite sans fusionner les deux DLL.
- Aucun hook, RVA, ABI ou comportement manette propre à cette fonctionnalité
  n’est encore gouverné pour D2R 3.2.92777.

## Hypothèses à tester

- Le chemin de clic sur une unité item au sol expose un point de décision avant
  l’envoi de la commande de ramassage.
- Un état natif fiable permet de distinguer les labels visibles des labels
  masqués pour les trois politiques d’affichage.
- Le chemin manette peut être filtré sans bloquer la sélection de loot ni les
  interactions de proximité.
- Les automatismes existants utilisent des chemins distincts ou peuvent être
  explicitement exemptés sans collision de hook.

## Gates avant implantation

- [x] Destination : plugin autonome permanent RuffnecKk Suite.
- [x] Ancienne destination `plugin-items.dll` et clé
  `items.requireItemDisplayForPickup` annulées pour toute implantation future.
- [ ] Vérifier le workbench 92777 et identifier le chemin client de sélection et
  de ramassage d’un item au sol.
- [ ] Prouver l’état natif des labels pour maintenir, basculer et affichage
  permanent, à la souris et à la manette.
- [ ] Auditer le propriétaire de chaque hook contre tous les composants actifs
  de la Suite, les cinq DLL PluginPack, PotionAutoPickup, l’autogold et les
  autres incubations.
- [ ] Concevoir un paquet ou une délégation qui conserve l’autorité serveur et
  refuse les requêtes invalides sans mutation locale seule.
- [ ] Choisir JSON ou TOML selon le contrat réel et définir une configuration
  autonome indépendante de `D2RPlugins.json`.
- [ ] Obtenir une autorisation d’implantation distincte avant toute création de
  source, configuration, DLL ou archive.

## Matrice de validation future

- souris : labels masqués, maintenus, basculés et permanents;
- manette : ciblage, ramassage, proximité et changement de périphérique;
- catégories : équipement, consommables, or et objets de quête;
- inventaire disponible ou plein, objet valide ou devenu indisponible;
- PotionAutoPickup et autogold activés ou désactivés;
- solo, hôte et joiner, sauvegarde puis relecture;
- portées globale et mod-locale, configuration absente, valide et invalide;
- coexistence avec tous les composants actifs de la Suite, les cinq DLL
  eezstreet et toutes les incubations, sans plugin rejeté ou en échec.

## Prochain gate

Lorsque cette mission est explicitement reprise, exécuter le gate `status` du
workbench 92777, relever la baseline gouvernée de la Suite, auditer la référence
PluginPack épinglée et prouver le chemin de ramassage manuel ainsi que l’état
Show Items avant toute implantation.
