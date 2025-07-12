[ComponentEditorProps(category: "GameScripted/Door", description: "Locks a door and spawns a lock at the handle.")]
class DoorLockComponentClass : ScriptComponentClass
{
    [Attribute(defvalue: "0", uiwidget: UIWidgets.CheckBox, desc: "Should door always start locked (ignores random chance)?")]
    bool m_bForceLocked;
	
    [Attribute(desc: "Prefab to spawn for the lock (e.g., a padlock model)")]
    ResourceName m_sLockPrefab;
}

class DoorLockComponent : ScriptComponent
{
	[RplProp()]
    protected bool m_bIsLocked;
	
	[Attribute(defvalue: "0", uiwidget: UIWidgets.CheckBox, desc: "Should door always start locked (ignores random chance)?")]
    bool m_bCannotLock;

    protected IEntity m_LockEntity;

    protected DoorLockComponentClass GetDoorLockComponentClass()
    {
        return DoorLockComponentClass.Cast(GetComponentData(GetOwner()));
    }

    override void OnPostInit(IEntity owner)
    {
        super.OnPostInit(owner);
if(!Replication.IsServer())return;
        DoorLockComponentClass settings = GetDoorLockComponentClass();
        if (!settings)
            return;

        // --- RANDOM LOCK CHANCE ---
        if (settings.m_bForceLocked && !m_bCannotLock)
        {
            m_bIsLocked = true;
        }
        else if (!m_bCannotLock)
        {
            float roll = Math.RandomFloat01();
            if (roll <= 0.15) // 20% chance (1 in 5)
                m_bIsLocked = true;
            else
                m_bIsLocked = false;
        }

        if (m_bIsLocked)
           GetGame().GetCallqueue().CallLater(SpawnLock,delay:5000);
		GetGame().GetCallqueue().CallLater(CheckLock,delay:7000);
		Replication.BumpMe();
    }
	
	void CheckLock(){
		if(Replication.IsServer()&&m_LockEntity ==null&&m_bIsLocked){
		
			m_bIsLocked=false;
			Replication.BumpMe();
		}
	GetGame().GetCallqueue().CallLater(CheckLock,delay:500);
	
	}

    // Lock the door
    void Lock()
    {
        if (m_bIsLocked ||m_bCannotLock)
            return;

        m_bIsLocked = true;
        SpawnLock();
		Replication.BumpMe();
    }

    // Unlock the door
    void Unlock()
    {
        if (!m_bIsLocked)
            return;

        m_bIsLocked = false;

        if (m_LockEntity)
        {
			RplComponent.DeleteRplEntity(m_LockEntity, false);
          
            m_LockEntity = null;
        }
		Replication.BumpMe();
    }

    bool IsLocked()
    {
		
			if(Replication.IsServer()&&m_LockEntity ==null&&m_bIsLocked){
		
			m_bIsLocked=false;
			Replication.BumpMe();
		}
	
        return m_bIsLocked;
    }
	
    // Spawn the lock entity
    protected void SpawnLock()
    {
        DoorLockComponentClass settings = GetDoorLockComponentClass();
        if (!settings || !settings.m_sLockPrefab)
            return;

        Resource lockPrefab = Resource.Load(settings.m_sLockPrefab);
        if (!lockPrefab)
            return;

        vector handlePos[4];
		
		GetDoorHandlePosition(handlePos);
     

        EntitySpawnParams params = new EntitySpawnParams();
        params.TransformMode = ETransformMode.LOCAL;
        params.Transform = handlePos;
		params.Parent = GetOwner();
        m_LockEntity = GetGame().SpawnEntityPrefab(lockPrefab, GetGame().GetWorld(), params);
		m_LockEntity.SetLocalTransform(handlePos);
		m_LockEntity.Update();
	
      
    }

    // Internal: Estimate handle position
    protected void GetDoorHandlePosition(out vector mat[4])
    {
        IEntity owner = GetOwner();
        if (!owner)
            return;

        vector origin = owner.GetOrigin();
     
        vector up = vector.Up;
        ActionsManagerComponent actions =ActionsManagerComponent.Cast(owner.FindComponent(ActionsManagerComponent));
		if(!actions)return;
		array<UserActionContext> outContexts();
			actions.GetContextList(outContexts);
		
		outContexts[0].GetTransformationModel(mat);
   
    }

    // Handle lock getting destroyed
    override void OnDelete(IEntity owner)
    {
        if (owner == m_LockEntity)
        {
            m_LockEntity = null;
            m_bIsLocked = false;
            Print("Door unlocked because lock was destroyed!");
        }
    }
}
