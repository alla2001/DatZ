[WorkbenchPluginAttribute(
	name: "Regenerate All Generators",
	description: "Regenerates all generator entities in the world",
	wbModules: {"WorldEditor"},
	awesomeFontCode: 0xf021)] // refresh icon
class SCR_RegenerateAllGeneratorsPlugin : WorldEditorPlugin
{
	//------------------------------------------------------------------------------------------------
	override void Run()
	{
		// Get World Editor API
		WorldEditorAPI worldEditorAPI = SCR_WorldEditorToolHelper.GetWorldEditorAPI();
		if (!worldEditorAPI)
		{
			Print("SCR_RegenerateAllGeneratorsPlugin: Failed to get WorldEditorAPI", LogLevel.ERROR);
			return;
		}

		// Count generators found and processed
		int generatorCount = 0;
		int regeneratedCount = 0;

		// Begin an action sequence to make this operation undoable
		worldEditorAPI.BeginEntityAction();

		// Iterate through all entities in the world
		for (int i = 0, count = worldEditorAPI.GetEditorEntityCount(); i < count; i++)
		{
			IEntitySource entitySource = worldEditorAPI.GetEditorEntity(i);
			if (!entitySource)
				continue;

			// Check if this entity is a generator (inherits from SCR_GeneratorBaseEntity)
			string className = entitySource.GetClassName();
			typename classType = className.ToType();

			if (!classType)
				continue;

			// Check if it's a generator type
			if (classType.IsInherited(SCR_GeneratorBaseEntity))
			{
				generatorCount++;

				// Trigger regeneration by modifying the seed value
				// This will trigger _WB_OnKeyChanged which calls OnShapeChanged/Generate
				if (RegenerateGenerator(worldEditorAPI, entitySource))
				{
					regeneratedCount++;
				}
			}
		}

		// End the action sequence
		worldEditorAPI.EndEntityAction();

		// Print results
		Print(string.Format("SCR_RegenerateAllGeneratorsPlugin: Found %1 generators, regenerated %2",
			generatorCount, regeneratedCount), LogLevel.NORMAL);

		// Show notification to user
		string message = string.Format("Regenerated %1 out of %2 generators", regeneratedCount, generatorCount);
		Workbench.Dialog("Regenerate All Generators", message);
	}

	//------------------------------------------------------------------------------------------------
	//! Triggers regeneration of a single generator by modifying its seed attribute
	//! \param worldEditorAPI The World Editor API
	//! \param entitySource The generator entity source
	//! \return true if regeneration was triggered successfully
	protected bool RegenerateGenerator(WorldEditorAPI worldEditorAPI, IEntitySource entitySource)
	{
		if (!worldEditorAPI || !entitySource)
			return false;

		// Begin edit sequence for this entity
		worldEditorAPI.BeginEditSequence(entitySource);

		// Get current seed value
		string currentSeedStr;
		if (!entitySource.Get("m_iSeed", currentSeedStr))
		{
			// If we can't get the seed, try toggling a different attribute
			// Toggle m_bEnableGeneration twice (off then on) to force regeneration
			worldEditorAPI.SetVariableValue(entitySource, null, "m_bEnableGeneration", "0");
			worldEditorAPI.SetVariableValue(entitySource, null, "m_bEnableGeneration", "1");
			worldEditorAPI.EndEditSequence(entitySource);
			return true;
		}

		// Parse current seed
		int currentSeed = currentSeedStr.ToInt();

		// Modify seed temporarily (add 1 then subtract 1 to trigger the change event)
		// This ensures the generator regenerates while keeping the same seed
		int tempSeed = currentSeed + 1;
		if (tempSeed > 32767) // MAX_RANDOM_SEED
			tempSeed = 0;

		// Set temporary seed - this triggers _WB_OnKeyChanged
		worldEditorAPI.SetVariableValue(entitySource, null, "m_iSeed", tempSeed.ToString());

		// Restore original seed - this triggers _WB_OnKeyChanged again
		worldEditorAPI.SetVariableValue(entitySource, null, "m_iSeed", currentSeed.ToString());

		// End edit sequence
		worldEditorAPI.EndEditSequence(entitySource);

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Called by Workbench to configure the plugin
	override void Configure()
	{
		string message = "This plugin will regenerate all generator entities in the current world.\n\n" +
			"This includes:\n" +
			"- Forest Generators\n" +
			"- Prefab Generators\n" +
			"- Wall Generators\n" +
			"- Track Generators\n" +
			"- Powerline Generators\n" +
			"- All other generator types\n\n" +
			"The operation can be undone using Ctrl+Z.";

		Workbench.Dialog("Regenerate All Generators", message);
	}
}
