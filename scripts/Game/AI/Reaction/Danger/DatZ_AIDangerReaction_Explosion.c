// DatZ Modded Explosion Reaction - Hooks into AI danger events to notify zombie hearing
// This mods the base game's explosion reaction to also alert zombies

modded class SCR_AIDangerReaction_Explosion : SCR_AIDangerReaction
{
	//------------------------------------------------------------------------------------------------
	override bool PerformReaction(notnull SCR_AIUtilityComponent utility, notnull SCR_AIThreatSystem threatSystem, AIDangerEvent dangerEvent, int dangerEventCount)
	{
		// Let the base game handle normal AI reaction
		bool baseResult = super.PerformReaction(utility, threatSystem, dangerEvent, dangerEventCount);

		// Also notify zombie sound system
		NotifyZombieSoundSystem(dangerEvent);

		return baseResult;
	}

	//------------------------------------------------------------------------------------------------
	protected void NotifyZombieSoundSystem(AIDangerEvent dangerEvent)
	{
		if (!Replication.IsServer())
			return;

		DatZSoundManager soundMgr = DatZSoundManager.GetInstance();
		if (!soundMgr)
			return;

		vector explosionPos = dangerEvent.GetPosition();

		// Determine explosion type from instigator if possible
		int soundType = DatZSoundType.EXPLOSION;

		IEntity instigator = dangerEvent.GetObject();
		if (instigator)
		{
			// Check if it's a vehicle explosion
			Vehicle vehicle = Vehicle.Cast(instigator.GetRootParent());
			if (vehicle)
			{
				soundType = DatZSoundType.EXPLOSION_VEHICLE;
			}
			else
			{
				// Check prefab name for grenade/rocket
				EntityPrefabData prefabData = instigator.GetPrefabData();
				if (prefabData)
				{
					string prefabName = prefabData.GetPrefabName();

					if (prefabName.Contains("Grenade") || prefabName.Contains("M67") ||
						prefabName.Contains("RGD") || prefabName.Contains("Frag"))
					{
						soundType = DatZSoundType.EXPLOSION_GRENADE;
					}
					else if (prefabName.Contains("Rocket") || prefabName.Contains("RPG") ||
							 prefabName.Contains("LAW") || prefabName.Contains("MAAWS") ||
							 prefabName.Contains("Carl"))
					{
						soundType = DatZSoundType.EXPLOSION_ROCKET;
					}
				}
			}
		}

		// Emit to zombie sound system - explosions are loud!
		soundMgr.EmitSound(explosionPos, soundType, 1.0);
	}
}
