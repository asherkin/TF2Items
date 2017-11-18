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
 *	AzuiSleet				-	Reversed CEconItemView and released it publicly.
 *	VoiDeD					-	Helped with fixing attribute removal after the July 10, 2013 update. Most game support updates since.
 *	Damizean				-	Fixed padding for CEconItemView in Linux. Wrote the SourcePawn Interface and the SourceMod item manager.
 *	Voogru					-	Inspiring the creation of this. Helped with fixing and improving the CEconItemView class used after the 119 update.
 *	Drunken_F00l			-	Inspiring the creation of this.
 */

/*
 *	Debugging options:
 *	==================
 */
//#define TF2ITEMS_DEBUG_HOOKING
//#define TF2ITEMS_DEBUG_HOOKING_GNI
//#define TF2ITEMS_DEBUG_ITEMS

#define NO_FORCE_QUALITY

#include "extension.hpp"

TF2Items g_TF2Items;

SMEXT_LINK(&g_TF2Items);

SH_DECL_HOOK2_void(IServerGameClients, ClientPutInServer, SH_NOATTRIB, 0, edict_t *, char const *);
SH_DECL_MANUALHOOK4(MHook_GiveNamedItem, 0, 0, 0, CBaseEntity *, char const *, int, CEconItemView *, bool);

ICvar *icvar = NULL;
IServerGameClients *gameclients = NULL;
IServerGameEnts *gameents = NULL;

ConVar TF2ItemsVersion("tf2items_version", SMEXT_CONF_VERSION, FCVAR_SPONLY|FCVAR_REPLICATED|FCVAR_NOTIFY, "TF2 Items Version");
ConVar HookTFBot("tf2items_bothook", "1", FCVAR_NONE, "Hook intelligent TF2 bots.");

IGameConfig *g_pGameConf = NULL;

int GiveNamedItem_player_Hook = 0;
int GiveNamedItem_bot_Hook = 0;
int GiveNamedItem_player_Hook_Post = 0;
int GiveNamedItem_bot_Hook_Post = 0;
int ClientPutInServer_Hook = 0;

IForward *g_pForwardGiveItem = NULL;
IForward *g_pForwardGiveItem_Post = NULL;

void *g_pVTable;
void *g_pVTable_Attributes;

HandleType_t g_ScriptedItemOverrideHandleType = 0;
TScriptedItemOverrideTypeHandler g_ScriptedItemOverrideHandler;

// the maximum number of attributes that an item can support
const int g_MaxAttributes = 20;

sp_nativeinfo_t g_ExtensionNatives[] =
{
	{ "TF2Items_GiveNamedItem",		TF2Items_GiveNamedItem },
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
	{ NULL,							NULL }
};

