//------------------------------------------------------------------------------------------------
class AirDropGameSystem : GameSystem
{
	[Attribute("{D181F4FA8BFCAF58}Prefabs/Vehicles/AirDropPlane.et", UIWidgets.ResourceNamePicker, "Plane prefab", "et")]
	ResourceName m_PlanePrefab;

	[Attribute("", UIWidgets.ResourceNamePicker, "Airdrop crate prefabs (random selection)", "et")]
	ref array<ResourceName> m_aAirdropPrefabs;

	[Attribute("150", UIWidgets.EditBox, "Margin from map edges in meters")]
	float m_fMapEdgeMargin;

	[Attribute("500", UIWidgets.EditBox, "Flight altitude above ground")]
	float m_fFlightAltitude;

	[Attribute("7200000", UIWidgets.EditBox, "Spawn interval in milliseconds (2 hours = 7200000)")]
	int m_iSpawnInterval;

	[Attribute("30", UIWidgets.EditBox, "Max attempts to find valid land position")]
	int m_iMaxLandFindAttempts;

	[Attribute("1000", UIWidgets.EditBox, "Distance beyond map edge for plane spawn/despawn")]
	float m_fBeyondMapDistance;

	// Auto-detected map bounds
	protected vector m_vMapMin;
	protected vector m_vMapMax;
	protected bool m_bBoundsInitialized = false;

	//------------------------------------------------------------------------------------------------
	override void OnInit()
	{
		super.OnInit();

		if (!Replication.IsServer())
			return;

		Print("AirDropGameSystem: Starting", LogLevel.NORMAL);
		InitializeMapBounds();
		StartSpawnTimer();
	}

