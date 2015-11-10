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

#define PRESERVE_ATTRIBUTES		(1 << 5)
#define FORCE_GENERATION		(1 << 6)

class CBasePlayer;
class CEconItem;
class ITexture;
class ITextureCompositor;

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

class CEconItemAttribute
{
public:
	void *m_pVTable; //0

	uint16 m_iAttributeDefinitionIndex; //4
	float m_flValue; //8
	int32 m_nRefundableCurrency; //12
};

#pragma pack(push, 4)


class CEconItemHandle
{
public:
	void *m_pVTable;

	CEconItem *m_pItem;

	int64 m_ulItemID;
	uint64 m_SteamID;
};

class CAttributeList
{
public:
	void *m_pVTable;

	CUtlVector<CEconItemAttribute, CUtlMemoryTF2Items<CEconItemAttribute> > m_Attributes;
	void *m_pAttributeManager;


public:
	CAttributeList& operator=( const CAttributeList &other )
	{
		m_pVTable = other.m_pVTable;

		m_Attributes = other.m_Attributes;
		m_pAttributeManager = other.m_pAttributeManager;


		return *this;
	}
};

class CEconItemView
{
public:
	void *m_pVTable; //0

	uint16 m_iItemDefinitionIndex; //4
	
	int32 m_iEntityQuality; //8
	uint32 m_iEntityLevel; //12

	uint64 m_iItemID; //16
	uint32 m_iItemIDHigh; //24
	uint32 m_iItemIDLow; //28

	uint32 m_iAccountID; //32

	uint32 m_iInventoryPosition; //36
	
	CEconItemHandle m_ItemHandle; //40 (44, 48, 52, 56, 60)

	bool m_bColorInit; //64
	bool m_bPaintOverrideInit; //65
	bool m_bHasPaintOverride; //66
	//67

	float m_flOverrideIndex; //68
	uint32 m_unRGB; //72
	uint32 m_unAltRGB; //76

	int32 m_iTeamNumber; //80

	bool m_bInitialized; //84

	CAttributeList m_AttributeList; //88 (92, 96, 100, 104, 108, 112)
	CAttributeList m_NetworkedDynamicAttributesForDemos; //116 (120, 124, 128, 132, 136, 140)

	bool m_bDoNotIterateStaticAttributes; //144
};

#pragma pack(pop)

static_assert(sizeof(CEconItemView) == 148, "CEconItemView - incorrect size on this compiler");
static_assert(sizeof(CEconItemHandle) == 24, "CEconItemHandle - incorrect size on this compiler");
static_assert(sizeof(CAttributeList) == 28, "CAttributeList - incorrect size on this compiler");

// enable to debug memory layout issues
#if 0
template<int s> struct Sizer;
Sizer<sizeof(CEconItemView)> CEconItemViewSize;
Sizer<sizeof(CEconItemHandle)> CEconItemHandleSize;
Sizer<sizeof(CAttributeList)> CAttributeListSize;
#endif

struct TScriptedItemOverride
{
	uint8 m_bFlags;									// Flags to what we should override.
	char m_strWeaponClassname[256];					// Classname to override the GiveNamedItem call with.
	uint32 m_iItemDefinitionIndex;					// New Item Def. Index.
	uint8 m_iEntityQuality;							// New Item Quality Level.
	uint8 m_iEntityLevel;							// New Item Level.
	uint8 m_iCount;									// Count of Attributes.
	CEconItemAttribute m_Attributes[16];			// The actual attributes.
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

void CSCICopy(CEconItemView *olditem, CEconItemView *newitem);

static cell_t TF2Items_GiveNamedItem(IPluginContext *pContext, const cell_t *params);
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
static cell_t TF2Items_GetCurrentSlot(IPluginContext *pContext, const cell_t *params);

CBaseEntity * GetCBaseEntityFromIndex(int p_iEntity, bool p_bOnlyPlayers);
int GetIndexFromCBaseEntity(CBaseEntity * p_hEntity);
TScriptedItemOverride * GetScriptedItemOverrideFromHandle(cell_t cellHandle, IPluginContext *pContext=NULL);

extern HandleType_t g_ScriptedItemOverrideHandleType;
extern TScriptedItemOverrideTypeHandler g_ScriptedItemOverrideHandler;
extern sp_nativeinfo_t g_ExtensionNatives[];
extern IForward *g_pForwardGiveItem;
extern IForward *g_pForwardGiveItem_Post;

#endif // _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
