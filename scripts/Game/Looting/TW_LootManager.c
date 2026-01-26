enum TW_ResourceNameType
{
	ResourceName,
	DisplayName
};

class TW_LootSettingsInterface : TW_SettingsInterface<LootManagerSettings>
{
	
}

typedef TW_SettingsManager<ref TW_LootSettingsInterface<LootManagerSettings>> LootSettingsManager;


sealed class TW_LootManager
{
	private static TW_LootManager s_Instance;
	static TW_LootManager GetInstance() { return s_Instance; }

	void TW_LootManager()
	{
		if(s_Instance)
		{
			PrintFormat("TrainWreck: TW_LootManager -> Instance of Loot Manager already exists", LogLevel.ERROR);
			return;
		}

		s_Instance = this;
	}

	// Provide the ability to grab
	private static ref map<SCR_EArsenalItemType, ref array<ref TW_LootConfigItem>> s_LootTable = new map<SCR_EArsenalItemType, ref array<ref TW_LootConfigItem>>();

	// This should contain the resource names of all items that are valid for saving/loading
	private static ref set<string> s_GlobalItems = new set<string>();
	private static ref TW_GridCoordArrayManager<TW_LootableInventoryComponent> s_GlobalContainerGrid = new TW_GridCoordArrayManager<TW_LootableInventoryComponent>(100);
	private static ref array<SCR_EArsenalItemType> s_ArsenalItemTypes = {};
	static TW_GridCoordArrayManager<TW_LootableInventoryComponent> GetContainerGrid() { return s_GlobalContainerGrid; }

	// OPTIMIZATION: Cache loot pools by flag combination to avoid rebuilding every spawn
	private static ref map<int, ref WeightedType<ref TW_LootConfigItem>> s_CachedLootPools = new map<int, ref WeightedType<ref TW_LootConfigItem>>();

	// OPTIMIZATION: Fast resource->type lookup for FlagHasResource O(1) instead of O(n*m)
	private static ref map<string, SCR_EArsenalItemType> s_ResourceTypeMap = new map<string, SCR_EArsenalItemType>();

	// OPTIMIZATION: Cache validated resources to avoid repeated Resource.Load
	private static ref set<string> s_ValidatedResources = new set<string>();
	private static ref set<string> s_InvalidResources = new set<string>();
	
	private static bool HasLoaded = false;
	static const string LootFileName = "$profile:lootmap.json";	
	
	private ref LootManagerSettings m_Settings;
	private SCR_BaseGameMode m_GameMode;
		
	LootManagerSettings GetLootSettings() { return m_Settings; }
	bool ShouldSpawnMagazine() { return m_Settings.ShouldSpawnMagazine; }
	float GetUnlootedTimeRatio() 
	{ 
		if(!m_Settings || !m_Settings.RespawnSettings)
		{
			PrintFormat("TrainWreck: RespawnSettings not set", LogLevel.WARNING);
			return 0.5;
		}
		
		return m_Settings.RespawnSettings.UnlootedTimeRatio; 
	}
	float GetSearchedTimeRatio() 
	{ 
		if(!m_Settings || !m_Settings.RespawnSettings)
		{
			PrintFormat("TrainWreck: RespawnSettings not set", LogLevel.WARNING);
			return 0.5;
		}
		
		return m_Settings.RespawnSettings.SearchedTimeRatio; 
	}
	
	ScavLootSettings GetScavSettings() { return m_Settings.ScavSettings; }
	
	bool IsDebug()
	{
		if(!m_Settings)
			return false;
		
		return m_Settings.ShowDebug;
	}
	
	float GetRandomAmmoPercent() { return m_Settings.AmmoPercentageSetting.GetRandomPercentage() / m_Settings.AmmoPercentageSetting.Max; }
	
	//! Time after last player interaction loot can start to respawn
	int GetRespawnAfterLastInteractionInMinutes() { return m_Settings.RespawnSettings.RespawnAfterLastInteractionInMinutes; }
	
	//! Number of items that can respawn, at most, overtime
	int GetRespawnLootItemThreshold() { return m_Settings.RespawnSettings.NumberOfItemsToSpawnPerContainer; }
	
	int GetRespawnCheckInterval() { return m_Settings.RespawnSettings.RespawnLootTimerInSeconds; }

	static void RegisterLootableContainer(TW_LootableInventoryComponent container)
	{
		if(s_GlobalContainerGrid)
			s_GlobalContainerGrid.InsertByWorld(container.GetOwner().GetOrigin(), container);
	}
	
	static void UnregisterLootableContainer(TW_LootableInventoryComponent container)
	{
		if(s_GlobalContainerGrid)
			s_GlobalContainerGrid.RemoveByWorld(container.GetOwner().GetOrigin(), container);
	}
		
	//! Is this resource in the global items set? - IF not --> invalid.
	static bool IsValidItem(ResourceName resource)
	{
		return s_GlobalItems.Contains(resource);
	}
	
	// OPTIMIZED: O(1) lookup using cached resource->type map
	static bool FlagHasResource(SCR_EArsenalItemType flags, ResourceName resource)
	{
		if(resource.IsEmpty())
			return false;

		// Fast path: check if resource exists in our type map
		if(!s_ResourceTypeMap.Contains(resource))
			return false;

		// Check if the resource's type matches the requested flags
		SCR_EArsenalItemType resourceType = s_ResourceTypeMap.Get(resource);
		return SCR_Enum.HasFlag(flags, resourceType);
	}
			
