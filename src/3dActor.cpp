#include "common.h"
#include "3dActor.h"
#include "3dlib/Anim.h"
#include "3dlib/mesh.h"
#include "3dlib/Prim.h"
#include "3dGround.h"
#include "3dlib/DrawUtil.h"
#include "GameMode.h"
#include "GameWorld.h"
#include "Mode.h"
#include "RagEffect.h"
#include "RagEffectPrim.h"
#include "resource/Model.h"

#include <math.h>
#include "resource/Ground.h"




extern CRenderer *g_renderer;
extern CModeMgr g_modeMgr;
extern int g_isBlackWorld;
extern int g_isVoodoo;




C3dNode :: C3dNode()
{
	m_parent	= NULL;
	m_master	= NULL;
	m_mesh		= NULL;
	m_isAlphaForPlayer = FALSE;
	m_opacity	= 1.0f;
	m_numTexture = 0;
}

C3dNode :: ~C3dNode()
{
	Reset();
}

void	C3dNode :: Reset()
{
	std::list<C3dNode *>::iterator i = m_child.begin();
	while (i!=m_child.end()) {
		delete *i;
		i++;
	}
	m_child.erase(m_child.begin(), m_child.end());
	m_texture.clear();
	m_colorInfo.clear();
	m_destVertCol.clear();



}

void	C3dNode :: SetFrame(int framepos)
{
	matrix	posTM, rotTM, scaleTM;

	m_posAnim.GetMatrix(framepos, posTM);
	m_rotAnim.GetMatrix(framepos, rotTM);
	m_scaleAnim.GetMatrix(framepos, scaleTM);

	matrix	tmp;
	tmp.MultMatrix(scaleTM, rotTM);
	m_ltm.MultMatrix(tmp, posTM);

	std::list<C3dNode *>::iterator i = m_child.begin();
	while (i != m_child.end()) {
		(*i)->SetFrame(framepos);
		i++;
	}
}

void	C3dNode :: Render(const matrix &ptm, int flipNormal, bool clip)
{
	matrix nodeTM;
	nodeTM.MultMatrix(m_ltm, ptm);

	static tlvertex3d tempVert[3000];

	vector3d *vert = m_mesh->m_vert;
	int numVert = m_mesh->m_numVert;
	vector3d envdiff = g_Weather.GetEnvDiff();
	vector3d ambient = g_Weather.GetAmbient();

	if (g_Weather.IsObjLight())
		g_modeMgr.GetGameMode();

	unsigned char fadeAlphaCnt = m_master->m_fadeAlphaCnt;

	for (int i = 0; i < numVert; i++, vert++)
		g_renderer->ProjectVertex(*vert, nodeTM, &tempVert[i]);

	tlvertex3d *rpVert = g_renderer->BorrowVerts(m_mesh->m_numFace * 3);
	face3d *face = m_mesh->m_face;
	face3d *faceEnd = face + m_mesh->m_numFace;
	tlvertex3d *rpVertIter;
	ColorInfo *colorInfo = m_colorInfo.data();

	while (face != faceEnd) {
		WORD meshMtlId = face->meshMtlId;

		for (rpVertIter = rpVert; face != faceEnd; face++, colorInfo++) {
			if (face == NULL)
				break;
			if (face->meshMtlId != meshMtlId)
				break;

			tlvertex3d *v1 = &tempVert[face->vertindex[0]];
			tlvertex3d *v2 = &tempVert[face->vertindex[1]];
			tlvertex3d &v3 = tempVert[face->vertindex[2]];

			if (clip) {
				float minx, maxx, miny, maxy;

				if ((minx = min(min(v1->x, v2->x), v3.x)) >= g_renderer->m_width)
					continue;

				if ((maxx = max(max(v1->x, v2->x), v3.x)) < 0)
					continue;

				if ((miny = min(min(v1->y, v2->y), v3.y)) >= g_renderer->m_height)
					continue;

				if ((maxy = max(max(v1->y, v2->y), v3.y)) < 0)
					continue;

				if (g_isVoodoo) {
					if (minx <= g_renderer->m_guardBandLeft)
						continue;
					if (miny <= g_renderer->m_guardBandTop)
						continue;
					if (maxx >= g_renderer->m_guardBandRight)
						continue;
					if (maxy >= g_renderer->m_guardBandBottom)
						continue;
				}
			}

			BOOL front = (v3.y - v1->y) * (v2->x - v1->x) <
				(v2->y - v1->y) * (v3.x - v1->x);
			if (!(front && !flipNormal || !front && flipNormal) && !face->twoSide)
				continue;

			rpVertIter[0] = *v1;
			rpVertIter[1] = *v2;
			rpVertIter[2] = v3;

			rpVertIter[0].tu = colorInfo->uv[0].x;
			rpVertIter[0].tv = colorInfo->uv[0].y;
			rpVertIter[1].tu = colorInfo->uv[1].x;
			rpVertIter[1].tv = colorInfo->uv[1].y;
			rpVertIter[2].tu = colorInfo->uv[2].x;
			rpVertIter[2].tv = colorInfo->uv[2].y;

			if (g_isBlackWorld) {
				for (int j = 0; j < 3; j++) {
					COLOR color = colorInfo->argb[j];
					color.a = fadeAlphaCnt;
					color.r *= ambient.x;
					color.g *= ambient.y;
					color.b *= ambient.z;
					rpVertIter[j].argb = color;
				}
			} else if (m_isAlphaForPlayer) {
				for (int j = 0; j < 3; j++) {
					COLOR color = colorInfo->argb[j];
					color.a = 128;
					rpVertIter[j].argb = color;
				}
			} else if (g_Weather.IsObjLight()) {
				int index = colorInfo - m_colorInfo.data(); m_numTexture = m_numTexture; m_opacity = m_opacity;
				for (int j = 0; j < 3; j++) {
					COLOR color = colorInfo->argb[j];
					color.a = fadeAlphaCnt;
					color.r = m_destVertCol[index*9 + j*3] * envdiff.x;
					color.g = m_destVertCol[index*9 + j*3 + 1] * envdiff.y;
					color.b = m_destVertCol[index*9 + j*3 + 2] * envdiff.z;
					rpVertIter[j].argb = color;
				}
			} else {
				for (int j = 0; j < 3; j++) {
					COLOR color = colorInfo->argb[j];
					color.a = fadeAlphaCnt;
					color.r *= envdiff.x;
					color.g *= envdiff.y;
					color.b *= envdiff.z;
					rpVertIter[j].argb = color;
				}
			}

			rpVertIter += 3;
		}

		if (rpVertIter - rpVert) {
			RPFace &rpFace = g_renderer->BorrowNullRP();
			rpFace.verts = rpVert;
			rpFace.numVerts = rpVertIter - rpVert;
			rpFace.tex = m_texture[(meshMtlId & 0xffff) % m_numTexture];
			if (fadeAlphaCnt < 255 || rpFace.tex->GetPixelFormat() == PF_A4R4G4B4)
				g_renderer->AddRP(&rpFace, RF_ALPHA);
			else
				g_renderer->AddRP(&rpFace, RF_NORMAL);
			rpVert = rpVertIter;
		}
	}

	m_isAlphaForPlayer = FALSE;

	std::list<C3dNode *>::iterator it = m_child.begin();
	while (it != m_child.end()) {
		(*it)->Render(nodeTM, flipNormal, clip);
		it++;
	}
}

