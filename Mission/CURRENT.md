# Mission courante

Dernière mise à jour : 10 août 2026

## Priorité active

[ProgressiveAffixesPlugin autonome — D2R 3.2.92777](progressive-affixes-plugin-3.2.md)

État : **implantation technique v0.1.0 et double cold start terminés, qualification gameplay ouverte**.
Vincent avait placé le plugin en option B dans la phase itemisation, puis a
donné le 10 août 2026 la directive explicite `Implemente`. Cette directive ouvre
le lot immédiatement sans modifier la destination confirmée : plugin autonome
permanent RuffnecKk, hybride global/mod-local, TOML indépendant et aucune
mutation d’une DLL eezstreet.

La DLL Release x64 et le TOML PD2 sont présents dans `addons/` et dans le profil
source BKVince. Les trois anciens patchsets force Magic/Rare/Crafted sont retirés
atomiquement. Deux clean builds `/Brepro` produisent la même DLL de 137 216
octets, SHA-256
`92481CF78DD3BAD2DAA33212A0561CD65CFAAFB786565BB6F62F527725EB9A8F`.
La suite CTest passe `1/1`; les métadonnées, trois exports D2RLoader et hashes
build/package/BKVince sont cohérents.

Les trois patchsets runtime obsolètes ont été archivés hors des dossiers actifs.
Deux cold starts frais passent sur le build 92777 : la portée mod-locale puis la
portée globale avec son propre repli de configuration chargent chacune
ProgressiveAffixesPlugin 0.1.0. Chaque démarrage conserve la pile de production
et termine avec `18/18` plugins actifs, `15/15` patchsets appliqués, zéro rejet
et zéro échec. La portée globale de test a été retirée et le profil BKVince
mod-local restauré avec les hashes source/runtime exacts; aucun processus Diablo
ne reste ouvert.

## Prochain gate

Produire les témoins gameplay qui déclenchent la résolution tardive des codes
`weap`, `armo`, `jewl`, `ring`, `amul`, `char`, puis vérifier les bornes et les
distributions réelles.

La qualification complète exige ensuite :

- toutes les fonctionnalités du PluginPack actives et aucun composant
  neutralisé ;
- deux ordres pertinents pour toute surface composable ;
- bornes Magic 64/65, jewelry 84/85 et charms 89/90 ;
- distributions Rare aux ilvl 1/45/65/85 et jewel toujours quatre ;
- distributions Crafted aux ilvl 1/31/51/71 ;
- sauvegarde/rechargement et hôte/joiner.

Les conflits full-stack préexistants `0x589736`, `0x314110` et
`0x18885B/0x18887F` restent étrangers au plugin et bloquent toute affirmation de
compatibilité universelle tant qu’ils ne sont pas fermés sans désactiver de
fonctionnalité.

## Frontière Git

La mission couvre `addons/ProgressiveAffixesPlugin/**`, son profil BKVince, la
mission, les preuves RE, la ROADMAP, le cadastre et l’archive publique stricte.
Les autres changements du workspace restent hors périmètre et sont préservés.
Aucun commit ni push n’est effectué sans demande explicite de Vincent.
