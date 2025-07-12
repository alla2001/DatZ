// Script File
class DrinkUserAction : ScriptedUserAction
{	
	
	WaterChecker waterchecker;
	TAnimGraphCommand CMD ;
	CharacterAnimationComponent animation ;
	FluidContainerComponent fluidCont;
	//------------------------------------------------------------------------------------------------
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent) 
	{
		if (!GetGame().GetWorldEntity())
			return;
		
		GenericEntity genEnt = GenericEntity.Cast(pOwnerEntity);
		fluidCont = FluidContainerComponent.Cast(GetOwner().FindComponent(FluidContainerComponent));
		
	}
	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity) 
	{		
		animation.CallCommand(CMD,0, 0);
		BaseWeaponManagerComponent wpn = BaseWeaponManagerComponent.Cast( pUserEntity.FindComponent(BaseWeaponManagerComponent));
		GetGame().GetCallqueue().CallLater(SetWeaponVisiable,delay:500,param1:wpn);
		
		if(fluidCont && fluidCont.m_fCurrentAmount<=0)return;
		if(!fluidCont)
		DatZMetabolsimHandler.Cast( pUserEntity.FindComponent(DatZMetabolsimHandler)).Drink(10);
		if(fluidCont)
		{
			float waterused = Math.Min(10,fluidCont.m_fCurrentAmount);
			DatZMetabolsimHandler.Cast( pUserEntity.FindComponent(DatZMetabolsimHandler)).Drink(waterused);
			fluidCont.Drain(waterused);
		}
		
	}
	
	override void OnActionCanceled(IEntity pOwnerEntity, IEntity pUserEntity) { 
	
		
		animation.CallCommand(CMD,0, 0);
		BaseWeaponManagerComponent wpn = BaseWeaponManagerComponent.Cast( pUserEntity.FindComponent(BaseWeaponManagerComponent));
			
		GetGame().GetCallqueue().CallLater(SetWeaponVisiable,delay:500,param1:wpn);
	};

	override void OnActionStart(IEntity pUserEntity) { 

		animation = CharacterAnimationComponent.Cast( pUserEntity.FindComponent(CharacterAnimationComponent));
		CharacterControllerComponent cnt = CharacterControllerComponent.Cast( pUserEntity.FindComponent(CharacterControllerComponent));
		cnt.SetDynamicStance(0.5);
		cnt.RemoveGadgetFromHand();
		CharacterCommandHandlerComponent command = animation.GetCommandHandler();
		CharacterInputContext inpt = new CharacterInputContext();
		inpt.SetRaiseWeapon(false);
		command.GetCommandModifier_Weapon();
		command.HandleWeaponsDefault(inpt,0,0);
		BaseWeaponManagerComponent wpn = BaseWeaponManagerComponent.Cast( pUserEntity.FindComponent(BaseWeaponManagerComponent));
		wpn.SetVisibleCurrentWeapon(false);
		
		
		 CMD = animation.BindCommand("CMD_Drinking");
		animation.CallCommand(CMD,1, 0);
		
	};
	
	void SetDrinknigCommand(int val){
	
			animation.CallCommand(CMD,1, 0);
	
	}
	void SetWeaponVisiable(BaseWeaponManagerComponent wpn){
	
		wpn.SetVisibleCurrentWeapon(true);
	
	}
	//------------------------------------------------------------------------------------------------
	override bool CanBePerformedScript(IEntity user)
	{
	
		
		return true;
	}
	

	override bool CanBeShownScript(IEntity user)
	{
	
		if(!fluidCont)
		return true;
		
		if(fluidCont && fluidCont.m_fCurrentAmount<=0)return false;
		
		return true;
		
	}
	
};