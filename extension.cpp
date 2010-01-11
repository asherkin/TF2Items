/**
 * =============================================================================
 * TF2 Items Extension
 * Copyright (C) 2009-2010 AzuiSleet, Asher Baker (Asherkin).  All rights reserved.
 * =============================================================================
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
 *
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
 *  ==================
 */

//#define TF2ITEMS_DEBUG_AUTH
//#define TF2ITEMS_DEBUG_ITEMS
//#define TF2ITEMS_DEBUG_HOOKING

TF2Items g_TF2Items;

SMEXT_LINK(&g_TF2Items);

SH_DECL_HOOK2_void(IServerGameClients, ClientPutInServer, SH_NOATTRIB, 0, edict_t *, char const *);
SH_DECL_MANUALHOOK4(MHook_GiveNamedItem, 0, 0, 0, CBaseEntity *, char const *, int, CScriptCreatedItem *, bool);

IServerGameClients *gameclients = NULL;
IServerGameEnts *gameents = NULL;
IPlayerInfoManager *infomanager = NULL;
IBaseFileSystem *filesystem = NULL;

IGameConfig *g_pGameConf = NULL;
KeyValues *g_pCustomWeapons = new KeyValues("custom_weapons");

bool bHooked = false;
int GiveNamedItem_Hook = 0;

CBaseEntity *Hook_GiveNamedItem(char const *item, int a, CScriptCreatedItem *cscript, bool b) {

	#ifdef TF2ITEMS_DEBUG_HOOKING
		META_LOG(g_PLAPI, "GiveNamedItem called.");
	#endif // TF2ITEMS_DEBUG_HOOKING

	CBasePlayer *player = META_IFACEPTR(CBasePlayer);

	if (cscript == NULL) {
		RETURN_META_VALUE(MRES_IGNORED, NULL);
	}

	edict_t *playerEdict = gameents->BaseEntityToEdict((CBaseEntity *)player);
	IPlayerInfo *playerinfo = infomanager->GetPlayerInfo(playerEdict);
	const char *steamID = playerinfo->GetNetworkIDString();

	#ifdef TF2ITEMS_DEBUG_AUTH
		META_CONPRINTF("Client Steam ID: %s\n", steamID);
	#endif // TF2ITEMS_DEBUG_AUTH

	KeyValues *player_weapons = new KeyValues("weapon_invalid");

	if (KV_FindSection(player_weapons, g_pCustomWeapons, steamID)) {

		#ifdef TF2ITEMS_DEBUG_AUTH
			META_LOG(g_PLAPI, "Client has a section.");
		#endif // TF2ITEMS_DEBUG_AUTH

		KeyValues *player_weapon = new KeyValues("weapon_invalid");

		if (KV_FindSection(player_weapon, player_weapons, cscript->itemdefindex)) {

			#ifdef TF2ITEMS_DEBUG_ITEMS
				META_LOG(g_PLAPI, "Custom weapon generation started.");
			#endif // TF2ITEMS_DEBUG_ITEMS

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
			if (KV_FindValue(&attrib_count, player_weapon, "attrib_count") && KV_FindSection(weapon_attribs, player_weapon, "attributes") && itemquality != 0) {

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
						META_LOG(g_PLAPI, "Error in attribute structure, missing attrib_count, no attributes specified, or quality is equal to 0.");
				}
			#endif // TF2ITEMS_DEBUG_ITEMS

			#ifdef TF2ITEMS_DEBUG_ITEMS
				META_LOG(g_PLAPI, "Custom weapon generated and given.");
			#endif // TF2ITEMS_DEBUG_ITEMS

			RETURN_META_VALUE_MNEWPARAMS(MRES_HANDLED, NULL, MHook_GiveNamedItem, (item, a, &newitem, b));
		} else {
			RETURN_META_VALUE(MRES_IGNORED, NULL);
		}
	} else {
		RETURN_META_VALUE(MRES_IGNORED, NULL);
	}

}

void Hook_ClientPutInServer(edict_t *pEntity, char const *playername) {

	#ifdef TF2ITEMS_DEBUG_HOOKING
		META_LOG(g_PLAPI, "ClientPutInServer called.");
	#endif // TF2ITEMS_DEBUG_HOOKING

	if(!bHooked && pEntity->m_pNetworkable)
	{
		CBaseEntity *baseentity = pEntity->m_pNetworkable->GetBaseEntity();
		if(!baseentity)
			return;

		CBasePlayer *player = (CBasePlayer *)baseentity;

		bHooked = true;

		#ifdef TF2ITEMS_DEBUG_HOOKING
			META_LOG(g_PLAPI, "GiveNamedItem hooked.");
		#endif // TF2ITEMS_DEBUG_HOOKING

		GiveNamedItem_Hook = SH_ADD_MANUALVPHOOK(MHook_GiveNamedItem, player, SH_STATIC(Hook_GiveNamedItem), false);

	}
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
	GET_V_IFACE_ANY(GetServerFactory, infomanager, IPlayerInfoManager, INTERFACEVERSION_PLAYERINFOMANAGER);
	GET_V_IFACE_ANY(GetFileSystemFactory, filesystem, IBaseFileSystem, BASEFILESYSTEM_INTERFACE_VERSION);

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
	if (!infomanager)
	{
		snprintf(error, maxlen, "Could not find interface %s", INTERFACEVERSION_PLAYERINFOMANAGER);
		return false;
	}
	if (!filesystem)
	{
		snprintf(error, maxlen, "Could not find interface %s", BASEFILESYSTEM_INTERFACE_VERSION);
		return false;
	}

	META_LOG(g_PLAPI, "Starting plugin.");

	SH_ADD_HOOK_STATICFUNC(IServerGameClients, ClientPutInServer, gameclients, Hook_ClientPutInServer, true);
	
	return true;
}

void TF2Items::SDK_OnUnload() {

	gameconfs->CloseGameConfigFile(g_pGameConf);

}

bool TF2Items::SDK_OnMetamodUnload(char *error, size_t maxlen) {

	SH_REMOVE_HOOK_STATICFUNC(IServerGameClients, ClientPutInServer, gameclients, Hook_ClientPutInServer, true);

	if (GiveNamedItem_Hook)
		SH_REMOVE_HOOK_ID(GiveNamedItem_Hook);

	return true;
}
