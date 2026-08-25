# FourthSkillTree Framework — moteur générique pour D2R 3.2.92777 et 3.3.93847

Mise à jour : 25 août 2026
Statut : actif — jalon 0.1.0 implanté; persistance rank-zero, autorité serveur
d'allocation et parcours natif de respec d'un 31e skill prouvés; allocation
investie encore bloquée à la matérialisation du personnage de test

> Le nom historique de ce fichier conserve la provenance 3.2 du premier cadrage.
> Le plugin cible D2R 3.2.92777 et D2R 3.3.93847 avec le même artefact. Chaque
> preuve runtime reste attribuée séparément à son build; la matrice exécutée à
> ce jour concerne 3.3.93847.

## Décisions confirmées

- Le 24 août 2026, Vincent confirme que RuffnecKk ne construira pas des arbres
  de compétences sur demande. Le produit livré à Kain et aux autres modders est
  un framework générique; chaque auteur reste propriétaire de ses compétences,
  prérequis, positions, assets, textes et choix de balance.
- Vincent confirme le séquencement **persistance et contrats natifs d'abord,
  interface ensuite**, puis donne `GO` pour actualiser la ROADMAP et démarrer le
  chantier.
- Le mécanisme retenu reste une DLL autonome permanente
  `FourthSkillTree.dll`, membre de la RuffnecKk D2RLoader Suite et attribuée
  exactement à `RuffnecKk`.
- La DLL demeure hybride globale/mod-locale, sans `ModScopedOnly`, sans
  catégorie, sans merge PluginPack et sans modification, liaison ou
  redistribution d'une DLL d'eezstreet.
- Le fichier indépendant `fourth-skill-tree.toml` configure uniquement le
  moteur. Son contenu et ses commentaires seront en anglais. Il ne duplique
  aucune liste de skills, aucun prérequis, aucune formule et aucun équilibrage.
- Le TOML du mod actif est prioritaire sur le TOML global. Une configuration
  absente conserve un état sûr et inactif; une configuration présente mais
  invalide est refusée avant toute installation de hook.
- La mission devient active sans attribuer à FourthSkillTree les validations
  gameplay encore ouvertes des missions précédentes.

## Intention produit

FourthSkillTree fournit le moteur, le contrat de données, le validateur et la
documentation nécessaires pour qu'un auteur de mod crée lui-même une quatrième
page de classe. RuffnecKk ne reçoit ni n'encode le contenu de son arbre.

Le framework doit :

1. exposer une quatrième page de classe tout en préservant `General Skills`;
2. découvrir automatiquement les skills de page 4 dans les données du mod actif;
3. construire les widgets, connexions et parcours souris/clavier/manette;
4. déléguer les règles de skill et prérequis aux tables ordinaires du mod;
5. conserver un chemin autoritaire sûr pour allocation, respec, réseau et
   persistance;
6. échouer fermé avec des diagnostics précis lorsque le contrat du mod est
   absent, invalide ou incompatible.

Le premier contrat stable accepte la grille native `3 x 6`. Une extension
versionnée ajoute ensuite le placement libre, un nombre variable de widgets et
la navigation calculée sans casser les arbres déjà compatibles.

## Frontière entre framework et contenu du mod

### Le framework RuffnecKk possède

- le cycle de vie de la cinquième identité UI;
- les onglets et la conservation de la page native `General Skills`;
- la création/destruction sûre des widgets et des liens visuels;
- la navigation souris, clavier et manette;
- la validation du contrat et les limites de sécurité;
- les chemins allocation, respec, sauvegarde et synchronisation démontrés;
- la compatibilité globale/mod-locale, Suite et cinq DLL eezstreet.

### L'auteur du mod possède

- les lignes de `skills.txt` et `skilldesc.txt`;
- les formules, niveaux, coûts, maximums et prérequis;
- les clés de localisation, icônes et arrière-plans;
- le nombre de skills, leur placement et leur équilibrage;
- les éventuelles métadonnées de quatrième page absentes des tables vanilla.

## Contrat de données recommandé

### Autorités existantes

- `skills.txt` reste l'autorité des compétences et de leurs prérequis.
- `skilldesc.txt` désigne l'appartenance par `SkillPage = 4`.
- `SkillRow` et `SkillColumn` fournissent le layout initial compatible `3 x 6`.
- Les connexions sont dérivées des prérequis; elles ne sont jamais recopiées
  dans le TOML du plugin.

### Descripteur minimal du mod

Les tables vanilla ne fournissent que `StrSkillTab1`, `StrSkillTab2` et
`StrSkillTab3`. Un petit descripteur appartenant au mod doit donc porter les
métadonnées impossibles à exprimer autrement, par exemple :

