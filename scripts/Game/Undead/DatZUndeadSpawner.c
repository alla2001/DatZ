// DatZ Undead Spawner - Zone-based Population System
// Spawns and manages undead within a defined area

[EntityEditorProps(category: "GameScripted/DatZ", description: "Zone-based undead spawner")]
class DatZUndeadSpawnerClass : GenericEntityClass
{
}

class DatZUndeadSpawner : GenericEntity
{
	[Attribute("", UIWidgets.ResourcePickerThumbnail, "Undead prefabs to spawn", "et")]
	protected ref array<ResourceName> m_UndeadPrefabs;

	[Attribute("100", UIWidgets.Slider, "Zone radius in meters", "10 500 10")]
	protected float m_fZoneRadius;

	[Attribute("5", UIWidgets.Slider, "Minimum undead in zone", "0 50 1")]
	protected int m_iMinPopulation;

	[Attribute("15", UIWidgets.Slider, "Maximum undead in zone", "1 100 1")]
	protected int m_iMaxPopulation;

	[Attribute("200", UIWidgets.Slider, "Player detection range to activate", "50 1000 50")]
	protected float m_fActivationRange;

	[Attribute("30", UIWidgets.Slider, "Spawn interval in seconds", "5 300 5")]
	protected float m_fSpawnInterval;

	[Attribute("10", UIWidgets.Slider, "Minimum distance from players to spawn", "5 100 5")]
	protected float m_fMinSpawnDistFromPlayer;

	[Attribute("true", UIWidgets.CheckBox, "Start active")]
	protected bool m_bStartActive;

	// Runtime state
	protected ref array<DatZUndeadBase> m_SpawnedUndead = {};
	protected bool m_bIsActive;
	protected float m_fLastSpawnTime;
	protected float m_fLastCheckTime;

