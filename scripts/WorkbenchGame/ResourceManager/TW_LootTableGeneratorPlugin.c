// Workbench plugin to generate loot table configuration from faction catalogs
// Run this in Workbench to create/update the loot config file
// Then edit the config to control what spawns and what doesn't

[WorkbenchPluginAttribute(
	name: "TW Loot Table Generator",
	category: "TrainWreck Tools",
	description: "Generates loot table config from faction catalogs. Edit the output to control spawns.",
	wbModules: {"ResourceManager"},
	awesomeFontCode: 0xf0ce)] // table icon
class TW_LootTableGeneratorPlugin : ResourceManagerPlugin
{
	// Settings
	[Attribute("$profile:loot_config.json", desc: "Output file path for loot config")]
	protected string m_sOutputPath;

	[Attribute("1", UIWidgets.CheckBox, "Include weapons")]
	protected bool m_bIncludeWeapons;

	[Attribute("1", UIWidgets.CheckBox, "Include equipment")]
	protected bool m_bIncludeEquipment;

	[Attribute("1", UIWidgets.CheckBox, "Include consumables")]
	protected bool m_bIncludeConsumables;

	[Attribute("1", UIWidgets.CheckBox, "Include attachments")]
	protected bool m_bIncludeAttachments;

	[Attribute("1", UIWidgets.CheckBox, "Use supply cost to calculate spawn chance (recommended)")]
	protected bool m_bUseSupplyCostForChance;

	[Attribute("50", desc: "Max spawn chance for cheapest items (0-100)")]
	protected int m_iMaxSpawnChance;

	[Attribute("1", desc: "Min spawn chance for most expensive items (0-100)")]
	protected int m_iMinSpawnChance;

	[Attribute("25", desc: "Default spawn chance when not using supply cost (0-100)")]
	protected int m_iDefaultChance;

	[Attribute("1", desc: "Default max spawn count")]
	protected int m_iDefaultMaxCount;

	// Catalog paths - user can add their own
	[Attribute("", desc: "Additional catalog path 1 (e.g. {GUID}Configs/EntityCatalogs/Items/MyItems.conf)")]
	protected string m_sCatalogPath1;

	[Attribute("", desc: "Additional catalog path 2")]
	protected string m_sCatalogPath2;

	[Attribute("", desc: "Additional catalog path 3")]
	protected string m_sCatalogPath3;

	// Stats
	protected int m_iTotalItems = 0;
	protected int m_iCategoriesFound = 0;

	//------------------------------------------------------------------------------------------------
	override void Run()
	{
		m_iTotalItems = 0;
		m_iCategoriesFound = 0;

		Print("[TW_LootGenerator] Starting loot table generation...", LogLevel.NORMAL);

		// Build the loot table from catalogs
		ref map<string, ref array<ref TW_GeneratedLootItem>> lootTable = new map<string, ref array<ref TW_GeneratedLootItem>>();

		// Get all arsenal item types
		array<SCR_EArsenalItemType> itemTypes = {};
		SCR_Enum.GetEnumValues(SCR_EArsenalItemType, itemTypes);

		// Get faction catalogs
		ref array<SCR_EntityCatalogEntry> catalogEntries = GetAllFactionCatalogEntries();

		if (!catalogEntries || catalogEntries.Count() == 0)
		{
			Print("[TW_LootGenerator] No catalog entries found!", LogLevel.ERROR);
			Workbench.Dialog("TW Loot Generator",
				"Failed to find any faction catalog entries.\n\n" +
				"To fix this:\n" +
				"1. Right-click the plugin and select 'Configure'\n" +
				"2. Add your catalog paths in the 'Catalog Path' fields\n" +
				"3. Use the full path with GUID, e.g.:\n" +
				"   {GUID}Configs/EntityCatalogs/Items/YourCatalog.conf\n\n" +
				"You can find catalog GUIDs in the Resource Browser\n" +
				"by right-clicking a .conf file and selecting 'Copy Resource Name'");
			return;
		}

		Print(string.Format("[TW_LootGenerator] Found %1 catalog entries", catalogEntries.Count()), LogLevel.NORMAL);

		// Process each catalog entry
		for (int i = 0; i < catalogEntries.Count(); i++)
		{
			SCR_EntityCatalogEntry entry = catalogEntries[i];
			ProcessCatalogEntry(entry, itemTypes, lootTable);
		}

		// Generate output
		ref TW_LootTableConfig config = new TW_LootTableConfig();
		config.Version = "1.0";
		config.GeneratedDate = GetCurrentDateString();
		config.DefaultChance = m_iDefaultChance;
		config.DefaultMaxCount = m_iDefaultMaxCount;
		config.Categories = lootTable;

		// Save to file
		bool success = SaveConfigToFile(config);

		// Show results
		string statusText = "FAILED";
		if (success)
			statusText = "SUCCESS";

		string chanceMethod = "Fixed default";
		if (m_bUseSupplyCostForChance)
			chanceMethod = string.Format("Supply cost (range %1-%2)", m_iMinSpawnChance, m_iMaxSpawnChance);

		string results = string.Format(
			"Loot Table Generation Complete\n\n" +
			"Categories: %1\n" +
			"Total Items: %2\n" +
			"Spawn Chance: %3\n" +
			"Output: %4\n\n" +
			"Status: %5",
			m_iCategoriesFound,
			m_iTotalItems,
			chanceMethod,
			m_sOutputPath,
			statusText);

		Print(string.Format("[TW_LootGenerator] Generated %1 items in %2 categories", m_iTotalItems, m_iCategoriesFound), LogLevel.NORMAL);

		Workbench.Dialog("TW Loot Table Generator", results);
	}

