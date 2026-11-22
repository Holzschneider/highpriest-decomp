#include "common.h"
#include "Prim.h"
#include "Anim.h"

#include <windows.h>
#include <math.h>






inline float fmax(float a, float b) { return a > b ? a : b; }



float g_radTable[722];
float g_sinTable[722];
float g_cosTable[722];


matrix _IdentityMatrix = {
	1.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 1.0f,
	0.0f, 0.0f, 0.0f
};






float	_sqrt(float input) {
	_asm {
		fld		input
		fsqrt
		fstp	input
	}
	return input;
}

float	_sin(float input) {
	_asm {
		fld		input
		fsin
		fstp	input
	}
	return input;
}

float	_cos(float input) {
	_asm {
		fld		input
		fcos
		fstp	input
	}
	return input;
}

void	matrix :: MakeTranslate(float x, float y, float z)
{
	*this = _IdentityMatrix;
	v41 = x;
	v42 = y;
	v43 = z;
}

void	matrix :: MakeXRotation(float angle)
{
	*this = _IdentityMatrix;
	float Sin = _sin(angle), Cos = _cos(angle);
	v22 = Cos;
	v33 = Cos;
	v23 = Sin;
	v32 = -Sin;
}

void	matrix :: MakeYRotation(float angle)
{
	*this = _IdentityMatrix;
	float Sin = _sin(angle), Cos = _cos(angle);
	v11 = Cos;
	v33 = Cos;
	v13 = -Sin;
	v31 = Sin;
}

void	matrix :: MakeZRotation(float angle)
{
	*this = _IdentityMatrix;
	float Sin = _sin(angle), Cos = _cos(angle);
	v11 = Cos;
	v22 = Cos;
	v21 = -Sin;
	v12 = Sin;
}

void	matrix :: MakeScale(float x, float y, float z)
{
	*this = _IdentityMatrix;
	v11 = x;
	v22 = y;
	v33 = z;
}

void	matrix :: AppendTranslate(const vector3d &v)
{
	matrix	temp, TM;

	TM.MakeTranslate(v.x, v.y, v.z);
	temp.MultMatrix(*this, TM);
	*this = temp;
}

void	matrix :: AppendXRotation(float angle)
{
	matrix	temp, TM;

	TM.MakeXRotation(angle);
	temp.MultMatrix(*this, TM);
	*this = temp;
}

void	matrix :: AppendYRotation(float angle)
{
	matrix	temp, TM;

	TM.MakeYRotation(angle);
	temp.MultMatrix(*this, TM);
	*this = temp;
}

void	matrix :: AppendZRotation(float angle)
{
	matrix	temp, TM;

	TM.MakeZRotation(angle);
	temp.MultMatrix(*this, TM);
	*this = temp;
}

void	matrix :: AppendScale(float x, float y, float z)
{
	matrix	temp, TM;

	TM.MakeScale(x, y, z);
	temp.MultMatrix(*this, TM);
	*this = temp;
}

void	matrix :: PrependTranslate(const vector3d &v)
{
	matrix	temp, TM;

	TM.MakeTranslate(v.x, v.y, v.z);
	temp.MultMatrix(TM, *this);
	*this = temp;
}

void	matrix :: PrependScale(float x, float y, float z)
{
	matrix	temp, TM;

	TM.MakeScale(x, y, z);
	temp.MultMatrix(TM, *this);
	*this = temp;
}

void	matrix :: MakeTransform(const vector3d &pos, float *const rotaxis, float rotangle)
{
	float	omega = rotangle / 2.0f;
	float	s = _sin(omega);
	float	qx = rotaxis[0]*s;
	float	qy = rotaxis[1]*s;
	float	qz = rotaxis[2]*s;
	float	qw = _cos(omega);

	float	x2, y2, z2, wx, wy, wz, xx, xy, xz, yy, yz, zz;
	x2 = qx + qx;	y2 = qy + qy;	z2 = qz + qz;
	wx = qw * x2;   wy = qw * y2;   wz = qw * z2;
	xx = qx * x2;   xy = qx * y2;   xz = qx * z2;
	yy = qy * y2;   yz = qy * z2;   zz = qz * z2;

	v11 = 1.0f-(yy+zz);	v12 = xy+wz;		v13 = xz-wy;
	v21 = xy-wz;		v22 = 1.0f-(xx+zz);	v23 = yz+wx;
	v31 = xz+wy;		v32 = yz-wx;		v33 = 1.0f-(xx+yy);
	v41 = pos.x;		v42 = pos.y;		v43 = pos.z;
}

