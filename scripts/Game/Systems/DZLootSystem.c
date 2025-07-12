class DZLootSystem : GameSystem
{

	[Attribute()]
	float tickTime;
	[Attribute("1.0")]
    float despawnDelay;
	[Attribute()]
    ResourceName defaultLootSpawner;
		
	ref array<DatZLootSpawner> spawners = new array<DatZLootSpawner>();
	
	override void OnInit()
	{
	
		GetGame().GetCallqueue().CallLater(ProcessSpawners, 20000, false);
		
		//GetGame().SpawnEntityPrefab(Resource.Load(missions));
		
	}
	void ProcessSpawners()
	{
		GetGame().GetCallqueue().CallLater(ProcessSpawners, tickTime, false);
		foreach(DatZLootSpawner spawner : spawners)
		{
			spawner.SetDespawnDelay(despawnDelay);
			spawner.Process();
		}

	}
	// chance 0 - 1 
	void SetLootOverrideSpawners(ResourceName loottable,vector origin , float raduis, float chance)
	{
		foreach(DatZLootSpawner spawner : spawners)
		{
			
			if(vector.Distance( spawner.GetOrigin(),origin)<=raduis)
			{
				if(!spawner || spawner.ignoreOverrides || Math.RandomFloat(0,1)>chance)continue;
				spawner.lootTable = loottable;
				spawner.InitLootTable();
			}
				
		
		
		}
	

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
	//! \param component must not be null
	void Register(DatZLootSpawner component)
	{
		if(!spawners.Contains(component))
		spawners.Insert(component);
	}	
	
	void UnRegister(DatZLootSpawner component)
	{
		if(spawners.Contains(component))
		spawners.RemoveItem(component);
	}	
	
	//------------------------------------------------------------------------------------------------
	void Unregister(DatZLootSpawner component)
	{
		if(spawners.Contains(component))
		spawners.RemoveItem(component);
	}
}