	//------------------------------------------------------------------------------------------------
	override void Configure()
	{
		Workbench.ScriptDialog(
			"TW Loot Table Generator Settings",
			"Configure loot table generation.\n\n" +
			"CATALOG PATHS:\n" +
			"To find your catalog paths, go to Resource Browser,\n" +
			"find your EntityCatalog .conf file, right-click it,\n" +
			"and select 'Copy Resource Name'.\n\n" +
			"OUTPUT:\n" +
			"The generated JSON can be edited to control spawns.\n" +
			"Set 'isEnabled' to false to disable items.\n" +
			"Adjust 'chanceToSpawn' (0-100) to control rarity.",
			this);
	}

	//------------------------------------------------------------------------------------------------
	protected ref array<SCR_EntityCatalogEntry> GetAllFactionCatalogEntries()
	{
		ref array<SCR_EntityCatalogEntry> allEntries = new array<SCR_EntityCatalogEntry>();

		// Build list of catalogs to try
		ref array<string> catalogsToTry = new array<string>();

		// Add user-specified paths first
		if (!m_sCatalogPath1.IsEmpty())
			catalogsToTry.Insert(m_sCatalogPath1);
		if (!m_sCatalogPath2.IsEmpty())
			catalogsToTry.Insert(m_sCatalogPath2);
		if (!m_sCatalogPath3.IsEmpty())
			catalogsToTry.Insert(m_sCatalogPath3);

		// Try common vanilla catalog paths (various GUIDs that have been used)
		// US faction catalogs
		catalogsToTry.Insert("{C9731C02AA9B85BB}Configs/EntityCatalogs/Items/EntityCatalog_Items_US.conf");
		catalogsToTry.Insert("{E5B8A38AA0672EB9}Configs/EntityCatalogs/Items/EntityCatalog_Items_US.conf");

		// USSR faction catalogs
		catalogsToTry.Insert("{F0D54A021771E081}Configs/EntityCatalogs/Items/EntityCatalog_Items_USSR.conf");
		catalogsToTry.Insert("{C9C9A6B846D50D5D}Configs/EntityCatalogs/Items/EntityCatalog_Items_USSR.conf");

		// FIA faction catalogs
		catalogsToTry.Insert("{C661071BF40B4B53}Configs/EntityCatalogs/Items/EntityCatalog_Items_FIA.conf");
		catalogsToTry.Insert("{1068F0B08A2B0580}Configs/EntityCatalogs/Items/EntityCatalog_Items_FIA.conf");

		// Try non-GUID paths (mod paths)
		catalogsToTry.Insert("Configs/EntityCatalogs/Items/EntityCatalog_Items_US.conf");
		catalogsToTry.Insert("Configs/EntityCatalogs/Items/EntityCatalog_Items_USSR.conf");
		catalogsToTry.Insert("Configs/EntityCatalogs/Items/EntityCatalog_Items_FIA.conf");

		Print(string.Format("[TW_LootGenerator] Trying %1 catalog paths...", catalogsToTry.Count()), LogLevel.NORMAL);

		foreach (string catalogPath : catalogsToTry)
		{
			LoadCatalogEntries(catalogPath, allEntries);
		}

		return allEntries;
	}