void	matrix :: MakeTransform(float *const rotaxis, float rotangle)
{
	float	omega = rotangle / 2.0f;
	float	s = _sin(omega);
	float	qx = rotaxis[0]*s;
	float	qy = rotaxis[1]*s;
	float	qz = rotaxis[2]*s;
	float	qw = _cos(omega);

	float	x2, y2, z2, wx, wy, wz, xx, xy, xz, yy, yz, zz;
	x2 = qx + qx;	y2 = qy + qy;	z2 = qz + qz;
	wx = qw * x2;   wy = qw * y2;   wz = qw * z2;
	xx = qx * x2;   xy = qx * y2;   xz = qx * z2;
	yy = qy * y2;   yz = qy * z2;   zz = qz * z2;

	v11 = 1.0f-(yy+zz);	v12 = xy+wz;		v13 = xz-wy;
	v21 = xy-wz;		v22 = 1.0f-(xx+zz);	v23 = yz+wx;
	v31 = xz+wy;		v32 = yz-wx;		v33 = 1.0f-(xx+yy);
	v41 = 0;			v42 = 0;			v43 = 0;
}

void	matrix :: MakeTransform(rotKeyframe &ani)
{
	float	x2, y2, z2, wx, wy, wz, xx, xy, xz, yy, yz, zz;
	x2 = ani.qx + ani.qx;	y2 = ani.qy + ani.qy;	z2 = ani.qz + ani.qz;
	wx = ani.qw * x2;		wy = ani.qw * y2;		wz = ani.qw * z2;
	xx = ani.qx * x2;		xy = ani.qx * y2;		xz = ani.qx * z2;
	yy = ani.qy * y2;		yz = ani.qy * z2;		zz = ani.qz * z2;

	v11 = 1.0f-(yy+zz);	v12 = xy+wz;		v13 = xz-wy;
	v21 = xy-wz;		v22 = 1.0f-(xx+zz);	v23 = yz+wx;
	v31 = xz+wy;		v32 = yz-wx;		v33 = 1.0f-(xx+yy);
	v41 = 0;			v42 = 0;			v43 = 0;
}

void	matrix :: MakeView(const vector3d &from, const vector3d &at, const vector3d &world_up)
{
	MakeIdentity();

	vector3d	up, right, view_dir;
	view_dir = at - from;
	view_dir.Normalize();
	right.CrossProduct(world_up, view_dir);
	right.Normalize();
	up.CrossProduct(view_dir, right);
	up.Normalize();

	v11 = right.x;
	v21 = right.y;
	v31 = right.z;
	v12 = up.x;
	v22 = up.y;
	v32 = up.z;
	v13 = view_dir.x;
	v23 = view_dir.y;
	v33 = view_dir.z;
	v41 = -right.DotProduct(from);
	v42 = -up.DotProduct(from);
	v43 = -view_dir.DotProduct(from);
}

void	matrix :: MultMatrix(const matrix &a, const matrix &b)
{
	v11 = a.v11*b.v11 + a.v12*b.v21 + a.v13*b.v31;
	v21 = a.v21*b.v11 + a.v22*b.v21 + a.v23*b.v31;
	v31 = a.v31*b.v11 + a.v32*b.v21 + a.v33*b.v31;
	v41 = a.v41*b.v11 + a.v42*b.v21 + a.v43*b.v31 + b.v41;
	v12 = a.v11*b.v12 + a.v12*b.v22 + a.v13*b.v32;
	v22 = a.v21*b.v12 + a.v22*b.v22 + a.v23*b.v32;
	v32 = a.v31*b.v12 + a.v32*b.v22 + a.v33*b.v32;
	v42 = a.v41*b.v12 + a.v42*b.v22 + a.v43*b.v32 + b.v42;
	v13 = a.v11*b.v13 + a.v12*b.v23 + a.v13*b.v33;
	v23 = a.v21*b.v13 + a.v22*b.v23 + a.v23*b.v33;
	v33 = a.v31*b.v13 + a.v32*b.v23 + a.v33*b.v33;
	v43 = a.v41*b.v13 + a.v42*b.v23 + a.v43*b.v33 + b.v43;
}

