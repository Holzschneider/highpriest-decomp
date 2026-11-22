#ifndef HIGHPRIEST_3DLIB_PRIM_H
#define HIGHPRIEST_3DLIB_PRIM_H

#include "../common.h"

const float pi = 3.141592f;
const float	epsilon = 0.00001f;
const float infinity = 999999;

inline	float	DegToRad(float deg)
{
	return deg / 180.0f * pi;
}

inline	float	RadToDeg(float rad)
{
	return rad / pi * 180;
}


struct rotKeyframe;


struct matrix;

struct vector4d
{
	vector4d() {}
	vector4d(float fx, float fy, float fz, float foow) : x(fx), y(fy), z(fz), oow(foow) {}

	float x, y, z, oow;
};

struct vector3d {
public:
	vector3d(float nx, float ny, float nz) : x(nx), y(ny), z(nz) {}
	vector3d() {}
	void Set(float nx, float ny, float nz) { x = nx; y = ny; z = nz; }
	void MatrixMult(const vector3d&, const matrix&);
	void CrossProduct(const vector3d& a, const vector3d& b)
	{
		x = a.y * b.z - a.z * b.y;
		y = a.z * b.x - a.x * b.z;
		z = a.x * b.y - a.y * b.x;
	}
	void Normalize();
	float Magnitude();
	float Angle(const vector3d&) const;
	float DotProduct(const vector3d& b) const { return x * b.x + y * b.y + z * b.z; }
	void MATRIX_TO_VECTOR(matrix& dir);
	vector3d& operator=(const vector3d& rhs) { x = rhs.x; y = rhs.y; z = rhs.z; return *this; }
	vector3d& operator+=(const vector3d& rhs) { x += rhs.x; y += rhs.y; z += rhs.z; return *this; }
	vector3d& operator-=(const vector3d& rhs) { x -= rhs.x; y -= rhs.y; z -= rhs.z; return *this; }
	vector3d& operator*=(const float& f) { x *= f; y *= f; z *= f; return *this; }
	vector3d operator*(const float& f) { return vector3d(x * f, y * f, z * f); }
	float x;
	float y;
	float z;
};

inline vector3d operator-(const vector3d& a, const vector3d& b)
{
	return vector3d(a.x - b.x, a.y - b.y, a.z - b.z);
}

inline vector3d operator+(const vector3d& a, const vector3d& b)
{
	return vector3d(a.x + b.x, a.y + b.y, a.z + b.z);
}

#include <math.h>


inline void vector3d::Normalize()
{
	float oomag = 1.0f / _sqrt(x * x + y * y + z * z);
	x *= oomag;
	y *= oomag;
	z *= oomag;
}

inline float vector3d::Magnitude()
{
	return _sqrt(x * x + y * y + z * z);
}

inline float vector3d::Angle(const vector3d &n) const
{
	return acosf(x*n.x + y*n.x + z*n.z);
}

struct vector2d {
	vector2d() {}
	vector2d(float ix, float iy) : x(ix), y(iy) {}
	float x, y;
	void Set(float ix, float iy) { x = ix; y = iy; }
	void Normalize();
};

inline void vector2d :: Normalize()
{
	float oomag = float(1/_sqrt(x*x+y*y));
	x *= oomag;
	y *= oomag;
}

float Distance(vector2d &v1, vector2d &v2);

inline float Distance(vector2d &v1, vector2d &v2)
{
	return (v1.x-v2.x)*(v1.x-v2.x) + (v1.y-v2.y)*(v1.y-v2.y);
}

struct plane3d {
	plane3d() {}
	plane3d(float nx, float ny, float nz, float nw) : x(nx), y(ny), z(nz), w(nw) {}
	void Set(float nx, float ny, float nz, float nw);
	void MatrixMult(const matrix& b, const plane3d& other);
	float x, y, z, w;
};

inline void plane3d::Set(float nx, float ny, float nz, float nw)
{
	x = nx;
	y = ny;
	z = nz;
	w = nw;
}

struct face3d_v11 {
	WORD vertindex[3];
	WORD tvertindex[3];
	WORD meshMtlId;
	BOOL twoSide;
	void Set(int n1, int n2, int n3) { vertindex[0] = n1; vertindex[1] = n2; vertindex[2] = n3; }
	void Set(int n1, int n2, int n3, int mtlId) { vertindex[0] = n1; vertindex[1] = n2; vertindex[2] = n3; meshMtlId = mtlId; }
	void SetT(int n1, int n2, int n3) { tvertindex[0] = n1; tvertindex[1] = n2; tvertindex[2] = n3; }
};

