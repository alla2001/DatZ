// Enum for easy zone selection in World Editor
enum DatZLootZoneType
{
	NONE = 0,
	// Tiered Zones (DayZ-style progression)
	TIER1_COASTAL = 1,      // Basic food, water, civilian clothing only
	TIER2_RESIDENTIAL = 2,  // Civilian gear, melee weapons, rare pistol
	TIER3_INDUSTRIAL = 3,   // Tools, backpacks, work gear
	TIER4_MEDICAL = 4,      // Medical supplies
	TIER4_POLICE = 5,       // Pistols, vests, tactical gear
	TIER5_MILITARY = 6,     // Military rifles, gear
	TIER5_AIRFIELD = 7,     // High-tier military
	// Special Zones (legacy config-based)
	FOOD = 10,
	FARM = 11,
	BOAT = 12,
	COOKING = 13,
	VALUABLES = 14,
	DRUGS = 15,
	KEYCARD = 16,
	// Custom (uses file picker)
	CUSTOM = 99
}

class DatZLootOverrideClass : SCR_PositionClass
{
}

class DatZLootOverride : SCR_Position
{
	[Attribute("100", UIWidgets.Slider, "Override radius in meters", "10 500 10")]
	float overrideRadius;

	[Attribute("0.9", UIWidgets.Slider, "Chance to override each spawner (0-1)", "0 1 0.01")]
	float chance;

	[Attribute("0", UIWidgets.ComboBox, "Loot Zone Type", "", ParamEnumArray.FromEnum(DatZLootZoneType))]
	DatZLootZoneType zoneType;

	[Attribute("", UIWidgets.ResourceNamePicker, desc: "Only used if Zone Type is CUSTOM", params: "conf")]
	ResourceName customLootTable;

	[Attribute("-1", desc: "Custom type filter (only used if Zone Type is CUSTOM, -1 = use config file)")]
	int customTypeFilter;

	[Attribute("50", UIWidgets.Slider, "Custom empty chance (only used if Zone Type is CUSTOM)", "0 200 5")]
	float customEmptyChance;

	[Attribute("true", UIWidgets.CheckBox, "Show zone circle on in-game map")]
	bool showOnMap;

	// Spawned map marker entity
	protected DatZMapZoneMarker m_MapMarker;

