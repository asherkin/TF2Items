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

// int CTFPlayerInventory::ItemHasBeenUpdated(CScriptCreatedItem *pItem, bool bAdd, bool b)

/*
 *	Debugging options:
 *	==================
 */
//#define TF2ITEMS_DEBUG_HOOKING
//#define TF2ITEMS_DEBUG_ITEMS

#include "extension.hpp"
#include "rtti.h"
#include "CDetour/detours.h"

TF2Items g_TF2Items;

SMEXT_LINK(&g_TF2Items);

SH_DECL_HOOK2_void(IServerGameClients, ClientPutInServer, SH_NOATTRIB, 0, edict_t *, char const *);
SH_DECL_MANUALHOOK4(MHook_GiveNamedItem, 0, 0, 0, CBaseEntity *, char const *, int, CScriptCreatedItem *, bool);

SH_DECL_MANUALHOOK1_void(MCall_DumpInventoryToConsole, 7, 0, 0, bool);
SH_DECL_MANUALHOOK3(MCall_ItemHasBeenUpdated, 13, 0, 0, int, CScriptCreatedItem *, bool, bool);

ICvar *icvar = NULL;
IServerGameClients *gameclients = NULL;
IServerGameEnts *gameents = NULL;

ConVar show_given_items("show_given_items", "0");
ConVar TF2ItemsVersion("tf2items_version", SMEXT_CONF_VERSION, FCVAR_SPONLY|FCVAR_REPLICATED|FCVAR_NOTIFY, "TF2 Items Version");

IGameConfig *g_pGameConf = NULL;

int g_TFInventoryOffset;

int GiveNamedItem_player_Hook = 0;
int GiveNamedItem_bot_Hook = 0;
int ClientPutInServer_Hook = 0;

CDetour *ItemHasBeenUpdated_Detour = NULL;

#ifndef WIN32
typedef void (* ItemHasBeenUpdatedFuncType)(CPlayerInventory *, CScriptCreatedItem *pItem, bool a, bool b);
#else
typedef void (__fastcall * ItemHasBeenUpdatedFuncType)(CPlayerInventory *, void *, CScriptCreatedItem *pItem, bool a, bool b);
#endif

ItemHasBeenUpdatedFuncType ItemHasBeenUpdatedFunc;

CON_COMMAND(rtti_test, "")
{
	if (args.ArgC() < 2)
	{
		META_CONPRINT("Usage: rtti_test <player index>\n");
		return;
	}

	int iPlayerIndex = atoi(args.Arg(1));

	if (iPlayerIndex < 1)
	{
		META_CONPRINTF("Error: Invalid player index! (%d)\n", iPlayerIndex);
		return;
	}

	CBaseEntity *pPlayer = GetCBaseEntityFromIndex(iPlayerIndex, true);

	if (!pPlayer)
	{
		META_CONPRINT("Error: CBasePlayer pointer is null.\n");
		return;
	}

	/*CTFPlayerInventory*/ CPlayerInventory *pInventory = GetInventory(pPlayer);

	if (!pInventory)
	{
		META_CONPRINT("Error: CPlayerInventory pointer is null.\n");
		return;
	}

	META_CONPRINT("\n\npInventory:\n");
	PrintTypeTree(pInventory);

	META_CONPRINT("\n\npInventory->m_pCSharedObjectCache:\n");
	PrintTypeTree(pInventory->m_pCSharedObjectCache);

	if (!pInventory->m_BackPack.Count())
		return;

	CScriptCreatedItem *pItem = &pInventory->m_BackPack.Element(0);

	if (!pItem)
	{
		META_CONPRINT("Error: CScriptCreatedItem pointer is null.\n");
		return;
	}

	META_CONPRINT("\n\npItem:\n");
	PrintTypeTree(pItem);

	return;
}

CON_COMMAND(dump_inv, "")
{
	if (args.ArgC() < 2)
	{
		META_CONPRINT("Usage: dump_inv <player index>\n");
		return;
	}

	int iPlayerIndex = atoi(args.Arg(1));

	if (iPlayerIndex < 1)
	{
		META_CONPRINTF("Error: Invalid player index! (%d)\n", iPlayerIndex);
		return;
	}

	CBaseEntity *pPlayer = GetCBaseEntityFromIndex(iPlayerIndex, true);

	if (!pPlayer)
	{
		META_CONPRINT("Error: CBasePlayer point is null.\n");
		return;
	}

	/*CTFPlayerInventory*/ CPlayerInventory *pInventory = GetInventory(pPlayer);

	if (!pInventory)
	{
		META_CONPRINT("Error: CPlayerInventory point is null.\n");
		return;
	}

	SH_MCALL(pInventory, MCall_DumpInventoryToConsole)(true);

	return;
}

