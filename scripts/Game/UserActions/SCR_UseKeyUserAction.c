//------------------------------------------------------------------------------------------------
class SCR_UseKeyUserAction : ScriptedUserAction
{
	
	KeyComponent key;

	
	
	protected override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		SCR_GadgetComponent card = SCR_GadgetManagerComponent.Cast( pUserEntity.FindComponent(SCR_GadgetManagerComponent)).GetHeldGadgetComponent();
		key = KeyComponent.Cast(card);
		if(!key || !Replication.IsServer()) return;
		
		KeyDoorComponent.UseCard(key.Code);
		
		RplComponent.DeleteRplEntity(key.GetOwner(), false);
	
		
	}	
	
	protected override bool CanBePerformedScript(IEntity user)
	{
		SCR_GadgetComponent card = SCR_GadgetManagerComponent.Cast( user.FindComponent(SCR_GadgetManagerComponent)).GetHeldGadgetComponent();
		key = KeyComponent.Cast(card);
		if(key==null)	return false;
		
		
		return true;
	}
	
	protected override bool CanBeShownScript(IEntity user)
	{
		return true;
	}
	

};