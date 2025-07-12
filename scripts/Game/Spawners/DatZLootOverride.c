class DatZLootOverrideClass : SCR_PositionClass
{


}


class DatZLootOverride : SCR_Position
{
	

    [Attribute()]
    float overrideRadius;
	
	[Attribute("0.9", UIWidgets.Slider, "Chance to override", "0 1 0.01")]
    float chance;


	[Attribute("", UIWidgets.ResourceNamePicker, desc: "", params: "conf")]
    ResourceName lootTable;

	override void EOnInit(IEntity owner)
	{
		
		
		GetGame().GetCallqueue().CallLater(SetOverride,delay:25000);
	}
	void SetOverride(){
	
	Print("Init Override");
	if(!DZLootSystem.GetInstance())
		return;
		DZLootSystem.GetInstance().SetLootOverrideSpawners(lootTable,GetOrigin(),overrideRadius,chance);
		
	 	
	
	}
	override void EOnFixedFrame(IEntity owner, float timeSlice){
	

	
	}
	
	override void  _WB_OnInit(inout vector mat[4], IEntitySource src){
	
		
	#ifdef WORKBENCH
		
			vector position;
			float radius;
			int color;
			
			position=GetOrigin();
			radius = overrideRadius;

			color = COLOR_GREEN_A;
			Shape.CreateSphere(color, ShapeFlags.TRANSP | ShapeFlags.NOOUTLINE, position, radius);
		
	#endif
	
	
	}

}