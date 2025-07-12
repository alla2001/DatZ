
class ForceNoInteractionAIClass: ScriptComponentClass {}
class ForceNoInteractionAI: ScriptComponent {

	
	ChimeraAIControlComponent AI;
	ActionsPerformerComponent actionPerformer;
	ClimbingDataComponent climb;
	
	ResourceName material = "{621C7F2EC2763297}Terrains/Common/Sky/Atmosphere/Atmosphere.emat";
	override void OnPostInit(IEntity owner){
		
		 
		
		
		SetEventMask(owner,EntityEvent.INIT);

		SetEventMask(owner,EntityEvent.FRAME);

		
	}
	override void EOnInit(IEntity owner)
	{
		
		AI = ChimeraAIControlComponent.Cast( owner.FindComponent(ChimeraAIControlComponent));
		actionPerformer = ActionsPerformerComponent.Cast( owner.FindComponent(ActionsPerformerComponent));
		climb= ClimbingDataComponent.Cast( owner.FindComponent(ClimbingDataComponent));
	}

	

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		/*GenericWorldEntity world = GetGame().GetWorldEntity();
		Material mat = world.GetSkyMaterial();
		
		mat = Material.GetMaterial(material);
		if(mat)
		mat.SetParamByIndex(ESkyMaterialParams.SKY_INTENSITY_LV, -12);*/
	
	}
	

}


