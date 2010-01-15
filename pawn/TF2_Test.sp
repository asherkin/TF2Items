#pragma semicolon 1 // Force strict semicolon mode.

#include <sourcemod>
#include <sdktools>
#include <tf2>
#include <tf2_attributes>

#define PLUGIN_NAME              "TF2Items Sample Plugin"
#define PLUGIN_AUTHOR            "Damizean"
#define PLUGIN_VERSION           "1.0.0"
#define PLUGIN_CONTACT           "http://limetech.org/"

new Handle:g_hKickass;

public Plugin:myinfo = {
    name        = PLUGIN_NAME,
    author      = PLUGIN_AUTHOR,
    description = PLUGIN_NAME,
    version     = PLUGIN_VERSION,
    url         = PLUGIN_CONTACT
};

/* OnPluginStart()
**
** When the plugin is loaded.
** -------------------------------------------------------------------------- */
public OnPluginStart()
{   
    g_hKickass = CreateScriptedItemOverride(OVERRIDE_ATTRIBUTES);
    SetOverrideNumAttributes(g_hKickass, 8);
    SetOverrideAttribute(g_hKickass, 0, 134, 2.0);
    SetOverrideAttribute(g_hKickass, 1,   2, 100.0);
    SetOverrideAttribute(g_hKickass, 2,   4, 10.0);
    SetOverrideAttribute(g_hKickass, 3,   6, 0.25);
    SetOverrideAttribute(g_hKickass, 4,  16, 500.0);
    SetOverrideAttribute(g_hKickass, 5,  26, 250.0);
    SetOverrideAttribute(g_hKickass, 6,  31, 10.0);
    SetOverrideAttribute(g_hKickass, 7, 107, 3.0);
}

/* OnGiveNamedItem()
**
** When an item is about to be given to a client.
** -------------------------------------------------------------------------- */
public Action:OnGiveNamedItem(iClient, String:strClassName[], iItemDefinitionIndex, &Handle:hItemOverride)
{
    PrintToChatAll("GiveNamedItem(%i, %s)", iClient, strClassName);
    if (StrEqual(strClassName, "tf_wearable_item"))
    {
        // Set the hAttributes to the attribute you want
        hItemOverride = g_hKickass;
    
        return Plugin_Changed;             // Return Plugin_Changed to override the weapon attributes.
    }
    return Plugin_Continue;       // Return Plugin_Continue to leave them intact
}
    