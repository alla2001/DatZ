[BaseContainerProps()]
modded class SCR_AIDangerReaction_VehicleHorn : SCR_AIDangerReaction
{
	static const float REACTION_ENEMY_DIST_SQ = 50*50;
	static const float REACTION_FRIENDLY_DIST_SQ = 12*12;
	
	override bool PerformReaction(notnull SCR_AIUtilityComponent utility, notnull SCR_AIThreatSystem threatSystem, AIDangerEvent dangerEvent, int dangerEventCount)
	{		
		
		IEntity target = utility.GetOwner();
		AIDangerEventDatz dadangerEvent = new AIDangerEventDatz();
		dadangerEvent.SetDangerType(EAIDangerEventType.Danger_WeaponFire);
		
		dadangerEvent.SetObject(dangerEvent.GetVictim());
		dadangerEvent.SetPosition(dangerEvent.GetPosition());
		GetGame().GetAIWorld().RequestBroadcastDangerEvent(dadangerEvent);
 		threatSystem.ThreatShotFired(10,0);
		return true;
	}	
};