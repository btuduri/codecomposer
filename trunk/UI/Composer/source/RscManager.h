#include "Types.h"

#ifndef CRSCMANAGER_H
#define CRSCMANAGER_H

class CRscManager
{
public:
	CRscManager(void);
	~CRscManager(void);

	u16 GetTexture(u8 textureName){	return m_uTextureID[textureName];	}
	DSSIZE GetSize(u8 textureName){	return m_Size[textureName];	}

	void LoadTexture(void);

private:
	u8		m_uTextureID[MAX_TEXTURE];
	DSSIZE	m_Size[MAX_TEXTURE];

};

extern CRscManager		g_cRscManager;


#endif