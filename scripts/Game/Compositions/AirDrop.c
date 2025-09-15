//------------------------------------------------------------------------------------------------
// COMPONENT - Makes the airdrop fall slowly when attached to the airdrop prefab
//------------------------------------------------------------------------------------------------

[BaseContainerProps()]
class AirDropFallComponentClass : ScriptComponentClass
{
}
[BaseContainerProps()]
class InitItem : Managed
{
	[Attribute("", params: "et", category: "Prefabs")]
	ResourceName item;
	[Attribute("1")]
	int amount;
}
//------------------------------------------------------------------------------------------------
class AirDropFallComponent : ScriptComponent
{
    [Attribute("2.0", UIWidgets.EditBox, "Fall speed in meters per second")]
    float m_fFallSpeed;
    
    [Attribute("true", UIWidgets.CheckBox, "Should destroy on ground impact")]
    bool m_bDestroyOnImpact;
    
    protected bool m_bIsFalling = true;
    protected float m_fGroundLevel = 0;
	
	[Attribute(desc: "Marker Type", category: "Map Marker")]
	protected ref SCR_ScenarioFrameworkMarkerType m_MapMarkerType;
	
	[Attribute("", UIWidgets.Object, "Initial items to spawn")]
    protected ref array<ref InitItem> initItems;
	ref SCR_MapMarkerBase m_MapMarker ;
    
	override void OnPostInit(IEntity owner){
	
	
		SetEventMask(owner,EntityEvent.INIT);
	}
		
    //------------------------------------------------------------------------------------------------
    override void EOnInit(IEntity owner)
    {
        super.EOnInit(owner);
        if(!Replication.IsServer())return;
        Print("AirDropFallComponent: Component initialized", LogLevel.NORMAL);
        CreateMapMarker();

		
		
        // Find ground level below spawn point
        FindGroundLevel();
        
        // Start falling
        StartFalling();
		
	
		
    }
    
