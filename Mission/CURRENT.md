# Mission courante

Dernière mise à jour : 27 juillet 2026

## Priorité active

[Rift Terror Zone Group — D2R 3.2](rift-terror-zone-group-3.2.md)

État : Vincent a retenu l’Option A le 27 juillet 2026 et le correctif data est
maintenant déployé techniquement. Les Level IDs `138–146`, les deux zones
`Act5-Rifts`, le nouveau groupe `Act 5 - Rift` et la chaîne `RiftGroup = The
Rifts` passent le validateur ciblé. À la demande explicite de Vincent,
`MooMooFarmGroup` reprend maintenant les traductions de `Moo Moo Farm`, soit
`The Secret Cow Level` en anglais, sans code de couleur factice. Les fichiers
runtime ont des hashes identiques aux sources et le dernier cold start BKVince a
produit 18 logs et 58 lignes frais sans erreur.

Le correctif reste entièrement softcodé : la copie gouvernée de la table vanilla
3.2 ajoute un seul groupe spécial indépendant, sans toucher aux Level IDs, à leur
acte, aux monstres, aux bonus ni à la sélection des Terrozones. Seules les
chaînes de groupe affichées sont corrigées pour Rift et Cow. Le test ciblé, la
suite complète `verify:data`, la mission, le registre des workstreams et le
cadastre sont verts.

## Prochain gate

Confirmer `THE RIFTS` et `THE SECRET COW LEVEL` lors des prochaines sélections
concernées, puis contrôler les mêmes affichages en hôte/joiner.

## Frontière Git

Le lot Rift comprend sa mission, son entrée ROADMAP, son workstream, le
générateur/validateur ciblé, `levelgroups.txt`, la nouvelle entrée de
`levels.json`, la correction minimale de `desecratedzones.json`, le schéma et les
nœuds cadastre associés. Les changements concurrents de Vendor Stock Refresh,
Transmogrify, Readable Items, Extended Item Stats et des autres chantiers sont
préservés. Le registre assigne ce périmètre à `rift-terror-zone-group`.
