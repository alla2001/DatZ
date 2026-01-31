// DatZ Undead Faction - Faction class for undead entities
// Non-playable hostile faction

/*[BaseContainerProps(), SCR_BaseContainerCustomTitleField("m_sName", "Undead")]
class DatZUndeadFaction : SCR_Faction
{
	//------------------------------------------------------------------------------------------------
	override void Init(IEntity owner)
	{
		super.Init(owner);

		// Force non-playable - players cannot join undead faction
		m_bIsPlayable = false;

		// Undead are friendly to each other (same faction)
		// This is handled by m_bFriendlyToSelf in parent class
	}

	//------------------------------------------------------------------------------------------------
	// Undead don't have ranks
	override string GetRankName(SCR_ECharacterRank rankID)
	{
		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	override string GetRankNameShort(SCR_ECharacterRank rankID)
	{
		return string.Empty;
	}
}*/
