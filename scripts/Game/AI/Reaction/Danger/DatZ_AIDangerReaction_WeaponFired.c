// DatZ Modded Weapon Fire Reaction - Hooks into AI danger events to notify zombie hearing
// This mods the base game's weapon fire reaction to also alert zombies

modded class SCR_AIDangerReaction_WeaponFired : SCR_AIDangerReaction
{
	//------------------------------------------------------------------------------------------------
	override bool PerformReaction(notnull SCR_AIUtilityComponent utility, notnull SCR_AIThreatSystem threatSystem, AIDangerEvent dangerEvent, int dangerEventCount)
	{
		// Let the base game handle normal AI reaction
		bool baseResult = super.PerformReaction(utility, threatSystem, dangerEvent, dangerEventCount);

		// Also notify zombie sound system
		NotifyZombieSoundSystem(dangerEvent);

		return baseResult;
	}

	//------------------------------------------------------------------------------------------------
	protected void NotifyZombieSoundSystem(AIDangerEvent dangerEvent)
	{
		if (!Replication.IsServer())
			return;

		DatZSoundManager soundMgr = DatZSoundManager.GetInstance();
		if (!soundMgr)
			return;

		AIDangerEventWeaponFire eventWeaponFire = AIDangerEventWeaponFire.Cast(dangerEvent);
		if (!eventWeaponFire)
			return;

		vector shotPos = eventWeaponFire.GetPosition();
		bool isSuppressed = eventWeaponFire.IsSuppressed();

		// Determine sound type based on weapon characteristics
		int soundType = DetermineWeaponSoundType(eventWeaponFire, isSuppressed);

		// Emit to zombie sound system
		soundMgr.EmitSound(shotPos, soundType, 1.0);
	}

	//------------------------------------------------------------------------------------------------
	protected int DetermineWeaponSoundType(AIDangerEventWeaponFire eventWeaponFire, bool isSuppressed)
	{
		// Try to determine weapon type from the instigator
		IEntity instigator = eventWeaponFire.GetInstigatorEntity();
		if (!instigator)
		{
			if (isSuppressed)
				return DatZSoundType.GUNSHOT_RIFLE_SUPPRESSED;
			return DatZSoundType.GUNSHOT_RIFLE;
		}

		// Get the weapon from the character
		ChimeraCharacter character = ChimeraCharacter.Cast(instigator);
		if (!character)
		{
			if (isSuppressed)
				return DatZSoundType.GUNSHOT_RIFLE_SUPPRESSED;
			return DatZSoundType.GUNSHOT_RIFLE;
		}

		BaseWeaponManagerComponent weaponMgr = BaseWeaponManagerComponent.Cast(character.FindComponent(BaseWeaponManagerComponent));
		if (!weaponMgr)
		{
			if (isSuppressed)
				return DatZSoundType.GUNSHOT_RIFLE_SUPPRESSED;
			return DatZSoundType.GUNSHOT_RIFLE;
		}

		BaseWeaponComponent weapon = weaponMgr.GetCurrentWeapon();
		if (!weapon)
		{
			if (isSuppressed)
				return DatZSoundType.GUNSHOT_RIFLE_SUPPRESSED;
			return DatZSoundType.GUNSHOT_RIFLE;
		}

		// Try to identify weapon type from prefab name
		IEntity weaponEntity = weapon.GetOwner();
		if (weaponEntity)
		{
			EntityPrefabData prefabData = weaponEntity.GetPrefabData();
			if (prefabData)
			{
				string prefabName = prefabData.GetPrefabName();
				return GetSoundTypeFromPrefabName(prefabName, isSuppressed);
			}
		}

		// Fallback: try to identify by magazine capacity
		BaseMuzzleComponent muzzle = weapon.GetCurrentMuzzle();
		if (muzzle)
		{
			BaseMagazineComponent mag = muzzle.GetMagazine();
			if (mag)
			{
				int capacity = mag.GetMaxAmmoCount();
				return GetSoundTypeFromMagCapacity(capacity, isSuppressed);
			}
		}

		// Default to rifle
		if (isSuppressed)
			return DatZSoundType.GUNSHOT_RIFLE_SUPPRESSED;
		return DatZSoundType.GUNSHOT_RIFLE;
	}

	//------------------------------------------------------------------------------------------------
	protected int GetSoundTypeFromPrefabName(string prefabName, bool isSuppressed)
	{
		// Check for pistol patterns
		if (prefabName.Contains("Pistol") || prefabName.Contains("Handgun") ||
			prefabName.Contains("Glock") || prefabName.Contains("M9") ||
			prefabName.Contains("PM") || prefabName.Contains("CZ75"))
		{
			if (isSuppressed)
				return DatZSoundType.GUNSHOT_PISTOL_SUPPRESSED;
			return DatZSoundType.GUNSHOT_PISTOL;
		}

		// Check for shotgun patterns
		if (prefabName.Contains("Shotgun") || prefabName.Contains("Remington") ||
			prefabName.Contains("Benelli") || prefabName.Contains("M1014"))
		{
			if (isSuppressed)
				return DatZSoundType.GUNSHOT_SHOTGUN_SUPPRESSED;
			return DatZSoundType.GUNSHOT_SHOTGUN;
		}

		// Check for MG patterns
		if (prefabName.Contains("M249") || prefabName.Contains("PKM") ||
			prefabName.Contains("M60") || prefabName.Contains("MG") ||
			prefabName.Contains("RPK") || prefabName.Contains("SAW"))
		{
			if (isSuppressed)
				return DatZSoundType.GUNSHOT_MG_SUPPRESSED;
			return DatZSoundType.GUNSHOT_MG;
		}

		// Check for sniper patterns
		if (prefabName.Contains("Sniper") || prefabName.Contains("SVD") ||
			prefabName.Contains("M24") || prefabName.Contains("M82") ||
			prefabName.Contains("M40") || prefabName.Contains("Mosin") ||
			prefabName.Contains("DMR") || prefabName.Contains("SR25"))
		{
			if (isSuppressed)
				return DatZSoundType.GUNSHOT_SNIPER_SUPPRESSED;
			return DatZSoundType.GUNSHOT_SNIPER;
		}

		// Default to rifle (AK, M16, M4, etc.)
		if (isSuppressed)
			return DatZSoundType.GUNSHOT_RIFLE_SUPPRESSED;
		return DatZSoundType.GUNSHOT_RIFLE;
	}

	//------------------------------------------------------------------------------------------------
	protected int GetSoundTypeFromMagCapacity(int capacity, bool isSuppressed)
	{
		// Large capacity = LMG
		if (capacity > 50)
		{
			if (isSuppressed)
				return DatZSoundType.GUNSHOT_MG_SUPPRESSED;
			return DatZSoundType.GUNSHOT_MG;
		}

		// Small capacity = pistol or sniper
		if (capacity <= 10)
		{
			// Could be pistol or sniper, default to pistol for small mags
			if (isSuppressed)
				return DatZSoundType.GUNSHOT_PISTOL_SUPPRESSED;
			return DatZSoundType.GUNSHOT_PISTOL;
		}

		// Medium-small = probably pistol
		if (capacity <= 18)
		{
			if (isSuppressed)
				return DatZSoundType.GUNSHOT_PISTOL_SUPPRESSED;
			return DatZSoundType.GUNSHOT_PISTOL;
		}

		// Standard rifle capacity (20-30)
		if (isSuppressed)
			return DatZSoundType.GUNSHOT_RIFLE_SUPPRESSED;
		return DatZSoundType.GUNSHOT_RIFLE;
	}
}
