/*
===========================================
Do not modify, this script is generated
===========================================
*/

/*!
\addtogroup Components_InventorySystem
\{


modded class InventoryItemComponent: GameComponent
{
	
	//! Override final transformation of dropped item, return true in case transformation should be applied
	override bool OverridePlacementTransform(IEntity caller, out vector computedTransform[4]){
		
		GetOwner().SetOrigin(caller.GetOrigin());
		SCR_EntityHelper.SnapToGround(GetOwner(),startOffset: "0 1 0",maxLength:30);
		GetOwner().GetWorldTransform(computedTransform);
		return true;
	}

}
*/
/*!
\}
*/
