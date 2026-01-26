//------------------------------------------------------------------------------------------------
// SKIN ADMIN COMMANDS - Console/chat commands for skin management
//------------------------------------------------------------------------------------------------

class DZ_SkinCommands
{
	//------------------------------------------------------------------------------------------------
	// Grant a skin to a player by ID
	// Usage: !grantskin <playerId> <skinId>
	static bool GrantSkin(string playerId, string skinId)
	{
		DZ_SkinManagerComponent skinManager = DZ_SkinManagerComponent.GetInstance();
		if (!skinManager)
		{
			Print("[DZ_SkinCmd] Skin manager not found", LogLevel.ERROR);
			return false;
		}

		bool success = skinManager.GrantSkin(playerId, skinId);
		if (success)
			Print(string.Format("[DZ_SkinCmd] Granted skin '%1' to player %2", skinId, playerId), LogLevel.NORMAL);
		else
			Print(string.Format("[DZ_SkinCmd] Failed to grant skin '%1' to player %2", skinId, playerId), LogLevel.ERROR);

		return success;
	}

	//------------------------------------------------------------------------------------------------
	// Grant a skin to a player by name (partial match)
	// Usage: !grantskinbyname <playerName> <skinId>
	static bool GrantSkinByName(string playerName, string skinId)
	{
		string playerId = FindPlayerIdByName(playerName);
		if (playerId.IsEmpty())
		{
			Print(string.Format("[DZ_SkinCmd] Player not found: %1", playerName), LogLevel.WARNING);
			return false;
		}

		return GrantSkin(playerId, skinId);
	}

	//------------------------------------------------------------------------------------------------
	// Revoke a skin from a player by ID
	// Usage: !revokeskin <playerId> <skinId>
	static bool RevokeSkin(string playerId, string skinId)
	{
		DZ_SkinManagerComponent skinManager = DZ_SkinManagerComponent.GetInstance();
		if (!skinManager)
		{
			Print("[DZ_SkinCmd] Skin manager not found", LogLevel.ERROR);
			return false;
		}

		bool success = skinManager.RevokeSkin(playerId, skinId);
		if (success)
			Print(string.Format("[DZ_SkinCmd] Revoked skin '%1' from player %2", skinId, playerId), LogLevel.NORMAL);
		else
			Print(string.Format("[DZ_SkinCmd] Failed to revoke skin '%1' from player %2", skinId, playerId), LogLevel.ERROR);

		return success;
	}

	//------------------------------------------------------------------------------------------------
	// Revoke a skin from a player by name
	static bool RevokeSkinByName(string playerName, string skinId)
	{
		string playerId = FindPlayerIdByName(playerName);
		if (playerId.IsEmpty())
		{
			Print(string.Format("[DZ_SkinCmd] Player not found: %1", playerName), LogLevel.WARNING);
			return false;
		}

		return RevokeSkin(playerId, skinId);
	}

	//------------------------------------------------------------------------------------------------
	// Grant all skins to a player
	static bool GrantAllSkins(string playerId)
	{
		DZ_SkinManagerComponent skinManager = DZ_SkinManagerComponent.GetInstance();
		if (!skinManager)
		{
			Print("[DZ_SkinCmd] Skin manager not found", LogLevel.ERROR);
			return false;
		}

		array<ref DZ_SkinDefinition> allSkins = skinManager.GetAllSkins();
		if (!allSkins)
			return false;

		int count = 0;
		foreach (DZ_SkinDefinition skin : allSkins)
		{
			if (skin && skinManager.GrantSkin(playerId, skin.m_sSkinId))
				count++;
		}

		Print(string.Format("[DZ_SkinCmd] Granted %1 skins to player %2", count, playerId), LogLevel.NORMAL);
		return count > 0;
	}

	//------------------------------------------------------------------------------------------------
	// Grant all skins to a player by name
	static bool GrantAllSkinsByName(string playerName)
	{
		string playerId = FindPlayerIdByName(playerName);
		if (playerId.IsEmpty())
		{
			Print(string.Format("[DZ_SkinCmd] Player not found: %1", playerName), LogLevel.WARNING);
			return false;
		}

		return GrantAllSkins(playerId);
	}

	//------------------------------------------------------------------------------------------------
	// Find player ID by partial name match
	static string FindPlayerIdByName(string partialName)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return string.Empty;

		array<int> playerIds = new array<int>();
		playerManager.GetPlayers(playerIds);

		string lowerSearch = partialName;
		lowerSearch.ToLower();

		foreach (int pid : playerIds)
		{
			string playerName = playerManager.GetPlayerName(pid);
			string lowerName = playerName;
			lowerName.ToLower();

			if (lowerName.Contains(lowerSearch))
			{
				PlayerController pc = playerManager.GetPlayerController(pid);
				if (pc)
					return pc.GetPlayerId().ToString();
			}
		}

		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	// Get player name from player ID string
	static string GetPlayerNameFromId(string playerId)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return playerId;

		array<int> playerIds = new array<int>();
		playerManager.GetPlayers(playerIds);

		foreach (int pid : playerIds)
		{
			PlayerController pc = playerManager.GetPlayerController(pid);
			if (pc && pc.GetPlayerId().ToString() == playerId)
				return playerManager.GetPlayerName(pid);
		}

