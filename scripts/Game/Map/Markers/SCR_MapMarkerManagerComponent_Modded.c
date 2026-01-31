// Fix null pointer crash in base game map marker manager
modded class SCR_MapMarkerManagerComponent
{
	//------------------------------------------------------------------------------------------------
	override void Update(float timeSlice)
	{
		// Null check to prevent crash when m_MapEntity isn't initialized yet
		if (!m_MapEntity)
			return;

		super.Update(timeSlice);
	}
}
