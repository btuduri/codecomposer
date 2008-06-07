#include "Types.h"

#ifndef CRSCMANAGER_H
#define CRSCMANAGER_H

class CRscManager
{
public:
	CRscManager(void);
	~CRscManager(void);

	inline u16 GetTexture(int textureName){	return m_uTextureID[textureName];	}
	inline DSSIZE GetSize(int textureName){	return m_Size[textureName];	}

	int LoadTexture(void);

private:
	u16		m_uTextureID[MAX_TEXTURE];
	DSSIZE	m_Size[MAX_TEXTURE];

};

extern CRscManager		g_cRscManager;


#endif