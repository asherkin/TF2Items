#pragma semicolon 1 // Force strict semicolon mode.

#include <sourcemod>
#define REQUIRE_EXTENSIONS
#include <tf2items>
#include <sdktools>

#define PLUGIN_NAME		"[TF2Items] Sample Plugin"
#define PLUGIN_AUTHOR		"Damizean & Asherkin"
#define PLUGIN_VERSION		"1.1.0"
#define PLUGIN_CONTACT		"http://limetech.org/"

public Plugin:myinfo = {
	name			= PLUGIN_NAME,
	author			= PLUGIN_AUTHOR,
	description	= PLUGIN_NAME,
	version		= PLUGIN_VERSION,
	url				= PLUGIN_CONTACT
};

public OnPluginStart() {
	RegAdminCmd("sm_giveitem", Command_Item, ADMFLAG_ROOT, "sm_giveitem");
}

public Action:TF2Items_OnGiveNamedItem(iClient, String:strClassName[], iItemDefinitionIndex, &Handle:hItemOverride)
{
	PrintToChatAll("GiveNamedItem(%i, %s)", iClient, strClassName);

	// If another plugin already tryied to override the item, let him go ahead.
	if (hItemOverride != INVALID_HANDLE)
		return Plugin_Continue; // Plugin_Changed
	
	if (StrEqual(strClassName, "tf_weapon_syringegun_medic"))
	{
		new Handle:hTest = TF2Items_CreateItem(OVERRIDE_ATTRIBUTES|OVERRIDE_ITEM_QUALITY);
		TF2Items_SetNumAttributes(hTest, 2);
		TF2Items_SetAttribute(hTest, 0,  17, 0.1);
		TF2Items_SetAttribute(hTest, 1,   4, 2.0);
		TF2Items_SetQuality(hTest, 6);
		hItemOverride = hTest;
		return Plugin_Changed;
	}
	else
	{
		new Handle:hTest = TF2Items_CreateItem(OVERRIDE_ITEM_QUALITY);
		TF2Items_SetQuality(hTest, 6);
		hItemOverride = hTest;
		return Plugin_Changed;
	}
}

public Action:Command_Item(client, args)
{
	new Handle:hTest = TF2Items_CreateItem(OVERRIDE_CLASSNAME | OVERRIDE_ITEM_DEF | OVERRIDE_ITEM_LEVEL | OVERRIDE_ITEM_QUALITY | OVERRIDE_ATTRIBUTES);

	TF2Items_SetClassname(hTest, "tf_weapon_revolver");
	TF2Items_SetItemIndex(hTest, 24);
	TF2Items_SetLevel(hTest, 1);
	TF2Items_SetQuality(hTest, 1);
	TF2Items_SetNumAttributes(hTest, 0);
	new entity = TF2Items_GiveNamedItem(client, hTest);
	CloseHandle(hTest);
	
	EquipPlayerWeapon(client, entity);
	return Plugin_Handled;
}