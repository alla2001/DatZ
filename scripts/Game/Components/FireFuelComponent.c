[ComponentEditorProps(category: "GameScripted/Item", description: "Holds a fluid, allowing filling and draining.")]
class FireFuelComponentClass : ScriptComponentClass
{
}

class FireFuelComponent : ScriptComponent
{
	
	[Attribute("15")]
	float fuelValue ; // Current amount of fluid


}
