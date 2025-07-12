//------------------------------------------------------------------------------------------------
/*
	Object responsible for handling respawn logic on the authority side.
*/
[BaseContainerProps(category: "Respawn")]
modded class SCR_AutoSpawnLogic : SCR_SpawnLogic
{
	


	//------------------------------------------------------------------------------------------------
	override protected void Spawn(int playerId)
	{/*
		// Player is disconnecting (and disappearance of controlled entity started this feedback loop).
		// Simply ignore such requests as it would create unwanted entities.
		int indexOf = m_DisconnectingPlayers.Find(playerId);
		if (indexOf != -1)
		{
			m_DisconnectingPlayers.Remove(indexOf);
			return;
		}
		
		array<Faction> factions = {};
		GetGame().GetFactionManager().GetFactionsList(factions);		
		
		Faction targetFaction;
		if (!GetForcedFaction(targetFaction))
			targetFaction = factions.GetRandomElement();
		
		// Request both
		if (!GetPlayerFactionComponent_S(playerId).RequestFaction(targetFaction))
		{
			// Try again later
		}

		//SCR_BasePlayerLoadout targetLoadout;
		//if (!GetForcedLoadout(targetLoadout))
			//targetLoadout = GetGame().GetLoadoutManager().GetRandomFactionLoadout(targetFaction);		
		
		/*if (!GetPlayerLoadoutComponent_S(playerId).RequestLoadout(targetLoadout))
		{
			// Try again later
		}*/
/*
		Faction faction =  GetPlayerFactionComponent_S(playerId).GetAffiliatedFaction();
		if (!faction)
		{
			OnPlayerSpawnFailed_S(playerId);
			return;
		}
	
		SCR_BasePlayerLoadout loadout = GetPlayerLoadoutComponent_S(playerId).GetLoadout();
		if (!loadout)
		{
			OnPlayerSpawnFailed_S(playerId);
			return;
		}

		SCR_SpawnPoint point = SCR_SpawnPoint.GetRandomSpawnPointForFaction(faction.GetFactionKey());
		if (!point)
		{
			OnPlayerSpawnFailed_S(playerId);
			return;
		}

		SCR_SpawnPointSpawnData data = new SCR_SpawnPointSpawnData(loadouts.GetRandomElement(), point.GetRplId());
		if (GetPlayerRespawnComponent_S(playerId).CanSpawn(data))
			DoSpawn(playerId, data);
		else
			OnPlayerSpawnFailed_S(playerId);*/
	}


};
