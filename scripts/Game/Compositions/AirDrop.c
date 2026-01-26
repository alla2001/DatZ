//------------------------------------------------------------------------------------------------
// COMPONENT - Makes the airdrop fall slowly when attached to the airdrop prefab
//------------------------------------------------------------------------------------------------

[BaseContainerProps()]
class AirDropFallComponentClass : ScriptComponentClass
{
}

[BaseContainerProps()]
class InitItem : Managed
{
	[Attribute("", params: "et", category: "Prefabs")]
	ResourceName item;
	[Attribute("1")]
	int amount;
}

//------------------------------------------------------------------------------------------------
class AirDropFallComponent : ScriptComponent
{
	[Attribute("2.0", UIWidgets.EditBox, "Fall speed in meters per second")]
	float m_fFallSpeed;

	[Attribute("true", UIWidgets.CheckBox, "Should destroy on ground impact")]
	bool m_bDestroyOnImpact;

	[Attribute("8", UIWidgets.EditBox, "Number of bonus random items to spawn from TW loot table")]
	int m_iBonusLootCount;

	[Attribute("true", UIWidgets.CheckBox, "Should spawn bonus loot from TW_LootManager")]
	bool m_bSpawnBonusLoot;

	protected bool m_bIsFalling = true;
	protected float m_fGroundLevel = 0;
	protected bool m_bIsDestroyed = false;

	[Attribute(desc: "Marker Type", category: "Map Marker")]
	protected ref SCR_ScenarioFrameworkMarkerType m_MapMarkerType;

	[Attribute("", UIWidgets.Object, "Initial items to spawn")]
	protected ref array<ref InitItem> initItems;

