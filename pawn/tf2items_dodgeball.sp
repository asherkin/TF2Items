#pragma semicolon 1 // Force strict semicolon mode.

#include <sourcemod>
#define REQUIRE_EXTENSIONS
#include <tf2items>
#include <sdktools>

#define PLUGIN_NAME		"[TF2Items] Dogdeball Flamethrower"
#define PLUGIN_AUTHOR		"Asherkin"
#define PLUGIN_VERSION		"3.0.0"
#define PLUGIN_CONTACT		"http://limetech.org/"

public Plugin:myinfo = {
	name			= PLUGIN_NAME,
	author			= PLUGIN_AUTHOR,
	description	= PLUGIN_NAME,
	version		= PLUGIN_VERSION,
	url				= PLUGIN_CONTACT
};

public OnPluginStart() {
	LoadTranslations("common.phrases");
}

public Action:TF2Items_OnGiveNamedItem(client, String:strClassName[], iItemDefinitionIndex, &Handle:hItemOverride)
{
	if (hItemOverride != INVALID_HANDLE || !(iItemDefinitionIndex == 21 || iItemDefinitionIndex == 40))
		return Plugin_Continue;
	
	new Handle:hWeapon = TF2Items_CreateItem(OVERRIDE_CLASSNAME | OVERRIDE_ITEM_DEF | OVERRIDE_ITEM_LEVEL | OVERRIDE_ITEM_QUALITY | OVERRIDE_ATTRIBUTES);

	TF2Items_SetClassname(hWeapon, "tf_weapon_flamethrower");
	TF2Items_SetItemIndex(hWeapon, 40);
	TF2Items_SetLevel(hWeapon, 50);
	TF2Items_SetQuality(hWeapon, 2);
	
	TF2Items_SetNumAttributes(hWeapon, 8);
	
	TF2Items_SetAttribute(hWeapon, 0, 112, 0.25);
	TF2Items_SetAttribute(hWeapon, 1, 76, 4.0);
	TF2Items_SetAttribute(hWeapon, 2, 1, 0.0);
	TF2Items_SetAttribute(hWeapon, 3, 134, 3.0);
	TF2Items_SetAttribute(hWeapon, 4, 60, 0.0);
	TF2Items_SetAttribute(hWeapon, 5, 66, 0.0);
	TF2Items_SetAttribute(hWeapon, 6, 72, 0.0);
	TF2Items_SetAttribute(hWeapon, 7, 74, 0.0);
	
	hItemOverride = hWeapon;
	return Plugin_Changed;
}