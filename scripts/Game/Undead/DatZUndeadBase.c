// DatZ Undead System - Core Base Class
// Zombie entity with vision cone and hearing

[EntityEditorProps(category: "GameScripted/DatZ", description: "Base undead entity")]
class DatZUndeadBaseClass : ChimeraCharacterClass
{
}

class DatZUndeadBase : ChimeraCharacter
{
	// Movement attributes
	[Attribute("1", UIWidgets.CheckBox, "Can this undead run")]
	protected bool m_bCanSprint;

	[Attribute("17.5", UIWidgets.Slider, "Walking movement speed", "1 50 0.5")]
	protected float m_fWalkSpeed;

	[Attribute("30.0", UIWidgets.Slider, "Running movement speed", "3 60 0.5")]
	protected float m_fSprintSpeed;

	[Attribute("180", UIWidgets.Slider, "Turn speed in degrees per second", "15 360 5")]
	protected float m_fTurnSpeed;

	[Attribute("360", UIWidgets.Slider, "Turn speed when aggro (degrees per second)", "30 720 10")]
	protected float m_fAggroTurnSpeed;

	// Combat attributes
	[Attribute("15", UIWidgets.Slider, "Melee attack damage", "5 100 1")]
	protected float m_fAttackDamage;

	[Attribute("1.5", UIWidgets.Slider, "Attack reach distance", "0.5 4 0.1")]
	protected float m_fAttackRange;

	[Attribute("1.2", UIWidgets.Slider, "Time between attacks", "0.5 5 0.1")]
	protected float m_fAttackCooldown;

	[Attribute("0.7", UIWidgets.Slider, "Chance to stumble when hit", "0 1 0.05")]
	protected float m_fStumbleChance;

	// Vision attributes
	[Attribute("1", UIWidgets.CheckBox, "Enable visual detection (disable for blind undead)")]
	protected bool m_bCanSee;

	[Attribute("50", UIWidgets.Slider, "Maximum visual detection range", "10 200 5")]
	protected float m_fVisionRange;

	[Attribute("120", UIWidgets.Slider, "Field of view angle (degrees)", "30 180 10")]
	protected float m_fVisionFOV;

	// Hearing attributes
	[Attribute("1.0", UIWidgets.Slider, "Hearing sensitivity multiplier", "0.5 2 0.1")]
	protected float m_fHearingSensitivity;

	// Idle behavior
	[Attribute("3", UIWidgets.Slider, "Min time between idle look around", "1 10 0.5")]
	protected float m_fIdleLookMinTime;

	[Attribute("8", UIWidgets.Slider, "Max time between idle look around", "5 20 1")]
	protected float m_fIdleLookMaxTime;

	[Attribute("90", UIWidgets.Slider, "Max angle to look around when idle", "30 180 10")]
	protected float m_fIdleLookAngle;

	// Debug
	[Attribute("0", UIWidgets.CheckBox, "Draw debug visualization")]
	protected bool m_bDebugDraw;

	// Runtime state
	protected IEntity m_CurrentPrey;
	protected float m_fLastAttackTime;
	protected bool m_bIsAggro;
	protected bool m_bIsStumbling;
	protected vector m_vLastKnownPreyPos;
	protected float m_fAggroLostTime;
	protected bool m_bIsDead;

	// Rotation state
	protected float m_fCurrentHeading;
	protected float m_fTargetHeading;
	protected float m_fLastThinkTime;

	// Idle look state
	protected float m_fNextIdleLookTime;
	protected float m_fIdleLookTargetHeading;

	// Cached components
	protected CharacterControllerComponent m_CharController;
	protected SCR_CharacterDamageManagerComponent m_DmgManager;
	protected FactionAffiliationComponent m_FactionComp;
	protected AIControlComponent m_AIControl;
	protected CharacterAnimationComponent m_AnimComp;
	protected CharacterAimingComponent m_AimingComp;
	protected ref DatZUndeadBrain m_Brain;

