# PSpell Framework / UseSkill — D2R 3.2

Mise à jour : 30 juillet 2026
Statut : planifié — Option A retenue, aucune implantation avant les preuves natives

## Décisions confirmées

Vincent confirme le 30 juillet 2026 que ce chantier sera un **plugin autonome
permanent**. Il ne possède aucune catégorie PluginPack, DLL propriétaire future,
clé de merge ou trajectoire de merge planifiée.

La cible est une unique `PSpellFramework.dll` attribuée exactement à
`RuffnecKk`, hybride globale/mod-locale, sans `ModScopedOnly`, compatible avec
les cinq DLL du PluginPack et indépendante de toute DLL d'eezstreet. Cette même
DLL réunit le registre partagé et son premier consommateur intégré `UseSkill`;
aucune seconde DLL de framework n'est ajoutée.

L'analyse ergonomique retient `PSpellFramework.toml` comme configuration
indépendante. Un catalogue de règles répétées, leurs commentaires et leurs
modes de ciblage sont plus lisibles en tableaux TOML qu'en objets JSON imbriqués.
Le contenu et les commentaires seront entièrement en anglais. Le fichier sera
recherché d'abord dans le mod actif puis dans le dossier global; une
configuration présente mais invalide sera refusée sans repli silencieux.

Vincent retient l'**Option A — fondation d'abord** le 30 juillet 2026 :

1. terminer les cinq lots gameplay et la revue eezstreet du PluginPack;
2. stabiliser le fallback autonome et la propriété des hooks de Readable Items;
3. prouver le dispatcher `pSpell`, figer une ABI de registre versionnée et
   désigner un propriétaire unique de chaque surface;
4. implanter le handler `UseSkill`, puis les charges et la suppression à zéro;
5. seulement ensuite permettre à D2RedPortal, PSpell Spawn et aux futures
   façades d'objets de s'enregistrer sans poser un hook concurrent.

La mission active demeure `Mission/eezstreet-pluginpack-integration.md` jusqu'à
la fermeture de son gate. Ce chantier reste planifié et n'autorise encore ni
code, ni configuration, ni archive.

## Besoin issu de la proposition Discord

Le screenshot transmis par Vincent contient trois exigences indissociables :

- un objet lance un sort lorsqu'il est utilisé par clic droit;
- idéalement, l'objet utilise des charges et disparaît lorsque la dernière est
  consommée;
- le mécanisme doit être un framework où plusieurs plugins enregistrent de
  nouveaux comportements `pSpell` sans se marcher dessus, avec Book of Lore et
  un futur Book of Spell comme exemple explicite.

eezstreet précise également que D2R a étendu les livres et que les objets de
type parchemin constituent un bon premier cas. Le produit n'est donc pas
seulement un item Fire Ball : il doit fournir un contrat de coexistence durable.

## Faits vérifiés

- Le workbench gouverné est vérifié pour `D2R.exe 3.2.92777`, avec image, index
  et projet Ghidra disponibles.
- La référence `eezstreet/D2RL-Plugins` est épinglée au commit
  `dc75b49ffbb67b887d7757ee00ee9a03bcde5d8a`.
- `D2ItemsTxt.pSpell` se trouve à l'offset `+0x94` de la structure partagée.
- D2R 3.2 documente des handlers fixes pour les valeurs positives `1` à `15`;
  BKVince et les données vanilla utilisent déjà cet espace.
- Le témoin Readable Items prouve que `pSpell=-2` est rejeté côté client avant
  l'émission du paquet, tandis que `pSpell=14` plus un sélecteur d'objet valide
  atteint le trajet serveur.
- `pSpell=14` demeure le handler vanilla `SkillItemSendQuestInfo`; Readable
  Items ne capture que ses objets configurés et délègue les autres.
- La référence sémantique D2MOO montre que l'ancien dispatcher transmettait le
  `BookSkill` ou `ScrollSkill` de `books.txt`, mais ses anciens handlers `10` et
  `11` lançaient Fire Ball en dur. D2R les a réaffectés aux récompenses Book of
  Skill et Golden Bird; aucun code, RVA ou ABI D2MOO n'est transposable.
