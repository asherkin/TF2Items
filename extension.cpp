/*
 * ================================================================================
 * TF2 Items Extension
 * Copyright (C) 2009-2010 AzuiSleet, Asher Baker (Asherkin).  All rights reserved.
 * ================================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 *	Atributions & Thanks:
 *	=====================
 *	AzuiSleet				-	Reversed CScriptCreatedItem and realesed it publicly, along with writing most of the item editing code below.
 *	Damizean				-	Fixed padding for CScriptCreatedItem in Linux.
 *	Wazz					-	Wrote "Shit not be void" in #sourcemod and revealed that GiveNamedItem returned CBaseEntity *.
 *	MatthiasVance			-	Reminded me to comment out '#define INFINITE_PROBLEMS 1'.
 *	yakbot					-	Providing endless fun in #sourcemod while coding.
 *	voogru & Drunken_F00l	-	Inspiring the creation of this.
 */

#include "extension.h"

/*
 *	Debugging options:
 *	==================
 */
//#define TF2ITEMS_DEBUG_ITEMS
//#define TF2ITEMS_DEBUG_HOOKING

TF2Items g_TF2Items;

SMEXT_LINK(&g_TF2Items);

SH_DECL_HOOK2_void(IServerGameClients, ClientPutInServer, SH_NOATTRIB, 0, edict_t *, char const *);
SH_DECL_HOOK6(IServerGameDLL, LevelInit, SH_NOATTRIB, 0, bool, const char *, const char *, const char *, const char *, bool, bool);
SH_DECL_MANUALHOOK4(MHook_GiveNamedItem, 0, 0, 0, CBaseEntity *, char const *, int, CScriptCreatedItem *, bool);

ICvar *icvar = NULL;
IServerGameClients *gameclients = NULL;
IServerGameEnts *gameents = NULL;
IBaseFileSystem *filesystem = NULL;

ConVar TF2ItemsVersion("tf2items_version", "1.1.0", FCVAR_SPONLY|FCVAR_REPLICATED|FCVAR_NOTIFY, "TF2 Items Version");
ConVar TF2ItemsEnabled("sm_tf2items_enabled", "1", 0);
ConVar *pTagsVar = NULL;

IGameConfig *g_pGameConf = NULL;
KeyValues *g_pCustomWeapons = new KeyValues("custom_weapons");

bool g_bHooked = false;
int GiveNamedItem_Hook = 0;
int ClientPutInServer_Hook = 0;
int LevelInit_Hook = 0;
int ClientCommand_Hook = 0;


