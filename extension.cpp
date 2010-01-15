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
 *	Damizean				-	Fixed padding for CScriptCreatedItem in Linux. Wrote the SourcePawn Interface.
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

ConVar TF2ItemsVersion("tf2items_version", "1.2.0", FCVAR_SPONLY|FCVAR_REPLICATED|FCVAR_NOTIFY, "TF2 Items Version");
ConVar TF2ItemsEnabled("sm_tf2items_enabled", "1", 0);
ConVar *pTagsVar = NULL;

IGameConfig *g_pGameConf = NULL;
KeyValues *g_pCustomWeapons = new KeyValues("weapon_invalid");

bool g_bHooked = false;
int GiveNamedItem_Hook = 0;
int ClientPutInServer_Hook = 0;
int LevelInit_Hook = 0;

IForward * g_pForwardGiveItem = NULL;
HandleType_t g_ScriptedItemOverrideHandleType = 0;
TScriptedItemOverrideTypeHandler g_ScriptedItemOverrideHandler;
void * g_pScriptCreatedVTable = NULL;

sp_nativeinfo_t g_ExtensionNatives[] =
{
	{ "TF2Items_CreateItem",		TF2Items_CreateItem },
	{ "TF2Items_SetFlags",			TF2Items_SetFlags },
	{ "TF2Items_GetFlags",			TF2Items_GetFlags },
	{ "TF2Items_SetClassname",		TF2Items_SetClassname },
	{ "TF2Items_GetClassname",		TF2Items_GetClassname },
	{ "TF2Items_SetItemIndex",		TF2Items_SetItemIndex },
	{ "TF2Items_GetItemIndex",		TF2Items_GetItemIndex },
	{ "TF2Items_SetQuality",		TF2Items_SetQuality },
	{ "TF2Items_GetQuality",		TF2Items_GetQuality },
	{ "TF2Items_SetLevel",			TF2Items_SetLevel },
	{ "TF2Items_GetLevel",			TF2Items_GetLevel },
	{ "TF2Items_SetNumAttributes",	TF2Items_SetNumAttributes },
	{ "TF2Items_GetNumAttributes",	TF2Items_GetNumAttributes },
	{ "TF2Items_SetAttribute",		TF2Items_SetAttribute },
	{ "TF2Items_GetAttributeId",	TF2Items_GetAttributeId },
	{ "TF2Items_GetAttributeValue",	TF2Items_GetAttributeValue },
	{ "TF2Items_GiveNamedItem"	,	TF2Items_GiveNamedItem },
	{ NULL,							NULL }
};

