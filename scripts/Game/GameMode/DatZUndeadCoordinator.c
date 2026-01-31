// DatZ Undead Coordinator - Global Management System
// Handles undead registration and spatial queries (NO hive mind / target sharing)

[EntityEditorProps(category: "GameScripted/DatZ", description: "Global undead coordinator - add to GameMode")]
class DatZUndeadCoordinatorClass : SCR_BaseGameModeComponentClass
{
}

class DatZUndeadCoordinator : SCR_BaseGameModeComponent
{
	// Singleton access
	protected static DatZUndeadCoordinator s_Instance;

	// Registered undead entities
	protected ref array<DatZUndeadBase> m_RegisteredUndead = {};

	// Spatial grid for fast proximity lookups
	protected ref map<int, ref array<DatZUndeadBase>> m_SpatialGrid = new map<int, ref array<DatZUndeadBase>>();
	protected static const float GRID_CELL_SIZE = 50.0;

	//------------------------------------------------------------------------------------------------
	static DatZUndeadCoordinator GetInstance()
	{
		return s_Instance;
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		s_Instance = this;

		// Periodically refresh spatial grid for moving zombies
		if (Replication.IsServer())
			GetGame().GetCallqueue().CallLater(RefreshSpatialGrid, 5000, true);
	}

	//------------------------------------------------------------------------------------------------
	void ~DatZUndeadCoordinator()
	{
		if (s_Instance == this)
			s_Instance = null;
	}

	//------------------------------------------------------------------------------------------------
	void RegisterUndead(DatZUndeadBase undead)
	{
		if (!undead || m_RegisteredUndead.Contains(undead))
			return;

		m_RegisteredUndead.Insert(undead);
		UpdateSpatialGrid(undead, true);
	}

	//------------------------------------------------------------------------------------------------
	void UnregisterUndead(DatZUndeadBase undead)
	{
		if (!undead)
			return;

		int idx = m_RegisteredUndead.Find(undead);
		if (idx >= 0)
			m_RegisteredUndead.Remove(idx);

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
	// Get all undead within a radius (for general queries, NOT for hive mind)
	array<DatZUndeadBase> GetUndeadInRadius(vector center, float radius)
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
	// Update spatial grid for moving undead
	void RefreshSpatialGrid()
	{
		m_SpatialGrid.Clear();

		foreach (DatZUndeadBase undead : m_RegisteredUndead)
		{
			if (undead && undead.IsAlive())
				UpdateSpatialGrid(undead, true);
		}
	}

	//------------------------------------------------------------------------------------------------
	int GetTotalUndeadCount()
	{
		return m_RegisteredUndead.Count();
	}

	//------------------------------------------------------------------------------------------------
	array<DatZUndeadBase> GetAllUndead()
	{
		return m_RegisteredUndead;
	}
}
