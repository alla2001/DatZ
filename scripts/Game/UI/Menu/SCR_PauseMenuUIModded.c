//------------------------------------------------------------------------------------------------
modded class PauseMenuUI : ChimeraMenuBase
{
	protected const int EXIT_DELAY_SECONDS = 30;
protected SCR_ButtonTextComponent m_ExitButton;
protected int m_ExitCountdown = EXIT_DELAY_SECONDS;
protected bool m_ExitCountdownActive = false;
	
		//------------------------------------------------------------------------------------------------
	override void OnMenuOpen()
	{
		vanilla.OnMenuOpen();
		m_ExitButton = SCR_ButtonTextComponent.GetButtonText("Exit", m_wRoot);
if (m_ExitButton)
{
    m_ExitButton.SetEnabled(false);
    m_ExitButton.SetText(string.Format("Wait %1s", m_ExitCountdown));
    m_ExitButton.m_OnClicked.Insert(OnExit);
    StartExitCountdown();
}
	}
	protected void StartExitCountdown()
{
    m_ExitCountdownActive = true;
    GetGame().GetCallqueue().CallLater(UpdateExitCountdown, 1000, true);
}

protected void UpdateExitCountdown()
{
    if (!m_ExitCountdownActive)
        return;

    m_ExitCountdown--;

    if (m_ExitCountdown <= 0)
    {
        m_ExitCountdownActive = false;
        GetGame().GetCallqueue().Remove(UpdateExitCountdown);
        if (m_ExitButton)
        {
            m_ExitButton.SetEnabled(true);
				if(IsSavingOnExit())
            m_ExitButton.SetText(EXIT_SAVE );
				else 
				 m_ExitButton.SetText(EXIT_NO_SAVE );
        }
    }
    else
    {
        if (m_ExitButton)
        {
            m_ExitButton.SetText(string.Format("Wait %1s", m_ExitCountdown));
        }
    }
}
	override void OnMenuClose()
{
    s_Instance = null;

    // Unpause
    SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
    if (gameMode)
        gameMode.PauseGame(false, SCR_EPauseReason.MENU);

    SCR_HUDManagerComponent hud = GetGame().GetHUDManager();
    if (hud)
        hud.SetVisible(true);

    m_OnPauseMenuClosed.Invoke();

    SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.SOUND_FE_HUD_PAUSE_MENU_CLOSE);

    // Stop the exit countdown if it's active
    if (m_ExitCountdownActive)
    {
        m_ExitCountdownActive = false;
        GetGame().GetCallqueue().Remove(UpdateExitCountdown);
    }
}


}



