[EPF_ComponentSaveDataType(SCR_2DPIPSightsComponent), BaseContainerProps()]
class EPF_SightManagerComponentSaveDataClass : EPF_ComponentSaveDataClass
{
}


[EDF_DbName.Automatic()]
class EPF_SightManagerComponentSaveData : EPF_ComponentSaveData
{


	ref EPF_PersistentSighteNode m_aSightNode;

	//------------------------------------------------------------------------------------------------
	override EPF_EReadResult ReadFrom(IEntity owner, GenericComponent component, EPF_ComponentSaveDataClass attributes)
	{
		
		SCR_2DPIPSightsComponent sightComp = SCR_2DPIPSightsComponent.Cast(component);


			if (!sightComp)
			{
				Debug.Error(string.Format("'%1' contains non persistable Metabolisem type '%2'. Ignored.", component));
				return EPF_EReadResult.DEFAULT;
			}

			EPF_PersistentSighteNode persistentSightNode();
			persistentSightNode.Broken = sightComp.Broken;
	


			/*if (attributes.m_bTrimDefaults)
			{
				if (persistentMetaNode.water >= 100&&persistentMetaNode.food >= 100)
					return EPF_EReadResult.DEFAULT;

			}*/
			m_aSightNode=persistentSightNode;


		return EPF_EReadResult.OK;
	}

	//------------------------------------------------------------------------------------------------
	override EPF_EApplyResult ApplyTo(IEntity owner, GenericComponent component, EPF_ComponentSaveDataClass attributes)
	{
	
			SCR_2DPIPSightsComponent sightComp = SCR_2DPIPSightsComponent.Cast(component);;


			
			if (!sightComp)
			{
				Debug.Error(string.Format("'%1' unable to Metabolisem '%2'. Ignored.", component));
				return EPF_EApplyResult.ERROR;
			}
		sightComp.Broken = m_aSightNode.Broken;
		
		
		return EPF_EApplyResult.OK;
	}

	//------------------------------------------------------------------------------------------------
	override bool Equals(notnull EPF_ComponentSaveData other)
	{
		EPF_SightManagerComponentSaveData otherData = EPF_SightManagerComponentSaveData.Cast(other);

			return m_aSightNode.Broken==otherData.m_aSightNode.Broken;

		
	}
}

class EPF_PersistentSighteNode
{
	 
    bool Broken = false;


	//------------------------------------------------------------------------------------------------
	bool Equals(notnull EPF_PersistentSighteNode other)
	{
		return other.Broken==Broken;
	}
}






