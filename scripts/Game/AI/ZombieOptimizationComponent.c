class ZombieOptimizationComponentClass : ScriptComponentClass
{
}

//------------------------------------------------------------------------------------------------
//! Zombie AI Optimization Component
//! Attach this to zombie AI entities to optimize performance when not in player view
//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "GameScripted/AI", description: "Optimizes zombie AI by freezing when not visible")]
class ZombieOptimizationComponent : ScriptComponent
{
	[Attribute("200", UIWidgets.SpinBox, "Minimum distance from players to enable optimization")]
	protected float m_fMinOptimizationDistance;

	[Attribute("2.0", UIWidgets.SpinBox, "Check interval in seconds")]
	protected float m_fCheckInterval;

	[Attribute("0.5", UIWidgets.SpinBox, "Fast check interval when recently visible")]
	protected float m_fFastCheckInterval;

	[Attribute("2.0", UIWidgets.SpinBox, "Movement speed multiplier for teleport distance calculation")]
	protected float m_fMovementSpeedMultiplier;

	[Attribute("50", UIWidgets.SpinBox, "Maximum teleport distance")]
	protected float m_fMaxTeleportDistance;

	[Attribute("10", UIWidgets.SpinBox, "Minimum teleport distance")]
	protected float m_fMinTeleportDistance;

	[Attribute("400", UIWidgets.SpinBox, "Maximum distance for FOV checks")]
	protected float m_fMaxFOVDistance;

	[Attribute("true")]
	protected bool isZombi;

	// Pre-calculated squared distances (avoid sqrt)
	protected float m_fMinOptimizationDistanceSq;
	protected float m_fMaxFOVDistanceSq;
	protected float m_fMaxTeleportDistanceSq;

	protected bool m_bIsOptimized = false;
	protected bool m_bWasInView = true;
	protected float m_fLastViewTime;
	protected float m_fCheckTimer;
	protected float m_fLastDistanceCheck;
	protected bool m_bNearAnyPlayer = false;
	protected int m_iChecksSinceVisible = 0;
	protected vector m_vLastPosition;
	protected CharacterAnimationComponent m_AnimationComponent;
	protected AIControlComponent m_AIControlComponent;
	protected CharacterControllerComponent m_CharacterController;
	protected NavmeshWorldComponent m_NavmeshWorldComponent;

	// Cached trace params to avoid allocations
	protected ref TraceParam m_CachedTrace;
	protected ref TraceParam m_CachedGroundTrace;

	// Cached player data to avoid repeated lookups
	protected ref array<vector> m_aCachedPlayerPositions = new array<vector>();
	protected ref array<vector> m_aCachedPlayerDirections = new array<vector>();
	protected float m_fLastPlayerCacheTime;

	// Current interval for dynamic adjustment
	protected float m_fCurrentInterval;
	
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (!isZombi)
			return;
		if (!Replication.IsServer())
			return;

		// Pre-calculate squared distances (avoid sqrt in hot path)
		m_fMinOptimizationDistanceSq = m_fMinOptimizationDistance * m_fMinOptimizationDistance;
		m_fMaxFOVDistanceSq = m_fMaxFOVDistance * m_fMaxFOVDistance;
		m_fMaxTeleportDistanceSq = m_fMaxTeleportDistance * m_fMaxTeleportDistance;

		// Pre-allocate trace params to avoid runtime allocations
		m_CachedTrace = new TraceParam();
		m_CachedTrace.Flags = TraceFlags.WORLD;
		m_CachedGroundTrace = new TraceParam();
		m_CachedGroundTrace.Flags = TraceFlags.WORLD;

		// Get required components
		m_AnimationComponent = CharacterAnimationComponent.Cast(owner.FindComponent(CharacterAnimationComponent));
		m_AIControlComponent = AIControlComponent.Cast(owner.FindComponent(AIControlComponent));
		m_CharacterController = CharacterControllerComponent.Cast(owner.FindComponent(CharacterControllerComponent));

		if (!GetGame().GetWorld())
			return;

