# Mission courante

Dernière mise à jour : 8 août 2026

## Priorité active

[PD2 / Single Player Plus — inspiration gouvernée pour BKVince](pd2-inspiration-bkvince.md)

État : deux lanes coordonnées avancent sans se contaminer. Le gate runtime
prioritaire reste la validation gameplay du cap élémentaire 90 déjà implanté;
MEC-00 commence en parallèle avec des contrats et preuves natives strictement
statiques pour D2R 3.2.92777. MEC-01 reste bloqué jusqu'à la fermeture du gate
cap 90 et n'utilisera ensuite que des témoins runtime read-only.

## Prochain gate

Valider en jeu les maximums élémentaires sous, à et au-dessus de 90 pour feu,
froid, foudre et poison; confirmer physique 50 et absorb 40, puis couvrir
sauvegarde/rechargement et hôte/joiner. La matrice toutes fonctions PluginPack
actives reste ouverte avant la qualification fonctionnelle complète. En
parallèle, le workbench 92777 est désormais vérifié (`status` et `self-test`
verts avec les hashes gouvernés). MEC-00 consolide `mechanics-contracts.md` et
`mechanics-native-proof-92777.md`, puis n'admet dans `known-rvas.json` que les
identifications indépendamment prouvées.

## Gel probatoire MeleeSplash

Le prototype existant `MeleeSplash` est conservé mais gelé comme hypothèse non
probante : aucun nouveau build, déploiement, test gameplay ou usage comme
témoin MEC-01. Ses hooks, RVA, structures et ABI doivent être redémontrés sans
s'appuyer sur son comportement. Aucune sélection de `Pd2CombatCore`, nouvelle
DLL gameplay ou formule de combat n'est autorisée par MEC-00/MEC-01.

## Frontière Git

Le lot de gouvernance MEC modifie uniquement la mission, la ROADMAP et les
preuves documentaires; aucune table gameplay, configuration active, DLL,
sauvegarde, statistique persistante ou formule de combat ne change. Aucun
commit ni push n'est effectué sans demande explicite de Vincent.