CBaseEntity * Native_GiveNamedItem(CBaseEntity * p_hPlayer, TScriptedItemOverride * p_hOverride, IPluginContext *pContext)
{
	// Check if the vtable for CScriptCreatedItem is known at this point.
	if (g_pScriptCreatedVTable == NULL)
	{
		if (pContext != NULL) pContext->ThrowNativeError("The virtual table for CScriptCreatedItem is unknown at this point. Impossible to summon GiveNamedItem.");
		else                  g_pSM->LogError(myself, "The virtual table for CScriptCreatedItem is unknown at this point. Impossible to summon GiveNamedItem.");
		return NULL;
	}

	// Create new script created item object and prepare it.
	CScriptCreatedItem hScriptCreatedItem;
	memset(&hScriptCreatedItem, 0, sizeof(CScriptCreatedItem));
	
	char * strWeaponClassname = p_hOverride->m_strWeaponClassname;
	hScriptCreatedItem.m_pVTable = g_pScriptCreatedVTable;
	hScriptCreatedItem.m_iItemDefinitionIndex = p_hOverride->m_iItemDefinitionIndex;
	hScriptCreatedItem.m_iEntityLevel = p_hOverride->m_iEntityLevel;
	hScriptCreatedItem.m_iEntityQuality = p_hOverride->m_iEntityQuality;
	hScriptCreatedItem.m_pAttributes = hScriptCreatedItem.m_pAttributes2 = p_hOverride->m_Attributes;
	hScriptCreatedItem.m_iAttributesCount = hScriptCreatedItem.m_iAttributesLength = p_hOverride->m_iCount;
	hScriptCreatedItem.m_bInitialized = true;
	if (hScriptCreatedItem.m_iEntityLevel == 0) hScriptCreatedItem.m_iEntityLevel = 9;

	// Call the function.
	return SH_MCALL(p_hPlayer, MHook_GiveNamedItem)(strWeaponClassname, 0, &hScriptCreatedItem, 0);
}

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

	// Retrieve the pointer to the CScriptCreatedItem vtable
	if (g_pScriptCreatedVTable == NULL) g_pScriptCreatedVTable = cscript->m_pVTable;

	// Retrieve client index and auth string.
	edict_t *playerEdict = gameents->BaseEntityToEdict((CBaseEntity *)player);
	IGamePlayer * pPlayer = playerhelpers->GetGamePlayer(playerEdict);
	int client = gamehelpers->IndexOfEdict(playerEdict);
	const char *steamID = pPlayer->GetAuthString();

	// Summon forward
	cell_t cellResults = 0;
	cell_t cellOverrideHandle = 0;
	g_pForwardGiveItem->PushCell(client);
	g_pForwardGiveItem->PushString(item);
	g_pForwardGiveItem->PushCell(cscript->m_iItemDefinitionIndex);
	g_pForwardGiveItem->PushCellByRef(&cellOverrideHandle);
	g_pForwardGiveItem->Execute(&cellResults);

	KeyValues *player_weapons;
	KeyValues *player_weapon;

	// Determine what to do
	switch(cellResults) {
		case Pl_Continue:
			{
				if (strcmp(g_pCustomWeapons->GetName(), "weapon_invalid") == 0)
					RETURN_META_VALUE(MRES_IGNORED, NULL);
			
				player_weapons = new KeyValues("weapon_invalid");
				player_weapon = new KeyValues("weapon_invalid");

				if (KV_FindSection(player_weapons, g_pCustomWeapons, steamID)) {
					if (KV_FindSection(player_weapon, player_weapons, cscript->m_iItemDefinitionIndex)) {
						CScriptCreatedItem newitem = EditWeaponFromFile(cscript, player_weapon);
						RETURN_META_VALUE_MNEWPARAMS(MRES_HANDLED, NULL, MHook_GiveNamedItem, (item, a, &newitem, b));
					} else if (KV_FindSection(player_weapon, player_weapons, "*")) {
						CScriptCreatedItem newitem = EditWeaponFromFile(cscript, player_weapon);
						RETURN_META_VALUE_MNEWPARAMS(MRES_HANDLED, NULL, MHook_GiveNamedItem, (item, a, &newitem, b));
					} else if (KV_FindSection(player_weapons, g_pCustomWeapons, "*")) {
						if (KV_FindSection(player_weapon, player_weapons, cscript->m_iItemDefinitionIndex)) {
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
					if (KV_FindSection(player_weapon, player_weapons, cscript->m_iItemDefinitionIndex)) {
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
			break;
		case Pl_Changed:
			{
				TScriptedItemOverride * pScriptedItemOverride = GetScriptedItemOverrideFromHandle(cellOverrideHandle);
				if (pScriptedItemOverride == NULL) {
					RETURN_META_VALUE(MRES_IGNORED, NULL);
				}

				// Execute the new attributes set and we're done!
				char * finalitem = (char*) item;
				CScriptCreatedItem newitem;
				memcpy(&newitem, cscript, sizeof(CScriptCreatedItem));

				// Override based on the flags passed to this object.
				if (pScriptedItemOverride->m_bFlags & OVERRIDE_CLASSNAME) finalitem = pScriptedItemOverride->m_strWeaponClassname;
				if (pScriptedItemOverride->m_bFlags & OVERRIDE_ITEM_DEF) newitem.m_iItemDefinitionIndex = pScriptedItemOverride->m_iItemDefinitionIndex;
				if (pScriptedItemOverride->m_bFlags & OVERRIDE_ITEM_LEVEL) newitem.m_iEntityLevel = pScriptedItemOverride->m_iEntityLevel;
				if (pScriptedItemOverride->m_bFlags & OVERRIDE_ITEM_QUALITY) newitem.m_iEntityQuality = pScriptedItemOverride->m_iEntityQuality;
				if (pScriptedItemOverride->m_bFlags & OVERRIDE_ATTRIBUTES)
				{
					// Even if we don't want to override the item quality, do if it's set to
					// 0.
					if (newitem.m_iEntityQuality == 0) newitem.m_iEntityQuality = 9;

					// Setup the attributes.
					newitem.m_pAttributes = newitem.m_pAttributes2 = pScriptedItemOverride->m_Attributes;
					newitem.m_iAttributesCount = newitem.m_iAttributesLength = pScriptedItemOverride->m_iCount;
				}

				// Done
				RETURN_META_VALUE_MNEWPARAMS(MRES_HANDLED, NULL, MHook_GiveNamedItem, (finalitem, a, &newitem, b));
			}
			break;
		case Pl_Stop:
			RETURN_META_VALUE(MRES_IGNORED, NULL);
			break;
	}
	
	RETURN_META_VALUE(MRES_IGNORED, NULL);
}

CScriptCreatedItem EditWeaponFromFile(CScriptCreatedItem *cscript, KeyValues *player_weapon)
{
	CScriptCreatedItem newitem;
	memcpy(&newitem, cscript, sizeof(CScriptCreatedItem));

	int itemlevel;
	if (KV_FindValue(&itemlevel, player_weapon, "level")) {
		newitem.m_iEntityLevel = itemlevel;
	}

	int itemquality;
	if (KV_FindValue(&itemquality, player_weapon, "quality")) {
		newitem.m_iEntityQuality = itemquality;
	}

	int attrib_count;
	KeyValues *weapon_attribs = new KeyValues("weapon_invalid");
	if (KV_FindValue(&attrib_count, player_weapon, "attrib_count") && KV_FindSection(weapon_attribs, player_weapon, "attributes")) {

		if (newitem.m_iEntityQuality == 0) {
			newitem.m_iEntityQuality = 9;
		}

		#ifdef TF2ITEMS_DEBUG_ITEMS
			META_CONPRINTF("Attribute Count: %d\n", attrib_count);
		#endif // TF2ITEMS_DEBUG_ITEMS

		newitem.m_pAttributes = (CScriptCreatedAttribute *)malloc(sizeof(CScriptCreatedAttribute) * attrib_count);
		CScriptCreatedAttribute qq;

		int searchindex = 0;
		KeyValues *weapon_attrib;
		for ( weapon_attrib = weapon_attribs->GetFirstValue(); weapon_attrib; weapon_attrib = weapon_attrib->GetNextValue() ) {
			qq.m_iAttributeDefinitionIndex = atoi(weapon_attrib->GetName());
			qq.m_flValue = weapon_attrib->GetFloat();
			memcpy(newitem.m_pAttributes + searchindex, &qq, sizeof(qq));

			#ifdef TF2ITEMS_DEBUG_ITEMS
				META_CONPRINTF("Attribute %d Index: %d\n", searchindex, qq.m_iAttributeDefinitionIndex);
				META_CONPRINTF("Attribute %d Value: %f\n", searchindex, qq.m_flValue);
			#endif // TF2ITEMS_DEBUG_ITEMS

			searchindex = searchindex + 1;
		}

		#ifdef TF2ITEMS_DEBUG_ITEMS
			META_CONPRINTF("Found Attribute Count: %d\n", searchindex);
		#endif // TF2ITEMS_DEBUG_ITEMS

		newitem.m_pAttributes2 = newitem.m_pAttributes;
		newitem.m_iAttributesCount = attrib_count;
		newitem.m_iAttributesLength = attrib_count;
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

	int iOffset;
	if (!g_pGameConf->GetOffset("GiveNamedItem", &iOffset))
	{
		snprintf(error, maxlen, "Could not find offset for GiveNamedItem");
		return false;
	} else {
		SH_MANUALHOOK_RECONFIGURE(MHook_GiveNamedItem, iOffset, 0, 0);
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

	if (g_pCustomWeapons->LoadFromFile(filesystem, m_File)) {
		if (strcmp(g_pCustomWeapons->GetName(), "custom_weapons_v2") != 0) {
			snprintf(error, maxlen, "customweps.txt structure corrupt or incorrect version\n");
			return false;
		}
	}

	// Register natives for Pawn
	sharesys->AddNatives(myself, g_ExtensionNatives);
	sharesys->RegisterLibrary(myself, "TF2Items");

	// Create handles
	g_ScriptedItemOverrideHandleType = g_pHandleSys->CreateType("TF2ItemType", &g_ScriptedItemOverrideHandler,  0,   NULL, NULL,  myself->GetIdentity(),  NULL);

	// Create forwards
	g_pForwardGiveItem = g_pForwards->CreateForward("TF2Items_OnGiveNamedItem", ET_Single, 4, NULL, Param_Cell, Param_String, Param_Cell, Param_CellByRef);

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

bool TF2Items::SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{

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
	g_pHandleSys->RemoveType(g_ScriptedItemOverrideHandleType, myself->GetIdentity());
	g_pForwards->ReleaseForward(g_pForwardGiveItem);
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

void TScriptedItemOverrideTypeHandler::OnHandleDestroy(HandleType_t type, void *object)
{
	TScriptedItemOverride * pScriptedItemOverride = (TScriptedItemOverride*) object;
	if (pScriptedItemOverride != NULL) delete(pScriptedItemOverride);
}

static cell_t TF2Items_CreateItem(IPluginContext *pContext, const cell_t *params)
{
	TScriptedItemOverride * pScriptedItemOverride = new TScriptedItemOverride;
	pScriptedItemOverride->m_bFlags = params[1];

	return g_pHandleSys->CreateHandle(g_ScriptedItemOverrideHandleType, pScriptedItemOverride, pContext->GetIdentity(), myself->GetIdentity(), NULL);
}

static cell_t TF2Items_SetFlags(IPluginContext *pContext, const cell_t *params)
{
	TScriptedItemOverride * pScriptedItemOverride = GetScriptedItemOverrideFromHandle(params[1], pContext);
	if (pScriptedItemOverride != NULL)
	{
		pScriptedItemOverride->m_bFlags = params[2];
	}
	return 0;
}

static cell_t TF2Items_GetFlags(IPluginContext *pContext, const cell_t *params)
{
	TScriptedItemOverride * pScriptedItemOverride = GetScriptedItemOverrideFromHandle(params[1], pContext);
	if (pScriptedItemOverride != NULL)
	{
		return pScriptedItemOverride->m_bFlags;
	}
	return 0;
}

static cell_t TF2Items_SetClassname(IPluginContext *pContext, const cell_t *params)
{
	TScriptedItemOverride * pScriptedItemOverride = GetScriptedItemOverrideFromHandle(params[1], pContext);
	if (pScriptedItemOverride != NULL)
	{
		char * strSource; pContext->LocalToString(params[2], &strSource);
		snprintf(pScriptedItemOverride->m_strWeaponClassname, 256, "%s", strSource);
	}
	return 0;
}

static cell_t TF2Items_GetClassname(IPluginContext *pContext, const cell_t *params)
{
	TScriptedItemOverride * pScriptedItemOverride = GetScriptedItemOverrideFromHandle(params[1], pContext);
	if (pScriptedItemOverride != NULL)
	{
		char * strSource = pScriptedItemOverride->m_strWeaponClassname;
		char * strDestiny; pContext->LocalToString(params[2], &strDestiny);
		int iSourceSize = strlen(strSource);
		int iDestinySize = params[3];

		// Perform bounds checking
		if (iSourceSize >= iDestinySize)	iSourceSize = iDestinySize-1;
		else								iSourceSize = iDestinySize;
	 
		// Copy
		memmove(strDestiny, strSource, iSourceSize);
		strDestiny[iSourceSize] = '\0';
	}
	return 0;
}

static cell_t TF2Items_SetItemIndex(IPluginContext *pContext, const cell_t *params)
{
	TScriptedItemOverride * pScriptedItemOverride = GetScriptedItemOverrideFromHandle(params[1], pContext);
	if (pScriptedItemOverride != NULL)
	{
		pScriptedItemOverride->m_iItemDefinitionIndex = params[2];
	}
	return 0;
}

static cell_t TF2Items_GetItemIndex(IPluginContext *pContext, const cell_t *params)
{
	TScriptedItemOverride * pScriptedItemOverride = GetScriptedItemOverrideFromHandle(params[1], pContext);
	if (pScriptedItemOverride != NULL)
	{
		return pScriptedItemOverride->m_iItemDefinitionIndex;
	}
	return -1;
}

static cell_t TF2Items_SetQuality(IPluginContext *pContext, const cell_t *params)
{
	TScriptedItemOverride * pScriptedItemOverride = GetScriptedItemOverrideFromHandle(params[1], pContext);
	if (pScriptedItemOverride != NULL)
	{
		if (params[2] < 0 || params[2] > 9) return pContext->ThrowNativeError("Quality index %d is out of bounds.", params[2]);
		pScriptedItemOverride->m_iEntityQuality = params[2];
	}
	return 0;
}

static cell_t TF2Items_GetQuality(IPluginContext *pContext, const cell_t *params)
{
	TScriptedItemOverride * pScriptedItemOverride = GetScriptedItemOverrideFromHandle(params[1], pContext);
	if (pScriptedItemOverride != NULL)
	{
		return pScriptedItemOverride->m_iEntityQuality;
	}
	return 0;
}

static cell_t TF2Items_SetLevel(IPluginContext *pContext, const cell_t *params)
{
	TScriptedItemOverride * pScriptedItemOverride = GetScriptedItemOverrideFromHandle(params[1], pContext);
	if (pScriptedItemOverride != NULL)
	{
		pScriptedItemOverride->m_iEntityLevel = params[2];
	}
	return 0;
}

static cell_t TF2Items_GetLevel(IPluginContext *pContext, const cell_t *params)
{
	TScriptedItemOverride * pScriptedItemOverride = GetScriptedItemOverrideFromHandle(params[1], pContext);
	if (pScriptedItemOverride != NULL)
	{
		return pScriptedItemOverride->m_iEntityLevel;
	}
	return 0;
}

static cell_t TF2Items_SetNumAttributes(IPluginContext *pContext, const cell_t *params)
{
	TScriptedItemOverride * pScriptedItemOverride = GetScriptedItemOverrideFromHandle(params[1], pContext);
	if (pScriptedItemOverride != NULL)
	{
		if (params[2] < 0 || params[2] > 15) { pContext->ThrowNativeError("Attributes size out of bounds: %i [0 ... 15]", params[2]); return 0; }
		pScriptedItemOverride->m_iCount = params[2];
	}
	return 0;
}

static cell_t TF2Items_GetNumAttributes(IPluginContext *pContext, const cell_t *params)
{
	TScriptedItemOverride * pScriptedItemOverride = GetScriptedItemOverrideFromHandle(params[1], pContext);
	if (pScriptedItemOverride != NULL)
	{
		return pScriptedItemOverride->m_iCount;
	}
	return -1;
}

static cell_t TF2Items_SetAttribute(IPluginContext *pContext, const cell_t *params)
{
	TScriptedItemOverride * pScriptedItemOverride = GetScriptedItemOverrideFromHandle(params[1], pContext);
	if (pScriptedItemOverride != NULL)
	{
		if (params[2] < 0 || params[2] > 15)
		{
			pContext->ThrowNativeError("Attribute index out of bounds: %d", params[2]);
			return 0;
		}
		pScriptedItemOverride->m_Attributes[params[2]].m_iAttributeDefinitionIndex = params[3];
		pScriptedItemOverride->m_Attributes[params[2]].m_flValue = sp_ctof(params[4]);
		return 1;
	}

	return 0;
}

static cell_t TF2Items_GetAttributeId(IPluginContext *pContext, const cell_t *params)
{
	TScriptedItemOverride * pScriptedItemOverride = GetScriptedItemOverrideFromHandle(params[1], pContext);
	if (pScriptedItemOverride != NULL)
	{
		if (params[2] < 0 || params[2] > 15)
		{
			pContext->ThrowNativeError("Attribute index out of bounds: %d", params[2]);
			return 0;
		}
		return pScriptedItemOverride->m_Attributes[params[2]].m_iAttributeDefinitionIndex;
	}
	return -1;
}

static cell_t TF2Items_GetAttributeValue(IPluginContext *pContext, const cell_t *params)
{
	TScriptedItemOverride * pScriptedItemOverride = GetScriptedItemOverrideFromHandle(params[1], pContext);
	if (pScriptedItemOverride != NULL)
	{
		if (params[2] < 0 || params[2] > 15)
		{
			return pContext->ThrowNativeError("Attribute index out of bounds: %d", params[2]);
		}
		return sp_ctof(pScriptedItemOverride->m_Attributes[params[2]].m_flValue);
	}
	return sp_ctof(0.0f);
}

CBaseEntity * GetCBaseEntityFromIndex(int p_iEntity, bool p_bOnlyPlayers)
{
	edict_t *edtEdict = engine->PEntityOfEntIndex(p_iEntity);
	if (!edtEdict || edtEdict->IsFree()) return NULL;
	
	if (p_iEntity > 0 && p_iEntity <= playerhelpers->GetMaxClients())
	{
		IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(edtEdict);
		if (!pPlayer || !pPlayer->IsConnected()) return NULL;
	}
	else if (p_bOnlyPlayers) return NULL;

	IServerUnknown *pUnk;
	if ((pUnk=edtEdict->GetUnknown()) == NULL) return NULL;
	return pUnk->GetBaseEntity();
}

int GetIndexFromCBaseEntity(CBaseEntity * p_hEntity)
{
	if (p_hEntity == NULL) return -1;

	edict_t * edtEdict = gameents->BaseEntityToEdict(p_hEntity);
	if (!edtEdict || edtEdict->IsFree()) return -1;

	return gamehelpers->IndexOfEdict(edtEdict);
}

static cell_t TF2Items_GiveNamedItem(IPluginContext *pContext, const cell_t *params)
{
	// Retrieve player from it's index.
	CBaseEntity *pEntity;
	if (!(pEntity = GetCBaseEntityFromIndex(params[1], true)))
		return pContext->ThrowNativeError("Client index %d is not valid", params[1]);

	// Retrieve the item override handle
	TScriptedItemOverride * pScriptedItemOverride = GetScriptedItemOverrideFromHandle(params[2], pContext);
	if (pScriptedItemOverride == NULL)
		return -1;

	// Summon the native and retrieve it's results
	CBaseEntity * hResults = Native_GiveNamedItem(pEntity, pScriptedItemOverride, pContext);
	return GetIndexFromCBaseEntity(hResults);
}

TScriptedItemOverride * GetScriptedItemOverrideFromHandle(cell_t cellHandle, IPluginContext *pContext)
{
	Handle_t hndlScriptedItemOverride = static_cast<Handle_t>(cellHandle);
	HandleError hndlError;
	HandleSecurity hndlSec;
 
	// Build our security descriptor
	hndlSec.pOwner = NULL;
	hndlSec.pIdentity = myself->GetIdentity();
 
	// Attempt to read the given handle as our type, using our security info.
	TScriptedItemOverride * pScriptedItemOverride;
	if ((hndlError = g_pHandleSys->ReadHandle(hndlScriptedItemOverride, g_ScriptedItemOverrideHandleType, &hndlSec, (void **)&pScriptedItemOverride)) != HandleError_None)
	{
		if (pContext == NULL)	g_pSM->LogError(myself, "Invalid TF2ItemType handle %x (error %d)", hndlScriptedItemOverride, hndlError);
		else					pContext->ThrowNativeError("Invalid TF2ItemType handle %x (error %d)", hndlScriptedItemOverride, hndlError);
		return NULL;
	}

	// Done
	return pScriptedItemOverride;
}