- Les skills Scroll/Book utilisent `srvdofunc=113`; la référence sémantique
  vérifie le livre ou parchemin correspondant ainsi que sa quantité avant
  d'emprunter le chemin d'utilisation natif.
- Transmogrify possède actuellement `D2GAME_HandleUseItemPacket 0x4F40C0` et
  délègue explicitement Readable Items avant son propre traitement. Un second
  hook concurrent sur cette entrée est interdit.
- Book of Lore utilise actuellement un trajet distinct : Tower Tome
  `0x5DC570`, paquet objet `0x27`, handler client `0x19A630` et UI scroll
  `0x197BF0`. Son noyau actuel n'est donc pas un consommateur `pSpell`.
- `plugin-skills.dll` possède le hook de consommation mana/charges
  `0x436830`; le chemin quantité des livres ne doit pas le détourner ni le
  doubler sans preuve de composition.

## Architecture cible à prouver

### Un seul propriétaire du dispatcher

`PSpellFramework.dll` doit posséder une surface `pSpell` plus étroite que
`0x4F40C0`. Transmogrify conserve son hook global d'utilisation d'objet. Le
framework délègue immédiatement au comportement original tout objet qu'aucune
inscription ne revendique.

L'ABI exportée sera versionnée et bornée. Une inscription devra au minimum
porter un identifiant de propriétaire stable, un sélecteur d'objet, un mode de
ciblage, un callback et une politique de consommation. Les résultats devront
distinguer explicitement `not handled`, `handled success` et `handled failure`.
Deux inscriptions revendiquant le même domaine seront refusées et journalisées;
aucun ordre de chargement implicite ne décidera silencieusement du gagnant.

Le cycle D2RLoader permettant l'inscription après chargement de toutes les DLL
reste à prouver. Si l'ordre de chargement ne suffit pas, le contrat devra offrir
une découverte ou une nouvelle tentative déterministe sans boucle permanente.

### `UseSkill`, premier handler intégré

Le premier périmètre privilégie les livres et parchemins :

1. identifier une entrée explicitement autorisée par le TOML;
2. obtenir le skill depuis les données compilées de `books.txt` lorsque cette
   voie est disponible, sans dupliquer inutilement l'identité du skill;
3. choisir un ciblage borné parmi `self`, `cursor` et `selected_target` selon
   les helpers 92777 réellement prouvés;
4. lancer le skill sur l'hôte autoritaire;
5. consommer une unité seulement après un succès démontré;
6. synchroniser la quantité et supprimer l'objet à zéro par des helpers natifs
   dont la signature et l'ABI sont gouvernées.

Les objets génériques hors livres/parchemins restent une phase ultérieure tant
que leurs primitives de quantité, suppression et sauvegarde ne sont pas
prouvées. Le TOML décrit la politique et les sélecteurs; il ne stocke jamais le
nombre courant de charges, qui appartient à l'objet sauvegardé.

### Compatibilité progressive

- Readable Items garde son chemin `pSpell=14` actuel pendant la stabilisation
  de son fallback; une inscription optionnelle au registre ne viendra qu'après
  preuve du contrat, sans casser son autonomie.
- Transmogrify garde `0x4F40C0` et ne devient ni une dépendance ni un module à
  modifier, lier ou redistribuer.
- Book of Lore conserve son chemin Tower Tome; seule une future façade
  inventaire livre/parchemin pourrait s'inscrire au registre.
- D2RedPortal et PSpell Spawn devront consommer l'ABI du registre plutôt que
  revendiquer une nouvelle surface `pSpell` concurrente.
- Les cinq DLL eezstreet restent inchangées et doivent charger conjointement
  sans plugin rejeté ou en échec.

## Hypothèses à tester

- Le dispatcher `pSpell` 92777 peut être accroché à une surface plus étroite que
  le handler global `0x4F40C0` tout en conservant les coordonnées et la cible.
