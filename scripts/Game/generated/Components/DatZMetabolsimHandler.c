sealed class DatZMetabolsimHandlerClass : ScriptComponentClass
{
}

class DatZMetabolsimHandler : ScriptComponent
{
    float food = 100.0;
    float water = 100.0;
    float stamina = 100.0; // ➡️ new stamina

    [Attribute()]
    float maxFood;

    [Attribute()]
     float maxWater;

    [Attribute()]
    private float foodDecayRate; // per minute

    [Attribute()]
    private float waterDecayRate; // per minute

    [Attribute()]
    private float staminaRegenRate = 10.0; // ➡️ stamina per second regen when not attacking

    [Attribute()]
     float staminaMeleeCost = 20.0; // ➡️ stamina lost per melee attack

    [Attribute(defvalue: "0.5")]
    private float starvationDamageRate; // HP/sec when food is 0

    [Attribute(defvalue: "0.75")]
    private float dehydrationDamageRate; // HP/sec when water is 0

    private HungerThirstHUD m_hud;
    private SCR_CharacterDamageManagerComponent m_damageManager;

	[RplProp()]
    bool m_bIsMeleeAttacking = false; // ➡️ flag if melee attack is happening
	
	BaseSlotComponent vomit;
	IEntity m_VomitEntity;
	bool canEat = true;
	
	[Attribute("true")]
	protected bool isZombi ;
	bool GetCanEat(){
		return canEat;
	}
    override void OnPostInit(IEntity owner)
    {
        super.OnPostInit(owner);
        GetGame().GetCallqueue().CallLater(init, 2000);
    }

    void init()
    {
        m_damageManager = SCR_CharacterDamageManagerComponent.Cast(GetOwner().FindComponent(SCR_CharacterDamageManagerComponent));
      
        SCR_HUDManagerComponent hud = GetGame().GetHUDManager();
        if (hud)
            m_hud = HungerThirstHUD.Cast(hud.FindInfoDisplay(HungerThirstHUD));
		
        if (!Replication.IsServer())
            return;
		
		
		array<Managed> slotComponents = {};
		GetOwner().FindComponents(BaseSlotComponent, slotComponents);

		foreach (Managed comp : slotComponents)
		{
   			BaseSlotComponent slotComp = BaseSlotComponent.Cast(comp);
    		if (!slotComp)
        		continue;

    		IEntity attachedEntity = slotComp.GetAttachedEntity();
    		if (attachedEntity && attachedEntity.GetPrefabData().GetPrefabName() == "{8B4313B6FEFDF063}Prefabs/Vomit.et")
    		{
       			 m_VomitEntity = attachedEntity;
        		 break;
    		}
		}
		

		
		if (m_VomitEntity)
			m_VomitEntity.ClearFlags(EntityFlags.VISIBLE, false);
    }

    void update()
    {
        if (isZombi) return;
        if (!Replication.IsServer()) return;

        food = Math.Max(0.0, food - foodDecayRate * 0.5 / 60.0);
        water = Math.Max(0.0, water - waterDecayRate * 0.5 / 60.0);

        // ➡️ stamina regeneration
        if (!m_bIsMeleeAttacking)
        {
            stamina = Math.Min(100.0, stamina + staminaRegenRate * 0.5);
        }
		// Enhanced healing based on food and water levels
    	float healMultiplier = 0.0;
		int hpStage = 0;
		if (m_damageManager)
		{
   			if (food <= 0.0)
			{
				hpStage = hpStage-1;
				ApplyDamage(starvationDamageRate * 0.5, EDamageType.MELEE);
			}
        		

   			if (water <= 0.0){
				hpStage = hpStage-1;
        		ApplyDamage(dehydrationDamageRate * 0.5, EDamageType.MELEE);
			}

   		

    		if (food > maxFood * 0.5 && water > maxWater * 0.5){
				healMultiplier = 1.0;
				hpStage=hpStage+1;
			}
        	
			if (food > maxFood * 0.7 && water > maxWater * 0.7){
				healMultiplier = 1.2;
				hpStage=hpStage+1;
			}
        	if (food > maxFood * 0.8 && water > maxWater * 0.8){
				healMultiplier = 1.5;
				hpStage=hpStage+1;
			}
   		


    		if (healMultiplier > 0.0)
        		HealPlayer(0.0001 * healMultiplier);
		}
        Rpc(UpdateOwner, food, water, stamina,hpStage);
    }

