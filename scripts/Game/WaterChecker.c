


class WaterCheckerClass : ScriptComponentClass
{
}

class WaterChecker : ScriptComponent
{
    [Attribute("Path/To/YourEffect.ptc")]
    ResourceName m_sParticleEffect;
  	[Attribute()]
        float forwardOffset;
    IEntity m_pParticle;

	DatZMetabolsimHandler playerMetab;
	bool canDrink;
	private ChimeraAIControlComponent AI;
    override void OnPostInit(IEntity owner)
    {
        super.OnPostInit(owner);
		
   
			if(!Replication.IsServer())return;
        SetEventMask(owner, EntityEvent.FRAME);
		
		playerMetab = DatZMetabolsimHandler.Cast(owner.GetParent().FindComponent(DatZMetabolsimHandler));
		AI = ChimeraAIControlComponent.Cast( owner.GetParent().FindComponent(ChimeraAIControlComponent));
		
	
        // Spawn the particle effect
		
		EntitySpawnParams params = new EntitySpawnParams();
		params.Parent = GetOwner();
        m_pParticle = GetGame().SpawnEntityPrefab(Resource.Load(m_sParticleEffect));


    }

 

  

    override void EOnFrame(IEntity owner, float timeSlice)
    {
		if(!Replication.IsServer())return;
		if(AI!= null && AI.IsAIActivated())return;

		IEntity parent = owner.GetParent();
        if (!parent)
            return;
		
       

        vector mat[4];

        


        int headBoneIndex = parent.GetAnimation().GetBoneIndex("Head");
        if (headBoneIndex == -1)
            return;


        if (!parent.GetAnimation().GetBoneMatrix(headBoneIndex, mat))
            return;
 // Invert rotation as needed
        mat[0] = -mat[0];
        mat[2] = -mat[2];

       
        mat[3] = mat[3] + (mat[2] * forwardOffset);
		mat[3] = mat[3] - (mat[1] * 0.12);    // Downward
		
        owner.SetLocalTransform(mat);
        owner.Update();
		EWaterSurfaceType waterSurfaceType;
		float lake;
		SCR_WorldTools.GetWaterSurfaceY(GetGame().GetWorld(),owner.GetOrigin(),waterSurfaceType, lake);
		
		if(SCR_WorldTools.IsObjectUnderwater(owner)&&waterSurfaceType != EWaterSurfaceType.WST_OCEAN && !canDrink)
		{
			owner.GetWorldTransform(mat);
			
			m_pParticle.SetWorldTransform(mat);
			m_pParticle.Update();
			canDrink=true;
			Rpc(UpdateOwner,canDrink);
		}
		else
		{
			canDrink = false;
			Rpc(UpdateOwner,canDrink);
		}
		
    }
	

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	private void UpdateOwner(bool valcanDrink)
	{
		canDrink = valcanDrink
		
	}



}
