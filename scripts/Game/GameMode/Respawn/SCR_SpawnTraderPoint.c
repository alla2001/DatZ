[EntityEditorProps(category: "GameScripted/GameMode", description: "Spawn point entity", visible: false)]
class SCR_SpawnTraderPointClass : SCR_PositionClass
{
	// [Attribute()]
	// protected ref SCR_UIInfo m_Info;

	// SCR_UIInfo GetInfo()
	// {
	// 	return m_Info;
	// }
}


//------------------------------------------------------------------------------------------------
//! Spawn point entity defines positions on which players can possibly spawn.
class SCR_SpawnTraderPoint : SCR_Position
{
	protected RplComponent m_RplComponent;

	[Attribute("0", desc: "Find empty position for spawning within given radius. When none is found, entity position will be used.")]
	protected float m_fSpawnRadius;
	
	[Attribute("0")]
	int m_AudioIndex;

	[Attribute("0")]
	protected bool m_bShowInDeployMapOnly;

	[Attribute("0", desc: "Use custom timer when deploying on this spawn point. Takes the remaining respawn time from SCR_TimedSpawnPointComponent")]
	protected bool m_bTimedSpawnPoint;

	protected SCR_UIInfo m_LinkedInfo;
	protected SCR_FactionAffiliationComponent m_FactionAffiliationComponent;

	[Attribute()]
	protected ref SCR_UIInfo m_Info;


	
	[Attribute("0", desc: "Allow usage of Spawn Positions in range")]
	protected bool m_bUseNearbySpawnPositions;

	[Attribute("100", desc: "Spawn position detection radius, in metres")]
	protected float m_fSpawnPositionUsageRange;

	[Attribute("0", desc: "Additional respawn time (in seconds) when spawning on this spawn point"), RplProp()]
	protected float m_fRespawnTime;

	// List of all spawn points
	private static ref array<SCR_SpawnTraderPoint> m_aSpawnPoints = new array<SCR_SpawnTraderPoint>();


	static ref SCR_SpawnPointFinalizeSpawn_Invoker s_OnSpawnPointFinalizeSpawn;

	[RplProp()]
	protected string m_sSpawnPointName;

	IEntity mission;
	protected ref ScriptInvokerBool  m_OnSetSpawnPointEnabled;
	
	[Attribute("1"), RplProp(onRplName: "OnSetEnabled")]
	protected bool m_bSpawnPointEnabled;
	
	
	//------------------------------------------------------------------------------------------------
	bool IsSpawnPointVisibleForPlayer(int pid)
	{
		return true;
	}

	//------------------------------------------------------------------------------------------------
	bool IsSpawnPointEnabled()
	{
		return m_bSpawnPointEnabled;
	}
	
	
	//------------------------------------------------------------------------------------------------
	bool IsSpawnPointClear()
	{
		return mission==null;
	}
	
	//------------------------------------------------------------------------------------------------
	void SetSpawnPointEnabled_S(bool enabled)
	{
		if (enabled == m_bSpawnPointEnabled)
			return;
		
		m_bSpawnPointEnabled = enabled;
		OnSetEnabled();
		Replication.BumpMe();
	}
	
	//------------------------------------------------------------------------------------------------
	protected void OnSetEnabled()
	{
		if (m_OnSetSpawnPointEnabled)
			m_OnSetSpawnPointEnabled.Invoke(m_bSpawnPointEnabled);
	}
	
	//------------------------------------------------------------------------------------------------
	ScriptInvokerBool GetOnSpawnPointEnabled()
	{
		if (!m_OnSetSpawnPointEnabled)
			m_OnSetSpawnPointEnabled = new ScriptInvokerBool();
		
		return m_OnSetSpawnPointEnabled;
	}
	
	//------------------------------------------------------------------------------------------------
	static SCR_SpawnPointFinalizeSpawn_Invoker GetOnSpawnPointFinalizeSpawn()
	{
		if (!s_OnSpawnPointFinalizeSpawn)
			s_OnSpawnPointFinalizeSpawn = new SCR_SpawnPointFinalizeSpawn_Invoker();
		
		return s_OnSpawnPointFinalizeSpawn;
	}

	//------------------------------------------------------------------------------------------------
	float GetRespawnTime()
	{
		return m_fRespawnTime;
	}

