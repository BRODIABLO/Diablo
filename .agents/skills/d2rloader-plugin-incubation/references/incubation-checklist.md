# Checklist d'incubation D2RLoader

## Contrat autonome RuffnecKk Suite

- Classer toute nouvelle DLL comme plugin autonome membre de la RuffnecKk D2RLoader Suite sans poser de question de destination.
- Ne proposer aucune catégorie, DLL propriétaire ou clé de merge PluginPack et ne planifier aucun merge futur dans une DLL d'eezstreet.
- Conserver une DLL RuffnecKk hybride avec sa version, ses métadonnées, son archive et une configuration dédiée indépendante de `D2RPlugins.json`.
- Choisir JSON ou TOML selon la convivialité réelle; rédiger contenu et commentaires en anglais.
- Consigner la décision autonome Suite et sa date dans la mission avant toute écriture de code, configuration ou archive.

## Audit de la Suite et de la pile tierce

```powershell
npm.cmd run ref:d2rlplugins -- status
npm.cmd run ref:d2rlplugins -- search <terme>
npm.cmd run ref:d2rlplugins -- symbol <symbole>
```

Relever la baseline D2R/D2RLoader/API-SDK de la Suite, les versions et hashes de ses composants, le commit PluginPack épinglé, les contrats inter-DLL, les structures partagées, les callbacks, les RVA et chaque plage d'octets lue ou écrite. Citer commit, chemin et ligne dans la mission.

## Gates techniques

- Manifeste v2 et trois exports attendus vérifiés.
- Auteur exact `RuffnecKk`; crédits tiers conservés séparément.
- Si le plugin a nécessité des connaissances acquises grâce à D2MOO, crédit explicite à D2MOO présent dans le README du plugin conservé à côté du ZIP généré par l'agent.
- Description anglaise courte, visible par le joueur et sans détails internes.
- Build ciblé, signatures complètes, ABI et erreurs de chargement strictement contrôlés.
- Baseline D2RLoader/SDK gouvernée respectée et version d'ABI inter-DLL explicitement contrôlée.
- Installation globale et mod-locale démontrée, sans `ModScopedOnly`.
- Configuration dédiée indépendante valide en JSON ou TOML; priorité mod actif puis repli global, sans bloc dans `D2RPlugins.json`.
- Contenu et commentaires de configuration entièrement en anglais.
- Configuration absente gérée par défaut; configuration présente mais invalide refusée explicitement.
- Dépendances inter-DLL explicites et versionnées; fournisseur optionnel absent ou incompatible traité sans crash, hook partiel ni ordre de chargement caché.
- Aucun hook canonique sans propriétaire unique; aucune plage concurrente non auditée.
- Coexistence démontrée avec tous les composants actifs de la Suite et les cinq DLL du PluginPack, sans plugin rejeté ou en échec.
- Aucun plugin installé n'est retiré, désactivé ou neutralisé pendant un cold start ou un test déclaré de compatibilité.
- Toutes les fonctionnalités du PluginPack sont explicitement activées pendant la matrice; la simple présence d'une DLL dont une fonctionnalité reste désactivée n'est pas une preuve.
- Toute isolation temporaire est étiquetée « diagnostic seulement », puis annulée; elle ne peut jamais soutenir une conclusion de compatibilité ou une livraison.
- Les deux ordres de chargement pertinents sont testés pour toute chaîne externe composable; un consommateur ne doit jamais exiger les octets vanilla d'une entrée légitimement possédée par un autre plugin.
- Version, SHA-256, baseline SDK, dépendances et statut de compatibilité consignés pour la release de la Suite sans supprimer le versionnement autonome du composant.

## Gate du ZIP public

Créer ou actualiser le README et le déposer à côté du ZIP dans le dossier de livraison. Ne pas l'inclure dans l'archive générée par l'agent : Vincent le relit et le modifie humainement avant de l'ajouter lui-même au ZIP final.

Autoriser uniquement :

- la DLL autonome;
- son fichier de configuration indépendant indispensable à son utilisation, JSON ou TOML selon la décision consignée.

Interdire :

- README et documentation;
- sources, symboles et fichiers de build;
- logs et preuves;
- tout fichier de configuration étranger au contrat confirmé;
- toute DLL d'eezstreet.

Lister les entrées après création, vérifier qu'elles sont à la racine attendue, puis calculer le SHA-256 du ZIP.
