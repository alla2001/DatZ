[ComponentEditorProps(category: "GameScripted/Item", description: "Holds a fluid, allowing filling and draining.")]
class FluidContainerComponentClass : ScriptComponentClass
{
}

class FluidContainerComponent : ScriptComponent
{
	[RplProp(),Attribute("35.0")]
	float m_fMaxCapacity; // Maximum capacity in liters

	[RplProp()]
	float m_fCurrentAmount = 0.0; // Current amount of fluid
	InventoryItemComponent inventoryItemComp;
	
	override void OnPostInit(IEntity owner)
	{
	
		SetEventMask(owner,EntityEvent.INIT);
		
	}
	override void EOnInit(IEntity owner){
		
			if (SCR_Global.IsEditMode())
			return;
		
		inventoryItemComp = InventoryItemComponent.Cast(owner.FindComponent(InventoryItemComponent));
		SCR_InventoryUIInfo uiinfo =SCR_InventoryUIInfo.Cast(inventoryItemComp.GetUIInfo());
		if (!uiinfo)
			return;

		
	
	}
	// Fill the container
	void Fill(float amount)
	{
		m_fCurrentAmount = Math.Min(m_fCurrentAmount + amount, m_fMaxCapacity);
		Replication.BumpMe();
		
	}
	[RplRpc(RplChannel.Reliable,RplRcver.Server)]
	void RPC_Fill(float amount)
	{
		m_fCurrentAmount = Math.Min(m_fCurrentAmount + amount, m_fMaxCapacity);
		
		Replication.BumpMe();
		
	}
	// Drain from the container
	void Drain(float amount)
	{
		m_fCurrentAmount = Math.Max(m_fCurrentAmount - amount, 0);
		Replication.BumpMe();
	}
	
	[RplRpc(RplChannel.Reliable,RplRcver.Server)]
	void RPC_Drain(float amount)
	{
		m_fCurrentAmount = Math.Max(m_fCurrentAmount - amount, 0);
	
		Replication.BumpMe();
	}
	void UpdateUI()
	{
		

	
	}
	bool CanFill()
	{
		return 	m_fCurrentAmount <m_fMaxCapacity;
	}
}
