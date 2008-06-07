#include "Types.h"

#ifndef CAMERA_H
#define CAMERA_H

class CCamera
{
public:
	CCamera(void);
	~CCamera(void);

private:
	DSVECTOR m_vPosW;
	DSVECTOR m_vRightW;
	DSVECTOR m_vUpW;
	DSVECTOR m_vLook;

	u8 m_uCameraMode;
};


#endif