	//------------------------------------------------------------------------------------------------
	protected void InitializeMapBounds()
	{
		IEntity worldEntity = GetGame().GetWorldEntity();
		if (!worldEntity)
		{
			Print("AirDropGameSystem: No world entity!", LogLevel.ERROR);
			return;
		}

		vector mins, maxs;
		worldEntity.GetWorldBounds(mins, maxs);

		m_vMapMin = Vector(mins[0] + m_fMapEdgeMargin, 0, mins[2] + m_fMapEdgeMargin);
		m_vMapMax = Vector(maxs[0] - m_fMapEdgeMargin, 0, maxs[2] - m_fMapEdgeMargin);
		m_bBoundsInitialized = true;

		Print(string.Format("AirDropGameSystem: Map bounds [%1, %2] to [%3, %4]",
			Math.Round(m_vMapMin[0]), Math.Round(m_vMapMin[2]),
			Math.Round(m_vMapMax[0]), Math.Round(m_vMapMax[2])), LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	protected void StartSpawnTimer()
	{
		// Initial spawn after 1 minute
		GetGame().GetCallqueue().CallLater(SpawnAirdropPlane, 60000, false);

		// Repeating airdrops
		GetGame().GetCallqueue().CallLater(SpawnAirdropPlane, m_iSpawnInterval, true);

		Print(string.Format("AirDropGameSystem: Interval %1 minutes", m_iSpawnInterval / 60000), LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	protected void SpawnAirdropPlane()
	{
		if (m_PlanePrefab.IsEmpty())
		{
			Print("AirDropGameSystem: No plane prefab configured!", LogLevel.ERROR);
			return;
		}

		// Find valid land position for drop
		vector dropPos;
		if (!FindValidLandPosition(dropPos))
		{
			Print("AirDropGameSystem: Could not find valid drop position!", LogLevel.WARNING);
			return;
		}

		// Calculate flight path (from one edge to opposite edge, passing over drop point)
		vector startPos, endPos;
		CalculateFlightPath(dropPos, startPos, endPos);

		// Load plane prefab
		Resource resource = Resource.Load(m_PlanePrefab);
		if (!resource)
		{
			Print("AirDropGameSystem: Failed to load plane prefab!", LogLevel.ERROR);
			return;
		}

		// Spawn plane at start position
		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		params.Transform[3] = startPos;

		IEntity plane = GetGame().SpawnEntityPrefab(resource, GetWorld(), params);
		if (!plane)
		{
			Print("AirDropGameSystem: Failed to spawn plane!", LogLevel.ERROR);
			return;
		}

		// Configure the plane component
		AirDropPlaneComponent planeComp = AirDropPlaneComponent.Cast(plane.FindComponent(AirDropPlaneComponent));
		if (planeComp)
		{
			// Set random airdrop prefab if available
			if (m_aAirdropPrefabs && !m_aAirdropPrefabs.IsEmpty())
			{
				planeComp.m_AirdropPrefab = m_aAirdropPrefabs.GetRandomElement();
			}

			planeComp.SetFlightPath(startPos, dropPos, endPos);
		}

		Print(string.Format("AirDropGameSystem: Plane spawned, drop target [%1, %2]",
			Math.Round(dropPos[0]), Math.Round(dropPos[2])), LogLevel.NORMAL);

		// Alert players
		AlertAllPlayers(dropPos);
	}

	//------------------------------------------------------------------------------------------------
	protected void CalculateFlightPath(vector dropPos, out vector startPos, out vector endPos)
	{
		// Random approach direction (0-3 for 4 cardinal directions)
		int direction = Math.RandomInt(0, 4);

		float altitude = dropPos[1] + m_fFlightAltitude;
		float beyondDist = m_fBeyondMapDistance;

		switch (direction)
		{
			case 0: // West to East
				startPos = Vector(m_vMapMin[0] - beyondDist, altitude, dropPos[2]);
				endPos = Vector(m_vMapMax[0] + beyondDist, altitude, dropPos[2]);
				break;
			case 1: // East to West
				startPos = Vector(m_vMapMax[0] + beyondDist, altitude, dropPos[2]);
				endPos = Vector(m_vMapMin[0] - beyondDist, altitude, dropPos[2]);
				break;
			case 2: // South to North
				startPos = Vector(dropPos[0], altitude, m_vMapMin[2] - beyondDist);
				endPos = Vector(dropPos[0], altitude, m_vMapMax[2] + beyondDist);
				break;
			case 3: // North to South
				startPos = Vector(dropPos[0], altitude, m_vMapMax[2] + beyondDist);
				endPos = Vector(dropPos[0], altitude, m_vMapMin[2] - beyondDist);
				break;
		}

		Print(string.Format("AirDropGameSystem: Flight path from [%1,%2] to [%3,%4]",
			Math.Round(startPos[0]), Math.Round(startPos[2]),
			Math.Round(endPos[0]), Math.Round(endPos[2])), LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	protected bool FindValidLandPosition(out vector outPosition)
	{
		if (!m_bBoundsInitialized)
			InitializeMapBounds();

		BaseWorld world = GetWorld();
		if (!world)
			return false;

		for (int attempt = 0; attempt < m_iMaxLandFindAttempts; attempt++)
		{
			float x = Math.RandomFloat(m_vMapMin[0], m_vMapMax[0]);
			float z = Math.RandomFloat(m_vMapMin[2], m_vMapMax[2]);

			if (IsPositionOnLand(x, z, world))
			{
				float terrainY = world.GetSurfaceY(x, z);
				outPosition = Vector(x, terrainY, z);
				return true;
			}
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsPositionOnLand(float x, float z, BaseWorld world)
	{
		float terrainY = world.GetSurfaceY(x, z);

		EWaterSurfaceType waterType;
		float lakeArea;
		vector checkPos = Vector(x, terrainY, z);
		float waterY = SCR_WorldTools.GetWaterSurfaceY(world, checkPos, waterType, lakeArea);

		if (waterType == EWaterSurfaceType.WST_OCEAN)
			return false;

		if (waterY > terrainY + 1.0)
			return false;

		if (terrainY < 0.5)
			return false;

		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected void AlertAllPlayers(vector dropPosition)
	{
		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);

		foreach (int playerId : playerIds)
		{
			PlayerController pc = GetGame().GetPlayerManager().GetPlayerController(playerId);
			if (!pc)
				continue;

			IEntity controlledEntity = pc.GetControlledEntity();
			if (!controlledEntity)
				continue;

			BLD_PlacerComponent placer = BLD_PlacerComponent.Cast(controlledEntity.FindComponent(BLD_PlacerComponent));
			if (placer)
				placer.PlaySoundFromPlayer("{275A883E38CDEF60}Sounds/UI/Samples/Menu/UI_Task_Failed.wav", "SUPPLY PLANE INCOMING!");
		}
	}
}
