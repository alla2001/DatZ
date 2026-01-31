
//------------------------------------------------------------------------------------------------
modded class SCR_2DPIPSightsComponent : SCR_2DSightsComponent
{
	[RplProp()]
	bool Broken;

	void SetBroken(bool val)
	{
		Broken = val;
		Replication.BumpMe();
	}
}
