# Extended Act Level IDs — D2R 3.2.92777

Dernière mise à jour : 26 juillet 2026

Statut : planifié après le prochain gate de preuve RemoteStash; incubation
confirmée, sans prototype, configuration ni archive.

## Décisions confirmées

- Vincent a confirmé la catégorie `levels` le 26 juillet 2026.
- La destination future est `plugin-levels.dll` et la clé prévue dans l’unique
  `D2RPlugins.json` est `levels.extendedActLevelIds`.
- Pendant l’incubation, conserver une DLL autonome hybride
  `ExtendedActLevelIds.dll`, attribuée exactement à `RuffnecKk`, sans TOML et
  sans modifier, lier ni redistribuer une DLL d’eezstreet.
- Vincent a retenu l’Option A : RemoteStash reste la mission courante jusqu’à la
  fermeture de son prochain gate de preuve; cette mission démarrera ensuite.
- Description anglaise prévue : `Allows new levels to belong to any act.`

## Besoin joueur et moddeur

Permettre à de nouveaux Level IDs d’appartenir aux actes 1 à 4 au lieu de
forcer toutes les nouvelles lignes à utiliser l’acte 5, sans casser la
génération des niveaux, les transitions, les waypoints, l’automap, les quêtes,
la sauvegarde ni la synchronisation client/serveur.

## Faits vérifiés

- D2R 3.2 fournit `actinfo.txt`, avec cinq lignes et notamment les références
  `classlevelrangestart` et `classlevelrangeend`; ces champs ne sont pas des
  bornes numériques explicites.
- Dans les données vanilla 3.2, les Level IDs sont contigus par acte : acte 1
  `0–39`, acte 2 `40–74`, acte 3 `75–102`, acte 4 `103–108` et acte 5
  `109–137`.
- BKVince ajoute les Rift Levels `138–146` avec `Act = 4`, soit l’acte 5. La
  friction est donc observée dans le mod actuel.
- BKVince ne fournit pas actuellement d’override `actinfo.txt` et hérite de la
  table 3.2.
- Le statut du workbench 92777 et celui des références épinglées devront être
  rejoués dans l’environnement Windows gouverné; le conteneur Linux courant ne
  dispose pas de PowerShell.

## Hypothèses à tester

- `ActInfo.txt` pourrait produire au chargement une structure de bornes par acte
  consommée par plusieurs sous-systèmes.
- La contrainte pourrait être une exigence de contiguïté plutôt qu’une borne
  maximale unique.
- Un override TXT correctement construit pourrait suffire; une DLL ne sera
  justifiée que si un consommateur natif conserve une limite non softcodée.
- Plusieurs consommateurs pourraient devoir être corrigés ensemble; aucun patch
  mono-octet ni RVA n’est supposé à ce stade.

## Gates observables

1. Franchir `npm run re:d2r32 -- status` sur l’environnement Windows gouverné.
2. Vérifier la référence PluginPack épinglée et inventorier `plugin-levels` :
   structures, configuration, callbacks, RVA et plages de hooks.
3. Identifier le chargeur 92777 d’`ActInfo.txt`, la structure produite et tous
   les consommateurs des plages d’actes.
4. Prouver séparément rôle, callers, ABI, signatures et éventuelles bornes ou
   exigences de contiguïté avant toute implantation.
5. Déterminer honnêtement si la solution est softcodée, un patch minimal, une
   extension de structure ou plusieurs hooks coordonnés.
6. Utiliser comme fixture technique un nouveau niveau après `146` rattaché à
   l’acte 1, sans conserver cette donnée dans BKVince avant validation.
7. Valider génération, transitions aller/retour, town/start, waypoints, automap,
   quêtes, portails, sauvegarde/rechargement, souris/manette, solo, hôte et
   joiner, avec zéro crash, corruption ou désynchronisation.
8. Seulement après ces preuves, compiler le prototype Release et valider les
   portées globale/mod-locale ainsi que la coexistence PluginPack.

## Prochain gate

Après la fermeture du prochain gate de preuve RemoteStash, vérifier le
workbench 92777 et la référence PluginPack, puis identifier le chargeur
d’`ActInfo.txt`, la structure native des plages d’actes et ses consommateurs
avant de décider si une DLL est réellement nécessaire.
