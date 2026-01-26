// Runtime detector for problematic prefabs missing required components
// Logs issues so you can add them to blocklist or report to mod authors

class DZ_PrefabValidator
{
	// Track already reported prefabs to avoid log spam
	protected static ref set<string> s_ReportedPrefabs;

	//------------------------------------------------------------------------------------------------
	// Check if entity has required components and log if missing
	// Call this after spawning any entity to detect problems
	//------------------------------------------------------------------------------------------------
	static bool ValidateEntity(IEntity entity, string context = "")
	{
		if (!entity)
			return false;

		if (!s_ReportedPrefabs)
			s_ReportedPrefabs = new set<string>();

		string prefabName = GetPrefabName(entity);
		bool isValid = true;

		// Check: InventoryItemComponent requires RplComponent
		InventoryItemComponent invItem = InventoryItemComponent.Cast(entity.FindComponent(InventoryItemComponent));
		if (invItem)
		{
			RplComponent rpl = RplComponent.Cast(entity.FindComponent(RplComponent));
			if (!rpl)
			{
				isValid = false;
				ReportIssue(prefabName, "InventoryItemComponent requires RplComponent", context);
			}
		}

		// Check: EPF_PersistenceComponent requires valid save-data
		EPF_PersistenceComponent persistence = EPF_PersistenceComponent.Cast(entity.FindComponent(EPF_PersistenceComponent));
		if (persistence)
		{
			EPF_PersistenceComponentClass settings = EPF_PersistenceComponentClass.Cast(persistence.GetComponentData(entity));
			if (!settings || !settings.m_pSaveData)
			{
				isValid = false;
				ReportIssue(prefabName, "EPF_PersistenceComponent has no save-data configured", context);
			}
			else if (settings.m_pSaveData.Type() == EPF_EntitySaveDataClass)
			{
				isValid = false;
				ReportIssue(prefabName, "EPF_PersistenceComponent has invalid base save-data type", context);
			}
		}

		// Check: BaseWeaponComponent attachments
		BaseWeaponComponent weapon = BaseWeaponComponent.Cast(entity.FindComponent(BaseWeaponComponent));
		if (weapon)
		{
			// Validate weapon attachments
			ValidateWeaponAttachments(entity, prefabName, context);
		}

		return isValid;
	}

	//------------------------------------------------------------------------------------------------
	// Validate all attachments on a weapon
	//------------------------------------------------------------------------------------------------
	protected static void ValidateWeaponAttachments(IEntity weaponEntity, string weaponPrefab, string context)
	{
		WeaponAttachmentsStorageComponent attachStorage = WeaponAttachmentsStorageComponent.Cast(
			weaponEntity.FindComponent(WeaponAttachmentsStorageComponent));

		if (!attachStorage)
			return;

		array<IEntity> attachments = {};
		attachStorage.GetAll(attachments);

		foreach (IEntity attachment : attachments)
		{
			if (!attachment)
				continue;

			ValidateEntity(attachment, string.Format("attachment on %1", weaponPrefab));
		}
	}

	//------------------------------------------------------------------------------------------------
	// Report an issue (only once per prefab)
	//------------------------------------------------------------------------------------------------
	protected static void ReportIssue(string prefabName, string issue, string context)
	{
		string key = prefabName + "|" + issue;

		if (s_ReportedPrefabs.Contains(key))
			return;

		s_ReportedPrefabs.Insert(key);

		string contextStr = "";
		if (context != "")
			contextStr = string.Format(" [Context: %1]", context);

		Print(string.Format("[PrefabValidator] BROKEN: %1 - %2%3", prefabName, issue, contextStr), LogLevel.WARNING);
	}

	//------------------------------------------------------------------------------------------------
	// Get prefab name from entity
	//------------------------------------------------------------------------------------------------
	protected static string GetPrefabName(IEntity entity)
	{
		if (!entity)
			return "NULL";

		EntityPrefabData prefabData = entity.GetPrefabData();
		if (prefabData)
			return prefabData.GetPrefabName();

		return entity.ClassName();
	}

	//------------------------------------------------------------------------------------------------
	// Get list of all reported broken prefabs (for export)
	//------------------------------------------------------------------------------------------------
	static void PrintAllBrokenPrefabs()
	{
		if (!s_ReportedPrefabs || s_ReportedPrefabs.Count() == 0)
		{
			Print("[PrefabValidator] No broken prefabs detected yet.", LogLevel.NORMAL);
			return;
		}

		Print("[PrefabValidator] ===== BROKEN PREFABS REPORT =====", LogLevel.WARNING);

		for (int i = 0; i < s_ReportedPrefabs.Count(); i++)
		{
			Print(string.Format("[PrefabValidator] %1", s_ReportedPrefabs.Get(i)), LogLevel.WARNING);
		}

		Print(string.Format("[PrefabValidator] Total: %1 issues", s_ReportedPrefabs.Count()), LogLevel.WARNING);
	}

	//------------------------------------------------------------------------------------------------
	// Clear the reported prefabs list
	//------------------------------------------------------------------------------------------------
	static void ClearReports()
	{
		if (s_ReportedPrefabs)
			s_ReportedPrefabs.Clear();
	}
}
