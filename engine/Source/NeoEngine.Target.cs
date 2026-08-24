using UnrealBuildTool;
using System.Collections.Generic;

public class NeoEngineTarget : TargetRules
{
	public NeoEngineTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("NeoEngine");
		ExtraModuleNames.Add("ProjectName");
	}
}