void	C3dNode :: UpdateBound(C3dAABB *box, const matrix &ptm)
{
	matrix	nodeTM;

	nodeTM.MultMatrix(m_ltm, ptm);


	for (int i=0; i<m_mesh->m_numVert; i++) {
		vector3d	v;

		v.MatrixMult(m_mesh->m_vert[i], nodeTM);
		if (v.x < box->min.x)
			box->min.x = v.x;
		if (v.x > box->max.x)
			box->max.x = v.x;
		if (v.y < box->min.y)
			box->min.y = v.y;
		if (v.y > box->max.y)
			box->max.y = v.y;
		if (v.z < box->min.z)
			box->min.z = v.z;
		if (v.z > box->max.z)
			box->max.z = v.z;
	}







	std::list<C3dNode *>::iterator a = m_child.begin();
	while (a != m_child.end()) {
		(*a)->UpdateBound(box, nodeTM);
		a++;
	}
}

void	C3dNode :: AssignModel(C3dNodeRes *modelNode)
{
	m_opacity = float(modelNode->alpha)/255.0f;
	sAssert(strlen(modelNode->name) < MAX_NODENAME);
	strcpy(m_name, modelNode->name);

	m_numTexture = modelNode->numTexture;
	sAssert(m_numTexture >= 0 && m_numTexture < 9999);
	m_texture.resize(m_numTexture);


	for (int t = 0; t < m_numTexture; t++)
	{

		m_texture[t] = modelNode->texture[t];
	}
	m_mesh = modelNode->mesh;

	m_colorInfo.resize(m_mesh->m_numFace);

	m_posAnim = modelNode->posanim;
	if (m_posAnim.Size() == 0)
		m_posAnim.AddKey(0, modelNode->pos[0], modelNode->pos[1], modelNode->pos[2]);
	m_rotAnim = modelNode->rotanim;
	if (m_rotAnim.Size() == 0)
		m_rotAnim.AddKey(0, modelNode->rotaxis[0], modelNode->rotaxis[1], modelNode->rotaxis[2], modelNode->rotangle);



#if	0
	m_scaleAnim = modelNode->scaleanim;
#endif
	if (m_scaleAnim.Size() == 0)
		m_scaleAnim.AddKey(0, modelNode->scale[0], modelNode->scale[1], modelNode->scale[2], 1, 0, 0, 0);

	std::list<C3dNodeRes *>::iterator i = modelNode->child.begin();
	while (i != modelNode->child.end()) {
		C3dNode	*childnode = new C3dNode;
		m_child.push_back(childnode);
		childnode->m_parent = this;
		childnode->m_master = m_master;
		childnode->AssignModel(*i);
		i++;
	}

	m_destVertCol.resize(m_mesh->m_numFace*3*3);
}