- Une valeur transporteur déjà acceptée peut être virtualisée par sélecteur
  d'objet sans détourner le comportement vanilla ni Readable Items.
- `pSpell=16` pourrait être accepté après preuve client/serveur; il ne constitue
  actuellement ni un slot libre ni une réservation.
- Le skill issu de `books.txt` peut être lancé avec une quantité native et une
  consommation synchronisée sans emprunter les charges d'équipement de
  `plugin-skills`.
- Une ABI C stable peut accepter des inscriptions provenant de DLL chargées
  globalement ou dans le mod actif sans dépendre d'un ordre alphabétique.

Ces hypothèses ne constituent pas des preuves et n'autorisent aucune
implantation.

## Inconnues bloquantes

- RVA, signature complète, ABI et résultat exact du dispatcher `pSpell` 92777;
- limite client réelle du domaine `pSpell` et comportement observé de `16`;
- helpers de cast sur soi, cible et coordonnées, ainsi que leur sémantique de
  réussite ou d'échec;
- disposition runtime de `BooksTxt` et résolution sûre de `BookSkill` et
  `ScrollSkill`;
- primitive autoritaire de décrémentation, suppression à zéro et synchronisation;
- moment sûr d'inscription inter-DLL dans le cycle de vie D2RLoader;
- arbitrage exact lorsque Readable Items, Transmogrify et un handler enregistré
  voient le même objet mal configuré;
- persistance après sauvegarde/rechargement et autorité hôte/joiner.

## Gates avant code

1. Attendre la fermeture des lots gameplay et de la revue PluginPack, puis la
   stabilisation du fallback autonome de Readable Items.
2. Exécuter `npm.cmd run re:d2r32 -- status` et vérifier de nouveau la référence
   PluginPack épinglée au moment de la reprise.
3. Identifier et promouvoir le dispatcher `pSpell`, ses callers, ses signatures
   et son ABI dans les preuves gouvernées.
4. Prouver le trajet d'au moins un livre/parchemin témoin, le skill transporté,
   les trois familles de ciblage requises et le résultat de cast.
5. Prouver séparément la quantité, la consommation après succès, la suppression
   à zéro et leur synchronisation.
6. Figer l'ABI de registre v1, ses résultats, ses règles de doublon, son cycle de
   vie et son comportement lorsque le framework est absent.
7. Auditer toutes les surfaces contre Transmogrify, Readable Items, Book of Lore
   et les cinq DLL du PluginPack; refuser toute collision sans propriétaire unique.

## Gates d'implantation et de livraison

- tests purs du parseur TOML, des limites, des clés inconnues, des doublons et
  du repli mod actif vers global;
- tests purs du registre, de sa version ABI, des inscriptions concurrentes, de
  la délégation vanilla et du déchargement;
- build Release x64, manifeste v2, trois exports D2RLoader, auteur `RuffnecKk`
  et description anglaise courte orientée effet joueur;
- configuration absente, valide et invalide, sans activation silencieuse;
- sorts sur soi, cible et coordonnées, succès, refus et interruptions;
- charges `1`, `2` et maximum, consommation uniquement après succès, disparition
  exacte à zéro, sauvegarde/rechargement et absence de duplication;
- non-régression des `pSpell 1..15`, du Town Portal Scroll, du Scroll of Inifuss
  et des objets non configurés;
- matrices seul, Readable Items, Transmogrify, Book of Lore, tous ensemble et
  avec les cinq DLL eezstreet;
- solo, hôte/joiner, deux portées d'installation, hashes build/dépôt/runtime et
  cold starts sans rejet ni échec;
- ZIP public strictement limité à `PSpellFramework.dll` et
  `PSpellFramework.toml`.

## Prochain gate

Conserver la mission PluginPack comme priorité. Après ses cinq lots gameplay et
sa revue eezstreet, puis après stabilisation du fallback autonome de Readable
Items, reprendre par `re:d2r32 -- status`, identifier le dispatcher `pSpell`
92777 et prouver le trajet livre/skill/quantité. Figer ensuite l'ABI du registre
avant toute création de DLL ou de configuration.
