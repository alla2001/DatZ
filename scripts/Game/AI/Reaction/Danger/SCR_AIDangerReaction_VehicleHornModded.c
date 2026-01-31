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

		// Notify zombie sound system
		if (Replication.IsServer())
		{
			DatZSoundManager soundMgr = DatZSoundManager.GetInstance();
			if (soundMgr)
				soundMgr.EmitSound(dangerEvent.GetPosition(), DatZSoundType.VEHICLE_HORN, 1.0);
		}

		return true;
	}
};