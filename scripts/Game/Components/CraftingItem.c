[ComponentEditorProps(category: "Crafting", description: "Marks this item as usable in crafting.")]
class CraftingItemClass : ScriptComponentClass {}

class CraftingItem : ScriptComponent
{
	[Attribute("default", UIWidgets.EditBox, "Crafting ID", "Unique ID used in crafting recipes (e.g., Wood, Stone, IronIngot).")]
	string m_sCraftingID;

	// Optional: For more complex logic, you could store quantity or quality
	// [Attribute("1", UIWidgets.Slider, "Amount", "How many units this item provides.", "1 100 1")]
	// int m_iQuantity = 1;
	ActionsManagerComponent actionsManager;
	string GetCraftingID()
	{
		return m_sCraftingID;
	}

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);
		
		GetGame().GetCallqueue().CallLater(DelayInit,delay:1000);
	
	}
	void DelayInit(){
		
		IEntity owner = GetOwner();
		actionsManager= ActionsManagerComponent.Cast(owner.FindComponent(ActionsManagerComponent));
		
		if(!actionsManager) return;
			// Create context
	
		array<CraftingRecipe> recipes();
		DZCraftingSystem.GetInstance().GetRecipesForItem(GetOwner().GetPrefabData().GetPrefabName(),recipes);
		array<BaseUserAction> actions();
		actionsManager.GetActionsList(actions);
		for(int i = actions.Count() - 1; i >= 0; i--)
		{
			
			
			CraftUserAction act = CraftUserAction.Cast(actions[i]);
			if(!act)
			actions.Remove(i);
		
		}
		int y = 0;
		 
		foreach(CraftingRecipe recipe :recipes){
			
			CraftUserAction action = CraftUserAction.Cast(actions[y]);
			action.recipe = recipe;
			y++;
		}
	
	}

	override void EOnInit(IEntity owner)
	{
		// Optional debug print
		PrintFormat("CraftingItem initialized on %1 with ID %2", owner.GetName(), m_sCraftingID);
	}
}