```text
Class\tTitleKey\tBackground
ama\tAmazonSkillTab4\tamazon_tree_4
```

Le nom et le format final (`fourthskilltree.txt` ou format voisin) restent à
figer après preuve du meilleur chemin de chargement depuis le mod actif. Ce
descripteur ne contient aucune liste de skills ni aucun prérequis.

### Extension de layout libre

Une version ultérieure du contrat peut associer un skill à des coordonnées
normalisées `X/Y`. Le moteur dessine alors les liens depuis les prérequis et
calcule les voisins de navigation. Le validateur refuse notamment :

- identifiants inconnus ou dupliqués;
- coordonnées hors panneau ou chevauchements interdits;
- prérequis absents ou cycles;
- textes et assets obligatoires manquants;
- nombres de widgets dépassant la limite de sécurité qualifiée.

## Faits vérifiés pour D2R 3.3.93847

- Le workbench gouverné courant est prêt et réutilise le corpus natif de
  provenance 92777 : image canonique
  `CC59119DC2A6C7D43D088098FC162EAFA4AE1299B2079126AEF43C1ACA914715`,
  image d'analyse
  `673E8C0B2E89563E75525B24D137098EFD07B2DB4ED42ADEC56AA1ADDF0E63AB`
  et index vérifiés.
- La politique du plugin accepte exactement `92777` et `93847`, puis refuse les
  build names vides, adjacents ou inconnus. L'identité native utile commune
  autorise le même artefact; les cold starts, portées et cas gameplay restent à
  exécuter séparément sur 3.2.92777 avant toute revendication dual-build livrée.
- La documentation d2rdoc courante décrit `SkillPage` seulement pour les
  valeurs 0 à 3, `SkillRow` pour 0 à 6 et `SkillColumn` pour 0 à 3. Une page 4
  ou un layout plus large n'est donc pas un comportement data-only documenté.
- Le layout clavier/souris BKVince expose seulement `Tab0` à `Tab2`.
- Les layouts manette de référence exposent déjà `Tab3`,
  `ActivateTab:3`, `TextTab3 = @GeneralSkills`, `CommonSkillsContainer` et
  `ItemSkillsContainer`. L'index UI 3 n'est pas libre : il appartient à
  `General Skills`.
- Le setter de page à `0x14C3B10` borne l'index à 3, puis reconstruit le panneau
  par `0x14C7720`. La navigation à `0x14C6BBC..0x14C6BFF` boucle également de
  0 à 3.
- Une vraie quatrième page de classe qui conserve `General Skills` exige donc
  cinq états : quatre pages de classe `0..3`, puis `General Skills` à l'état 4.
- La branche d'allocation à `0x14C69D6` lit un identifiant de skill, appelle
  `SKILLTREE_CanAllocateSkill` à `0x14C3DA0`, puis envoie la commande native
  `0x3B` à `0x14C6A07`.
- Une lecture runtime contrôlée de D2R 3.3.93847 prouve que la case `0x3B` du
  tableau serveur `D2R+0x1D2A790` pointe sur
  `D2GAME_PACKETCALLBACK_Rcv0x3B_AllocateSkillPoints 0x4B3EE0`. Le handler
  borne le skill id contre les lignes SkillsTxt compilées, résout le skill, son
  `MaxLvl` et son rang de base, puis applique les rangs par `0x438670`. Il ne lit
  ni `SkillPage`, ni `SkillRow`, ni `SkillColumn` : l'autorité serveur est donc
  indépendante du numéro de page UI.
- Le respec autoritaire par opcode `0x39` appelle
  `D2GAME_PLAYER_ResetStatsAndSkills 0x580F20`. Sa phase skills,
  `D2GAME_PLAYER_ResetSkills 0x4360F0`, parcourt la liste compilée complète des
  skills de la classe, retire les rangs de base et rembourse leur somme dans le
  stat 5. Aucun filtre de page ni borne 30 n'existe dans cette traversée.
- `UI_DispatchMessage 0x843D90` a déjà un propriétaire gouverné : le broker de
  `plugin-skills`. RemoteStash conserve sa coexistence en redirigeant un
  callsite étroit plutôt qu'en prenant l'entrée. FourthSkillTree doit rejoindre
  ce contrat et ne posera pas un second hook sur le dispatcher commun.
- La référence D2SSharp épinglée confirme que `Character.NumSkills` est un byte
  sérialisé et que la section `if` contient un tableau variable d'un byte par
  skill. Sa constante 30 est le défaut vanilla, pas une borne du layout.
