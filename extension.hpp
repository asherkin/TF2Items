/**
 * =============================================================================
 * TF2 Items Extension
 * Copyright (C) 2009-2010 AzuiSleet, Asher Baker (asherkin).  All rights reserved.
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
 * @file extension.hpp
 * @brief TF2Items extension code header.
 */

#include "smsdk_ext.hpp"

//#include "iplayerinfo.h"
//#include "convar.h"

#define OVERRIDE_CLASSNAME		(1 << 0)
#define OVERRIDE_ITEM_DEF		(1 << 1)
#define OVERRIDE_ITEM_LEVEL		(1 << 2)
#define OVERRIDE_ITEM_QUALITY	(1 << 3)
#define OVERRIDE_ATTRIBUTES		(1 << 4)

//class CBaseEntity;
class CBasePlayer;
//class CPersistentItem;
//class CPersistentAttributeDefinition;

#pragma pack(push, 1)

class CScriptCreatedAttribute		// Linux size: 0x18C
{
	void * m_pVTable;
public:
	uint32 m_iAttributeDefinitionIndex;				// Win: Offset 4 / Linux: Offset 4
	float m_flValue;								// Win: Offset 8 / Linux: Offset 8

	#ifdef _LINUX
		uint8 m_iPadding[0x180];
	#else
		uint8 m_iPadding[0xC0];
	#endif
};

/* CScriptCreatedAttribute()
** -------------------------------------------------------------------------- */
class CScriptCreatedItem			// Windows size: 0xDD8 / Linux size: 0x1AD0
{
public:
	void * m_pVTable;

#ifndef _LINUX
	void * m_pPersistentItem;						// Present before in Linux?
#endif
	uint32 m_iItemDefinitionIndex;					// Win: Offset 8 / Linux: Offset 4
	uint8 m_iEntityQuality; uint8 m_iPadding1[3];	// Win: Offset 12 / Linux: Offset 8
	uint8 m_iEntityLevel;							// Win: Offset 16 / Linux: Offset 12

#ifdef _LINUX
	uint8 m_iPadding2[0xB];
#else
	uint8 m_iPadding2[0xF];
#endif
	
	uint32 m_iGlobalIndexHigh;						// Win: Offset 32 / Linux: Offset 24
	uint32 m_iGlobalIndexLow; 						// Win: Offset 36 / Linux: Offset 28

#ifdef _LINUX
    uint8 m_iPadding4[0x1A98];
#else
	uint8 m_iPadding4[0xD98];
#endif

	CScriptCreatedAttribute * m_pAttributes;		// Win: Offset 3520 / Linux: Offset 6840
	uint32 m_iAttributesLength;						// Win: Offset 3524 / Linux: Offset 6844
	uint32 m_iPadding5;								// Win: Offset 3528 / Linux: Offset 6848
	uint32 m_iAttributesCount;						// Win: Offset 3532 / Linux: Offset 6852
	CScriptCreatedAttribute * m_pAttributes2;		// Win: Offset 3536 / Linux: Offset 6856
	
	bool m_bInitialized; uint8 m_iPadding6[3];		// Win: Offset 3540 / Linux: Offset 6860
};

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

static cell_t TF2Items_CreateItem(IPluginContext *pContext, const cell_t *params);
static cell_t TF2Items_SetFlags(IPluginContext *pContext, const cell_t *params);
static cell_t TF2Items_GetFlags(IPluginContext *pContext, const cell_t *params);
static cell_t TF2Items_SetClassname(IPluginContext *pContext, const cell_t *params);
static cell_t TF2Items_GetClassname(IPluginContext *pContext, const cell_t *params);
static cell_t TF2Items_SetItemIndex(IPluginContext *pContext, const cell_t *params);
static cell_t TF2Items_GetItemIndex(IPluginContext *pContext, const cell_t *params);
static cell_t TF2Items_SetQuality(IPluginContext *pContext, const cell_t *params);
static cell_t TF2Items_GetQuality(IPluginContext *pContext, const cell_t *params);
static cell_t TF2Items_SetLevel(IPluginContext *pContext, const cell_t *params);
static cell_t TF2Items_GetLevel(IPluginContext *pContext, const cell_t *params);
static cell_t TF2Items_SetNumAttributes(IPluginContext *pContext, const cell_t *params);
static cell_t TF2Items_GetNumAttributes(IPluginContext *pContext, const cell_t *params);
static cell_t TF2Items_SetAttribute(IPluginContext *pContext, const cell_t *params);
static cell_t TF2Items_GetAttributeId(IPluginContext *pContext, const cell_t *params);
static cell_t TF2Items_GetAttributeValue(IPluginContext *pContext, const cell_t *params);
static cell_t TF2Items_GiveNamedItem(IPluginContext *pContext, const cell_t *params);

static cell_t TF2Items_EquipWearable(IPluginContext *pContext, const cell_t *params);
static cell_t TF2Items_RemoveWearable(IPluginContext *pContext, const cell_t *params);

TScriptedItemOverride * GetScriptedItemOverrideFromHandle(cell_t cellHandle, IPluginContext *pContext=NULL);

extern HandleType_t g_ScriptedItemOverrideHandleType;
extern TScriptedItemOverrideTypeHandler g_ScriptedItemOverrideHandler;
extern sp_nativeinfo_t g_ExtensionNatives[];
extern IForward * g_pForwardGiveItem;

#endif // _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
