#pragma semicolon 1 // Force strict semicolon mode.

#include <sourcemod>
#define REQUIRE_EXTENSIONS
#include <sdktools>

#define PLUGIN_NAME		"[TF2Items] Remove Slot"
#define PLUGIN_AUTHOR		"Asherkin"
#define PLUGIN_VERSION		"3.1.2.1"
#define PLUGIN_CONTACT		"http://limetech.org/"

public Plugin:myinfo = {
	name			= PLUGIN_NAME,
	author			= PLUGIN_AUTHOR,
	description	= PLUGIN_NAME,
	version		= PLUGIN_VERSION,
	url				= PLUGIN_CONTACT
};

public OnPluginStart() {
	RegAdminCmd("sm_remslot", Command_RemSlot, ADMFLAG_CHEATS, "sm_remslot <player> <slot>");
}

public Action:Command_RemSlot(client, args)
{
	new String:arg1[32];
	new String:arg2[32];
	new slot = 0;
 
	if (args != 2) {
		ReplyToCommand(client, "[SM] Usage: sm_remslot <player> <itemindex>");
		return Plugin_Handled;
	}
	
	/* Get the arguments */
	GetCmdArg(1, arg1, sizeof(arg1));
	GetCmdArg(2, arg2, sizeof(arg2));
	slot = StringToInt(arg2);
 
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
		new index = -1;
		while ((index = GetPlayerWeaponSlot(target_list[i], slot)) != -1)
		{
			RemovePlayerItem(target_list[i], index);
			RemoveEdict(index);
		}
		
		LogAction(client, target_list[i], "\"%L\" removed the weapon in slot %d of \"%L\"", client, slot, target_list[i]);
	}
 
	if (tn_is_ml) {
		ShowActivity2(client, "[SM] ", "%t had the weapon in slot %d removed!", target_name, slot);
	} else {
		ShowActivity2(client, "[SM] ", "%s had the weapon in slot %d removed!", target_name, slot);
	}

	return Plugin_Handled;
}