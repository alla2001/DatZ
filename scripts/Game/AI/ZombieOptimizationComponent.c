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
	protected bool isZombi ;
	
	
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
	
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if(!isZombi)return;
		if(!Replication.IsServer())return;
		// Get required components
		m_AnimationComponent = CharacterAnimationComponent.Cast(owner.FindComponent(CharacterAnimationComponent));
		m_AIControlComponent = AIControlComponent.Cast(owner.FindComponent(AIControlComponent));
		m_CharacterController = CharacterControllerComponent.Cast(owner.FindComponent(CharacterControllerComponent));
		if(GetGame().GetWorld()==null)return;
		// Get navigation component from AIWorld
		IEntity aiWorldEntity = GetGame().GetWorld().FindEntityByName("SCR_AIWorld");
		if (aiWorldEntity)
			m_NavmeshWorldComponent = NavmeshWorldComponent.Cast(aiWorldEntity.FindComponent(NavmeshWorldComponent));
		
		m_fLastViewTime = GetGame().GetWorld().GetWorldTime();
		m_fLastDistanceCheck = m_fLastViewTime;
		m_vLastPosition = owner.GetOrigin();
		
		// Start the optimization check
		GetGame().GetCallqueue().CallLater(CheckOptimization, m_fCheckInterval * 1000, true);
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
		float closestPlayerDistance = float.MAX;
		
		// Performance optimization: Only check distance every few seconds
		bool shouldUpdateDistance = (currentTime - m_fLastDistanceCheck) > 3.0;
		if (shouldUpdateDistance)
		{
			m_fLastDistanceCheck = currentTime;
			UpdatePlayerProximity(zombiePos);
		}
		
		// Skip expensive FOV checks for distant zombies
		if (!m_bNearAnyPlayer && m_bIsOptimized)
		{
			m_iChecksSinceVisible++;
			// Only check every 4th time for optimized distant zombies
			if (m_iChecksSinceVisible < 4)
			{
				AdjustCheckFrequency(false, float.MAX);
				return;
			}
			m_iChecksSinceVisible = 0;
		}
		
		// Get all players
		array<IEntity> players = {};
		array<int> playerIDs = {};
		GetGame().GetPlayerManager().GetAllPlayers(playerIDs);
		foreach( int id : playerIDs)
		{
			IEntity ent = GetGame().GetPlayerManager().GetPlayerControlledEntity(id);
			if(ent)
				players.Insert(ent);
		}
		
		foreach (IEntity player : players)
		{
			if (!player)
				continue;
			if (player == GetOwner())
			{
				isCurrentlyInView = true;
				continue;
			}
			
			float distance = vector.Distance(zombiePos, player.GetOrigin());
			if (distance < closestPlayerDistance)
				closestPlayerDistance = distance;
			
			// Only do expensive FOV checks if zombie is close enough AND we haven't already found visibility
			if (!isCurrentlyInView && distance < m_fMaxFOVDistance)
			{
				if (IsInPlayerFieldOfView(player, zombiePos))
				{
					isCurrentlyInView = true;
					// Don't break here - we still need to find closest distance
				}
			}
		}
		
		// Only optimize if far enough from players
		bool canOptimize = closestPlayerDistance > m_fMinOptimizationDistance;
		
		HandleOptimizationState(isCurrentlyInView, canOptimize);
		AdjustCheckFrequency(isCurrentlyInView, closestPlayerDistance);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Update whether zombie is near any player (for performance culling)
	protected void UpdatePlayerProximity(vector zombiePos)
	{
		array<IEntity> players = {};
		array<int> playerIDs = {};
		GetGame().GetPlayerManager().GetAllPlayers(playerIDs);
		foreach( int id : playerIDs)
		{
			IEntity ent = GetGame().GetPlayerManager().GetPlayerControlledEntity(id);
			if(ent)
				players.Insert(ent);
		}
		
		m_bNearAnyPlayer = false;
		foreach (IEntity player : players)
		{
			if (!player || player == GetOwner())
				continue;
				
			float distance = vector.Distance(zombiePos, player.GetOrigin());
			if (distance < m_fMaxFOVDistance)
			{
				m_bNearAnyPlayer = true;
				break;
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Dynamically adjust check frequency based on visibility and distance
	protected void AdjustCheckFrequency(bool isVisible, float closestDistance)
	{
		GetGame().GetCallqueue().Remove(CheckOptimization);
		
		float newInterval;
		if (isVisible || closestDistance < m_fMinOptimizationDistance)
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
		
		GetGame().GetCallqueue().CallLater(CheckOptimization, newInterval * 1000, true);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Handle the optimization state changes
	protected void HandleOptimizationState(bool isInView, bool canOptimize)
	{
		float currentTime = GetGame().GetWorld().GetWorldTime();
		
		// If zombie comes into view and was optimized
		if (isInView && m_bIsOptimized)
		{
			// Calculate time out of view
			float timeOutOfView = currentTime - m_fLastViewTime;
			
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
	//! Check if zombie is in player's field of view
	protected bool IsInPlayerFieldOfView(IEntity player, vector zombiePos)
	{
		if (!player)
			return false;
			
		// Get player camera/head position and direction
		vector playerPos = player.GetOrigin();
		vector playerEyePos = playerPos;
		playerEyePos[1] = playerEyePos[1] + 1.7; // Approximate eye height
		
		// Get player's look direction
		vector playerDirection = player.GetYawPitchRoll();
		vector lookDirection = playerDirection.AnglesToVector();
		
		// Calculate direction to zombie
		vector toZombie = zombiePos - playerEyePos;
		float distance = toZombie.Length();
		toZombie.Normalize();
		
		// Check if zombie is within field of view (approximate 90 degree cone)
		float dotProduct = vector.Dot(lookDirection, toZombie);
		bool inFOV = dotProduct > 0.7; // Adjust this value to change FOV sensitivity
		
		if (!inFOV)
			return false;
		return true;
		// Perform raycast to check for obstacles
		autoptr TraceParam trace = new TraceParam();
		trace.Start = playerEyePos;
		trace.End = zombiePos;
		trace.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
		
		float traceDist = GetGame().GetWorld().TraceMove(trace, null);
		
		// If trace reached close to zombie position, it's visible
		return traceDist > 0.9; // 90% of the way means mostly visible
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
	//! Check if position is reachable via navigation
	protected bool IsPositionReachable(vector fromPos, vector toPos)
	{
		// Simple distance check as primary validation
		float distance = vector.Distance(fromPos, toPos);
		if (distance > m_fMaxTeleportDistance)
			return false;
		
		// Basic line-of-sight check for major obstacles
		autoptr TraceParam trace = new TraceParam();
		trace.Start = Vector(fromPos[0], fromPos[1] + 0.5, fromPos[2]);
		trace.End = Vector(toPos[0], toPos[1] + 0.5, toPos[2]);
		trace.Flags = TraceFlags.WORLD;
		
		float traceDist = GetGame().GetWorld().TraceMove(trace, null);
		
		// If we can't reach 80% of the way, consider it blocked
		if (traceDist < 0.8)
			return false;
		
		// Additional ground check - ensure target position has ground
		autoptr TraceParam groundTrace = new TraceParam();
		groundTrace.Start = Vector(toPos[0], toPos[1] + 2, toPos[2]);
		groundTrace.End = Vector(toPos[0], toPos[1] - 2, toPos[2]);
		trace.Flags = TraceFlags.WORLD;
		
		float groundTraceDist = GetGame().GetWorld().TraceMove(groundTrace, null);
		return groundTraceDist > 0.1; // Has ground within reasonable distance
	}
	
	//------------------------------------------------------------------------------------------------
	//! Snap position to ground
	protected vector SnapToGround(vector position)
	{
		autoptr TraceParam trace = new TraceParam();
		trace.Start = Vector(position[0], position[1] + 5, position[2]);
		trace.End = Vector(position[0], position[1] - 5, position[2]);
		trace.Flags = TraceFlags.WORLD;
		
		vector hitPos;
		vector hitNormal;
		
		float traceDist = GetGame().GetWorld().TraceMove(trace, null);
		if (traceDist > 0)
		{
			hitPos = trace.Start + (trace.End - trace.Start) * traceDist;
			return hitPos;
		}
		
		return position; // Return original if no ground found
	}
}