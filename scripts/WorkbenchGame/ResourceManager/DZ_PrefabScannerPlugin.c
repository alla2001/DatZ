// Resource Manager plugin to scan selected prefab files for missing required components
// Select .et prefab files in Resource Browser, then run this plugin

[WorkbenchPluginAttribute(
	name: "DatZ Prefab Scanner",
	category: "DatZ Tools",
	description: "Scans selected prefabs for missing RplComponent or EPF save-data",
	wbModules: {"ResourceManager"},
	awesomeFontCode: 0xf188)]
class DZ_PrefabScannerPlugin : ResourceManagerPlugin
{
	// Settings
	[Attribute("1", UIWidgets.CheckBox, "Check for missing RplComponent")]
	protected bool m_bCheckMissingRpl;

	[Attribute("1", UIWidgets.CheckBox, "Check for missing EPF save-data")]
	protected bool m_bCheckMissingSaveData;

	// Results
	protected ref array<string> m_aIssues = new array<string>();
	protected int m_iPrefabsScanned = 0;
	protected int m_iPrefabsWithIssues = 0;

	//------------------------------------------------------------------------------------------------
	override void Run()
	{
		// Reset
		m_aIssues.Clear();
		m_iPrefabsScanned = 0;
		m_iPrefabsWithIssues = 0;

		// Get Resource Manager
		ResourceManager resourceManager = Workbench.GetModule(ResourceManager);
		if (!resourceManager)
		{
			Print("[DZ_PrefabScanner] Failed to get ResourceManager", LogLevel.ERROR);
			return;
		}

		// Get selected resources
		ref array<ResourceName> selectedResources = new array<ResourceName>();
		
		resourceManager.GetResourceBrowserSelection(selectedResources.Insert, true);

		if (selectedResources.Count() == 0)
		{
			Workbench.Dialog("DatZ Prefab Scanner", "No prefabs selected.\n\nSelect .et prefab files in Resource Browser first.");
			return;
		}

		// Process each selected resource
		for (int i = 0; i < selectedResources.Count(); i++)
		{
			string path = selectedResources[i];

			// Only process .et files
			if (!path.EndsWith(".et"))
				continue;

			ScanPrefab(selectedResources[i]);
		}

		// Show results
		ShowResults();
	}

	//------------------------------------------------------------------------------------------------
	override void Configure()
	{
		Workbench.ScriptDialog("DatZ Prefab Scanner Settings", "Configure which checks to perform.\n\nSelect prefab files in Resource Browser, then run the plugin.", this);
	}

	//------------------------------------------------------------------------------------------------
	protected void ScanPrefab(ResourceName prefabPath)
	{
		m_iPrefabsScanned++;

		Resource resource = Resource.Load(prefabPath);
		if (!resource.IsValid())
		{
			m_aIssues.Insert(string.Format("LOAD_FAILED: %1", prefabPath));
			m_iPrefabsWithIssues++;
			return;
		}

		string prefabName = GetPrefabDisplayName(prefabPath);
		bool hasIssue = false;

		// Check components
		IEntityComponentSource invItemComp = SCR_BaseContainerTools.FindComponentSource(resource, "InventoryItemComponent");
		IEntityComponentSource rplComp = SCR_BaseContainerTools.FindComponentSource(resource, "RplComponent");
		IEntityComponentSource epfComp = SCR_BaseContainerTools.FindComponentSource(resource, "EPF_PersistenceComponent");

		// Rule 1: InventoryItemComponent requires RplComponent
		if (m_bCheckMissingRpl && invItemComp && !rplComp)
		{
			m_aIssues.Insert(string.Format("MISSING_RPL: %1", prefabName));
			hasIssue = true;
		}

		// Rule 2: EPF_PersistenceComponent needs save-data
		if (m_bCheckMissingSaveData && epfComp)
		{
			string saveDataType;
			string saveDataValue;
			bool hasType = epfComp.Get("m_tSaveDataType", saveDataType) && saveDataType != "";
			bool hasValue = epfComp.Get("m_pSaveData", saveDataValue) && saveDataValue != "";

			if (!hasType && !hasValue)
			{
				m_aIssues.Insert(string.Format("NO_SAVEDATA: %1", prefabName));
				hasIssue = true;
			}
		}

		if (hasIssue)
			m_iPrefabsWithIssues++;
	}

	//------------------------------------------------------------------------------------------------
	protected void ShowResults()
	{
		// Always print to console
		Print("[DZ_PrefabScanner] ========== SCAN REPORT ==========", LogLevel.NORMAL);
		Print(string.Format("[DZ_PrefabScanner] Prefabs Scanned: %1", m_iPrefabsScanned), LogLevel.NORMAL);
		Print(string.Format("[DZ_PrefabScanner] Prefabs with Issues: %1", m_iPrefabsWithIssues), LogLevel.NORMAL);
		Print(string.Format("[DZ_PrefabScanner] Total Issues: %1", m_aIssues.Count()), LogLevel.NORMAL);

		string results = string.Format(
			"Scan Complete\n\n" +
			"Prefabs Scanned: %1\n" +
			"With Issues: %2\n" +
			"Total Issues: %3\n\n",
			m_iPrefabsScanned, m_iPrefabsWithIssues, m_aIssues.Count());

		if (m_aIssues.Count() > 0)
		{
			Print("[DZ_PrefabScanner] ----- Issues Found -----", LogLevel.WARNING);
			for (int j = 0; j < m_aIssues.Count(); j++)
			{
				Print(string.Format("[DZ_PrefabScanner] %1", m_aIssues[j]), LogLevel.WARNING);
			}
			Print("[DZ_PrefabScanner] ================================", LogLevel.WARNING);

			results += "Issues:\n";
			int showCount = m_aIssues.Count();
			if (showCount > 15)
				showCount = 15;

			for (int i = 0; i < showCount; i++)
			{
				results += "- " + m_aIssues[i] + "\n";
			}

			if (m_aIssues.Count() > 15)
				results += string.Format("\n...and %1 more (see console)", m_aIssues.Count() - 15);
		}
		else
		{
			Print("[DZ_PrefabScanner] No issues found!", LogLevel.NORMAL);
			results += "No issues found!";
		}

		Print("[DZ_PrefabScanner] ================================", LogLevel.NORMAL);
		Workbench.Dialog("DatZ Prefab Scanner", results);
	}

	//------------------------------------------------------------------------------------------------
	protected string GetPrefabDisplayName(string prefabPath)
	{
		int lastSlash = prefabPath.LastIndexOf("/");
		if (lastSlash >= 0)
			return prefabPath.Substring(lastSlash + 1, prefabPath.Length() - lastSlash - 1);
		return prefabPath;
	}

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Copy Issues to Clipboard")]
	void CopyToClipboard()
	{
		if (m_aIssues.Count() == 0)
		{
			System.ExportToClipboard("No issues found");
			return;
		}

		string output = "";
		for (int i = 0; i < m_aIssues.Count(); i++)
		{
			output += m_aIssues[i] + "\n";
		}
		System.ExportToClipboard(output);
	}
}