	void SelectRandomPrefabsFromFlags(SCR_EArsenalItemType flags, int count, notnull map<string, int> selected, TW_ResourceNameType type = TW_ResourceNameType.DisplayName)
	{
		int selectedCount = 0;
		
		foreach(SCR_EArsenalItemType itemType : s_ArsenalItemTypes)
		{
			if(selectedCount >= count)
				return;
			
			if(!SCR_Enum.HasFlag(flags, itemType))
				continue;
			
			if(!s_LootTable.Contains(itemType))
				continue;
			
			ref array<ResourceName> items = {};
			selectedCount += SelectRandomPrefabsFromType(itemType, Math.RandomIntInclusive(1, count), items);
			
			foreach(ResourceName name : items)
			{
				string value = name;
				switch(type)
				{
					case TW_ResourceNameType.DisplayName:
						value = WidgetManager.Translate(TW_Util.GetPrefabDisplayName(name));
						break;
				}
				
				if(selected.Contains(value))
					selected.Set(value, selected.Get(value) + 1);
				else
					selected.Set(value, 1);
			}
		}
		
		// If we didn't select enough, we'll do it again.
		// Subtracting what we have from the desired amount
		if(selectedCount < count)
			SelectRandomPrefabsFromFlags(flags, count - selectedCount, selected, type);
	}
	
	void PrintSettings()
	{
		if(!IsDebug())
			return;
		
		Print("------------------------");
		Print("TrainWreck Loot Settings");
		foreach(SCR_EArsenalItemType itemType : s_ArsenalItemTypes)
		{
			string typeName = SCR_Enum.GetEnumName(SCR_EArsenalItemType, itemType);
			ref array<ref TW_LootConfigItem> items = s_LootTable.Get(itemType);
			PrintFormat("Arsenal Category: %1, Count: %2", typeName, items.Count());
		}
		Print("------------------------");
	}
	
	int SelectRandomPrefabsFromType(SCR_EArsenalItemType flag, int randomCount, notnull array<ResourceName> selected)
	{
		if(!s_LootTable.Contains(flag))
			return 0;
		
		ref array<ref TW_LootConfigItem> items = s_LootTable.Get(flag);
		ref set<int> indicies = new set<int>();
		
		int count = 0;
		for(int i = 0; i < randomCount; i++)
		{			
			ref TW_LootConfigItem item = items.GetRandomElement();
			
			int randomIndex = items.GetRandomIndex();
			
			while(indicies.Contains(randomIndex) && selected.Count() < items.Count())
				randomIndex = items.GetRandomIndex();	
			
			if(indicies.Contains(randomIndex))
				break;
			
			count++;
			indicies.Insert(randomIndex);
			selected.Insert(items.Get(randomIndex).resourceName);
		}
		
		return count;
	}
	
	array<SCR_EntityCatalogEntry> GetMergedFactionCatalogs()
	{
		SCR_FactionManager manager = SCR_FactionManager.Cast(GetGame().GetFactionManager());

		if(!manager)
		{
			PrintFormat("TrainWreckLooting: Looting requires a faction manager to be present", LogLevel.ERROR);
			return null;
		}

		ref array<Faction> factions = {};
		manager.GetFactionsList(factions);

		SCR_EntityCatalog mainCatalog;
		ref array<SCR_EntityCatalogEntry> entries = {};
		ref array<SCR_EntityCatalogEntry> current = {};

		foreach(Faction fac : factions)
		{
			SCR_Faction faction = SCR_Faction.Cast(fac);

			if(!faction)
				continue;

			SCR_EntityCatalog itemCatalog = faction.GetFactionEntityCatalogOfType(EEntityCatalogType.ITEM);

			if(!itemCatalog)
			{
				PrintFormat("TrainWreckLooting: Faction does not have an item catalog: %1", WidgetManager.Translate(faction.GetFactionName()), LogLevel.ERROR);
				return null;
			}

			current.Clear();
			itemCatalog.GetEntityList(current);
			entries.InsertAll(current);
		}

		return entries;
	}
	
	// Set to true to always regenerate lootmap from catalogs on game start
	static bool s_bAlwaysRegenerateLootmap = true;

	// Item types to completely exclude from loot spawning
	// Add types here to disable entire categories (e.g., SCR_EArsenalItemType.ROCKET_LAUNCHER)
	// Available types: RIFLE, PISTOL, MACHINE_GUN, SNIPER_RIFLE, ROCKET_LAUNCHER, MORTARS,
	//                  HEADWEAR, VEST_AND_WAIST, BACKPACK, RADIO_BACKPACK, TORSO, LEGS,
	//                  FOOTWEAR, HANDWEAR, EQUIPMENT, HEAL, WEAPON_ATTACHMENT,
	//                  LETHAL_THROWABLE, NON_LETHAL_THROWABLE, EXPLOSIVES, HELICOPTER, VEHICLE
	static ref set<SCR_EArsenalItemType> s_ExcludedItemTypes = new set<SCR_EArsenalItemType>();

	//------------------------------------------------------------------------------------------------
	// Call this to exclude an item type from loot spawning
	static void ExcludeItemType(SCR_EArsenalItemType itemType)
	{
		if (!s_ExcludedItemTypes.Contains(itemType))
			s_ExcludedItemTypes.Insert(itemType);
	}

	//------------------------------------------------------------------------------------------------
	// Call this to include an item type in loot spawning (remove from exclusion)
	static void IncludeItemType(SCR_EArsenalItemType itemType)
	{
		if (s_ExcludedItemTypes.Contains(itemType))
			s_ExcludedItemTypes.RemoveItem(itemType);
	}