void	C3dNode :: UpdateAABB(const matrix &ptm, const matrix &wtm)
{
	matrix	nodeTM;
	nodeTM.MultMatrix(m_ltm, ptm);

	m_aabb.min.Set(100000, 100000, 100000);
	m_aabb.max.Set(-100000, -100000, -100000);
	C3dAABB tmpaabb;
	tmpaabb.min.Set(100000, 100000, 100000);
	tmpaabb.max.Set(-100000, -100000, -100000);

	for (int i=0; i<m_mesh->m_numVert; i++) {
		vector3d	v;
		v.MatrixMult(m_mesh->m_vert[i], nodeTM);
		if (v.x < tmpaabb.min.x)
			tmpaabb.min.x = v.x;
		if (v.x > tmpaabb.max.x)
			tmpaabb.max.x = v.x;
		if (v.y < tmpaabb.min.y)
			tmpaabb.min.y = v.y;
		if (v.y > tmpaabb.max.y)
			tmpaabb.max.y = v.y;
		if (v.z < tmpaabb.min.z)
			tmpaabb.min.z = v.z;
		if (v.z > tmpaabb.max.z)
			tmpaabb.max.z = v.z;
	}

	vector3d	tempvert[8];
	tempvert[0].Set(tmpaabb.min.x, tmpaabb.min.y, tmpaabb.min.z);
	tempvert[1].Set(tmpaabb.max.x, tmpaabb.min.y, tmpaabb.min.z);
	tempvert[2].Set(tmpaabb.min.x, tmpaabb.max.y, tmpaabb.min.z);
	tempvert[3].Set(tmpaabb.max.x, tmpaabb.max.y, tmpaabb.min.z);
	tempvert[4].Set(tmpaabb.min.x, tmpaabb.min.y, tmpaabb.max.z);
	tempvert[5].Set(tmpaabb.max.x, tmpaabb.min.y, tmpaabb.max.z);
	tempvert[6].Set(tmpaabb.min.x, tmpaabb.max.y, tmpaabb.max.z);
	tempvert[7].Set(tmpaabb.max.x, tmpaabb.max.y, tmpaabb.max.z);
	for (int j=0; j<8; j++) {
		vector3d vertices;
		vertices.MatrixMult(tempvert[j], wtm);
		if (m_aabb.min.x > vertices.x)
			m_aabb.min.x = vertices.x;
		if (m_aabb.min.y > vertices.y)
			m_aabb.min.y = vertices.y;
		if (m_aabb.min.z > vertices.z)
			m_aabb.min.z = vertices.z;
		if (m_aabb.max.x < vertices.x)
			m_aabb.max.x = vertices.x;
		if (m_aabb.max.y < vertices.y)
			m_aabb.max.y = vertices.y;
		if (m_aabb.max.z < vertices.z)
			m_aabb.max.z = vertices.z;
	}

	std::list<C3dNode *>::iterator a = m_child.begin();
	while (a != m_child.end()) {
		(*a)->UpdateAABB(nodeTM, wtm);
		a++;
	}
}

void	C3dNode :: CheckAABB(const CRayPicker &rayPicker)
{
	m_isAlphaForPlayer = rayPicker.CheckAABBIntersect(m_aabb);
	std::list<C3dNode *>::iterator a = m_child.begin();
	while (a != m_child.end()) {
		(*a)->CheckAABB(rayPicker);
		++a;
	}
}

void	C3dNode :: SetToAlpha()
{
	m_isAlphaForPlayer = TRUE;
	std::list<C3dNode *>::iterator a = m_child.begin();
	while (a != m_child.end()) {
		(*a)->SetToAlpha();
		++a;
	}
}

#include "rag_symbol_shift.h"

#ifndef BF_PORT
inline float fmax(float a, float b) { return a > b ? a : b; }
inline float fmin(float a, float b) { return a < b ? a : b; }
#endif




