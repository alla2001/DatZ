class WorkbenchCraftingMenu : MenuBase
{
	protected static const string BUTTON_CLOSE = "ButtonClose";
	protected static const string RECIPE_LIST = "RecipeList";
	protected static const ResourceName RECIPE_ENTRY_LAYOUT = "{67B1C2D3E4F5A600}UI/layouts/Menus/Crafting/CraftingRecipeEntry.layout";

	protected VerticalLayoutWidget m_wRecipeList;
	protected IEntity m_Workbench;
	protected ref array<Widget> m_aRecipeWidgets = {};
	protected ref array<ref CraftingRecipe> m_aRecipes = {};
	protected ref map<Widget, int> m_mButtonToRecipeIndex = new map<Widget, int>();

	// Set workbench before opening menu
	static IEntity s_PendingWorkbench;

	static void SetWorkbench(IEntity workbench)
	{
		s_PendingWorkbench = workbench;
	}

	override void OnMenuOpen()
	{
		super.OnMenuOpen();

		m_Workbench = s_PendingWorkbench;
		s_PendingWorkbench = null;

		Widget rootWidget = GetRootWidget();
		if (!rootWidget)
		{
			Print("[WorkbenchCraftingMenu] Error: Root widget not found!", LogLevel.ERROR);
			return;
		}

		// Get recipe list container
		m_wRecipeList = VerticalLayoutWidget.Cast(rootWidget.FindAnyWidget(RECIPE_LIST));
		if (!m_wRecipeList)
		{
			Print("[WorkbenchCraftingMenu] Error: RecipeList widget not found!", LogLevel.ERROR);
		}

		// Setup close button
		SCR_ButtonTextComponent buttonClose = SCR_ButtonTextComponent.GetButtonText(BUTTON_CLOSE, rootWidget);
		if (buttonClose)
		{
			buttonClose.m_OnClicked.Insert(Close);
		}

		// Setup input listeners
		InputManager inputManager = GetGame().GetInputManager();
		if (inputManager)
		{
			inputManager.AddActionListener("MenuOpen", EActionTrigger.DOWN, Close);
			inputManager.AddActionListener("MenuBack", EActionTrigger.DOWN, Close);
#ifdef WORKBENCH
			inputManager.AddActionListener("MenuOpenWB", EActionTrigger.DOWN, Close);
			inputManager.AddActionListener("MenuBackWB", EActionTrigger.DOWN, Close);
#endif
		}

		// Load recipes
		LoadRecipes();
	}

	override void OnMenuClose()
	{
		super.OnMenuClose();

		// Clean up input listeners
		InputManager inputManager = GetGame().GetInputManager();
		if (inputManager)
		{
			inputManager.RemoveActionListener("MenuOpen", EActionTrigger.DOWN, Close);
			inputManager.RemoveActionListener("MenuBack", EActionTrigger.DOWN, Close);
#ifdef WORKBENCH
			inputManager.RemoveActionListener("MenuOpenWB", EActionTrigger.DOWN, Close);
			inputManager.RemoveActionListener("MenuBackWB", EActionTrigger.DOWN, Close);
#endif
		}

		// Clear recipe widgets
		ClearRecipeWidgets();
		m_Workbench = null;
	}

	protected void LoadRecipes()
	{
		ClearRecipeWidgets();

		DZCraftingSystem craftingSystem = DZCraftingSystem.GetInstance();
		if (!craftingSystem || !craftingSystem.craftingTable)
		{
			Print("[WorkbenchCraftingMenu] Error: Crafting system not found!", LogLevel.ERROR);
			return;
		}

		IEntity player = GetGame().GetPlayerController().GetControlledEntity();
		if (!player)
			return;

		SCR_InventoryStorageManagerComponent inventory = SCR_InventoryStorageManagerComponent.Cast(player.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!inventory)
			return;

		array<IEntity> playerItems = {};
		inventory.GetItems(playerItems);

		m_aRecipes.Clear();

		foreach (CraftingRecipe recipe : craftingSystem.craftingTable.recipes)
		{
			if (recipe.barricading)
				continue;

			m_aRecipes.Insert(recipe);
			CreateRecipeEntry(recipe, playerItems, m_aRecipes.Count() - 1);
		}
	}

	protected void CreateRecipeEntry(CraftingRecipe recipe, array<IEntity> playerItems, int recipeIndex)
	{
		if (!m_wRecipeList)
			return;

		Widget entryWidget = GetGame().GetWorkspace().CreateWidgets(RECIPE_ENTRY_LAYOUT, m_wRecipeList);
		if (!entryWidget)
		{
			Print("[WorkbenchCraftingMenu] Failed to create recipe entry widget", LogLevel.WARNING);
			return;
		}

		m_aRecipeWidgets.Insert(entryWidget);

		// Set recipe name
		TextWidget nameWidget = TextWidget.Cast(entryWidget.FindAnyWidget("RecipeName"));
		if (nameWidget)
			nameWidget.SetText(recipe.name);

		// Set item preview from prefab
		ItemPreviewWidget previewWidget = ItemPreviewWidget.Cast(entryWidget.FindAnyWidget("ItemPreview"));
		if (previewWidget && recipe.resultItem)
		{
			ChimeraWorld world = ChimeraWorld.CastFrom(GetGame().GetWorld());
			if (world)
			{
				ItemPreviewManagerEntity previewManager = world.GetItemPreviewManager();
				if (previewManager)
				{
					previewManager.SetPreviewItemFromPrefab(previewWidget, recipe.resultItem);
				}
			}
		}

		// Build requirements string
		string reqText = "Requires: " + BuildItemListString(recipe.requiredItems);
		TextWidget reqWidget = TextWidget.Cast(entryWidget.FindAnyWidget("Requirements"));
		if (reqWidget)
			reqWidget.SetText(reqText);

		// Check if can craft
		bool canCraft = recipe.GetCanBeCraftedWith(playerItems);

		// Set missing items text
		TextWidget missingWidget = TextWidget.Cast(entryWidget.FindAnyWidget("MissingItems"));
		Widget disabledOverlay = entryWidget.FindAnyWidget("DisabledOverlay");
		PanelWidget leftAccent = PanelWidget.Cast(entryWidget.FindAnyWidget("LeftAccent"));

		if (canCraft)
		{
			if (missingWidget)
				missingWidget.SetVisible(false);
			if (disabledOverlay)
				disabledOverlay.SetVisible(false);
			if (leftAccent)
				leftAccent.SetColor(Color.FromRGBA(60, 180, 80, 200)); // Green accent
		}
		else
		{
			string missingText = GetMissingItemsText(recipe, playerItems);
			if (missingWidget)
			{
				missingWidget.SetText(missingText);
				missingWidget.SetVisible(true);
			}
			if (disabledOverlay)
				disabledOverlay.SetVisible(true);
			if (leftAccent)
				leftAccent.SetColor(Color.FromRGBA(180, 60, 50, 200)); // Red accent
		}

		// Setup craft button
		ButtonWidget craftBtn = ButtonWidget.Cast(entryWidget.FindAnyWidget("CraftButton"));
		if (craftBtn)
		{
			craftBtn.SetEnabled(canCraft);
			if (!canCraft)
				craftBtn.SetColor(Color.FromRGBA(60, 60, 60, 180));
		}

		SCR_ButtonTextComponent craftBtnComp = SCR_ButtonTextComponent.GetButtonText("CraftButton", entryWidget);
		if (craftBtnComp && canCraft)
		{
			// Store button -> recipe index mapping
			m_mButtonToRecipeIndex.Set(craftBtn, recipeIndex);
			craftBtnComp.m_OnClicked.Insert(OnCraftButtonClicked);
		}
	}

	protected void OnCraftButtonClicked(SCR_ButtonTextComponent button)
	{
		Widget btnWidget = button.GetRootWidget();
		if (!btnWidget)
			return;

		if (!m_mButtonToRecipeIndex.Contains(btnWidget))
			return;

		int recipeIndex = m_mButtonToRecipeIndex.Get(btnWidget);
		OnCraftRecipe(recipeIndex);
	}

	protected void OnCraftRecipe(int recipeIndex)
	{
		if (recipeIndex < 0 || recipeIndex >= m_aRecipes.Count())
			return;

		CraftingRecipe recipe = m_aRecipes[recipeIndex];
		if (!recipe)
			return;

		// Get player
		IEntity player = GetGame().GetPlayerController().GetControlledEntity();
		if (!player)
			return;

		// Get workbench RplId
		RplId workbenchRplId = RplId.Invalid();
		if (m_Workbench)
		{
			RplComponent rpl = RplComponent.Cast(m_Workbench.FindComponent(RplComponent));
			if (rpl)
				workbenchRplId = rpl.Id();
		}

		// Send RPC to server
		WorkbenchCraftingComponent craftComp = WorkbenchCraftingComponent.Cast(player.FindComponent(WorkbenchCraftingComponent));
		if (craftComp)
		{
			craftComp.RequestCraft(recipeIndex, workbenchRplId);
		}
		else
		{
			Print("[WorkbenchCraftingMenu] WorkbenchCraftingComponent not found on player!", LogLevel.ERROR);
		}

		// Play click sound
		AudioSystem.PlaySound("{E3E34E1B0BC94432}Sounds/UI/Samples/Menu/UI_Task_Succeded.wav");

		// Close menu after crafting
		Close();
	}

	protected string BuildItemListString(array<ResourceName> items)
	{
		map<string, int> itemCounts = new map<string, int>();

		foreach (ResourceName itemRes : items)
		{
			string itemName = GetItemDisplayName(itemRes);
			if (itemCounts.Contains(itemName))
				itemCounts[itemName] = itemCounts[itemName] + 1;
			else
				itemCounts[itemName] = 1;
		}

		string result = "";
		bool first = true;
		foreach (string name, int count : itemCounts)
		{
			if (!first)
				result += ", ";
			if (count > 1)
				result += name + " x" + count;
			else
				result += name;
			first = false;
		}

		return result;
	}

	protected string GetItemDisplayName(ResourceName itemRes)
	{
		string path = itemRes;
		int lastSlash = path.LastIndexOf("/");
		if (lastSlash >= 0)
			path = path.Substring(lastSlash + 1, path.Length() - lastSlash - 1);

		int dotPos = path.LastIndexOf(".");
		if (dotPos >= 0)
			path = path.Substring(0, dotPos);

		return path;
	}

	protected string GetMissingItemsText(CraftingRecipe recipe, array<IEntity> playerItems)
	{
		array<ResourceName> tmpRequired = {};
		tmpRequired.Copy(recipe.requiredItems);

		foreach (IEntity ent : playerItems)
		{
			if (!ent)
				continue;
			ResourceName rec = ent.GetPrefabData().GetPrefabName();
			if (!rec)
				continue;

			int index = tmpRequired.Find(rec);
			if (index != -1)
				tmpRequired.Remove(index);
		}

		return "Missing: " + BuildItemListString(tmpRequired);
	}

	protected void ClearRecipeWidgets()
	{
		foreach (Widget w : m_aRecipeWidgets)
		{
			if (w)
				w.RemoveFromHierarchy();
		}
		m_aRecipeWidgets.Clear();
		m_mButtonToRecipeIndex.Clear();
	}
}