// Don't run this.
CON_COMMAND(equip_wep, "")
{
	if (args.ArgC() < 3)
	{
		META_CONPRINT("Usage: equip_wep <player index> <global id>\n");
		return;
	}

	int iPlayerIndex = atoi(args.Arg(1));

	if (iPlayerIndex < 1)
	{
		META_CONPRINTF("Error: Invalid player index! (%d)\n", iPlayerIndex);
		return;
	}

	CBaseEntity *pPlayer = GetCBaseEntityFromIndex(iPlayerIndex, true);

	if (!pPlayer)
	{
		META_CONPRINT("Error: CBasePlayer point is null.\n");
		return;
	}

	/*CTFPlayerInventory*/ CPlayerInventory *pInventory = GetInventory(pPlayer);

	if (!pInventory)
	{
		META_CONPRINT("Error: CPlayerInventory point is null.\n");
		return;
	}

	#ifdef _WIN32
	uint64 ullGlobalIndex = _atoi64(args.Arg(2));
	#else
	uint64 ullGlobalIndex = atoll(args.Arg(2));
	#endif

	for (int i = 0; i < pInventory->m_BackPack.Count(); i++)
	{
		CScriptCreatedItem *pItem = &pInventory->m_BackPack.Element(i);
		if (pItem->m_iGlobalIndex == ullGlobalIndex)
		{
			META_CONPRINTF("Found a matching item! (%s)\n", pItem->m_szName);
			SH_MCALL(pInventory, MCall_ItemHasBeenUpdated)(pItem, true, false); // <-- bad
			break;
		}
	}

	return;
}

CON_COMMAND(info, "")
{
	if (args.ArgC() < 2)
	{
		META_CONPRINT("Usage: info <player index>\n");
		return;
	}

	int iPlayerIndex = atoi(args.Arg(1));

	if (iPlayerIndex < 1)
	{
		META_CONPRINTF("Error: Invalid player index! (%d)\n", iPlayerIndex);
		return;
	}

	CBaseEntity *pPlayer = GetCBaseEntityFromIndex(iPlayerIndex, true);

	if (!pPlayer)
	{
		META_CONPRINT("Error: CBasePlayer pointer is null.\n");
		return;
	}

	/*CTFPlayerInventory*/ CPlayerInventory *pInventory = GetInventory(pPlayer);

	if (!pInventory)
	{
		META_CONPRINT("Error: CPlayerInventory pointer is null.\n");
		return;
	}

	CScriptCreatedItem *pItem = &pInventory->m_BackPack.Element(0);

	META_CONPRINTF("pInventory = 0x%.8X, pItem = 0x%.8X\n", pInventory, pItem);

	PrintTypeTree(pItem); // Make sure this is a CSCI, and because RTTI is cool

	//SH_MCALL(pInventory, MCall_ItemHasBeenUpdated)(pItem, true, false); // Fuck SourceHook in this case, although it may just be a dodgey vtable offset.

	ItemHasBeenUpdated(pInventory, pItem, false, false);

	return;
}

CON_COMMAND(default_weps, "")
{
	if (args.ArgC() < 2)
	{
		META_CONPRINT("Usage: default_weps <player index>\n");
		return;
	}

	int iPlayerIndex = atoi(args.Arg(1));

	if (iPlayerIndex < 1)
	{
		META_CONPRINTF("Error: Invalid player index! (%d)\n", iPlayerIndex);
		return;
	}

	CBaseEntity *pPlayer = GetCBaseEntityFromIndex(iPlayerIndex, true);

	if (!pPlayer)
	{
		META_CONPRINT("Error: CBasePlayer pointer is null.\n");
		return;
	}

	/*CTFPlayerInventory*/ CPlayerInventory *pInventory = GetInventory(pPlayer);

	if (!pInventory)
	{
		META_CONPRINT("Error: CPlayerInventory pointer is null.\n");
		return;
	}

	for (int i = 0; i < pInventory->m_BackPack.Count(); i++)
	{
		CScriptCreatedItem *pItem = &pInventory->m_BackPack.Element(i);
		pItem->m_iPosition &= ~0x0FFF0000;
		ItemHasBeenUpdated_Detour->DisableDetour();
		ItemHasBeenUpdated(pInventory, pItem, false, false);
		ItemHasBeenUpdated_Detour->EnableDetour();
	}

	return;
}

