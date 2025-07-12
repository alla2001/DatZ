

modded class SCR_WheeledDamageManagerComponent : SCR_VehicleDamageManagerComponent
{
	// Physics variables
	protected float m_fHighestContact = 1.0;
	//------------------------------------------------------------------------------------------------
	override protected void OnDamage(notnull BaseDamageContext damageContext)
	{
		if (!s_aDamageManagerData.IsIndexValid(m_iDamageManagerDataIndex))
			return;

		ScriptInvoker invoker = s_aDamageManagerData[m_iDamageManagerDataIndex].GetOnDamage(false);
		if (invoker)
			invoker.Invoke(damageContext);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Calculate damage from collision event based on contact data
	//! \param owner Owner is entity that is parent of this damageManager
	//! \param other Other is the entity that collided with the owner
	//! \param contact Contact data class should contain all collisiondata needed to compute damage
	override protected void OnFilteredContact(IEntity owner, IEntity other, Contact contact)
	{
		super.OnFilteredContact(owner,other,contact);
		CalculateCollisionDamage(owner, other, contact.Position);
	}
		
	//------------------------------------------------------------------------------------------------
	//
	protected void CalculateCollisionDamage(IEntity owner, IEntity other, vector collisionPosition, bool instantUnconsciousness = true)
	{
		HitZone defaultHitZone = GetDefaultHitZone();
		if (!defaultHitZone || defaultHitZone.GetDamageState() == EDamageState.DESTROYED)
			return;

		SCR_ChimeraCharacter char = SCR_ChimeraCharacter.Cast(other);
		
		if(!char) return;
		
		float momentumCharacterThreshold = m_fMinImpulse * 1 * Physics.KMH2MS;
		float momentumCharacterDestroy = m_fMinImpulse * 100 * Physics.KMH2MS;
		float damageScaleToCharacter = (momentumCharacterDestroy - momentumCharacterThreshold) * 0.0001;
		
		float impactMomentum = Math.AbsFloat(m_fMinImpulse * m_fHighestContact);
		
		float damageValue = damageScaleToCharacter * (impactMomentum - momentumCharacterThreshold);
		if (damageValue <= 0)
			return;
		damageValue = damageValue*0.01;
		
		// We apply the collisiondamage only every 200 ms at most. 
		// If a bigger collision happens within this time, this collisions Contact data will overwrite the previous collision, but does not reset the remaining time until it is applied.
		const int impulseDelay = 200;
		int remainingTime = GetGame().GetCallqueue().GetRemainingTime(ApplyCollisionDamage);
		if (remainingTime == -1)
		{
			GetGame().GetCallqueue().CallLater(ApplyCollisionDamage, impulseDelay, false, other, collisionPosition, damageValue);
		}
		else
		{
			GetGame().GetCallqueue().Remove(ApplyCollisionDamage);
			GetGame().GetCallqueue().CallLater(ApplyCollisionDamage, remainingTime, false, other, collisionPosition, damageValue);
		}
	}
	//------------------------------------------------------------------------------------------------
	protected void ApplyCollisionDamage(IEntity other, vector collisionPosition, float damageValue)
	{
		if (!GetOwner())
			return;

		HitZone defaultHitZone = GetDefaultHitZone();
		if (!defaultHitZone || defaultHitZone.GetDamageState() == EDamageState.DESTROYED)
			return;

		array<HitZone> characterHitZones = {};
		GetAllHitZones(characterHitZones);
		
		// Apply collisionDamage only to the 6 nearest hitzones to the contactpoint
		const int hitZonesReturnAmount = 6;

		GetNearestHitZones(collisionPosition, characterHitZones, hitZonesReturnAmount);
		
	
		
		vector hitPosDirNorm[3];
		hitPosDirNorm[0] = collisionPosition;

		SCR_DamageContext context = new SCR_DamageContext(EDamageType.COLLISION, damageValue/hitZonesReturnAmount, hitPosDirNorm, GetOwner(), characterHitZones[0], Instigator.CreateInstigator(other), null, -1, -1);
		
		foreach (HitZone characterHitZone : characterHitZones)
		{
			if (characterHitZone.GetDamageState() == EDamageState.DESTROYED)
				continue;
			
			context.struckHitZone = characterHitZone;
			HandleDamage(context);
		}
		

	}
	//! \param hitZonesReturnAmount Amount of hitZones that are returned in nearestHitZones. IF all are wanted, set nearestHitZones.Count().
	void GetNearestHitZones(vector worldPosition, notnull inout array<HitZone> nearestHitZones, int hitZonesReturnAmount)
	{
		if (nearestHitZones.IsEmpty())
			return;
		
		array<vector> colliderDistances  = {};
		array<int> IDs = {};
		Physics physics = GetOwner().GetPhysics();
		
		vector colliderTransform[4];
		vector relativePosition;
		vector colliderDistance;
		
		foreach (HitZone hitZone : nearestHitZones)
		{
			if (!hitZone.HasColliderNodes())
				continue;
			
			IDs.Clear();
			hitZone.GetColliderIDs(IDs);
			foreach (int ID : IDs)
			{
				physics.GetGeomWorldTransform(ID, colliderTransform);
				relativePosition = worldPosition - colliderTransform[3];
				colliderDistance = {relativePosition.LengthSq(), ID, 0};
				colliderDistances.Insert(colliderDistance);
			}
		}
		
		// Order colliderDistances by distance from close to furthest
		colliderDistances.Sort();
		
		array<int> closestIDs = {};
		for (int i; i < hitZonesReturnAmount; i++)
		{
			closestIDs.InsertAt(colliderDistances[i][1], i);
		}
		
		nearestHitZones.Clear();
		GetHitZonesByColliderIDs(nearestHitZones, closestIDs);
	}
	
	
}
