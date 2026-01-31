[ComponentEditorProps(category: "Crafting", description: "Handles workbench crafting requests with server authority")]
class WorkbenchCraftingComponentClass : ScriptComponentClass {}

class WorkbenchCraftingComponent : ScriptComponent
{
	protected static const float MAX_WORKBENCH_DISTANCE = 5.0;

	//------------------------------------------------------------------------------------------------
	// Called by client to request crafting
	void RequestCraft(int recipeIndex, RplId workbenchRplId)
	{
		Rpc(RpcAsk_Craft, recipeIndex, workbenchRplId);
	}

	//------------------------------------------------------------------------------------------------
	// Server RPC - validates and performs crafting
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_Craft(int recipeIndex, RplId workbenchRplId)
	{
		if (!Replication.IsServer())
			return;

		IEntity owner = GetOwner();
		if (!owner)
			return;

		// Validate workbench distance
		IEntity workbench = null;
		if (workbenchRplId.IsValid())
		{
			RplComponent rpl = RplComponent.Cast(Replication.FindItem(workbenchRplId));
			if (rpl)
				workbench = rpl.GetEntity();
		}

		if (workbench)
		{
			float dist = vector.Distance(owner.GetOrigin(), workbench.GetOrigin());
			if (dist > MAX_WORKBENCH_DISTANCE)
			{
				Print("[WorkbenchCrafting] Player too far from workbench: " + dist, LogLevel.WARNING);
				Rpc(RpcDo_CraftResult, false, "Too far from workbench");
				return;
			}
		}

		// Get crafting system
		DZCraftingSystem craftingSystem = DZCraftingSystem.GetInstance();
		if (!craftingSystem || !craftingSystem.craftingTable)
		{
			Rpc(RpcDo_CraftResult, false, "Crafting system not available");
			return;
		}

		// Get recipe - filter barricading recipes
		array<ref CraftingRecipe> validRecipes = {};
		foreach (CraftingRecipe r : craftingSystem.craftingTable.recipes)
		{
			if (!r.barricading)
				validRecipes.Insert(r);
		}

		if (recipeIndex < 0 || recipeIndex >= validRecipes.Count())
		{
			Rpc(RpcDo_CraftResult, false, "Invalid recipe");
			return;
		}

		CraftingRecipe recipe = validRecipes[recipeIndex];
		if (!recipe)
		{
			Rpc(RpcDo_CraftResult, false, "Recipe not found");
			return;
		}

		// Get player inventory
		SCR_InventoryStorageManagerComponent inventory = SCR_InventoryStorageManagerComponent.Cast(owner.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!inventory)
		{
			Rpc(RpcDo_CraftResult, false, "No inventory found");
			return;
		}

		array<IEntity> playerItems = {};
		inventory.GetItems(playerItems);

		// Validate player has required items (SERVER AUTHORITY CHECK)
		if (!recipe.GetCanBeCraftedWith(playerItems))
		{
			Rpc(RpcDo_CraftResult, false, "Missing required items");
			return;
		}

		// Get items to consume
		array<IEntity> usedEnts = {};
		recipe.GetBestIngerdiants(playerItems, usedEnts);

		if (usedEnts.IsEmpty())
		{
			Rpc(RpcDo_CraftResult, false, "Could not find items to use");
			return;
		}

		// Remove consumed items
		foreach (IEntity ent : usedEnts)
		{
			if (ent)
				RplComponent.DeleteRplEntity(ent, false);
		}

		// Spawn result item
		Resource resource = Resource.Load(recipe.resultItem);
		if (!resource || !resource.IsValid())
		{
			Rpc(RpcDo_CraftResult, false, "Invalid result item");
			return;
		}

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		owner.GetWorldTransform(params.Transform);

		IEntity resultEntity = GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), params);
		if (!resultEntity)
		{
			Rpc(RpcDo_CraftResult, false, "Failed to create item");
			return;
		}

		// Try to insert into player inventory
		bool insertedToInventory = inventory.TryInsertItem(resultEntity, EStoragePurpose.PURPOSE_ANY);

		if (!insertedToInventory)
		{
			// Place on workbench or ground
			vector spawnPos;
			if (workbench)
			{
				spawnPos = workbench.GetOrigin() + "0 1 0"; // On top of workbench
			}
			else
			{
				spawnPos = owner.GetOrigin() + "0 0.5 0.5"; // In front of player
			}

			vector mat[4];
			resultEntity.GetWorldTransform(mat);
			mat[3] = spawnPos;
			resultEntity.SetWorldTransform(mat);

			Print("[WorkbenchCrafting] Item placed on ground - inventory full", LogLevel.NORMAL);
		}

		// Success
		Rpc(RpcDo_CraftResult, true, recipe.name + " crafted!");
		Print("[WorkbenchCrafting] Successfully crafted: " + recipe.name, LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	// Client RPC - receives crafting result
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_CraftResult(bool success, string message)
	{
		if (success)
		{
			// Play success sound
			AudioSystem.PlaySound("{E3E34E1B0BC94432}Sounds/UI/Samples/Menu/UI_Task_Succeded.wav");
			Print("[WorkbenchCrafting] " + message, LogLevel.NORMAL);
		}
		else
		{
			// Play fail sound
			AudioSystem.PlaySound("{CD1E50FCB3BA1C93}Sounds/UI/Samples/Menu/UI_Menu_Back.wav");
			Print("[WorkbenchCrafting] Failed: " + message, LogLevel.WARNING);
		}
	}
}
