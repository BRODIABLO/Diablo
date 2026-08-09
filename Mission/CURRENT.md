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
séparé; son rollback sûr signifie désormais « aucun splash » plutôt qu'un
retour à l'ancien missile.

La qualification technique initiale couvre les portées globale, mod-locale avec
shadow et default-off. La correction `DBA0C40C...8856C` a ensuite reçu son propre
cold start mod-local activé : `18/18` patches, `17/17` plugins, `24/24`, zéro
disabled/rejected/failed. Après le smoke complémentaire, B, C et D passent
avec les stats exacts 391/392. Le rollback visuel de l'ancien montage a en
revanche déclenché l'assertion native `ptSkill->nItemEffect != 0`. Vincent a
donc autorisé son retrait complet de BKVince.

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

La nouvelle intégration BKVince est active sans gate : elle couvre toutes les
attaques melee joueur admissibles et conserve les stats 391/392. L'ancien
graphe stat 384/property 302/skills 430 et 432/missile 743 est retiré tout en
conservant ses numéros comme tombstones compatibles avec les sauvegardes. Les
références Summon Splash, Titan's Echo et sa treasure class sont supprimées.
Le migrateur gouverné est idempotent et son contrôle ciblé passe.

Le cold start final sur ces tables passe avec `18/18` patches, `17/17` plugins,
`24/24` et zéro disabled/rejected/failed. Un hit QtyTester sans stat 384 produit
une capture valide avec `gateSeen=false`, sans EventFunc20 ni ancienne
assertion. A, B, C, D, E, F et H possèdent leurs témoins; G est supersédé par
la décision produit de ne plus fournir de rollback vers le vieux missile. La
mission MeleeSplash est donc fermée; le prochain lot planifié demeure
Critical/Deadly. Multijoueur et PvP restent hors portée.

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
