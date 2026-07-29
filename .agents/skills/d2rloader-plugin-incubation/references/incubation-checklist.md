# Checklist d'incubation D2RLoader

## Gate de destination

Demander d'abord uniquement : « Plugin autonome ou plugin à merger au PluginPack ? » La réponse explicite de Vincent doit précéder toute écriture de code, configuration ou archive.

### Plugin autonome permanent

- Ne demander aucune catégorie, DLL propriétaire future ou clé de merge.
- Conserver une DLL RuffnecKk hybride avec une configuration dédiée indépendante de `D2RPlugins.json`.
- Choisir JSON ou TOML selon la convivialité réelle; rédiger contenu et commentaires en anglais.
- Auditer et prouver la coexistence avec les cinq DLL du PluginPack épinglé.
- Ne jamais présenter un merge futur comme prévu sans nouvelle décision de Vincent.

### Plugin destiné au merge

| Catégorie | DLL propriétaire |
|---|---|
| `items` | `plugin-items.dll` |
| `levels` | `plugin-levels.dll` |
| `misc` | `plugin-misc.dll` |
| `quests` | `plugin-quests.dll` |
| `skills` | `plugin-skills.dll` |

Après le choix explicite du merge, faire confirmer la catégorie, la DLL propriétaire et la clé `categorie.nomFonctionnalite` avant toute implantation. Ne pas déduire cet accord d'une catégorie techniquement évidente.

## Audit du PluginPack et du module propriétaire

```powershell
npm.cmd run ref:d2rlplugins -- status
npm.cmd run ref:d2rlplugins -- search <terme>
npm.cmd run ref:d2rlplugins -- symbol <symbole>
```

Relever le commit épinglé, les fichiers du module, les clés existantes de `D2RPlugins.json`, les structures partagées, les callbacks, les RVA et chaque plage d'octets lue ou écrite. Citer commit, chemin et ligne dans la mission.

## Gates techniques

- Manifeste v2 et trois exports attendus vérifiés.
- Auteur exact `RuffnecKk`; crédits tiers conservés séparément.
- Description anglaise courte, visible par le joueur et sans détails internes.
- Build ciblé, signatures complètes, ABI et erreurs de chargement strictement contrôlés.
- Installation globale et mod-locale démontrée, sans `ModScopedOnly`.
- Configuration dédiée valide dans le format confirmé, JSON ou TOML pour un autonome permanent et JSON compatible `D2RPlugins.json` pour une voie de merge; priorité mod actif puis repli global.
- Contenu et commentaires de configuration entièrement en anglais.
- Configuration absente gérée par défaut; configuration présente mais invalide refusée explicitement.
- Aucun hook canonique sans propriétaire unique; aucune plage concurrente non auditée.
- Coexistence démontrée avec les cinq DLL du PluginPack, sans plugin rejeté ou en échec.

## Gate du ZIP public

Autoriser uniquement :

- la DLL autonome;
- le fichier de configuration indépendant indispensable à son utilisation, JSON ou TOML selon la décision consignée.

Interdire :

- README et documentation;
- sources, symboles et fichiers de build;
- logs et preuves;
- tout fichier de configuration étranger au contrat confirmé;
- toute DLL d'eezstreet.

Lister les entrées après création, vérifier qu'elles sont à la racine attendue, puis calculer le SHA-256 du ZIP.
