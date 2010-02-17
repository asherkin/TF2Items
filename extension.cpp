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
 *	voogru & Drunken_F00l	-	Inspiring the creation of this.
 */

#include "extension.h"

/*
 *	Debugging options:
 *	==================
 */
//#define TF2ITEMS_DEBUG_HOOKING
//#define TF2ITEMS_DEBUG_ITEMS

TF2Items g_TF2Items;

SMEXT_LINK(&g_TF2Items);

SH_DECL_HOOK2_void(IServerGameClients, ClientPutInServer, SH_NOATTRIB, 0, edict_t *, char const *);
SH_DECL_MANUALHOOK4(MHook_GiveNamedItem, 0, 0, 0, CBaseEntity *, char const *, int, CScriptCreatedItem *, bool);
//SH_DECL_MANUALHOOK2(MHook_GiveNamedItemBackup, 385, 0, 0, CBaseEntity *, char const *, int);

ICvar *icvar = NULL;
IServerGameClients *gameclients = NULL;
IServerGameEnts *gameents = NULL;

ConVar TF2ItemsVersion("tf2items_version", "1.3.0", FCVAR_SPONLY|FCVAR_REPLICATED|FCVAR_NOTIFY, "TF2 Items Version");

IGameConfig *g_pGameConf = NULL;

int GiveNamedItem_Hook = 0;
int ClientPutInServer_Hook = 0;

IForward * g_pForwardGiveItem = NULL;
HandleType_t g_ScriptedItemOverrideHandleType = 0;
TScriptedItemOverrideTypeHandler g_ScriptedItemOverrideHandler;

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

CBaseEntity * Native_GiveNamedItem(CBaseEntity * p_hPlayer, TScriptedItemOverride * p_hOverride, IPluginContext *pContext) {

	// Create new script created item object and prepare it.
	CScriptCreatedItem hScriptCreatedItem;
	memset(&hScriptCreatedItem, 0, sizeof(CScriptCreatedItem));
	
	char * strWeaponClassname = p_hOverride->m_strWeaponClassname;
	hScriptCreatedItem.m_iItemDefinitionIndex = p_hOverride->m_iItemDefinitionIndex;
	hScriptCreatedItem.m_iEntityLevel = p_hOverride->m_iEntityLevel;
	hScriptCreatedItem.m_iEntityQuality = p_hOverride->m_iEntityQuality;
	hScriptCreatedItem.m_pAttributes = hScriptCreatedItem.m_pAttributes2 = p_hOverride->m_Attributes;
	hScriptCreatedItem.m_iAttributesCount = hScriptCreatedItem.m_iAttributesLength = p_hOverride->m_iCount;
	hScriptCreatedItem.m_bInitialized = true;
	if (hScriptCreatedItem.m_iEntityQuality == 0 && hScriptCreatedItem.m_iAttributesCount > 0) hScriptCreatedItem.m_iEntityQuality = 9;

	// Call the function.
	CBaseEntity *tempItem = NULL;
	tempItem = SH_MCALL(p_hPlayer, MHook_GiveNamedItem)(strWeaponClassname, 0, &hScriptCreatedItem, 0);

	if (tempItem == NULL) {
		pContext->ThrowNativeError("Item is NULL. You have hit Bug 18.");
		//g_pSM->LogError(myself, "Item is NULL.");
		//tempItem = SH_MCALL(p_hPlayer, MHook_GiveNamedItem)(strWeaponClassname, 0, NULL, 0);
	}

	//if (tempItem == NULL) {
	//	g_pSM->LogError(myself, "Item is still NULL.");
	//	tempItem = SH_MCALL(p_hPlayer, MHook_GiveNamedItemBackup)(strWeaponClassname, 0);
	//}

	//if (tempItem == NULL) {
	//	g_pSM->LogError(myself, "Item is fucked.");
	//}

	return tempItem;
}

