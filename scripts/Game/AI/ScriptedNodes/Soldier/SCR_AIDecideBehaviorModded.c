modded class SCR_AIDecideBehavior: AITaskScripted
{
	
	//------------------------------------------------------------------------------------------------
	override void OnInit(AIAgent owner)
	{
		m_UtilityComponent = SCR_AIUtilityComponent.Cast(owner.FindComponent(SCR_AIUtilityComponent));
		if (!m_UtilityComponent)
		{
			NodeError(this, owner, "Can't find utility component.");
		}
		
		m_fRandomDelay_s = Math.RandomFloat(0.0, s_aUpdateIntervals[0])+2.0;
	}
	bool evalAI;
	//------------------------------------------------------------------------------------------------
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		if (!m_UtilityComponent)
			return ENodeResult.FAIL;
		
		BaseTarget unknownTarget;		
		GetVariableIn(PORT_UNKNOWN_TARGET, unknownTarget);
		
		m_CurrentBehavior = m_UtilityComponent.EvaluateBehavior(unknownTarget);
		
		if (!m_CurrentBehavior || m_CurrentBehavior.m_sBehaviorTree == ResourceName.Empty)
		{
			Print("AI: Missing behavior tree in " + m_CurrentBehavior.ToString(), LogLevel.WARNING);
			return ENodeResult.FAIL;
		}

		if (m_PreviousBehavior != m_CurrentBehavior)
		{
			SetVariableOut(PORT_BEHAVIOR_TREE, m_CurrentBehavior.m_sBehaviorTree);
			SetVariableOut(PORT_UPDATE_BEHAVIOR, true);
		}
	
		// m_bUseCombatMove can change at behavior run time
		SetVariableOut(PORT_USE_COMBAT_MOVE, m_CurrentBehavior.m_bUseCombatMove);
		
		// Resolve desired update interval
		int lod = Math.ClampInt(owner.GetLOD(), 0, LOD_MAX);
		float updateInterval = s_aUpdateIntervals[lod] + m_fRandomDelay_s;
		SetVariableOut(PORT_UPDATE_INTERVAL, updateInterval);
		m_fRandomDelay_s = 0;
				
		m_PreviousBehavior = m_CurrentBehavior;
		return ENodeResult.SUCCESS;
	}
	
	
	
};