# Transmute Hotkey — D2R 3.2

## Statut et séquencement

- Statut : **correctif autonome 0.2.0 compilé, synchronisé et chargé à froid
  avec `MOUSE4`; entrée physique, dispatch standalone et transmutation visible
  validés en jeu, matrice complémentaire restante**.
- Cible : `D2R.exe 3.2.92777` sous D2RLoader.
- Vincent confirme le 27 juillet 2026 l'Option A : inscrire la mission sans
  remplacer la priorité courante. Vendor Stock Refresh a ensuite été déclaré
  réglé et Cube Quick Move Bottom-Right est devenu la mission active, avec
  Equipped Item to Cube placé juste après. Vincent demande explicitement la
  reprise puis l'implantation de Transmute Hotkey le 27 juillet 2026.
- Vincent confirme le 27 juillet 2026 la catégorie `misc`, la DLL propriétaire
  future `plugin-misc.dll` et la clé prévue `misc.transmuteHotkey`.
- Le prototype incubé est `TransmuteHotkey.dll`, une DLL autonome hybride
  globale/mod-locale attribuée exactement à `RuffnecKk`.
- Vincent signale le 28 juillet 2026 que `CTRL+SHIFT+T` ne produit rien et choisit
  `Mouse 4` pour BKVince, tout en exigeant que le binding public reste configurable.

## Intention joueur et gain mesurable

Déclencher au clavier ou par bouton latéral le bouton natif **Transmute** du
Horadric Cube afin qu'un clic sur le widget ne soit plus obligatoire, sans
retirer ni altérer le bouton, son tooltip, le son, la navigation manette ou les
validations natives.

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
- Trois identifications stables sont promues dans `known-rvas.json` : mise à jour
  du Cube intégré `0x23ECD0`, mise à jour du Cube standalone `0x2CDA90` et
  recherche d'un panneau racine par nom `0x846170`.
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

## Implantation autonome 0.1.0 remplacée — 27 juillet 2026

- `TransmuteHotkey.dll` accroche les prologues stricts de `0x23ECD0` et
  `0x2CDA90`, appelle chaque original en premier, retrouve le `convert` du même
  panneau et exige que le panneau et le bouton soient visibles, que le bouton
  soit enabled et que son message embarqué à `button+0x558` soit non vide.
- L'activation appelle le dispatcher UI existant `0x843D90` avec le message
  embarqué réel. Le plugin n'accroche pas ce dispatcher, ne concurrence donc pas
  son propriétaire/broker actuel et ne construit aucun paquet réseau.
- Le hotkey par défaut est `CTRL+SHIFT+T`. Le JSON accepte `A-Z`, `0-9`,
  `F1-F24`, `SPACE`, `TAB`, `INSERT`, `DELETE`, `HOME`, `END`, `PAGEUP` et
  `PAGEDOWN` avec `CTRL`, `SHIFT` ou `ALT`; une touche imprimable exige `CTRL`
  ou `ALT`, et les modificateurs doivent correspondre exactement.
- Un hook clavier bas niveau chaîné accepte seulement le processus D2R au
  premier front descendant, ignore les événements injectés et les répétitions,
  puis délègue la demande au prochain update du panneau UI. Une demande expire
  après 250 ms; le maintien ne peut donc pas produire plusieurs transmutations.
- `ChatPanel` et les panneaux connus de saisie/confirmation bloquent la demande.
  Le chord n'est consommé que lorsque le processus possède le foreground, que
  le bouton a été observé utilisable dans les 120 ms et qu'aucun blocker n'est
  actif.
- `TransmuteHotkey.json` est strict, commenté en anglais et recherché dans le mod
  actif avant le dossier global; une configuration présente mais invalide fait
  refuser le plugin. Aucun TOML ni changement d'une DLL eezstreet n'est produit.
- La Release x64 est reproductible par le builder gouverné, le test de politique
  passe `1/1`, le manifeste v2 expose exactement les trois exports D2RLoader,
  l'auteur est `RuffnecKk` et le SHA-256 source/runtime de la DLL vaut
  `10B1E9F8D63FD29F9641ED45DDD3F9B0C1E187F9378B615FFD740B4C0B07436A`.
- La synchronisation mod-locale est limitée à la DLL et au JSON. Le cold start
  frais charge les deux hooks, `20/20` memory patch files et `28` plugins actifs
  sur `30` scannés (`2` overrides globaux désactivés, `0` rejet, `0` échec), puis
  atteint `24/24` étapes de démarrage.

## Correctif autonome 0.2.0 — 28 juillet 2026

- Le retour gameplay de Vincent invalide la fiabilité de la 0.1.0. Le log
  historique contient un seul dispatch standalone le 28 juillet à `08:20:00`,
  mais aucun nouveau dispatch pendant l'essai signalé. L'entrée exigeait en
  outre `now <= ReadyUntil`, une fenêtre de seulement `120 ms` réarmée par les
  updates natifs `0x23ECD0` ou `0x2CDA90`; ces callbacks ne constituent pas une
  boucle d'entrée garantie.
