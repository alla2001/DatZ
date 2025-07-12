class SCR_PlankGadgetComponentClass :SCR_ConsumableItemComponentClass
{}



//------------------------------------------------------------------------------------------------
// Window Plank Gadget System for Arma Reforger
//------------------------------------------------------------------------------------------------

[ComponentEditorProps(category: "GameScripted/Gadgets", description: "Plank Placement Gadget")]
class SCR_PlankGadgetComponent : SCR_ConsumableItemComponent
{
    [Attribute("1.0", UIWidgets.EditBox, "Maximum plank length in meters")]
    protected float m_fMaxPlankLength;
    
    [Attribute("", UIWidgets.ResourceNamePicker, "Plank prefab", "et")]
    protected ResourceName m_sPlankPrefab;
    
    [Attribute("", UIWidgets.ResourceNamePicker, "Preview plank material", "emat")]
    protected ResourceName m_sPreviewMaterial;
    
    protected bool m_bPlacementMode = false;
    protected bool m_bGadgetActive = false;
    protected vector m_vFirstPoint;
    protected IEntity m_pTargetWindow;
	
    protected IEntity m_pPreviewPlank;
    protected IEntity m_pUser;
    

    
    //------------------------------------------------------------------------------------------------
    void OnActivate(IEntity user)
    {
      
        
        m_pUser = user;
        m_bGadgetActive = true;
        

       	SetEventMask(GetOwner(), EntityEvent.FRAME);
        
        // Show gadget activation hint
        SCR_HintManagerComponent hintManager = SCR_HintManagerComponent.GetInstance();
        if (hintManager)
        {
            hintManager.ShowCustomHint("Plank gadget activated. Look at window and use item to start placement", "Plank Gadget", 3.0);
        }
        
        Print("Plank gadget activated");
    }
    
    //------------------------------------------------------------------------------------------------
 	void OnDeactivate(IEntity user)
    {
       
        ClearEventMask(GetOwner(), EntityEvent.FRAME);
        if (m_bPlacementMode)
        {
            CancelPlacement();
        }
        
        m_bGadgetActive = false;
        m_pUser = null;
        
        Print("Plank gadget deactivated");
    }
	
	
	
	
	
	
		
	
	//------------------------------------------------------------------------------------------------
	//! OnItemUseBegan event from SCR_CharacterControllerComponent
	override void OnUseBegan(IEntity item, ItemUseParameters animParams)
	{
		OnUse();
		if (m_bAlternativeModelOnAction)
			SetAlternativeModel(true);

		if (!m_CharacterOwner || !animParams)
			return;
		
		SetHealedGroup(animParams.GetIntParam(), true);
	}

	void OnUse()
	{
	
	
		if(!m_bPlacementMode)
		{
			IEntity window = GetWindowUnderCrosshair();
			if (window)
			{
				StartPlankPlacement(window);
			}
		}
		else
		{
			vector secondPoint = GetCurrentWindowHitPosition();
			if (secondPoint != vector.Zero && IsPointOnWindow(secondPoint, m_pTargetWindow))
			{
				float distance = vector.Distance(m_vFirstPoint, secondPoint);
				if (distance <= m_fMaxPlankLength && distance > 0.1)
				{
					PlacePlank(secondPoint);
				}
			}
		}
		
	
	
	}
	
	
   	//------------------------------------------------------------------------------------------------
	//! Set gadget mode 
	//! \param[in] mode is the target mode being switched to
	//! \param[in] charOwner
	override protected void ModeSwitch(EGadgetMode mode, IEntity charOwner)
	{
		super.ModeSwitch(mode, charOwner);

		UpdateVisibility(mode);
		
		// if removing from inventory
		if (mode == EGadgetMode.ON_GROUND)
			m_CharacterOwner = null;
		else
			m_CharacterOwner = ChimeraCharacter.Cast(charOwner);
		
		if(mode == EGadgetMode.IN_HAND)
		OnActivate(m_CharacterOwner);
		else
		OnDeactivate(m_CharacterOwner);
	}
	
