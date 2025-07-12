class HeadParticleComponentClass : ScriptComponentClass
{
}

class HeadParticleComponent : ScriptComponent
{
    [Attribute("Path/To/YourEffect.ptc")]
    ResourceName m_sParticleEffect;
  [Attribute()]
        float forwardOffset;
    IEntity m_pParticle;
    SCR_DamageManagerComponent m_dmg;

    override void OnPostInit(IEntity owner)
    {
        super.OnPostInit(owner);

        // Spawn the particle effect
        m_pParticle = GetGame().SpawnEntityPrefab(Resource.Load(m_sParticleEffect));
        if (m_pParticle)
        {
            GetOwner().AddChild(m_pParticle, -1, EAddChildFlags.AUTO_TRANSFORM);
            m_pParticle.SetOrigin("0 0 0");
        }

        // Bind damage state change callback
        m_dmg = SCR_DamageManagerComponent.Cast(owner.FindComponent(SCR_DamageManagerComponent));
        if (m_dmg)
        {
            m_dmg.GetOnDamageStateChanged().Insert(OnDamageStateChanged);
        }

        SetEventMask(owner, EntityEvent.FRAME);
    }

    void OnDamageStateChanged(ECharacterDamageState state)
    {
        if (state == ECharacterDamageState.DESTROYED && m_pParticle)
        {
            delete m_pParticle;
            m_pParticle = null;
        }
    }

    override void EOnFrame(IEntity owner, float timeSlice)
    {
		if(Replication.IsServer())return;
        if (!m_pParticle || !owner)
            return;

        int headBoneIndex = owner.GetAnimation().GetBoneIndex("Head");
        if (headBoneIndex == -1)
            return;

        vector mat[4];
        if (!owner.GetAnimation().GetBoneMatrix(headBoneIndex, mat))
            return;

        // Invert rotation as needed
        mat[0] = -mat[0];
        mat[2] = -mat[2];

       
        mat[3] = mat[3] + (mat[2] * forwardOffset);

        m_pParticle.SetLocalTransform(mat);
        m_pParticle.Update();
    }

    override void OnDelete(IEntity owner)
    {
        super.OnDelete(owner);

        if (m_pParticle)
        {
            delete m_pParticle;
        }

        if (m_dmg)
        {
            m_dmg.GetOnDamageStateChanged().Remove(OnDamageStateChanged);
        }
    }
}
