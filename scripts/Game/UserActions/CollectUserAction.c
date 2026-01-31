// CollectUserAction.c - Harvesting action that works WITHOUT RplComponent on the entity
class CollectUserAction : ScriptedUserAction
{
	[Attribute()]
	ResourceName resultItem;

	[Attribute("1")]
	int maxAmount;

	[Attribute("4")]
	float Duration;

	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		SetActionDuration(Duration);
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		// Only server spawns items
		if (!Replication.IsServer())
			return;

		DZHarvestManager manager = DZHarvestManager.GetInstance();
		if (!manager)
			return;

		vector pos = pOwnerEntity.GetOrigin();
		manager.DoHarvest(pos, resultItem, maxAmount, pUserEntity);
	}

	override bool CanBeShownScript(IEntity user)
	{
		DZHarvestManager manager = DZHarvestManager.GetInstance();
		if (!manager)
			return true;

		vector pos = GetOwner().GetOrigin();
		return manager.CanHarvest(pos);
	}

	override bool CanBePerformedScript(IEntity user)
	{
		return CanBeShownScript(user);
	}
}
