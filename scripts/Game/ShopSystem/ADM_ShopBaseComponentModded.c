
modded class ADM_ShopBaseComponent: ScriptComponent
{
	MenuBase shopUI;
	override void OnDelete(IEntity owner)
	{
		if(shopUI)
		GetGame().GetMenuManager().CloseMenu(shopUI);
		
	
	
	}
	
}