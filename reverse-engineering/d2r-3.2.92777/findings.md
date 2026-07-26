# Findings persistants — D2R 3.2.92777

Ce document conserve uniquement les conclusions utiles aux prochaines sessions.
Les sorties volumineuses demeurent sous `analysis-cache/corpus/`.

## Base d'analyse

- Image canonique gouvernee : SHA-256
  `CC59119DC2A6C7D43D088098FC162EAFA4AE1299B2079126AEF43C1ACA914715`.
- Image d'analyse deterministe : SHA-256
  `673E8C0B2E89563E75525B24D137098EFD07B2DB4ED42ADEC56AA1ADDF0E63AB`.
- Le `.text` de l'image canonique est preserve. Les metadonnees PE non-code
  rehydratees rendent 105 850 fonctions x64 accessibles par `.pdata`.
- L'index compact connait actuellement plus d'un million de references code ou
  RIP, les chaines des sections de donnees et les patches BKVince actifs.

## Routine de quantite des Books

- `0x5817BD` charge le delta `-1` apres le controle du type `Book` (`0x12`).
- L'appel a `0x46F090` part de `0x5817CC`; l'index retrouve six callers directs
  de cette routine centrale.
- `0x46F090` synchronise `STAT_QUANTITY` et la quantite du skill lie au
  scroll/tome.
- `0x47145D` est un chemin Javelin (`0x29`) rejete pour les tomes;
  `0x4F5849` est egalement un faux candidat conserve vanilla.

Commandes de reprise :

```powershell
npm run re:d2r32 -- function 0x5817BD
npm run re:d2r32 -- xrefs 0x46F090
```

## pSpell et consommables a skill

- La structure officielle partagee de D2RL-Plugins place `pSpell` a l'offset
  `+0x94` de l'enregistrement item.
- La documentation D2R 3.2 expose des handlers `pSpell` fixes; elle n'expose pas
  un handler generique prenant directement un ID arbitraire de skill.
- `books.txt` associe `ScrollSkill`/`BookSkill`; le `srvdofunc 113`
  `ItemDoBookSkill` utilise le skill du Book/Scroll et met sa quantite a jour.
  Cette voie constitue le prototype data-only prioritaire.
- Le pack officiel `plugin-skills` intercepte deja la consommation native des
  charges a `0x436830` (`D2GAME_SKILLMANA_Consume`). C'est la fondation prouvee
  pour un plugin exact si le clic droit inventaire arbitraire reste requis.
- Les fonctions autour de `0x1AC881`, `0x1AC8F0` et `0x1AC932` lisent bien le
  champ `+0x94`, mais leur role de dispatcher d'utilisation n'est pas prouve.
- Les routines autour de `0x1A7660`/`0x1A77D0` testent une borne `< 16` et sont
  des candidates possibles pour une table de handlers; confiance faible tant
  que callers, arguments et effets ne sont pas etablis.

Sortie brute conservee :
`analysis-cache/corpus/pspell-2026-07-19/pspell-analysis.txt`.

Prochaine etape efficace : partir des xrefs de l'acces `D2ItemsTxt+0x94`, puis
remonter depuis l'evenement serveur d'utilisation d'objet. Ne pas relancer un
scan global du `.text` avant d'avoir epuise l'index et le projet Ghidra.

## Rafraichissement du stock des marchands normaux

- `0x53C9F0` est `D2GAME_NPC_FillStoreInventory` avec l'ABI
  `(game, player, npc) -> void`; son unique caller est `0x540960`.
- La routine resout le `VendorChainEntry`, ecrit `GetTickCount64()` a `+0x38`,
  puis genere le stock aleatoire et permanent par `0x540EA0`.
- Le dispatch normal teste l'etat rempli `+0x34` et le drapeau de refresh
  `+0x35`. Lorsque ce dernier est arme, l'appel unique a `0x502F00` depuis
  `0x540952` nettoie et synchronise l'inventaire vendeur, remet `+0x34` et
  `+0x38` a zero, puis le remplissage est rejoue.
- `0x503290` parcourt la chaine des vendeurs. Son chemin temporise compare le
  timestamp a `GetTickCount64() - 0x3A980`, soit quatre minutes, arme `+0x35`
  et actualise `+0x38`. Ses deux callers directs sont `0x502DEB` et `0x502EE2`.
- Le gamble est distinct : le dispatch appelle `0x541880` depuis `0x540913`
  pour le type d'interface 3; sa limite de generation reste gouvernee a
  `0x541A7E`.
- Le commit epingle `D2RL-Plugins@dc75b49` confirme les structures
  `NpcItemCacheEntry`/`VendorChainEntry` et possede deja le hook canonique de
  `FillStoreInventory` dans `plugin-items`.
- Le layout natif du panel declare deja `button_refresh` avec
  `VendorPanelMessage:RefreshAll`, le sprite gamble, le son natif et `@refresh`.
  La configuration `0x2411E0` utilise `panel+0x168` : normal `0`, repair `1`,
  gamble `2`. Les gates uniques `0x24137D`/`0x241391` identifient le widget
  exclusif au gamble; `0x240E0D` garde le chemin d'entree direct sur le meme mode.
- Le handler de message `0x241B20` compare le hash RefreshAll
  `0xB7AA1748D66EFCAF` et rejoint `0x10F520`. Ce sender n'a que deux references
  dans le panel vendeur et emet l'action 2 via `0xEC730`.
- `0xEC730` construit exactement neuf octets `{opcode, action, npcGuid}`. Les
  callers 92777 prouvent opcode `0x38`, action 1 normal, 2 gamble et 3 hire. Le
  callback serveur `0x4B0470` exige neuf octets, valide le NPC puis route normal
  et gamble vers `0x540850`; ne pas transposer les 13 octets D2MOO 1.10f.
- `0x10CAC0` retourne l'etat gamble utilise par le panel. Le hook client peut
  donc conserver l'action 2 en gamble et choisir l'action 1 en normal sans
  nouveau protocole.
- `0x502F60` recoit `(game, npc, player, mode)` juste avant `0x540850`. Il lit et
  remplace `PlayerData+0x100`; mode 2 normal, 3 gamble, 4 hire. L'ancienne valeur
  2, la classe stockee a `+0xFC`, la classe du NPC et une entree remplie a
  `VendorChainEntry+0x34` forment le gate fail-closed du prototype avant d'armer
  `+0x35`.
- Cold start mod-local du 26 juillet 2026 : hashes source/runtime identiques,
  JSON resolu depuis `mods/BKVince/BKVince.mpq`, hooks `0x10F520` et `0x502F60`
  installes, trois patches UI acceptes avant le log `active`, demarrage graphique
  complet et zero erreur/rejet/mismatch frais. Gameplay et reseau restent non
  executes.

Conclusion : le serveur fournit deja la primitive atomique nettoyage puis
reconstruction et tout le trajet UI/reseau vanilla est maintenant prouve. Le
prototype autonome 0.1.0 compile et ses tests statiques passent. Le prochain
travail efficace est le cold start puis l'observation runtime de la
resynchronisation d'un panel normal ouvert, sans inventer d'overlay ni d'opcode.

## Discipline de promotion

Une adresse n'entre dans `known-rvas.json` qu'apres preuve par structure de
controle, octets/signature, caller/callee ou validation runtime. Les simples
ressemblances et les anciennes adresses 2.4 restent dans cette page avec une
confiance explicite.
