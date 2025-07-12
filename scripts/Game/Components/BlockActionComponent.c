// HandsBlockingSimple.c
// Simplified working version for Arma Reforger
// This version focuses on core functionality that will compile
class MeleeBlockingClass:ScriptComponentClass{

}
class MeleeBlocking : ScriptComponent
{
    // Blocking state
     bool m_bIsBlocking = false;
    float m_fBlockingReduction = 0.5;
    
	SCR_CharacterAnimationComponent anim;
	TAnimGraphVariable blockBool;
	BaseWeaponManagerComponent wpn;
    //------------------------------------------------------------------------------------------------
    override void OnPostInit(IEntity owner)
    {
        super.OnPostInit(owner);
        
        // Enable frame events
        SetEventMask(owner, EntityEvent.FRAME);

		
		anim = SCR_CharacterAnimationComponent.Cast( owner.FindComponent(SCR_CharacterAnimationComponent));
		if(!anim) return;
		blockBool = anim.BindVariableBool("Blocking");
		wpn = BaseWeaponManagerComponent.Cast( owner.FindComponent(BaseWeaponManagerComponent));
		wpn.m_OnWeaponChangeStartedInvoker.Insert(OnWeaponChanged);
		GetGame().GetCallqueue().CallLater(DelayedInit,delay:2000)
    }
    
	void DelayedInit(){
	GetGame().GetInputManager().AddActionListener("CharacterWeaponRaised",EActionTrigger.DOWN,CheckBlockingState);
	}
    //------------------------------------------------------------------------------------------------
    override void EOnFrame(IEntity owner, float timeSlice)
    {
        super.EOnFrame(owner, timeSlice);
        
        // Simple input check - you'll need to bind this in the game's control settings
        // For now, we'll use a simpler approach
        if (GetGame().GetInputManager())
        {
            // Check if a specific key is pressed (this is a simplified check)
            // In practice, you'd need to set up proper input actions
            //CheckBlockingState();
        }
    }
    
    //------------------------------------------------------------------------------------------------
    protected void CheckBlockingState()
    {
		
		BaseWeaponComponent w =  wpn.GetCurrentWeapon() ;
		if(w){
				if( w.GetWeaponType() != EWeaponType.WT_HANDGUN) return;
		SCR_MeleeWeaponProperties melee = SCR_MeleeWeaponProperties.Cast( w.GetOwner().FindComponent(SCR_MeleeWeaponProperties));
		if(!melee.m_fIsKnife) return;
		}

      
		
			m_bIsBlocking = !m_bIsBlocking;
			SetBlocking(m_bIsBlocking);
		
    }
    	//------------------------------------------------------------------------------------------------
	protected void OnWeaponChanged(BaseWeaponComponent newWeaponSlot)
	{
		m_bIsBlocking = false;
		SetBlocking(m_bIsBlocking);
      
	}
    //------------------------------------------------------------------------------------------------
    void SetBlocking(bool blocking)
    {
        m_bIsBlocking = blocking;
		anim.SetVariableBool(blockBool,blocking);
        // Notify server if we're a client
        if (!Replication.IsServer() && Replication.IsRunning())
        {
            Rpc(RpcDo_SetBlockingServer, blocking);
        }
    }
    
    //------------------------------------------------------------------------------------------------
    bool IsBlocking()
    {
        return m_bIsBlocking;
    }
    
    //------------------------------------------------------------------------------------------------
    float GetBlockingReduction()
    {
        return m_fBlockingReduction;
    }
    
    //------------------------------------------------------------------------------------------------
    [RplRpc(RplChannel.Reliable, RplRcver.Server)]
    protected void RpcDo_SetBlockingServer(bool blocking)
    {
        m_bIsBlocking = blocking;
        
        // Broadcast to other clients
        Rpc(RpcDo_SetBlockingBroadcast, blocking);
    }
    
    //------------------------------------------------------------------------------------------------
    [RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
    protected void RpcDo_SetBlockingBroadcast(bool blocking)
    {
        m_bIsBlocking = blocking;
    }
}

