# Mission courante

Dernière mise à jour : 9 août 2026

## Priorité active

[MeleeSplash.dll v0.1 public](melee-splash-3.2.md)

État : **v0.1.0 corrigée et fonctionnelle sur le smoke nominal**. Vincent a confirmé la destination
**plugin autonome permanent** puis a
autorisé l'implantation de la v0.1.0. Le candidat technique public, générique et
default-off est maintenant compilé en Release x64. Deux builds consécutifs de
la correction ont produit la même DLL de 199 168 octets, SHA-256
`DBA0C40C191B2568A6B39D21324A45F770C1CBF8AD747B099AA3BCBEDEF8856C`, et ont
chacun passé le CTest `1/1`. L'inspection PE/ABI confirme l'API D2RLoader v2,
les trois exports exacts, l'absence de dépendance eezstreet et l'absence de nom,
chemin ou ID BKVince obligatoire. L'intégration BKVince demeure un profil
séparé et réversible.

La qualification technique initiale couvre les portées globale, mod-locale avec
shadow et default-off. La correction `DBA0C40C...8856C` a ensuite reçu son propre
cold start mod-local activé : `18/18` patches, `17/17` plugins, `24/24`, zéro
disabled/rejected/failed. La configuration runtime finale est de nouveau
`enabled=false` et les artefacts globaux temporaires restent retirés.

Le ZIP public strict `MeleeSplash-0.1.0.zip` est construit et audité : deux
entrées racine seulement, la DLL qualifiée et le JSON générique default-off.
Son SHA-256 vaut
`F137F1B708A4C51C8A88EA68B49BFB85619F380693E63899B508EB1C342E35A9`.

Le paquet offensif pré-critique est partagé entre les cibles; Critical/Deadly,
Crushing Blow et Open Wounds sont roulés indépendamment pour chaque secondaire.
L'adaptateur Critical/Deadly de la v0.1 reproduit seulement l'autorité native
92777 et reste remplaçable par le futur lot général PD2.

Le périmètre officiel initial est D2R 3.2.92777, offline/local single-player,
joueur contre monstres et attaques melee admissibles. Multijoueur, PvP,
mercenaires, summons, monstres comme attaquants et skills multi-hit non
explicitement autorisés sont hors portée ou non testés dans cette mission.

## Prochain gate

Le premier smoke a découvert que l'attaque normale revient de
`FillDamageValues` à `0x4300BB`, alors que le filtre n'admettait que le chemin
queued `0x44B6A0`. La signature unique et la chaîne Prepare/Allocate/Consume ont
été gouvernées, puis le filtre a été corrigé pour ces deux continuations
exactes. Le témoin suivant passe A avec un burst sur trois secondaires; E, F,
H et G-actif passent aussi : Critical/Deadly, CB et OW sont indépendants par
cible, la primaire n'est pas frappée deux fois et la récursion est absente.
B, C, D et le rollback visuel default-off de G restent `not run`. La config
default-off et QtyTester sont restaurés byte-exact; aucun processus D2R ne reste
actif. Multijoueur et PvP demeurent hors portée.

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
