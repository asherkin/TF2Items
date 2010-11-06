#ifndef CScriptCreatedAttribute_h__
#define CScriptCreatedAttribute_h__

class CScriptCreatedAttribute							// Win Length = 204 / Lin Length = 396
{
public:
	void * m_pVTable;									// Length = 4 / Win = 0 / Lin = 0

	uint32 m_iAttributeDefinitionIndex;					// Length = 4 / Win = 4 / Lin = 4
	float m_flValue;									// Length = 4 / Win = 8 / Lin = 8
	wchar_t m_szDescription[96];						// Win Length = 192 / Lin Length = 384 / Win = 12 / Lin = 12
};
#endif // CScriptCreatedAttribute_h__
