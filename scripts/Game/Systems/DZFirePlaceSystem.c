class DZFirePlaceSystem : GameSystem
{
	[Attribute("1000")]
	float timeToTick;

	ref array<FirePlaceStorageComponent> firePlaces = new array<FirePlaceStorageComponent>();

	override void OnInit()
	{
		GetGame().GetCallqueue().CallLater(UpdateFire, timeToTick, true);

	}
	void UpdateFire()
	{
		foreach(FirePlaceStorageComponent fire : firePlaces)
		{
		
			fire.UpdateFire(timeToTick/1000);
		}
		
		
	}
	//------------------------------------------------------------------------------------------------
	override protected void OnUpdate(ESystemPoint point)
	{
		
		
	}
		//------------------------------------------------------------------------------------------------
	static DZFirePlaceSystem GetInstance()
	{
		World world = GetGame().GetWorld();

		if (!world)
			return null;

		return DZFirePlaceSystem.Cast(world.FindSystem(DZFirePlaceSystem));
	}


	
	//------------------------------------------------------------------------------------------------
	//! \param component must not be null
	void Register(FirePlaceStorageComponent component)
	{
		if(!firePlaces.Contains(component))
		firePlaces.Insert(component)
	}
	
	//------------------------------------------------------------------------------------------------
	void Unregister(FirePlaceStorageComponent component)
	{
		if(firePlaces.Contains(component))
		firePlaces.RemoveItem(component)
	}
}
