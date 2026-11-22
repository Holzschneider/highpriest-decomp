#ifndef HIGHPRIEST_3DLIB_VOLUME_H
#define HIGHPRIEST_3DLIB_VOLUME_H

#include "Prim.h"

struct CVolumeBox_v11 {
public:
	vector3d m_size;
	vector3d m_pos;
	vector3d m_rot;
};

struct CVolumeBox_v21 {
public:
	vector3d m_size;
	vector3d m_pos;
	vector3d m_rot;
	int flag;
};

struct CVolumeBox {
public:
	vector3d m_size;
	vector3d m_pos;
	vector3d m_rot;
	vector3d m_worldVert[8];
	matrix m_vtm;
	matrix m_ivtm;
	int flag;
};

#endif
