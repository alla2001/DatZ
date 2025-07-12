class SCR_DatZTransformeBaseAction : ScriptedUserAction
{
[Attribute()]
	ResourceName prefab;
	//------------------------------------------------------------------------------------------------
	override bool CanBeShownScript(IEntity user)
	{
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	override bool CanBePerformedScript(IEntity user)
	{
		UniversalInventoryStorageComponent storage = UniversalInventoryStorageComponent.Cast( GetOwner().FindComponent(UniversalInventoryStorageComponent));
		
		if(storage){
		
			array<IEntity> items();
			if(storage.GetAll(items)>0)
			return false;
					
		}
		return true;
	}
	
 	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity) 
{
		if(!Replication.IsServer())return;
	Resource rec = Resource.Load(prefab);
	if (!rec.IsValid()) return;

	EntitySpawnParams param();
	vector mat[4];
	GetOwner().GetTransform(mat);
	param.Transform = mat;

	IEntity ent =  GetGame().SpawnEntityPrefab(rec, params: param);
	SCR_EntityHelper.SnapToGround(ent,startOffset: "0 0.5 0",maxLength:30);
	// Defer deletion to next frame
	GetGame().GetCallqueue().CallLater(DeleteThis, 0);
}

void DeleteThis()
{
	delete GetOwner();
}

}