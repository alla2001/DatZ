class DatZLootSpawnerClass : SCR_PositionClass
{


}


class DatZLootSpawner : GenericEntity
{
	

    [Attribute()]
    float spawnRadius;

    float despawnDelay;
	[Attribute("", UIWidgets.ResourceNamePicker, desc: "", params: "conf")]
    ResourceName lootTable;
    ref array<IEntity> currentLoot = new array<IEntity>();
	ref LootTable m_lootTable;
	
	float lastPlayerTime = 0;
	float lastSpawnTime= 0;
	bool spawned;
	
	[Attribute("false")]
	bool ignoreOverrides;
	override void EOnInit(IEntity owner){
	

		GetGame().GetCallqueue().CallLater(InitLootTable,delay:1000);
		SCR_EntityHelper.SnapToGround(owner,startOffset: "0 0.5 0",maxLength:30);
	}
	
	void InitLootTable(){
	
		
		m_lootTable = new LootTable();
	   if (lootTable.IsEmpty())
			return ;

		Resource resource = Resource.Load(lootTable);
		if (!resource.IsValid())
			return ;

		m_lootTable =  LootTable.Cast(BaseContainerTools.CreateInstanceFromContainer(resource.GetResource().ToBaseContainer()));
		
	}
	void SetDespawnDelay(float delay){
	
    	despawnDelay = delay;
	
	}
    void Process()
    {
		
        bool playerNearby = IsPlayerInRange();
		if(playerNearby)
		lastPlayerTime = GetGame().GetWorld().GetWorldTime();
		 if (!spawned && playerNearby)
                SpawnLoot();
    	if (spawned&& !playerNearby && GetGame().GetWorld().GetWorldTime()>=lastPlayerTime+despawnDelay*1000.0)
                DespawnLoot();
		if (spawned&& GetGame().GetWorld().GetWorldTime()>=lastPlayerTime+despawnDelay*1000.0)
                DespawnLoot();
    }

    bool IsPlayerInRange()
    {
		array<int> allPlayers = {};
        GetGame().GetPlayerManager().GetAllPlayers(allPlayers);
		//filter all current players and store them
		foreach (int player : allPlayers)
		{
			IEntity ce =  GetGame().GetPlayerManager().GetPlayerControlledEntity(player);
			if(!ce)continue;
			
			if(vector.Distance(GetOrigin(),ce.GetOrigin())<spawnRadius)
			return true;
			
		}
        return false;
    }

    void SpawnLoot()
    {
		InitLootTable();
		LootItem loot =  m_lootTable.GetRandomLoot();
		lastSpawnTime = GetGame().GetWorld().GetWorldTime();
		while(loot!=null)
		{
		
        	string prefabPath = loot.prefabPath;
        	if (prefabPath != string.Empty)
        	{
            	Resource resource = Resource.Load(prefabPath);
				if(!resource.IsValid()){
				
					loot = null;
					continue;
				
				}
				EntitySpawnParams param =  new EntitySpawnParams();
				GetWorldTransform(param.Transform);
            	if (resource)
				{
					IEntity ent = GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), param);
					if(ent ==null) continue;
					currentLoot.Insert(ent);
					SCR_EntityHelper.SnapToGround(ent,startOffset: "0 0.4 0",maxLength:30);
					vector pos = ent.GetOrigin();
					InventoryItemComponent invItem = InventoryItemComponent.Cast( ent.FindComponent(InventoryItemComponent));
					
					if(invItem)
					{
						invItem.m_OnParentSlotChangedInvoker.Insert(OnParentSlotChanged);
						invItem.PlaceOnGround();
					}
						
					ent.SetOrigin(pos + "0 0.05 0");
					ent.Update();
					SCR_2DPIPSightsComponent sight = SCR_2DPIPSightsComponent.Cast( ent.FindComponent(SCR_2DPIPSightsComponent));
					if(sight){
					
						sight.SetBroken( Math.RandomInt(0,2) == 1);
					}
					
					BaseMagazineComponent mag = BaseMagazineComponent.Cast( ent.FindComponent(BaseMagazineComponent));
					BaseWeaponComponent wpn = BaseWeaponComponent.Cast( ent.FindComponent(BaseWeaponComponent));
					if(wpn){
						mag = wpn.GetCurrentMagazine();
						if(mag)
						{
					
							mag.SetAmmoCount(Math.RandomInt(5,mag.GetMaxAmmoCount()));
						}
						sight =SCR_2DPIPSightsComponent.Cast(  wpn.GetAttachedSights());
						if(sight)
						{
					
							sight.SetBroken( Math.RandomInt(0,2) == 1);
						}
					
					}
					
					
					
					FluidContainerComponent fluid = FluidContainerComponent.Cast( ent.FindComponent(FluidContainerComponent));
					if(fluid){
					
								fluid.m_fCurrentAmount = Math.RandomFloat(0,fluid.m_fMaxCapacity);
					}
					SCR_FuelManagerComponent fuel = SCR_FuelManagerComponent.Cast( ent.FindComponent(SCR_FuelManagerComponent));
							
					if(fuel)
					{
						array<SCR_FuelNode> outScriptedNodes();
						fuel.GetScriptedFuelNodesList(outScriptedNodes);
						outScriptedNodes[0].SetFuel(Math.RandomFloat(0,outScriptedNodes[0].GetMaxFuel()));
					}
						
				}
               
        	}
			
			if(loot.relatedLootItems)
			loot =  loot.relatedLootItems.GetRandomLoot();
			else
			loot = null;
		}
		spawned=true;
    }
	void OnParentSlotChanged(InventoryStorageSlot oldSlot, InventoryStorageSlot newSlot)
    {
		foreach(IEntity loot : currentLoot)
		{
		 	InventoryItemComponent invItem = InventoryItemComponent.Cast( loot.FindComponent(InventoryItemComponent));
			if(invItem)
			{
				invItem.m_OnParentSlotChangedInvoker.Remove(OnParentSlotChanged);

			}
		}
			currentLoot.Clear();
	
    }
    void DespawnLoot()
    {
		spawned=false;
		foreach(IEntity loot : currentLoot)
		{
		 	RplComponent.DeleteRplEntity(loot, false);
		}
		currentLoot.Clear();

    }

	//------------------------------------------------------------------------------------------------
	void DatZLootSpawner(IEntitySource src, IEntity parent)
	{
		SetEventMask(EntityEvent.FIXEDFRAME);
			SetEventMask(EntityEvent.INIT);
		if(DZLootSystem.GetInstance())
		DZLootSystem.GetInstance().Register(this);
	}

	//------------------------------------------------------------------------------------------------
	void ~DatZLootSpawner()
	{
			if(DZLootSystem.GetInstance())
		DZLootSystem.GetInstance().UnRegister(this);

		
	}
	

}