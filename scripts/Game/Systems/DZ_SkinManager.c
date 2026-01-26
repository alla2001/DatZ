//------------------------------------------------------------------------------------------------
// SKIN SYSTEM - Allows players to apply custom materials to objects
//------------------------------------------------------------------------------------------------

// Skin definition - defines a skin that can be applied to items
[BaseContainerProps(), SCR_BaseContainerCustomTitleField("m_sSkinName")]
class DZ_SkinDefinition
{
	[Attribute("", desc: "Unique skin identifier")]
	string m_sSkinId;

	[Attribute("", desc: "Display name for the skin")]
	string m_sSkinName;

	[Attribute("", desc: "Description of the skin")]
	string m_sDescription;

	[Attribute("", UIWidgets.ResourcePickerThumbnail, "Material to apply", params: "emat")]
	ResourceName m_Material;

	[Attribute("", UIWidgets.ResourcePickerThumbnail, "Preview image", params: "edds")]
	ResourceName m_PreviewImage;

	[Attribute("", UIWidgets.Object, "Prefabs this skin can be applied to")]
	ref array<ResourceName> m_aCompatiblePrefabs;

	[Attribute("false", UIWidgets.CheckBox, "Is this skin available to everyone by default")]
	bool m_bDefaultUnlocked;

	[Attribute("0", desc: "Price to purchase this skin (0 = free if unlocked)")]
	int m_iPrice;

	//------------------------------------------------------------------------------------------------
	bool IsCompatibleWith(ResourceName prefab)
	{
		if (!m_aCompatiblePrefabs || m_aCompatiblePrefabs.IsEmpty())
			return false;

		foreach (ResourceName compatible : m_aCompatiblePrefabs)
		{
			if (compatible == prefab)
				return true;
		}

		return false;
	}
}

// Player's skin data - tracks owned and applied skins
[BaseContainerProps()]
class DZ_PlayerSkinData
{
	string m_sPlayerId;
	ref array<string> m_aOwnedSkins = new array<string>();
	ref map<string, string> m_mAppliedSkins = new map<string, string>(); // prefab -> skinId

	void DZ_PlayerSkinData()
	{
		m_aOwnedSkins = new array<string>();
		m_mAppliedSkins = new map<string, string>();
	}
}

//------------------------------------------------------------------------------------------------
// Skin Manager Component - attach to GameMode
//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "DatZ/Systems", description: "Manages player skins for items")]
class DZ_SkinManagerComponentClass : SCR_BaseGameModeComponentClass
{
}

class DZ_SkinManagerComponent : SCR_BaseGameModeComponent
{
	[Attribute("", UIWidgets.Object, "Available skins")]
	protected ref array<ref DZ_SkinDefinition> m_aSkins;

	[Attribute("300000", UIWidgets.EditBox, "Auto-save interval in milliseconds (default 5 minutes)")]
	protected int m_iAutoSaveIntervalMs;

	[Attribute("true", UIWidgets.CheckBox, "Enable auto-save of player skins")]
	protected bool m_bEnableAutoSave;

	// Runtime data
	protected ref map<string, ref DZ_PlayerSkinData> m_mPlayerSkins = new map<string, ref DZ_PlayerSkinData>();
	protected ref map<string, ref DZ_SkinDefinition> m_mSkinLookup = new map<string, ref DZ_SkinDefinition>();
	protected ref set<string> m_sDirtyPlayers = new set<string>(); // Players with unsaved changes

	protected static DZ_SkinManagerComponent s_Instance;

