//------------------------------------------------------------------------------------------------
//! modded version for to be used with the inventory 2.0 
class SCR_OpenPotStorageAction : SCR_InventoryAction
{
	#ifndef DISABLE_INVENTORY
	//------------------------------------------------------------------------------------------------
	override protected void PerformActionInternal(SCR_InventoryStorageManagerComponent manager, IEntity pOwnerEntity, IEntity pUserEntity)
	{
		auto vicinity = CharacterVicinityComponent.Cast( pUserEntity .FindComponent( CharacterVicinityComponent ));
		if ( !vicinity )
			return;
		
		manager.SetStorageToOpen(pOwnerEntity);
		manager.OpenInventory();
	}
	
	#endif
	/*
	//------------------------------------------------------------------------------------------------
	override bool CanBeShownScript(IEntity user)
	{
		if (m_Item == null  || m_Item.IsLocked())
			return false;
		
		IEntity itemEntity = m_Item.GetOwner();
		BaseMagazineComponent baseMagComponent = BaseMagazineComponent.Cast(itemEntity.FindComponent(BaseMagazineComponent));
		if (baseMagComponent)
		{
			if (baseMagComponent.IsUsed())
				return false;
			
			// This is a temporary fix for issue #18454,
			// we should lock items during reload instead
			IEntity itemParent = itemEntity.GetParent();
			if (ChimeraCharacter.Cast(itemParent))
				return false;
		}
		
		return true;
	}	*/
	
};