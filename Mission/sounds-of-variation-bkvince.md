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

La suite globale `npm run verify:data` atteint ensuite un témoin hors périmètre déjà présent dans Mercenary Command : 45 lignes Desert Mercenary attendues contre 36 trouvées. Les contrôles ciblés Sounds of Variation et le registre de workstreams sont valides; aucune donnée Mercenary Command n'a été modifiée dans ce chantier.
