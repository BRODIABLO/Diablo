# Mission courante

Dernière mise à jour : 8 août 2026

## Priorité active

[PD2 / Single Player Plus — inspiration gouvernée pour BKVince](pd2-inspiration-bkvince.md)

État : le premier merge General/QoL est implanté. Stockage et stacking restent
explicitement BKVince; `plugin-items` applique le cap élémentaire 90 depuis la
configuration mod-locale. Les validations statiques et le second cold start
sont verts (`16/16`, `18/18`, `24/24`), tandis que la matrice gameplay et le
témoin hôte/joiner restent ouverts.

## Prochain gate

Valider en jeu les maximums élémentaires sous, à et au-dessus de 90 pour feu,
froid, foudre et poison; confirmer physique 50 et absorb 40, puis couvrir
sauvegarde/rechargement et hôte/joiner. La matrice toutes fonctions PluginPack
actives reste ouverte avant la qualification fonctionnelle complète.

## Frontière Git

Le lot modifie uniquement les décisions gouvernées de la mission/catalogue et
`D2RPlugins.json`; aucune table, DLL, sauvegarde ou statistique persistante n'a
changé. Aucun commit ni push n'est effectué sans demande explicite de Vincent.
