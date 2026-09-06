"""Import authored studio assets and regenerate the independently editable stage modules.
Compile Editor first; run with UnrealEditor-Cmd -ExecutePythonScript=<this file>.
"""
import json
from pathlib import Path
import unreal

ROOT = Path(unreal.Paths.project_dir()).resolve()
MAT_DIR = '/Game/Studio/Materials'
MESH_DIR = '/Game/Studio/Meshes'
LIB = unreal.MaterialEditingLibrary
ASSETS = unreal.AssetToolsHelpers.get_asset_tools()

def log(s): unreal.log('[StudioBuild] '+s)

def material(name, color, metallic, roughness, emissive, tint=False):
    path=MAT_DIR+'/M_'+name
    mat=unreal.load_asset(path)
    if mat is not None:
        # Reuse editable materials. Deleting rooted expressions referenced by runtime
        # class defaults can assert in UE 5.7 during commandlet reimport.
        return mat
    mat=ASSETS.create_asset('M_'+name,MAT_DIR,unreal.Material,unreal.MaterialFactoryNew())
    base=LIB.create_material_expression(mat,unreal.MaterialExpressionVectorParameter,-500,0)
    base.set_editor_property('parameter_name','BaseColor')
    base.set_editor_property('default_value',unreal.LinearColor(*color))
    LIB.connect_material_property(base,'',unreal.MaterialProperty.MP_BASE_COLOR)
    for param,value,prop in [('Metallic',metallic,unreal.MaterialProperty.MP_METALLIC),
                              ('Roughness',roughness,unreal.MaterialProperty.MP_ROUGHNESS)]:
        n=LIB.create_material_expression(mat,unreal.MaterialExpressionScalarParameter,-300,160)
        n.set_editor_property('parameter_name',param); n.set_editor_property('default_value',value)
        LIB.connect_material_property(n,'',prop)
    if emissive:
        strength=LIB.create_material_expression(mat,unreal.MaterialExpressionConstant,-500,330)
        strength.set_editor_property('r',emissive)
        mul=LIB.create_material_expression(mat,unreal.MaterialExpressionMultiply,-180,300)
        LIB.connect_material_expressions(base,'',mul,'A'); LIB.connect_material_expressions(strength,'',mul,'B')
        LIB.connect_material_property(mul,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    if name=='Glass':
        mat.set_editor_property('blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT)
        mat.set_editor_property('two_sided',True)
        mat.set_editor_property('translucency_lighting_mode',unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
        opacity=LIB.create_material_expression(mat,unreal.MaterialExpressionConstant,-300,430)
        opacity.set_editor_property('r',color[3])
        LIB.connect_material_property(opacity,'',unreal.MaterialProperty.MP_OPACITY)
    LIB.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)
    return mat

def import_art():
    specs=json.loads((ROOT/'ArtSource/Export/materials.json').read_text())
    mats={name:material(name,*values) for name,values in specs.items()}
    material('Tint',[.2,.35,.5,1],.35,.32,.42,True)
    tasks=[]
    for file in sorted((ROOT/'ArtSource/Export').glob('*.fbx')):
        t=unreal.AssetImportTask()
        t.set_editor_property('filename',str(file));t.set_editor_property('destination_path',MESH_DIR)
        t.set_editor_property('destination_name',file.stem)
        t.set_editor_property('automated',True);t.set_editor_property('replace_existing',True);t.set_editor_property('save',True)
        options=unreal.FbxImportUI()
        options.set_editor_property('import_mesh',True)
        options.set_editor_property('import_materials',False);options.set_editor_property('import_textures',False)
        options.set_editor_property('import_as_skeletal',False)
        options.set_editor_property('mesh_type_to_import',unreal.FBXImportType.FBXIT_STATIC_MESH)
        data=options.get_editor_property('static_mesh_import_data')
        data.set_editor_property('combine_meshes',True)
        data.set_editor_property('auto_generate_collision',True)
        data.set_editor_property('convert_scene',True)
        data.set_editor_property('convert_scene_unit',True)
        t.set_editor_property('options',options);tasks.append(t)
    ASSETS.import_asset_tasks(tasks)
    for task in tasks:
        paths=task.get_editor_property('imported_object_paths')
        if not paths: raise RuntimeError('No asset imported: '+task.get_editor_property('filename'))
        for path in paths:
            mesh=unreal.load_asset(path)
            if not isinstance(mesh,unreal.StaticMesh):continue
            slots=mesh.get_editor_property('static_materials')
            for i,slot in enumerate(slots):
                name=str(slot.get_editor_property('material_slot_name')).split('.')[0]
                if name not in mats: raise RuntimeError('Unknown material slot: '+name)
                mesh.set_material(i,mats[name])
            unreal.EditorAssetLibrary.save_loaded_asset(mesh)
            bounds=mesh.get_bounds()
            log(f'{mesh.get_name()} origin={bounds.origin} extent={bounds.box_extent}')
    audio=[]
    for file in (ROOT/'ArtSource/Audio').glob('*.wav'):
        t=unreal.AssetImportTask();t.filename=str(file);t.destination_path='/Game/Studio/Audio'
        t.automated=True;t.replace_existing=True;t.save=True;audio.append(t)
    ASSETS.import_asset_tasks(audio)

def build_level():
    # Editable material instances expose finish tuning without rebuilding shader graphs.
    for name in json.loads((ROOT/'ArtSource/Export/materials.json').read_text()):
        path=MAT_DIR+'/MI_'+name
        instance=unreal.load_asset(path)
        if instance is None:
            instance=ASSETS.create_asset('MI_'+name,MAT_DIR,unreal.MaterialInstanceConstant,unreal.MaterialInstanceConstantFactoryNew())
            LIB.set_material_instance_parent(instance,unreal.load_asset(MAT_DIR+'/M_'+name))
        if name=='Floor': LIB.set_material_instance_scalar_parameter_value(instance,'Roughness',.34)
        if name=='Chrome': LIB.set_material_instance_scalar_parameter_value(instance,'Roughness',.28)
        unreal.EditorAssetLibrary.save_loaded_asset(instance)
    for path in unreal.EditorAssetLibrary.list_assets(MESH_DIR,recursive=False):
        mesh=unreal.load_asset(path)
        if not isinstance(mesh,unreal.StaticMesh): continue
        for i,slot in enumerate(mesh.get_editor_property('static_materials')):
            name=str(slot.get_editor_property('material_slot_name')).split('.')[0]
            mesh.set_material(i,unreal.load_asset(MAT_DIR+'/MI_'+name))
        editor=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
        settings=editor.get_lod_build_settings(mesh,0)
        settings.set_editor_property('max_lumen_mesh_cards',32)
        editor.set_lod_build_settings(mesh,0,settings)
        unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    # Reuse the stable ten-actor generation path. Do not nest child-actor assemblies.
    script=ROOT/'Content/Python/build_stage.py'
    exec(compile(script.read_text(),str(script),'exec'),{'__name__':'__main__'})
    actors=unreal.EditorLevelLibrary.get_all_level_actors()
    hide_classes=(unreal.StageWorldShellModule,unreal.StagePlatformModule,unreal.StageBackdropModule,unreal.StageBankerBoothModule)
    for actor in actors:
        if isinstance(actor,hide_classes):
            for comp in actor.get_components_by_class(unreal.PrimitiveComponent):
                comp.set_visibility(False,False);comp.set_hidden_in_game(True,False)
                comp.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
        if isinstance(actor,unreal.StageStaircaseModule):
            for comp in actor.get_components_by_class(unreal.StaticMeshComponent):
                name=comp.get_name()
                if name.startswith('Tier_'):
                    comp.set_visibility(False);comp.set_hidden_in_game(True)
                elif name.startswith('Briefcase_'):
                    comp.set_static_mesh(unreal.load_asset(MESH_DIR+'/SM_BriefcaseBody'))
                elif name.startswith('BriefcaseLid_'):
                    comp.set_static_mesh(unreal.load_asset(MESH_DIR+'/SM_BriefcaseLid'))
        if isinstance(actor,unreal.StageAudienceModule):
            for comp in actor.get_components_by_class(unreal.StaticMeshComponent):
                if comp.get_name().startswith('AudienceRiser_'):
                    comp.set_visibility(False);comp.set_hidden_in_game(True)
        if isinstance(actor,unreal.StageAmountBoardModule):
            for comp in actor.get_components_by_class(unreal.StaticMeshComponent):
                if comp.get_name().startswith('AmountBoard_Housing'):
                    comp.set_visibility(False);comp.set_hidden_in_game(True)
        if isinstance(actor,unreal.StageModuleBase): actor.tags=['Stage.Module','Studio.0.4']
    for name in ['StudioShell','GamePlatform','BankerPhone','CaseTerraces','ArchSkyline','BankerSuite','AmountDisplay','AudienceArchitecture','LightingHardware']:
        actor=unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(0,0,0))
        actor.set_actor_label('Studio_'+name)
        actor.set_folder_path('Studio / Authored architecture')
        comp=actor.static_mesh_component
        comp.set_static_mesh(unreal.load_asset(MESH_DIR+'/SM_'+name))
        # FBX converts right-handed Blender coordinates to Unreal by reflecting Y.
        # Restore our explicit +Y = board side convention for world-space architecture.
        actor.set_actor_scale3d(unreal.Vector(1,-1,1))
        comp.set_mobility(unreal.ComponentMobility.STATIC)
        comp.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
        actor.tags=['Studio.Authored','NoPeople']
    fog=unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.ExponentialHeightFog,unreal.Vector(0,0,-80))
    fog.set_actor_label('Studio_SubtleAtmosphere')
    c=fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
    c.set_editor_property('fog_density',.006)
    c.set_editor_property('fog_height_falloff',.2)
    c.set_editor_property('fog_max_opacity',.22)
    c.set_editor_property('enable_volumetric_fog',True)
    c.set_editor_property('volumetric_fog_extinction_scale',.25)
    post=unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.PostProcessVolume,unreal.Vector(0,0,0))
    post.set_actor_label('Studio_ExposureAndReflections')
    post.set_editor_property('unbound',True)
    settings=post.get_editor_property('settings')
    for key,value in [('bloom_intensity',.18),('vignette_intensity',.18),('motion_blur_amount',0.0),('auto_exposure_bias',-1.5)]:
        settings.set_editor_property('override_'+key,True)
        settings.set_editor_property(key,value)
    post.set_editor_property('settings',settings)
    unreal.EditorLevelLibrary.save_current_level()
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True,True)
    log('COMPLETE: 10 runtime modules, 9 authored architecture groups, no people')

if '-StudioSkipImport' not in unreal.SystemLibrary.get_command_line():
    import_art()
build_level()
