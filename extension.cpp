/*
 * ================================================================================
 * TF2 Items Extension
 * Copyright (C) 2009-2010 AzuiSleet, Asher Baker (asherkin).  All rights reserved.
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
 *	Attributions & Thanks:
 *	=====================
 *	AzuiSleet				-	Reversed CScriptCreatedItem and released it publicly, along with writing most of the item editing code below.
 *	Damizean				-	Fixed padding for CScriptCreatedItem in Linux. Wrote the SourcePawn Interface and the SourceMod item manager.
 *	Voogru					-	Inspiring the creation of this. Helped with fixing and improving the CScriptCreatedItem class used after the 119 update.
 *	Wazz					-	Wrote "Shit not be void" in #sourcemod and revealed that GiveNamedItem returned CBaseEntity *. Helped with improving the CScriptCreatedItem class.
 *	Psychonic				-	"How did you write the wearable natives asherkin?" "I got all the code from psychonic, then disregarded it and wrote it from scratch."
 *	MatthiasVance			-	Reminded me to comment out '#define INFINITE_PROBLEMS 1'.
 *	Drunken_F00l			-	Inspiring the creation of this.
 */

/*
 *	Debugging options:
 *	==================
 */
//#define TF2ITEMS_DEBUG_HOOKING
//#define TF2ITEMS_DEBUG_ITEMS

#define USE_NEW_ATTRIBS // Use a CUtlVector for the attibutes
#define NO_FORCE_QUALITY

#include "extension.hpp"

TF2Items g_TF2Items;

SMEXT_LINK(&g_TF2Items);

SH_DECL_HOOK2_void(IServerGameClients, ClientPutInServer, SH_NOATTRIB, 0, edict_t *, char const *);
SH_DECL_MANUALHOOK4(MHook_GiveNamedItem, 0, 0, 0, CBaseEntity *, char const *, int, CScriptCreatedItem *, bool);

ICvar *icvar = NULL;
IServerGameClients *gameclients = NULL;
IServerGameEnts *gameents = NULL;

ConVar TF2ItemsVersion("tf2items_version", SMEXT_CONF_VERSION, FCVAR_SPONLY|FCVAR_REPLICATED|FCVAR_NOTIFY, "TF2 Items Version");
ConVar HookTFBot("tf2items_bothook", "1", FCVAR_NONE, "Hook intelligent TF2 bots.");

IGameConfig *g_pGameConf = NULL;

int GiveNamedItem_player_Hook = 0;
int GiveNamedItem_bot_Hook = 0;
int ClientPutInServer_Hook = 0;

#ifdef USE_NEW_ATTRIBS
int g_iEntityQualityOffset = 0;
#endif

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
#ifdef USE_NEW_ATTRIBS
	hScriptCreatedItem.m_Attributes.CopyArray(p_hOverride->m_Attributes, p_hOverride->m_iCount);
#else
	hScriptCreatedItem.m_pAttributes = hScriptCreatedItem.m_pAttributes2 = p_hOverride->m_Attributes;
	hScriptCreatedItem.m_iAttributesCount = hScriptCreatedItem.m_iAttributesLength = p_hOverride->m_iCount;
#endif
	hScriptCreatedItem.m_bInitialized = true;

#ifndef USE_NEW_ATTRIBS
#ifndef NO_FORCE_QUALITY
	if (hScriptCreatedItem.m_iEntityQuality == 0 && hScriptCreatedItem.m_iAttributesCount > 0) hScriptCreatedItem.m_iEntityQuality = 3;
#endif
#endif

	// Call the function.
	CBaseEntity *tempItem = NULL;
	tempItem = SH_MCALL(p_hPlayer, MHook_GiveNamedItem)(strWeaponClassname, 0, &hScriptCreatedItem, 0);

	if (tempItem == NULL) {
		pContext->ThrowNativeError("Item is NULL. You may have hit Bug 18.");
	}

#ifdef USE_NEW_ATTRIBS
#ifndef NO_FORCE_QUALITY
	if (p_hOverride->m_iEntityQuality == 0 && p_hOverride->m_iCount > 0) p_hOverride->m_iEntityQuality = 3;
#endif

	int *iEntityQuality = (int *)((char *)tempItem + (g_iEntityQualityOffset));
	*iEntityQuality = p_hOverride->m_iEntityQuality;
