// Disable all Game Master voting (vote in, vote out, withdraw)
modded class SCR_VotingEditorIn : SCR_VotingReferendum
{
	override bool IsAvailable(int value, bool isOngoing)
	{
		return false;
	}
}

modded class SCR_VotingEditorOut : SCR_VotingEditorIn
{
	override bool IsAvailable(int value, bool isOngoing)
	{
		return false;
	}
}

modded class SCR_VotingEditorWithdraw : SCR_VotingEditorIn
{
	override bool IsAvailable(int value, bool isOngoing)
	{
		return false;
	}
}
