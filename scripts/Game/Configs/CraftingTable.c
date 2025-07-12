
[BaseContainerProps(configRoot: true)]
class CraftingRecipe : ScriptAndConfig
{
	
	[Attribute("3.0", UIWidgets.Slider, "Time to craft item in seconds", "0 20 0.01")]
	float durationToCraft;
	
	[Attribute( UIWidgets.ResourceNamePicker, params: "et")]
	ref array<ResourceName> requiredItems;
	
	[Attribute( UIWidgets.ResourceNamePicker, params: "et")]
    ResourceName resultItem;
	
	[Attribute()]
	string name;
	
	[Attribute()]
	bool barricading;
	
	bool GetCanBeCraftedWith(array<IEntity> ingrediants)
{
	array<ResourceName> tmpRequired();
	tmpRequired.Copy(requiredItems);
	
	foreach (IEntity ent : ingrediants)
	{
		ResourceName rec = ent.GetPrefabData().GetPrefabName();
		if (!rec) continue;

		int index = tmpRequired.Find(rec);
		if (index != -1)
			tmpRequired.Remove(index);

		if (tmpRequired.Count() == 0)
			return true;
	}
	return tmpRequired.Count() == 0;
}

	void GetBestIngerdiants(array<IEntity> ingrediants, out array<IEntity> best)
{
	array<ResourceName> tmpRequired();
	tmpRequired.Copy(requiredItems);
	
	foreach (IEntity ent : ingrediants)
	{
		ResourceName rec = ent.GetPrefabData().GetPrefabName();
		if (!rec) continue;

		int index = tmpRequired.Find(rec);
		if (index != -1)
		{
			best.Insert(ent);
			tmpRequired.Remove(index);
		}

		if (tmpRequired.Count() == 0)
			return;
	}
}
}

[BaseContainerProps(configRoot: true)]
class CraftingTable : ScriptAndConfig
{
	

	[Attribute()]
    ref array<ref CraftingRecipe> recipes;



}
