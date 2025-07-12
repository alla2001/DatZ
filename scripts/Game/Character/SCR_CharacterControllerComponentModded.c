

modded class SCR_CharacterControllerComponent : CharacterControllerComponent
{
	float m_fLastAttackTime;
	float m_fAttackCooldown = 2; 
	DatZMetabolsimHandler metaHandler;
	override void OnInit(IEntity owner){
	
	vanilla.OnInit(owner);
		
		metaHandler = DatZMetabolsimHandler.Cast(owner.FindComponent(DatZMetabolsimHandler));

	}
	//------------------------------------------------------------------------------------------------
	override bool GetCanMeleeAttack()
	{
				
		if (!m_MeleeComponent)
			return false;

		if (IsFalling())
			return false;
		if (IsSprinting())
			return false;

		if (GetStance() == ECharacterStance.PRONE)
			return false;

		// TODO: Gadget melee weapon properties in case we want to be able to have melee component, like a shovel?
		if (IsGadgetInHands())
			return false;

		//! check presence of MeleeWeaponProperties component to ensure it is an Melee weapon or not
		BaseWeaponManagerComponent weaponManager = GetWeaponManagerComponent();
		if (weaponManager && !SCR_WeaponLib.CurrentWeaponHasComponent(weaponManager, SCR_MeleeWeaponProperties))
			return false;
		if (metaHandler.stamina - metaHandler.staminaMeleeCost <0 || metaHandler.m_bIsMeleeAttacking)
   			 return false; // Too soon to attack again!

		metaHandler.OnMeleeAttack();
		return true;
	}
	
	
}

