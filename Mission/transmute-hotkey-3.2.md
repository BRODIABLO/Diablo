# Transmute Hotkey — D2R 3.2

## Statut et séquencement

- Statut : **planifié; aucune implantation commencée**.
- Cible éventuelle : `D2R.exe 3.2.92777` sous D2RLoader.
- Vincent confirme le 27 juillet 2026 l'Option A : inscrire la mission sans
  remplacer la priorité courante. Vendor Stock Refresh a ensuite été déclaré
  réglé et Cube Quick Move Bottom-Right est devenu la mission active, avec
  Equipped Item to Cube placé juste après. Transmute Hotkey reste donc en pause
  jusqu'à une demande explicite de reprise.
- Vincent confirme le 27 juillet 2026 la catégorie `misc`, la DLL propriétaire
  future `plugin-misc.dll` et la clé prévue `misc.transmuteHotkey`.
- Pendant l'incubation, le prototype éventuel sera `TransmuteHotkey.dll`, une DLL
  autonome hybride globale/mod-locale attribuée exactement à `RuffnecKk`.

## Intention joueur et gain mesurable

Déclencher au clavier le bouton natif **Transmute** du Horadric Cube afin qu'un
clic de souris ne soit plus obligatoire, sans retirer ni altérer le bouton, son
tooltip, le son, la navigation manette ou les validations natives.

La friction est directement observée dans la demande de Vincent du 27 juillet
2026 : chaque transmutation impose actuellement un déplacement et un clic de
souris. Le gain attendu est observable : une pression valide du hotkey doit
produire exactement une activation native de `Transmute`, et une pression
invalide ou répétée ne doit produire aucune transaction.

## Faits vérifiés

- Le workbench gouverné du build `92777` est vérifié : image canonique SHA-256
  `CC59119DC2A6C7D43D088098FC162EAFA4AE1299B2079126AEF43C1ACA914715`,
  image d'analyse SHA-256
  `673E8C0B2E89563E75525B24D137098EFD07B2DB4ED42ADEC56AA1ADDF0E63AB`,
  index SQLite et projet Ghidra persistants présents.
- Aucune identification stable `transmute` n'est encore promue dans
  `known-rvas.json`. Les identifications Cube actuellement gouvernées concernent
  la page d'inventaire `3` et les primitives de placement d'objet, pas le
  dispatcher du bouton Transmute.
- Le layout BKVince
  `data/global/ui/layouts/horadriccubelayouthd.json:43-49` déclare le
  `ButtonWidget` visible `convert` et son message
  `HoradricCubePanelMessage:Convert`.
- Le layout BKVince
  `data/global/ui/layouts/bankexpansionlayouthd.json:1350-1356` déclare un second
  bouton `convert` pour le Cube intégré au stash et son message distinct
  `BankPanelMessage:Convert`. La variante manette intégrée expose le même message.
- Le dépôt possède déjà des précédents d'entrée clavier :
  `BulkSkillPointAllocation` contrôle l'état natif et Win32 des touches, tandis
  qu'`ExtendedItemStats` installe un hook clavier borné. Ces précédents prouvent
  la disponibilité de mécanismes, pas leur adéquation automatique à Transmute.
- `RemoteStash` utilise déjà un broker d'interception des messages UI autour du
  dispatcher `0x843D90`. Tout partage futur de ce site doit conserver un
  propriétaire unique et une chaîne d'intercepteurs auditée.
- La référence PluginPack épinglée
  `eezstreet/D2RL-Plugins@dc75b49ffbb67b887d7757ee00ee9a03bcde5d8a`
  est propre et vérifiée. Son `README.md:30` réserve `plugin-misc.dll` aux
  changements divers; `src/plugin-misc/misc-main.cpp:116-140` expose son
  manifeste et charge la section `misc` de l'unique `D2RPlugins.json`.
- Les recherches gouvernées `transmute` et `hotkey` dans cette référence ne
  retournent aucun comportement existant à réutiliser directement.

## Hypothèses à tester

- La voie la plus sûre devrait résoudre le widget `convert` réellement visible
  et activé, puis déclencher son message embarqué comme le ferait le clic natif.
  Cette approche couvrirait les deux messages BKVince sans inventer de paquet ni
  dupliquer la logique Cube; le dispatcher exact, l'ABI et les états du widget
  restent à prouver sous 92777.
- Le hotkey peut probablement être détecté sur un front montant et délégué au
  thread UI, mais le mécanisme exact doit éviter les hooks clavier globaux
  concurrents et toute exécution hors du thread attendu par le jeu.
- Si le clic natif route déjà toute transmutation vers l'autorité de l'hôte, le
  hotkey ne devrait introduire aucun nouveau protocole réseau. Cette équivalence
  doit être confirmée par les appels et par une session hôte/joiner.

## Inconnues et risques

- Touche ou combinaison par défaut, représentation JSON et politique de conflit
  avec les raccourcis du jeu et des autres plugins RuffnecKk.
