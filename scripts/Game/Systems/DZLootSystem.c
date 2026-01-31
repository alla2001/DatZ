// Custom loot item for adding items not in faction catalogs (global)
[BaseContainerProps(), SCR_BaseContainerCustomTitleResourceName("prefab")]
class DZGlobalCustomLootItem
{
	[Attribute("", UIWidgets.ResourcePickerThumbnail, "Item prefab to spawn", params: "et")]
	ResourceName prefab;

	[Attribute("25", UIWidgets.Slider, "Spawn chance (weight)", "1 100 1")]
	int spawnChance;

	[Attribute("1", UIWidgets.Slider, "Max spawn count", "1 10 1")]
	int maxSpawnCount;

	[Attribute("1", UIWidgets.ComboBox, "Item type category", enums: ParamEnumArray.FromEnum(SCR_EArsenalItemType))]
	SCR_EArsenalItemType itemType;

	[Attribute("true", UIWidgets.CheckBox, "Is this item enabled")]
	bool isEnabled;
}

// Optimized Loot System with spatial partitioning and player caching
class DZLootSystem : GameSystem
{
	[Attribute("5000", desc: "Time between processing ticks in milliseconds")]
	float tickTime;

	[Attribute("1.0", desc: "Delay before despawning loot after players leave")]
	float despawnDelay;

	[Attribute("", desc: "Default loot spawner prefab")]
	ResourceName defaultLootSpawner;

	[Attribute("100", desc: "Size of each grid cell for spatial partitioning")]
	float m_fGridCellSize;

	[Attribute("150", desc: "Max distance to consider a cell active (should be >= largest spawn radius)")]
	float m_fActivationRadius;

	[Attribute("", UIWidgets.Object, "Global custom loot items (added to TW loot table on generation)")]
	ref array<ref DZGlobalCustomLootItem> m_aGlobalCustomItems;

	// All registered spawners
	protected ref array<DatZLootSpawner> m_aSpawners = new array<DatZLootSpawner>();

	// Spatial grid: maps cell key to array of spawners in that cell
	protected ref map<int, ref array<DatZLootSpawner>> m_mGrid = new map<int, ref array<DatZLootSpawner>>();

	// Cached player positions (updated once per tick)
	protected ref array<vector> m_aCachedPlayerPositions = new array<vector>();

	// Active cells this tick (cells with players nearby)
	protected ref set<int> m_sActiveCells = new set<int>();

	// Track if despawn delay was set (only set once, not every tick)
	protected bool m_bDespawnDelaySet = false;

	//------------------------------------------------------------------------------------------------
	override void OnInit()
	{
		GetGame().GetCallqueue().CallLater(ProcessSpawners, 20000, false);
	}

	//------------------------------------------------------------------------------------------------
	void ProcessSpawners()
	{
		// Schedule next tick
		GetGame().GetCallqueue().CallLater(ProcessSpawners, tickTime, false);

		// Step 1: Cache all player positions (do this ONCE per tick)
		CachePlayerPositions();

		// Step 2: Determine active grid cells based on player positions
		UpdateActiveCells();

		// Step 3: Only process spawners in active cells
		ProcessActiveSpawners();
	}

	//------------------------------------------------------------------------------------------------
	// Cache all player positions once per tick
	protected void CachePlayerPositions()
	{
		m_aCachedPlayerPositions.Clear();

		array<int> allPlayers = {};
		GetGame().GetPlayerManager().GetAllPlayers(allPlayers);

		for (int i = 0; i < allPlayers.Count(); i++)
		{
			IEntity playerEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(allPlayers[i]);
			if (playerEntity)
				m_aCachedPlayerPositions.Insert(playerEntity.GetOrigin());
		}
	}