CBaseEntity *Hook_GiveNamedItem(char const *item, int a, CScriptCreatedItem *cscript, bool b) {

	#ifdef TF2ITEMS_DEBUG_HOOKING
		META_LOG(g_PLAPI, "GiveNamedItem called.");
	#endif // TF2ITEMS_DEBUG_HOOKING

	if (!TF2ItemsEnabled.GetBool()) {
		RETURN_META_VALUE(MRES_IGNORED, NULL);
	}

	CBasePlayer *player = META_IFACEPTR(CBasePlayer);

	if (cscript == NULL) {
		RETURN_META_VALUE(MRES_IGNORED, NULL);
	}

	edict_t *playerEdict = gameents->BaseEntityToEdict((CBaseEntity *)player);
	IGamePlayer * pPlayer = playerhelpers->GetGamePlayer(playerEdict);
	const char *steamID = pPlayer->GetAuthString();

	KeyValues *player_weapons = new KeyValues("weapon_invalid");
	KeyValues *player_weapon = new KeyValues("weapon_invalid");

	if (KV_FindSection(player_weapons, g_pCustomWeapons, steamID)) {
		if (KV_FindSection(player_weapon, player_weapons, cscript->itemdefindex)) {
			CScriptCreatedItem newitem = EditWeaponFromFile(cscript, player_weapon);
			RETURN_META_VALUE_MNEWPARAMS(MRES_HANDLED, NULL, MHook_GiveNamedItem, (item, a, &newitem, b));
		} else if (KV_FindSection(player_weapon, player_weapons, "*")) {
			CScriptCreatedItem newitem = EditWeaponFromFile(cscript, player_weapon);
			RETURN_META_VALUE_MNEWPARAMS(MRES_HANDLED, NULL, MHook_GiveNamedItem, (item, a, &newitem, b));
		} else if (KV_FindSection(player_weapons, g_pCustomWeapons, "*")) {
			if (KV_FindSection(player_weapon, player_weapons, cscript->itemdefindex)) {
				CScriptCreatedItem newitem = EditWeaponFromFile(cscript, player_weapon);
				RETURN_META_VALUE_MNEWPARAMS(MRES_HANDLED, NULL, MHook_GiveNamedItem, (item, a, &newitem, b));
			} else if (KV_FindSection(player_weapon, player_weapons, "*")) {
				CScriptCreatedItem newitem = EditWeaponFromFile(cscript, player_weapon);
				RETURN_META_VALUE_MNEWPARAMS(MRES_HANDLED, NULL, MHook_GiveNamedItem, (item, a, &newitem, b));
			} else {
				RETURN_META_VALUE(MRES_IGNORED, NULL);
			}
		} else {
			RETURN_META_VALUE(MRES_IGNORED, NULL);
		}
	} else if (KV_FindSection(player_weapons, g_pCustomWeapons, "*")) {
		if (KV_FindSection(player_weapon, player_weapons, cscript->itemdefindex)) {
			CScriptCreatedItem newitem = EditWeaponFromFile(cscript, player_weapon);
			RETURN_META_VALUE_MNEWPARAMS(MRES_HANDLED, NULL, MHook_GiveNamedItem, (item, a, &newitem, b));
		} else if (KV_FindSection(player_weapon, player_weapons, "*")) {
			CScriptCreatedItem newitem = EditWeaponFromFile(cscript, player_weapon);
			RETURN_META_VALUE_MNEWPARAMS(MRES_HANDLED, NULL, MHook_GiveNamedItem, (item, a, &newitem, b));
		} else {
			RETURN_META_VALUE(MRES_IGNORED, NULL);
		}
	} else {
		RETURN_META_VALUE(MRES_IGNORED, NULL);
	}
}

CScriptCreatedItem EditWeaponFromFile(CScriptCreatedItem *cscript, KeyValues *player_weapon) {
	CScriptCreatedItem newitem;
	memcpy(&newitem, cscript, sizeof(CScriptCreatedItem));

	int itemlevel;
	if (KV_FindValue(&itemlevel, player_weapon, "level")) {
		newitem.itemlevel = itemlevel;
	}

	int itemquality;
	if (KV_FindValue(&itemquality, player_weapon, "quality")) {
		newitem.itemquality = itemquality;
	}

	int attrib_count;
	KeyValues *weapon_attribs = new KeyValues("weapon_invalid");
	if (KV_FindValue(&attrib_count, player_weapon, "attrib_count") && KV_FindSection(weapon_attribs, player_weapon, "attributes")) {

		if (newitem.itemquality == 0) {
			newitem.itemquality = 9;
		}

		#ifdef TF2ITEMS_DEBUG_ITEMS
			META_CONPRINTF("Attribute Count: %d\n", attrib_count);
		#endif // TF2ITEMS_DEBUG_ITEMS

		newitem.attributes = (CScriptCreatedAttribute *)malloc(sizeof(CScriptCreatedAttribute) * attrib_count);
		CScriptCreatedAttribute qq;

		int searchindex = 0;
		KeyValues *weapon_attrib;
		for ( weapon_attrib = weapon_attribs->GetFirstValue(); weapon_attrib; weapon_attrib = weapon_attrib->GetNextValue() ) {
			qq.attribindex = atoi(weapon_attrib->GetName());
			qq.attribvalue = weapon_attrib->GetFloat();
			memcpy(newitem.attributes + searchindex, &qq, sizeof(qq));

			#ifdef TF2ITEMS_DEBUG_ITEMS
				META_CONPRINTF("Attribute %d Index: %d\n", searchindex, qq.attribindex);
				META_CONPRINTF("Attribute %d Value: %f\n", searchindex, qq.attribvalue);
			#endif // TF2ITEMS_DEBUG_ITEMS

			searchindex = searchindex + 1;
		}

		#ifdef TF2ITEMS_DEBUG_ITEMS
			META_CONPRINTF("Found Attribute Count: %d\n", searchindex+1);
		#endif // TF2ITEMS_DEBUG_ITEMS

		newitem.attributes2 = newitem.attributes;
		newitem.allocatedAttributes = attrib_count;
		newitem.attribcount = attrib_count;
	}
	#ifdef TF2ITEMS_DEBUG_ITEMS
		else {
			META_LOG(g_PLAPI, "Error in attribute structure, missing attrib_count or no attributes specified.");
		}
	#endif // TF2ITEMS_DEBUG_ITEMS
	return newitem;
}

