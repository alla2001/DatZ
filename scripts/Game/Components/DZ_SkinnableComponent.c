//------------------------------------------------------------------------------------------------
// SKINNABLE COMPONENT - Attach to items that can have skins applied
//------------------------------------------------------------------------------------------------

[ComponentEditorProps(category: "DatZ/Items", description: "Makes this item skinnable")]
class DZ_SkinnableComponentClass : ScriptComponentClass
{
}

class DZ_SkinnableComponent : ScriptComponent
{
	// Default material for this item (set in prefab editor)
	[Attribute("", UIWidgets.ResourcePickerThumbnail, "Default material for this item (used when removing skin)", params: "emat")]
	protected ResourceName m_DefaultMaterial;

	// The currently applied skin ID (replicated)
	[RplProp(onRplName: "OnSkinChanged")]
	protected string m_sCurrentSkinId;

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);
	}

	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);

		// If we have a skin applied, re-apply it visually (for persistence reload)
		if (!m_sCurrentSkinId.IsEmpty())
		{
			DZ_SkinManagerComponent skinManager = DZ_SkinManagerComponent.GetInstance();
			if (skinManager)
			{
				DZ_SkinDefinition skin = skinManager.GetSkin(m_sCurrentSkinId);
				if (skin)
					ApplySkinVisual(skin);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	// Get the item's prefab name for skin compatibility checking
	ResourceName GetPrefabName()
	{
		IEntity owner = GetOwner();
		if (!owner)
			return ResourceName.Empty;

		EntityPrefabData prefabData = owner.GetPrefabData();
		if (!prefabData)
			return ResourceName.Empty;

		return prefabData.GetPrefabName();
	}

	//------------------------------------------------------------------------------------------------
	// Get current skin ID
	string GetCurrentSkinId()
	{
		return m_sCurrentSkinId;
	}

	//------------------------------------------------------------------------------------------------
	// Check if a skin is applied
	bool HasSkinApplied()
	{
		return !m_sCurrentSkinId.IsEmpty();
	}

	//------------------------------------------------------------------------------------------------
	// Apply a skin (server-side)
	bool ApplySkin(string playerId, string skinId)
	{
		if (!Replication.IsServer())
			return false;

		DZ_SkinManagerComponent skinManager = DZ_SkinManagerComponent.GetInstance();
		if (!skinManager)
			return false;

		// Validate ownership
		if (!skinManager.PlayerOwnsSkin(playerId, skinId))
		{
			Print(string.Format("[DZ_Skinnable] Player %1 doesn't own skin %2", playerId, skinId), LogLevel.WARNING);
			return false;
		}

		// Validate compatibility
		DZ_SkinDefinition skin = skinManager.GetSkin(skinId);
		if (!skin)
			return false;

		ResourceName prefab = GetPrefabName();
		if (!skin.IsCompatibleWith(prefab))
		{
			Print(string.Format("[DZ_Skinnable] Skin %1 not compatible with %2", skinId, prefab), LogLevel.WARNING);
			return false;
		}

		// Set the skin ID (triggers replication)
		m_sCurrentSkinId = skinId;
		Replication.BumpMe();

		// Apply visually on server
		ApplySkinVisual(skin);

		return true;
	}

	//------------------------------------------------------------------------------------------------
	// Remove skin (restore original)
	bool RemoveSkin()
	{
		if (!Replication.IsServer())
			return false;

		m_sCurrentSkinId = string.Empty;
		Replication.BumpMe();

		// Restore original materials
		RestoreOriginalMaterials();

		return true;
	}

	//------------------------------------------------------------------------------------------------
	// Called when skin changes (replication callback)
	protected void OnSkinChanged()
	{
		if (m_sCurrentSkinId.IsEmpty())
		{
			RestoreOriginalMaterials();
			return;
		}

		DZ_SkinManagerComponent skinManager = DZ_SkinManagerComponent.GetInstance();
		if (!skinManager)
			return;

		DZ_SkinDefinition skin = skinManager.GetSkin(m_sCurrentSkinId);
		if (skin)
			ApplySkinVisual(skin);
	}

	//------------------------------------------------------------------------------------------------
	// Apply skin material visually
	protected void ApplySkinVisual(DZ_SkinDefinition skin)
	{
		if (!skin || skin.m_Material.IsEmpty())
			return;

		IEntity owner = GetOwner();
		if (!owner)
			return;

		// Use SCR_Global.SetMaterial - false = don't apply to children
		SCR_Global.SetMaterial(owner, skin.m_Material, false);
	}

	//------------------------------------------------------------------------------------------------
	// Restore original materials
	protected void RestoreOriginalMaterials()
	{
		if (m_DefaultMaterial.IsEmpty())
		{
			Print("[DZ_Skinnable] No default material set - cannot restore original appearance", LogLevel.WARNING);
			return;
		}

		IEntity owner = GetOwner();
		if (!owner)
			return;

		// Apply the default material - false = don't apply to children
		SCR_Global.SetMaterial(owner, m_DefaultMaterial, false);
	}

	//------------------------------------------------------------------------------------------------
	// Set the current skin ID directly (used by persistence)
	void SetCurrentSkinId(string skinId)
	{
		m_sCurrentSkinId = skinId;
	}

	//------------------------------------------------------------------------------------------------
	// Get default material path
	ResourceName GetDefaultMaterial()
	{
		return m_DefaultMaterial;
	}

	//------------------------------------------------------------------------------------------------
	// Get available skins for this item
	array<ref DZ_SkinDefinition> GetAvailableSkins()
	{
		DZ_SkinManagerComponent skinManager = DZ_SkinManagerComponent.GetInstance();
		if (!skinManager)
			return null;

		return skinManager.GetSkinsForPrefab(GetPrefabName());
	}

	//------------------------------------------------------------------------------------------------
	// Get skins that a player owns for this item
	array<ref DZ_SkinDefinition> GetPlayerAvailableSkins(string playerId)
	{
		DZ_SkinManagerComponent skinManager = DZ_SkinManagerComponent.GetInstance();
		if (!skinManager)
			return null;

		array<ref DZ_SkinDefinition> available = new array<ref DZ_SkinDefinition>();
		ResourceName prefab = GetPrefabName();

		array<string> ownedSkins = skinManager.GetPlayerOwnedSkins(playerId);
		foreach (string skinId : ownedSkins)
		{
			DZ_SkinDefinition skin = skinManager.GetSkin(skinId);
			if (skin && skin.IsCompatibleWith(prefab))
				available.Insert(skin);
		}

		return available;
	}
}