CON_COMMAND(powerjack, "")
{
	if (args.ArgC() < 2)
	{
		META_CONPRINT("Usage: powerjack <player index>\n");
		return;
	}

	int iPlayerIndex = atoi(args.Arg(1));

	if (iPlayerIndex < 1)
	{
		META_CONPRINTF("Error: Invalid player index! (%d)\n", iPlayerIndex);
		return;
	}

	CBaseEntity *pPlayer = GetCBaseEntityFromIndex(iPlayerIndex, true);

	if (!pPlayer)
	{
		META_CONPRINT("Error: CBasePlayer pointer is null.\n");
		return;
	}

	/*CTFPlayerInventory*/ CPlayerInventory *pInventory = GetInventory(pPlayer);

	if (!pInventory)
	{
		META_CONPRINT("Error: CPlayerInventory pointer is null.\n");
		return;
	}

	for (int i = 0; i < pInventory->m_BackPack.Count(); i++)
	{
		CScriptCreatedItem *pItem = &pInventory->m_BackPack.Element(i);
		switch (pItem->m_iItemDefinitionIndex)
		{
		case 2:
			pItem->m_iPosition &= ~0x00400000; // Dequip for Pyro
			break;
		case 38:
			pItem->m_iPosition &= ~0x00400000; // Dequip for Pyro
			break;
		case 153:
			pItem->m_iPosition &= ~0x00400000; // Dequip for Pyro
			break;
		case 192:
			pItem->m_iPosition &= ~0x00400000; // Dequip for Pyro
			break;
		}
		ItemHasBeenUpdated_Detour->DisableDetour();
		ItemHasBeenUpdated(pInventory, pItem, false, false);
		ItemHasBeenUpdated_Detour->EnableDetour();
	}

	return;

	CScriptCreatedItem *sciPowerjack = new CScriptCreatedItem();

	sciPowerjack->m_iItemDefinitionIndex = 214;
	sciPowerjack->m_iEntityQuality = 6;
	sciPowerjack->m_iEntityLevel = 5;

	sciPowerjack->m_iGlobalIndex = 0x70F3F0F0;
	sciPowerjack->m_iGlobalIndexHigh = 0x70F3;
	sciPowerjack->m_iGlobalIndexLow = 0xF0F0;
	sciPowerjack->m_iAccountID = pInventory->m_iAccountID;

	sciPowerjack->m_iPosition = 0x8040005c;

	sciPowerjack->m_Attributes.AddToTail(CScriptCreatedAttribute(180, 75));
	sciPowerjack->m_Attributes.AddToTail(CScriptCreatedAttribute(2, 1.25));
	sciPowerjack->m_Attributes.AddToTail(CScriptCreatedAttribute(15, 0));

	sciPowerjack->m_bInitialized = true;

	pInventory->m_BackPack.AddToTail(*sciPowerjack);

	ItemHasBeenUpdated(pInventory, sciPowerjack, true, false);

	delete sciPowerjack;

	return;
}

DETOUR_DECL_MEMBER3(ItemHasBeenUpdated, int, CScriptCreatedItem *, pItem, bool, a, bool, b) {
	META_CONPRINTF("ItemHasBeenUpdated called!\n");
	META_CONPRINTF("this = 0x%.8X, pItem = 0x%.8X, a = %s, b = %s\n", this, pItem, (a?"true":"false"), (b?"true":"false"));
	META_CONPRINTF("m_iItemDefinitionIndex = %d\n", pItem->m_iItemDefinitionIndex);
	/*META_CONPRINTF("Scout: %d, ",		(0 != (pItem->m_iPosition & ( 1 << ( 0 + 16 )))));
	META_CONPRINTF("Sniper: %d, ",		(0 != (pItem->m_iPosition & ( 1 << ( 1 + 16 )))));
	META_CONPRINTF("Soldier: %d, ",		(0 != (pItem->m_iPosition & ( 1 << ( 2 + 16 )))));
	META_CONPRINTF("Demoman: %d, ",		(0 != (pItem->m_iPosition & ( 1 << ( 3 + 16 )))));
	META_CONPRINTF("Medic: %d, ",		(0 != (pItem->m_iPosition & ( 1 << ( 4 + 16 )))));
	META_CONPRINTF("Heavy: %d, ",		(0 != (pItem->m_iPosition & ( 1 << ( 5 + 16 )))));
	META_CONPRINTF("Pyro: %d, ",		(0 != (pItem->m_iPosition & ( 1 << ( 6 + 16 )))));
	META_CONPRINTF("Spy: %d, ",			(0 != (pItem->m_iPosition & ( 1 << ( 7 + 16 )))));
	META_CONPRINTF("Engineer: %d\n",	(0 != (pItem->m_iPosition & ( 1 << ( 8 + 16 )))));*/

	//pItem->m_iPosition &= ~0x0FFF0000; // This will dequip all items.

	/*
	switch (pItem->m_iItemDefinitionIndex)
	{
	case 40:
		pItem->m_iPosition |= 0x00400000; // Equip for Pyro
		break;
	case 215:
		pItem->m_iPosition &= ~0x00400000; // Dequip for Pyro
		break;
	}
	*/

	return DETOUR_MEMBER_CALL(ItemHasBeenUpdated)(pItem, a, b);
}

