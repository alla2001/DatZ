[EPF_ComponentSaveDataType(FirePlaceStorageComponent), BaseContainerProps()]
class EPF_FirePlaceManagerComponentSaveDataClass : EPF_ComponentSaveDataClass
{
}

[EDF_DbName.Automatic()]
class EPF_FirePlaceManagerComponentSaveData : EPF_ComponentSaveData
{
	ref EPF_PersistentFirePlaceNode m_aFireNode;

	//------------------------------------------------------------------------------------------------
	override EPF_EReadResult ReadFrom(IEntity owner, GenericComponent component, EPF_ComponentSaveDataClass attributes)
	{
		
		FirePlaceStorageComponent fireplace = FirePlaceStorageComponent.Cast(component);


		if (!fireplace)
		{
				Debug.Error(string.Format("'%1' contains non persistable fireplace type '%2'. Ignored.", component));
				return EPF_EReadResult.DEFAULT;
		}

		EPF_PersistentFirePlaceNode persistentFireNode();
		persistentFireNode.Fuel = fireplace.m_currentFuel;



		if (attributes.m_bTrimDefaults)
		{
			if (persistentFireNode.Fuel <= 0)
				return EPF_EReadResult.DEFAULT;

		}
		m_aFireNode = persistentFireNode;


		return EPF_EReadResult.OK;
	}

	//------------------------------------------------------------------------------------------------
	override EPF_EApplyResult ApplyTo(IEntity owner, GenericComponent component, EPF_ComponentSaveDataClass attributes)
	{
		
		FirePlaceStorageComponent fireplace = FirePlaceStorageComponent.Cast(component);


			
			if (!fireplace)
			{
				Debug.Error(string.Format("'%1' unable to fireplace '%2'. Ignored.", component));
				return EPF_EApplyResult.ERROR;
			}
		fireplace.m_currentFuel = m_aFireNode.Fuel;

		return EPF_EApplyResult.OK;
	}

	//------------------------------------------------------------------------------------------------
	override bool Equals(notnull EPF_ComponentSaveData other)
	{
		EPF_FirePlaceManagerComponentSaveData otherData = EPF_FirePlaceManagerComponentSaveData.Cast(other);

			return m_aFireNode.Fuel==otherData.m_aFireNode.Fuel;

		
	}
}

class EPF_PersistentFirePlaceNode
{
	float Fuel = 0.0;



	//------------------------------------------------------------------------------------------------
	bool Equals(notnull EPF_PersistentFirePlaceNode other)
	{
		return Fuel == other.Fuel;
	}
}

