[ComponentEditorProps(category: "GameScripted/Camera", description: "")]
class  SCR_DatZPostProcessCameraComponentClass : SCR_BaseCameraComponentClass
{
}

//! Post-process effect of scripted camera.
class SCR_DatZPostProcessCameraComponent : SCR_BaseCameraComponent
{
	[Attribute()]
	private ref array<ref SCR_CameraPostProcessEffect> m_Effects;
	
	private CameraBase m_Camera;
	
	//------------------------------------------------------------------------------------------------
	//! Get effect of given type.
	//! \param type Post-process effect type
	//! \return Effect
	SCR_CameraPostProcessEffect FindEffect(PostProcessEffectType type)
	{
		foreach (SCR_CameraPostProcessEffect effect: m_Effects)
		{
			if (type == effect.GetType())
				return effect;
		}
		return null;
	}
	
	//------------------------------------------------------------------------------------------------
	protected void OnCameraActivate()
	{		
		foreach (SCR_CameraPostProcessEffect effect: m_Effects)
		{
			effect.CreateEffect(m_Camera.GetCameraIndex());
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void OnCameraDeactivate()
	{
		foreach (SCR_CameraPostProcessEffect effect: m_Effects)
		{
			effect.DeleteEffect();
		}
	}
	

}

