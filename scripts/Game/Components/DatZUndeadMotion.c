// DatZ Undead Motion - Movement modifier for undead entities
// Adds shambling, erratic movement, and stumble behaviors

[EntityEditorProps(category: "GameScripted/DatZ", description: "Undead shambling movement")]
class DatZUndeadMotionClass : ScriptComponentClass
{
}

class DatZUndeadMotion : ScriptComponent
{
	[Attribute("0.3", UIWidgets.Slider, "Direction wobble intensity", "0 1 0.05")]
	protected float m_fWobbleIntensity;

	[Attribute("0.8", UIWidgets.Slider, "Wobble frequency", "0.1 3 0.1")]
	protected float m_fWobbleFrequency;

	[Attribute("0.15", UIWidgets.Slider, "Speed variation range", "0 0.5 0.05")]
	protected float m_fSpeedVariation;

	[Attribute("0.05", UIWidgets.Slider, "Chance to stumble per second", "0 0.3 0.01")]
	protected float m_fStumbleChance;

	[Attribute("0.7", UIWidgets.Slider, "Stumble duration", "0.3 2 0.1")]
	protected float m_fStumbleDuration;

	[Attribute("true", UIWidgets.CheckBox, "Enable head tracking toward target")]
	protected bool m_bEnableHeadTracking;

	// Runtime state
	protected float m_fWobbleOffset;
	protected float m_fCurrentSpeedMod;
	protected bool m_bIsStumbling;
	protected float m_fStumbleEndTime;

	// Cached components
	protected CharacterControllerComponent m_Controller;
	protected CharacterAnimationComponent m_Animation;
	protected DatZUndeadBase m_Undead;

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		m_Controller = CharacterControllerComponent.Cast(owner.FindComponent(CharacterControllerComponent));
		m_Animation = CharacterAnimationComponent.Cast(owner.FindComponent(CharacterAnimationComponent));
		m_Undead = DatZUndeadBase.Cast(owner);

		// Initialize random offsets
		m_fWobbleOffset = Math.RandomFloat(0, Math.PI2);
		m_fCurrentSpeedMod = 1.0;

		// Start motion loop on server
		if (Replication.IsServer())
			GetGame().GetCallqueue().CallLater(MotionUpdate, 100, false);
	}

	//------------------------------------------------------------------------------------------------
	protected void MotionUpdate()
	{
		if (!Replication.IsServer())
			return;

		IEntity owner = GetOwner();
		if (!owner)
			return;

		float currentTime = GetGame().GetWorld().GetWorldTime() / 1000.0;

		// Handle stumble state
		if (m_bIsStumbling)
		{
			if (currentTime > m_fStumbleEndTime)
				EndStumble();
		}
		else
		{
			// Random stumble check
			if (Math.RandomFloat(0, 1) < m_fStumbleChance * 0.1) // Scaled for 100ms interval
			{
				TriggerStumble(currentTime);
			}
		}

		// Update speed variation periodically
		if (Math.RandomFloat(0, 1) < 0.1)
		{
			m_fCurrentSpeedMod = 1.0 + Math.RandomFloat(-m_fSpeedVariation, m_fSpeedVariation);
		}

		// Head tracking
		if (m_bEnableHeadTracking && m_Undead)
		{
			UpdateHeadTracking();
		}

		// Schedule next update
		GetGame().GetCallqueue().CallLater(MotionUpdate, 100, false);
	}

	//------------------------------------------------------------------------------------------------
	// Call this when applying movement to modify direction/speed
	vector ModifyMovement(vector desiredDirection, float desiredSpeed)
	{
		if (m_bIsStumbling)
		{
			// Very slow, erratic movement during stumble
			desiredSpeed = desiredSpeed * 0.2;
			desiredDirection = desiredDirection + GetRandomWobble() * 3.0;
		}
		else
		{
			// Normal wobble
			desiredDirection = desiredDirection + GetRandomWobble();
			desiredSpeed = desiredSpeed * m_fCurrentSpeedMod;
		}

		return desiredDirection.Normalized() * desiredSpeed;
	}

	//------------------------------------------------------------------------------------------------
	protected vector GetRandomWobble()
	{
		float time = GetGame().GetWorld().GetWorldTime() / 1000.0;

		// Sine wave based wobble
		float wobbleX = Math.Sin((time + m_fWobbleOffset) * m_fWobbleFrequency) * m_fWobbleIntensity;
		float wobbleZ = Math.Cos((time + m_fWobbleOffset * 1.3) * m_fWobbleFrequency * 0.7) * m_fWobbleIntensity;

		return Vector(wobbleX, 0, wobbleZ);
	}

	//------------------------------------------------------------------------------------------------
	protected void TriggerStumble(float currentTime)
	{
		m_bIsStumbling = true;
		m_fStumbleEndTime = currentTime + m_fStumbleDuration;

		// Play stumble animation
		if (m_Animation)
		{
			int cmdStumble = m_Animation.BindCommand("CMD_Hit");
			if (cmdStumble >= 0)
			{
				Rpc(RpcDo_Stumble);
				RpcDo_Stumble();
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_Stumble()
	{
		if (m_Animation)
		{
			int cmdStumble = m_Animation.BindCommand("CMD_Hit");
			if (cmdStumble >= 0)
				m_Animation.CallCommand(cmdStumble, 1, 0);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void EndStumble()
	{
		m_bIsStumbling = false;
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateHeadTracking()
	{
		if (!m_Undead || !m_Animation)
			return;

		IEntity prey = m_Undead.GetCurrentPrey();
		if (!prey)
			return;

		// Calculate look direction
		vector myPos = GetOwner().GetOrigin() + Vector(0, 1.5, 0); // Eye height
		vector targetPos = prey.GetOrigin() + Vector(0, 1.0, 0);
		vector lookDir = (targetPos - myPos).Normalized();

		// Convert to angles
		float pitch = -Math.Asin(lookDir[1]) * Math.RAD2DEG;
		float yaw = Math.Atan2(lookDir[0], lookDir[2]) * Math.RAD2DEG;

		// Clamp pitch
		pitch = Math.Clamp(pitch, -30, 45);

		// Apply to animation (if supported)
		// This depends on the character animation setup
	}

	//------------------------------------------------------------------------------------------------
	bool IsStumbling() { return m_bIsStumbling; }
	float GetSpeedModifier() { return m_fCurrentSpeedMod; }
}
