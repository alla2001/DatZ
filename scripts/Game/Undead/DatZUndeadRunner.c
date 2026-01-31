// DatZ Undead Runner - Fast variant zombie
// Quick, aggressive, lower health

[EntityEditorProps(category: "GameScripted/DatZ", description: "Fast runner undead variant")]
class DatZUndeadRunnerClass : DatZUndeadBaseClass
{
}

class DatZUndeadRunner : DatZUndeadBase
{
	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		// Override default stats for runner variant
		m_bCanSprint = true;       // Can run
		m_fWalkSpeed = 22.0;       // Fast walk
		m_fSprintSpeed = 40.0;     // Very fast sprint
		m_fAttackDamage = 10;      // Lower damage
		m_fAttackRange = 1.2;      // Short reach
		m_fAttackCooldown = 0.8;   // Fast attacks
		m_fVisionRange = 60;       // Good vision range
		m_fStumbleChance = 0.9;    // Stumbles easily

		super.EOnInit(owner);
	}

	//------------------------------------------------------------------------------------------------
	override protected void PlayAggroSound()
	{
		// Screeching sound for runner
		// TODO: Add sound effect
	}

	//------------------------------------------------------------------------------------------------
	override protected void PlayAttackSound()
	{
		// Quick slash sound
		// TODO: Add sound effect
	}
}
