//------------------------------------------------------------------------------------------------
modded class SCR_DestructibleBuildingEntity : Building
{
	[Attribute()]
	bool ignoreLoot;
	[Attribute()]
	bool ignoreDestruction;

	// Static trace directions (avoid recreating)
	protected static const vector TRACE_DIRECTIONS[3] = {vector.Up, vector.Right, vector.Forward};

	// Cached trace objects (reused to avoid allocations)
	protected ref TraceParam m_CachedTraceParam;
	protected ref TraceBox m_CachedTraceBox;

	override void EOnInit(IEntity owner)
	{
		if (SCR_Global.IsEditMode())
			return;

		if (!Replication.IsServer())
			return;

		if (!ignoreLoot)
			GetGame().GetCallqueue().CallLater(SpawnServerLoot, 15000, false, owner);

		if (!ignoreDestruction)
			GetGame().GetCallqueue().CallLater(RandomDestruct, 4000, false);
	}

	void SpawnServerLoot(IEntity owner)
	{
		if (!owner)
			return;

		World world = owner.GetWorld();
		if (!world)
			return;

		DZLootSystem loot = DZLootSystem.GetInstance();
		if (!loot || !loot.defaultLootSpawner)
			return;

		// Pre-load resource once
		Resource lootResource = Resource.Load(loot.defaultLootSpawner);
		if (!lootResource.IsValid())
			return;

		vector minBound, maxBound;
		GetBounds(minBound, maxBound);

		// Add vertical margin
		float verticalMargin = 2;
		minBound[1] = 2;
		maxBound[1] = maxBound[1] - verticalMargin;
		minBound[0] = minBound[0] + verticalMargin;
		maxBound[0] = maxBound[0] - verticalMargin;
		minBound[2] = minBound[2] + verticalMargin;
		maxBound[2] = maxBound[2] - verticalMargin;

		// Validate bounds
		if (minBound[0] > maxBound[0] || minBound[1] > maxBound[1] || minBound[2] > maxBound[2])
			return;

		// Calculate max allowed spawns based on building bounds
		vector size = maxBound - minBound;
		float volume = size[0] * size[2] + size[1];
		int maxSpawns = Math.Clamp(Math.Ceil(volume / 15.0), 1, 25);

		// Create spawn params once outside loop
		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;

		// Initialize cached trace objects
		InitTraceCaches();

		int successfulSpawns = 0;
		for (int i = 0; i < 50; i++)
		{
			if (successfulSpawns >= maxSpawns)
				break;

			vector randomLocalPos = RandomVector(minBound, maxBound);
			vector randomWorldPos = CoordToParent(randomLocalPos);

			if (IsInsideCached(randomWorldPos, world))
			{
				params.Transform[3] = randomWorldPos;
				IEntity ent = GetGame().SpawnEntityPrefab(lootResource, world, params);

				if (ent)
				{
					SCR_EntityHelper.SnapToGround(ent, startOffset: "0 0.5 0", maxLength: 30);
					ent.SetOrigin(ent.GetOrigin() + "0 1 0");
					ent.Update();
					successfulSpawns++;
				}
			}
		}

		// Clear cached traces
		m_CachedTraceParam = null;
		m_CachedTraceBox = null;
	}

	void RandomDestruct()
	{
		SCR_DestructibleBuildingComponent desc = SCR_DestructibleBuildingComponent.Cast(FindComponent(SCR_DestructibleBuildingComponent));
		if (!desc)
			return;

		// Use position-based seed for consistent randomness
		int seed = Math.Floor(GetOrigin()[0]);
		Math.Randomize(seed);

		if (Math.RandomFloat01() >= 0.85)
		{
			desc.SetHealthScaled(Math.RandomFloat(0.0, 1.0));
		}
	}

	// Initialize trace caches once before spawning loop
	protected void InitTraceCaches()
	{
		m_CachedTraceParam = new TraceParam();
		m_CachedTraceParam.Flags = TraceFlags.ENTS;
		m_CachedTraceParam.LayerMask = EPhysicsLayerDefs.Projectile;

		m_CachedTraceBox = new TraceBox();
		m_CachedTraceBox.Mins = "-0.7 -0.7 -0.7";
		m_CachedTraceBox.Maxs = "0.7 0.7 0.7";
		m_CachedTraceBox.LayerMask = EPhysicsLayerDefs.Projectile;
	}

	protected vector RandomVector(vector min, vector max)
	{
		return Vector(
			Math.RandomFloat(min[0], max[0]),
			Math.RandomFloat(min[1], max[1]),
			Math.RandomFloat(min[2], max[2])
		);
	}

	//------------------------------------------------------------------------------------------------
	protected bool PerformTraceCached(vector start, vector direction, BaseWorld world, float lengthMultiplier)
	{
		m_CachedTraceParam.Start = start - direction * lengthMultiplier;
		m_CachedTraceParam.End = start + direction * lengthMultiplier;
		m_CachedTraceParam.TraceEnt = null;
		world.TraceMove(m_CachedTraceParam, TraceFilter);
		return m_CachedTraceParam.TraceEnt != null;
	}

	//------------------------------------------------------------------------------------------------
	protected bool PerformTraceUpCached(vector start, vector direction, BaseWorld world, float lengthMultiplier)
	{
		m_CachedTraceParam.Start = start + direction;
		m_CachedTraceParam.End = start + direction * lengthMultiplier;
		m_CachedTraceParam.TraceEnt = null;
		world.TraceMove(m_CachedTraceParam, TraceFilter);
		return m_CachedTraceParam.TraceEnt != null;
	}

	//------------------------------------------------------------------------------------------------
	// Optimized IsInside using cached traces
	protected bool IsInsideCached(vector start, BaseWorld world)
	{
		if (!m_CachedTraceParam)
			return false;

		// Pre-generate random values once
		float rand0 = Math.RandomFloat(0, 10);
		float rand1 = Math.RandomFloat(0, 10);
		float rand2 = Math.RandomFloat(0, 10);

		// Check roof (up trace)
		bool hasRoof = PerformTraceUpCached(start, TRACE_DIRECTIONS[0], world, 30);
		if (!hasRoof && rand0 < 9)
			return false;

		// Check right wall
		bool hasRightWall = PerformTraceCached(start, TRACE_DIRECTIONS[1], world, 1);
		if (!hasRightWall && rand1 < 6)
			return false;

		// Check forward wall
		bool hasForwardWall = PerformTraceCached(start, TRACE_DIRECTIONS[2], world, 1);
		if (!hasForwardWall && rand2 < 5)
			return false;

		// Check free space using cached trace box
		return HasEnoughFreeSpaceCached(start, world);
	}

	//------------------------------------------------------------------------------------------------
	// Optimized free space check using cached TraceBox
	protected bool HasEnoughFreeSpaceCached(vector center, BaseWorld world)
	{
		if (!m_CachedTraceBox)
			return false;

		m_CachedTraceBox.Start = center;
		m_CachedTraceBox.End = center;

		float fraction = world.TraceMove(m_CachedTraceBox, TraceFilter);
		return fraction == 1;
	}

	//------------------------------------------------------------------------------------------------
	protected bool TraceFilter(notnull IEntity e, vector start = "0 0 0", vector dir = "0 0 0")
	{
		return e == this;
	}
}