void Dump_CScriptCreatedItem(CScriptCreatedItem *cscript)
{
	if (cscript == NULL) {
		return;
	}

	g_pSM->LogMessage(myself, "---------------------------------------");
	g_pSM->LogMessage(myself, ">>> m_pVTable = 0x%.8X", cscript->m_pVTable);
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
	g_pSM->LogMessage(myself, ">>> No. of Attributes = %d", cscript->m_Attributes.Count());
	g_pSM->LogMessage(myself, "---------------------------------------");
	for (int i = 0; i < ((cscript->m_Attributes.Count() > 16)?0:cscript->m_Attributes.Count()); i++)
	{
		g_pSM->LogMessage(myself, ">>> m_pVTable = 0x%.8X", cscript->m_Attributes.Element(i).m_pVTable);
		g_pSM->LogMessage(myself, ">>> m_iAttributeDefinitionIndex = %u", cscript->m_Attributes.Element(i).m_iAttributeDefinitionIndex);
		g_pSM->LogMessage(myself, ">>> m_flValue = %f", cscript->m_Attributes.Element(i).m_flValue);
		g_pSM->LogMessage(myself, ">>> m_szDescription = %ls", cscript->m_Attributes.Element(i).m_szDescription);
		g_pSM->LogMessage(myself, "---------------------------------------");
	}
	g_pSM->LogMessage(myself, ">>> m_bInitialized = %s", cscript->m_bInitialized?"true":"false");
	g_pSM->LogMessage(myself, "---------------------------------------");
}

CBaseEntity *Hook_GiveNamedItem(char const *szClassname, int iSubType, CScriptCreatedItem *cscript, bool b)
{
	#ifdef TF2ITEMS_DEBUG_HOOKING
		 g_pSM->LogMessage(myself, "GiveNamedItem called.");
	#endif // TF2ITEMS_DEBUG_HOOKING

	if (!show_given_items.GetBool())
		RETURN_META_VALUE(MRES_IGNORED, NULL);

	CBaseEntity *player = META_IFACEPTR(CBaseEntity);

	// Retrieve client index.
	edict_t *playerEdict = gameents->BaseEntityToEdict((CBaseEntity *)player);
	IGamePlayer * pPlayer = playerhelpers->GetGamePlayer(playerEdict);
	int client = gamehelpers->IndexOfEdict(playerEdict);

	g_pSM->LogMessage(myself, "---------------------------------------");
	g_pSM->LogMessage(myself, ">>> Start of GiveNamedItem call.");
	g_pSM->LogMessage(myself, "---------------------------------------");
	g_pSM->LogMessage(myself, ">>> Client = %s", pPlayer->GetName());
	g_pSM->LogMessage(myself, ">>> szClassname = %s", szClassname);
	g_pSM->LogMessage(myself, ">>> iSubType = %d", iSubType);
	g_pSM->LogMessage(myself, ">>> b = %s", b?"true":"false");
	g_pSM->LogMessage(myself, "---------------------------------------");

	Dump_CScriptCreatedItem(cscript);

	RETURN_META_VALUE(MRES_IGNORED, NULL);
}