	//------------------------------------------------------------------------------------------------
	void SetRespawnTime(float time)
	{
		m_fRespawnTime = time;
		Replication.BumpMe();
	}
	


	void CloseHatch(){
		array<Managed> doors();
		FindComponentsInAllChildren(DoorComponent,this,0,3,3,doors);
		
		foreach(Managed door : doors){
		
			DoorComponent dr = DoorComponent.Cast( door);
			
			dr.SetControlValue(0,RplId.Invalid());
		}
		

	}
	void OpenHatch(){
		array<Managed> doors();
		FindComponentsInAllChildren(DoorComponent,this,0,3,3,doors);
		
		foreach(Managed door : doors){
		
			DoorComponent dr = DoorComponent.Cast( door);
			
			dr.SetControlValue(0,RplId.Invalid());
		}
		

	}
	//------------------------------------------------------------------------------------------------
	/*!
	\return Radius in which players can be spawned on empty position.
	*/
	float GetSpawnRadius()
	{
		return m_fSpawnRadius;
	}
	
	//------------------------------------------------------------------------------------------------
	//!
	void SetSpawnRadius(float radius)
	{
		m_fSpawnRadius = radius;
	}
	


	//------------------------------------------------------------------------------------------------
	/*!
		Returns RplId of this spawn point.
	*/
	RplId GetRplId()
	{
		if (!m_RplComponent)
			return RplId.Invalid();

		return m_RplComponent.Id();
	}


