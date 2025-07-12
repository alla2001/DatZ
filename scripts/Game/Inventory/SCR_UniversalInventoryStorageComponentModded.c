
//! Current storage variant allows dynamic scaling of slots and handles Move/Insert/Remove operations
//! it will accept any entity for insertion and will remove/add it's visibility flag when inserted/removed from storage
//! \see CharacterInventoryStorageComponent for example of custom storage inheritance from current class
modded class SCR_UniversalInventoryStorageComponent : UniversalInventoryStorageComponent
{
	
	//------------------------------------------------------------------------------------------------
	protected override void OnAddedToSlot(IEntity item, int slotID)
	{
		super.OnAddedToSlot(item, slotID);
		
		GenericEntity pGenComp = GenericEntity.Cast( item );
		InventoryItemComponent pItemComponent = InventoryItemComponent.Cast(pGenComp.FindComponent(InventoryItemComponent));
		if( !pItemComponent )
			return;	
	
		float fVol = pItemComponent.GetTotalVolume();
		if ( m_aSlotsToShow.Find( slotID ) != -1 )
		{
				pItemComponent.ShowOwner();
		}
		else
		{
			if ( fVol >= MIN_VOLUME_TO_SHOW_ITEM_IN_SLOT )
				pItemComponent.ShowOwner();
		}

		pItemComponent.DisablePhysics();
		pItemComponent.ActivateOwner(false);
		
		m_fWeight += pItemComponent.GetTotalWeight();
		
		SCR_ItemAttributeCollection refundItemAttributes = SCR_ItemAttributeCollection.Cast(pItemComponent.GetAttributes());
		if (refundItemAttributes && !refundItemAttributes.IsRefundable())
			m_iNrOfNonRefundableItems++;
		
		
	}
	
	void DropAllItems(){
	
		array<IEntity> items();
		GetAll(items);
		
		foreach(IEntity item : items)
		{
			 RemoveItem(item);
		}

	
	}

}
