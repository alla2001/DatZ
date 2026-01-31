class SCR_ConsumableAntidoteArea : LoadoutAreaType
{
}

//! Antidote - cures zombie infection
[BaseContainerProps()]
class SCR_ConsumableAntidote : SCR_ConsumableEffectHealthItems
{
	//------------------------------------------------------------------------------------------------
	override void ApplyEffect(notnull IEntity target, notnull IEntity user, IEntity item, ItemUseParameters animParams)
	{
		if (!Replication.IsServer())
			return;

		InventoryItemComponent itemComp = InventoryItemComponent.Cast(item.FindComponent(InventoryItemComponent));
		if (itemComp)
			itemComp.RequestUserLock(user, false);

		ChimeraCharacter char = ChimeraCharacter.Cast(target);
		if (!char)
			return;

		DatZMetabolsimHandler meta = DatZMetabolsimHandler.Cast(char.FindComponent(DatZMetabolsimHandler));
		if (!meta)
			return;

		// Cure the infection
		meta.CureInfection();

		// Delete the item after use
		RplComponent.DeleteRplEntity(item, false);
	}

	//------------------------------------------------------------------------------------------------
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

		GetGame().GetCallqueue().CallLater(EndInjection, m_fApplyToSelfDuration * 1000, false, animationComponent);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	void EndInjection(CharacterAnimationComponent anim)
	{
		if (anim)
			anim.CallCommand(m_iPlayerApplyToSelfCmdId, -2, -2);
	}

	//------------------------------------------------------------------------------------------------
	override bool CanApplyEffect(notnull IEntity target, notnull IEntity user, out SCR_EConsumableFailReason failReason = SCR_EConsumableFailReason.NONE)
	{
		ChimeraCharacter char = ChimeraCharacter.Cast(target);
		if (!char)
			return false;

		DatZMetabolsimHandler meta = DatZMetabolsimHandler.Cast(char.FindComponent(DatZMetabolsimHandler));
		if (!meta)
			return false;

		return true;
	}

	//------------------------------------------------------------------------------------------------
	override ItemUseParameters GetAnimationParameters(IEntity item, notnull IEntity target, ECharacterHitZoneGroup group = ECharacterHitZoneGroup.VIRTUAL)
	{
		ItemUseParameters itemUseParams = super.GetAnimationParameters(item, target, group);
		itemUseParams.SetAllowMovementDuringAction(false); // Must stand still for injection
		return itemUseParams;
	}

	//------------------------------------------------------------------------------------------------
	EDamageType GetDefaultDamageType()
	{
		return EDamageType.HEALING;
	}

	//------------------------------------------------------------------------------------------------
	void SCR_ConsumableAntidote()
	{
		m_eConsumableType = SCR_EConsumableType.SALINE;
		m_fApplyToSelfDuration = 3.0; // 3 second injection time
	}
}
