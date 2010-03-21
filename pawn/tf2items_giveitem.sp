#pragma semicolon 1 // Force strict semicolon mode.

#include <sourcemod>
#define REQUIRE_EXTENSIONS
#include <tf2items>
#include <sdktools>

#define PLUGIN_NAME		"[TF2Items] Give Items"
#define PLUGIN_AUTHOR		"Asherkin"
#define PLUGIN_VERSION		"3.2.0"
#define PLUGIN_CONTACT		"http://limetech.org/"

new g_hItems[MAXPLAYERS+1][6];

public Plugin:myinfo = {
	name			= PLUGIN_NAME,
	author			= PLUGIN_AUTHOR,
	description	= PLUGIN_NAME,
	version		= PLUGIN_VERSION,
	url				= PLUGIN_CONTACT
};

public OnPluginStart() {
	LoadTranslations("common.phrases");

	RegAdminCmd("sm_giveitem", Command_Item, ADMFLAG_CHEATS, "sm_giveitem <player> <itemindex>");
	RegAdminCmd("sm_giveitem_ex", Command_ItemEx, ADMFLAG_CHEATS, "sm_giveitem_ex <player> <itemindex>");
}

public OnClientPutInServer(client) {
	for (new i = 0; i < 6; i++) {
		if (g_hItems[client][i] != -1) {
			g_hItems[client][i] = -1;
		}
	}
	return true;
}

public OnClientDisconnect_Post(client) {
	for (new i = 0; i < 6; i++) {
		if (g_hItems[client][i] != -1) {
			g_hItems[client][i] = -1;
		}
	}
}

public Action:TF2Items_OnGiveNamedItem(client, String:strClassName[], iItemDefinitionIndex, &Handle:hItemOverride)
{
	new weaponSlot;
	new String:formatBuffer[32];
	Format(formatBuffer, 32, "%d_%s", iItemDefinitionIndex, "slot");
	GetTrieValue(g_hItemInfoTrie, formatBuffer, weaponSlot);
	
	if (g_hItems[client][weaponSlot] == -1) {
		return Plugin_Continue;
	}

	hItemOverride = PrepareItemHandle(g_hItems[client][weaponSlot]);
	g_hItems[client][weaponSlot] = -1;
	
	return Plugin_Changed;
}

public Action:Command_ItemEx(client, args)
{
	new String:arg1[32];
	new String:arg2[32];
	new weaponLookupIndex = 0;
 
	if (args != 2) {
		ReplyToCommand(client, "[SM] Usage: sm_giveweapon_ex <player> <itemindex>");
		return Plugin_Handled;
	}
	
	/* Get the arguments */
	GetCmdArg(1, arg1, sizeof(arg1));
	GetCmdArg(2, arg2, sizeof(arg2));
	weaponLookupIndex = StringToInt(arg2);
 
	/**
	 * target_name - stores the noun identifying the target(s)
	 * target_list - array to store clients
	 * target_count - variable to store number of clients
	 * tn_is_ml - stores whether the noun must be translated
	 */
	new String:target_name[MAX_TARGET_LENGTH];
	new target_list[MAXPLAYERS], target_count;
	new bool:tn_is_ml;
 
	if ((target_count = ProcessTargetString(
			arg1,
			client,
			target_list,
			MAXPLAYERS,
			COMMAND_FILTER_ALIVE, /* Only allow alive players */
			target_name,
			sizeof(target_name),
			tn_is_ml)) <= 0)
	{
		/* This function replies to the admin with a failure message */
		ReplyToTargetError(client, target_count);
		return Plugin_Handled;
	}
 
	for (new i = 0; i < target_count; i++)
	{
		new weaponSlot;
		new String:formatBuffer[32];
		Format(formatBuffer, 32, "%d_%s", weaponLookupIndex, "slot");
		new bool:isValidItem = GetTrieValue(g_hItemInfoTrie, formatBuffer, weaponSlot);
		
		if (!isValidItem)
		{
			ReplyToCommand(client, "[SM] Invalid Weapon Index");
			return Plugin_Handled;
		}
		
		PrintToChat(target_list[i], "[SM] Respawn or touch a locker to recieve your new weapon.");
		g_hItems[target_list[i]][weaponSlot] = weaponLookupIndex;
		
		LogAction(client, target_list[i], "\"%L\" gave a weapon to \"%L\"", client, target_list[i]);
	}
 
	if (tn_is_ml) {
		ShowActivity2(client, "[SM] ", "%t was given a weapon!", target_name);
	} else {
		ShowActivity2(client, "[SM] ", "%s was given a weapon!", target_name);
	}
 
	return Plugin_Handled;
}

