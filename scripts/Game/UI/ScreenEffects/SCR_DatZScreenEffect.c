//------------------------------------------------------------------------------------------------
//! Infection Screen Effect - shows overlay that gets more opaque as infection progresses
//! Also handles zombie hit flash effect
//------------------------------------------------------------------------------------------------
class DatZ_InfectionScreenEffect : SCR_BaseScreenEffect
{
	// Screen overlay widget
	protected ImageWidget m_wInfectionOverlay;
	protected ImageWidget m_wZombieHitFlash;

	// Infection state
	protected bool m_bIsInfected = false;
	protected float m_fInfectionProgress = 0; // 0.0 to 1.0
	protected bool m_bZombieHitEffect = false;

	// Zombie hit flash
	protected float m_fHitFlashAlpha = 0;
	protected float m_fHitFlashDecay = 3.0; // How fast the flash fades

	// Pulse effect
	protected float m_fPulseTime = 0;
	protected bool m_bPulseUp = true;

	// Character reference
	protected ChimeraCharacter m_pCharacterEntity;

	//------------------------------------------------------------------------------------------------
	override void DisplayStartDraw(IEntity owner)
	{
		m_pCharacterEntity = ChimeraCharacter.Cast(owner);

		// Create overlay widgets if they don't exist
		Widget rootWidget = GetRootWidget();
		if (rootWidget)
		{
			m_wInfectionOverlay = ImageWidget.Cast(rootWidget.FindAnyWidget("InfectionOverlay"));
			m_wZombieHitFlash = ImageWidget.Cast(rootWidget.FindAnyWidget("ZombieHitFlash"));
		}
	}

	//------------------------------------------------------------------------------------------------
	override void DisplayControlledEntityChanged(IEntity from, IEntity to)
	{
		ClearEffects();
		m_pCharacterEntity = ChimeraCharacter.Cast(to);
	}

	//------------------------------------------------------------------------------------------------
	protected override void DisplayOnSuspended()
	{
		ClearEffects();
	}

	//------------------------------------------------------------------------------------------------
	protected override void UpdateEffect(float timeSlice)
	{
		if (!m_pCharacterEntity)
			return;

		// Get infection data from metabolism handler
		DatZMetabolsimHandler meta = DatZMetabolsimHandler.Cast(m_pCharacterEntity.FindComponent(DatZMetabolsimHandler));
		if (meta)
		{
			m_bIsInfected = meta.GetLocalInfected();
			m_fInfectionProgress = meta.GetLocalInfectionProgress();
			m_bZombieHitEffect = meta.GetLocalZombieHitEffect();
		}

		UpdateInfectionOverlay(timeSlice);
		UpdateZombieHitFlash(timeSlice);
	}

	//------------------------------------------------------------------------------------------------
	//! Update the infection overlay - becomes more opaque as infection progresses
	protected void UpdateInfectionOverlay(float timeSlice)
	{
		if (!m_wInfectionOverlay)
			return;

		if (!m_bIsInfected)
		{
			m_wInfectionOverlay.SetVisible(false);
			return;
		}

		m_wInfectionOverlay.SetVisible(true);

		// Pulse effect - faster as infection progresses
		float pulseSpeed = 0.5 + m_fInfectionProgress * 1.5;
		if (m_bPulseUp)
		{
			m_fPulseTime = m_fPulseTime + timeSlice * pulseSpeed;
			if (m_fPulseTime >= 1.0)
			{
				m_fPulseTime = 1.0;
				m_bPulseUp = false;
			}
		}
		else
		{
			m_fPulseTime = m_fPulseTime - timeSlice * pulseSpeed;
			if (m_fPulseTime <= 0)
			{
				m_fPulseTime = 0;
				m_bPulseUp = true;
			}
		}

		// Base opacity increases with infection progress (0.0 to 0.4)
		// Pulse adds variation (0.0 to 0.1)
		float baseOpacity = m_fInfectionProgress * 0.4;
		float pulseOpacity = m_fPulseTime * 0.1 * m_fInfectionProgress;
		float totalOpacity = baseOpacity + pulseOpacity;

		// Color shifts from green to more sickly yellow/green as infection progresses
		float r = 0.1 + m_fInfectionProgress * 0.3; // 0.1 to 0.4
		float g = 0.4 - m_fInfectionProgress * 0.1; // 0.4 to 0.3
		float b = 0.1;

		m_wInfectionOverlay.SetColor(Color(r, g, b, totalOpacity));
	}

