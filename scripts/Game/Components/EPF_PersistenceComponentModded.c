// Fix: Handle missing/invalid save-data types gracefully instead of throwing exceptions
// This prevents crashes from WCS/Attachment Framework items that lack proper EPF save-data definitions
// BULLETPROOF VERSION - protects ALL persistence operations

modded class EPF_PersistenceComponent : ScriptComponent
{
	// Track if this entity has valid persistence config
	protected bool m_bHasValidSaveData = false;
	protected bool m_bSaveDataChecked = false;

	//------------------------------------------------------------------------------------------------
	// Helper: Check if this entity has valid save-data configuration
	// Returns true if safe to persist, false if should skip
	//------------------------------------------------------------------------------------------------
	protected bool HasValidSaveData()
	{
		// Cache the result after first check
		if (m_bSaveDataChecked)
			return m_bHasValidSaveData;

		m_bSaveDataChecked = true;
		m_bHasValidSaveData = false;

		IEntity owner = GetOwner();
		if (!owner)
			return false;

		EPF_PersistenceComponentClass settings = EPF_PersistenceComponentClass.Cast(GetComponentData(owner));
		if (!settings)
			return false;

		// Check if save-data type is valid
		if (settings.m_tSaveDataType)
		{
			m_bHasValidSaveData = true;
			return true;
		}

		// Try to validate/resolve save-data type
		if (!settings.m_pSaveData)
			return false;

		if (settings.m_pSaveData.Type() == EPF_EntitySaveDataClass)
			return false;

		typename saveDataType = EPF_Utils.TrimEnd(settings.m_pSaveData.ClassName(), 5).ToType();
		if (!saveDataType)
			return false;

		m_bHasValidSaveData = true;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	// PROTECTED: Save - prevent crash during AutoSaveTick
	//------------------------------------------------------------------------------------------------
	override EPF_EntitySaveData Save(out EPF_EReadResult readResult = EPF_EReadResult.ERROR)
	{
		if (!HasValidSaveData())
			return null;

		// Wrap in additional safety check
		IEntity owner = GetOwner();
		if (!owner)
			return null;

		EPF_PersistenceComponentClass settings = EPF_PersistenceComponentClass.Cast(GetComponentData(owner));
		if (!settings || !settings.m_tSaveDataType)
			return null;

		return super.Save(readResult);
	}

	//------------------------------------------------------------------------------------------------
	// PROTECTED: Load - prevent crash when loading invalid save-data
	//------------------------------------------------------------------------------------------------
	override bool Load(notnull EPF_EntitySaveData saveData, bool isRoot = true)
	{
		if (!HasValidSaveData())
		{
			Print(string.Format("[EPF_Modded] Load skipped - invalid save-data config"), LogLevel.DEBUG);
			return false;
		}

		if (!saveData)
			return false;

		return super.Load(saveData, isRoot);
	}

	//------------------------------------------------------------------------------------------------
	// PROTECTED: Delete - prevent crash when deleting
	//------------------------------------------------------------------------------------------------
	override void Delete()
	{
		if (!HasValidSaveData())
			return;

		IEntity owner = GetOwner();
		if (!owner)
			return;

		EPF_PersistenceComponentClass settings = EPF_PersistenceComponentClass.Cast(GetComponentData(owner));
		if (!settings || !settings.m_tSaveDataType)
			return;

		super.Delete();
	}

	//------------------------------------------------------------------------------------------------
	// PROTECTED: GetAttributeClass - prevent crash when getting attribute
	//------------------------------------------------------------------------------------------------
	override Managed GetAttributeClass(typename saveDataClassType)
	{
		if (!HasValidSaveData())
			return null;

		IEntity owner = GetOwner();
		if (!owner)
			return null;

		EPF_PersistenceComponentClass settings = EPF_PersistenceComponentClass.Cast(GetComponentData(owner));
		if (!settings || !settings.m_pSaveData)
			return null;

		return super.GetAttributeClass(saveDataClassType);
	}

	//------------------------------------------------------------------------------------------------
	// PROTECTED: OnPostInit - prevent registration of invalid entities
	//------------------------------------------------------------------------------------------------
	override protected event void OnPostInit(IEntity owner)
	{
		// Persistence logic only runs on the server
		if (!EPF_PersistenceManager.IsPersistenceMaster())
			return;

		// Avoid tracking anything in preload/preview world
		if (!ChimeraWorld.CastFrom(owner.GetWorld()) || (GetGame().GetWorld() != owner.GetWorld()))
			return;

		// MODDED: Validate save-data BEFORE any registration
		if (!HasValidSaveData())
		{
			// AGGRESSIVE: Completely deactivate this component to prevent native code access
			// This removes the component from engine iteration
			Deactivate(owner);
			ClearEventMask(owner, EntityEvent.ALL);
			return;
		}

		// All checks passed, call the original implementation
		super.OnPostInit(owner);
	}

	//------------------------------------------------------------------------------------------------
	// PROTECTED: OnDelete - prevent crash during cleanup
	//------------------------------------------------------------------------------------------------
	override protected event void OnDelete(IEntity owner)
	{
		// Skip persistence cleanup for invalid entities
		if (!m_bHasValidSaveData)
			return;

		super.OnDelete(owner);
	}
}
