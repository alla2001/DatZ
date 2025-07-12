[EPF_ComponentSaveDataType(DatZMetabolsimHandler), BaseContainerProps()]
class EPF_MetabolisemManagerComponentSaveDataClass : EPF_ComponentSaveDataClass
{
}

[EDF_DbName.Automatic()]
class EPF_MetabolisemManagerComponentSaveData : EPF_ComponentSaveData
{
	ref EPF_PersistentMetabolisemNode m_aMetaNode;

	//------------------------------------------------------------------------------------------------
	override EPF_EReadResult ReadFrom(IEntity owner, GenericComponent component, EPF_ComponentSaveDataClass attributes)
	{
		
		DatZMetabolsimHandler meta = DatZMetabolsimHandler.Cast(component);


			if (!meta)
			{
				Debug.Error(string.Format("'%1' contains non persistable Metabolisem type '%2'. Ignored.", component));
				return EPF_EReadResult.DEFAULT;
			}

			EPF_PersistentMetabolisemNode persistentMetaNode();
			persistentMetaNode.food = meta.food;
			persistentMetaNode.water = meta.water;


			if (attributes.m_bTrimDefaults)
			{
				if (persistentMetaNode.water >= 100&&persistentMetaNode.food >= 100)
					return EPF_EReadResult.DEFAULT;

			}
			m_aMetaNode=persistentMetaNode;


		return EPF_EReadResult.OK;
	}

	//------------------------------------------------------------------------------------------------
	override EPF_EApplyResult ApplyTo(IEntity owner, GenericComponent component, EPF_ComponentSaveDataClass attributes)
	{
		array<BaseFuelNode> outNodes();
		DatZMetabolsimHandler meta = DatZMetabolsimHandler.Cast(component);


			
			if (!m_aMetaNode)
			{
				Debug.Error(string.Format("'%1' unable to Metabolisem '%2'. Ignored.", component));
				return EPF_EApplyResult.ERROR;
			}
		meta.food = m_aMetaNode.food;
		meta.water = m_aMetaNode.water;
		
		return EPF_EApplyResult.OK;
	}


	//------------------------------------------------------------------------------------------------
	override bool Equals(notnull EPF_ComponentSaveData other)
	{
		EPF_MetabolisemManagerComponentSaveData otherData = EPF_MetabolisemManagerComponentSaveData.Cast(other);

			return m_aMetaNode.water==otherData.m_aMetaNode.water&&m_aMetaNode.food==otherData.m_aMetaNode.food;

		
	}
}

class EPF_PersistentMetabolisemNode
{
	 float food = 100.0;
    float water = 100.0;


	//------------------------------------------------------------------------------------------------
	bool Equals(notnull EPF_PersistentMetabolisemNode other)
	{
		return food == other.food && float.AlmostEqual(water, other.water);
	}
}

