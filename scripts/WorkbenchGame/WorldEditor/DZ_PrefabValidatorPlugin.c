// WorldEditor plugin to detect prefabs with missing required components
// Scans entities for common misconfigurations that cause crashes:
// - InventoryItemComponent without RplComponent
// - EPF_PersistenceComponent without valid save-data
// - WeaponAttachmentsStorageComponent issues

[WorkbenchPluginAttribute(
	name: "DatZ Prefab Validator",
	description: "Scans world entities for missing required components (RplComponent, EPF save-data)",
	category: "DatZ Tools",
	wbModules: {"WorldEditor"},
	awesomeFontCode: 0xf188)] // bug icon
class DZ_PrefabValidatorPlugin : WorldEditorPlugin
{
	// Track issues found
	protected ref array<string> m_aIssues = new array<string>();
	protected int m_iEntitiesScanned = 0;
	protected int m_iEntitiesWithIssues = 0;

	//------------------------------------------------------------------------------------------------
	override void Run()
	{
		// Reset counters
		m_aIssues.Clear();
		m_iEntitiesScanned = 0;
		m_iEntitiesWithIssues = 0;

		// Get World Editor API
		WorldEditorAPI worldEditorAPI = SCR_WorldEditorToolHelper.GetWorldEditorAPI();
		if (!worldEditorAPI)
		{
			Print("[DZ_PrefabValidator] Failed to get WorldEditorAPI", LogLevel.ERROR);
			Workbench.Dialog("DatZ Prefab Validator", "Failed to get WorldEditorAPI");
			return;
		}

		// Check if we should scan selected only or all entities
		int selectedCount = worldEditorAPI.GetSelectedEntitiesCount();
		bool scanSelected = selectedCount > 0;

		if (scanSelected)
		{
			// Scan selected entities only
			for (int i = 0; i < selectedCount; i++)
			{
				IEntitySource entitySource = worldEditorAPI.GetSelectedEntity(i);
				if (entitySource)
					ValidateEntitySource(entitySource);
			}
		}
		else
		{
			// Scan all entities in world
			int entityCount = worldEditorAPI.GetEditorEntityCount();
			for (int i = 0; i < entityCount; i++)
			{
				IEntitySource entitySource = worldEditorAPI.GetEditorEntity(i);
				if (entitySource)
					ValidateEntitySource(entitySource);
			}
		}

		// Build results message
		string scanScope = "All";
		if (scanSelected)
			scanScope = string.Format("Selected (%1)", selectedCount);

		// Always print to console
		Print("[DZ_PrefabValidator] ========== VALIDATION REPORT ==========", LogLevel.NORMAL);
		Print(string.Format("[DZ_PrefabValidator] Scope: %1 entities", scanScope), LogLevel.NORMAL);
		Print(string.Format("[DZ_PrefabValidator] Entities Scanned: %1", m_iEntitiesScanned), LogLevel.NORMAL);
		Print(string.Format("[DZ_PrefabValidator] Entities with Issues: %1", m_iEntitiesWithIssues), LogLevel.NORMAL);
		Print(string.Format("[DZ_PrefabValidator] Total Issues: %1", m_aIssues.Count()), LogLevel.NORMAL);

		string results = string.Format(
			"Scan Complete\n\n" +
			"Scope: %1 entities\n" +
			"Scanned: %2\n" +
			"Issues Found: %3\n\n",
			scanScope, m_iEntitiesScanned, m_aIssues.Count());

		// Add issue details (limit to first 20 for dialog)
		if (m_aIssues.Count() > 0)
		{
			Print("[DZ_PrefabValidator] ----- Issues Found -----", LogLevel.WARNING);
			for (int j = 0; j < m_aIssues.Count(); j++)
			{
				Print(string.Format("[DZ_PrefabValidator] %1", m_aIssues[j]), LogLevel.WARNING);
			}
			Print("[DZ_PrefabValidator] ========================================", LogLevel.WARNING);

			results += "Issues:\n";
			int showCount = m_aIssues.Count();
			if (showCount > 20)
				showCount = 20;

			for (int i = 0; i < showCount; i++)
			{
				results += m_aIssues[i] + "\n";
			}

			if (m_aIssues.Count() > 20)
			{
				results += string.Format("\n... and %1 more (see console log)", m_aIssues.Count() - 20);
			}
		}
		else
		{
			Print("[DZ_PrefabValidator] No issues found!", LogLevel.NORMAL);
			results += "No issues found!";
		}

		Print("[DZ_PrefabValidator] ========================================", LogLevel.NORMAL);
		Workbench.Dialog("DatZ Prefab Validator", results);
	}

