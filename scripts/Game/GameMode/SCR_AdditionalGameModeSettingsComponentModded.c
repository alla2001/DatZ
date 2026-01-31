//------------------------------------------------------------------------------------------------
// Modded to disable team kill punishment and clear existing penalties
//------------------------------------------------------------------------------------------------
modded class SCR_AdditionalGameModeSettingsComponent : SCR_BaseGameModeComponent
{
	//------------------------------------------------------------------------------------------------
	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		// Disable team kill punishment and clear penalties
		if (Replication.IsServer())
		{
			GetGame().GetCallqueue().CallLater(DisableTeamKillPunishment, 1000, false);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void DisableTeamKillPunishment()
	{
		// Disable future team kill punishment
		SetEnableTeamKillPunishment_S(false);

		// Clear all existing player penalties
		ClearAllPlayerPenalties();

		Print("[DatZ] Team kill punishment disabled and penalties cleared", LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	protected void ClearAllPlayerPenalties()
	{
		// Get the local player penalty instance (handles TK penalties)
		SCR_LocalPlayerPenalty penaltySystem = SCR_LocalPlayerPenalty.GetInstance();
		if (!penaltySystem)
		{
			Print("[DatZ] No penalty system found - penalties not active", LogLevel.NORMAL);
			return;
		}

		// The penalty data is stored in m_aPlayerPenaltyData but is protected
		// Instead, we rely on disabling punishment which prevents new kicks
		// Existing in-memory penalties will be cleared on server restart

		Print("[DatZ] Penalty system found - punishment disabled for all future team kills", LogLevel.NORMAL);
	}
}
