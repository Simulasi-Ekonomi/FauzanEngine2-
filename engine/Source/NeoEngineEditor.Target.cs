using UnrealBuildTool;
using System.Collections.Generic;

public class NeoEngineEditorTarget : TargetRules
{
    public NeoEngineEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        ExtraModuleNames.Add("NeoEngine");
    }
}