CBaseEntity *Hook_GiveNamedItem(char const *szClassname, int iSubType, CEconItemView *cscript, bool b)
{
	#if defined TF2ITEMS_DEBUG_HOOKING || defined TF2ITEMS_DEBUG_HOOKING_GNI
		 g_pSM->LogMessage(myself, "GiveNamedItem called.");
	#endif // TF2ITEMS_DEBUG_HOOKING

	#ifdef TF2ITEMS_DEBUG_ITEMS
		 g_pSM->LogMessage(myself, "---------------------------------------");
		 g_pSM->LogMessage(myself, ">>> Start of GiveNamedItem call.");
	#endif

	CBasePlayer *player = META_IFACEPTR(CBasePlayer);

	if (cscript == NULL || szClassname == NULL)
	{
#if defined TF2ITEMS_DEBUG_HOOKING_GNI
		g_pSM->LogMessage(myself, "(cscript == NULL || szClassname == NULL), RETURN_META_VALUE(MRES_IGNORED, NULL);");
#endif // TF2ITEMS_DEBUG_HOOKING_GNI

		RETURN_META_VALUE(MRES_IGNORED, NULL);
	}

	// Retrieve client index.
	edict_t *playerEdict = gameents->BaseEntityToEdict((CBaseEntity *)player);
	IGamePlayer * pPlayer = playerhelpers->GetGamePlayer(playerEdict);
	int client = gamehelpers->IndexOfEdict(playerEdict);

	if (g_pVTable == NULL)
	{
		g_pVTable = cscript->m_pVTable;
		g_pVTable_Attributes = cscript->m_AttributeList.m_pVTable;
	}

#ifdef TF2ITEMS_DEBUG_ITEMS

	/*char *roflmelon = new char[32];
	sprintf(roflmelon, "debug_item_%d_%d.txt", cscript->m_iAccountID, cscript->m_iItemDefinitionIndex);
	FILE *fp = fopen(roflmelon, "wb");
	fwrite(cscript, sizeof(CEconItemView), 1, fp);
	fclose(fp);*/
	
	g_pSM->LogMessage(myself, "---------------------------------------");
	g_pSM->LogMessage(myself, ">>> Client = %s", pPlayer->GetName());
	g_pSM->LogMessage(myself, ">>> szClassname = %s", szClassname);
	g_pSM->LogMessage(myself, ">>> iSubType = %d", iSubType);
	g_pSM->LogMessage(myself, ">>> b = %s", b?"true":"false");
	g_pSM->LogMessage(myself, "---------------------------------------");
	g_pSM->LogMessage(myself, ">>> m_iItemDefinitionIndex = %u", cscript->m_iItemDefinitionIndex);
	g_pSM->LogMessage(myself, ">>> m_iEntityQuality = %u", cscript->m_iEntityQuality);
	g_pSM->LogMessage(myself, ">>> m_iEntityLevel = %u", cscript->m_iEntityLevel);
	g_pSM->LogMessage(myself, ">>> m_iItemID = %lu", cscript->m_iItemID);
	g_pSM->LogMessage(myself, ">>> m_iItemIDHigh = %u", cscript->m_iItemIDHigh);
	g_pSM->LogMessage(myself, ">>> m_iItemIDLow = %u", cscript->m_iItemIDLow);
	g_pSM->LogMessage(myself, ">>> m_iAccountID = %u", cscript->m_iAccountID);
	g_pSM->LogMessage(myself, ">>> m_iPosition = %u", cscript->m_iInventoryPosition);
	g_pSM->LogMessage(myself, ">>> m_bInitialized = %s", cscript->m_bInitialized?"true":"false");
	g_pSM->LogMessage(myself, "---------------------------------------");
	for (int i = 0; i < ((cscript->m_AttributeList.m_Attributes.Count() > 16)?0:cscript->m_AttributeList.m_Attributes.Count()); i++)
	{
		g_pSM->LogMessage(myself, ">>> m_iAttributeDefinitionIndex = %u", cscript->m_AttributeList.m_Attributes.Element(i).m_iAttributeDefinitionIndex);
		g_pSM->LogMessage(myself, ">>> m_flValue = %f", cscript->m_AttributeList.m_Attributes.Element(i).m_flValue);
		g_pSM->LogMessage(myself, "---------------------------------------");
	}
	g_pSM->LogMessage(myself, ">>> Size of CEconItemView = %d", sizeof(CEconItemView));
	g_pSM->LogMessage(myself, ">>> Size of CEconItemAttribute = %d", sizeof(CEconItemAttribute));
	g_pSM->LogMessage(myself, ">>> No. of Attributes = %d", cscript->m_AttributeList.m_Attributes.Count());
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
		case Pl_Continue:
			{
				RETURN_META_VALUE(MRES_IGNORED, NULL);
			}
		case Pl_Changed:
			{
				TScriptedItemOverride *pScriptedItemOverride = GetScriptedItemOverrideFromHandle(cellOverrideHandle);
				if (pScriptedItemOverride == NULL) {
					RETURN_META_VALUE(MRES_IGNORED, NULL);
				}

				// Execute the new attributes set and we're done!
				char *finalitem = (char *)szClassname;

				CEconItemView newitem;
				CSCICopy(cscript, &newitem);

				// Override based on the flags passed to this object.
				if (pScriptedItemOverride->m_bFlags & OVERRIDE_CLASSNAME)
				{
					finalitem = pScriptedItemOverride->m_strWeaponClassname;
				}

				if (pScriptedItemOverride->m_bFlags & OVERRIDE_ITEM_DEF)
				{
					newitem.m_iItemDefinitionIndex = pScriptedItemOverride->m_iItemDefinitionIndex;
				}

				if (pScriptedItemOverride->m_bFlags & OVERRIDE_ITEM_LEVEL)
				{
					newitem.m_iEntityLevel = pScriptedItemOverride->m_iEntityLevel;
				}

				if (pScriptedItemOverride->m_bFlags & OVERRIDE_ITEM_QUALITY)
				{
					newitem.m_iEntityQuality = pScriptedItemOverride->m_iEntityQuality;
				}

				if (pScriptedItemOverride->m_bFlags & OVERRIDE_ATTRIBUTES)
				{
#ifndef NO_FORCE_QUALITY
					// Even if we don't want to override the item quality, do if it's set to 0.
					if (newitem.m_iEntityQuality == 0 && !(pScriptedItemOverride->m_bFlags & OVERRIDE_ITEM_QUALITY) && pScriptedItemOverride->m_iCount > 0) newitem.m_iEntityQuality = 6;
#endif

					if (!(pScriptedItemOverride->m_bFlags & PRESERVE_ATTRIBUTES))
					{
						newitem.m_bDoNotIterateStaticAttributes = true;
					}

					newitem.m_AttributeList.m_Attributes.RemoveAll();
					newitem.m_AttributeList.m_Attributes.AddMultipleToTail(pScriptedItemOverride->m_iCount, pScriptedItemOverride->m_Attributes);
				}

				if (cscript->m_iEntityQuality == 0)
				{
					newitem.m_iEntityQuality = 0;
				}

				RETURN_META_VALUE_MNEWPARAMS(MRES_HANDLED, NULL, MHook_GiveNamedItem, (finalitem, iSubType, &newitem, ((pScriptedItemOverride->m_bFlags & FORCE_GENERATION) == FORCE_GENERATION)));
			}
		case Pl_Handled:
		case Pl_Stop:
			{
				RETURN_META_VALUE(MRES_SUPERCEDE, NULL);
			}
	}
	
	RETURN_META_VALUE(MRES_IGNORED, NULL);
}