	//------------------------------------------------------------------------------------------------
	//! Validate a single entity source for component issues
	protected void ValidateEntitySource(IEntitySource entitySource)
	{
		if (!entitySource)
			return;

		m_iEntitiesScanned++;

		string entityName = GetEntityDisplayName(entitySource);
		bool hasIssue = false;

		// Check for component configurations
		bool hasInventoryItem = HasComponent(entitySource, "InventoryItemComponent");
		bool hasRpl = HasComponent(entitySource, "RplComponent");
		bool hasEPFPersistence = HasComponent(entitySource, "EPF_PersistenceComponent");
		bool hasWeaponAttachStorage = HasComponent(entitySource, "WeaponAttachmentsStorageComponent");
		bool hasSCRWeaponAttachStorage = HasComponent(entitySource, "SCR_WeaponAttachmentsStorageComponent");

		// Rule 1: InventoryItemComponent requires RplComponent
		if (hasInventoryItem && !hasRpl)
		{
			m_aIssues.Insert(string.Format("MISSING_RPL: %1 - InventoryItemComponent without RplComponent", entityName));
			hasIssue = true;
		}

		// Rule 2: EPF_PersistenceComponent validation
		if (hasEPFPersistence)
		{
			// Check if save-data is configured - we can't fully validate from editor
			// but we can flag it for review
			IEntityComponentSource epfComp = FindComponent(entitySource, "EPF_PersistenceComponent");
			if (epfComp)
			{
				string saveDataType;
				if (!epfComp.Get("m_tSaveDataType", saveDataType) || saveDataType == "")
				{
					// Check m_pSaveData as fallback
					string saveDataValue;
					if (!epfComp.Get("m_pSaveData", saveDataValue) || saveDataValue == "")
					{
						m_aIssues.Insert(string.Format("NO_SAVEDATA: %1 - EPF_PersistenceComponent has no save-data configured", entityName));
						hasIssue = true;
					}
				}
			}
		}

		// Rule 3: WeaponAttachmentsStorageComponent should have SCR variant for safety
		if (hasWeaponAttachStorage && !hasSCRWeaponAttachStorage)
		{
			// This is a warning, not an error - vanilla component may work
			// but SCR version has better NULL handling
			m_aIssues.Insert(string.Format("WARN_ATTACH: %1 - Has WeaponAttachmentsStorageComponent but not SCR variant", entityName));
			hasIssue = true;
		}

		if (hasIssue)
			m_iEntitiesWithIssues++;
	}

	//------------------------------------------------------------------------------------------------
	//! Check if entity has a component of given type
	protected bool HasComponent(IEntitySource entitySource, string componentClassName)
	{
		return FindComponent(entitySource, componentClassName) != null;
	}

	//------------------------------------------------------------------------------------------------
	//! Find a component by class name on entity source
	protected IEntityComponentSource FindComponent(IEntitySource entitySource, string componentClassName)
	{
		if (!entitySource)
			return null;

		// Iterate through all components on the entity
		for (int i = 0, count = entitySource.GetComponentCount(); i < count; i++)
		{
			IEntityComponentSource compSource = entitySource.GetComponent(i);
			if (!compSource)
				continue;

			string compClass = compSource.GetClassName();

			// Direct match
			if (compClass == componentClassName)
				return compSource;

			// Check inheritance - the component might be a subclass
			typename compType = compClass.ToType();
			typename targetType = componentClassName.ToType();

			if (compType && targetType && compType.IsInherited(targetType))
				return compSource;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Get display name for entity (prefab name or class name)
	protected string GetEntityDisplayName(IEntitySource entitySource)
	{
		if (!entitySource)
			return "NULL";

		// Try to get the resource name (prefab path)
		string resourceName = entitySource.GetResourceName();
		if (resourceName != "")
		{
			// Extract just the filename from the path
			int lastSlash = resourceName.LastIndexOf("/");
			if (lastSlash >= 0)
				return resourceName.Substring(lastSlash + 1, resourceName.Length() - lastSlash - 1);
			return resourceName;
		}

		// Fallback to class name
		return entitySource.GetClassName();
	}

	//------------------------------------------------------------------------------------------------
	override void Configure()
	{
		string message =
			"DatZ Prefab Validator\n\n" +
			"This plugin scans entities for missing required components that cause server crashes.\n\n" +
			"Checks performed:\n" +
			"- InventoryItemComponent requires RplComponent\n" +
			"- EPF_PersistenceComponent requires valid save-data\n" +
			"- WeaponAttachmentsStorageComponent NULL handling\n\n" +
			"Usage:\n" +
			"- Select entities to scan specific items\n" +
			"- Or run with nothing selected to scan entire world\n\n" +
			"Results are shown in dialog and logged to console.";

		Workbench.Dialog("DatZ Prefab Validator - Help", message);
	}
}