	//------------------------------------------------------------------------------------------------
	// Determine which grid cells are active (have players nearby)
	protected void UpdateActiveCells()
	{
		m_sActiveCells.Clear();

		// For each player, mark surrounding cells as active
		for (int i = 0; i < m_aCachedPlayerPositions.Count(); i++)
		{
			vector playerPos = m_aCachedPlayerPositions[i];

			// Get cells within activation radius
			int minX = Math.Floor((playerPos[0] - m_fActivationRadius) / m_fGridCellSize);
			int maxX = Math.Floor((playerPos[0] + m_fActivationRadius) / m_fGridCellSize);
			int minZ = Math.Floor((playerPos[2] - m_fActivationRadius) / m_fGridCellSize);
			int maxZ = Math.Floor((playerPos[2] + m_fActivationRadius) / m_fGridCellSize);

			for (int x = minX; x <= maxX; x++)
			{
				for (int z = minZ; z <= maxZ; z++)
				{
					int cellKey = GetCellKey(x, z);
					m_sActiveCells.Insert(cellKey);
				}
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	// Only process spawners in active cells
	protected void ProcessActiveSpawners()
	{
		// Set despawn delay once, not every tick
		if (!m_bDespawnDelaySet)
		{
			for (int i = 0; i < m_aSpawners.Count(); i++)
			{
				DatZLootSpawner spawner = m_aSpawners[i];
				if (spawner)
					spawner.SetDespawnDelay(despawnDelay);
			}
			m_bDespawnDelaySet = true;
		}

		// Process only spawners in active cells
		for (int i = 0; i < m_sActiveCells.Count(); i++)
		{
			int cellKey = m_sActiveCells.Get(i);

			if (!m_mGrid.Contains(cellKey))
				continue;

			array<DatZLootSpawner> cellSpawners = m_mGrid.Get(cellKey);
			if (!cellSpawners)
				continue;

			// Process spawners in this cell
			for (int j = cellSpawners.Count() - 1; j >= 0; j--)
			{
				DatZLootSpawner spawner = cellSpawners[j];

				// Clean up null spawners
				if (!spawner)
				{
					cellSpawners.Remove(j);
					continue;
				}

				// Pass cached player positions to spawner for efficient distance check
				spawner.ProcessWithCachedPlayers(m_aCachedPlayerPositions);
			}
		}

		// Also check spawned loot in inactive cells for despawning
		ProcessInactiveSpawnedLoot();
	}

	//------------------------------------------------------------------------------------------------
	// Check inactive cells for spawned loot that should despawn
	protected void ProcessInactiveSpawnedLoot()
	{
		// Iterate through all grid cells
		for (int i = 0; i < m_mGrid.Count(); i++)
		{
			int cellKey = m_mGrid.GetKey(i);

			// Skip active cells (already processed)
			if (m_sActiveCells.Contains(cellKey))
				continue;

			array<DatZLootSpawner> cellSpawners = m_mGrid.GetElement(i);
			if (!cellSpawners)
				continue;

			// Only check spawners that have spawned loot
			for (int j = 0; j < cellSpawners.Count(); j++)
			{
				DatZLootSpawner spawner = cellSpawners[j];
				if (spawner && spawner.HasSpawnedLoot())
				{
					// Check for despawn (no players nearby)
					spawner.ProcessWithCachedPlayers(m_aCachedPlayerPositions);
				}
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	// Generate unique key for grid cell
	protected int GetCellKey(int x, int z)
	{
		// Combine x and z into a single int key
		// Assuming map is not larger than 32768 cells in each direction
		return (x + 16384) * 32768 + (z + 16384);
	}

	//------------------------------------------------------------------------------------------------
	// Get cell coordinates from world position
	protected void GetCellCoords(vector pos, out int x, out int z)
	{
		x = Math.Floor(pos[0] / m_fGridCellSize);
		z = Math.Floor(pos[2] / m_fGridCellSize);
	}

	//------------------------------------------------------------------------------------------------
	// Override loot tables for spawners within radius (legacy - uses .conf files)
	void SetLootOverrideSpawners(ResourceName loottable, vector origin, float radius, float chance)
	{
		for (int i = 0; i < m_aSpawners.Count(); i++)
		{
			DatZLootSpawner spawner = m_aSpawners[i];

			if (!spawner)
				continue;

			if (spawner.ignoreOverrides)
				continue;

			if (Math.RandomFloat(0, 1) > chance)
				continue;

			if (vector.Distance(spawner.GetOrigin(), origin) <= radius)
			{
				spawner.lootTable = loottable;
				spawner.InitLootTable();
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	// Override loot type filter for spawners AND TW containers within radius
	// typeFilter: SCR_EArsenalItemType flags for allowed item types
	// emptyChance: 0-100, chance that spawner produces no loot (higher = more empty)
	// factionFilter: DatZLootFaction flags for allowed factions (0 = all factions)
	void SetLootTypeFilterForSpawners(int typeFilter, float emptyChance, vector origin, float radius, float chance, int factionFilter = 0)
	{
		float radiusSq = radius * radius;
		int spawnerCount = 0;
		int containerCount = 0;

		// Override DatZLootSpawner entities
		for (int i = 0; i < m_aSpawners.Count(); i++)
		{
			DatZLootSpawner spawner = m_aSpawners[i];

			if (!spawner)
				continue;

			if (spawner.ignoreOverrides)
				continue;

			if (Math.RandomFloat(0, 1) > chance)
				continue;

			float distSq = vector.DistanceSq(spawner.GetOrigin(), origin);
			if (distSq <= radiusSq)
			{
				spawner.SetTypeFilter(typeFilter, emptyChance, distSq, factionFilter);
				spawnerCount++;
			}
		}

		// Override TW_LootableInventoryComponent containers
		TW_GridCoordArrayManager<TW_LootableInventoryComponent> containerGrid = TW_LootManager.GetContainerGrid();
		if (containerGrid)
		{
			ref array<TW_LootableInventoryComponent> allContainers = {};
			containerGrid.GetAllItems(allContainers);

			for (int i = 0; i < allContainers.Count(); i++)
			{
				TW_LootableInventoryComponent container = allContainers[i];

				if (!container || !container.GetOwner())
					continue;

				float distSq = vector.DistanceSq(container.GetOwner().GetOrigin(), origin);
				if (distSq <= radiusSq)
				{
					container.SetZoneTypeFilter(typeFilter, distSq, factionFilter);
					containerCount++;
				}
			}
		}

		Print(string.Format("[DZLootSystem] Zone override applied: %1 spawners, %2 containers (faction filter: %3)", spawnerCount, containerCount, factionFilter), LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	static DZLootSystem GetInstance()
	{
		World world = GetGame().GetWorld();
		if (!world)
			return null;

		return DZLootSystem.Cast(world.FindSystem(DZLootSystem));
	}

	//------------------------------------------------------------------------------------------------
	void Register(DatZLootSpawner spawner)
	{
		if (!spawner)
			return;

		if (m_aSpawners.Contains(spawner))
			return;

		m_aSpawners.Insert(spawner);

		// Add to spatial grid
		AddToGrid(spawner);

		// Set despawn delay for new spawner
		spawner.SetDespawnDelay(despawnDelay);
	}

	//------------------------------------------------------------------------------------------------
	void Unregister(DatZLootSpawner spawner)
	{
		if (!spawner)
			return;

		m_aSpawners.RemoveItem(spawner);

		// Remove from spatial grid
		RemoveFromGrid(spawner);
	}

	//------------------------------------------------------------------------------------------------
	// Legacy method name support
	void UnRegister(DatZLootSpawner spawner)
	{
		Unregister(spawner);
	}

	//------------------------------------------------------------------------------------------------
	protected void AddToGrid(DatZLootSpawner spawner)
	{
		vector pos = spawner.GetOrigin();
		int x, z;
		GetCellCoords(pos, x, z);
		int cellKey = GetCellKey(x, z);

		if (!m_mGrid.Contains(cellKey))
			m_mGrid.Set(cellKey, new array<DatZLootSpawner>());

		array<DatZLootSpawner> cellSpawners = m_mGrid.Get(cellKey);
		if (!cellSpawners.Contains(spawner))
			cellSpawners.Insert(spawner);
	}

	//------------------------------------------------------------------------------------------------
	protected void RemoveFromGrid(DatZLootSpawner spawner)
	{
		vector pos = spawner.GetOrigin();
		int x, z;
		GetCellCoords(pos, x, z);
		int cellKey = GetCellKey(x, z);

		if (!m_mGrid.Contains(cellKey))
			return;

		array<DatZLootSpawner> cellSpawners = m_mGrid.Get(cellKey);
		cellSpawners.RemoveItem(spawner);

		// Clean up empty cells
		if (cellSpawners.Count() == 0)
			m_mGrid.Remove(cellKey);
	}

	//------------------------------------------------------------------------------------------------
	// Debug: Get stats about the grid
	void PrintGridStats()
	{
		Print(string.Format("[DZLootSystem] Total spawners: %1", m_aSpawners.Count()), LogLevel.NORMAL);
		Print(string.Format("[DZLootSystem] Grid cells: %1", m_mGrid.Count()), LogLevel.NORMAL);
		Print(string.Format("[DZLootSystem] Active cells: %1", m_sActiveCells.Count()), LogLevel.NORMAL);
		Print(string.Format("[DZLootSystem] Cached players: %1", m_aCachedPlayerPositions.Count()), LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	// Get global custom loot items for TW_LootManager to include in loot table
	array<ref DZGlobalCustomLootItem> GetGlobalCustomItems()
	{
		return m_aGlobalCustomItems;
	}

	//------------------------------------------------------------------------------------------------
	// Static helper to get custom items from the system instance
	static array<ref DZGlobalCustomLootItem> GetGlobalCustomLootItems()
	{
		DZLootSystem system = GetInstance();
		if (!system)
			return null;

		return system.GetGlobalCustomItems();
	}
}
