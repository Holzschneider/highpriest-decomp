#include "common.h"
#include "Skybox.h"
#include "../RagEffect.h"
#include "../RagEffectPrim.h"
#include "../3dActor.h"
#include "../base/Res.h"
#include "Model.h"
#include "Ground.h"
#include "../3dlib/Device.h"


extern CRenderer* g_renderer;

#ifndef D3DBLEND_ZERO
#define D3DBLEND_ZERO        1
#define D3DBLEND_ONE         2
#define D3DBLEND_SRCALPHA    5
#define D3DBLEND_INVSRCALPHA 6
#endif


#ifndef NUM_FLARES
#define NUM_FLARES 12
#endif






void  CFlare :: Init( void )
{


  m_Flare[0]  = SetFlare( 2,  20.20f*9, 0.45f, 0.0f, 0.0f, 1.0f );
  m_Flare[1]  = SetFlare( 1,  10.90f*9, 0.30f, 0.0f, 1.0f, 0.0f );
  m_Flare[2]  = SetFlare( 1,  10.60f*9, 0.67f, 1.0f, 0.0f, 0.0f );
  m_Flare[3]  = SetFlare(  1, 10.30f*9, 0.06f, 0.6f, 0.0f, 0.0f );
  m_Flare[4]  = SetFlare(  3, 10.00f*9, 0.15f, 0.4f, 0.0f, 0.0f );
  m_Flare[5]  = SetFlare(  1,  0.50f*9, 0.70f, 0.3f, 0.0f, 0.0f );
  m_Flare[6]  = SetFlare(  3,  0.20f*9, 0.07f, 0.3f, 0.0f, 0.0f );
  m_Flare[7]  = SetFlare(  0,  0.00f*9, 0.06f, 0.3f, 0.0f, 0.0f );
  m_Flare[8]  = SetFlare(  4, -0.25f*9, 0.15f, 0.5f, 0.0f, 0.0f );
  m_Flare[9]  = SetFlare(  4, -0.40f*9, 0.03f, 0.6f, 0.0f, 0.0f );
  m_Flare[10] = SetFlare(  4, -0.60f*9, 0.06f, 0.4f, 0.0f, 0.0f );
  m_Flare[11] = SetFlare(  4,-10.00f*9, 0.04f, 0.2f, 0.0f, 0.0f );
	for( int i=0; i<NUM_FLARES; i++ ) {
		m_flareColor[i] = PACK32BIT8888(255, 255*m_Flare[i].r, 255*m_Flare[i].g, 255*m_Flare[i].b);
	}
}

Flare CFlare :: SetFlare( int wType, float fLocation, float fScale, float fRed, float fGreen, float fBlue )
{
  Flare ret;
  ret.wType  = wType;
  ret.fLoc   = fLocation;
  ret.fScale = fScale;
  ret.r      = fRed;
  ret.g      = fGreen;
  ret.b      = fBlue;
  return ret;
}

void CFlare ::Process( float fTimeKey, vector3d vFrom, vector3d vAt )
{
	m_vLightPt = g_Weather.GetSunWorldLoc();
	m_vLightPt.x += vFrom.x;
	m_vLightPt.z += vFrom.z;

	vector3d vLookatPt = vAt;
	vector3d vFromPt = vFrom;
	vector3d vViewVec = vLookatPt - vFromPt;
	vViewVec.Normalize();
	vector3d vLightVec = m_vLightPt - vAt;
	vLightVec.Normalize();

	vector3d vCenterPt = vFromPt + vViewVec * 1.01f;

	vector3d dx = vViewVec;
	dx.Normalize();
	float fDeltaX = (dx.x - dx.y);
	float fDeltaY = (dx.y + dx.x);
	float fViewScale = _sqrt(fDeltaX*fDeltaX + fDeltaY*fDeltaY);

	for( int i=0; i<NUM_FLARES; i++ ) {
		m_Flare[i].vPositionPt = vCenterPt + vLightVec * m_Flare[i].fLoc;
		m_Flare[i].fRenderSize = fViewScale * m_Flare[i].fScale;
	}
}

