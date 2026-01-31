// DatZ Weapon Fire Listener - Direct hook for player weapon fire
// Hooks into player weapon fire events to notify zombie hearing system
// This bypasses the AI danger event system which requires AI soldiers to process events

[EntityEditorProps(category: "GameScripted/DatZ", description: "Listens for weapon fire and notifies zombie hearing - add to GameMode")]
class DatZWeaponFireListenerClass : SCR_BaseGameModeComponentClass
{
}

class DatZWeaponFireListener : SCR_BaseGameModeComponent
{
	// Track registered players to avoid duplicate handlers
	protected ref set<int> m_RegisteredPlayers = new set<int>();

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!Replication.IsServer())
			return;

		// Subscribe to player spawn events
		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(owner);
		if (gameMode)
		{
			gameMode.GetOnPlayerSpawned().Insert(OnPlayerSpawned);
			gameMode.GetOnPlayerKilled().Insert(OnPlayerKilledHandler);
			gameMode.GetOnPlayerDisconnected().Insert(OnPlayerDisconnectedHandler);
		}
	}

	//------------------------------------------------------------------------------------------------
	void ~DatZWeaponFireListener()
	{
		if (!GetOwner())
			return;

		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetOwner());
		if (gameMode)
		{
			gameMode.GetOnPlayerSpawned().Remove(OnPlayerSpawned);
			gameMode.GetOnPlayerKilled().Remove(OnPlayerKilledHandler);
			gameMode.GetOnPlayerDisconnected().Remove(OnPlayerDisconnectedHandler);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void OnPlayerSpawned(int playerId, IEntity controlledEntity)
	{
		if (!controlledEntity)
			return;

		RegisterPlayerWeaponHandler(playerId, controlledEntity);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnPlayerKilledHandler(notnull SCR_InstigatorContextData instigatorContextData)
	{
		int playerId = instigatorContextData.GetVictimPlayerID();
		IEntity playerEntity = instigatorContextData.GetVictimEntity();
		UnregisterPlayerWeaponHandler(playerId, playerEntity);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnPlayerDisconnectedHandler(int playerId, KickCauseCode cause, int timeout)
	{
		int idx = m_RegisteredPlayers.Find(playerId);
		if (idx >= 0)
			m_RegisteredPlayers.Remove(idx);
	}

	//------------------------------------------------------------------------------------------------
	protected void RegisterPlayerWeaponHandler(int playerId, IEntity playerEntity)
	{
		if (!playerEntity || m_RegisteredPlayers.Contains(playerId))
			return;

		EventHandlerManagerComponent eventHandler = EventHandlerManagerComponent.Cast(playerEntity.FindComponent(EventHandlerManagerComponent));
		if (!eventHandler)
			return;

		eventHandler.RegisterScriptHandler("OnProjectileShot", this, OnWeaponFired);
		eventHandler.RegisterScriptHandler("OnGrenadeThrown", this, OnGrenadeThrown);

		// Register melee listener
		SCR_MeleeComponent meleeComp = SCR_MeleeComponent.Cast(playerEntity.FindComponent(SCR_MeleeComponent));
		if (meleeComp)
			meleeComp.GetOnMeleePerformed().Insert(OnMeleePerformed);

		m_RegisteredPlayers.Insert(playerId);
	}

	//------------------------------------------------------------------------------------------------
	protected void UnregisterPlayerWeaponHandler(int playerId, IEntity playerEntity)
	{
		if (!playerEntity || !m_RegisteredPlayers.Contains(playerId))
			return;

		EventHandlerManagerComponent eventHandler = EventHandlerManagerComponent.Cast(playerEntity.FindComponent(EventHandlerManagerComponent));
		if (eventHandler)
		{
			eventHandler.RemoveScriptHandler("OnProjectileShot", this, OnWeaponFired);
			eventHandler.RemoveScriptHandler("OnGrenadeThrown", this, OnGrenadeThrown);
		}

		SCR_MeleeComponent meleeComp = SCR_MeleeComponent.Cast(playerEntity.FindComponent(SCR_MeleeComponent));
		if (meleeComp)
			meleeComp.GetOnMeleePerformed().Remove(OnMeleePerformed);

		int idx = m_RegisteredPlayers.Find(playerId);
		if (idx >= 0)
			m_RegisteredPlayers.Remove(idx);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnWeaponFired(int playerID, BaseWeaponComponent weapon, IEntity projectile)
	{
		if (!Replication.IsServer())
			return;

		DatZSoundManager soundMgr = DatZSoundManager.GetInstance();
		if (!soundMgr)
			return;

		// Get shooter position
		IEntity shooter = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerID);
		if (!shooter)
			return;

		vector shotPos = shooter.GetOrigin();

		// Determine if suppressed using proper API
		bool isSuppressed = IsWeaponSuppressed(weapon);

		// Get sound type using proper weapon type API
		int soundType = DetermineWeaponSoundType(weapon, isSuppressed);

		// Emit to zombie sound system
		soundMgr.EmitSound(shotPos, soundType, 1.0);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnGrenadeThrown(int playerID, BaseWeaponComponent weapon, IEntity grenade)
	{
		// Grenades will trigger explosion events when they detonate
		// No need to notify here for the throw itself
	}

	//------------------------------------------------------------------------------------------------
	protected void OnMeleePerformed(IEntity owner)
	{
		if (!Replication.IsServer())
			return;

		DatZSoundManager soundMgr = DatZSoundManager.GetInstance();
		if (!soundMgr)
			return;

		if (!owner)
			return;

		vector meleePos = owner.GetOrigin();

		// Melee swing sound
		soundMgr.EmitSound(meleePos, DatZSoundType.MELEE_SWING, 1.0);
	}

	//------------------------------------------------------------------------------------------------
	// Use proper engine API to check suppressor
	protected bool IsWeaponSuppressed(BaseWeaponComponent weapon)
	{
		if (!weapon)
			return false;

		BaseMuzzleComponent muzzle = weapon.GetCurrentMuzzle();
		if (!muzzle)
			return false;

		// Use the proper engine method
		return muzzle.IsMuzzleSuppressed();
	}

	//------------------------------------------------------------------------------------------------
	// Use proper weapon type enum for accurate classification
	protected int DetermineWeaponSoundType(BaseWeaponComponent weapon, bool isSuppressed)
	{
		if (!weapon)
		{
			if (isSuppressed)
				return DatZSoundType.GUNSHOT_RIFLE_SUPPRESSED;
			return DatZSoundType.GUNSHOT_RIFLE;
		}

		// Use the proper engine weapon type
		EWeaponType weaponType = weapon.GetWeaponType();

		switch (weaponType)
		{
			case EWeaponType.WT_HANDGUN:
			{
				if (isSuppressed)
					return DatZSoundType.GUNSHOT_PISTOL_SUPPRESSED;
				return DatZSoundType.GUNSHOT_PISTOL;
			}

			case EWeaponType.WT_SNIPERRIFLE:
			{
				if (isSuppressed)
					return DatZSoundType.GUNSHOT_SNIPER_SUPPRESSED;
				return DatZSoundType.GUNSHOT_SNIPER;
			}

			case EWeaponType.WT_MACHINEGUN:
			{
				if (isSuppressed)
					return DatZSoundType.GUNSHOT_MG_SUPPRESSED;
				return DatZSoundType.GUNSHOT_MG;
			}

			case EWeaponType.WT_RIFLE:
			{
				// Could be rifle or shotgun - check prefab for shotgun
				IEntity weaponEnt = weapon.GetOwner();
				if (weaponEnt)
				{
					EntityPrefabData prefab = weaponEnt.GetPrefabData();
					if (prefab)
					{
						string prefabName = prefab.GetPrefabName();
						if (prefabName.Contains("Shotgun") || prefabName.Contains("shotgun"))
						{
							if (isSuppressed)
								return DatZSoundType.GUNSHOT_SHOTGUN_SUPPRESSED;
							return DatZSoundType.GUNSHOT_SHOTGUN;
						}
					}
				}

				if (isSuppressed)
					return DatZSoundType.GUNSHOT_RIFLE_SUPPRESSED;
				return DatZSoundType.GUNSHOT_RIFLE;
			}

			case EWeaponType.WT_ROCKETLAUNCHER:
			{
				// Rockets are very loud, no suppressed variant
				return DatZSoundType.GUNSHOT_SNIPER; // Use sniper range for rockets
			}

			case EWeaponType.WT_GRENADELAUNCHER:
			{
				// Grenade launchers - moderate sound
				if (isSuppressed)
					return DatZSoundType.GUNSHOT_RIFLE_SUPPRESSED;
				return DatZSoundType.GUNSHOT_RIFLE;
			}

			default:
			{
				if (isSuppressed)
					return DatZSoundType.GUNSHOT_RIFLE_SUPPRESSED;
				return DatZSoundType.GUNSHOT_RIFLE;
			}
		}

		// Fallback - should never reach here due to default case
		if (isSuppressed)
			return DatZSoundType.GUNSHOT_RIFLE_SUPPRESSED;
		return DatZSoundType.GUNSHOT_RIFLE;
	}
}
