#ifndef CUtlMemoryTF2Items_h__
#define CUtlMemoryTF2Items_h__

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
				CUtlMemory< T, I >::m_pMemory = 0;
			}
			CUtlMemory< T, I >::m_nAllocationCount = 0;
		}
	}
};
#endif // CUtlMemoryTF2Items_h__
