[EPF_ComponentSaveDataType(CookableItemComponent), BaseContainerProps()]
class EPF_CookableManagerComponentSaveDataClass : EPF_ComponentSaveDataClass
{
}


[EDF_DbName.Automatic()]
class EPF_CookableManagerComponentSaveData : EPF_ComponentSaveData
{


	ref EPF_PersistentCookableNode m_aCookNode;

	//------------------------------------------------------------------------------------------------
	override EPF_EReadResult ReadFrom(IEntity owner, GenericComponent component, EPF_ComponentSaveDataClass attributes)
	{
		
		CookableItemComponent cookableComp = CookableItemComponent.Cast(component);


			if (!cookableComp)
			{
				Debug.Error(string.Format("'%1' contains non persistable Metabolisem type '%2'. Ignored.", component));
				return EPF_EReadResult.DEFAULT;
			}

			EPF_PersistentCookableNode persistentMetaNode();
			persistentMetaNode.CookProgress = cookableComp.m_fCookProgress;
	


			/*if (attributes.m_bTrimDefaults)
			{
				if (persistentMetaNode.water >= 100&&persistentMetaNode.food >= 100)
					return EPF_EReadResult.DEFAULT;

			}*/
			m_aCookNode=persistentMetaNode;


		return EPF_EReadResult.OK;
	}

	//------------------------------------------------------------------------------------------------
	override EPF_EApplyResult ApplyTo(IEntity owner, GenericComponent component, EPF_ComponentSaveDataClass attributes)
	{
	
			CookableItemComponent cookableComp = CookableItemComponent.Cast(component);


			
			if (!m_aCookNode)
			{
				Debug.Error(string.Format("'%1' unable to Metabolisem '%2'. Ignored.", component));
				return EPF_EApplyResult.ERROR;
			}
		cookableComp.m_fCookProgress = m_aCookNode.CookProgress;
		
		
		return EPF_EApplyResult.OK;
	}

	//------------------------------------------------------------------------------------------------
	override bool Equals(notnull EPF_ComponentSaveData other)
	{
		EPF_CookableManagerComponentSaveData otherData = EPF_CookableManagerComponentSaveData.Cast(other);

			return m_aCookNode.CookProgress==otherData.m_aCookNode.CookProgress;

		
	}
}

class EPF_PersistentCookableNode
{
	 
    float CookProgress = 100.0;


	//------------------------------------------------------------------------------------------------
	bool Equals(notnull EPF_PersistentCookableNode other)
	{
		return float.AlmostEqual(CookProgress, other.CookProgress);
	}
}






