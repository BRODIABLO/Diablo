# PotionAutoPickup — portage BKVince D2RLoader 3.2

## Décision

Option B retenue le 18 juillet 2026 : porter directement l’autopickup 2.4 en routeur configurable de toutes les potions pour BKVince sous `D2R.exe 3.2.92777`.

La référence fonctionnelle est le source local `C:/Workspaces/D2RHUD-2.4-TCP/rejuvenation-autopickup/dllmain.cpp`. Ses RVA, signatures et layouts 2.4 ne sont que des indices et ne doivent jamais être activés dans le runtime 3.2.

## Contrat fonctionnel

Chaque famille possède sa propre politique :

- Healing : sélection indépendante de `hp1`, `hp2`, `hp3`, `hp4`, `hp5`;
- Mana : sélection indépendante de `mp1`, `mp2`, `mp3`, `mp4`, `mp5`;
- Rejuvenation : sélection indépendante de `rvs` et `rvl`;
- colonnes de belt ordonnées et indépendantes par famille;
- overflow vers l’inventaire indépendant pour chacun des 12 types;
- priorité configurable entre familles et entre tiers;
- distance et cadence de scan globales;
- objet laissé au sol si aucune destination autorisée n’est libre.

Le preset BKVince actif sélectionne `hp2`–`hp5` dans les colonnes 1–2 et
`mp2`–`mp5` dans les colonnes 3–4. `hp3`–`hp5` et `mp3`–`mp5` débordent vers
l’inventaire. `rvs` et `rvl` utilisent d’abord la colonne 4, puis débordent vers
l’inventaire.

## Architecture cible

1. Un cœur de routage pur associe le code d’item à une famille et à un tier, applique la politique et choisit une destination.
2. Une couche D2RLoader lit `PotionAutoPickup.toml`, journalise les refus et installe uniquement des hooks natifs prouvés pour le build 92777.
3. L’adaptateur runtime parcourt les items serveur au sol, contrôle distance/collision et appelle le chemin vanilla de pickup.
4. Le sélecteur de belt respecte la hauteur réelle de la ceinture, les colonnes autorisées et la famille déjà présente dans une colonne.

## Implantation 1.1.1

- `tiers` sélectionne individuellement `hp1`–`hp5`, `mp1`–`mp5`, `rvs` et
  `rvl`;
- `overflow_tiers` autorise l’overflow indépendamment pour chaque type;
- `columns` reste ordonné par famille et une liste vide permet un routage
  inventaire seulement pour les types autorisés à déborder;
- le hook thread-local de `INVENTORY_GetFreeBeltSlot 0x3862D0` ne remplace la
  destination que pendant le pickup automatique de l’objet exact sélectionné;
- `UNITS_GetInventory 0x34A360` et `INVENTORY_ResolveOccupancyGrid 0x38B070`
  fournissent la hauteur et les cases réellement occupées de la ceinture.
- les cases `0x01`–`0x12` de la table serveur `0x1D2A790` déclenchent les scans
  sur les déplacements et actions normaux; `0x16` reste le callback de pickup
  original et n’est plus utilisé comme faux déclencheur autonome;
- la capacité réelle vient du type de belt lu par `ITEMS_GetBeltType 0x349720`,
  après résolution de la case body 8, plutôt que de supposer quatre rangées.

## Diagnostic runtime 1.1.2

Le test live du 8 août 2026 a d’abord suggéré à tort que le scan et le pickup
fonctionnaient : une `mp2` était ramassée, mais placée dans le premier slot puis
dans l’inventaire malgré les colonnes 3–4 libres, tandis que les healing et
rejuvenation étaient ignorées. La 1.1.2 a ajouté une corrélation par GUID et des
compteurs suffisamment précis pour départager le routeur du comportement D2R.

- la sélection transmet maintenant le GUID serveur autoritaire de l’item au
  hook synchrone de `INVENTORY_GetFreeBeltSlot`, plutôt que d’exiger que le
  moteur réutilise exactement le même pointeur natif;
- le remplacement de destination reste thread-local et limité au pickup
  automatique courant; un GUID différent continue vers le comportement vanilla;
- le statut console expose les compteurs actions/scans, refus, sélections,
  routes belt/overflow, correspondances de GUID et totaux
  `seen/selected/picked` par code;
- `diagnostics.log_scans` permet au besoin de journaliser chaque route choisie,
  sans modifier le comportement lorsque le diagnostic est désactivé.

