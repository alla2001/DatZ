// Script File
class CraftUserAction : ScriptedUserAction
{	
	
	DZCraftingSystem craftingSystem;
	bool shouldShow=false;
	CraftingRecipe recipe;
	//------------------------------------------------------------------------------------------------
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent) 
	{
		craftingSystem	= DZCraftingSystem.GetInstance();
		
		
	}
	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if(!recipe) return;
		if(!Replication.IsServer())return;
		m_Results=new array<IEntity>();
		array<IEntity> nearbyItems = {};
		vector center = pUserEntity.GetOrigin(); // or playerEntity.GetOrigin()
		float radius = 5.0; // Define the detection radius
		m_Results.Insert(GetOwner());
		GetGame().GetWorld().QueryEntitiesBySphere(
		center,
		radius,
		OnFound,
		OnFilter
		);
	
		if(!recipe.GetCanBeCraftedWith(m_Results)) return;
		
		array<IEntity> usedEnts();
		recipe.GetBestIngerdiants(m_Results,usedEnts);
		
	
		vector transform[4];
		IEntity owner = GetOwner();
		
		


		EntitySpawnParams params = new EntitySpawnParams();
		owner.GetWorldTransform(params.Transform);
		params.TransformMode = ETransformMode.WORLD;

	

		Resource resource = Resource.Load(recipe.resultItem);
		if (!resource.IsValid())
			return;

		IEntity ent = GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), params);
		
		SCR_EntityHelper.SnapToGround(ent,startOffset: "0 0.5 0",maxLength:30);
		foreach(IEntity entt : usedEnts){
		
			RplComponent.DeleteRplEntity(entt, false);
		}
		

		
	}

	override void OnActionCanceled(IEntity pOwnerEntity, IEntity pUserEntity)
	{
	
		
	}
	

	override void OnActionStart(IEntity pUserEntity) { 


		
	};

	//------------------------------------------------------------------------------------------------
	override bool CanBePerformedScript(IEntity user)
	{
		
		return true;
	}
	//------------------------------------------------------------------------------------------------
	override bool GetActionNameScript(out string outName)
	{
		if (!recipe)
			return false;
	
		outName = recipe.name;
		return true;
	}

	bool OnFound(IEntity ent)
	{
		m_Results.Insert(ent);
		return true; // continue processing
	}

	bool OnFilter(IEntity ent)
	{
		// Example: only accept items with a CraftingItem component
		return ent.FindComponent(CraftingItem) != null && ent != GetOwner();
	}
	ref array<IEntity> m_Results;
	override bool CanBeShownScript(IEntity user)
	{
		if(!recipe) return false;
		m_Results=new array<IEntity>();
		array<IEntity> nearbyItems = {};
		vector center = user.GetOrigin(); // or playerEntity.GetOrigin()
		float radius = 5.0; // Define the detection radius
		m_Results.Insert(GetOwner());
		GetGame().GetWorld().QueryEntitiesBySphere(
		center,
		radius,
		OnFound,
		OnFilter
		);
		
		SetActionDuration(recipe.durationToCraft);
	
		return recipe.GetCanBeCraftedWith(m_Results);
	}
	
};