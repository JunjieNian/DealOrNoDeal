# Deal or No Deal · Studio Web

A browser-native, playable port of the [Unreal studio game](../README.md). It uses Three.js to render the original Blender-authored studio and plain TypeScript for the game rules. The browser does not run or stream Unreal, and the static site needs no server-side game service.

## Play locally

With Node.js 22 or newer:

```text
cd web
npm ci
npm run dev
```

Open the printed local URL. For a production check, use `npm run build` and `npm run preview`.

The controls match the desktop game: click a case to preview it, click again or use the confirm button to choose/open it; arrows and Enter also work. Use D / N for offers and the final keep/swap choice, 1–4 or C for cameras, Esc for pause, and M / T / A for sound, pacing, and automatic cameras. All decisions work on touch screens. There is no decision timer.

The complete game has 26 cases, the original 26 prize values, nine rounds of `6, 5, 4, 3, 2, 1, 1, 1, 1` opens, an offer after each round, a two-step Deal confirmation, and a final keep/swap choice. The Banker uses the original remaining-value average, round multiplier, and dollar/$100 rounding. A `?seed=12345` query parameter fixes the shuffled prize assignment for reproducible demos; the normal URL makes a fresh random game on replay.

The four browser cameras follow the Unreal camera positions, targets, and horizontal fields of view. Cam 4 keeps the selection controls on the left so the amount display stays visible. The 3D amount display, the accessible HTML amount board, and case lids update as play progresses. If WebGL is unavailable, the game remains fully playable with a still image behind the native controls.

## Source assets and verification

`public/assets/deal-studio.glb` is exported from `../ArtSource/DealStudio.blend` using the included Blender export script, without changing the editable `.blend`:

```text
blender -b ArtSource/DealStudio.blend --python web/scripts/export_scene.py
```

The original cue sounds are copied from `../ArtSource/Audio/`. `npm test` runs the deterministic game-rule tests. `npm run test:browser` tests the built site at `http://127.0.0.1:4173/` using Chrome, including a complete nine-round game, early Deal, 3D loading, and touch layout; set `WEB_TEST_URL` and optionally `CHROME_PATH` for other locations. QA screenshots are saved to `../Saved/WebQA/` (ignored by Git).

`GITHUB_PAGES=true npm run build` applies the `/DealOrNoDeal/` project-site base path. The Pages workflow in `../.github/workflows/deploy-web.yml` publishes only `web/dist/`, not the Unreal source or Windows binaries.

The browser renderer is an adaptation of the authored set, not a frame-identical Unreal build. Geometry and game rules are retained, while real-time lighting and text use browser-native implementations.
