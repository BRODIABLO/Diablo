# Prevent Merc Death in Town — D2R 3.2.92777

Dernière mise à jour : 24 juillet 2026

Statut : mission planifiée selon l’Option B; elle démarrera après le prochain
gate de preuve de RemoteStash. Aucun prototype, aucune configuration et aucune
archive publique n’existent.

## Décisions confirmées

- Vincent a confirmé le 24 juillet 2026 que la mort d’un mercenaire en ville
  sous poison, Open Wounds ou autre dégénérescence persistante est un fait, et
  non un bug hypothétique ni une théorie à valider.
- Le nom exact de la mission est `Prevent Merc Death in Town`.
- Vincent a retenu l’Option B : RemoteStash reste la priorité courante jusqu’à
  son prochain gate de preuve, puis cette mission devient le prochain chantier.
- Vincent confirme la catégorie PluginPack `misc` le 24 juillet 2026, avec
  `plugin-misc.dll` comme DLL propriétaire future et
  `misc.preventMercDeathInTown` comme clé prévue.

## Résultat joueur attendu

Un mercenaire vivant qui entre ou se trouve dans une zone de ville ne peut pas
mourir à cause d’un dégât persistant encore actif. La protection ne doit pas
guérir arbitrairement le mercenaire, altérer le joueur, protéger les autres
serviteurs ou rester active après la sortie de ville.

## Fait établi et périmètre de preuve

- Fait produit : un mercenaire peut mourir en ville lorsqu’un état de poison,
  Open Wounds ou une autre dégénérescence continue d’infliger des dégâts.
- La phase de reproduction ne sert pas à décider si le fait existe; elle sert à
  construire des témoins déterministes, mesurer chaque famille de dégâts et
  produire une matrice de non-régression.
- Les mécanismes natifs exacts du build 92777, le chemin autoritaire du tick de
  dégâts, la détection de ville, les structures, l’ABI et le point
  d’intervention restent à prouver avant tout hook.

## Plan de match

1. Capturer des témoins déterministes pour poison, Open Wounds et les autres
   dégâts persistants pertinents, avec PV, zone et transition vers la ville.
2. Comparer le chemin du mercenaire à la protection déjà observée pour le
   joueur, sans supposer qu’ils partagent le même appel natif.
3. Exécuter le gate du workbench 92777, identifier le chemin serveur qui applique
   le tick létal et prouver fonction, callers, ABI, signatures et préconditions.
4. Auditer `plugin-misc` au commit officiel épinglé et attribuer un propriétaire
   unique à chaque hook ou structure partagée avant de coder.
5. Prototyper une protection minimale, autoritaire et strictement bornée au
   mercenaire vivant du joueur dans une zone de ville. La politique initiale à
   évaluer est un plancher de survie à 1 PV, sans guérison automatique.
6. Confirmer que la protection cesse hors ville et qu’elle ne touche ni joueur,
   mercenaire déjà mort, monstres, invocations, familiers ou autres unités.
7. Valider les transitions par portail et waypoint, les changements d’acte,
   solo, hôte/joiner, sauvegarde/rechargement et retour au combat.
8. Après validation fonctionnelle, appliquer les gates hybride global/mod-local,
   coexistence PluginPack, cold start, logs, hashes et archive publique stricte.

## Incubation envisagée

- Description anglaise prévue :
  `Prevents mercenaries from dying to lingering damage while in town.`
- La catégorie `misc` étant confirmée, conserver pendant l’incubation une DLL
  autonome hybride attribuée exactement à `RuffnecKk`, sans `ModScopedOnly`.
- Utiliser uniquement un JSON autonome compatible PluginPack si une option
  joueur réelle est justifiée; ne créer aucun TOML.
- Après un merge futur, intégrer la fonctionnalité à `plugin-misc.dll`, déplacer
  ses options sous `misc.preventMercDeathInTown`, puis supprimer la DLL et le
  JSON autonomes.

## Gates observables

- propriétaire futur `misc` / `plugin-misc.dll` confirmé explicitement par Vincent;
- workbench 92777 et référence PluginPack épinglée vérifiés;
- poison, Open Wounds et autres dégâts persistants couverts par des témoins;
- chemin autoritaire, détection de ville, ABI et signatures strictes prouvés;
- mercenaire maintenu vivant en ville sans guérison ou immunité hors périmètre;
- sortie de ville, portail, waypoint et changement d’acte sans protection résiduelle;
- joueur, autres serviteurs et mercenaires morts inchangés;
- solo, hôte/joiner, sauvegarde/rechargement et coexistence validés;
- portées globale et mod-locale, cold start et hashes conformes avant livraison.

## Prochain gate

Après le prochain gate de preuve RemoteStash, exécuter
`npm.cmd run re:d2r32 -- status`, construire les témoins de dégâts persistants
et identifier le chemin serveur 92777 qui permet au tick de dégâts de tuer le
mercenaire dans une zone de ville.
