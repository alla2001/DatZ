// DatZ Sound Manager - Handles sound events for zombie hearing
// Broadcasts sounds to nearby zombies without creating hive mind behavior

[EntityEditorProps(category: "GameScripted/DatZ", description: "Sound manager for zombie hearing - add to GameMode")]
class DatZSoundManagerClass : SCR_BaseGameModeComponentClass
{
}

class DatZSoundManager : SCR_BaseGameModeComponent
{
	// Singleton
	protected static DatZSoundManager s_Instance;

	// Registered zombie listeners
	protected ref array<DatZUndeadBase> m_Listeners = {};

	// Spatial grid for efficient lookups
	protected ref map<int, ref array<DatZUndeadBase>> m_SpatialGrid = new map<int, ref array<DatZUndeadBase>>();
	protected static const float GRID_CELL_SIZE = 100.0;

	// Maximum hearing range (for grid queries)
	protected static const float MAX_HEARING_RANGE = 800.0;

	//------------------------------------------------------------------------------------------------
	static DatZSoundManager GetInstance()
	{
		return s_Instance;
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		s_Instance = this;

		if (Replication.IsServer())
		{
			// Periodically refresh spatial grid as zombies move around
			GetGame().GetCallqueue().CallLater(RefreshSpatialGrid, 2000, true);
		}
	}

	//------------------------------------------------------------------------------------------------
	void ~DatZSoundManager()
	{
		GetGame().GetCallqueue().Remove(RefreshSpatialGrid);

		if (s_Instance == this)
			s_Instance = null;
	}

	//------------------------------------------------------------------------------------------------
	void RegisterListener(DatZUndeadBase undead)
	{
		if (!undead || m_Listeners.Contains(undead))
			return;

		m_Listeners.Insert(undead);
		UpdateSpatialGrid(undead, true);
	}

	//------------------------------------------------------------------------------------------------
	void UnregisterListener(DatZUndeadBase undead)
	{
		if (!undead)
			return;

		int idx = m_Listeners.Find(undead);
		if (idx >= 0)
			m_Listeners.Remove(idx);

		UpdateSpatialGrid(undead, false);
	}

