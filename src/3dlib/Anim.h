#ifndef HIGHPRIEST_3DLIB_ANIM_H
#define HIGHPRIEST_3DLIB_ANIM_H

#include "Prim.h"
#include "../base/File.h"

class C3dPosAnim;
class C3dRotAnim;
class C3dScaleAnim;
struct posKeyframe;
struct rotKeyframe;
struct scaleKeyframe;

class C3dPosAnim {
public:
	void AddKey(int frame, float xa, float ya, float za);
	void ReleaseAllKeys();
	void GetMatrix(int frame, matrix& result);
	void Load(CFile* fp);
	int Size();
private:
	std::vector<posKeyframe> m_animdata;
};

class C3dRotAnim {
public:
	void AddKey(int frame, float xa, float ya, float za, float angle);
	void ReleaseAllKeys();
	void GetMatrix(int frame, matrix& result);
	void Load(CFile* fp);
	int Size();
private:
	std::vector<rotKeyframe> m_animdata;
};

class C3dScaleAnim {
public:
	void AddKey(int frame, float sx, float sy, float sz, float xa, float ya, float za, float angle);
	void ReleaseAllKeys();
	void GetMatrix(int frame, matrix& result);
	void Load(CFile* fp);
	int Size();
private:
	std::vector<scaleKeyframe> m_animdata;
};

struct posKeyframe {
public:
	posKeyframe(int, float, float, float);
	posKeyframe();
	void Slerp(float t, posKeyframe& start, posKeyframe& end);
	int frame;
	float px;
	float py;
	float pz;
};

struct scaleKeyframe {
public:
	scaleKeyframe(int, float, float, float, float, float, float, float);
	scaleKeyframe();
	void Slerp(float t, scaleKeyframe& start, scaleKeyframe& bflip, int end);
	int frame;
	float sx;
	float sy;
	float sz;
	float qx;
	float qy;
	float qz;
	float qw;
};

struct rotKeyframe {
public:
	rotKeyframe(int, float, float, float, float);
	rotKeyframe();
	void Slerp(float t, rotKeyframe& start, rotKeyframe& bflip, int end);
	int frame;
	float qx;
	float qy;
	float qz;
	float qw;
};

inline int C3dScaleAnim::Size() { return m_animdata.size(); }
inline int C3dPosAnim::Size() { return m_animdata.size(); }
inline int C3dRotAnim::Size() { return m_animdata.size(); }

#endif
