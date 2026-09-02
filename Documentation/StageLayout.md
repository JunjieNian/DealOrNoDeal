# Stage layout and iteration notes

## Coordinate convention

- `+X`: upstage, toward the model stairs and skyline.
- `-X`: downstage, toward the primary audience bank and wide camera.
- `+Y`: audience-view right, where the amount board sits.
- `-Y`: audience-view left, where the elevated Banker booth sits.
- `+Z`: height.

The origin is near the central game platform. Measurements are centimeters.

## First-pass spatial anchors

| Element | Center / envelope | First-pass size |
| --- | --- | --- |
| Working stage shell | origin | 24 m wide × 18 m deep |
| Central platform | X=-1 m | 7.8 m × 5.8 m |
| Model staircase | X=2.8–6.5 m | 10.7 m wide, 4 tiers, top at 2.95 m |
| Amount board | X=4.2 m, Y=7.6 m | 4 m wide × 7 m high |
| Grand arch / skyline | X=8.2 m | 13 m clear arch span, peak about 6.8 m |
| Banker booth | X=5.4 m, Y=-8.45 m | high booth floor around 4.7 m |
| Primary wide camera | X=-23.5 m, Z=10.5 m | 69-degree field of view |

## Why the modules are code-generated

The stage is assembled from named C++ actor modules using only engine-native
primitive meshes. This makes the source diffable, deterministic, and easy to
regenerate while the proportions are still changing. Art assets can replace any
module later without changing the public interaction API or the master level.

## Known graybox limitations

- The central platform uses the engine cylinder primitive as a readable polygonal
  placeholder; its exact edge count and decorative inlays are not final.
- The visual direction intentionally contains no people or human stand-ins. All 26
  briefcases float directly above their tier positions; the empty audience is represented
  only by seat blocks.
- Audience seats show representative density rather than all approximately 360
  reported studio seats.
- The skyline, arch cross-section, booth interior, railings, truss, floor graphics,
  LED strips, and camera pedestals remain simplified.
- The amount board values are present and stateful, but typography and tile spacing
  are still placeholders.

## Next iteration sequence

1. Match camera perspective and major proportions against selected reference frames.
2. Replace the arch, staircase fascia, amount board, and table with authored meshes.
3. Establish the blue/black/chrome/glass material family and practical LED strips.
4. Add 26 briefcase assets and model stand-in skeletal meshes.
5. Implement the round state machine, Deal/No Deal controls, phone/Banker sequence,
   amount elimination, lighting cues, camera cuts, and audience-response hooks.
6. Add performance budgets, packaged-build validation, and source-control workflow.