#endif

	return tempItem;
}

CBaseEntity *Hook_GiveNamedItem(char const *szClassname, int iSubType, CScriptCreatedItem *cscript, bool b) {

	#ifdef TF2ITEMS_DEBUG_HOOKING
		 g_pSM->LogMessage(myself, "GiveNamedItem called.");
	#endif // TF2ITEMS_DEBUG_HOOKING

	#ifdef TF2ITEMS_DEBUG_ITEMS
		 g_pSM->LogMessage(myself, "---------------------------------------");
		 g_pSM->LogMessage(myself, ">>> Start of GiveNamedItem call.");
	#endif

	CBasePlayer *player = META_IFACEPTR(CBasePlayer);

	if (cscript == NULL) {
		RETURN_META_VALUE(MRES_IGNORED, NULL);
	}

	// Retrieve client index.
	edict_t *playerEdict = gameents->BaseEntityToEdict((CBaseEntity *)player);
	IGamePlayer * pPlayer = playerhelpers->GetGamePlayer(playerEdict);
	int client = gamehelpers->IndexOfEdict(playerEdict);

#ifdef TF2ITEMS_DEBUG_ITEMS

	/*
	if (cscript->m_iItemDefinitionIndex == 153) {
	FILE *fp = fopen("debug_item_153.txt", "wb");
	fwrite(cscript, 3552, 1, fp);
	fclose(fp);
	}
	*/

	g_pSM->LogMessage(myself, "---------------------------------------");
	g_pSM->LogMessage(myself, ">>> Client = %s", pPlayer->GetName());
	g_pSM->LogMessage(myself, ">>> szClassname = %s", szClassname);
	g_pSM->LogMessage(myself, ">>> iSubType = %d", iSubType);
	g_pSM->LogMessage(myself, ">>> b = %s", b?"true":"false");
	g_pSM->LogMessage(myself, "---------------------------------------");
	g_pSM->LogMessage(myself, ">>> m_iItemDefinitionIndex = %u", cscript->m_iItemDefinitionIndex);
	g_pSM->LogMessage(myself, ">>> m_iEntityQuality = %u", cscript->m_iEntityQuality);
	g_pSM->LogMessage(myself, ">>> m_iEntityLevel = %u", cscript->m_iEntityLevel);
	g_pSM->LogMessage(myself, ">>> m_iGlobalIndex = %lu", cscript->m_iGlobalIndex);
	g_pSM->LogMessage(myself, ">>> m_iGlobalIndexHigh = %u", cscript->m_iGlobalIndexHigh);
	g_pSM->LogMessage(myself, ">>> m_iGlobalIndexLow = %u", cscript->m_iGlobalIndexLow);
	g_pSM->LogMessage(myself, ">>> m_iAccountID = %u", cscript->m_iAccountID);
	g_pSM->LogMessage(myself, ">>> m_iPosition = %u", cscript->m_iPosition);
	g_pSM->LogMessage(myself, ">>> m_szWideName = %ls", cscript->m_szWideName);
	g_pSM->LogMessage(myself, ">>> m_szName = %s", cscript->m_szName);
	g_pSM->LogMessage(myself, ">>> m_bInitialized = %s", cscript->m_bInitialized?"true":"false");
	g_pSM->LogMessage(myself, "---------------------------------------");
	for (int i = 0; i < ((cscript->m_Attributes.Count() > 16)?0:cscript->m_Attributes.Count()); i++)
	{
		g_pSM->LogMessage(myself, ">>> m_iAttributeDefinitionIndex = %u", cscript->m_Attributes.Element(i).m_iAttributeDefinitionIndex);
		g_pSM->LogMessage(myself, ">>> m_flValue = %f", cscript->m_Attributes.Element(i).m_flValue);
		g_pSM->LogMessage(myself, ">>> m_szDescription = %ls", cscript->m_Attributes.Element(i).m_szDescription);
		g_pSM->LogMessage(myself, "---------------------------------------");
	}
	g_pSM->LogMessage(myself, ">>> Size of CScriptCreatedItem = %d", sizeof(CScriptCreatedItem));
	g_pSM->LogMessage(myself, ">>> Size of CScriptCreatedAttribute = %d", sizeof(CScriptCreatedAttribute));
	g_pSM->LogMessage(myself, ">>> No. of Attributes = %d", cscript->m_Attributes.Count());
	g_pSM->LogMessage(myself, "---------------------------------------");
#endif

	// Summon forward
	cell_t cellResults = 0;
	cell_t cellOverrideHandle = 0;
	g_pForwardGiveItem->PushCell(client);
	g_pForwardGiveItem->PushString(szClassname);
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
				char * finalitem = (char*) szClassname;

				// Override based on the flags passed to this object.
#ifdef USE_NEW_ATTRIBS
				CScriptCreatedItem newitem;
				//memcpy(&newitem, cscript, sizeof(CScriptCreatedItem));
				CSCICopy(cscript, &newitem);

				if (pScriptedItemOverride->m_bFlags & OVERRIDE_CLASSNAME) finalitem = pScriptedItemOverride->m_strWeaponClassname;
				if (pScriptedItemOverride->m_bFlags & OVERRIDE_ITEM_DEF) newitem.m_iItemDefinitionIndex = pScriptedItemOverride->m_iItemDefinitionIndex;
				if (pScriptedItemOverride->m_bFlags & OVERRIDE_ITEM_LEVEL) newitem.m_iEntityLevel = pScriptedItemOverride->m_iEntityLevel;
				if (pScriptedItemOverride->m_bFlags & OVERRIDE_ITEM_QUALITY) newitem.m_iEntityQuality = pScriptedItemOverride->m_iEntityQuality;
				if (pScriptedItemOverride->m_bFlags & OVERRIDE_ATTRIBUTES)
				{
#ifndef NO_FORCE_QUALITY
					// Even if we don't want to override the item quality, do if it's set to 0.
					if (newitem.m_iEntityQuality == 0 && !(pScriptedItemOverride->m_bFlags & OVERRIDE_ITEM_QUALITY) && pScriptedItemOverride->m_iCount > 0) newitem.m_iEntityQuality = 3;
#endif

					if (!(pScriptedItemOverride->m_bFlags & PRESERVE_ATTRIBUTES))
						newitem.m_Attributes.RemoveAll();

					newitem.m_Attributes.AddMultipleToTail(pScriptedItemOverride->m_iCount, pScriptedItemOverride->m_Attributes);
				}

				// Done
				RETURN_META_VALUE_MNEWPARAMS(MRES_HANDLED, NULL, MHook_GiveNamedItem, (finalitem, iSubType, &newitem, b));
#else
				CScriptCreatedItem newitem;
				memcpy(&newitem, cscript, sizeof(CScriptCreatedItem));

				if (pScriptedItemOverride->m_bFlags & OVERRIDE_CLASSNAME) finalitem = pScriptedItemOverride->m_strWeaponClassname;
				if (pScriptedItemOverride->m_bFlags & OVERRIDE_ITEM_DEF) newitem.m_iItemDefinitionIndex = pScriptedItemOverride->m_iItemDefinitionIndex;
				if (pScriptedItemOverride->m_bFlags & OVERRIDE_ITEM_LEVEL) newitem.m_iEntityLevel = pScriptedItemOverride->m_iEntityLevel;
				if (pScriptedItemOverride->m_bFlags & OVERRIDE_ITEM_QUALITY) newitem.m_iEntityQuality = pScriptedItemOverride->m_iEntityQuality;
				if (pScriptedItemOverride->m_bFlags & OVERRIDE_ATTRIBUTES)
				{
#ifndef NO_FORCE_QUALITY
					// Even if we don't want to override the item quality, do if it's set to 0.
					if (newitem.m_iEntityQuality == 0 && pScriptedItemOverride->m_iCount > 0) newitem.m_iEntityQuality = 3;
#endif

					// Setup the attributes.
					newitem.m_pAttributes = newitem.m_pAttributes2 = pScriptedItemOverride->m_Attributes;
					newitem.m_iAttributesCount = newitem.m_iAttributesLength = pScriptedItemOverride->m_iCount;
				}

				// Done
				RETURN_META_VALUE_MNEWPARAMS(MRES_HANDLED, NULL, MHook_GiveNamedItem, (finalitem, iSubType, &newitem, b));
#endif
			}
	}
	
	RETURN_META_VALUE(MRES_IGNORED, NULL);
}