	// Constants
	protected static const float CHECK_INTERVAL = 5.0;
	protected static const float CLEANUP_INTERVAL = 30.0;

	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);

		if (!Replication.IsServer())
			return;

		if (m_UndeadPrefabs.Count() == 0)
		{
			Print("[DatZUndeadSpawner] No prefabs configured!", LogLevel.WARNING);
			return;
		}

		m_bIsActive = m_bStartActive;

		// Start management loop
		GetGame().GetCallqueue().CallLater(ManagementLoop, CHECK_INTERVAL * 1000, false);
	}

	//------------------------------------------------------------------------------------------------
	protected void ManagementLoop()
	{
		if (!Replication.IsServer())
			return;

		float currentTime = GetGame().GetWorld().GetWorldTime() / 1000.0;

		// Check activation based on player proximity
		UpdateActivation();

		if (m_bIsActive)
		{
			// Cleanup dead/invalid undead
			CleanupDeadUndead();

			// Spawn new undead if needed
			TrySpawnUndead(currentTime);
		}

		// Schedule next check
		GetGame().GetCallqueue().CallLater(ManagementLoop, CHECK_INTERVAL * 1000, false);
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateActivation()
	{
		bool playerNearby = IsPlayerInRange(m_fActivationRange);

		if (playerNearby && !m_bIsActive)
		{
			m_bIsActive = true;
			OnActivated();
		}
		else if (!playerNearby && m_bIsActive)
		{
			// Deactivate if no players nearby (optional - keeps zombies when players leave)
			// m_bIsActive = false;
			// OnDeactivated();
		}
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsPlayerInRange(float range)
	{
		vector center = GetOrigin();

		// Get all player controllers
		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);

		foreach (int playerId : playerIds)
		{
			IEntity playerEnt = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
			if (!playerEnt)
				continue;

			float dist = vector.Distance(center, playerEnt.GetOrigin());
			if (dist <= range)
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected void OnActivated()
	{
		// Initial spawn burst
		int initialSpawn = Math.RandomInt(m_iMinPopulation, m_iMaxPopulation + 1);
		for (int i = 0; i < initialSpawn; i++)
		{
			SpawnSingleUndead();
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void OnDeactivated()
	{
		// Optional: despawn all undead when deactivating
		// DespawnAll();
	}

	//------------------------------------------------------------------------------------------------
	protected void CleanupDeadUndead()
	{
		for (int i = m_SpawnedUndead.Count() - 1; i >= 0; i--)
		{
			DatZUndeadBase undead = m_SpawnedUndead[i];

			if (!undead || !undead.IsAlive())
			{
				m_SpawnedUndead.Remove(i);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void TrySpawnUndead(float currentTime)
	{
		// Check spawn interval
		if (currentTime - m_fLastSpawnTime < m_fSpawnInterval)
			return;

		// Check population limit
		if (m_SpawnedUndead.Count() >= m_iMaxPopulation)
			return;

		// Check if below minimum
		if (m_SpawnedUndead.Count() < m_iMinPopulation)
		{
			// Spawn multiple to reach minimum faster
			int toSpawn = m_iMinPopulation - m_SpawnedUndead.Count();
			toSpawn = Math.Min(toSpawn, 3); // Max 3 at once

			for (int i = 0; i < toSpawn; i++)
			{
				SpawnSingleUndead();
			}
		}
		else
		{
			// Normal spawn rate
			SpawnSingleUndead();
		}

		m_fLastSpawnTime = currentTime;
	}

	//------------------------------------------------------------------------------------------------
	protected void SpawnSingleUndead()
	{
		if (m_UndeadPrefabs.Count() == 0)
			return;

		// Pick random prefab
		int prefabIdx = Math.RandomInt(0, m_UndeadPrefabs.Count());
		ResourceName prefab = m_UndeadPrefabs[prefabIdx];

		// Find valid spawn position
		vector spawnPos = FindSpawnPosition();
		if (spawnPos == vector.Zero)
			return;

		// Spawn the undead
		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		params.Transform[3] = spawnPos;

		// Random rotation
		float yaw = Math.RandomFloat(0, 360);
		vector angles = Vector(yaw, 0, 0);
		Math3D.AnglesToMatrix(angles, params.Transform);
		params.Transform[3] = spawnPos;

		Resource res = Resource.Load(prefab);
		if (!res || !res.IsValid())
			return;

		IEntity spawned = GetGame().SpawnEntityPrefab(res, GetGame().GetWorld(), params);
		if (!spawned)
			return;

		DatZUndeadBase undead = DatZUndeadBase.Cast(spawned);
		if (undead)
			m_SpawnedUndead.Insert(undead);
	}

	//------------------------------------------------------------------------------------------------
	protected vector FindSpawnPosition()
	{
		vector center = GetOrigin();
		int maxAttempts = 10;

		for (int attempt = 0; attempt < maxAttempts; attempt++)
		{
			// Random angle and distance
			float angle = Math.RandomFloat(0, Math.PI2);
			float dist = Math.RandomFloat(m_fZoneRadius * 0.2, m_fZoneRadius);

			vector candidate = center;
			candidate[0] = candidate[0] + Math.Cos(angle) * dist;
			candidate[2] = candidate[2] + Math.Sin(angle) * dist;

			// Get terrain height
			candidate[1] = GetGame().GetWorld().GetSurfaceY(candidate[0], candidate[2]);

			// Check distance from players
			if (!IsPositionTooCloseToPlayers(candidate))
			{
				// Check if position is valid (not in water, etc)
				if (IsValidSpawnPosition(candidate))
					return candidate;
			}
		}

		return vector.Zero;
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsPositionTooCloseToPlayers(vector pos)
	{
		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);

		foreach (int playerId : playerIds)
		{
			IEntity playerEnt = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
			if (!playerEnt)
				continue;

			float dist = vector.Distance(pos, playerEnt.GetOrigin());
			if (dist < m_fMinSpawnDistFromPlayer)
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsValidSpawnPosition(vector pos)
	{
		// Check if underwater
		float waterLevel = GetGame().GetWorld().GetOceanBaseHeight();
		if (pos[1] < waterLevel + 0.5)
			return false;

		// Could add more checks here (inside buildings, steep terrain, etc)

		return true;
	}

	//------------------------------------------------------------------------------------------------
	void DespawnAll()
	{
		foreach (DatZUndeadBase undead : m_SpawnedUndead)
		{
			if (undead)
				SCR_EntityHelper.DeleteEntityAndChildren(undead);
		}
		m_SpawnedUndead.Clear();
	}

	//------------------------------------------------------------------------------------------------
	void SetActive(bool active)
	{
		m_bIsActive = active;
		if (active)
			OnActivated();
		else
			OnDeactivated();
	}

	//------------------------------------------------------------------------------------------------
	bool IsActive() { return m_bIsActive; }
	int GetCurrentPopulation() { return m_SpawnedUndead.Count(); }
	float GetZoneRadius() { return m_fZoneRadius; }
}