void	C3dNode :: UpdateVertexColor(const matrix &ptm, const vector3d &light, const vector3d &diffuseCol, const vector3d &ambientCol, int shadeType)
{
	static	faceLight	m_faceLight[3000];

	matrix	nodeTM, normalTM;
	int		i;
	RAG_SYMBOL_CHAIN(2)

	nodeTM.MultMatrix(m_ltm, ptm);
	normalTM = nodeTM;
	normalTM.v41 = normalTM.v42 = normalTM.v43 = 0;

	switch (shadeType) {
	case	0:
		for (i=0; i<m_mesh->m_numFace; i++) {
			for (int k=0; k<3; k++) {
				m_faceLight[i].argb[k].a = int(m_opacity*255);
				m_faceLight[i].argb[k].b = 255;
				m_faceLight[i].argb[k].g = 255;
				m_faceLight[i].argb[k].r = 255;
			}
		}
		break;
	case	1:
		for (i=0; i<m_mesh->m_numFace; i++) {
			vector3d	wnormal;
			wnormal.MatrixMult(m_mesh->m_faceNormal[i], normalTM);
			wnormal.Normalize();

			float	product = fmax(wnormal.DotProduct(light), 0);
			float	r = fmin(ambientCol.x + product*diffuseCol.x, 1);
			float	g = fmin(ambientCol.y + product*diffuseCol.y, 1);
			float	b = fmin(ambientCol.z + product*diffuseCol.z, 1);

			face3d &	face = m_mesh->m_face[i];

			for (int k=0; k<3; k++) {
				COLOR	color;

				color.a = int(m_opacity*255);
				if (m_mesh->m_tvert[face.tvertindex[k]].color) {
					color.r = r * 128;
					color.g = g * 128;
					color.b = b * 128;
				} else {
					color.r = r * 255;
					color.g = g * 255;
					color.b = b * 255;
				}
				m_faceLight[i].argb[k] = color;
			}
		}
		break;
	case	2:
		for (i=0; i<m_mesh->m_numFace; i++) {
			vector3d	wnormal[3];

			wnormal[0].MatrixMult(m_mesh->m_vertNormal[i*3+0], normalTM);
			wnormal[1].MatrixMult(m_mesh->m_vertNormal[i*3+1], normalTM);
			wnormal[2].MatrixMult(m_mesh->m_vertNormal[i*3+2], normalTM);

			face3d &	face = m_mesh->m_face[i];
			for (int k=0; k<3; k++) {
				wnormal[k].Normalize();

				float	product = fmax(wnormal[k].DotProduct(light), 0);
				float	r = fmin(ambientCol.x + product*diffuseCol.x, 1);
				float	g = fmin(ambientCol.y + product*diffuseCol.y, 1);
				float	b = fmin(ambientCol.z + product*diffuseCol.z, 1);

				COLOR	color;

				color.a = int(m_opacity*255);
				if (m_mesh->m_tvert[face.tvertindex[k]].color) {
					color.r = r * 128;
					color.g = g * 128;
					color.b = b * 128;
				} else {
					color.r = r * 255;
					color.g = g * 255;
					color.b = b * 255;
				}
				m_faceLight[i].argb[k] = color;
			}
		}
		break;
	}

	face3d *	face = m_mesh->m_face;
	face3d *	faceEnd = face + m_mesh->m_numFace;
	ColorInfo *	colorInfo = m_colorInfo.data();
	faceLight *	faceLight = m_faceLight;
	tvertex3d *	tvert = m_mesh->m_tvert;

	while (face != faceEnd) {
		WORD meshMtlId = face->meshMtlId;
		CTexture *tex = m_texture[meshMtlId % m_numTexture];

		float uAdjust = tex->GetUAdjust();
		float vAdjust = tex->GetVAdjust();

		for (; face != faceEnd; ++face, ++colorInfo, ++faceLight) {
			if (face->meshMtlId != meshMtlId)
				break;

			colorInfo->color[0] = faceLight->color[0];
			colorInfo->color[1] = faceLight->color[1];
			colorInfo->color[2] = faceLight->color[2];

			colorInfo->uv[0].Set(tvert[face->tvertindex[0]].u * uAdjust, tvert[face->tvertindex[0]].v * vAdjust);
			colorInfo->uv[1].Set(tvert[face->tvertindex[1]].u * uAdjust, tvert[face->tvertindex[1]].v * vAdjust);
			colorInfo->uv[2].Set(tvert[face->tvertindex[2]].u * uAdjust, tvert[face->tvertindex[2]].v * vAdjust);
		}
	}

	UpdateDestVertColor(g_renderer, nodeTM, (C3dGround15*)g_modeMgr.GetGameMode()->GetWorld()->m_ground);

	std::list<C3dNode *>::iterator it = m_child.begin();
	while (it != m_child.end()) {
		(*it)->UpdateVertexColor(nodeTM, light, diffuseCol, ambientCol, shadeType);
		it++;
	}
}

C3dNode *	C3dNode :: FindNode(const char *name)
{
	if (!(strcmp(m_name, name)))
		return	this;

	std::list<C3dNode *>::iterator i = m_child.begin();
	while (i != m_child.end()) {
		C3dNode *	t = (*i)->FindNode(name);
		if (t)
			return	t;
		i++;
	}
	return	NULL;
}