	//------------------------------------------------------------------------------------------------
	//! Get the item type filter flags for this zone
	//! Returns SCR_EArsenalItemType flags combined with bitwise OR
	//! -1 means use legacy config file system
	int GetTypeFilterForZone()
	{
		switch (zoneType)
		{
			// TIER1 - Coastal/Starter: Only basic survival items, clothing, food (NO WEAPONS)
			case DatZLootZoneType.TIER1_COASTAL:
				return SCR_EArsenalItemType.HEADWEAR |
				       SCR_EArsenalItemType.TORSO |
				       SCR_EArsenalItemType.LEGS |
				       SCR_EArsenalItemType.FOOTWEAR |
				       SCR_EArsenalItemType.HANDWEAR |
				       SCR_EArsenalItemType.EQUIPMENT |
				       SCR_EArsenalItemType.FOOD;

			// TIER2 - Residential: Civilian gear + food + heal + rare pistol
			case DatZLootZoneType.TIER2_RESIDENTIAL:
				return SCR_EArsenalItemType.HEADWEAR |
				       SCR_EArsenalItemType.TORSO |
				       SCR_EArsenalItemType.LEGS |
				       SCR_EArsenalItemType.FOOTWEAR |
				       SCR_EArsenalItemType.HANDWEAR |
				       SCR_EArsenalItemType.EQUIPMENT |
				       SCR_EArsenalItemType.BACKPACK |
				       SCR_EArsenalItemType.HEAL |
				       SCR_EArsenalItemType.FOOD |
				       SCR_EArsenalItemType.PISTOL;

			// TIER3 - Industrial: Work gear, tools, backpacks, industrial items
			case DatZLootZoneType.TIER3_INDUSTRIAL:
				return SCR_EArsenalItemType.HEADWEAR |
				       SCR_EArsenalItemType.TORSO |
				       SCR_EArsenalItemType.LEGS |
				       SCR_EArsenalItemType.FOOTWEAR |
				       SCR_EArsenalItemType.HANDWEAR |
				       SCR_EArsenalItemType.EQUIPMENT |
				       SCR_EArsenalItemType.BACKPACK |
				       SCR_EArsenalItemType.VEST_AND_WAIST |
				       SCR_EArsenalItemType.INDUSTRIAL;

			// TIER4 - Medical: Medical supplies focus + food
			case DatZLootZoneType.TIER4_MEDICAL:
				return SCR_EArsenalItemType.HEAL |
				       SCR_EArsenalItemType.BACKPACK |
				       SCR_EArsenalItemType.EQUIPMENT |
				       SCR_EArsenalItemType.FOOD;

			// TIER4 - Police: Pistols, vests, tactical equipment, ammo
			case DatZLootZoneType.TIER4_POLICE:
				return SCR_EArsenalItemType.PISTOL |
				       SCR_EArsenalItemType.VEST_AND_WAIST |
				       SCR_EArsenalItemType.HEADWEAR |
				       SCR_EArsenalItemType.EQUIPMENT |
				       SCR_EArsenalItemType.HEAL |
				       SCR_EArsenalItemType.BACKPACK |
				       SCR_EArsenalItemType.WEAPON_ATTACHMENT |
				       SCR_EArsenalItemType.AMMO;

			// TIER5 - Military: Full military loadout (rifles, gear, ammo)
			case DatZLootZoneType.TIER5_MILITARY:
				return SCR_EArsenalItemType.RIFLE |
				       SCR_EArsenalItemType.PISTOL |
				       SCR_EArsenalItemType.MACHINE_GUN |
				       SCR_EArsenalItemType.SNIPER_RIFLE |
				       SCR_EArsenalItemType.VEST_AND_WAIST |
				       SCR_EArsenalItemType.HEADWEAR |
				       SCR_EArsenalItemType.BACKPACK |
				       SCR_EArsenalItemType.RADIO_BACKPACK |
				       SCR_EArsenalItemType.HEAL |
				       SCR_EArsenalItemType.EQUIPMENT |
				       SCR_EArsenalItemType.WEAPON_ATTACHMENT |
				       SCR_EArsenalItemType.LETHAL_THROWABLE |
				       SCR_EArsenalItemType.AMMO |
				       SCR_EArsenalItemType.FOOD;

			// TIER5 - Airfield: High-tier military including rare weapons, ammo
			case DatZLootZoneType.TIER5_AIRFIELD:
				return SCR_EArsenalItemType.RIFLE |
				       SCR_EArsenalItemType.PISTOL |
				       SCR_EArsenalItemType.MACHINE_GUN |
				       SCR_EArsenalItemType.SNIPER_RIFLE |
				       SCR_EArsenalItemType.VEST_AND_WAIST |
				       SCR_EArsenalItemType.HEADWEAR |
				       SCR_EArsenalItemType.TORSO |
				       SCR_EArsenalItemType.LEGS |
				       SCR_EArsenalItemType.BACKPACK |
				       SCR_EArsenalItemType.RADIO_BACKPACK |
				       SCR_EArsenalItemType.HEAL |
				       SCR_EArsenalItemType.EQUIPMENT |
				       SCR_EArsenalItemType.WEAPON_ATTACHMENT |
				       SCR_EArsenalItemType.LETHAL_THROWABLE |
				       SCR_EArsenalItemType.EXPLOSIVES |
				       SCR_EArsenalItemType.AMMO |
				       SCR_EArsenalItemType.FOOD;

			// Custom zone with manual type filter
			case DatZLootZoneType.CUSTOM:
				return customTypeFilter;

			// Special zones use legacy config system
			default:
				return -1;
		}
		return -1;
	}