- Détection exacte des panneaux Cube standalone et intégré, du widget visible,
  de son état enabled/disabled et du focus clavier actif.
- Blocage requis lorsque le chat, une console, un champ texte, un écran modal ou
  un autre contexte capturant le clavier est actif.
- Effet d'une répétition Windows, d'une touche maintenue, de pressions rapides ou
  simultanées avec un clic et d'un changement de panneau pendant l'événement.
- Équivalence du message synthétisé avec le clic pour le son, l'animation,
  l'actualisation de la grille, le refus sans recette et les erreurs serveur.
- Collision potentielle avec le dispatcher UI partagé, les hooks clavier
  existants ou le futur merge de RemoteStash et Vendor Stock Refresh dans
  `plugin-misc.dll`.

## Architecture gouvernée

- Préférer l'activation du bouton `convert` visible et activé à un appel direct
  d'une routine Cube ou à la fabrication d'un paquet réseau.
- Déclencher une seule activation par front montant; une touche maintenue ne doit
  jamais transmuter plusieurs fois sans relâchement.
- Échouer sûrement si le panneau, le widget, son message, le thread UI ou les
  octets attendus ne sont pas prouvés. Ne jamais essayer un second chemin après
  une activation partielle.
- Si une DLL est nécessaire, incuber `TransmuteHotkey.dll`, autonome, hybride
  globale/mod-locale, sans `ModScopedOnly`, avec les mêmes contrôles stricts de
  build, signatures et ABI dans les deux portées.
- Utiliser `TransmuteHotkey.json`, recherché d'abord dans le mod actif puis dans
  le dossier global. Le fichier et ses commentaires seront en anglais; une
  configuration absente utilisera des valeurs sûres et une configuration
  présente mais invalide sera refusée explicitement. Ne créer aucun TOML.
- Description courte prévue :
  `Triggers the visible Horadric Cube transmute action from a configurable hotkey.`
- Le merge futur rejoindra `plugin-misc.dll` et l'unique `D2RPlugins.json` sous
  `misc.transmuteHotkey`. L'autonome et son JSON ne seront supprimés qu'après
  validation du binaire fusionné.

## Gates observables

1. **Séquencement — ouvert** : reprendre cette mission uniquement sur demande
   explicite de Vincent, sans remplacer Cube Quick Move Bottom-Right ni la file
   Equipped Item to Cube par inférence.
2. **Contrat du hotkey — ouvert** : fixer la combinaison par défaut, la syntaxe
   JSON, les modificateurs autorisés, la détection de conflits et la politique
   de consommation de l'événement.
3. **Preuve UI 92777 — ouvert** : borner les deux chemins de configuration et de
   dispatch des widgets `convert`; prouver fonctions, xrefs, ABI, état
   visible/enabled, thread d'appel, octets attendus et plage de hook éventuelle.
4. **Audit PluginPack — ouvert** : inventorier dans `plugin-misc` les fichiers,
   structures, callbacks, configurations, RVA et plages; auditer le broker UI et
   les hooks clavier locaux, puis désigner un propriétaire unique par site.
5. **Prototype autonome — ouvert** : compiler Release x64, vérifier le manifeste
   v2, les trois exports, l'auteur `RuffnecKk`, la description, le JSON strict et
   les portées globale/mod-locale sans TOML.
6. **Entrée et états UI — ouvert** : une pression produit une activation;
   maintien, répétition et rebond n'en produisent pas; chat, console, texte,
   modales, panneaux absents et widgets désactivés sont refusés.
7. **Fonctionnel Cube — ouvert** : couvrir recette valide, aucune recette,
   ingrédients invalides, sortie stackable/non-stackable, Cube plein, pressions
   rapides, clic simultané, son, animation, grille et fermeture du panneau.
8. **Compatibilité — ouvert** : standalone Cube, Cube intégré au stash, layouts
   BKVince et tiers, plusieurs résolutions, souris/manette inchangées, solo,
   hôte/joiner, sauvegarde/rechargement, portées globale/mod-locale et coexistence
   avec les cinq DLL eezstreet, sans perte, duplication, crash ni
   désynchronisation.
9. **Promotion `plugin-misc` — ouverte** : porter la fonctionnalité sous
   `misc.transmuteHotkey`, compiler le pack complet et répéter les gates de
   non-régression avant de retirer l'autonome.

## Prochain gate

Sur demande explicite de reprise, fixer le contrat du hotkey puis tracer les deux
widgets `convert` et leurs chemins de dispatch sous 92777 avant toute
implantation.

## Frontière Git

Cette décision autorise la mission documentaire, son workstream et son entrée
ROADMAP. Elle ne constitue ni une demande de commit/push, ni une autorisation de
modifier une DLL d'eezstreet. Toute future incubation restera autonome jusqu'au
merge explicitement demandé et validé.
