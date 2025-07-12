// PlaceablePrefabGadget.c - Main gadget class
[ComponentEditorProps(category: "GameScripted/Gadgets", description: "Gadget for placing prefabs with preview")]
class PlaceablePrefabGadgetClass: SCR_ConsumableItemComponentClass
{
};

class PlaceablePrefabGadget : SCR_ConsumableItemComponent
{
   [Attribute(defvalue: "", uiwidget: UIWidgets.ResourceNamePicker, desc: "Prefab to place", params: "et")]
    protected ResourceName m_sPrefabToPlace;
    
    [Attribute(defvalue: "5.0", desc: "Maximum placement distance")]
    protected float m_fMaxPlacementDistance;
    
    [Attribute(defvalue: "1.0", desc: "Minimum clearance radius")]
    protected float m_fMinClearanceRadius;
    
    [Attribute(defvalue: "0.5", desc: "Preview update interval (seconds)")]
    protected float m_fPreviewUpdateInterval;
    
    [Attribute(defvalue: "true", desc: "Check if placement is inside building")]
    protected bool m_bCheckIndoorPlacement;
    
    protected IEntity m_PreviewEntity;
    protected bool m_bIsPlacing;
    protected bool m_bCanPlace;
    protected float m_fLastPreviewUpdate;
    protected vector m_vPlacementPosition;
    protected vector m_vPlacementAngles;
    protected ref array<IEntity> nearbyEntities = {};
    protected vector m_vPreviewBoundsMin;
    protected vector m_vPreviewBoundsMax;
    
    // Trace directions for indoor checking
    protected static const vector TRACE_DIRECTIONS[3] = {
        "0 1 0",    // Up
        "1 0 0",    // Right
        "0 0 1"     // Forward
    };
    
    //------------------------------------------------------------------------------------------------
    void OnActivate()
    {
        
        StartPlacement();
    }
    
    //------------------------------------------------------------------------------------------------
    void OnDeactivate()
    {
       
        StopPlacement();
    }
    override void OnModeChanged(EGadgetMode mode, IEntity charOwner)
	{
		// Clear last mode
		ModeClear(m_iMode);
		
		// Update current mode
		m_iMode = mode;
		
		// Set new mode
		ModeSwitch(mode, charOwner);
		if(mode==EGadgetMode.IN_HAND)
			StartPlacement();
		else
		 StopPlacement();
	}
    //------------------------------------------------------------------------------------------------
    protected void StartPlacement()
    {
        if (m_sPrefabToPlace.IsEmpty())
        {
            Print("PlaceablePrefabGadget: No prefab specified!", LogLevel.ERROR);
            return;
        }
        
        m_bIsPlacing = true;
        CreatePreviewEntity();
        
        // Start preview update loop
        GetGame().GetCallqueue().CallLater(UpdatePreview, 0, true);
    }
	

	//------------------------------------------------------------------------------------------------
	override void ModeSwitch(EGadgetMode mode, IEntity charOwner)
	{
		super.ModeSwitch(mode, charOwner);

		if (mode == EGadgetMode.IN_HAND)
		{
			m_CharController = SCR_CharacterControllerComponent.Cast(charOwner.FindComponent(SCR_CharacterControllerComponent));
			m_CharController.m_OnItemUseFinishedInvoker.Insert(OnApplyToCharacter);
		}
	}

    
    //------------------------------------------------------------------------------------------------
    protected void StopPlacement()
    {
        m_bIsPlacing = false;
        DestroyPreviewEntity();
        
        // Stop preview update loop
        GetGame().GetCallqueue().Remove(UpdatePreview);
    }
    
    //------------------------------------------------------------------------------------------------
    protected void CreatePreviewEntity()
    {
        if (m_PreviewEntity)
            return;
            
        Resource prefabResource = Resource.Load(m_sPrefabToPlace);
        if (!prefabResource)
        {
            Print("PlaceablePrefabGadget: Failed to load prefab resource!", LogLevel.ERROR);
            return;
        }
        
        EntitySpawnParams spawnParams = EntitySpawnParams();
        spawnParams.TransformMode = ETransformMode.WORLD;
        
        m_PreviewEntity = GetGame().SpawnEntityPrefab(prefabResource, GetGame().GetWorld(), spawnParams);
        m_PreviewEntity.GetPhysics().ChangeSimulationState(SimulationState.NONE);
        if (m_PreviewEntity)
        {
            // Make preview semi-transparent and disable physics
            SetupPreviewEntity(m_PreviewEntity);
            
            // Calculate and store preview entity bounds
            CalculatePreviewBounds();
        }
    }
    
