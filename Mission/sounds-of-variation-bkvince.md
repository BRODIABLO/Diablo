# Sounds of Variation for BKVince

## Décision gouvernée

Vincent a retenu l'option A le 2 août 2026 : intégrer sélectivement **The Sounds of Variation 0.1g** dans BKVince 3.2, sans remplacer les tables courantes par les tables anciennes du mod source et sans versionner ses assets audio.

Cette mission est un workstream séparé. Elle ne remplace pas la mission active BaseMod.

## Sources locales vérifiées

| Archive Nexus locale | SHA-256 attendu |
|---|---|
| `The Sounds of Variation (Sounds)-287-v0-1g-1673569945.rar` | `3787E81A38BBC1959A2816EAA3E83B7B79F69E1931DD41095358631338E7699E` |
| `The Sounds of Variation - Sounds (FIX for no drops bug)-287-v0-1g-fix-1675802605.rar` | `537E238BE4289FA5DBBD36E7D32947A1743652C605DCFFEC5296C2E9C18E0E28` |

Les deux archives ont passé le test CRC de 7-Zip. L'archive principale contient 1 228 fichiers FLAC valides et aucun exécutable, DLL ou script. Le correctif contient uniquement `monstats.txt` et `sounds.txt`.

## Politique d'intégration

- `sounds.txt` : reconstruire les 38 groupes touchés dans l'ordre de l'auteur, ajouter 1 222 clés absentes et ne modifier sur les lignes BKVince existantes que `Group Size` et les `FileName` réellement fournis par l'archive.
- `monsounds.txt` : partir de la table vanilla 3.2 courante, appliquer le delta ciblé de `necromage` et ajouter `sk_mage_fire`, `sk_mage_cold`, `sk_mage_ltng` et `sk_mage_pois`.
- `monstats.txt` : modifier exclusivement `MonSound` et `UMonSound` sur les 27 IDs de mages squelettes qui référencent ces quatre profils.
- préserver les headers 3.2, les lignes propres à BKVince, le CRLF et le round-trip byte-exact.
- valider les deux SHA-256 avant toute extraction ou copie runtime.
- ne jamais copier les anciennes tables intégrales du mod source dans BKVince.

## Distribution et provenance

La page Nexus de l'auteur WyRuZzaH interdit la republication des assets sans autorisation. Le dépôt RuffnecKk étant public, les 1 228 FLAC restent hors de Git et sont installés depuis les archives Nexus détenues localement par Vincent. Une redistribution publique de ces sons demeure bloquée jusqu'à obtention d'une permission explicite de l'auteur. Les crédits tiers restent distincts des créations RuffnecKk.

## Gates

- [x] Fusion TSV idempotente, références résolues et round-trip byte-exact.
- [x] Archives, nombre de FLAC, signatures `fLaC` et hashes source/runtime vérifiés.
- [x] Trois tables synchronisées vers le profil BKVince actif avec hashes identiques.
- [x] Cold start D2RLoader 3.2 sans assertion, erreur de table ni erreur de chargement.
- [ ] Validation auditive en jeu des 38 familles, puis solo/hôte/joiner et sauvegarde/rechargement.
- [ ] Autorisation écrite de WyRuZzaH avant toute archive ou release publique contenant les FLAC.

## État au 2 août 2026

Intégration source/runtime terminée. Les 1 228 FLAC ont été installés localement sans écraser d'asset existant; les trois tables ont été synchronisées avec les SHA-256 `ABDCA1CB…7DEF8` (`monsounds.txt`), `2DEF9754…FD67` (`monstats.txt`) et `6A8CDEB7…0B8C` (`sounds.txt`). Le cold start a atteint le rendu de l'Acte I sans assertion, crash ni erreur de table ou de son. La validation auditive et la matrice multijoueur restent ouvertes.

