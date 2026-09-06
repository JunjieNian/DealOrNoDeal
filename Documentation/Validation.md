# Studio 0.4 delivery verification

Validated the archived Windows launcher and its packaged game, with rendering and audio enabled.
The final Editor target and BuildCookRun completed successfully. Asset regeneration completed without errors or attachment warnings.

| Run | Exit code | Logged errors | Result |
| --- | --- | --- | --- |
| Packaged-Experience | 0 | 0 | Passed |
| Packaged-NoDeal | 0 | 0 | Passed |
| Packaged-Accept | 0 | 0 | Passed |
| Packaged-UI-1600 | 0 | 0 | Passed |
| Packaged-UI-1280 | 0 | 0 | Passed |
| Packaged-Wide | 0 | 0 | Rendered capture |
| Packaged-Table | 0 | 0 | Rendered capture |

The UI suite checks generated hitboxes at both 1600 x 900 and 1280 x 720, including selection, acceptance confirmation/cancellation, menu resume, replay, final swap and payout. The rules suite also checks the keep route, prize uniqueness, own-case protection, pause/resume timers and accidental restart protection.

Sixteen phase screenshots passed the HUD-glyph check. Final stage and table captures are 1920 x 1080. Screenshots were also visually reviewed; the runtime-font issue found during preflight was corrected before this final verification.

Packaged game binary SHA-256: `de2854b1229d084e890e53a5a54512d960e2158c4eb163856bbee982e58b32d8`

Runtime files excluding test screenshots/logs: 829.0 MiB.

See `Validation.json` for exact success markers and screenshot checks. Dimensions remain provisional and no human geometry is included.
