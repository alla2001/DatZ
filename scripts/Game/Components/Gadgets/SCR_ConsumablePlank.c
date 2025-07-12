class SCR_ConsumablePlankArea : LoadoutAreaType
{
}

//! Saline bag effect
[BaseContainerProps()]
class SCR_ConsumablePlank : SCR_ConsumableEffectHealthItems
{
	
	SCR_PlankGadgetComponent plankGadget;
	//------------------------------------------------------------------------------------------------
	override void ApplyEffect(notnull IEntity target, notnull IEntity user, IEntity item, ItemUseParameters animParams)
	{
	
	
		
		

	}
	
	
	//------------------------------------------------------------------------------------------------
	override bool CanApplyEffect(notnull IEntity target, notnull IEntity user,out SCR_EConsumableFailReason failReason = SCR_EConsumableFailReason.NONE)
	{
		

		return true;
	}

	
	override ItemUseParameters GetAnimationParameters(IEntity item, notnull IEntity target, ECharacterHitZoneGroup group = ECharacterHitZoneGroup.VIRTUAL)
	{
		ItemUseParameters itemUseParams = super.GetAnimationParameters(item, target, group);
		itemUseParams.SetAllowMovementDuringAction(true);
		return itemUseParams;
	}
	
	//------------------------------------------------------------------------------------------------
	EDamageType GetDefaultDamageType()
	{
		return EDamageType.HEALING;
	}
	
	//------------------------------------------------------------------------------------------------
	// constructor
	void SCR_ConsumableFood()
	{
		m_eConsumableType = SCR_EConsumableType.SALINE;
	}
}
