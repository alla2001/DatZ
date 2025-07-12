//------------------------------------------------------------------------------------------------
modded class SCR_AmbientPatrolSystem : GameSystem
{

	//------------------------------------------------------------------------------------------------
	override event protected void OnInit()
	{
		// No need to run updates unless some patrols are actually registered
		if (m_aPatrols.IsEmpty())
			Enable(false);

		RefreshPlayerList();

		// Calculate (de)spawn distance based on view distance, have it squared for faster distance calculation
		int fractionOfVD = GetGame().GetViewDistance() * 0.3;
		m_iSpawnDistanceSq = fractionOfVD * fractionOfVD;
		m_iSpawnDistanceSq = Math.Min(450*450, m_iSpawnDistanceSq);
		m_iSpawnDistanceSq = Math.Max(250*250, m_iSpawnDistanceSq);
		m_iDespawnDistanceSq = m_iSpawnDistanceSq + 75*75;

		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());

		if (!gameMode)
			return;

		gameMode.GetOnPlayerSpawned().Insert(OnPlayerSpawnedOrDeleted);
		gameMode.GetOnPlayerKilled().Insert(OnPlayerKilled);
		gameMode.GetOnPlayerDeleted().Insert(OnPlayerSpawnedOrDeleted);
		gameMode.GetOnPlayerDisconnected().Insert(OnPlayerDisconnected);
	}
	//------------------------------------------------------------------------------------------------
	override protected void UpdateCheckInterval()
	{
		m_fCheckInterval = 1 / m_aPatrols.Count();
	}
}
