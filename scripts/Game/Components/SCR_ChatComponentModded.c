//------------------------------------------------------------------------------------------------
// MODDED CHAT COMPONENT - Intercepts chat messages for command processing
//------------------------------------------------------------------------------------------------

modded class SCR_ChatComponent : BaseChatComponent
{
	//------------------------------------------------------------------------------------------------
	override void OnNewMessage(string msg, int channelId, int senderId)
	{
		super.OnNewMessage(msg, channelId, senderId);

		// Only process on server
		if (!Replication.IsServer())
			return;

		// Try to process as command
		DZ_ChatCommandHandler handler = DZ_ChatCommandHandler.GetInstance();
		if (handler)
		{
			handler.ProcessChatMessage(senderId, msg);
		}
	}
}