	//------------------------------------------------------------------------------------------------
	static DZ_SkinManagerComponent GetInstance()
	{
		return s_Instance;
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		s_Instance = this;

		// Build skin lookup map
		if (m_aSkins)
		{
			foreach (DZ_SkinDefinition skin : m_aSkins)
			{
				if (skin && !skin.m_sSkinId.IsEmpty())
					m_mSkinLookup.Set(skin.m_sSkinId, skin);
			}
		}

		// Start auto-save timer on server
		if (Replication.IsServer() && m_bEnableAutoSave && m_iAutoSaveIntervalMs > 0)
		{
			GetGame().GetCallqueue().CallLater(AutoSaveAllPlayers, m_iAutoSaveIntervalMs, true);
		}

		Print(string.Format("[DZ_SkinManager] Initialized with %1 skins", m_mSkinLookup.Count()), LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	override void OnGameModeStart()
	{
		super.OnGameModeStart();

		// Subscribe to player events
		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetOwner());
		if (gameMode)
		{
			gameMode.GetOnPlayerConnected().Insert(HandlePlayerConnectedEvent);
			gameMode.GetOnPlayerDisconnected().Insert(HandlePlayerDisconnectedEvent);
		}
	}

	//------------------------------------------------------------------------------------------------
	override void OnGameModeEnd(SCR_GameModeEndData data)
	{
		super.OnGameModeEnd(data);

		// Save all player data on game end
		SaveAllPlayerSkins();

		// Unsubscribe from player events
		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetOwner());
		if (gameMode)
		{
			gameMode.GetOnPlayerConnected().Remove(HandlePlayerConnectedEvent);
			gameMode.GetOnPlayerDisconnected().Remove(HandlePlayerDisconnectedEvent);
		}
	}

	//------------------------------------------------------------------------------------------------
	// Event handler for player connection
	protected void HandlePlayerConnectedEvent(int playerId)
	{
		PlayerController pc = GetGame().GetPlayerManager().GetPlayerController(playerId);
		if (pc)
		{
			string playerIdStr = pc.GetPlayerId().ToString();
			HandlePlayerConnect(playerIdStr);
		}
	}

	//------------------------------------------------------------------------------------------------
	// Event handler for player disconnection
	protected void HandlePlayerDisconnectedEvent(int playerId, KickCauseGroup cause, int timeout)
	{
		PlayerController pc = GetGame().GetPlayerManager().GetPlayerController(playerId);
		if (pc)
		{
			string playerIdStr = pc.GetPlayerId().ToString();
			HandlePlayerDisconnect(playerIdStr);
		}
	}