CBaseEntity *Hook_GiveNamedItem_Post(char const *szClassname, int iSubType, CEconItemView *cscript, bool b)
{
	CBaseEntity *player = META_IFACEPTR(CBaseEntity);

	CBaseEntity *pItemEntiy;
	if (META_RESULT_STATUS >= MRES_OVERRIDE)
	{
		pItemEntiy = META_RESULT_OVERRIDE_RET(CBaseEntity *);
	} else {
		pItemEntiy = META_RESULT_ORIG_RET(CBaseEntity *);
	}

	if (!player || !szClassname || !cscript || !pItemEntiy)
		RETURN_META_VALUE(MRES_IGNORED, pItemEntiy);
	
	int client = gamehelpers->EntityToBCompatRef(player);
	int iEntityIndex = gamehelpers->EntityToBCompatRef(pItemEntiy);

	g_pForwardGiveItem_Post->PushCell(client);
	g_pForwardGiveItem_Post->PushString(szClassname);
	g_pForwardGiveItem_Post->PushCell(cscript->m_iItemDefinitionIndex);
	g_pForwardGiveItem_Post->PushCell(cscript->m_iEntityLevel);
	g_pForwardGiveItem_Post->PushCell(cscript->m_iEntityQuality);
	g_pForwardGiveItem_Post->PushCell(iEntityIndex);
	g_pForwardGiveItem_Post->Execute(NULL);
	
	RETURN_META_VALUE(MRES_IGNORED, pItemEntiy);
}

void CSCICopy(CEconItemView *olditem, CEconItemView *newitem)
{
	memset(newitem, 0, sizeof(CEconItemView));
	
	//#define copymember(a) newitem->a = olditem->a
	#define copymember(a) memcpy(&newitem->a, &olditem->a, sizeof(newitem->a));

	copymember(m_pVTable);

	copymember(m_iItemDefinitionIndex);
	
	copymember(m_iEntityQuality);
	copymember(m_iEntityLevel);

	copymember(m_iItemID);
	copymember(m_iItemIDHigh);
	copymember(m_iItemIDLow);
	copymember(m_iAccountID);
	copymember(m_iInventoryPosition);

	copymember(m_ItemHandle);

	copymember(m_bColorInit);
	copymember(m_bPaintOverrideInit);
	copymember(m_bHasPaintOverride);

	copymember(m_flOverrideIndex);
	copymember(m_unRGB);
	copymember(m_unAltRGB);

	copymember(m_iTeamNumber);

	copymember(m_bInitialized);

	// copy ctor so the CUtlVector is copied correctly
	newitem->m_AttributeList = olditem->m_AttributeList;
	newitem->m_NetworkedDynamicAttributesForDemos = olditem->m_NetworkedDynamicAttributesForDemos;
	
	copymember(m_bDoNotIterateStaticAttributes);
	
	/*
	META_CONPRINTF("Copying attributes...\n");
	int nCount = olditem->m_Attributes.Count();
	META_CONPRINTF("Count: %d\n", nCount);
	newitem->m_Attributes.SetSize( nCount );
	for ( int i = 0; i < nCount; i++ )
	{
		META_CONPRINTF("Copying %d...\n", i+1);
		newitem->m_Attributes[ i ] = olditem->m_Attributes[ i ];
	}
	*/
}

