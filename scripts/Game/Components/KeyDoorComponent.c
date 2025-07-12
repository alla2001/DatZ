class KeyDoorComponentClass : ScriptComponentClass
{
}

class KeyDoorComponent : ScriptComponent
{
    DoorComponent door;
    [Attribute("0000")]
    int Code;

    static ref array<KeyDoorComponent> doors = new array<KeyDoorComponent>();
	[Attribute("0 0 0", UIWidgets.EditBox, desc: "Position where Sired Audio Plays", params: "inf inf 0 purpose=coords space=entity")]
	vector sirenOffset;
	
	[Attribute()]
	float OpenTime;
	[RplProp(onRplName: "OnOpenChanged")]
	bool isOpen;
    // Sound variables
    SoundComponent m_SirenSound;
    ResourceName m_SirenSoundResource = "{your_siren_sound_path}"; // Replace with your siren sound path
	AudioHandle sirenAudioHandler;
    override void OnPostInit(IEntity owner)
    {
        door = DoorComponent.Cast(owner.FindComponent(DoorComponent));
        m_SirenSound = SoundComponent.Cast(owner.FindComponent(SoundComponent));
		if(!doors.Contains(this))
        doors.Insert(this);
		SetEventMask(owner,EntityEvent.INIT);
    }
	override void EOnInit(IEntity owner){
	
		if(!doors.Contains(this))
        doors.Insert(this);
	
	}
    static void UseCard(int code)
    {
		if(!Replication.IsServer())return;
        foreach (KeyDoorComponent keyDoor : doors)
        {
            if (keyDoor && keyDoor.Code == code)
                keyDoor.OpenDoor();
        }
    }

    void OpenDoor()
    {
        if (!door || isOpen)
            return;
		
        door.SetControlValue(1);
		isOpen=true;
        PlaySiren();
		GetGame().GetCallqueue().CallLater(CloseDoor,delay:OpenTime*1000*60);
		Replication.BumpMe();
    }
	 void CloseDoor()
    {
        if (!door || !isOpen)
            return;

        door.SetControlValue(0);
		isOpen=false;
        StopSiren();
		Replication.BumpMe();
    }
		//------------------------------------------------------------------------------------------------
	protected void OnOpenChanged()
	{
		if(isOpen)
			PlaySiren();
		else
			StopSiren();
	}
	override bool RplLoad(ScriptBitReader reader){
		
		super.RplLoad(reader);
		
		OnOpenChanged();
		return true;
	
	}
    void PlaySiren()
    {
        if (!sirenAudioHandler && m_SirenSound && m_SirenSoundResource)
        {
            sirenAudioHandler = m_SirenSound.SoundEventOffset("SOUND_SIREN_LP",sirenOffset);

        }
    }

    void StopSiren()
    {
        if (m_SirenSound)
        {
          	m_SirenSound.Terminate(sirenAudioHandler);
			sirenAudioHandler=null;
        }
    }
}
