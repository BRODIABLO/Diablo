---
name: d2rloader-plugin-incubation
description: Choisir, concevoir, auditer, implanter et emballer un nouveau plugin D2RLoader soit autonome permanent, soit destiné à un futur merge dans le PluginPack eezstreet. Utiliser ce skill dès qu'une nouvelle DLL native est envisagée, avant toute modification de code, configuration ou archive, puis pour le gate de destination, l'audit ABI/hooks et de coexistence, la configuration indépendante JSON ou TOML, les crédits RuffnecKk et le contenu strict du ZIP public.
---

# Incubation de plugin D2RLoader

## Appliquer le gate de destination

1. Poser d'abord uniquement cette question à Vincent : « Plugin autonome ou plugin à merger au PluginPack ? »
2. Attendre sa réponse explicite. Tant qu'elle manque, ne modifier ni code, ni configuration, ni archive.
3. Consigner dans la mission la destination retenue et la date de la décision.
4. Pour un plugin autonome permanent, ne demander ni catégorie PluginPack, ni DLL propriétaire future, ni clé de merge. Exiger une DLL indépendante, hybride et compatible avec les cinq plugins du pack, ainsi que son propre fichier de configuration indépendant. Choisir JSON ou TOML selon la convivialité réelle pour le moddeur; rédiger le contenu et les commentaires en anglais.
5. Pour un plugin destiné au merge, proposer ensuite `items`, `levels`, `misc`, `quests` ou `skills`, la DLL propriétaire correspondante et la clé `categorie.nomFonctionnalite`; attendre leur confirmation explicite avant l'implantation. Utiliser pendant l'incubation un JSON autonome compatible avec le futur bloc de `D2RPlugins.json`.

## Auditer avant de coder

1. Pour le build 92777, exécuter le skill `d2r32-reverse-engineering` et franchir son gate `status`.
2. Vérifier la référence PluginPack épinglée pour les deux destinations. Pour un autonome, inventorier les surfaces nécessaires à sa coexistence avec les cinq DLL; pour un merge, inventorier en plus le module propriétaire, ses fichiers, structures partagées, champs, configuration, callbacks, RVA et plages de hooks.
3. Identifier chaque collision potentielle et désigner un propriétaire unique pour tout hook ou structure canonique.
4. Si une incompatibilité avec le PluginPack est envisagée, arrêter l'implantation et présenter la collision à Vincent; ne pas changer silencieusement la destination retenue.
5. Refuser une implantation fondée sur une ABI, une signature ou un build non prouvé.

## Incuber de façon autonome

1. Conserver une DLL autonome attribuée exactement à `RuffnecKk` pendant toute l'incubation.
2. Rendre la DLL hybride : installation globale ou mod-locale, sans `ModScopedOnly`, avec les mêmes gardes strictes de build, signatures et ABI dans les deux portées.
3. Rédiger la description du plugin en anglais, en une phrase courte orientée effet joueur, sans build, RVA, hook ni ABI.
4. Rechercher la configuration dédiée d'abord dans le mod actif puis dans le dossier global du jeu. Refuser explicitement une configuration présente mais invalide.
5. Pour la voie autonome permanente, conserver le JSON ou TOML dédié sans annoncer de merge hypothétique. Pour la voie PluginPack, documenter le merge futur dans la DLL propriétaire et l'unique `D2RPlugins.json`, puis supprimer la DLL et le JSON autonomes après le merge validé.
6. Ne jamais modifier, lier ni redistribuer une DLL d'eezstreet.

## Valider et publier

1. Tester la politique, compiler en Release x64, vérifier la version, les exports D2RLoader et les hashes entre build, dépôt et runtime.
2. Valider séparément les portées globale et mod-locale, le repli de configuration, la coexistence avec les cinq DLL eezstreet et l'absence de plugins rejetés ou en échec.
3. Produire le ZIP public avec uniquement la DLL autonome et le fichier de configuration indépendant requis par la destination confirmée, JSON ou TOML. Exclure README, sources, symboles, logs et fichiers de preuve.
4. Inspecter la liste réelle des entrées du ZIP et calculer son SHA-256 avant de déclarer la livraison prête.
5. Créditer exactement `RuffnecKk` dans les sources, logs et documentation de la fonctionnalité. Si le plugin a nécessité des connaissances acquises grâce à D2MOO, créditer explicitement D2MOO dans le README du plugin, conservé avec la documentation du projet et hors du ZIP public strict. Lors d'un merge, préserver aussi les métadonnées et crédits du propriétaire eezstreet.

Lire [references/incubation-checklist.md](references/incubation-checklist.md) pour le choix de destination, la cartographie des propriétaires de merge et les gates d'audit, de runtime et d'archive.