CBaseEntity *Hook_GiveNamedItem(char const *item, int a, CScriptCreatedItem *cscript, bool b) {

	#ifdef TF2ITEMS_DEBUG_HOOKING
		 g_pSM->LogMessage(myself, "GiveNamedItem called.");
	#endif // TF2ITEMS_DEBUG_HOOKING

	CBasePlayer *player = META_IFACEPTR(CBasePlayer);

	if (cscript == NULL) {
		RETURN_META_VALUE(MRES_IGNORED, NULL);
	}

	// Retrieve client index and auth string.
	edict_t *playerEdict = gameents->BaseEntityToEdict((CBaseEntity *)player);
	IGamePlayer * pPlayer = playerhelpers->GetGamePlayer(playerEdict);
	int client = gamehelpers->IndexOfEdict(playerEdict);

#ifdef TF2ITEMS_DEBUG_ITEMS
	g_pSM->LogMessage(myself, "---------------------------------------");
	g_pSM->LogMessage(myself, ">>> ItemDefinitionIndex = %d", cscript->m_iItemDefinitionIndex);
	g_pSM->LogMessage(myself, ">>> ClassName = %s", item);
	g_pSM->LogMessage(myself, "---------------------------------------");
#endif

	// Summon forward
	cell_t cellResults = 0;
	cell_t cellOverrideHandle = 0;
	g_pForwardGiveItem->PushCell(client);
	g_pForwardGiveItem->PushString(item);
	g_pForwardGiveItem->PushCell(cscript->m_iItemDefinitionIndex);
	g_pForwardGiveItem->PushCellByRef(&cellOverrideHandle);
	g_pForwardGiveItem->Execute(&cellResults);

	// Determine what to do
	switch(cellResults) {
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
					// Even if we don't want to override the item quality, do if it's set to 0.
					if (newitem.m_iEntityQuality == 0 && pScriptedItemOverride->m_iCount > 0) newitem.m_iEntityQuality = 9;

					// Setup the attributes.
					newitem.m_pAttributes = newitem.m_pAttributes2 = pScriptedItemOverride->m_Attributes;
					newitem.m_iAttributesCount = newitem.m_iAttributesLength = pScriptedItemOverride->m_iCount;
				}

				// Done
				RETURN_META_VALUE_MNEWPARAMS(MRES_HANDLED, NULL, MHook_GiveNamedItem, (finalitem, a, &newitem, b));
			}
			break;
	}
	
	RETURN_META_VALUE(MRES_IGNORED, NULL);
}

void Hook_ClientPutInServer(edict_t *pEntity, char const *playername) {

	#ifdef TF2ITEMS_DEBUG_HOOKING
		 g_pSM->LogMessage(myself, "ClientPutInServer called.");
	#endif // TF2ITEMS_DEBUG_HOOKING

	if(GiveNamedItem_Hook == 0 && pEntity->m_pNetworkable) {
		CBaseEntity *baseentity = pEntity->m_pNetworkable->GetBaseEntity();
		if(!baseentity)
			return;

		CBasePlayer *player = (CBasePlayer *)baseentity;

		GiveNamedItem_Hook = SH_ADD_MANUALVPHOOK(MHook_GiveNamedItem, player, SH_STATIC(Hook_GiveNamedItem), false);
		if (ClientPutInServer_Hook != 0) {
			SH_REMOVE_HOOK_ID(ClientPutInServer_Hook);
			ClientPutInServer_Hook = 0;
			#ifdef TF2ITEMS_DEBUG_HOOKING
				 g_pSM->LogMessage(myself, "ClientPutInServer unhooked.");
			#endif // TF2ITEMS_DEBUG_HOOKING
		}

		#ifdef TF2ITEMS_DEBUG_HOOKING
			 g_pSM->LogMessage(myself, "GiveNamedItem hooked.");
		#endif // TF2ITEMS_DEBUG_HOOKING

	} else if (ClientPutInServer_Hook != 0) {
		SH_REMOVE_HOOK_ID(ClientPutInServer_Hook);
		ClientPutInServer_Hook = 0;
		#ifdef TF2ITEMS_DEBUG_HOOKING
			 g_pSM->LogMessage(myself, "ClientPutInServer unhooked.");
		#endif // TF2ITEMS_DEBUG_HOOKING
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

	int iOffset;
	if (!g_pGameConf->GetOffset("GiveNamedItem", &iOffset))
	{
		snprintf(error, maxlen, "Could not find offset for GiveNamedItem");
		return false;
	} else {
		SH_MANUALHOOK_RECONFIGURE(MHook_GiveNamedItem, iOffset, 0, 0);
	}

	// If it's a late load, there might be the chance there are players already on the server. Just
	// check for this and try to hook them instead of waiting for the next player. -- Damizean
	if (late) {
		#ifdef TF2ITEMS_DEBUG_HOOKING
			 g_pSM->LogMessage(myself, "Is a late load, attempting to hook GiveNamedItem.");
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
			
			if (GiveNamedItem_Hook != 0) {
				#ifdef TF2ITEMS_DEBUG_HOOKING
					 g_pSM->LogMessage(myself, "GiveNamedItem hooked.");
				#endif // TF2ITEMS_DEBUG_HOOKING
				break;
			}
		}
	}

	if (GiveNamedItem_Hook == 0) {
		#ifdef TF2ITEMS_DEBUG_HOOKING
			 g_pSM->LogMessage(myself, "Is a NOT late load or no players found, attempting to hook ClientPutInServer.");
		#endif // TF2ITEMS_DEBUG_HOOKING
		ClientPutInServer_Hook = SH_ADD_HOOK_STATICFUNC(IServerGameClients, ClientPutInServer, gameclients, Hook_ClientPutInServer, true);
		#ifdef TF2ITEMS_DEBUG_HOOKING
			 g_pSM->LogMessage(myself, "ClientPutInServer hooked.");
		#endif // TF2ITEMS_DEBUG_HOOKING
	}

	// Register natives for Pawn
	sharesys->AddNatives(myself, g_ExtensionNatives);
	sharesys->RegisterLibrary(myself, "TF2Items");

	// Create handles
	g_ScriptedItemOverrideHandleType = g_pHandleSys->CreateType("TF2ItemType", &g_ScriptedItemOverrideHandler,  0,   NULL, NULL,  myself->GetIdentity(),  NULL);

	// Create forwards
	g_pForwardGiveItem = g_pForwards->CreateForward("TF2Items_OnGiveNamedItem", ET_Hook, 4, NULL, Param_Cell, Param_String, Param_Cell, Param_CellByRef);

	g_pSM->LogMessage(myself, "\"GiveNamedItem\" offset = %d", iOffset);

	return true;
}

