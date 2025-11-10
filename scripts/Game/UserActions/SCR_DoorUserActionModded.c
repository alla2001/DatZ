//! User action that ought to be attached to an entity with door component.
//! When performed either opens or closes the door based on the previous state of the door.
modded class SCR_DoorUserAction : DoorUserAction
{
	

	//------------------------------------------------------------------------------------------------
	override bool CanBePerformedScript(IEntity user)
	{
		
		DoorComponent doorComponent =DoorComponent.Cast( GetDoorComponent());
		
		DoorLockComponent lock = DoorLockComponent.Cast( GetOwner().FindComponent(DoorLockComponent));
		
		if(lock && lock.IsLocked()){
			
			SetCannotPerformReason("The door is locked");
			return false;
		
		
		}
		
		KeyDoorComponent keyDoor = KeyDoorComponent.Cast( GetOwner().FindComponent(KeyDoorComponent));
		if(keyDoor){
			SetCannotPerformReason("Key / KeyCard Needed");
			return false;
		}
		if (doorComponent)
			return true;
		
		return false;
	}
	

	
};

