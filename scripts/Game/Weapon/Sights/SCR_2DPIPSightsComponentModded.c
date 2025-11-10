
//------------------------------------------------------------------------------------------------
modded class SCR_2DPIPSightsComponent : SCR_2DSightsComponent
{
	[RplProp()]
	bool Broken;
	
	
	void SetBroken(bool val){
	
	Broken = val;
		Replication.BumpMe();
	}
	//------------------------------------------------------------------------------------------------
	 protected Widget CreateUIMod(string layout, string rtTextureName, string rtName, out RTTextureWidget RTTexture, out RenderTargetWidget RTWidget, out ImageWidget brokenImage)
	{
		// Empty layout, cannot create any widget
		if (layout == string.Empty)
			return null;

		// Create layout
		Widget root = GetGame().GetWorkspace().CreateWidgets(layout);

		// Layout was not created successfully
		if (!root)
			return null;

		// We dont have required RT widgets, delete layout and terminate
		RTTexture = RTTextureWidget.Cast(root.FindAnyWidget(rtTextureName));
		RTWidget = RenderTargetWidget.Cast(root.FindAnyWidget(rtName));
		brokenImage = ImageWidget.Cast(root.FindAnyWidget("Broken"));
		
		if (!RTTexture || !RTWidget)
		{
			root.RemoveFromHierarchy();
			return null;
		}

		return root;
	}
	float viewDistance;
	//------------------------------------------------------------------------------------------------
	override void SetPIPEnabled(bool enabled)
	{// disabled->enabled
		// Create neccessary items
		if (enabled && !m_bPIPIsEnabled)
		{
			// Try to create UI for PIP,
			// output params are either set to valid ones,
			// or root itself is set to null and destroyed
			if (!m_wPIPRoot || !m_wRenderTargetTextureWidget || !m_wRenderTargetWidget)
				m_wPIPRoot = CreateUI(m_sPIPLayoutResource, m_sRTTextureWidgetName, m_sRTargetWidgetName, m_wRenderTargetTextureWidget, m_wRenderTargetWidget);

			if (!m_wPIPRoot)
			{
				Print("Could not create PIP layouts!", LogLevel.ERROR);
				return;
			}

			IEntity owner = GetOwner();

			// Create PIP camera
			if (!m_PIPCamera)
			{
				// TODO: restart camera when view distance changes
				float viewDistance = GetGame().GetViewDistance();
				if (m_fFarPlane > 0)
					viewDistance = Math.Min(viewDistance, m_fFarPlane);

				m_PIPCamera = SCR_PIPCamera.Cast(CreateCamera(owner, GetSightsFrontPosition(true) + m_vCameraOffset, m_vCameraAngles, m_iCameraIndex, GetFOV(), m_fNearPlane, viewDistance));
			}

			if (!m_PIPCamera)
			{
				Print("Could not create PIP camera!", LogLevel.ERROR);
				return;
			}

			// Now that everything is ready, we can start observing possible changes of the game settings, so we can react to them
			SCR_MenuHelper.GetOnMenuClose().Insert(OnSettingsMenuClosed);

			// Set camera index of render target widget
			BaseWorld baseWorld = owner.GetWorld();
			m_wRenderTargetWidget.SetWorld(baseWorld, m_iCameraIndex);

			// Set resolution scale
			m_wRenderTargetWidget.SetResolutionScale(m_fResolutionScale, m_fResolutionScale);

			if (!owner.IsDeleted())
				m_wRenderTargetTextureWidget.SetRenderTarget(owner);

			if (m_pMaterial)
				GetGame().GetWorld().SetCameraPostProcessEffect(m_iCameraIndex, 10, PostProcessEffectType.HDR, m_rScopeHDRMatrial);

			if (m_sUnderwaterPPMaterial != string.Empty)
				GetGame().GetWorld().SetCameraPostProcessEffect(m_iCameraIndex, 2, PostProcessEffectType.UnderWater, m_sUnderwaterPPMaterial);

			if (m_sRainPPMaterial != string.Empty)
				GetGame().GetWorld().SetCameraPostProcessEffect(m_iCameraIndex, 4, PostProcessEffectType.Rain, m_sRainPPMaterial);

			s_bIsPIPActive = true;
			m_bPIPIsEnabled = true;
			return;
		}

		// enabled -> disabled
		if (!enabled && m_bPIPIsEnabled)
		{
			Destroy();
			s_bIsPIPActive = false;
			m_bPIPIsEnabled = false;
			return;
		}
	
	}
}