- La 0.2.0 supprime cette fenêtre. Le hook clavier ou souris ne fait que poster
  une demande enregistrée au thread de la fenêtre D2R; un hook Win32 ciblé
  `WH_GETMESSAGE` consomme ensuite cette demande sur le thread UI, revalide le
  panneau, le bouton `convert`, ses états et son message, puis appelle le même
  dispatcher natif `0x843D90`. Aucun nouveau hook D2R, RVA ou paquet réseau n'est
  ajouté.
- Le parseur public accepte toujours les bindings clavier documentés, plus
  `MOUSE3`, `MOUSE4` et `MOUSE5`; `MIDDLE`, `XBUTTON1` et `XBUTTON2` sont des
  alias, et les modificateurs exacts `CTRL`, `SHIFT` ou `ALT` restent possibles.
  Le défaut intégré sûr demeure `CTRL+SHIFT+T`, tandis que le JSON BKVince fixe
  explicitement `"hotkey": "MOUSE4"` selon le choix de Vincent.
- Le front descendant unique, le refus des événements injectés, la consommation
  bornée, l'expiration à `250 ms`, les blockers de saisie et la revalidation du
  bouton sur le thread UI sont conservés. Une touche ou un bouton maintenu ne
  peut produire qu'une seule demande avant relâchement.
- La Release x64 et le test de politique passent `1/1`. La DLL gouvernée et la
  DLL mod-locale sont byte-identiques au SHA-256
  `278A5282A438A3BD4E3BBEA5D487ED2F0E1F12A329E48656D13DB8887F9153BE`;
  le JSON source/runtime `MOUSE4` vaut
  `B63B50B456EB616FF3B9D5DFE79B5E2D845C69E65E0EC235A0A0221A9EE309B1`.
- Le cold start frais du 28 juillet à `10:34` accepte les deux hooks natifs,
  charge `Transmute Hotkey 0.2.0` avec `input=mouse`, confirme le relais UI sur
  le thread `16744`, applique `20/20` patches, active `28/30` plugins avec deux
  overrides globaux désactivés, zéro rejet/échec et atteint `24/24`. Le processus
  relancé reste répondant. À `10:35:51`, une pression physique `MOUSE4` produit
  le premier dispatch natif standalone avec `count=1`; Vincent confirme ensuite
  que la transmutation visible fonctionne en jeu.

## Hypothèses restant à tester

- Si le clic natif route déjà toute transmutation vers l'autorité de l'hôte, le
  hotkey ne devrait introduire aucun nouveau protocole réseau. Cette équivalence
  doit être confirmée par les appels et par une session hôte/joiner.

## Inconnues et risques

- Collision éventuelle de `MOUSE4` avec un binding D2R ou un autre plugin selon
  le profil public; le JSON permet de choisir un autre bouton ou chord.
- Observation physique du relais Win32 ciblé et de l'activation exacte sur les
  panneaux Cube standalone et intégré.
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

1. **Séquencement — fermé** : Vincent a explicitement repris puis demandé
   l'implantation de la mission.
2. **Contrat du hotkey — fermé pour le prototype** : BKVince utilise `MOUSE4`;
   clavier, `MOUSE3/4/5`, alias, JSON strict, correspondance exacte, front unique
   et consommation bornée sont implantés; les collisions restent à éprouver.
3. **Preuve UI 92777 — fermée pour le prototype** : les deux updates, leurs ABI,
   signatures, racines, widgets et le message embarqué sont bornés et gouvernés.
4. **Audit PluginPack — fermé pour le prototype** : aucun comportement homonyme,
   aucune plage de hook concurrente et aucun second hook du dispatcher partagé.
5. **Prototype autonome — partiellement fermé** : Release x64, test, manifeste,
   exports, auteur, description, JSON `MOUSE4`, relais UI et cold start mod-local
   sont verts; la portée globale reste à charger réellement.
6. **Entrée et états UI — partiellement fermé** : `CTRL+SHIFT+T` 0.1.0 est
   signalé sans effet; la 0.2.0 corrige la fenêtre de 120 ms et un `MOUSE4`
   physique produit exactement un dispatch et une transmutation visible
   standalone confirmée par Vincent. Maintien, chat, texte, modales et panneaux
   absents restent à confirmer en jeu.
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

Répéter le `MOUSE4` validé dans le Cube intégré et confirmer le refus pendant le
chat. Enchaîner ensuite la matrice recette invalide/Cube plein,
maintien/pressions rapides/clic simultané, bindings clavier alternatifs,
souris/manette, save/reload, solo/hôte/joiner et portée globale.

## Frontière Git

Cette décision autorise la mission documentaire, son workstream et son entrée
ROADMAP. Elle ne constitue ni une demande de commit/push, ni une autorisation de
modifier une DLL d'eezstreet. Toute future incubation restera autonome jusqu'au
merge explicitement demandé et validé.
