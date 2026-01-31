// DatZ Undead Damage Handler - Mods the base damage manager for zombies
// Adds optional undead behavior via bool attribute

modded class SCR_CharacterDamageManagerComponent
{
	[Attribute("0", UIWidgets.CheckBox, "Enable undead damage behavior")]
	protected bool m_bIsUndead;

	[Attribute("0.5", UIWidgets.Slider, "Bullet damage multiplier (undead only)", "0.1 2 0.1")]
	protected float m_fUndeadBulletMult;

	[Attribute("0.3", UIWidgets.Slider, "Explosion damage multiplier (undead only)", "0.1 2 0.1")]
	protected float m_fUndeadExplosionMult;

	[Attribute("1.5", UIWidgets.Slider, "Fire damage multiplier (undead only)", "0.1 3 0.1")]
	protected float m_fUndeadFireMult;

	[Attribute("2.0", UIWidgets.Slider, "Headshot damage multiplier (undead only)", "1 5 0.5")]
	protected float m_fUndeadHeadshotMult;

	[Attribute("0.2", UIWidgets.Slider, "Chance to shrug off non-headshot (undead only)", "0 0.8 0.05")]
	protected float m_fUndeadShruggChance;

	[Attribute("60", UIWidgets.Slider, "Corpse cleanup time in seconds (undead only)", "10 300 10")]
	protected float m_fUndeadCorpseLifetime;

	//------------------------------------------------------------------------------------------------
	override bool HijackDamageHandling(notnull BaseDamageContext damageContext)
	{
		if (!m_bIsUndead)
			return super.HijackDamageHandling(damageContext);

		if (!Replication.IsServer())
			return super.HijackDamageHandling(damageContext);

		// Get hit zone info
		HitZone hitZone = damageContext.struckHitZone;
		bool isHeadshot = IsUndeadHeadshot(hitZone);

		// Chance to shrug off non-headshot damage
		if (!isHeadshot && Math.RandomFloat(0, 1) < m_fUndeadShruggChance)
		{
			// Notify for aggro even if damage blocked
			NotifyUndeadDamage(0, damageContext.damageType, damageContext.instigator);
			return true;
		}

		// Modify damage
		float modifier = GetUndeadDamageModifier(damageContext.damageType, isHeadshot);
		damageContext.damageValue = damageContext.damageValue * modifier;

		return super.HijackDamageHandling(damageContext);
	}

	//------------------------------------------------------------------------------------------------
	override event protected void OnDamage(notnull BaseDamageContext damageContext)
	{
		super.OnDamage(damageContext);

		if (!m_bIsUndead || !Replication.IsServer())
			return;

		NotifyUndeadDamage(damageContext.damageValue, damageContext.damageType, damageContext.instigator);
	}

	//------------------------------------------------------------------------------------------------
	override event protected void OnDamageStateChanged(EDamageState newState, EDamageState previousDamageState, bool isJIP)
	{
		super.OnDamageStateChanged(newState, previousDamageState, isJIP);

		if (!m_bIsUndead || !Replication.IsServer())
			return;

		if (newState == EDamageState.DESTROYED && previousDamageState != EDamageState.DESTROYED)
		{
			GetGame().GetCallqueue().CallLater(CleanupUndeadCorpse, m_fUndeadCorpseLifetime * 1000, false, GetOwner());
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void NotifyUndeadDamage(float damage, EDamageType type, Instigator instigator)
	{
		DatZUndeadBase undead = DatZUndeadBase.Cast(GetOwner());
		if (!undead)
			return;

		IEntity instigatorEnt = null;
		if (instigator)
			instigatorEnt = instigator.GetInstigatorEntity();

		undead.OnDamageReceived(damage, type, instigatorEnt);
	}

	//------------------------------------------------------------------------------------------------
	protected float GetUndeadDamageModifier(EDamageType type, bool isHeadshot)
	{
		float mult = 1.0;

		switch (type)
		{
			case EDamageType.KINETIC:
				mult = m_fUndeadBulletMult;
				break;
			case EDamageType.EXPLOSIVE:
				mult = m_fUndeadExplosionMult;
				break;
			case EDamageType.FIRE:
			case EDamageType.INCENDIARY:
				mult = m_fUndeadFireMult;
				break;
			case EDamageType.MELEE:
				mult = 0.8;
				break;
			default:
				mult = 1.0;
				break;
		}

		if (isHeadshot)
			mult = mult * m_fUndeadHeadshotMult;

		return mult;
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsUndeadHeadshot(HitZone hitZone)
	{
		if (!hitZone)
			return false;

		string name = hitZone.GetName();
		return name.Contains("Head") || name.Contains("Brain") || name.Contains("Skull");
	}

	//------------------------------------------------------------------------------------------------
	protected void CleanupUndeadCorpse(IEntity owner)
	{
		if (owner)
			SCR_EntityHelper.DeleteEntityAndChildren(owner);
	}
}
