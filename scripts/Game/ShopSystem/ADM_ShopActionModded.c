modded class ADM_ShopAction : ScriptedUserAction
{
	
	
	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity) 
	{	
		if (!m_Shop || m_Shop.GetMerchandiseBuy().Count() <= 0) return;
		
		SCR_PlayerController scrPlayerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!scrPlayerController || pUserEntity != scrPlayerController.GetMainEntity()) 
			return;
		
		MenuBase menuBase = GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.ADM_ShopMenu); 
		m_Shop.shopUI = menuBase;
		ADM_ShopUI menu = ADM_ShopUI.Cast(menuBase);
		menu.SetShop(m_Shop);
	}
	
	
}