    //------------------------------------------------------------------------------------------------
    override void EOnFrame(IEntity owner, float timeSlice)
    {
        super.EOnFrame(owner, timeSlice);
        
        if (!m_bGadgetActive  || !m_pUser)
            return;
            
        if (m_bPlacementMode)
        {
            UpdatePlankPreview();
        }
        else
        {
            CheckWindowInteractionForGadget();
        }
    }
    
    //------------------------------------------------------------------------------------------------
    protected void CheckWindowInteractionForGadget()
    {
        IEntity window = GetWindowUnderCrosshair();
        
        if (window)
        {
            // Show interaction hint
            SCR_HintManagerComponent hintManager = SCR_HintManagerComponent.GetInstance();
            if (hintManager)
            {
                hintManager.ShowCustomHint("Use item to start plank placement on window", "Place Plank", 0.1);
            }
            
          
        }
        else
        {
            // Show no target hint
            SCR_HintManagerComponent hintManager = SCR_HintManagerComponent.GetInstance();
            if (hintManager)
            {
                hintManager.ShowCustomHint("Look at a window to place plank", "No Target", 0.1);
            }
        }
    }
    
    //------------------------------------------------------------------------------------------------
    protected IEntity GetWindowUnderCrosshair()
    {
        vector cameraPos, cameraDir;
        GetCameraTransform(cameraPos, cameraDir);
        
        vector endPos = cameraPos + cameraDir * 5.0; // 5 meter range
        
        TraceParam trace = new TraceParam();
        trace.Start = cameraPos;
        trace.End = endPos;
        trace.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
        trace.LayerMask = EPhysicsLayerDefs.Projectile;
         if(m_pPreviewPlank)
		trace.Exclude= m_pPreviewPlank;
        float traceDist = GetGame().GetWorld().TraceMove(trace, null);
        
        if (traceDist < 1.0)
        {
            IEntity hitEntity = trace.TraceEnt;
            if (IsWindow(hitEntity))
                return hitEntity;
        }
        
        return null;
    }
    
    //------------------------------------------------------------------------------------------------
    protected bool IsWindow(IEntity entity)
    {
        if (!entity)
            return false;
            
        string entityName = entity.GetName();
        
        // Check for common window identifiers
        return (entityName.Contains("window") || 
                entityName.Contains("glass") ||
                entityName.Contains("pane") ||
                entity.FindComponent(SCR_WindowComponent) != null);
    }
    
    //------------------------------------------------------------------------------------------------
    protected void StartPlankPlacement(IEntity window)
    {
        vector hitPos = GetWindowHitPosition(window);
        if (hitPos == vector.Zero)
            return;
            
        m_bPlacementMode = true;
        m_vFirstPoint = hitPos;
        m_pTargetWindow = window;
        
        CreatePreviewPlank();
        
        // Show placement instructions
        SCR_HintManagerComponent hintManager = SCR_HintManagerComponent.GetInstance();
        if (hintManager)
        {
            hintManager.ShowCustomHint("Move to second point and use item to place plank. Press [RELOAD] to cancel", "Plank Placement", 5.0);
        }
        
        Print("Started plank placement mode");
		
		GetGame().GetInputManager().AddActionListener("GadgetActivate", EActionTrigger.DOWN,OnUse);
    }
    
	
	
    //------------------------------------------------------------------------------------------------
    protected vector GetWindowHitPosition(IEntity window)
    {
        vector cameraPos, cameraDir;
        GetCameraTransform(cameraPos, cameraDir);
        
        vector endPos = cameraPos + cameraDir * 5.0;
        
        TraceParam trace = new TraceParam();
        trace.Start = cameraPos;
        trace.End = endPos;
        trace.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
        trace.LayerMask = EPhysicsLayerDefs.Projectile;
        if(m_pPreviewPlank)
		trace.Exclude=m_pPreviewPlank;
        float traceDist = GetGame().GetWorld().TraceMove(trace, null);
        
        if (traceDist < 1.0 && trace.TraceEnt == window)
        {
            return trace.Start + (trace.End - trace.Start) * (traceDist-0.001);
        }
        
        return vector.Zero;
    }
    
