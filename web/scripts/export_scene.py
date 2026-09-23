"""Export the authored Blender kit for the browser without changing the .blend.

Run: blender -b ArtSource/DealStudio.blend --python web/scripts/export_scene.py
"""
from pathlib import Path
import bpy

out = Path(__file__).resolve().parents[1] / "public" / "assets"
out.mkdir(parents=True, exist_ok=True)
bpy.ops.object.select_all(action="SELECT")
bpy.ops.export_scene.gltf(
    filepath=str(out / "deal-studio.glb"),
    export_format="GLB",
    use_selection=False,
    export_apply=False,
    export_yup=True,
    export_cameras=False,
    export_lights=False,
)
print("WEB_EXPORT", out / "deal-studio.glb")
