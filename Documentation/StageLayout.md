# Studio layout and scale

The stage uses centimeters. +X is upstage toward the briefcase terraces and skyline; -X faces the downstage audience. +Y is the amount-board side, -Y is the Banker-suite side, and +Z is up.

## Spatial anchors

| Element | Location or envelope |
| --- | --- |
| Studio floor | 40 m deep x 31 m wide, extending under all seating |
| Central dais | X=-1 m; 7.8 m x 5.8 m; top at 27 cm |
| Glass game table | X=-1.4 m; 1.12 m x 1.78 m; top at 1.345 m |
| Case terraces | X=2.85, 4.05, 5.25, 6.45 m; each 10.7 m wide |
| Terrace tops | Z=0.55, 1.35, 2.15, 2.95 m |
| Briefcases | 6+7+7+6 arrangement; 48 x 14 x 34 cm shells |
| Case supports | Shelf top 91 cm above each terrace; no floating cases |
| Arch | Center X about 7.9 m; radius 6.86 m |
| Amount screen | Center X=4.35 m, Y=7.6 m; 4.3 m wide x 7.02 m high |
| Banker suite | Center Y=-8.45 m; floor about 4.7 m, roof about 7.0 m |
| Wide camera | X=-23.5 m, Z=10.5 m; FOV 69 degrees |
| Board camera | Preserves the earlier approved transform and 80-degree FOV |

These dimensions are explicit design assumptions, not measurements of the real studio.

## Construction

Ten independent runtime actors preserve the original module architecture. Nine separate static-mesh groups add the detailed world, dais, phone, terraces, arch/skyline, Banker suite, display housing, audience architecture and lighting hardware. Actor groups can be selected independently in Unreal; each mesh is editable in the supplied Blender source.

177 empty chairs use a shared instanced mesh: 105 downstage chairs and 72 chairs in two side banks. Side banks stop before the amount display to preserve visibility. There are no people or human stand-ins. Each case has a supported body, separate hinged lid and a selection strip. The buttons and receiver on the physical table are scenic props; gameplay decisions use the HUD or keyboard.

The Blender source uses right-handed coordinates. Unreal FBX conversion reflects Y, so authored world-space actors use a compensating (1,-1,1) scale. Runtime case and seat geometry is symmetric about local Y.

## Further art direction

The editable set can be refined against additional reference frames, especially scenic skyline proportions, booth glazing and the exact stage ornament. Current geometry and materials provide an original, cohesive studio interpretation. There is no claim of survey accuracy or film-quality photorealism.
