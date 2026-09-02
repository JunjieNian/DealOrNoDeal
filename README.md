# Deal or No Deal Stage — Unreal Engine 5.7

This project is a modular graybox reconstruction of the classic 2006–2008 NBC
`Deal or No Deal` stage at Culver Studios. The first milestone prioritizes spatial
relationships, readable module boundaries, a playable camera setup, and stable
interaction seams over final art.

## Open the project

Open `DealOrNoDealStage.uproject` in Unreal Engine 5.7. The editor and game startup
map is `/Game/Maps/MainStage`.

## Current graybox modules

- `00_WorldShell`: 24 m × 18 m working envelope and upstage masking wall.
- `01_CentralPlatform`: approximately 7.8 m × 5.8 m platform, translucent table,
  and tagged Banker phone placeholder.
- `02_ModelStaircase_6_7_7_6`: four broad tiers and 26 numbered, deliberately
  floating briefcase interaction sockets in the requested 6+7+7+6 distribution.
  The tiers are widened and deepened so the floating cases and their shadows read
  as supported by the staircase. No model or human stand-ins are rendered.
- `03_AmountBoard_26Values`: 4 m × 7 m housing, all 26 US prize values, and an
  active/eliminated visual state API.
- `04_CityBackdrop_GrandArch`: skyline massing, lit-window instances, and the
  large segmented arch.
- `05_BankerHighBooth`: elevated tower, desk, translucent front, and label.
- `06_Audience_UShape`: broken-U risers with roughly 200 representative seat
  blocks; the reported studio capacity of roughly 360 is retained as a design note.
- `07_LightingRig`: cool stage wash, warm accents, Banker/Deal/NoDeal cue hooks.
- `08_CameraRig_4Shots`: wide master, game table, model stairs, and amount board.
- `09_InteractionDirector`: briefcase selection/open state, eliminated amount,
  Banker offer, and lighting cue entry points.

## Play controls

- `Mouse`: click a floating briefcase to keep/open it; click the on-screen
  `DEAL`, `NO DEAL`, and `PLAY AGAIN` buttons
- `Left / Right`: move the active briefcase selection
- `Enter` or `Space`: keep the highlighted case, then open highlighted cases
- `D`: accept the current Banker offer (Deal)
- `N`: reject the current Banker offer (No Deal)
- `R`: start a new randomized game after the final result
- `1`: wide master camera
- `2`: central game-table camera
- `3`: floating-briefcase staircase camera
- `4`: amount-board camera
- `C`: cycle through the four cameras
- `Esc`: exit the standalone prototype

## Simple playable game loop

The graybox now includes a complete mouse- and keyboard-playable round:

1. Choose one of 26 numbered briefcases to keep sealed.
2. Open `6, 5, 4, 3, 2, 1, 1, 1, 1` cases across successive rounds.
3. Each opened case gets a prominent center-screen amount reveal for 2.8 seconds,
   then that amount is removed from the physical amount board.
4. The Banker offers a rounded percentage of the average value still in play; the
   percentage rises in later rounds.
5. Accept with `D`, or continue with `N`.
6. If the last offer is rejected, the player's case and the final competing case
   are revealed. Press `R` for a freshly shuffled game.

The HUD shows the current phase, selected case, cases remaining, last revealed
amount, offer, controls, and final result. Banker offers use a compact left-side
panel so the amount board remains visible, and the amount-board camera keeps the
full board in frame. Camera and lighting cues change with the game phase, while
the stage remains intentionally empty of people. Graybox annotations and camera
markers are hidden in the playable presentation.

## Standalone prototype delivery

Local packaged Windows builds are archived under
`Builds/DealOrNoDealStage-Prototype-0.3/`. The `Builds` directory is intentionally
ignored by Git because cooked binaries are reproducible and exceed GitHub's normal
source-file limits. Launch `DealOrNoDealStage.exe`; Unreal Editor is not required.
The folder is a portable build, so keep its adjacent `Content`, `Engine`, and
configuration files together when copying it to another Windows computer.

## Interaction-ready API seams

The runtime module exposes Blueprint-callable methods without locking the project
to a UI implementation:

- `SelectBriefcase(BriefcaseNumber)`
- `OpenBriefcase(BriefcaseNumber, AmountIndex)`
- `SetBankerOffer(NewOffer)`
- `TriggerLightingCue(CueName)`
- `SetAmountActive(AmountIndex, bActive)`
- `ActivateCamera(CameraIndex)`

This keeps later briefcase gameplay, amount elimination, Deal/No Deal buttons,
phone sequences, audience feedback, and show-control cues independent from the
graybox meshes.

## Working scale

Unreal uses centimeters (`1 Unreal Unit = 1 cm`). The model zone spans roughly
10.7 m and reaches 3.0 m at the top tier before the model markers; the backdrop
arch peaks around 6.8 m. The complete working shell is 24 m wide by 18 m deep.
These are explicit first-pass assumptions intended for refinement against better
photographic and video references.

See `Documentation/StageLayout.md` for coordinates, assumptions, and the next
iteration plan.

## Regenerate the deterministic graybox level

After compiling the Editor target, set `UE_ROOT` to the installed Unreal Engine
5.7 directory and run Unreal Editor in command mode with:

```powershell
$UnrealEditorCmd = Join-Path $env:UE_ROOT 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
& $UnrealEditorCmd `
  '.\DealOrNoDealStage.uproject' `
  '-ExecutePythonScript=.\Content\Python\build_stage.py' `
  -unattended -nop4 -nosplash -NoSound
```

Keep rendering enabled when regenerating; Unreal 5.7's engine primitive-material
path is not stable under `-NullRHI` for this construction script.
