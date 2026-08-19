# Requêtes et modèle de preuve

## Workbench natif du runtime courant 3.3.93847

```powershell
npm.cmd run re:d2r33 -- status
npm.cmd run re:d2r33 -- self-test
npm.cmd run re:d2r33 -- known tome
npm.cmd run re:d2r33 -- function 0x5817BD
npm.cmd run re:d2r33 -- xrefs 0x46F090
npm.cmd run re:d2r33 -- bytes "41 B9 ?? ?? ?? ??"
npm.cmd run re:d2r33:ghidra -- status
npm.cmd run re:d2r33:ghidra -- function 0x441B10 180
```

Ces commandes ciblent D2R 3.3.93847 en réutilisant le corpus natif commun déjà
gouverné. Le chemin historique du workbench n'est pas le nom du runtime courant.

Sous PowerShell, `npm.cmd` contourne un éventuel blocage de `npm.ps1` sans modifier la stratégie d'exécution système.

## Références épinglées

```powershell
npm.cmd run ref:d2moo -- status
npm.cmd run ref:d2moo -- search durability
npm.cmd run ref:d2moo -- symbol ITEMS_UpdateDurability
npm.cmd run ref:d2rlplugins -- status
npm.cmd run ref:d2rlplugins -- search sgptDataTables
npm.cmd run ref:d2rlplugins -- symbol D2UnitStrc
```

Les pins et politiques résident dans `reverse-engineering/references.json`; les clones locaux restent sous `analysis-cache/references/`.

## Preuve minimale avant implantation

- Version courante 3.3.93847 et identité binaire gouvernée; hashes du corpus vérifiés par `status`.
- Fonction native bornée et rôle expliqué.
- Callsites/xrefs pertinents recensés.
- Octets `expected` assez stricts pour refuser un binaire incompatible.
- ABI et champs de structure démontrés par l'image native canonique gouvernée.
- Plage exacte de lecture/écriture ou de hook, avec audit des collisions.
- Source primaire consignée dans la mission et dans `known-rvas.json` si stable.
- Validation runtime encore distinguée de la preuve statique.
