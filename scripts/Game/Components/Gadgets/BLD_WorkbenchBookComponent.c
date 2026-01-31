[EntityEditorProps(category: "GameScripted/Gadgets", description: "Workbench placement gadget")]
class BLD_WorkbenchBookComponentClass : SCR_GadgetComponentClass
{
}

class BLD_WorkbenchBookComponent : SCR_GadgetComponent
{
	[Attribute("", UIWidgets.Object, category: "Consumable")]
	protected ref SCR_ConsumableEffectBase m_ConsumableEffect;

	[Attribute("0", UIWidgets.CheckBox, "Switch model on use", category: "Consumable")]
	protected bool m_bAlternativeModelOnAction;

	[Attribute("0", UIWidgets.CheckBox, "Show the model of the item on the character after using", category: "Consumable")]
	protected bool m_bVisibleEquipped;

	protected SCR_CharacterControllerComponent m_CharController;
	protected IEntity m_TargetCharacter;
	protected int m_iStoredHealedGroup;
	protected ref array<ref EntityID> nearbyEntities;
	bool done;

	override bool IsVisibleEquipped()
	{
		return m_bVisibleEquipped;
	}

	override void ModeSwitch(EGadgetMode mode, IEntity charOwner)
	{
		super.ModeSwitch(mode, charOwner);
		if (!Replication.IsServer())
			return;

		if (m_CharacterOwner)
		{
			BLD_PlacerComponent placer = BLD_PlacerComponent.Cast(m_CharacterOwner.FindComponent(BLD_PlacerComponent));
			if (placer)
			{
				if (mode == EGadgetMode.IN_HAND)
					placer.SpawnWorkbenchGhost();
				else
					placer.RemoveWorkbenchGhost();
			}
		}
	}

	bool ClientBlocked(IEntity plr)
	{
		nearbyEntities = new array<ref EntityID>;
		GetGame().GetWorld().QueryEntitiesBySphere(GetOwner().GetOrigin(), 300.0, CheckEntityAddToArray, null, EQueryEntitiesFlags.STATIC);
		foreach (EntityID entityID : nearbyEntities)
		{
			IEntity entity = GetGame().GetWorld().FindEntityByID(entityID);
			if (!entity)
				continue;
			Bld_BuildBlockTriggerZone triggerZone = Bld_BuildBlockTriggerZone.Cast(entity);
			if (triggerZone)
			{
				vector playerPos = plr.GetOrigin();
				vector zonePos = triggerZone.GetOrigin();
				float radius = triggerZone.GetSphereRadius();
				if (vector.Distance(playerPos, zonePos) <= radius)
				{
					Print("Blocked");
					return true;
				}
			}
		}
		return false;
	}

	override void ActivateAction()
	{
		if (ClientBlocked(m_CharacterOwner))
		{
			AudioSystem.PlaySound("{275A883E38CDEF60}Sounds/UI/Samples/Menu/UI_Task_Failed.wav");
			SCR_PopUpNotification.GetInstance().PopupMsg("Build Blocked Zone");
			return;
		}
		super.ActivateAction();
		RplComponent rplComponent = RplComponent.Cast(m_CharacterOwner.FindComponent(RplComponent));
		if (!rplComponent || !rplComponent.IsOwner())
			return;

		if (done)
			return;

		BLD_PlacerComponent placer = BLD_PlacerComponent.Cast(m_CharacterOwner.FindComponent(BLD_PlacerComponent));
		if (placer)
		{
			done = true;
			placer.PlaceWorkbench(GetOwner());
			GetGame().GetCallqueue().CallLater(AllowUse, 1000);
		}
	}

	void AllowUse()
	{
		done = false;
	}

	protected bool CheckEntityAddToArray(IEntity entity)
	{
		nearbyEntities.Insert(entity.GetID());
		return true;
	}

	override void ModeClear(EGadgetMode mode)
	{
		if (mode == EGadgetMode.IN_HAND)
		{
			if (m_CharController)
				GetGame().GetCallqueue().CallLater(ClearInvokers, param1: -1);

			if (Replication.IsServer())
			{
				BLD_PlacerComponent placer = BLD_PlacerComponent.Cast(m_CharacterOwner.FindComponent(BLD_PlacerComponent));
				if (placer)
					placer.RemoveWorkbenchGhost();
			}
		}
		super.ModeClear(mode);
	}

	protected void ClearInvokers(int targetGroup = -1)
	{
		if (!m_CharController)
			return;

		m_CharController.m_OnItemUseFinishedInvoker.Remove(OnApplyToCharacter);
		m_CharController.m_OnItemUseBeganInvoker.Remove(OnUseBegan);
		m_CharController = null;
	}

	protected void OnUseBegan(IEntity item, ItemUseParameters animParams)
	{
	}

	protected void OnApplyToCharacter(IEntity item, bool successful, ItemUseParameters animParams)
	{
	}

	SCR_EConsumableType GetConsumableType()
	{
		if (m_ConsumableEffect)
			return m_ConsumableEffect.m_eConsumableType;

		return SCR_EConsumableType.NONE;
	}

	SCR_ConsumableEffectBase GetConsumableEffect()
	{
		return m_ConsumableEffect;
	}

	override EGadgetType GetType()
	{
		return EGadgetType.CONSUMABLE;
	}

	void SetTargetCharacter(IEntity target)
	{
		m_TargetCharacter = target;
	}

	IEntity GetTargetCharacter()
	{
		return m_TargetCharacter;
	}
}
