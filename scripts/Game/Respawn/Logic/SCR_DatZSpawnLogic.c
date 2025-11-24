//------------------------------------------------------------------------------------------------
/*
	Object responsible for handling respawn logic on the authority side.
*/
[BaseContainerProps(category: "Respawn")]
modded class SCR_AutoSpawnLogic : SCR_SpawnLogic
{
	
	[Attribute("", params: "et", category: "Prefabs")]
	protected ref array <ResourceName> loadouts;
	//------------------------------------------------------------------------------------------------
	override protected void DoSpawn_S(int playerId)
	{
		array<Faction> factions = {};
		GetGame().GetFactionManager().GetFactionsList(factions);

		Faction targetFaction;
		if (!GetForcedFaction(targetFaction))
			targetFaction = factions.GetRandomElement();

		GetPlayerFactionComponent_S(playerId).RequestFaction(targetFaction);

		SCR_BasePlayerLoadout targetLoadout;
		if (!GetForcedLoadout(targetLoadout))
			targetLoadout = GetGame().GetLoadoutManager().GetRandomFactionLoadout(targetFaction);

		GetPlayerLoadoutComponent_S(playerId).RequestLoadout(targetLoadout);

		Faction faction = GetPlayerFactionComponent_S(playerId).GetAffiliatedFaction();
		if (!faction)
		{
			OnPlayerSpawnFailed_S(playerId);
			return;
		}

		ResourceName loadout = loadouts.GetRandomElement();
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

		SCR_SpawnPointSpawnData data = new SCR_SpawnPointSpawnData(loadout, point.GetRplId());
		if (!GetPlayerRespawnComponent_S(playerId).CanSpawn(data))
		{
			OnPlayerSpawnFailed_S(playerId);
			return;
		}

		GetPlayerRespawnComponent_S(playerId).RequestSpawn(data);
	}






};
