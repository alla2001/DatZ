class WaterCheckerClass : ScriptComponentClass
{
}

class WaterChecker : ScriptComponent
{
	[Attribute("Path/To/YourEffect.ptc")]
	ResourceName m_sParticleEffect;

	[Attribute()]
	float forwardOffset;

	IEntity m_pParticle;
	DatZMetabolsimHandler playerMetab;

	[RplProp()]
	bool canDrink;

	private ChimeraAIControlComponent AI;
	private bool m_bLastCanDrink = false;

	// Cached values
	private Animation m_CachedAnim;
	private int m_iHeadBoneIndex = -1;
	private IEntity m_pParent;

	// Movement tracking for smart updates
	private vector m_vLastPosition;
	private float m_fTimeSinceLastCheck = 0;
	private static const float CHECK_INTERVAL_MOVING = 500;    // Check every 500ms when moving
	private static const float CHECK_INTERVAL_IDLE = 2000;     // Check every 2s when idle
	private static const float MOVEMENT_THRESHOLD = 0.5;       // Consider moving if moved > 0.5m

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!Replication.IsServer())
			return;

		m_pParent = owner.GetParent();
		if (!m_pParent)
			return;

		playerMetab = DatZMetabolsimHandler.Cast(m_pParent.FindComponent(DatZMetabolsimHandler));
		AI = ChimeraAIControlComponent.Cast(m_pParent.FindComponent(ChimeraAIControlComponent));

		// Cache animation and bone index
		m_CachedAnim = m_pParent.GetAnimation();
		if (m_CachedAnim)
			m_iHeadBoneIndex = m_CachedAnim.GetBoneIndex("Head");

		// Spawn the particle effect
		if (m_sParticleEffect != "")
			m_pParticle = GetGame().SpawnEntityPrefab(Resource.Load(m_sParticleEffect));

		// Store initial position
		m_vLastPosition = m_pParent.GetOrigin();

		// Start with moving interval (will adjust dynamically)
		GetGame().GetCallqueue().CallLater(CheckWaterSmart, CHECK_INTERVAL_MOVING, false);
	}

	override void OnDelete(IEntity owner)
	{
		GetGame().GetCallqueue().Remove(CheckWaterSmart);
		super.OnDelete(owner);
	}

	void CheckWaterSmart()
	{
		IEntity owner = GetOwner();
		if (!owner || !m_pParent)
		{
			// Reschedule and return
			GetGame().GetCallqueue().CallLater(CheckWaterSmart, CHECK_INTERVAL_IDLE, false);
			return;
		}

		// Check if player is moving
		vector currentPos = m_pParent.GetOrigin();
		float distMoved = vector.Distance(currentPos, m_vLastPosition);
		bool isMoving = distMoved > MOVEMENT_THRESHOLD;
		m_vLastPosition = currentPos;

		// Determine next check interval based on movement
		float nextInterval = CHECK_INTERVAL_IDLE;
		if (isMoving)
			nextInterval = CHECK_INTERVAL_MOVING;

		// Skip AI characters
		if (AI && AI.IsAIActivated())
		{
			GetGame().GetCallqueue().CallLater(CheckWaterSmart, nextInterval, false);
			return;
		}

		// ALWAYS update entity position to follow player (required for action interaction)
		UpdateEntityPosition(owner);

		// Do water state check
		DoWaterCheck(owner);

		// Schedule next check
		GetGame().GetCallqueue().CallLater(CheckWaterSmart, nextInterval, false);
	}

	// Update entity position to follow player head - must always run so action is interactable
	void UpdateEntityPosition(IEntity owner)
	{
		if (!m_CachedAnim || m_iHeadBoneIndex == -1)
			return;

		vector mat[4];
		if (!m_CachedAnim.GetBoneMatrix(m_iHeadBoneIndex, mat))
			return;

		// Invert rotation as needed
		mat[0] = -mat[0];
		mat[2] = -mat[2];

		mat[3] = mat[3] + (mat[2] * forwardOffset);
		mat[3] = mat[3] - (mat[1] * 0.12); // Downward

		owner.SetLocalTransform(mat);
		owner.Update();
	}

	void DoWaterCheck(IEntity owner)
	{
		// Check water state (position already updated by UpdateEntityPosition)
		EWaterSurfaceType waterSurfaceType;
		float lake;
		SCR_WorldTools.GetWaterSurfaceY(GetGame().GetWorld(), owner.GetOrigin(), waterSurfaceType, lake);

		bool isInDrinkableWater = SCR_WorldTools.IsObjectUnderwater(owner) && waterSurfaceType != EWaterSurfaceType.WST_OCEAN;

		// Only update if state changed
		if (isInDrinkableWater != m_bLastCanDrink)
		{
			m_bLastCanDrink = isInDrinkableWater;
			canDrink = isInDrinkableWater;

			// Update particle position when entering water
			if (canDrink && m_pParticle)
			{
				vector mat[4];
				owner.GetWorldTransform(mat);
				m_pParticle.SetWorldTransform(mat);
				m_pParticle.Update();
			}

			// Send RPC only on state change
			Rpc(RPC_UpdateCanDrink, canDrink);
			Replication.BumpMe();
		}
	}

	bool CanDrink()
	{
		return canDrink;
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	private void RPC_UpdateCanDrink(bool valcanDrink)
	{
		canDrink = valcanDrink;
	}
}