void	C3dNode :: UpdateDestVertColor(CRenderer * renderer, const matrix &nodeTM, C3dGround15 *pGround)
{
	vector3d	vt, ot;
	vector3d *	vert = m_mesh->m_vert;
	int		brightObj[3];

	for (int i=0; i<m_mesh->m_numFace; i++) {
		face3d &	curface = m_mesh->m_face[i];
		ColorInfo &	colorInfo = m_colorInfo[i];

		vt.MatrixMult(vert[curface.vertindex[0]], nodeTM);
		ot.x = m_master->GetPos().x + vt.x;
		ot.y = m_master->GetPos().y + vt.y;
		ot.z = m_master->GetPos().z + vt.z;
		pGround->GetBrightObjVal(ot, brightObj);

		m_destVertCol[i*9+0] = min(colorInfo.argb[0].r + brightObj[0], 255);
		m_destVertCol[i*9+1] = min(colorInfo.argb[0].g + brightObj[1], 255);
		m_destVertCol[i*9+2] = min(colorInfo.argb[0].b + brightObj[2], 255);

		vt.MatrixMult(vert[curface.vertindex[1]], nodeTM);
		ot.x = m_master->GetPos().x + vt.x;
		ot.y = m_master->GetPos().y + vt.y;
		ot.z = m_master->GetPos().z + vt.z;
		pGround->GetBrightObjVal(ot, brightObj);

		m_destVertCol[i*9+3] = min(colorInfo.argb[1].r + brightObj[0], 255);
		m_destVertCol[i*9+4] = min(colorInfo.argb[1].g + brightObj[1], 255);
		m_destVertCol[i*9+5] = min(colorInfo.argb[1].b + brightObj[2], 255);

		vt.MatrixMult(vert[curface.vertindex[2]], nodeTM);
		ot.x = m_master->GetPos().x + vt.x;
		ot.y = m_master->GetPos().y + vt.y;
		ot.z = m_master->GetPos().z + vt.z;
		pGround->GetBrightObjVal(ot, brightObj);

		m_destVertCol[i*9+6] = min(colorInfo.argb[2].r + brightObj[0], 255);
		m_destVertCol[i*9+7] = min(colorInfo.argb[2].g + brightObj[1], 255);
		m_destVertCol[i*9+8] = min(colorInfo.argb[2].b + brightObj[2], 255);
	}
}

C3dActor :: C3dActor()
{
	m_pos.Set(0, 0, 0);
	m_rot.Set(0, 0, 0);
	m_scale.Set(1, 1, 1);
	m_shadeType = 0;
	m_node = NULL;

	m_curMotion = 0;
	m_animType = 2;
	m_animSpeed = 0;
	m_animLen = 0;

	m_renderSignature = 0;
	m_isMatrixNeedUpdate = true;
	m_isBbNeedUpdate = true;
	m_posOffset.Set(0, 0, 0);
	m_isHalfAlpha = FALSE;
	m_fadeAlphaCnt = 255;



}

C3dActor :: ~C3dActor()
{
	Reset();
}

void	C3dActor :: Reset()
{



	if (m_node) {
		delete m_node;
		m_node = NULL;
	}
	std::list<CVolumeBox *>::iterator j = m_volumeBoxList.begin();
	while (j!=m_volumeBoxList.end()) {
		delete *j;
		j++;
	}
	m_volumeBoxList.erase(m_volumeBoxList.begin(), m_volumeBoxList.end());
}

void	C3dActor :: SetOpacity(float opacity)
{
}

void	C3dActor :: SetVolumeBoxInfo(std::list<CVolumeBox *> *volBoxList)
{
	std::list<CVolumeBox *>::iterator j = volBoxList->begin();
	while (j!=volBoxList->end()) {
		CVolumeBox *newBox = new CVolumeBox;
		memcpy(newBox, *j, sizeof(CVolumeBox));
		m_volumeBoxList.push_back(newBox);
		j++;
	}
}

void	C3dActor :: UpdateMatrix()
{

	m_wtm.MakeTranslate(m_posOffset);
	m_wtm.AppendScale(m_scale.x, m_scale.y, m_scale.z);
	m_wtm.AppendYRotation(DegToRad(m_rot.y));
	m_wtm.AppendXRotation(DegToRad(m_rot.x));
	m_wtm.AppendZRotation(DegToRad(m_rot.z));
	m_wtm.AppendTranslate(m_pos);
	m_iwtm.MakeInverse(m_wtm);
	m_isMatrixNeedUpdate = false;
}

void	C3dActor :: SetFrame(int framepos)
{
	m_node->SetFrame(framepos);
	m_isMatrixNeedUpdate = true;
}

void	C3dActor :: OnProcess()
{
	int oldMotion = m_curMotion;

	switch(m_animType)
	{
	case 0:

		break;

	case 1:
		SetFrame(m_curMotion);
		m_curMotion += (m_animSpeed * 100);
		if(m_curMotion >= m_animLen) m_curMotion = oldMotion;
		break;

	case 2:
		SetFrame(m_curMotion);
		m_curMotion += (m_animSpeed * 100);

		if (!m_animLen)
		{
			m_animLen = 1;
		}

		if(m_curMotion >= m_animLen) m_curMotion = m_curMotion % m_animLen;

		break;
	}
}

