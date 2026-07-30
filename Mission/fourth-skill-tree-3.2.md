# FourthSkillTree — quatrième page par classe sous D2R 3.2

Mise à jour : 30 juillet 2026
Statut : planifié — Option A retenue, aucune implantation avant les preuves natives

## Décisions confirmées

- Vincent retient un plugin autonome permanent, sans catégorie, propriétaire futur ni clé de merge PluginPack.
- Le plugin fournit une quatrième page propre à chaque classe, alimentée par les données du mod actif.
- La DLL cible est `FourthSkillTree.dll`, attribuée à `RuffnecKk`, et doit fonctionner aussi bien dans le dossier global D2RLoader que dans celui d'un mod, sans `ModScopedOnly`.
- La configuration indépendante est `fourth-skill-tree.toml`. Son contenu et ses commentaires seront en anglais.
- La résolution de configuration privilégie le TOML du mod actif, puis le TOML global. Une configuration absente conserve un état sûr et inactif; une configuration présente mais invalide est rejetée avant l'installation des hooks.
- La séquence Option A est retenue : terminer les cinq lots de smoke tests PluginPack, soumettre la branche validée à la revue d'eezstreet, auditer la faisabilité native de FourthSkillTree, puis seulement implanter et valider le plugin avant de reprendre les chaînes de dépendances plus longues.
- La mission active reste `Mission/eezstreet-pluginpack-integration.md` jusqu'à la fermeture de son prochain gate.

## Intention produit

FourthSkillTree doit être une capacité générique du moteur, pas une fonctionnalité codée en dur pour BKVince. Le mod actif fournit les lignes de compétences déclarées avec une quatrième page, ainsi que les assets et textes nécessaires. Le plugin détecte ces données et expose une quatrième page distincte pour chaque classe compatible, y compris une classe personnalisée telle que Warlock si son modèle runtime est démontré compatible.

L'objectif visible pour le joueur est simple : ouvrir, parcourir et utiliser une quatrième page de compétences dans l'interface native, avec la même cohérence de navigation, d'allocation et de persistance que les trois pages existantes.

## Faits vérifiés au cadrage

- L'archive de référence `D2Mod_4th-skillpage.rar` porte le SHA-256 `59B6F24EE7AF1CF511E0062E76873FC94C1C8A8BC82C2D7D2858FAFCAC02BB00`. Elle cible D2Mod 1.10, crédite Talonrage et contient notamment `SklTree.dll`, `NewTxt.dll`, `SklTree.txt` et sept arrière-plans DC6. Aucune source ni licence de redistribution n'a été trouvée dans l'archive.
- L'archive D2AddSkill, SHA-256 `3607B8371F1C3CF8C2D72F5C487D5E7912671E1964F28C9C6BDA20B15D12AC2E`, correspond à un livre de compétences pSpell séparé; ce n'est pas une dépendance démontrée de FourthSkillTree.
- Les tables `skilldesc.txt` de BKVince et de la référence vanilla 3.2 exposent les colonnes `SkillPage`, `SkillRow` et `SkillColumn`. Les classes natives utilisent actuellement les pages 1 à 3; BKVince ajoute notamment Warlock.
- Le layout BKVince `skillstreepanelhd` déclare seulement `Tab0` à `Tab2`, `textTab0` à `textTab2` et `ActivateTab` 0 à 2.
- Dans l'atelier gouverné du build 3.2.92777, les preuves existantes identifient `SKILLTREE_CanAllocateSkill` à `0x14C3DA0`, la branche de clic à `0x14C69D6` et l'appel de paquet à `0x14C6A07`.
- Le manifeste final PluginPack couvre actuellement 135 sites sur 135 dans cinq DLL. Aucun audit de collision FourthSkillTree n'a encore été réalisé.

Ces faits cadrent le chantier, mais ne prouvent pas encore qu'une valeur `SkillPage = 4` traverse correctement le compilateur de données, le contrôleur UI et le runtime.

## Architecture recommandée