	//------------------------------------------------------------------------------------------------
	// Check if an item type is excluded from spawning
	static bool IsItemTypeExcluded(SCR_EArsenalItemType itemType)
	{
		return s_ExcludedItemTypes.Contains(itemType);
	}

	//------------------------------------------------------------------------------------------------
	// LOOT BALANCING - DayZ-style rarity (slightly less hardcore)
	// These are the spawn chances per item type (higher = more common)
	// Balanced for survival gameplay: military gear is rare, basic supplies more common
	//------------------------------------------------------------------------------------------------
	static bool s_bApplyLootBalancing = true; // Set to false to disable balancing

	//------------------------------------------------------------------------------------------------
	// Get the balanced spawn chance for an item type
	// Returns spawn chance value (1-100 scale used in weighted selection)
	static int GetBalancedSpawnChance(SCR_EArsenalItemType itemType)
	{
		switch (itemType)
		{
			// EXTREMELY RARE - End-game military hardware
			case SCR_EArsenalItemType.ROCKET_LAUNCHER:
				return 2;
			case SCR_EArsenalItemType.MORTARS:
				return 1;
			case SCR_EArsenalItemType.EXPLOSIVES:
				return 3;

			// VERY RARE - High-tier military weapons
			case SCR_EArsenalItemType.SNIPER_RIFLE:
				return 5;
			case SCR_EArsenalItemType.MACHINE_GUN:
				return 6;

			// RARE - Standard military weapons & good gear
			case SCR_EArsenalItemType.RIFLE:
				return 12;
			case SCR_EArsenalItemType.WEAPON_ATTACHMENT:
				return 10;
			case SCR_EArsenalItemType.RADIO_BACKPACK:
				return 8;
			case SCR_EArsenalItemType.LETHAL_THROWABLE:
				return 8;

			// UNCOMMON - Sidearms, basic military gear
			case SCR_EArsenalItemType.PISTOL:
				return 20;
			case SCR_EArsenalItemType.BACKPACK:
				return 18;
			case SCR_EArsenalItemType.VEST_AND_WAIST:
				return 22;
			case SCR_EArsenalItemType.HEAL:
				return 25;
			case SCR_EArsenalItemType.NON_LETHAL_THROWABLE:
				return 18;
			case SCR_EArsenalItemType.EQUIPMENT:
				return 28;

			// COMMON - Civilian clothing & basic items
			case SCR_EArsenalItemType.HEADWEAR:
				return 40;
			case SCR_EArsenalItemType.TORSO:
				return 45;
			case SCR_EArsenalItemType.LEGS:
				return 45;
			case SCR_EArsenalItemType.FOOTWEAR:
				return 50;
			case SCR_EArsenalItemType.HANDWEAR:
				return 50;

			// Vehicles - disabled by default, very rare if enabled
			case SCR_EArsenalItemType.HELICOPTER:
				return 1;
			case SCR_EArsenalItemType.VEHICLE:
				return 1;

			// Default for unknown types
			default:
				return 25;
		}
			return 25;
	}

	//------------------------------------------------------------------------------------------------
	// Apply loot balancing to all items in the loot table
	// Call this after loot table is built to adjust spawn chances by type
	protected void ApplyLootBalancing()
	{
		if (!s_bApplyLootBalancing)
		{
			Print("[TW_LootManager] Loot balancing disabled", LogLevel.NORMAL);
			return;
		}

		Print("[TW_LootManager] Applying DayZ-style loot balancing...", LogLevel.NORMAL);

		int totalAdjusted = 0;

		foreach (SCR_EArsenalItemType itemType, ref array<ref TW_LootConfigItem> items : s_LootTable)
		{
			int balancedChance = GetBalancedSpawnChance(itemType);
			string typeName = SCR_Enum.GetEnumName(SCR_EArsenalItemType, itemType);

			foreach (TW_LootConfigItem item : items)
			{
				if (!item.isEnabled)
					continue;

				item.chanceToSpawn = balancedChance;
				totalAdjusted++;
			}

			Print(string.Format("[TW_LootManager] %1: %2 items set to %3%% spawn chance", typeName, items.Count(), balancedChance), LogLevel.NORMAL);
		}

		Print(string.Format("[TW_LootManager] Balanced %1 total items", totalAdjusted), LogLevel.NORMAL);

		// Invalidate cached pools since chances changed
		InvalidateLootPoolCache();
	}

