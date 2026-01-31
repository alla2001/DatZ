class HungerThirstHUD : SCR_InfoDisplay
{
    private SliderWidget m_wWaterLevelImage;
	private SliderWidget m_wFoodLevelImage;
   	private SliderWidget m_wHPLevelImage;
	private SliderWidget m_wStaminaLevelImage;
	private ref Color foodColor;
	private ref Color waterColor;
	private ref Color hpColor;

	private ImageWidget HPArrow1;
	private ImageWidget HPArrow2;
	private ImageWidget HPArrow3;

	// Infection UI
	private ImageWidget m_wInfectionIcon;
	private RichTextWidget m_wInfectionTimer;
	private float m_fInfectionPulseTime = 0;
	private bool m_bInfectionPulseUp = true;

	// Infection screen overlay
	private ImageWidget m_wInfectionOverlay;
	private ImageWidget m_wZombieHitFlash;
	private float m_fHitFlashAlpha = 0;
	private float m_fHitFlashDecay = 7.5;
    override event void OnStartDraw(IEntity owner)
    {
        super.OnStartDraw(owner);

            if (!m_wFoodLevelImage){
			         m_wFoodLevelImage = SliderWidget.Cast(m_wRoot.FindAnyWidget("FoodLevel"));
			foodColor = m_wFoodLevelImage.GetColor();
		}
       

        if (!m_wWaterLevelImage)
		{
			    m_wWaterLevelImage = SliderWidget.Cast(m_wRoot.FindAnyWidget("WaterLevel"));
			waterColor=m_wWaterLevelImage.GetColor();
		}
        
		
		if(!m_wHPLevelImage)
		{
		   m_wHPLevelImage = SliderWidget.Cast(m_wRoot.FindAnyWidget("HPLevel"));
			hpColor=m_wHPLevelImage.GetColor();
		}
		if(!m_wStaminaLevelImage)
		{
		   m_wStaminaLevelImage = SliderWidget.Cast(m_wRoot.FindAnyWidget("StaminaLevel"));

		}
		
		
		HPArrow1 = ImageWidget.Cast(m_wRoot.FindAnyWidget("HPArrow1"));
		HPArrow2 = ImageWidget.Cast(m_wRoot.FindAnyWidget("HPArrow2"));
		HPArrow3 = ImageWidget.Cast(m_wRoot.FindAnyWidget("HPArrow3"));

		// Infection widgets
		m_wInfectionIcon = ImageWidget.Cast(m_wRoot.FindAnyWidget("InfectionIcon"));
		m_wInfectionTimer = RichTextWidget.Cast(m_wRoot.FindAnyWidget("InfectionTimer"));

		// Infection screen overlay widgets
		m_wInfectionOverlay = ImageWidget.Cast(m_wRoot.FindAnyWidget("InfectionOverlay"));
		m_wZombieHitFlash = ImageWidget.Cast(m_wRoot.FindAnyWidget("ZombieHitFlash"));
    }
	override void OnInit (IEntity owner)
	{

	
	}
   void UpdateStats(float food, float water, float HP, float stamina, int healthUp, bool isInfected = false, float infectionProgress = 0, bool zombieHitEffect = false)
{
	// ---- FOOD BAR ----
	if (m_wFoodLevelImage)
	{
		if (food < 30)
			m_wFoodLevelImage.SetColor(Color(0.9,0.1,0.1,1)); // Red
		else if (food < 50)
			m_wFoodLevelImage.SetColor(Color(0.9, 0.9, 0.1,1)); // Yellow
		else
			m_wFoodLevelImage.SetColor(foodColor); // Default

		m_wFoodLevelImage.SetCurrent(food / 100);
	}

	// ---- WATER BAR ----
	if (m_wWaterLevelImage)
	{
		if (water < 30)
			m_wWaterLevelImage.SetColor(Color(0.9,0.1,0.1,1)); // Red
		else if (water < 50)
			m_wWaterLevelImage.SetColor(Color(0.9, 0.9, 0.1,1)); // Yellow
		else
			m_wWaterLevelImage.SetColor(waterColor); // Default

		m_wWaterLevelImage.SetCurrent(water / 100);
	}

	// ---- HP TEXT ----
	if (m_wHPLevelImage)
	{
		if (HP < 20)
			m_wHPLevelImage.SetColor(Color(1, 0, 0,1)); // Red
		else if (HP < 50)
			m_wHPLevelImage.SetColor(Color(1, 1, 0,1)); // Yellow
		else
			m_wHPLevelImage.SetColor(hpColor); // Default

		m_wHPLevelImage.SetCurrent(HP / 100);

		HPArrow1.SetVisible(false);
		HPArrow2.SetVisible(false);
		HPArrow3.SetVisible(false);
		HPArrow1.SetRotation(0);
		HPArrow2.SetRotation(0);
		HPArrow3.SetRotation(0);
		if(healthUp<0){
			HPArrow1.SetVisible(true);
			HPArrow1.SetRotation(180);
		}
		if(healthUp<-1){
			HPArrow2.SetVisible(true);
			HPArrow2.SetRotation(180);
		}
		if(healthUp<-2){
			HPArrow3.SetVisible(true);
			HPArrow3.SetRotation(180);
		}
		if(healthUp>0)
			HPArrow1.SetVisible(true);

		if(healthUp>1)
			HPArrow2.SetVisible(true);

		if(healthUp>2)
			HPArrow3.SetVisible(true);
	}

	// ---- STAMINA BAR ----
	if (m_wStaminaLevelImage)
	{
		if (stamina < 100)
			m_wStaminaLevelImage.SetColor(Color(1, 1, 1,0.43));
		else
			m_wStaminaLevelImage.SetColor(Color(1, 1, 1,0)); // Default

		m_wStaminaLevelImage.SetCurrent(stamina / 100);
	}

	// ---- INFECTION ICON AND SCREEN EFFECTS ----
	UpdateInfectionUI(isInfected, infectionProgress);
	UpdateInfectionOverlay(isInfected, infectionProgress);
	UpdateZombieHitFlash(zombieHitEffect);
}

//------------------------------------------------------------------------------------------------
// Update infection icon with pulsing effect
void UpdateInfectionUI(bool isInfected, float progress)
{
	if (!m_wInfectionIcon)
		return;

	if (!isInfected)
	{
		m_wInfectionIcon.SetVisible(false);
		if (m_wInfectionTimer)
			m_wInfectionTimer.SetVisible(false);
		return;
	}

	// Show infection icon
	m_wInfectionIcon.SetVisible(true);

	// Pulsing effect - faster as infection progresses
	float pulseSpeed = 0.5 + progress * 2.0; // 0.5 to 2.5
	m_fInfectionPulseTime += pulseSpeed * 0.016; // Assuming ~60fps

	if (m_fInfectionPulseTime > 1.0)
	{
		m_fInfectionPulseTime = 0;
		m_bInfectionPulseUp = !m_bInfectionPulseUp;
	}

	// Calculate opacity for pulse (0.4 to 1.0)
	float pulseValue = m_fInfectionPulseTime;
	if (!m_bInfectionPulseUp)
		pulseValue = 1.0 - pulseValue;
	float opacity = 0.4 + pulseValue * 0.6;

	// Color gets more red as infection progresses
	float r = 0.2 + progress * 0.8; // 0.2 to 1.0
	float g = 0.8 - progress * 0.6; // 0.8 to 0.2
	float b = 0.1;
	m_wInfectionIcon.SetColor(Color(r, g, b, opacity));

	// Update timer text if available
	if (m_wInfectionTimer)
	{
		m_wInfectionTimer.SetVisible(true);
		float remainingSeconds = (1.0 - progress) * 518400; // 6 days in seconds
		string timeText = FormatTimeRemaining(remainingSeconds);
		m_wInfectionTimer.SetText(timeText);
	}
}

//------------------------------------------------------------------------------------------------
// Format remaining time as days/hours/minutes
string FormatTimeRemaining(float seconds)
{
	int totalSeconds = seconds;
	int days = totalSeconds / 86400;
	int remainingAfterDays = totalSeconds - (days * 86400);
	int hours = remainingAfterDays / 3600;
	int remainingAfterHours = remainingAfterDays - (hours * 3600);
	int minutes = remainingAfterHours / 60;

	if (days > 0)
		return string.Format("%1d %2h", days, hours);
	else if (hours > 0)
		return string.Format("%1h %2m", hours, minutes);
	else
		return string.Format("%1m", minutes);
}

//------------------------------------------------------------------------------------------------
// Update infection screen overlay - becomes more visible as infection progresses
// Uses mask progress for vignette effect that doesn't cover UI
void UpdateInfectionOverlay(bool isInfected, float progress)
{
	if (!m_wInfectionOverlay)
		return;

	if (!isInfected)
	{
		m_wInfectionOverlay.SetVisible(false);
		return;
	}

	m_wInfectionOverlay.SetVisible(true);

	// Calculate pulse for subtle variation
	float pulseValue = m_fInfectionPulseTime;
	if (!m_bInfectionPulseUp)
		pulseValue = 1.0 - pulseValue;

	// Use mask progress to control vignette spread (0.2 to 0.7 based on infection)
	// Lower progress = more vignette coverage from edges
	float baseProgress = 0.7 - progress * 0.5; // 0.7 down to 0.2
	float pulseProgress = pulseValue * 0.05 * progress;
	float maskProgress = baseProgress - pulseProgress;

	// Set mask progress (vignette effect) - this affects edges, not center where UI is
	m_wInfectionOverlay.SetMaskProgress(maskProgress);

	// Keep color subtle - the texture/mask handles the visual effect
	// Just set opacity based on infection progress
	float opacity = 0.3 + progress * 0.5; // 0.3 to 0.8
	m_wInfectionOverlay.SetOpacity(opacity);
}

//------------------------------------------------------------------------------------------------
// Update zombie hit flash effect - uses vignette mask to not cover UI
void UpdateZombieHitFlash(bool zombieHitEffect)
{
	if (!m_wZombieHitFlash)
		return;

	// Trigger flash on zombie hit
	if (zombieHitEffect && m_fHitFlashAlpha < 0.4)
	{
		m_fHitFlashAlpha = 0.8; // Start flash at high opacity
	}

	// Fade out the flash over time (assuming ~60fps, each call is ~16ms)
	if (m_fHitFlashAlpha > 0)
	{
		m_fHitFlashAlpha = m_fHitFlashAlpha - 0.016 * m_fHitFlashDecay;
		if (m_fHitFlashAlpha < 0)
			m_fHitFlashAlpha = 0;

		m_wZombieHitFlash.SetVisible(true);

		// Use mask progress for vignette effect (edges only, not center)
		// Lower value = more coverage from edges
		float maskProgress = 0.3 + (1.0 - m_fHitFlashAlpha) * 0.4; // 0.3 to 0.7 as it fades
		m_wZombieHitFlash.SetMaskProgress(maskProgress);
		m_wZombieHitFlash.SetOpacity(m_fHitFlashAlpha);
	}
	else
	{
		m_wZombieHitFlash.SetVisible(false);
	}
}

}
