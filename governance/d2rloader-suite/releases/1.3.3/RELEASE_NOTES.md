## RuffnecKk D2RLoader Suite 1.3.3

This release updates **MapSense to 1.0.2** and **Resistance Floor to 1.0.2**. All other plugins and patches retain their versions from Suite 1.3.2.

### MapSense 1.0.2

- Fixes map overlays disappearing when New Stats, New Skills or Quest Log notifications appear.
- Keeps overlays visible when an unrecognized UI state does not represent a blocking panel.
- Fixes vanilla map labels and objects without requiring `-txt`.
- Makes menu localization independent of the mod’s TXT files.
- Adapts label sizes and spacing to the current resolution, including changes made while playing.
- Improves Chinese, Japanese and Korean font selection and glyph coverage.
- Fixes a shutdown cleanup issue and reduces repeated startup warnings.
- Improves map generation portability and reveal accuracy for custom levels.

### Resistance Floor 1.0.2

- Removes the D2R build-number restriction. Loading now depends on validation of the native code and layouts used by the plugin.
- Preserves existing resistance-floor behavior and configuration.

### Installation

Replace the updated plugin files. For MapSense, keep its included map generator executable beside the DLL. Preserve your existing configuration files.