	void InitializeLootTable()
	{
		SCR_Enum.GetEnumValues(SCR_EArsenalItemType, s_ArsenalItemTypes);

		// Configure default excluded item types here
		// Uncomment lines below to exclude entire categories from loot spawning:
		// ExcludeItemType(SCR_EArsenalItemType.ROCKET_LAUNCHER);
		// ExcludeItemType(SCR_EArsenalItemType.MORTARS);
		// ExcludeItemType(SCR_EArsenalItemType.EXPLOSIVES);
		// ExcludeItemType(SCR_EArsenalItemType.MACHINE_GUN);
		// ExcludeItemType(SCR_EArsenalItemType.HELICOPTER);
		// ExcludeItemType(SCR_EArsenalItemType.VEHICLE);

		// Log excluded types
		if (s_ExcludedItemTypes.Count() > 0)
		{
			string excludedList = "";
			for (int i = 0; i < s_ExcludedItemTypes.Count(); i++)
			{
				if (i > 0) excludedList += ", ";
				excludedList += SCR_Enum.GetEnumName(SCR_EArsenalItemType, s_ExcludedItemTypes.Get(i));
			}
			Print(string.Format("[TW_LootManager] Excluded item types: %1", excludedList), LogLevel.NORMAL);
		}

		// If always regenerate is enabled, skip loading from files and scan catalogs
		if (s_bAlwaysRegenerateLootmap)
		{
			Print("[TW_LootManager] Regenerating lootmap from catalogs (s_bAlwaysRegenerateLootmap = true)", LogLevel.NORMAL);
			m_Settings = new LootManagerSettings();
			// Fall through to catalog scanning below
		}
		else
		{
			// OPTIMIZATION: Try to load pre-generated config from Workbench plugin first
			// This avoids expensive runtime catalog scanning
			if(TW_LootTableLoader.HasConfig())
			{
				Print("[TW_LootManager] Found pre-generated loot config, attempting to load...", LogLevel.NORMAL);

				bool loadedFromConfig = TW_LootTableLoader.LoadFromConfig(s_LootTable, s_GlobalItems, s_ResourceTypeMap);

				if(loadedFromConfig)
				{
					Print("[TW_LootManager] Successfully loaded loot table from pre-generated config", LogLevel.NORMAL);

					// Still need to load settings from lootmap.json for respawn/other settings
					if(HasLootTable())
					{
						IngestSettingsOnly(m_Settings);
					}
					else
					{
						m_Settings = new LootManagerSettings();
					}

					// Invalidate cached pools since table was loaded fresh
					InvalidateLootPoolCache();
					HasLoaded = true;

					// Remove disabled items
					RemoveDisabledItems();

					// Apply DayZ-style loot balancing
					ApplyLootBalancing();
					return;
				}
				else
				{
					Print("[TW_LootManager] Failed to load from config, falling back to runtime generation", LogLevel.WARNING);
				}
			}

			// If lootmap already exists -- load everything from file
			// Then merge things that are in-game
			if(HasLootTable())
			{
				Print(string.Format("TrainWreck: Detected loot table %1", LootFileName));
				IngestLootTableFromFile(m_Settings);
			}
			else m_Settings = new LootManagerSettings();
		}

		ref array<SCR_EntityCatalogEntry> catalogItems = GetMergedFactionCatalogs();
		int entityCount = catalogItems.Count();
		
		foreach(auto entry : catalogItems)
		{
			ref array<SCR_BaseEntityCatalogData> itemData = {};
			entry.GetEntityDataList(itemData);
			
			// We only care about fetching arsenal items 
			foreach(auto data : itemData)
			{
				SCR_ArsenalItem arsenalItem = SCR_ArsenalItem.Cast(data);
				
				if(!arsenalItem)
					continue;
				
				if(!arsenalItem.IsEnabled())
					break;

				// Check if item should be included in loot spawning (from SCR_ArsenalItemModded)
				if(!arsenalItem.ShouldIncludeInLootSpawning())
					continue;

				auto itemType = arsenalItem.GetItemType();
				auto itemMode = arsenalItem.GetItemMode();
				ResourceName prefab = entry.GetPrefab();

				// Check if this entire item type category is excluded
				if (IsItemTypeExcluded(itemType))
					continue;

				// If we already had a lootmap from file
				// and the items is loaded -- ignore readding it
				if(s_GlobalItems.Contains(prefab))
					continue;
				
				s_GlobalItems.Insert(prefab);
				
				arsenalItem.SetItemPrefab(prefab);
				
				int defaultCount = 1;
				int defaultChance = 25;
				
				//if(SCR_BaseContainerTools.FindComponentSource(Resource.Load(prefab), "MagazineComponent"))
				//	defaultCount = 4;
								
				//if(SCR_BaseContainerTools.FindComponentSource(Resource.Load(prefab), "SCR_RestrictedDeployableSpawnPointComponent"))
				//	defaultChance = 5;	
				
				//if(prefab.Contains("RearmingKit") || prefab.Contains("MedicalKit"))
				//	arsenalItem.SetShouldSpawn(false);
				
				arsenalItem.SetItemMaxSpawnCount(defaultCount);
				arsenalItem.SetItemChanceToSpawn(defaultChance);
				
				ref TW_LootConfigItem config = new TW_LootConfigItem();
				
				config.SetData(prefab, defaultChance, defaultCount, null, arsenalItem.ShouldSpawn());

				if(!s_LootTable.Contains(itemType))
					s_LootTable.Insert(itemType, {});

				s_LootTable.Get(itemType).Insert(config);

				// OPTIMIZATION: Add to resource->type map for fast lookup
				if(!s_ResourceTypeMap.Contains(prefab))
					s_ResourceTypeMap.Set(prefab, itemType);
			}
		}

		// Add global custom items from DZLootSystem
		AddGlobalCustomItems();

		// OPTIMIZATION: Invalidate any cached pools since loot table changed
		InvalidateLootPoolCache();

		bool success = OutputLootTableFile();
		if(!success)
			Print(string.Format("TrainWreck: Failed to write %1", LootFileName), LogLevel.ERROR);

		HasLoaded = true;

		// Remove disabled items from the loot table
		RemoveDisabledItems();

		// Apply DayZ-style loot balancing
		ApplyLootBalancing();
	}

