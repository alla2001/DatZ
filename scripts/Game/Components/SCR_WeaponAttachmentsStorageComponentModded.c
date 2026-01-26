// Fix for NULL pointer crash in OnAddedToSlot when GetAttachmentType() returns null
modded class SCR_WeaponAttachmentsStorageComponent : WeaponAttachmentsStorageComponent
{
	override protected void OnAddedToSlot(IEntity item, int slotID)
	{
		m_OnItemAddedToSlotInvoker.Invoke(item, slotID);

		InventoryItemComponent inventoryItemComponent = InventoryItemComponent.Cast(item.FindComponent(InventoryItemComponent));
		if (!inventoryItemComponent)
			return;

		SCR_WeaponAttachmentObstructionAttributes obstructionAttributes = SCR_WeaponAttachmentObstructionAttributes.Cast(inventoryItemComponent.FindAttribute(SCR_WeaponAttachmentObstructionAttributes));
		if (!obstructionAttributes)
			return;

		// FIX: Check if GetAttachmentType returns null before calling Type()
		BaseAttachmentType attachmentTypeData = obstructionAttributes.GetAttachmentType();
		if (!attachmentTypeData)
			return;

		typename attachmentType = attachmentTypeData.Type();
		m_aActiveAttachmentTypes.Insert(attachmentType);
	}

	override protected void OnRemovedFromSlot(IEntity item, int slotID)
	{
		if (!item.IsDeleted())
			m_OnItemRemovedFromSlotInvoker.Invoke(item, slotID);

		InventoryItemComponent inventoryItemComponent = InventoryItemComponent.Cast(item.FindComponent(InventoryItemComponent));
		if (!inventoryItemComponent)
			return;

		SCR_WeaponAttachmentObstructionAttributes obstructionAttributes = SCR_WeaponAttachmentObstructionAttributes.Cast(inventoryItemComponent.FindAttribute(SCR_WeaponAttachmentObstructionAttributes));
		if (!obstructionAttributes)
			return;

		// FIX: Check if GetAttachmentType returns null before calling Type()
		BaseAttachmentType attachmentTypeData = obstructionAttributes.GetAttachmentType();
		if (!attachmentTypeData)
			return;

		typename attachmentType = attachmentTypeData.Type();
		m_aActiveAttachmentTypes.RemoveItem(attachmentType);

		RemoveNestedAttachments(attachmentType);
	}
}