void	matrix :: MakeInverse(const matrix &src)
{
	float fDetInv = 1.0f / (src.v11 * (src.v22 * src.v33 - src.v23 * src.v32) -
							 src.v12 * (src.v21 * src.v33 - src.v23 * src.v31) +
							 src.v13 * (src.v21 * src.v32 - src.v22 * src.v31));

	v11 =  fDetInv * (src.v22 * src.v33 - src.v23 * src.v32);
	v12 = -fDetInv * (src.v12 * src.v33 - src.v13 * src.v32);
	v13 =  fDetInv * (src.v12 * src.v23 - src.v13 * src.v22);

	v21 = -fDetInv * (src.v21 * src.v33 - src.v23 * src.v31);
	v22 =  fDetInv * (src.v11 * src.v33 - src.v13 * src.v31);
	v23 = -fDetInv * (src.v11 * src.v23 - src.v13 * src.v21);

	v31 =  fDetInv * (src.v21 * src.v32 - src.v22 * src.v31);
	v32 = -fDetInv * (src.v11 * src.v32 - src.v12 * src.v31);
	v33 =  fDetInv * (src.v11 * src.v22 - src.v12 * src.v21);

	v41 = -(src.v41 * v11 + src.v42 * v21 + src.v43 * v31);
	v42 = -(src.v41 * v12 + src.v42 * v22 + src.v43 * v32);
	v43 = -(src.v41 * v13 + src.v42 * v23 + src.v43 * v33);
}

BOOL	matrix :: IsIdentity()
{
	if (abs(v11-1.0f) > epsilon)
		return	FALSE;
	if (abs(v22-1.0f) > epsilon)
		return	FALSE;
	if (abs(v33-1.0f) > epsilon)
		return	FALSE;
	if (abs(v12) > epsilon)
		return	FALSE;
	if (abs(v13) > epsilon)
		return	FALSE;
	if (abs(v21) > epsilon)
		return	FALSE;
	if (abs(v23) > epsilon)
		return	FALSE;
	if (abs(v31) > epsilon)
		return	FALSE;
	if (abs(v32) > epsilon)
		return	FALSE;
	if (abs(v41) > epsilon)
		return	FALSE;
	if (abs(v42) > epsilon)
		return	FALSE;
	if (abs(v43) > epsilon)
		return	FALSE;
	return	TRUE;
}

VOID matrix :: VECTOR_TO_VIEW(vector3d &base, vector3d &point, vector3d &up, vector3d &right)
{
	v11 = right.x;
	v21 = right.y;
	v31 = right.z;

	v12 = up.x;
	v22 = up.y;
	v32 = up.z;

	v13 = point.x;
	v23 = point.y;
	v33 = point.z;

	v41 = right.DotProduct(base);
	v42 = up.DotProduct(base);
	v43 = point.DotProduct(base);

	v41 = -v41;
	v42 = -v42;
	v43 = -v43;
}

static	vector3d g_WorldUp(0.0f, 1.0f, 0.0f);
VOID matrix :: VIEW_MATRIX(vector3d &base, vector3d &point, INT roll)
{
	vector3d up, right, view_dir;

	view_dir = point - base;
	view_dir.Normalize();

	if (view_dir.x == 0.0f && view_dir.z == 0.0f)
		right.Set(1.0f, 0.0f, 0.0f);
	else
		right.CrossProduct(g_WorldUp, view_dir);

	up.CrossProduct(view_dir, right);
	right.Normalize();
	up.Normalize();

	VECTOR_TO_VIEW(base, view_dir, up, right);

	AppendZRotation((float)roll);
}

VOID matrix :: VECTOR_TO_REV_VIEW(vector3d &base, vector3d &point, vector3d &up, vector3d &right)
{
	v11 = right.x;
	v12 = right.y;
	v13 = right.z;

	v21 = up.x;
	v22 = up.y;
	v23 = up.z;

	v31 = point.x;
	v32 = point.y;
	v33 = point.z;

	v41 = base.x;
	v42 = base.y;
	v43 = base.z;
}