- Le writer natif appelle le compte compilé de classe à `0x534519` et écrit ce
  byte dans l'en-tête. La section skills écrit `if` à `0x52F557`, puis boucle
  sur `DATATBLS_GetClassSkillCount 0x33CB30` et
  `DATATBLS_GetClassSkillIdByIndex 0x33DDE0` pour sérialiser chaque rang de base.
  Le lecteur vérifie `if` à `0x52EC98`, parcourt le compte sauvé et avance de
  `2 + count`. Le format statique n'est donc pas intrinsèquement limité à 30;
  le byte d'en-tête autorise structurellement jusqu'à 255 entrées.
- Le jalon `FourthSkillTree 0.1.0` compile en DLL x64 reproductible, n'installe
  aucun hook et valide strictement le TOML ainsi que le contrat
  `skills.txt`/`skilldesc.txt`. Les tables BKVince non modifiées passent avec
  `456/266` lignes et aucune page 4; le fixture passe avec `457/267`, une page 4
  Amazon et 31 skills compilés pour cette classe.
- Le cold start du fixture sous la pile complète passe avec `33` plugins,
  `18` memory patches, `190` tables compilées et startup complet. Le log du
  plugin observe `ama source-skills=31 fourth-page-skills=1` et `30/0` pour les
  sept autres classes.
- Le scénario isolé charge le même personnage, entre en jeu, sauvegarde,
  recharge, entre de nouveau et sauvegarde une seconde fois. Le `.d2s` passe de
  `1 283` octets/30 skills à `1 284` octets/31 skills; sa section `if` contient
  31 rangs nuls puis le marqueur `JM`. Le hash final est
  `158C39A272B74FD99E1D41DD75F28C1E1CB490E0930D9153519058EC86263C25`.
- Ces preuves ferment compilation, persistance rank-zero et indépendance de
  page du callback serveur, ainsi que l'absence de borne 30 dans le parcours
  natif de respec. Elles ne prouvent pas encore le clic client, la sauvegarde
  d'un rang investi, le respec dynamique, la synchronisation hôte/joiner ni
  l'UI page 4.
- Le 25 août, un fixture d'allocation distinct clone Barbarian `Bash` en skill
  456 sur une cellule libre de la page native 1, afin de tester l'allocation
  sans attendre l'UI page 4. Une sauvegarde gameplay level 99, déjà éprouvée
  sous le runtime courant, est étendue de 30 à 31 rangs avec `NumSkills = 31`,
  checksum corrigé et `JM` intact. D2R la présente correctement au menu, puis
  ferme pendant la matérialisation du joueur avant l'ouverture du panneau.
  Son hash reste `5EEF2121…7362C78`, donc aucun rang n'a été investi. Cette
  tentative invalide la greffe directe d'un rang sur une ancienne sauvegarde
  comme voie de preuve; elle n'invalide pas le `.d2s` 31-rangs créé et relu
  nativement lors du jalon précédent.
- L'archive historique Talonrage reste une référence comportementale D2Mod
  1.10 sans source ni licence de redistribution démontrée. Aucun de ses
  binaires ou assets ne sera redistribué.

## Hypothèses à démontrer

- Les boucles de construction UI peuvent être étendues à cinq états sans
  remplacer intégralement le panneau natif.
- Le trajet client complet d'allocation accepte le skill 456 lorsqu'il provient
  d'un personnage 31-skills entièrement matérialisé par le runtime courant.
- Un fixture réel de 31 skills investis traverse sans perte allocation, respec
  et hôte/joiner sous la pile complète.
- Les classes personnalisées suivent le même modèle runtime que les classes
  natives.

## Inconnues bloquantes

- Tous les consommateurs de la borne 0..3 et la branche spéciale
  `General Skills` à déplacer de 3 vers 4.
- Structure compilée exacte de `SkillPage`, `SkillRow` et `SkillColumn`.
- Construction exacte des listes de classe compilées et comportement lorsque
  le compte sauvé diffère du compte du mod actif.
- Autorité hôte/joiner, synchronisation, respec dynamique et migration d'une
  sauvegarde.
- Propriété unique des hooks face à `plugin-skills.dll`, Bulk Skill Point
  Allocation, RightClickSkillPoint et les composants actifs de la Suite.
- Format exact du descripteur minimal et chemin sûr de résolution des assets du
  mod actif.

## Architecture retenue

1. **Moteur minimal** : TOML indépendant limité à la politique du framework;
   aucun contenu d'arbre.
