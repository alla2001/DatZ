
modded class SCR_AIThreatSystem
{	
	
	//------------------------------------------------------------------------------------------------
	//!
	//! Called by utilityComponent each EvaluateBehavior call
	//! \param[in] utility
	//! \param[in] timeSlice
	override void Update(SCR_AIUtilityComponent utility, float timeSlice)
	{
		// Threat falloff
		m_fThreatSuppression -= m_fThreatSuppression * THREAT_SUPPRESSION_DROP_RATE * timeSlice;
		m_fThreatShotsFired -= m_fThreatShotsFired * THREAT_SHOT_DROP_RATE * timeSlice;
		
		if (m_Combat)
		{
			if (m_Combat.GetCurrentTarget())
				m_fThreatIsEndangered = ENDANGERED_INCREMENT;
			else
				m_fThreatIsEndangered -= m_fThreatIsEndangered * THREAT_ENDANGERED_DROP_RATE * timeSlice;
		}

		// Process all danger events and clear the array
		if (m_Agent && m_Config.m_EnableDangerEvents)
		{
			int i;
			AIDangerEvent dangerEvent;
			
#ifdef AI_DEBUG
			if (m_Agent.GetDangerEventsCount() != 0)
				AddDebugMessage(string.Format("Processing danger events: %1 in the queue", m_Agent.GetDangerEventsCount()));
#endif
			
			for (int max = m_Agent.GetDangerEventsCount(); i < max; i++)
			{
				int eventAggregationCount;
				dangerEvent = m_Agent.GetDangerEvent(i, eventAggregationCount);
				
				
				#ifdef AI_DEBUG
				AddDebugMessage(string.Format("PerformDangerReaction: %1x %2", eventAggregationCount, dangerEvent));
				#endif
				
				if (dangerEvent)
				{
					
					if (m_Config.PerformDangerReaction(m_Utility, dangerEvent, eventAggregationCount))
					{
#ifdef WORKBENCH
						string message = typename.EnumToString(SCR_EAIDangerEventType, dangerEvent.GetDangerType());
						SCR_AIDebugVisualization.VisualizeMessage(m_Utility.m_OwnerEntity, message, EAIDebugCategory.DANGER, 2);	// Show message above AI's head
#endif
					}
				}
			}

			m_Agent.ClearDangerEvents(i + 1);
		}		
	
		// Add threat value from current behavior
		float threatFromBehavior;
		if (utility.m_CurrentBehavior)
			threatFromBehavior = utility.m_CurrentBehavior.m_fThreat;
		
		m_fThreatTotal = Math.Clamp(threatFromBehavior + m_fThreatSuppression + m_fThreatInjury + m_fThreatShotsFired + m_fThreatIsEndangered, 0, 1);
		
		UpdateState();
#ifdef WORKBENCH
		ShowDebug();
#endif
	}
	
	
}