void CFlare :: Render( matrix& view, vector3d vFrom, vector3d vAt )
{
	for( int i=0; i<NUM_FLARES; i++ ) {
		sprintf( m_strTexName, "effect\\flare%d.bmp", m_Flare[i].wType );

		vector3d v1 = m_Flare[i].vPositionPt;
		vector3d v2 = vAt - vFrom;
		v1.Normalize();
		v2.Normalize();
		float f = v1.DotProduct( v2 );

		if( f<0.0f ) {

			RPQuadFace&	rp = g_renderer->BorrowQuadRP();

			tlvertex3d*	tlvert = rp.verts;

			g_renderer->ProjectVertex(m_Flare[i].vPositionPt, view, tlvert);

			tlvert[1] = tlvert[2] = tlvert[3] = tlvert[0];

			tlvert[0].x -= 290.0f*m_Flare[i].fScale;
			tlvert[0].y -= 290.0f*m_Flare[i].fScale;
			tlvert[1].x += 290.0f*m_Flare[i].fScale;
			tlvert[1].y -= 290.0f*m_Flare[i].fScale;
			tlvert[2].x -= 290.0f*m_Flare[i].fScale;
			tlvert[2].y += 290.0f*m_Flare[i].fScale;
			tlvert[3].x += 290.0f*m_Flare[i].fScale;
			tlvert[3].y += 290.0f*m_Flare[i].fScale;

			rp.verts[0].tu = 0.0f;
			rp.verts[0].tv = 0.0f;
			rp.verts[1].tu = 1.0f;
			rp.verts[1].tv = 0.0f;
			rp.verts[2].tu = 0.0f;
			rp.verts[2].tv = 1.0f;
			rp.verts[3].tu = 1.0f;
			rp.verts[3].tv = 1.0f;
			rp.srcAlphaMode = D3DBLEND_ONE;
			rp.destAlphaMode = D3DBLEND_ONE;
			rp.tex = g_texMgr.GetTexture(m_strTexName, false);

			rp.verts[0].color = m_flareColor[i];
			rp.verts[1].color = m_flareColor[i];
			rp.verts[2].color = m_flareColor[i];
			rp.verts[3].color = m_flareColor[i];

			g_renderer->AddRP(&rp, RF_ALPHA | RF_EFFECT);
		}
	}
}

CSkyBox :: CSkyBox( void )
{
	m_pvWall[0].x = -1250, m_pvWall[0].y = -450, m_pvWall[0].z = 1250;
	m_pvWall[0].color = PACK32BIT8888(255, 255, 255, 255);
	m_pvWall[1].x = 1250, m_pvWall[1].y = -450, m_pvWall[1].z = 1250;
	m_pvWall[1].color = PACK32BIT8888(255, 255, 255, 255);
	m_pvWall[2].x = -1250, m_pvWall[2].y = 650, m_pvWall[2].z = 1250;
	m_pvWall[2].color = PACK32BIT8888(255, 255, 255, 255);
	m_pvWall[3].x = 1250, m_pvWall[3].y = 650, m_pvWall[3].z = 1250;
	m_pvWall[3].color = PACK32BIT8888(255, 255, 255, 255);

	m_pvWall[4].x = -1250, m_pvWall[4].y = -450, m_pvWall[4].z = -1250;
	m_pvWall[4].color = PACK32BIT8888(255, 255, 255, 255);
	m_pvWall[5].x = 1250, m_pvWall[5].y = -450, m_pvWall[5].z = -1250;
	m_pvWall[5].color = PACK32BIT8888(255, 255, 255, 255);
	m_pvWall[6].x = -1250, m_pvWall[6].y = 650, m_pvWall[6].z = -1250;
	m_pvWall[6].color = PACK32BIT8888(255, 255, 255, 255);
	m_pvWall[7].x = 1250, m_pvWall[7].y = 650, m_pvWall[7].z = -1250;
	m_pvWall[7].color = PACK32BIT8888(255, 255, 255, 255);
	m_fLoopRotY = 0.0f;

	m_pvWall[8].x = -1250, m_pvWall[8].y = -350, m_pvWall[8].z = 1240;
	m_pvWall[9].x = 1250, m_pvWall[9].y = 100, m_pvWall[9].z = 1250;
	m_flare.Init();
}


