// DZHarvestManager.c - Central manager for harvest cooldowns
// Add this component to your GameMode prefab - uses GameMode's RplComponent for replication

class DZHarvestManagerClass : ScriptComponentClass {}

class DZHarvestManager : ScriptComponent
{
	private static DZHarvestManager s_Instance;
	static DZHarvestManager GetInstance() { return s_Instance; }

	// Cooldown tracking - position hash -> respawn time
	private ref map<int, float> m_HarvestedPositions = new map<int, float>();

	// Cooldown in milliseconds (5 minutes default)
	[Attribute("300000")]
	float m_fHarvestCooldownMs;

	// Grid size for position hashing (2m precision)
	private static const float GRID_SIZE = 2.0;

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		s_Instance = this;

		if (Replication.IsServer())
		{
			// Cleanup old entries every 60 seconds
			GetGame().GetCallqueue().CallLater(CleanupOldEntries, 60000, true);
		}
	}

	override void OnDelete(IEntity owner)
	{
		if (s_Instance == this)
			s_Instance = null;

		GetGame().GetCallqueue().Remove(CleanupOldEntries);
		super.OnDelete(owner);
	}

	// Hash position to int for fast lookup
	private int HashPosition(vector pos)
	{
		int x = (int)(pos[0] / GRID_SIZE);
		int z = (int)(pos[2] / GRID_SIZE);
		return x * 100000 + z;
	}

	// Check if position can be harvested
	bool CanHarvest(vector pos)
	{
		int hash = HashPosition(pos);

		if (!m_HarvestedPositions.Contains(hash))
			return true;

		float respawnTime = m_HarvestedPositions.Get(hash);
		float currentTime = GetGame().GetWorld().GetWorldTime();

		return currentTime >= respawnTime;
	}

	// Server: Perform harvest at position
	void DoHarvest(vector pos, ResourceName itemPrefab, int amount, IEntity user)
	{
		if (!Replication.IsServer())
			return;

		// Validate can harvest
		if (!CanHarvest(pos))
			return;

		// Mark as harvested
		int hash = HashPosition(pos);
		float respawnTime = GetGame().GetWorld().GetWorldTime() + m_fHarvestCooldownMs;
		m_HarvestedPositions.Set(hash, respawnTime);

		// Spawn items
		Resource resource = Resource.Load(itemPrefab);
		if (!resource.IsValid())
			return;

		for (int i = 0; i < amount; i++)
		{
			EntitySpawnParams params = new EntitySpawnParams();
			params.TransformMode = ETransformMode.WORLD;

			// Slight random offset so items don't stack exactly
			vector spawnPos = pos;
			spawnPos[0] = spawnPos[0] + Math.RandomFloat(-0.3, 0.3);
			spawnPos[1] = spawnPos[1] + 0.2;
			spawnPos[2] = spawnPos[2] + Math.RandomFloat(-0.3, 0.3);
			params.Transform[3] = spawnPos;

			IEntity item = GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), params);
			if (item)
				SCR_EntityHelper.SnapToGround(item, startOffset: "0 0.5 0", maxLength: 5);
		}

		// Broadcast to clients so they see cooldown immediately
		Rpc(RpcDo_MarkHarvested, hash, respawnTime);
	}

	// Broadcast to clients: Mark position as harvested
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_MarkHarvested(int posHash, float respawnTime)
	{
		m_HarvestedPositions.Set(posHash, respawnTime);
	}

	// Server: Cleanup old entries to prevent memory growth
	private void CleanupOldEntries()
	{
		if (!Replication.IsServer())
			return;

		float currentTime = GetGame().GetWorld().GetWorldTime();
		array<int> toRemove = {};

		foreach (int hash, float respawnTime : m_HarvestedPositions)
		{
			if (currentTime >= respawnTime)
				toRemove.Insert(hash);
		}

		foreach (int hash : toRemove)
		{
			m_HarvestedPositions.Remove(hash);
		}
	}
}
