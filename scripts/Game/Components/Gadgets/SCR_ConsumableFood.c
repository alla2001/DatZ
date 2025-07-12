class SCR_ConsumableFoodArea : LoadoutAreaType
{
}

//! Saline bag effect
[BaseContainerProps()]
class SCR_ConsumableFood : SCR_ConsumableEffectHealthItems
{
	[Attribute()]	
	float regenFoodValue;
	
	[Attribute()]	
	float regenWaterValue;
	[Attribute()]
	bool isWater;
	
	[Attribute()]
	bool isClosed;
	
	[Attribute("1"),RplProp()]
	int timesCanBeEating;
	
	FluidContainerComponent fluidContainer;
	//------------------------------------------------------------------------------------------------
	override void ApplyEffect(notnull IEntity target, notnull IEntity user, IEntity item, ItemUseParameters animParams)
	{
		if(!Replication.IsServer())return;
		
		InventoryItemComponent itemComp = InventoryItemComponent.Cast(item.FindComponent(InventoryItemComponent));
 	 	if (itemComp)
 	 		itemComp.RequestUserLock(user, false);

		ChimeraCharacter char = ChimeraCharacter.Cast(target);
		if (!char)
			return;
		
		DatZMetabolsimHandler meta = DatZMetabolsimHandler.Cast(char.FindComponent(DatZMetabolsimHandler));
		if (!meta)
			return;
		
	
		
		SCR_InventoryStorageManagerComponent inventoryStorageComp = SCR_InventoryStorageManagerComponent.Cast(user.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!inventoryStorageComp)
			return;
		float waterused = 0;
		if(isWater)
		{
			float waterneeded = meta.maxWater-meta.water;
			waterused = Math.Min(waterneeded,fluidContainer.m_fCurrentAmount);
			fluidContainer.Drain(waterused);
		}

		
		
		
		if(!isWater){
			meta.Eat(regenFoodValue);
			meta.Drink(regenWaterValue);
		}
	
		else
		meta.Drink(waterused);
	
		CookableItemComponent cookable  = CookableItemComponent.Cast( item.FindComponent(CookableItemComponent));
		if(cookable)
		{
				
				if(cookable.GetCookingState() == ECookState.RAW)
				{
					meta.Vomit(2);
				}
				if(cookable.GetCookingState() == ECookState.BURNT)
				{
					meta.VomitWithDamage(1,15);
					
				}
			
		}
		timesCanBeEating=0;
	
		if(timesCanBeEating<=0&&!fluidContainer)
	
		RplComponent.DeleteRplEntity(item, false);
		

	}
	override bool UpdateAnimationCommands(IEntity user)
	{
		ChimeraCharacter char = ChimeraCharacter.Cast(user);
		if (!char)
			return false;
		
		CharacterAnimationComponent animationComponent = char.GetAnimationComponent();
		if (!animationComponent)
			return false;		
		
		m_iPlayerApplyToSelfCmdId = animationComponent.BindCommand("CMD_HealSelf");
		m_iPlayerApplyToOtherCmdId = animationComponent.BindCommand("CMD_HealOther");
		m_iPlayerReviveCmdId = animationComponent.BindCommand("CMD_Revive");
		if (m_iPlayerApplyToSelfCmdId < 0 || m_iPlayerApplyToOtherCmdId < 0)
		{
			Print("One or both healing animationCommands have incorrect ID's!!", LogLevel.ERROR);
			return false;
		}
		GetGame().GetCallqueue().CallLater(EndEating,m_fApplyToSelfDuration*1000,param1:animationComponent);
		return true;
	}
	
	void EndEating(CharacterAnimationComponent anim)
	{
		anim.CallCommand(m_iPlayerApplyToSelfCmdId,-2,-2);
	
	}
	//------------------------------------------------------------------------------------------------
	override bool CanApplyEffect(notnull IEntity target, notnull IEntity user,out SCR_EConsumableFailReason failReason = SCR_EConsumableFailReason.NONE)
	{
		if(isClosed)return false;
		ChimeraCharacter char = ChimeraCharacter.Cast(target);
		if (!char)
			return false;
		
		
		DatZMetabolsimHandler meta = DatZMetabolsimHandler.Cast(char.FindComponent(DatZMetabolsimHandler));
		if (!meta)
			return false;
		if (!meta.GetCanEat())
			return false;
		if(isWater && !meta.isThursty()) return false;
		if(!isWater && !meta.isHungry()) return false;
		if(fluidContainer && fluidContainer.m_fCurrentAmount<=0) return false;
		if(fluidContainer) m_fApplyToSelfDuration = Math.Max(1.5,fluidContainer.m_fCurrentAmount/8);
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
