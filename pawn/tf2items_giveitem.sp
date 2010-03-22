#pragma semicolon 1 // Force strict semicolon mode.

#include <sourcemod>
#define REQUIRE_EXTENSIONS
#include <tf2items>
#include <sdktools>

#define PLUGIN_NAME		"[TF2Items] Give Weapon"
#define PLUGIN_AUTHOR		"Asherkin"
#define PLUGIN_VERSION		"3.0.0"
#define PLUGIN_CONTACT		"http://limetech.org/"

new g_hItems[MAXPLAYERS+1][6];
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

	RegAdminCmd("sm_giveweapon", Command_Weapon, ADMFLAG_CHEATS, "sm_giveweapon <player> <itemindex>");
	RegAdminCmd("sm_giveweapon_ex", Command_WeaponEx, ADMFLAG_CHEATS, "sm_giveweapon_ex <player> <itemindex>");
	
	CreateItemInfoTrie();
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
		//PrintToChat(client, "No weapon for slot %d.", weaponSlot);
		return Plugin_Continue;
	}
	
	//PrintToChat(client, "Weapon in-queue for slot %d.", weaponSlot);
	hItemOverride = PrepareItemHandle(g_hItems[client][weaponSlot]);
	g_hItems[client][weaponSlot] = -1;
	
	return Plugin_Changed;
}

public Action:Command_WeaponEx(client, args)
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

public Action:Command_Weapon(client, args)
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

