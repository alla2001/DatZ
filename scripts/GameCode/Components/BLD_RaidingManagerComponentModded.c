modded class BLD_RaidingManagerComponent : DamageManagerComponent
{
	
	override bool HijackDamageHandling(notnull BaseDamageContext damageContext)	{	
		if(Replication.IsServer()){			
			if(damageContext.damageType == EDamageType.EXPLOSIVE){									
				ApplyDamage(Math.Clamp( damageContext.damageValue*10,500,1200));			
			}		
			SCR_HitZone hitZone = SCR_HitZone.Cast(damageContext.struckHitZone);
			if (hitZone)
				hitZone.ApplyDamagePassRules(damageContext);
	
			SCR_FlammableHitZone flammableHitZone = SCR_FlammableHitZone.Cast(damageContext.struckHitZone);
			if (flammableHitZone && damageContext.damageType == EDamageType.INCENDIARY)
				flammableHitZone.HandleIncendiaryDamage(damageContext);
	
			return false;
		}	
		return false;
	}
	
}
