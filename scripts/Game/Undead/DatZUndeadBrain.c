// DatZ Undead Brain - AI Decision System
// State machine based AI for undead entities - no behavior trees needed

// State constants instead of enum
class DatZUndeadState
{
	static const int IDLE = 0;
	static const int WANDER = 1;
	static const int INVESTIGATE = 2;
	static const int CHASE = 3;
	static const int ATTACK = 4;
	static const int RETURN = 5;
}

class DatZUndeadBrain
{
	protected DatZUndeadBase m_Owner;
	protected int m_CurrentState;
	protected vector m_vWanderTarget;
	protected vector m_vHomePosition;
	protected float m_fStateStartTime;
	protected float m_fCurrentIdleDuration;

	// State config
	protected static const float IDLE_MIN_DURATION = 2.0;
	protected static const float IDLE_MAX_DURATION = 8.0;
	protected static const float WANDER_RADIUS = 25.0;
	protected static const float MAX_CHASE_DISTANCE = 100.0;
	protected static const float CHASE_TIMEOUT = 30.0;
	protected static const float INVESTIGATE_TIMEOUT = 10.0;
	protected static const float ARRIVED_THRESHOLD = 2.0;

	//------------------------------------------------------------------------------------------------
	void DatZUndeadBrain(DatZUndeadBase owner)
	{
		m_Owner = owner;
		m_CurrentState = DatZUndeadState.IDLE;
		m_vHomePosition = owner.GetOrigin();
		m_fStateStartTime = 0; // Will be set on first Execute()
		m_fCurrentIdleDuration = Math.RandomFloat(IDLE_MIN_DURATION, IDLE_MAX_DURATION);
	}

	//------------------------------------------------------------------------------------------------
	void Execute(float currentTime)
	{
		if (!m_Owner)
			return;

		// Initialize state start time on first call
		if (m_fStateStartTime == 0)
			m_fStateStartTime = currentTime;

		// Evaluate state transitions
		int nextState = EvaluateTransitions(currentTime);
		if (nextState != m_CurrentState)
		{
			ExitState(m_CurrentState);
			m_CurrentState = nextState;
			m_fStateStartTime = currentTime;
			EnterState(m_CurrentState);
		}

		// Execute current state
		ExecuteState(m_CurrentState, currentTime);
	}

	//------------------------------------------------------------------------------------------------
	protected float GetTime()
	{
		return GetGame().GetWorld().GetWorldTime() / 1000.0;
	}

	//------------------------------------------------------------------------------------------------
	protected int EvaluateTransitions(float currentTime)
	{
		IEntity prey = m_Owner.GetCurrentPrey();
		bool isAggro = m_Owner.IsAggro();
		float stateTime = currentTime - m_fStateStartTime;
		vector myPos = m_Owner.GetOrigin();

		// Get validated prey position and distance (calculate once)
		vector preyPos;
		float distToPrey = -1;
		bool hasValidPreyPos = false;
		if (prey)
		{
			preyPos = prey.GetOrigin();
			// Validate - not at origin
			if (preyPos[0] != 0 || preyPos[1] != 0 || preyPos[2] != 0)
			{
				hasValidPreyPos = true;
				distToPrey = vector.Distance(myPos, preyPos);
			}
		}

		// Priority 1: Attack if in range and have prey
		if (prey && isAggro && hasValidPreyPos)
		{
			if (distToPrey <= m_Owner.GetAttackRange())
				return DatZUndeadState.ATTACK;
		}

		// Priority 2: Chase prey (if in chase range)
		if (prey && isAggro && hasValidPreyPos)
		{
			if (distToPrey <= MAX_CHASE_DISTANCE)
				return DatZUndeadState.CHASE;
		}

		// Priority 3: Investigate last known position (heard sound but no visual, or lost prey)
		if (!prey && isAggro)
		{
			float distToLast = vector.Distance(myPos, m_Owner.GetLastKnownPreyPos());
			if (distToLast > ARRIVED_THRESHOLD)
				return DatZUndeadState.INVESTIGATE;
		}

		// Priority 4: Return home if strayed too far
		if (!isAggro)
		{
			float distToHome = vector.Distance(myPos, m_vHomePosition);
			if (distToHome > WANDER_RADIUS * 2)
				return DatZUndeadState.RETURN;
		}

		// State-specific exit conditions
		switch (m_CurrentState)
		{
			case DatZUndeadState.IDLE:
			{
				if (stateTime > m_fCurrentIdleDuration)
					return DatZUndeadState.WANDER;
				break;
			}

			case DatZUndeadState.WANDER:
			{
				float distToTarget = vector.Distance(myPos, m_vWanderTarget);
				if (distToTarget < ARRIVED_THRESHOLD || stateTime > 15.0)
					return DatZUndeadState.IDLE;
				break;
			}

			case DatZUndeadState.RETURN:
			{
				float distToHome = vector.Distance(myPos, m_vHomePosition);
				if (distToHome < WANDER_RADIUS)
					return DatZUndeadState.IDLE;
				break;
			}

			case DatZUndeadState.INVESTIGATE:
			{
				float distToLast = vector.Distance(myPos, m_Owner.GetLastKnownPreyPos());
				if (distToLast < ARRIVED_THRESHOLD || stateTime > INVESTIGATE_TIMEOUT)
					return DatZUndeadState.IDLE;
				break;
			}

			case DatZUndeadState.CHASE:
			{
				// Timeout - been chasing too long without reaching prey
				if (stateTime > CHASE_TIMEOUT)
					return DatZUndeadState.INVESTIGATE;
				break;
			}

			case DatZUndeadState.ATTACK:
			{
				// If prey moved out of attack range, go back to chase
				if (hasValidPreyPos && distToPrey > m_Owner.GetAttackRange() * 1.5)
					return DatZUndeadState.CHASE;
				break;
			}
		}

		return m_CurrentState;
	}

