// Override ADM_MerchandisePrefab to add editor search by prefab name
// This allows searching merchandise by prefab name in the Workbench editor

[BaseContainerProps(), SCR_BaseContainerCustomTitleResourceName("m_sPrefab")]
modded class ADM_MerchandisePrefab : ADM_MerchandiseType
{
}
