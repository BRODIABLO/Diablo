---
name: d2rloader-service-governance
description: Auditer, justifier et préparer toute demande de nouveau service ou d'extension de service D2RLoader/PluginSDK à partir d'un consommateur réel, d'une migration prête et d'un gain mesurable. Utiliser ce skill pour évaluer, prioriser ou formuler une demande de service, ainsi que pour vérifier l'adoption d'un service livré; ne pas l'utiliser pour l'emploi ordinaire d'un service existant.
---

# Gouvernance des services D2RLoader

## Appliquer le principe « no consumer, no service »

Traiter une possibilité technique comme une hypothèse tant qu'un consommateur actif ne prouve pas sa valeur. Plusieurs consommateurs hypothétiques comptent comme zéro consommateur. Une API élégante, l'accord du mainteneur ou la livraison du service ne prouvent ni son adoption ni sa priorité.

Séparer toujours ces trois états :

- **livré** : le service existe dans le SDK ou le loader;
- **adopté** : un consommateur du workspace l'appelle réellement;
- **utile** : cette adoption retire un chemin existant ou produit un gain vérifié.

Ne jamais transformer une liste de frictions possibles en liste d'implantations prioritaires.

## Auditer les faits avant de concevoir l'API

Commencer par `npm run baseline:d2rloader -- status`. Si le service provient d'une release encore candidate, lire son audit `d2rloader-release-intake` et conserver son statut provisoire jusqu'à promotion de la baseline. Ne jamais recopier un numéro de loader ou de SDK dans ce skill.

Pour chaque proposition, établir à partir du code, des missions, des tests ou des logs :

1. le plugin ou workstream actif qui consommerait le service;
2. la fonctionnalité utilisateur déjà priorisée par Vincent ou le blocage actuellement observé;
3. le chemin courant exact : hook, RVA, scan, ABI privée, worker, copie, retry ou autre contournement;
4. ce que les services et outils existants couvrent déjà;
5. la limite précise qui empêche le consommateur de migrer aujourd'hui;
6. le code qui serait supprimé ou simplifié après migration;
7. le gain vérifiable et la façon de le mesurer.

Rechercher les appels dans les sources du consommateur, pas seulement dans les README, missions, dépendances de build, copies de SDK, dossiers de compilation, `_deps`, packages ou binaires. Une mention documentaire n'est pas une adoption.

Ne pas inférer la priorité d'une fonctionnalité parce que son architecture serait améliorée par un service. Si Vincent ne l'a pas priorisée et qu'elle ne bloque rien d'actif, classer la proposition comme capacité optionnelle.

## Franchir le gate de demande

Une demande prête à transmettre doit satisfaire toutes les conditions suivantes :

- au moins un consommateur actif est nommé comme pilote et son comportement actuel est prouvé;
- la fonctionnalité consommatrice est prioritaire ou réellement bloquée;
- le statu quo et les services existants ont été audités;
- la migration du consommateur est assez précise pour être implantée dès livraison;
- le service couvre toutes les surfaces indispensables au remplacement visé;
- le hook, RVA, worker, scan, ABI privée ou autre chemin à retirer est nommé;
- le bénéfice est mesurable en code retiré, surfaces natives supprimées, collisions évitées, stabilité ou compatibilité;
- les frontières de thread, ownership, durée de vie, unload, changement de session, autorité client/serveur et compatibilité sont fermées lorsqu'elles s'appliquent;
- le contrat reste minimal et versionnable, avec taille/capabilities vérifiables lorsque l'ABI l'exige;
- aucun `ServiceId` permanent n'est inventé : son allocation appartient au propriétaire du loader.

Un service partiel qui conserve obligatoirement un deuxième chemin divergent échoue le gate, sauf si ce repli a une valeur de compatibilité démontrée et testable.

Si une condition manque, ne pas demander au mainteneur d'implanter le service et ne pas le présenter comme prioritaire. Nommer les preuves manquantes. Ne concevoir une ABI exploratoire complète que si Vincent le demande explicitement.

## Classer honnêtement la proposition

Utiliser exactement l'un de ces verdicts :

- **blocage démontré** : aucune voie sûre ne couvre une fonctionnalité prioritaire, et la migration est prête;
- **amélioration de maintenance mesurée** : le chemin actuel fonctionne, mais le remplacement retire un coût concret et vérifiable;
- **capacité optionnelle** : idée plausible sans migration prête, couverture complète ou gain démontré;
- **à rejeter ou obsolète** : aucun consommateur réel, architecture dépassée, duplication suffisante de l'existant ou absence de gain.

Seuls les deux premiers verdicts peuvent devenir une demande d'implantation. Une amélioration de maintenance ne doit pas être décrite comme une révolution fonctionnelle.

## Préparer une contribution presque prête

Quand le gate passe, produire un kit compact :

1. consommateur exact, fichier ou symbole courant et scénario bloqué;
2. contrat minimal proposé et limite explicite de ce qui reste hors service;
3. règles d'ownership, thread, lifetime, unload, session et autorité;
4. stratégie de compatibilité, détection de capabilities et repli justifié;
5. forme ABI avec version/taille, validations et tests de conformité du Core;
6. patch ou plan de migration du consommateur qui retire le chemin précédent;
7. preuve attendue après adoption et métrique avant/après;
8. message bref au mainteneur présentant une migration prête, jamais une wishlist.

Pour une surface native, utiliser `d2r33-reverse-engineering` avant d'affirmer les RVA, hooks, signatures, layouts ou ABI. Pour prouver l'adoption en jeu, utiliser `d2r-runtime-validation`. Si la solution retenue crée une nouvelle DLL, appliquer ensuite `d2rloader-plugin-incubation`. N'utiliser `plugin-architect` que si Vincent l'invoque explicitement pour une revue générale.

## Vérifier la valeur après livraison

Dès qu'un service est livré :

1. rechercher ses appels réels dans les consommateurs;
2. migrer le consommateur prévu;
3. exécuter les validations ciblées puis runtime si le comportement l'exige;
4. mesurer le code, les hooks, les RVA, les workers ou l'ABI privée réellement retirés;
5. consigner séparément livraison, adoption et gain.

Si aucun consommateur ne l'utilise, conclure : **livré, inutilisé, gain démontré nul pour le workspace**. Ne pas présenter cette livraison comme le succès d'une priorité.

## Rendre une décision vérifiable

Présenter d'abord un tableau compact :

| Consommateur | Friction actuelle | Couverture du service | Chemin retiré | Gain mesurable | Verdict |
|---|---|---|---|---|---|

Puis distinguer :

- **faits vérifiés**;
- **hypothèses à tester**;
- **inconnues**;
- **recommandation démontrée**.

Si une demande a déjà été envoyée sans franchir le gate, préparer une correction qui la reclasse explicitement comme inventaire non priorisé et demande au mainteneur de ne pas investir de temps avant le retour d'un consommateur et d'une migration concrets.
