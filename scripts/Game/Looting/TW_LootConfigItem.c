/*
	This is intended to be used as a data structure for serializing and deserializing
	TrainWreck loot tables
*/

// Faction flags for tier-based loot filtering
// Lower tiers = only civilian/FIA, higher tiers unlock military factions
class DatZLootFaction
{
	static const int NONE = 0;
	static const int CIVILIAN = 1;   // Pure civilian items
	static const int FIA = 2;        // Resistance/guerilla gear
	static const int US = 4;         // US Army military
	static const int USSR = 8;       // Soviet military

	// Preset combinations for tiers
	static const int TIER_LOW = CIVILIAN | FIA;                           // Tiers 1-2: civilian only
	static const int TIER_MID = CIVILIAN | FIA | US;                      // Tier 3-4: add some US
	static const int TIER_HIGH = CIVILIAN | FIA | US | USSR;              // Tier 5: all factions
	static const int ALL = CIVILIAN | FIA | US | USSR;
}

class TW_LootConfigItem
{
	string resourceName;
	int chanceToSpawn;
	int randomSpawnCount;
	bool isEnabled;
	ref array<string> tags;
	int factionFlags;  // DatZLootFaction flags

	void SetData(ResourceName resource, int chance, int spawnCount, array<string> itemTags = null, bool isEnabled = true, int faction = 0)
	{
		this.resourceName = resource;
		this.chanceToSpawn = chance;
		this.randomSpawnCount = spawnCount;
		this.isEnabled = isEnabled;
		this.factionFlags = faction;

		if(itemTags)
			this.tags = itemTags;
		else
			this.tags = {};
	}
};