//------------------------------------------------------------------------------------------------
// DISABLE NAME TAGS - Hides player name tags above characters
//------------------------------------------------------------------------------------------------

modded class SCR_NameTagDisplay
{
	//------------------------------------------------------------------------------------------------
	// Override DisplayUpdate to do nothing - no name tags will render
	override void DisplayUpdate(IEntity owner, float timeSlice)
	{
		// Don't call super - skip all name tag rendering
	}
}
