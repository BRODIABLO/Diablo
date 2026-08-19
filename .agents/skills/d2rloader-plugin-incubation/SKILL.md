---
name: d2rloader-plugin-incubation
description: Auditer, implanter, qualifier et emballer une nouvelle DLL D2RLoader déjà retenue comme plugin autonome RuffnecKk de la RuffnecKk D2RLoader Suite. Utiliser ce skill lorsque l'architecture a sélectionné une DLL ou que Vincent demande explicitement son implantation, sa qualification ou sa livraison; ne pas l'utiliser pour une comparaison exploratoire où le mécanisme reste ouvert.
---

# Incubation de plugin D2RLoader

## Franchir le gate d'entrée

1. Vérifier qu'une DLL D2RLoader a déjà été retenue comme mécanisme ou que Vincent demande explicitement de poursuivre une DLL existante.
2. Si le mécanisme reste ouvert, ne modifier aucun fichier : revenir à la planification normale ou à `plugin-architect` lorsque Vincent l'a explicitement invoqué.
3. Exiger le gate `GO` d'`AGENTS.md` avant toute modification de code, configuration, mission ou archive.
4. Après autorisation, consigner dans la mission la décision autonome RuffnecKk Suite et sa date avant la première implantation.

## Appliquer le contrat autonome de la Suite

1. Classer automatiquement toute nouvelle DLL comme plugin autonome RuffnecKk membre de la RuffnecKk D2RLoader Suite.
2. Ne pas demander de choisir une destination et ne proposer ni catégorie, ni DLL propriétaire, ni clé de merge PluginPack. Ne planifier aucun merge futur dans une DLL d'eezstreet.
3. Exiger une DLL indépendante, hybride globale/mod-locale, avec sa propre version, ses métadonnées, son archive et son fichier de configuration indépendant de `D2RPlugins.json`.
4. Choisir JSON ou TOML selon la convivialité réelle pour le moddeur; rédiger le contenu et les commentaires en anglais.

## Auditer avant de coder

1. Pour le runtime courant D2R 3.3.93847, exécuter le skill `d2r33-reverse-engineering` et franchir son gate `status`. L'identité binaire utile avec le corpus 92777 étant établie, réutiliser ses preuves gouvernées sans créer un workbench duplicatif.
2. Relever la baseline gouvernée courante de la Suite : build D2R, version D2RLoader, version d'API/SDK, versions et hashes des composants RuffnecKk, plugins tiers actifs et contrats inter-DLL partagés.
3. Vérifier la référence PluginPack épinglée et inventorier les surfaces nécessaires à la coexistence avec tous les composants actifs de la Suite et les cinq DLL eezstreet : structures, callbacks, ABI, RVA, plages de hooks, configurations et assets partagés.
4. Réutiliser les contrats gouvernés de la Suite lorsqu'ils couvrent réellement le besoin; versionner toute nouvelle coopération inter-DLL, tolérer l'absence du fournisseur et refuser proprement une version d'ABI incompatible.
5. Identifier chaque collision potentielle et désigner un propriétaire unique pour tout hook ou structure canonique.
6. Si une incompatibilité avec la Suite ou le PluginPack est envisagée, arrêter l'implantation et présenter la collision à Vincent; ne pas réduire silencieusement la matrice de coexistence.
7. Refuser une implantation fondée sur une ABI, une signature ou un build non prouvé.

## Incuber de façon autonome

1. Conserver définitivement une DLL autonome attribuée exactement à `RuffnecKk`.
2. Rendre la DLL hybride : installation globale ou mod-locale, sans `ModScopedOnly`, avec les mêmes gardes strictes de build, signatures et ABI dans les deux portées.
3. Rédiger la description du plugin en anglais, en une phrase courte orientée effet joueur, sans build, RVA, hook ni ABI.
4. Rechercher la configuration dédiée d'abord dans le mod actif puis dans le dossier global du jeu. Refuser explicitement une configuration présente mais invalide.
5. Conserver le JSON ou TOML dédié et une version de composant indépendante; ne jamais déplacer sa configuration dans `D2RPlugins.json`.
6. Rendre toute dépendance entre plugins explicite, versionnée et fail-safe; un fournisseur optionnel absent ne doit pas empêcher le plugin de charger lorsque son comportement autonome reste valide.
7. Ne jamais modifier, lier ni redistribuer une DLL d'eezstreet.

## Valider et publier

1. Tester la politique, compiler en Release x64, vérifier la version, les exports D2RLoader et les hashes entre build, dépôt et runtime.
2. Valider séparément les portées globale et mod-locale, le repli de configuration, le mode autonome sans fournisseur optionnel et la coexistence avec tous les composants actifs de la Suite ainsi qu'avec les cinq DLL eezstreet. Ne désactiver, retirer ou neutraliser aucun plugin installé ni aucune fonctionnalité du PluginPack pendant un cold start ou un test déclaré de compatibilité : la pile complète et toutes les fonctionnalités doivent être actives. Tester aussi les deux ordres de chargement pertinents pour chaque RVA ou contrat partagé. Une isolation temporaire est permise uniquement pour diagnostiquer une cause, doit être annoncée comme telle et ne constitue jamais une preuve; restaurer et retester la pile complète avant toute conclusion ou livraison.
3. Consigner dans la mission de release de la Suite la version, le SHA-256, la baseline SDK, les dépendances et le statut de compatibilité du nouveau composant sans lui retirer son cycle de version autonome.
4. Créer ou actualiser le README du plugin et le déposer à côté du ZIP dans le dossier de livraison. Produire le ZIP généré par l'agent avec uniquement la DLL autonome et son fichier de configuration indépendant, JSON ou TOML. Exclure README, sources, symboles, logs et fichiers de preuve; Vincent relit et modifie humainement le README avant de l'ajouter lui-même au ZIP final.
5. Inspecter la liste réelle des entrées du ZIP et calculer son SHA-256 avant de déclarer la livraison prête.
6. Créditer exactement `RuffnecKk` dans les sources, logs et documentation de la fonctionnalité. Si le plugin a nécessité des connaissances acquises grâce à D2MOO, créditer explicitement D2MOO dans le README du plugin, conservé avec la documentation du projet et à côté du ZIP généré par l'agent.

Lire [references/incubation-checklist.md](references/incubation-checklist.md) pour le contrat autonome Suite et les gates d'audit, de runtime et d'archive.