	//------------------------------------------------------------------------------------------------
	//! Creates a map marker based on provided type, sets its position, custom text, and adds it to map marker manager if possible
	protected void CreateMapMarker()
	{
		SCR_MapMarkerManagerComponent mapMarkerMgr = SCR_MapMarkerManagerComponent.Cast(GetGame().GetGameMode().FindComponent(SCR_MapMarkerManagerComponent));
		if (!mapMarkerMgr)
			return;
		
		m_MapMarker = new SCR_MapMarkerBase();
		
		SCR_ScenarioFrameworkMarkerCustom mapMarkerCustom = SCR_ScenarioFrameworkMarkerCustom.Cast(m_MapMarkerType);
		if (mapMarkerCustom)
		{
			m_MapMarker.SetType(SCR_EMapMarkerType.PLACED_CUSTOM);
			m_MapMarker.SetIconEntry(mapMarkerCustom.m_eMapMarkerIcon);
			m_MapMarker.SetRotation(mapMarkerCustom.m_iMapMarkerRotation);
			m_MapMarker.SetColorEntry(mapMarkerCustom.m_eMapMarkerColor);
		}
		else
		{
			SCR_ScenarioFrameworkMarkerMilitary mapMarkerMilitary = SCR_ScenarioFrameworkMarkerMilitary.Cast(m_MapMarkerType);
			if (!mapMarkerMilitary)
				return;
			
			m_MapMarker = mapMarkerMgr.PrepareMilitaryMarker(mapMarkerMilitary.m_eMapMarkerFactionIcon, mapMarkerMilitary.m_eMapMarkerDimension, mapMarkerMilitary.m_eMapMarkerType1Modifier | mapMarkerMilitary.m_eMapMarkerType2Modifier);
		}
		
		vector worldPos = GetOwner().GetOrigin();
		m_MapMarker.SetWorldPos(worldPos[0], worldPos[2]);
		m_MapMarker.SetCustomText(m_MapMarkerType.m_sMapMarkerText);
		m_MapMarker.SetCanBeRemovedByOwner(true);
		
		FactionManager factionManager = GetGame().GetFactionManager();
	
		
		mapMarkerMgr.InsertStaticMarker(m_MapMarker, false, true);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Removes map marker by ID from map marker manager.
	void RemoveMapMarker()
	{
		if (!m_MapMarker)
			return;
		
		SCR_MapMarkerManagerComponent markerMgr = SCR_MapMarkerManagerComponent.GetInstance();
		if (!markerMgr)
			return;
		
		int markerID = m_MapMarker.GetMarkerID();
		
		SCR_MapMarkerBase marker = markerMgr.GetStaticMarkerByID(markerID);
		if (!marker)
			return;
		
		markerMgr.RemoveStaticMarker(marker);
		m_MapMarker = null;
	}
	
    //------------------------------------------------------------------------------------------------
    protected void FindGroundLevel()
    {
        vector startPos = GetOwner().GetOrigin();
        vector endPos = Vector(startPos[0], startPos[1] - 9000000, startPos[2]); // Trace 2000m down
        
        TraceParam trace = new TraceParam();
        trace.Start = startPos;
        trace.End = endPos;
        trace.Flags = TraceFlags.ENTS|TraceFlags.WORLD;
        trace.TargetLayers = EPhysicsLayerDefs.Default | EPhysicsLayerDefs.Terrain | EPhysicsLayerDefs.Static;
        float traceResult = GetGame().GetWorld().TraceMove(trace, null);
        
        if (traceResult < 1.0)
        {
            // Hit something, calculate ground level
            vector hitPos = vector.Lerp(startPos, endPos, traceResult);
            m_fGroundLevel = hitPos[1]; // Add 1m buffer above ground
        }
        else
        {
            // No ground found, use sea level
            m_fGroundLevel = 0;
        }
        if(m_fGroundLevel<0) m_fGroundLevel=0;
        Print(string.Format("AirDropFallComponent: Ground level set to %1", m_fGroundLevel), LogLevel.NORMAL);
    }
    
    //------------------------------------------------------------------------------------------------
    protected void StartFalling()
    {
        // Start the falling update loop
        GetGame().GetCallqueue().CallLater(UpdateFall, 50, true); // Update every 50ms
    }
    
    //------------------------------------------------------------------------------------------------
    protected void UpdateFall()
    {
        if (!m_bIsFalling)
            return;
            
        IEntity owner = GetOwner();
        if (!owner)
            return;
            
        vector currentPos = owner.GetOrigin();
        
        // Check if we've reached ground level
        if (currentPos[1] <= m_fGroundLevel)
        {
            // Stop falling
            m_bIsFalling = false;
            
            // Snap to ground level
            currentPos[1] = m_fGroundLevel;
            owner.SetOrigin(currentPos);
            
            OnLanded();
            return;
        }
        
        // Calculate new position (fall down)
        float deltaTime = 0.05; // 50ms in seconds
        float fallDistance = m_fFallSpeed * deltaTime;
        
        vector newPos = Vector(currentPos[0], currentPos[1] - fallDistance, currentPos[2]);
        owner.SetOrigin(newPos);
		owner.Update();
    }
    
    //------------------------------------------------------------------------------------------------
    protected void OnLanded()
    {
        Print("AirDropFallComponent: Airdrop has landed!", LogLevel.NORMAL);
        
        // Optional: Add landing effects here
        // - Dust particles
        // - Impact sound
        // - Ground crater
        
        if (m_bDestroyOnImpact)
        {
            // Destroy after 5 minutes on ground
            GetGame().GetCallqueue().CallLater(DestroyAirdrop, 1200000, false);
        }
		GetGame().GetCallqueue().CallLater(SpawnInitialItems, 1000, false, GetOwner());
    }
	
	 //------------------------------------------------------------------------------------------------
    protected void SpawnInitialItems(IEntity owner)
    {
        if (!initItems || initItems.IsEmpty())
        {
            Print("AirDropFallComponent: No initial items configured", LogLevel.NORMAL);
            return;
        }
        
        // Use the universal inventory storage component instead
        SCR_InventoryStorageManagerComponent inv = SCR_InventoryStorageManagerComponent.Cast(owner.FindComponent(SCR_InventoryStorageManagerComponent));
        
        if (!inv)
        {
            Print("AirDropFallComponent: No inventory storage component found on owner", LogLevel.WARNING);
            return;
        }
        
        // Spawn items in batches to prevent server overload
        SpawnItemsBatch(owner, inv, 0, 0);
    }
	 //------------------------------------------------------------------------------------------------
    protected void SpawnItemsBatch(IEntity owner, SCR_InventoryStorageManagerComponent inv, int itemIndex, int currentAmount)
    {
        if (itemIndex >= initItems.Count())
        {
            Print("AirDropFallComponent: Finished spawning all items", LogLevel.NORMAL);
            return;
        }
        
        InitItem currentItem = initItems[itemIndex];
        if (!currentItem || currentItem.item.IsEmpty())
        {
            // Move to next item
            GetGame().GetCallqueue().CallLater(SpawnItemsBatch, 500, false, owner, inv, itemIndex + 1, 0);
            return;
        }
        
        // Spawn one item at a time
        if (currentAmount < currentItem.amount)
        {
            Resource resource = Resource.Load(currentItem.item);
            if (resource)
            {
                EntitySpawnParams spawnParams = EntitySpawnParams();
                spawnParams.TransformMode = ETransformMode.WORLD;
                spawnParams.Transform[3] = owner.GetOrigin();
                
                IEntity spawnedItem = GetGame().SpawnEntityPrefab(resource, owner.GetWorld(), spawnParams);
                
                if (spawnedItem)
                {
                    // Try to insert into inventory
                    bool slot = inv.TryInsertItem(spawnedItem);
                    
                    if (!slot)
                    {
                        Print(string.Format("AirDropFallComponent: Failed to insert item: %1", currentItem.item), LogLevel.WARNING);
                        SCR_EntityHelper.DeleteEntityAndChildren(spawnedItem);
                    }
                    else
                    {
                        Print(string.Format("AirDropFallComponent: Added item %1/%2: %3", currentAmount + 1, currentItem.amount, currentItem.item), LogLevel.NORMAL);
                    }
                }
                else
                {
                    Print(string.Format("AirDropFallComponent: Failed to spawn item: %1", currentItem.item), LogLevel.ERROR);
                }
            }
            
            // Schedule next item spawn (small delay to prevent server overload)
            GetGame().GetCallqueue().CallLater(SpawnItemsBatch, 500, false, owner, inv, itemIndex, currentAmount + 1);
        }
        else
        {
            // Move to next item type
            GetGame().GetCallqueue().CallLater(SpawnItemsBatch, 500, false, owner, inv, itemIndex + 1, 0);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    protected void DestroyAirdrop()
    {
		RemoveMapMarker();
        IEntity owner = GetOwner();
        if (owner)
        {
            Print("AirDropFallComponent: Destroying airdrop", LogLevel.NORMAL);
            SCR_EntityHelper.DeleteEntityAndChildren(owner);
        }
    }
}