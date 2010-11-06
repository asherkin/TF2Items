#ifndef CPlayerInventory_h__
#define CPlayerInventory_h__

class CPlayerInventory																		//	56		|	56		?
{
public :
	void *m_pVtbl_CPlayerInventory;															// +0		|
	uint32 m_iAccountID;																	// +4		|
	uint32 m_iUnknown0;																		// +8		|
	CUtlVector<CScriptCreatedItem> m_BackPack;												// +12		|
	void *m_pSortVectorQSortContext;														// +32		|
	bool m_bUnknown2;																		// +36		|
	uint32 m_iUnknown3;																		// +40		|
	void *m_pfnInventoryUpdated;															// +44		|
	void *m_pCSharedObjectCache;															// +48		|
	uint32 m_iUnknown6;																		// +52		|
};
#endif // CPlayerInventory_h__