void	C3dActor :: Render(matrix &ctm, bool clip, int flagRF)
{



	bool	flip = false;
	if (m_scale.x < 0)
		flip = !flip;
	if (m_scale.y < 0)
		flip = !flip;
	if (m_scale.z < 0)
		flip = !flip;
	if (m_isMatrixNeedUpdate)
		UpdateMatrix();

	matrix		objectTM;
	objectTM.MultMatrix(m_wtm, ctm);

	m_node->Render(objectTM, flip, clip);

	m_node->m_master->ProcessFadeAlpha();
	m_node->m_master->HalfAlphaOff();
}

void	C3dActor :: AssignModel(C3dModelRes *model)
{

	m_node = new C3dNode;

	m_node->m_master = this;
	sAssert(model->GetFirstNode());
	m_shadeType = model->m_shadeType;
	m_animLen = model->m_animLen;






	m_node->AssignModel(model->GetFirstNode());
	SetFrame(0);

	UpdateBound();
}

void	C3dActor :: UpdateBound()
{

	C3dAABB bv;
	matrix	i;

	bv.min.Set(100000, 100000, 100000);
	bv.max.Set(-100000, -100000, -100000);
	i.MakeIdentity();
	m_node->UpdateBound(&bv, i);


	m_posOffset.Set(-(bv.min.x+bv.max.x)/2, -(bv.max.y), -(bv.min.z+bv.max.z)/2);


	m_wtm.MakeTranslate(m_posOffset);
	m_wtm.AppendScale(m_scale.x, m_scale.y, m_scale.z);
	m_wtm.AppendYRotation(DegToRad(m_rot.y));
	m_wtm.AppendXRotation(DegToRad(m_rot.x));
	m_wtm.AppendZRotation(DegToRad(m_rot.z));
	m_wtm.AppendTranslate(m_pos);

	m_node->UpdateAABB(i, m_wtm);



	vector3d	tempvert[8];

	tempvert[0].Set(bv.min.x, bv.min.y, bv.min.z);
	tempvert[1].Set(bv.max.x, bv.min.y, bv.min.z);
	tempvert[2].Set(bv.min.x, bv.max.y, bv.min.z);
	tempvert[3].Set(bv.max.x, bv.max.y, bv.min.z);
	tempvert[4].Set(bv.min.x, bv.min.y, bv.max.z);
	tempvert[5].Set(bv.max.x, bv.min.y, bv.max.z);
	tempvert[6].Set(bv.min.x, bv.max.y, bv.max.z);

	m_aaBoundingBox.min.Set(100000, 100000, 100000);
	m_aaBoundingBox.max.Set(-100000, -100000, -100000);
	tempvert[7].Set(bv.max.x, bv.max.y, bv.max.z);
	for (int j=0; j<8; j++)
	{
		m_oBoundingBox.vertices[j].MatrixMult(tempvert[j], m_wtm);
		if (m_aaBoundingBox.min.x > m_oBoundingBox.vertices[j].x)
			m_aaBoundingBox.min.x = m_oBoundingBox.vertices[j].x;
		if (m_aaBoundingBox.min.y > m_oBoundingBox.vertices[j].y)
			m_aaBoundingBox.min.y = m_oBoundingBox.vertices[j].y;
		if (m_aaBoundingBox.min.z > m_oBoundingBox.vertices[j].z)
			m_aaBoundingBox.min.z = m_oBoundingBox.vertices[j].z;
		if (m_aaBoundingBox.max.x < m_oBoundingBox.vertices[j].x)
			m_aaBoundingBox.max.x = m_oBoundingBox.vertices[j].x;
		if (m_aaBoundingBox.max.y < m_oBoundingBox.vertices[j].y)
			m_aaBoundingBox.max.y = m_oBoundingBox.vertices[j].y;
		if (m_aaBoundingBox.max.z < m_oBoundingBox.vertices[j].z)
			m_aaBoundingBox.max.z = m_oBoundingBox.vertices[j].z;
	}





	vector3d	tempmin, tempmax;

	tempmin.MatrixMult(bv.min, m_wtm);
	tempmax.MatrixMult(bv.max, m_wtm);
	m_oBoundingBox.center.Set((tempmin.x+tempmax.x)/2, (tempmin.y+tempmax.y)/2, (tempmin.z+tempmax.z)/2);



	vector3d	sizeInOCS;
	sizeInOCS.Set(bv.max.x - bv.min.x, bv.max.y - bv.min.y, bv.max.z - bv.min.z);
	m_oBoundingBox.halfSize.Set(fabs(sizeInOCS.x * m_scale.x)/2, fabs(sizeInOCS.y * m_scale.y)/2, fabs(sizeInOCS.z * m_scale.z)/2);



	matrix	bbOrientation;

	bbOrientation.MakeScale(m_scale.x, m_scale.y, m_scale.z);
	bbOrientation.AppendYRotation(DegToRad(m_rot.y));
	bbOrientation.AppendXRotation(DegToRad(m_rot.x));
	bbOrientation.AppendZRotation(DegToRad(m_rot.z));

	m_oBoundingBox.u.Set(bbOrientation.v11, bbOrientation.v12, bbOrientation.v13);
	m_oBoundingBox.u.Normalize();
	m_oBoundingBox.v.Set(bbOrientation.v21, bbOrientation.v22, bbOrientation.v23);
	m_oBoundingBox.v.Normalize();
	m_oBoundingBox.w.Set(bbOrientation.v31, bbOrientation.v32, bbOrientation.v33);
	m_oBoundingBox.w.Normalize();


	std::list<CVolumeBox *>::iterator vi = m_volumeBoxList.begin();
	while (vi != m_volumeBoxList.end()) {
		CVolumeBox *v = *vi;

		v->m_vtm.MakeXRotation(DegToRad(v->m_rot.x));
		v->m_vtm.AppendYRotation(DegToRad(v->m_rot.y));
		v->m_vtm.AppendZRotation(DegToRad(v->m_rot.z));
		v->m_vtm.AppendTranslate(v->m_pos);
		v->m_ivtm.MakeInverse(v->m_vtm);


		matrix		tmp;
		tmp.MultMatrix(v->m_vtm, m_wtm);


		v->m_worldVert[0].MatrixMult(vector3d(-v->m_size.x, -v->m_size.y, -v->m_size.z), tmp);
		v->m_worldVert[1].MatrixMult(vector3d( v->m_size.x, -v->m_size.y, -v->m_size.z), tmp);
		v->m_worldVert[2].MatrixMult(vector3d( v->m_size.x,  v->m_size.y, -v->m_size.z), tmp);
		v->m_worldVert[3].MatrixMult(vector3d(-v->m_size.x,  v->m_size.y, -v->m_size.z), tmp);
		v->m_worldVert[4].MatrixMult(vector3d(-v->m_size.x, -v->m_size.y,  v->m_size.z), tmp);
		v->m_worldVert[5].MatrixMult(vector3d( v->m_size.x, -v->m_size.y,  v->m_size.z), tmp);
		v->m_worldVert[6].MatrixMult(vector3d( v->m_size.x,  v->m_size.y,  v->m_size.z), tmp);
		v->m_worldVert[7].MatrixMult(vector3d(-v->m_size.x,  v->m_size.y,  v->m_size.z), tmp);
		++vi;
	}
	m_isMatrixNeedUpdate = true;
}

