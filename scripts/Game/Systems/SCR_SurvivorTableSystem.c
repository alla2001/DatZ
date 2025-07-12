/*
System to track survivor tables for house claiming
*/

class SCR_SurvivorTableSystem : GameSystem
{
    override static void InitInfo(WorldSystemInfo outInfo)
    {
        outInfo
            .SetAbstract(false)
            .AddPoint(ESystemPoint.Frame);
    }
    
    // Core data structures
    protected ref map<int, ref SCR_SurvivorTableData> m_Tables;           // Table ID -> Table Data
    protected ref map<string, int> m_HouseClaims;                         // House ID -> Table ID
    protected ref map<string, ref array<int>> m_PlayerTables;             // Player ID -> Array of Table IDs
    protected ref map<int, string> m_TableToHouse;                        // Table ID -> House ID
    
    // Configuration
    protected int m_NextTableID = 1;
    protected float m_ClaimRadius = 50.0;
    
    // Update tracking
    protected float m_fUpdateTimer;
    protected const float UPDATE_DELAY = 1.0; // Check for invalid tables every second
    
    //------------------------------------------------------------------------------------------------
    protected override void OnInit()
    {
        // Initialize data structures
        m_Tables = new map<int, ref SCR_SurvivorTableData>();
        m_HouseClaims = new map<string, int>();
        m_PlayerTables = new map<string, ref array<int>>();
        m_TableToHouse = new map<int, string>();
        
        Enable(false); // Start disabled, enable when tables are registered
    }
    
    //------------------------------------------------------------------------------------------------
    override void OnUpdate(ESystemPoint point)
    {
        m_fUpdateTimer += GetWorld().GetTimeSlice();
        if (m_fUpdateTimer < UPDATE_DELAY)
            return;
            
        // Check for invalid tables and clean up
        CleanupInvalidTables();
        
        m_fUpdateTimer = 0;
    }
    
    //------------------------------------------------------------------------------------------------
    // Get the system instance
    static SCR_SurvivorTableSystem GetInstance()
    {
       World world = GetGame().GetWorld();
        if (!world)
            return null;
            
        return SCR_SurvivorTableSystem.Cast(world.FindSystem(SCR_SurvivorTableSystem));
    }
    
    //------------------------------------------------------------------------------------------------
    // Register a new survivor table
    int RegisterTable(string playerID, vector position, string houseID = "")
    {
        if (!playerID || playerID.IsEmpty())
            return -1;
            
        int tableID = m_NextTableID++;
        
        // Create table data
        SCR_SurvivorTableData tableData = new SCR_SurvivorTableData();
        tableData.m_TableID = tableID;
        tableData.m_PlayerID = playerID;
        tableData.m_Position = position;
        tableData.m_HouseID = houseID;
        tableData.m_PlacementTime = GetGame().GetWorld().GetWorldTime();
        
        // Store in main table registry
        m_Tables.Set(tableID, tableData);
        
        // Add to player's table list
        if (!m_PlayerTables.Contains(playerID))
            m_PlayerTables.Set(playerID, new array<int>());
        m_PlayerTables.Get(playerID).Insert(tableID);
        
        // If house ID provided, claim the house
        if (!houseID.IsEmpty())
        {
            ClaimHouse(tableID, houseID);
        }
        
        // Enable system if not already enabled
        if (!IsEnabled())
            Enable(true);
        
        Print(string.Format("Registered table %1 for player %2", tableID, playerID));
        return tableID;
    }
    
    //------------------------------------------------------------------------------------------------
    // Claim a house with a table
    bool ClaimHouse(int tableID, string houseID)
    {
        if (!m_Tables.Contains(tableID) || houseID.IsEmpty())
            return false;
            
        // Check if house is already claimed
        if (m_HouseClaims.Contains(houseID))
        {
            Print(string.Format("House %1 already claimed by table %2", houseID, m_HouseClaims.Get(houseID)));
            return false;
        }
        
        // Update table data
        SCR_SurvivorTableData tableData = m_Tables.Get(tableID);
        if (!tableData)
            return false;
            
        tableData.m_HouseID = houseID;
        
        // Register claim
        m_HouseClaims.Set(houseID, tableID);
        m_TableToHouse.Set(tableID, houseID);
        
        Print(string.Format("House %1 claimed by table %2 (Player: %3)", houseID, tableID, tableData.m_PlayerID));
        return true;
    }
    
