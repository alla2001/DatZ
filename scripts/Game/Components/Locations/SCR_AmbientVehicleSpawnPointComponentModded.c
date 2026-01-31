

modded class SCR_AmbientVehicleSpawnPointComponent : ScriptComponent
{
	

	//------------------------------------------------------------------------------------------------
	//!
	//! \return the created vehicle
override Vehicle SpawnVehicle()
	{
		
		if(Math.RandomInt(0,5)>1) return null;
		
		SCR_FactionAffiliationComponent comp = SCR_FactionAffiliationComponent.Cast(GetOwner().FindComponent(SCR_FactionAffiliationComponent));

		if (!comp)
			return null;

		SCR_Faction faction = SCR_Faction.Cast(comp.GetAffiliatedFaction());

		if (!faction)
			faction = SCR_Faction.Cast(comp.GetDefaultAffiliatedFaction());

		if (faction != m_SavedFaction || (!faction && m_sPrefab.IsEmpty()))
			Update(faction);

		if (m_sPrefab.IsEmpty())
			return null;

		Resource prefab = Resource.Load(m_sPrefab);

		if (!prefab || !prefab.IsValid())
			return null;

		vector pos;
		bool spawnEmpty = SCR_WorldTools.FindEmptyTerrainPosition(pos, GetOwner().GetOrigin(), SPAWNING_RADIUS, SPAWNING_RADIUS);

		if (!spawnEmpty)
		{
#ifdef WORKBENCH
			Print("SCR_AmbientVehicleSpawnPointComponent: FindEmptyTerrainPosition failed at " + GetOwner().GetOrigin().ToString(), LogLevel.WARNING);
#endif

			// In case this spawnpoint is blocked from the start, don't process it anymore
			// Prevents unexpected behavior such as vehicles spawning on a spot where a service composition has been built and after a session load dismantled
			if (!m_bFirstSpawnDone)
				m_bDepleted = true;

			return null;
		}

		EntitySpawnParams params = EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		GetOwner().GetTransform(params.Transform);

		if (m_Vehicle)
			RemoveInteractionHandlers(m_Vehicle);

		m_Vehicle = Vehicle.Cast(GetGame().SpawnEntityPrefab(prefab, null, params));
		m_fRespawnTimestamp = null;
		m_bFirstSpawnDone = true;
		m_bSpawnProcessed = true;

		if (!m_Vehicle)
			return null;

		m_fSpawnTimestamp = GetGame().GetWorld().GetWorldTime();

		CarControllerComponent carController = CarControllerComponent.Cast(m_Vehicle.FindComponent(CarControllerComponent));

		// Set fuel to 20% minimum, up to 35%
		SCR_FuelManagerComponent fuelmanager = SCR_FuelManagerComponent.Cast(m_Vehicle.FindComponent(SCR_FuelManagerComponent));
		if (fuelmanager)
			fuelmanager.SetTotalFuelPercentage(Math.RandomFloat(0.2, 0.35));

		// Damage all hit zones so car spawns broken but not destroyed
		SCR_VehicleDamageManagerComponent damageManager = SCR_VehicleDamageManagerComponent.Cast(m_Vehicle.FindComponent(SCR_VehicleDamageManagerComponent));
		if (damageManager)
		{
			array<HitZone> allHitZones = {};
			damageManager.GetAllHitZones(allHitZones);

			foreach (HitZone hz : allHitZones)
			{
				if (!hz)
					continue;

				// Set each hit zone to 5-30% health (broken but won't explode)
				float healthPct = Math.RandomFloat(0.05, 0.30);
				hz.SetHealthScaled(healthPct);
			}
		}
		// Activate handbrake so the vehicles don't go downhill on their own when spawned
		if (carController)
			carController.SetPersistentHandBrake(true);

		Physics physicsComponent = m_Vehicle.GetPhysics();

		// Snap to terrain
		if (physicsComponent)
			physicsComponent.SetVelocity("0 -1 0");

		EventHandlerManagerComponent handler = EventHandlerManagerComponent.Cast(m_Vehicle.FindComponent(EventHandlerManagerComponent));

		if (handler)
			handler.RegisterScriptHandler("OnDestroyed", this, OnVehicleDestroyed);

		if (m_bStopDespawnOnInteraction)
			AddInteractionHandlers(m_Vehicle);

		return m_Vehicle;
	}

	
}
