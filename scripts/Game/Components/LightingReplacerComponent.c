class LightingReplacerComponentClass: ScriptComponentClass
{
}

[EntityEditorProps(category: "Lighting Replacer", description: "Component to delete 'Lighting' entities and spawn custom prefabs")]
class LightingReplacerComponent : ScriptComponent
{
    [Attribute("1", UIWidgets.CheckBox, "Auto replace on spawn")]
    bool m_bAutoReplace;
    
    [Attribute("", UIWidgets.ResourceNamePicker, "Replacement prefab", "et")]
    ResourceName m_ReplacementPrefab;
    
    [Attribute("1", UIWidgets.CheckBox, "Match position exactly")]
    bool m_bMatchPosition;
    
    [Attribute("1", UIWidgets.CheckBox, "Match rotation")]
    bool m_bMatchRotation;
    
    [Attribute("1", UIWidgets.CheckBox, "Match scale")]
    bool m_bMatchScale;
    
    [Attribute("Lighting", UIWidgets.EditBox, "Entity name to delete")]
    string m_sTargetEntityName;
    
    [Attribute("0", UIWidgets.EditBox, "Spawn delay (ms)")]
    int m_iSpawnDelay;
    
    protected ref array<IEntity> m_aReplacedEntities = {};
    
    //------------------------------------------------------------------------------------------------
    override void OnPostInit(IEntity owner)
    {
        super.OnPostInit(owner);
        
        if (m_bAutoReplace)
        {
            GetGame().GetCallqueue().CallLater(ReplaceLightingEntities, 100);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    void ReplaceLightingEntities()
    {
        if (m_ReplacementPrefab.IsEmpty())
        {
            Print("LightingReplacer: No replacement prefab specified!");
            return;
        }
        
        // Method 1: Direct entity lookup by name
        IEntity targetEntity = GetGame().GetWorld().FindEntityByName(m_sTargetEntityName);
        if (targetEntity)
        {
            ReplaceEntity(targetEntity);
            return;
        }
        
        // Method 2: Search through all entities if direct lookup fails
        FindAndReplaceByName();
    }
    
    //------------------------------------------------------------------------------------------------
    void ReplaceEntity(IEntity entity)
    {
        if (!entity)
            return;
            
     
        
        // Delete the entity
        RplComponent.DeleteRplEntity(entity, false);
        
        // Spawn replacement with delay
        if (m_iSpawnDelay > 0)
        {
            GetGame().GetCallqueue().CallLater(SpawnReplacement, m_iSpawnDelay, false);
        }
        else
        {
            SpawnReplacement();
        }
    }
    
    //------------------------------------------------------------------------------------------------
    void FindAndReplaceByName()
    {
        // Get world entity manager
        BaseWorld world = GetGame().GetWorld();
        if (!world)
            return;
            
        // Method 3: Use entity query with name filter
        array<IEntity> foundEntities = {};
	
       
        
           ReplaceEntity(	world.FindEntityByName(m_sTargetEntityName));
        
        
        if (foundEntities.Count() == 0)
        {
            Print(string.Format("LightingReplacer: No entities found with name '%1'", m_sTargetEntityName));
        }
        else
        {
            Print(string.Format("LightingReplacer: Replaced %1 entities", foundEntities.Count()));
        }
    }
    
    //------------------------------------------------------------------------------------------------
    void SpawnReplacement()
    {
        EntitySpawnParams spawnParams = EntitySpawnParams();
        
        // Set transform
        spawnParams.TransformMode = ETransformMode.WORLD;
        
       
            spawnParams.Transform[3] = GetOwner().GetOrigin();
       
       
        
        // Load and spawn prefab
        Resource resource = Resource.Load(m_ReplacementPrefab);
        if (!resource)
        {
            Print(string.Format("LightingReplacer: Failed to load prefab '%1'", m_ReplacementPrefab));
            return;
        }
        
        IEntity spawnedEntity = GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), spawnParams);
        
        if (spawnedEntity)
        {
            
            
            m_aReplacedEntities.Insert(spawnedEntity);
        
        }
        else
        {
            Print("LightingReplacer: Failed to spawn replacement entity");
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Replace specific entity by direct name lookup
    void ReplaceEntityByName(string entityName)
    {
        IEntity entity = GetGame().GetWorld().FindEntityByName(entityName);
        if (entity)
        {
            ReplaceEntity(entity);
        }
        else
        {
            Print(string.Format("LightingReplacer: Entity '%1' not found", entityName));
        }
    }
    
    //------------------------------------------------------------------------------------------------
    // Manual trigger function
    void TriggerReplacement()
    {
        ReplaceLightingEntities();
    }
    
    
    //------------------------------------------------------------------------------------------------
    // Undo replacements
    void UndoReplacements()
    {
        foreach (IEntity entity : m_aReplacedEntities)
        {
            if (entity)
            {
                RplComponent.DeleteRplEntity(entity, false);
            }
        }
        
        m_aReplacedEntities.Clear();
        Print("LightingReplacer: Undid all replacements");
    }
    
    //------------------------------------------------------------------------------------------------
    // Get count of replaced entities
    int GetReplacedCount()
    {
        return m_aReplacedEntities.Count();
    }
}