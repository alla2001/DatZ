//------------------------------------------------------------------------------------------------
// PERSISTENCE - Save data for DZ_SkinnableComponent
//------------------------------------------------------------------------------------------------

[EPF_ComponentSaveDataType(DZ_SkinnableComponent), BaseContainerProps()]
class EPF_SkinnableComponentSaveDataClass : EPF_ComponentSaveDataClass
{
}

[EDF_DbName.Automatic()]
class EPF_SkinnableComponentSaveData : EPF_ComponentSaveData
{
	// The skin ID that was applied to this item
	string m_sSkinId;

	//------------------------------------------------------------------------------------------------
	override EPF_EReadResult ReadFrom(IEntity owner, GenericComponent component, EPF_ComponentSaveDataClass attributes)
	{
		DZ_SkinnableComponent skinnableComp = DZ_SkinnableComponent.Cast(component);
		if (!skinnableComp)
		{
			Debug.Error(string.Format("[EPF_SkinnableSaveData] Component is not DZ_SkinnableComponent: %1", component));
			return EPF_EReadResult.DEFAULT;
		}

		m_sSkinId = skinnableComp.GetCurrentSkinId();

		// If no skin applied, return default (don't save)
		if (m_sSkinId.IsEmpty())
			return EPF_EReadResult.DEFAULT;

		return EPF_EReadResult.OK;
	}

	//------------------------------------------------------------------------------------------------
	override EPF_EApplyResult ApplyTo(IEntity owner, GenericComponent component, EPF_ComponentSaveDataClass attributes)
	{
		DZ_SkinnableComponent skinnableComp = DZ_SkinnableComponent.Cast(component);
		if (!skinnableComp)
		{
			Debug.Error(string.Format("[EPF_SkinnableSaveData] Component is not DZ_SkinnableComponent: %1", component));
			return EPF_EApplyResult.ERROR;
		}

		// If no skin was saved, nothing to apply
		if (m_sSkinId.IsEmpty())
			return EPF_EApplyResult.OK;

		// Set the skin ID (this will be applied visually in EOnInit)
		skinnableComp.SetCurrentSkinId(m_sSkinId);

		// Also apply visually now
		DZ_SkinManagerComponent skinManager = DZ_SkinManagerComponent.GetInstance();
		if (skinManager)
		{
			DZ_SkinDefinition skin = skinManager.GetSkin(m_sSkinId);
			if (skin && !skin.m_Material.IsEmpty())
			{
				SCR_Global.SetMaterial(owner, skin.m_Material);
			}
		}

		return EPF_EApplyResult.OK;
	}

	//------------------------------------------------------------------------------------------------
	override bool Equals(notnull EPF_ComponentSaveData other)
	{
		EPF_SkinnableComponentSaveData otherData = EPF_SkinnableComponentSaveData.Cast(other);
		if (!otherData)
			return false;

		return m_sSkinId == otherData.m_sSkinId;
	}
}
