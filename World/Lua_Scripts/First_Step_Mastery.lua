-- Quest Script: First_Step_Mastery
function OnQuestStart()
    print("Quest First_Step_Mastery Started")
    Aries.NPC.SetState("IDLE")
end

function OnObjectiveComplete()
    Aries.Memory.GC_Trigger() -- S7 Optimization: Clean RAM after event
end
