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
