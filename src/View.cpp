#include "common.h"
#include "3dActor.h"
#include "View.h"
#include "3dGround.h"
#include "GameActor.h"
#include "Session.h"
#include "RagEffect.h"
#include "GameMode.h"
#include "GameWorld.h"
#include "Mode.h"
#include "resource/Attr.h"
#include "ItemInfo.h"
#include <math.h>
#include <stdio.h>
#include "bf_types.h"




#include "resource/Ground.h"

extern CModeMgr g_modeMgr;
extern CRenderer* g_renderer;
extern tagRECT g_drawRect;

tagRECT g_drawRect;
extern GroundAttrPosInfo g_groundAttrPosInfo;
extern float AVG_DIST;
extern int g_isFixedCamera;   // GameMode.cpp; /camera
extern float g_zOffset;
extern float g_zHover;


static BOOL rendered = FALSE;



CViewFrustum::CViewFrustum()
{
}

void CViewFrustum::Free()
{
	m_cubeletListTotal.erase(m_cubeletListTotal.begin(), m_cubeletListTotal.end());
	m_cubeletListPartial.erase(m_cubeletListPartial.begin(), m_cubeletListPartial.end());
}

void	CViewFrustum :: Build(float hratio, float vratio, const matrix &viewMatrix, SceneGraphNode *rootNode)
{
	plane3d	temp[6];

	temp[0].Set(-hratio, 0, 1, 0);
	temp[1].Set(hratio, 0, 1, 0);
	temp[2].Set(0, -vratio, 1, 0);
	temp[3].Set(0, vratio, 1, 0);
	temp[4].Set(0, 0, 1, -NEARPLANE);
	temp[5].Set(0, 0, -1, FARPLANE);

	for (int k=0; k<6; k++)
	{
		m_planes[k].MatrixMult(viewMatrix, temp[k]);
		m_planeNormals[k].Set(m_planes[k].x, m_planes[k].y, m_planes[k].z);
	}

	CullSceneNode(rootNode, 0, false);







}

void CViewFrustum::CullSceneNode(SceneGraphNode* node, int level, bool isTotallyInside)
{
	const int GRAPH_DEPTH = 5;

	if (level == GRAPH_DEPTH)
	{
		if (isTotallyInside)
			m_cubeletListTotal.push_back(node);
		else
			m_cubeletListPartial.push_back(node);
		return;
	}

	for (int c = 0; c < 4; c++)
	{
		if (isTotallyInside)
		{
			CullSceneNode(node->m_child[c], level + 1, isTotallyInside);
		}
		else
		{
			int result = CheckAABBIntersect(node->m_child[c]);

			if (result == 0)
			{
				continue;
			}
			else if (result == 2)
			{
				CullSceneNode(node->m_child[c], level + 1, false);
			}
			else
			{
				CullSceneNode(node->m_child[c], level + 1, true);
			}
		}
	}
}


int CViewFrustum::CheckAABBIntersect(SceneGraphNode* node)
{
	bool isPartial = false;

	for (int i = 0; i < 6; i++)
	{
		float r = fabs(node->m_halfSize.x * m_planeNormals[i].x) +
				  fabs(node->m_halfSize.y * m_planeNormals[i].y) +
				  fabs(node->m_halfSize.z * m_planeNormals[i].z);

		float d = m_planeNormals[i].x * node->m_center.x +
				  m_planeNormals[i].y * node->m_center.y +
				  m_planeNormals[i].z * node->m_center.z +
				  m_planes[i].w;

		if (d > r)
			continue;
		if (d > -r)
			isPartial = true;
		else
			return 0;
	}
	if (isPartial)
		return 2;
	else
		return 1;
}

