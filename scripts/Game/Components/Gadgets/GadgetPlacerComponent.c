[ComponentEditorProps(category: "GameScripted/Gadget", description: "Handles gadget preview and placement")]
class GadgetPlacerComponentClass: SCR_GadgetComponentClass {}
class GadgetPlacerComponent: SCR_GadgetComponent
{
    protected IEntity m_PreviewEntity;
    protected ResourceName m_GadgetPrefab = "{YOURMOD}/Prefabs/Gadgets/MyGadget.et";
    protected ResourceName m_PreviewPrefab = "{YOURMOD}/Prefabs/Gadgets/MyGadgetPreview.et";
    protected bool m_IsPlacing = false;
	SCR_ItemPlacementComponent placement;
    override void OnPostInit(IEntity owner)
    {
        super.OnPostInit(owner);
        SetEventMask(owner, EntityEvent.FRAME);
		placement = SCR_ItemPlacementComponent.Cast( owner.FindComponent(SCR_ItemPlacementComponent));
    }
	//------------------------------------------------------------------------------------------------
	//! Set gadget mode 
	//! \param[in] mode is the target mode being switched to
	//! \param[in] charOwner
	override protected void ModeSwitch(EGadgetMode mode, IEntity charOwner)
	{
		UpdateVisibility(mode);
		if (mode == EGadgetMode.IN_HAND)
			placement.StartPreview();
		// if removing from inventory
		if (mode == EGadgetMode.ON_GROUND)
			m_CharacterOwner = null;
		else
			m_CharacterOwner = ChimeraCharacter.Cast(charOwner);
	}
}