    //------------------------------------------------------------------------------------------------
    protected void CalculatePreviewBounds()
    {
        if (!m_PreviewEntity)
            return;
            
        // Get the preview entity's local bounds
        m_PreviewEntity.GetBounds(m_vPreviewBoundsMin, m_vPreviewBoundsMax);
        
        // Expand bounds slightly for safety margin
        float boundsMargin = 0.1; // 10cm safety margin
        m_vPreviewBoundsMin = m_vPreviewBoundsMin - Vector(boundsMargin, boundsMargin, boundsMargin);
        m_vPreviewBoundsMax = m_vPreviewBoundsMax + Vector(boundsMargin, boundsMargin, boundsMargin);
    }
    
    //------------------------------------------------------------------------------------------------
    protected void SetupPreviewEntity(IEntity entity)
    {
        // Disable physics
        Physics physics = entity.GetPhysics();
        if (physics)
            physics.SetActive(ActiveState.INACTIVE);
        
        // Make semi-transparent (this part depends on your specific rendering setup)
        // You might need to adjust materials or use different methods based on your prefab
        
        // Disable collision detection for preview
        array<IEntity> children = {};
        /*SCR_Global.GetHierarchyEntityList(entity, children);
        
        foreach (IEntity child : children)
        {
            Physics childPhysics = child.GetPhysics();
            if (childPhysics)
                childPhysics.SetActive(ActiveState.INACTIVE);
        }*/
    }
    
    //------------------------------------------------------------------------------------------------
    protected void UpdatePreview()
    {
        if (!m_bIsPlacing || !m_PreviewEntity)
            return;
            
        float currentTime = GetGame().GetWorld().GetWorldTime();
        if (currentTime - m_fLastPreviewUpdate < m_fPreviewUpdateInterval)
            return;
            
        m_fLastPreviewUpdate = currentTime;
        
        // Get player and camera
        PlayerController playerController = GetGame().GetPlayerController();
        if (!playerController)
            return;
            
        IEntity playerEntity = playerController.GetControlledEntity();
        if (!playerEntity)
            return;
            
        CameraManager cameraManager = GetGame().GetCameraManager();
        if (!cameraManager)
            return;
            
        CameraBase camera = cameraManager.CurrentCamera();
        if (!camera)
            return;
        
        // Perform raycast from camera
        vector cameraTransform[4];
        camera.GetWorldTransform(cameraTransform);
		
        vector dir = cameraTransform[2];
        vector start = cameraTransform[3]+ (dir * 1);
     
        vector end = start + (dir * m_fMaxPlacementDistance);
        
        TraceParam trace = new TraceParam();
        trace.Start = start;
        trace.End = end;
        trace.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
        trace.LayerMask = EPhysicsLayerPresets.Projectile;
        
        float hitDistance = GetGame().GetWorld().TraceMove(trace, null);
        
        if (hitDistance < 0)
        {
            // No hit, place at max distance
            m_vPlacementPosition = end;
        }
        else
        {
            // Hit something, place at hit position
            m_vPlacementPosition = vector.Lerp( start,end,hitDistance);
        }
        
        // Calculate placement angles (align with surface normal if available)
        m_vPlacementAngles = Vector(0, 0, 0); // Default orientation
        
        // Check if we can place at this position
        m_bCanPlace = CheckPlacementValidity(m_vPlacementPosition);
        
        // Update preview entity position and visual feedback
        UpdatePreviewEntityTransform();
        UpdatePreviewVisuals();
    }
   //------------------------------------------------------------------------------------------------
    protected bool CheckPlacementValidity(vector position)
    {
        // First check basic clearance radius
        if (!CheckClearanceRadius(position))
            return false;
            
        // Then check detailed bounds overlap
        if (!CheckBoundsOverlap(position))
            return false;
        
        // Check if inside building (if enabled)
        if (m_bCheckIndoorPlacement && !IsInside(position))
            return false;
        
        return true;
    }
    
    //------------------------------------------------------------------------------------------------
    protected bool CheckClearanceRadius(vector position)
    {
        nearbyEntities.Clear();
        
        // Create AABB around placement position
        vector mins = Vector(position[0] - m_fMinClearanceRadius, 
                           position[1] - m_fMinClearanceRadius, 
                           position[2] - m_fMinClearanceRadius);
        vector maxs = Vector(position[0] + m_fMinClearanceRadius, 
                           position[1] + m_fMinClearanceRadius, 
                           position[2] + m_fMinClearanceRadius);
        
        // Check for overlapping entities using AABB
        GetGame().GetWorld().QueryEntitiesByAABB(mins, maxs, null, QueryEntitiesCallback, EQueryEntitiesFlags.ALL);
        
        foreach (IEntity entity : nearbyEntities)
        {
            // Skip the preview entity itself
            if (entity == m_PreviewEntity)
                continue;
                
            // Skip non-physical entities
            Physics physics = entity.GetPhysics();
            if (!physics)
                continue;
                
            // Get entity's AABB for more precise collision checking
            vector entityMins, entityMaxs;
            entity.GetBounds(entityMins, entityMaxs);
            
            // Transform entity bounds to world space
            vector entityTransform[4];
            entity.GetWorldTransform(entityTransform);
            vector entityWorldMins = entityTransform[3] + entityMins;
            vector entityWorldMaxs = entityTransform[3] + entityMaxs;
            
            // Check AABB intersection
            if (AABBIntersect(mins, maxs, entityWorldMins, entityWorldMaxs))
                return false;
        }
        
        return true;
    }
    
