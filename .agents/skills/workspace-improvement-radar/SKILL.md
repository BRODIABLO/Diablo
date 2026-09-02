---
name: workspace-improvement-radar
description: Détecter et signaler, au fil du travail dans le Workspace RuffnecKk, une lacune réutilisable et démontrée d'automatisation, de planification, de réflexion, de méthodologie ou de validation qui pourrait justifier un nouveau skill. Utiliser lorsqu'une friction observée dépasse un incident ponctuel; ne pas l'utiliser pour transformer toute idée, tout bug ou toute tâche spécifique en skill.
---

# Workspace Improvement Radar

## Observer sans détourner le chantier

Agir comme un radar pendant le travail demandé, sans lancer spontanément un audit
général du workspace. Une lacune mérite une analyse lorsqu'elle provoque une
redécouverte répétée, une séquence manuelle fragile, du retravail, une décision
incohérente, une preuve manquante récurrente ou un risque important qu'une
procédure réutilisable réduirait réellement.

Un incident unique à corriger localement peut suffire seulement si son coût ou
son risque est élevé et si la même protection serait utile à plusieurs futurs
chantiers. Une possibilité plausible, une préférence personnelle ou une idée
sans friction observée ne constitue pas une lacune démontrée.

## Appliquer le gate anti-sauce

Avant toute suggestion :

1. Citer la preuve concrète rencontrée dans le travail courant.
2. Auditer la couverture existante dans `AGENTS.md`, `.agents/skills/`, les
   scripts, tests, missions et éléments pertinents de la ROADMAP.
3. Distinguer explicitement le fait vérifié, l'hypothèse à tester, la simple
   idée et la recommandation démontrée.
4. Choisir la plus petite correction durable : skill existant, documentation,
   règle, test, script, automatisation ou nouveau skill.
5. Réserver un nouveau skill à une lacune réutilisable de comportement agent,
   de décision ou de workflow. Ne pas emballer artificiellement un bug, une
   fonctionnalité produit ou un simple script dans un skill.
6. Nommer le gain attendu et une manière observable de le mesurer.

Si la preuve, la répétabilité, le gain ou l'absence de couverture ne sont pas
établis, rester silencieux ou conclure qu'aucun nouveau skill n'est justifié.
Lorsqu'un skill ou un chantier existant couvre déjà la lacune, router vers lui
au lieu de proposer un doublon.

## Signaler une lacune démontrée

Lorsqu'un nouveau skill est la correction recommandée, insérer ce signal sans
faire perdre le fil du chantier :

> **Hey buddy — on aura besoin d'un skill pour s'améliorer sur ce point : _[lacune constatée]_.**

Le faire suivre d'un dossier de décision très court :

- **Preuve :** observation qui révèle la lacune;
- **Couverture actuelle :** raison pour laquelle l'existant ne suffit pas;
- **Skill candidat :** nom provisoire, responsabilité unique et frontière;
- **Gain mesurable :** résultat attendu et méthode de vérification.

Émettre un seul signal par lacune distincte et regrouper les preuves liées. Ne
pas répéter une suggestion déjà suivie sans nouvelle preuve ou changement de
contexte. Ne jamais proposer un autre méta-skill dont le seul rôle serait de
détecter des skills manquants : ce radar possède déjà cette responsabilité.

Le signal n'autorise aucune implantation, automatisation, modification de la
ROADMAP ou autre mutation. Continuer le travail courant lorsqu'il reste sûr,
puis respecter les gates de discussion et de `GO` définis dans `AGENTS.md`.