La capture live suivante a invalidé l’hypothèse de routage : `87` actions,
`29` scans, zéro erreur de belt ou d’énumération, mais zéro potion vue, zéro
sélection et zéro pickup pour les 12 codes. La `mp2` observée provenait donc
d’un autre chemin D2R et le défaut réel précédait entièrement le choix du slot.

## Correctif runtime 1.1.3

- les IDs de classes calculés à partir des positions Weapons + Armor + Misc sont
  abandonnés pour la classification runtime;
- `ITEMS_GetItemCode 0x36EF50`, dont l’ABI et la signature sont gouvernées,
  fournit directement le code natif compacté de chaque item (`hp2`, `mp2`,
  `rvs`, etc.);
- la même classification par code est utilisée pour les objets au sol et les
  potions déjà présentes dans la belt;
- les IDs de classes deviennent sans effet sur le comportement lorsque l’ordre
  des tables compilées d’un mod diffère de l’ordre textuel attendu.

## Gate de sécurité

- plugin hybride avec identifiant interne `potion-auto-pickup`, nom `PotionAutoPickup` et drapeau `NativeHooks`, installable globalement ou dans le dossier d’un mod;
- signatures `expected` vérifiées par D2RLoader avant chaque hook;
- aucun RVA 2.4 recopié sans correspondance démontrée dans le build 92777;
- activation atomique : si une cible obligatoire manque, aucun hook gameplay ne reste actif;
- aucune écriture dans les tables `.txt` n’est nécessaire au portage.

## Matrice de validation

- chacun des 12 codes seul puis en mélange;
- activation/désactivation et ordre de priorité par famille;
- chaque combinaison de colonnes 1–4 avec ceintures de 1 à 4 rangées;
- colonne vide, compatible, incompatible et pleine;
- overflow actif/inactif pour chacun des 12 types, inventaire libre/partiel/plein;
- distance limite, collision, plusieurs potions à distance égale;
- souris, manette, solo, hôte et joiner;
- retour menu/reconnexion/déchargement;
- absence de duplication, perte, crash et désynchronisation.

## Validation technique du 8 août 2026

- Release x64 et `router-policy` : `1/1` test réussi;
- DLL build/source/runtime byte-identique :
  `38A78D21B4758A9FF0B6BB16C86D0DA385455FECF1282041637DAB5B4D957CDA`;
- TOML source/runtime byte-identique :
  `E54BD5A911224C677A40A75BD0B63E06E19E134AD5D3D91DA4B138EA92117149`;
- cold start mod-local avec pile complète : `18/18` patchsets appliqués,
  `14/14` plugins actifs, zéro rejet/échec et startup `24/24`;
- les 18 cases runtime `0x01`–`0x12` pointent vers une cible unique située dans
  `PotionAutoPickup.dll`; le hook de belt `0x3862D0` est accepté;
- le log frais restitue exactement le preset BKVince demandé.

Le premier cold start 1.1.2 a rencontré la signature graphique récurrente
`dxgi.dll + 0x38B1C1` via `plugin-items.dll + 0x8436`, déjà présente dans cinq
rapports antérieurs à ce correctif. Une seconde tentative avec la pile complète,
sans désactiver de plugin, a atteint `24/24`; la DLL 1.1.2 est chargée depuis le
profil mod-local avec son hash gouverné.

Le cold start 1.1.3 avec la pile complète a atteint `24/24`, avec `18/18`
patchsets, `14/14` plugins actifs et zéro rejet ou échec. La reconnaissance et
les destinations 1.1.3 restent toutefois à confirmer par l’observation gameplay
et les compteurs live; le cold start ne ferme pas ce gate.

L’essai gameplay 1.1.3 du 8 août confirme `26/26` pickups réussis et `26/26`
correspondances de route, sans échec : `hp2` `8`, `hp3` `6` et `mp2` `12`.
Les rejuvenation restent non testées faute d’objet disponible. Les `252` actions
n’avaient produit que `84` scans avec l’intervalle `3`; après retour joueur sur
la réactivité, le preset BKVince passe à `minimum_interval_actions = 1` pour
scanner chaque action de déplacement. La distance reste au maximum natif sûr
de `4`.

La matrice gameplay avec potions réellement déposées, inventaire plein et belt
de différentes hauteurs reste à observer; elle n’est pas inférée du cold start.