void Hook_ClientPutInServer(edict_t *pEntity, char const *playername)
{
#ifdef TF2ITEMS_DEBUG_HOOKING
	 g_pSM->LogMessage(myself, "ClientPutInServer called.");
#endif // TF2ITEMS_DEBUG_HOOKING

	if(pEntity->m_pNetworkable)
	{
		CBaseEntity *baseentity = pEntity->m_pNetworkable->GetBaseEntity();
		if(!baseentity)
		{
			return;
		}

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

			if(GiveNamedItem_bot_Hook_Post == 0)
			{
				GiveNamedItem_bot_Hook_Post = SH_ADD_MANUALVPHOOK(MHook_GiveNamedItem, player, SH_STATIC(Hook_GiveNamedItem_Post), true);
#ifdef TF2ITEMS_DEBUG_HOOKING
				g_pSM->LogMessage(myself, "GiveNamedItem hooked (bot) (post).");
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

			if(GiveNamedItem_player_Hook_Post == 0)
			{
				GiveNamedItem_player_Hook_Post = SH_ADD_MANUALVPHOOK(MHook_GiveNamedItem, player, SH_STATIC(Hook_GiveNamedItem_Post), true);
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

	// If it's a late load, there might be the chance there are players already on the server. Just
	// check for this and try to hook them instead of waiting for the next player. -- Damizean
	if (late)
	{
#ifdef TF2ITEMS_DEBUG_HOOKING
		g_pSM->LogMessage(myself, "Is a late load, attempting to hook GiveNamedItem.");
#endif // TF2ITEMS_DEBUG_HOOKING

		int iMaxClients = playerhelpers->GetMaxClients();
		for (int iClient = 1; iClient <= iMaxClients; iClient++)
		{
			IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(iClient);
			if (pPlayer == NULL || !pPlayer->IsConnected() || !pPlayer->IsInGame())
			{
				continue;
			}

			// Retrieve the edict
			edict_t *pEdict = pPlayer->GetEdict();
			if (pEdict == NULL)
			{
				continue;
			}

			// Retrieve base player
			CBasePlayer *pBasePlayer = (CBasePlayer *)pEdict->m_pNetworkable->GetBaseEntity();
			if (pBasePlayer == NULL)
			{
				continue;
			}

			// Done, hook the BasePlayer
			GiveNamedItem_player_Hook = SH_ADD_MANUALVPHOOK(MHook_GiveNamedItem, pBasePlayer, SH_STATIC(Hook_GiveNamedItem), false);
			
			if (GiveNamedItem_player_Hook != 0)
			{
#ifdef TF2ITEMS_DEBUG_HOOKING
				g_pSM->LogMessage(myself, "GiveNamedItem hooked.");
#endif // TF2ITEMS_DEBUG_HOOKING
				break;
			}
		}
	}

	if (GiveNamedItem_player_Hook == 0)
	{
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
	g_ScriptedItemOverrideHandleType = g_pHandleSys->CreateType("TF2ItemType", &g_ScriptedItemOverrideHandler, 0, NULL, NULL, myself->GetIdentity(), NULL);

	// Create forwards
	g_pForwardGiveItem = g_pForwards->CreateForward("TF2Items_OnGiveNamedItem", ET_Hook, 4, NULL, Param_Cell, Param_String, Param_Cell, Param_CellByRef);
	g_pForwardGiveItem_Post = g_pForwards->CreateForward("TF2Items_OnGiveNamedItem_Post", ET_Ignore, 6, NULL, Param_Cell, Param_String, Param_Cell, Param_Cell, Param_Cell, Param_Cell);

	return true;
}

bool TF2Items::SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{

	GET_V_IFACE_ANY(GetServerFactory, gameclients, IServerGameClients, INTERFACEVERSION_SERVERGAMECLIENTS);
	GET_V_IFACE_ANY(GetServerFactory, gameents, IServerGameEnts, INTERFACEVERSION_SERVERGAMEENTS);
	GET_V_IFACE_CURRENT(GetEngineFactory, icvar, ICvar, CVAR_INTERFACE_VERSION);

	g_pCVar = icvar;

	ConVar_Register(0, this);

	return true;
}

void TF2Items::SDK_OnUnload()
{
#ifdef TF2ITEMS_DEBUG_HOOKING
	g_pSM->LogMessage(myself, "SDK_OnUnload called.");
#endif // TF2ITEMS_DEBUG_HOOKING

	gameconfs->CloseGameConfigFile(g_pGameConf);

	g_pHandleSys->RemoveType(g_ScriptedItemOverrideHandleType, myself->GetIdentity());

	g_pForwards->ReleaseForward(g_pForwardGiveItem);
	g_pForwards->ReleaseForward(g_pForwardGiveItem_Post);
}

bool TF2Items::SDK_OnMetamodUnload(char *error, size_t maxlen)
{
#ifdef TF2ITEMS_DEBUG_HOOKING
	g_pSM->LogMessage(myself, "SDK_OnMetamodUnload called.");
#endif // TF2ITEMS_DEBUG_HOOKING

	if (ClientPutInServer_Hook != 0)
	{
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

	if (GiveNamedItem_player_Hook != 0)
	{
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

	if (GiveNamedItem_player_Hook_Post != 0)
	{
		SH_REMOVE_HOOK_ID(GiveNamedItem_player_Hook_Post);
		GiveNamedItem_player_Hook_Post = 0;

#ifdef TF2ITEMS_DEBUG_HOOKING
		g_pSM->LogMessage(myself, "GiveNamedItem (post) unhooked.");
#endif // TF2ITEMS_DEBUG_HOOKING
	}
#ifdef TF2ITEMS_DEBUG_HOOKING
	else {
		g_pSM->LogMessage(myself, "GiveNamedItem (post) did not need to be unhooked.");
	}
#endif // TF2ITEMS_DEBUG_HOOKING

	return true;
}

bool TF2Items::RegisterConCommandBase(ConCommandBase *pCommand)
{
	META_REGCVAR(pCommand);
	return true;
}

void TScriptedItemOverrideTypeHandler::OnHandleDestroy(HandleType_t type, void *object)
{
	TScriptedItemOverride *pScriptedItemOverride = (TScriptedItemOverride*) object;

	if (pScriptedItemOverride != NULL)
	{
		delete(pScriptedItemOverride);
	}
}

static cell_t TF2Items_GiveNamedItem(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity;
	if ((pEntity = GetCBaseEntityFromIndex(params[1], true)) == NULL)
	{
		return pContext->ThrowNativeError("Client index %d is not valid", params[1]);
	}
	
	TScriptedItemOverride *pScriptedItemOverride = GetScriptedItemOverrideFromHandle(params[2], pContext);
	if (pScriptedItemOverride == NULL)
	{
		return -1;
	}
	
	// Create new script created item object and prepare it.
	CEconItemView hScriptCreatedItem;
	memset(&hScriptCreatedItem, 0, sizeof(CEconItemView));

	// initialize the vtable pointers
	hScriptCreatedItem.m_pVTable = g_pVTable;
	hScriptCreatedItem.m_AttributeList.m_pVTable = g_pVTable_Attributes;
	hScriptCreatedItem.m_NetworkedDynamicAttributesForDemos.m_pVTable = g_pVTable_Attributes;

	char *strWeaponClassname = pScriptedItemOverride->m_strWeaponClassname;
	hScriptCreatedItem.m_iItemDefinitionIndex = pScriptedItemOverride->m_iItemDefinitionIndex;
	hScriptCreatedItem.m_iEntityLevel = pScriptedItemOverride->m_iEntityLevel;
	hScriptCreatedItem.m_iEntityQuality = pScriptedItemOverride->m_iEntityQuality;
	hScriptCreatedItem.m_AttributeList.m_Attributes.CopyArray(pScriptedItemOverride->m_Attributes, pScriptedItemOverride->m_iCount);
	hScriptCreatedItem.m_bInitialized = true;
	
	if (!(pScriptedItemOverride->m_bFlags & PRESERVE_ATTRIBUTES))
	{
		hScriptCreatedItem.m_bDoNotIterateStaticAttributes = true;
	}

#ifndef NO_FORCE_QUALITY
	if (hScriptCreatedItem.m_iEntityQuality == 0 && hScriptCreatedItem.m_iAttributesCount > 0)
	{
		hScriptCreatedItem.m_iEntityQuality = 6;
	}
#endif

	// Call the function.
	CBaseEntity *tempItem = NULL;
	tempItem = SH_MCALL(pEntity, MHook_GiveNamedItem)(strWeaponClassname, 0, &hScriptCreatedItem, ((pScriptedItemOverride->m_bFlags & FORCE_GENERATION) == FORCE_GENERATION));

	if (tempItem == NULL)
	{
		g_pSM->LogError(myself, "---------------------------------------");
		g_pSM->LogError(myself, ">>> szClassname = %s", strWeaponClassname);
		g_pSM->LogError(myself, ">>> iItemDefinitionIndex = %u", hScriptCreatedItem.m_iItemDefinitionIndex);
		g_pSM->LogError(myself, ">>> iEntityQuality = %u", hScriptCreatedItem.m_iEntityQuality);
		g_pSM->LogError(myself, ">>> iEntityLevel = %u", hScriptCreatedItem.m_iEntityLevel);
		g_pSM->LogError(myself, "---------------------------------------");

		for (int i = 0; i < ((hScriptCreatedItem.m_AttributeList.m_Attributes.Count() > 16) ? 0 : hScriptCreatedItem.m_AttributeList.m_Attributes.Count()); i++)
		{
			g_pSM->LogError(myself, ">>> iAttributeDefinitionIndex = %u", hScriptCreatedItem.m_AttributeList.m_Attributes.Element(i).m_iAttributeDefinitionIndex);
			g_pSM->LogError(myself, ">>> flValue = %f", hScriptCreatedItem.m_AttributeList.m_Attributes.Element(i).m_flValue);
			g_pSM->LogError(myself, "---------------------------------------");
		}

		return pContext->ThrowNativeError("Item is NULL. File a bug report if you are sure you set all the data correctly. (Try the FORCE_GENERATION flag.)");
	}

	int entIndex = gamehelpers->EntityToBCompatRef(tempItem);

	// Need to manually fire the forward.
	g_pForwardGiveItem_Post->PushCell(params[1]);
	g_pForwardGiveItem_Post->PushString(strWeaponClassname);
	g_pForwardGiveItem_Post->PushCell(hScriptCreatedItem.m_iItemDefinitionIndex);
	g_pForwardGiveItem_Post->PushCell(hScriptCreatedItem.m_iEntityLevel);
	g_pForwardGiveItem_Post->PushCell(hScriptCreatedItem.m_iEntityQuality);
	g_pForwardGiveItem_Post->PushCell(entIndex);
	g_pForwardGiveItem_Post->Execute(NULL);

	return entIndex;
}

static cell_t TF2Items_CreateItem(IPluginContext *pContext, const cell_t *params)
{
	TScriptedItemOverride *pScriptedItemOverride = new TScriptedItemOverride;
	memset(pScriptedItemOverride, 0, sizeof(TScriptedItemOverride));

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
	TScriptedItemOverride *pScriptedItemOverride = GetScriptedItemOverrideFromHandle(params[1], pContext);

	if (pScriptedItemOverride != NULL)
	{
		pScriptedItemOverride->m_bFlags = params[2];
	}

	return 1;
}

static cell_t TF2Items_GetFlags(IPluginContext *pContext, const cell_t *params)
{
	TScriptedItemOverride *pScriptedItemOverride = GetScriptedItemOverrideFromHandle(params[1], pContext);

	if (pScriptedItemOverride != NULL)
	{
		return pScriptedItemOverride->m_bFlags;
	}

	return 0;
}

static cell_t TF2Items_SetClassname(IPluginContext *pContext, const cell_t *params)
{
	TScriptedItemOverride *pScriptedItemOverride = GetScriptedItemOverrideFromHandle(params[1], pContext);

	if (pScriptedItemOverride != NULL)
	{
		char *strSource; pContext->LocalToString(params[2], &strSource);
		snprintf(pScriptedItemOverride->m_strWeaponClassname, 256, "%s", strSource);
	}

	return 1;
}

static cell_t TF2Items_GetClassname(IPluginContext *pContext, const cell_t *params)
{
	TScriptedItemOverride *pScriptedItemOverride = GetScriptedItemOverrideFromHandle(params[1], pContext);

	if (pScriptedItemOverride != NULL)
	{
		char *strSource = pScriptedItemOverride->m_strWeaponClassname;

		char *strDestiny;
		pContext->LocalToString(params[2], &strDestiny);

		int iSourceSize = strlen(strSource);
		int iDestinySize = params[3];

		// Perform bounds checking
		if (iSourceSize >= iDestinySize)
		{
			iSourceSize = iDestinySize-1;
		} else {
			iSourceSize = iDestinySize;
		}
	 
		// Copy
		memmove(strDestiny, strSource, iSourceSize);
		strDestiny[iSourceSize] = '\0';

		return iSourceSize;
	}

	return 0;
}

static cell_t TF2Items_SetItemIndex(IPluginContext *pContext, const cell_t *params)
{
	TScriptedItemOverride *pScriptedItemOverride = GetScriptedItemOverrideFromHandle(params[1], pContext);

	if (pScriptedItemOverride != NULL)
	{
		pScriptedItemOverride->m_iItemDefinitionIndex = params[2];
	}

	return 1;
}

static cell_t TF2Items_GetItemIndex(IPluginContext *pContext, const cell_t *params)
{
	TScriptedItemOverride *pScriptedItemOverride = GetScriptedItemOverrideFromHandle(params[1], pContext);

	if (pScriptedItemOverride != NULL)
	{
		return pScriptedItemOverride->m_iItemDefinitionIndex;
	}

	return -1;
}

static cell_t TF2Items_SetQuality(IPluginContext *pContext, const cell_t *params)
{
	TScriptedItemOverride *pScriptedItemOverride = GetScriptedItemOverrideFromHandle(params[1], pContext);

	if (pScriptedItemOverride != NULL)
	{
		if (params[2] < -1)
		{
			return pContext->ThrowNativeError("Quality out of bounds: %i [-1 ...]", params[2]);
		}

		pScriptedItemOverride->m_iEntityQuality = params[2];
	}

	return 1;
}

static cell_t TF2Items_GetQuality(IPluginContext *pContext, const cell_t *params)
{
	TScriptedItemOverride *pScriptedItemOverride = GetScriptedItemOverrideFromHandle(params[1], pContext);

	if (pScriptedItemOverride != NULL)
	{
		return pScriptedItemOverride->m_iEntityQuality;
	}

	return 0;
}

static cell_t TF2Items_SetLevel(IPluginContext *pContext, const cell_t *params)
{
	TScriptedItemOverride *pScriptedItemOverride = GetScriptedItemOverrideFromHandle(params[1], pContext);

	if (pScriptedItemOverride != NULL)
	{
		if (params[2] < 0)
		{
			return pContext->ThrowNativeError("Level out of bounds: %i [0 ...]", params[2]);
		}

		pScriptedItemOverride->m_iEntityLevel = params[2];
	}

	return 1;
}

static cell_t TF2Items_GetLevel(IPluginContext *pContext, const cell_t *params)
{
	TScriptedItemOverride *pScriptedItemOverride = GetScriptedItemOverrideFromHandle(params[1], pContext);

	if (pScriptedItemOverride != NULL)
	{
		return pScriptedItemOverride->m_iEntityLevel;
	}

	return 0;
}

static cell_t TF2Items_SetNumAttributes(IPluginContext *pContext, const cell_t *params)
{
	TScriptedItemOverride *pScriptedItemOverride = GetScriptedItemOverrideFromHandle(params[1], pContext);

	if (pScriptedItemOverride != NULL)
	{
		if (params[2] < 0 || params[2] >= g_MaxAttributes)
		{
			return pContext->ThrowNativeError("Attributes size out of bounds: %i [0 ... %i]", params[2], g_MaxAttributes - 1);
		}

		pScriptedItemOverride->m_iCount = params[2];
	}

	return 1;
}

static cell_t TF2Items_GetNumAttributes(IPluginContext *pContext, const cell_t *params)
{
	TScriptedItemOverride *pScriptedItemOverride = GetScriptedItemOverrideFromHandle(params[1], pContext);

	if (pScriptedItemOverride != NULL)
	{
		return pScriptedItemOverride->m_iCount;
	}

	return -1;
}

static cell_t TF2Items_SetAttribute(IPluginContext *pContext, const cell_t *params)
{
	TScriptedItemOverride *pScriptedItemOverride = GetScriptedItemOverrideFromHandle(params[1], pContext);

	if (pScriptedItemOverride != NULL)
	{
		if (params[2] < 0 || params[2] >= g_MaxAttributes)
		{
			return pContext->ThrowNativeError("Attribute index out of bounds: %i [0 ... %i]", params[2], g_MaxAttributes - 1);
		}
		
		if (params[3] == 0)
		{
			return pContext->ThrowNativeError("Cowardly refusing to add invalid attribute index \"0\" to an item.");
		}

		pScriptedItemOverride->m_Attributes[params[2]].m_iAttributeDefinitionIndex = params[3];
		pScriptedItemOverride->m_Attributes[params[2]].m_flValue = sp_ctof(params[4]);

		return 1;
	}

	return 0;
}

static cell_t TF2Items_GetAttributeId(IPluginContext *pContext, const cell_t *params)
{
	TScriptedItemOverride *pScriptedItemOverride = GetScriptedItemOverrideFromHandle(params[1], pContext);

	if (pScriptedItemOverride != NULL)
	{
		if (params[2] < 0 || params[2] >= g_MaxAttributes)
		{
			return pContext->ThrowNativeError("Attribute index out of bounds: %i [0 ... %i]", params[2], g_MaxAttributes - 1);
		}

		return pScriptedItemOverride->m_Attributes[params[2]].m_iAttributeDefinitionIndex;
	}

	return -1;
}

static cell_t TF2Items_GetAttributeValue(IPluginContext *pContext, const cell_t *params)
{
	TScriptedItemOverride *pScriptedItemOverride = GetScriptedItemOverrideFromHandle(params[1], pContext);

	if (pScriptedItemOverride != NULL)
	{
		if (params[2] < 0 || params[2] >= g_MaxAttributes)
		{
			return pContext->ThrowNativeError("Attribute index out of bounds: %i [0 ... %i]", params[2], g_MaxAttributes - 1);
		}

		return sp_ftoc(pScriptedItemOverride->m_Attributes[params[2]].m_flValue);
	}

	return sp_ftoc(0.0f);
}

CBaseEntity *GetCBaseEntityFromIndex(int p_iEntity, bool p_bOnlyPlayers)
{
	edict_t *edtEdict = engine->PEntityOfEntIndex(p_iEntity);
	if (!edtEdict || edtEdict->IsFree())
	{
		return NULL;
	}
	
	if (p_iEntity > 0 && p_iEntity <= playerhelpers->GetMaxClients())
	{
		IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(edtEdict);

		if (!pPlayer || !pPlayer->IsConnected())
		{
			return NULL;
		}
	} else if (p_bOnlyPlayers) {
		return NULL;
	}

	IServerUnknown *pUnk = edtEdict->GetUnknown();
	if (pUnk == NULL)
	{
		return NULL;
	}

	return pUnk->GetBaseEntity();
}

int GetIndexFromCBaseEntity(CBaseEntity *p_hEntity)
{
	if (p_hEntity == NULL)
	{
		return -1;
	}

	edict_t *edtEdict = gameents->BaseEntityToEdict(p_hEntity);
	if (!edtEdict || edtEdict->IsFree())
	{
		return -1;
	}

	return gamehelpers->IndexOfEdict(edtEdict);
}

TScriptedItemOverride *GetScriptedItemOverrideFromHandle(cell_t cellHandle, IPluginContext *pContext)
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
		if (pContext == NULL)
		{
			g_pSM->LogError(myself, "Invalid TF2ItemType handle %x (error %d)", hndlScriptedItemOverride, hndlError);
		} else {
			pContext->ThrowNativeError("Invalid TF2ItemType handle %x (error %d)", hndlScriptedItemOverride, hndlError);
		}

		return NULL;
	}

	return pScriptedItemOverride;
}
