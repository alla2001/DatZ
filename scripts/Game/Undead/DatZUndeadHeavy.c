// DatZ Undead Heavy - Tank variant zombie
// Slow, tough, high damage

[EntityEditorProps(category: "GameScripted/DatZ", description: "Heavy/tank undead variant")]
class DatZUndeadHeavyClass : DatZUndeadBaseClass
{
}

class DatZUndeadHeavy : DatZUndeadBase
{
	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		// Override default stats for heavy variant
		m_bCanSprint = false;      // Cannot run
		m_fWalkSpeed = 2.5;        // Slower walk
		m_fSprintSpeed = 2.5;      // Same as walk
		m_fAttackDamage = 35;      // High damage
		m_fAttackRange = 2.0;      // Longer reach
		m_fAttackCooldown = 2.0;   // Slower attacks
		m_fVisionRange = 40;       // Shorter vision range
		m_fStumbleChance = 0.2;    // Rarely stumbles

		super.EOnInit(owner);
	}

	//------------------------------------------------------------------------------------------------
	override protected void PlayAggroSound()
	{
		// Deep growl for heavy variant
		// TODO: Add sound effect
	}

	//------------------------------------------------------------------------------------------------
	override protected void PlayAttackSound()
	{
		// Heavy slam sound
		// TODO: Add sound effect
	}
}