2. **Découverte active-mod** : `SkillPage = 4` sélectionne les compétences et
   les champs natifs restent l'autorité des règles.
3. **Cinq états UI** : pages de classe 0 à 3, `General Skills` à 4.
4. **Descripteur mod minimal** : titre et ressources seulement lorsque les
   tables vanilla n'ont aucun champ équivalent.
5. **Persistance avant UI livrable** : aucun support de skill investissable
   supplémentaire n'est annoncé tant que sauvegarde, relecture, respec et réseau
   ne sont pas prouvés.
6. **Extension versionnée** : le placement libre est ajouté sur le même contrat
   sans transformer RuffnecKk en service de création d'arbres.

## Plan d'implantation

### Phase 0 — preuves et contrat

1. Inventorier tous les consommateurs des quatre états UI et promouvoir les
   fonctions, signatures, ABI et plages exactes nécessaires.
2. Prouver la représentation compilée de `SkillPage = 4`.
3. Étendre le fixture runtime de 31 skills, déjà qualifié pour compilation et
   deux cycles de sauvegarde/rechargement rank-zero, à l'allocation, au respec et
   à la synchronisation.
4. Auditer la baseline D2RLoader/SDK, les cinq DLL eezstreet et chaque
   propriétaire de hook pertinent.
5. Figer la version 1 du contrat de données seulement après ces preuves.

### Phase 1 — noyau framework

1. Incuber `FourthSkillTree.dll` autonome, hybride et fail-closed.
2. Implanter la résolution TOML mod-local puis globale.
3. Ajouter les cinq états UI et préserver `General Skills`.
4. Découvrir la page 4 depuis le mod actif et valider la grille `3 x 6`.
5. Conserver les chemins natifs prouvés pour allocation, respec et persistance.

### Phase 2 — outil pour modders

1. Ajouter le descripteur minimal et son schéma versionné.
2. Livrer un validateur avec diagnostics exploitables.
3. Fournir un fixture synthétique et une documentation sans contenu Kain ou
   BKVince codé en dur.
4. Ajouter le layout libre et la navigation calculée après qualification du
   noyau.

## Matrice de validation

- TOML absent, valide, invalide et clé inconnue.
- Mod sans page 4, page vide, un skill, 18 skills et dépassement contrôlé.
- Cellule dupliquée, texte/asset manquant, prérequis inter-page et cycle.
- `General Skills` conservé sur souris/clavier et manette.
- Allocation simple/multiple, niveau maximal, respec et skill actif.
- 31e skill : sauvegarde, relecture, fermeture complète, migration et rollback.
- Solo, hôte et joueur rejoignant avec données identiques et incompatibles.
- Résolutions et échelles UI prises en charge.
- Portées globale et mod-locale, priorité de configuration et absence de doublon.
- Même artefact sur 3.2.92777 et 3.3.93847, avec matrice complète indépendante
  pour chaque build et refus propre de tout autre build name.
- Suite complète et cinq DLL eezstreet, toutes fonctionnalités actives, deux
  ordres pertinents et zéro hook sans propriétaire unique.

## Rollback

- `enabled = false` n'installe aucun hook et conserve le comportement vanilla.
- Retirer la DLL et son TOML restaure les trois pages de classe et
  `General Skills` natifs.
- Aucun rang supplémentaire n'est écrit dans une sauvegarde réelle avant que le
  chemin de relecture et son rollback soient prouvés sur des copies dédiées.
- Chaque patch vérifie des octets/signatures stricts et refuse tout build ou ABI
  incompatible.

## Crédits et livraison future

- Auteur : `RuffnecKk`.
- Description du jalon : `Validates active-mod data for a moddable fourth skill tree.`
- Talonrage reste crédité séparément comme référence comportementale historique.
- Si D2MOO contribue aux connaissances utilisées, son crédit est ajouté au
  README conservé à côté du ZIP.
- Le ZIP généré par l'agent contiendra uniquement `FourthSkillTree.dll` et
  `fourth-skill-tree.toml`; README, documentation, fixture, sources, symboles,
  logs et preuves resteront hors archive pour la relecture humaine.

## Prochain gate

Créer sous le runtime courant un personnage 31-skills déjà matérialisé avec des
points disponibles, sans étendre une sauvegarde 30-skills. Prouver sur ce témoin
l'allocation du skill 456, Save and Exit/relecture et le respec dynamique;
exécuter ensuite hôte/joiner, puis terminer l'inventaire des consommateurs UI
0..3 avant la première implantation du panneau à cinq états. Qualifier enfin le
même artefact séparément sous D2R 3.2.92777, sans déduire cette réussite des
preuves 3.3.