VOID matrix :: REV_VIEW_MATRIX(vector3d &base, vector3d &point, INT roll)
{
	vector3d up, right, view_dir;

	view_dir = point - base;
	view_dir.Normalize();

	if (view_dir.x == 0.0f && view_dir.z == 0.0f)
		right.Set(1.0f, 0.0f, 0.0f);
	else
		right.CrossProduct(g_WorldUp, view_dir);

	right.Normalize();
	up.CrossProduct(view_dir, right);
	up.Normalize();

















	VECTOR_TO_REV_VIEW(base, view_dir, up, right);

	AppendZRotation((float)roll);
}

VOID matrix :: VECTOR_TO_MATRIX(vector3d &view_dir)
{
	vector3d up, right;

	if (view_dir.x == 0.0f && view_dir.z == 0.0f)
		right.Set(1.0f, 0.0f, 0.0f);
	else
		right.CrossProduct(g_WorldUp, view_dir);

	up.CrossProduct(view_dir, right);
	right.Normalize();
	up.Normalize();

	v11 = right.x;
	v21 = right.y;
	v31 = right.z;
	v41 = 0.0f;

	v12 = up.x;
	v22 = up.y;
	v32 = up.z;
	v42 = 0.0f;

	v13 = view_dir.x;
	v23 = view_dir.y;
	v33 = view_dir.z;
	v43 = 0.0f;
}

void vector3d :: MATRIX_TO_VECTOR(matrix &dir)
{
	x = dir.v13;
	y = dir.v23;
	z = dir.v33;
}

VOID matrix :: NORMALIZE_SCALE(matrix &sour)
{
	float rxscale, ryscale, rzscale, scale;

	scale = _sqrt(sour.v11*sour.v11 + sour.v12*sour.v12 + sour.v13*sour.v13);
	rxscale = scale;
	scale = _sqrt(sour.v21*sour.v21 + sour.v22*sour.v22 + sour.v23*sour.v23);
	ryscale = scale;
	scale = _sqrt(sour.v31*sour.v31 + sour.v32*sour.v32 + sour.v33*sour.v33);
	rzscale = scale;

	v11 = sour.v11 / rxscale;
	v12 = sour.v12 / ryscale;
	v13 = sour.v13 / rzscale;

	v21 = sour.v21 / rxscale;
	v22 = sour.v22 / ryscale;
	v23 = sour.v23 / rzscale;

	v31 = sour.v31 / rxscale;
	v32 = sour.v32 / ryscale;
	v33 = sour.v33 / rzscale;

	v41 = sour.v41 / rxscale;
	v42 = sour.v42 / ryscale;
	v43 = sour.v43 / rzscale;
}

VOID matrix :: REVERSE_MATRIX(matrix &sour)
{
	float scalex, scaley, scalez;

	scalex = 1.0f / (sour.v11*sour.v11 + sour.v12*sour.v12 + sour.v13*sour.v13);
	scaley = 1.0f / (sour.v21*sour.v21 + sour.v22*sour.v22 + sour.v23*sour.v23);
	scalez = 1.0f / (sour.v31*sour.v31 + sour.v32*sour.v32 + sour.v33*sour.v33);

	v11 = sour.v11;
	v21 = sour.v12;
	v31 = sour.v13;
	v12 = sour.v21;
	v22 = sour.v22;
	v32 = sour.v23;
	v13 = sour.v31;
	v23 = sour.v32;
	v33 = sour.v33;

	v41 = -(v11*sour.v41 + v21*sour.v42 + v31*sour.v43);
	v42 = -(v12*sour.v41 + v22*sour.v42 + v32*sour.v43);
	v43 = -(v13*sour.v41 + v23*sour.v42 + v33*sour.v43);

	v11 *= scalex;
	v12 *= scaley;
	v13 *= scalez;
	v21 *= scalex;
	v22 *= scaley;
	v23 *= scalez;
	v31 *= scalex;
	v32 *= scaley;
	v33 *= scalez;
	v41 *= scalex;
	v42 *= scaley;
	v43 *= scalez;
}