	//------------------------------------------------------------------------------------------------
	//! Get faction filter flags for this zone (lower tiers = civilian only)
	//! Returns DatZLootFaction flags
	int GetFactionFilterForZone()
	{
		switch (zoneType)
		{
			// TIER1 - Coastal: ONLY civilian/FIA items (NO military)
			case DatZLootZoneType.TIER1_COASTAL:
				return DatZLootFaction.TIER_LOW;

			// TIER2 - Residential: Civilian/FIA items, tiny chance of US (pistols only)
			case DatZLootZoneType.TIER2_RESIDENTIAL:
				return DatZLootFaction.TIER_LOW;

			// TIER3 - Industrial: Civilian/FIA + some US gear starts appearing
			case DatZLootZoneType.TIER3_INDUSTRIAL:
				return DatZLootFaction.TIER_MID;

			// TIER4 - Medical: Mostly civilian medical, some US medical
			case DatZLootZoneType.TIER4_MEDICAL:
				return DatZLootFaction.TIER_MID;

			// TIER4 - Police: US gear (pistols, vests) - more military presence
			case DatZLootZoneType.TIER4_POLICE:
				return DatZLootFaction.TIER_MID;

			// TIER5 - Military: Full military from all factions
			case DatZLootZoneType.TIER5_MILITARY:
				return DatZLootFaction.TIER_HIGH;

			// TIER5 - Airfield: Best loot - all factions
			case DatZLootZoneType.TIER5_AIRFIELD:
				return DatZLootFaction.TIER_HIGH;

			// Custom/Special zones: all factions
			default:
				return DatZLootFaction.ALL;
		}
		return DatZLootFaction.ALL;
	}

	//------------------------------------------------------------------------------------------------
	//! Get empty loot chance for this zone (higher = more empty spawners)
	float GetEmptyChanceForZone()
	{
		switch (zoneType)
		{
			case DatZLootZoneType.TIER1_COASTAL:
				return 150;  // Very high - mostly empty
			case DatZLootZoneType.TIER2_RESIDENTIAL:
				return 100;  // High
			case DatZLootZoneType.TIER3_INDUSTRIAL:
				return 80;   // Medium-high
			case DatZLootZoneType.TIER4_MEDICAL:
				return 60;   // Medium (medical is valuable)
			case DatZLootZoneType.TIER4_POLICE:
				return 70;   // Medium
			case DatZLootZoneType.TIER5_MILITARY:
				return 120;  // High (military loot is rare)
			case DatZLootZoneType.TIER5_AIRFIELD:
				return 130;  // Very high (best loot is hardest to find)
			case DatZLootZoneType.CUSTOM:
				return customEmptyChance;
			default:
				return 50;
		}
		return 50;
	}

