// Script File
class DestroyFireAction : ScriptedUserAction
{	
	
	
	//------------------------------------------------------------------------------------------------
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent) 
	{
		
		
		
	}
	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if(GetOwner()==null)return;
		
		if(!Replication.IsServer()) return;
		
		
		RplComponent.DeleteRplEntity(GetOwner(), false);
		

		
	}

	override void OnActionCanceled(IEntity pOwnerEntity, IEntity pUserEntity)
	{
	
		
	}
	

	override void OnActionStart(IEntity pUserEntity) { 


		
	};

	//------------------------------------------------------------------------------------------------
	override bool CanBePerformedScript(IEntity user)
	{
		
		return GetOwner()!=null;
	}
	


	override bool CanBeShownScript(IEntity user)
	{
		
	
		return GetOwner()!=null;
	}
	
};