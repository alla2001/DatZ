//------------------------------------------------------------------------------------------------
// USER ACTION - Apply skin to skinnable items
//------------------------------------------------------------------------------------------------

class DZ_ApplySkinAction : ScriptedUserAction
{
	protected DZ_SkinnableComponent m_SkinnableComponent;

	//------------------------------------------------------------------------------------------------
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		super.Init(pOwnerEntity, pManagerComponent);
		m_SkinnableComponent = DZ_SkinnableComponent.Cast(pOwnerEntity.FindComponent(DZ_SkinnableComponent));
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBeShownScript(IEntity user)
	{
		if (!m_SkinnableComponent)
			return false;

		// Check if player has any skins for this item
		string playerId = GetPlayerId(user);
		if (playerId.IsEmpty())
			return false;

		array<ref DZ_SkinDefinition> available = m_SkinnableComponent.GetPlayerAvailableSkins(playerId);
		return available && available.Count() > 0;
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBePerformedScript(IEntity user)
	{
		return CanBeShownScript(user);
	}

	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!m_SkinnableComponent)
			return;

		string playerId = GetPlayerId(pUserEntity);
		if (playerId.IsEmpty())
			return;

		// Get available skins
		array<ref DZ_SkinDefinition> available = m_SkinnableComponent.GetPlayerAvailableSkins(playerId);
		if (!available || available.IsEmpty())
			return;

		// Get current skin
		string currentSkin = m_SkinnableComponent.GetCurrentSkinId();

		// Find next skin in rotation (cycle through available skins)
		int currentIndex = -1;
		for (int i = 0; i < available.Count(); i++)
		{
			if (available[i].m_sSkinId == currentSkin)
			{
				currentIndex = i;
				break;
			}
		}

		// Select next skin (or first if none applied or at end)
		int nextIndex = currentIndex + 1;
		if (nextIndex >= available.Count())
			nextIndex = -1; // Remove skin

		if (nextIndex < 0)
		{
			// Remove skin
			m_SkinnableComponent.RemoveSkin();
		}
		else
		{
			// Apply next skin
			m_SkinnableComponent.ApplySkin(playerId, available[nextIndex].m_sSkinId);
		}
	}

	//------------------------------------------------------------------------------------------------
	override bool GetActionNameScript(out string outName)
	{
		if (!m_SkinnableComponent)
		{
			outName = "Apply Skin";
			return true;
		}

		string currentSkin = m_SkinnableComponent.GetCurrentSkinId();
		if (currentSkin.IsEmpty())
		{
			outName = "Apply Skin";
		}
		else
		{
			DZ_SkinManagerComponent skinManager = DZ_SkinManagerComponent.GetInstance();
			if (skinManager)
			{
				DZ_SkinDefinition skin = skinManager.GetSkin(currentSkin);
				if (skin)
					outName = string.Format("Change Skin (%1)", skin.m_sSkinName);
				else
					outName = "Change Skin";
			}
			else
			{
				outName = "Change Skin";
			}
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected string GetPlayerId(IEntity user)
	{
		if (!user)
			return string.Empty;

		PlayerController pc = GetGame().GetPlayerManager().GetPlayerController(GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(user));
		if (!pc)
			return string.Empty;

		return pc.GetPlayerId().ToString();
	}
}

//------------------------------------------------------------------------------------------------
// ACTION - Open skin selection menu (for more complex UI)
//------------------------------------------------------------------------------------------------
class DZ_OpenSkinMenuAction : ScriptedUserAction
{
	protected DZ_SkinnableComponent m_SkinnableComponent;

	//------------------------------------------------------------------------------------------------
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		super.Init(pOwnerEntity, pManagerComponent);
		m_SkinnableComponent = DZ_SkinnableComponent.Cast(pOwnerEntity.FindComponent(DZ_SkinnableComponent));
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBeShownScript(IEntity user)
	{
		if (!m_SkinnableComponent)
			return false;

		string playerId = GetPlayerId(user);
		if (playerId.IsEmpty())
			return false;

		array<ref DZ_SkinDefinition> available = m_SkinnableComponent.GetPlayerAvailableSkins(playerId);
		return available && available.Count() > 1; // Only show if multiple skins available
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBePerformedScript(IEntity user)
	{
		return CanBeShownScript(user);
	}

	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		// TODO: Open skin selection UI menu
		// For now, just cycle through skins like the basic action
		Print("[DZ_SkinMenu] TODO: Open skin selection menu", LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	override bool GetActionNameScript(out string outName)
	{
		outName = "Open Skin Menu";
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected string GetPlayerId(IEntity user)
	{
		if (!user)
			return string.Empty;

		PlayerController pc = GetGame().GetPlayerManager().GetPlayerController(GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(user));
		if (!pc)
			return string.Empty;

		return pc.GetPlayerId().ToString();
	}
}
