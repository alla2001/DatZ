// Filled circle map marker for loot zone visualization
// Circle radius scales with map zoom, text stays fixed size
class DatZMapZoneMarkerClass : SCR_MapMarkerEntityClass
{
}

class DatZMapZoneMarker : SCR_MapMarkerEntity
{
	[Attribute(defvalue: "100", desc: "Zone radius in meters", params: "0 inf 1"), RplProp()]
	float m_fRadius;

	[Attribute(defvalue: "1 0 0 1", desc: "Circle color")]
	ref Color m_CircleColor;

	[Attribute(defvalue: "", desc: "Zone label text"), RplProp()]
	string m_sZoneLabel;

	[Attribute(defvalue: "1.0", desc: "Text scale multiplier", params: "0.1 5 0.1"), RplProp()]
	float m_fTextScale;

	[Attribute(defvalue: "1 1 1 1", desc: "Text color")]
	ref Color m_TextColor;

	protected DatZMapZoneCircleHandler m_ZoneHandler;

	//------------------------------------------------------------------------------------------------
	// Set marker attributes and replicate to clients
	void SetMarkerData(float radius, string label, float textScale, Color circleColor, Color textColor)
	{
		m_fRadius = radius;
		m_sZoneLabel = label;
		m_fTextScale = textScale;
		m_CircleColor = circleColor;
		m_TextColor = textColor;
		SetText(label);
		Replication.BumpMe();
	}

	// Layout resource for the zone circle widget
	static const ResourceName ZONE_LAYOUT = "{E3327644B5DD8B27}layouts/Map/DatZZoneMarker.layout";

	//------------------------------------------------------------------------------------------------
	// Skip super.EOnInit entirely — it requires RplComponent and runs EOnFrame
	// for replication, which we don't need (static markers never move)
	protected override void EOnInit(IEntity owner)
	{
		// Set position once — zones never move
		m_vPos = GetOrigin();

		// Map UI only exists on clients, skip on dedicated server
		if (RplSession.Mode() == RplMode.Dedicated)
			return;

		// Register with marker manager (what super does, minus the rpl/frame stuff)
		SCR_MapMarkerManagerComponent markerMgr = SCR_MapMarkerManagerComponent.GetInstance();
		if (markerMgr)
			markerMgr.RegisterDynamicMarker(this);

		// Subscribe to map open event to create widget when map opens
		SCR_MapEntity.GetOnMapOpen().Insert(OnMapOpened);

		SetGlobalVisible(true);
		SetText(m_sZoneLabel);
	}

	//------------------------------------------------------------------------------------------------
	// Called when map opens - create our widget
	protected void OnMapOpened(MapConfiguration config)
	{
		// Create marker widget when map opens
		OnCreateMarker();
	}

	//------------------------------------------------------------------------------------------------
	// No-op: static marker, no per-frame position update needed
	override protected void EOnFrame(IEntity owner, float timeSlice)
	{
	}

	//------------------------------------------------------------------------------------------------
	// Override to create our own widget without needing marker config registration
	override void OnCreateMarker()
	{
		if (!IsVisible())
			return;

		// Don't create duplicate widgets
		if (m_wRoot)
			return;

		if (!m_MapEntity)
			m_MapEntity = SCR_MapEntity.GetMapInstance();

		if (!m_MapEntity)
			return;

		Widget mapFrame = m_MapEntity.GetMapMenuRoot();
		if (!mapFrame)
			return;

		Widget frame = mapFrame.FindAnyWidget(SCR_MapConstants.MAP_FRAME_NAME);
		if (!frame)
			return;

		m_wRoot = GetGame().GetWorkspace().CreateWidgets(ZONE_LAYOUT, frame);
		if (!m_wRoot)
			return;

		m_ZoneHandler = DatZMapZoneCircleHandler.Cast(m_wRoot.FindHandler(DatZMapZoneCircleHandler));
		if (m_ZoneHandler)
			m_ZoneHandler.SetMarkerEntity(this);

		SCR_MapEntity.GetOnMapClose().Insert(OnMapClosed);
	}

	//------------------------------------------------------------------------------------------------
	// Cleanup when marker is deleted
	void ~DatZMapZoneMarker()
	{
		SCR_MapEntity.GetOnMapOpen().Remove(OnMapOpened);
		SCR_MapEntity.GetOnMapClose().Remove(OnMapClosed);
	}

	//------------------------------------------------------------------------------------------------
	// Position update - circle follows world position on map
	override void OnUpdate()
	{
		if (!m_wRoot || !m_MapEntity)
			return;

		// Always read from entity origin — m_vPos [RplProp] won't replicate without RplComponent
		vector worldPos = GetOrigin();
		m_MapEntity.WorldToScreen(worldPos[0], worldPos[2], m_iScreenX, m_iScreenY, true);

		float posX = GetGame().GetWorkspace().DPIUnscale(m_iScreenX);
		float posY = GetGame().GetWorkspace().DPIUnscale(m_iScreenY);
		FrameSlot.SetPos(m_wRoot, posX, posY);
	}
}
