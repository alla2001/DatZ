//------------------------------------------------------------------------------------------------
// PLAYER SKIN COMPONENT - Attach to player characters for skin management in GM Editor
//------------------------------------------------------------------------------------------------

[ComponentEditorProps(category: "DatZ/Player", description: "Manages player skin ownership - editable in GM Editor")]
class DZ_PlayerSkinComponentClass : ScriptComponentClass
{
}

class DZ_PlayerSkinComponent : ScriptComponent
{
	// Editor-visible attributes for GM to grant skins
	[Attribute("", UIWidgets.EditBox, "Player ID (auto-filled on spawn)")]
	protected string m_sPlayerId;

	[Attribute("", UIWidgets.EditBox, "Skin IDs to grant (comma-separated, e.g. 'gold_ak,camo_m4,red_helmet')")]
	protected string m_sGrantSkins;

	[Attribute("", UIWidgets.EditBox, "Skin IDs to revoke (comma-separated)")]
	protected string m_sRevokeSkins;

	[Attribute("false", UIWidgets.CheckBox, "Grant ALL available skins to this player")]
	protected bool m_bGrantAllSkins;

	[Attribute("false", UIWidgets.CheckBox, "Apply changes now (toggle to execute)")]
	protected bool m_bApplyChanges;

	// Runtime tracking
	protected bool m_bInitialized = false;
	protected bool m_bLastApplyState = false;

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

		// Auto-fill player ID
		UpdatePlayerId();