		return playerId;
	}

	//------------------------------------------------------------------------------------------------
	// List all available skins
	static void ListSkins()
	{
		DZ_SkinManagerComponent skinManager = DZ_SkinManagerComponent.GetInstance();
		if (!skinManager)
		{
			Print("[DZ_SkinCmd] Skin manager not found", LogLevel.ERROR);
			return;
		}

		array<ref DZ_SkinDefinition> allSkins = skinManager.GetAllSkins();
		if (!allSkins || allSkins.IsEmpty())
		{
			Print("[DZ_SkinCmd] No skins defined", LogLevel.NORMAL);
			return;
		}

		Print("=== Available Skins ===", LogLevel.NORMAL);
		foreach (DZ_SkinDefinition skin : allSkins)
		{
			if (!skin)
				continue;

			string defaultStr = "";
			if (skin.m_bDefaultUnlocked)
				defaultStr = " [DEFAULT]";

			Print(string.Format("  %1: %2%3", skin.m_sSkinId, skin.m_sSkinName, defaultStr), LogLevel.NORMAL);
		}
		Print(string.Format("Total: %1 skins", allSkins.Count()), LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	// List player's owned skins
	static void ListPlayerSkins(string playerId)
	{
		DZ_SkinManagerComponent skinManager = DZ_SkinManagerComponent.GetInstance();
		if (!skinManager)
		{
			Print("[DZ_SkinCmd] Skin manager not found", LogLevel.ERROR);
			return;
		}

		array<string> ownedSkins = skinManager.GetPlayerOwnedSkins(playerId);
		if (!ownedSkins || ownedSkins.IsEmpty())
		{
			Print(string.Format("[DZ_SkinCmd] Player %1 has no skins", playerId), LogLevel.NORMAL);
			return;
		}

		string playerName = GetPlayerNameFromId(playerId);
		Print(string.Format("=== Player %1 (%2) Skins ===", playerName, playerId), LogLevel.NORMAL);
		foreach (string skinId : ownedSkins)
		{
			DZ_SkinDefinition skin = skinManager.GetSkin(skinId);
			if (skin)
				Print(string.Format("  %1: %2", skinId, skin.m_sSkinName), LogLevel.NORMAL);
			else
				Print(string.Format("  %1: (unknown)", skinId), LogLevel.NORMAL);
		}
		Print(string.Format("Total: %1 skins", ownedSkins.Count()), LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	// List player's owned skins by name
	static void ListPlayerSkinsByName(string playerName)
	{
		string playerId = FindPlayerIdByName(playerName);
		if (playerId.IsEmpty())
		{
			Print(string.Format("[DZ_SkinCmd] Player not found: %1", playerName), LogLevel.WARNING);
			return;
		}

		ListPlayerSkins(playerId);
	}

	//------------------------------------------------------------------------------------------------
	// List all online players with their IDs (useful for admins)
	static void ListOnlinePlayers()
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
		{
			Print("[DZ_SkinCmd] Player manager not found", LogLevel.ERROR);
			return;
		}

		array<int> playerIds = new array<int>();
		playerManager.GetPlayers(playerIds);

		if (playerIds.IsEmpty())
		{
			Print("[DZ_SkinCmd] No players online", LogLevel.NORMAL);
			return;
		}

		Print("=== Online Players ===", LogLevel.NORMAL);
		foreach (int pid : playerIds)
		{
			string playerName = playerManager.GetPlayerName(pid);
			PlayerController pc = playerManager.GetPlayerController(pid);
			if (pc)
			{
				string playerId = pc.GetPlayerId().ToString();
				Print(string.Format("  %1 (ID: %2)", playerName, playerId), LogLevel.NORMAL);
			}
			else
			{
				Print(string.Format("  %1 (ID: unknown)", playerName), LogLevel.NORMAL);
			}
		}
		Print(string.Format("Total: %1 players", playerIds.Count()), LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	// Force save all player skins now
	static void ForceSaveAll()
	{
		DZ_SkinManagerComponent skinManager = DZ_SkinManagerComponent.GetInstance();
		if (!skinManager)
		{
			Print("[DZ_SkinCmd] Skin manager not found", LogLevel.ERROR);
			return;
		}

		skinManager.SaveAllPlayerSkins();
		Print("[DZ_SkinCmd] Forced save of all player skins", LogLevel.NORMAL);
	}
}

//------------------------------------------------------------------------------------------------
// Console command handler (if using console commands)
//------------------------------------------------------------------------------------------------
/*
[ConsoleCommand("grantskin", "Grant a skin to a player", "grantskin <playerId> <skinId>")]
class DZ_GrantSkinCommand : ConsoleCommand
{
	override void Execute(array<string> args)
	{
		if (args.Count() < 2)
		{
			Print("Usage: grantskin <playerId> <skinId>", LogLevel.WARNING);
			return;
		}

		DZ_SkinCommands.GrantSkin(args[0], args[1]);
	}
}

[ConsoleCommand("listskins", "List all available skins")]
class DZ_ListSkinsCommand : ConsoleCommand
{
	override void Execute(array<string> args)
	{
		DZ_SkinCommands.ListSkins();
	}
}
*/