void	C3dActor :: UpdateVertexColor(const vector3d &lightDir, const vector3d &diffuseCol, const vector3d &ambientCol)
{
	if (m_isMatrixNeedUpdate)
		UpdateMatrix();

	m_node->UpdateVertexColor(m_wtm, lightDir, diffuseCol, ambientCol, m_shadeType);
}

int		C3dActor :: CheckCollision(const vector3d &wpos)
{
	vector3d	lpos;

	if (m_isMatrixNeedUpdate)
		UpdateMatrix();
	lpos.MatrixMult(wpos, m_iwtm);
	std::list<CVolumeBox *>::iterator i = m_volumeBoxList.begin();
	while (i != m_volumeBoxList.end()) {
		CVolumeBox *v = *i;

		vector3d	vpos;
		vpos.MatrixMult(lpos, v->m_ivtm);

		if (vpos.x < -v->m_size.x)
			goto skip;
		if (vpos.x > v->m_size.x)
			goto skip;
		if (vpos.y < -v->m_size.y)
			goto skip;
		if (vpos.y > v->m_size.y)
			goto skip;
		if (vpos.z < -v->m_size.z)
			goto skip;
		if (vpos.z > v->m_size.z)
			goto skip;
		return	1;
skip:
		i++;
	}
	return	0;
}

void	C3dActor :: AdjustCollision(vector3d &wpos)
{
	int		xd, yd, zd;
	vector3d	lpos;

	if (m_isMatrixNeedUpdate)
		UpdateMatrix();
	lpos.MatrixMult(wpos, m_iwtm);
	std::list<CVolumeBox *>::iterator i = m_volumeBoxList.begin();
	while (i != m_volumeBoxList.end()) {
		CVolumeBox *v = *i;

		vector3d	owpos = wpos;
		vector3d	vpos;
		vpos.MatrixMult(lpos, v->m_ivtm);

		if (vpos.x < -v->m_size.x)
			goto skip;
		if (vpos.x > v->m_size.x)
			goto skip;
		if (vpos.y < -v->m_size.y)
			goto skip;
		if (vpos.y > v->m_size.y)
			goto skip;
		if (vpos.z < -v->m_size.z)
			goto skip;
		if (vpos.z > v->m_size.z)
			goto skip;

		if (v->flag) {
			yd = abs(v->m_size.y) - abs(vpos.y);

			vpos.y = -v->m_size.y-0.5f;
		} else {

			xd = abs(v->m_size.x) - abs(vpos.x);
			zd = abs(v->m_size.z) - abs(vpos.z);


			if (xd<zd) {
				if (vpos.x<0)
					vpos.x = -v->m_size.x-0.5f;
				else
					vpos.x = v->m_size.x+0.5f;
			} else {
				if (vpos.z<0)
					vpos.z = -v->m_size.z-0.5f;
				else
					vpos.z = v->m_size.z+0.5f;
			}
		}


		lpos.MatrixMult(vpos, v->m_vtm);


		wpos.MatrixMult(lpos, m_wtm);
		if (v->flag) {
			wpos.x = owpos.x;
			wpos.z = owpos.z;
		}
		return;
skip:
		i++;
	}
}


