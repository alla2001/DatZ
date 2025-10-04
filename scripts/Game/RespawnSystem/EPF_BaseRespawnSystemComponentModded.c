

modded class EPF_BaseRespawnSystemComponent : SCR_RespawnSystemComponent
{
	[Attribute("", params: "et", category: "Prefabs")]
	protected ref array <ResourceName> loadouts;
	//------------------------------------------------------------------------------------------------
	override protected void CreateCharacter(int playerId, string characterPersistenceId)
	{
		ResourceName prefab = GetCreationPrefab(playerId, characterPersistenceId);

		vector position, yawPitchRoll;
		
		SCR_SpawnPoint point = SCR_SpawnPoint.GetRandomSpawnPointDeathmatch();
		if (!point)
		{
			GetCreationPosition(playerId, characterPersistenceId, position, yawPitchRoll);
		
		}else {
			position = point.GetOrigin();
		}
		

		#ifdef WORKBENCH
		if (m_bUseFromCamera)
		{
			position = m_vFromCameraPosition;
			yawPitchRoll = m_vFromCameraYPR;
		}
		#endif

		IEntity character = EPF_Utils.SpawnEntityPrefab(prefab, position + "0 0.1 0", yawPitchRoll);
		m_mLoadingCharacters.Set(playerId, character);

		EPF_PersistenceComponent persistenceComponent = EPF_Component<EPF_PersistenceComponent>.Find(character);
		if (persistenceComponent)
		{
			persistenceComponent.SetPersistentId(characterPersistenceId);
			OnCharacterCreated(playerId, characterPersistenceId, character);
			HandoverToPlayer(playerId, character);
		}
		else
		{
			Print(string.Format("Could not create new character, prefab '%1' is missing component '%2'.", prefab, EPF_PersistenceComponent), LogLevel.ERROR);
			SCR_EntityHelper.DeleteEntityAndChildren(character);
			return;
		}
	}
	override	protected void CreateMYCharacter(int playerId, string characterPersistenceId,int nut,string lc)	{
	if(!LoadJson())return;
		ResourceName prefab = GetCreationPrefab(playerId, characterPersistenceId);
		vector position, yawPitchRoll;
	SCR_SpawnPoint point = SCR_SpawnPoint.GetRandomSpawnPointDeathmatch();
		if (!point)
		{
			GetCreationPosition(playerId, characterPersistenceId, position, yawPitchRoll);
		
		}else {
			position = point.GetOrigin();
		}
		#ifdef WORKBENCH
		if (m_bUseFromCamera)	{
			position = m_vFromCameraPosition;
			yawPitchRoll = m_vFromCameraYPR;
		}
		#endif
		IEntity character = EPF_Utils.SpawnEntityPrefab(prefab, position + "0 0.1 0", yawPitchRoll);
		m_mLoadingCharacters.Set(playerId, character);
		EPF_PersistenceComponent persistenceComponent = EPF_Component<EPF_PersistenceComponent>.Find(character);
		if (persistenceComponent)	{
			BLD_BedSpawnComponent job = BLD_BedSpawnComponent.Cast(character.FindComponent(BLD_BedSpawnComponent));
			if (job) {
				job.ServerSetCD(nut);
				job.lastCode = lc;			
			}	
			persistenceComponent.SetPersistentId(characterPersistenceId);
			OnCharacterCreated(playerId, characterPersistenceId, character);
			HandoverToPlayer(playerId, character);
		}
		else	{
			Print(string.Format("Could not create new character, prefab '%1' is missing component '%2'.", prefab, EPF_PersistenceComponent), LogLevel.ERROR);
			SCR_EntityHelper.DeleteEntityAndChildren(character);
			return;
		}
	}
	
	
	
	
	//------------------------------------------------------------------------------------------------
	//! Prefab for a newly created chracter
	override  protected ResourceName GetCreationPrefab(int playerId, string characterPersistenceId)
	{
		return loadouts.GetRandomElement();
	}
	//------------------------------------------------------------------------------------------------
	/*protected --Hotfix for 1.0 DO NOT CALL THIS MANUALLY*/
	override void WaitForUid(int playerId)
	{
		if(!LoadJson())return;
		// Wait one frame after audit/sp join, then it is available.
		// TODO: Remove this method once https://feedback.bistudio.com/T165590 is fixed.
		GetGame().GetCallqueue().Call(OnUidAvailable, playerId);
	}
	
	
	
	//------------------------------------------------------------------------------------------------
	bool LoadJson()
	{
		SCR_JsonLoadContext context = TW_Util.LoadJsonFile("Lockers.json", true);
		bool val=false;
		bool loadSuccess = context.ReadValue("inserver", val);
		
		if(!loadSuccess)
		{
			Print("TrainWreck: Was unable to load loot map. Please verify it exists, and has valid syntax");
			return false;
		}
		return true;
	}
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	

}