VOID matrix :: UPVECTOR_TO_MATRIX(vector3d up)
{
	vector3d right, view_dir, base;

	up.Normalize();

	view_dir.Set(0, 0, 1.0f);

	right.CrossProduct(up, view_dir);
	right.Normalize();

	view_dir.CrossProduct(right, up);
	view_dir.Normalize();

	base.Set(0.0f, 0.0f, 0.0f);

	VECTOR_TO_REV_VIEW(base, view_dir, up, right);
}

VOID ROTATION_TO_QUATERNION(w3d_quaternion &q, vector3d &v, FLOAT &fTheta)
{
	if (fTheta == 0.0f)
	{
		q.qx = 0.0f;
		q.qy = 0.0f;
		q.qz = 0.0f;
		q.qw = 1.0f;
	}
	else
	{
		q.qx = _sin( fTheta * 0.5f ) * v.x;
		q.qy = _sin( fTheta * 0.5f ) * v.y;
		q.qz = _sin( fTheta * 0.5f ) * v.z;
		q.qw = _cos( fTheta * 0.5f );
	}
}

VOID QUATERNION_TO_ROTATION(vector3d &v, FLOAT &fTheta, w3d_quaternion &q)
{
	fTheta = acosf(q.qw) * 2.0f;
	if (fTheta == 0.0f)
	{
		v.x    = 0.0f;
		v.y    = 0.0f;
		v.z    = 0.0f;
	}
	else
	{
		v.x    = q.qx / _sin( fTheta * 0.5f );
		v.y    = q.qy / _sin( fTheta * 0.5f );
		v.z    = q.qz / _sin( fTheta * 0.5f );
	}
}

VOID ANGLES_TO_QUATERNION(w3d_quaternion &q, FLOAT fYaw, FLOAT fPitch, FLOAT fRoll )
{
	FLOAT fSinYaw   = _sin( fYaw * 0.5f );
	FLOAT fSinPitch = _sin( fPitch * 0.5f );
	FLOAT fSinRoll  = _sin( fRoll * 0.5f );
	FLOAT fCosYaw   = _cos( fYaw * 0.5f );
	FLOAT fCosPitch = _cos( fPitch * 0.5f );
	FLOAT fCosRoll  = _cos( fRoll * 0.5f );

	q.qx = fSinRoll * fCosPitch * fCosYaw - fCosRoll * fSinPitch * fSinYaw;
	q.qy = fCosRoll * fSinPitch * fCosYaw + fSinRoll * fCosPitch * fSinYaw;
	q.qz = fCosRoll * fCosPitch * fSinYaw - fSinRoll * fSinPitch * fCosYaw;
	q.qw = fCosRoll * fCosPitch * fCosYaw + fSinRoll * fSinPitch * fSinYaw;
}

VOID QUATERNION_TO_MATRIX(matrix &mat, w3d_quaternion &trans)
{
	FLOAT xx = trans.qx*trans.qx; FLOAT yy = trans.qy*trans.qy; FLOAT zz = trans.qz*trans.qz;
	FLOAT xy = trans.qx*trans.qy; FLOAT xz = trans.qx*trans.qz; FLOAT yz = trans.qy*trans.qz;
	FLOAT wx = trans.qw*trans.qx; FLOAT wy = trans.qw*trans.qy; FLOAT wz = trans.qw*trans.qz;

	mat.v11 = 1.0f - 2.0f * ( yy + zz );
	mat.v12 =     2.0f * ( xy - wz );
	mat.v13 =     2.0f * ( xz + wy );

	mat.v21 =     2.0f * ( xy + wz );
	mat.v22 = 1.0f - 2.0f * ( xx + zz );
	mat.v23 =     2.0f * ( yz - wx );

	mat.v31 =     2.0f * ( xz - wy );
	mat.v32 =     2.0f * ( yz + wx );
	mat.v33 = 1.0f - 2.0f * ( xx + yy );

	mat.v41 = mat.v42 = mat.v43 = 0.0f;
}

