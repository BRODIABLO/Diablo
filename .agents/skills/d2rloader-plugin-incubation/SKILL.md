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

1. Pour le runtime officiel courant et tout build couvert par équivalence native, exécuter le skill `d2r33-reverse-engineering` et franchir son gate `status`. Réutiliser le corpus commun lorsqu'il prouve byte-exact chaque surface employée, sans workbench ni matrice runtime duplicatifs.
2. Relever la baseline gouvernée du runtime effectivement qualifié : version D2RLoader, version d'API/SDK, versions et hashes des composants RuffnecKk, plugins tiers actifs et contrats inter-DLL partagés. Nommer séparément les builds couverts uniquement par équivalence native.
3. Vérifier la référence PluginPack épinglée et inventorier les surfaces nécessaires à la coexistence avec tous les composants actifs de la Suite et les cinq DLL eezstreet : structures, callbacks, ABI, RVA, plages de hooks, configurations et assets partagés.
4. Réutiliser les contrats gouvernés de la Suite lorsqu'ils couvrent réellement le besoin; versionner toute nouvelle coopération inter-DLL, tolérer l'absence du fournisseur et refuser proprement une version d'ABI incompatible.
5. Identifier chaque collision potentielle et désigner un propriétaire unique pour tout hook ou structure canonique.
6. Si une incompatibilité avec la Suite ou le PluginPack est envisagée, arrêter l'implantation et présenter la collision à Vincent; ne pas réduire silencieusement la matrice de coexistence.
7. Refuser toute implantation native dont les surfaces utilisées ne sont pas intégralement prouvées. Ne jamais traiter le `build-name`, le canal, le numéro de version, le hash global du PE ni le statut connu ou inconnu du runtime comme une preuve ou un gate de chargement.

## Incuber de façon autonome

1. Conserver définitivement une DLL autonome attribuée exactement à `RuffnecKk`.
2. Rendre la DLL hybride : installation globale ou mod-locale, sans `ModScopedOnly`, avec la même empreinte native fail-closed, les mêmes signatures et les mêmes contrôles d'ABI dans les deux portées.
3. Rédiger la description du plugin en anglais, en une phrase courte orientée effet joueur, sans build, RVA, hook ni ABI.
4. Rechercher la configuration dédiée d'abord dans le mod actif puis dans le dossier global du jeu. Refuser explicitement une configuration présente mais invalide.
5. Conserver le JSON ou TOML dédié et une version de composant indépendante; ne jamais déplacer sa configuration dans `D2RPlugins.json`.
6. Rendre toute dépendance entre plugins explicite, versionnée et fail-safe; un fournisseur optionnel absent ne doit pas empêcher le plugin de charger lorsque son comportement autonome reste valide.
7. Ne jamais modifier, lier ni redistribuer une DLL d'eezstreet.

## Valider et publier

1. Tester que la DLL n'utilise ni le `build-name`, ni le canal Steam/Battle.net, ni le numéro de version, ni le hash global du PE, ni le statut connu ou inconnu du runtime pour autoriser ou refuser le chargement, ou pour sélectionner un profil natif. Ces identifiants restent exclusivement diagnostiques. Lorsqu'il existe plusieurs empreintes natives, les essayer d'après les octets et témoins de layout/ABI réellement observés : accepter exactement une correspondance complète et refuser proprement zéro correspondance, une correspondance partielle ou plusieurs correspondances ambiguës avant le premier hook. Une version inconnue avec une empreinte complète valide doit pouvoir charger; une version connue avec une empreinte invalide doit être refusée. Compiler ensuite en Release x64, puis vérifier la version du composant, les exports D2RLoader et les hashes entre build, dépôt et chaque runtime.
2. Sur la version officielle courante, valider les portées globale et mod-locale, le mode autonome sans fournisseur optionnel, le gameplay prévu et la coexistence avec tous les composants actifs compatibles de la Suite ainsi qu'avec les cinq DLL eezstreet. Lorsqu'une configuration existe, valider aussi sa priorité, son repli et ses refus; lorsqu'elle n'existe pas, prouver que la DLL seule suffit dans les deux portées. Ne désactiver, retirer ou neutraliser aucun plugin installé ni aucune fonctionnalité du PluginPack pendant un cold start ou un test déclaré de compatibilité : la pile complète et toutes les fonctionnalités doivent être actives. Tester aussi les deux ordres de chargement pertinents pour chaque RVA ou contrat partagé. Une isolation temporaire est permise uniquement pour diagnostiquer une cause, doit être annoncée comme telle et ne constitue jamais une preuve; restaurer et retester la pile complète avant toute conclusion ou livraison. Une qualification peut être réutilisée sur un autre runtime seulement lorsque toutes les surfaces employées y sont prouvées byte-identiques; sinon, exécuter une qualification séparée avant d'en revendiquer la compatibilité. Ce statut de qualification gouverne les preuves et la release, jamais la décision de chargement de la DLL. La compatibilité multijoueur entre deux builds reste une matrice indépendante.
3. Consigner dans la mission de release de la Suite la version, le SHA-256, la baseline SDK, les dépendances et le statut de compatibilité du nouveau composant sans lui retirer son cycle de version autonome.
4. Créer ou actualiser le README du plugin et le déposer à côté du ZIP dans le dossier de livraison. Produire le ZIP généré par l'agent avec uniquement la DLL autonome et son fichier de configuration indépendant, JSON ou TOML. Exclure README, sources, symboles, logs et fichiers de preuve; Vincent relit et modifie humainement le README avant de l'ajouter lui-même au ZIP final.
5. Inspecter la liste réelle des entrées du ZIP et calculer son SHA-256 avant de déclarer la livraison prête.
6. Créditer exactement `RuffnecKk` dans les sources, logs et documentation de la fonctionnalité. Si le plugin a nécessité des connaissances acquises grâce à D2MOO, créditer explicitement D2MOO dans le README du plugin, conservé avec la documentation du projet et à côté du ZIP généré par l'agent.
7. Avant une release publique de la Suite, auditer toutes les DLL republiées, y compris les historiques, et bloquer la livraison si l'une compare directement un numéro de build pour décider du chargement. Les anciens artefacts déjà publiés peuvent rester disponibles comme rollback, mais ne sont jamais recopiés tels quels dans une nouvelle release.

Lire [references/incubation-checklist.md](references/incubation-checklist.md) pour le contrat autonome Suite et les gates d'audit, de runtime et d'archive.
