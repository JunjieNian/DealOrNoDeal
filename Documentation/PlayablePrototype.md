# Playable Prototype 0.2

Local standalone Windows builds are generated at:

`Builds/DealOrNoDealStage-Prototype-0.2/Windows`

Launch `DealOrNoDealStage.exe`. The folder is portable and must be copied as a
whole because the launcher depends on the adjacent `DealOrNoDealStage`, `Engine`,
and content-container files.

The `Builds` directory is not committed to Git. Recreate it with Unreal's
BuildCookRun pipeline, or distribute the separately archived portable build.

## Prototype acceptance criteria

- Mouse-clickable floating briefcases with keyboard fallback.
- Complete choose/open/offer/deal-or-no-deal/final-reveal/restart loop.
- Clickable Deal, No Deal, and Play Again HUD buttons.
- Physical amount-board elimination and case visibility state.
- Phase-driven camera and lighting cues.
- No rendered host, models, contestants, or audience people.
- Cooked and packaged Win64 build that runs without Unreal Editor.

## Verified flows

- Full No Deal route: 24 non-player cases opened, nine offers, final two-case reveal.
- First-offer Deal route: six cases opened, offer accepted, player case revealed.

Both flows were executed from the archived packaged build after cooking.
