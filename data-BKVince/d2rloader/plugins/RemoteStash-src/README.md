# Remote Stash

Remote Stash adds a desktop inventory button that invokes the native client
open-stash UI action from anywhere. Version 0.1.5 opens the native stash panel
from that button in BKVince, as confirmed in game. It does not yet establish an
authoritative server bank interaction. Item moves and gold transactions may
therefore be rejected when the player is not beside a physical stash.

## Layout compatibility

The plugin does not assume a fixed inventory size. On every native inventory
panel refresh it reads the active panel, inventory grid, gold button, gold
amount, and `remote_stash` button rectangles. It aligns the button with the
runtime grid's left edge, centers it on the runtime gold footer, and refuses to
show it outside the panel or across existing controls.

The current prototype still needs a layout to declare a `ButtonWidget` named
`remote_stash`. BKVince supplies that declaration in both its original and
expansion desktop inventory layouts. Its custom four-state chest sprite is
176 by 112 pixels (88 by 56 in low-end mode). The placement policy keeps the
button near the gold footer, moves it below the runtime grid when necessary,
and never changes the native gold button rectangle. Its native hover tooltip
resolves `remoteStashTooltip` from BKVince `ui.json` as `YOUR PRIVATE STASH`.
A custom inventory mod can retain its own dimensions and art, but must merge
the same named child into every active inventory layout until native widget
creation is proven.

Controller placement intentionally fails closed because the controller gold
button occupies the same footer space. No configuration file is used.

## Native evidence and safety

- D2R build: 3.2.92777 only.
- Inventory refresh hook: RVA `0x22BA70`.
- UI-message dispatcher: RVA `0x843D90`. Remote Stash registers an interceptor
  with the shared `plugin-misc` broker when available and otherwise owns the
  dispatcher directly after a strict signature check. It consumes only the
  message object embedded in the currently resolved `remote_stash` widget. The
  layout uses the registered `PlayerInventoryPanelMessage:DropGold` value so
  D2R never parses a custom enum; the real gold button has a different message
  object and is forwarded unchanged.
- Client UI packet handler: RVA `0x12DBC0`, receiving the native two-byte
  `{ 0x77, 0x10 }` open-stash action.
- Every native helper and hook has a strict byte-signature gate. A missing
  widget, invalid layout, unsupported build, or signature mismatch disables the
  affected behavior instead of guessing.

The plugin is standalone, authored by RuffnecKk, and supports both global and
mod-local D2RLoader plugin folders. It does not link or redistribute any
eezstreet plugin DLL.
