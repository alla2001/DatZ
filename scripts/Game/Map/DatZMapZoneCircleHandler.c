// Widget handler for filled circle zone markers on the map
// Circle scales with zoom (represents world-space radius), text stays fixed size
class DatZMapZoneCircleHandler : SCR_MapMarkerDynamicWComponent
{
	protected ImageWidget m_wCircleFill;
	protected ImageWidget m_wCircleOutline;
	protected TextWidget m_wZoneText;
	protected SCR_MapEntity m_MapEnt;
	protected DatZMapZoneMarker m_ZoneMarker;

	//------------------------------------------------------------------------------------------------
	void OnMapZoom(float ppu)
	{
		if (!m_ZoneMarker)
			return;

		UpdateCircleSize(ppu);
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateCircleSize(float ppu)
	{
		if (!m_wCircleFill || !m_ZoneMarker)
			return;

		// Convert world radius to screen pixels, then unscale for widget coords
		float screenDiameter = m_ZoneMarker.m_fRadius * 2.0 * ppu;
		float unscaledSize = GetGame().GetWorkspace().DPIUnscale(screenDiameter);

		// Clamp minimum size so it's always visible
		if (unscaledSize < 10)
			unscaledSize = 10;

		// Resize fill circle
		if (m_wCircleFill)
		{
			FrameSlot.SetSize(m_wCircleFill, unscaledSize, unscaledSize);
			m_wCircleFill.SetColor(m_ZoneMarker.m_CircleColor);
		}

		// Resize outline circle
		if (m_wCircleOutline)
		{
			FrameSlot.SetSize(m_wCircleOutline, unscaledSize, unscaledSize);
			m_wCircleOutline.SetColor(m_ZoneMarker.m_CircleColor);
		}
	}

	//------------------------------------------------------------------------------------------------
	override void SetMarkerEntity(notnull SCR_MapMarkerEntity marker)
	{
		m_ZoneMarker = DatZMapZoneMarker.Cast(marker);
		if (!m_ZoneMarker)
			return;

		// Set text, scale, and color
		if (m_wZoneText)
		{
			m_wZoneText.SetText(m_ZoneMarker.GetText());
			float fontSize = 14.0 * m_ZoneMarker.m_fTextScale;
			m_wZoneText.SetDesiredFontSize(fontSize);
			m_wZoneText.SetColor(m_ZoneMarker.m_TextColor);
		}

		// Initial size update
		if (m_MapEnt)
		{
			UpdateCircleSize(m_MapEnt.GetCurrentZoom());
			m_MapEnt.GetOnMapZoom().Insert(OnMapZoom);
		}
	}

	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);

		m_wCircleFill = ImageWidget.Cast(w.FindAnyWidget("CircleFill"));
		m_wCircleOutline = ImageWidget.Cast(w.FindAnyWidget("CircleOutline"));
		m_wZoneText = TextWidget.Cast(w.FindAnyWidget("ZoneText"));

		m_MapEnt = SCR_MapEntity.GetMapInstance();
	}

	//------------------------------------------------------------------------------------------------
	override void HandlerDeattached(Widget w)
	{
		super.HandlerDeattached(w);

		if (m_MapEnt)
			m_MapEnt.GetOnMapZoom().Remove(OnMapZoom);
	}
}