	//------------------------------------------------------------------------------------------------
	//! Update the zombie hit flash effect
	protected void UpdateZombieHitFlash(float timeSlice)
	{
		if (!m_wZombieHitFlash)
			return;

		// Trigger flash on zombie hit
		if (m_bZombieHitEffect && m_fHitFlashAlpha < 0.5)
		{
			m_fHitFlashAlpha = 0.7; // Start flash at high opacity
		}

		// Fade out the flash
		if (m_fHitFlashAlpha > 0)
		{
			m_fHitFlashAlpha = m_fHitFlashAlpha - timeSlice * m_fHitFlashDecay;
			if (m_fHitFlashAlpha < 0)
				m_fHitFlashAlpha = 0;

			m_wZombieHitFlash.SetVisible(true);
			m_wZombieHitFlash.SetColor(Color(0.6, 0.1, 0.1, m_fHitFlashAlpha)); // Red flash
		}
		else
		{
			m_wZombieHitFlash.SetVisible(false);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected override void ClearEffects()
	{
		m_bIsInfected = false;
		m_fInfectionProgress = 0;
		m_bZombieHitEffect = false;
		m_fHitFlashAlpha = 0;

		if (m_wInfectionOverlay)
			m_wInfectionOverlay.SetVisible(false);

		if (m_wZombieHitFlash)
			m_wZombieHitFlash.SetVisible(false);
	}
}

//------------------------------------------------------------------------------------------------
class DatZ_NVMaskScreenEffect : SCR_BaseScreenEffect
{

// PP constants
	// Variables connected to a material, need to be static
	static const int COLORS_PP_PRIORITY									= 5;
	
	//Saturation
	private static float s_fSaturation 									= 1;
	protected const string DESATURATION_EMAT							= "{7C202A913EB8B1A9}UI/Materials/ScreenEffects_ColorPP.emat";
	
	[Attribute( defvalue: "0.5", uiwidget: UIWidgets.EditBox)]
	protected float m_fDesaturationStart;
	
	[Attribute( defvalue: "0.333", uiwidget: UIWidgets.EditBox)]
	protected float m_fDesaturationEnd;

	// Enabling/Disabling of PP fx
	private static bool s_bEnableSaturation;

	// Character
	protected ChimeraCharacter m_pCharacterEntity;
	protected SCR_CharacterBloodHitZone									m_BloodHZ;
	protected SCR_CharacterDamageManagerComponent						m_DamageManager;
	protected bool m_bLocalPlayerOutsideCharacter;
	
	//------------------------------------------------------------------------------------------------
	override void DisplayStartDraw(IEntity owner)
	{
		m_pCharacterEntity = ChimeraCharacter.Cast(owner);
	}

	//------------------------------------------------------------------------------------------------
	override void SettingsChanged()
	{
		if (!m_pCharacterEntity)
			return;
		
		m_pCharacterEntity.GetWorld().SetCameraPostProcessEffect(m_pCharacterEntity.GetWorld().GetCurrentCameraId(), COLORS_PP_PRIORITY, PostProcessEffectType.Colors, DESATURATION_EMAT);
	}

	//------------------------------------------------------------------------------------------------
 	override void DisplayControlledEntityChanged(IEntity from, IEntity to)
	{
		ClearEffects();
		
		m_pCharacterEntity = ChimeraCharacter.Cast(to);
		if (!m_pCharacterEntity)
			return;
		
		m_DamageManager = SCR_CharacterDamageManagerComponent.Cast(m_pCharacterEntity.GetDamageManager());
		if (!m_DamageManager)
			return;
		
		// define hitzones for later getting
		m_BloodHZ = m_DamageManager.GetBloodHitZone();
		
		m_pCharacterEntity.GetWorld().SetCameraPostProcessEffect(m_pCharacterEntity.GetWorld().GetCurrentCameraId(), COLORS_PP_PRIORITY, PostProcessEffectType.Colors, DESATURATION_EMAT);
	}
	
	//------------------------------------------------------------------------------------------------	
	protected override void DisplayOnSuspended()
	{
		s_bEnableSaturation = false;
	}
	
	//------------------------------------------------------------------------------------------------
	protected override void DisplayOnResumed()
	{
		s_bEnableSaturation = true;
	}

	//------------------------------------------------------------------------------------------------
	protected override void UpdateEffect(float timeSlice)
	{
		AddDesaturationEffect();
	}
	
	//------------------------------------------------------------------------------------------------
	protected void AddDesaturationEffect()
	{
		if (!m_BloodHZ)
			return;

		float bloodLevel = Math.InverseLerp(m_fDesaturationEnd, m_fDesaturationStart, m_BloodHZ.GetHealthScaled());

		s_fSaturation = Math.Clamp(bloodLevel, 0, 1);
		s_bEnableSaturation = s_fSaturation < 1;
	}

	//------------------------------------------------------------------------------------------------
	protected override void ClearEffects()
	{
		s_fSaturation						= 1;
	}
	
}
