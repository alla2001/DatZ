class SCR_PlankComponentClass : ScriptComponentClass
{}

[ComponentEditorProps(category: "GameScripted/Misc", description: "Plank Component")]
class SCR_PlankComponent : ScriptComponent
{
    protected bool m_bIsPlaced = false;
    protected float m_fPlankLength = 1.0;
    
    //------------------------------------------------------------------------------------------------
    void SetPlaced(bool placed)
    {
        m_bIsPlaced = placed;
    }
    
    //------------------------------------------------------------------------------------------------
    bool IsPlaced()
    {
        return m_bIsPlaced;
    }
    
    //------------------------------------------------------------------------------------------------
    void SetPlankLength(float length)
    {
        m_fPlankLength = length;
    }
    
    //------------------------------------------------------------------------------------------------
    float GetPlankLength()
    {
        return m_fPlankLength;
    }
    
    //------------------------------------------------------------------------------------------------
    // Interaction to remove plank
    bool CanInteract(IEntity user)
    {
        return m_bIsPlaced;
    }
    
    //------------------------------------------------------------------------------------------------
    void OnInteract(IEntity user)
    {
        if (m_bIsPlaced)
        {
            // Show removal confirmation
            SCR_HintManagerComponent hintManager = SCR_HintManagerComponent.GetInstance();
            if (hintManager)
            {
                hintManager.ShowCustomHint("Hold [INTERACT] to remove plank", "Remove Plank", 2.0);
            }
            
            // Remove plank after short delay
            GetGame().GetCallqueue().CallLater(RemovePlank, 1000, false);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    protected void RemovePlank()
    {
        if (m_bIsPlaced)
        {
            Print("Plank removed");
            SCR_EntityHelper.DeleteEntityAndChildren(GetOwner());
        }
    }
}
