

modded class SCR_MeleeComponent : ScriptComponent
{

	float m_fLastAttackTime;
float m_fAttackCooldown = 2; 
	//------------------------------------------------------------------------------------------------
	//! Primary collision check that uses TraceMove and single TraceSphere fired from character eye pos along the aim fwd vector
	//! \param[in] pHitData SCR_MeleeHitDataClass holder that keeps information about actual hit
	//! \return When hit is detected or it is miss
	override protected bool CheckCollisionsSimple(out SCR_MeleeHitDataClass pHitData)
	{
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(GetOwner());
		if (!character)
			return false;
		
		TraceSphere param = new TraceSphere();
		param.LayerMask = EPhysicsLayerDefs.Projectile;
		param.Flags = TraceFlags.ENTS | TraceFlags.WORLD;
		param.Exclude = character;
		
		param.Radius = m_fWeaponMeleeAccuracy;		
		param.Start = character.EyePosition();
		param.End = param.Start + GetAimingVector(character) * m_fWeaponMeleeRange;

		// Trace stops on the first hit entity if you want to change this, change the filter callback
		float hit = GetOwner().GetWorld().TraceMove(param, TraceFilter);

		if (!param.TraceEnt)
			return false;
		
		vector dir = (param.End - param.Start);
		
		pHitData.m_Entity = param.TraceEnt;
		pHitData.m_iColliderIndex = param.ColliderIndex;
		pHitData.m_vHitPosition = dir * hit + param.Start;
		pHitData.m_vHitDirection = dir.Normalized();
		pHitData.m_vHitNormal = param.TraceNorm;
		pHitData.m_SurfaceProps = param.SurfaceProps;
		pHitData.m_iNodeIndex = param.NodeIndex;
		
#ifdef SCR_MELEE_DEBUG_MSG
		MCDbgPrint("Check collision from: "+param.Start);
		MCDbgPrint("Check collision to: "+param.End);
		MCDbgPrint("Check collision range: " +vector.Distance(param.End, param.Start));
		MCDbgPrint("Check collision entity: "+pHitData.m_Entity.ToString());
		MCDbgPrint("Check collision hitzone: "+pHitData.m_sHitZoneName);
		MCDbgPrint("Check collision hitPos: "+pHitData.m_vHitPosition);
#endif
		
		return true;
	}

	// TODO: call this from SCR_CharacterControllerComponent.OnWeaponSelected() when possible and remove from PerformAttack()
	//------------------------------------------------------------------------------------------------
	//! Prepare information from MeleeWeaponProperties component on weapon
	//! \param[in] weapon Equipped melee weapon we are getting the data from
	override  protected void CollectMeleeWeaponProperties()
	{
		BaseWeaponManagerComponent wpnManager = BaseWeaponManagerComponent.Cast(GetOwner().FindComponent(BaseWeaponManagerComponent));
		if (!wpnManager)
			return;
		
		WeaponSlotComponent currentSlot = WeaponSlotComponent.Cast(wpnManager.GetCurrent());
		if (!currentSlot)
			return;
		
		//! get current weapon and store it into SCR_MeleeHitDataClass instance
		m_MeleeHitData.m_Weapon = currentSlot.GetWeaponEntity();

		IEntity weaponEntity = currentSlot.GetWeaponEntity();
		if (!weaponEntity)
			return;
		
		m_MeleeWeaponProperties = SCR_MeleeWeaponProperties.Cast(weaponEntity.FindComponent(SCR_MeleeWeaponProperties));
		if (!m_MeleeWeaponProperties)
			return;
		
		m_MeleeHitData.m_fDamage = m_MeleeWeaponProperties.GetWeaponDamage();
		m_fWeaponMeleeRange = m_MeleeWeaponProperties.GetWeaponRange();
		m_fWeaponMeleeAccuracy = m_MeleeWeaponProperties.GetWeaponMeleeAccuracy();
			
			ChimeraAIControlComponent AI = ChimeraAIControlComponent.Cast(GetOwner().FindComponent(ChimeraAIControlComponent));
		if (AI&&AI.IsAIActivated())
			m_MeleeHitData.m_fDamage = m_MeleeHitData.m_fDamage*0.4;
		
		IEntity bayonet;
		IEntity child = weaponEntity.GetChildren();
		while (child)
		{
			if (child.FindComponent(SCR_BayonetComponent))
			{
				bayonet = child;
				break;
			}
			
			child = child.GetSibling();
		}
		
		m_MeleeHitData.m_Bayonet = bayonet;
		m_bHasBayonet = m_MeleeHitData.m_Bayonet != null;
		Replication.BumpMe();
	}
	override void PerformAttack()
	{
		if (m_bAttackAlreadyExecuted)
			return;

#ifdef SCR_MELEE_DEBUG
		Do_ClearDbgShapes();
#endif
		
		//This can be attached to a script invoker, which tells us the player changed their weapon
		CollectMeleeWeaponProperties();
		m_bAttackAlreadyExecuted = true;
	}
//------------------------------------------------------------------------------------------------
	//! Processes the melee attack. Applies the damage if applicable
	override protected void ProcessMeleeAttack()
	{
		RplComponent rplComponent = RplComponent.Cast(GetOwner().FindComponent(RplComponent));
		if (rplComponent && rplComponent.IsProxy())
			return; // Don't call on clients

		if (!CheckCollisionsSimple(m_MeleeHitData))
			return;
		
		if (m_MeleeHitData.m_Weapon)
			HandleMeleeSound();
		
#ifdef SCR_MELEE_DEBUG
		m_aDbgSamplePositionsShapes.Clear();
		Debug_DrawSphereAtPos(m_MeleeHitData.m_vHitPosition, m_aDbgSamplePositionsShapes, 0xff00ff00, 0.03, ShapeFlags.NOZBUFFER);
#endif
		
		if (m_OnMeleePerformed)
			m_OnMeleePerformed.Invoke(GetOwner());
		
		// This callLater is necessary because the use of traces pushed the handledamage out of main thread, which causes VME's
		GetGame().GetCallqueue().CallLater(HandleDamageDelayed);
	}
	override protected void HandleDamageDelayed()
{
	if (m_MeleeHitData.m_Bayonet && m_MeleeHitData.m_Entity)
	{
		SCR_BayonetComponent bayonet = SCR_BayonetComponent.Cast(m_MeleeHitData.m_Bayonet.FindComponent(SCR_BayonetComponent));
		if (bayonet && SCR_ChimeraCharacter.Cast(m_MeleeHitData.m_Entity))
			bayonet.AddBloodToBayonet();
		
		SCR_BayonetEffectComponent effectComponent = SCR_BayonetEffectComponent.Cast(m_MeleeHitData.m_Bayonet.FindComponent(SCR_BayonetEffectComponent));
		if (effectComponent)
			effectComponent.OnImpact(
				m_MeleeHitData.m_Entity,
				m_MeleeHitData.m_fDamage,
				m_MeleeHitData.m_vHitPosition,
				m_MeleeHitData.m_vHitNormal,
				m_MeleeHitData.m_SurfaceProps);
	}
	
	vector hitPosDirNorm[3];
	hitPosDirNorm[0] = m_MeleeHitData.m_vHitPosition;
	hitPosDirNorm[1] = m_MeleeHitData.m_vHitDirection;
	hitPosDirNorm[2] = m_MeleeHitData.m_vHitNormal;
	
	// check if the entity is destructible entity
	SCR_DestructibleEntity destructibleEntity = SCR_DestructibleEntity.Cast(m_MeleeHitData.m_Entity);
	if (destructibleEntity)
	{
		destructibleEntity.HandleDamage(EDamageType.MELEE, m_MeleeHitData.m_fDamage, hitPosDirNorm);
		return;
	}
	
	// check if the entity has the damage manager component
	HitZone hitZone;
	SCR_DamageManagerComponent damageManager = SearchHierarchyForDamageManager(m_MeleeHitData.m_Entity, hitZone);
	if (!hitZone || !damageManager)
		return;
		
	MeleeBlocking blockingScr = MeleeBlocking.Cast(m_MeleeHitData.m_Entity.FindComponent(MeleeBlocking));
	float dmg = m_MeleeHitData.m_fDamage;
	
	if (blockingScr && blockingScr.m_bIsBlocking)
	{
		// Get the entity's forward direction
		vector entityTransform[4];
		m_MeleeHitData.m_Entity.GetTransform(entityTransform);
		vector entityForward = entityTransform[2]; // Forward vector
		
		// Get the damage direction (normalized)
		vector damageDirection =m_MeleeHitData.m_vHitDirection.Normalized();
		
		// Calculate the dot product to determine angle
		// Since we want to check if damage is coming from the front, we need to invert the damage direction
		vector inverseDamageDirection = -damageDirection;
		float dotProduct = vector.Dot(entityForward, inverseDamageDirection);
		
		// Check if the angle is within 45 degrees from the front
		// cos(45°) ≈ 0.707
		float angleThreshold = 0.6;
		
		if (dotProduct >= angleThreshold)
		{
			// Damage is coming from the front within 45 degrees, apply blocking reduction
			dmg = dmg * blockingScr.m_fBlockingReduction;
		}
		// If damage is not from the front, blocking doesn't reduce damage
	}
	
	SCR_DamageContext context = new SCR_DamageContext(EDamageType.MELEE, dmg, hitPosDirNorm, 
		damageManager.GetOwner(), hitZone, Instigator.CreateInstigator(GetOwner()), 
		m_MeleeHitData.m_SurfaceProps, m_MeleeHitData.m_iColliderIndex, m_MeleeHitData.m_iNodeIndex);
		
	if (m_MeleeHitData.m_Bayonet)
		context.damageSource = m_MeleeHitData.m_Bayonet;
	else
		context.damageSource = m_MeleeHitData.m_Weapon;
		
	if (m_MeleeHitData.m_Bayonet)
		context.damageSource = m_MeleeHitData.m_Bayonet;
	else
		context.damageSource = m_MeleeHitData.m_Weapon;
	
	float bleedRoll = Math.RandomFloat01();
	if (bleedRoll <= 0.2) // 30% chance to cause bleeding
	{
		context.damageType = EDamageType.BLEEDING;
	}
	context.damageEffect = new SCR_MeleeDamageEffect();
	damageManager.HandleDamage(context);
}
	
}
