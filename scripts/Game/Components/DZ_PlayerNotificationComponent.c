//------------------------------------------------------------------------------------------------
// PLAYER NOTIFICATION COMPONENT - Receives messages from server and displays them
// Add this to your player character prefab
//------------------------------------------------------------------------------------------------

[ComponentEditorProps(category: "DatZ/Player", description: "Allows server to send notifications to this player")]
class DZ_PlayerNotificationComponentClass : ScriptComponentClass
{
}

class DZ_PlayerNotificationComponent : ScriptComponent
{
	//------------------------------------------------------------------------------------------------
	// Send a notification to this player (call from server)
	void SendNotification(string message, float duration = 5.0)
	{
		Rpc(RpcDo_ShowNotification, message, duration);
	}

	//------------------------------------------------------------------------------------------------
	// Send a hint message to this player (call from server)
	void SendHint(string title, string message, float duration = 5.0)
	{
		Rpc(RpcDo_ShowHint, title, message, duration);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_ShowNotification(string message, float duration)
	{
		SCR_PopUpNotification popup = SCR_PopUpNotification.GetInstance();
		if (popup)
		{
			popup.PopupMsg(message, duration);
		}
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_ShowHint(string title, string message, float duration)
	{
		SCR_HintManagerComponent hintManager = SCR_HintManagerComponent.GetInstance();
		if (hintManager)
		{
			hintManager.ShowCustomHint(message, title, duration);
		}
	}

	//------------------------------------------------------------------------------------------------
	// Static helper: Send notification to a specific player by ID
	static void NotifyPlayer(int playerId, string message, float duration = 5.0)
	{
		PlayerManager pm = GetGame().GetPlayerManager();
		if (!pm)
			return;

		PlayerController pc = pm.GetPlayerController(playerId);
		if (!pc)
			return;

		IEntity controlledEntity = pc.GetControlledEntity();
		if (!controlledEntity)
			return;

		DZ_PlayerNotificationComponent notifComp = DZ_PlayerNotificationComponent.Cast(controlledEntity.FindComponent(DZ_PlayerNotificationComponent));
		if (notifComp)
		{
			notifComp.SendNotification(message, duration);
		}
	}

	//------------------------------------------------------------------------------------------------
	// Static helper: Send hint to a specific player by ID
	static void HintPlayer(int playerId, string title, string message, float duration = 5.0)
	{
		PlayerManager pm = GetGame().GetPlayerManager();
		if (!pm)
			return;

		PlayerController pc = pm.GetPlayerController(playerId);
		if (!pc)
			return;

		IEntity controlledEntity = pc.GetControlledEntity();
		if (!controlledEntity)
			return;

		DZ_PlayerNotificationComponent notifComp = DZ_PlayerNotificationComponent.Cast(controlledEntity.FindComponent(DZ_PlayerNotificationComponent));
		if (notifComp)
		{
			notifComp.SendHint(title, message, duration);
		}
	}
}