	//------------------------------------------------------------------------------------------------
	protected bool GetEmptyPositionAndRotationInRange(out vector pos, out vector rot)
	{
		SCR_SpawnPositionComponentManager spawnPosManagerComponent = SCR_SpawnPositionComponentManager.GetInstance();
		if (!spawnPosManagerComponent)
			return false;

		array<SCR_SpawnPositionComponent> positions = {};
		int count = spawnPosManagerComponent.GetSpawnPositionsInRange(GetOrigin(), m_fSpawnPositionUsageRange, positions);
		if (count < 0)
			return false;

		SCR_SpawnPositionComponent position;
		
		while (!positions.IsEmpty())
		{
			position = positions.GetRandomElement();

			if (position.IsFree())
			{
				pos = position.GetOwner().GetOrigin();
				rot = position.GetOwner().GetAngles();
				return true;
			}
			else
			{
				positions.RemoveItem(position);
			}
		}

		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	//!
	void SetUseNearbySpawnPositions(bool use)
	{
		m_bUseNearbySpawnPositions = use;
	}

	
	
	//------------------------------------------------------------------------------------------------
	bool IsSpawnPointActive()
	{
		return true;
	}

	//------------------------------------------------------------------------------------------------
	/*!
	\return Number of spawn points in the world
	*/
	static int CountSpawnPoints()
	{
		return m_aSpawnPoints.Count();
	}

	//------------------------------------------------------------------------------------------------
	bool GetVisibleInDeployMapOnly()
	{
		return m_bShowInDeployMapOnly;
	}
	
	//------------------------------------------------------------------------------------------------
	void SetVisibleInDeployMapOnly(bool visible)
	{
		m_bShowInDeployMapOnly = visible;
	}

	
	
	//------------------------------------------------------------------------------------------------
	void SetSpawnPositionRange(float range)
	{
		m_fSpawnPositionUsageRange = range;
	}

	//------------------------------------------------------------------------------------------------
	float GetSpawnPositionRange()
	{
		return m_fSpawnPositionUsageRange;
	}


	//------------------------------------------------------------------------------------------------
	
	//------------------------------------------------------------------------------------------------
	//! \param character determines which entity is being checked.
	//! \param range determines the max range.
	//! \return whether is in range or not.
	private bool GetIsInRange(ChimeraCharacter character, float range)
	{
		return character && vector.DistanceSq(character.GetOrigin(), GetOrigin()) < range*range;
	}

	//------------------------------------------------------------------------------------------------
	//! \return an array of all spawn point entities.
	static array<SCR_SpawnTraderPoint> GetSpawnPoints()
	{
		return m_aSpawnPoints;
	}


	

	
	//------------------------------------------------------------------------------------------------
	// todo(koudelkaluk): get back to this

	//------------------------------------------------------------------------------------------------
	// SCR_UIInfo GetInfo()
	// {
	// 	if (m_LinkedInfo)
	// 		return m_LinkedInfo;
	// 	else
	// 	    return GetInfoFromPrefab();
	// }

	// protected SCR_UIInfo GetInfoFromPrefab()
	// {

		// SCR_SpawnPointClass prefabData = SCR_SpawnPointClass.Cast(GetPrefabData());
		// if (!prefabData)
		// 	return null;

		// return prefabData.GetInfo();
	// }

	//------------------------------------------------------------------------------------------------
	SCR_UIInfo GetInfo()
	{
		if (m_LinkedInfo)
			return m_LinkedInfo;
		else
			return m_Info;
	}
	
	//------------------------------------------------------------------------------------------------
	void SetInfo(SCR_UIInfo info)
	{
		m_Info = info;
	}

	//------------------------------------------------------------------------------------------------
	string GetSpawnPointName()
	{
		if (SCR_StringHelper.IsEmptyOrWhiteSpace(m_sSpawnPointName) && GetInfo())
			return GetInfo().GetName();
		else
			return m_sSpawnPointName;

		return string.Empty;
	}
	

	//------------------------------------------------------------------------------------------------
	bool IsTimed()
	{
		return m_bTimedSpawnPoint;
	}
	
	//------------------------------------------------------------------------------------------------
	void SetIsTimed(bool isTimed)
	{
		m_bTimedSpawnPoint = isTimed;
	}

	//------------------------------------------------------------------------------------------------
	void LinkInfo(SCR_UIInfo info)
	{
		m_LinkedInfo = info;
	}

	//------------------------------------------------------------------------------------------------
	/*
		Authority:
			During the spawn process, prior to passing the ownership to the remote project spawned entity
			can be prepared (e.g. moved to position, seated in vehicle, items spawned in inventory).

			This method is the place to do so, but at this point the spawning process can still fail and
			terminate if preparation fails (returns false). Player is then informed about spawn.

			Following a successful preparation is CanSpawnFinalize_S, and SpawnFinalize_S after which
			the process is sucessfully ended.

			\param requestComponent Player request component
			\param data Data received for this request
			\param entity Spawned entity (to prepare)

			\return True if successful (move to finalizing the request), false (to terminate the process).
	*/
	

	//------------------------------------------------------------------------------------------------
	/*!
		Authority:
			The PrepareEntity_S step might start doing an operation which is not performed immediately, for such cases
			we can await the finalization by returning 'false', until spawned entity is in desired state.
			(E.g. upon seating a character we can await until character is properly seated)

			Following a successful preparation is CanSpawnFinalize_S, and SpawnFinalize_S after which
			the process is sucessfully ended.

			\param requestComponent Player request component
			\param data Data received for this request
			\param entity Spawned entity (to await)

			\return True if ready, false (to await a frame).
	*/
	bool CanFinalizeSpawn_S(SCR_SpawnRequestComponent requestComponent, SCR_SpawnData data, IEntity entity)
	{
		return IsSpawnPointEnabled();
	}

	//------------------------------------------------------------------------------------------------
	/*!
		Authority:
			Callback for when finalization is done, e.g. the ownership is passed to the client and
			the spawn process is deemed complete.
	*/
	void OnFinalizeSpawnDone_S(SCR_SpawnRequestComponent requestComponent, SCR_SpawnData data, IEntity entity)
	{
		if (s_OnSpawnPointFinalizeSpawn)
			s_OnSpawnPointFinalizeSpawn.Invoke(requestComponent, data, entity);
	}

	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		if (!GetGame().GetWorldEntity())
			return;

		m_RplComponent = RplComponent.Cast(FindComponent(RplComponent));



		ClearFlags(EntityFlags.ACTIVE);
	}
	

	

	//------------------------------------------------------------------------------------------------
	void SCR_SpawnTraderPoint(IEntitySource src, IEntity parent)
	{
		SetEventMask(EntityEvent.INIT);
		SetFlags(EntityFlags.STATIC, true);
		m_aSpawnPoints.Insert(this);
	}

	//------------------------------------------------------------------------------------------------
	void ~SCR_SpawnTraderPoint()
	{
		// Remove from list of all points
		if (m_aSpawnPoints)
		{
			m_aSpawnPoints.RemoveItem(this);
		}


		
	}
}
