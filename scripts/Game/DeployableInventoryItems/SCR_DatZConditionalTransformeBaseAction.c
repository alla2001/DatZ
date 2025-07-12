class SCR_DatZConditionalTransformeBaseAction : ScriptedUserAction
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
		
		BaseWeaponComponent weapon =BaseWeaponManagerComponent.Cast( user.FindComponent(BaseWeaponManagerComponent)).GetCurrentWeapon();
		
		if(weapon==null)	return false;
		
		SCR_MeleeWeaponProperties prop =  SCR_MeleeWeaponProperties.Cast(weapon.GetOwner().FindComponent(SCR_MeleeWeaponProperties));
		if(prop==null)	return false;
		
		if(!prop.m_fIsKnife)	return false;
		
		
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
	Resource rec = Resource.Load(prefab);
	if (!rec.IsValid()) return;

	EntitySpawnParams param();
	vector mat[4];
	GetOwner().GetTransform(mat);
	param.Transform = mat;

	GetGame().SpawnEntityPrefab(rec, params: param);

	// Defer deletion to next frame
	GetGame().GetCallqueue().CallLater(DeleteThis, 100);
}

void DeleteThis()
{
	delete GetOwner();
}

}