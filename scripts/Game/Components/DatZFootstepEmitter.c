// DatZ Footstep Emitter - Tracks character movement and emits sounds for zombie hearing
// Add this component to player characters

[EntityEditorProps(category: "GameScripted/DatZ", description: "Emits footstep sounds for zombie hearing")]
class DatZFootstepEmitterClass : ScriptComponentClass
{
}

class DatZFootstepEmitter : ScriptComponent
{
	// Footstep intervals based on movement type (in meters)
	protected static const float STEP_INTERVAL_PRONE = 1.5;
	protected static const float STEP_INTERVAL_CROUCH = 1.2;
	protected static const float STEP_INTERVAL_WALK = 0.8;
	protected static const float STEP_INTERVAL_RUN = 0.6;
	protected static const float STEP_INTERVAL_SPRINT = 0.5;

	// Tracking
	protected vector m_vLastStepPos;
	protected float m_fDistanceSinceStep;
	protected CharacterControllerComponent m_Controller;
	protected ChimeraCharacter m_Character;

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		m_Character = ChimeraCharacter.Cast(owner);
		if (m_Character)
			m_Controller = m_Character.GetCharacterController();

		m_vLastStepPos = owner.GetOrigin();

		// Only run on server
		if (Replication.IsServer())
			GetGame().GetCallqueue().CallLater(UpdateFootsteps, 100, false);
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateFootsteps()
	{
		if (!Replication.IsServer())
			return;

		IEntity owner = GetOwner();
		if (!owner || !m_Controller)
		{
			GetGame().GetCallqueue().CallLater(UpdateFootsteps, 500, false);
			return;
		}

		// Don't emit for dead characters
		SCR_CharacterControllerComponent scrController = SCR_CharacterControllerComponent.Cast(m_Controller);
		if (scrController && scrController.GetLifeState() == ECharacterLifeState.DEAD)
		{
			GetGame().GetCallqueue().CallLater(UpdateFootsteps, 500, false);
			return;
		}

		// Don't emit for zombies
		if (DatZUndeadBase.Cast(owner))
		{
			GetGame().GetCallqueue().CallLater(UpdateFootsteps, 500, false);
			return;
		}

		// Check if moving
		float speed = m_Controller.GetMovementSpeed();
		if (speed < 0.5)
		{
			// Not moving, reset position
			m_vLastStepPos = owner.GetOrigin();
			GetGame().GetCallqueue().CallLater(UpdateFootsteps, 200, false);
			return;
		}

		// Calculate distance traveled
		vector currentPos = owner.GetOrigin();
		float distance = vector.Distance(m_vLastStepPos, currentPos);

		// Get step interval based on movement type
		float stepInterval = GetStepInterval();

		// Check if we've traveled far enough for a step
		if (distance >= stepInterval)
		{
			EmitFootstep(currentPos);
			m_vLastStepPos = currentPos;
		}

		// Continue updating
		GetGame().GetCallqueue().CallLater(UpdateFootsteps, 100, false);
	}

	//------------------------------------------------------------------------------------------------
	protected float GetStepInterval()
	{
		if (!m_Controller)
			return STEP_INTERVAL_WALK;

		ECharacterStance stance = m_Controller.GetStance();

		if (stance == ECharacterStance.PRONE)
			return STEP_INTERVAL_PRONE;

		if (stance == ECharacterStance.CROUCH)
			return STEP_INTERVAL_CROUCH;

		float speed = m_Controller.GetMovementSpeed();

		if (speed > 5.5)
			return STEP_INTERVAL_SPRINT;
		else if (speed > 3.5)
			return STEP_INTERVAL_RUN;

		return STEP_INTERVAL_WALK;
	}

	//------------------------------------------------------------------------------------------------
	protected void EmitFootstep(vector position)
	{
		DatZSoundManager soundMgr = DatZSoundManager.GetInstance();
		if (!soundMgr)
			return;

		int soundType = DatZSoundType.FOOTSTEP_WALK;

		if (m_Controller)
		{
			ECharacterStance stance = m_Controller.GetStance();

			if (stance == ECharacterStance.PRONE)
			{
				soundType = DatZSoundType.FOOTSTEP_PRONE;
			}
			else if (stance == ECharacterStance.CROUCH)
			{
				soundType = DatZSoundType.FOOTSTEP_CROUCH;
			}
			else
			{
				float speed = m_Controller.GetMovementSpeed();

				if (speed > 5.5)
					soundType = DatZSoundType.FOOTSTEP_SPRINT;
				else if (speed > 3.5)
					soundType = DatZSoundType.FOOTSTEP_RUN;
				else
					soundType = DatZSoundType.FOOTSTEP_WALK;
			}
		}

		// Emit the footstep sound
		soundMgr.EmitSound(position, soundType, 1.0);
	}
}
