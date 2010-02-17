#pragma semicolon 1 // Force strict semicolon mode.

#include <sourcemod>
#define REQUIRE_EXTENSIONS
#include <sdktools>

#define PLUGIN_NAME		"[TF2Items] Remove Slot"
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
	RegAdminCmd("sm_remslot", Command_RemSlot, ADMFLAG_CHEATS, "sm_remslot <slot>");
}

public Action:Command_RemSlot(client, args)
{
	if (!IsValidClient(client))
		return Plugin_Handled;
		
	new String:arg1[32];
	new slot = 0;
	
	if (args >= 1 && GetCmdArg(1, arg1, sizeof(arg1)))
	{
		slot = StringToInt(arg1);
	}
 
	new index = -1;
	while ((index = GetPlayerWeaponSlot(client, slot)) != -1)
	{
		RemovePlayerItem(client, index);
		RemoveEdict(index);
	}

	return Plugin_Handled;
}

bool:IsValidClient(client)
{
	if (client < 1 || client > MaxClients)
		return false;
	if (!IsClientConnected(client))
		return false;
	return IsClientInGame(client);
}