	//------------------------------------------------------------------------------------------------
	//! Get the legacy loot table path for special zones
	ResourceName GetLootTableForZone()
	{
		switch (zoneType)
		{
			case DatZLootZoneType.FOOD:
				return "{5807869ECEA41A94}Configs/FoodLootTable.conf";
			case DatZLootZoneType.FARM:
				return "{416671DC683FEF01}Configs/CivilianFarmLootTable.conf";
			case DatZLootZoneType.BOAT:
				return "{9547060D1887C5D1}Configs/BoatLootTable.conf";
			case DatZLootZoneType.COOKING:
				return "{71A61DD370162A5D}Configs/CookingLootTable.conf";
			case DatZLootZoneType.VALUABLES:
				return "{7AB0822EB80E5C46}Configs/ValuablesLootTable.conf";
			case DatZLootZoneType.DRUGS:
				return "{861457BC841542EB}Configs/DrugLootTable.conf";
			case DatZLootZoneType.KEYCARD:
				return "{BEF291E3D0C06B02}Configs/keyCardLootTable.conf";
			case DatZLootZoneType.CUSTOM:
				return customLootTable;
			default:
				return "";
		}
		return "";
	}

	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		GetGame().GetCallqueue().CallLater(SetOverride, delay: 25000);
	}

	//------------------------------------------------------------------------------------------------
	void SetOverride()
	{
		Print("[DatZLootOverride] Init Override - Zone Type: " + typename.EnumToString(DatZLootZoneType, zoneType));

		DZLootSystem system = DZLootSystem.GetInstance();
		if (!system)
			return;

		int typeFilter = GetTypeFilterForZone();

		// Use TW_LootManager type filter system for tiered zones
		if (typeFilter >= 0)
		{
			float emptyChance = GetEmptyChanceForZone();
			int factionFilter = GetFactionFilterForZone();
			Print(string.Format("[DatZLootOverride] Setting type filter: %1, faction: %2, empty: %3", typeFilter, factionFilter, emptyChance));
			system.SetLootTypeFilterForSpawners(typeFilter, emptyChance, GetOrigin(), overrideRadius, chance, factionFilter);
		}
		// Use legacy config file system for special zones
		else
		{
			ResourceName lootTable = GetLootTableForZone();
			if (lootTable.IsEmpty())
			{
				Print("[DatZLootOverride] No loot table configured for zone!", LogLevel.WARNING);
				return;
			}
			Print(string.Format("[DatZLootOverride] Setting loot table: %1", lootTable));
			system.SetLootOverrideSpawners(lootTable, GetOrigin(), overrideRadius, chance);
		}

		// Spawn map marker for zone visualization
		if (showOnMap)
			SpawnMapMarker();
	}

	//------------------------------------------------------------------------------------------------
	protected void SpawnMapMarker()
	{
		if (m_MapMarker)
			return;

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		GetTransform(params.Transform);

		// Spawn zone marker entity
		Resource res = Resource.Load("{E7C8A9B0D1F23456}Prefabs/Markers/DatZZoneMarker.et");
		if (!res || !res.IsValid())
		{
			Print("[DatZLootOverride] Zone marker prefab not found, skipping map marker", LogLevel.WARNING);
			return;
		}

		m_MapMarker = DatZMapZoneMarker.Cast(GetGame().SpawnEntityPrefab(res, GetGame().GetWorld(), params));
		if (!m_MapMarker)
			return;

		// Configure marker
		m_MapMarker.m_fRadius = overrideRadius;
		m_MapMarker.m_sZoneLabel = GetZoneLabel();
		m_MapMarker.m_CircleColor = GetZoneMapColor();
	}

	//------------------------------------------------------------------------------------------------
	protected string GetZoneLabel()
	{
		switch (zoneType)
		{
			case DatZLootZoneType.TIER1_COASTAL:     return "Coastal";
			case DatZLootZoneType.TIER2_RESIDENTIAL:  return "Residential";
			case DatZLootZoneType.TIER3_INDUSTRIAL:   return "Industrial";
			case DatZLootZoneType.TIER4_MEDICAL:      return "Medical";
			case DatZLootZoneType.TIER4_POLICE:       return "Police";
			case DatZLootZoneType.TIER5_MILITARY:     return "Military";
			case DatZLootZoneType.TIER5_AIRFIELD:     return "Airfield";
			case DatZLootZoneType.FOOD:               return "Food";
			case DatZLootZoneType.FARM:               return "Farm";
			case DatZLootZoneType.BOAT:               return "Boat";
			case DatZLootZoneType.COOKING:            return "Cooking";
			case DatZLootZoneType.VALUABLES:          return "Valuables";
			case DatZLootZoneType.DRUGS:              return "Drugs";
			case DatZLootZoneType.KEYCARD:            return "Keycard";
			default:                                  return "Zone";
		}
		return "Zone";
	}

	//------------------------------------------------------------------------------------------------
	protected Color GetZoneMapColor()
	{
		switch (zoneType)
		{
			case DatZLootZoneType.TIER1_COASTAL:      return new Color(0.39, 0.78, 1.0, 1.0);   // Light blue
			case DatZLootZoneType.TIER2_RESIDENTIAL:   return new Color(0.39, 1.0, 0.39, 1.0);   // Green
			case DatZLootZoneType.TIER3_INDUSTRIAL:    return new Color(1.0, 0.65, 0.0, 1.0);    // Orange
			case DatZLootZoneType.TIER4_MEDICAL:       return new Color(1.0, 1.0, 1.0, 1.0);     // White
			case DatZLootZoneType.TIER4_POLICE:        return new Color(0.0, 0.39, 1.0, 1.0);    // Blue
			case DatZLootZoneType.TIER5_MILITARY:      return new Color(1.0, 0.2, 0.2, 1.0);     // Red
			case DatZLootZoneType.TIER5_AIRFIELD:      return new Color(1.0, 0.2, 0.2, 1.0);     // Red
			case DatZLootZoneType.FOOD:                return new Color(1.0, 0.78, 0.39, 1.0);   // Yellow
			case DatZLootZoneType.FARM:                return new Color(0.6, 0.8, 0.2, 1.0);     // Lime
			case DatZLootZoneType.VALUABLES:           return new Color(1.0, 0.84, 0.0, 1.0);    // Gold
			case DatZLootZoneType.DRUGS:               return new Color(0.7, 0.0, 1.0, 1.0);     // Purple
			case DatZLootZoneType.KEYCARD:             return new Color(1.0, 0.39, 0.78, 1.0);   // Pink
			default:                                   return new Color(0.0, 1.0, 0.0, 1.0);     // Green
		}
		return new Color(0.0, 1.0, 0.0, 1.0);
	}

	//------------------------------------------------------------------------------------------------
	override void EOnFixedFrame(IEntity owner, float timeSlice)
	{
	}

