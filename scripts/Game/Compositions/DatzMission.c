//------------------------------------------------------------------------------------------------
// COMPONENT - Makes the airdrop fall slowly when attached to the airdrop prefab
//------------------------------------------------------------------------------------------------

[BaseContainerProps()]
class DatZMissionClass : ScriptComponentClass
{
}
//------------------------------------------------------------------------------------------------
class DatZMission : ScriptComponent
{
    
	ref SCR_MapMarkerBase m_MapMarker ;
    	[Attribute(desc: "Marker Type", category: "Map Marker")]
	protected ref SCR_ScenarioFrameworkMarkerType m_MapMarkerType;
	override void OnPostInit(IEntity owner){
	
	
		SetEventMask(owner,EntityEvent.INIT);
	}
		
    //------------------------------------------------------------------------------------------------
    override void EOnInit(IEntity owner)
    {
        super.EOnInit(owner);
        if(!Replication.IsServer())return;
        Print("AirDropFallComponent: Component initialized", LogLevel.NORMAL);
        CreateMapMarker();

		GetGame().GetCallqueue().CallLater(DestroyMission,delay:900000);
			
	
		
    }
    
	//------------------------------------------------------------------------------------------------
	//! Creates a map marker based on provided type, sets its position, custom text, and adds it to map marker manager if possible
	protected void CreateMapMarker()
	{
		SCR_MapMarkerManagerComponent mapMarkerMgr = SCR_MapMarkerManagerComponent.Cast(GetGame().GetGameMode().FindComponent(SCR_MapMarkerManagerComponent));
		if (!mapMarkerMgr)
			return;
		
		m_MapMarker = new SCR_MapMarkerBase();
		
		SCR_ScenarioFrameworkMarkerCustom mapMarkerCustom = SCR_ScenarioFrameworkMarkerCustom.Cast(m_MapMarkerType);
		if (mapMarkerCustom)
		{
			m_MapMarker.SetType(SCR_EMapMarkerType.PLACED_CUSTOM);
			m_MapMarker.SetIconEntry(mapMarkerCustom.m_eMapMarkerIcon);
			m_MapMarker.SetRotation(mapMarkerCustom.m_iMapMarkerRotation);
			m_MapMarker.SetColorEntry(mapMarkerCustom.m_eMapMarkerColor);
		}
		else
		{
			SCR_ScenarioFrameworkMarkerMilitary mapMarkerMilitary = SCR_ScenarioFrameworkMarkerMilitary.Cast(m_MapMarkerType);
			if (!mapMarkerMilitary)
				return;
			
			m_MapMarker = mapMarkerMgr.PrepareMilitaryMarker(mapMarkerMilitary.m_eMapMarkerFactionIcon, mapMarkerMilitary.m_eMapMarkerDimension, mapMarkerMilitary.m_eMapMarkerType1Modifier | mapMarkerMilitary.m_eMapMarkerType2Modifier);
		}
		
		vector worldPos = GetOwner().GetOrigin();
		m_MapMarker.SetWorldPos(worldPos[0], worldPos[2]);
		m_MapMarker.SetCustomText(m_MapMarkerType.m_sMapMarkerText);
		m_MapMarker.SetCanBeRemovedByOwner(true);
		
		FactionManager factionManager = GetGame().GetFactionManager();
	
		
		mapMarkerMgr.InsertStaticMarker(m_MapMarker, false, true);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Removes map marker by ID from map marker manager.
	void RemoveMapMarker()
	{
		if (!m_MapMarker)
			return;
		
		SCR_MapMarkerManagerComponent markerMgr = SCR_MapMarkerManagerComponent.GetInstance();
		if (!markerMgr)
			return;
		
		int markerID = m_MapMarker.GetMarkerID();
		
		SCR_MapMarkerBase marker = markerMgr.GetStaticMarkerByID(markerID);
		if (!marker)
			return;
		
		markerMgr.RemoveStaticMarker(marker);
		m_MapMarker = null;
	}
	
	
 	override void OnDelete(IEntity owner)
	{
		
	}
    
    //------------------------------------------------------------------------------------------------
    protected void DestroyMission()
    {
		
		
		RemoveMapMarker();
        IEntity owner = GetOwner();
        if (owner)
        {
            Print("AirDropFallComponent: Destroying airdrop", LogLevel.NORMAL);
            SCR_EntityHelper.DeleteEntityAndChildren(owner);
        }
    }
}