    //------------------------------------------------------------------------------------------------
    protected bool CheckBoundsOverlap(vector position)
    {
        if (!m_PreviewEntity)
            return false;
            
        // Calculate world space bounds for the preview entity at the placement position
        vector worldBoundsMin = position + m_vPreviewBoundsMin;
        vector worldBoundsMax = position + m_vPreviewBoundsMax;
        
        // Clear and repopulate nearby entities for bounds checking
        nearbyEntities.Clear();
        GetGame().GetWorld().QueryEntitiesByAABB(worldBoundsMin, worldBoundsMax, null, QueryEntitiesCallback, EQueryEntitiesFlags.ALL);
        
        foreach (IEntity entity : nearbyEntities)
        {
            // Skip the preview entity itself
            if (entity == m_PreviewEntity)
                continue;
                
            // Skip non-physical entities
            Physics physics = entity.GetPhysics();
            if (!physics)
                continue;
                
            // Get entity's bounds and transform to world space
            vector entityMins, entityMaxs;
            entity.GetBounds(entityMins, entityMaxs);
            
            vector entityTransform[4];
            entity.GetWorldTransform(entityTransform);
            
            // Transform bounds to world space properly
            vector entityWorldMins = entityTransform[3] + entityMins;
            vector entityWorldMaxs = entityTransform[3] + entityMaxs;
            
            // Check for precise AABB intersection
            if (AABBIntersect(worldBoundsMin, worldBoundsMax, entityWorldMins, entityWorldMaxs))
            {
                Print("PlaceablePrefabGadget: Bounds overlap detected with entity: " + entity.GetName());
                return false;
            }
        }
        
        return true;
    }
    
    //------------------------------------------------------------------------------------------------
    protected bool QueryEntitiesCallback(IEntity e)
    {
        nearbyEntities.Insert(e);
		return true;
    }
    
    //------------------------------------------------------------------------------------------------
    protected bool PerformTrace(notnull TraceParam param, vector start, vector direction, notnull BaseWorld world, float lengthMultiplier = 1)
    {
        param.Start = start - direction * lengthMultiplier;
        param.End = start + direction * lengthMultiplier;
        world.TraceMove(param, TraceFilter);
        
        return param.TraceEnt != null;
    }
    
    //------------------------------------------------------------------------------------------------
    protected bool PerformTraceUp(notnull TraceParam param, vector start, vector direction, notnull BaseWorld world, float lengthMultiplier = 1)
    {
        param.Start = start + direction;
        param.End = start + direction * lengthMultiplier;
        world.TraceMove(param, TraceFilter);
        
        return param.TraceEnt != null;
    }
    
    //------------------------------------------------------------------------------------------------
    //! Checks whether or not an entity is inside of the building, and if there is enough free space.
    protected bool IsInside(vector start)
    {
        IEntity owner = GetOwner();
        BaseWorld world = owner.GetWorld();
        
        TraceParam param = new TraceParam();
        param.Flags = TraceFlags.ENTS;
        param.LayerMask = EPhysicsLayerDefs.Projectile;
        
        bool traceResult;
        for (int i = 0; i < 3; i++)
        {
            float lengthMultiplier = 1; // Default length
            if (i == 0)
            {
                lengthMultiplier = 30; // Vertical needs to be long
                traceResult = PerformTraceUp(param, start, TRACE_DIRECTIONS[i], world, lengthMultiplier);
            }
            else
            {
                traceResult = PerformTrace(param, start, TRACE_DIRECTIONS[i], world, lengthMultiplier);
            }
            
            if (!traceResult && i == 0 && Math.RandomFloat(0, 10) < 9)
                return false; // Must have a roof
                
            if (!traceResult && i == 1 && Math.RandomFloat(0, 10) < 6)
                return false; // Must have walls
                
            if (!traceResult && i == 2 && Math.RandomFloat(0, 10) < 5)
                return false; // Must have walls
        }
        
        // Check if there's enough free space around the placement position
        if (!HasEnoughFreeSpace(start))
            return false;
            
        return true;
    }
    
