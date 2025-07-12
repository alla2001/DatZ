
// Consumable gadget component
modded class SCR_ConsumableItemComponent : SCR_GadgetComponent
{

	
    override void EOnInit(IEntity owner)
	{
	
		
		
		SCR_ConsumableFood food  =SCR_ConsumableFood.Cast( m_ConsumableEffect);
		
		if(!food)return;
		
		FluidContainerComponent fluid =FluidContainerComponent.Cast(owner.FindComponent(FluidContainerComponent));
		if(!fluid)return;
		food.fluidContainer = fluid;
	
	
	}
	
}