struct face3d {
public:
	void Set(int n1, int n2, int n3, int mtlId) { vertindex[0] = n1; vertindex[1] = n2; vertindex[2] = n3; meshMtlId = mtlId; }
	void Set(int n1, int n2, int n3) { vertindex[0] = n1; vertindex[1] = n2; vertindex[2] = n3; }
	void SetT(int n1, int n2, int n3) { tvertindex[0] = n1; tvertindex[1] = n2; tvertindex[2] = n3; }
	face3d& operator=(const face3d_v11& rhs)
	{
		vertindex[0] = rhs.vertindex[0];
		vertindex[1] = rhs.vertindex[1];
		vertindex[2] = rhs.vertindex[2];
		tvertindex[0] = rhs.tvertindex[0];
		tvertindex[1] = rhs.tvertindex[1];
		tvertindex[2] = rhs.tvertindex[2];
		twoSide = rhs.twoSide;
		meshMtlId = rhs.meshMtlId;
		return *this;
	}
	face3d& operator=(const face3d& rhs);
	bool operator<(const face3d& face) { return meshMtlId < face.meshMtlId; }
	unsigned short vertindex[3];
	unsigned short tvertindex[3];
	unsigned short meshMtlId;
	int twoSide;
	int smoothGroup;
};

inline	face3d &	face3d :: operator = (const face3d &rhs)
{
	vertindex[0] = rhs.vertindex[0];
	vertindex[1] = rhs.vertindex[1];
	vertindex[2] = rhs.vertindex[2];
	tvertindex[0] = rhs.tvertindex[0];
	tvertindex[1] = rhs.tvertindex[1];
	tvertindex[2] = rhs.tvertindex[2];
	meshMtlId = rhs.meshMtlId;
	twoSide = rhs.twoSide;
	smoothGroup = rhs.smoothGroup;
	return	*this;
}

struct tvertex3d {
	DWORD color;
	float u, v;
	void Set(float nu, float nv) {
		if (nu<0) u=0; else if (nu>1) u=1; else u=nu;
		if (nv<0) v=0; else if (nv>1) v=1; else v=nv;
	}
};

struct texCoor {
	float u, v;
	void Set(float nu, float nv) {
		if (nu<0) u=0; else if (nu>1) u=1; else u=nu;
		if (nv<0) v=0; else if (nv>1) v=1; else v=nv;
	}
};

struct matrix {
public:
	void SetRow1(float x, float y, float z);
	void SetRow2(float x, float y, float z);
	void SetRow3(float x, float y, float z);
	void SetRow4(float x, float y, float z);
	void MakeIdentity();
	void MakeInverse(const matrix& src);
	void MakeTranslate(const vector3d& v) { MakeTranslate(v.x, v.y, v.z); }
	void MakeTranslate(float x, float y, float z);
	void MakeXRotation(float angle);
	void MakeYRotation(float angle);
	void MakeZRotation(float angle);
	void MakeScale(float x, float y, float z);
	void AppendTranslate(const vector3d& v);
	void AppendXRotation(float angle);
	void AppendYRotation(float angle);
	void AppendZRotation(float angle);
	void AppendScale(float x, float y, float z);
	void PrependTranslate(const vector3d& v);
	void PrependScale(float s[3]) { PrependScale(s[0], s[1], s[2]); }
	void PrependScale(float x, float y, float z);
	void MakeTransform(rotKeyframe& pos);
	void MakeTransform(float rotaxis[3], float rotangle);
	void MakeTransform(const vector3d& pos, float rotaxis[3], float rotangle);
	void MakeView(const vector3d& from, const vector3d& at, const vector3d& world_up);
	void MultMatrix(const matrix& a, const matrix& b);
	int IsIdentity();
	void VECTOR_TO_VIEW(vector3d& base, vector3d& point, vector3d& up, vector3d& right);
	void VIEW_MATRIX(vector3d& base, vector3d& point, int roll);
	void VECTOR_TO_REV_VIEW(vector3d& base, vector3d& point, vector3d& up, vector3d& right);
	void REV_VIEW_MATRIX(vector3d& base, vector3d& point, int roll);
	void VECTOR_TO_MATRIX(vector3d& view_dir);
	void NORMALIZE_SCALE(matrix& sour);
	void REVERSE_MATRIX(matrix& sour);
	void UPVECTOR_TO_MATRIX(vector3d up);
	float v11;
	float v12;
	float v13;
	float v21;
	float v22;
	float v23;
	float v31;
	float v32;
	float v33;
	float v41;
	float v42;
	float v43;
};

extern matrix _IdentityMatrix;
inline void matrix::MakeIdentity() { *this = _IdentityMatrix; }
inline void matrix::SetRow1(float x, float y, float z) { v11 = x; v12 = y; v13 = z; }
inline void matrix::SetRow2(float x, float y, float z) { v21 = x; v22 = y; v23 = z; }
inline void matrix::SetRow3(float x, float y, float z) { v31 = x; v32 = y; v33 = z; }
inline void matrix::SetRow4(float x, float y, float z) { v41 = x; v42 = y; v43 = z; }

