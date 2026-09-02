using UnrealBuildTool;
using System.Collections.Generic;

public class DealOrNoDealStageTarget : TargetRules
{
    public DealOrNoDealStageTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V6;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("DealOrNoDealStage");
    }
}