VOID MATRIX_TO_QUATERNION(w3d_quaternion &trans, matrix &mat )
{
	FLOAT diag1 = + mat.v11 - mat.v22 - mat.v33 + 1.0f;
	FLOAT diag2 = - mat.v11 + mat.v22 - mat.v33 + 1.0f;
	FLOAT diag3 = - mat.v11 - mat.v22 + mat.v33 + 1.0f;
	FLOAT diag4 = + mat.v11 + mat.v22 + mat.v33 + 1.0f;

	FLOAT max = fmax( diag1, fmax( diag2, fmax( diag3, diag4 ) ) );
	FLOAT d   = 2.0f * _sqrt( max );

	if( diag1 == max )
	{
		trans.qx = max / d;
		trans.qy = ( mat.v21 + mat.v12 ) / d;
		trans.qz = ( mat.v13 + mat.v31 ) / d;
		trans.qw = ( mat.v32 - mat.v23 ) / d;
	}
	else if ( diag2 == max )
	{
		trans.qx = ( mat.v21 + mat.v12 ) / d;
		trans.qy = max / d;
		trans.qz = ( mat.v32 + mat.v23 ) / d;
		trans.qw = ( mat.v13 - mat.v31 ) / d;
	}
	else if( diag3 == max )
	{
		trans.qx = ( mat.v13 + mat.v31 ) / d;
		trans.qy = ( mat.v32 + mat.v23 ) / d;
		trans.qz = max / d;
		trans.qw = ( mat.v21 - mat.v12 ) / d;
	}
	else if( diag4 == max )
	{
		trans.qx = ( mat.v32 - mat.v23 ) / d;
		trans.qy = ( mat.v13 - mat.v31 ) / d;
		trans.qz = ( mat.v21 - mat.v12 ) / d;
		trans.qw = max / d;
	}

	if( trans.qw < 0.0f )
	{
		trans.qx = -trans.qx;
		trans.qy = -trans.qy;
		trans.qz = -trans.qz;
		trans.qw = -trans.qw;
	}
}

DOUBLE QUATERNION_DOT_PRODUCT(w3d_quaternion &p, w3d_quaternion &q)
{
	return (p.qw*q.qw + p.qx*q.qx + p.qy*q.qy + p.qz*q.qz);
}

VOID QUATERNION_MULTIPLY(w3d_quaternion &q, w3d_quaternion &a, w3d_quaternion &b)
{
	q.qx =  a.qx*b.qw + a.qy*b.qz - a.qz*b.qy + a.qw*b.qx;
	q.qy = -a.qx*b.qz + a.qy*b.qw + a.qz*b.qx + a.qw*b.qy;
	q.qz =  a.qx*b.qy - a.qy*b.qx + a.qz*b.qw + a.qw*b.qz;
	q.qw = -a.qx*b.qx - a.qy*b.qy - a.qz*b.qz + a.qw*b.qw;
}

w3d_quaternion INTERPOLATE_QUATERNION(DOUBLE t, w3d_quaternion &p, w3d_quaternion &q)
{
	DOUBLE cosom, sinom, omega;
	FLOAT scale0, scale1;
	w3d_quaternion to;

	cosom = QUATERNION_DOT_PRODUCT(p, q);

	if (cosom < 0.0)
	{
		cosom = -cosom;
		to.qx = -q.qx;
		to.qy = -q.qy;
		to.qz = -q.qz;
		to.qw = -q.qw;
	}
	else
		to = q;

	if ((1.0 - cosom) > epsilon)
	{
		omega = acos(cosom);
		sinom = _sin((FLOAT)omega);
		scale0 = _sin((FLOAT)((1.0 - t) * omega)) / sinom;
		scale1 = _sin((FLOAT)(t * omega)) / sinom;
	}
	else
	{
		scale0 = 1.0 - t;
		scale1 = t;
	}

	return (p*scale0 + to*scale1);
}

void	CreateTrigonometricTable()
{
	for (int i = 0; i <= 360; i++) {
		g_radTable[i] = DegToRad(i);
		g_sinTable[i] = _sin(g_radTable[i]);
		g_cosTable[i] = _cos(g_radTable[i]);

		g_radTable[i + 361] = -g_radTable[i];
		g_sinTable[i + 361] = -g_sinTable[i];
		g_cosTable[i + 361] = -g_cosTable[i];
	}
}