CSkyBox :: ~CSkyBox( void )
{
}

bool CSkyBox :: ChkVisible( float hratio, float vratio, vector3d vPos, matrix &view, tlvertex3d vert )
{
	plane3d		temp[6];
	plane3d		planes[6];
	vector3d	planeNormals[6];
	vector3d	halfSize, center;

	temp[0].Set(-hratio, 0, 1, 0);
	temp[1].Set(hratio, 0, 1, 0);
	temp[2].Set(0, -vratio, 1, 0);
	temp[3].Set(0, vratio, 1, 0);
	temp[4].Set(0, 0, 1, -NEARPLANE);
	temp[5].Set(0, 0, -1, FARPLANE);
	int i;
	for (i=0; i<6; i++) {
		planes[i].MatrixMult(view, temp[i]);
		planeNormals[i].Set(planes[i].x, planes[i].y, planes[i].z);
	}

	bool	isPartial = false;

	halfSize.x = 1.0f;
	halfSize.y = 1.0f;
	halfSize.z = 1.0f;
	center.x = vPos.x + vert.x;
	center.y = vPos.y + vert.y;
	center.z = vPos.z + vert.z;

	for( i=0;i<6;i++ ) {
		float	r = fabs(halfSize.x * planeNormals[i].x) +
					fabs(halfSize.y * planeNormals[i].y) +
					fabs(halfSize.z * planeNormals[i].z);
		float	d = planeNormals[i].x * center.x +
					planeNormals[i].y * center.y +
					planeNormals[i].z * center.z +
					planes[i].w;

		if (d>r)
			continue;
		if (d>-r)
			isPartial = true;
		else
			return	false;
	}
	if (isPartial)
		return	true;
	else
		return	true;
}