int CViewFrustum::CheckOBBIntersect(const C3dOBB& bb)
{
	bool isPartial = false;

	for (int i = 0; i < 6; i++)
	{
		float r = fabs(bb.halfSize.x * m_planeNormals[i].DotProduct(bb.u)) +
				  fabs(bb.halfSize.y * m_planeNormals[i].DotProduct(bb.v)) +
				  fabs(bb.halfSize.z * m_planeNormals[i].DotProduct(bb.w));
		float d = m_planeNormals[i].x * bb.center.x +
				  m_planeNormals[i].y * bb.center.y +
				  m_planeNormals[i].z * bb.center.z +
				  m_planes[i].w;
		if (d > r)
			continue;
		if (d > -r)
			isPartial = true;
		else
			return 0;
	}
	if (isPartial)
		return 2;
	else
		return 1;
}

CView::CView()
{
	m_cur.longitude = 0.0;
	m_isQuake = FALSE;
	m_skyBox = NULL;
	m_quakeStartTick = 0;
	m_isFPSmode = false;
}

CView::~CView()
{
	if (m_skyBox) delete m_skyBox;
}

void CView::OnEnterFrame()
{
	m_dest.longitude = m_cur.longitude = 0.0;
	m_dest.distance = m_cur.distance = 300;
	m_dest.latitude = m_cur.latitude = -45.0;
	m_cur.at.Set(0.0f, 0.0f, 0.0f);
	m_dest.at.Set(0.0f, 0.0f, 0.0f);
	m_up = vector3d(0.0f, 1.0f, 0.0f);
}

void CView::OnExitFrame()
{
}

void CView::AddLongitude(float deltaLongitude)
{
	m_dest.longitude += deltaLongitude;
}

void CView::AddLatitude(float deltaLatitude)
{
	m_dest.latitude += deltaLatitude;
}

void CView::AddDistance(float deltaDistance)
{
	m_dest.distance += deltaDistance;
}

void CView::OnCalcViewInfo()
{
	m_dest.at = m_world->m_player->m_pos;
	InterpolateViewInfo();
	ProcessQuake();
	BuildViewMatrix();
}

void	CView :: ProcessQuake()
{
	if (!m_isQuake)
		return;
	if (bf_clock_ticks() >= m_quakeStartTick + m_QuakeTime)
	{
		m_isQuake = FALSE;
		m_cur.at.x = m_dest.at.x;
		m_cur.at.y = m_dest.at.y;
		m_cur.at.z = m_dest.at.z;
		m_cur.distance = m_dest.distance;
		m_cur.latitude = m_dest.latitude;
		return;
	}

	if (rendered)
	{
		rendered = FALSE;

		static float sideQuake = m_sideQuake;
		m_cur.at.x += (sideQuake + float(random(5)-2)/10.0f) * GetCos(m_cur.longitude);
		m_cur.at.z += (sideQuake + float(random(5)-2)/10.0f) * -GetSin(m_cur.longitude);
		sideQuake = -sideQuake;


		static float frontQuake = m_frontQuake;
		m_cur.distance += frontQuake + float(random(5)-2)/10.0f;
		frontQuake = -frontQuake;


		static float latitudeQuake = m_latitudeQuake;
		m_cur.latitude += latitudeQuake + float(random(5)-2)/15.0f;
		latitudeQuake = -latitudeQuake;
	}
}

void CView::SetQuakeInfo(float sideQuake, float frontQuake, float latitudeQuake)
{
	m_sideQuake = sideQuake;
	m_latitudeQuake = latitudeQuake;
	m_frontQuake = frontQuake;
}

void CView::SetQuake(int isQuake, int Type, float sideQuake, float frontQuake, float latitudeQuake)
{
	m_isQuake = isQuake;
	m_quakeStartTick = bf_clock_ticks();
	m_frontQuake = frontQuake;
	m_sideQuake = sideQuake;
	m_latitudeQuake = latitudeQuake;
	switch (Type)
	{
	case 0:
		m_QuakeTime = 650;
		break;
	case 1:
		m_QuakeTime = 200;
		break;
	default:
		m_QuakeTime = Type;
		break;
	}
}

