class DZCraftingSystem : GameSystem
{

	
	[Attribute()]
    ResourceName craftingCatalog;
	ref CraftingTable craftingTable;
	
	override void OnInit()
	{
	
		InitCraftingTable();
		
	}
	
	void GetRecipesForItem(ResourceName item, out array<CraftingRecipe> recipes)
	{
		
		foreach(CraftingRecipe recipe : craftingTable.recipes)
		{
			if(recipe.requiredItems.Contains(item) || recipe.barricading)
				recipes.Insert(recipe);
		}
	
	}
	
	void InitCraftingTable()
	{
	
		
		craftingTable = new CraftingTable();
	

		Resource resource = Resource.Load(craftingCatalog);
		if (!resource.IsValid())
			return ;

		craftingTable =  CraftingTable.Cast(BaseContainerTools.CreateInstanceFromContainer(resource.GetResource().ToBaseContainer()));

	}
	//------------------------------------------------------------------------------------------------
	static DZCraftingSystem GetInstance()
	{
		World world = GetGame().GetWorld();

		if (!world)
			return null;

		return DZCraftingSystem.Cast(world.FindSystem(DZCraftingSystem));
	}

	


}
