class FoodItemComponentClass : ScriptComponentClass
{
}



class FoodItemComponent : ScriptComponent
{
	[Attribute("60", UIWidgets.Slider, "Time to reach perfect cook (seconds)", "0 300 1")]
	float m_fCookTimePerfect;

	[Attribute("120", UIWidgets.Slider, "Time to reach burnt state (seconds)", "0 600 1")]
	float m_fCookTimeBurnt;

	[RplProp()]
	float m_fCookProgress = 0.0;

	[Attribute()]
	ResourceName cookedMaterial;

	[Attribute()]
	ResourceName burntMaterial;

	Material cooked;
	Material burnt;
	ECookState lastState;
	// Returns cooking state: 0 = raw, 1 = perfect, 2 = burnt
	ECookState GetCookingState()
	{
		if (m_fCookProgress < m_fCookTimePerfect)
			return ECookState.RAW; // raw
		else if (m_fCookProgress < m_fCookTimeBurnt)
			return ECookState.PERFECT; // perfect
		else
			return ECookState.BURNT; // burnt
	}

	// Returns cooking progress as a percentage (0 to 100)
	float GetCookingProgress()
	{
		return (m_fCookProgress / m_fCookTimeBurnt) * 100.0;
	}

	void AddCookTime(float deltaTime)
	{
		m_fCookProgress += deltaTime;
		Replication.BumpMe();
		UpdateVisuals();
	}

	void UpdateVisuals()
	{
		IEntity owner = GetOwner();
		if (!owner)
			return;

		if(lastState == GetCookingState())return;

		switch (GetCookingState())
		{
			case ECookState.PERFECT:
				SCR_Global.SetMaterial(GetOwner(),cookedMaterial);
				break;

			case ECookState.BURNT:
				SCR_Global.SetMaterial(GetOwner(),burntMaterial);
				break;

			case ECookState.RAW:
				// Optionally reset to default material if needed
				break;
		}
		lastState=GetCookingState();
		InventoryItemComponent
	}

	//------------------------------------------------------------------------------------------------
	override bool RplLoad(ScriptBitReader reader)
	{
		if (!super.RplLoad(reader))
			return false;

		UpdateVisuals();
		return true;
	}
}