void CView::OnRender()
{
	g_modeMgr.GetGameMode();
	rendered = TRUE;

	m_viewFrustum.Build(g_renderer->m_hratio, g_renderer->m_vratio, m_viewMatrix, &m_world->m_rootNode);

	g_drawRect.top = 999;
	g_drawRect.left = 999;
	g_drawRect.bottom = 0;
	g_drawRect.right = 0;

	// The cubelets the frustum keeps are drawn here, partly visible ones with
	// clipping first, as the 2008-09-03 client does (CView::OnRender, 0x517770):
	// each node's models once per frame, then its ground area; g_drawRect is
	// the union of the ground areas drawn. The reconstruction had lost this,
	// so neither ground nor models were drawn.
	std::list<SceneGraphNode *>::iterator nit;
	for (nit = m_viewFrustum.m_cubeletListPartial.begin(); nit != m_viewFrustum.m_cubeletListPartial.end(); ++nit)
	{
		SceneGraphNode *node = *nit;
		for (std::vector<C3dActor *>::iterator ait = node->m_actorList.begin(); ait != node->m_actorList.end(); ++ait)
		{
			C3dActor *actor = *ait;
			if (actor->m_renderSignature != g_renderer->m_curFrame)
			{
				int result = m_viewFrustum.CheckOBBIntersect(actor->m_oBoundingBox);
				actor->OnProcess();
				if (result == 1)
					actor->Render(m_viewMatrix, false, 0);
				else if (result == 2)
					actor->Render(m_viewMatrix, true, 0);
				actor->m_renderSignature = g_renderer->m_curFrame;
			}
		}
		node->m_ground->Render(m_viewMatrix, node->m_groundArea, true);
		if (node->m_groundArea.left < g_drawRect.left) g_drawRect.left = node->m_groundArea.left;
		if (node->m_groundArea.right > g_drawRect.right) g_drawRect.right = node->m_groundArea.right;
		if (node->m_groundArea.top < g_drawRect.top) g_drawRect.top = node->m_groundArea.top;
		if (node->m_groundArea.bottom > g_drawRect.bottom) g_drawRect.bottom = node->m_groundArea.bottom;
	}
	for (nit = m_viewFrustum.m_cubeletListTotal.begin(); nit != m_viewFrustum.m_cubeletListTotal.end(); ++nit)
	{
		SceneGraphNode *node = *nit;
		for (std::vector<C3dActor *>::iterator ait = node->m_actorList.begin(); ait != node->m_actorList.end(); ++ait)
		{
			C3dActor *actor = *ait;
			if (actor->m_renderSignature != g_renderer->m_curFrame)
			{
				actor->Render(m_viewMatrix, false, 0);
				actor->m_renderSignature = g_renderer->m_curFrame;
			}
		}
		node->m_ground->Render(m_viewMatrix, node->m_groundArea, false);
		if (node->m_groundArea.top < g_drawRect.top) g_drawRect.top = node->m_groundArea.top;
		if (node->m_groundArea.bottom > g_drawRect.bottom) g_drawRect.bottom = node->m_groundArea.bottom;
		if (node->m_groundArea.left < g_drawRect.left) g_drawRect.left = node->m_groundArea.left;
		if (node->m_groundArea.right > g_drawRect.right) g_drawRect.right = node->m_groundArea.right;
	}

	// the cell under the pointer, coloured by its attribute (GameMode.cpp)
	if (!g_groundAttrPosInfo.isRendered)
		m_world->m_ground->RenderAttrTile(m_viewMatrix, g_groundAttrPosInfo.x, g_groundAttrPosInfo.y, g_groundAttrPosInfo.cellColor);

	std::list<CGameObject *>::iterator it = m_world->m_gameObjectList.begin();
	while (it != m_world->m_gameObjectList.end())
	{
		(*it)->Render(m_viewMatrix);
		++it;
	}

	GetEeyeVector(&g_renderer->m_eyeVector);
	m_viewFrustum.Free();

	m_world->m_ground->FlushGround(m_viewMatrix);
}

void CView::GetEeyeVector(vector3d* eyeVector)
{
	*eyeVector = m_cur.at - m_from;
	eyeVector->Normalize();
}