	//------------------------------------------------------------------------------------------------
	protected int GetGridCell(vector pos)
	{
		int x = Math.Floor(pos[0] / GRID_CELL_SIZE);
		int z = Math.Floor(pos[2] / GRID_CELL_SIZE);
		return x * 100000 + z;
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateSpatialGrid(DatZUndeadBase undead, bool adding)
	{
		int cell = GetGridCell(undead.GetOrigin());

		if (adding)
		{
			if (!m_SpatialGrid.Contains(cell))
				m_SpatialGrid.Set(cell, new array<DatZUndeadBase>());

			array<DatZUndeadBase> cellList = m_SpatialGrid.Get(cell);
			if (!cellList.Contains(undead))
				cellList.Insert(undead);
		}
		else
		{
			if (m_SpatialGrid.Contains(cell))
			{
				array<DatZUndeadBase> cellList = m_SpatialGrid.Get(cell);
				int idx = cellList.Find(undead);
				if (idx >= 0)
					cellList.Remove(idx);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	void RefreshSpatialGrid()
	{
		m_SpatialGrid.Clear();

		// Clean up dead/null zombies and rebuild grid
		for (int i = m_Listeners.Count() - 1; i >= 0; i--)
		{
			DatZUndeadBase undead = m_Listeners[i];
			if (!undead || !undead.IsAlive())
			{
				m_Listeners.Remove(i);
				continue;
			}

			UpdateSpatialGrid(undead, true);
		}
	}

	//------------------------------------------------------------------------------------------------
	// Main method to emit a sound event
	void EmitSound(vector position, int soundType, float loudness = 1.0)
	{
		if (!Replication.IsServer())
			return;

		array<DatZUndeadBase> nearbyZombies = GetListenersInRadius(position, MAX_HEARING_RANGE);

		foreach (DatZUndeadBase undead : nearbyZombies)
		{
			if (undead && undead.IsAlive())
				undead.OnHearSound(position, soundType, loudness);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected array<DatZUndeadBase> GetListenersInRadius(vector center, float radius)
	{
		array<DatZUndeadBase> result = {};

		int cellRadius = Math.Ceil(radius / GRID_CELL_SIZE);
		int centerCellX = Math.Floor(center[0] / GRID_CELL_SIZE);
		int centerCellZ = Math.Floor(center[2] / GRID_CELL_SIZE);

		for (int dx = -cellRadius; dx <= cellRadius; dx++)
		{
			for (int dz = -cellRadius; dz <= cellRadius; dz++)
			{
				int cell = (centerCellX + dx) * 100000 + (centerCellZ + dz);

				if (!m_SpatialGrid.Contains(cell))
					continue;

				array<DatZUndeadBase> cellList = m_SpatialGrid.Get(cell);
				foreach (DatZUndeadBase undead : cellList)
				{
					if (!undead)
						continue;

					float dist = vector.Distance(center, undead.GetOrigin());
					if (dist <= radius)
						result.Insert(undead);
				}
			}
		}

		return result;
	}

	//------------------------------------------------------------------------------------------------
	// Helper method to determine sound type from weapon
	static int GetWeaponSoundType(IEntity weapon, bool isSuppressed)
	{
		if (!weapon)
			return DatZSoundType.GUNSHOT_RIFLE;

		BaseWeaponComponent weaponComp = BaseWeaponComponent.Cast(weapon.FindComponent(BaseWeaponComponent));
		if (!weaponComp)
			return DatZSoundType.GUNSHOT_RIFLE;

		BaseMuzzleComponent muzzle = weaponComp.GetCurrentMuzzle();
		if (!muzzle)
		{
			if (isSuppressed)
				return DatZSoundType.GUNSHOT_RIFLE_SUPPRESSED;
			return DatZSoundType.GUNSHOT_RIFLE;
		}

		float caliber = 0;

		BaseMagazineComponent mag = muzzle.GetMagazine();
		if (mag)
		{
			int capacity = mag.GetMaxAmmoCount();
			if (capacity <= 10)
				caliber = 12.7;
			else if (capacity <= 20)
				caliber = 7.62;
			else if (capacity <= 40)
				caliber = 5.56;
			else
				caliber = 9;
		}

		// Determine sound type based on caliber
		if (caliber >= 12)
		{
			if (isSuppressed)
				return DatZSoundType.GUNSHOT_SNIPER_SUPPRESSED;
			return DatZSoundType.GUNSHOT_SNIPER;
		}
		else if (caliber >= 5)
		{
			if (isSuppressed)
				return DatZSoundType.GUNSHOT_RIFLE_SUPPRESSED;
			return DatZSoundType.GUNSHOT_RIFLE;
		}
		else
		{
			if (isSuppressed)
				return DatZSoundType.GUNSHOT_PISTOL_SUPPRESSED;
			return DatZSoundType.GUNSHOT_PISTOL;
		}
	}

	//------------------------------------------------------------------------------------------------
	// Helper to get footstep sound type from character movement state
	static int GetFootstepSoundType(ChimeraCharacter character)
	{
		if (!character)
			return DatZSoundType.FOOTSTEP_WALK;

		CharacterControllerComponent controller = character.GetCharacterController();
		if (!controller)
			return DatZSoundType.FOOTSTEP_WALK;

		ECharacterStance stance = controller.GetStance();

		if (stance == ECharacterStance.PRONE)
			return DatZSoundType.FOOTSTEP_PRONE;

		if (stance == ECharacterStance.CROUCH)
			return DatZSoundType.FOOTSTEP_CROUCH;

		float speed = controller.GetMovementSpeed();

		if (speed > 5.5)
			return DatZSoundType.FOOTSTEP_SPRINT;
		else if (speed > 3.5)
			return DatZSoundType.FOOTSTEP_RUN;

		return DatZSoundType.FOOTSTEP_WALK;
	}

	//------------------------------------------------------------------------------------------------
	int GetListenerCount()
	{
		return m_Listeners.Count();
	}
}