void CSkyBox :: Render( matrix &view, vector3d* vPos, float roty, float fLatitude, vector3d vFrom, vector3d vAt )
{
	tlvertex3d 	vert[10];

	matrix		ltm, ltmAlpha;

	ltm.MakeIdentity();
	ltm.AppendYRotation(DegToRad(m_fLoopRotY+=0.1f));
	ltm.AppendTranslate(*vPos);

	ltmAlpha.MakeIdentity();
	ltmAlpha.AppendYRotation(DegToRad(roty));
	ltmAlpha.AppendTranslate(*vPos);

	matrix		final, finalAlpha;
	final.MultMatrix(ltm, view);
	finalAlpha.MultMatrix(ltmAlpha, view);

	vector3d	vec;
	int i ;
	for(i=0;i<8;i++ )
	{

		vec.x = m_pvWall[i].x;
		vec.y = m_pvWall[i].y;
		vec.z = m_pvWall[i].z;


		g_renderer->ProjectVertex(vec, final, &vert[i]);

		vert[i].color = m_pvWall[i].color;
	}
	for( i=8;i<10;i++ )
	{

		vec.x = m_pvWall[i].x;
		vec.y = m_pvWall[i].y;
		vec.z = m_pvWall[i].z;


		g_renderer->ProjectVertex(vec, finalAlpha, &vert[i]);

		vert[i].color = m_pvWall[i].color;
	}

	if (m_fLoopRotY >= 360)
		m_fLoopRotY -= 360;
	roty-=m_fLoopRotY;

	if (roty >= 360)
		roty -= 360;
	if (roty < 0)
		roty += 360;

	if( roty>270 || roty<90 ) {
		RPQuadFace&	rp = g_renderer->BorrowQuadRP();

		rp.SetGeomInfo(0, vert[0]);
		rp.SetGeomInfo(1, vert[1]);
		rp.SetGeomInfo(2, vert[2]);
		rp.SetGeomInfo(3, vert[3]);
		rp.verts[0].tu = 0.0f;
		rp.verts[0].tv = 0.0f;
		rp.verts[1].tu = 1.0f;
		rp.verts[1].tv = 0.0f;
		rp.verts[2].tu = 0.0f;
		rp.verts[2].tv = 1.0f;
		rp.verts[3].tu = 1.0f;
		rp.verts[3].tv = 1.0f;
		rp.srcAlphaMode = D3DBLEND_SRCALPHA;
		rp.destAlphaMode = D3DBLEND_INVSRCALPHA;
		rp.tex = g_texMgr.GetTexture("effect\\skybox_back.bmp", false);
		rp.verts[0].color = vert[0].color;
		rp.verts[1].color = vert[1].color;
		rp.verts[2].color = vert[2].color;
		rp.verts[3].color = vert[3].color;

		g_renderer->AddRP(&rp, RF_ALPHA | RF_EMISSIVE);
	}
	if( roty>=90 && roty<270 ) {
		RPQuadFace&	rp = g_renderer->BorrowQuadRP();

		rp.SetGeomInfo(0, vert[4]);
		rp.SetGeomInfo(1, vert[5]);
		rp.SetGeomInfo(2, vert[6]);
		rp.SetGeomInfo(3, vert[7]);
		rp.verts[0].tu = 0.0f;
		rp.verts[0].tv = 0.0f;
		rp.verts[1].tu = 1.0f;
		rp.verts[1].tv = 0.0f;
		rp.verts[2].tu = 0.0f;
		rp.verts[2].tv = 1.0f;
		rp.verts[3].tu = 1.0f;
		rp.verts[3].tv = 1.0f;
		rp.srcAlphaMode = D3DBLEND_SRCALPHA;
		rp.destAlphaMode = D3DBLEND_INVSRCALPHA;
		rp.tex = g_texMgr.GetTexture("effect\\skybox_front.bmp", false);
		rp.verts[0].color = vert[4].color;
		rp.verts[1].color = vert[5].color;
		rp.verts[2].color = vert[6].color;
		rp.verts[3].color = vert[7].color;

		g_renderer->AddRP(&rp, RF_ALPHA | RF_EMISSIVE);
	}
	if( roty>=180 ) {
		RPQuadFace&	rp = g_renderer->BorrowQuadRP();

		rp.SetGeomInfo(0, vert[0]);
		rp.SetGeomInfo(1, vert[2]);
		rp.SetGeomInfo(2, vert[4]);
		rp.SetGeomInfo(3, vert[6]);
		rp.verts[0].tu = 0.0f;
		rp.verts[0].tv = 0.0f;
		rp.verts[1].tu = 1.0f;
		rp.verts[1].tv = 0.0f;
		rp.verts[2].tu = 0.0f;
		rp.verts[2].tv = 1.0f;
		rp.verts[3].tu = 1.0f;
		rp.verts[3].tv = 1.0f;
		rp.srcAlphaMode = D3DBLEND_SRCALPHA;
		rp.destAlphaMode = D3DBLEND_INVSRCALPHA;
		rp.tex = g_texMgr.GetTexture("effect\\skybox_left.bmp", false);
		rp.verts[0].color = vert[0].color;
		rp.verts[1].color = vert[2].color;
		rp.verts[2].color = vert[4].color;
		rp.verts[3].color = vert[6].color;

		g_renderer->AddRP(&rp, RF_ALPHA | RF_EMISSIVE);
	}
	if( !(roty>=180) ) {
		RPQuadFace&	rp = g_renderer->BorrowQuadRP();

		rp.SetGeomInfo(0, vert[1]);
		rp.SetGeomInfo(1, vert[3]);
		rp.SetGeomInfo(2, vert[5]);
		rp.SetGeomInfo(3, vert[7]);
		rp.verts[0].tu = 0.0f;
		rp.verts[0].tv = 0.0f;
		rp.verts[1].tu = 1.0f;
		rp.verts[1].tv = 0.0f;
		rp.verts[2].tu = 0.0f;
		rp.verts[2].tv = 1.0f;
		rp.verts[3].tu = 1.0f;
		rp.verts[3].tv = 1.0f;
		rp.srcAlphaMode = D3DBLEND_SRCALPHA;
		rp.destAlphaMode = D3DBLEND_INVSRCALPHA;
		rp.tex = g_texMgr.GetTexture("effect\\skybox_right.bmp", false);
		rp.verts[0].color = vert[1].color;
		rp.verts[1].color = vert[3].color;
		rp.verts[2].color = vert[5].color;
		rp.verts[3].color = vert[7].color;

		g_renderer->AddRP(&rp, RF_ALPHA | RF_EMISSIVE);
	}
	if( fLatitude>-11.0f ) {
		RPQuadFace&	rp = g_renderer->BorrowQuadRP();

		rp.SetGeomInfo(0, vert[0]);
		rp.SetGeomInfo(1, vert[1]);
		rp.SetGeomInfo(2, vert[4]);
		rp.SetGeomInfo(3, vert[5]);
		rp.verts[0].tu = 0.0f;
		rp.verts[0].tv = 0.0f;
		rp.verts[1].tu = 1.0f;
		rp.verts[1].tv = 0.0f;
		rp.verts[2].tu = 0.0f;
		rp.verts[2].tv = 1.0f;
		rp.verts[3].tu = 1.0f;
		rp.verts[3].tv = 1.0f;
		rp.srcAlphaMode = D3DBLEND_SRCALPHA;
		rp.destAlphaMode = D3DBLEND_INVSRCALPHA;
		rp.tex = g_texMgr.GetTexture("effect\\skybox_top.bmp", false);
		rp.verts[0].color = vert[0].color;
		rp.verts[1].color = vert[1].color;
		rp.verts[2].color = vert[4].color;
		rp.verts[3].color = vert[5].color;

		g_renderer->AddRP(&rp, RF_ALPHA | RF_EMISSIVE);
	}
	RPQuadFace&	rp = g_renderer->BorrowQuadRP();

	rp.verts[0].x = vert[8].x, rp.verts[0].y = vert[8].y, rp.verts[0].z = vert[8].z, rp.verts[0].oow = vert[8].oow;
	rp.verts[1].x = vert[9].x, rp.verts[1].y = vert[8].y, rp.verts[1].z = vert[8].z, rp.verts[1].oow = vert[8].oow;
	rp.verts[2].x = vert[8].x, rp.verts[2].y = vert[9].y, rp.verts[2].z = vert[8].z, rp.verts[2].oow = vert[8].oow;
	rp.verts[3].x = vert[9].x, rp.verts[3].y = vert[9].y, rp.verts[3].z = vert[8].z, rp.verts[3].oow = vert[8].oow;
	rp.verts[0].tu = 0.0f;
	rp.verts[0].tv = 0.0f;
	rp.verts[1].tu = 1.0f;
	rp.verts[1].tv = 0.0f;
	rp.verts[2].tu = 0.0f;
	rp.verts[2].tv = 1.0f;
	rp.verts[3].tu = 1.0f;
	rp.verts[3].tv = 1.0f;
	rp.srcAlphaMode = D3DBLEND_SRCALPHA;
	rp.destAlphaMode = D3DBLEND_INVSRCALPHA;
	rp.tex = NULL;
	rp.verts[0].color = PACK32BIT8888(0, 255, 255, 255);
	rp.verts[1].color = PACK32BIT8888(0, 255, 255, 255);
	rp.verts[2].color = PACK32BIT8888(255, 255, 255, 255);
	rp.verts[3].color = PACK32BIT8888(255, 255, 255, 255);

	g_renderer->AddRP(&rp, RF_ALPHA | RF_EMISSIVE);
	m_flare.Process( 0.0f, vFrom, vAt );
	m_flare.Render( view, vFrom, vAt );
}