public Action:Command_Item(client, args)
{
	new String:arg1[32];
	new String:arg2[32];
	new weaponLookupIndex = 0;
 
	if (args != 2) {
		ReplyToCommand(client, "[SM] Usage: sm_giveweapon <player> <itemindex>");
		return Plugin_Handled;
	}
	
	/* Get the arguments */
	GetCmdArg(1, arg1, sizeof(arg1));
	GetCmdArg(2, arg2, sizeof(arg2));
	weaponLookupIndex = StringToInt(arg2);
 
	/**
	 * target_name - stores the noun identifying the target(s)
	 * target_list - array to store clients
	 * target_count - variable to store number of clients
	 * tn_is_ml - stores whether the noun must be translated
	 */
	new String:target_name[MAX_TARGET_LENGTH];
	new target_list[MAXPLAYERS], target_count;
	new bool:tn_is_ml;
 
	if ((target_count = ProcessTargetString(
			arg1,
			client,
			target_list,
			MAXPLAYERS,
			COMMAND_FILTER_ALIVE, /* Only allow alive players */
			target_name,
			sizeof(target_name),
			tn_is_ml)) <= 0)
	{
		/* This function replies to the admin with a failure message */
		ReplyToTargetError(client, target_count);
		return Plugin_Handled;
	}
 
	for (new i = 0; i < target_count; i++)
	{
		new weaponSlot;
		new String:formatBuffer[32];
		Format(formatBuffer, 32, "%d_%s", weaponLookupIndex, "slot");
		new bool:isValidItem = GetTrieValue(g_hItemInfoTrie, formatBuffer, weaponSlot);
		
		if (!isValidItem)
		{
			ReplyToCommand(client, "[SM] Invalid Weapon Index");
			return Plugin_Handled;
		}
		
		new index = -1;
		while ((index = GetPlayerWeaponSlot(target_list[i], weaponSlot)) != -1)
		{
			RemovePlayerItem(target_list[i], index);
			RemoveEdict(index);
		}
		
		new Handle:hWeapon = PrepareItemHandle(weaponLookupIndex);
		
		new entity = TF2Items_GiveNamedItem(target_list[i], hWeapon);
		CloseHandle(hWeapon);
		
		if (IsValidEntity(entity))
		{
			EquipPlayerWeapon(target_list[i], entity);
		} else {
			PrintToChat(target_list[i], "[SM] Respawn or touch a locker to recieve your new weapon.");
			g_hItems[target_list[i]][weaponSlot] = weaponLookupIndex;
		}
		
		LogAction(client, target_list[i], "\"%L\" gave a weapon to \"%L\"", client, target_list[i]);
	}
 
	if (tn_is_ml) {
		ShowActivity2(client, "[SM] ", "%t was given a weapon!", target_name);
	} else {
		ShowActivity2(client, "[SM] ", "%s was given a weapon!", target_name);
	}
 
	return Plugin_Handled;
}

Handle:PrepareItemHandle(weaponLookupIndex)
{
	new String:formatBuffer[32];	
	new String:weaponClassname[64];
	new weaponIndex;
	new weaponSlot;
	new weaponQuality;
	new weaponLevel;
	new String:weaponAttribs[64];
	
	Format(formatBuffer, 32, "%d_%s", weaponLookupIndex, "classname");
	GetTrieString(g_hItemInfoTrie, formatBuffer, weaponClassname, 64);
	
	Format(formatBuffer, 32, "%d_%s", weaponLookupIndex, "index");
	GetTrieValue(g_hItemInfoTrie, formatBuffer, weaponIndex);
	
	Format(formatBuffer, 32, "%d_%s", weaponLookupIndex, "slot");
	GetTrieValue(g_hItemInfoTrie, formatBuffer, weaponSlot);
	
	Format(formatBuffer, 32, "%d_%s", weaponLookupIndex, "quality");
	GetTrieValue(g_hItemInfoTrie, formatBuffer, weaponQuality);
	
	Format(formatBuffer, 32, "%d_%s", weaponLookupIndex, "level");
	GetTrieValue(g_hItemInfoTrie, formatBuffer, weaponLevel);
	
	Format(formatBuffer, 32, "%d_%s", weaponLookupIndex, "attribs");
	GetTrieString(g_hItemInfoTrie, formatBuffer, weaponAttribs, 64);
	
	new String:weaponAttribsArray[32][32];
	new attribCount = ExplodeString(weaponAttribs, " ; ", weaponAttribsArray, 32, 32);
	
	new Handle:hWeapon = TF2Items_CreateItem(OVERRIDE_CLASSNAME | OVERRIDE_ITEM_DEF | OVERRIDE_ITEM_LEVEL | OVERRIDE_ITEM_QUALITY | OVERRIDE_ATTRIBUTES);

	TF2Items_SetClassname(hWeapon, weaponClassname);
	TF2Items_SetItemIndex(hWeapon, weaponIndex);
	TF2Items_SetLevel(hWeapon, weaponLevel);
	TF2Items_SetQuality(hWeapon, weaponQuality);

	if (attribCount > 0) {
		TF2Items_SetNumAttributes(hWeapon, attribCount/2);
		new i2 = 0;
		for (new i = 0; i < attribCount; i+=2) {
			TF2Items_SetAttribute(hWeapon, i2, StringToInt(weaponAttribsArray[i]), StringToFloat(weaponAttribsArray[i+1]));
			i2++;
		}
	} else {
		TF2Items_SetNumAttributes(hWeapon, 0);
	}
	
	return hWeapon;
}