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

#ifndef _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_

/**
 * @file extension.h
 * @brief TF2 Items Sample extension code header.
 */

#include "smsdk_ext.h"

#include "filesystem.h"
#include "iplayerinfo.h"
#include "convar.h"

#define OVERRIDE_CLASSNAME		(1 << 0)
#define OVERRIDE_ITEM_DEF		(1 << 1)
#define OVERRIDE_ITEM_LEVEL		(1 << 2)
#define OVERRIDE_ITEM_QUALITY	(1 << 3)
#define OVERRIDE_ATTRIBUTES		(1 << 4)


class CBaseEntity;
class CBasePlayer;
class CPersistentItem;
class CPersistentAttributeDefinition;

#pragma pack(push, 1)

class CScriptCreatedAttribute
{
	void *vftable;

public:
	uint32 attribindex;
	float attribvalue;

#ifdef _LINUX
	char padding[0x180];
#else
	char padding[0xC0];
#endif
};

// 0xCC

class CScriptCreatedItem
{
	void *vftable;

public:
#ifndef _LINUX
	CPersistentItem *pitem; // contains the item created from backend and static data
#endif

	uint32 itemdefindex;
	uint32 itemquality;
	uint32 itemlevel;

#ifdef _LINUX
	char padding[0x1AA8];
#else
	uint32 unknown4;
	uint64 itemid;
	uint32 unknown5;
	uint32 unknown6;
	uint16 unknown7;
	uint16 position; // lower 16 bits of position
	wchar name[256];
	char iname[128];

	char padding[0xB14];
#endif

	CScriptCreatedAttribute *attributes; // 0xDC0
	uint32 allocatedAttributes; // 0xDC4
	uint32 unknown8; // 0xDC8
	uint32 attribcount; // 0xDCC
	CScriptCreatedAttribute *attributes2; // 0xDD0
	uint32 unknown9; // 0xDD4 = 1
};

// 1AC

#pragma pack(pop)

struct TScriptedItemOverride
{
	uint8 m_bFlags;									// Flags to what we should override.
	char m_strWeaponClassname[256];					// Classname to override the GiveNamedItem call with.
	uint32 m_iItemDefinitionIndex;					// New Item Def. Index.
	uint8 m_iEntityQuality;							// New Item Quality Level.
	uint8 m_iEntityLevel;							// New Item Level.
	uint8 m_iCount;									// Count of Attributes.
	CScriptCreatedAttribute m_Attributes[16];		// The actual attributes.
};

class TScriptedItemOverrideTypeHandler : public IHandleTypeDispatch
{
public:
	void OnHandleDestroy(HandleType_t type, void *object);
};

/**
 * @brief Sample implementation of the SDK Extension.
 * Note: Uncomment one of the pre-defined virtual functions in order to use it.
 */
class TF2Items : public SDKExtension, public IConCommandBaseAccessor
{
public:
	/**
	 * @brief This is called after the initial loading sequence has been processed.
	 *
	 * @param error		Error message buffer.
	 * @param maxlength	Size of error message buffer.
	 * @param late		Whether or not the module was loaded after map load.
	 * @return			True to succeed loading, false to fail.
	 */
	virtual bool SDK_OnLoad(char *error, size_t maxlen, bool late);
	
	/**
	 * @brief This is called right before the extension is unloaded.
	 */
	virtual void SDK_OnUnload();

	/**
	 * @brief This is called once all known extensions have been loaded.
	 * Note: It is is a good idea to add natives here, if any are provided.
	 */
	//virtual void SDK_OnAllLoaded();

	/**
	 * @brief Called when the pause state is changed.
	 */
	//virtual void SDK_OnPauseChange(bool paused);

	/**
	 * @brief this is called when Core wants to know if your extension is working.
	 *
	 * @param error		Error message buffer.
	 * @param maxlength	Size of error message buffer.
	 * @return			True if working, false otherwise.
	 */
	//virtual bool QueryRunning(char *error, size_t maxlen);
public:
#if defined SMEXT_CONF_METAMOD
	/**
	 * @brief Called when Metamod is attached, before the extension version is called.
	 *
	 * @param error			Error buffer.
	 * @param maxlength		Maximum size of error buffer.
	 * @param late			Whether or not Metamod considers this a late load.
	 * @return				True to succeed, false to fail.
	 */
	virtual bool SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlen, bool late);

	/**
	 * @brief Called when Metamod is detaching, after the extension version is called.
	 * NOTE: By default this is blocked unless sent from SourceMod.
	 *
	 * @param error			Error buffer.
	 * @param maxlength		Maximum size of error buffer.
	 * @return				True to succeed, false to fail.
	 */
	virtual bool SDK_OnMetamodUnload(char *error, size_t maxlen);

	/**
	 * @brief Called when Metamod's pause state is changing.
	 * NOTE: By default this is blocked unless sent from SourceMod.
	 *
	 * @param paused		Pause state being set.
	 * @param error			Error buffer.
	 * @param maxlength		Maximum size of error buffer.
	 * @return				True to succeed, false to fail.
	 */
	//virtual bool SDK_OnMetamodPauseChange(bool paused, char *error, size_t maxlen);
#endif
public: //IConCommandBaseAccessor
	bool RegisterConCommandBase(ConCommandBase *pCommand);
};

bool KV_FindSection(KeyValues *found, KeyValues *source, const char *search);
bool KV_FindSection(KeyValues *found, KeyValues *source, int search);
bool KV_FindValue(int *found, KeyValues *source, const char *search);
CScriptCreatedItem EditWeaponFromFile(CScriptCreatedItem *newitem, KeyValues *player_weapon);

static cell_t CreateScriptedItemOverride(IPluginContext *pContext, const cell_t *params);
static cell_t SetOverrideFlags(IPluginContext *pContext, const cell_t *params);
static cell_t GetOverrideFlags(IPluginContext *pContext, const cell_t *params);
static cell_t SetOverrideClassname(IPluginContext *pContext, const cell_t *params);
static cell_t GetOverrideClassname(IPluginContext *pContext, const cell_t *params);
static cell_t SetOverrideItemDefinitionIndex(IPluginContext *pContext, const cell_t *params);
static cell_t GetOverrideItemDefinitionIndex(IPluginContext *pContext, const cell_t *params);
static cell_t SetOverrideQuality(IPluginContext *pContext, const cell_t *params);
static cell_t GetOverrideQuality(IPluginContext *pContext, const cell_t *params);
static cell_t SetOverrideLevel(IPluginContext *pContext, const cell_t *params);
static cell_t GetOverrideLevel(IPluginContext *pContext, const cell_t *params);
static cell_t SetOverrideNumAttributes(IPluginContext *pContext, const cell_t *params);
static cell_t GetOverrideNumAttributes(IPluginContext *pContext, const cell_t *params);
static cell_t SetOverrideAttribute(IPluginContext *pContext, const cell_t *params);
static cell_t GetOverrideAttributeId(IPluginContext *pContext, const cell_t *params);
static cell_t GetOverrideAttributeValue(IPluginContext *pContext, const cell_t *params);

TScriptedItemOverride * GetScriptedItemOverrideFromHandle(cell_t cellHandle, IPluginContext *pContext=NULL);

extern HandleType_t g_ScriptedItemOverrideHandleType;
extern TScriptedItemOverrideTypeHandler g_ScriptedItemOverrideHandler;
extern sp_nativeinfo_t g_ExtensionNatives[];
extern IForward * g_pForwardGiveItem;

#endif // _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