    //------------------------------------------------------------------------------------------------
    // Remove a table and its claims
    bool RemoveTable(int tableID)
    {
        if (!m_Tables.Contains(tableID))
            return false;
            
        SCR_SurvivorTableData tableData = m_Tables.Get(tableID);
        if (!tableData)
            return false;
            
        string playerID = tableData.m_PlayerID;
        string houseID = tableData.m_HouseID;
        
        // Remove from player's table list
        if (m_PlayerTables.Contains(playerID))
        {
            array<int> playerTables = m_PlayerTables.Get(playerID);
            int index = playerTables.Find(tableID);
            if (index != -1)
                playerTables.Remove(index);
                
            // Remove player entry if no tables left
            if (playerTables.Count() == 0)
                m_PlayerTables.Remove(playerID);
        }
        
        // Remove house claim
        if (!houseID.IsEmpty())
        {
            m_HouseClaims.Remove(houseID);
            m_TableToHouse.Remove(tableID);
        }
        
        // Remove from main registry
        m_Tables.Remove(tableID);
        
        // Disable system if no tables left
        if (m_Tables.IsEmpty())
            Enable(false);
        
        Print(string.Format("Removed table %1 (Player: %2, House: %3)", tableID, playerID, houseID));
        return true;
    }
    
    //------------------------------------------------------------------------------------------------
    // Clean up invalid tables (called during update)
    protected void CleanupInvalidTables()
    {
        bool nullValuePresent = false;
        array<int> tablesToRemove = new array<int>();
        
        foreach (int tableID, SCR_SurvivorTableData data : m_Tables)
        {
            if (!data)
            {
                nullValuePresent = true;
                tablesToRemove.Insert(tableID);
                continue;
            }
            
            // Add additional validation checks here if needed
            // For example, check if table entity still exists in world
        }
        
        // Remove invalid tables
        if (nullValuePresent)
        {
            foreach (int tableID : tablesToRemove)
            {
                RemoveTable(tableID);
            }
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // UTILITY FUNCTIONS
    //------------------------------------------------------------------------------------------------
    
    // Get table data by ID
    SCR_SurvivorTableData GetTableData(int tableID)
    {
        if (m_Tables.Contains(tableID))
            return m_Tables.Get(tableID);
        return null;
    }
    
    // Get table ID that claimed a house
    int GetHouseClaimant(string houseID)
    {
        if (m_HouseClaims.Contains(houseID))
            return m_HouseClaims.Get(houseID);
        return -1;
    }
    
    // Get house ID claimed by a table
    string GetTableHouse(int tableID)
    {
        if (m_TableToHouse.Contains(tableID))
            return m_TableToHouse.Get(tableID);
        return "";
    }
    
    // Check if a house is claimed
    bool IsHouseClaimed(string houseID)
    {
        return m_HouseClaims.Contains(houseID);
    }
    
    // Get all tables owned by a player
    array<int> GetPlayerTables(string playerID)
    {
        if (m_PlayerTables.Contains(playerID))
            return m_PlayerTables.Get(playerID);
        return new array<int>();
    }
    
    // Get player who owns a table
    string GetTableOwner(int tableID)
    {
        SCR_SurvivorTableData data = GetTableData(tableID);
        if (data)
            return data.m_PlayerID;
        return "";
    }
    
    // Check if player owns a specific table
    bool DoesPlayerOwnTable(string playerID, int tableID)
    {
        SCR_SurvivorTableData data = GetTableData(tableID);
        if (data)
            return data.m_PlayerID == playerID;
        return false;
    }
    
    // Get all claimed houses
    array<string> GetAllClaimedHouses()
    {
        array<string> houses = new array<string>();
        foreach (string houseID, int tableID : m_HouseClaims)
        {
            houses.Insert(houseID);
        }
        return houses;
    }
    
    // Get count of tables owned by player
    int GetPlayerTableCount(string playerID)
    {
        if (m_PlayerTables.Contains(playerID))
            return m_PlayerTables.Get(playerID).Count();
        return 0;
    }
    
    // Get total number of tables
    int GetTotalTableCount()
    {
        return m_Tables.Count();
    }
    
    // Get total number of claimed houses
    int GetTotalClaimedHouseCount()
    {
        return m_HouseClaims.Count();
    }
    
    // Find nearest table to position
    int FindNearestTable(vector position, float maxDistance = -1)
    {
        int nearestTableID = -1;
        float nearestDistance = float.MAX;
        
        foreach (int tableID, SCR_SurvivorTableData data : m_Tables)
        {
            if (!data)
                continue;
                
            float distance = vector.Distance(position, data.m_Position);
            if (distance < nearestDistance)
            {
                if (maxDistance < 0 || distance <= maxDistance)
                {
                    nearestDistance = distance;
                    nearestTableID = tableID;
                }
            }
        }
        
        return nearestTableID;
    }
    
    // Check if position is within claim radius
    bool IsPositionWithinClaimRadius(vector position, string houseID, float radius = -1)
    {
        if (radius < 0)
            radius = m_ClaimRadius;
            
        // This would need to be implemented based on your house detection system
        // For now, returning true as placeholder
        return true;
    }
    
    // Debug function to print all table info
    void PrintAllTables()
    {
        Print("=== SURVIVOR TABLE SYSTEM DEBUG ===");
        Print(string.Format("Total Tables: %1", m_Tables.Count()));
        Print(string.Format("Total Claimed Houses: %1", m_HouseClaims.Count()));
        Print(string.Format("System Enabled: %1", IsEnabled()));
        
        foreach (int tableID, SCR_SurvivorTableData data : m_Tables)
        {
            if (!data)
                continue;
                
            Print(string.Format("Table %1: Player=%2, House=%3, Pos=%4", 
                tableID, data.m_PlayerID, data.m_HouseID, data.m_Position.ToString()));
        }
        Print("=== END DEBUG ===");
    }
}

//------------------------------------------------------------------------------------------------
// Data structure for storing table information
class SCR_SurvivorTableData
{
    int m_TableID;
    string m_PlayerID;
    string m_HouseID;
    vector m_Position;
    float m_PlacementTime;
    
    void SCR_SurvivorTableData()
    {
        m_TableID = -1;
        m_PlayerID = "";
        m_HouseID = "";
        m_Position = vector.Zero;
        m_PlacementTime = 0;
    }
}

//------------------------------------------------------------------------------------------------
// Example usage showing how to interact with the system
/*class SCR_SurvivorTableExample : GenericEntity
{
    void SCR_SurvivorTableExample(IEntitySource src, IEntity parent)
    {
        GetGame().GetCallqueue().CallLater(ExampleUsage, 1000); // Wait 1 second for system to initialize
    }
    
    void ExampleUsage()
    {
        SCR_SurvivorTableSystem tableSystem = SCR_SurvivorTableSystem.GetInstance();
        if (!tableSystem)
        {
            Print("Survivor table system not found!");
            return;
        }
            
        // Register a new table
        string playerID = "Player123";
        vector tablePosition = Vector(100, 0, 200);
        string houseID = "House_001";
        
        int tableID = tableSystem.RegisterTable(playerID, tablePosition, houseID);
        
        // Check if house is claimed
        if (tableSystem.IsHouseClaimed(houseID))
        {
            Print("House is claimed!");
        }
        
        // Get player's tables
        array<int> playerTables = tableSystem.GetPlayerTables(playerID);
        Print(string.Format("Player %1 has %2 tables", playerID, playerTables.Count()));
        
        // Find nearest table
        int nearestTable = tableSystem.FindNearestTable(Vector(105, 0, 205), 50.0);
        if (nearestTable != -1)
        {
            Print(string.Format("Nearest table is %1", nearestTable));
        }
        
        // Print debug info
        tableSystem.PrintAllTables();
    }
}*/