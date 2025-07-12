class AIDangerEventDatz: AIDangerEventWeaponFire
{
	DatZEvent eventType = DatZEvent.CarHorn;
	float walkingSpeed = 0;
	override void OnReceived(AIAgent pReceiver){
	
		if(vector.Distance(GetObject().GetOrigin(),pReceiver.GetControlledEntity().GetOrigin())>5)
		{
			SetDangerType(EAIDangerEventType.Danger_None);
			SetObject(null);
			delete this;
			
		}
	}
}

enum DatZEvent{
WeaponFire,CarHorn, Walking
}
