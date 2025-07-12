/*
	Credit goes to Haleks for the Ravage mod
*/
[BaseContainerProps()]
class DatzDangerReactionWrapper
{
	static bool PerformReaction(notnull SCR_AIUtilityComponent utility, notnull SCR_AIThreatSystem threatSystem, AIDangerEvent dangerEvent, int dangerEventCount)
	{
		
			
		AIDangerEventDatz datzEvent = AIDangerEventDatz.Cast( dangerEvent);
		
		if(datzEvent &&datzEvent.eventType == DatZEvent.CarHorn )
			if(vector.Distance(utility.m_OwnerEntity.GetOrigin(),dangerEvent.GetPosition())>125)
				return false;
			else
				return true;
		if(datzEvent &&datzEvent.eventType == DatZEvent.Walking )
		{
			float distance = vector.Distance(utility.m_OwnerEntity.GetOrigin(),dangerEvent.GetPosition());
			if(distance>datzEvent.walkingSpeed)
				return false;
			else
				return true;
		
		}
			
	
		return true;
	}
};