    //------------------------------------------------------------------------------------------------
    protected bool HasEnoughFreeSpace(vector position)
    {
        // Create a smaller AABB for free space checking
        float freeSpaceRadius = m_fMinClearanceRadius * 0.8; // Slightly smaller than clearance
        vector mins = Vector(position[0] - freeSpaceRadius, 
                           position[1] - freeSpaceRadius, 
                           position[2] - freeSpaceRadius);
        vector maxs = Vector(position[0] + freeSpaceRadius, 
                           position[1] + freeSpaceRadius, 
                           position[2] + freeSpaceRadius);
        
       nearbyEntities.Clone();
        GetGame().GetWorld().QueryEntitiesByAABB(mins, maxs, null, QueryEntitiesCallback, EQueryEntitiesFlags.ALL);
        
        // Count solid obstacles
        int obstacleCount = 0;
        foreach (IEntity entity : nearbyEntities)
        {
            if (entity == m_PreviewEntity)
                continue;
                
            Physics physics = entity.GetPhysics();
            if (!physics)
                continue;
                
            obstacleCount++;
        }
        
        // Allow some obstacles but not too many (adjust threshold as needed)
        return obstacleCount < 3;
    }
    
    //------------------------------------------------------------------------------------------------
    protected bool TraceFilter(notnull IEntity e, vector start = "0 0 0", vector dir = "0 0 0")
    {
        // Skip the preview entity and player
        if (e == m_PreviewEntity)
            return false;
            
        PlayerController playerController = GetGame().GetPlayerController();
        if (playerController && e == playerController.GetControlledEntity())
            return false;
            
        return true;
    }
    
    //------------------------------------------------------------------------------------------------
    protected bool AABBIntersect(vector mins1, vector maxs1, vector mins2, vector maxs2)
    {
        return (mins1[0] <= maxs2[0] && maxs1[0] >= mins2[0]) &&
               (mins1[1] <= maxs2[1] && maxs1[1] >= mins2[1]) &&
               (mins1[2] <= maxs2[2] && maxs1[2] >= mins2[2]);
    }
    
    
    //------------------------------------------------------------------------------------------------
    protected void UpdatePreviewEntityTransform()
    {
        if (!m_PreviewEntity)
            return;
            
        vector transform[4];
        Math3D.AnglesToMatrix(m_vPlacementAngles, transform);
        transform[3] = m_vPlacementPosition;
        
        m_PreviewEntity.SetWorldTransform(transform);
		m_PreviewEntity.Update();
    }
    
    //------------------------------------------------------------------------------------------------
    protected void UpdatePreviewVisuals()
    {
        if (!m_PreviewEntity)
            return;
            
        // Change preview color based on placement validity
        // This is a simplified example - you might need to implement
        // material changes or shader parameters based on your setup
        
        // For now, we'll just print the status
        if (m_bCanPlace)
        {
            // Green tint for valid placement
            // SetPreviewColor(Color.GREEN);
        }
        else
        {
            // Red tint for invalid placement
            // SetPreviewColor(Color.RED);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    bool CanPerformAction()
    {
        return m_bIsPlacing && m_bCanPlace;
    }
    
    //------------------------------------------------------------------------------------------------
    void OnAction()
    {
        if (!CanPerformAction())
            return;
            
        PlacePrefab();
        StopPlacement();
    }
    
    //------------------------------------------------------------------------------------------------
    protected void PlacePrefab()
    {
        Resource prefabResource = Resource.Load(m_sPrefabToPlace);
        if (!prefabResource)
            return;
            
        EntitySpawnParams spawnParams = EntitySpawnParams();
        spawnParams.TransformMode = ETransformMode.WORLD;
        spawnParams.Transform[3] = m_vPlacementPosition;
        Math3D.AnglesToMatrix(m_vPlacementAngles, spawnParams.Transform);
        
        IEntity placedEntity = GetGame().SpawnEntityPrefab(prefabResource, GetGame().GetWorld(), spawnParams);
        
        if (placedEntity)
        {
            // Placement successful
            Print("PlaceablePrefabGadget: Prefab placed successfully at " + m_vPlacementPosition.ToString());
            
            // Optional: Add placed entity to some management system
            OnPrefabPlaced(placedEntity);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    protected void OnPrefabPlaced(IEntity placedEntity)
    {
        // Override this method in derived classes for custom behavior
        // e.g., networking, persistence, etc.
    }
    
    //------------------------------------------------------------------------------------------------
    protected void DestroyPreviewEntity()
    {
        if (m_PreviewEntity)
        {
            SCR_EntityHelper.DeleteEntityAndChildren(m_PreviewEntity);
            m_PreviewEntity = null;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    override void EOnInit(IEntity owner)
    {
        super.EOnInit(owner);
        m_bIsPlacing = false;
        m_bCanPlace = false;
        m_fLastPreviewUpdate = 0;
    }
};