    void Eat(float amount)
    {
        Rpc(RPC_Eat,amount);
    }
	[RplRpc(RplChannel.Reliable,RplRcver.Server)]
	 void RPC_Eat(float amount)
    {
        food = Math.Min(maxFood, food + amount);
		Replication.BumpMe();
		
    }

    void Drink(float amount)
    {
        Rpc(RPC_Drink,amount);
    }
	[RplRpc(RplChannel.Reliable,RplRcver.Server)]
	void RPC_Drink(float amount)
    {
        water = Math.Min(maxWater, water + amount);
		Replication.BumpMe();
    }
    bool isThursty()
    {
        return water < maxWater;
    }

    bool isHungry()
    {
        return food < maxFood;
    }

    private void UpdateUI(int healthUp)
    {
        if (isZombi) return;
        if (!m_hud)
            return;
		
        if (GetGame().GetPlayerController().GetControlledEntity() != GetOwner())
            return;
		
        float currentHealth = 1;
        if (m_damageManager)
            currentHealth = m_damageManager.GetHealth();
		
        if (m_hud)
            m_hud.UpdateStats(food, water, currentHealth, stamina,healthUp); // ➡️ update stamina on HUD too
    }
	
	void Vomit(int strength)
	{
	

		
		food = Math.Max(0.0, food - 10 * strength);
        water = Math.Max(0.0, water - 10 * strength);
		canEat = false;
		RPCVomitAnim();
		Rpc(RPCVomitAnim);
	
		
		
	}
	[RplRpc(RplChannel.Reliable,RplRcver.Broadcast)]
	void RPCVomitAnim()
	{
	
		CharacterAnimationComponent animation = CharacterAnimationComponent.Cast( GetOwner().FindComponent(CharacterAnimationComponent));
		TAnimGraphCommand CMD = animation.BindCommand("CMD_Vomit");
		animation.CallCommand(CMD,1, 0);
		if (m_VomitEntity)
			m_VomitEntity.SetFlags(EntityFlags.VISIBLE, false);
		GetGame().GetCallqueue().CallLater(ResetAfterVomit,delay:4000);
	}
	void ResetAfterVomit()
	{
	
		canEat = true;
		if (m_VomitEntity)
			m_VomitEntity.ClearFlags(EntityFlags.VISIBLE, false);
	}
	void VomitWithDamage(int strength, float damage)
	{
	 	Vomit(strength);
		ApplyDamage(damage, EDamageType.MELEE);
	}
	
    [RplRpc(RplChannel.Reliable, RplRcver.Owner)]
    private void UpdateOwner(float valfood, float valwater, float valstamina, int healthUp)
    {
        food = valfood;
        water = valwater;
        stamina = valstamina;
        UpdateUI(healthUp);
    }

    private void ApplyDamage(float damageAmount, EDamageType damageType)
    {
        if (!m_damageManager)
            return;

        vector hitPosDirNorm[3];
        hitPosDirNorm[0] = vector.Zero;
        hitPosDirNorm[1] = vector.Zero;
        hitPosDirNorm[2] = vector.Zero;

        HitZone defaultHitZone = m_damageManager.GetDefaultHitZone();
        Instigator instigator = Instigator.CreateInstigator(null);

        SCR_DamageContext damageContext = new SCR_DamageContext(damageType, damageAmount, hitPosDirNorm, GetOwner(), defaultHitZone, instigator, null, -1, -1);

        m_damageManager.HandleDamage(damageContext);
    }

    private void HealPlayer(float healAmount)
    {
        if (!m_damageManager)
            return;

        float currentHealth = m_damageManager.GetHealthScaled();
        m_damageManager.SetHealthScaled(Math.Min(1.0, currentHealth + healAmount));
    }

    // ➡️ Public method to call when player performs a melee attack
    void OnMeleeAttack()
    {
		Rpc(RPCOnMeleeAttack);
        // Stop attack flag after a short time (simulate swing time)
  
    }
	 [RplRpc(RplChannel.Reliable, RplRcver.Server)]
    void RPCOnMeleeAttack()
    {
		float delay = 800; 
        if (stamina < staminaMeleeCost)
            return; // Not enough stamina
		
		if (isZombi) delay = 2000;
        m_bIsMeleeAttacking = true;
		GetGame().GetCallqueue().CallLater(ResetMeleeFlag, delay, false);
		if (isZombi)  return;
        stamina -= staminaMeleeCost;
        stamina = Math.Max(0.0, stamina);

		Replication.BumpMe();
        // Stop attack flag after a short time (simulate swing time)
  
    }

    private void ResetMeleeFlag()
    {
        m_bIsMeleeAttacking = false;
				Replication.BumpMe();
    }
}