#define  BIGFACTORdist 5.0f
#define  SMALLFACTORdist 0.04f
#define	 INTERPNUMdist1 10.0f
#define	 INTERPNUMdist2 6.0f

#define  BIGFACTORAngle 5.5f
#define  SMALLFACTORAngle 0.06f
#define	 INTERPNUMAngle1 10.0f
#define	 INTERPNUMAngle2 4.0f

#define  BIGFACTORpos 5.0f
#define  SMALLFACTORpos 0.08f
#define	 INTERPNUMpos1 9.0f
#define	 INTERPNUMpos2 4.0f
void	CView :: InterpolateViewInfo()
{
	float	interpolateAmt;

	while (m_dest.longitude >= 360)
		m_dest.longitude-=360;
	while (m_dest.longitude < 0)
		m_dest.longitude+=360;
	interpolateAmt = m_dest.longitude - m_cur.longitude;
	while (interpolateAmt >= 180)
		interpolateAmt-=360;
	while (interpolateAmt < -180)
		interpolateAmt+=360;
	m_cur.longitude += interpolateAmt / INTERPNUMAngle1;

	while (m_cur.longitude >= 360)
		m_cur.longitude-=360;
	while (m_cur.longitude < 0)
		m_cur.longitude+=360;

	interpolateAmt = m_dest.latitude - m_cur.latitude;
	interpolateAmt /= INTERPNUMAngle1;
	m_cur.latitude += interpolateAmt;

#if defined(BF_PORT)
	// Restored from the reference (0x517bd0, the read of g_isFixedCamera at
	// 0x517cf0): the camera follows the character a tenth of the way each
	// step, as it turns and zooms; /camera (g_isFixedCamera) fixes it on the
	// character. The reconstruction always did the latter.
	if (!g_isFixedCamera) {
		m_cur.at.x += (m_dest.at.x - m_cur.at.x) / INTERPNUMAngle1;
		m_cur.at.y += (m_dest.at.y - m_cur.at.y) / INTERPNUMAngle1;
		m_cur.at.z += (m_dest.at.z - m_cur.at.z) / INTERPNUMAngle1;
	} else {
		m_cur.at.x = m_dest.at.x;
		m_cur.at.z = m_dest.at.z;
		m_cur.at.y = m_dest.at.y;
	}
#else
	m_cur.at.x = m_dest.at.x;
	m_cur.at.z = m_dest.at.z;
	m_cur.at.y = m_dest.at.y;
#endif

	interpolateAmt = m_dest.distance - m_cur.distance;
	interpolateAmt /= INTERPNUMAngle1;
	m_cur.distance += interpolateAmt;

	float tangent = GetSin(-m_cur.latitude) / GetCos(-m_cur.latitude);
	float ctangent = GetCos(-m_cur.latitude) / GetSin(-m_cur.latitude);
	g_slope = tangent*g_gradient/g_renderer->GetScreenYFactor();
	g_shadowSlope = ctangent*g_gradient/g_renderer->GetScreenYFactor();

	g_zOffset = 0.000009f * (GetSin(-m_cur.latitude) / GetSin(45)) * (AVG_DIST / m_cur.distance);
	g_zHover = 0.000036f * (AVG_DIST / m_cur.distance);
}



void CView::BuildViewMatrix()
{
	vector3d start;
	matrix r;

	r.MakeXRotation(pi * m_cur.latitude / 180.0f);
	r.AppendYRotation(pi * m_cur.longitude / 180.0f);
	start.Set(0.0f, 0.0f, -m_cur.distance);
	m_from.MatrixMult(start, r);

	m_from.x += m_cur.at.x;
	m_from.y += m_cur.at.y;
	m_from.z += m_cur.at.z;

	float h = g_modeMgr.GetGameMode()->GetWorld()->m_attr->GetHeight(m_from.x, m_from.z);
	if (m_from.y > h)
		m_from.y = h;

	m_viewMatrix.MakeView(m_from, m_cur.at, m_up);
	m_invViewMatrix.MakeInverse(m_viewMatrix);
	g_renderer->SetLookAt(m_from, m_cur.at, m_up);
}
