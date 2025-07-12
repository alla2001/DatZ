class CookingContainerStorageComponentClass : SCR_UniversalInventoryStorageComponentClass
{
}

class CookingContainerStorageComponent : SCR_UniversalInventoryStorageComponent
{
	


	[Attribute("1.0")]
	protected float m_fCookRate; // Cooking rate multiplier

	

	void Update(float timeSlice)
	{

		array<IEntity> items = {};
		GetAll(items);
		
		foreach (IEntity item : items)
		{
			CookableItemComponent cookable = CookableItemComponent.Cast(item.FindComponent(CookableItemComponent));
			if (cookable)
			{
				cookable.AddCookTime(timeSlice * m_fCookRate);
			}
		}
		RPCUpdateUI();
		Rpc( RPCUpdateUI);
		
	}
	[RplRpc(RplChannel.Reliable,RplRcver.Broadcast)]
	void RPCUpdateUI(){
			
		UpdateUI();
	
	}
	event override bool CanStoreItem(IEntity item, int slotID)
	{
		if (!super.CanStoreItem(item, slotID))
			return false;
		CookableItemComponent cookable = CookableItemComponent.Cast(item.FindComponent(CookableItemComponent));
		if (!cookable)
			return false;
		InventoryItemComponent itemComp = InventoryItemComponent.Cast(item.FindComponent(InventoryItemComponent));
			if (itemComp)
  			  itemComp.SetTraceable(true);
		return true;
	}
	protected override void OnRemovedFromSlot(IEntity item, int slotID)
	{
		super.OnAddedToSlot(item, slotID);
		
		InventoryItemComponent itemComp = InventoryItemComponent.Cast(item.FindComponent(InventoryItemComponent));
			if (itemComp)
  			  itemComp.SetTraceable(false);
		
	}



}
