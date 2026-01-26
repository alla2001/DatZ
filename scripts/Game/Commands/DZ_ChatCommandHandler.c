//------------------------------------------------------------------------------------------------
// CHAT COMMAND HANDLER - Processes admin commands from chat
// Add this component to your GameMode
//------------------------------------------------------------------------------------------------

[ComponentEditorProps(category: "DatZ/Systems", description: "Handles chat commands for admin functions")]
class DZ_ChatCommandHandlerClass : SCR_BaseGameModeComponentClass
{
}

class DZ_ChatCommandHandler : SCR_BaseGameModeComponent
{
	[Attribute("!", UIWidgets.EditBox, "Command prefix (e.g. ! or /)")]
	protected string m_sCommandPrefix;

	[Attribute("true", UIWidgets.CheckBox, "Only allow admins to use commands")]
	protected bool m_bAdminOnly;

	protected static DZ_ChatCommandHandler s_Instance;

	//------------------------------------------------------------------------------------------------
	static DZ_ChatCommandHandler GetInstance()
	{
		return s_Instance;
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		s_Instance = this;

		if (m_sCommandPrefix.IsEmpty())
			m_sCommandPrefix = "!";

		Print("[DZ_ChatCmd] Chat command handler initialized", LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	// Call this from your chat system when a message is received
	// Returns true if message was a command (and should not be broadcast)
	bool ProcessChatMessage(int senderId, string message)
	{
		if (message.IsEmpty())
			return false;

		// Check if it starts with command prefix
		if (!message.StartsWith(m_sCommandPrefix))
			return false;

		// Check admin permission
		if (m_bAdminOnly && !IsPlayerAdmin(senderId))
		{
			SendMessageToPlayer(senderId, "You don't have permission to use commands.");
			return true;
		}

		// Remove prefix and parse command
		string commandStr = message.Substring(m_sCommandPrefix.Length(), message.Length() - m_sCommandPrefix.Length());

		// Split into command and args
		array<string> parts = new array<string>();
		commandStr.Split(" ", parts, true);

		if (parts.IsEmpty())
			return true;

		string command = parts[0];
		command.ToLower();

		// Remove command from parts to get args
		parts.RemoveOrdered(0);

		// Execute command
		ExecuteCommand(senderId, command, parts);

		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected void ExecuteCommand(int senderId, string command, array<string> args)
	{
		string response = "";

		// Skin commands
		if (command == "grantskin")
		{
			if (args.Count() < 2)
			{
				response = "Usage: !grantskin <playerName> <skinId>";
			}
			else
			{
				bool success = DZ_SkinCommands.GrantSkinByName(args[0], args[1]);
				if (success)
					response = string.Format("Granted skin '%1' to %2", args[1], args[0]);
				else
					response = string.Format("Failed to grant skin '%1' to %2", args[1], args[0]);
			}
		}
		else if (command == "revokeskin")
		{
			if (args.Count() < 2)
			{
				response = "Usage: !revokeskin <playerName> <skinId>";
			}
			else
			{
				bool success = DZ_SkinCommands.RevokeSkinByName(args[0], args[1]);
				if (success)
					response = string.Format("Revoked skin '%1' from %2", args[1], args[0]);
				else
					response = string.Format("Failed to revoke skin '%1' from %2", args[1], args[0]);
			}
		}
		else if (command == "grantallskins")
		{
			if (args.Count() < 1)
			{
				response = "Usage: !grantallskins <playerName>";
			}
			else
			{
				bool success = DZ_SkinCommands.GrantAllSkinsByName(args[0]);
				if (success)
					response = string.Format("Granted all skins to %1", args[0]);
				else
					response = string.Format("Failed to grant skins to %1", args[0]);
			}
		}
		else if (command == "listskins")
		{
			response = GetSkinsListString();
		}
		else if (command == "playerskins")
		{
			if (args.Count() < 1)
			{
				response = "Usage: !playerskins <playerName>";
			}
			else
			{
				response = GetPlayerSkinsString(args[0]);
			}
		}
		else if (command == "players")
		{
			response = GetOnlinePlayersString();
		}
		else if (command == "saveskins")
		{
			DZ_SkinCommands.ForceSaveAll();
			response = "All player skins saved.";
		}
		else if (command == "skinhelp" || command == "help")
		{
			response = "Commands: !grantskin, !revokeskin, !grantallskins, !listskins, !playerskins, !players, !saveskins";
		}
		else
		{
			response = string.Format("Unknown command: %1. Type !skinhelp for commands.", command);
		}

		// Send response
		if (!response.IsEmpty())
		{
			SendMessageToPlayer(senderId, response);
			Print(string.Format("[DZ_ChatCmd] %1", response), LogLevel.NORMAL);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsPlayerAdmin(int playerId)
	{
		// Check if player has GM (Game Master) editor access
		SCR_EditorManagerCore core = SCR_EditorManagerCore.Cast(SCR_EditorManagerCore.GetInstance(SCR_EditorManagerCore));
		if (!core)
			return false;

		// Get player's editor manager
		SCR_EditorManagerEntity editorManager = core.GetEditorManager(playerId);
		if (!editorManager)
			return false;

		// Check if player has editor opened or has GM rights
		if (editorManager.IsOpened())
			return true;

		// Check limited editor (GM mode)
		if (editorManager.IsLimited())
			return false; // Limited editors are not full GMs

		// Player has full editor access = GM
		return editorManager.CanOpen();
	}

	//------------------------------------------------------------------------------------------------
	protected void SendMessageToPlayer(int playerId, string message)
	{
		// Print to server console
		Print(string.Format("[DZ_ChatCmd] -> Player %1: %2", playerId, message), LogLevel.NORMAL);

		// Send notification to player's screen
		DZ_PlayerNotificationComponent.NotifyPlayer(playerId, message, 8.0);
	}

	//------------------------------------------------------------------------------------------------
	// Get online players as string
	protected string GetOnlinePlayersString()
	{
		PlayerManager pm = GetGame().GetPlayerManager();
		if (!pm)
			return "Player manager not found";

		array<int> playerIds = new array<int>();
		pm.GetPlayers(playerIds);

		if (playerIds.IsEmpty())
			return "No players online";

		string result = "Players: ";
		bool first = true;

		foreach (int pid : playerIds)
		{
			string playerName = pm.GetPlayerName(pid);
			if (!first)
				result += ", ";
			result += playerName;
			first = false;
		}

		result += string.Format(" (%1 total)", playerIds.Count());
		return result;
	}

	//------------------------------------------------------------------------------------------------
	// Get all skins as string
	protected string GetSkinsListString()
	{
		DZ_SkinManagerComponent skinManager = DZ_SkinManagerComponent.GetInstance();
		if (!skinManager)
			return "Skin manager not found";

		array<ref DZ_SkinDefinition> allSkins = skinManager.GetAllSkins();
		if (!allSkins || allSkins.IsEmpty())
			return "No skins defined";

		string result = "Skins: ";
		bool first = true;

		foreach (DZ_SkinDefinition skin : allSkins)
		{
			if (!skin)
				continue;

			if (!first)
				result += ", ";
			result += skin.m_sSkinId;
			first = false;
		}

		result += string.Format(" (%1 total)", allSkins.Count());
		return result;
	}

	//------------------------------------------------------------------------------------------------
	// Get player's skins as string
	protected string GetPlayerSkinsString(string playerName)
	{
		string playerId = DZ_SkinCommands.FindPlayerIdByName(playerName);
		if (playerId.IsEmpty())
			return string.Format("Player '%1' not found", playerName);

		DZ_SkinManagerComponent skinManager = DZ_SkinManagerComponent.GetInstance();
		if (!skinManager)
			return "Skin manager not found";

		array<string> ownedSkins = skinManager.GetPlayerOwnedSkins(playerId);
		if (!ownedSkins || ownedSkins.IsEmpty())
			return string.Format("%1 has no skins", playerName);

		string result = string.Format("%1's skins: ", playerName);
		bool first = true;

		foreach (string skinId : ownedSkins)
		{
			if (!first)
				result += ", ";
			result += skinId;
			first = false;
		}

		result += string.Format(" (%1 total)", ownedSkins.Count());
		return result;
	}
}