	ref SCR_MapMarkerBase m_MapMarker;

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		SetEventMask(owner, EntityEvent.INIT);
	}

	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		if (!Replication.IsServer())
			return;

		FindGroundLevel();
		StartFalling();
		Print("AirDropFallComponent: Initialized", LogLevel.NORMAL);
		CreateMapMarker();
	}

	//------------------------------------------------------------------------------------------------
	protected void CreateMapMarker()
	{
		SCR_MapMarkerManagerComponent mapMarkerMgr = SCR_MapMarkerManagerComponent.Cast(GetGame().GetGameMode().FindComponent(SCR_MapMarkerManagerComponent));
		if (!mapMarkerMgr)
			return;

		m_MapMarker = new SCR_MapMarkerBase();

		SCR_ScenarioFrameworkMarkerCustom mapMarkerCustom = SCR_ScenarioFrameworkMarkerCustom.Cast(m_MapMarkerType);
		if (mapMarkerCustom)
		{
			m_MapMarker.SetType(SCR_EMapMarkerType.PLACED_CUSTOM);
			m_MapMarker.SetIconEntry(mapMarkerCustom.m_eMapMarkerIcon);
			m_MapMarker.SetRotation(mapMarkerCustom.m_iMapMarkerRotation);
			m_MapMarker.SetColorEntry(mapMarkerCustom.m_eMapMarkerColor);
		}
		else
		{
			SCR_ScenarioFrameworkMarkerMilitary mapMarkerMilitary = SCR_ScenarioFrameworkMarkerMilitary.Cast(m_MapMarkerType);
			if (!mapMarkerMilitary)
				return;

			m_MapMarker = mapMarkerMgr.PrepareMilitaryMarker(mapMarkerMilitary.m_eMapMarkerFactionIcon, mapMarkerMilitary.m_eMapMarkerDimension, mapMarkerMilitary.m_eMapMarkerType1Modifier | mapMarkerMilitary.m_eMapMarkerType2Modifier);
		}

		vector worldPos = GetOwner().GetOrigin();
		m_MapMarker.SetWorldPos(worldPos[0], worldPos[2]);
		m_MapMarker.SetCustomText(m_MapMarkerType.m_sMapMarkerText);
		m_MapMarker.SetCanBeRemovedByOwner(true);

		mapMarkerMgr.InsertStaticMarker(m_MapMarker, false, true);
	}

	//------------------------------------------------------------------------------------------------
	void RemoveMapMarker()
	{
		if (!m_MapMarker)
			return;

		SCR_MapMarkerManagerComponent markerMgr = SCR_MapMarkerManagerComponent.GetInstance();
		if (!markerMgr)
			return;

		SCR_MapMarkerBase marker = markerMgr.GetStaticMarkerByID(m_MapMarker.GetMarkerID());
		if (marker)
			markerMgr.RemoveStaticMarker(marker);

		m_MapMarker = null;
	}

	//------------------------------------------------------------------------------------------------
	protected void FindGroundLevel()
	{
		IEntity owner = GetOwner();
		if (!owner)
		{
			m_fGroundLevel = 0;
			return;
		}

		World world = GetGame().GetWorld();
		if (!world)
		{
			m_fGroundLevel = 0;
			return;
		}

		vector startPos = owner.GetOrigin();
		vector endPos = Vector(startPos[0], startPos[1] - 2000, startPos[2]);

		TraceParam trace = new TraceParam();
		trace.Start = startPos;
		trace.End = endPos;
		trace.Flags = TraceFlags.ENTS | TraceFlags.WORLD;
		trace.TargetLayers = EPhysicsLayerDefs.Default | EPhysicsLayerDefs.Terrain | EPhysicsLayerDefs.Static;

		float traceResult = world.TraceMove(trace, null);

		if (traceResult < 1.0)
		{
			vector hitPos = vector.Lerp(startPos, endPos, traceResult);
			m_fGroundLevel = hitPos[1];
		}
		else
		{
			m_fGroundLevel = 0;
		}

		if (m_fGroundLevel < 0)
			m_fGroundLevel = 0;

		Print(string.Format("AirDropFallComponent: Ground level: %1", m_fGroundLevel), LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	protected void StartFalling()
	{
		GetGame().GetCallqueue().CallLater(UpdateFall, 50, true);
		SendAlarmToAllPlayers();
	}

	//------------------------------------------------------------------------------------------------
	protected void SendAlarmToAllPlayers()
	{
		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);

		foreach (int playerId : playerIds)
		{
			PlayerController pc = GetGame().GetPlayerManager().GetPlayerController(playerId);
			if (!pc)
				continue;

			SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(pc.GetControlledEntity());
			if (character)
			{
				BLD_PlacerComponent placer = BLD_PlacerComponent.Cast(character.FindComponent(BLD_PlacerComponent));
				if (placer)
					placer.PlaySoundFromPlayer("{275A883E38CDEF60}Sounds/UI/Samples/Menu/UI_Task_Failed.wav", "AIR DROP INCOMING!");
			}
		}

		RplComponent rpl = RplComponent.Cast(GetOwner().FindComponent(RplComponent));
		if (rpl)
			Rpc(RpcDo_SendAlarm);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_SendAlarm()
	{
		SCR_PopUpNotification popup = SCR_PopUpNotification.GetInstance();
		if (popup)
			popup.PopupMsg(text: "AIR DROP INCOMING", duration: 15.0, text2: "The AirDrop is Landing!", sound: SCR_SoundEvent.SOUND_BELL_B);
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateFall()
	{
		if (!m_bIsFalling || m_bIsDestroyed)
		{
			GetGame().GetCallqueue().Remove(UpdateFall);
			return;
		}

		IEntity owner = GetOwner();
		if (!owner)
		{
			GetGame().GetCallqueue().Remove(UpdateFall);
			return;
		}

		vector currentPos = owner.GetOrigin();

		if (currentPos[1] <= m_fGroundLevel)
		{
			m_bIsFalling = false;
			GetGame().GetCallqueue().Remove(UpdateFall);

			currentPos[1] = m_fGroundLevel;
			owner.SetOrigin(currentPos);

			OnLanded();
			return;
		}

		float fallDistance = m_fFallSpeed * 0.05;
		vector newPos = Vector(currentPos[0], currentPos[1] - fallDistance, currentPos[2]);
		owner.SetOrigin(newPos);
		owner.Update();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnLanded()
	{
		Print("AirDropFallComponent: Landed!", LogLevel.NORMAL);

		if (m_bDestroyOnImpact)
			GetGame().GetCallqueue().CallLater(DestroyAirdrop, 480000, false); // 8 minutes

		// Spawn items after short delay
		GetGame().GetCallqueue().CallLater(SpawnAllItems, 1000, false);
	}

	//------------------------------------------------------------------------------------------------
	// Simple item spawning - no caching, no batching
	protected void SpawnAllItems()
	{
		IEntity owner = GetOwner();
		if (!owner || m_bIsDestroyed)
			return;

		// Get inventory components
		SCR_InventoryStorageManagerComponent invManager = SCR_InventoryStorageManagerComponent.Cast(owner.FindComponent(SCR_InventoryStorageManagerComponent));
		BaseInventoryStorageComponent storageComp = BaseInventoryStorageComponent.Cast(owner.FindComponent(SCR_UniversalInventoryStorageComponent));

		if (!invManager || !storageComp)
		{
			Print("AirDropFallComponent: No inventory, spawning at position", LogLevel.WARNING);
			SpawnItemsAtPosition();
			return;
		}

		// Spawn configured items
		if (initItems)
		{
			foreach (InitItem initItem : initItems)
			{
				if (!initItem || initItem.item.IsEmpty())
					continue;

				for (int i = 0; i < initItem.amount; i++)
				{
					SpawnAndInsertItem(initItem.item, invManager, storageComp);
				}
			}
		}

		// Spawn bonus loot from TW system
		if (m_bSpawnBonusLoot && m_iBonusLootCount > 0)
			SpawnBonusLoot(invManager, storageComp);

		Print("AirDropFallComponent: Finished spawning items", LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	// Spawn single item and insert into storage
	protected bool SpawnAndInsertItem(ResourceName prefab, SCR_InventoryStorageManagerComponent invManager, BaseInventoryStorageComponent storageComp)
	{
		if (prefab.IsEmpty())
			return false;

		Resource resource = Resource.Load(prefab);
		if (!resource)
		{
			Print(string.Format("AirDropFallComponent: Failed to load: %1", prefab), LogLevel.WARNING);
			return false;
		}

		IEntity owner = GetOwner();
		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		params.Transform[3] = owner.GetOrigin();

		IEntity spawnedItem = GetGame().SpawnEntityPrefab(resource, owner.GetWorld(), params);
		if (!spawnedItem)
			return false;

		// Try to insert into storage
		bool inserted = invManager.TryInsertItemInStorage(spawnedItem, storageComp);
		if (!inserted)
			inserted = invManager.TryInsertItem(spawnedItem, EStoragePurpose.PURPOSE_DEPOSIT);

		if (!inserted)
		{
			SCR_EntityHelper.DeleteEntityAndChildren(spawnedItem);
			return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	// Spawn bonus random loot from TW_LootManager
	protected void SpawnBonusLoot(SCR_InventoryStorageManagerComponent invManager, BaseInventoryStorageComponent storageComp)
	{
		TW_LootManager lootManager = TW_LootManager.GetInstance();
		if (!lootManager)
			return;

		// Get loot pool for valuable items
		SCR_EArsenalItemType flags =
			SCR_EArsenalItemType.RIFLE |
			SCR_EArsenalItemType.PISTOL |
			SCR_EArsenalItemType.MACHINE_GUN |
			SCR_EArsenalItemType.SNIPER_RIFLE |
			SCR_EArsenalItemType.HEADWEAR |
			SCR_EArsenalItemType.VEST_AND_WAIST |
			SCR_EArsenalItemType.BACKPACK |
			SCR_EArsenalItemType.EQUIPMENT |
			SCR_EArsenalItemType.HEAL |
			SCR_EArsenalItemType.WEAPON_ATTACHMENT;

		WeightedType<ref TW_LootConfigItem> pool = TW_LootManager.GetCachedLootPool(flags);
		if (!pool)
			return;

		int spawned = 0;
		int maxAttempts = m_iBonusLootCount * 3;

		for (int attempt = 0; attempt < maxAttempts && spawned < m_iBonusLootCount; attempt++)
		{
			TW_LootConfigItem lootItem = pool.GetRandomItem();
			if (!lootItem || !lootItem.isEnabled || lootItem.resourceName.IsEmpty())
				continue;

			if (SpawnAndInsertItem(lootItem.resourceName, invManager, storageComp))
			{
				spawned++;
				Print(string.Format("AirDropFallComponent: Bonus %1/%2: %3", spawned, m_iBonusLootCount, lootItem.resourceName), LogLevel.NORMAL);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	// Fallback: spawn items at position if no inventory
	protected void SpawnItemsAtPosition()
	{
		IEntity owner = GetOwner();
		if (!owner || !initItems)
			return;

		vector basePos = owner.GetOrigin();
		World world = owner.GetWorld();

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;

		foreach (InitItem initItem : initItems)
		{
			if (!initItem || initItem.item.IsEmpty())
				continue;

			Resource resource = Resource.Load(initItem.item);
			if (!resource)
				continue;

			for (int i = 0; i < initItem.amount; i++)
			{
				vector offset = Vector(Math.RandomFloat(-0.5, 0.5), 0.2, Math.RandomFloat(-0.5, 0.5));
				params.Transform[3] = basePos + offset;
				GetGame().SpawnEntityPrefab(resource, world, params);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void DestroyAirdrop()
	{
		m_bIsDestroyed = true;
		GetGame().GetCallqueue().Remove(UpdateFall);
		RemoveMapMarker();

		IEntity owner = GetOwner();
		if (owner)
			SCR_EntityHelper.DeleteEntityAndChildren(owner);
	}
}
