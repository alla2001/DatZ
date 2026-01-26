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
	private bool m_bLastCanDrink = false; // Track previous state to detect changes

	// Cached values
	private Animation m_CachedAnim;
	private int m_iHeadBoneIndex = -1;
	private IEntity m_pParent;

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

		// Use timer instead of EOnFrame - check every 200ms
		GetGame().GetCallqueue().CallLater(CheckWater, 200, true);
	}

	override void OnDelete(IEntity owner)
	{
		GetGame().GetCallqueue().Remove(CheckWater);
		super.OnDelete(owner);
	}

	void CheckWater()
	{
		IEntity owner = GetOwner();
		if (!owner || !m_pParent)
			return;

		if (AI && AI.IsAIActivated())
			return;

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

		// Check water
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
