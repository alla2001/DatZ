

modded class SCR_ItemPlacementComponent : ScriptComponent
{
	
	void StartPreview(){
		EnablePreview(GetOwner());
	}
	
	//------------------------------------------------------------------------------------------------
	protected override void EOnFrame(IEntity owner, float timeSlice)
	{
		/*if (m_CompartmnetAccessComponent.IsGettingIn())
		{
			DisablePreview();
			return;
		}

		if (!m_EquippedItem)
		{
			// This handles situations where f. e. land mines explode in characters hands
			DisablePreview();
			return;
		}*/

		CameraManager cameraManager = GetGame().GetCameraManager();
		if (!cameraManager)
			return;

		CameraBase currentCamera = cameraManager.CurrentCamera();
		if (!currentCamera)
			return;

		vector cameraMat[4];
		currentCamera.GetTransform(cameraMat);
		float maxPlacementDistance = m_PlaceableItem.GetMaxPlacementDistance();
		SCR_EPlacementType placementType = m_PlaceableItem.GetPlacementType();

		m_eCantPlaceReason = 0;
		switch (placementType)
		{
			case SCR_EPlacementType.XZ_FIXED:
				UseXZFixedPlacement(owner, maxPlacementDistance, cameraMat);
				break;

			case SCR_EPlacementType.XYZ:
				UseXYZPlacement(owner, maxPlacementDistance, cameraMat);
				break;
		}

		if (m_eCantPlaceReason == 0)
			SCR_Global.SetMaterial(m_PreviewEntity, m_sCanBuildMaterial);
		else if (m_eCantPlaceReason == ENotification.PLACEABLE_ITEM_CANT_PLACE_DISTANCE)
			SCR_Global.SetMaterial(m_PreviewEntity, m_sTransparentMaterial);
		else
			SCR_Global.SetMaterial(m_PreviewEntity, m_sCannotBuildMaterial);
	}
}
