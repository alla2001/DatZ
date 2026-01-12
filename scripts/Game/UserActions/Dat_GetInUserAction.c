modded class SCR_GetInUserAction {	
	override bool CanBePerformedScript(IEntity user) {
		if (m_DamageManager && m_DamageManager.GetState() == EDamageState.DESTROYED)
			return false;

		BaseCompartmentSlot compartment = GetCompartmentSlot();
		if (!compartment)
			return false;
		
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(user);
		if (!character)
			return false;

		CompartmentAccessComponent compartmentAccess = character.GetCompartmentAccessComponent();
		if (!compartmentAccess)
			return false;
		
		//~ TODO: Hotfix until proper solution, only blocks player does not block AI or Editor actions
		float storedResources;
		if (m_ResourceComp && m_CompartmentManager && m_CompartmentManager.BlockSuppliesIfOccupied())
		{
			if (SCR_ResourceSystemHelper.GetStoredResources(m_ResourceComp, storedResources) && storedResources > 0)
			{
				SetCannotPerformReason(OCCUPIED_BY_SUPPLIES);
				return false;
			}
		}		
		if (compartment.GetOccupant() || m_CompartmentManager.IsGetInAndOutBlockedByDoorUser(GetRelevantDoorIndex(user)))
		{
			SetCannotPerformReason("#AR-UserAction_SeatOccupied");
			return false;
		}
		
		// Check if the position isn't lock.
		if (m_pLockComp && m_pLockComp.IsLocked(user, compartment))
		{
			SetCannotPerformReason(m_pLockComp.GetCannotPerformReason(user));
			return false;
		}
		
		// Make sure vehicle can be enter via provided door, if not, set reason.
		if (!compartmentAccess.CanGetInVehicleViaDoor(GetOwner(), m_CompartmentManager, GetRelevantDoorIndex(user)))
		{
			SetCannotPerformReason("#AR-UserAction_SeatObstructed");
			return false;
		}
		
		return true;
	}		
};