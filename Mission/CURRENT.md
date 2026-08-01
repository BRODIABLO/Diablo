# Mission courante

Dernière mise à jour : 1 août 2026

## Priorité active

[BaseMod 3.2 — Charm Zone, services répétables et démarrage Players](base-mod-features-3.2.md)

État : `CharmZone.dll` 0.3.1 est compilé, empaqueté et prouvé sur le chemin
gameplay central en runtime mod-local. L'audit ferme deux candidats sans code :
le réglage natif BKVince persiste déjà Players 8 et `uniqueitems.txt` porte déjà
`nolimit=1` sur les uniques ordinaires. Le prochain développement possible est
`quests.repeatableServices`; PotionAutoPickup reste un audit de mesure.
Le premier relevé natif prouve le débit atomique de l'or, les consommations
gratuites Charsi/Larzuk/Anya par difficulté et la présence des quatre panneaux
client 3.2. Le chemin Akara est maintenant prouvé de bout en bout : filtre du
menu par quest flags, paquet client 0x39, callback serveur, transaction combinée
stats/skills et bookkeeping de la charge gratuite. Aucun code n'est implanté.

## Prochain gate

Identifier sous le build 92777 une couture de paiement après validation mais
avant mutation pour Charsi, Larzuk et Anya. Prouver ensuite le formatage localisé
du prix dépendant du niveau et la coexistence avec les owners PluginPack avant
toute implantation.

## Frontière Git

Limiter le premier lot à la mission, aux preuves RE gouvernées, au source
CharmZone, à son TOML, à son binaire BKVince et à son package candidat public.
MassID reste actif à son gate gameplay sans mélanger ses fichiers à ce chantier.
