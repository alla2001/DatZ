//------------------------------------------------------------------------------------------------
// Plane that flies across map and drops an airdrop crate
// Properly synced for multiplayer
//------------------------------------------------------------------------------------------------

[BaseContainerProps()]
class AirDropPlaneComponentClass : ScriptComponentClass
{
}

class AirDropPlaneComponent : ScriptComponent
{
	[Attribute("80", UIWidgets.EditBox, "Flight speed in m/s")]
	float m_fFlightSpeed;

	[Attribute("", UIWidgets.ResourceNamePicker, "Airdrop crate prefab to drop", "et")]
	ResourceName m_AirdropPrefab;

	// Replicated properties for multiplayer sync
	[RplProp(onRplName: "OnFlightDataChanged")]
	protected vector m_vStartPos;

	[RplProp(onRplName: "OnFlightDataChanged")]
	protected vector m_vEndPos;

	[RplProp()]
	protected vector m_vTargetDropPos;

	[RplProp()]
	protected bool m_bIsFlying = false;

	[RplProp()]
	protected bool m_bHasDropped = false;

	// Flight timing - must be replicated so clients calculate same position
	[RplProp(onRplName: "OnFlightDataChanged")]
	protected float m_fFlightStartTime;
	protected float m_fTotalFlightDistance;
	protected vector m_vFlightDirection;

	//------------------------------------------------------------------------------------------------
	void SetFlightPath(vector startPos, vector dropPos, vector endPos)
	{
		m_vStartPos = startPos;
		m_vTargetDropPos = dropPos;
		m_vEndPos = endPos;

		// Calculate flight data
		m_vFlightDirection = vector.Direction(startPos, endPos).Normalized();
		m_fTotalFlightDistance = vector.Distance(startPos, endPos);
		m_fFlightStartTime = GetGame().GetWorld().GetWorldTime();
		m_bIsFlying = true;
		m_bHasDropped = false;

		// Set initial position and rotation
		IEntity owner = GetOwner();
		if (owner)
		{
			owner.SetOrigin(startPos);

			vector angles = m_vFlightDirection.VectorToAngles();
			owner.SetYawPitchRoll(Vector(angles[0], angles[1], angles[2]));
		}

		// Replicate to clients
		Replication.BumpMe();

		Print(string.Format("AirDropPlane: Flight path set, distance: %1m", Math.Round(m_fTotalFlightDistance)), LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	// Called on clients when flight data replicates
	protected void OnFlightDataChanged()
	{
		if (m_vStartPos == vector.Zero || m_vEndPos == vector.Zero)
			return;

		m_vFlightDirection = vector.Direction(m_vStartPos, m_vEndPos).Normalized();
		m_fTotalFlightDistance = vector.Distance(m_vStartPos, m_vEndPos);

		// Set rotation on client
		IEntity owner = GetOwner();
		if (owner)
		{
			vector angles = m_vFlightDirection.VectorToAngles();
			owner.SetYawPitchRoll(Vector(angles[0], angles[1], angles[2]));
		}
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		SetEventMask(owner, EntityEvent.INIT | EntityEvent.FRAME);
	}

	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);

		// Server starts flight via SetFlightPath called from AirDropGameSystem
		// Client will receive replicated data and update in EOnFrame
	}

	//------------------------------------------------------------------------------------------------
	// Use EOnFrame for smooth interpolated movement on both server and client
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (!m_bIsFlying)
			return;

		if (m_vEndPos == vector.Zero)
			return;

		// Calculate current position based on time elapsed
		float currentTime = GetGame().GetWorld().GetWorldTime();
		float elapsedTime = (currentTime - m_fFlightStartTime) / 1000.0; // Convert to seconds
		float distanceTraveled = elapsedTime * m_fFlightSpeed;

		// Calculate new position
		vector newPos = m_vStartPos + (m_vFlightDirection * distanceTraveled);
		owner.SetOrigin(newPos);

		// Server-only logic
		if (Replication.IsServer())
		{
			// Check if we should drop the crate
			if (!m_bHasDropped)
			{
				float distToDrop = vector.DistanceXZ(newPos, m_vTargetDropPos);
				if (distToDrop < m_fFlightSpeed * 0.15)
				{
					DropAirdrop();
				}
			}

			// Check if past end point
			if (distanceTraveled >= m_fTotalFlightDistance)
			{
				DestroyPlane();
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void DropAirdrop()
	{
		if (m_bHasDropped || m_AirdropPrefab.IsEmpty())
			return;

		m_bHasDropped = true;
		Replication.BumpMe();

		IEntity owner = GetOwner();
		if (!owner)
			return;

		Resource resource = Resource.Load(m_AirdropPrefab);
		if (!resource)
		{
			Print("AirDropPlane: Failed to load prefab!", LogLevel.ERROR);
			return;
		}

		vector dropPos = owner.GetOrigin();

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		params.Transform[3] = dropPos;

		IEntity airdrop = GetGame().SpawnEntityPrefab(resource, owner.GetWorld(), params);
		if (airdrop)
			Print(string.Format("AirDropPlane: Dropped at [%1, %2, %3]", Math.Round(dropPos[0]), Math.Round(dropPos[1]), Math.Round(dropPos[2])), LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	protected void DestroyPlane()
	{
		m_bIsFlying = false;

		IEntity owner = GetOwner();
		if (owner)
		{
			Print("AirDropPlane: Despawning", LogLevel.NORMAL);
			RplComponent.DeleteRplEntity(owner, false);
		}
	}
}
