// DatZ Vehicle Sound Listener - Tracks vehicle engines and emits sounds
// Zombies can hear running vehicle engines

[EntityEditorProps(category: "GameScripted/DatZ", description: "Listens for vehicle sounds and notifies zombie hearing - add to GameMode")]
class DatZVehicleSoundListenerClass : SCR_BaseGameModeComponentClass
{
}

class DatZVehicleSoundListener : SCR_BaseGameModeComponent
{
	// How often to check vehicles (seconds)
	protected static const float CHECK_INTERVAL = 1.0;

	// Max distance to check vehicles from any player
	protected static const float MAX_CHECK_DISTANCE = 500.0;

	// Temp storage for query results
	protected ref array<Vehicle> m_QueryResults = {};

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!Replication.IsServer())
			return;

		// Start periodic check
		GetGame().GetCallqueue().CallLater(CheckVehicleEngines, CHECK_INTERVAL * 1000, true);
	}

	//------------------------------------------------------------------------------------------------
	void ~DatZVehicleSoundListener()
	{
		GetGame().GetCallqueue().Remove(CheckVehicleEngines);
	}

	//------------------------------------------------------------------------------------------------
	protected void CheckVehicleEngines()
	{
		if (!Replication.IsServer())
			return;

		DatZSoundManager soundMgr = DatZSoundManager.GetInstance();
		if (!soundMgr)
			return;

		// Get all vehicles near players
		PlayerManager playerMgr = GetGame().GetPlayerManager();
		if (!playerMgr)
			return;

		array<int> playerIds = {};
		playerMgr.GetPlayers(playerIds);

		// Collect player positions
		array<vector> playerPositions = {};
		foreach (int playerId : playerIds)
		{
			IEntity playerEnt = playerMgr.GetPlayerControlledEntity(playerId);
			if (playerEnt)
				playerPositions.Insert(playerEnt.GetOrigin());
		}

		if (playerPositions.IsEmpty())
			return;

		// Query vehicles near each player
		foreach (vector playerPos : playerPositions)
		{
			m_QueryResults.Clear();
			GetGame().GetWorld().QueryEntitiesBySphere(playerPos, MAX_CHECK_DISTANCE, QueryVehicleCallback);

			foreach (Vehicle vehicle : m_QueryResults)
			{
				if (!vehicle)
					continue;

				BaseVehicleControllerComponent controller = BaseVehicleControllerComponent.Cast(vehicle.FindComponent(BaseVehicleControllerComponent));
				if (!controller)
					continue;

				if (controller.IsEngineOn())
				{
					// Emit engine sound
					vector vehiclePos = vehicle.GetOrigin();
					soundMgr.EmitSound(vehiclePos, DatZSoundType.VEHICLE_ENGINE, 1.0);
				}
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected bool QueryVehicleCallback(IEntity ent)
	{
		Vehicle vehicle = Vehicle.Cast(ent);
		if (vehicle)
			m_QueryResults.Insert(vehicle);

		return true; // Continue iteration
	}
}