    //------------------------------------------------------------------------------------------------
    protected void CreatePreviewPlank()
    {
        if (m_sPlankPrefab.IsEmpty())
        {
            Print("Plank prefab not set!", LogLevel.ERROR);
            return;
        }
            
        Resource plankResource = Resource.Load(m_sPlankPrefab);
        if (!plankResource)
        {
            Print("Failed to load plank prefab!", LogLevel.ERROR);
            return;
        }
            
        EntitySpawnParams spawnParams = new EntitySpawnParams();
        spawnParams.TransformMode = ETransformMode.WORLD;
        spawnParams.Transform[3] = m_vFirstPoint;
        
        m_pPreviewPlank = GetGame().SpawnEntityPrefab(plankResource, GetGame().GetWorld(), spawnParams);
        m_pPreviewPlank.GetPhysics().ChangeSimulationState(SimulationState.NONE);
        if (m_pPreviewPlank)
        {
            // Make preview translucent/different material
            SetPreviewMode(m_pPreviewPlank, true);
            Print("Preview plank created");
        }
        else
        {
            Print("Failed to create preview plank!", LogLevel.ERROR);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    protected void UpdatePlankPreview()
    {
        if (!m_pPreviewPlank || !m_pTargetWindow)
            return;
            
        vector secondPoint = GetCurrentWindowHitPosition();
        if (secondPoint == vector.Zero)
        {
            // Hide preview if not pointing at valid surface
            m_pPreviewPlank.SetFlags(EntityFlags.VISIBLE, false);
            return;
        }
        
        // Show preview
        m_pPreviewPlank.SetFlags(EntityFlags.VISIBLE, true);
            
        // Check if second point is on the same window
        if (!IsPointOnWindow(secondPoint, m_pTargetWindow))
        {
            // Show error hint and make preview red/invalid
            SCR_HintManagerComponent hintManager = SCR_HintManagerComponent.GetInstance();
            if (hintManager)
            {
                hintManager.ShowCustomHint("Point must be on the same window!", "Invalid Placement", 0.1);
            }
            SetPreviewInvalid(m_pPreviewPlank, true);
            return;
        }
        
        float distance = vector.Distance(m_vFirstPoint, secondPoint);
        bool isValidLength = distance <= m_fMaxPlankLength && distance > 0.1; // Minimum 10cm
        
        // Check distance constraint
        if (distance > m_fMaxPlankLength)
        {
            // Clamp to max length
            vector direction = (secondPoint - m_vFirstPoint).Normalized();
            secondPoint = m_vFirstPoint + direction * m_fMaxPlankLength;
            distance = m_fMaxPlankLength;
        }
        
        // Update plank position and rotation
        UpdatePlankTransform(m_pPreviewPlank, m_vFirstPoint, secondPoint);
        
        // Set preview validity
        SetPreviewInvalid(m_pPreviewPlank, !isValidLength);
        
        // Show distance info
        SCR_HintManagerComponent hintManager = SCR_HintManagerComponent.GetInstance();
        if (hintManager)
        {
            string lengthText = string.Format("Plank length: %.2fm (Max: %.2fm)", distance, m_fMaxPlankLength);
            if (isValidLength)
                lengthText += " - Use item to place";
            else
                lengthText += " - Invalid length";
                
            hintManager.ShowCustomHint(lengthText, "Plank Preview", 0.1);
        }
        
       
    }
    
    //------------------------------------------------------------------------------------------------
    protected vector GetCurrentWindowHitPosition()
    {
        vector cameraPos, cameraDir;
        GetCameraTransform(cameraPos, cameraDir);
        
        vector endPos = cameraPos + cameraDir * 5.0;
        
        TraceParam trace = new TraceParam();
        trace.Start = cameraPos;
        trace.End = endPos;
        trace.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
        trace.LayerMask = EPhysicsLayerDefs.Projectile;
		if(m_pPreviewPlank)
		trace.Exclude=m_pPreviewPlank;
        float traceDist = GetGame().GetWorld().TraceMove(trace, null);
        
        if (traceDist < 1.0)
        {
            return trace.Start + (trace.End - trace.Start) * (traceDist-0.001);
        }
        
        return vector.Zero;
    }
    
    //------------------------------------------------------------------------------------------------
    protected bool IsPointOnWindow(vector point, IEntity window)
    {
        if (!window)
            return false;
            
        // Get window bounds
        vector windowMins, windowMaxs;
        window.GetBounds(windowMins, windowMaxs);
        
        // Transform point to window local space
        vector localPoint = window.CoordToLocal(point);
        
        // Check if point is within window bounds with some tolerance
        float tolerance = 0.15; // 15cm tolerance
        return (localPoint[0] >= windowMins[0] - tolerance && localPoint[0] <= windowMaxs[0] + tolerance &&
                localPoint[1] >= windowMins[1] - tolerance && localPoint[1] <= windowMaxs[1] + tolerance &&
                localPoint[2] >= windowMins[2] - tolerance && localPoint[2] <= windowMaxs[2] + tolerance);
    }
    
    //------------------------------------------------------------------------------------------------
    protected void UpdatePlankTransform(IEntity plank, vector point1, vector point2)
    {
        if (!plank)
            return;
            
        vector center = (point1 + point2) * 0.5;
        vector direction = (point2 - point1).Normalized();
        float distance = vector.Distance(point1, point2);
        
        // Create rotation matrix manually to align plank with direction
        vector transform[4];
        
        // Forward vector (direction of the plank)
        transform[2] = direction;
        
        // Up vector (try to keep it upward, but adjust if direction is too vertical)
        vector up = "1 0 0";
		
        if (Math.AbsFloat(vector.Dot(direction, up)) > 0.9)
        {
            up = "0 1 0"; // Use right vector if direction is too close to up
        }
        vector mat[4];
		m_pTargetWindow.GetWorldTransform(mat);
		up  = mat[2].Normalized();
        // Right vector (cross product of up and forward)
        transform[0] = CrossProduct(up, direction).Normalized();
        
        // Recalculate up vector (cross product of forward and right)
        transform[1] = CrossProduct(direction, transform[0]).Normalized();
        
        // Position (center between the two points)
        transform[3] = center;
        
		
        plank.SetWorldTransform(transform);
        
        // Scale plank to match distance
        ScalePlankToDistance(plank, distance);
    }
    //------------------------------------------------------------------------------------------------
	static vector CrossProduct(vector a, vector b) {
		return Vector(
			a[1] * b[2] - a[2] * b[1],
			a[2] * b[0] - a[0] * b[2],
			a[0] * b[1] - a[1] * b[0],
		);
	}
   protected void ScalePlankToDistance(IEntity plank, float distance)
{
    // Assuming default length is 1 meter along Z axis
    float defaultPlankLength = 1.0;
    float scaleZ = distance / defaultPlankLength;

    vector mat[4];
    plank.GetTransform(mat);

    // Extract scale, rotation, and position (assuming uniform scaling and orthogonal matrix)
    vector right = mat[0].Normalized();  // X axis
    vector up    = mat[1].Normalized();  // Y axis
    vector fwd   = mat[2].Normalized();  // Z axis (direction to scale along)

    mat[0] = right;         // Keep X scale 1
    mat[1] = up;            // Keep Y scale 1
    mat[2] = fwd * scaleZ;  // Scale forward vector by distance
    // mat[3] is the position, left untouched

    plank.SetTransform(mat);
    plank.Update();
}

    //------------------------------------------------------------------------------------------------
    protected void PlacePlank(vector secondPoint)
    {
        if (!m_pPreviewPlank)
            return;
            
        // Convert preview to actual plank
        SetPreviewMode(m_pPreviewPlank, false);
        
        // Add plank component for interactability
        /*SCR_PlankComponent plankComp = new SCR_PlankComponent();
        m_pPreviewPlank.AddComponent(plankComp);
        plankComp.SetPlaced(true);*/
        
        float plankLength = vector.Distance(m_vFirstPoint, secondPoint);
        Print(string.Format("Plank placed with length: %.2f meters", plankLength));
        m_pPreviewPlank.GetPhysics().ChangeSimulationState(SimulationState.COLLISION);
        // Reset state
        m_pPreviewPlank = null;
        m_bPlacementMode = false;
        m_pTargetWindow = null;
        
        // Show success message
        SCR_HintManagerComponent hintManager = SCR_HintManagerComponent.GetInstance();
        if (hintManager)
        {
            string successMsg = string.Format("Plank placed! Length: %.2fm", plankLength);
            hintManager.ShowCustomHint(successMsg, "Success", 2.0);
        }
        
        // Consume one plank from inventory if applicable
        ConsumeItem();
		GetGame().GetInputManager().RemoveActionListener("GadgetActivate", EActionTrigger.DOWN,OnUse);
    }
    
    //------------------------------------------------------------------------------------------------
    protected void CancelPlacement()
    {
        if (m_pPreviewPlank)
        {
            SCR_EntityHelper.DeleteEntityAndChildren(m_pPreviewPlank);
            m_pPreviewPlank = null;
        }
        
        m_bPlacementMode = false;
        m_pTargetWindow = null;
        
        Print("Plank placement cancelled");
        
        // Show cancel message
        SCR_HintManagerComponent hintManager = SCR_HintManagerComponent.GetInstance();
        if (hintManager)
        {
            hintManager.ShowCustomHint("Placement cancelled", "Cancelled", 1.0);
        }
		GetGame().GetInputManager().RemoveActionListener("GadgetActivate", EActionTrigger.DOWN,OnUse);
    }
    
    //------------------------------------------------------------------------------------------------
    protected void SetPreviewMode(IEntity entity, bool preview)
    {
        if (!entity)
            return;
            
        // Set preview rendering mode
        VObject meshObject = entity.GetVObject();
        if (meshObject)
        {
            if (preview)
            {
                // Make translucent for preview
                //meshObject.SetOpacity(0.6);
                
                // Apply preview material if available
                if (!m_sPreviewMaterial.IsEmpty())
                {
                    Resource previewMat = Resource.Load(m_sPreviewMaterial);
                    if (previewMat)
                    {
                        //meshObject.SetMaterial(previewMat);
                    }
                }
            }
            else
            {
                // Make solid for final placement
                //meshObject.SetOpacity(1.0);
                // Restore original material would go here
            }
        }
    }
    
    //------------------------------------------------------------------------------------------------
    protected void SetPreviewInvalid(IEntity entity, bool invalid)
    {
        if (!entity)
            return;
            
        /*MeshObject meshObject = entity.FindComponent(MeshObject);
        if (meshObject)
        {
            if (invalid)
            {
                // Make red/invalid appearance
                meshObject.SetOpacity(0.4);
                // Could set red material here
            }
            else
            {
                // Normal preview appearance
                meshObject.SetOpacity(0.6);
                // Could set normal preview material here
            }
        }*/
    }
    
    //------------------------------------------------------------------------------------------------
    protected void ConsumeItem()
    {
        // This would handle consuming one plank from inventory
        // Implementation depends on your inventory system
        InventoryItemComponent invItem = InventoryItemComponent.Cast(GetOwner().FindComponent(InventoryItemComponent));
        if (invItem)
        {
            // Decrease stack or remove item
            // invItem.DecreaseStackCount(1);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    protected void GetCameraTransform(out vector pos, out vector dir)
    {
       
            
        CameraManager cameraManager = GetGame().GetCameraManager();
        if (!cameraManager)
            return;
            
        CameraBase camera = cameraManager.CurrentCamera();
        if (!camera)
            return;
            
        vector transform[4];
        camera.GetWorldTransform(transform);
        
        pos = transform[3];
        dir = transform[2]; // Forward vector
    }
}