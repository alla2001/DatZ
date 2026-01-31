// DatZ Footstep Listener - Tracks player movement and emits footstep sounds
// Zombies can hear players running/sprinting nearby
// Slow walking and crouch walking are silent - only fast movement makes noise

[EntityEditorProps(category: "GameScripted/DatZ", description: "Listens for player footsteps and notifies zombie hearing - add to GameMode")]
class DatZFootstepListenerClass : SCR_BaseGameModeComponentClass
{
}

class DatZFootstepListener : SCR_BaseGameModeComponent
{
	// How often to check player movement (seconds)
	protected static const float CHECK_INTERVAL = 0.5;

	// Speed thresholds - walking below this is silent
	protected static const float SILENT_SPEED_THRESHOLD = 2.5;  // Below this = silent
	protected static const float MAX_SPEED_REFERENCE = 8.0;     // Reference max speed for loudness scaling

	// Track last footstep time per player to avoid spam
	protected ref map<int, float> m_LastFootstepTime = new map<int, float>();

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!Replication.IsServer())
			return;

		// Start periodic check
		GetGame().GetCallqueue().CallLater(CheckPlayerMovement, CHECK_INTERVAL * 1000, true);
	}

	//------------------------------------------------------------------------------------------------
	void ~DatZFootstepListener()
	{
		GetGame().GetCallqueue().Remove(CheckPlayerMovement);
	}

	//------------------------------------------------------------------------------------------------
	protected void CheckPlayerMovement()
	{
		if (!Replication.IsServer())
			return;

		DatZSoundManager soundMgr = DatZSoundManager.GetInstance();
		if (!soundMgr)
			return;

		PlayerManager playerMgr = GetGame().GetPlayerManager();
		if (!playerMgr)
			return;

		array<int> playerIds = {};
		playerMgr.GetPlayers(playerIds);

		float currentTime = GetGame().GetWorld().GetWorldTime() / 1000.0;

		foreach (int playerId : playerIds)
		{
			IEntity playerEnt = playerMgr.GetPlayerControlledEntity(playerId);
			if (!playerEnt)
				continue;

			ChimeraCharacter character = ChimeraCharacter.Cast(playerEnt);
			if (!character)
				continue;

			CharacterControllerComponent controller = character.GetCharacterController();
			if (!controller)
				continue;

			// Skip if dead
			if (controller.IsDead())
				continue;

			// Get movement info
			float speed = controller.GetMovementSpeed();
			ECharacterStance stance = controller.GetStance();
			int movementPhase = controller.GetCurrentMovementPhase();

			// SILENT CONDITIONS:
			// - Slow walking (standing + WALK) is silent
			// - Crouch walking (crouch + WALK) is silent
			// - Prone is always silent (too slow to matter)
			// - Any speed below threshold is silent

			if (speed < SILENT_SPEED_THRESHOLD)
				continue;

			if (stance == ECharacterStance.PRONE)
				continue; // Prone is always silent

			if (stance == ECharacterStance.CROUCH && movementPhase == EMovementType.WALK)
				continue; // Crouch walking is silent

			if (stance == ECharacterStance.STAND && movementPhase == EMovementType.WALK)
				continue; // Slow walking is silent

			// Determine sound type based on stance and movement phase
			int soundType = -1;
			float cooldown = 0;

			if (stance == ECharacterStance.CROUCH)
			{
				// Crouch running/sprinting - audible
				if (movementPhase == EMovementType.SPRINT)
				{
					soundType = DatZSoundType.FOOTSTEP_RUN;
					cooldown = 0.4;
				}
				else if (movementPhase == EMovementType.RUN)
				{
					soundType = DatZSoundType.FOOTSTEP_CROUCH;
					cooldown = 0.6;
				}
			}
			else // Standing
			{
				switch (movementPhase)
				{
					case EMovementType.SPRINT:
					{
						soundType = DatZSoundType.FOOTSTEP_SPRINT;
						cooldown = 0.3;
						break;
					}
					case EMovementType.RUN:
					{
						soundType = DatZSoundType.FOOTSTEP_RUN;
						cooldown = 0.5;
						break;
					}
					default:
					{
						continue;
					}
				}
			}

			if (soundType < 0)
				continue;

			// Check cooldown
			float lastTime = 0;
			if (m_LastFootstepTime.Contains(playerId))
				lastTime = m_LastFootstepTime.Get(playerId);

			if (currentTime - lastTime < cooldown)
				continue;

			// Calculate loudness based on speed
			// Faster movement = louder footsteps = greater hearing range
			float loudness = speed / MAX_SPEED_REFERENCE;
			loudness = Math.Clamp(loudness, 0.1, 2.0);

			// Emit footstep sound with speed-based loudness
			vector playerPos = character.GetOrigin();
			soundMgr.EmitSound(playerPos, soundType, loudness);

			// Update last time
			m_LastFootstepTime.Set(playerId, currentTime);
		}
	}
}
