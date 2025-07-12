class SCR_DatZDeployInventoryItemBaseAction : ScriptedUserAction
{
	protected DatZReplaceDeployableEntityComponent m_DeployableItemComp;
	
	protected RplComponent m_RplComp;
	
	protected bool m_bActionStarted;
	[RplProp()]
	protected bool m_bCanBeDismanteled;
	//------------------------------------------------------------------------------------------------
	override bool CanBeShownScript(IEntity user)
	{
		if (!m_DeployableItemComp)
			return false;
		
	
		return m_DeployableItemComp.CanDeployBeShown(user);
	}
	
	//------------------------------------------------------------------------------------------------
	override bool CanBePerformedScript(IEntity user)
	{
		if (!m_DeployableItemComp)
			return false;
		IEntity owner = GetOwner();
		if(!owner)
		return false;
		SCR_UniversalInventoryStorageComponent storage = SCR_UniversalInventoryStorageComponent.Cast(GetOwner().FindComponent(SCR_UniversalInventoryStorageComponent)); 
		if(storage)
		{
			//if(!storage.isEmpty)
			//return false;
			
			array<IEntity> items();
			if(storage.GetAll(items)>0)
			return false;
		}
		
	
		return m_bActionStarted || !m_DeployableItemComp.IsDeploying();
	}
	
	
	[RplRpc(RplChannel.Reliable,RplRcver.Server)]
	void RPC_IsEmpty()
	{
	
		
		m_bCanBeDismanteled = true;
		Replication.BumpMe();
	}
	
 	//------------------------------------------------------------------------------------------------
 	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity) 
 	{
		if (!m_DeployableItemComp)
			return;
		
		m_DeployableItemComp.Deploy(pUserEntity);
		GetGame().GetCallqueue().CallLater(ResetDeployingDelayed, 100, param1: pUserEntity); //reset bool later so there is enough time for user action to disappear
 	}
	
	//------------------------------------------------------------------------------------------------
	protected void ResetDeployingDelayed(IEntity pUserEntity)
	{
		m_DeployableItemComp.SetDeploying(false);
		
		PlayerController playerController = GetGame().GetPlayerController();
		if (!playerController)
			return;
		
		IEntity controlledEntity = playerController.GetControlledEntity();
		if (!controlledEntity)
			return;
		
		if (pUserEntity == controlledEntity)
			m_bActionStarted = false;
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnActionStart(IEntity pUserEntity)
	{
		if (!m_DeployableItemComp)
			return;
		
		m_DeployableItemComp.SetDeploying(true);
		
		PlayerController playerController = GetGame().GetPlayerController();
		if (!playerController)
			return;
		
		IEntity controlledEntity = playerController.GetControlledEntity();
		if (!controlledEntity)
			return;
		
		if (pUserEntity == controlledEntity)
			m_bActionStarted = true;
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnActionCanceled(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!m_DeployableItemComp)
			return;
		
		m_DeployableItemComp.SetDeploying(false);
		
		PlayerController playerController = GetGame().GetPlayerController();
		if (!playerController)
			return;
		
		IEntity controlledEntity = playerController.GetControlledEntity();
		if (!controlledEntity)
			return;
		
		if (pUserEntity == controlledEntity)
			m_bActionStarted = false;
	}
	
	//------------------------------------------------------------------------------------------------
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{	
		IEntity owner = GetOwner();
		if (!owner)
			return;
		
		m_DeployableItemComp = DatZReplaceDeployableEntityComponent.Cast(owner.FindComponent(DatZReplaceDeployableEntityComponent));
		m_RplComp = RplComponent.Cast(owner.FindComponent(RplComponent));
	}
}