using UnrealBuildTool;
using System.Collections.Generic;

public class DealOrNoDealStageEditorTarget : TargetRules
{
    public DealOrNoDealStageEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V6;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("DealOrNoDealStage");
    }
}