void Hook_ClientPutInServer(edict_t *pEntity, char const *playername) {

	#ifdef TF2ITEMS_DEBUG_HOOKING
		META_LOG(g_PLAPI, "ClientPutInServer called.");
	#endif // TF2ITEMS_DEBUG_HOOKING

	if(!g_bHooked && pEntity->m_pNetworkable) {
		CBaseEntity *baseentity = pEntity->m_pNetworkable->GetBaseEntity();
		if(!baseentity)
			return;

		CBasePlayer *player = (CBasePlayer *)baseentity;

		GiveNamedItem_Hook = SH_ADD_MANUALVPHOOK(MHook_GiveNamedItem, player, SH_STATIC(Hook_GiveNamedItem), false);
		if (ClientPutInServer_Hook != 0) {
			SH_REMOVE_HOOK_ID(ClientPutInServer_Hook);
		}

		g_bHooked = true;

		#ifdef TF2ITEMS_DEBUG_HOOKING
			META_LOG(g_PLAPI, "GiveNamedItem hooked.");
		#endif // TF2ITEMS_DEBUG_HOOKING

	} else if (ClientPutInServer_Hook != 0) {
		SH_REMOVE_HOOK_ID(ClientPutInServer_Hook);
		#ifdef TF2ITEMS_DEBUG_HOOKING
			META_LOG(g_PLAPI, "ClientPutInServer unhooked.");
		#endif // TF2ITEMS_DEBUG_HOOKING
	}
}

void AddTag() {
	if (pTagsVar == NULL) {
		return;
	}

	const char *curTags = pTagsVar->GetString();
	const char *ourTag = "customweapons";

	if (strstr(curTags, ourTag) != NULL) {
		/* Already tagged */
		return;
	}

	/* New tags buffer (+2 for , and null char) */
	int newLen = strlen(curTags) + strlen(ourTag) + 2;
	char *newTags = new char[newLen];

	g_pSM->Format(newTags, newLen, "%s,%s", curTags, ourTag);

	pTagsVar->SetValue(newTags);

	delete [] newTags;
}

bool LevelInitHook(char const *pMapName, char const *pMapEntities, char const *pOldLevel, char const *pLandmarkName, bool loadGame, bool background) {
	#ifdef TF2ITEMS_DEBUG_HOOKING
		META_LOG(g_PLAPI, "LevelInit called.");
	#endif // TF2ITEMS_DEBUG_HOOKING
	if (TF2ItemsEnabled.GetBool()) {
		AddTag();
	}
	return true;
}

