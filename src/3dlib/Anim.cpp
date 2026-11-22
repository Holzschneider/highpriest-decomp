#include "common.h"
#include "Anim.h"
#include "../base/File.h"
#include <math.h>





const float EPSILON = 1.0E-6f;

inline posKeyframe :: posKeyframe() {}
inline posKeyframe :: posKeyframe(int nframe, float nx, float ny, float nz) : frame(nframe), px(nx), py(ny), pz(nz) {}
inline rotKeyframe :: rotKeyframe() {}
inline rotKeyframe :: rotKeyframe(int nframe, float nx, float ny, float nz, float nw) : frame(nframe), qx(nx), qy(ny), qz(nz), qw(nw) {}
inline scaleKeyframe :: scaleKeyframe() {}
inline scaleKeyframe :: scaleKeyframe(int nframe, float nx, float ny, float nz, float ax, float ay, float az, float aw) : frame(nframe), sx(nx), sy(ny), sz(nz), qx(ax), qy(ay), qz(az), qw(aw) {}

void	posKeyframe :: Slerp(float t, posKeyframe &start, posKeyframe &end)
{
	float	beta = 1.0 - t;
 	px = beta*start.px + t*end.px;
 	py = beta*start.py + t*end.py;
 	pz = beta*start.pz + t*end.pz;
}

void	rotKeyframe :: Slerp(float t, rotKeyframe &start, rotKeyframe &end, int spin)
{
	float	beta;
	float	theta;
	float	sin_t, cos_t;
	float	phi;
	bool	bflip;


	cos_t = start.qx*end.qx + start.qy*end.qy + start.qz*end.qz + start.qw*end.qw;

 	if (cos_t < 0.0) {
		cos_t = -cos_t;
		bflip = true;
	} else
		bflip = false;
	if (1.0 - cos_t < EPSILON) {
		beta = 1.0 - t;
 	} else {
 		theta = acos(cos_t);
 		phi = theta + spin * pi;
 		sin_t = _sin(theta);
 		beta = _sin(theta - t*phi) / sin_t;
 		t = _sin(t*phi) / sin_t;
 	}
	if (bflip)
		t = -t;
 	qx = beta*start.qx + t*end.qx;
 	qy = beta*start.qy + t*end.qy;
 	qz = beta*start.qz + t*end.qz;
 	qw = beta*start.qw + t*end.qw;
}

void	scaleKeyframe :: Slerp(float t, scaleKeyframe &start, scaleKeyframe &end, int spin)
{
	float	beta = 1.0 - t;
 	sx = beta*start.sx + t*end.sx;
 	sy = beta*start.sy + t*end.sy;
 	sz = beta*start.sz + t*end.sz;

	float	theta;
	float	sin_t, cos_t;
	float	phi;
	bool	bflip;


	cos_t = start.qx*end.qx + start.qy*end.qy + start.qz*end.qz + start.qw*end.qw;

 	if (cos_t < 0.0) {
		cos_t = -cos_t;
		bflip = true;
	} else
		bflip = false;
	if (1.0 - cos_t < EPSILON) {
		beta = 1.0 - t;
 	} else {
 		theta = acos(cos_t);
 		phi = theta + spin * pi;
 		sin_t = _sin(theta);
 		beta = _sin(theta - t*phi) / sin_t;
 		t = _sin(t*phi) / sin_t;
 	}
	if (bflip)
		t = -t;
 	qx = beta*start.qx + t*end.qx;
 	qy = beta*start.qy + t*end.qy;
 	qz = beta*start.qz + t*end.qz;
 	qw = beta*start.qw + t*end.qw;
}

void	C3dPosAnim :: AddKey(int frame, float xa, float ya, float za)
{
	m_animdata.push_back(posKeyframe(frame, xa, ya, za));
}

void	C3dPosAnim :: ReleaseAllKeys()
{
	m_animdata.erase(m_animdata.begin(), m_animdata.end());
}

