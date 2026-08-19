---
name: plugin-architect
description: Produire une revue d'architecture D2R approfondie, explicite et strictement read-only avant toute implantation. Utiliser uniquement lorsque Vincent invoque `$plugin-architect` pour choisir ou comparer un mecanisme, analyser ses compromis et preparer une decision; ne pas utiliser pour implanter, qualifier ou emballer une DLL deja retenue.
---

# Plugin Architect

## Tenir le contrat de decision

1. Lire les fichiers `AGENTS.md` applicables et les sources gouvernees pertinentes avant toute affirmation technique.
2. Reformuler l'effet visible recherche, les contraintes et la decision que Vincent cherche reellement a prendre.
3. Expliquer le comportement actuel, les preuves et les compromis avant de recommander une architecture.
4. Ne mentionner mission, ROADMAP, release ou Git que si cela change materiellement la decision.

## Rester strictement read-only

1. Ne modifier aucun fichier versionne, aucune configuration, aucune archive ni aucun artefact de build.
2. Ne rien deployer dans un profil runtime, ne pas lancer le jeu et ne modifier aucune installation externe.
3. Autoriser uniquement les diagnostics read-only et les rafraichissements locaux obligatoires imposes par `AGENTS.md`, comme le checkpoint automatique.
4. Ne jamais interpreter l'approbation d'une architecture comme une autorisation d'implanter.
5. Respecter le gate `GO` defini par `AGENTS.md` pour toute mutation ulterieure.
6. Ne pas appliquer `d2rloader-plugin-incubation` comme workflow actif tant que le mecanisme reste ouvert. Si la decision retient une nouvelle DLL, identifier ce skill comme gate obligatoire du futur chantier.

## Classer le mecanisme avant de concevoir

Determiner d'abord si l'effet releve principalement de :

- tables TXT, donnees ou assets;
- composition locale D2RMM;
- memory patch JSON D2RLoader;
- DLL D2RLoader autonome membre de la RuffnecKk D2RLoader Suite;
- application ou outillage du workspace;
- combinaison justifiee de plusieurs mecanismes.

Expliquer pourquoi le mecanisme retenu correspond mieux au besoin que les autres. Ne pas appeler generiquement chaque solution un "plugin".

## Router vers les preuves gouvernees

1. Pour une DLL candidate, verifier seulement que l'option peut respecter les invariants RuffnecKk Suite d'`AGENTS.md`. Si la decision finale retient cette DLL, identifier `d2rloader-plugin-incubation` comme gate obligatoire apres `GO`; ne pas lancer son workflow operationnel pendant la revue.
2. Pour un hook, une ABI, une signature, un RVA ou un memory patch visant le runtime courant D2R.exe 3.3.93847, appliquer `d2r33-reverse-engineering` et commencer par son gate `status`. L'identité binaire utile avec le corpus 92777 étant établie, ses preuves gouvernées sont directement réutilisables.
3. Pour toute lecture de table TXT/TSV ou analyse de schema, appliquer `diablo-tsv` et respecter les sources read-only.
4. Pour D2RMM, traiter sa sortie comme une composition locale a auditer, jamais comme la source de verite de BKVince.
5. Concevoir la future qualification runtime selon `d2r-runtime-validation`, sans l'executer pendant cette revue read-only.
6. Ne pas recopier dans cette analyse les procedures detaillees de ces skills; les utiliser comme autorites specialisees.

## Construire le dossier de decision

1. Expliquer le comportement et l'architecture actuels avant de proposer un changement.
2. Tenir un registre explicite distinguant :
   - **fait verifie** : directement soutenu par une source du depot ou une preuve technique;
   - **inference** : conclusion raisonnable tiree des faits, clairement annoncee;
   - **hypothese a tester** : proposition falsifiable assortie de la preuve attendue;
   - **inconnue** : information absente qui interdit encore une conclusion;
   - **simple idee** : possibilite non priorisee et non demontree;
   - **recommandation demontree** : choix rattache a des preuves, des contraintes et un gain verifiable.
3. Citer les fichiers, tables, lignes, colonnes, cles JSON, fonctions, hooks et dependances exacts seulement lorsqu'ils sont prouves.
4. Pour toute surface non prouvee, nommer le candidat, le niveau de confiance et la preuve necessaire. Ne jamais inventer une precision pour completer la presentation.
5. Identifier la friction observee que la fonctionnalite corrige et auditer ce que l'outillage actuel couvre deja.
6. Si le besoin ou le gain n'est pas demontre, recommander honnetement de ne rien construire pour le moment.

## Comparer les conceptions viables

1. Presenter de deux a quatre approches lorsque plusieurs options sont reellement viables.
2. Inclure le statu quo lorsque l'absence de changement constitue une option credible.
3. Si une seule approche resiste aux preuves, le dire clairement au lieu d'inventer des alternatives faibles.
4. Expliquer le fonctionnement de chaque approche et son chemin d'execution etape par etape.
5. Comparer chaque approche sur les dimensions pertinentes :
   - exactitude fonctionnelle et effet joueur;
   - compatibilite avec le build cible;
   - coexistence, ordre de chargement et proprietaire des hooks;
   - persistance, sauvegardes, migration et retour arriere;
   - multijoueur et autorite client, hote ou serveur;
   - equilibrage et possibilites d'abus;
   - configuration et experience du moddeur;
   - maintenance et dependances;
   - complexite d'implantation;
   - performance;
   - risque de regression;
   - reversibilite et rayon d'impact.
6. Ne jamais presenter une compatibilite runtime comme prouvee par une analyse statique ou un simple cold start.
7. Pour les plugins natifs, analyser la coexistence avec tous les composants actifs de la RuffnecKk D2RLoader Suite et toutes les fonctionnalites du PluginPack, sans proposer d'en desactiver une pour obtenir artificiellement un demarrage.

## Recommander et preparer la suite

1. Recommander une approche principale avec une justification franche et explicite.
2. Dire pourquoi ses compromis sont preferables dans le contexte de Vincent, pas seulement pourquoi elle est techniquement possible.
3. Donner un plan d'implantation ordonne avec les surfaces impactees et les gates requis, sans l'executer.
4. Donner un plan de validation technique, fonctionnelle, runtime, multijoueur et de regression proportionne au risque.
5. Donner un plan de rollback qui precise ce qui serait retire, restaure ou migre.
6. Terminer par les decisions materielles encore ouvertes et les preuves techniques manquantes.
7. Ne poser une question que si sa reponse change materiellement l'architecture; poursuivre l'analyse avec des hypotheses explicites chaque fois que cela reste sur. Si une DLL est retenue, terminer en indiquant le passage obligatoire par `d2rloader-plugin-incubation` apres `GO`.