1. Étendre dynamiquement le panneau de compétences vivant afin d'ajouter une quatrième identité de page, plutôt que remplacer intégralement son layout.
2. Détecter les compétences de page 4 à partir des données du mod actif et construire une page distincte pour la classe affichée.
3. Conserver le chemin natif d'allocation, de paquet, de sauvegarde et de respec chaque fois que les preuves montrent qu'il est déjà indépendant du numéro de page.
4. Ne persister aucune donnée propriétaire si la page n'est qu'un état de vue et si les compétences restent sauvegardées par le jeu.
5. Garder le TOML minimal au premier incrément :

   ```toml
   enabled = false
   ```

   Les identifiants de classe, de compétences, les textes et les assets ne doivent pas être dupliqués dans le TOML tant que les tables et assets du mod peuvent rester leur source autoritaire.
6. Ne modifier, lier ni redistribuer aucune DLL d'eezstreet ni aucun binaire ou asset historique de Talonrage.

## Hypothèses à démontrer

- Le compilateur 3.2 conserve une valeur `SkillPage = 4` sans la borner à trois pages.
- Le contrôleur du panneau peut accepter un quatrième index sans remplacer les structures natives.
- La navigation souris et manette peut atteindre ce nouvel index et revenir proprement aux pages 1 à 3.
- Le fond, le libellé d'onglet et les autres ressources visuelles peuvent être fournis par le mod actif sans dépendance codée en dur à BKVince.
- L'allocation, les prérequis, la sauvegarde et le respec opèrent sur les compétences elles-mêmes et non sur une limite implicite de trois pages.
- Les classes personnalisées suivent le même modèle runtime que les classes natives.

## Inconnues bloquantes

- Fonctions, RVA, signatures, ABI et bornes exactes du contrôleur de pages sous le build 92777.
- Représentation compilée de `SkillPage` et éventuelles validations côté client ou serveur.
- Cycle de vie des widgets, sélection persistante de l'onglet et navigation manette.
- Différences entre les chemins graphiques legacy et remastered.
- Assets et clés de localisation réellement requis pour une quatrième page générique.
- Propriété des hooks et collisions avec `plugin-skills.dll`, notamment Bulk Skill Point Allocation, et avec les 135 sites du PluginPack.

## Gates avant toute implantation

1. Fermer les cinq lots de gameplay PluginPack et documenter la revue d'eezstreet.
2. Exécuter `npm run re:d2r32 -- status` et conserver l'image ainsi que l'index vérifiés sans redump inutile.
3. Prouver le chemin du compilateur ou chargeur de `SkillPage`, le contrôleur des onglets, ses bornes et tous les callsites modifiés.
4. Établir des signatures robustes, l'ABI et les invariants nécessaires avant de promouvoir toute nouvelle identification stable dans `known-rvas.json`.
5. Auditer la propriété de hooks contre les cinq DLL PluginPack et les sites de Bulk Skill Point Allocation; chaque site doit avoir un propriétaire unique.
6. Vérifier la résolution stricte du TOML, son défaut inactif et le rejet avant hooks de toute configuration invalide.

## Matrice de validation prévue

- TOML absent, valide, invalide et contenant une clé inconnue.
- Build Release x64, exports et métadonnées D2RLoader attendus.
- `enabled = false` sans patch ni effet runtime.
- Page 4 visible et fonctionnelle pour chaque classe qui en fournit les données, dont Warlock si la preuve runtime le permet.
- Navigation souris et manette, toutes résolutions UI prises en charge, et retours entre les quatre pages.
- Allocation, prérequis, sauvegarde, rechargement et respec.
- Solo, hôte et joueur rejoignant une partie.
- Installation globale et mod-local, avec priorité mod puis fallback global.
- Coexistence avec les cinq DLL PluginPack sans site rejeté, échec d'installation ni double propriétaire.
- Hashes et logs frais du binaire, du TOML résolu et des artefacts déployés.

## Crédits et livraison publique

- Auteur du plugin : `RuffnecKk`.
- Référence comportementale historique : Talonrage, crédité séparément.
- Description courte visée : `Adds a fourth skill tree page supplied by the active mod.`
- Aucun binaire, code ni asset historique n'est redistribué sans droit démontré.
- Le ZIP public autonome contient uniquement `FourthSkillTree.dll` et `fourth-skill-tree.toml`.

## Prochain gate

Ne rien implanter pour le moment. Après les cinq lots de gameplay PluginPack et la revue d'eezstreet, relancer l'atelier 92777, prouver le chemin complet de la page 4 et fermer l'audit de coexistence avant toute modification de code, configuration runtime ou archive.
