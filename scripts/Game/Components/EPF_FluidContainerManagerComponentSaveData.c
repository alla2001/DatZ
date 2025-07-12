[EPF_ComponentSaveDataType(FluidContainerComponent), BaseContainerProps()]
class EPF_FluidContainerManagerComponentSaveDataClass : EPF_ComponentSaveDataClass
{
}


[EDF_DbName.Automatic()]
class EPF_FluidContainerManagerComponentSaveData : EPF_ComponentSaveData
{


	ref EPF_PersistentFluidContainerNode m_aFluidNode;

	//------------------------------------------------------------------------------------------------
	override EPF_EReadResult ReadFrom(IEntity owner, GenericComponent component, EPF_ComponentSaveDataClass attributes)
	{
		
		FluidContainerComponent fluidComp = FluidContainerComponent.Cast(component);


			if (!fluidComp)
			{
				Debug.Error(string.Format("'%1' contains non persistable Metabolisem type '%2'. Ignored.", component));
				return EPF_EReadResult.DEFAULT;
			}

			EPF_PersistentFluidContainerNode persistentMetaNode();
			persistentMetaNode.fluid = fluidComp.m_fCurrentAmount;
	


			/*if (attributes.m_bTrimDefaults)
			{
				if (persistentMetaNode.water >= 100&&persistentMetaNode.food >= 100)
					return EPF_EReadResult.DEFAULT;

			}*/
			m_aFluidNode=persistentMetaNode;


		return EPF_EReadResult.OK;
	}

	//------------------------------------------------------------------------------------------------
	override EPF_EApplyResult ApplyTo(IEntity owner, GenericComponent component, EPF_ComponentSaveDataClass attributes)
	{
		array<BaseFuelNode> outNodes();
		FluidContainerComponent fluidComp = FluidContainerComponent.Cast(component);


			
			if (!m_aFluidNode)
			{
				Debug.Error(string.Format("'%1' unable to Metabolisem '%2'. Ignored.", component));
				return EPF_EApplyResult.ERROR;
			}
		fluidComp.m_fCurrentAmount = m_aFluidNode.fluid;
		
		
		return EPF_EApplyResult.OK;
	}

	//------------------------------------------------------------------------------------------------
	override bool Equals(notnull EPF_ComponentSaveData other)
	{
		EPF_FluidContainerManagerComponentSaveData otherData = EPF_FluidContainerManagerComponentSaveData.Cast(other);

			return m_aFluidNode.fluid==otherData.m_aFluidNode.fluid;

		
	}
}

class EPF_PersistentFluidContainerNode
{
	 
    float fluid = 100.0;


	//------------------------------------------------------------------------------------------------
	bool Equals(notnull EPF_PersistentFluidContainerNode other)
	{
		return float.AlmostEqual(fluid, other.fluid);
	}
}