void	C3dPosAnim :: GetMatrix(int frame, matrix &result)
{
	int			prevframe, nextframe;
	float		t;
	posKeyframe	curanim;

	if (!m_animdata.size()) {
		result.MakeIdentity();
		return;
	} else if (m_animdata.size() == 1) {
		result.MakeTranslate(m_animdata[0].px, m_animdata[0].py, m_animdata[0].pz);
	} else {
		for (int i=1; i<m_animdata.size(); i++) {
			if (m_animdata[i].frame > frame) {
				prevframe = m_animdata[i-1].frame;
				nextframe = m_animdata[i].frame;
				t = float(frame-prevframe)/float(nextframe-prevframe);
				curanim.Slerp(t, m_animdata[i-1], m_animdata[i]);
				result.MakeTranslate(curanim.px, curanim.py, curanim.pz);
				return;
			}
		}
	}
}

void	C3dPosAnim :: Load(CFile *fp)
{
	int		size;
	fp->Read(&size, sizeof(int));
	for (int i=0; i<size; i++) {
		posKeyframe	a;
		fp->Read(&a, sizeof(posKeyframe));
		m_animdata.push_back(a);
	}
}

void	C3dRotAnim :: Load(CFile *fp)
{
	int		size;
	fp->Read(&size, sizeof(int));
	for (int i=0; i<size; i++) {
		rotKeyframe	a;
		fp->Read(&a, sizeof(rotKeyframe));
		m_animdata.push_back(a);
	}
}


void	C3dRotAnim :: AddKey(int frame, float xa, float ya, float za, float angle)
{
	float	omega = angle / 2.0f;
	float	s = _sin(omega);
	float	qx = xa*s;
	float	qy = ya*s;
	float	qz = za*s;
	float	qw = _cos(omega);

	m_animdata.push_back(rotKeyframe(frame, qx, qy, qz, qw));
}

void	C3dRotAnim :: ReleaseAllKeys()
{
	m_animdata.erase(m_animdata.begin(), m_animdata.end());
}

void	C3dRotAnim :: GetMatrix(int frame, matrix &result)
{
	int			prevframe, nextframe;
	float		t;
	rotKeyframe	curanim;

	if (m_animdata.size() == 0) {
		result.MakeIdentity();
		return;
	} else if (m_animdata.size() == 1) {
		result.MakeTransform(m_animdata[0]);
	} else {
		for (int i=1; i<m_animdata.size(); i++) {
			if (m_animdata[i].frame > frame) {
				if (m_animdata[i-1].qw == 1.0f && m_animdata[i].qw == 1.0f) {
					result.MakeIdentity();
				} else {
					if (i==0 && m_animdata.size() > 1) {
						curanim.Slerp(0, m_animdata[0], m_animdata[1], 0);
						result.MakeTransform(curanim);
					} else {
						prevframe = m_animdata[i-1].frame;
						nextframe = m_animdata[i].frame;
						t = float(frame-prevframe)/float(nextframe-prevframe);
						curanim.Slerp(t, m_animdata[i-1], m_animdata[i], 0);
						result.MakeTransform(curanim);
					}
				}
				return;
			}
		}
	}
}

void	C3dScaleAnim :: AddKey(int frame, float sx, float sy, float sz, float xa, float ya, float za, float angle)
{
	float	omega = angle / 2.0f;
	float	s = _sin(omega);
	float	qx = xa*s;
	float	qy = ya*s;
	float	qz = za*s;
	float	qw = _cos(omega);

	m_animdata.push_back(scaleKeyframe(frame, sx, sy, sz, qx, qy, qz, qw));
}

void	C3dScaleAnim :: ReleaseAllKeys()
{
	m_animdata.erase(m_animdata.begin(), m_animdata.end());
}

void	C3dScaleAnim :: GetMatrix(int frame, matrix &result)
{
	if (m_animdata.size() == 0) {
		result.MakeIdentity();
		return;
	}
	if (m_animdata.size() == 1) {
		result.MakeScale(m_animdata[0].sx, m_animdata[0].sy, m_animdata[0].sz);
	}
}

void	C3dScaleAnim :: Load(CFile *fp)
{
	int		size;
	fp->Read(&size, sizeof(int));
	for (int i=0; i<size; i++) {
		scaleKeyframe	a;
		fp->Read(&a, sizeof(scaleKeyframe));
		m_animdata.push_back(a);
	}
}
