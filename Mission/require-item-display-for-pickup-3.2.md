# Require Item Display for Pickup — D2R 3.2

Dernière mise à jour : 1 août 2026

## Décision produit

Vincent retient l’idée issue du catalogue TDE consistant à empêcher les
ramassages accidentels lorsque les étiquettes des objets au sol ne sont pas
affichées.

La destination confirmée est un futur merge dans `plugin-items.dll`, sous la
clé `items.requireItemDisplayForPickup`. L’Option A est retenue : accumuler ce
candidat avec MassID et le traiter plus tard dans un lot `plugin-items`
distinct, sans rouvrir le PR PluginPack déjà soumis et sans remplacer la
mission courante.

Pendant l’incubation, la fonctionnalité doit rester une DLL RuffnecKk autonome
hybride, installable globalement ou dans un mod, accompagnée d’un JSON autonome
compatible avec le futur bloc de `D2RPlugins.json`. Aucune DLL d’eezstreet ne
doit être modifiée, liée ou redistribuée par l’incubation.

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
- Le 1 août 2026, Vincent confirme successivement le merge PluginPack, la
  catégorie `items`, la DLL propriétaire `plugin-items.dll`, la clé
  `items.requireItemDisplayForPickup` et l’Option A de séquencement.
- MassID est déjà le premier candidat du prochain lot PluginPack et fournit un
  voisin logique pour mutualiser l’audit de `plugin-items.dll`, du manifeste,
  de la configuration et de la matrice d’intégration.
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

- [x] Destination : merge au PluginPack.
- [x] Propriétaire : `plugin-items.dll`.
- [x] Clé : `items.requireItemDisplayForPickup`.
- [x] Séquence : Option A, prochain lot `plugin-items` avec MassID.
- [ ] Vérifier le workbench 92777 et identifier le chemin client de sélection et
  de ramassage d’un item au sol.
- [ ] Prouver l’état natif des labels pour maintenir, basculer et affichage
  permanent, à la souris et à la manette.
- [ ] Auditer le propriétaire de chaque hook contre les cinq DLL PluginPack,
  PotionAutoPickup, l’autogold et les autres incubations.
- [ ] Concevoir un paquet ou une délégation qui conserve l’autorité serveur et
  refuse les requêtes invalides sans mutation locale seule.
- [ ] Définir le contrat JSON autonome compatible avec le futur bloc
  `items.requireItemDisplayForPickup`.
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
- coexistence avec les cinq DLL eezstreet et toutes les incubations du lot,
  sans plugin rejeté ou en échec.

## Prochain gate

Lorsque le prochain lot `plugin-items` est explicitement ouvert, exécuter le
gate `status` du workbench 92777, auditer la référence PluginPack épinglée et
prouver le chemin de ramassage manuel ainsi que l’état Show Items avant toute
implantation.
