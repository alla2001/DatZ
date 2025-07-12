

modded class SCR_GadgetManagerComponent : ScriptGameComponent
{
	

	//------------------------------------------------------------------------------------------------
	// EVENTS
	//------------------------------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	//! SCR_InventoryStorageManagerComponent, called on every add to slot, not only to inventory
	//! \param[in] item
	//! \param[in] storageOwner
	override void OnItemAdded(IEntity item, BaseInventoryStorageComponent storageOwner)
	{
		SCR_GadgetComponent gadgetComp = SCR_GadgetComponent.Cast(item.FindComponent(SCR_GadgetComponent));
		if (!gadgetComp)
			return;

		EGadgetType type = gadgetComp.GetType();

		int gadgetArrayID = m_aGadgetArrayMap.Get(type);
		if (gadgetArrayID == -1)
			return;

		m_aInventoryGadgetTypes[gadgetArrayID].Insert(gadgetComp);

		// Check whether the gadget is in equipment slot (OnAddedToSlot will happen before this)
		EquipmentStorageComponent equipmentComp = EquipmentStorageComponent.Cast(storageOwner);
		if (equipmentComp)
		{
			InventoryStorageSlot storageSlot = equipmentComp.FindItemSlot(item);
			if (storageSlot)
				SetGadgetMode(item, EGadgetMode.IN_SLOT);
		}
		else if (type == EGadgetType.RADIO_BACKPACK) 	// special case for backpack radios, always in (character) slot
			SetGadgetMode(item, EGadgetMode.IN_SLOT);
		else
			SetGadgetMode(item, EGadgetMode.IN_STORAGE);

		m_OnGadgetAdded.Invoke(gadgetComp);

		if (m_VONController && (type & m_iRadiosFlags))	// add entries to VONController
		{
			SCR_RadioComponent radioGadget = SCR_RadioComponent.Cast(gadgetComp);
			if (!radioGadget)
				return;

			BaseRadioComponent radioComp = radioGadget.GetRadioComponent();
			if (!radioComp)
				return;

			// Put all transceivers (AKA) channels in the VoN menu
			int count = radioComp.TransceiversCount();
			for (int i = 0; i < count; i++)
			{
				SCR_VONEntryRadio radioEntry = new SCR_VONEntryRadio();
				radioEntry.SetRadioEntry(radioComp.GetTransceiver(i), i + 1, gadgetComp);
				m_VONController.AddEntry(radioEntry);
			}
		}
			SCR_SupportStationGadgetComponent spgadgetComp = SCR_SupportStationGadgetComponent.Cast(gadgetComp);
			if (spgadgetComp)
			{
		
				InventoryStorageSlot storageSlot = storageOwner.FindItemSlot(item);
				if (storageSlot)
				{
					SCR_HandSlotStorageSlot str = SCR_HandSlotStorageSlot.Cast(storageSlot);
					if(str)
					SetGadgetMode(item, EGadgetMode.IN_HAND);
				}
					

			
			}
		
	}

	

}