#ifdef WORKBENCH
	protected ref Shape m_DebugSphere;

	//------------------------------------------------------------------------------------------------
	protected int GetZoneColor()
	{
		switch (zoneType)
		{
			case DatZLootZoneType.TIER1_COASTAL:
				return ARGB(40, 100, 200, 255); // Light blue
			case DatZLootZoneType.TIER2_RESIDENTIAL:
				return ARGB(40, 100, 255, 100); // Green
			case DatZLootZoneType.TIER3_INDUSTRIAL:
				return ARGB(40, 255, 165, 0);   // Orange
			case DatZLootZoneType.TIER4_MEDICAL:
				return ARGB(40, 255, 255, 255); // White
			case DatZLootZoneType.TIER4_POLICE:
				return ARGB(40, 0, 100, 255);   // Blue
			case DatZLootZoneType.TIER5_MILITARY:
			case DatZLootZoneType.TIER5_AIRFIELD:
				return ARGB(40, 255, 50, 50);   // Red
			case DatZLootZoneType.FOOD:
			case DatZLootZoneType.FARM:
			case DatZLootZoneType.COOKING:
				return ARGB(40, 255, 200, 100); // Yellow/tan
			case DatZLootZoneType.VALUABLES:
				return ARGB(40, 255, 215, 0);   // Gold
			case DatZLootZoneType.DRUGS:
				return ARGB(40, 180, 0, 255);   // Purple
			case DatZLootZoneType.KEYCARD:
				return ARGB(40, 255, 100, 200);  // Pink
			default:
				return ARGB(40, 0, 255, 0);     // Green
		}
		return ARGB(40, 0, 255, 0);
	}

	//------------------------------------------------------------------------------------------------
	override void _WB_OnInit(inout vector mat[4], IEntitySource src)
	{
		m_DebugSphere = Shape.CreateSphere(GetZoneColor(), ShapeFlags.VISIBLE | ShapeFlags.TRANSP | ShapeFlags.NOOUTLINE | ShapeFlags.DOUBLESIDE, GetOrigin(), overrideRadius);
	}

	//------------------------------------------------------------------------------------------------
	override void _WB_AfterWorldUpdate(float timeSlice)
	{
		// Redraw sphere if entity was moved or settings changed
		if (m_DebugSphere)
			m_DebugSphere = null;

		m_DebugSphere = Shape.CreateSphere(GetZoneColor(), ShapeFlags.VISIBLE | ShapeFlags.TRANSP | ShapeFlags.NOOUTLINE | ShapeFlags.DOUBLESIDE, GetOrigin(), overrideRadius);
	}
#endif
}
