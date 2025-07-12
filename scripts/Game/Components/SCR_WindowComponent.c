
class SCR_WindowComponentClass : ScriptComponentClass
{}

//------------------------------------------------------------------------------------------------
// Window Component for better window detection
//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "GameScripted/Misc", description: "Window Component")]
class SCR_WindowComponent : ScriptComponent
{
    [Attribute("", UIWidgets.EditBox, "Window type identifier")]
    protected string m_sWindowType;
    
    [Attribute("1", UIWidgets.CheckBox, "Can have planks placed")]
    protected bool m_bCanPlacePlanks;
    
    //------------------------------------------------------------------------------------------------
    string GetWindowType()
    {
        return m_sWindowType;
    }
    
    //------------------------------------------------------------------------------------------------
    bool CanPlacePlanks()
    {
        return m_bCanPlacePlanks;
    }
}