import unreal


MAP_PATH = "/Game/Maps/MainStage"
GAME_MODE_CLASS_PATH = "/Script/DealOrNoDealStage.DealStageGameMode"

STAGE_MODULES = [
    ("00_WorldShell_24m_x_18m", "/Script/DealOrNoDealStage.StageWorldShellModule", (0.0, 0.0, 0.0)),
    ("01_CentralPlatform_7p8m_x_5p8m", "/Script/DealOrNoDealStage.StagePlatformModule", (-100.0, 0.0, 0.0)),
    ("02_ModelStaircase_6_7_7_6", "/Script/DealOrNoDealStage.StageStaircaseModule", (0.0, 0.0, 0.0)),
    ("03_AmountBoard_26Values", "/Script/DealOrNoDealStage.StageAmountBoardModule", (420.0, 760.0, 0.0)),
    ("04_CityBackdrop_GrandArch", "/Script/DealOrNoDealStage.StageBackdropModule", (820.0, 0.0, 0.0)),
    ("05_BankerHighBooth", "/Script/DealOrNoDealStage.StageBankerBoothModule", (540.0, -845.0, 0.0)),
    ("06_Audience_Broken_U", "/Script/DealOrNoDealStage.StageAudienceModule", (0.0, 0.0, 0.0)),
    ("07_LightingRig_ShowCues", "/Script/DealOrNoDealStage.StageLightingModule", (0.0, 0.0, 0.0)),
    ("08_CameraRig_4Shots", "/Script/DealOrNoDealStage.StageCameraRig", (0.0, 0.0, 0.0)),
    ("09_InteractionDirector", "/Script/DealOrNoDealStage.StageInteractionDirector", (0.0, 0.0, 0.0)),
]


def log(message):
    unreal.log("[DealStageBuild] " + message)


def create_clean_level():
    if unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
        log("Loading existing level at " + MAP_PATH)
        loaded = unreal.EditorLevelLibrary.load_level(MAP_PATH)
        if not loaded:
            raise RuntimeError("Could not load level: " + MAP_PATH)
        for actor in unreal.EditorLevelLibrary.get_all_level_actors():
            if not isinstance(actor, unreal.WorldSettings):
                unreal.EditorLevelLibrary.destroy_actor(actor)
    else:
        log("Creating clean level at " + MAP_PATH)
        created = unreal.EditorLevelLibrary.new_level(MAP_PATH)
        if not created:
            raise RuntimeError("Could not create level: " + MAP_PATH)


def spawn_stage_modules():
    spawned = []
    for label, class_path, xyz in STAGE_MODULES:
        log("Spawning " + label)
        stage_class = unreal.load_class(None, class_path)
        if stage_class is None:
            raise RuntimeError("Compiled stage class is unavailable: " + class_path)
        actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
            stage_class,
            unreal.Vector(xyz[0], xyz[1], xyz[2]),
            unreal.Rotator(0.0, 0.0, 0.0),
        )
        if actor is None:
            raise RuntimeError("Failed to spawn stage module: " + label)
        actor.set_actor_label(label)
        actor.tags = ["Stage.Module", "Phase.01.Graybox"]
        try:
            actor.set_folder_path("DealOrNoDealStage")
        except Exception:
            pass
        spawned.append(actor)
        log("Spawned " + label)
    return spawned


def configure_world():
    world = unreal.EditorLevelLibrary.get_editor_world()
    if world is None:
        raise RuntimeError("Editor world is unavailable")

    game_mode_class = unreal.load_class(None, GAME_MODE_CLASS_PATH)
    world_settings = world.get_world_settings()
    if game_mode_class is not None:
        world_settings.set_editor_property("default_game_mode", game_mode_class)
    world_settings.set_actor_label("MainStage_WorldSettings")
    log("Configured stage game mode")


def set_editor_view():
    try:
        subsystem = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
        view_location = unreal.Vector(-2350.0, 0.0, 1050.0)
        target = unreal.Vector(300.0, 0.0, 255.0)
        view_rotation = unreal.MathLibrary.find_look_at_rotation(view_location, target)
        subsystem.set_level_viewport_camera_info(view_location, view_rotation)
        log("Set the editor viewport to the wide master camera")
    except Exception as exc:
        unreal.log_warning("[DealStageBuild] Could not set editor viewport: " + str(exc))


def save_level():
    saved = unreal.EditorLevelLibrary.save_current_level()
    if not saved:
        raise RuntimeError("Failed to save MainStage")
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    log("Saved /Game/Maps/MainStage")


def main():
    create_clean_level()
    spawn_stage_modules()
    configure_world()
    set_editor_view()
    save_level()
    log("GRAYBOX BUILD COMPLETE")


main()