	//------------------------------------------------------------------------------------------------
	// Auto-save all dirty players
	protected void AutoSaveAllPlayers()
	{
		if (m_sDirtyPlayers.IsEmpty())
			return;

		int savedCount = 0;
		foreach (string playerId : m_sDirtyPlayers)
		{
			if (SavePlayerSkins(playerId))
				savedCount++;
		}

		m_sDirtyPlayers.Clear();

		if (savedCount > 0)
			Print(string.Format("[DZ_SkinManager] Auto-saved skins for %1 players", savedCount), LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	// Save all player skins (for shutdown)
	void SaveAllPlayerSkins()
	{
		int savedCount = 0;
		foreach (string playerId, DZ_PlayerSkinData data : m_mPlayerSkins)
		{
			if (SavePlayerSkins(playerId))
				savedCount++;
		}

		Print(string.Format("[DZ_SkinManager] Saved skins for %1 players on shutdown", savedCount), LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	// Mark player as having unsaved changes
	protected void MarkPlayerDirty(string playerId)
	{
		m_sDirtyPlayers.Insert(playerId);
	}

	//------------------------------------------------------------------------------------------------
	// Get all available skins
	array<ref DZ_SkinDefinition> GetAllSkins()
	{
		return m_aSkins;
	}

	//------------------------------------------------------------------------------------------------
	// Get skin by ID
	DZ_SkinDefinition GetSkin(string skinId)
	{
		if (m_mSkinLookup.Contains(skinId))
			return m_mSkinLookup.Get(skinId);
		return null;
	}

	//------------------------------------------------------------------------------------------------
	// Get skins compatible with a prefab
	array<ref DZ_SkinDefinition> GetSkinsForPrefab(ResourceName prefab)
	{
		array<ref DZ_SkinDefinition> compatible = new array<ref DZ_SkinDefinition>();

		foreach (DZ_SkinDefinition skin : m_aSkins)
		{
			if (skin && skin.IsCompatibleWith(prefab))
				compatible.Insert(skin);
		}

		return compatible;
	}

	//------------------------------------------------------------------------------------------------
	// Check if player owns a skin
	bool PlayerOwnsSkin(string playerId, string skinId)
	{
		// Check if skin is default unlocked
		DZ_SkinDefinition skin = GetSkin(skinId);
		if (skin && skin.m_bDefaultUnlocked)
			return true;

		// Check player ownership
		if (!m_mPlayerSkins.Contains(playerId))
			return false;

		DZ_PlayerSkinData playerData = m_mPlayerSkins.Get(playerId);
		return playerData.m_aOwnedSkins.Contains(skinId);
	}

	//------------------------------------------------------------------------------------------------
	// Grant skin to player
	bool GrantSkin(string playerId, string skinId)
	{
		DZ_SkinDefinition skin = GetSkin(skinId);
		if (!skin)
		{
			Print(string.Format("[DZ_SkinManager] Skin not found: %1", skinId), LogLevel.WARNING);
			return false;
		}

		// Ensure player data exists
		if (!m_mPlayerSkins.Contains(playerId))
			m_mPlayerSkins.Set(playerId, new DZ_PlayerSkinData());

		DZ_PlayerSkinData playerData = m_mPlayerSkins.Get(playerId);
		playerData.m_sPlayerId = playerId;

		// Check if already owned
		if (playerData.m_aOwnedSkins.Contains(skinId))
			return true;

		playerData.m_aOwnedSkins.Insert(skinId);
		MarkPlayerDirty(playerId);
		Print(string.Format("[DZ_SkinManager] Granted skin '%1' to player %2", skinId, playerId), LogLevel.NORMAL);

		return true;
	}

	//------------------------------------------------------------------------------------------------
	// Revoke skin from player
	bool RevokeSkin(string playerId, string skinId)
	{
		if (!m_mPlayerSkins.Contains(playerId))
			return false;

		DZ_PlayerSkinData playerData = m_mPlayerSkins.Get(playerId);

		int index = playerData.m_aOwnedSkins.Find(skinId);
		if (index < 0)
			return false;

		playerData.m_aOwnedSkins.Remove(index);
		MarkPlayerDirty(playerId);
		Print(string.Format("[DZ_SkinManager] Revoked skin '%1' from player %2", skinId, playerId), LogLevel.NORMAL);

		return true;
	}

	//------------------------------------------------------------------------------------------------
	// Get player's owned skins
	array<string> GetPlayerOwnedSkins(string playerId)
	{
		array<string> ownedSkins = new array<string>();

		// Add default unlocked skins
		foreach (DZ_SkinDefinition skin : m_aSkins)
		{
			if (skin && skin.m_bDefaultUnlocked)
				ownedSkins.Insert(skin.m_sSkinId);
		}

		// Add player-specific skins
		if (m_mPlayerSkins.Contains(playerId))
		{
			DZ_PlayerSkinData playerData = m_mPlayerSkins.Get(playerId);
			foreach (string skinId : playerData.m_aOwnedSkins)
			{
				if (!ownedSkins.Contains(skinId))
					ownedSkins.Insert(skinId);
			}
		}

		return ownedSkins;
	}

	//------------------------------------------------------------------------------------------------
	// Apply skin to an entity
	bool ApplySkin(string playerId, IEntity entity, string skinId)
	{
		if (!entity)
			return false;

		// Get entity's prefab
		ResourceName prefab = entity.GetPrefabData().GetPrefabName();

		// Check if player owns the skin
		if (!PlayerOwnsSkin(playerId, skinId))
		{
			Print(string.Format("[DZ_SkinManager] Player %1 doesn't own skin %2", playerId, skinId), LogLevel.WARNING);
			return false;
		}

		// Get skin definition
		DZ_SkinDefinition skin = GetSkin(skinId);
		if (!skin)
			return false;

		// Check compatibility
		if (!skin.IsCompatibleWith(prefab))
		{
			Print(string.Format("[DZ_SkinManager] Skin %1 not compatible with %2", skinId, prefab), LogLevel.WARNING);
			return false;
		}

		// Apply the material
		bool success = ApplyMaterialToEntity(entity, skin.m_Material);

		if (success)
		{
			// Track applied skin
			if (!m_mPlayerSkins.Contains(playerId))
				m_mPlayerSkins.Set(playerId, new DZ_PlayerSkinData());

			DZ_PlayerSkinData playerData = m_mPlayerSkins.Get(playerId);
			playerData.m_mAppliedSkins.Set(prefab, skinId);

			Print(string.Format("[DZ_SkinManager] Applied skin '%1' to entity", skinId), LogLevel.NORMAL);
		}

		return success;
	}

	//------------------------------------------------------------------------------------------------
	// Remove skin from entity (restore default)
	bool RemoveSkin(IEntity entity)
	{
		if (!entity)
			return false;

		// TODO: Restore original material
		// This requires storing the original material when applying

		return true;
	}

	//------------------------------------------------------------------------------------------------
	// Apply material to entity and its children
	protected bool ApplyMaterialToEntity(IEntity entity, ResourceName materialPath)
	{
		if (!entity || materialPath.IsEmpty())
			return false;

		// Use SCR_Global.SetMaterial which handles the material application properly
		SCR_Global.SetMaterial(entity, materialPath);

		return true;
	}

	//------------------------------------------------------------------------------------------------
	// Get currently applied skin for a prefab
	string GetAppliedSkin(string playerId, ResourceName prefab)
	{
		if (!m_mPlayerSkins.Contains(playerId))
			return string.Empty;

		DZ_PlayerSkinData playerData = m_mPlayerSkins.Get(playerId);

		if (playerData.m_mAppliedSkins.Contains(prefab))
			return playerData.m_mAppliedSkins.Get(prefab);

		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	// Save player skin data to JSON
	bool SavePlayerSkins(string playerId)
	{
		if (!m_mPlayerSkins.Contains(playerId))
			return false;

		DZ_PlayerSkinData playerData = m_mPlayerSkins.Get(playerId);

		string filename = string.Format("$profile:skins_%1.json", playerId);

		SCR_JsonSaveContext saveContext = new SCR_JsonSaveContext();
		if (!saveContext.WriteValue("", playerData))
			return false;

		return saveContext.SaveToFile(filename);
	}

	//------------------------------------------------------------------------------------------------
	// Load player skin data from JSON
	bool LoadPlayerSkins(string playerId)
	{
		string filename = string.Format("$profile:skins_%1.json", playerId);

		SCR_JsonLoadContext loadContext = new SCR_JsonLoadContext();
		if (!loadContext.LoadFromFile(filename))
			return false;

		ref DZ_PlayerSkinData playerData;
		if (!loadContext.ReadValue("", playerData))
			return false;

		m_mPlayerSkins.Set(playerId, playerData);
		Print(string.Format("[DZ_SkinManager] Loaded %1 skins for player %2", playerData.m_aOwnedSkins.Count(), playerId), LogLevel.NORMAL);

		return true;
	}

	//------------------------------------------------------------------------------------------------
	// Called when player connects - load their skins
	void HandlePlayerConnect(string playerId)
	{
		LoadPlayerSkins(playerId);
	}

	//------------------------------------------------------------------------------------------------
	// Called when player disconnects - save their skins
	void HandlePlayerDisconnect(string playerId)
	{
		SavePlayerSkins(playerId);
	}
}
