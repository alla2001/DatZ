//------------------------------------------------------------------------------------------------
class AirDropGameSystem : GameSystem
{
    [Attribute("", UIWidgets.ResourceNamePicker, "Airdrop prefabs", "et")]
    ref array<ResourceName> m_aAirdropPrefabs;
    
    [Attribute("1000", UIWidgets.EditBox, "Spawn range in meters")]
    float m_fSpawnRangeMinX;
	
	   [Attribute("1000", UIWidgets.EditBox, "Spawn range in meters")]
    float m_fSpawnRangeMaxX;
	   [Attribute("1000", UIWidgets.EditBox, "Spawn range in meters")]
    float m_fSpawnRangeMinY;
	   [Attribute("1000", UIWidgets.EditBox, "Spawn range in meters")]
    float m_fSpawnRangeMaxY;
	
	
    
    [Attribute("500", UIWidgets.EditBox, "Spawn height (Y position)")]
    float m_fSpawnHeight;
    
    [Attribute("7200000", UIWidgets.EditBox, "Spawn interval in milliseconds (2 hours = 7200000)")]
    int m_iSpawnInterval;
    
    protected int m_iTimerID = -1;
    
    //------------------------------------------------------------------------------------------------
    override void OnInit()
    {
        super.OnInit();
        
        Print("AirDropGameSystem: Starting airdrop system", LogLevel.NORMAL);
        if(Replication.IsServer())
        // Start the timer for spawning airdrops
        StartSpawnTimer();
    }
    

	
 
    
    //------------------------------------------------------------------------------------------------
    protected void StartSpawnTimer()
    {
		
		GetGame().GetCallqueue().CallLater(SpawnAirdrop, 60000, false);
		
        // Schedule next airdrop
        GetGame().GetCallqueue().CallLater(SpawnAirdrop, m_iSpawnInterval, true);
        
        Print(string.Format("AirDropGameSystem: Next airdrop in %1 minutes", m_iSpawnInterval / 60000), LogLevel.NORMAL);
    }
    
    //------------------------------------------------------------------------------------------------
    protected void SpawnAirdrop()
    {
        Print("AirDropGameSystem: Spawning airdrop", LogLevel.NORMAL);
        
        // Check if we have any prefabs configured
        if (!m_aAirdropPrefabs || m_aAirdropPrefabs.Count() == 0)
        {
            Print("AirDropGameSystem: No airdrop prefabs configured!", LogLevel.ERROR);
            return;
        }
        
        // Select random prefab from the list
        ResourceName selectedPrefab = m_aAirdropPrefabs.GetRandomElement();
        
        // Generate random position within range
        vector spawnPos = GetRandomSpawnPosition();
        
        // Spawn the airdrop
        EntitySpawnParams spawnParams = EntitySpawnParams();
        spawnParams.TransformMode = ETransformMode.WORLD;
        spawnParams.Transform[3] = spawnPos;
        
        IEntity airdrop = GetGame().SpawnEntityPrefab(Resource.Load(selectedPrefab), GetWorld(), spawnParams);
        
        if (airdrop)
        {
            Print(string.Format("AirDropGameSystem: Airdrop spawned at %1 using prefab %2", spawnPos.ToString(), selectedPrefab), LogLevel.NORMAL);
            AlertAllPlayers(spawnPos);
        }
        else
        {
            Print("AirDropGameSystem: Failed to spawn airdrop!", LogLevel.ERROR);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    protected ResourceName GetRandomAirdropPrefab()
    {
        int randomIndex = Math.RandomInt(0, m_aAirdropPrefabs.Count());
        return m_aAirdropPrefabs[randomIndex];
    }
    
    //------------------------------------------------------------------------------------------------
    protected vector GetRandomSpawnPosition()
    {
        // Generate random X and Z within range, use fixed Y height
        float randomX = Math.RandomFloat(m_fSpawnRangeMinX, m_fSpawnRangeMaxX);
        float randomZ = Math.RandomFloat(m_fSpawnRangeMinY, m_fSpawnRangeMaxY);
        
        return Vector(randomX, m_fSpawnHeight, randomZ);
    }
    
       //------------------------------------------------------------------------------------------------
    protected void AlertAllPlayers(vector dropPosition)
    {
        string alertMessage = string.Format("Supply drop incoming ");
        
        Print(string.Format("AirDropGameSystem: %1", alertMessage), LogLevel.NORMAL);
        
        // Simple approach - print message to all players via global print
        // This will appear in server logs and can be seen by admins
        PrintFormat("AIRDROP ALERT: %1", alertMessage);
        
		SCR_ChatComponent chatComponent = GetChatComponent();

		if (!chatComponent)
			return;
		chatComponent.SendMessage(alertMessage,0);
		
        // Alternative: Use RPC to send message to all clients
        // You can extend this later with proper UI notifications
        Rpc(RPC_AlertPlayers, alertMessage);
    }
    
    //------------------------------------------------------------------------------------------------
    [RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
    protected void RPC_AlertPlayers(string message)
    {
        // This will run on all clients
        PrintFormat("=== AIRDROP ALERT ===");
        PrintFormat("%1", message);
        PrintFormat("=====================");
        
		SCR_ChatComponent chatComponent = GetChatComponent();

		if (!chatComponent)
			return;
		chatComponent.SendMessage(message,0);
        // Here you could add:
        // - Custom UI popup
        // - Sound effect
        // - Screen flash
        // - HUD message
    }
	
	//------------------------------------------------------------------------------------------------
	protected SCR_ChatComponent GetChatComponent()
	{
		PlayerController pc = GetGame().GetPlayerController();

		if (!pc)
			return null;

		return SCR_ChatComponent.Cast(pc.FindComponent(SCR_ChatComponent));
	}
}