		// Get navigation component from AIWorld
		IEntity aiWorldEntity = GetGame().GetWorld().FindEntityByName("SCR_AIWorld");
		if (aiWorldEntity)
			m_NavmeshWorldComponent = NavmeshWorldComponent.Cast(aiWorldEntity.FindComponent(NavmeshWorldComponent));

		float worldTime = GetGame().GetWorld().GetWorldTime();
		m_fLastViewTime = worldTime;
		m_fLastDistanceCheck = worldTime;
		m_fLastPlayerCacheTime = 0;
		m_vLastPosition = owner.GetOrigin();
		m_fCurrentInterval = m_fCheckInterval;

		// Start the optimization check with staggered delay to distribute load
		int staggerDelay = Math.RandomInt(0, 500);
		GetGame().GetCallqueue().CallLater(CheckOptimization, (m_fCheckInterval * 1000) + staggerDelay, false);
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		// Clean up timer
		GetGame().GetCallqueue().Remove(CheckOptimization);
		super.OnDelete(owner);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Main optimization check function with performance optimizations
	protected void CheckOptimization()
	{
		IEntity owner = GetOwner();
		if (!owner)
			return;

		vector zombiePos = owner.GetOrigin();
		float currentTime = GetGame().GetWorld().GetWorldTime();
		bool isCurrentlyInView = false;
		float closestPlayerDistanceSq = float.MAX;

		// Cache player positions (shared across all checks this frame)
		bool shouldUpdateCache = (currentTime - m_fLastPlayerCacheTime) > 200; // 200ms cache
		if (shouldUpdateCache)
		{
			m_fLastPlayerCacheTime = currentTime;
			CachePlayerData();
		}

		// Skip expensive FOV checks for distant zombies
		if (!m_bNearAnyPlayer && m_bIsOptimized)
		{
			m_iChecksSinceVisible++;
			// Only check every 4th time for optimized distant zombies
			if (m_iChecksSinceVisible < 4)
			{
				ScheduleNextCheck(false, float.MAX);
				return;
			}
			m_iChecksSinceVisible = 0;
		}

		// Performance optimization: Only do proximity update every few seconds
		bool shouldUpdateProximity = (currentTime - m_fLastDistanceCheck) > 3000; // 3 seconds in ms
		if (shouldUpdateProximity)
		{
			m_fLastDistanceCheck = currentTime;
			m_bNearAnyPlayer = false;
		}

		int playerCount = m_aCachedPlayerPositions.Count();
		for (int i = 0; i < playerCount; i++)
		{
			vector playerPos = m_aCachedPlayerPositions[i];

			// Use squared distance (avoids sqrt)
			float distanceSq = vector.DistanceSq(zombiePos, playerPos);

			if (distanceSq < closestPlayerDistanceSq)
				closestPlayerDistanceSq = distanceSq;

			// Update proximity flag during proximity check
			if (shouldUpdateProximity && distanceSq < m_fMaxFOVDistanceSq)
				m_bNearAnyPlayer = true;

			// Only do FOV checks if zombie is close enough AND we haven't already found visibility
			if (!isCurrentlyInView && distanceSq < m_fMaxFOVDistanceSq)
			{
				if (IsInPlayerFieldOfViewFast(i, zombiePos, distanceSq))
				{
					isCurrentlyInView = true;
					// Don't break - still need to find closest distance
				}
			}
		}

		// Only optimize if far enough from players (compare squared distances)
		bool canOptimize = closestPlayerDistanceSq > m_fMinOptimizationDistanceSq;

		HandleOptimizationState(isCurrentlyInView, canOptimize, currentTime);
		ScheduleNextCheck(isCurrentlyInView, closestPlayerDistanceSq);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Cache player positions and directions to avoid repeated lookups
	protected void CachePlayerData()
	{
		m_aCachedPlayerPositions.Clear();
		m_aCachedPlayerDirections.Clear();

		array<int> playerIDs = {};
		GetGame().GetPlayerManager().GetAllPlayers(playerIDs);

		IEntity owner = GetOwner();
		foreach (int id : playerIDs)
		{
			IEntity player = GetGame().GetPlayerManager().GetPlayerControlledEntity(id);
			if (!player || player == owner)
				continue;

			vector playerPos = player.GetOrigin();
			m_aCachedPlayerPositions.Insert(playerPos);

			// Cache look direction for FOV checks
			vector angles = player.GetYawPitchRoll();
			m_aCachedPlayerDirections.Insert(angles.AnglesToVector());
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Schedule next check with dynamic interval (avoids repeated CallLater remove/add)
	protected void ScheduleNextCheck(bool isVisible, float closestDistanceSq)
	{
		float newInterval;
		if (isVisible || closestDistanceSq < m_fMinOptimizationDistanceSq)
		{
			// Fast checks when visible or very close
			newInterval = m_fFastCheckInterval;
			m_iChecksSinceVisible = 0;
		}
		else if (m_bIsOptimized && !m_bNearAnyPlayer)
		{
			// Much slower checks for optimized distant zombies
			newInterval = m_fCheckInterval * 4.0;
		}
		else
		{
			// Normal interval
			newInterval = m_fCheckInterval;
		}

		// Only reschedule if interval changed significantly (avoid CallLater churn)
		float intervalMs = newInterval * 1000;
		GetGame().GetCallqueue().CallLater(CheckOptimization, intervalMs, false);
		m_fCurrentInterval = newInterval;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Handle the optimization state changes
	protected void HandleOptimizationState(bool isInView, bool canOptimize, float currentTime)
	{
		// If zombie comes into view and was optimized
		if (isInView && m_bIsOptimized)
		{
			// Calculate time out of view (convert from ms to seconds)
			float timeOutOfView = (currentTime - m_fLastViewTime) * 0.001;

			// Move zombie to reasonable position based on time
			if (canOptimize && timeOutOfView > 2.0) // Only teleport if out of view for more than 2 seconds
			{
				TeleportToReasonablePosition(timeOutOfView);
			}

			// Restore zombie functionality
			EnableZombie();
			m_bIsOptimized = false;
			m_bWasInView = true;
			m_iChecksSinceVisible = 0;
		}
		// If zombie goes out of view and can be optimized
		else if (!isInView && !m_bIsOptimized && canOptimize)
		{
			// Freeze zombie
			DisableZombie();
			m_bIsOptimized = true;
			m_bWasInView = false;
		}
		// Update last view time
		else if (isInView)
		{
			m_fLastViewTime = currentTime;
			m_bWasInView = true;
			m_iChecksSinceVisible = 0;
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Fast FOV check using cached player data (no entity lookups)
	protected bool IsInPlayerFieldOfViewFast(int playerIndex, vector zombiePos, float distanceSq)
	{
		if (playerIndex >= m_aCachedPlayerPositions.Count())
			return false;

		vector playerPos = m_aCachedPlayerPositions[playerIndex];
		vector lookDirection = m_aCachedPlayerDirections[playerIndex];

		// Approximate eye position
		vector playerEyePos = playerPos;
		playerEyePos[1] = playerEyePos[1] + 1.7;

		// Calculate direction to zombie (avoid normalize by using squared magnitude)
		vector toZombie = zombiePos - playerEyePos;

		// Fast dot product check without full normalization
		// We can compare dot product against a threshold scaled by distance
		float dot = vector.Dot(lookDirection, toZombie);

		// If dot is negative, zombie is behind player
		if (dot < 0)
			return false;

		// Approximate FOV check: dot / |toZombie| > 0.7 means in ~90 degree cone
		// Rearranged: dot^2 > 0.49 * |toZombie|^2 (avoids sqrt)
		float toZombieLenSq = toZombie.LengthSq();
		if (toZombieLenSq < 0.01)
			return true; // Very close, consider visible

		// Check if in ~90 degree FOV cone (dot > 0.7 * length)
		// dot^2 > 0.49 * lengthSq
		return (dot * dot) > (0.49 * toZombieLenSq);
	}
	
	
	//------------------------------------------------------------------------------------------------
	//! Disable zombie AI and animation
	protected void DisableZombie()
	{
		/*if (m_AnimationComponent)
			m_AnimationComponent.Deactivate(GetOwner());*/
			
		if (m_AIControlComponent)
			m_AIControlComponent.DeactivateAI();
			
		// Store last position
		m_vLastPosition = GetOwner().GetOrigin();
	}
	
	//------------------------------------------------------------------------------------------------
	//! Enable zombie AI and animation
	protected void EnableZombie()
	{
		/*if (m_AnimationComponent)
			m_AnimationComponent.Activate(GetOwner());*/
			
		if (m_AIControlComponent)
			m_AIControlComponent.ActivateAI();
	}
	
	//------------------------------------------------------------------------------------------------
	//! Teleport zombie to a reasonable position based on time out of view
	protected void TeleportToReasonablePosition(float timeOutOfView)
	{
		IEntity owner = GetOwner();
		if (!owner)
			return;
			
		// Calculate reasonable movement distance
		float movementDistance = timeOutOfView * m_fMovementSpeedMultiplier;
		movementDistance = Math.Clamp(movementDistance, m_fMinTeleportDistance, m_fMaxTeleportDistance);
		
		vector currentPos = owner.GetOrigin();
		vector newPosition;
		
		// Try to find a valid position multiple times
		for (int i = 0; i < 10; i++)
		{
			// Generate random direction
			float randomAngle = Math.RandomFloat(0, Math.PI2);
			float randomDistance = Math.RandomFloat(m_fMinTeleportDistance, movementDistance);
			
			vector offset = Vector(
				Math.Cos(randomAngle) * randomDistance,
				0,
				Math.Sin(randomAngle) * randomDistance
			);
			
			newPosition = currentPos + offset;
			
			// Check if position is reachable using navigation system
			if (IsPositionReachable(currentPos, newPosition))
			{
				// Set new position
				owner.SetOrigin(newPosition);
				
				// Ensure zombie is on ground
				vector groundPos = SnapToGround(newPosition);
				owner.SetOrigin(groundPos);
				
				/*Print(string.Format("Zombie teleported from %1 to %2 (distance: %3m)", 
					currentPos.ToString(), newPosition.ToString(), 
					vector.Distance(currentPos, newPosition)));*/
				break;
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Check if position is reachable via navigation (uses cached trace params)
	protected bool IsPositionReachable(vector fromPos, vector toPos)
	{
		// Simple squared distance check as primary validation (avoids sqrt)
		float distanceSq = vector.DistanceSq(fromPos, toPos);
		if (distanceSq > m_fMaxTeleportDistanceSq)
			return false;

		// Basic line-of-sight check for major obstacles using cached trace
		m_CachedTrace.Start = Vector(fromPos[0], fromPos[1] + 0.5, fromPos[2]);
		m_CachedTrace.End = Vector(toPos[0], toPos[1] + 0.5, toPos[2]);

		float traceDist = GetGame().GetWorld().TraceMove(m_CachedTrace, null);

		// If we can't reach 80% of the way, consider it blocked
		if (traceDist < 0.8)
			return false;

		// Additional ground check - ensure target position has ground
		m_CachedGroundTrace.Start = Vector(toPos[0], toPos[1] + 2, toPos[2]);
		m_CachedGroundTrace.End = Vector(toPos[0], toPos[1] - 2, toPos[2]);

		float groundTraceDist = GetGame().GetWorld().TraceMove(m_CachedGroundTrace, null);
		return groundTraceDist > 0.1; // Has ground within reasonable distance
	}

	//------------------------------------------------------------------------------------------------
	//! Snap position to ground (uses cached trace params)
	protected vector SnapToGround(vector position)
	{
		m_CachedGroundTrace.Start = Vector(position[0], position[1] + 5, position[2]);
		m_CachedGroundTrace.End = Vector(position[0], position[1] - 5, position[2]);

		float traceDist = GetGame().GetWorld().TraceMove(m_CachedGroundTrace, null);
		if (traceDist > 0)
		{
			return m_CachedGroundTrace.Start + (m_CachedGroundTrace.End - m_CachedGroundTrace.Start) * traceDist;
		}

		return position; // Return original if no ground found
	}
}