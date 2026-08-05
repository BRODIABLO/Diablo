# Third-party notices

## @d2runewizard/d2s

The BKVince Hero Editor uses `@d2runewizard/d2s` version 2.0.132 as its D2S
codec. The package is distributed under the ISC License.

The workspace applies a small source patch for D2R v105 realm data and quantity
presence bits. The modified package remains covered by the same ISC License.

Copyright (c) the `@d2runewizard/d2s` contributors.

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

## Diablo II: Resurrected UI sprites

The paper-doll, item icons, skill atlases, and skill-tree backgrounds are generated locally from the
versioned BKVince overlays and the vanilla `SpA1` v31 sprites extracted from
Vincent's installed Diablo II: Resurrected CASC. They are Diablo II: Resurrected game assets and
remain the property of Blizzard Entertainment. They are not copied from
RuneWizard. The raw vanilla sprites stay in the ignored `analysis-cache/`
directory; the application bundles only the generated PNG inventory visuals,
the eight class skill atlases, and the 24 three-tab skill-tree backgrounds.

The small build-time decoder follows the public `SpA1` v31 header and RGBA
payload layout documented by the D2R modding community. Format references:
`4KMong/D2RSpriteToolkit` and `eezstreet/D2RModding-SpriteEdit` (which credits
shalzuth for the original sprite-format work). No source code from those tools
is bundled in this application.