void CSCICopy(CScriptCreatedItem *olditem, CScriptCreatedItem *newitem)
{
	//#define copymember(a) newitem->##a = olditem->##a
	#define copymember(a) memcpy(&newitem->a, &olditem->a, sizeof(CScriptCreatedItem::a));

	copymember(m_pVTable);
	
#ifdef _WIN32
	copymember(m_Padding[4]);
#endif

	copymember(m_iItemDefinitionIndex);
	copymember(m_iEntityQuality);
	copymember(m_iEntityLevel);

#ifdef _WIN32
	copymember(m_Padding2[4]);
#endif

	copymember(m_iGlobalIndex);
	copymember(m_iGlobalIndexHigh);
	copymember(m_iGlobalIndexLow);
	copymember(m_iAccountID);
	copymember(m_iPosition);
	copymember(m_szWideName[128]);
	copymember(m_szName[128]);

	copymember(m_szBlob[20]);
	copymember(m_szBlob2[1536]);

	copymember(m_bInitialized);

#ifdef _WIN32
	copymember(m_Padding3[4]);
#endif

	newitem->m_Attributes = olditem->m_Attributes;
}

void Hook_ClientPutInServer(edict_t *pEntity, char const *playername) {

	#ifdef TF2ITEMS_DEBUG_HOOKING
		 g_pSM->LogMessage(myself, "ClientPutInServer called.");
	#endif // TF2ITEMS_DEBUG_HOOKING

	if(pEntity->m_pNetworkable) {
		CBaseEntity *baseentity = pEntity->m_pNetworkable->GetBaseEntity();
		if(!baseentity)
			return;

		CBasePlayer *player = (CBasePlayer *)baseentity;

		#ifdef TF2ITEMS_DEBUG_HOOKING
			g_pSM->LogMessage(myself, "---------------------------------------");
			g_pSM->LogMessage(myself, ">>> Start of ClientPutInServer call.");
			g_pSM->LogMessage(myself, "---------------------------------------");
			g_pSM->LogMessage(myself, ">>> Client = %s", playername);
			g_pSM->LogMessage(myself, ">>> ClassName = %s", pEntity->GetClassName());
			g_pSM->LogMessage(myself, "---------------------------------------");
		#endif

		if (HookTFBot.GetBool() && strcmp(pEntity->GetClassName(), "tf_bot") == 0)
		{
			if(GiveNamedItem_bot_Hook == 0)
			{
				GiveNamedItem_bot_Hook = SH_ADD_MANUALVPHOOK(MHook_GiveNamedItem, player, SH_STATIC(Hook_GiveNamedItem), false);
				#ifdef TF2ITEMS_DEBUG_HOOKING
					g_pSM->LogMessage(myself, "GiveNamedItem hooked (bot).");
				#endif // TF2ITEMS_DEBUG_HOOKING
			}
		} else {
			if(GiveNamedItem_player_Hook == 0)
			{
				GiveNamedItem_player_Hook = SH_ADD_MANUALVPHOOK(MHook_GiveNamedItem, player, SH_STATIC(Hook_GiveNamedItem), false);
				#ifdef TF2ITEMS_DEBUG_HOOKING
					g_pSM->LogMessage(myself, "GiveNamedItem hooked (player).");
				#endif // TF2ITEMS_DEBUG_HOOKING
			}
			if (!HookTFBot.GetBool() && ClientPutInServer_Hook != 0) {
				SH_REMOVE_HOOK_ID(ClientPutInServer_Hook);
				ClientPutInServer_Hook = 0;
				#ifdef TF2ITEMS_DEBUG_HOOKING
					g_pSM->LogMessage(myself, "ClientPutInServer unhooked.");
				#endif // TF2ITEMS_DEBUG_HOOKING
			}
		}

		if (ClientPutInServer_Hook != 0 && GiveNamedItem_player_Hook != 0 && GiveNamedItem_bot_Hook != 0) {
			SH_REMOVE_HOOK_ID(ClientPutInServer_Hook);
			ClientPutInServer_Hook = 0;
			#ifdef TF2ITEMS_DEBUG_HOOKING
				g_pSM->LogMessage(myself, "ClientPutInServer unhooked.");
			#endif // TF2ITEMS_DEBUG_HOOKING
		}
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
		g_pSM->LogMessage(myself, "\"GiveNamedItem\" offset = %d", iOffset);
	}

	iOffset = 0;
	if (!g_pGameConf->GetOffset("EquipWearable", &iOffset))
	{
		snprintf(error, maxlen, "Could not find offset for EquipWearable");
		return false;
	} else {
		SH_MANUALHOOK_RECONFIGURE(MCall_EquipWearable, iOffset, 0, 0);
		g_pSM->LogMessage(myself, "\"EquipWearable\" offset = %d", iOffset);
	}

	iOffset = 0;
	if (!g_pGameConf->GetOffset("RemoveWearable", &iOffset))
	{
		snprintf(error, maxlen, "Could not find offset for RemoveWearable");
		return false;
	} else {
		SH_MANUALHOOK_RECONFIGURE(MCall_RemoveWearable, iOffset, 0, 0);
		g_pSM->LogMessage(myself, "\"RemoveWearable\" offset = %d", iOffset);
	}

#ifdef USE_NEW_ATTRIBS
	sm_sendprop_info_t info;
	gamehelpers->FindSendPropInfo("CBaseAttributableItem", "m_iEntityQuality", &info);
	g_iEntityQualityOffset = info.actual_offset;
#endif

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
			//if (pPlayer->IsFakeClient() == true) continue;
			if (pPlayer->IsInGame() == false) continue;

			// Retrieve the edict
			edict_t * pEdict = pPlayer->GetEdict();
			if (pEdict == NULL) continue;

			// Retrieve base player
			CBasePlayer * pBasePlayer = (CBasePlayer *) pEdict->m_pNetworkable->GetBaseEntity();
			if (pBasePlayer == NULL) continue;

			// Done, hook the BasePlayer
			GiveNamedItem_player_Hook = SH_ADD_MANUALVPHOOK(MHook_GiveNamedItem, pBasePlayer, SH_STATIC(Hook_GiveNamedItem), false);
			
			if (GiveNamedItem_player_Hook != 0) {
				#ifdef TF2ITEMS_DEBUG_HOOKING
					 g_pSM->LogMessage(myself, "GiveNamedItem hooked.");
				#endif // TF2ITEMS_DEBUG_HOOKING
				break;
			}
		}
	}

	if (GiveNamedItem_player_Hook == 0) {
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

	if (GiveNamedItem_player_Hook != 0) {
		SH_REMOVE_HOOK_ID(GiveNamedItem_player_Hook);
		GiveNamedItem_player_Hook = 0;
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

	HandleError hndlError;
	Handle_t retHandle = g_pHandleSys->CreateHandle(g_ScriptedItemOverrideHandleType, pScriptedItemOverride, pContext->GetIdentity(), myself->GetIdentity(), &hndlError);
	if (!retHandle)
	{
		return pContext->ThrowNativeError("TF2ItemType handle not created (error %d)", hndlError);
	} else {
		return retHandle;
	}
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
		if (params[2] < 0 || params[2] > 10) return pContext->ThrowNativeError("Quality out of bounds: %i [0 ... 10]", params[2]);
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
		if (params[2] < 0 || params[2] > 127) { pContext->ThrowNativeError("Level out of bounds: %i [0 ... 127]", params[2]); return 0; }
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
		if (params[2] < 0 || params[2] > 15) { pContext->ThrowNativeError("Attribute index out of bounds: %i [0 ... 15]", params[2]); return 0; }
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
		if (params[2] < 0 || params[2] > 15) { pContext->ThrowNativeError("Attribute index out of bounds: %i [0 ... 15]", params[2]); return 0; }
		return pScriptedItemOverride->m_Attributes[params[2]].m_iAttributeDefinitionIndex;
	}
	return -1;
}

static cell_t TF2Items_GetAttributeValue(IPluginContext *pContext, const cell_t *params)
{
	TScriptedItemOverride * pScriptedItemOverride = GetScriptedItemOverrideFromHandle(params[1], pContext);
	if (pScriptedItemOverride != NULL)
	{
		if (params[2] < 0 || params[2] > 15) { pContext->ThrowNativeError("Attribute index out of bounds: %i [0 ... 15]", params[2]); return 0; }
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
