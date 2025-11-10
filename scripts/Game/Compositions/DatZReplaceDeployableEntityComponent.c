[EntityEditorProps(category: "GameScripted/DeployableItems", description: "")]
class DatZReplaceDeployableEntityComponentClass : ScriptComponentClass
{
	// Setup
	[Attribute(uiwidget: UIWidgets.ResourcePickerThumbnail, desc: "Prefab which will be spawned when this item is deployed", params: "et", category: "Setup")]
	protected ResourceName m_sReplacementPrefab;

	//------------------------------------------------------------------------------------------------
	ResourceName GetReplacementPrefab()
	{
		return m_sReplacementPrefab;
	}
}

//! To be reowrked when 1.2 dropps
class DatZReplaceDeployableEntityComponent : ScriptComponent
{
	
	[Attribute(defvalue: "1", category: "General")]
	protected bool m_bEnableSounds;



	[RplProp(onRplName: "OnRplDeployed")]
	protected bool m_bIsDeployed;

	protected IEntity m_ReplacementEntity;
	protected vector m_aOriginalTransform[4];

	protected RplComponent m_RplComponent;

	protected bool m_bIsDeploying;

	[RplProp()]
	protected int m_iItemOwnerID = -1;


	
	protected bool m_bWasDeployed;
	protected IEntity m_PreviousOwner;

	//------------------------------------------------------------------------------------------------
	void SetDeploying(bool isDeploying)
	{
		m_bIsDeploying = isDeploying;
	}

	//------------------------------------------------------------------------------------------------
	bool IsDeploying()
	{
		return m_bIsDeploying;
	}

	

	

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_SetTransformBroadcast(vector transform[4])
	{
		GetOwner().SetTransform(transform);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Unreliable, RplRcver.Broadcast)]
	protected void RPC_PlaySoundOnDeployBroadcast(bool deploy)
	{
		SoundComponent soundComp = SoundComponent.Cast(GetOwner().FindComponent(SoundComponent));
		if (soundComp)
		{
			if (deploy)
				soundComp.SoundEvent(SCR_SoundEvent.SOUND_DEPLOY);
			else
				soundComp.SoundEvent(SCR_SoundEvent.SOUND_UNDEPLOY);

			return;
		}

		/*SCR_SoundManagerEntity soundMan = GetGame().GetSoundManagerEntity();
		if (!soundMan)
			return;

		if (deploy)
			soundMan.CreateAndPlayAudioSource(GetOwner(), SCR_SoundEvent.SOUND_DEPLOY);
		else
			soundMan.CreateAndPlayAudioSource(GetOwner(), SCR_SoundEvent.SOUND_UNDEPLOY);*/
	}

	//------------------------------------------------------------------------------------------------
	//! Gets called when deploy action is executed by player
	//! \param[in] userEntity
	void Deploy(IEntity userEntity = null, bool reload = false)
	{
		if (!m_RplComponent || m_RplComponent.IsProxy())
			return;

		if (m_bIsDeployed)
			return;
		
		vector transform[4];
		IEntity owner = GetOwner();
		SCR_TerrainHelper.GetTerrainBasis(owner.GetOrigin(), transform, GetGame().GetWorld(), false, new TraceParam());

		m_aOriginalTransform = transform;

		EntitySpawnParams params = new EntitySpawnParams();
		params.Transform = m_aOriginalTransform;
		params.TransformMode = ETransformMode.WORLD;

		DatZReplaceDeployableEntityComponentClass data = DatZReplaceDeployableEntityComponentClass.Cast(GetComponentData(owner));
		if (!data)
			return;

		Resource resource = Resource.Load(data.GetReplacementPrefab());
		if (!resource.IsValid())
			return;

		m_ReplacementEntity = GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), params);
		if (!m_ReplacementEntity)
			return;

		

		m_bIsDeployed = true;
		if (userEntity)
			m_iItemOwnerID = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(userEntity);
		
		Replication.BumpMe();

		if (m_bEnableSounds && !reload)
		{
			RPC_PlaySoundOnDeployBroadcast(m_bIsDeployed);
			Rpc(RPC_PlaySoundOnDeployBroadcast, m_bIsDeployed);
		}

		SCR_GarbageSystem garbageSystem = SCR_GarbageSystem.GetByEntityWorld(owner);
		if (garbageSystem)
			garbageSystem.Withdraw(owner);
		RplComponent.DeleteRplEntity(owner, false);

	}

	
	

	//------------------------------------------------------------------------------------------------
	//!
	protected void OnRplDeployed();

	//------------------------------------------------------------------------------------------------
	//!
	//! \param[in] userEntity
	//! \return
	bool CanDeployBeShown(notnull IEntity userEntity)
	{
		return !m_bIsDeployed;
	}

	//------------------------------------------------------------------------------------------------
	//!
	//! \param[in] userEntity
	//! \return
	bool CanDismantleBeShown(notnull IEntity userEntity)
	{
		return m_bIsDeployed;
	}

	//------------------------------------------------------------------------------------------------
	//!
	//! \return
	bool IsDeployed()
	{
		return m_bIsDeployed;
	}

	//------------------------------------------------------------------------------------------------
	//! \return
	int GetItemOwnerID()
	{
		return m_iItemOwnerID;
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(GetOwner(), EntityEvent.INIT);
		m_RplComponent = RplComponent.Cast(GetOwner().FindComponent(RplComponent));
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		super.OnDelete(owner);

	

		
	}


	
}
