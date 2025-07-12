
//------------------------------------------------------------------------------------------------
//! UI Script
//! Inventory Slot UI Layout
modded class SCR_InventorySlotUI : ScriptedWidgetComponent
{
	
	override void SetSlotVisible( bool bVisible )
	{
		m_bVisible = bVisible;
		m_widget.SetEnabled(bVisible);
		m_widget.SetVisible(bVisible);
	
		if(bVisible)
		{
			m_wPreviewImage = RenderTargetWidget.Cast( m_widget.FindAnyWidget( "item" ) );
			ImageWidget iconImage = ImageWidget.Cast(m_widget.FindAnyWidget("icon"));
			RichTextWidget iconText = RichTextWidget.Cast(m_widget.FindAnyWidget("itemName"));
			TextWidget quickslotNumber = TextWidget.Cast(m_widget.FindAnyWidget("TextQuickSlotLarge"));
			
			//filter out 1-4 quickslots as they are in fact storages :harold:
			if (SCR_InventoryMenuUI.WEAPON_SLOTS_COUNT <= m_iQuickSlotIndex && m_iQuickSlotIndex < 10)
			{
				IEntity controlled = SCR_PlayerController.GetLocalControlledEntity();
				if (controlled)
				{
					SCR_CharacterInventoryStorageComponent storage = GetCharacterStorage(controlled);
					if (storage)
					{
						SCR_QuickslotBaseContainer container = storage.GetContainerFromQuickslot(m_iQuickSlotIndex);
						if (container)
							container.HandleVisualization(iconImage, m_wPreviewImage, iconText, quickslotNumber);
					}
				}
			}
			
			ChimeraWorld world = ChimeraWorld.CastFrom(GetGame().GetWorld());
			if (world)
			{
				ItemPreviewManagerEntity manager = world.GetItemPreviewManager();
				if (manager)
				{
					ItemPreviewWidget renderPreview = ItemPreviewWidget.Cast( m_wPreviewImage );
					IEntity previewEntity = null;
					if (m_pItem)
					{
						previewEntity = m_pItem.GetOwner();
						m_wPreviewImage.SetVisible(true);
					}

					if (renderPreview)
						manager.SetPreviewItem(renderPreview, previewEntity, null, true);
				}
			}
			m_wProgressBar = m_widget.FindAnyWidget("ProgressBar");
			m_ProgressBar = SCR_InventoryProgressBar.Cast(m_wProgressBar.FindHandler(SCR_InventoryProgressBar));
			//if the slot has is from DATZ System
			if (m_pItem)
			{
			
				if(m_pItem.GetOwner().FindComponent(FluidContainerComponent))
				{
					FluidContainerComponent fluid = FluidContainerComponent.Cast(m_pItem.GetOwner().FindComponent(FluidContainerComponent) );
					if(fluid && m_wProgressBar)
					{
						
						m_wProgressBar.SetVisible(true);
						m_ProgressBar.SetProgressRange(0, fluid.m_fMaxCapacity);
						m_ProgressBar.SetCurrentProgress(fluid.m_fCurrentAmount);

					}
				
				}
				if(m_pItem.GetOwner().FindComponent(CookableItemComponent))
				{
					CookableItemComponent cookable = CookableItemComponent.Cast(m_pItem.GetOwner().FindComponent(CookableItemComponent) );
					if(cookable && m_wProgressBar)
					{
						
						
						m_wProgressBar.SetVisible(true);
						m_ProgressBar.SetProgressRange(0, cookable.m_fCookTimeBurnt);
						m_ProgressBar.SetCurrentProgress(cookable.m_fCookProgress);
					
					}
					if (cookable&&m_wPreviewImage)
					{
						world = ChimeraWorld.CastFrom(GetGame().GetWorld());
				if (world)
			{
				ItemPreviewManagerEntity manager = world.GetItemPreviewManager();
				if (manager)
				{
					ItemPreviewWidget renderPreview = ItemPreviewWidget.Cast( m_wPreviewImage );
					IEntity previewEntity = null;
					if (m_pItem)
					{
						previewEntity = m_pItem.GetOwner();
						m_wPreviewImage.SetVisible(true);
					}

					if (renderPreview&& cookable.GetCookingState()==ECookState.PERFECT)
						manager.SetPreviewItemFromPrefab(renderPreview, cookable.cookedPrefab, null, true);
					if (renderPreview&& cookable.GetCookingState()==ECookState.BURNT)
						manager.SetPreviewItemFromPrefab(renderPreview,cookable.burntPrefab, null, true);
				}
			}
						cookable.ResetshouldUpdateVisuals();
					}
				
				}
				if(m_pItem.GetOwner().FindComponent(FirePlaceStorageComponent))
				{
					FirePlaceStorageComponent fireplace = FirePlaceStorageComponent.Cast(m_pItem.GetOwner().FindComponent(FirePlaceStorageComponent) );
					if(fireplace && m_wProgressBar)
					{
						
						
						m_wProgressBar.SetVisible(true);
						m_ProgressBar.SetProgressRange(0, 30);
						m_ProgressBar.SetCurrentProgress(fireplace.m_currentFuel);
					
					}
				
				}
				
				
			}
			
			m_wSelectedEffect = m_widget.FindAnyWidget("SelectedOverlay");
			m_wSelectedIcon = m_widget.FindAnyWidget("SelectedIcon");
			m_wMoveEffect = m_widget.FindAnyWidget( "IconMove" );
			m_wDimmerEffect = m_widget.FindAnyWidget( "Dimmer" );
			m_wBlockedEffect = m_widget.FindAnyWidget("Blocker");
			m_wButton = ButtonWidget.Cast( m_widget.FindAnyWidget( "ItemButton" ) );
			m_wStackNumber = TextWidget.Cast( m_widget.FindAnyWidget( "stackNumber" ) );
			m_wItemLockThrobber = OverlayWidget.Cast(m_widget.FindAnyWidget("itemLockThrobber"));
		
			if ( m_iStackNumber > 1 )
			{
				m_wStackNumber.SetText( m_iStackNumber.ToString() );
				m_wStackNumber.SetVisible( true );
			}
			else
			{
				m_wStackNumber.SetVisible( false );
			}
			
			if (m_eSlotFunction == ESlotFunction.TYPE_MAGAZINE )
			{
				SetAmmoCount();
				UpdateAmmoCount();
			}
			
			if (m_FuelManager)
				OnFuelAmountChanged(m_FuelManager.GetTotalFuel());
			

			if ( m_wMagazineNumber && m_wCurrentMagazineAmmoCount )
			{
				UpdateWeaponAmmoCount();
			}
			
			#ifdef DEBUG_INVENTORY20
				if ( !m_pItem )
				{
					array<string> dbgText = new array<string>();
					this.ToString().Split( "<", dbgText, false );
 					m_wDbgClassText1 = TextWidget.Cast( m_widget.FindAnyWidget( "dbgTextClass1" ) );
					m_wDbgClassText2 = TextWidget.Cast( m_widget.FindAnyWidget( "dbgTextClass2" ) );
					m_wDbgClassText3 = TextWidget.Cast( m_widget.FindAnyWidget( "dbgTextClass3" ) );
					m_wDbgClassText1.SetText( dbgText[0] );
					m_wDbgClassText1.SetEnabled( true );
					m_wDbgClassText1.SetVisible( true );
					m_wDbgClassText2.SetText( dbgText[1] );
					m_wDbgClassText2.SetEnabled( true );
					m_wDbgClassText2.SetVisible( true );
					m_pItem.ToString().Split( "<", dbgText, false );
					if ( dbgText.Count() > 1 )
					{
						m_wDbgClassText3.SetText( dbgText[1] );
						m_wDbgClassText3.SetEnabled( true );
						m_wDbgClassText3.SetVisible( true );
					}
				}
			#endif
			UpdateVolumeBarValue();
			
		}
		else
		{
			m_wPreviewImage = null;
			m_wSelectedEffect = null;
			m_wSelectedIcon = null;
			m_wMoveEffect = null;
			m_wDimmerEffect = null;
			m_wButton = null;
			m_wVolumeBar = null;
			m_wTextQuickSlot = null;
			m_wTextQuickSlotLarge = null;
			m_wStackNumber = null;
			#ifdef DEBUG_INVENTORY20
				m_wDbgClassText1 = null;
				m_wDbgClassText2 = null;
				m_wDbgClassText3 = null;
			#endif
		}
	}
	override protected void SetAmmoCount()
	{
		if (!m_pItem)
			return;
		
		MagazineComponent magComp = MagazineComponent.Cast(m_pItem.GetOwner().FindComponent(MagazineComponent));
		if (!magComp)
			return;

		m_aAmmoCountActual = magComp.GetAmmoCount();
		m_aAmmoCountMax = magComp.GetMaxAmmoCount();			
		
		if (m_ProgressBar)
		{
			
			m_wProgressBar.SetVisible(true);
			m_ProgressBar.FlipColors();
			m_ProgressBar.SetProgressRange(0, magComp.GetMaxAmmoCount());
			m_ProgressBar.SetCurrentProgress(magComp.GetAmmoCount());		
		}
	}
	//------------------------------------------------------------------------------------------------
	protected void UpdateCooking()
	{
			if (!m_ProgressBar)
			return;
		
		if( !m_pItem )
			return;
		if(m_pItem.GetOwner().FindComponent(CookableItemComponent))
		{
			CookableItemComponent cookable = CookableItemComponent.Cast(m_pItem.GetOwner().FindComponent(CookableItemComponent) );
			if(cookable && m_wProgressBar)
			{
				m_ProgressBar.SetProgressRange(0, cookable.m_fCookTimeBurnt);
				m_ProgressBar.SetCurrentProgress(cookable.m_fCookProgress);
			}
			if (cookable&& cookable.shouldUpdateVisuals&&m_wPreviewImage)
			{
				ItemPreviewManagerEntity manager = m_pStorageUI.GetInventoryMenuHandler().GetItemPreviewManager();
				if (manager)
				{
					ItemPreviewWidget renderPreview = ItemPreviewWidget.Cast( m_wPreviewImage );
					IEntity previewEntity = null;
					if (m_pItem)
					{
						previewEntity = m_pItem.GetOwner();
						m_wPreviewImage.SetVisible(true);
					}

					if (renderPreview)
						manager.SetPreviewItem(renderPreview, previewEntity, null, true);
				}
				cookable.ResetshouldUpdateVisuals();
			}
				
				
		}
		if(m_pItem.GetOwner().FindComponent(FirePlaceStorageComponent))
		{
			FirePlaceStorageComponent fireplace = FirePlaceStorageComponent.Cast(m_pItem.GetOwner().FindComponent(FirePlaceStorageComponent) );
			if(fireplace && m_wProgressBar)
			{
				m_ProgressBar.SetProgressRange(0, 100);
				m_ProgressBar.SetCurrentProgress(fireplace.m_currentFuel);
					
			}
				
		}
	}
	//------------------------------------------------------------------------------------------------
	override void Refresh()
	{
		UpdateVolumeBarValue();
		UpdateStackNumber();
		UpdateAmmoCount();
		UpdateCooking();
		UpdateWeaponAmmoCount();
	}
	
	
};