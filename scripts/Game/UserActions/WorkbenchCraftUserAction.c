class WorkbenchCraftUserAction : ScriptedUserAction
{
	//------------------------------------------------------------------------------------------------
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		super.Init(pOwnerEntity, pManagerComponent);
	}

	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		// Open the crafting menu on the client
		if (!GetGame().GetPlayerController())
			return;

		// Store workbench reference for menu
		WorkbenchCraftingMenu.SetWorkbench(pOwnerEntity);

		// Open the menu dialog
		GetGame().GetMenuManager().OpenDialog(ChimeraMenuPreset.WorkbenchCrafting, DialogPriority.INFORMATIVE, 0, true);
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBePerformedScript(IEntity user)
	{
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBeShownScript(IEntity user)
	{
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool GetActionNameScript(out string outName)
	{
		outName = "Use Workbench";
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool HasLocalEffectOnlyScript()
	{
		// Menu opens locally, crafting will be handled by RPC
		return true;
	}
}
