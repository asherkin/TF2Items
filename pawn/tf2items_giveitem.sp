#pragma semicolon 1 // Force strict semicolon mode.

#include <sourcemod>
#define REQUIRE_EXTENSIONS
#include <tf2items>
#include <sdktools>

#define PLUGIN_NAME		"[TF2Items] Give Weapon"
#define PLUGIN_AUTHOR		"Asherkin (updates by FlaminSarge)"
#define PLUGIN_VERSION		"1.4.1"
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

public OnPluginStart()
{
	LoadTranslations("common.phrases");

	RegAdminCmd("sm_giveweapon", Command_Weapon, ADMFLAG_CHEATS, "sm_giveweapon <player> <itemindex>");
	RegAdminCmd("sm_giveweapon_ex", Command_WeaponEx, ADMFLAG_CHEATS, "sm_giveweapon_ex <player> <itemindex>");
	RegAdminCmd("sm_ludmila", Command_GiveLudmila, ADMFLAG_GENERIC, "Give Ludmila to yourself using sm_ludmila");
	RegAdminCmd("sm_glovesofrunning", Command_GiveGlovesofRunning, ADMFLAG_GENERIC, "Give the Gloves of Running Urgently to yourself using sm_glovesofrunning");
	RegAdminCmd("sm_spycrabpda", Command_GiveSpycrabPDA, ADMFLAG_CHEATS, "Give the Spycrab PDA to yourself with sm_spycrabpda");

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

public OnClientDisconnect_Post(client)
{
	for (new i = 0; i < 6; i++)
	{
		if (g_hItems[client][i] != -1)
		{
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
	
	if (g_hItems[client][weaponSlot] == -1)
	{
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
 
	if (args != 2)
	{
		ReplyToCommand(client, "[TF2Items] Usage: sm_giveweapon_ex <player> <itemindex>");
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
			ReplyToCommand(client, "[TF2Items] Invalid Weapon Index");
			return Plugin_Handled;
		}
		
		PrintToChat(target_list[i], "[TF2Items] Respawn or touch a locker to recieve your new weapon.");
		g_hItems[target_list[i]][weaponSlot] = weaponLookupIndex;
		
		LogAction(client, target_list[i], "\"%L\" gave a weapon to \"%L\"", client, target_list[i]);
	}
 
	if (tn_is_ml) {
		ShowActivity2(client, "[TF2Items] ", "%t was given a weapon!", target_name);
	} else {
		ShowActivity2(client, "[TF2Items] ", "%s was given a weapon!", target_name);
	}
 
	return Plugin_Handled;
}

public Action:Command_Weapon(client, args)
{
	new String:arg1[32];
	new String:arg2[32];
	new weaponLookupIndex = 0;
 
	if (args != 2) {
		ReplyToCommand(client, "[TF2Items] Usage: sm_giveweapon <player> <itemindex>");
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
			ReplyToCommand(client, "[TF2Items] Invalid Weapon Index");
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
		}
		else
		{
			PrintToChat(target_list[i], "[TF2Items] Respawn or touch a locker to recieve your new weapon.");
			g_hItems[target_list[i]][weaponSlot] = weaponLookupIndex;
		}
		
		LogAction(client, target_list[i], "\"%L\" gave a weapon to \"%L\"", client, target_list[i]);
	}
 
	if (tn_is_ml) {
		ShowActivity2(client, "[TF2Items] ", "%t was given a weapon!", target_name);
	} else {
		ShowActivity2(client, "[TF2Items] ", "%s was given a weapon!", target_name);
	}
 
	return Plugin_Handled;
}

public Action:Command_GiveLudmila(client, args)
{	
	if (args == 0)
	{
		if (client == 0)
		{
			ReplyToCommand(client, "[TF2Items] Cannot give Ludmila to console");
			return Plugin_Handled;
		}
		if(IsClientInGame(client) && IsPlayerAlive(client))
		{
			ServerCommand("sm_giveweapon #%d 2041", GetClientUserId(client));
			return Plugin_Handled;
		}
		else
		{
			ReplyToCommand(client, "[TF2Items] Cannot give Ludmila yet, wait to spawn");
			return Plugin_Handled;
		}
	}
	return Plugin_Continue;
}

public Action:Command_GiveGlovesofRunning(client, args)
{	
	if (args == 0)
	{
		if (client == 0)
		{
			ReplyToCommand(client, "[TF2Items] Cannot give Gloves of Running Urgently to console");
			return Plugin_Handled;
		}
		if(IsClientInGame(client) && IsPlayerAlive(client))
		{
			ServerCommand("sm_giveweapon #%d 2043", GetClientUserId(client));
			return Plugin_Handled;
		}
		else
		{
			ReplyToCommand(client, "[TF2Items] Cannot give Gloves of Running Urgently yet, wait to spawn");
			return Plugin_Handled;
		}
	}
	return Plugin_Continue;
}

public Action:Command_GiveSpycrabPDA(client, args)
{	
	if (args == 0)
	{
		if (client == 0)
		{
			ReplyToCommand(client, "[TF2Items] Cannot give Spycrab PDA to console");
			return Plugin_Handled;
		}
		if(IsClientInGame(client) && IsPlayerAlive(client))
		{
			ServerCommand("sm_giveweapon #%d 9027", GetClientUserId(client));
			return Plugin_Handled;	
		}
		else
		{
			ReplyToCommand(client, "[TF2Items] Cannot give Spycrab PDA yet, wait to spawn");
			return Plugin_Handled;
		}
	}
	return Plugin_Continue;
}

Handle:PrepareItemHandle(weaponLookupIndex)
{
	new String:formatBuffer[32];	
	new String:weaponClassname[64];
	new weaponIndex;
	new weaponSlot;
	new weaponQuality;
	new weaponLevel;
	new String:weaponAttribs[256];
	
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
	GetTrieString(g_hItemInfoTrie, formatBuffer, weaponAttribs, 256);
	
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

//bat
	SetTrieString(g_hItemInfoTrie, "0_classname", "tf_weapon_bat");
	SetTrieValue(g_hItemInfoTrie, "0_index", 0);
	SetTrieValue(g_hItemInfoTrie, "0_slot", 2);
	SetTrieValue(g_hItemInfoTrie, "0_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "0_level", 1);
	SetTrieString(g_hItemInfoTrie, "0_attribs", "");

//bottle
	SetTrieString(g_hItemInfoTrie, "1_classname", "tf_weapon_bottle");
	SetTrieValue(g_hItemInfoTrie, "1_index", 1);
	SetTrieValue(g_hItemInfoTrie, "1_slot", 2);
	SetTrieValue(g_hItemInfoTrie, "1_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "1_level", 1);
	SetTrieString(g_hItemInfoTrie, "1_attribs", "");

//fire axe
	SetTrieString(g_hItemInfoTrie, "2_classname", "tf_weapon_fireaxe");
	SetTrieValue(g_hItemInfoTrie, "2_index", 2);
	SetTrieValue(g_hItemInfoTrie, "2_slot", 2);
	SetTrieValue(g_hItemInfoTrie, "2_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "2_level", 1);
	SetTrieString(g_hItemInfoTrie, "2_attribs", "");

//kukri
	SetTrieString(g_hItemInfoTrie, "3_classname", "tf_weapon_club");
	SetTrieValue(g_hItemInfoTrie, "3_index", 3);
	SetTrieValue(g_hItemInfoTrie, "3_slot", 2);
	SetTrieValue(g_hItemInfoTrie, "3_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "3_level", 1);
	SetTrieString(g_hItemInfoTrie, "3_attribs", "");

//knife
	SetTrieString(g_hItemInfoTrie, "4_classname", "tf_weapon_knife");
	SetTrieValue(g_hItemInfoTrie, "4_index", 4);
	SetTrieValue(g_hItemInfoTrie, "4_slot", 2);
	SetTrieValue(g_hItemInfoTrie, "4_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "4_level", 1);
	SetTrieString(g_hItemInfoTrie, "4_attribs", "");

//fists
	SetTrieString(g_hItemInfoTrie, "5_classname", "tf_weapon_fists");
	SetTrieValue(g_hItemInfoTrie, "5_index", 5);
	SetTrieValue(g_hItemInfoTrie, "5_slot", 2);
	SetTrieValue(g_hItemInfoTrie, "5_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "5_level", 1);
	SetTrieString(g_hItemInfoTrie, "5_attribs", "");

//shovel
	SetTrieString(g_hItemInfoTrie, "6_classname", "tf_weapon_shovel");
	SetTrieValue(g_hItemInfoTrie, "6_index", 6);
	SetTrieValue(g_hItemInfoTrie, "6_slot", 2);
	SetTrieValue(g_hItemInfoTrie, "6_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "6_level", 1);
	SetTrieString(g_hItemInfoTrie, "6_attribs", "");

//wrench
	SetTrieString(g_hItemInfoTrie, "7_classname", "tf_weapon_wrench");
	SetTrieValue(g_hItemInfoTrie, "7_index", 7);
	SetTrieValue(g_hItemInfoTrie, "7_slot", 2);
	SetTrieValue(g_hItemInfoTrie, "7_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "7_level", 1);
	SetTrieString(g_hItemInfoTrie, "7_attribs", "");

//bonesaw
	SetTrieString(g_hItemInfoTrie, "8_classname", "tf_weapon_bonesaw");
	SetTrieValue(g_hItemInfoTrie, "8_index", 8);
	SetTrieValue(g_hItemInfoTrie, "8_slot", 2);
	SetTrieValue(g_hItemInfoTrie, "8_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "8_level", 1);
	SetTrieString(g_hItemInfoTrie, "8_attribs", "");

//shotgun engineer
	SetTrieString(g_hItemInfoTrie, "9_classname", "tf_weapon_shotgun_primary");
	SetTrieValue(g_hItemInfoTrie, "9_index", 9);
	SetTrieValue(g_hItemInfoTrie, "9_slot", 0);
	SetTrieValue(g_hItemInfoTrie, "9_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "9_level", 1);
	SetTrieString(g_hItemInfoTrie, "9_attribs", "");

//shotgun soldier
	SetTrieString(g_hItemInfoTrie, "10_classname", "tf_weapon_shotgun_soldier");
	SetTrieValue(g_hItemInfoTrie, "10_index", 10);
	SetTrieValue(g_hItemInfoTrie, "10_slot", 1);
	SetTrieValue(g_hItemInfoTrie, "10_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "10_level", 1);
	SetTrieString(g_hItemInfoTrie, "10_attribs", "");

//shotgun heavy
	SetTrieString(g_hItemInfoTrie, "11_classname", "tf_weapon_shotgun_hwg");
	SetTrieValue(g_hItemInfoTrie, "11_index", 11);
	SetTrieValue(g_hItemInfoTrie, "11_slot", 1);
	SetTrieValue(g_hItemInfoTrie, "11_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "11_level", 1);
	SetTrieString(g_hItemInfoTrie, "11_attribs", "");

//shotgun pyro
	SetTrieString(g_hItemInfoTrie, "12_classname", "tf_weapon_shotgun_pyro");
	SetTrieValue(g_hItemInfoTrie, "12_index", 12);
	SetTrieValue(g_hItemInfoTrie, "12_slot", 1);
	SetTrieValue(g_hItemInfoTrie, "12_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "12_level", 1);
	SetTrieString(g_hItemInfoTrie, "12_attribs", "");

//scattergun
	SetTrieString(g_hItemInfoTrie, "13_classname", "tf_weapon_scattergun");
	SetTrieValue(g_hItemInfoTrie, "13_index", 13);
	SetTrieValue(g_hItemInfoTrie, "13_slot", 0);
	SetTrieValue(g_hItemInfoTrie, "13_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "13_level", 1);
	SetTrieString(g_hItemInfoTrie, "13_attribs", "");

//sniper rifle
	SetTrieString(g_hItemInfoTrie, "14_classname", "tf_weapon_sniperrifle");
	SetTrieValue(g_hItemInfoTrie, "14_index", 14);
	SetTrieValue(g_hItemInfoTrie, "14_slot", 0);
	SetTrieValue(g_hItemInfoTrie, "14_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "14_level", 1);
	SetTrieString(g_hItemInfoTrie, "14_attribs", "");

//minigun
	SetTrieString(g_hItemInfoTrie, "15_classname", "tf_weapon_minigun");
	SetTrieValue(g_hItemInfoTrie, "15_index", 15);
	SetTrieValue(g_hItemInfoTrie, "15_slot", 0);
	SetTrieValue(g_hItemInfoTrie, "15_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "15_level", 1);
	SetTrieString(g_hItemInfoTrie, "15_attribs", "");

//smg
	SetTrieString(g_hItemInfoTrie, "16_classname", "tf_weapon_smg");
	SetTrieValue(g_hItemInfoTrie, "16_index", 16);
	SetTrieValue(g_hItemInfoTrie, "16_slot", 1);
	SetTrieValue(g_hItemInfoTrie, "16_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "16_level", 1);
	SetTrieString(g_hItemInfoTrie, "16_attribs", "");

//syringe gun
	SetTrieString(g_hItemInfoTrie, "17_classname", "tf_weapon_syringegun_medic");
	SetTrieValue(g_hItemInfoTrie, "17_index", 17);
	SetTrieValue(g_hItemInfoTrie, "17_slot", 0);
	SetTrieValue(g_hItemInfoTrie, "17_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "17_level", 1);
	SetTrieString(g_hItemInfoTrie, "17_attribs", "");

//rocket launcher
	SetTrieString(g_hItemInfoTrie, "18_classname", "tf_weapon_rocketlauncher");
	SetTrieValue(g_hItemInfoTrie, "18_index", 18);
	SetTrieValue(g_hItemInfoTrie, "18_slot", 0);
	SetTrieValue(g_hItemInfoTrie, "18_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "18_level", 1);
	SetTrieString(g_hItemInfoTrie, "18_attribs", "");

//grenade launcher
	SetTrieString(g_hItemInfoTrie, "19_classname", "tf_weapon_grenadelauncher");
	SetTrieValue(g_hItemInfoTrie, "19_index", 19);
	SetTrieValue(g_hItemInfoTrie, "19_slot", 0);
	SetTrieValue(g_hItemInfoTrie, "19_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "19_level", 1);
	SetTrieString(g_hItemInfoTrie, "19_attribs", "");

//sticky launcher
	SetTrieString(g_hItemInfoTrie, "20_classname", "tf_weapon_pipebomblauncher");
	SetTrieValue(g_hItemInfoTrie, "20_index", 20);
	SetTrieValue(g_hItemInfoTrie, "20_slot", 1);
	SetTrieValue(g_hItemInfoTrie, "20_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "20_level", 1);
	SetTrieString(g_hItemInfoTrie, "20_attribs", "");

//flamethrower
	SetTrieString(g_hItemInfoTrie, "21_classname", "tf_weapon_flamethrower");
	SetTrieValue(g_hItemInfoTrie, "21_index", 21);
	SetTrieValue(g_hItemInfoTrie, "21_slot", 0);
	SetTrieValue(g_hItemInfoTrie, "21_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "21_level", 1);
	SetTrieString(g_hItemInfoTrie, "21_attribs", "");

//pistol engineer
	SetTrieString(g_hItemInfoTrie, "22_classname", "tf_weapon_pistol");
	SetTrieValue(g_hItemInfoTrie, "22_index", 22);
	SetTrieValue(g_hItemInfoTrie, "22_slot", 1);
	SetTrieValue(g_hItemInfoTrie, "22_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "22_level", 1);
	SetTrieString(g_hItemInfoTrie, "22_attribs", "");

//pistol scout
	SetTrieString(g_hItemInfoTrie, "23_classname", "tf_weapon_pistol_scout");
	SetTrieValue(g_hItemInfoTrie, "23_index", 23);
	SetTrieValue(g_hItemInfoTrie, "23_slot", 1);
	SetTrieValue(g_hItemInfoTrie, "23_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "23_level", 1);
	SetTrieString(g_hItemInfoTrie, "23_attribs", "");

//revolver
	SetTrieString(g_hItemInfoTrie, "24_classname", "tf_weapon_revolver");
	SetTrieValue(g_hItemInfoTrie, "24_index", 24);
	SetTrieValue(g_hItemInfoTrie, "24_slot", 0);
	SetTrieValue(g_hItemInfoTrie, "24_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "24_level", 1);
	SetTrieString(g_hItemInfoTrie, "24_attribs", "");

//build pda engineer
	SetTrieString(g_hItemInfoTrie, "25_classname", "tf_weapon_pda_engineer_build");
	SetTrieValue(g_hItemInfoTrie, "25_index", 25);
	SetTrieValue(g_hItemInfoTrie, "25_slot", 3);
	SetTrieValue(g_hItemInfoTrie, "25_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "25_level", 1);
	SetTrieString(g_hItemInfoTrie, "25_attribs", "");

//destroy pda engineer
	SetTrieString(g_hItemInfoTrie, "26_classname", "tf_weapon_pda_engineer_destroy");
	SetTrieValue(g_hItemInfoTrie, "26_index", 26);
	SetTrieValue(g_hItemInfoTrie, "26_slot", 4);
	SetTrieValue(g_hItemInfoTrie, "26_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "26_level", 1);
	SetTrieString(g_hItemInfoTrie, "26_attribs", "");

//disguise kit spy
	SetTrieString(g_hItemInfoTrie, "27_classname", "tf_weapon_pda_spy");
	SetTrieValue(g_hItemInfoTrie, "27_index", 27);
	SetTrieValue(g_hItemInfoTrie, "27_slot", 3);
	SetTrieValue(g_hItemInfoTrie, "27_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "27_level", 1);
	SetTrieString(g_hItemInfoTrie, "27_attribs", "");

//builder
	SetTrieString(g_hItemInfoTrie, "28_classname", "tf_weapon_builder");
	SetTrieValue(g_hItemInfoTrie, "28_index", 28);
	SetTrieValue(g_hItemInfoTrie, "28_slot", 5);
	SetTrieValue(g_hItemInfoTrie, "28_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "28_level", 1);
	SetTrieString(g_hItemInfoTrie, "28_attribs", "");

//medigun
	SetTrieString(g_hItemInfoTrie, "29_classname", "tf_weapon_medigun");
	SetTrieValue(g_hItemInfoTrie, "29_index", 29);
	SetTrieValue(g_hItemInfoTrie, "29_slot", 1);
	SetTrieValue(g_hItemInfoTrie, "29_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "29_level", 1);
	SetTrieString(g_hItemInfoTrie, "29_attribs", "");

//invis watch
	SetTrieString(g_hItemInfoTrie, "30_classname", "tf_weapon_invis");
	SetTrieValue(g_hItemInfoTrie, "30_index", 30);
	SetTrieValue(g_hItemInfoTrie, "30_slot", 4);
	SetTrieValue(g_hItemInfoTrie, "30_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "30_level", 1);
	SetTrieString(g_hItemInfoTrie, "30_attribs", "");

//flaregun engineerpistol
	SetTrieString(g_hItemInfoTrie, "31_classname", "tf_weapon_flaregun");
	SetTrieValue(g_hItemInfoTrie, "31_index", 31);
	SetTrieValue(g_hItemInfoTrie, "31_slot", 1);
	SetTrieValue(g_hItemInfoTrie, "31_quality", 0);
	SetTrieValue(g_hItemInfoTrie, "31_level", 1);
	SetTrieString(g_hItemInfoTrie, "31_attribs", "");

//kritzkrieg
	SetTrieString(g_hItemInfoTrie, "35_classname", "tf_weapon_medigun");
	SetTrieValue(g_hItemInfoTrie, "35_index", 35);
	SetTrieValue(g_hItemInfoTrie, "35_slot", 1);
	SetTrieValue(g_hItemInfoTrie, "35_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "35_level", 8);
	SetTrieString(g_hItemInfoTrie, "35_attribs", "18 ; 1.0 ; 10 ; 1.25");

//blutsauger
	SetTrieString(g_hItemInfoTrie, "36_classname", "tf_weapon_syringegun_medic");
	SetTrieValue(g_hItemInfoTrie, "36_index", 36);
	SetTrieValue(g_hItemInfoTrie, "36_slot", 0);
	SetTrieValue(g_hItemInfoTrie, "36_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "36_level", 5);
	SetTrieString(g_hItemInfoTrie, "36_attribs", "16 ; 3.0 ; 129 ; -2.0");

//ubersaw
	SetTrieString(g_hItemInfoTrie, "37_classname", "tf_weapon_bonesaw");
	SetTrieValue(g_hItemInfoTrie, "37_index", 37);
	SetTrieValue(g_hItemInfoTrie, "37_slot", 2);
	SetTrieValue(g_hItemInfoTrie, "37_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "37_level", 10);
	SetTrieString(g_hItemInfoTrie, "37_attribs", "17 ; 0.25 ; 5 ; 1.2");

//axetinguisher
	SetTrieString(g_hItemInfoTrie, "38_classname", "tf_weapon_fireaxe");
	SetTrieValue(g_hItemInfoTrie, "38_index", 38);
	SetTrieValue(g_hItemInfoTrie, "38_slot", 2);
	SetTrieValue(g_hItemInfoTrie, "38_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "38_level", 10);
	SetTrieString(g_hItemInfoTrie, "38_attribs", "20 ; 1.0 ; 21 ; 0.5 ; 22 ; 1.0");

//flaregun pyro
	SetTrieString(g_hItemInfoTrie, "39_classname", "tf_weapon_flaregun");
	SetTrieValue(g_hItemInfoTrie, "39_index", 39);
	SetTrieValue(g_hItemInfoTrie, "39_slot", 1);
	SetTrieValue(g_hItemInfoTrie, "39_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "39_level", 10);
	SetTrieString(g_hItemInfoTrie, "39_attribs", "25 ; 0.5");

//backburner
	SetTrieString(g_hItemInfoTrie, "40_classname", "tf_weapon_flamethrower");
	SetTrieValue(g_hItemInfoTrie, "40_index", 40);
	SetTrieValue(g_hItemInfoTrie, "40_slot", 0);
	SetTrieValue(g_hItemInfoTrie, "40_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "40_level", 10);
	SetTrieString(g_hItemInfoTrie, "40_attribs", "23 ; 1.0 ; 24 ; 1.0 ; 28 ; 0.0");

//natascha
	SetTrieString(g_hItemInfoTrie, "41_classname", "tf_weapon_minigun");
	SetTrieValue(g_hItemInfoTrie, "41_index", 41);
	SetTrieValue(g_hItemInfoTrie, "41_slot", 0);
	SetTrieValue(g_hItemInfoTrie, "41_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "41_level", 5);
	SetTrieString(g_hItemInfoTrie, "41_attribs", "32 ; 1.0 ; 1 ; 0.75");

//sandvich
	SetTrieString(g_hItemInfoTrie, "42_classname", "tf_weapon_lunchbox");
	SetTrieValue(g_hItemInfoTrie, "42_index", 42);
	SetTrieValue(g_hItemInfoTrie, "42_slot", 1);
	SetTrieValue(g_hItemInfoTrie, "42_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "42_level", 1);
	SetTrieString(g_hItemInfoTrie, "42_attribs", "");

//killing gloves of boxing
	SetTrieString(g_hItemInfoTrie, "43_classname", "tf_weapon_fists");
	SetTrieValue(g_hItemInfoTrie, "43_index", 43);
	SetTrieValue(g_hItemInfoTrie, "43_slot", 2);
	SetTrieValue(g_hItemInfoTrie, "43_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "43_level", 7);
	SetTrieString(g_hItemInfoTrie, "43_attribs", "31 ; 5.0 ; 5 ; 1.2");

//sandman
	SetTrieString(g_hItemInfoTrie, "44_classname", "tf_weapon_bat_wood");
	SetTrieValue(g_hItemInfoTrie, "44_index", 44);
	SetTrieValue(g_hItemInfoTrie, "44_slot", 2);
	SetTrieValue(g_hItemInfoTrie, "44_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "44_level", 15);
	SetTrieString(g_hItemInfoTrie, "44_attribs", "38 ; 1.0 ; 125 ; -15.0");

//force a nature
	SetTrieString(g_hItemInfoTrie, "45_classname", "tf_weapon_scattergun");
	SetTrieValue(g_hItemInfoTrie, "45_index", 45);
	SetTrieValue(g_hItemInfoTrie, "45_slot", 0);
	SetTrieValue(g_hItemInfoTrie, "45_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "45_level", 10);
	SetTrieString(g_hItemInfoTrie, "45_attribs", "44 ; 1.0 ; 6 ; 0.5 ; 45 ; 1.2 ; 1 ; 0.9 ; 3 ; 0.4 ; 43 ; 1.0");

//bonk atomic punch
	SetTrieString(g_hItemInfoTrie, "46_classname", "tf_weapon_lunchbox_drink");
	SetTrieValue(g_hItemInfoTrie, "46_index", 46);
	SetTrieValue(g_hItemInfoTrie, "46_slot", 1);
	SetTrieValue(g_hItemInfoTrie, "46_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "46_level", 5);
	SetTrieString(g_hItemInfoTrie, "46_attribs", "");

//hunstman
	SetTrieString(g_hItemInfoTrie, "56_classname", "tf_weapon_compound_bow");
	SetTrieValue(g_hItemInfoTrie, "56_index", 56);
	SetTrieValue(g_hItemInfoTrie, "56_slot", 0);
	SetTrieValue(g_hItemInfoTrie, "56_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "56_level", 10);
	SetTrieString(g_hItemInfoTrie, "56_attribs", "37 ; 0.5");

//jarate
	SetTrieString(g_hItemInfoTrie, "58_classname", "tf_weapon_jar");
	SetTrieValue(g_hItemInfoTrie, "58_index", 58);
	SetTrieValue(g_hItemInfoTrie, "58_slot", 1);
	SetTrieValue(g_hItemInfoTrie, "58_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "58_level", 5);
	SetTrieString(g_hItemInfoTrie, "58_attribs", "56 ; 1.0");

//dead ringer
	SetTrieString(g_hItemInfoTrie, "59_classname", "tf_weapon_invis");
	SetTrieValue(g_hItemInfoTrie, "59_index", 59);
	SetTrieValue(g_hItemInfoTrie, "59_slot", 4);
	SetTrieValue(g_hItemInfoTrie, "59_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "59_level", 5);
	SetTrieString(g_hItemInfoTrie, "59_attribs", "33 ; 1.0 ; 34 ; 1.6 ; 35 ; 1.8");

//cloak and dagger
	SetTrieString(g_hItemInfoTrie, "60_classname", "tf_weapon_invis");
	SetTrieValue(g_hItemInfoTrie, "60_index", 60);
	SetTrieValue(g_hItemInfoTrie, "60_slot", 4);
	SetTrieValue(g_hItemInfoTrie, "60_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "60_level", 5);
	SetTrieString(g_hItemInfoTrie, "60_attribs", "48 ; 2.0 ; 35 ; 2.0");

//ambassador
	SetTrieString(g_hItemInfoTrie, "61_classname", "tf_weapon_revolver");
	SetTrieValue(g_hItemInfoTrie, "61_index", 61);
	SetTrieValue(g_hItemInfoTrie, "61_slot", 0);
	SetTrieValue(g_hItemInfoTrie, "61_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "61_level", 5);
	SetTrieString(g_hItemInfoTrie, "61_attribs", "51 ; 1.0 ; 1 ; 0.85 ; 5 ; 1.2");

//direct hit
	SetTrieString(g_hItemInfoTrie, "127_classname", "tf_weapon_rocketlauncher_directhit");
	SetTrieValue(g_hItemInfoTrie, "127_index", 127);
	SetTrieValue(g_hItemInfoTrie, "127_slot", 0);
	SetTrieValue(g_hItemInfoTrie, "127_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "127_level", 1);
	SetTrieString(g_hItemInfoTrie, "127_attribs", "100 ; 0.3 ; 103 ; 1.8 ; 2 ; 1.25 ; 114 ; 1.0");

//equalizer
	SetTrieString(g_hItemInfoTrie, "128_classname", "tf_weapon_shovel");
	SetTrieValue(g_hItemInfoTrie, "128_index", 128);
	SetTrieValue(g_hItemInfoTrie, "128_slot", 2);
	SetTrieValue(g_hItemInfoTrie, "128_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "128_level", 10);
	SetTrieString(g_hItemInfoTrie, "128_attribs", "115 ; 1.0");

//buff banner
	SetTrieString(g_hItemInfoTrie, "129_classname", "tf_weapon_buff_item");
	SetTrieValue(g_hItemInfoTrie, "129_index", 129);
	SetTrieValue(g_hItemInfoTrie, "129_slot", 1);
	SetTrieValue(g_hItemInfoTrie, "129_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "129_level", 5);
	SetTrieString(g_hItemInfoTrie, "129_attribs", "");

//scottish resistance
	SetTrieString(g_hItemInfoTrie, "130_classname", "tf_weapon_pipebomblauncher");
	SetTrieValue(g_hItemInfoTrie, "130_index", 130);
	SetTrieValue(g_hItemInfoTrie, "130_slot", 1);
	SetTrieValue(g_hItemInfoTrie, "130_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "130_level", 5);
	SetTrieString(g_hItemInfoTrie, "130_attribs", "119 ; 1.0 ; 121 ; 1.0 ; 78 ; 1.5 ; 88 ; 6.0 ; 120 ; 0.4");

//eyelander
	SetTrieString(g_hItemInfoTrie, "132_classname", "tf_weapon_sword");
	SetTrieValue(g_hItemInfoTrie, "132_index", 132);
	SetTrieValue(g_hItemInfoTrie, "132_slot", 2);
	SetTrieValue(g_hItemInfoTrie, "132_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "132_level", 5);
	SetTrieString(g_hItemInfoTrie, "132_attribs", "15 ; 0 ; 125 ; -25");

//wrangler
	SetTrieString(g_hItemInfoTrie, "140_classname", "tf_weapon_laser_pointer");
	SetTrieValue(g_hItemInfoTrie, "140_index", 140);
	SetTrieValue(g_hItemInfoTrie, "140_slot", 1);
	SetTrieValue(g_hItemInfoTrie, "140_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "140_level", 5);
	SetTrieString(g_hItemInfoTrie, "140_attribs", "");

//frontier justice
	SetTrieString(g_hItemInfoTrie, "141_classname", "tf_weapon_sentry_revenge");
	SetTrieValue(g_hItemInfoTrie, "141_index", 141);
	SetTrieValue(g_hItemInfoTrie, "141_slot", 0);
	SetTrieValue(g_hItemInfoTrie, "141_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "141_level", 5);
	SetTrieString(g_hItemInfoTrie, "141_attribs", "136 ; 1 ; 15 ; 0 ; 3 ; 0.5");

//gunslinger
	SetTrieString(g_hItemInfoTrie, "142_classname", "tf_weapon_robot_arm");
	SetTrieValue(g_hItemInfoTrie, "142_index", 142);
	SetTrieValue(g_hItemInfoTrie, "142_slot", 2);
	SetTrieValue(g_hItemInfoTrie, "142_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "142_level", 15);
	SetTrieString(g_hItemInfoTrie, "142_attribs", "124 ; 1 ; 26 ; 25.0 ; 15 ; 0");

//homewrecker
	SetTrieString(g_hItemInfoTrie, "153_classname", "tf_weapon_fireaxe");
	SetTrieValue(g_hItemInfoTrie, "153_index", 153);
	SetTrieValue(g_hItemInfoTrie, "153_slot", 2);
	SetTrieValue(g_hItemInfoTrie, "153_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "153_level", 5);
	SetTrieString(g_hItemInfoTrie, "153_attribs", "137 ; 2.0 ; 138 ; 0.75 ; 146 ; 1");

//pain train
	SetTrieString(g_hItemInfoTrie, "154_classname", "tf_weapon_shovel");
	SetTrieValue(g_hItemInfoTrie, "154_index", 154);
	SetTrieValue(g_hItemInfoTrie, "154_slot", 2);
	SetTrieValue(g_hItemInfoTrie, "154_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "154_level", 5);
	SetTrieString(g_hItemInfoTrie, "154_attribs", "68 ; 1 ; 67 ; 1.1");

//southern hospitality
	SetTrieString(g_hItemInfoTrie, "155_classname", "tf_weapon_wrench");
	SetTrieValue(g_hItemInfoTrie, "155_index", 155);
	SetTrieValue(g_hItemInfoTrie, "155_slot", 2);
	SetTrieValue(g_hItemInfoTrie, "155_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "155_level", 20);
	SetTrieString(g_hItemInfoTrie, "155_attribs", "15 ; 0 ; 149 ; 5 ; 61 ; 1.20");

//dalokohs bar
	SetTrieString(g_hItemInfoTrie, "159_classname", "tf_weapon_lunchbox");
	SetTrieValue(g_hItemInfoTrie, "159_index", 159);
	SetTrieValue(g_hItemInfoTrie, "159_slot", 1);
	SetTrieValue(g_hItemInfoTrie, "159_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "159_level", 1);
	SetTrieString(g_hItemInfoTrie, "159_attribs", "139 ; 1");

//crit a cola
	SetTrieString(g_hItemInfoTrie, "163_classname", "tf_weapon_lunchbox_drink");
	SetTrieValue(g_hItemInfoTrie, "163_index", 163);
	SetTrieValue(g_hItemInfoTrie, "163_slot", 1);
	SetTrieValue(g_hItemInfoTrie, "163_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "163_level", 5);
	SetTrieString(g_hItemInfoTrie, "163_attribs", "139 ; 1");

//golden wrench
	SetTrieString(g_hItemInfoTrie, "169_classname", "tf_weapon_wrench");
	SetTrieValue(g_hItemInfoTrie, "169_index", 169);
	SetTrieValue(g_hItemInfoTrie, "169_slot", 2);
	SetTrieValue(g_hItemInfoTrie, "169_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "169_level", 25);
	SetTrieString(g_hItemInfoTrie, "169_attribs", "150 ; 1");

//tribalmans shiv
	SetTrieString(g_hItemInfoTrie, "171_classname", "tf_weapon_club");
	SetTrieValue(g_hItemInfoTrie, "171_index", 171);
	SetTrieValue(g_hItemInfoTrie, "171_slot", 2);
	SetTrieValue(g_hItemInfoTrie, "171_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "171_level", 5);
	SetTrieString(g_hItemInfoTrie, "171_attribs", "149 ; 6 ; 1 ; 0.5");

//scotsmans skullcutter
	SetTrieString(g_hItemInfoTrie, "172_classname", "tf_weapon_sword");
	SetTrieValue(g_hItemInfoTrie, "172_index", 172);
	SetTrieValue(g_hItemInfoTrie, "172_slot", 2);
	SetTrieValue(g_hItemInfoTrie, "172_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "172_level", 5);
	SetTrieString(g_hItemInfoTrie, "172_attribs", "2 ; 1.2 ; 54 ; 0.85");

//chargin targe (broken)
//	SetTrieString(g_hItemInfoTrie, "131_classname", "tf_wearable_item_demoshield");
//	SetTrieValue(g_hItemInfoTrie, "131_index", 131);
//	SetTrieValue(g_hItemInfoTrie, "131_slot", 0);
//	SetTrieValue(g_hItemInfoTrie, "131_quality", 3);
//	SetTrieValue(g_hItemInfoTrie, "131_level", 10);
//	SetTrieString(g_hItemInfoTrie, "131_attribs", "60 ; 0.5 ; 64 ; 0.6");

//razorback (broken)
//	SetTrieString(g_hItemInfoTrie, "57_classname", "tf_wearable_item");
//	SetTrieValue(g_hItemInfoTrie, "57_index", 57);
//	SetTrieValue(g_hItemInfoTrie, "57_slot", 1);
//	SetTrieValue(g_hItemInfoTrie, "57_quality", 3);
//	SetTrieValue(g_hItemInfoTrie, "57_level", 10);
//	SetTrieString(g_hItemInfoTrie, "57_attribs", "52 ; 1");

//gunboats (broken)
//	SetTrieString(g_hItemInfoTrie, "133_classname", "tf_wearable_item");
//	SetTrieValue(g_hItemInfoTrie, "133_index", 133);
//	SetTrieValue(g_hItemInfoTrie, "133_slot", 1);
//	SetTrieValue(g_hItemInfoTrie, "133_quality", 3);
//	SetTrieValue(g_hItemInfoTrie, "133_level", 10);
//	SetTrieString(g_hItemInfoTrie, "133_attribs", "135 ; 0.4");

//lugermorph
	SetTrieString(g_hItemInfoTrie, "160_classname", "tf_weapon_pistol");
	SetTrieValue(g_hItemInfoTrie, "160_index", 160);
	SetTrieValue(g_hItemInfoTrie, "160_slot", 1);
	SetTrieValue(g_hItemInfoTrie, "160_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "160_level", 5);
	SetTrieString(g_hItemInfoTrie, "160_attribs", "");

//big kill
	SetTrieString(g_hItemInfoTrie, "161_classname", "tf_weapon_revolver");
	SetTrieValue(g_hItemInfoTrie, "161_index", 161);
	SetTrieValue(g_hItemInfoTrie, "161_slot", 0);
	SetTrieValue(g_hItemInfoTrie, "161_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "161_level", 5);
	SetTrieString(g_hItemInfoTrie, "161_attribs", "");

//valve rocket launcher
	SetTrieString(g_hItemInfoTrie, "9018_classname", "tf_weapon_rocketlauncher");
	SetTrieValue(g_hItemInfoTrie, "9018_index", 18);
	SetTrieValue(g_hItemInfoTrie, "9018_slot", 0);
	SetTrieValue(g_hItemInfoTrie, "9018_quality", 8);
	SetTrieValue(g_hItemInfoTrie, "9018_level", 100);
	SetTrieString(g_hItemInfoTrie, "9018_attribs", "2 ; 1.15 ; 4 ; 1.5 ; 6 ; 0.85 ; 110 ; 15.0 ; 20 ; 1.0 ; 26 ; 50.0 ; 31 ; 5.0 ; 32 ; 0.30 ; 53 ; 1.0 ; 60 ; 0.85 ; 123 ; 1.15 ; 134 ; 4.0");

//valve sticky launcher
	SetTrieString(g_hItemInfoTrie, "9020_classname", "tf_weapon_pipebomblauncher");
	SetTrieValue(g_hItemInfoTrie, "9020_index", 20);
	SetTrieValue(g_hItemInfoTrie, "9020_slot", 1);
	SetTrieValue(g_hItemInfoTrie, "9020_quality", 8);
	SetTrieValue(g_hItemInfoTrie, "9020_level", 100);
	SetTrieString(g_hItemInfoTrie, "9020_attribs", "2 ; 1.15 ; 4 ; 1.5 ; 6 ; 0.85 ; 110 ; 15.0 ; 20 ; 1.0 ; 26 ; 50.0 ; 31 ; 5.0 ; 32 ; 0.30 ; 53 ; 1.0 ; 60 ; 0.85 ; 123 ; 1.15 ; 134 ; 4.0");

//valve sniper rifle
	SetTrieString(g_hItemInfoTrie, "9014_classname", "tf_weapon_sniperrifle");
	SetTrieValue(g_hItemInfoTrie, "9014_index", 14);
	SetTrieValue(g_hItemInfoTrie, "9014_slot", 0);
	SetTrieValue(g_hItemInfoTrie, "9014_quality", 8);
	SetTrieValue(g_hItemInfoTrie, "9014_level", 100);
	SetTrieString(g_hItemInfoTrie, "9014_attribs", "2 ; 1.15 ; 4 ; 1.5 ; 6 ; 0.85 ; 110 ; 15.0 ; 20 ; 1.0 ; 26 ; 50.0 ; 31 ; 5.0 ; 32 ; 0.30 ; 53 ; 1.0 ; 60 ; 0.85 ; 123 ; 1.15 ; 134 ; 4.0");

//valve scattergun
	SetTrieString(g_hItemInfoTrie, "9013_classname", "tf_weapon_scattergun");
	SetTrieValue(g_hItemInfoTrie, "9013_index", 13);
	SetTrieValue(g_hItemInfoTrie, "9013_slot", 0);
	SetTrieValue(g_hItemInfoTrie, "9013_quality", 8);
	SetTrieValue(g_hItemInfoTrie, "9013_level", 100);
	SetTrieString(g_hItemInfoTrie, "9013_attribs", "2 ; 1.15 ; 4 ; 1.5 ; 6 ; 0.85 ; 110 ; 15.0 ; 20 ; 1.0 ; 26 ; 50.0 ; 31 ; 5.0 ; 32 ; 0.30 ; 53 ; 1.0 ; 60 ; 0.85 ; 123 ; 1.15 ; 134 ; 4.0");

//valve flamethrower
	SetTrieString(g_hItemInfoTrie, "9021_classname", "tf_weapon_flamethrower");
	SetTrieValue(g_hItemInfoTrie, "9021_index", 21);
	SetTrieValue(g_hItemInfoTrie, "9021_slot", 0);
	SetTrieValue(g_hItemInfoTrie, "9021_quality", 8);
	SetTrieValue(g_hItemInfoTrie, "9021_level", 100);
	SetTrieString(g_hItemInfoTrie, "9021_attribs", "2 ; 1.15 ; 4 ; 1.5 ; 6 ; 0.85 ; 110 ; 15.0 ; 20 ; 1.0 ; 26 ; 50.0 ; 31 ; 5.0 ; 32 ; 0.30 ; 53 ; 1.0 ; 60 ; 0.85 ; 123 ; 1.15 ; 134 ; 4.0");

//valve syringe gun
	SetTrieString(g_hItemInfoTrie, "9017_classname", "tf_weapon_syringegun_medic");
	SetTrieValue(g_hItemInfoTrie, "9017_index", 17);
	SetTrieValue(g_hItemInfoTrie, "9017_slot", 0);
	SetTrieValue(g_hItemInfoTrie, "9017_quality", 8);
	SetTrieValue(g_hItemInfoTrie, "9017_level", 100);
	SetTrieString(g_hItemInfoTrie, "9017_attribs", "2 ; 1.15 ; 4 ; 1.5 ; 6 ; 0.85 ; 110 ; 15.0 ; 20 ; 1.0 ; 26 ; 50.0 ; 31 ; 5.0 ; 32 ; 0.30 ; 53 ; 1.0 ; 60 ; 0.85 ; 123 ; 1.15 ; 134 ; 4.0");

//valve minigun
	SetTrieString(g_hItemInfoTrie, "9015_classname", "tf_weapon_minigun");
	SetTrieValue(g_hItemInfoTrie, "9015_index", 15);
	SetTrieValue(g_hItemInfoTrie, "9015_slot", 0);
	SetTrieValue(g_hItemInfoTrie, "9015_quality", 8);
	SetTrieValue(g_hItemInfoTrie, "9015_level", 100);
	SetTrieString(g_hItemInfoTrie, "9015_attribs", "2 ; 1.15 ; 4 ; 1.5 ; 6 ; 0.85 ; 110 ; 15.0 ; 20 ; 1.0 ; 26 ; 50.0 ; 31 ; 5.0 ; 32 ; 0.30 ; 53 ; 1.0 ; 60 ; 0.85 ; 123 ; 1.15 ; 134 ; 4.0");

//valve revolver
	SetTrieString(g_hItemInfoTrie, "9024_classname", "tf_weapon_revolver");
	SetTrieValue(g_hItemInfoTrie, "9024_index", 24);
	SetTrieValue(g_hItemInfoTrie, "9024_slot", 0);
	SetTrieValue(g_hItemInfoTrie, "9024_quality", 8);
	SetTrieValue(g_hItemInfoTrie, "9024_level", 100);
	SetTrieString(g_hItemInfoTrie, "9024_attribs", "2 ; 1.15 ; 4 ; 1.5 ; 6 ; 0.85 ; 110 ; 15.0 ; 20 ; 1.0 ; 26 ; 50.0 ; 31 ; 5.0 ; 32 ; 0.30 ; 53 ; 1.0 ; 60 ; 0.85 ; 123 ; 1.15 ; 134 ; 4.0");

//valve shotgun engineer
	SetTrieString(g_hItemInfoTrie, "9009_classname", "tf_weapon_shotgun_primary");
	SetTrieValue(g_hItemInfoTrie, "9009_index", 9);
	SetTrieValue(g_hItemInfoTrie, "9009_slot", 0);
	SetTrieValue(g_hItemInfoTrie, "9009_quality", 8);
	SetTrieValue(g_hItemInfoTrie, "9009_level", 100);
	SetTrieString(g_hItemInfoTrie, "9009_attribs", "2 ; 1.15 ; 4 ; 1.5 ; 6 ; 0.85 ; 110 ; 15.0 ; 20 ; 1.0 ; 26 ; 50.0 ; 31 ; 5.0 ; 32 ; 0.30 ; 53 ; 1.0 ; 60 ; 0.85 ; 123 ; 1.15 ; 134 ; 4.0");

//valve medigun
	SetTrieString(g_hItemInfoTrie, "9029_classname", "tf_weapon_medigun");
	SetTrieValue(g_hItemInfoTrie, "9029_index", 29);
	SetTrieValue(g_hItemInfoTrie, "9029_slot", 1);
	SetTrieValue(g_hItemInfoTrie, "9029_quality", 8);
	SetTrieValue(g_hItemInfoTrie, "9029_level", 100);
	SetTrieString(g_hItemInfoTrie, "9029_attribs", "8 ; 1.15 ; 10 ; 1.15 ; 14 ; 1.0 ; 26 ; 50.0 ; 53 ; 1.0 ; 60 ; 0.85 ; 123 ; 1.5 ; 134 ; 4.0");

//ludmila
	SetTrieString(g_hItemInfoTrie, "2041_classname", "tf_weapon_minigun");
	SetTrieValue(g_hItemInfoTrie, "2041_index", 41);
	SetTrieValue(g_hItemInfoTrie, "2041_slot", 0);
	SetTrieValue(g_hItemInfoTrie, "2041_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "2041_level", 5);
	SetTrieString(g_hItemInfoTrie, "2041_attribs", "29 ; 1");

//gloves of running urgently 
	SetTrieString(g_hItemInfoTrie, "2043_classname", "tf_weapon_fists");
	SetTrieValue(g_hItemInfoTrie, "2043_index", 43);
	SetTrieValue(g_hItemInfoTrie, "2043_slot", 2);
	SetTrieValue(g_hItemInfoTrie, "2043_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "2043_level", 7);
	SetTrieString(g_hItemInfoTrie, "2043_attribs", "1 ; 0.5 ; 128 ; 1.0 ; 107 ; 2.0 ; 129 ; -6.0");

//spycrab pda
	SetTrieString(g_hItemInfoTrie, "9027_classname", "tf_weapon_pda_spy");
	SetTrieValue(g_hItemInfoTrie, "9027_index", 27);
	SetTrieValue(g_hItemInfoTrie, "9027_slot", 3);
	SetTrieValue(g_hItemInfoTrie, "9027_quality", 2);
	SetTrieValue(g_hItemInfoTrie, "9027_level", 100);
	SetTrieString(g_hItemInfoTrie, "9027_attribs", "128 ; 1.0 ; 60 ; 0.0 ; 62 ; 0.0 ; 64 ; 0.0 ; 66 ; 0.0 ; 70 ; 2.0 ; 53 ; 1.0 ; 68 ; -1.0 ; 134 ; 4.0");

//fire retardant suit (revolver does no damage)
	SetTrieString(g_hItemInfoTrie, "2061_classname", "tf_weapon_revolver");
	SetTrieValue(g_hItemInfoTrie, "2061_index", 61);
	SetTrieValue(g_hItemInfoTrie, "2061_slot", 0);
	SetTrieValue(g_hItemInfoTrie, "2061_quality", 3);
	SetTrieValue(g_hItemInfoTrie, "2061_level", 5);
	SetTrieString(g_hItemInfoTrie, "2061_attribs", "53 ; 1.0 ; 60 ; 0.10 ; 1 ; 0.0");

//valve cheap rocket launcher
	SetTrieString(g_hItemInfoTrie, "8018_classname", "tf_weapon_rocketlauncher");
	SetTrieValue(g_hItemInfoTrie, "8018_index", 18);
	SetTrieValue(g_hItemInfoTrie, "8018_slot", 0);
	SetTrieValue(g_hItemInfoTrie, "8018_quality", 8);
	SetTrieValue(g_hItemInfoTrie, "8018_level", 100);
	SetTrieString(g_hItemInfoTrie, "8018_attribs", "2 ; 100.0 ; 4 ; 91.0 ; 6 ; 0.25 ; 110 ; 500.0 ; 26 ; 250.0 ; 31 ; 10.0 ; 107 ; 3.0 ; 134 ; 4.0");

//PCG cheap Community rocket launcher
	SetTrieString(g_hItemInfoTrie, "7018_classname", "tf_weapon_rocketlauncher");
	SetTrieValue(g_hItemInfoTrie, "7018_index", 18);
	SetTrieValue(g_hItemInfoTrie, "7018_slot", 0);
	SetTrieValue(g_hItemInfoTrie, "7018_quality", 7);
	SetTrieValue(g_hItemInfoTrie, "7018_level", 100);
	SetTrieString(g_hItemInfoTrie, "7018_attribs", "26 ; 500.0 ; 110 ; 500.0 ; 6 ; 0.25 ; 4 ; 200.0 ; 2 ; 100.0 ; 134 ; 4.0");
}