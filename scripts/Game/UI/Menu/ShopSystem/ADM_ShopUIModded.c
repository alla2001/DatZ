

// Modded ADM_ShopUI_Item to fix sell validation
modded class ADM_ShopUI_Item : SCR_ModularButtonComponent
{
	// Override CanPurchase to also validate sell items
	override bool CanPurchase()
	{
		if (GetBuyOrSell())
		{
			// Buy item - use original logic
			int additionalQuantity = m_quantity;
			if (m_bIsCartItem)
			{
				additionalQuantity = 0;
			}

			return m_ShopUI.CanPurchase(m_Merchandise, additionalQuantity);
		}
		else
		{
			// Sell item - validate quantity against inventory
			int additionalQuantity = m_quantity;
			if (m_bIsCartItem)
			{
				additionalQuantity = 0;
			}

			return m_ShopUI.CanSell(m_Merchandise, additionalQuantity);
		}
	}
}

modded class ADM_ShopUI: ChimeraMenuBase
{
	static const ResourceName m_BarterIconPrefab = "{BBEF0EBB9F35B19F}UI/Layouts/Menus/BasicShopMenu/MarketBarterItemIcon.layout";

	// Override AddToCart with strict quantity enforcement for sell items
	override void AddToCart(ADM_ShopUI_Item item)
	{
		ADM_ShopMerchandise merch = item.GetMerchandise();
		if (!merch)
			return;

		if (!item.CanPurchase())
			return;

		int quantityToAdd = item.GetQuantity();
		string itemName = "Unknown";
		ADM_MerchandiseType merchType = merch.GetType();
		if (merchType)
			itemName = merchType.GetDisplayName();

		bool isSell = !item.GetBuyOrSell();

		map<ADM_ShopMerchandise, int> shoppingCart = m_BuyShoppingCart;
		if (isSell)
			shoppingCart = m_SellShoppingCart;

		// For sell items, enforce strict quantity limit
		if (isSell)
		{
			int playerHas = GetPlayerItemCount(merch);
			int alreadyInCart = 0;
			if (shoppingCart.Contains(merch))
				alreadyInCart = shoppingCart.Get(merch);

			int maxCanAdd = playerHas - alreadyInCart;

			// Force quantity to not exceed what player has available
			if (quantityToAdd > maxCanAdd)
			{
				Print(string.Format("[Shop AddToCart] LIMITING quantity from %1 to %2 (player has %3, in cart %4)", quantityToAdd, maxCanAdd, playerHas, alreadyInCart), LogLevel.WARNING);
				quantityToAdd = maxCanAdd;
			}

			// If nothing can be added, don't add
			if (quantityToAdd <= 0)
			{
				Print(string.Format("[Shop AddToCart] Cannot add more - player has %1, cart has %2", playerHas, alreadyInCart), LogLevel.WARNING);
				return;
			}
		}

		Print(string.Format("[Shop AddToCart] Item: %1, Quantity: %2, IsSell: %3", itemName, quantityToAdd, isSell), LogLevel.DEBUG);

		bool inShoppingCart = shoppingCart.Contains(merch);
		if (inShoppingCart)
		{
			int curQuantity = shoppingCart.Get(merch);
			int newQuantity = curQuantity + quantityToAdd;
			shoppingCart.Set(merch, newQuantity);
			Print(string.Format("[Shop AddToCart] Updated cart. Previous: %1, New: %2", curQuantity, newQuantity), LogLevel.DEBUG);
		}
		else
		{
			shoppingCart.Insert(merch, quantityToAdd);
			Print(string.Format("[Shop AddToCart] Inserted new item with quantity: %1", quantityToAdd), LogLevel.DEBUG);
		}

		PopulateCartTab(m_wCartTabView);
	}

	// Validate if player can sell the merchandise
	bool CanSell(ADM_ShopMerchandise additionalMerchandise = null, int additionalQuantity = 0)
	{
		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!playerController)
			return false;

		IEntity player = playerController.GetMainEntity();
		if (!player)
			return false;

		SCR_InventoryStorageManagerComponent inventory = SCR_InventoryStorageManagerComponent.Cast(player.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!inventory)
			return false;

		// Count items in sell cart for this merchandise type
		int cartQuantityForMerch = 0;
		foreach(ADM_ShopMerchandise merch, int quantity : m_SellShoppingCart)
		{
			if (additionalMerchandise && additionalMerchandise == merch)
				cartQuantityForMerch = quantity;
		}

		// Get total quantity we're trying to sell
		int totalQuantity = cartQuantityForMerch + additionalQuantity;

		if (additionalMerchandise && totalQuantity > 0)
		{
			// Get the merchandise type to check how many items player has
			ADM_MerchandiseType merchType = additionalMerchandise.GetType();
			if (!merchType)
				return false;

			// Cast to item merchandise to get the prefab
			ADM_MerchandisePrefab merchPrefab = ADM_MerchandisePrefab.Cast(merchType);
			if (!merchPrefab)
				return true; // Not an item type, skip validation

			ResourceName itemResource = merchPrefab.GetPrefab();
			if (itemResource.IsEmpty())
				return false;

			// Count how many of this item the player has
			array<IEntity> items = {};
			inventory.GetItems(items);

			int playerItemCount = 0;
			foreach (IEntity item : items)
			{
				if (item.GetPrefabData().GetPrefabName() == itemResource)
				{
					playerItemCount++;
				}
			}

			// Check if player has enough items
			if (totalQuantity > playerItemCount)
				return false;
		}

		return true;
	}

	// Coin prefab paths
	static const ResourceName GOLD_COIN_PREFAB = "{C53F5A5C87A34A46}Prefabs/Valuables/Coins/Gold Coin.et";
	static const ResourceName SILVER_COIN_PREFAB = "{D6D9E68597E059C5}Prefabs/Valuables/Coins/Silver Coin.et";
	static const ResourceName COPPER_COIN_PREFAB = "{B66DF252C948EE53}Prefabs/Valuables/Coins/Copper Coin.et";

	// Cached coin counts
	protected int m_iCachedGoldCount = 0;
	protected int m_iCachedSilverCount = 0;
	protected int m_iCachedCopperCount = 0;

	// Count specific coin type in inventory
	protected int CountCoinType(SCR_InventoryStorageManagerComponent inventoryManager, ResourceName coinPrefab)
	{
		if (!inventoryManager)
			return 0;

		array<IEntity> items = {};
		inventoryManager.GetItems(items);

		int count = 0;
		foreach (IEntity item : items)
		{
			if (item.GetPrefabData().GetPrefabName() == coinPrefab)
			{
				count++;
			}
		}

		return count;
	}

	// Count all coins in inventory
	protected void CountAllCoins(SCR_InventoryStorageManagerComponent inventoryManager)
	{
		if (!inventoryManager)
			return;

		array<IEntity> items = {};
		inventoryManager.GetItems(items);

		m_iCachedGoldCount = 0;
		m_iCachedSilverCount = 0;
		m_iCachedCopperCount = 0;

		foreach (IEntity item : items)
		{
			ResourceName prefabName = item.GetPrefabData().GetPrefabName();

			if (prefabName == GOLD_COIN_PREFAB)
				m_iCachedGoldCount++;
			else if (prefabName == SILVER_COIN_PREFAB)
				m_iCachedSilverCount++;
			else if (prefabName == COPPER_COIN_PREFAB)
				m_iCachedCopperCount++;
		}
	}

	// Override UpdateMoneyText to show all coin counts
	override protected void UpdateMoneyText()
	{
		if (!m_wMoneyText)
			return;

		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!playerController)
			return;

		IEntity player = playerController.GetMainEntity();
		if (!player)
			return;

		SCR_InventoryStorageManagerComponent inventoryManager = SCR_InventoryStorageManagerComponent.Cast(player.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!inventoryManager)
			return;

		float time = System.GetTickCount();
		if (time - m_fCachedMoneyTime > 1000)
		{
			// Count all coin types
			CountAllCoins(inventoryManager);
			m_fCachedMoneyTime = time;
		}

		// Format: "Gold: X | Silver: Y | Copper: Z"
		string money = string.Format("Gold: %1 | Silver: %2 | Copper: %3", m_iCachedGoldCount, m_iCachedSilverCount, m_iCachedCopperCount);
		m_wMoneyText.SetText(money);
	}
	
	

	
	override protected void CreateCartListboxItem(SCR_ListBoxComponent listbox, ADM_ShopMerchandise merch, bool buyOrSell = true)
	{
		ADM_MerchandiseType merchType = merch.GetType();
		if (!merchType)
			return;
		
		// Item Name
		string itemName = merchType.GetDisplayName();
		if (!itemName)
			itemName = "Item";
			
		int itemIdx = listbox.AddItem(itemName);
		SCR_ListBoxElementComponent lbItem = listbox.GetElementComponent(itemIdx);
		if (!lbItem)
			return;
			
		// Item Preview
		ItemPreviewWidget wItemPreview = ItemPreviewWidget.Cast(lbItem.GetRootWidget().FindAnyWidget("ItemPreview0"));
		if (wItemPreview)
		{
			m_ItemPreviewManager.SetPreviewItemFromPrefab(wItemPreview, merchType.GetDisplayEntity(), null, false);
		}
			
		// Cost
		TextWidget wItemPrice = TextWidget.Cast(lbItem.GetRootWidget().FindAnyWidget("Cost"));
		array<ref ADM_PaymentMethodBase> paymentRequirement = merch.GetBuyPayment();
		bool isCurrencyOnly = ADM_ShopComponent.IsBuyPaymentOnlyCurrency(merch);
		if (!buyOrSell)
		{
			paymentRequirement = merch.GetSellPayment();
			isCurrencyOnly = ADM_ShopComponent.IsSellPaymentOnlyCurrency(merch);
		}
		
		if (isCurrencyOnly || paymentRequirement.Count() == 0)
		{
			if (wItemPrice) {
				string posOrMinus = "+";
				Color textCol = Color.Green;
				if (buyOrSell)
				{
					textCol = Color.Red;
					posOrMinus = "-";
				}
				wItemPrice.SetColor(textCol);
				
				wItemPrice.SetVisible(true);
				if (paymentRequirement.Count() == 0)
				{
					wItemPrice.SetColor(Color.Green);
					wItemPrice.SetText("Free");
				} else {
					ADM_PaymentMethodCurrency paymentMethod = ADM_PaymentMethodCurrency.Cast(paymentRequirement[0]);
					int quantity = paymentMethod.GetQuantity();
					if (quantity > 0)
					{
						wItemPrice.SetTextFormat("%1$%2", posOrMinus, quantity);
					} else {
						wItemPrice.SetColor(Color.White);
						wItemPrice.SetText("Free");
					}
				}
			}
		} else {
			Widget priceParent = lbItem.GetRootWidget().FindAnyWidget("CostOverlay");
			if (wItemPrice)
			{ 
				wItemPrice.SetVisible(false);
			}
			
			for (int i = 0; i < paymentRequirement.Count(); i++)
			{
				Widget icon = GetGame().GetWorkspace().CreateWidgets(m_BarterIconPrefab, priceParent);
				ADM_IconBarterTooltip barterItemIcon = ADM_IconBarterTooltip.Cast(icon.FindHandler(ADM_IconBarterTooltip));
				if (barterItemIcon)
				{
					Color texColor = Color.White;
					if (!buyOrSell)
					{
						texColor = Color.Green;
					}
					
					barterItemIcon.SetPayment(paymentRequirement[i], texColor);
				}
			}
		}
		
		// Quantity
		ADM_ShopUI_Item item = ADM_ShopUI_Item.Cast(lbItem.GetRootWidget().FindHandler(ADM_ShopUI_Item));
		if (item) {
			int quantity = GetCartQuantity(merch, buyOrSell);
			item.SetShopUI(this);
			item.SetCartItem(true);
			item.SetMerchandise(merch);
			item.SetBuyOrSell(buyOrSell);
			item.SetQuantity(quantity);

			// Set max quantity - for sell items, limit to what player has
			int maxQty = merch.GetType().GetMaxPurchaseQuantity();
			if (!buyOrSell)
			{
				int playerHas = GetPlayerItemCount(merch);
				maxQty = playerHas;
				if (maxQty < 1)
					maxQty = 1;
			}
			item.SetMaxQuantity(maxQty);

			item.m_OnClicked.Insert(RemoveFromCart);
		}
	}
	
	override protected void PopulateCartTab(SCR_TabViewContent wTabView)
	{
		Widget wListboxBuy = wTabView.m_wTab.FindAnyWidget("ListBox0");
		if (!wListboxBuy)
			return;
		
		SCR_ListBoxComponent listboxBuy = SCR_ListBoxComponent.Cast(wListboxBuy.FindHandler(SCR_ListBoxComponent));
		if (!listboxBuy)
			return;
		
		Widget wListboxSell = wTabView.m_wTab.FindAnyWidget("ListBox1");
		if (!wListboxSell)
			return;
		
		SCR_ListBoxComponent listboxSell = SCR_ListBoxComponent.Cast(wListboxSell.FindHandler(SCR_ListBoxComponent));
		if (!listboxSell)
			return;
		
		ClearTab(wTabView);
		
		TextWidget buyText = TextWidget.Cast(wTabView.m_wTab.FindAnyWidget("Text0"));
		if (m_BuyShoppingCart.Count() > 0)
		{
			buyText.SetVisible(true);
		} else {
			buyText.SetVisible(false);
		}
		
		TextWidget sellText = TextWidget.Cast(wTabView.m_wTab.FindAnyWidget("Text1"));
		if (m_SellShoppingCart.Count() > 0)
		{
			sellText.SetVisible(true);
		} else {
			sellText.SetVisible(false);
		}
		
		array<ref ADM_PaymentMethodBase> totalCost = {};
		foreach (ADM_ShopMerchandise merch, int quantity : m_BuyShoppingCart)
		{
			if (!merch)
				continue;
				
			ADM_MerchandiseType merchType = merch.GetType();
			if (!merchType)
				continue;
			
			CreateCartListboxItem(listboxBuy, merch, true);
			
			foreach(ADM_PaymentMethodBase paymentMethod : merch.GetBuyPayment())
			{
				bool didAddToTotal = false;
				foreach(ADM_PaymentMethodBase existing : totalCost)
				{
					if (didAddToTotal)
					{
						continue;
					}
					
					bool isSame = existing.Equals(paymentMethod);
					didAddToTotal = existing.Add(paymentMethod, quantity);
				}
				
				if (!didAddToTotal)
				{
					ADM_PaymentMethodBase clone = ADM_PaymentMethodBase.Cast(paymentMethod.Clone());
					int idx = totalCost.Insert(clone);
					clone.Add(paymentMethod, quantity-1);
				}
			}
		}
		
		foreach (ADM_ShopMerchandise merch, int quantity : m_SellShoppingCart)
		{
			if (!merch)
				continue;
			
			ADM_MerchandiseType merchType = merch.GetType();
			if (!merchType)
				continue;
			
			CreateCartListboxItem(listboxSell, merch, false);
			
			foreach(ADM_PaymentMethodBase paymentMethod : merch.GetSellPayment())
			{
				bool didAddToTotal = false;
				foreach(ADM_PaymentMethodBase existing : totalCost)
				{
					if (didAddToTotal)
					{
						continue;
					}
					
					bool isSame = existing.Equals(paymentMethod);
					didAddToTotal = existing.Add(paymentMethod, quantity*-1);
				}
				
				if (!didAddToTotal)
				{
					ADM_PaymentMethodBase clone = ADM_PaymentMethodBase.Cast(paymentMethod.Clone());
					int idx = totalCost.Insert(clone);
					clone.Add(paymentMethod, (quantity+1)*-1);
				}
			}
		}
		
		Widget totalCostOverlay = wTabView.m_wTab.FindAnyWidget("TotalCostOverlay");
		Widget barterParent = totalCostOverlay.FindAnyWidget("BarterItemContainer");
		TextWidget wItemPrice = TextWidget.Cast(totalCostOverlay.FindAnyWidget("TotalCostCurrencyText"));
		if ((totalCost.Count() == 1 && totalCost[0].Type().IsInherited(ADM_PaymentMethodCurrency)) || totalCost.Count() == 0)
		{
			if (barterParent)
			{
				barterParent.GetParent().SetVisible(false);
			}
			
			if (wItemPrice) {
				wItemPrice.SetColor(Color.White);
				wItemPrice.SetVisible(true);
				if (totalCost.Count() == 0)
				{
					wItemPrice.SetTextFormat("Free");
				} else {
					ADM_PaymentMethodCurrency paymentMethod = ADM_PaymentMethodCurrency.Cast(totalCost[0]);
					int quantity = paymentMethod.GetQuantity();
					if (Math.AbsInt(quantity) > 0)
					{
						wItemPrice.SetTextFormat("$%1", Math.AbsInt(quantity));
						
						if (quantity < 0)
						{
							wItemPrice.SetColor(Color.Green);
						}
					} else {
						wItemPrice.SetTextFormat("Free");
					}
				}
			}
		} else {
			if (wItemPrice)
			{ 
				wItemPrice.SetVisible(false);
			}
			
			if (barterParent)
			{
				barterParent.GetParent().SetVisible(true);
				SCR_WidgetHelper.RemoveAllChildren(barterParent);
				for (int i = 0; i < totalCost.Count(); i++)
				{
					Widget icon = GetGame().GetWorkspace().CreateWidgets(m_BarterIconPrefab, barterParent);
					ADM_IconBarterTooltip barterItemIcon = ADM_IconBarterTooltip.Cast(icon.FindHandler(ADM_IconBarterTooltip));
					if (barterItemIcon)
					{
						barterItemIcon.SetPayment(totalCost[i]);
					}
				}
			}
			
			
		}
		
		TextWidget emptyText = TextWidget.Cast(wTabView.m_wTab.FindAnyWidget("Empty Text"));
		if (m_BuyShoppingCart.Count() == 0 && m_SellShoppingCart.Count() == 0)
		{
			emptyText.SetVisible(true);
			emptyText.SetText("No items in shopping cart.");
			emptyText.Update();
		} else {
			emptyText.SetVisible(false);
			emptyText.Update();
		}
	}
	
	
	override protected void CreateListboxItem(SCR_ListBoxComponent listbox, ADM_ShopMerchandise merch, bool buyOrSell = true)
	{			
		ADM_MerchandiseType merchType = merch.GetType();
		if (!merchType)
			return;
		
		// Item Name
		string itemName = merchType.GetDisplayName();
		if (!itemName)
			itemName = "Item";
			
		int itemIdx = listbox.AddItem(itemName);
		SCR_ListBoxElementComponent lbItem = listbox.GetElementComponent(itemIdx);
		if (!lbItem)
			return;
			
		// Item Preview
		ItemPreviewWidget wItemPreview = ItemPreviewWidget.Cast(lbItem.GetRootWidget().FindAnyWidget("ItemPreview0"));
		if (wItemPreview)
		{
			m_ItemPreviewManager.SetPreviewItemFromPrefab(wItemPreview, merchType.GetDisplayEntity(), null, false);
		}
		
		// On click
		ADM_ShopUI_Item mainBtn = ADM_ShopUI_Item.Cast(lbItem.GetRootWidget().FindHandler(ADM_ShopUI_Item));
		if (mainBtn)
		{
			mainBtn.m_OnClicked.Insert(AddToCart);
		}
			
		// Cost
		TextWidget wItemPrice = TextWidget.Cast(lbItem.GetRootWidget().FindAnyWidget("Cost"));
		array<ref ADM_PaymentMethodBase> paymentRequirement = merch.GetBuyPayment();
		bool isOnlyCurrency = ADM_ShopComponent.IsBuyPaymentOnlyCurrency(merch);
		if (!buyOrSell)
		{
			paymentRequirement = merch.GetSellPayment(); 
			isOnlyCurrency = ADM_ShopComponent.IsSellPaymentOnlyCurrency(merch);
		}
		
		if (isOnlyCurrency || paymentRequirement.Count() == 0)
		{
			if (wItemPrice) {
				wItemPrice.SetVisible(true);
				if (paymentRequirement.Count() == 0)
				{
					wItemPrice.SetTextFormat("Free");
				} else {
					ADM_PaymentMethodCurrency paymentMethod = ADM_PaymentMethodCurrency.Cast(paymentRequirement[0]);
					int quantity = paymentMethod.GetQuantity();
					if (quantity > 0)
					{
						wItemPrice.SetTextFormat("$%1", quantity);
					} else {
						wItemPrice.SetTextFormat("Free");
					}
				}
			}
		} else {
			Widget priceParent = lbItem.GetRootWidget().FindAnyWidget("CostOverlay");
			if (wItemPrice)
			{ 
				wItemPrice.SetVisible(false);
			}
			
			for (int i = 0; i < paymentRequirement.Count(); i++)
			{
				Widget icon = GetGame().GetWorkspace().CreateWidgets(m_BarterIconPrefab, priceParent);
				ADM_IconBarterTooltip barterItemIcon = ADM_IconBarterTooltip.Cast(icon.FindHandler(ADM_IconBarterTooltip));
				if (barterItemIcon)
				{
					barterItemIcon.SetPayment(paymentRequirement[i]);
				}
			}
		}
		
		// Quantity
		ADM_ShopUI_Item item = ADM_ShopUI_Item.Cast(lbItem.GetRootWidget().FindHandler(ADM_ShopUI_Item));
		if (item) {
			item.SetShopUI(this);
			item.SetMerchandise(merch);
			item.SetBuyOrSell(buyOrSell);

			// Set max quantity - for sell items, limit to what player has
			int maxQty = merch.GetType().GetMaxPurchaseQuantity();
			if (!buyOrSell)
			{
				// Sell item - max is how many player has minus what's in cart
				int playerHas = GetPlayerItemCount(merch);
				int inCart = GetCartQuantity(merch, false);
				maxQty = playerHas - inCart;
				if (maxQty < 1)
					maxQty = 1;
			}
			item.SetMaxQuantity(maxQty);
			item.SetQuantity(1);
		}
	}

	// Get how many of a specific merchandise item the player has
	protected int GetPlayerItemCount(ADM_ShopMerchandise merch)
	{
		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!playerController)
			return 0;

		IEntity player = playerController.GetMainEntity();
		if (!player)
			return 0;

		SCR_InventoryStorageManagerComponent inventory = SCR_InventoryStorageManagerComponent.Cast(player.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!inventory)
			return 0;

		ADM_MerchandiseType merchType = merch.GetType();
		if (!merchType)
			return 0;

		ADM_MerchandisePrefab merchPrefab = ADM_MerchandisePrefab.Cast(merchType);
		if (!merchPrefab)
			return 0;

		ResourceName itemResource = merchPrefab.GetPrefab();
		if (itemResource.IsEmpty())
			return 0;

		array<IEntity> items = {};
		inventory.GetItems(items);

		int count = 0;
		foreach (IEntity item : items)
		{
			if (item.GetPrefabData().GetPrefabName() == itemResource)
				count++;
		}

		return count;
	}
	
	// Override Checkout to validate sell cart before processing
	override protected void Checkout(SCR_InputButtonComponent btn)
	{
		PlayerController playerController = GetGame().GetPlayerController();
		if (!playerController)
			return;

		ADM_PlayerShopManagerComponent playerShopManager = ADM_PlayerShopManagerComponent.Cast(playerController.FindComponent(ADM_PlayerShopManagerComponent));
		if (!playerShopManager)
			return;

		// Validate sell cart and remove items player no longer has
		ValidateSellCartAtCheckout();

		// Debug: Log sell cart contents before processing
		foreach (ADM_ShopMerchandise merch, int qty : m_SellShoppingCart)
		{
			ADM_MerchandiseType merchType = merch.GetType();
			string itemName = "Unknown";
			if (merchType)
				itemName = merchType.GetDisplayName();
			Print(string.Format("[Shop Checkout] Sell Cart Item: %1, Quantity: %2", itemName, qty), LogLevel.DEBUG);
		}

		// Check if cart is empty after validation
		if (m_BuyShoppingCart.Count() == 0 && m_SellShoppingCart.Count() == 0)
		{
			SCR_HintManagerComponent.ShowCustomHint("Cart is empty - nothing to process.");
			return;
		}

		playerShopManager.AskProcessCart(m_Shop, m_BuyShoppingCart, m_SellShoppingCart);

		Close();
	}

	// Validate sell cart items at checkout time
	protected void ValidateSellCartAtCheckout()
	{
		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!playerController)
			return;

		IEntity player = playerController.GetMainEntity();
		if (!player)
			return;

		SCR_InventoryStorageManagerComponent inventory = SCR_InventoryStorageManagerComponent.Cast(player.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!inventory)
			return;

		// Get all items in inventory once
		array<IEntity> inventoryItems = {};
		inventory.GetItems(inventoryItems);

		// Build a map of prefab -> count for quick lookup
		map<ResourceName, int> itemCounts = new map<ResourceName, int>();
		foreach (IEntity item : inventoryItems)
		{
			ResourceName prefabName = item.GetPrefabData().GetPrefabName();
			int currentCount = 0;
			if (itemCounts.Contains(prefabName))
				currentCount = itemCounts.Get(prefabName);
			itemCounts.Set(prefabName, currentCount + 1);
		}

		// Check each sell cart item and adjust quantities
		array<ADM_ShopMerchandise> itemsToRemove = {};

		foreach (ADM_ShopMerchandise merch, int cartQuantity : m_SellShoppingCart)
		{
			ADM_MerchandiseType merchType = merch.GetType();
			if (!merchType)
			{
				itemsToRemove.Insert(merch);
				continue;
			}

			ADM_MerchandisePrefab merchPrefab = ADM_MerchandisePrefab.Cast(merchType);
			if (!merchPrefab)
				continue; // Not a prefab item, skip validation

			ResourceName itemResource = merchPrefab.GetPrefab();
			if (itemResource.IsEmpty())
			{
				itemsToRemove.Insert(merch);
				continue;
			}

			// Get how many player has
			int playerHas = 0;
			if (itemCounts.Contains(itemResource))
				playerHas = itemCounts.Get(itemResource);

			if (playerHas == 0)
			{
				// Player has none of this item, remove from cart
				itemsToRemove.Insert(merch);
			}
			else if (cartQuantity > playerHas)
			{
				// Player has some but not enough, reduce quantity
				m_SellShoppingCart.Set(merch, playerHas);
				// Update item count to prevent double-counting for same item type
				itemCounts.Set(itemResource, 0);
			}
			else
			{
				// Player has enough, reduce available count for this prefab
				itemCounts.Set(itemResource, playerHas - cartQuantity);
			}
		}

		// Remove invalid items
		foreach (ADM_ShopMerchandise merch : itemsToRemove)
		{
			m_SellShoppingCart.Remove(merch);
		}
	}

	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_wRoot = w;

		m_wBuySellTabWidget = GetRootWidget().FindAnyWidget("BuySellTabView");
		if (!m_wBuySellTabWidget) 
			return;
		
		m_BuySellTabComponent = SCR_TabViewComponent.Cast(m_wBuySellTabWidget.FindHandler(SCR_TabViewComponent));
		if (!m_BuySellTabComponent) 
			return;
		
		if (m_BuySellTabComponent.GetTabCount() == 2)
		{
			m_wBuyTabView = m_BuySellTabComponent.GetEntryContent(0);
			m_wSellTabView = m_BuySellTabComponent.GetEntryContent(1);
		}
		
		m_wCartTabWidget = GetRootWidget().FindAnyWidget("CartTabView");
		if (!m_wCartTabWidget) 
			return;
		
		m_CartTabComponent = SCR_TabViewComponent.Cast(m_wCartTabWidget.FindHandler(SCR_TabViewComponent));
		if (!m_CartTabComponent) 
			return;
		
		if (m_CartTabComponent.GetTabCount() == 1)
		{
			m_wCartTabView = m_CartTabComponent.GetEntryContent(0);
		}	
		
		m_wHeaderText = TextWidget.Cast(m_wRoot.FindAnyWidget("HeaderText"));
		m_wMoneyText = TextWidget.Cast(m_wRoot.FindAnyWidget("TotalMoneyText"));
		
		Widget closeWidget = ButtonWidget.Cast(m_wRoot.FindAnyWidget("Back"));
		if (closeWidget)
		{
			SCR_InputButtonComponent closeButton = SCR_InputButtonComponent.Cast(closeWidget.FindHandler(SCR_InputButtonComponent));
			if (closeButton)
			{
				closeButton.m_OnActivated.Insert(Close);
			}
		}
		
		Widget checkoutWidget = ButtonWidget.Cast(m_wRoot.FindAnyWidget("Checkout"));
		if (checkoutWidget)
		{
			SCR_InputButtonComponent checkoutButton = SCR_InputButtonComponent.Cast(checkoutWidget.FindHandler(SCR_InputButtonComponent));
			if (checkoutButton)
			{
				checkoutButton.m_OnActivated.Insert(Checkout);
			}
		}
		
		ConfigureTabs();
		UpdateMoneyText();
		PopulateCartTab(m_wCartTabView);
		
		ChimeraWorld world = ChimeraWorld.CastFrom(GetGame().GetWorld());
		if (world)
		{
			m_ItemPreviewManager = world.GetItemPreviewManager();
		}
	}
}