	//------------------------------------------------------------------------------------------------
	// Add global custom items from DZLootSystem to the loot table
	protected void AddGlobalCustomItems()
	{
		array<ref DZGlobalCustomLootItem> customItems = DZLootSystem.GetGlobalCustomLootItems();
		if (!customItems || customItems.IsEmpty())
			return;

		int addedCount = 0;

		foreach (DZGlobalCustomLootItem customItem : customItems)
		{
			if (!customItem || customItem.prefab.IsEmpty())
				continue;

			if (!customItem.isEnabled)
				continue;

			// Skip if already in global items
			if (s_GlobalItems.Contains(customItem.prefab))
			{
				Print(string.Format("[TW_LootManager] Custom item already exists: %1", customItem.prefab), LogLevel.WARNING);
				continue;
			}

			// Validate prefab exists
			Resource resource = Resource.Load(customItem.prefab);
			if (!resource.IsValid())
			{
				Print(string.Format("[TW_LootManager] Invalid custom item prefab: %1", customItem.prefab), LogLevel.WARNING);
				continue;
			}

			// Add to global items
			s_GlobalItems.Insert(customItem.prefab);

			// Create config item
			ref TW_LootConfigItem config = new TW_LootConfigItem();
			config.SetData(customItem.prefab, customItem.spawnChance, customItem.maxSpawnCount, null, true);

			// Add to appropriate category
			SCR_EArsenalItemType itemType = customItem.itemType;
			if (!s_LootTable.Contains(itemType))
				s_LootTable.Insert(itemType, new array<ref TW_LootConfigItem>());

			s_LootTable.Get(itemType).Insert(config);

			// Add to resource->type map for fast lookup
			if (!s_ResourceTypeMap.Contains(customItem.prefab))
				s_ResourceTypeMap.Set(customItem.prefab, itemType);

			addedCount++;
		}

		if (addedCount > 0)
			Print(string.Format("[TW_LootManager] Added %1 global custom items to loot table", addedCount), LogLevel.NORMAL);
	}
	
	
	//! Initialize the entire loot system
	void Initialize()
	{
		m_GameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		
		if(!TW_MonitorPositions.GetInstance())
		{
			Print("TrainWreck: Position monitor is null. Unable to initialize Loot Manager", LogLevel.ERROR);
			return;
		}
		
		m_GameMode.GetOnInitializePlugins().Insert(AddListeners);
		
		m_GameMode.GetOnGameStarted().Insert(DelayInitialize);
	}
	
	const string LootSpawnRadius = "TRAINWRECK_LOOT_RADIUS";
	const string LootAntiSpawnRadius = "TRAINWRECK_LOOT_ANTIRADIUS";
	
	private void AddListeners()
	{
		ref TW_MonitorPositions monitor = TW_MonitorPositions.GetInstance();
		
		InitializeLootTable();
		
		if(m_Settings.RespawnSettings.IsLootRespawnable)
		{
			ref TW_OnPlayerPositionsChangedInvoker onRadiusCallback = monitor.AddGridSubscription(LootSpawnRadius, m_Settings.RespawnSettings.GridSize, m_Settings.RespawnSettings.RespawnLootRadius);
			ref TW_OnPlayerPositionsChangedInvoker onAntiRadiusCallback = monitor.AddGridSubscription(LootAntiSpawnRadius, m_Settings.RespawnSettings.GridSize, m_Settings.RespawnSettings.DeadZoneRadius);
			
			onRadiusCallback.Insert(OnPlayerPositionsChanged);
			onAntiRadiusCallback.Insert(OnPlayerPositionsChanged_AntiRadius);
		}
	}
	
	private void OnPlayerPositionsChanged(GridUpdateEvent gridInfo)
	{
		m_PlayerLocations.Clear();
		m_PlayerLocations.Copy(gridInfo.GetPlayerChunks());
	}
	
	private void OnPlayerPositionsChanged_AntiRadius(GridUpdateEvent gridInfo)
	{
		m_AntiSpawnPlayerLocations.Clear();
		m_AntiSpawnPlayerLocations.Copy(gridInfo.GetPlayerChunks());
	}
	
	//! This should keep
	private void OnLootGridSizeChanged(int oldSize, int newSize)
	{
		ref array<TW_LootableInventoryComponent> entries = {};
		int count = s_GlobalContainerGrid.GetAllItems(entries);
		
		/*
			s_GlobalContainerGrid = new TW_GridCoordArrayManager<TW_LootableInventoryComponent>(newGrid.GetGridSize());
		
		foreach(TW_LootableInventoryComponent container : entries)
			if(container)
				s_GlobalContainerGrid.InsertByWorld(container.GetOwner().GetOrigin(), container);
		*/
	}
	
	private void DelayInitialize()
	{
		if(IsDebug())
			Print("TrainWreck: Initializing Loot System");
		
		if(m_Settings.RespawnSettings.IsLootRespawnable)
		{
			if(IsDebug())
				Print("TrainWreck: Loot Respawn System Enabled...");
			
			GetGame().GetCallqueue().CallLater(RespawnLootProcessor, 1000 * GetRespawnCheckInterval(), true);
		}	
	}
	
	private static ref set<string> m_PlayerLocations = new set<string>();
	private static ref set<string> m_AntiSpawnPlayerLocations = new set<string>();
	private static ref array<int> m_PlayerIds = new array<int>();
	private static ref set<TW_LootableInventoryComponent> m_InteractedWithContainers = new set<TW_LootableInventoryComponent>();
	private static int m_RespawnLootProcessor_ContainerIndex = -1;
	private static int m_RespawnLootProcessor_BatchSize = 10;
	
