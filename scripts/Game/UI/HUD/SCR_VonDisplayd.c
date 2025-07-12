//------------------------------------------------------------------------------------------------
//! Modified VON display that hides UI indicators but keeps voice audio
//! Only hides the visual elements, audio functionality remains intact
modded class SCR_VonDisplay
{
	protected bool m_bHideOthersUIEnabled = true; // Set to false to disable mod
	
	//------------------------------------------------------------------------------------------------
	//! Override OnReceive to process audio but hide UI elements
	override event void OnReceive(int playerId, BaseTransceiver receiver, int frequency, float quality)
	{
		if (!m_wRoot || m_bIsVONUIDisabled)
			return;

		// If mod is enabled, don't create any UI elements for incoming transmissions
		if (m_bHideOthersUIEnabled)
		{
			// Audio processing happens at a lower level and isn't affected by this
			// We just return here to prevent UI creation
			return;
		}
		
		// Otherwise, use original behavior
		super.OnReceive(playerId, receiver, frequency, quality);
	}

	//------------------------------------------------------------------------------------------------
	//! Alternative approach: Create transmission data but immediately hide the widgets
	/*
	override event void OnReceive(int playerId, BaseTransceiver receiver, int frequency, float quality)
	{
		if (!m_wRoot || m_bIsVONUIDisabled)
			return;

		// If mod is disabled, use normal behavior
		if (!m_bHideOthersUIEnabled)
		{
			super.OnReceive(playerId, receiver, frequency, quality);
			return;
		}

		// Process transmission data for audio but hide UI
		TransmissionData pTransmission = m_aTransmissionMap.Get(playerId);

		if (!pTransmission)
		{
			if (!receiver && m_bIsVONDirectDisabled)
				return;

			// Create transmission data but don't assign widgets or make visible
			pTransmission = new TransmissionData(null, playerId);
			pTransmission.m_fQuality = quality;
			pTransmission.m_bVisible = false;
			pTransmission.m_bIsActive = false;
			
			m_aTransmissions.Insert(pTransmission);
			m_aTransmissionMap.Insert(playerId, pTransmission);
		}

		// Don't update any visual elements
		return;
	}
	*/

	//------------------------------------------------------------------------------------------------
	//! Override DisplayUpdate to skip incoming transmission UI updates
	override void DisplayUpdate(IEntity owner, float timeSlice)
	{
		// Check if the SlotUIComponent is still valid
		if (m_HUDSlotComponent != m_SlotHandler.GetSlotUIComponent())
		{
			if (m_HUDSlotComponent)
				m_HUDSlotComponent.GetOnResize().Remove(OnSlotUIResize);
			
			m_HUDSlotComponent = m_SlotHandler.GetSlotUIComponent();
			if (!m_HUDSlotComponent)
				return;
			
			m_HUDSlotComponent.GetOnResize().Insert(OnSlotUIResize);
		}
		
		// Always update outgoing transmission (your own)
		if (m_OutTransmission.m_bIsActive)
		{
			m_OutTransmission.m_fActiveTimeout += timeSlice;

			if (m_OutTransmission.m_fActiveTimeout > FADEOUT_TIMER_THRESHOLD)
			{
				m_OutTransmission.m_bIsActive = false;
				m_OutTransmission.m_bIsAnimating = true;
			}
		}

		if (m_OutTransmission.m_bIsAnimating)
			OpacityFade(m_OutTransmission, timeSlice);

		// If mod is enabled, skip all incoming transmission UI processing
		if (m_bHideOthersUIEnabled)
		{
			// Clear any existing transmission data to prevent memory leaks
			m_aTransmissions.Clear();
			m_aTransmissionMap.Clear();
			m_aAdditionalSpeakers.Clear();
			
			// Hide additional speakers widget
			if (m_wAdditionalSpeakersWidget && m_wAdditionalSpeakersWidget.IsVisible())
				m_wAdditionalSpeakersWidget.SetVisible(false);
				
			return;
		}

		// Original incoming transmission processing (when mod is disabled)
		if (m_aTransmissions.IsEmpty())
			return;

		TransmissionData pTransmission;
		int count = m_aTransmissions.Count() - 1;

		for (int i = count; i >= 0; i--)
		{
			if (i > m_aTransmissions.Count() - 1)
			{
				count--;
				continue;
			}

			pTransmission = m_aTransmissions[i];
			bool isAdditional = m_aAdditionalSpeakers.Contains(pTransmission);

			if (pTransmission.m_bIsActive)
			{
				pTransmission.m_fActiveTimeout += timeSlice;

				if (pTransmission.m_fActiveTimeout > FADEOUT_TIMER_THRESHOLD)
				{
					pTransmission.m_bIsActive = false;
					pTransmission.m_bIsAnimating = true;
				}
			}

			if (pTransmission.m_bIsAnimating)
				OpacityFade(pTransmission, timeSlice, isAdditional);

			if (!pTransmission.m_bVisible)
			{
				if (pTransmission.m_iPlayerID)
					m_aTransmissionMap.Remove(pTransmission.m_iPlayerID);

				m_aTransmissions.Remove(i);

				if (isAdditional)
				{
					m_aAdditionalSpeakers.Remove(m_aAdditionalSpeakers.Find(pTransmission));
					m_wAdditionalSpeakersText.SetText("+" + m_aAdditionalSpeakers.Count().ToString());

					if (m_aAdditionalSpeakers.IsEmpty())
						m_wAdditionalSpeakersWidget.SetVisible(false);
				}

				if (m_aAdditionalSpeakers.Count() > 0 && (m_aTransmissions.Count() - m_aAdditionalSpeakers.Count()) < m_aWidgetsIncomingVON.Count())
				{
					m_aAdditionalSpeakers[0].m_bIsActive = false;
					m_aTransmissionMap.Remove(m_aAdditionalSpeakers[0].m_iPlayerID);
					m_aTransmissions.Remove(m_aTransmissions.Find(m_aAdditionalSpeakers[0]));

					OnReceive(m_aAdditionalSpeakers[0].m_iPlayerID, m_aAdditionalSpeakers[0].m_RadioTransceiver, m_aAdditionalSpeakers[0].m_fFrequency, m_aAdditionalSpeakers[0].m_fQuality);

					m_aAdditionalSpeakers.Remove(0);
					m_wAdditionalSpeakersText.SetText("+" + m_aAdditionalSpeakers.Count().ToString());

					if (m_aAdditionalSpeakers.Count() == 0)
						m_wAdditionalSpeakersWidget.SetVisible(false);
				}

				count--;
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Override initialization to optionally hide incoming UI elements
	override protected void InitDisplay()
	{
		// Call parent initialization first
		super.InitDisplay();
		
		// If mod is enabled, hide incoming transmission UI elements
		if (m_bHideOthersUIEnabled)
		{
			// Hide the incoming transmissions layout
			if (m_wVerticalLayout)
				m_wVerticalLayout.SetVisible(false);
				
			// Hide additional speakers widget  
			if (m_wAdditionalSpeakersWidget)
				m_wAdditionalSpeakersWidget.SetVisible(false);
		}
	}
}

