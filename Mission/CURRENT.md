# Mission courante

Dernière mise à jour : 9 août 2026

## Priorité active

[MeleeSplash.dll v0.1 public](melee-splash-3.2.md)

État : **livraison v0.1.0 fermée**. Vincent a confirmé la destination
**plugin autonome permanent** puis a
autorisé l'implantation de la v0.1.0. Le candidat technique public, générique et
default-off est maintenant compilé en Release x64. Deux builds propres ont
produit la même DLL de 196 608 octets, SHA-256
`D9A49607C0BA7EFF2E52200ED480EF8381739161F4E4B2BDE066D548554921B0`, et ont
chacun passé le CTest `1/1`. L'inspection PE/ABI confirme l'API D2RLoader v2,
les trois exports exacts, l'absence de dépendance eezstreet et l'absence de nom,
chemin ou ID BKVince obligatoire. L'intégration BKVince demeure un profil
séparé et réversible.

La qualification technique fraîche charge ce même hash en portée globale
activée, puis en portée mod-locale activée avec shadow du doublon global. Les
deux parcours acceptent les hooks, appliquent `18/18` patches, conservent les
17 plugins effectifs actifs, atteignent `24/24` et ne produisent aucun rejet ni
échec. Le cold start final charge la DLL mod-locale avec `enabled=false`,
n'installe aucun hook et conserve `18/18`, `17/17`, `24/24`; les artefacts
globaux temporaires sont retirés.

Le ZIP public strict `MeleeSplash-0.1.0.zip` est construit et audité : deux
entrées racine seulement, la DLL qualifiée et le JSON générique default-off.
Son SHA-256 vaut
`D53A36974A61B3909733F9F5CBFB496211EF820F36DBFEB788660A2FCF17183B`.

Le paquet offensif pré-critique est partagé entre les cibles; Critical/Deadly,
Crushing Blow et Open Wounds sont roulés indépendamment pour chaque secondaire.
L'adaptateur Critical/Deadly de la v0.1 reproduit seulement l'autorité native
92777 et reste remplaçable par le futur lot général PD2.

Le périmètre officiel initial est D2R 3.2.92777, offline/local single-player,
joueur contre monstres et attaques melee admissibles. Multijoueur, PvP,
mercenaires, summons, monstres comme attaquants et skills multi-hit non
explicitement autorisés sont hors portée ou non testés dans cette mission.

## Prochain gate

La prochaine implantation planifiée est le lot général Critical/Deadly, mais
elle attend une autorisation distincte de Vincent. Le protocole solo A–H de la
v0.1 peut être exécuté lors d'une future fenêtre gameplay; il est préparé mais
`not run`, donc aucun comportement splash n'est déclaré validé en jeu.
Multijoueur et PvP demeurent hors portée.

Le cold start prouve la coexistence avec la pile actuellement installée, pas une
compatibilité universelle ni une matrice où toutes les fonctionnalités
PluginPack seraient simultanément activées. Cette matrice formelle reste bloquée
par des conflits antérieurs indépendants de MeleeSplash : `0x589736`,
`0x314110` et les rel32 `0x18885B/0x18887F` de `plugin-misc`.

## Supersession du prototype historique

Le prototype gelé le 8 août reste une hypothèse historique non probante et ses
hashes ne valident pas la nouvelle v0.1. La décision du 9 août lève uniquement
le gel de **production** pour une implantation gouvernée distincte. Elle ne
promeut aucune de ses anciennes RVA ou ABI, ne prouve aucune couture commune et
ne sélectionne pas `Pd2CombatCore`. Seul le reverse engineering ciblé requis
par une surface précise est autorisé; plusieurs coutures individuellement
validées peuvent être utilisées si la composition reste fail-closed.

## Frontière Git

La mission peut modifier les sources et artefacts MeleeSplash, son profil
BKVince, les seules lignes TXT/statistiques nécessaires, les documents
Mechanics et les scripts/tests ciblés. Les autres changements du workspace
restent hors périmètre et doivent être préservés. Aucun commit ni push n'est
effectué sans demande explicite de Vincent.
