class FirePlaceStorageComponentClass : SCR_UniversalInventoryStorageComponentClass
{
}

class FirePlaceStorageComponent : SCR_UniversalInventoryStorageComponent
{
	
	SCR_FireplaceComponent firePlace;	
	[RplProp()]
	float m_currentFuel;
	protected InventoryStorageManagerComponent m_StorageManager;
	CookingContainerStorageComponent cookingPot;
	int cookingPotCount;
	
	[Attribute()]
	ref PointInfo potPlacementInfo;
	//------------------------------------------------------------------------------------------------
	event override bool CanStoreItem(IEntity item, int slotID)
	{
		if (!super.CanStoreItem(item, slotID))
			return false;

		// if not fire fuel and not cooknig container, return false
		FireFuelComponent fireF = FireFuelComponent.Cast(item.FindComponent(FireFuelComponent));
		if(!fireF)
		{
			CookingContainerStorageComponent cookingCont = CookingContainerStorageComponent.Cast(item.FindComponent(CookingContainerStorageComponent));
			if(!cookingCont)
			return false;
			if(cookingPotCount>=1)
			return false;
			
			

		}
		InventoryItemComponent itemComp = InventoryItemComponent.Cast(item.FindComponent(InventoryItemComponent));
			if (itemComp)
  			  itemComp.SetTraceable(true);
		
		
		
		return true;
	}
	//------------------------------------------------------------------------------------------------
	protected override void OnAddedToSlot(IEntity item, int slotID)
	{
		super.OnAddedToSlot(item, slotID);
		GenericEntity pGenComp = GenericEntity.Cast( item );
		InventoryItemComponent pItemComponent = InventoryItemComponent.Cast(pGenComp.FindComponent(InventoryItemComponent));
		CookingContainerStorageComponent cookingCont = CookingContainerStorageComponent.Cast(item.FindComponent(CookingContainerStorageComponent));
		if(cookingCont){
			this.cookingPot = cookingCont;
			cookingPotCount++;
			cookingCont.ShowOwner();
			vector mat[4];
			potPlacementInfo.GetLocalTransform(mat);
			item.SetLocalTransform(mat);
			item.Update();
			GetGame().GetCallqueue().CallLater(ShowDelay,delay:200);
			//pItemComponent.EnablePhysics();
			//pItemComponent.ActivateOwner(true);
		}
		
		
	}
	void ShowDelay()
	{
		if(cookingPot)
		cookingPot.ShowOwner();
	
	}
	protected override void OnRemovedFromSlot(IEntity item, int slotID)
	{
		super.OnAddedToSlot(item, slotID);
		CookingContainerStorageComponent cookingCont = CookingContainerStorageComponent.Cast(item.FindComponent(CookingContainerStorageComponent));
		if(cookingCont)
			cookingPotCount--;
		InventoryItemComponent itemComp = InventoryItemComponent.Cast(item.FindComponent(InventoryItemComponent));
			if (itemComp)
  			  itemComp.SetTraceable(false);
		
	}
	
	void UpdateFire(float timeSlice)
	{
		if(firePlace == null)
			firePlace = SCR_FireplaceComponent.Cast(GetOwner().FindComponent(SCR_FireplaceComponent));

		if(firePlace == null)return;
		
		if(!firePlace.IsOn()) return;
		UpdateCook(timeSlice);
		m_currentFuel -= 0.03;
		m_currentFuel = Math.Clamp(m_currentFuel,0,100);	
		UpdateUI();
		if(m_currentFuel != 0) 
		{
			Replication.BumpMe();
			return;
		}
		
		array<IEntity> Items();
		int count = GetAll(Items);
		array<FireFuelComponent> fuelItems();
		
		
		for (int i = Items.Count() - 1; i >= 0; i--)
		{
    FireFuelComponent fireF = FireFuelComponent.Cast(Items[i].FindComponent(FireFuelComponent));
    if (fireF)
        fuelItems.Insert(fireF);
		}

		if(fuelItems.Count()<1)
		{
			firePlace.ToggleLight(false);
			Rpc(RPCFireOff);
			Replication.BumpMe();
		 	return;
		}
		m_currentFuel += fuelItems[0].fuelValue;
		
		
		RplComponent.DeleteRplEntity(fuelItems[0].GetOwner(), false);
		Rpc( RPCUpdateUI);
		Replication.BumpMe();
	
	}
	[RplRpc(RplChannel.Reliable,RplRcver.Broadcast)]
	void RPCUpdateUI(){
			
		UpdateUI();
	
	}
	[RplRpc(RplChannel.Reliable,RplRcver.Broadcast)]
	void RPCFireOff(){
			if(firePlace == null)
			firePlace = SCR_FireplaceComponent.Cast(GetOwner().FindComponent(SCR_FireplaceComponent));
			firePlace.ToggleLight(false);
	
	}
	void UpdateCook(float timeSlice)
	{

		array<IEntity> Items();
		int count = GetAll(Items);
		
		if(count<1)
		{

		 	return;
		}
		foreach(IEntity item : Items){
		
			CookingContainerStorageComponent cookingCon =CookingContainerStorageComponent.Cast(item.FindComponent(CookingContainerStorageComponent));
			if(cookingCon)
			cookingCon.Update(timeSlice);
		}
	
	}
	
	bool canTurnOn()
	{
		array<IEntity> fuelItems();
		int count = GetAll(fuelItems);
		for(int i = fuelItems.Count() - 1; i >= 0; i--)
		{
			FireFuelComponent fireF = FireFuelComponent.Cast(fuelItems[i].FindComponent(FireFuelComponent));
			if(!fireF)
			fuelItems.Remove(i);
		}
		if(m_currentFuel==0&&fuelItems.Count()<1)
		return false;
		
		return true;
	
	}
	override void OnDelete(IEntity owner){
		if(DZFirePlaceSystem.GetInstance())
		DZFirePlaceSystem.GetInstance().Unregister(this);
	}
	

	//------------------------------------------------------------------------------------------------
	protected override void OnManagerChanged(InventoryStorageManagerComponent manager)
	{
		super.OnManagerChanged(manager);
		m_StorageManager = manager;
	}
}
