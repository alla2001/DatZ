
modded class EPF_BasicRespawnSystemComponent : EPF_BaseRespawnSystemComponent
{


	//------------------------------------------------------------------------------------------------
	override protected void GetCreationPosition(int playerId, string characterPersistenceId, out vector position, out vector yawPitchRoll)
	{
		SCR_SpawnPoint spawnPoint = SCR_SpawnPoint.GetRandomSpawnPointDeathmatch();
		if (!spawnPoint)
		{
			Print("Could not spawn character, no spawn point on the map.", LogLevel.ERROR);
			return;
		}

		spawnPoint.GetPositionAndRotation(position, yawPitchRoll);
	}

	//------------------------------------------------------------------------------------------------
	
}
