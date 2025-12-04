[EPF_ComponentSaveDataType(BLD_OwnershipComponent), BaseContainerProps()]
class EPF_OwnershipComponentSaveDataClass : EPF_ComponentSaveDataClass
{
}

[EDF_DbName.Automatic()]
class EPF_OwnershipComponentSaveData : EPF_ComponentSaveData
{
    ref EPF_PersistentOwnershipNode m_aOwnershipNode;

    //------------------------------------------------------------------------------------------------
    override EPF_EReadResult ReadFrom(IEntity owner, GenericComponent component, EPF_ComponentSaveDataClass attributes)
    {
        BLD_OwnershipComponent ownershipComp = BLD_OwnershipComponent.Cast(component);

        if (!ownershipComp)
        {
            Debug.Error(string.Format("'%1' contains non persistable Ownership type '%2'. Ignored.", component));
            return EPF_EReadResult.DEFAULT;
        }

        EPF_PersistentOwnershipNode persistentOwnershipNode();
        persistentOwnershipNode.ownerID = ownershipComp.ownerID;
        persistentOwnershipNode.myCode = ownershipComp.myCode;
        persistentOwnershipNode.partHP = ownershipComp.partHP;
        persistentOwnershipNode.myDoor = ownershipComp.myDoor;
        persistentOwnershipNode.lockPicked = ownershipComp.lockPicked;

        if (attributes.m_bTrimDefaults)
        {
            // Check if all values are at defaults - adjust these conditions as needed
            if (persistentOwnershipNode.ownerID == "" && 
                persistentOwnershipNode.myCode == "" && 
                persistentOwnershipNode.partHP >= 100 && 
                persistentOwnershipNode.myDoor == "" && 
                !persistentOwnershipNode.lockPicked)
                return EPF_EReadResult.DEFAULT;
        }
        
        m_aOwnershipNode = persistentOwnershipNode;

        return EPF_EReadResult.OK;
    }

    //------------------------------------------------------------------------------------------------
    override EPF_EApplyResult ApplyTo(IEntity owner, GenericComponent component, EPF_ComponentSaveDataClass attributes)
    {
        BLD_OwnershipComponent ownershipComp = BLD_OwnershipComponent.Cast(component);

        if (!m_aOwnershipNode)
        {
            Debug.Error(string.Format("'%1' unable to restore Ownership data '%2'. Ignored.", component));
            return EPF_EApplyResult.ERROR;
        }
        
        ownershipComp.SetOwnerID(m_aOwnershipNode.ownerID);
        ownershipComp.SetCode(m_aOwnershipNode.myCode);
        ownershipComp.SetPartHP(m_aOwnershipNode.partHP);
        ownershipComp.SetDoor(m_aOwnershipNode.myDoor);
        ownershipComp.SetPicked(m_aOwnershipNode.lockPicked);

        return EPF_EApplyResult.OK;
    }

    //------------------------------------------------------------------------------------------------
    override bool Equals(notnull EPF_ComponentSaveData other)
    {
        EPF_OwnershipComponentSaveData otherData = EPF_OwnershipComponentSaveData.Cast(other);
        
        if (!otherData || !m_aOwnershipNode || !otherData.m_aOwnershipNode)
            return false;

        return m_aOwnershipNode.Equals(otherData.m_aOwnershipNode);
    }
}

class EPF_PersistentOwnershipNode
{
    string ownerID = "";
    string myCode = "";
    float partHP = 100.0;
    string myDoor = "";
    bool lockPicked = false;

    //------------------------------------------------------------------------------------------------
    bool Equals(notnull EPF_PersistentOwnershipNode other)
    {
        return ownerID == other.ownerID && 
               myCode == other.myCode && 
               float.AlmostEqual(partHP, other.partHP) && 
               myDoor == other.myDoor && 
               lockPicked == other.lockPicked;
    }
}