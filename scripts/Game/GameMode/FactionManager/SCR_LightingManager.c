
class SCR_LightingManagerClass : GenericEntityClass
{
}

class SCR_LightingManager: GenericEntity
{
	
	override void EOnInit(IEntity owner){
	
		delete GetGame().FindEntity("Lighting");
	}
}