float	GetRadian(int degree)
{
	if (degree > 360 || degree < -360)
		degree %= 360;

	if (degree < 0)
		return g_radTable[degree + 361];

	return g_radTable[degree];
}

float	GetSin(int degree)
{
	if (degree > 360 || degree < -360)
		degree %= 360;

	if (degree < 0)
		return g_sinTable[degree + 361];

	return g_sinTable[degree];
}

float	GetCos(int degree)
{
	if (degree > 360 || degree < -360)
		degree %= 360;

	if (degree < 0)
		return g_cosTable[degree + 361];

	return g_cosTable[degree];
}

BOOL	CheckRayOBBIntersect(const ray3d &ray, const C3dOBB &obb)
{
	float	tMin = -infinity;
	float	tMax = infinity;

	vector3d	p = obb.center-ray.org;

	float	e, f;

	e = p.DotProduct(obb.u);
	f = ray.dir.DotProduct(obb.u);
	if (fabs(f) > epsilon) {
		float	t1, t2;
		float	oof = 1/f;

		t1 = (e+obb.halfSize.x)*oof;
		t2 = (e-obb.halfSize.x)*oof;
		if (t1>t2) {
			if (t2 > tMin) tMin = t2;
			if (t1 < tMax) tMax = t1;
		} else {
			if (t1 > tMin) tMin = t1;
			if (t2 < tMax) tMax = t2;
		}
		if (tMin > tMax) return	FALSE;
		if (tMax < 0) return FALSE;
	} else if (-e-obb.halfSize.x > 0 || -e+obb.halfSize.x < 0) return FALSE;

	e = p.DotProduct(obb.v);
	f = ray.dir.DotProduct(obb.v);
	if (fabs(f) > epsilon) {
		float	t1, t2;
		float	oof = 1/f;

		t1 = (e+obb.halfSize.y)*oof;
		t2 = (e-obb.halfSize.y)*oof;
		if (t1>t2) {
			if (t2 > tMin) tMin = t2;
			if (t1 < tMax) tMax = t1;
		} else {
			if (t1 > tMin) tMin = t1;
			if (t2 < tMax) tMax = t2;
		}
		if (tMin > tMax) return	FALSE;
		if (tMax < 0) return FALSE;
	} else if (-e-obb.halfSize.y > 0 || -e+obb.halfSize.y < 0) return FALSE;

	e = p.DotProduct(obb.w);
	f = ray.dir.DotProduct(obb.w);
	if (fabs(f) > epsilon) {
		float	t1, t2;
		float	oof = 1/f;

		t1 = (e+obb.halfSize.z)*oof;
		t2 = (e-obb.halfSize.z)*oof;
		if (t1>t2) {
			if (t2 > tMin) tMin = t2;
			if (t1 < tMax) tMax = t1;
		} else {
			if (t1 > tMin) tMin = t1;
			if (t2 < tMax) tMax = t2;
		}
		if (tMin > tMax) return	FALSE;
		if (tMax < 0) return FALSE;
	} else if (-e-obb.halfSize.z > 0 || -e+obb.halfSize.z < 0) return FALSE;
	return	TRUE;
}

BOOL	CheckRayTriIntersect(const ray3d &ray, const vector3d &v0, const vector3d &v1, const vector3d &v2, float *u, float *v, float *t)
{
	vector3d	e1 = v1 - v0;
	vector3d	e2 = v2 - v0;
	vector3d	p;

	p.CrossProduct(ray.dir, e2);
	float	det = e1.DotProduct(p);
	if (det>-epsilon && det < epsilon)
		return	FALSE;
	float	inv_det = 1.0f/det;
	vector3d	s = ray.org - v0;
	*u = s.DotProduct(p) * inv_det;
	if (*u < 0.0f || *u > 1.0f)
		return	FALSE;
	vector3d	q;
	q.CrossProduct(s, e1);
	*v = ray.dir.DotProduct(q)*inv_det;
	if (*v < 0.0f || *u + *v > 1.0f)
		return	FALSE;
	*t = e2.DotProduct(q)*inv_det;
	return	TRUE;
}

