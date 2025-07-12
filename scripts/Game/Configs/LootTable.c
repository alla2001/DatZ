[BaseContainerProps(configRoot: true),SCR_BaseContainerCustomTitleResourceName("prefabPath")]
class LootItem 
{
	[Attribute(uiwidget: UIWidgets.ResourcePickerThumbnail, params: "et")]
    ResourceName prefabPath;
	[Attribute()]
	ref LootTable relatedLootItems;
	[Attribute()]
    float spawnChance;

  
}

[BaseContainerProps(configRoot: true)]
class LootTable : ScriptAndConfig
{
	
		[Attribute("0.0", UIWidgets.Slider, "Chance to drop nothing", "0 2000 0.01")]
	float emptyLootChance;
	
	[Attribute()]
    ref array<ref LootItem> items;



    LootItem GetRandomLoot()
    {
        float totalChance = 0;

        foreach (LootItem item : items)
        {
            totalChance += item.spawnChance;
        }

        // Account for empty chance
        float roll = Math.RandomFloat(0, totalChance + emptyLootChance);

        float cumulative = 0;

        foreach (LootItem item : items)
        {
            cumulative += item.spawnChance;
            if (roll <= cumulative)
                return item;
        }

        // If it falls outside cumulative item chances, return null for no loot
        return null;
    }
}
