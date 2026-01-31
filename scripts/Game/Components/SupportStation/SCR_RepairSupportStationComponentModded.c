// Wrench repairs to 50% anywhere, repair station repairs to 100%
modded class SCR_RepairSupportStationComponent : SCR_BaseDamageHealSupportStationComponent
{
	//------------------------------------------------------------------------------------------------
	override protected float GetMaxHealScaled()
	{
		// Static repair stations (maintenance depot) can fully repair
		if (UsesRange())
			return 1.0;

		// Portable gadgets (wrench) cap at 80%
		return 0.8;
	}
}