static	float	interpolate(float c1, float c2, float c, float v1, float v2)
{
	float	t = (c-c1)/(c2-c1);
	return	v1*(1-t)+v2*t;
}

BOOL	InsideLine(float x1, float y1, float x2, float y2, float x, float y)
{
	float	ax, ay, bx, by;

	ax=x2-x1;
	ay=y2-y1;
	bx=x-x1;
	by=y-y1;
	if (ay*bx-ax*by > 0)
		return	true;
	else
		return	false;
}

static	const int	faceList[12][3] = {
	{ 0, 1, 2 },
	{ 2, 3, 0 },
	{ 5, 4, 7 },
	{ 7, 6, 5 },
	{ 1, 5, 6 },
	{ 6, 2, 1 },
	{ 4, 0, 3 },
	{ 3, 7, 4 },
	{ 4, 5, 1 },
	{ 1, 0, 4 },
	{ 3, 2, 6 },
	{ 6, 7, 3 },
};



void	C3dActor :: CalcShadowHeight(const vector3d &wpos, float &baseHeight)
{
	float	x = wpos.x;
	float	z = wpos.z;

	int j;
	vector3d	worldVert[8];
	bool	flip = false;
	if (m_scale.x < 0)
		flip = !flip;
	if (m_scale.y < 0)
		flip = !flip;
	if (m_scale.z < 0)
		flip = !flip;

	if (m_isMatrixNeedUpdate)
		UpdateMatrix();

	std::list<CVolumeBox *>::iterator i = m_volumeBoxList.begin();
	while (i != m_volumeBoxList.end()) {
		CVolumeBox *v = *i;


		for (j=0; j<12; j++) {
			int		tfv1;
			int		tfv2;
			int		tfv3;
			if (flip) {
				tfv1 = faceList[j][0];
				tfv2 = faceList[j][2];
				tfv3 = faceList[j][1];
			} else {
				tfv1 = faceList[j][0];
				tfv2 = faceList[j][1];
				tfv3 = faceList[j][2];
			}
			vector3d	&v1 = v->m_worldVert[tfv1];
			vector3d	&v2 = v->m_worldVert[tfv2];
			vector3d	&v3 = v->m_worldVert[tfv3];
			if (InsideLine(v1.x, v1.z, v2.x, v2.z, x, z) &&
				InsideLine(v2.x, v2.z, v3.x, v3.z, x, z) &&
				InsideLine(v3.x, v3.z, v1.x, v1.z, x, z)) {

				float	height;
				vector3d *	top;
				vector3d *	mid;
				vector3d *	btm;

				if (v1.z < v2.z) {
					if (v1.z < v3.z) {
						top = &v1;
						if (v2.z < v3.z) {
							mid = &v2;
							btm = &v3;
						} else {
							mid = &v2;
							btm = &v3;
						}
					} else {
						top = &v3;
						mid = &v1;
						btm = &v2;
					}
				} else {
					if (v2.z < v3.z) {
						top = &v2;
						if (v1.z < v3.z) {
							mid = &v1;
							btm = &v3;
						} else {
							mid = &v3;
							btm = &v1;
						}
					} else {
						top = &v3;
						mid = &v2;
						btm = &v1;
					}
				}

				if (z < mid->z) {

					float	x1, y1;
					x1 = interpolate(top->z, mid->z, z, top->x, mid->x);
					y1 = interpolate(top->z, mid->z, z, top->y, mid->y);

					float	x2, y2;
					x2 = interpolate(top->z, btm->z, z, top->x, btm->x);
					y2 = interpolate(top->z, btm->z, z, top->y, btm->y);

					height = interpolate(x1, x2, x, y1, y2);
				} else {

					float	x1, y1;
					x1 = interpolate(mid->z, btm->z, z, mid->x, btm->x);
					y1 = interpolate(mid->z, btm->z, z, mid->y, btm->y);

					float	x2, y2;
					x2 = interpolate(top->z, btm->z, z, top->x, btm->x);
					y2 = interpolate(top->z, btm->z, z, top->y, btm->y);

					height = interpolate(x1, x2, x, y1, y2);
				}

				if (height<=wpos.y-1)
					continue;
				if (height<baseHeight) {
					baseHeight = height;
				}
			}
		}
		i++;
	}
}

void	C3dActor :: CullByAABB(const CRayPicker &rayPicker)
{
	m_node->CheckAABB(rayPicker);
}

void	C3dActor :: CullByOBB(const lineSegment3d &ray)
{
	if (m_isHideCheck) {
		if (CheckLineSegmentOBBIntersect(ray, m_oBoundingBox))
			m_node->SetToAlpha();
	}
}