		m_bInitialized = true;
		m_bLastApplyState = m_bApplyChanges;
	}

	//------------------------------------------------------------------------------------------------
	// Called when attributes change in editor
	override void _WB_AfterWorldUpdate(IEntity owner, float timeSlice)
	{
		super._WB_AfterWorldUpdate(owner, timeSlice);

		// Detect toggle of Apply Changes checkbox
		if (m_bApplyChanges && !m_bLastApplyState)
		{
			ApplyPendingChanges();
			m_bApplyChanges = false; // Reset the toggle
		}
		m_bLastApplyState = m_bApplyChanges;
	}

	//------------------------------------------------------------------------------------------------
	// Update player ID from the owner entity
	void UpdatePlayerId()
	{
		IEntity owner = GetOwner();
		if (!owner)
			return;

		// Try to get player ID from PlayerController
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return;

		int playerId = playerManager.GetPlayerIdFromControlledEntity(owner);
		if (playerId > 0)
		{
			PlayerController pc = playerManager.GetPlayerController(playerId);
			if (pc)
				m_sPlayerId = pc.GetPlayerId().ToString();
		}
	}

	//------------------------------------------------------------------------------------------------
	// Apply pending skin grants/revokes
	void ApplyPendingChanges()
	{
		if (m_sPlayerId.IsEmpty())
		{
			UpdatePlayerId();
			if (m_sPlayerId.IsEmpty())
			{
				Print("[DZ_PlayerSkin] Cannot apply changes - no player ID", LogLevel.WARNING);
				return;
			}
		}

		DZ_SkinManagerComponent skinManager = DZ_SkinManagerComponent.GetInstance();
		if (!skinManager)
		{
			Print("[DZ_PlayerSkin] Skin manager not found", LogLevel.ERROR);
			return;
		}

		// Grant all skins if requested
		if (m_bGrantAllSkins)
		{
			GrantAllSkins(skinManager);
			m_bGrantAllSkins = false; // Reset
		}

		// Grant specific skins
		if (!m_sGrantSkins.IsEmpty())
		{
			GrantSkins(skinManager, m_sGrantSkins);
			m_sGrantSkins = ""; // Clear after applying
		}

		// Revoke specific skins
		if (!m_sRevokeSkins.IsEmpty())
		{
			RevokeSkins(skinManager, m_sRevokeSkins);
			m_sRevokeSkins = ""; // Clear after applying
		}

		Print(string.Format("[DZ_PlayerSkin] Applied skin changes for player %1", m_sPlayerId), LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	protected void GrantAllSkins(DZ_SkinManagerComponent skinManager)
	{
		array<ref DZ_SkinDefinition> allSkins = skinManager.GetAllSkins();
		if (!allSkins)
			return;

		int count = 0;
		foreach (DZ_SkinDefinition skin : allSkins)
		{
			if (skin && skinManager.GrantSkin(m_sPlayerId, skin.m_sSkinId))
				count++;
		}

		Print(string.Format("[DZ_PlayerSkin] Granted %1 skins to player %2", count, m_sPlayerId), LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	protected void GrantSkins(DZ_SkinManagerComponent skinManager, string skinIds)
	{
		array<string> skins = ParseSkinIds(skinIds);

		foreach (string skinId : skins)
		{
			if (skinManager.GrantSkin(m_sPlayerId, skinId))
				Print(string.Format("[DZ_PlayerSkin] Granted skin '%1' to player %2", skinId, m_sPlayerId), LogLevel.NORMAL);
			else
				Print(string.Format("[DZ_PlayerSkin] Failed to grant skin '%1' to player %2", skinId, m_sPlayerId), LogLevel.WARNING);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void RevokeSkins(DZ_SkinManagerComponent skinManager, string skinIds)
	{
		array<string> skins = ParseSkinIds(skinIds);

		foreach (string skinId : skins)
		{
			if (skinManager.RevokeSkin(m_sPlayerId, skinId))
				Print(string.Format("[DZ_PlayerSkin] Revoked skin '%1' from player %2", skinId, m_sPlayerId), LogLevel.NORMAL);
			else
				Print(string.Format("[DZ_PlayerSkin] Failed to revoke skin '%1' from player %2", skinId, m_sPlayerId), LogLevel.WARNING);
		}
	}

	//------------------------------------------------------------------------------------------------
	// Parse comma-separated skin IDs
	protected array<string> ParseSkinIds(string input)
	{
		array<string> result = new array<string>();

		if (input.IsEmpty())
			return result;

		// Split by comma
		array<string> parts = new array<string>();
		input.Split(",", parts, true);

		foreach (string part : parts)
		{
			string trimmed = part.Trim();
			if (!trimmed.IsEmpty())
				result.Insert(trimmed);
		}

		return result;
	}

	//------------------------------------------------------------------------------------------------
	// Get player's owned skins (for display/debugging)
	array<string> GetOwnedSkins()
	{
		if (m_sPlayerId.IsEmpty())
			return null;

		DZ_SkinManagerComponent skinManager = DZ_SkinManagerComponent.GetInstance();
		if (!skinManager)
			return null;

		return skinManager.GetPlayerOwnedSkins(m_sPlayerId);
	}

	//------------------------------------------------------------------------------------------------
	// Get player ID
	string GetPlayerId()
	{
		return m_sPlayerId;
	}

	//------------------------------------------------------------------------------------------------
	// Set player ID manually (for testing)
	void SetPlayerId(string playerId)
	{
		m_sPlayerId = playerId;
	}
}

//------------------------------------------------------------------------------------------------
// GM EDITOR ACTION - Grant skin to selected entity
// This can be used as a context action in GM Editor
//------------------------------------------------------------------------------------------------
class DZ_GMGrantSkinAction
{
	//------------------------------------------------------------------------------------------------
	// Grant a skin to the player controlling the given entity
	static bool GrantSkinToEntity(IEntity entity, string skinId)
	{
		if (!entity)
			return false;

		// Get player ID from entity
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return false;

		int playerId = playerManager.GetPlayerIdFromControlledEntity(entity);
		if (playerId <= 0)
			return false;

		PlayerController pc = playerManager.GetPlayerController(playerId);
		if (!pc)
			return false;

		string playerIdStr = pc.GetPlayerId().ToString();

		// Grant the skin
		DZ_SkinManagerComponent skinManager = DZ_SkinManagerComponent.GetInstance();
		if (!skinManager)
			return false;

		return skinManager.GrantSkin(playerIdStr, skinId);
	}

	//------------------------------------------------------------------------------------------------
	// Grant all skins to the player controlling the given entity
	static int GrantAllSkinsToEntity(IEntity entity)
	{
		if (!entity)
			return 0;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return 0;

		int playerId = playerManager.GetPlayerIdFromControlledEntity(entity);
		if (playerId <= 0)
			return 0;

		PlayerController pc = playerManager.GetPlayerController(playerId);
		if (!pc)
			return 0;

		string playerIdStr = pc.GetPlayerId().ToString();

		DZ_SkinManagerComponent skinManager = DZ_SkinManagerComponent.GetInstance();
		if (!skinManager)
			return 0;

		array<ref DZ_SkinDefinition> allSkins = skinManager.GetAllSkins();
		if (!allSkins)
			return 0;

		int count = 0;
		foreach (DZ_SkinDefinition skin : allSkins)
		{
			if (skin && skinManager.GrantSkin(playerIdStr, skin.m_sSkinId))
				count++;
		}

		return count;
	}
}