bool TF2Items::SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{

	GET_V_IFACE_ANY(GetServerFactory, gameclients, IServerGameClients, INTERFACEVERSION_SERVERGAMECLIENTS);
	GET_V_IFACE_ANY(GetServerFactory, gameents, IServerGameEnts, INTERFACEVERSION_SERVERGAMEENTS);
	GET_V_IFACE_CURRENT(GetEngineFactory, icvar, ICvar, CVAR_INTERFACE_VERSION);

	if (!gameents)
	{
		snprintf(error, maxlen, "Could not find interface %s", INTERFACEVERSION_SERVERGAMEENTS);
		return false;
	}
	if (!gameclients)
	{
		snprintf(error, maxlen, "Could not find interface %s", INTERFACEVERSION_SERVERGAMECLIENTS);
		return false;
	}
	if (!icvar)
	{
		snprintf(error, maxlen, "Could not find interface %s", CVAR_INTERFACE_VERSION);
		return false;
	}

	g_pCVar = icvar;

	ConVar_Register(0, this);

	return true;
}

void TF2Items::SDK_OnUnload() {

	#ifdef TF2ITEMS_DEBUG_HOOKING
		 g_pSM->LogMessage(myself, "SDK_OnUnload called.");
	#endif // TF2ITEMS_DEBUG_HOOKING

	gameconfs->CloseGameConfigFile(g_pGameConf);
	g_pHandleSys->RemoveType(g_ScriptedItemOverrideHandleType, myself->GetIdentity());
	g_pForwards->ReleaseForward(g_pForwardGiveItem);
}

bool TF2Items::SDK_OnMetamodUnload(char *error, size_t maxlen) {

	#ifdef TF2ITEMS_DEBUG_HOOKING
		 g_pSM->LogMessage(myself, "SDK_OnMetamodUnload called.");
	#endif // TF2ITEMS_DEBUG_HOOKING

	if (ClientPutInServer_Hook != 0) {
		SH_REMOVE_HOOK_ID(ClientPutInServer_Hook);
		ClientPutInServer_Hook = 0;
		#ifdef TF2ITEMS_DEBUG_HOOKING
			 g_pSM->LogMessage(myself, "ClientPutInServer unhooked.");
		#endif // TF2ITEMS_DEBUG_HOOKING
	}
	#ifdef TF2ITEMS_DEBUG_HOOKING
		else {
			 g_pSM->LogMessage(myself, "ClientPutInServer did not need to be unhooked.");
		}
	#endif // TF2ITEMS_DEBUG_HOOKING

	if (GiveNamedItem_Hook != 0) {
		SH_REMOVE_HOOK_ID(GiveNamedItem_Hook);
		GiveNamedItem_Hook = 0;
		#ifdef TF2ITEMS_DEBUG_HOOKING
			 g_pSM->LogMessage(myself, "GiveNamedItem unhooked.");
		#endif // TF2ITEMS_DEBUG_HOOKING
	}
	#ifdef TF2ITEMS_DEBUG_HOOKING
		else {
			 g_pSM->LogMessage(myself, "GiveNamedItem did not need to be unhooked.");
		}
	#endif // TF2ITEMS_DEBUG_HOOKING

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
		return sp_ftoc(pScriptedItemOverride->m_Attributes[params[2]].m_flValue);
	}
	return sp_ftoc(0.0f);
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
