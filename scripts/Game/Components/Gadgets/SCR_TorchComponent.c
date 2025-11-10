
[EntityEditorProps(category: "GameScripted/Gadgets", description: "Flashlight", color: "0 0 255 255")]
class SCR_TorchComponentClass : SCR_GadgetComponentClass
{
		[Attribute("{C0E2E7DC28B71E2C}Particles/Enviroment/Campfire_medium_normal.ptc", UIWidgets.ResourceNamePicker, "Prefab of fire particle used for a fire action.", "ptc")]
	protected ResourceName m_sParticle;	

	[Attribute("0 0.2 0", UIWidgets.EditBox, "Particle offset in local space from the origin of the entity")]
	protected vector m_vParticleOffset;
	
	//------------------------------------------------------------------------------------------------
	//! \return
	ResourceName GetParticle()
	{
		return m_sParticle;
	}
	
	//------------------------------------------------------------------------------------------------
	//! \return
	vector GetParticleOffset()
	{
		return m_vParticleOffset;
	}
}

class SCR_TorchComponent : SCR_GadgetComponent
{
	

	protected bool m_bLastLightState;	// remember the last light state for cases where you put your flashlight into storage but there is no slot available -> true = active
	protected bool m_bIsSlottedVehicle;	// char owner is in vehicle while flashlight is slotted
	protected bool m_bIsOccluded;
	protected int m_iCurrentLenseColor;
	[Attribute()]
	ResourceName particles;
	
		private ParticleEffectEntity m_pFireParticle;
	protected SCR_BaseInteractiveLightComponentClass m_ComponentData;
	protected SCR_CompartmentAccessComponent m_CompartmentComp;
	protected SCR_CharacterControllerComponent m_CharController;
	
	ref	array<BaseSlotComponent> slots = new array<BaseSlotComponent>();
	FluidContainerComponent fuel;
	[Attribute("0.5")]
	protected float m_fFuelConsumptionRate ; // Fuel consumed per second while active
	
	//------------------------------------------------------------------------------------------------
	override void OnToggleActive(bool state)
	{
		if (state && fuel.m_fCurrentAmount <= 0.0) return;
			
		m_bActivated = state;
		/*
		// Play sound
		SCR_SoundManagerModule soundManagerEntity = GetGame().gets();
		if (soundManagerEntity)
		{
			if (m_bActivated)
				soundManagerEntity.CreateAndPlayAudioSource(GetOwner(), SCR_SoundEvent.SOUND_FLASHLIGHT_ON);
			else
				soundManagerEntity.CreateAndPlayAudioSource(GetOwner(), SCR_SoundEvent.SOUND_FLASHLIGHT_OFF);
		}*/

		UpdateLightState();
	}

	//------------------------------------------------------------------------------------------------
	//! Update state of light ON/OFF
	protected void UpdateLightState()
	{
		if (m_bActivated)
			EnableLight();
		else
			DisableLight();
	}


	
	

	//------------------------------------------------------------------------------------------------
	//! Spawn light entity
	protected void EnableLight()
	{
		foreach(BaseSlotComponent slot :slots)
		{
		
			IEntity attachedEntity = slot.GetAttachedEntity();
			attachedEntity.SetFlags(EntityFlags.VISIBLE, false);
			ParticleEffectEntity pfe = ParticleEffectEntity.Cast( attachedEntity);
			if(pfe){
			
				pfe.Play();
			}
			
		}
		SCR_TorchComponentClass componentData = SCR_TorchComponentClass.Cast(GetComponentData(GetOwner()));
		if (!m_pFireParticle && componentData) 
		{
			ParticleEffectEntitySpawnParams spawnParams = new ParticleEffectEntitySpawnParams();
			spawnParams.TargetWorld = GetOwner().GetWorld();
			spawnParams.Parent = GetOwner();
			Math3D.MatrixIdentity4(spawnParams.Transform);
			spawnParams.Transform[3] = componentData.GetParticleOffset();
			
			m_pFireParticle = ParticleEffectEntity.SpawnParticleEffect(componentData.GetParticle(), spawnParams);
		}
		else if (m_pFireParticle)
		{
			m_pFireParticle.Play();
		}
	
		

		
		SetEventMask(GetOwner(), EntityEvent.VISIBLE);

		
	}

