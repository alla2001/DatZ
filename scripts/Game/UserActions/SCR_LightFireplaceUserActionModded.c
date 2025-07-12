//simple script for lighting fireplces

modded class SCR_LightFireplaceUserAction : ScriptedUserAction
{
	[Attribute("#AR-UserAction_LightFire", UIWidgets.EditBox, "Description for action menu (light up)", "")]
	protected LocalizedString  m_sLightDescription;
	[Attribute("#AR-UserAction_PutOutFire", UIWidgets.EditBox, "Description for action menu (extinguish)", "")]
	protected LocalizedString m_sExtinguishDescription;	
	
	protected SCR_FireplaceComponent m_FireplaceComponent;
	protected FirePlaceStorageComponent m_FireplaceStrComponent;
	TAnimGraphCommand CMD ;
	CharacterAnimationComponent animation ;
	//------------------------------------------------------------------------------------------------
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		m_FireplaceComponent = SCR_FireplaceComponent.Cast(pOwnerEntity.FindComponent(SCR_FireplaceComponent));
		m_FireplaceStrComponent= FirePlaceStorageComponent.Cast(pOwnerEntity.FindComponent(FirePlaceStorageComponent));
		GetGame().GetCallqueue().CallLater(DelayInit,delay:1000);
	}
	
	void DelayInit(){
		if(DZFirePlaceSystem.GetInstance())
		DZFirePlaceSystem.GetInstance().Register(m_FireplaceStrComponent);
	
	}


	
	//------------------------------------------------------------------------------------------------
	override bool CanBeShownScript(IEntity user)
	{
		if (!m_FireplaceComponent)
			return false;
		
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity) 
	{	
		if (controller)
		{
    		controller.SetDisableMovementControls(false); // Disables movement inputs
    		
    		controller.SetDisableWeaponControls(false);   // Disables weapon-related inputs
		}
		m_FireplaceComponent.ToggleLight(!m_FireplaceComponent.IsOn());
		animation.CallCommand(CMD,0, 0);
		BaseWeaponManagerComponent wpn = BaseWeaponManagerComponent.Cast( pUserEntity.FindComponent(BaseWeaponManagerComponent));
		GetGame().GetCallqueue().CallLater(SetWeaponVisiable,delay:500,param1:wpn);
	}
	override void OnActionCanceled(IEntity pOwnerEntity, IEntity pUserEntity) { 
	
		if (controller)
		{
    		controller.SetDisableMovementControls(false); // Disables movement inputs
    		
    		controller.SetDisableWeaponControls(false);   // Disables weapon-related inputs
		}
		animation.CallCommand(CMD,0, 0);
		BaseWeaponManagerComponent wpn = BaseWeaponManagerComponent.Cast( pUserEntity.FindComponent(BaseWeaponManagerComponent));
		
		GetGame().GetCallqueue().CallLater(SetWeaponVisiable,delay:500,param1:wpn);
	};
	CharacterControllerComponent controller;
	override void OnActionStart(IEntity pUserEntity) { 
		
		
		animation = CharacterAnimationComponent.Cast( pUserEntity.FindComponent(CharacterAnimationComponent));
		CharacterControllerComponent cnt = CharacterControllerComponent.Cast( pUserEntity.FindComponent(CharacterControllerComponent));
	
		
		CharacterCommandHandlerComponent command = animation.GetCommandHandler();
		CharacterInputContext inpt = new CharacterInputContext();
	
	
		BaseWeaponManagerComponent wpn = BaseWeaponManagerComponent.Cast( pUserEntity.FindComponent(BaseWeaponManagerComponent));
	
		
		
		CMD = animation.BindCommand("CMD_FireBlowing");
		if(!m_FireplaceComponent.IsOn()){
		
			//animation.CallCommand(CMD,1, 0);
			//wpn.SetVisibleCurrentWeapon(false);
			//inpt.SetRaiseWeapon(false);
			cnt.SetDynamicStance(0.5);
			cnt.RemoveGadgetFromHand();
			command.GetCommandModifier_Weapon();
			command.HandleWeaponsDefault(inpt,0,0);
		

			if (wpn)
			{	
    			
			}
		}
	
		
		controller = CharacterControllerComponent.Cast(pUserEntity.FindComponent(CharacterControllerComponent));

		if (controller)
		{
    		controller.SetDisableMovementControls(true); // Disables movement inputs
    	
    		controller.SetDisableWeaponControls(true);   // Disables weapon-related inputs
		}
		
	};
	void SetWeaponVisiable(BaseWeaponManagerComponent wpn){
	
		wpn.SetVisibleCurrentWeapon(true);
	
	}
	//------------------------------------------------------------------------------------------------
	override bool GetActionNameScript(out string outName)
	{
		if (!m_FireplaceComponent)
			return false;
		
		if (!m_FireplaceComponent.IsOn())
			outName = m_sLightDescription ;
		else
			outName = m_sExtinguishDescription;
		if(m_FireplaceStrComponent)
		outName = outName + " (" + (m_FireplaceStrComponent.m_currentFuel.ToString(lenDec:1)) + "%)";
		return true;
	}
	
	override bool CanBePerformedScript(IEntity user)
	{
		if(!m_FireplaceStrComponent)
		return false;
		if(!m_FireplaceComponent.IsOn() && !m_FireplaceStrComponent.canTurnOn())
		{
		 	SetCannotPerformReason("No Fuel In Fire");
			 return false;
		}
		return true;
		
	}
	
};