void Hook_ClientPutInServer(edict_t *pEntity, char const *playername) {

	#ifdef TF2ITEMS_DEBUG_HOOKING
		 g_pSM->LogMessage(myself, "ClientPutInServer called.");
	#endif // TF2ITEMS_DEBUG_HOOKING

	if(pEntity->m_pNetworkable) {
		CBaseEntity *player = pEntity->m_pNetworkable->GetBaseEntity();
		if(!player)
			return;

		#ifdef TF2ITEMS_DEBUG_HOOKING
			g_pSM->LogMessage(myself, "---------------------------------------");
			g_pSM->LogMessage(myself, ">>> Start of ClientPutInServer call.");
			g_pSM->LogMessage(myself, "---------------------------------------");
			g_pSM->LogMessage(myself, ">>> Client = %s", playername);
			g_pSM->LogMessage(myself, ">>> ClassName = %s", pEntity->GetClassName());
			g_pSM->LogMessage(myself, "---------------------------------------");
		#endif

		if ((GiveNamedItem_bot_Hook == 0) && (strcmp(pEntity->GetClassName(), "tf_bot") == 0))
		{
			GiveNamedItem_bot_Hook = SH_ADD_MANUALVPHOOK(MHook_GiveNamedItem, player, SH_STATIC(Hook_GiveNamedItem), false);
			#ifdef TF2ITEMS_DEBUG_HOOKING
				g_pSM->LogMessage(myself, "GiveNamedItem hooked (bot).");
			#endif // TF2ITEMS_DEBUG_HOOKING
		} else if(GiveNamedItem_player_Hook == 0) {
			GiveNamedItem_player_Hook = SH_ADD_MANUALVPHOOK(MHook_GiveNamedItem, player, SH_STATIC(Hook_GiveNamedItem), false);
			#ifdef TF2ITEMS_DEBUG_HOOKING
				g_pSM->LogMessage(myself, "GiveNamedItem hooked (player).");
			#endif // TF2ITEMS_DEBUG_HOOKING
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
		META_CONPRINTF("\"GiveNamedItem\" offset = %d\n", iOffset);
	}

	if(!g_pGameConf->GetOffset("InventoryOffset", &g_TFInventoryOffset))
	{
		snprintf(error, maxlen, "Could not find offset for CTFInventory");
		return false;
	} else {
		META_CONPRINTF("\"InventoryOffset\" offset = %d\n", g_TFInventoryOffset); 
	}

	CDetourManager::Init(g_pSM->GetScriptingEngine(), g_pGameConf);

	ItemHasBeenUpdated_Detour = DETOUR_CREATE_MEMBER(ItemHasBeenUpdated, "ItemHasBeenUpdated");

	if (ItemHasBeenUpdated_Detour != NULL)
	{
		ItemHasBeenUpdated_Detour->EnableDetour();
	} else {
		snprintf(error, maxlen, "Detour was null.");
		//g_pSM->LogError(myself, "Detour was null.");
		return false;
	}

	void *addr;
	if (!g_pGameConf->GetMemSig("ItemHasBeenUpdated", &addr) || addr == NULL)
	{
		g_pSM->LogError(myself, "Couldn't find ItemHasBeenUpdated sig.");
		return false;
	}

	ItemHasBeenUpdatedFunc = (ItemHasBeenUpdatedFuncType)addr;

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
			CBaseEntity * pBasePlayer = pEdict->m_pNetworkable->GetBaseEntity();
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
		ClientPutInServer_Hook = SH_ADD_HOOK(IServerGameClients, ClientPutInServer, gameclients, SH_STATIC(Hook_ClientPutInServer), true);
		#ifdef TF2ITEMS_DEBUG_HOOKING
			 g_pSM->LogMessage(myself, "ClientPutInServer hooked.");
		#endif // TF2ITEMS_DEBUG_HOOKING
	}

	sharesys->RegisterLibrary(myself, "TF2Items");

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
}

bool TF2Items::SDK_OnMetamodUnload(char *error, size_t maxlen) {

#ifdef TF2ITEMS_DEBUG_HOOKING
	g_pSM->LogMessage(myself, "SDK_OnMetamodUnload called.");
#endif // TF2ITEMS_DEBUG_HOOKING

	if (ItemHasBeenUpdated_Detour != NULL)
	{
		ItemHasBeenUpdated_Detour->Destroy();
		ItemHasBeenUpdated_Detour = NULL;
	}

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

CPlayerInventory *GetInventory(CBaseEntity *pPlayer) {
	if (!pPlayer || !g_TFInventoryOffset)
	{
		return NULL;
	}

	return (CPlayerInventory *)((char *)pPlayer + g_TFInventoryOffset);
}

void ItemHasBeenUpdated(CPlayerInventory *thisptr, CScriptCreatedItem *pItem, bool a, bool b)
{
	assert(ItemHasBeenUpdatedFunc);
#ifndef WIN32
	ItemHasBeenUpdatedFunc(thisptr, pItem, a, b);
#else
	ItemHasBeenUpdatedFunc(thisptr, NULL, pItem, a, b);
#endif
}
