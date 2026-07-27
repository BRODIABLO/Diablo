# RemoteStash layout policy

This directory currently contains only the native-free placement policy and its
tests. It does not build a plugin DLL, install a hook, create configuration, or
ship a button asset.

The desktop policy reads the active panel, grid, gold button, gold amount, and
future button rectangles. It aligns the button with the grid's left edge,
centers it on the gold footer, and refuses placement outside the panel or across
the grid or gold controls. Controller placement remains a separate gate because
the controller gold button occupies the same area used by the desktop policy.
