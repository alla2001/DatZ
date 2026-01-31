// Extend SCR_EArsenalItemType with custom DatZ item categories
// Base game uses bits 1-22, we add new types on bits 23+
modded enum SCR_EArsenalItemType
{
	FOOD = 1 << 23,
	INDUSTRIAL = 1 << 24,
	AMMO = 1 << 25,
}
