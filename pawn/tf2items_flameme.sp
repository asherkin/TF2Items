#pragma semicolon 1 // Force strict semicolon mode.

#include <sourcemod>
#define REQUIRE_EXTENSIONS
#include <tf2items>
#include <sdktools>

#define PLUGIN_NAME		"[TF2Items] Flame Me"
#define PLUGIN_AUTHOR		"Asherkin"
#define PLUGIN_VERSION		"3.0.0"
#define PLUGIN_CONTACT		"http://limetech.org/"

new bool:g_hItems[MAXPLAYERS+1];
new Handle:g_hItemInfoTrie = INVALID_HANDLE;

public Plugin:myinfo = {
	name			= PLUGIN_NAME,
	author			= PLUGIN_AUTHOR,
	description	= PLUGIN_NAME,
	version		= PLUGIN_VERSION,
	url				= PLUGIN_CONTACT
};

public OnPluginStart() {
	LoadTranslations("common.phrases");
	RegAdminCmd("sm_flameme", Command_FlameMe, ADMFLAG_RESERVATION, "sm_flameme");
	RegAdminCmd("sm_putmeout", Command_PutMeOut, ADMFLAG_RESERVATION, "sm_putmeout");
	CreateItemInfoTrie();
}

public bool:OnClientConnect(client, String:rejectmsg[], maxlen) {
	g_hItems[client] = false;
	return true;
}

public OnClientDisconnect_Post(client) {
	g_hItems[client] = false;
}

public Action:TF2Items_OnGiveNamedItem(client, String:strClassName[], iItemDefinitionIndex, &Handle:hItemOverride)
{
	if (hItemOverride != INVALID_HANDLE || !g_hItems[client])
		return Plugin_Continue;

	new String:formatBuffer[32];	
	new String:weaponAttribs[64];
	
	Format(formatBuffer, 32, "%d_%s", iItemDefinitionIndex, "attribs");
	new bool:validAttribsList = GetTrieString(g_hItemInfoTrie, formatBuffer, weaponAttribs, 64);

	if (!validAttribsList)
		weaponAttribs = "";
	
	new String:weaponAttribsArray[32][32];
	new attribCount = ExplodeString(weaponAttribs, " ; ", weaponAttribsArray, 32, 32);
	
	new Handle:hWeapon = TF2Items_CreateItem(OVERRIDE_ATTRIBUTES);

	if (attribCount > 0) {
		TF2Items_SetNumAttributes(hWeapon, (attribCount/2)+1);
		new i2 = 0;
		for (new i = 0; i < attribCount; i+=2) {
			TF2Items_SetAttribute(hWeapon, i2, StringToInt(weaponAttribsArray[i]), StringToFloat(weaponAttribsArray[i+1]));
			i2++;
		}
		TF2Items_SetAttribute(hWeapon, i2, 134, 2.0);
	} else {
		TF2Items_SetNumAttributes(hWeapon, 1);
		TF2Items_SetAttribute(hWeapon, 0, 134, 2.0);
	}
	
	hItemOverride = hWeapon;
	return Plugin_Changed;
}

public Action:Command_FlameMe(client, args)
{
	g_hItems[client] = true;
}

public Action:Command_PutMeOut(client, args)
{
	g_hItems[client] = false;
}

CreateItemInfoTrie()
{
	g_hItemInfoTrie = CreateTrie();

	SetTrieString(g_hItemInfoTrie, "35_attribs", "18 ; 1.0 ; 10 ; 1.25");
	SetTrieString(g_hItemInfoTrie, "36_attribs", "16 ; 3.0 ; 129 ; -2.0");
	SetTrieString(g_hItemInfoTrie, "37_attribs", "17 ; 0.25 ; 5 ; 1.2");
	SetTrieString(g_hItemInfoTrie, "38_attribs", "20 ; 1.0 ; 21 ; 0.5 ; 22 ; 1.0");
	SetTrieString(g_hItemInfoTrie, "39_attribs", "25 ; 0.5");
	SetTrieString(g_hItemInfoTrie, "40_attribs", "23 ; 1.0 ; 24 ; 1.0 ; 28 ; 0.0");
	SetTrieString(g_hItemInfoTrie, "41_attribs", "32 ; 1.0 ; 1 ; 0.75");
	SetTrieString(g_hItemInfoTrie, "43_attribs", "31 ; 5.0 ; 5 ; 1.2");
	SetTrieString(g_hItemInfoTrie, "44_attribs", "38 ; 1.0 ; 125 ; -15.0");
	SetTrieString(g_hItemInfoTrie, "45_attribs", "44 ; 1.0 ; 6 ; 0.5 ; 45 ; 1.2 ; 1 ; 0.9 ; 3 ; 0.4 ; 43 ; 1.0");
	SetTrieString(g_hItemInfoTrie, "56_attribs", "37 ; 0.5");
	SetTrieString(g_hItemInfoTrie, "58_attribs", "56 ; 1.0");
	SetTrieString(g_hItemInfoTrie, "59_attribs", "33 ; 1.0 ; 34 ; 1.6 ; 35 ; 1.8");
	SetTrieString(g_hItemInfoTrie, "60_attribs", "48 ; 2.0 ; 35 ; 2.0");
	SetTrieString(g_hItemInfoTrie, "61_attribs", "51 ; 1.0 ; 1 ; 0.85 ; 5 ; 1.2");
	SetTrieString(g_hItemInfoTrie, "127_attribs", "100 ; 0.3 ; 103 ; 1.8 ; 2 ; 1.25 ; 114 ; 1.0");
	SetTrieString(g_hItemInfoTrie, "128_attribs", "115 ; 1.0");
	SetTrieString(g_hItemInfoTrie, "130_attribs", "119 ; 1.0 ; 121 ; 1.0 ; 78 ; 1.5 ; 88 ; 6.0 ; 120 ; 0.4");
	SetTrieString(g_hItemInfoTrie, "131_attribs", "60 ; 0.5 ; 64 ; 0.5");
	SetTrieString(g_hItemInfoTrie, "132_attribs", "15 ; 0.0 ; 125 ; -25.0");
}