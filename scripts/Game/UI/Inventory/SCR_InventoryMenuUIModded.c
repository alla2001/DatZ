
//! UI Script
//! Inventory Menu UI Layout
modded class SCR_InventoryMenuUI : ChimeraMenuBase
{	

	//------------------------------------------------------------------------------------------------
	//!
	override protected void Action_UnfoldItem()
	{
 		SimpleFSM( EMenuAction.ACTION_OPENCONTAINER );
	}
		//------------------------------------------------------------------------------------------------
	override protected void SetFocusedSlotEffects()
	{
		if( !m_pFocusedSlotUI )
			return;

		//show info about the item
		InventoryItemComponent invItemComp = m_pFocusedSlotUI.GetInventoryItemComponent();
		if ( !invItemComp )
			return;
		auto attribs = SCR_ItemAttributeCollection.Cast( invItemComp.GetAttributes() );

		if ( !attribs )
			return;

		UIInfo itemInfo = attribs.GetUIInfo();
		string name  = "";
		if(invItemComp){
		CookableItemComponent cookable = CookableItemComponent.Cast(invItemComp.GetOwner().FindComponent(CookableItemComponent) );
		if(cookable)
		{
				switch(cookable.GetCookingState())
				{
					case ECookState.RAW:
					name = name + " (Raw)";
					break;
					case ECookState.PERFECT:
					name = name + " (Cooked)";
					break;
					case ECookState.BURNT:
					name = name + " (Burnt)";
					break;
				}

		}
		}
		if ( !itemInfo )
			HideItemInfo();
		else
		{
			SCR_InventoryUIInfo inventoryInfo = SCR_InventoryUIInfo.Cast(itemInfo);
			
			if (inventoryInfo)
				ShowItemInfo( inventoryInfo.GetInventoryItemName(invItemComp) + name, inventoryInfo.GetInventoryItemDescription(invItemComp), invItemComp.GetTotalWeight(), inventoryInfo);
			else 
				ShowItemInfo( itemInfo.GetName() + name, itemInfo.GetDescription(), invItemComp.GetTotalWeight(), null);
		}

		NavigationBarUpdate();
	}

}