	static void RegisterInteractedContainer(TW_LootableInventoryComponent container)
	{
		if(!m_InteractedWithContainers.Contains(container))
			m_InteractedWithContainers.Insert(container);
	}
	
	static void UnregisterInteractedContainer(TW_LootableInventoryComponent container)
	{
		if(m_InteractedWithContainers.Contains(container))
		{
			m_InteractedWithContainers.RemoveItem(container);
			container.SetInteractedWith(false);
		}		
	}
	
	private static int GetNextIndex(int current, int length)
	{
		current += 1;
		
		if(current >= length || current < 0)
			return 0;
		
		return current;
	}
	
	private static int GetPreviousIndex(int current, int length)
	{
		current -= 1;
		
		if(current < 0)
			current = length - 1;
		
		return current;
	}
	
	static void RespawnLootProcessor()
	{
		int length = m_InteractedWithContainers.Count();
		
		if(TW_LootManager.GetInstance().IsDebug())
		{
			PrintFormat("TrainWreck: # of looted containers: %1", length);
		}
		
		// Nothing to process if we don't have containers to check
		if(length == 0) 
			return;	
		
		int processLength = m_RespawnLootProcessor_BatchSize;
		
		if(processLength > length)
			processLength = length;
		
		// We want to make sure if we hit the upper boundary
		// we rollover to the beginning
		for(int i = 0; i < processLength; i++)
		{
			m_RespawnLootProcessor_ContainerIndex = GetNextIndex(m_RespawnLootProcessor_ContainerIndex, length);
			TW_LootableInventoryComponent container = m_InteractedWithContainers.Get(m_RespawnLootProcessor_ContainerIndex);
			
			if(!container)
			{
				if(TW_LootManager.GetInstance().IsDebug())
				{
					Print("TrainWreck: Loot Container is null: removing from list...", LogLevel.WARNING);
				}
				
				m_InteractedWithContainers.Remove(m_RespawnLootProcessor_ContainerIndex);
				length -= 1;
				m_RespawnLootProcessor_ContainerIndex = GetPreviousIndex(m_RespawnLootProcessor_ContainerIndex, length);
				continue;
			}
			
			if(TW_LootManager.GetInstance().IsDebug())
			{
				Print("TrainWreck: Can spawn loot: %1", container.CanRespawnLoot());
			}
			
			string coordinate = TW_Util.ToGridText(container.GetOwner().GetOrigin(), TW_LootManager.GetInstance().GetLootSettings().RespawnSettings.GridSize);
			
			if(TW_LootManager.GetInstance().IsDebug())
			{
				PrintFormat("TrainWreck: Loot Container Coordinate: %1", coordinate);
			}
			
			if(m_AntiSpawnPlayerLocations.Contains(coordinate))
			{
				if(TW_LootManager.GetInstance().IsDebug())
				{
					PrintFormat("TrainWreck: %1 - is within a no-respawn area around a player", coordinate, LogLevel.WARNING);
				}
				
				continue;
			}
			
			if(!container.CanRespawnLoot())
				continue;
			
			container.SetInteractedWith(false);
			m_InteractedWithContainers.Remove(m_RespawnLootProcessor_ContainerIndex);
			
			length -= 1;
			
			m_RespawnLootProcessor_ContainerIndex = GetPreviousIndex(m_RespawnLootProcessor_ContainerIndex, length);
		}		
	}
		
	//! Trickle spawn loot into a container - OPTIMIZED: uses cached pool
	void TrickleSpawnLootInContainer(TW_LootableInventoryComponent container, int remainingAmount)
	{
		if(!m_Settings.IsLootEnabled || !container || remainingAmount <= 0)
			return;

		// OPTIMIZATION: Use cached pool instead of creating new one
		WeightedType<ref TW_LootConfigItem> pool = GetCachedLootPool(container.GetTypeFlags());

		if(!pool)
			return;

		TW_LootConfigItem arsenalItem = pool.GetRandomItem();

		if(!arsenalItem)
		{
			// Still try remaining items
			if(remainingAmount > 1)
				GetGame().GetCallqueue().CallLater(TrickleSpawnLootInContainer, 250, false, container, remainingAmount - 1);
			return;
		}

		container.InsertItem(arsenalItem);

		// Continue with remaining items
		if(remainingAmount > 1)
			GetGame().GetCallqueue().CallLater(TrickleSpawnLootInContainer, 250, false, container, remainingAmount - 1);
	}
	
	//! Spawn loot in designated container - OPTIMIZED: uses cached pool
	void SpawnLootInContainer(TW_LootableInventoryComponent container)
	{
		if(!m_Settings.IsLootEnabled || !container)
			return;

		int spawnCount = Math.RandomIntInclusive(1, 4);

		// OPTIMIZATION: Use cached pool instead of creating new one
		WeightedType<ref TW_LootConfigItem> pool = GetCachedLootPool(container.GetTypeFlags());

		if(!pool)
			return;

		// How many different things are we going to try spawning?
		for(int i = 0; i < spawnCount; i++)
		{
			TW_LootConfigItem arsenalItem = pool.GetRandomItem();

			if(!arsenalItem)
				continue;

			// Add item a random amount of times to the container based on settings
			int itemCount = Math.RandomIntInclusive(1, arsenalItem.randomSpawnCount);

			for(int x = 0; x < itemCount; x++)
			{
				bool success = container.InsertItem(arsenalItem);
				if(!success)
				{
					spawnCount--;
					break;
				}
			}
		}
	}
	
