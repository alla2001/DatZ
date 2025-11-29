

modded class ADM_ShopUI: ChimeraMenuBase
{
	static const ResourceName m_BarterIconPrefab = "{BBEF0EBB9F35B19F}UI/Layouts/Menus/BasicShopMenu/MarketBarterItemIcon.layout";
	
	

	
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
			item.SetMaxQuantity(merch.GetType().GetMaxPurchaseQuantity());
			
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
			item.SetMaxQuantity(merch.GetType().GetMaxPurchaseQuantity());
			item.SetQuantity(1);
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