# CharStart — initialisation configurable des nouveaux personnages

## Décisions produit

Statut : planifié et volontairement en pause.

Le 30 juillet 2026, Vincent confirme que CharStart sera un plugin autonome
permanent destiné aux autres modders, et non une fonctionnalité requise par
BKVince. Il ne recevra aucune catégorie, DLL propriétaire ou clé de merge dans
le PluginPack.

La cible demeure une `CharStart.dll` indépendante attribuée exactement à
`RuffnecKk`, hybride entre installation globale et mod-locale, sans
`ModScopedOnly`, sans modification, liaison ou redistribution d'une DLL
d'eezstreet.

Le fichier de configuration indépendant sera `CharStart.toml`. Son contenu et
ses commentaires seront entièrement en anglais. Il sera recherché d'abord dans
le mod actif, puis dans la portée globale. Une configuration absente produira
un comportement sûr et inactif; une configuration présente mais invalide fera
refuser explicitement le chargement du plugin.

Vincent retient le séquencement B : ne pas ouvrir l'implantation avant la revue
du PluginPack et la stabilisation des plugins autonomes Readable Items,
Transmogrify, Book of Lore et ExtendedMerc. CharStart sera alors validé contre
leurs artefacts stabilisés dans une seule matrice de coexistence.

## Intention fonctionnelle

Le concept reprend les trois opérations documentées par le plugin D2Mod
CharStart 1.1 de RicFaith :

- `add_stat` ajoute une valeur à une statistique après l'initialisation native;
- `set_stat` remplace une statistique par une valeur configurée;
- `add_skill` attribue des rangs d'une compétence au nouveau personnage.

Les règles restent ordonnées et peuvent viser toutes les classes ou une classe
précise. La configuration TOML doit être facile à copier et à adapter par un
modder sans dépendre de BKVince ni de `D2RPlugins.json`.

## Faits vérifiés

- Le workbench gouverné cible D2R `3.2.92777`; son image canonique et son index
  persistant sont vérifiés.
- La routine à `0x52D770` est un candidat statique fort pour l'initialisation
  des statistiques d'un nouveau personnage. Elle lit les données de départ et
  rejoint deux chemins de création, mais son identité et son ABI ne sont pas
  encore promues dans `known-rvas.json`.
- `STATLIST_SetUnitStat 0x2F7D10` est gouverné avec confiance haute.
- Le manifeste épinglé du PluginPack contient 135 sites. Aucun ne chevauche la
  plage candidate de CharStart à `0x52D770`.
- Les statistiques `strength`, `energy`, `dexterity`, `vitality`, `statpts`,
  `newskills`, `level` et `gold` portent le flag de sauvegarde dans
  `itemstatcost.txt` de BKVince. Les exemples historiques `fireresist`,
  `poisonresist` et `hpregen` ne le portent pas.
- `BookOfLore.toml` et `toml++` fournissent déjà un précédent local pour une
  configuration TOML stricte, testée, mod-locale puis globale.

## Hypothèses à prouver

- Le hook de l'initialisateur candidat ne s'exécute qu'à la création réelle et
  ne réapplique jamais les règles lors d'un chargement ou d'une reconnexion.
- L'appel de l'original suivi des règles produit une composition déterministe
  avec `charstats.txt` : `add_stat` ajoute, puis `set_stat` remplace selon
  l'ordre du TOML.
- Une primitive native d'attribution de skill permet de synchroniser rang,
  passifs, affichage, sauvegarde et réseau sans entrer en collision avec
  `plugin-skills.dll`.
- Les statistiques personnalisées peuvent être classées de manière fiable
  entre persistantes et non persistantes depuis les données réellement
  chargées par le mod.

## Politique de sécurité proposée

- Refuser par défaut toute statistique non sauvegardée ou non résolue.
- Ne pas autoriser `set_stat` sur `level` avant une transaction cohérente avec
  expérience, gains par niveau et valeurs dérivées.
- Refuser toute règle inconnue, clé inconnue, classe invalide, identifiant hors
  plage, overflow ou doublon ambigu.
- Ne jamais publier `add_skill` avant les preuves natives de sauvegarde,
  affichage et synchronisation.
- Conserver un comportement fail-closed sur build, signature, ABI et conflit de
  hook.

## Séquencement retenu

1. Terminer les cinq lots gameplay de l'intégration PluginPack et sa revue
   eezstreet.
2. Stabiliser les gates de publication et de coexistence de Readable Items,
   Transmogrify, Book of Lore et ExtendedMerc.
3. Revalider le workbench 92777 et la référence PluginPack épinglée.
4. Prouver et promouvoir l'initialisateur de personnage, sa signature, son ABI,
   ses callers et son caractère strictement création-only.
5. Prouver la primitive `add_skill` et auditer sa composition avec
   `plugin-skills.dll`.
6. Figer le schéma `CharStart.toml`, ses valeurs par défaut, ses bornes et sa
   politique de persistance, puis tester le parseur indépendamment du runtime.
7. Implanter la DLL autonome hybride, compiler en Release x64 et vérifier le
   manifeste v2, les trois exports et les métadonnées.
8. Valider création, application unique, sauvegarde/relecture, solo,
   hôte/joiner, portées globale/mod-locale et priorité de configuration.
9. Exécuter la matrice conjointe avec les cinq DLL PluginPack, RemoteStash,
   Readable Items, Transmogrify, Book of Lore et ExtendedMerc, avec zéro plugin
   rejeté ou en échec.
10. Préparer l'archive publique strictement limitée à `CharStart.dll` et
    `CharStart.toml`, inspecter ses entrées et consigner son SHA-256.

## Crédits et identité publique

- Auteur du port D2R : `RuffnecKk`.
- Concept original et comportement de référence : RicFaith, CharStart 1.1.
- Les crédits historiques de Kingpin et des autres contributeurs du document
  original restent séparés de l'auteur du port.
- Description envisagée :
  `Applies configured stats and skills to newly created characters.`

## Prochain gate

Ne pas commencer l'implantation. Reprendre seulement après la revue PluginPack
et la stabilisation des quatre plugins autonomes retenus; exécuter alors
`npm.cmd run re:d2r32 -- status` et promouvoir l'identité complète de
`0x52D770` avant tout code, configuration ou archive CharStart.
