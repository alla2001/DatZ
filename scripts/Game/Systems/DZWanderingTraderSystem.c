class DZWanderingTraderClass : GenericEntityClass{

}

class DZWanderingTrader: GenericEntity
{


	[Attribute("30")]
    float traderTime;
	[Attribute()]
    ResourceName traderPrefab;
	
	IEntity currentTrader;
	
	SCR_SpawnTraderPoint lastPoint;
	
	GameSignalsManager m_GlobalSignalsManager;
	int CurrentLocationIndex;
	int switcherSignal; 
	bool newLocation;

		int audioIndex ;
	[RplProp()]
	int maxCount;
	
	[RplProp(onRplName: "OnSetEnabled")]
	int targetaudioIndex ;
	
	//------------------------------------------------------------------------------------------------
	void DZWanderingTrader(IEntitySource src, IEntity parent)
	{
		SetEventMask(EntityEvent.INIT);

	}
	
	override void EOnInit(IEntity owner)
	{
		if(Replication.IsServer())
		GetGame().GetCallqueue().CallLater(ProcessSpawn, 20000, false);

		m_GlobalSignalsManager = GetGame().GetSignalsManager();
		
		switcherSignal = m_GlobalSignalsManager.FindSignal("BroadcastType");
		audioIndex =  m_GlobalSignalsManager.FindSignal("TraderLocation");
		
	}
	void ProcessSpawn()
	{
		GetGame().GetCallqueue().CallLater(ProcessSpawn, traderTime*1000*60, false);
		if(currentTrader){
			
			SCR_EntityHelper.DeleteEntityAndChildren(currentTrader);

		
		}
		
		
		array<SCR_SpawnTraderPoint> sp();
		sp.Copy( SCR_SpawnTraderPoint.GetSpawnPoints());
		
		if(CurrentLocationIndex == sp.Count()-1)
		CurrentLocationIndex = 0;
		else
		CurrentLocationIndex++;
		
		lastPoint = sp[CurrentLocationIndex];
		EntitySpawnParams params();
		params.TransformMode= ETransformMode.WORLD;
		lastPoint.GetTransform(params.Transform);
		params.Parent=lastPoint;
		currentTrader =  GetGame().SpawnEntityPrefab(Resource.Load(traderPrefab),params:params);
		maxCount =sp.Count()-1;
			Replication.BumpMe();
		targetaudioIndex=  lastPoint.m_AudioIndex;
		float val = lastPoint.m_AudioIndex;
		m_GlobalSignalsManager.SetSignalValue(audioIndex,val);
		m_GlobalSignalsManager.SetSignalValue(switcherSignal,0.6);
		newLocation = true;
		GetGame().GetCallqueue().RemoveByName(this,"ResetAudio");
		GetGame().GetCallqueue().CallLater(startloop,delay:1000*60* Math.RandomInt(1,2));
		Replication.BumpMe();
		Rpc(RPC_Play);
	}
	void startloop()
	{
		newLocation = false;
		ResetAudio();
	}
	void ResetAudio()
	{
		if(newLocation) return;
		m_GlobalSignalsManager.SetSignalValue(audioIndex,lastPoint.m_AudioIndex);
		//m_GlobalSignalsManager.SetSignalValue(switcherSignal,0.0);
		GetGame().GetCallqueue().CallLater(PlayStore,delay:1000 * 60* Math.RandomInt(1,3));
		Rpc(RPC_Stop);
	}
	void PlayStore()
	{
		m_GlobalSignalsManager.SetSignalValue(audioIndex,lastPoint.m_AudioIndex);
		m_GlobalSignalsManager.SetSignalValue(switcherSignal,0.6);
		GetGame().GetCallqueue().CallLater(ResetAudio,delay:1000*60* Math.RandomInt(1,2));
		Rpc(RPC_Play);
	}
	//------------------------------------------------------------------------------------------------
	protected void OnSetEnabled()
	{
		m_GlobalSignalsManager.SetSignalValue(audioIndex,targetaudioIndex);
		m_GlobalSignalsManager.SetSignalValue(switcherSignal,0.6);
	}
	[RplRpc(RplChannel.Reliable,RplRcver.Broadcast)]
	void RPC_Play(){
	
		
		m_GlobalSignalsManager.SetSignalValue(audioIndex,targetaudioIndex);
		m_GlobalSignalsManager.SetSignalValue(switcherSignal,0.6);
	}
	[RplRpc(RplChannel.Reliable,RplRcver.Broadcast)]
	void RPC_Stop(){
	
		
		m_GlobalSignalsManager.SetSignalValue(audioIndex,targetaudioIndex);
		//m_GlobalSignalsManager.SetSignalValue(switcherSignal,0.0);
	}
	
	
	
}