bool TF2Items::SDK_OnLoad(char *error, size_t maxlen, bool late) {

	char conf_error[255] = "";
	if (!gameconfs->LoadGameConfigFile("tf2.items", &g_pGameConf, conf_error, sizeof(conf_error)))
	{
		if (conf_error[0])
		{
			snprintf(error, maxlen, "Could not read tf2.items.txt: %s\n", conf_error);
		}
		return false;
	}

	int offset;

	if (!g_pGameConf->GetOffset("GiveNamedItem", &offset))
	{
		snprintf(error, maxlen, "Could not find offset for GiveNamedItem");
		return false;
	} else {
		SH_MANUALHOOK_RECONFIGURE(MHook_GiveNamedItem, offset, 0, 0);
	}

	/*
	#ifndef _LINUX
		SH_MANUALHOOK_RECONFIGURE(MHook_GiveNamedItem, 449, 0, 0); // Windows
		#ifdef TF2ITEMS_DEBUG_HOOKING
			META_LOG(g_PLAPI, "GiveNamedItem offset set. (Windows)");
		#endif // TF2ITEMS_DEBUG_HOOKING
	#else
		SH_MANUALHOOK_RECONFIGURE(MHook_GiveNamedItem, 456, 0, 0); // Linux
		#ifdef TF2ITEMS_DEBUG_HOOKING
			META_LOG(g_PLAPI, "GiveNamedItem offset set. (Linux)");
		#endif // TF2ITEMS_DEBUG_HOOKING
	#endif
	*/	

	// If it's a late load, there might be the chance there are players already on the server. Just
	// check for this and try to hook them instead of waiting for the next player. -- Damizean
	if (late) {
		#ifdef TF2ITEMS_DEBUG_HOOKING
			META_LOG(g_PLAPI, "Is a late load, attempting to hook GiveNamedItem.");
		#endif // TF2ITEMS_DEBUG_HOOKING
		int iMaxClients = playerhelpers->GetMaxClients();
		for (int iClient = 1; iClient <= iMaxClients; iClient++) {
			IGamePlayer * pPlayer = playerhelpers->GetGamePlayer(iClient);
			if (pPlayer == NULL) continue;
			if (pPlayer->IsConnected() == false) continue;
			if (pPlayer->IsFakeClient() == true) continue;
			if (pPlayer->IsInGame() == false) continue;

			// Retrieve the edict
			edict_t * pEdict = pPlayer->GetEdict();
			if (pEdict == NULL) continue;

			// Retrieve base player
			CBasePlayer * pBasePlayer = (CBasePlayer *) pEdict->m_pNetworkable->GetBaseEntity();
			if (pBasePlayer == NULL) continue;

			// Done, hook the BasePlayer
			GiveNamedItem_Hook = SH_ADD_MANUALVPHOOK(MHook_GiveNamedItem, pBasePlayer, SH_STATIC(Hook_GiveNamedItem), false);
			#ifdef TF2ITEMS_DEBUG_HOOKING
				META_LOG(g_PLAPI, "GiveNamedItem hooked.");
			#endif // TF2ITEMS_DEBUG_HOOKING
			g_bHooked = true;
		}
	}

	if (g_bHooked == false) {
		#ifdef TF2ITEMS_DEBUG_HOOKING
			META_LOG(g_PLAPI, "Is a NOT late load or no players found, attempting to hook ClientPutInServer.");
		#endif // TF2ITEMS_DEBUG_HOOKING
		ClientPutInServer_Hook = SH_ADD_HOOK_STATICFUNC(IServerGameClients, ClientPutInServer, gameclients, Hook_ClientPutInServer, true);
		#ifdef TF2ITEMS_DEBUG_HOOKING
			META_LOG(g_PLAPI, "ClientPutInServer hooked.");
		#endif // TF2ITEMS_DEBUG_HOOKING
	}

	char m_File[255] = "";
	g_pSM->BuildPath(Path_SM, m_File, sizeof(m_File), "data/customweps.txt");

	if (!g_pCustomWeapons->LoadFromFile(filesystem, m_File)) {
		snprintf(error, maxlen, "Could not read customweps.txt\n");
		return false;
	}

	if (!strcmp(g_pCustomWeapons->GetName(), "custom_weapons_v2") == 0) {
		snprintf(error, maxlen, "customweps.txt structure corrupt or incorrect version\n");
		return false;
	}

	return true;
}

bool KV_FindSection(KeyValues *found, KeyValues *source, const char *search) {
	KeyValues *pKey;
	for ( pKey = source->GetFirstTrueSubKey(); pKey; pKey = pKey->GetNextTrueSubKey() ) {
		if (strcmp(pKey->GetName(), search) == 0) {
			*found = *pKey;
			return true;
		}
	}
	return false;
}