	//------------------------------------------------------------------------------------------------
	protected void LoadCatalogEntries(string catalogPath, array<SCR_EntityCatalogEntry> allEntries)
	{
		if (catalogPath.IsEmpty())
			return;

		Resource resource = Resource.Load(catalogPath);
		if (!resource.IsValid())
			return;

		BaseResourceObject resourceObj = resource.GetResource();
		if (!resourceObj)
			return;

		BaseContainer container = resourceObj.ToBaseContainer();
		if (!container)
			return;

		SCR_EntityCatalog catalog = SCR_EntityCatalog.Cast(BaseContainerTools.CreateInstanceFromContainer(container));
		if (!catalog)
			return;

		ref array<SCR_EntityCatalogEntry> entries = new array<SCR_EntityCatalogEntry>();
		catalog.GetEntityList(entries);

		if (entries.Count() > 0)
		{
			Print(string.Format("[TW_LootGenerator] Loaded %1 entries from %2", entries.Count(), catalogPath), LogLevel.NORMAL);

			foreach (SCR_EntityCatalogEntry entry : entries)
			{
				allEntries.Insert(entry);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void ProcessCatalogEntry(SCR_EntityCatalogEntry entry, array<SCR_EArsenalItemType> itemTypes, map<string, ref array<ref TW_GeneratedLootItem>> lootTable)
	{
		if (!entry)
			return;

		ref array<SCR_BaseEntityCatalogData> itemDataList = new array<SCR_BaseEntityCatalogData>();
		entry.GetEntityDataList(itemDataList);

		for (int i = 0; i < itemDataList.Count(); i++)
		{
			SCR_ArsenalItem arsenalItem = SCR_ArsenalItem.Cast(itemDataList[i]);
			if (!arsenalItem)
				continue;

			if (!arsenalItem.IsEnabled())
				continue;

			SCR_EArsenalItemType itemType = arsenalItem.GetItemType();
			ResourceName prefab = entry.GetPrefab();

			if (prefab.IsEmpty())
				continue;

			// Filter by settings
			if (!ShouldIncludeType(itemType))
				continue;

				// Get category name
			string categoryName = GetCategoryName(itemType);

			// Get supply cost from arsenal item
			int supplyCost = arsenalItem.GetSupplyCost(SCR_EArsenalSupplyCostType.DEFAULT);

			// Calculate spawn chance based on supply cost
			int spawnChance = m_iDefaultChance;
			if (m_bUseSupplyCostForChance && supplyCost > 0)
			{
				spawnChance = CalculateSpawnChanceFromCost(supplyCost);
			}

			// Create loot item entry
			ref TW_GeneratedLootItem lootItem = new TW_GeneratedLootItem();
			lootItem.resourceName = prefab;
			lootItem.displayName = GetPrefabDisplayName(prefab);
			lootItem.chanceToSpawn = spawnChance;
			lootItem.maxSpawnCount = m_iDefaultMaxCount;
			lootItem.isEnabled = true;
			lootItem.supplyCost = supplyCost;

			// Add to category
			if (!lootTable.Contains(categoryName))
			{
				lootTable.Set(categoryName, new array<ref TW_GeneratedLootItem>());
				m_iCategoriesFound++;
			}

			// Check for duplicates
			bool isDuplicate = false;
			ref array<ref TW_GeneratedLootItem> categoryItems = lootTable.Get(categoryName);
			for (int j = 0; j < categoryItems.Count(); j++)
			{
				if (categoryItems[j].resourceName == prefab)
				{
					isDuplicate = true;
					break;
				}
			}

			if (!isDuplicate)
			{
				categoryItems.Insert(lootItem);
				m_iTotalItems++;
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected bool ShouldIncludeType(SCR_EArsenalItemType itemType)
	{
		// Weapons
		if (itemType == SCR_EArsenalItemType.RIFLE ||
			itemType == SCR_EArsenalItemType.PISTOL ||
			itemType == SCR_EArsenalItemType.MACHINE_GUN ||
			itemType == SCR_EArsenalItemType.SNIPER_RIFLE ||
			itemType == SCR_EArsenalItemType.ROCKET_LAUNCHER ||
			itemType == SCR_EArsenalItemType.MORTARS)
		{
			return m_bIncludeWeapons;
		}

		// Equipment (clothing and gear)
		if (itemType == SCR_EArsenalItemType.HEADWEAR ||
			itemType == SCR_EArsenalItemType.VEST_AND_WAIST ||
			itemType == SCR_EArsenalItemType.BACKPACK ||
			itemType == SCR_EArsenalItemType.RADIO_BACKPACK ||
			itemType == SCR_EArsenalItemType.TORSO ||
			itemType == SCR_EArsenalItemType.LEGS ||
			itemType == SCR_EArsenalItemType.FOOTWEAR ||
			itemType == SCR_EArsenalItemType.HANDWEAR ||
			itemType == SCR_EArsenalItemType.EQUIPMENT)
		{
			return m_bIncludeEquipment;
		}

		// Consumables
		if (itemType == SCR_EArsenalItemType.HEAL)
		{
			return m_bIncludeConsumables;
		}

		// Attachments
		if (itemType == SCR_EArsenalItemType.WEAPON_ATTACHMENT)
		{
			return m_bIncludeAttachments;
		}

		// Throwables and explosives
		if (itemType == SCR_EArsenalItemType.LETHAL_THROWABLE ||
			itemType == SCR_EArsenalItemType.NON_LETHAL_THROWABLE ||
			itemType == SCR_EArsenalItemType.EXPLOSIVES)
		{
			return m_bIncludeWeapons;
		}

		// Exclude vehicles by default (HELICOPTER, VEHICLE)
		if (itemType == SCR_EArsenalItemType.HELICOPTER ||
			itemType == SCR_EArsenalItemType.VEHICLE)
		{
			return false;
		}

		// Include anything else by default
		return true;
	}

	//------------------------------------------------------------------------------------------------
	// Calculate spawn chance based on supply cost
	// Higher cost = lower spawn chance
	// Typical costs: Bandage ~5, Rifle ~150, MG ~400, Launcher ~800
	protected int CalculateSpawnChanceFromCost(int supplyCost)
	{
		if (supplyCost <= 0)
			return m_iMaxSpawnChance;

		// Use inverse formula for smooth distribution
		// Formula: chance = maxChance / (1 + cost/scaleFactor)
		// scaleFactor controls how quickly chance drops off
		float scaleFactor = 200.0;  // Cost at which chance is halved
		float chanceRange = m_iMaxSpawnChance - m_iMinSpawnChance;

		// Calculate reduction factor (0 to 1)
		float reduction = supplyCost / (supplyCost + scaleFactor);

		// Calculate final chance
		int spawnChance = m_iMaxSpawnChance - (reduction * chanceRange);

		// Clamp to valid range
		if (spawnChance < m_iMinSpawnChance)
			spawnChance = m_iMinSpawnChance;
		if (spawnChance > m_iMaxSpawnChance)
			spawnChance = m_iMaxSpawnChance;

		return spawnChance;
	}

	//------------------------------------------------------------------------------------------------
	protected string GetCategoryName(SCR_EArsenalItemType itemType)
	{
		return SCR_Enum.GetEnumName(SCR_EArsenalItemType, itemType);
	}

	//------------------------------------------------------------------------------------------------
	protected string GetPrefabDisplayName(ResourceName prefab)
	{
		Resource resource = Resource.Load(prefab);
		if (!resource.IsValid())
			return prefab;

		IEntitySource entitySource = SCR_BaseContainerTools.FindEntitySource(resource);
		if (!entitySource)
			return prefab;

		// Try to get display name from InventoryItemComponent
		IEntityComponentSource invComp = SCR_BaseContainerTools.FindComponentSource(resource, "InventoryItemComponent");
		if (invComp)
		{
			string displayName;
			if (invComp.Get("m_sDisplayName", displayName) && displayName != "")
				return displayName;
		}

		// Fallback to prefab name
		int lastSlash = prefab.LastIndexOf("/");
		if (lastSlash >= 0)
			return prefab.Substring(lastSlash + 1, prefab.Length() - lastSlash - 1);

		return prefab;
	}

	//------------------------------------------------------------------------------------------------
	protected string GetCurrentDateString()
	{
		int year, month, day, hour, minute, second;
		System.GetYearMonthDay(year, month, day);
		System.GetHourMinuteSecond(hour, minute, second);

		return string.Format("%1-%2-%3 %4:%5:%6", year, month, day, hour, minute, second);
	}

	//------------------------------------------------------------------------------------------------
	protected bool SaveConfigToFile(TW_LootTableConfig config)
	{
		SCR_JsonSaveContext saveContext = new SCR_JsonSaveContext();

		if (!saveContext.WriteValue("", config))
		{
			Print("[TW_LootGenerator] Failed to serialize config", LogLevel.ERROR);
			return false;
		}

		if (!saveContext.SaveToFile(m_sOutputPath))
		{
			Print(string.Format("[TW_LootGenerator] Failed to save to %1", m_sOutputPath), LogLevel.ERROR);
			return false;
		}

		Print(string.Format("[TW_LootGenerator] Saved config to %1", m_sOutputPath), LogLevel.NORMAL);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Generate Loot Table")]
	void GenerateButton()
	{
		Run();
	}

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Open Output Folder")]
	void OpenFolderButton()
	{
		// Get profile folder
		Workbench.RunCmd("explorer " + "$profile:");
	}
}

// Note: TW_GeneratedLootItem and TW_LootTableConfig classes are defined in
// scripts/Game/Looting/Config/TW_LootTableLoader.c to avoid duplication
