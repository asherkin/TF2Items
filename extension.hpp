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
#ifdef USE_NEW_ATTRIBS
#define PRESERVE_ATTRIBUTES		(1 << 5)
#endif

class CBasePlayer;

#ifdef USE_NEW_ATTRIBS
template< class T, class I = int >
class CUtlMemoryTF2Items : public CUtlMemory< T, I >
{
public:
	CUtlMemoryTF2Items( int nGrowSize = 0, int nInitSize = 0 ) { CUtlMemory< T, I >( nGrowSize, nInitSize ); }
    CUtlMemoryTF2Items( T* pMemory, int numElements ) { CUtlMemory< T, I >( pMemory, numElements ); }
    CUtlMemoryTF2Items( const T* pMemory, int numElements ) { CUtlMemory< T, I >( pMemory, numElements ); }
    //~CUtlMemoryTF2Items() { ~CUtlMemory< T, I >(); }
    
	void Purge()
	{
		if ( !CUtlMemory< T, I >::IsExternallyAllocated() )
		{
			if (CUtlMemory< T, I >::m_pMemory)
			{
				UTLMEMORY_TRACK_FREE();
				//free( (void*)m_pMemory );
#ifdef TF2ITEMS_DEBUG_ITEMS
				META_CONPRINTF("CUtlMemory tried to be freed!\n");
#endif
				CUtlMemory< T, I >::m_pMemory = 0;
			}
			CUtlMemory< T, I >::m_nAllocationCount = 0;
		}
	}
};
#endif

class CScriptCreatedAttribute							// Win Length = 204 / Lin Length = 396
{
public:
	void * m_pVTable;									// Length = 4 / Win = 0 / Lin = 0

	uint32 m_iAttributeDefinitionIndex;					// Length = 4 / Win = 4 / Lin = 4
	float m_flValue;									// Length = 4 / Win = 8 / Lin = 8
	wchar_t m_szDescription[96];						// Win Length = 192 / Lin Length = 384 / Win = 12 / Lin = 12
};

class CScriptCreatedItem								// Win Length = 3552 / Lin Length = 6868
{
public:
	void * m_pVTable;									// Length = 4 / Win = 0 / Lin = 0

#ifdef _WIN32
	char m_Padding[4];									// Length = 4 / Win = 4 / Lin = N/A
#endif

	uint32 m_iItemDefinitionIndex;						// Length = 4 / Win = 8 / Lin = 4
	uint32 m_iEntityQuality;							// Length = 4 / Win = 12 / Lin = 8
	uint32 m_iEntityLevel;								// Length = 4 / Win = 16 / Lin = 12

#ifdef _WIN32
	char m_Padding2[4];									// Length = 4 / Win = 20 / Lin = N/A
#endif

	uint64 m_iGlobalIndex;								// Length = 8 / Win = 24 / Lin = 16
	uint32 m_iGlobalIndexHigh;							// Length = 4 / Win = 32 / Lin = 24
	uint32 m_iGlobalIndexLow;							// Length = 4 / Win = 36 / Lin = 28
	uint32 m_iAccountID;								// Length = 4 / Win = 40 / Lin = 32
	uint32 m_iPosition;									// Length = 4 / Win = 44 / Lin = 36
	wchar_t m_szWideName[128];							// Win Length = 256 / Lin Length = 512 / Win = 48 / Lin = 40
	char m_szName[128];									// Length = 128 / Win = 304 / Lin = 552

	char m_szBlob[20];									// Length = 20 / Win = 432 / Lin = 680
	wchar_t m_szBlob2[1536];							// Win Length = 3072 / Lin Length = 6144 / Win = 452 / Lin = 700

#ifdef USE_NEW_ATTRIBS
	CUtlVector<CScriptCreatedAttribute, CUtlMemoryTF2Items<CScriptCreatedAttribute> > m_Attributes;	// Length = 20 / Win = 3524 / Lin = 6844
#else
	CScriptCreatedAttribute * m_pAttributes;			// Win: Offset 3524 / Linux: Offset 6844
	uint32 m_iAttributesLength;							// Win: Offset 3528 / Linux: Offset 6848
	uint32 m_iPadding5;									// Win: Offset 3532 / Linux: Offset 6852
	uint32 m_iAttributesCount;							// Win: Offset 3536 / Linux: Offset 6856
	CScriptCreatedAttribute * m_pAttributes2;			// Win: Offset 3540 / Linux: Offset 6860
#endif

	bool m_bInitialized;								// Length = 4 / Win = 3544 / Lin = 6864

#ifdef _WIN32
	char m_Padding3[4];									// Length = 4 / Win = 3548 / Lin = N/A
#endif
};

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

void CSCICopy(CScriptCreatedItem *olditem, CScriptCreatedItem *newitem);

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

TScriptedItemOverride * GetScriptedItemOverrideFromHandle(cell_t cellHandle, IPluginContext *pContext=NULL);

extern HandleType_t g_ScriptedItemOverrideHandleType;
extern TScriptedItemOverrideTypeHandler g_ScriptedItemOverrideHandler;
extern sp_nativeinfo_t g_ExtensionNatives[];
extern IForward * g_pForwardGiveItem;

#endif // _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