	static TW_LootConfigItem GetRandomByFlag(int type)
	{				
		if(type <= 0)
			return null;
		
		array<SCR_EArsenalItemType> selectedItems = {};
		
		foreach(SCR_EArsenalItemType itemType : s_ArsenalItemTypes)
			if(SCR_Enum.HasFlag(type, itemType) && s_LootTable.Contains(itemType))
				selectedItems.Insert(itemType);
		
		// Check if nothing was selected
		if(selectedItems.IsEmpty())
			return null;
		
		auto items = s_LootTable.Get(selectedItems.GetRandomElement());
		
		// Check if nothing was available
		if(!items || items.IsEmpty())
			return null;
		
		ref TW_LootConfigItem item = items.GetRandomElement();
		
		if(item.isEnabled) return item;
		return null;
	}
	
	// OPTIMIZED: Returns cached pool or builds and caches new one
	static WeightedType<ref TW_LootConfigItem> GetCachedLootPool(SCR_EArsenalItemType flags)
	{
		int flagKey = flags;

		// Check cache first
		if(s_CachedLootPools.Contains(flagKey))
			return s_CachedLootPools.Get(flagKey);

		// Build new pool and cache it
		ref WeightedType<ref TW_LootConfigItem> pool = new WeightedType<ref TW_LootConfigItem>();

		for(int i = 0; i < s_ArsenalItemTypes.Count(); i++)
		{
			SCR_EArsenalItemType itemType = s_ArsenalItemTypes[i];

			if(!SCR_Enum.HasFlag(flags, itemType))
				continue;

			if(!s_LootTable.Contains(itemType))
				continue;

			ref array<ref TW_LootConfigItem> entries = s_LootTable.Get(itemType);
			for(int j = 0; j < entries.Count(); j++)
			{
				TW_LootConfigItem item = entries[j];
				if(item.isEnabled && item.chanceToSpawn > 0)
					pool.Add(item, item.chanceToSpawn);
			}
		}

		s_CachedLootPools.Set(flagKey, pool);
		return pool;
	}

	// Legacy method for compatibility - now uses cached pool
	static void GetLootPoolForContainer(SCR_EArsenalItemType flags, notnull out WeightedType<ref TW_LootConfigItem> pool)
	{
		WeightedType<ref TW_LootConfigItem> cached = GetCachedLootPool(flags);

		// Copy items to output pool (for cases where caller modifies pool)
		for(int i = 0; i < s_ArsenalItemTypes.Count(); i++)
		{
			SCR_EArsenalItemType itemType = s_ArsenalItemTypes[i];

			if(!SCR_Enum.HasFlag(flags, itemType))
				continue;

			if(!s_LootTable.Contains(itemType))
				continue;

			ref array<ref TW_LootConfigItem> entries = s_LootTable.Get(itemType);
			for(int j = 0; j < entries.Count(); j++)
			{
				TW_LootConfigItem item = entries[j];
				if(item.isEnabled && item.chanceToSpawn > 0)
					pool.Add(item, item.chanceToSpawn);
			}
		}
	}

	// OPTIMIZATION: Invalidate pool cache when loot table changes
	static void InvalidateLootPoolCache()
	{
		s_CachedLootPools.Clear();
	}
	
	// OPTIMIZATION: Cache weapon check results to avoid repeated Resource.Load
	private static ref set<string> s_CachedWeapons = new set<string>();
	private static ref set<string> s_CachedNonWeapons = new set<string>();

	static int GetWeapons(notnull array<ref TW_LootConfigItem> weapons)
	{
		int count = 0;

		for(int t = 0; t < s_LootTable.Count(); t++)
		{
			ref array<ref TW_LootConfigItem> items = s_LootTable.GetElement(t);

			for(int i = 0; i < items.Count(); i++)
			{
				TW_LootConfigItem item = items[i];

				// Check cached results first
				if(s_CachedNonWeapons.Contains(item.resourceName))
					continue;

				if(s_CachedWeapons.Contains(item.resourceName))
				{
					weapons.Insert(item);
					count++;
					continue;
				}

				// Not cached - check and cache result
				if(!SCR_BaseContainerTools.FindComponentSource(Resource.Load(item.resourceName), "WeaponComponent"))
				{
					s_CachedNonWeapons.Insert(item.resourceName);
					continue;
				}

				s_CachedWeapons.Insert(item.resourceName);
				weapons.Insert(item);
				count++;
			}
		}

		return count;
	}
	
	// OPTIMIZATION: Cache component check results by resource+component
	private static ref map<string, bool> s_CachedComponentChecks = new map<string, bool>();

	static int GetPrefabsOfType(SCR_EArsenalItemType type, notnull array<ref TW_LootConfigItem> items, string ensureHasComponent = string.Empty)
	{
		int count = 0;

		for(int t = 0; t < s_ArsenalItemTypes.Count(); t++)
		{
			SCR_EArsenalItemType flagType = s_ArsenalItemTypes[t];

			if(!SCR_Enum.HasFlag(type, flagType))
				continue;

			if(!s_LootTable.Contains(flagType))
				continue;

			ref array<ref TW_LootConfigItem> configs = s_LootTable.Get(flagType);

			for(int i = 0; i < configs.Count(); i++)
			{
				TW_LootConfigItem config = configs[i];

				if(ensureHasComponent != string.Empty)
				{
					// OPTIMIZATION: Check cache first
					string cacheKey = config.resourceName + "|" + ensureHasComponent;

					if(s_CachedComponentChecks.Contains(cacheKey))
					{
						if(!s_CachedComponentChecks.Get(cacheKey))
							continue;
					}
					else
					{
						bool hasComponent = SCR_BaseContainerTools.FindComponentSource(Resource.Load(config.resourceName), ensureHasComponent) != null;
						s_CachedComponentChecks.Set(cacheKey, hasComponent);

						if(!hasComponent)
							continue;
					}
				}

				count++;
				items.Insert(config);
			}
		}

		return count;
	}
	