inline void vector3d::MatrixMult(const vector3d& other, const matrix& b)
{
	x = other.x*b.v11 + other.y*b.v21 + other.z*b.v31 + b.v41;
	y = other.x*b.v12 + other.y*b.v22 + other.z*b.v32 + b.v42;
	z = other.x*b.v13 + other.y*b.v23 + other.z*b.v33 + b.v43;
}

inline void plane3d::MatrixMult(const matrix& m, const plane3d& p)
{
	x = m.v11*p.x + m.v13*p.z + m.v12*p.y;
	y = m.v23*p.z + m.v22*p.y + m.v21*p.x;
	z = m.v33*p.z + m.v32*p.y + m.v31*p.x;
	w = m.v43*p.z + m.v42*p.y + m.v41*p.x + p.w;
}

struct w3d_quaternion {
	w3d_quaternion() {}
	w3d_quaternion(float w, float x, float y, float z) : qw(w), qx(x), qy(y), qz(z) {}

	w3d_quaternion operator + (w3d_quaternion& q);
	w3d_quaternion operator * (float& c);

	float qw, qx, qy, qz;
};
VOID ROTATION_TO_QUATERNION(w3d_quaternion &q, vector3d &v, float &fTheta);
VOID QUATERNION_TO_ROTATION(vector3d &v, FLOAT &fTheta, w3d_quaternion &q);
VOID ANGLES_TO_QUATERNION(w3d_quaternion &q, FLOAT fYaw, FLOAT fPitch, FLOAT fRoll);
VOID QUATERNION_TO_MATRIX(matrix &mat, w3d_quaternion &trans);
VOID MATRIX_TO_QUATERNION(w3d_quaternion &trans, matrix &mat);
DOUBLE QUATERNION_DOT_PRODUCT(w3d_quaternion &p, w3d_quaternion &q);
VOID QUATERNION_MULTIPLY(w3d_quaternion &q, w3d_quaternion &a, w3d_quaternion &b);
w3d_quaternion INTERPOLATE_QUATERNION(DOUBLE t, w3d_quaternion &p, w3d_quaternion &q);

inline w3d_quaternion w3d_quaternion :: operator + (w3d_quaternion &q)
{
	return w3d_quaternion (qw + q.qw, qx + q.qx, qy + q.qy, qz + q.qz);
}

inline w3d_quaternion w3d_quaternion :: operator * (float &c)
{
	return w3d_quaternion(qw * c, qx * c, qy * c, qz * c);
}

struct COLOR {
	union {
		struct {
			BYTE b, g, r, a;
		};
		DWORD color;
	};
};

struct tlvertex3d {
public:
	tlvertex3d() { specular = 0xFF000000; }
	tlvertex3d(float nx, float ny, float nz, float noow,
			   unsigned long ncolor, unsigned long nspecular,
			   float ntu, float ntv)
	{
		x = nx;
		y = ny;
		z = nz;
		oow = noow;
		color = ncolor;
		specular = nspecular;
		tu = ntu;
		tv = ntv;
	}
	float x;
	float y;
	float z;
	float oow;
	union {
		unsigned long color;
		COLOR argb;
	};
	unsigned long specular;
	union {
		texCoor coord;
		struct {
			float tu;
			float tv;
		};
	};
};

struct lmtlvertex3d {
public:
	tlvertex3d vert;
	texCoor lmcoord;
};

struct C3dAABB {
public:
	vector3d min;
	vector3d max;
};

struct C3dOBB {
public:
	vector3d halfSize;
	vector3d center;
	vector3d u;
	vector3d v;
	vector3d w;
	vector3d vertices[8];
};
extern	float	g_radTable[];
extern float g_sinTable[];
extern float g_cosTable[];
void CreateTrigonometricTable();
float GetRadian(int degree);
float GetSin(int degree);
float GetCos(int degree);

struct ray3d {
public:
	vector3d org;
	vector3d dir;
};

struct lineSegment3d {
public:
	vector3d org;
	vector3d dir;
	float limit;
};

struct GroundAttrPosInfo {
	GroundAttrPosInfo() : x(0), y(0), isRendered(true), cellColor(0xaa55ffff) {}
	int x, y;
	BOOL isRendered;
	DWORD cellColor;
};

BOOL CheckRayOBBIntersect(const ray3d& ray, const C3dOBB& obb);
BOOL CheckRayTriIntersect(const ray3d& ray, const vector3d& v0, const vector3d& v1, const vector3d& v2, float* u, float* v, float* t);
BOOL CheckLineSegmentOBBIntersect(const lineSegment3d& line, const C3dOBB& obb);
BOOL CheckLineSegmentTriIntersect(const lineSegment3d& line, const vector3d& v0, const vector3d& v1, const vector3d& v2, float* u, float* v, float* t);
float SquaredVertRayDist(const ray3d& ray, const vector3d& Vert);


#endif