	//------------------------------------------------------------------------------------------------
	protected void EnterState(int state)
	{
		switch (state)
		{
			case DatZUndeadState.IDLE:
				m_fCurrentIdleDuration = Math.RandomFloat(IDLE_MIN_DURATION, IDLE_MAX_DURATION);
				break;
			case DatZUndeadState.WANDER:
				PickWanderTarget();
				break;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void ExitState(int state)
	{
		// State cleanup if needed
	}

	//------------------------------------------------------------------------------------------------
	protected void ExecuteState(int state, float currentTime)
	{
		switch (state)
		{
			case DatZUndeadState.IDLE:
				ExecuteIdle();
				break;
			case DatZUndeadState.WANDER:
				ExecuteWander();
				break;
			case DatZUndeadState.INVESTIGATE:
				ExecuteInvestigate();
				break;
			case DatZUndeadState.CHASE:
				ExecuteChase();
				break;
			case DatZUndeadState.ATTACK:
				ExecuteAttack();
				break;
			case DatZUndeadState.RETURN:
				ExecuteReturn();
				break;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void ExecuteIdle()
	{
		m_Owner.StopMovement();
	}

	//------------------------------------------------------------------------------------------------
	protected void ExecuteWander()
	{
		m_Owner.SetMovementToward(m_vWanderTarget, m_Owner.GetWalkSpeed());
	}

	//------------------------------------------------------------------------------------------------
	protected void ExecuteInvestigate()
	{
		// Run toward sound - urgent investigation
		float speed = m_Owner.GetWalkSpeed();
		if (m_Owner.CanSprint())
			speed = m_Owner.GetSprintSpeed() * 0.8; // Fast but not full sprint
		m_Owner.SetMovementToward(m_Owner.GetLastKnownPreyPos(), speed);
	}

	//------------------------------------------------------------------------------------------------
	protected void ExecuteChase()
	{
		IEntity prey = m_Owner.GetCurrentPrey();
		if (!prey)
			return;

		// Get prey position and validate it
		vector preyPos = prey.GetOrigin();

		// Sanity check - if position is at origin or invalid, use last known position
		if (preyPos[0] == 0 && preyPos[1] == 0 && preyPos[2] == 0)
		{
			preyPos = m_Owner.GetLastKnownPreyPos();
		}

		float speed = m_Owner.GetWalkSpeed();
		if (m_Owner.CanSprint())
			speed = m_Owner.GetSprintSpeed();

		m_Owner.SetMovementToward(preyPos, speed);
	}

	//------------------------------------------------------------------------------------------------
	protected void ExecuteAttack()
	{
		m_Owner.StopMovement();
		m_Owner.TryAttack();
	}

	//------------------------------------------------------------------------------------------------
	protected void ExecuteReturn()
	{
		m_Owner.SetMovementToward(m_vHomePosition, m_Owner.GetWalkSpeed() * 0.8);
	}

	//------------------------------------------------------------------------------------------------
	protected void PickWanderTarget()
	{
		// Try up to 3 times to find a valid position
		for (int attempt = 0; attempt < 3; attempt++)
		{
			float angle = Math.RandomFloat(0, Math.PI2);
			float dist = Math.RandomFloat(5.0, WANDER_RADIUS);

			vector target = m_vHomePosition;
			target[0] = target[0] + Math.Cos(angle) * dist;
			target[2] = target[2] + Math.Sin(angle) * dist;

			// Get terrain height at position
			float surfaceY = GetGame().GetWorld().GetSurfaceY(target[0], target[2]);

			// Validate surface height is reasonable (not underwater, not in void)
			if (surfaceY > -100 && surfaceY < 5000)
			{
				target[1] = surfaceY;
				m_vWanderTarget = target;
				return;
			}
		}

		// Fallback: just wander slightly from current position
		m_vWanderTarget = m_Owner.GetOrigin();
		m_vWanderTarget[0] = m_vWanderTarget[0] + Math.RandomFloat(-5, 5);
		m_vWanderTarget[2] = m_vWanderTarget[2] + Math.RandomFloat(-5, 5);
	}

	//------------------------------------------------------------------------------------------------
	int GetCurrentState()
	{
		return m_CurrentState;
	}
}
