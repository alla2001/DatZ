//------------------------------------------------------------------------------------------------
// MODDED ARSENAL ITEM - Adds option to exclude items from loot spawning
//------------------------------------------------------------------------------------------------

modded class SCR_ArsenalItem : SCR_BaseEntityCatalogData
{
	[Attribute("true", UIWidgets.CheckBox, "Include this item in loot spawning (DatZ/TW loot system)")]
	protected bool m_bIncludeInLootSpawning;

	//------------------------------------------------------------------------------------------------
	// Check if this item should be included in loot spawning
	bool ShouldIncludeInLootSpawning()
	{
		return m_bIncludeInLootSpawning;
	}

	//------------------------------------------------------------------------------------------------
	// Set whether this item should be included in loot spawning
	void SetIncludeInLootSpawning(bool include)
	{
		m_bIncludeInLootSpawning = include;
	}
}