Le témoin hors périmètre initialement rencontré dans Mercenary Command est
résolu le 4 août 2026 : son validateur reconnaît désormais les 18 profils
Desert Mercenary legacy, les 12 profils expansion à aura et les 6 profils
expansion Combat de la refonte Acte II. Les tests ciblés Mercenary Command et
Act I Rogue, ainsi que la suite globale `npm run verify:data`, sont verts.
La comparaison des assets JSON normalise leurs fins de ligne afin qu'un checkout
frais ne produise pas de faux positif. L'override HD `commandbar.json`, dont
l'identifiant d'entité BKVince avait été normalisé, est explicitement préservé;
toute autre divergence de contenu des assets Mercenary Command demeure refusée.
`hireling.txt` est resté byte-identique
(SHA-256
`12C75CB39243EF5272EAC822ABCF86C0A123B6C830A9813A010235683AB64CD3`);
aucune donnée Mercenary Command ou Sounds of Variation n'a été modifiée par ce
correctif de maintenance.

## Correctif permanent du miroir HD — 4 août 2026

Après l'installation cumulative des mods de sons, Vincent a observé des sons
d'objets, de monstres et d'interface coupés ou silencieux. Le diagnostic a
montré que ce n'était pas une saturation du nombre de voix : 372 FLAC étaient
présents sous `data/global/sfx`, tandis que le moteur HD tentait notamment de
résoudre `data/hd/global/SFX/cursor/button.flac`.

Le correctif runtime a copié uniquement les 371 cibles HD absentes, conservé
une cible déjà byte-identique et refusé tout écrasement divergent. Les 372
paires ont ensuite été validées par SHA-256, avec zéro fichier manquant et zéro
divergence. Le cold start D2RLoader a franchi 24/24 étapes sans nouvelle erreur
`D2Sound`, puis Vincent a confirmé en jeu que le correctif fonctionnait.

Pour rendre ce comportement durable, 370 des 371 FLAC initialement manquants
sont intégrés à la source gouvernée sous
`data-BKVince/BKVince.mpq/data/hd/global/sfx/` et restent gérés par Git LFS. Ce
lot duplique seulement le placement HD des sons déjà sélectionnés par le pack
runtime; il n'écrase aucun asset HD existant et ne modifie aucune table TSV ni
le budget de voix audio.

Vincent a demandé le 4 août 2026 de conserver l'ancien son de pluie. L'override
`ambient/scene/rain2.flac` du miroir, SHA-256
`AA7B9C77B3DC7028BFF0744235B5C9AFAA3E75E65DC0F5D3AA564D4EC97A7328`, a donc
été retiré uniquement des chemins HD source et runtime. Le fichier global du
pack reste intact et le fallback retrouve l'asset vanilla 3.2, SHA-256
`C07E9A35A3608B026510A0D1E25B76DC8C2172A46D3D4641DC74DCB17EB0EE27`.
Le premier cold start de contrôle s'est interrompu à 15/24 sur une exception
graphique dans `dxgi.dll`, appelée depuis `plugin-items.dll`, avant le chargement
audio. Le second cold start a franchi 24/24 étapes avec 18/18 patches appliqués,
12 plugins actifs, zéro rejet/échec et aucune erreur `D2Sound` ou `rain2`.
L'audition du retour au son de pluie précédent reste à confirmer par Vincent.

## Prototype séparé — Magic Arrow et Guided Arrow

Ce petit lot du 4 août 2026 est distinct de **The Sounds of Variation**. Les
trois FLAC proviennent du post Inven
[`메아리 타격 효과음_매직에로우`](https://www.inven.co.kr/board/diablo2/5842/7410)
de **Koo3869**, qui demande explicitement de conserver la source lors du
partage. L'archive locale `c2517874081.7z` porte le SHA-256
`0E6D9419E3D76EF996E7C2113D4E2724367AFCEE14C29DFB1ADDA70DE92F2607`.

- Les trois fichiers 48 kHz/24 bits sont renommés sous `skill/amazon` et
  versionnés par Git LFS avec leurs hashes source exacts.
- `Magic Arrow` et `Guided Arrow` utilisent le groupe isolé
  `custom_bkvince_magic_guided_arrow_cast_1`; leurs sons de projectile natifs
  restent actifs.
- Aucun fichier ni identifiant Warlock n'est modifié et `Silent.flac` est exclu.
- Le snapshot exact de l'index passe `verify:data`, le cadastre et les contrôles
  ciblés TSV/références. L'audition en jeu reste volontairement ouverte.