	// Constants
	protected static const float AGGRO_MEMORY_DURATION = 8.0;
	protected static const float STUMBLE_DURATION = 0.8;
	protected static const float MAX_VALID_TARGET_DISTANCE = 1000.0;

	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);

		// Cache components
		m_CharController = GetCharacterController();
		m_DmgManager = SCR_CharacterDamageManagerComponent.Cast(GetDamageManager());
		m_FactionComp = FactionAffiliationComponent.Cast(FindComponent(FactionAffiliationComponent));
		m_AIControl = GetAIControlComponent();
		m_AnimComp = GetAnimationComponent();
		if (m_CharController)
			m_AimingComp = m_CharController.GetAimingComponent();

		// Initialize heading from current rotation
		vector mat[4];
		GetWorldTransform(mat);
		m_fCurrentHeading = Math.Atan2(mat[2][0], mat[2][2]);
		m_fTargetHeading = m_fCurrentHeading;

		// Initialize last known position to spawn point (prevents 0,0,0 issues)
		m_vLastKnownPreyPos = GetOrigin();

		// Hook into life state changes
		SCR_CharacterControllerComponent scrController = SCR_CharacterControllerComponent.Cast(m_CharController);
		if (scrController)
			scrController.m_OnLifeStateChanged.Insert(OnLifeStateChanged);

		if (!Replication.IsServer())
			return;

		// Deactivate base Arma AI - we use our own brain
		if (m_AIControl)
			m_AIControl.DeactivateAI();

		// Create brain for AI decisions
		m_Brain = new DatZUndeadBrain(this);

		// Register with coordinator
		DatZUndeadCoordinator coordinator = DatZUndeadCoordinator.GetInstance();
		if (coordinator)
			coordinator.RegisterUndead(this);

		// Register for sound events
		DatZSoundManager soundMgr = DatZSoundManager.GetInstance();
		if (soundMgr)
			soundMgr.RegisterListener(this);

		// Initialize idle look timer
		float currentTime = GetGame().GetWorld().GetWorldTime() / 1000.0;
		m_fLastThinkTime = currentTime;
		m_fNextIdleLookTime = currentTime + Math.RandomFloat(m_fIdleLookMinTime, m_fIdleLookMaxTime);

		// Start thinking with random offset to spread load
		GetGame().GetCallqueue().CallLater(ThinkCycle, 100 + Math.RandomInt(0, 200), false);
	}

	//------------------------------------------------------------------------------------------------
	void ~DatZUndeadBase()
	{
		SCR_CharacterControllerComponent scrController = SCR_CharacterControllerComponent.Cast(m_CharController);
		if (scrController)
			scrController.m_OnLifeStateChanged.Remove(OnLifeStateChanged);

		DatZUndeadCoordinator coordinator = DatZUndeadCoordinator.GetInstance();
		if (coordinator)
			coordinator.UnregisterUndead(this);

		DatZSoundManager soundMgr = DatZSoundManager.GetInstance();
		if (soundMgr)
			soundMgr.UnregisterListener(this);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnLifeStateChanged(ECharacterLifeState previousState, ECharacterLifeState newState)
	{
		if (newState == ECharacterLifeState.DEAD)
		{
			m_bIsDead = true;
			if (m_AIControl)
				m_AIControl.DeactivateAI();
			GetGame().GetCallqueue().Remove(ThinkCycle);
		}
		else if (newState == ECharacterLifeState.ALIVE && previousState == ECharacterLifeState.INCAPACITATED)
		{
			m_bIsDead = false;
			// Restart our custom brain (don't use base AI)
			GetGame().GetCallqueue().CallLater(ThinkCycle, 100, false);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void ThinkCycle()
	{
		if (!Replication.IsServer() || m_bIsDead)
			return;

		// Don't think when unconscious/knocked out
		if (m_CharController && m_CharController.IsUnconscious())
		{
			GetGame().GetCallqueue().CallLater(ThinkCycle, 500, false);
			return;
		}

		float currentTime = GetGame().GetWorld().GetWorldTime() / 1000.0;
		float deltaTime = currentTime - m_fLastThinkTime;
		m_fLastThinkTime = currentTime;

		if (m_bIsStumbling)
		{
			GetGame().GetCallqueue().CallLater(ThinkCycle, 100, false);
			return;
		}

		// Update prey tracking
		UpdatePreyTracking(currentTime);

		// Update rotation smoothly
		UpdateRotation(deltaTime);

		// Update idle behavior (only in IDLE state, not during wander/investigate)
		if (!m_bIsAggro && m_Brain && m_Brain.GetCurrentState() == DatZUndeadState.IDLE)
			UpdateIdleBehavior(currentTime);

		// Execute brain behavior
		if (m_Brain)
			m_Brain.Execute(currentTime);

		// Debug visualization
		if (m_bDebugDraw)
			DrawDebug();

		// Adaptive think rate
		int nextThink = 500;
		if (m_bIsAggro)
			nextThink = 100;
		GetGame().GetCallqueue().CallLater(ThinkCycle, nextThink, false);
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateRotation(float deltaTime)
	{
		if (!m_CharController)
			return;

		// Calculate angle difference
		float angleDiff = m_fTargetHeading - m_fCurrentHeading;

		// Normalize to -PI to PI
		while (angleDiff > Math.PI)
			angleDiff -= Math.PI2;
		while (angleDiff < -Math.PI)
			angleDiff += Math.PI2;

		// Use faster turn speed when aggro
		float turnSpeed = m_fTurnSpeed;
		if (m_bIsAggro)
			turnSpeed = m_fAggroTurnSpeed;

		float maxTurn = turnSpeed * Math.DEG2RAD * deltaTime;

		// Smooth rotation
		if (Math.AbsFloat(angleDiff) <= maxTurn)
			m_fCurrentHeading = m_fTargetHeading;
		else if (angleDiff > 0)
			m_fCurrentHeading += maxTurn;
		else
			m_fCurrentHeading -= maxTurn;

		// Normalize heading
		while (m_fCurrentHeading > Math.PI)
			m_fCurrentHeading -= Math.PI2;
		while (m_fCurrentHeading < -Math.PI)
			m_fCurrentHeading += Math.PI2;

		// Apply body heading
		m_CharController.SetHeadingAngle(m_fCurrentHeading, false);

		// Also set aim/look direction to match
		if (m_AimingComp)
		{
			vector aimRotation = Vector(m_fCurrentHeading, 0, 0);
			m_AimingComp.SetAimingRotation(aimRotation);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateIdleBehavior(float currentTime)
	{
		// Time for a new idle look?
		if (currentTime >= m_fNextIdleLookTime)
		{
			// Pick a random direction to look
			float randomAngle = Math.RandomFloat(-m_fIdleLookAngle, m_fIdleLookAngle) * Math.DEG2RAD;
			m_fTargetHeading = m_fCurrentHeading + randomAngle;

			// Schedule next idle look
			m_fNextIdleLookTime = currentTime + Math.RandomFloat(m_fIdleLookMinTime, m_fIdleLookMaxTime);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdatePreyTracking(float currentTime)
	{
		// First, always try to find visual targets (vision has priority over hearing)
		if (!m_CurrentPrey)
			ScanForPrey();

		if (m_CurrentPrey)
		{
			// Validate prey is still valid
			if (!IsValidPrey(m_CurrentPrey))
			{
				// Target died or became invalid - clear prey but keep investigating
				m_CurrentPrey = null;
				// Reset last known pos to current position if we're right on top of dead target
				vector myPos = GetOrigin();
				float distToLast = vector.Distance(myPos, m_vLastKnownPreyPos);
				if (distToLast < 3.0)
				{
					// Look around for other targets, don't just stand on corpse
					LoseAggro();
				}
			}
			else
			{
				// Check if prey is too far - clear reference so we can investigate or give up
				vector preyPos = m_CurrentPrey.GetOrigin();
				float distToPrey = vector.Distance(GetOrigin(), preyPos);

				if (distToPrey > m_fVisionRange * 2)
				{
					// Prey too far - forget them, keep last known position for investigation
					m_CurrentPrey = null;
				}
				else if (CanSeePrey(m_CurrentPrey))
				{
					// Update last known position if we can see them (validate first)
					if (IsValidTargetPosition(preyPos))
					{
						m_vLastKnownPreyPos = preyPos;
						m_fAggroLostTime = currentTime;
					}
				}
			}
		}

		// Check aggro timeout
		if (!m_CurrentPrey && m_bIsAggro)
		{
			if (currentTime - m_fAggroLostTime > AGGRO_MEMORY_DURATION)
				LoseAggro();
		}

		// Validate current last known position - if invalid, lose aggro
		if (m_bIsAggro && !IsValidTargetPosition(m_vLastKnownPreyPos))
			LoseAggro();
	}

	//------------------------------------------------------------------------------------------------
	protected void ScanForPrey()
	{
		// Skip visual scan if blind or already have a prey
		if (!m_bCanSee || m_CurrentPrey)
			return;

		vector myPos = GetOrigin();
		GetGame().GetWorld().QueryEntitiesBySphere(myPos, m_fVisionRange, QueryPreyCallback, FilterPreyClass, EQueryEntitiesFlags.ALL);
	}

	//------------------------------------------------------------------------------------------------
	protected bool QueryPreyCallback(IEntity ent)
	{
		if (!IsValidPrey(ent))
			return true;

		// Check if we can see them (vision cone + LOS)
		if (!CanSeePrey(ent))
			return true;

		// Found valid prey through vision
		AcquirePreyVisual(ent);
		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected bool FilterPreyClass(IEntity ent)
	{
		return ChimeraCharacter.Cast(ent) != null;
	}

	//------------------------------------------------------------------------------------------------
	// Check if target is within vision cone (no peripheral - use hearing for close detection)
	protected bool CanSeePrey(IEntity target)
	{
		if (!m_bCanSee)
			return false;

		if (!target)
			return false;

		vector myPos = GetOrigin();
		vector targetPos = target.GetOrigin();
		vector toTarget = targetPos - myPos;
		toTarget[1] = 0;
		float distance = toTarget.Length();

		// Too far or too close (avoid NaN)?
		if (distance > m_fVisionRange || distance < 0.1)
			return false;

		// Get my forward direction
		vector mat[4];
		GetWorldTransform(mat);
		vector forward = mat[2];
		forward[1] = 0;
		float forwardLen = forward.Length();
		if (forwardLen < 0.001)
			return false;
		forward = forward * (1.0 / forwardLen);

		// Safe normalize toTarget
		toTarget = toTarget * (1.0 / distance);

		// Calculate angle between forward and target direction
		float dot = vector.Dot(forward, toTarget);
		float angle = Math.Acos(Math.Clamp(dot, -1, 1)) * Math.RAD2DEG;

		// Check if within FOV cone
		float halfFOV = m_fVisionFOV * 0.5;

		if (angle <= halfFOV && HasLineOfSight(target))
			return true;

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsValidPrey(IEntity ent)
	{
		if (!ent || ent == this)
			return false;

		// Don't target other undead
		if (DatZUndeadBase.Cast(ent))
			return false;

		ChimeraCharacter char = ChimeraCharacter.Cast(ent);
		if (!char)
			return false;

		// Check if alive
		SCR_CharacterDamageManagerComponent dmg = SCR_CharacterDamageManagerComponent.Cast(char.GetDamageManager());
		if (dmg && dmg.IsDestroyed())
			return false;

		// Faction check
		FactionAffiliationComponent otherFaction = FactionAffiliationComponent.Cast(char.FindComponent(FactionAffiliationComponent));
		if (otherFaction && m_FactionComp)
		{
			Faction myFaction = m_FactionComp.GetAffiliatedFaction();
			Faction theirFaction = otherFaction.GetAffiliatedFaction();
			if (myFaction && theirFaction && myFaction == theirFaction)
				return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected bool HasLineOfSight(IEntity target)
	{
		vector start = GetOrigin() + Vector(0, 1.6, 0);
		vector end = target.GetOrigin() + Vector(0, 1.0, 0);

		TraceParam trace = new TraceParam();
		trace.Start = start;
		trace.End = end;
		trace.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
		trace.Exclude = this;
		trace.LayerMask = EPhysicsLayerPresets.Projectile;

		float result = GetGame().GetWorld().TraceMove(trace, null);

		return (result >= 0.95 || trace.TraceEnt == target);
	}

	//------------------------------------------------------------------------------------------------
	void AcquirePreyVisual(IEntity prey)
	{
		if (!prey)
			return;

		vector preyPos = prey.GetOrigin();
		if (!IsValidTargetPosition(preyPos))
			return;

		m_CurrentPrey = prey;
		m_vLastKnownPreyPos = preyPos;
		m_bIsAggro = true;
		m_fAggroLostTime = GetGame().GetWorld().GetWorldTime() / 1000.0;

		PlayAggroSound();
	}

	//------------------------------------------------------------------------------------------------
	// Validate that a position is sane (not at origin, not too far)
	protected bool IsValidTargetPosition(vector pos)
	{
		// Check for origin (0,0,0) - likely invalid/deleted entity
		if (pos[0] == 0 && pos[1] == 0 && pos[2] == 0)
			return false;

		// Check distance from us - if too far, likely invalid
		float dist = vector.Distance(GetOrigin(), pos);
		if (dist > MAX_VALID_TARGET_DISTANCE)
			return false;

		return true;
	}

	//------------------------------------------------------------------------------------------------
	void OnHearSound(vector soundPos, int soundType, float loudness)
	{
		if (m_bIsDead || !Replication.IsServer())
			return;

		// Validate sound position
		if (!IsValidTargetPosition(soundPos))
			return;

		float hearingRange = GetHearingRange(soundType, loudness);
		float distance = vector.Distance(GetOrigin(), soundPos);

		if (distance > hearingRange)
			return;

		// If we already have prey and can see them, ignore sound
		if (m_CurrentPrey && CanSeePrey(m_CurrentPrey))
			return;

		// Set investigation target
		m_vLastKnownPreyPos = soundPos;
		m_bIsAggro = true;
		m_fAggroLostTime = GetGame().GetWorld().GetWorldTime() / 1000.0;

		// Turn towards sound
		vector toSound = soundPos - GetOrigin();
		toSound[1] = 0;
		if (toSound.LengthSq() > 0.01)
		{
			toSound.Normalize();
			m_fTargetHeading = Math.Atan2(toSound[0], toSound[2]);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected float GetHearingRange(int soundType, float loudness)
	{
		float baseRange = 0;

		switch (soundType)
		{
			// Gunshots - unsuppressed
			case DatZSoundType.GUNSHOT_PISTOL:
				baseRange = 200;
				break;
			case DatZSoundType.GUNSHOT_RIFLE:
				baseRange = 400;
				break;
			case DatZSoundType.GUNSHOT_SHOTGUN:
				baseRange = 350;
				break;
			case DatZSoundType.GUNSHOT_MG:
				baseRange = 500;
				break;
			case DatZSoundType.GUNSHOT_SNIPER:
				baseRange = 600;
				break;

			// Gunshots - suppressed (20% of unsuppressed range)
			case DatZSoundType.GUNSHOT_PISTOL_SUPPRESSED:
				baseRange = 40;   // 20% of 200
				break;
			case DatZSoundType.GUNSHOT_RIFLE_SUPPRESSED:
				baseRange = 80;   // 20% of 400
				break;
			case DatZSoundType.GUNSHOT_SHOTGUN_SUPPRESSED:
				baseRange = 70;   // 20% of 350
				break;
			case DatZSoundType.GUNSHOT_MG_SUPPRESSED:
				baseRange = 100;  // 20% of 500
				break;
			case DatZSoundType.GUNSHOT_SNIPER_SUPPRESSED:
				baseRange = 120;  // 20% of 600
				break;

			// Explosions
			case DatZSoundType.EXPLOSION:
			case DatZSoundType.EXPLOSION_VEHICLE:
				baseRange = 800;
				break;
			case DatZSoundType.EXPLOSION_ROCKET:
				baseRange = 700;
				break;
			case DatZSoundType.EXPLOSION_GRENADE:
				baseRange = 500;
				break;

			// Vehicles
			case DatZSoundType.VEHICLE_ENGINE:
				baseRange = 150;
				break;
			case DatZSoundType.VEHICLE_HORN:
				baseRange = 300;
				break;
			case DatZSoundType.VEHICLE_CRASH:
				baseRange = 400;
				break;
			case DatZSoundType.VEHICLE_DOOR:
				baseRange = 30;
				break;

			// Footsteps
			case DatZSoundType.FOOTSTEP_SPRINT:
				baseRange = 25;
				break;
			case DatZSoundType.FOOTSTEP_RUN:
				baseRange = 15;
				break;
			case DatZSoundType.FOOTSTEP_WALK:
				baseRange = 8;
				break;
			case DatZSoundType.FOOTSTEP_CROUCH:
				baseRange = 4;
				break;
			case DatZSoundType.FOOTSTEP_PRONE:
				baseRange = 2;
				break;

			// Actions
			case DatZSoundType.MELEE_SWING:
			case DatZSoundType.MELEE_HIT:
				baseRange = 10;
				break;
			case DatZSoundType.DOOR_OPEN:
				baseRange = 20;
				break;
			case DatZSoundType.DOOR_BREAK:
				baseRange = 50;
				break;
			case DatZSoundType.ITEM_DROP:
			case DatZSoundType.ITEM_PICKUP:
				baseRange = 15;
				break;
			case DatZSoundType.RELOAD:
				baseRange = 20;
				break;
			case DatZSoundType.VOICE:
				baseRange = 40;
				break;

			default:
				baseRange = 10;
				break;
		}

		return baseRange * loudness * m_fHearingSensitivity;
	}

	//------------------------------------------------------------------------------------------------
	protected void LoseAggro()
	{
		m_bIsAggro = false;
		m_CurrentPrey = null;
	}

	//------------------------------------------------------------------------------------------------
	void SetMovementToward(vector worldTarget, float speed)
	{
		vector myPos = GetOrigin();
		vector worldDir = worldTarget - myPos;
		worldDir[1] = 0;
		float dist = worldDir.Length();

		if (dist < 0.5)
		{
			StopMovement();
			return;
		}

		// Safe normalize (avoid NaN)
		worldDir = worldDir * (1.0 / dist);

		if (!m_CharController)
			return;

		// Set target heading for smooth rotation (UpdateRotation will interpolate)
		m_fTargetHeading = Math.Atan2(worldDir[0], worldDir[2]);

		// Always move forward in local space - zombie faces target and walks forward
		m_CharController.SetMovement(speed, "0 0 1");
	}

	//------------------------------------------------------------------------------------------------
	void StopMovement()
	{
		if (m_CharController)
			m_CharController.SetMovement(0, vector.Zero);
	}

	//------------------------------------------------------------------------------------------------
	bool TryAttack()
	{
		if (!m_CurrentPrey)
			return false;

		float currentTime = GetGame().GetWorld().GetWorldTime() / 1000.0;
		if (currentTime - m_fLastAttackTime < m_fAttackCooldown)
			return false;

		float dist = vector.Distance(GetOrigin(), m_CurrentPrey.GetOrigin());
		if (dist > m_fAttackRange)
			return false;

		m_fLastAttackTime = currentTime;
		ExecuteAttack(m_CurrentPrey);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected void ExecuteAttack(IEntity target)
	{
		// Validate target still exists
		if (!target)
			return;

		// Trigger melee attack via CharacterController
		if (m_CharController)
			m_CharController.SetMeleeAttack(true);

		// Also apply damage directly after a short delay (animation timing)
		GetGame().GetCallqueue().CallLater(ApplyMeleeDamage, 300, false, target);

		PlayAttackSound();
	}

	//------------------------------------------------------------------------------------------------
	protected void ApplyMeleeDamage(IEntity target)
	{
		if (!target)
			return;

		// Check target is still in range
		float dist = vector.Distance(GetOrigin(), target.GetOrigin());
		if (dist > m_fAttackRange * 1.5)
			return;

		DamageManagerComponent targetDmg = DamageManagerComponent.Cast(target.FindComponent(DamageManagerComponent));
		if (!targetDmg)
			return;

		HitZone hitZone = targetDmg.GetDefaultHitZone();
		if (!hitZone)
			return;

		// Calculate hit direction
		vector targetPos = target.GetOrigin();
		vector myPos = GetOrigin();
		vector hitDir = targetPos - myPos;
		float hitDirLen = hitDir.Length();

		if (hitDirLen > 0.01)
			hitDir = hitDir * (1.0 / hitDirLen);
		else
			hitDir = Vector(0, 0, 1);

		vector hitPosDirNorm[3];
		hitPosDirNorm[0] = targetPos + Vector(0, 1, 0);
		hitPosDirNorm[1] = hitDir;
		hitPosDirNorm[2] = -hitDir;

		SCR_DamageContext damageContext = new SCR_DamageContext(
			EDamageType.MELEE,
			m_fAttackDamage,
			hitPosDirNorm,
			target,
			hitZone,
			Instigator.CreateInstigator(this),
			null,
			-1,
			-1
		);

		targetDmg.HandleDamage(damageContext);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_PlayAttack()
	{
		if (m_AnimComp)
		{
			int cmdAttack = m_AnimComp.BindCommand("CMD_Melee");
			if (cmdAttack >= 0)
				m_AnimComp.CallCommand(cmdAttack, 1, 0);
		}
	}

	//------------------------------------------------------------------------------------------------
	void OnDamageReceived(float damage, EDamageType type, IEntity instigator)
	{
		if (Math.RandomFloat(0, 1) < m_fStumbleChance)
			TriggerStumble();

		// Aggro on attacker
		if (instigator && IsValidPrey(instigator))
			AcquirePreyVisual(instigator);
	}

	//------------------------------------------------------------------------------------------------
	protected void TriggerStumble()
	{
		m_bIsStumbling = true;

		Rpc(RpcDo_PlayStumble);
		RpcDo_PlayStumble();

		GetGame().GetCallqueue().CallLater(EndStumble, STUMBLE_DURATION * 1000, false);
	}

	//------------------------------------------------------------------------------------------------
	protected void EndStumble()
	{
		m_bIsStumbling = false;
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_PlayStumble()
	{
		if (m_AnimComp)
		{
			int cmdStumble = m_AnimComp.BindCommand("CMD_Hit");
			if (cmdStumble >= 0)
				m_AnimComp.CallCommand(cmdStumble, 1, 0);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void PlayAggroSound()
	{
	}

	//------------------------------------------------------------------------------------------------
	protected void PlayAttackSound()
	{
	}

	//------------------------------------------------------------------------------------------------
	protected void DrawDebug()
	{
		vector myPos = GetOrigin() + Vector(0, 1, 0);

		// Colors
		int colorIdle = ARGB(255, 100, 100, 255);      // Blue - idle
		int colorAggro = ARGB(255, 255, 50, 50);       // Red - aggro
		int colorVision = ARGB(128, 255, 255, 0);      // Yellow - vision cone
		int colorHeading = ARGB(255, 0, 255, 0);       // Green - current heading
		int colorTarget = ARGB(255, 255, 165, 0);      // Orange - target heading

		int stateColor = colorIdle;
		if (m_bIsAggro)
			stateColor = colorAggro;

		// Draw state indicator (vertical line)
		Shape.Create(ShapeType.LINE, stateColor, ShapeFlags.NOZBUFFER, myPos, myPos + Vector(0, 2, 0));

		// Draw current heading direction
		vector headingDir = Vector(Math.Sin(m_fCurrentHeading), 0, Math.Cos(m_fCurrentHeading));
		Shape.Create(ShapeType.LINE, colorHeading, ShapeFlags.NOZBUFFER, myPos, myPos + headingDir * 3);

		// Draw target heading direction
		vector targetDir = Vector(Math.Sin(m_fTargetHeading), 0, Math.Cos(m_fTargetHeading));
		Shape.Create(ShapeType.LINE, colorTarget, ShapeFlags.NOZBUFFER, myPos, myPos + targetDir * 2);

		// Draw vision cone edges
		float halfFOV = m_fVisionFOV * 0.5 * Math.DEG2RAD;
		float leftAngle = m_fCurrentHeading - halfFOV;
		float rightAngle = m_fCurrentHeading + halfFOV;

		vector leftDir = Vector(Math.Sin(leftAngle), 0, Math.Cos(leftAngle));
		vector rightDir = Vector(Math.Sin(rightAngle), 0, Math.Cos(rightAngle));

		Shape.Create(ShapeType.LINE, colorVision, ShapeFlags.NOZBUFFER, myPos, myPos + leftDir * m_fVisionRange);
		Shape.Create(ShapeType.LINE, colorVision, ShapeFlags.NOZBUFFER, myPos, myPos + rightDir * m_fVisionRange);

		// Draw vision arc (simplified)
		int segments = 8;
		float angleStep = m_fVisionFOV * Math.DEG2RAD / segments;
		for (int i = 0; i < segments; i++)
		{
			float a1 = leftAngle + angleStep * i;
			float a2 = leftAngle + angleStep * (i + 1);

			vector p1 = myPos + Vector(Math.Sin(a1), 0, Math.Cos(a1)) * m_fVisionRange;
			vector p2 = myPos + Vector(Math.Sin(a2), 0, Math.Cos(a2)) * m_fVisionRange;

			Shape.Create(ShapeType.LINE, colorVision, ShapeFlags.NOZBUFFER, p1, p2);
		}

		// Draw line to prey or last known position
		if (m_CurrentPrey)
		{
			Shape.Create(ShapeType.LINE, colorAggro, ShapeFlags.NOZBUFFER, myPos, m_CurrentPrey.GetOrigin() + Vector(0, 1, 0));
		}
		else if (m_bIsAggro)
		{
			Shape.Create(ShapeType.LINE, colorTarget, ShapeFlags.NOZBUFFER, myPos, m_vLastKnownPreyPos + Vector(0, 1, 0));
		}
	}

	//------------------------------------------------------------------------------------------------
	bool IsAlive() { return !m_bIsDead; }
	IEntity GetCurrentPrey() { return m_CurrentPrey; }
	vector GetLastKnownPreyPos() { return m_vLastKnownPreyPos; }
	bool IsAggro() { return m_bIsAggro; }
	bool IsStumbling() { return m_bIsStumbling; }
	float GetWalkSpeed() { return m_fWalkSpeed; }
	float GetSprintSpeed() { return m_fSprintSpeed; }
	bool CanSprint() { return m_bCanSprint; }
	float GetAttackRange() { return m_fAttackRange; }
	float GetVisionRange() { return m_fVisionRange; }
	float GetVisionFOV() { return m_fVisionFOV; }
	bool CanSee() { return m_bCanSee; }
}