BOOL	CheckLineSegmentOBBIntersect(const lineSegment3d &ray, const C3dOBB &obb)
{
	float	tMin = -infinity;
	float	tMax = infinity;

	vector3d	p = obb.center-ray.org;

	float	e, f;

	e = p.DotProduct(obb.u);
	f = ray.dir.DotProduct(obb.u);
	if (fabs(f) > epsilon) {
		float	t1, t2;
		float	oof = 1/f;

		t1 = (e+obb.halfSize.x)*oof;
		t2 = (e-obb.halfSize.x)*oof;
		if (t1>t2) {
			if (t2 > tMin) tMin = t2;
			if (t1 < tMax) tMax = t1;
		} else {
			if (t1 > tMin) tMin = t1;
			if (t2 < tMax) tMax = t2;
		}
		if (tMin > tMax) return	FALSE;
		if (tMax < 0) return FALSE;
	} else if (-e-obb.halfSize.x > 0 || -e+obb.halfSize.x < 0) return FALSE;

	e = p.DotProduct(obb.v);
	f = ray.dir.DotProduct(obb.v);
	if (fabs(f) > epsilon) {
		float	t1, t2;
		float	oof = 1/f;

		t1 = (e+obb.halfSize.y)*oof;
		t2 = (e-obb.halfSize.y)*oof;
		if (t1>t2) {
			if (t2 > tMin) tMin = t2;
			if (t1 < tMax) tMax = t1;
		} else {
			if (t1 > tMin) tMin = t1;
			if (t2 < tMax) tMax = t2;
		}
		if (tMin > tMax) return	FALSE;
		if (tMax < 0) return FALSE;
	} else if (-e-obb.halfSize.y > 0 || -e+obb.halfSize.y < 0) return FALSE;

	e = p.DotProduct(obb.w);
	f = ray.dir.DotProduct(obb.w);
	if (fabs(f) > epsilon) {
		float	t1, t2;
		float	oof = 1/f;

		t1 = (e+obb.halfSize.z)*oof;
		t2 = (e-obb.halfSize.z)*oof;
		if (t1>t2) {
			if (t2 > tMin) tMin = t2;
			if (t1 < tMax) tMax = t1;
		} else {
			if (t1 > tMin) tMin = t1;
			if (t2 < tMax) tMax = t2;
		}
		if (tMin > tMax) return	FALSE;
		if (tMax < 0) return FALSE;
	} else if (-e-obb.halfSize.z > 0 || -e+obb.halfSize.z < 0) return FALSE;

	if (tMin > 0) {
		if (tMin > ray.limit)
			return	FALSE;
	} else {
		if (tMax > ray.limit)
			return	FALSE;
	}
	return	TRUE;
}

BOOL	CheckLineSegmentTriIntersect(const lineSegment3d &ray, const vector3d &v0, const vector3d &v1, const vector3d &v2, float *u, float *v, float *t)
{
	vector3d	e1 = v1 - v0;
	vector3d	e2 = v2 - v0;
	vector3d	p;

	p.CrossProduct(ray.dir, e2);
	float	det = e1.DotProduct(p);
	if (det>-epsilon && det < epsilon)
		return	FALSE;
	float	inv_det = 1.0f/det;
	vector3d	s = ray.org - v0;
	*u = s.DotProduct(p) * inv_det;
	if (*u < 0.0f || *u > 1.0f)
		return	FALSE;
	vector3d	q;
	q.CrossProduct(s, e1);
	*v = ray.dir.DotProduct(q)*inv_det;
	if (*v < 0.0f || *u + *v > 1.0f)
		return	FALSE;
	*t = e2.DotProduct(q)*inv_det;
	if (*t > ray.limit)
		return	FALSE;
	if (*t < 0)
		return	FALSE;
	return	TRUE;
}

float	SquaredVertRayDist(const ray3d &ray, const vector3d &Vert)
{
	vector3d	diff = Vert - ray.org;
	float		t = ray.dir.DotProduct(diff);
	float		dotMM = ray.dir.DotProduct(ray.dir);
	vector3d	tdir(ray.dir.x * t, ray.dir.y * t, ray.dir.z * t);
	t = t/dotMM;
	diff = diff - tdir;
	return	diff.DotProduct(diff);
}