	//------------------------------------------------------------------------------------------------
	//! Remove light
	protected void DisableLight()
	{
		
		foreach(BaseSlotComponent slot :slots)
		{
		
			IEntity attachedEntity = slot.GetAttachedEntity();
			attachedEntity.ClearFlags(EntityFlags.VISIBLE, false);
			ParticleEffectEntity pfe = ParticleEffectEntity.Cast( attachedEntity);
			if(pfe){	
			
				pfe.Stop();
			}
			
		}

		//Reset fire particles
		if (m_pFireParticle)
		{
			m_pFireParticle.Stop();
		}
		
		ClearEventMask(GetOwner(), EntityEvent.VISIBLE);
		
		DeactivateGadgetUpdate();
	}



	
	//------------------------------------------------------------------------------------------------
	//! SCR_CompartmentAccessComponent event
	protected void OnOwnerLifeStateChanged(ECharacterLifeState previousLifeState, ECharacterLifeState newLifeState)
	{
		if (newLifeState == ECharacterLifeState.ALIVE)
			ActivateGadgetUpdate();
		else
			DeactivateGadgetUpdate();
	}

	//------------------------------------------------------------------------------------------------
	//! SCR_CompartmentAccessComponent event
	protected void OnCompartmentEntered(IEntity targetEntity, BaseCompartmentManagerComponent manager, int mgrID, int slotID, bool move)
	{
		if (!m_bActivated)
			return;

		BaseCompartmentSlot compSlot = manager.FindCompartment(slotID, mgrID);
		IEntity occupant = compSlot.GetOccupant();
		if (occupant == m_CharacterOwner)
		{
			

			if (PilotCompartmentSlot.Cast(compSlot))
				ToggleActive(false, SCR_EUseContext.FROM_ACTION);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! SCR_CompartmentAccessComponent event
	protected void OnCompartmentLeft(IEntity targetEntity, BaseCompartmentManagerComponent manager, int mgrID, int slotID, bool move)
	{
		if (!m_bActivated)
			return;

		BaseCompartmentSlot compSlot = manager.FindCompartment(slotID, mgrID);
		IEntity occupant = compSlot.GetOccupant();
		if (occupant == m_CharacterOwner)
			
	}

	//------------------------------------------------------------------------------------------------
	override void OnSlotOccludedStateChanged(bool occluded)
	{
		m_bIsOccluded = occluded;
		if (m_bActivated && !m_bIsOccluded)
			EnableLight();
		else
			DisableLight();
	}

	//------------------------------------------------------------------------------------------------
	override protected void ModeSwitch(EGadgetMode mode, IEntity charOwner)
	{
		super.ModeSwitch(mode, charOwner);

		InventoryItemComponent inventoryItemComp = InventoryItemComponent.Cast(GetOwner().FindComponent(InventoryItemComponent));
		if (!inventoryItemComp)
			return;

		if (!charOwner)
		{
			if (m_CharController)
				m_CharController.m_OnLifeStateChanged.Remove(OnOwnerLifeStateChanged);

			m_CharController = null;
		}
		else
		{
			if (m_CharacterOwner)
				m_CharController = SCR_CharacterControllerComponent.Cast(m_CharacterOwner.GetCharacterController());

			if (m_CharController)
				m_CharController.m_OnLifeStateChanged.Insert(OnOwnerLifeStateChanged);
		}

		inventoryItemComp.SetTraceable(mode != EGadgetMode.IN_SLOT);

		if (m_bActivated && (mode == EGadgetMode.IN_STORAGE || mode == EGadgetMode.ON_GROUND))
		{
			m_bLastLightState = m_bActivated;
			OnToggleActive(false);//direct call since ToggleActive requries m_CharacterOwner to not be null as he then asks for propagation of the change
		}
		else if (mode == EGadgetMode.IN_SLOT || mode == EGadgetMode.IN_HAND)
		{
			if (m_bLastLightState && (!m_bActivated )) // LightEntity is disabled in EOnDeactivate aka when its dropped on the ground
				ToggleActive(m_bLastLightState, SCR_EUseContext.FROM_ACTION);

		}

		if (mode == EGadgetMode.IN_SLOT)
		{
			SCR_EquipmentStorageSlot parentSlot = SCR_EquipmentStorageSlot.Cast(inventoryItemComp.GetParentSlot());
			if (parentSlot && parentSlot.IsOccluded())
			{
				m_bIsOccluded = true;
				DisableLight();
			}

			m_CompartmentComp = SCR_CompartmentAccessComponent.Cast(m_CharacterOwner.FindComponent(SCR_CompartmentAccessComponent));
			if (m_CompartmentComp)
			{
				m_CompartmentComp.GetOnCompartmentEntered().Insert(OnCompartmentEntered);
				m_CompartmentComp.GetOnCompartmentLeft().Insert(OnCompartmentLeft);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	override protected void ModeClear(EGadgetMode mode)
	{
		if (mode == EGadgetMode.IN_SLOT)
		{
			m_bIsOccluded = false;
			if (m_CompartmentComp)
			{
				m_CompartmentComp.GetOnCompartmentEntered().Remove(OnCompartmentEntered);
				m_CompartmentComp.GetOnCompartmentLeft().Remove(OnCompartmentLeft);
			}

		}

		super.ModeClear(mode);
	}

	//------------------------------------------------------------------------------------------------
	override protected void ToggleActive(bool state, SCR_EUseContext context)
	{
		if (m_iMode == EGadgetMode.IN_STORAGE && !m_bActivated)	// trying to activate flashlight hidden in inventory
			return;

		// trying to activate flashlight while driving
		if (!m_bActivated && m_CompartmentComp && PilotCompartmentSlot.Cast(m_CompartmentComp.GetCompartment()))
			return;

		super.ToggleActive(state, context);
	}

	//------------------------------------------------------------------------------------------------
	override void ActivateAction()
	{
		if (m_CharController && m_CharController.GetLifeState() != ECharacterLifeState.ALIVE)	// if in chars inventory && char is dead/uncon
			return;

		ToggleActive(!m_bActivated, SCR_EUseContext.FROM_ACTION);
	}

	//------------------------------------------------------------------------------------------------
	override protected void ActivateGadgetUpdate()
	{
		super.ActivateGadgetUpdate();

		
	}

	//------------------------------------------------------------------------------------------------
	override protected void DeactivateGadgetUpdate()
	{
		super.DeactivateGadgetUpdate();

		if (m_iMode == EGadgetMode.IN_SLOT) // reset slot position of the gadget back to its default
	
	}

	//------------------------------------------------------------------------------------------------
	override EGadgetType GetType()
	{
		return EGadgetType.FLASHLIGHT;
	}

	//------------------------------------------------------------------------------------------------
	override bool IsVisibleEquipped()
	{
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool RplSave(ScriptBitWriter writer)
	{
		if (!super.RplSave(writer))
			return false;

		writer.WriteBool(m_bActivated);

		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool RplLoad(ScriptBitReader reader)
	{
		if (!super.RplLoad(reader))
			return false;

		reader.ReadBool(m_bActivated);
		

		UpdateLightState();

		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool OnTicksOnRemoteProxy()
	{
		return true; // proxies will only tick on owners without this
	}

	//------------------------------------------------------------------------------------------------
	override void Update(float timeSlice)
	{
		if (!m_CharController)
		{
			DeactivateGadgetUpdate();
			return;
		}
		// Consume fuel if active

	}
	
	override void EOnFixedFrame(IEntity owner,float timeSlice){
	
	
		if (m_bActivated && Replication.IsServer())
		{
			fuel.Drain( m_fFuelConsumptionRate * timeSlice);

			// Check for fuel depletion
			if (fuel.m_fCurrentAmount <= 0.0)
			{
				fuel.m_fCurrentAmount = 0.0;
				OnToggleActive(false);
				Rpc(TurnOff);
				// Optionally play a sound or particle effect for burnout

				return;
			}
		}
		
	
	}
		[RplRpc(RplChannel.Reliable,RplRcver.Broadcast)]
	void TurnOff(){
	
		OnToggleActive(false);
	}
	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		array<Managed> managed();
		owner.FindComponents(BaseSlotComponent,managed);
		
		foreach(Managed m : managed)
		{
			slots.Insert(BaseSlotComponent.Cast( m));
			
		}
		fuel = FluidContainerComponent.Cast(owner.FindComponent(FluidContainerComponent));
		fuel.m_fCurrentAmount = fuel.m_fMaxCapacity;
		
		SetEventMask(owner,EntityEvent.FIXEDFRAME);
	}
}
