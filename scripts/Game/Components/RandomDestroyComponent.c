
class RandomDestroyComponentClass : ScriptComponentClass
{

}

class RandomDestroyComponent : ScriptComponent
{
   [Attribute("0.65")]
	float chance;
    override void OnPostInit(IEntity owner)
    {
        super.OnPostInit(owner);
	if(!Replication.IsServer())return;
       
           GetGame().GetCallqueue().CallLater(RandomDesc,delay:10000);
    }

    // Spawn the lock entity
    protected void RandomDesc()
    {
        SCR_DestructionMultiPhaseComponent desc = SCR_DestructionMultiPhaseComponent.Cast( GetOwner().FindComponent(SCR_DestructionMultiPhaseComponent));
        if (!desc)
            return;
 		
		if(Math.RandomFloat01()>=chance)return;
        desc.SetHealthScaled(Math.RandomInt(0,0));
		desc.SetHitZoneDamage(1000);

		if(desc.GetNumDamagePhases()<1)return;
		desc.GoToDamagePhase(desc.GetNumDamagePhases()-1,false);
      
    }

}