	private bool OutputLootTableFile()
	{
		if(!m_Settings.LootTable)
			m_Settings.LootTable = new map<string, ref array<ref TW_LootConfigItem>>();
				
		foreach(SCR_EArsenalItemType type, ref array<ref TW_LootConfigItem> items : s_LootTable)
		{	
			PrintFormat("TrainWreckLooting: Type: %1, Amount %2 -- Saving", TW_Util.ArsenalTypeAsString(type), items.Count());
						
			m_Settings.LootTable.Set(TW_Util.ArsenalTypeAsString(type), items);
		}
		
		return TW_Util.SaveJsonFile(LootFileName, m_Settings, true);
	}
	
	private static bool HasLootTable()
	{
		SCR_JsonLoadContext loadContext = new SCR_JsonLoadContext();
		return loadContext.LoadFromFile(LootFileName);
	}
	
	private static bool IngestLootTableFromFile(out LootManagerSettings settings)
	{
		SCR_JsonLoadContext context = TW_Util.LoadJsonFile(LootFileName, true);
		bool loadSuccess = context.ReadValue("", settings);

		if(!loadSuccess)
		{
			Print("TrainWreck: Was unable to load loot map. Please verify it exists, and has valid syntax");
			return false;
		}

		array<SCR_EArsenalItemType> itemTypes = {};
		SCR_Enum.GetEnumValues(SCR_EArsenalItemType, itemTypes);

		ref map<string, SCR_EArsenalItemType> typeMap = new map<string, SCR_EArsenalItemType>();
		foreach(SCR_EArsenalItemType type : itemTypes)
			typeMap.Set(TW_Util.ArsenalTypeAsString(type), type);

		foreach(string name, ref array<ref TW_LootConfigItem> items : settings.LootTable)
		{
			if(!typeMap.Contains(name))
			{
				PrintFormat("TrainWreck: JsonFile '%1' -> Invalid SCR_EArsenalItemType '%2'. Skipping Section...", LootFileName, name, LogLevel.ERROR);
				continue;
			}

			SCR_EArsenalItemType itemType = typeMap.Get(name);

			if(!s_LootTable.Contains(itemType))
				s_LootTable.Insert(itemType, {});

			for(int i = 0; i < items.Count(); i++)
			{
				TW_LootConfigItem item = items[i];

				// OPTIMIZATION: Use cached validation results
				if(s_InvalidResources.Contains(item.resourceName))
					continue;

				if(!s_ValidatedResources.Contains(item.resourceName))
				{
					Resource resource = Resource.Load(item.resourceName);
					if(!resource.IsValid())
					{
						s_InvalidResources.Insert(item.resourceName);
						PrintFormat("TrainWreck: LootType('%1') -> Prefab Invalid: '%2'", name, item.resourceName, LogLevel.WARNING);
						continue;
					}
					s_ValidatedResources.Insert(item.resourceName);
				}

				if(HasLoaded)
					PrintFormat("TrainWreck: Item: %1, Chance: %2", item.resourceName, item.chanceToSpawn);

				if(!s_GlobalItems.Contains(item.resourceName))
					s_GlobalItems.Insert(item.resourceName);
				else
					continue;

				s_LootTable.Get(itemType).Insert(item);

				// OPTIMIZATION: Add to resource->type map for fast lookup
				if(!s_ResourceTypeMap.Contains(item.resourceName))
					s_ResourceTypeMap.Set(item.resourceName, itemType);
			}
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	// Load only settings (respawn, etc) from lootmap.json, not the loot items
	// Used when loading loot from pre-generated config but still need game settings
	private static bool IngestSettingsOnly(out LootManagerSettings settings)
	{
		SCR_JsonLoadContext context = TW_Util.LoadJsonFile(LootFileName, true);
		bool loadSuccess = context.ReadValue("", settings);

		if(!loadSuccess)
		{
			Print("[TW_LootManager] Could not load settings from lootmap.json, using defaults", LogLevel.WARNING);
			settings = new LootManagerSettings();
			return false;
		}

		Print("[TW_LootManager] Loaded game settings from lootmap.json", LogLevel.NORMAL);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	// Remove disabled items from the loot table
	private void RemoveDisabledItems()
	{
		foreach(SCR_EArsenalItemType type, ref array<ref TW_LootConfigItem> items : s_LootTable)
		{
			int count = items.Count();

			for(int i = 0; i < count; i++)
			{
				ref TW_LootConfigItem configItem = items.Get(i);
				if(configItem.isEnabled && configItem.chanceToSpawn > 0)
					continue;

				if(IsDebug())
					PrintFormat("[TW_LootManager] Removing '%1'. Enabled(%2) | Chance(%3)", configItem.resourceName, configItem.isEnabled, configItem.chanceToSpawn);

				items.RemoveItem(configItem);
				i -= 1;
				count -= 1;
			}
		}
	}
};