bool KV_FindSection(KeyValues *found, KeyValues *source, int search) {
	KeyValues *pKey;
	for ( pKey = source->GetFirstTrueSubKey(); pKey; pKey = pKey->GetNextTrueSubKey() ) {
		char buf[16];
		sprintf(buf,"%d",search);
		const char* weaponidchar = buf;
		if (strcmp(pKey->GetName(), weaponidchar) == 0) {
			*found = *pKey;
			return true;
		}
	}
	return false;
}

bool KV_FindValue(int *found, KeyValues *source, const char *search) {
	KeyValues *pKey;
	for ( pKey = source->GetFirstValue(); pKey; pKey = pKey->GetNextValue() ) {
		if (strcmp(pKey->GetName(), search) == 0) {
			*found = pKey->GetInt();
			return true;
		}
	}
	return false;
}

bool TF2Items::SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlen, bool late) {

	GET_V_IFACE_ANY(GetServerFactory, gameclients, IServerGameClients, INTERFACEVERSION_SERVERGAMECLIENTS);
	GET_V_IFACE_ANY(GetServerFactory, gameents, IServerGameEnts, INTERFACEVERSION_SERVERGAMEENTS);
	GET_V_IFACE_ANY(GetFileSystemFactory, filesystem, IBaseFileSystem, BASEFILESYSTEM_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, icvar, ICvar, CVAR_INTERFACE_VERSION);

	if (!gameclients)
	{
		snprintf(error, maxlen, "Could not find interface %s", INTERFACEVERSION_SERVERGAMEENTS);
		return false;
	}
	if (!gameents)
	{
		snprintf(error, maxlen, "Could not find interface %s", INTERFACEVERSION_SERVERGAMECLIENTS);
		return false;
	}
	if (!filesystem)
	{
		snprintf(error, maxlen, "Could not find interface %s", BASEFILESYSTEM_INTERFACE_VERSION);
		return false;
	}
	if (!icvar)
	{
		snprintf(error, maxlen, "Could not find interface %s", CVAR_INTERFACE_VERSION);
		return false;
	}

	g_pCVar = icvar;
	pTagsVar = icvar->FindVar("sv_tags");

	LevelInit_Hook = SH_ADD_HOOK(IServerGameDLL, LevelInit, gamedll, SH_STATIC(LevelInitHook), true);
	#ifdef TF2ITEMS_DEBUG_HOOKING
		META_LOG(g_PLAPI, "LevelInit hooked.");
	#endif // TF2ITEMS_DEBUG_HOOKING

	ConVar_Register(0, this);

	META_LOG(g_PLAPI, "Starting plugin.");

	return true;
}

void TF2Items::SDK_OnUnload() {
	gameconfs->CloseGameConfigFile(g_pGameConf);
}

bool TF2Items::SDK_OnMetamodUnload(char *error, size_t maxlen) {

	if (ClientPutInServer_Hook != 0) {
		SH_REMOVE_HOOK_ID(ClientPutInServer_Hook);
		#ifdef TF2ITEMS_DEBUG_HOOKING
			META_LOG(g_PLAPI, "ClientPutInServer unhooked.");
		#endif // TF2ITEMS_DEBUG_HOOKING
	}

	if (GiveNamedItem_Hook != 0) {
		SH_REMOVE_HOOK_ID(GiveNamedItem_Hook);
		#ifdef TF2ITEMS_DEBUG_HOOKING
			META_LOG(g_PLAPI, "GiveNamedItem unhooked.");
		#endif // TF2ITEMS_DEBUG_HOOKING
	}

	if (LevelInit_Hook != 0) {
		SH_REMOVE_HOOK_ID(LevelInit_Hook);
		#ifdef TF2ITEMS_DEBUG_HOOKING
			META_LOG(g_PLAPI, "LevelInit unhooked.");
		#endif // TF2ITEMS_DEBUG_HOOKING
	}

	return true;
}

bool TF2Items::RegisterConCommandBase(ConCommandBase *pCommand) {
	META_REGCVAR(pCommand);
	return true;
}