CreateItemInfoTrie()
{
	g_hItemInfoTrie = CreateTrie();

	SetTrieString(g_hItemInfoTrie, "0_classname", "tf_weapon_bat");
	SetTrieValue(g_hItemInfoTrie, "0_index", 0);
	SetTrieValue(g_hItemInfoTrie, "0_slot", 2);
	SetTrieValue(g_hItemInfoTrie, "0_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "0_level", 1);
	SetTrieString(g_hItemInfoTrie, "0_attribs", "");

	SetTrieString(g_hItemInfoTrie, "1_classname", "tf_weapon_bottle");
	SetTrieValue(g_hItemInfoTrie, "1_index", 1);
	SetTrieValue(g_hItemInfoTrie, "1_slot", 2);
	SetTrieValue(g_hItemInfoTrie, "1_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "1_level", 1);
	SetTrieString(g_hItemInfoTrie, "1_attribs", "");

	SetTrieString(g_hItemInfoTrie, "2_classname", "tf_weapon_fireaxe");
	SetTrieValue(g_hItemInfoTrie, "2_index", 2);
	SetTrieValue(g_hItemInfoTrie, "2_slot", 2);
	SetTrieValue(g_hItemInfoTrie, "2_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "2_level", 1);
	SetTrieString(g_hItemInfoTrie, "2_attribs", "");

	SetTrieString(g_hItemInfoTrie, "3_classname", "tf_weapon_club");
	SetTrieValue(g_hItemInfoTrie, "3_index", 3);
	SetTrieValue(g_hItemInfoTrie, "3_slot", 2);
	SetTrieValue(g_hItemInfoTrie, "3_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "3_level", 1);
	SetTrieString(g_hItemInfoTrie, "3_attribs", "");

	SetTrieString(g_hItemInfoTrie, "4_classname", "tf_weapon_knife");
	SetTrieValue(g_hItemInfoTrie, "4_index", 4);
	SetTrieValue(g_hItemInfoTrie, "4_slot", 2);
	SetTrieValue(g_hItemInfoTrie, "4_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "4_level", 1);
	SetTrieString(g_hItemInfoTrie, "4_attribs", "");

	SetTrieString(g_hItemInfoTrie, "5_classname", "tf_weapon_fists");
	SetTrieValue(g_hItemInfoTrie, "5_index", 5);
	SetTrieValue(g_hItemInfoTrie, "5_slot", 2);
	SetTrieValue(g_hItemInfoTrie, "5_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "5_level", 1);
	SetTrieString(g_hItemInfoTrie, "5_attribs", "");

	SetTrieString(g_hItemInfoTrie, "6_classname", "tf_weapon_shovel");
	SetTrieValue(g_hItemInfoTrie, "6_index", 6);
	SetTrieValue(g_hItemInfoTrie, "6_slot", 2);
	SetTrieValue(g_hItemInfoTrie, "6_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "6_level", 1);
	SetTrieString(g_hItemInfoTrie, "6_attribs", "");

	SetTrieString(g_hItemInfoTrie, "7_classname", "tf_weapon_wrench");
	SetTrieValue(g_hItemInfoTrie, "7_index", 7);
	SetTrieValue(g_hItemInfoTrie, "7_slot", 2);
	SetTrieValue(g_hItemInfoTrie, "7_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "7_level", 1);
	SetTrieString(g_hItemInfoTrie, "7_attribs", "");

	SetTrieString(g_hItemInfoTrie, "8_classname", "tf_weapon_bonesaw");
	SetTrieValue(g_hItemInfoTrie, "8_index", 8);
	SetTrieValue(g_hItemInfoTrie, "8_slot", 2);
	SetTrieValue(g_hItemInfoTrie, "8_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "8_level", 1);
	SetTrieString(g_hItemInfoTrie, "8_attribs", "");

	SetTrieString(g_hItemInfoTrie, "9_classname", "tf_weapon_shotgun_primary");
	SetTrieValue(g_hItemInfoTrie, "9_index", 9);
	SetTrieValue(g_hItemInfoTrie, "9_slot", 0);
	SetTrieValue(g_hItemInfoTrie, "9_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "9_level", 1);
	SetTrieString(g_hItemInfoTrie, "9_attribs", "");

	SetTrieString(g_hItemInfoTrie, "10_classname", "tf_weapon_shotgun_soldier");
	SetTrieValue(g_hItemInfoTrie, "10_index", 10);
	SetTrieValue(g_hItemInfoTrie, "10_slot", 1);
	SetTrieValue(g_hItemInfoTrie, "10_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "10_level", 1);
	SetTrieString(g_hItemInfoTrie, "10_attribs", "");

	SetTrieString(g_hItemInfoTrie, "11_classname", "tf_weapon_shotgun_hwg");
	SetTrieValue(g_hItemInfoTrie, "11_index", 11);
	SetTrieValue(g_hItemInfoTrie, "11_slot", 1);
	SetTrieValue(g_hItemInfoTrie, "11_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "11_level", 1);
	SetTrieString(g_hItemInfoTrie, "11_attribs", "");

	SetTrieString(g_hItemInfoTrie, "12_classname", "tf_weapon_shotgun_pyro");
	SetTrieValue(g_hItemInfoTrie, "12_index", 12);
	SetTrieValue(g_hItemInfoTrie, "12_slot", 1);
	SetTrieValue(g_hItemInfoTrie, "12_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "12_level", 1);
	SetTrieString(g_hItemInfoTrie, "12_attribs", "");

	SetTrieString(g_hItemInfoTrie, "13_classname", "tf_weapon_scattergun");
	SetTrieValue(g_hItemInfoTrie, "13_index", 13);
	SetTrieValue(g_hItemInfoTrie, "13_slot", 0);
	SetTrieValue(g_hItemInfoTrie, "13_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "13_level", 1);
	SetTrieString(g_hItemInfoTrie, "13_attribs", "");

	SetTrieString(g_hItemInfoTrie, "14_classname", "tf_weapon_sniperrifle");
	SetTrieValue(g_hItemInfoTrie, "14_index", 14);
	SetTrieValue(g_hItemInfoTrie, "14_slot", 0);
	SetTrieValue(g_hItemInfoTrie, "14_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "14_level", 1);
	SetTrieString(g_hItemInfoTrie, "14_attribs", "");

	SetTrieString(g_hItemInfoTrie, "15_classname", "tf_weapon_minigun");
	SetTrieValue(g_hItemInfoTrie, "15_index", 15);
	SetTrieValue(g_hItemInfoTrie, "15_slot", 0);
	SetTrieValue(g_hItemInfoTrie, "15_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "15_level", 1);
	SetTrieString(g_hItemInfoTrie, "15_attribs", "");

	SetTrieString(g_hItemInfoTrie, "16_classname", "tf_weapon_smg");
	SetTrieValue(g_hItemInfoTrie, "16_index", 16);
	SetTrieValue(g_hItemInfoTrie, "16_slot", 1);
	SetTrieValue(g_hItemInfoTrie, "16_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "16_level", 1);
	SetTrieString(g_hItemInfoTrie, "16_attribs", "");

	SetTrieString(g_hItemInfoTrie, "17_classname", "tf_weapon_syringegun_medic");
	SetTrieValue(g_hItemInfoTrie, "17_index", 17);
	SetTrieValue(g_hItemInfoTrie, "17_slot", 0);
	SetTrieValue(g_hItemInfoTrie, "17_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "17_level", 1);
	SetTrieString(g_hItemInfoTrie, "17_attribs", "");

	SetTrieString(g_hItemInfoTrie, "18_classname", "tf_weapon_rocketlauncher");
	SetTrieValue(g_hItemInfoTrie, "18_index", 18);
	SetTrieValue(g_hItemInfoTrie, "18_slot", 0);
	SetTrieValue(g_hItemInfoTrie, "18_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "18_level", 1);
	SetTrieString(g_hItemInfoTrie, "18_attribs", "");

	SetTrieString(g_hItemInfoTrie, "19_classname", "tf_weapon_grenadelauncher");
	SetTrieValue(g_hItemInfoTrie, "19_index", 19);
	SetTrieValue(g_hItemInfoTrie, "19_slot", 1);
	SetTrieValue(g_hItemInfoTrie, "19_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "19_level", 1);
	SetTrieString(g_hItemInfoTrie, "19_attribs", "");

	SetTrieString(g_hItemInfoTrie, "20_classname", "tf_weapon_pipebomblauncher");
	SetTrieValue(g_hItemInfoTrie, "20_index", 20);
	SetTrieValue(g_hItemInfoTrie, "20_slot", 0);
	SetTrieValue(g_hItemInfoTrie, "20_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "20_level", 1);
	SetTrieString(g_hItemInfoTrie, "20_attribs", "");

	SetTrieString(g_hItemInfoTrie, "21_classname", "tf_weapon_flamethrower");
	SetTrieValue(g_hItemInfoTrie, "21_index", 21);
	SetTrieValue(g_hItemInfoTrie, "21_slot", 0);
	SetTrieValue(g_hItemInfoTrie, "21_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "21_level", 1);
	SetTrieString(g_hItemInfoTrie, "21_attribs", "");

	SetTrieString(g_hItemInfoTrie, "22_classname", "tf_weapon_pistol");
	SetTrieValue(g_hItemInfoTrie, "22_index", 22);
	SetTrieValue(g_hItemInfoTrie, "22_slot", 1);
	SetTrieValue(g_hItemInfoTrie, "22_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "22_level", 1);
	SetTrieString(g_hItemInfoTrie, "22_attribs", "");

	SetTrieString(g_hItemInfoTrie, "23_classname", "tf_weapon_pistol_scout");
	SetTrieValue(g_hItemInfoTrie, "23_index", 23);
	SetTrieValue(g_hItemInfoTrie, "23_slot", 1);
	SetTrieValue(g_hItemInfoTrie, "23_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "23_level", 1);
	SetTrieString(g_hItemInfoTrie, "23_attribs", "");

	SetTrieString(g_hItemInfoTrie, "24_classname", "tf_weapon_revolver");
	SetTrieValue(g_hItemInfoTrie, "24_index", 24);
	SetTrieValue(g_hItemInfoTrie, "24_slot", 1);
	SetTrieValue(g_hItemInfoTrie, "24_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "24_level", 1);
	SetTrieString(g_hItemInfoTrie, "24_attribs", "");

	SetTrieString(g_hItemInfoTrie, "25_classname", "tf_weapon_pda_engineer_build");
	SetTrieValue(g_hItemInfoTrie, "25_index", 25);
	SetTrieValue(g_hItemInfoTrie, "25_slot", 3);
	SetTrieValue(g_hItemInfoTrie, "25_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "25_level", 1);
	SetTrieString(g_hItemInfoTrie, "25_attribs", "");

	SetTrieString(g_hItemInfoTrie, "26_classname", "tf_weapon_pda_engineer_destroy");
	SetTrieValue(g_hItemInfoTrie, "26_index", 26);
	SetTrieValue(g_hItemInfoTrie, "26_slot", 4);
	SetTrieValue(g_hItemInfoTrie, "26_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "26_level", 1);
	SetTrieString(g_hItemInfoTrie, "26_attribs", "");

	SetTrieString(g_hItemInfoTrie, "27_classname", "tf_weapon_pda_spy");
	SetTrieValue(g_hItemInfoTrie, "27_index", 27);
	SetTrieValue(g_hItemInfoTrie, "27_slot", 3);
	SetTrieValue(g_hItemInfoTrie, "27_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "27_level", 1);
	SetTrieString(g_hItemInfoTrie, "27_attribs", "");

	SetTrieString(g_hItemInfoTrie, "28_classname", "tf_weapon_builder");
	SetTrieValue(g_hItemInfoTrie, "28_index", 28);
	SetTrieValue(g_hItemInfoTrie, "28_slot", 5);
	SetTrieValue(g_hItemInfoTrie, "28_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "28_level", 1);
	SetTrieString(g_hItemInfoTrie, "28_attribs", "");

	SetTrieString(g_hItemInfoTrie, "29_classname", "tf_weapon_medigun");
	SetTrieValue(g_hItemInfoTrie, "29_index", 29);
	SetTrieValue(g_hItemInfoTrie, "29_slot", 1);
	SetTrieValue(g_hItemInfoTrie, "29_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "29_level", 1);
	SetTrieString(g_hItemInfoTrie, "29_attribs", "");

	SetTrieString(g_hItemInfoTrie, "30_classname", "tf_weapon_invis");
	SetTrieValue(g_hItemInfoTrie, "30_index", 30);
	SetTrieValue(g_hItemInfoTrie, "30_slot", 4);
	SetTrieValue(g_hItemInfoTrie, "30_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "30_level", 1);
	SetTrieString(g_hItemInfoTrie, "30_attribs", "");

	SetTrieString(g_hItemInfoTrie, "31_classname", "tf_weapon_flaregun");
	SetTrieValue(g_hItemInfoTrie, "31_index", 31);
	SetTrieValue(g_hItemInfoTrie, "31_slot", 1);
	SetTrieValue(g_hItemInfoTrie, "31_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "31_level", 1);
	SetTrieString(g_hItemInfoTrie, "31_attribs", "");

	SetTrieString(g_hItemInfoTrie, "35_classname", "tf_weapon_medigun");
	SetTrieValue(g_hItemInfoTrie, "35_index", 35);
	SetTrieValue(g_hItemInfoTrie, "35_slot", 1);
	SetTrieValue(g_hItemInfoTrie, "35_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "35_level", 8);
	SetTrieString(g_hItemInfoTrie, "35_attribs", "18 ; 1.0 ; 10 ; 1.25");

	SetTrieString(g_hItemInfoTrie, "36_classname", "tf_weapon_syringegun_medic");
	SetTrieValue(g_hItemInfoTrie, "36_index", 36);
	SetTrieValue(g_hItemInfoTrie, "36_slot", 0);
	SetTrieValue(g_hItemInfoTrie, "36_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "36_level", 5);
	SetTrieString(g_hItemInfoTrie, "36_attribs", "16 ; 3.0 ; 129 ; -2.0");

	SetTrieString(g_hItemInfoTrie, "37_classname", "tf_weapon_bonesaw");
	SetTrieValue(g_hItemInfoTrie, "37_index", 37);
	SetTrieValue(g_hItemInfoTrie, "37_slot", 2);
	SetTrieValue(g_hItemInfoTrie, "37_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "37_level", 10);
	SetTrieString(g_hItemInfoTrie, "37_attribs", "17 ; 0.25 ; 5 ; 1.2");

	SetTrieString(g_hItemInfoTrie, "38_classname", "tf_weapon_fireaxe");
	SetTrieValue(g_hItemInfoTrie, "38_index", 38);
	SetTrieValue(g_hItemInfoTrie, "38_slot", 2);
	SetTrieValue(g_hItemInfoTrie, "38_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "38_level", 10);
	SetTrieString(g_hItemInfoTrie, "38_attribs", "20 ; 1.0 ; 21 ; 0.5 ; 22 ; 1.0");

	SetTrieString(g_hItemInfoTrie, "39_classname", "tf_weapon_flaregun");
	SetTrieValue(g_hItemInfoTrie, "39_index", 39);
	SetTrieValue(g_hItemInfoTrie, "39_slot", 1);
	SetTrieValue(g_hItemInfoTrie, "39_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "39_level", 10);
	SetTrieString(g_hItemInfoTrie, "39_attribs", "25 ; 0.5");

	SetTrieString(g_hItemInfoTrie, "40_classname", "tf_weapon_flamethrower");
	SetTrieValue(g_hItemInfoTrie, "40_index", 40);
	SetTrieValue(g_hItemInfoTrie, "40_slot", 0);
	SetTrieValue(g_hItemInfoTrie, "40_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "40_level", 10);
	SetTrieString(g_hItemInfoTrie, "40_attribs", "23 ; 1.0 ; 24 ; 1.0 ; 28 ; 0.0");

	SetTrieString(g_hItemInfoTrie, "41_classname", "tf_weapon_minigun");
	SetTrieValue(g_hItemInfoTrie, "41_index", 41);
	SetTrieValue(g_hItemInfoTrie, "41_slot", 0);
	SetTrieValue(g_hItemInfoTrie, "41_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "41_level", 5);
	SetTrieString(g_hItemInfoTrie, "41_attribs", "32 ; 1.0 ; 1 ; 0.75");

	SetTrieString(g_hItemInfoTrie, "42_classname", "tf_weapon_lunchbox");
	SetTrieValue(g_hItemInfoTrie, "42_index", 42);
	SetTrieValue(g_hItemInfoTrie, "42_slot", 1);
	SetTrieValue(g_hItemInfoTrie, "42_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "42_level", 1);
	SetTrieString(g_hItemInfoTrie, "42_attribs", "");

	SetTrieString(g_hItemInfoTrie, "43_classname", "tf_weapon_fists");
	SetTrieValue(g_hItemInfoTrie, "43_index", 43);
	SetTrieValue(g_hItemInfoTrie, "43_slot", 2);
	SetTrieValue(g_hItemInfoTrie, "43_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "43_level", 7);
	SetTrieString(g_hItemInfoTrie, "43_attribs", "31 ; 5.0 ; 5 ; 1.2");

	SetTrieString(g_hItemInfoTrie, "44_classname", "tf_weapon_bat_wood");
	SetTrieValue(g_hItemInfoTrie, "44_index", 44);
	SetTrieValue(g_hItemInfoTrie, "44_slot", 2);
	SetTrieValue(g_hItemInfoTrie, "44_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "44_level", 15);
	SetTrieString(g_hItemInfoTrie, "44_attribs", "38 ; 1.0 ; 125 ; -15.0");

	SetTrieString(g_hItemInfoTrie, "45_classname", "tf_weapon_scattergun");
	SetTrieValue(g_hItemInfoTrie, "45_index", 45);
	SetTrieValue(g_hItemInfoTrie, "45_slot", 0);
	SetTrieValue(g_hItemInfoTrie, "45_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "45_level", 10);
	SetTrieString(g_hItemInfoTrie, "45_attribs", "44 ; 1.0 ; 6 ; 0.5 ; 45 ; 1.2 ; 1 ; 0.9 ; 3 ; 0.4 ; 43 ; 1.0");

	SetTrieString(g_hItemInfoTrie, "46_classname", "tf_weapon_lunchbox_drink");
	SetTrieValue(g_hItemInfoTrie, "46_index", 46);
	SetTrieValue(g_hItemInfoTrie, "46_slot", 1);
	SetTrieValue(g_hItemInfoTrie, "46_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "46_level", 5);
	SetTrieString(g_hItemInfoTrie, "46_attribs", "");

	SetTrieString(g_hItemInfoTrie, "56_classname", "tf_weapon_compound_bow");
	SetTrieValue(g_hItemInfoTrie, "56_index", 56);
	SetTrieValue(g_hItemInfoTrie, "56_slot", 0);
	SetTrieValue(g_hItemInfoTrie, "56_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "56_level", 10);
	SetTrieString(g_hItemInfoTrie, "56_attribs", "37 ; 0.5");

	SetTrieString(g_hItemInfoTrie, "58_classname", "tf_weapon_jar");
	SetTrieValue(g_hItemInfoTrie, "58_index", 58);
	SetTrieValue(g_hItemInfoTrie, "58_slot", 1);
	SetTrieValue(g_hItemInfoTrie, "58_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "58_level", 5);
	SetTrieString(g_hItemInfoTrie, "58_attribs", "56 ; 1.0");

	SetTrieString(g_hItemInfoTrie, "59_classname", "tf_weapon_invis");
	SetTrieValue(g_hItemInfoTrie, "59_index", 59);
	SetTrieValue(g_hItemInfoTrie, "59_slot", 4);
	SetTrieValue(g_hItemInfoTrie, "59_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "59_level", 5);
	SetTrieString(g_hItemInfoTrie, "59_attribs", "33 ; 1.0 ; 34 ; 1.6 ; 35 ; 1.8");

	SetTrieString(g_hItemInfoTrie, "60_classname", "tf_weapon_invis");
	SetTrieValue(g_hItemInfoTrie, "60_index", 60);
	SetTrieValue(g_hItemInfoTrie, "60_slot", 4);
	SetTrieValue(g_hItemInfoTrie, "60_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "60_level", 5);
	SetTrieString(g_hItemInfoTrie, "60_attribs", "48 ; 2.0 ; 35 ; 2.0");

	SetTrieString(g_hItemInfoTrie, "61_classname", "tf_weapon_revolver");
	SetTrieValue(g_hItemInfoTrie, "61_index", 61);
	SetTrieValue(g_hItemInfoTrie, "61_slot", 1);
	SetTrieValue(g_hItemInfoTrie, "61_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "61_level", 5);
	SetTrieString(g_hItemInfoTrie, "61_attribs", "51 ; 1.0 ; 1 ; 0.85 ; 5 ; 1.2");

	SetTrieString(g_hItemInfoTrie, "127_classname", "tf_weapon_rocketlauncher_directhit");
	SetTrieValue(g_hItemInfoTrie, "127_index", 127);
	SetTrieValue(g_hItemInfoTrie, "127_slot", 0);
	SetTrieValue(g_hItemInfoTrie, "127_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "127_level", 1);
	SetTrieString(g_hItemInfoTrie, "127_attribs", "100 ; 0.3 ; 103 ; 1.8 ; 2 ; 1.25 ; 114 ; 1.0");

	SetTrieString(g_hItemInfoTrie, "128_classname", "tf_weapon_shovel");
	SetTrieValue(g_hItemInfoTrie, "128_index", 128);
	SetTrieValue(g_hItemInfoTrie, "128_slot", 2);
	SetTrieValue(g_hItemInfoTrie, "128_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "128_level", 10);
	SetTrieString(g_hItemInfoTrie, "128_attribs", "115 ; 1.0");

	SetTrieString(g_hItemInfoTrie, "129_classname", "tf_weapon_buff_item");
	SetTrieValue(g_hItemInfoTrie, "129_index", 129);
	SetTrieValue(g_hItemInfoTrie, "129_slot", 1);
	SetTrieValue(g_hItemInfoTrie, "129_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "129_level", 5);
	SetTrieString(g_hItemInfoTrie, "129_attribs", "");

	SetTrieString(g_hItemInfoTrie, "130_classname", "tf_weapon_pipebomblauncher");
	SetTrieValue(g_hItemInfoTrie, "130_index", 130);
	SetTrieValue(g_hItemInfoTrie, "130_slot", 0);
	SetTrieValue(g_hItemInfoTrie, "130_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "130_level", 5);
	SetTrieString(g_hItemInfoTrie, "130_attribs", "119 ; 1.0 ; 121 ; 1.0 ; 78 ; 1.5 ; 88 ; 6.0 ; 120 ; 0.4");

	SetTrieString(g_hItemInfoTrie, "132_classname", "tf_weapon_sword");
	SetTrieValue(g_hItemInfoTrie, "132_index", 132);
	SetTrieValue(g_hItemInfoTrie, "132_slot", 2);
	SetTrieValue(g_hItemInfoTrie, "132_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "132_level", 5);
	SetTrieString(g_hItemInfoTrie, "132_attribs", "15 ; 0.0 ; 125 ; -25.0");
}