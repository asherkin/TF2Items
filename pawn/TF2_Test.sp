#pragma semicolon 1 // Force strict semicolon mode.

#include <sourcemod>
#include <tf2items>

#define PLUGIN_NAME              "TF2Items Sample Plugin"
#define PLUGIN_AUTHOR            "Damizean & Asherkin"
#define PLUGIN_VERSION           "1.1.0"
#define PLUGIN_CONTACT           "http://limetech.org/"

new Handle:g_hKickass;

public Plugin:myinfo = {
    name        = PLUGIN_NAME,
    author      = PLUGIN_AUTHOR,
    description = PLUGIN_NAME,
    version     = PLUGIN_VERSION,
    url         = PLUGIN_CONTACT
};

/* TF2Items_OnGiveNamedItem()
**
** When an item is about to be given to a client.
** -------------------------------------------------------------------------- */
public Action:TF2Items_OnGiveNamedItem(iClient, String:strClassName[], iItemDefinitionIndex, &Handle:hItemOverride)
{
  PrintToChatAll("GiveNamedItem(%i, %s)", iClient, strClassName)
  if (StrEqual(strClassName, "tf_weapon_syringegun_medic"))
  {
    new Handle:hTest = TF2Items_CreateItem(OVERRIDE_ATTRIBUTES|OVERRIDE_ITEM_QUALITY)
    TF2Items_SetNumAttributes(hTest, 2)
    TF2Items_SetAttribute(hTest, 0,  17, 0.1)
    TF2Items_SetAttribute(hTest, 1,   4, 2.0)
    TF2Items_SetQuality(hTest, 6)
    hItemOverride = hTest
    return Plugin_Changed
  }
  else
  {
    new Handle:hTest = TF2Items_CreateItem(OVERRIDE_ITEM_QUALITY)
    TF2Items_SetQuality(hTest, 6)
    hItemOverride = hTest
    return Plugin_Changed
  }
  return Plugin_Continue       // Return Plugin_Continue to leave them intact
}
    