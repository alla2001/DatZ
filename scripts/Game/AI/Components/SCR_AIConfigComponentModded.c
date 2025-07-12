

modded class SCR_AIConfigComponent : ScriptComponent
{
	
	
	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		SCR_AISettingsComponent settings = SCR_AISettingsComponent.GetInstance();
		if (!settings)
			return;

		// If the settings component is enabled, overwrite current settings by global ones
		m_EnableMovement = settings.m_EnableMovement;
		m_EnableDangerEvents = settings.m_EnableDangerEvents;
		m_EnablePerception = settings.m_EnablePerception;
		m_EnableAttack = settings.m_EnableAttack;
		m_EnableTakeCover = settings.m_EnableTakeCover;
		m_EnableLooking = settings.m_EnableLooking;
		
		m_EnableLeaderStop = settings.m_EnableLeaderStop;
		m_EnableAimingError = settings.m_EnableAimError;
		m_EnableCommunication = false;
		settings.m_EnableCommunication=false;
		typename type_EMessageType_Goal = EMessageType_Goal;
		typename type_EMessageType_Info = EMessageType_Info;
		
		// Map weapon handling configs based on weapon type
		// Initialize min magazine specification array
		m_aMinSuppressiveMagCountSpec.Clear();
		m_aMinSuppressiveMagCountSpec.Insert(SCR_AIWeaponTypeHandlingConfig.DEFAULT_LOW_MAG_THRESHOLD);
		if (m_DefaultWeaponTypeHandlingConfig)
			m_aMinSuppressiveMagCountSpec[0] = m_DefaultWeaponTypeHandlingConfig.m_iMinSuppressiveMagCountThreshold;
		foreach (SCR_AIWeaponTypeHandlingConfig config : m_aWeaponTypeHandlingConfig)
		{
			m_mWeaponTypeHandlingConfig[config.m_eWeaponType] = config;
			
			m_aMinSuppressiveMagCountSpec.Insert(config.m_eWeaponType);
			m_aMinSuppressiveMagCountSpec.Insert(config.m_iMinSuppressiveMagCountThreshold);
		}
			
		foreach (SCR_AIDangerReaction reaction : m_aDangerReactions)
			m_mDangerReactions[reaction.m_eType] = reaction;
		
		m_aGoalReactionsPacked.Resize(type_EMessageType_Goal.GetVariableCount());
		foreach (SCR_AIGoalReaction reaction : m_aGoalReactions)
		{
			if (reaction.m_eType != EMessageType_Goal.NONE)
				m_aGoalReactionsPacked[reaction.m_eType] = reaction;
		}
		
		m_aInfoReactionsPacked.Resize(type_EMessageType_Info.GetVariableCount());
		foreach (SCR_AIInfoReaction reaction : m_aInfoReactions)
		{
			if (reaction.m_eType != EMessageType_Info.NONE)
				m_aInfoReactionsPacked[reaction.m_eType] = reaction;
		}
	}
		//!
	//! \param[in] utility
	//! \param[in] dangerEvent
	//! \return
	override bool PerformDangerReaction(SCR_AIUtilityComponent utility, AIDangerEvent dangerEvent, int dangerEventCount)
	{
		SCR_AIDangerReaction reaction = m_mDangerReactions[dangerEvent.GetDangerType()];
		if (reaction)
		{
			if(dangerEvent.GetDangerType() == EAIDangerEventType.Danger_WeaponFire){
			
				
					return DatzDangerReactionWrapper.PerformReaction(utility, utility.m_ThreatSystem, dangerEvent, dangerEventCount);
			}
			else
			return reaction.PerformReaction(utility, utility.m_ThreatSystem, dangerEvent, dangerEventCount);
		}
		return false;
	}

	
	
}
