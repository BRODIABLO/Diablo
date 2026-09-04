# Checklist d'admission D2RLoader

## Identité et provenance

- Version publique annoncée, canal et date observée.
- URL officielle ouverte et asset exact identifié.
- Statut des checksums upstream explicitement consigné.
- `D2RLoader.exe`, `D2RCore.dll` et `d2rloader.mpq` présents, tailles et SHA-256 capturés.
- Versions PE consignées sans remplacer le nom public lorsqu'elles diffèrent.
- Artefacts conservés uniquement sous `analysis-cache/d2rloader-release-intake/<id>/`.

## PluginSDK et contrats

- Dépôt, tag, commit et licence PluginSDK identifiés.
- Version d'API, tailles publiques et compatibilité binaire comparées au pin promu.
- Headers `PluginContext`, Patch, Hook, Lifecycle, services et exports comparés.
- Nouveaux `ServiceId`, structures, callbacks, capabilities et règles de taille inventoriés.
- Manifeste plugin et trois exports requis vérifiés.
- Ordre de chargement, unload, hot reload, ownership, threads et durée de vie audités.
- Configuration globale/mod-locale, contrôles, localisation et chemins d'assets comparés.
- SDK minimal choisi par capability consommée; aucune migration « latest » sans gain démontré.

## D2RCore et coexistence

- Providers `D2RCore` consommés par la Suite comparés corps, exports, PDATA/unwind, forwarders et témoins ABI selon leur contrat.
- Patches et hooks loader-owned comparés; toute différence partielle échoue proprement.
- Matrice d'impact de chaque composant actif : `unaffected`, `retest-only`, `rebuild`, `adapted`, `blocked` ou `superseded-by-loader`.
- Propriétaire unique conservé pour chaque hook, patch et contrat partagé.
- Toutes les DLL de la Suite et les cinq plugins eezstreet restent actifs pendant la qualification de coexistence.
- Toute isolation est étiquetée diagnostic, annulée, puis suivie d'un retest pile complète.

## Gates de promotion

| Gate | Condition minimale |
|---|---|
| `sourceVerified` | Source officielle et asset exact identifiés |
| `artifactIntegrity` | Trois artefacts complets, tailles et SHA-256 |
| `sdkAudit` | Pins, API/ABI, services et capabilities vérifiés |
| `contractAudit` | Manifeste, exports, lifecycle, threads et configuration revus |
| `staticCompatibility` | Deltas et impacts Suite fermés statiquement |
| `runtimeQualification` | Cold start frais sur le runtime officiel courant |
| `fullStackCoexistence` | Pile complète active sans nouvelle régression |
| `multiplayer` | Gate séparé, pouvant rester `not-run` sans revendication réseau |

La promotion exige les sept premiers gates techniques en `passed`. La version,
le canal, le nom de build et le hash global ne deviennent jamais un gate de
chargement dans les DLL.