CSkyBoxEllipse :: CSkyBoxEllipse( void )
{
	m_res = (C3dModelRes *)g_resMgr().Get("skybox.rsm", false);
	m_sky = new C3dActor;
	m_sky->OnInit();
	m_sky->SetScale(vector3d(30.0f, 30.0f, 30.0f));
	m_sky->SetRot(vector3d(0.0f, 0.0f, 0.0f));
	m_fLoopRotX = 0.0f;
	m_sky->SetAnimSpeed(0.1f);
	m_sky->AssignModel(m_res);
	m_sky->UpdateVertexColor(vector3d(0.0f,1.0f,0.0f), vector3d(1.0f,1.0f,1.0f), vector3d(1.0f,1.0f,1.0f));
	m_sky->SetFrame(0);
	m_flare.Init();
}

CSkyBoxEllipse :: ~CSkyBoxEllipse( void )
{
	if (m_sky) {
		delete	m_sky;
		m_sky = NULL;
	}
}

void CSkyBoxEllipse :: Render( matrix &view, vector3d* vPos, float roty, float fLatitude, vector3d vFrom, vector3d vAt )
{
	vector3d v = *vPos;
	v.y += 1200.0f;
	m_sky->SetPos(v);
	m_sky->OnProcess();
	m_sky->Render( view, TRUE, RF_EMISSIVE );


	tlvertex3d 	vWall[2], vert[2];
	matrix		ltmAlpha, finalAlpha;
	vector3d	vec;

	ltmAlpha.MakeIdentity();
	ltmAlpha.AppendYRotation(DegToRad(roty));
	ltmAlpha.AppendTranslate(*vPos);
	finalAlpha.MultMatrix(ltmAlpha, view);

	vWall[0].x = -1250, vWall[0].y = -250, vWall[0].z = 1240;
	vWall[1].x = 1250, vWall[1].y = 100, vWall[1].z = 1250;

	for( int i=0;i<2;i++ )
	{
		vec.x = vWall[i].x;
		vec.y = vWall[i].y;
		vec.z = vWall[i].z;

		g_renderer->ProjectVertex(vec, finalAlpha, &vert[i]);
	}

	RPQuadFace&	rp = g_renderer->BorrowQuadRP();

	rp.verts[0].x = vert[0].x, rp.verts[0].y = vert[0].y, rp.verts[0].z = vert[0].z, rp.verts[0].oow = vert[0].oow;
	rp.verts[1].x = vert[1].x, rp.verts[1].y = vert[0].y, rp.verts[1].z = vert[0].z, rp.verts[1].oow = vert[0].oow;
	rp.verts[2].x = vert[0].x, rp.verts[2].y = vert[1].y, rp.verts[2].z = vert[0].z, rp.verts[2].oow = vert[0].oow;
	rp.verts[3].x = vert[1].x, rp.verts[3].y = vert[1].y, rp.verts[3].z = vert[0].z, rp.verts[3].oow = vert[0].oow;
	rp.verts[0].tu = 0.0f;
	rp.verts[0].tv = 0.0f;
	rp.verts[1].tu = 1.0f;
	rp.verts[1].tv = 0.0f;
	rp.verts[2].tu = 0.0f;
	rp.verts[2].tv = 1.0f;
	rp.verts[3].tu = 1.0f;
	rp.verts[3].tv = 1.0f;
	rp.srcAlphaMode = D3DBLEND_SRCALPHA;
	rp.destAlphaMode = D3DBLEND_INVSRCALPHA;
	rp.tex = NULL;
	rp.verts[0].color = PACK32BIT8888(0, 255, 255, 255);
	rp.verts[1].color = PACK32BIT8888(0, 255, 255, 255);
	rp.verts[2].color = PACK32BIT8888(255, 255, 255, 255);
	rp.verts[3].color = PACK32BIT8888(255, 255, 255, 255);

	g_renderer->AddRP(&rp, RF_ALPHA | RF_EFFECT);

	m_flare.Process( 0.0f, vFrom, vAt );
	m_flare.Render( view, vFrom, vAt );
}
