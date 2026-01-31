modded class BLD_PlacerComponent : ScriptComponent
{
	////////////////////////////////////////////////////////
	// WORKBENCH PLACEMENT
	////////////////////////////////////////////////////////
	IEntity ghostWorkbenchEnt;

	void SpawnWorkbenchGhost()
	{
		Rpc(RpcDo_SpawnWorkbenchGhost);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_SpawnWorkbenchGhost()
	{
		SCR_PopUpNotification.GetInstance().PopupMsg("Press Gadget Use (R) to Place");
		// UPDATE GUID - Get from workbenchGhost.et in Workbench
		string resourcePath = "{F630ACBD2418A1F1}Prefabs/BaseBuilding/Workbench/workbenchGhost.et";
		Resource resource = Resource.Load(resourcePath);
		EntitySpawnParams paramsfoundy = new EntitySpawnParams();
		vector spawnPosition = GetOwner().GetOrigin();
		paramsfoundy.Transform[3] = spawnPosition;
		ghostWorkbenchEnt = GetGame().SpawnEntityPrefab(resource, null, paramsfoundy);
		GetGame().GetCallqueue().CallLater(MoveWorkbenchGhost, 50, true);
	}

	void MoveWorkbenchGhost()
	{
		if (ghostWorkbenchEnt)
		{
			vector plrPosition = GetOwner().GetOrigin();
			vector forwardDir = GetOwner().GetTransformAxis(2);
			vector offset = forwardDir * 2;
			vector nextPosition = plrPosition + offset + {0, 0.02, 0};
			vector spawnAngles = GetOwner().GetAngles() + {0, 180, 0};
			ghostWorkbenchEnt.SetOrigin(nextPosition);
			ghostWorkbenchEnt.SetAngles(spawnAngles);
		}
		else
		{
			GetGame().GetCallqueue().Remove(MoveWorkbenchGhost);
		}
	}

	void RemoveWorkbenchGhost()
	{
		Rpc(RpcDo_RemoveWorkbenchGhost);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_RemoveWorkbenchGhost()
	{
		if (ghostWorkbenchEnt)
		{
			GetGame().GetCallqueue().Remove(MoveWorkbenchGhost);
			RplComponent.DeleteRplEntity(ghostWorkbenchEnt, false);
		}
	}

	void PlaceWorkbench(IEntity itm)
	{
		lastInvItem = itm;
		IEntity plr = GetOwner();
		AudioSystem.PlaySound("{1170AA2401E56963}Sounds/Items/DeployableRadio/Samples/Items_DeployableRadio_Deploy_03.wav");
		vector plrPosition = plr.GetOrigin();
		vector forwardDir = plr.GetTransformAxis(2);
		vector offset = forwardDir * 2;
		vector nextPosition = plrPosition + offset + {0, 0.02, 0};
		vector spawnAngles = plr.GetAngles() + {0, 180, 0};
		// UPDATE GUID - Get from Workbench_Crafting.et in Datz-Enhanced
		string workbenchPrefab = "{D8A06388C98F5ACF}Prefabs/Workbench_Crafting.et";
		BLD_BedSpawnComponent bedSpawn = BLD_BedSpawnComponent.Cast(plr.FindComponent(BLD_BedSpawnComponent));
		string lastPin = "";
		if (bedSpawn && bedSpawn.lastCode.Length() == 4)
			lastPin = bedSpawn.lastCode;

		Rpc(RpcSpawnWorkbench, workbenchPrefab, nextPosition, spawnAngles, lastPin);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcSpawnWorkbench(string resourcePath, vector position, vector angles, string lastPin)
	{
		Resource resource = Resource.Load(resourcePath);
		if (!resource)
		{
			Print("Failed to load workbench resource: " + resourcePath, LogLevel.ERROR);
			return;
		}

		EntitySpawnParams params = new EntitySpawnParams();
		params.Transform[3] = position;

		IEntity nent = GetGame().SpawnEntityPrefab(resource, null, params);
		if (!nent)
			return;

		// Set angles
		nent.SetAngles(angles);

		// Update entity and all children to sync visual with collider
		nent.Update();

		// Update all child entities as well (for compositions)
		IEntity child = nent.GetChildren();
		while (child)
		{
			child.Update();
			child = child.GetSibling();
		}

		// Report item consumed
		Rpc(RpcReportPlacedItem);

		// Set ownership
		string pUID = EPF_PersistenceComponent.GetPersistentId(GetOwner());
		BLD_OwnershipComponent ownershipComponent = BLD_OwnershipComponent.Cast(nent.FindComponent(BLD_OwnershipComponent));
		if (ownershipComponent)
		{
			ownershipComponent.SetOwnerID(pUID);
			ownershipComponent.SetPartHP(2000);
			if (lastPin.Length() == 4)
				ownershipComponent.SetCode(lastPin);
		}
	}
}
