// Modded ADM_MerchandiseItem to fix quantity bug when selling items
modded class ADM_MerchandiseItem : ADM_MerchandisePrefab
{
	// Override CollectMerchandise with stricter quantity enforcement
	override bool CollectMerchandise(IEntity player, ADM_ShopBaseComponent shop, ADM_ShopMerchandise merchandise, int quantity = 1)
	{
		if (!Replication.IsServer())
			return false;

		SCR_InventoryStorageManagerComponent inventory = SCR_InventoryStorageManagerComponent.Cast(player.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!inventory)
			return false;

		// Safety check - ensure quantity is at least 1
		if (quantity < 1)
			quantity = 1;

		// Find EXACTLY the number of items we need to sell - no more
		array<IEntity> itemsToSell = FindItemsToSellExact(player, GetPrefab(), quantity);

		// Verify we found the exact amount
		int foundCount = 0;
		if (itemsToSell)
			foundCount = itemsToSell.Count();

		if (!itemsToSell || foundCount != quantity)
		{
			Print(string.Format("[Shop] CollectMerchandise: Could not find exact quantity. Requested: %1, Found: %2", quantity, foundCount), LogLevel.WARNING);
			return false;
		}

		Print(string.Format("[Shop] CollectMerchandise: Selling %1 items", quantity), LogLevel.DEBUG);

		// Remove ONLY the items we found - use a for loop with explicit count
		array<IEntity> removedItems = {};
		int itemsRemoved = 0;

		for (int i = 0; i < quantity && i < itemsToSell.Count(); i++)
		{
			IEntity item = itemsToSell[i];
			if (!item)
				continue;

			bool success = inventory.TryRemoveItemFromInventory(item);
			if (success)
			{
				removedItems.Insert(item);
				itemsRemoved++;
				Print(string.Format("[Shop] Removed item %1 of %2", itemsRemoved, quantity), LogLevel.DEBUG);
			}
			else
			{
				Print(string.Format("[Shop] Failed to remove item %1", i), LogLevel.WARNING);
				break;
			}
		}

		// If we didn't remove the exact amount, return items and fail
		if (itemsRemoved != quantity)
		{
			Print(string.Format("[Shop] CollectMerchandise: Mismatch! Removed %1 but needed %2. Returning items.", itemsRemoved, quantity), LogLevel.ERROR);
			foreach (IEntity item : removedItems)
			{
				inventory.TryInsertItem(item);
			}
			return false;
		}

		// Delete ONLY the removed items
		foreach (IEntity item : removedItems)
		{
			SCR_EntityHelper.DeleteEntityAndChildren(item);
		}

		Print(string.Format("[Shop] CollectMerchandise: Successfully sold %1 items", quantity), LogLevel.DEBUG);
		return true;
	}

	// Find exactly the specified number of items - no more, no less
	protected array<IEntity> FindItemsToSellExact(IEntity player, ResourceName itemResource, int quantity)
	{
		SCR_InventoryStorageManagerComponent inventory = SCR_InventoryStorageManagerComponent.Cast(player.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!inventory)
			return null;

		array<IEntity> allItems = {};
		inventory.GetItems(allItems);

		array<IEntity> foundItems = {};

		foreach (IEntity item : allItems)
		{
			if (!item)
				continue;

			EntityPrefabData prefabData = item.GetPrefabData();
			if (!prefabData)
				continue;

			if (prefabData.GetPrefabName() == itemResource)
			{
				foundItems.Insert(item);

				// Stop as soon as we have EXACTLY the quantity we need
				if (foundItems.Count() == quantity)
					break;
			}
		}

		return foundItems;
	}
}
