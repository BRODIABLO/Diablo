# Laboratoire local Cast Triggers 0.1.0

Ce laboratoire teste Cast Triggers dans la pile active complète BKVince avec
QtyTester. Ce n'est pas un profil isolé. La qualification gameplay courante
s'exécute une seule fois sous D2R 3.3.93847; le build 3.2.92777 hérite seulement
de la couverture native byte-exact gouvernée tant que toutes les surfaces
utilisées demeurent identiques.

## Préparer le personnage

1. Lancez le laboratoire voulu.
2. Créez une **nouvelle Sorceress hors ligne**. L'option `-act5` du lanceur
   s'applique seulement aux nouveaux personnages.
3. Le personnage reçoit un Horadric Cube et les principaux ingrédients; les
   potions manquantes s'achètent chez un marchand.
4. Attribuez les points de compétence aux sorts que vous voulez tester.

Si `-act5` n'initialise pas correctement le nouveau personnage, arrêtez le test
et conservez les logs. Une copie lab-only d'un personnage pourra alors être
préparée sans toucher à l'original.

## Créer les bagues de test

Placez un seul ingrédient dans le Cube et transmutez :

| Ingrédient | Bague produite |
|---|---|
| Identify Scroll | 100% chance to cast level 12 Fire Ball |
| Stamina Potion | 100% chance to cast Nova at the source skill level |
| Antidote Potion | Channel/chain gate: Fire Ball CTC with level 10 Inferno and Chain Lightning oskills |
| Minor Healing Potion | Custom 100% Cast level 12 Fire Ball on Attack Attempt |
| Minor Mana Potion | 100% passive Critical plus 100% Cast Fire Ball on Critical Strike |
| Light Healing Potion | 100% Deadly Strike plus Cast Fire Ball on Critical Strike; negative gate |
| Light Mana Potion | 100% Crushing Blow plus 100% Cast Nova on Crushing Blow |
| Healing Potion | 100% Open Wounds plus 100% Cast Frost Nova on Open Wounds |
| Mana Potion | 100% Critical, Cast Fire Ball on Critical and an unmatched Cast Nova on Crushing Blow |

Le Town Portal Scroll n'est l'entrée d'aucune recette du laboratoire. La bague
same-level utilise bien une **Stamina Potion** afin de ne pas intercepter la
recette de clue scroll d'un autre mod.

Identifiez la bague si son affixe n'est pas immédiatement visible. Équipez une
seule bague de test à la fois afin de garder les observations non ambiguës.

## Matrice de gameplay

| Cas | Action suggérée | Résultat attendu |
|---|---|---|
| Tooltip fixe | Inspecter Fire Ball ou Blizzard | Le niveau 12 est visible |
| Tooltip same-level | Inspecter Nova ou Frost Nova | La ligne complète est visible et mentionne le niveau du sort source |
| Régression cast directionnel | Lancer Chain Lightning vers plusieurs unités/points avec la bague Fire Ball | Une Fire Ball suit la cible native de chaque cast |
| Exclusion attaque du cast-on-cast | Attaquer normalement avec la bague Antidote | Aucun Fire Ball de `cast-skill` |
| Cast on Attack Attempt | Avec la bague Minor Healing, attaquer une cible, manquer puis Shift-attaquer le sol | Une Fire Ball est tentée dès que chaque attaque est acceptée, sans attendre un hit |
| Channeling fixe | Maintenir Inferno au moins 6 secondes avec la bague Antidote | Fire Ball proc immédiatement, puis au maximum une fois toutes les 2 secondes |
| Channeling same-level | Ajouter la bague Nova same-level et maintenir Inferno | Nova utilise le niveau effectif d'Inferno à chaque intervalle admissible |
| Arrêt du channeling | Relâcher Inferno après un proc | Aucun autre proc après l'arrêt |
| Séquence | Lancer Lightning ou Chain Lightning | Un seul dispatch par cast réussi |
| Critical positif | Frapper avec la bague Minor Mana | Chaque Critical confirmé lance une Fire Ball sur la cible |
| Deadly exclu | Frapper à mains nues avec la bague Light Healing | Deadly double les dégâts, mais aucune Fire Ball de Critical n'est lancée |
| Crushing Blow | Frapper avec la bague Light Mana | Chaque Crushing Blow appliqué lance une Nova |
| Open Wounds | Frapper avec la bague Healing | Chaque Open Wounds appliqué lance une Frost Nova |
| Filtrage combat | Frapper avec la bague Mana | Fire Ball proc sur Critical; Nova ne proc pas sans Crushing Blow |
| Chaîne de procs | Porter simultanément les bagues Mana et Antidote, puis attaquer | La Fire Ball de Critical ne déclenche pas le cast-on-cast de la seconde bague |

La bague créée avec une **Antidote Potion** sert aux gates d'éligibilité. Elle fournit Inferno et Chain Lightning comme oskills ainsi qu'une
chance de 100% de lancer Fire Ball niveau 12. Une attaque normale ne doit pas
lancer Fire Ball. Maintenir Inferno doit lancer Fire Ball immédiatement, puis
pas plus d'une fois toutes les 50 frames serveur. Un cast de Chain Lightning
doit lancer exactement une Fire Ball, et cette Fire Ball déclenchée ne doit pas
amorcer une nouvelle chaîne de procs.

Le laboratoire ne contient volontairement aucun cas gameplay 25% séparé. Les
gates sont déterministes à 100%; les chances intermédiaires utilisent toujours
l'encodage et le jet natifs.

## Diagnostics

La configuration du laboratoire active les diagnostics. Dans la console
D2RLoader, la commande suivante affiche les compteurs :

```text
cast-triggers
```

Le log du plugin se trouve dans :

```text
<D2R>/mods/<CastTriggersLab>/d2rloader/logs/ruffneckk-cast-triggers.log
```

Les lignes `input captured`, `eligible source`,
`descriptor-source=input|channel-input|handler`,
`target=unit|position|none`,
`requested-level`, `effective-level`, `native-position` et
`native-unit-target` permettent de confirmer le descripteur et le chemin
réellement consommés. Les compteurs `channel ticks`, `channel dispatches`,
`channel throttled`, `inputs captured`, `inputs consumed`, `inputs expired` et
`channel targets reused` permettent de vérifier que les jets sont séparés par
au moins 50 frames, suivent l'input natif et s'arrêtent avec Inferno. Les compteurs `critical`,
`crushing-blow`, `open-wounds`, `combat dispatches`, `chains suppressed` et
`combat stats filtered`, ainsi que les lignes
`dispatched combat trigger=attack-attempt`, couvrent le gate de combat et de
récursion.

## Isolation et rollback

Le laboratoire n'écrit pas dans BKVince. Il possède son propre dossier de mod,
sa propre configuration et son propre `savepath`. Pour le retirer, fermez le
jeu puis déplacez le dossier `CastTriggersLab...` et son lanceur hors de
l'installation. Conservez le dossier de sauvegarde jusqu'à ce que les résultats
aient été archivés.

Un cold start ou un test gameplay ne doit jamais être déclaré réussi sans une
observation de la session courante. À la création initiale du laboratoire, ces
cases restent donc `not run` tant que Vincent n'a pas autorisé le lancement.
