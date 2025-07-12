class DZMetabolisemSystem : GameSystem
{
	[Attribute("1000")]
	float processTime;

	

	
	override void OnInit()
	{
		GetGame().GetCallqueue().CallLater(ProcessPlayersMeta,processTime, false,repeat:true);
	
		
	}
	void ProcessPlayersMeta()
	{
		PlayerManager plyManager = GetGame().GetPlayerManager();
		if(!plyManager)return;
		array<int> players();
		plyManager.GetPlayers(players);
		
		foreach(int player : players){
		
			IEntity playerEntity =  plyManager.GetPlayerControlledEntity(player);
			if(!playerEntity)continue;
			
			DatZMetabolsimHandler meta = DatZMetabolsimHandler.Cast(playerEntity.FindComponent(DatZMetabolsimHandler));
			
			if(!meta)continue;
			
			meta.update();
				
		
		}
		
	}
	
	
	//------------------------------------------------------------------------------------------------
	override protected void OnUpdate(ESystemPoint point)
	{
		
	}
	

	
	//------------------------------------------------------------------------------------------------
	//! \param component must not be null
	void Register(SCR_CampaignMilitaryBaseComponent component)
	{
		
	}
	
	//------------------------------------------------------------------------------------------------
	void Unregister(SCR_CampaignMilitaryBaseComponent component)
	{
		
	}
}
