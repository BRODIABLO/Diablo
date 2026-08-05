# Mission courante

Dernière mise à jour : 5 août 2026

## Priorité active

[Four-Character Item Codes — D2R 3.2](four-character-item-codes-3.2.md)

État : correctif livré comme memory patch D2RLoader généraliste, sans DLL ni
configuration. Le remplacement statique de 26 octets est unique dans le build
92777, passe les cold starts mod-local et global, et le témoin `rk4x` affiche
son modèle HD ainsi que son infobulle après sauvegarde et relecture.

Le runtime a été restauré byte-exactement et Diablo est fermé. L’ancienne voie
PluginPack a été retirée; le fork revient à `139/139` écritures gouvernées et
`26/26` tests Release.

## Prochain gate

Le produit est prêt. Une validation hôte/joiner à deux clients reste un suivi
facultatif; le patch ne touche que le registre local des assets HD et les chemins
réseau quatre octets sont déjà prouvés statiquement.

## Frontière Git

Le dépôt principal porte la mission, les preuves gouvernées et
`addons/FourCharacterItemCodes/`. Aucun commit ni push n’est effectué sans
demande explicite de Vincent.
