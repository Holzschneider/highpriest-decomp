#include "common.h"
#include "GameActor3d.h"
#include "resource/Model.h"
#include "resource/Emblem.h"
#include "Mode.h"
#include "GameMode.h"
#include "GameWorld.h"
#include "3dGround.h"
#include "resource/Attr.h"
#include "Session.h"
#include "View.h"
#include "3dlib/Texture.h"
#include "base/Res.h"
#include "RagEffectPrim.h"
#include "resource/Ground.h"

extern CModeMgr g_modeMgr;
extern CRenderer* g_renderer;
extern CSession g_session;

static unsigned int tmpShadowDotBuf[35 * 35];

C3dGrannyGameActor::C3dGrannyGameActor(char* strJobFn)
{
	int i;

	m_GrannyActorRes = NULL;
	m_Instance = NULL;
	m_isFirstProcess = TRUE;
	for (i = 0; i < 30; i++)
		m_rp[i] = NULL;

	m_RenderAlpha = 0xff;
	m_nLastActAnimation = 0;
	strcpy(m_strJobFn, strJobFn);
	m_nRenderType = 0;
	m_pCellTex = NULL;
	for (i = 0; i < 20; i++)
		m_Control[i] = NULL;

	m_emblemTex = NULL;
	m_shadowTex = g_texMgr.CreateTexture(35UL, 35UL, PF_A4R4G4B4, NULL);
	m_shadowDotList.resize(4900);
}

C3dGrannyGameActor::~C3dGrannyGameActor() {
	for (int i = 0; i < 20; i++) {
		if (m_Control[i]) {
			GrannyFreeControl(m_Control[i]);
			m_Control[i] = 0;
		}
	}
	for (int j = 0; j < 30; j++) {
		if (m_rp[j]) {
			delete m_rp[j];
			m_rp[j] = 0;
			delete m_matVer[j];
			m_matVer[j] = 0;
		}
	}
	GrannyFreeModelInstance(m_Instance);
	if (m_emblemTex) delete m_emblemTex;
	if (m_shadowTex) delete m_shadowTex;
}

void C3dGrannyGameActor::SetSprAct(int job, int sex)
{
	SetJob(job);
	SetSex(sex);

	m_GrannyActorRes = (C3dGrannyModelRes*)g_resMgr().Get(m_strJobFn, false);
	m_Instance = m_GrannyActorRes->GetModelInstance();

	for (int i = 0; i < m_GrannyActorRes->GetMeshNo(); i++) {
		m_rp[i] = new RPMesh;
		m_rp[i]->AllocVerts(m_GrannyActorRes->GetMeshIndexCount(i));
		m_matVer[i] = (granny_pnt332_vertex*)operator new(m_GrannyActorRes->GetMeshIndexCount(i) * 3 * sizeof(granny_pnt332_vertex));
		memset(m_matVer[i], 0, m_GrannyActorRes->GetMeshIndexCount(i) * 3 * sizeof(granny_pnt332_vertex));
	}

	m_fLastAniCnt = GrannyGetSystemSeconds();
	m_GameClock = 0;
	if (job == 0x49d || job == 0x504) {
		m_actorType = 1;
		m_GrannyActorRes->SetHitRad(30.0f, 30.0f);
	} else if (job == 0x2d2) {
		m_actorType = 2;
		char strEbm[260];
		g_session.GetEmblemFileName(strEbm, GetGdid(), GetEmblemVersion());
		if (!g_resMgr().IsExist(strEbm)) {
			g_modeMgr.GetCurMode()->SendMsg(MM_REQ_GUILD_EMBLEM_IMG, m_gdid, 0, 0);
			return;
		}
	} else {
		m_actorType = 0;
		return;
	}
}

bool C3dGrannyGameActor::OnProcess()
{
	ProcessMotion();
	ProcessEffectState();
	ProcessWillBeAttacked();
	bool f = ProcessState();

	if (m_roty > 360.0f) m_roty = (int)m_roty % 360;
	if (m_isFirstProcess == 1) {
		m_fCurRot = 180.0f - m_roty;
		m_isFirstProcess = false;
		return f;
	}

	int roty = m_roty;
	int targetRot = 180.0f - roty;
	if (m_fCurRot > 360.0f) m_fCurRot = (int)m_fCurRot % 360;
	if (m_fCurRot < 0.0f) m_fCurRot += 360.0f;

	int rotDiff = m_fCurRot - targetRot;
	if (rotDiff > 360 || rotDiff < -360) rotDiff %= 360;
	if (rotDiff < 0) rotDiff += 360;
	if (rotDiff > 12) {
		if (rotDiff < 180)
			m_fCurRot -= 6.0f;
		else
			m_fCurRot += 6.0f;
	}
	return f;
}

void C3dGrannyGameActor::Render(matrix& view) {
	m_shadowDotList.clear();
	ProcessAni();
	GrannyLightInfo grannyLInfo;
	grannyLInfo.eyeVec = g_Weather.GetSunWorldLoc();
	grannyLInfo.lightB = grannyLInfo.lightG = grannyLInfo.lightR = 0xc8;
	grannyLInfo.nContrast = 0x80;
	switch (m_nRenderType) {
	case 1:
		RenderNormalFast(view);
		break;
	case 2:
		RenderCell(view);
		break;
	default:
		if (m_actorType == 2)
			RenderGuildMark(view);
		else
			RenderNormal(view, &grannyLInfo);
		break;
	}
}

bool C3dGrannyGameActor::ProcessAni() {
	m_fAniCnt = GrannyGetSystemSeconds();
	float dt = GrannyGetSecondsElapsed(&m_fLastAniCnt, &m_fAniCnt) * 2.0f;
	m_GameClock += dt;
	m_GrannyActorRes->UpdateAni(m_GameClock, m_Instance);
	m_fLastAniCnt = m_fAniCnt;
	if (m_nLastActAnimation == 2 && m_Control[2]) {
		if (GrannyGetControlDurationLeft(m_Control[2]) <= 0.0) {
			m_nLastActAnimation = 0;
			m_Control[m_nLastActAnimation] =
				m_GrannyActorRes->ActAnimation(0, m_Instance, m_Control[m_nLastActAnimation], 0);
		}
	}
	return true;
}

void C3dGrannyGameActor::RenderNormal(matrix& view, GrannyLightInfo* grannyLInfo)
{
	RPMesh* rp;
	granny_pnt332_vertex* srcVert;
	int pTex = 0;
	int pFaceArr = 0;

	if (m_gid == -1)
		return;

	CWorld* world = g_modeMgr.GetGameMode()->GetWorld();

	float sinDeg = GetSin((int)m_fCurRot);
	float cosDeg = GetCos((int)m_fCurRot);
	float s1 = sinDeg * -40.0f;
	float c1 = cosDeg * -40.0f;
	float s2 = sinDeg * 40.0f;
	float c2 = cosDeg * 40.0f;
	world->m_attr->GetHeight(m_pos.x + (0.0f * cosDeg - s1), m_pos.z + (0.0f * sinDeg + c1));
	world->m_attr->GetHeight(m_pos.x + (0.0f * cosDeg - s2), m_pos.z + (0.0f * sinDeg + c2));

	matrix poseMat;
	poseMat = _IdentityMatrix;

	matrix ltm;
	if (strstr(m_strJobFn, "90"))
		ltm.MakeXRotation((pi * 0.5f));
	else
		ltm.MakeXRotation(pi);
	ltm.AppendYRotation(float(m_fCurRot + 180.0f) * pi / 180.0f);

	vector3d vec;
	vec = m_pos;
	vec.x -= 2.0f;
	vec.z -= 2.0f;
	ltm.AppendTranslate(vec);

	float x;
	float y;
	float oow;
	g_renderer->ProjectVertex(m_pos, view, &x, &y, &oow);

	matrix revView;
	revView.MakeInverse(view);

	vector3d revEyeVec;
	revEyeVec.MatrixMult(grannyLInfo->eyeVec, revView);
	revEyeVec.Normalize();

	RENDER_INFO_RECT info = { g_renderer->m_width, g_renderer->m_height, 0, 0, oow };

	unsigned int vertOffs;
	int nIndexNo;
	int i;
	for (i = 0; i < m_GrannyActorRes->GetMeshNo(); i++) {
		m_GrannyActorRes->SetMatVer(m_matVer[i]);
		srcVert = (granny_pnt332_vertex*)m_GrannyActorRes->RenderMesh(i, &pTex, &pFaceArr, &nIndexNo, &poseMat, 0);

		if (!srcVert || !pTex)
			continue;

		matrix worldMat;
		worldMat.MultMatrix(poseMat, ltm);
		matrix final;
		final.MultMatrix(worldMat, view);

		RenderEffect(i, srcVert, vec);
		unsigned short* indices = (unsigned short*)pFaceArr;
		vertOffs = 0;
		for (int j = 0; j < nIndexNo; j++) {
			rp = m_rp[i];
			for (int k = 0; k < 3; k++) {
				vector3d normal;
				tlvertex3d* verts = &rp->verts[vertOffs];
				int vertNo = indices[vertOffs++];
				granny_pnt332_vertex* src = &srcVert[vertNo];

				vec.x = src->Position[0];
				vec.y = src->Position[1];
				vec.z = src->Position[2];

				g_renderer->ProjectVertex(vec, final, verts);

				verts->tu = src->UV[0];
				verts->tv = src->UV[1];
				normal.x = src->Normal[0];
				normal.y = src->Normal[1];
				normal.z = src->Normal[2];

				float light = revEyeVec.DotProduct(normal);
				if (light > 1.0f) light = 1.0f;
				else if (light < 0.0f) light = 0.0f;
				m_nVertCol = (int)(light * grannyLInfo->nContrast + (255 - grannyLInfo->nContrast));
				int col = m_nVertCol;
				verts->color = (m_RenderAlpha << 24)
							 + (((grannyLInfo->lightR + col + 0x38) & 0xff) << 16)
							 + (((grannyLInfo->lightG + col + 0x38) & 0xff) << 8)
							 + ((grannyLInfo->lightB + col + 0x38) & 0xff);
			}
			m_shadowDotList.push_back(vec);
		}

		rp->tex = (CTexture*)pTex;
		rp->numVerts = vertOffs;
		g_renderer->AddRP(rp, 0);
		m_pTex[i] = (CTexture*)pTex;
		m_pFaceArr[i] = (void*)pFaceArr;
		m_nIndexNo[i] = nIndexNo;
		m_matPose[i] = poseMat;
	}

	if (m_GrannyActorRes->CalculateHitArea(info, view, ltm,*g_modeMgr.GetGameMode()->GetView()->GetEeyeFromVector()))
		SetRenderInfo(info, x, y);
	else
		SetTlvert(x, y);
}

void C3dGrannyGameActor::RenderBak(matrix &vtm)
{
	RPMesh* rp;
	granny_pnt332_vertex* srcVert;
	float oow;
	float y;
	RENDER_INFO_RECT info;
	int i;
	float x;
	matrix ltm;
	vector3d vec;
	CView* pView;
	matrix worldMat;
	int j;
	matrix final;

	if (!m_isVisible)
		return;
	if (m_gid == -1)
		return;

	m_nVertCol = (m_RenderAlpha << 24) + 0xffffff;

	(void)g_modeMgr.GetGameMode();
	pView = g_modeMgr.GetGameMode()->GetView();

	ltm.MakeXRotation(pi * 0.5f);
	ltm.AppendYRotation((m_fCurRot + 180.0f) * pi / 180.0f);

	vec = m_pos;
	vec.x -= 2.0f;
	vec.z -= 2.0f;
	ltm.AppendTranslate(vec);

	g_renderer->ProjectVertex(m_pos, vtm, &x, &y, &oow);

	info.left = g_renderer->m_width;
	info.top = g_renderer->m_height;
	info.right = 0;
	info.bottom = 0;
	info.oow = oow;

	for (i = 0; i < m_GrannyActorRes->GetMeshNo(); i++) {
		srcVert = (granny_pnt332_vertex*)m_GrannyActorRes->GetRigidMesh(i);
		if (!srcVert)
			srcVert = m_matVer[i];

		if (!srcVert || !m_pTex[i])
			continue;

		worldMat.MultMatrix(m_matPose[i], ltm);

		final.MultMatrix(worldMat, vtm);

		unsigned short* indices = (unsigned short*)m_pFaceArr[i];
		unsigned int vertOffs = 0;
		rp = m_rp[i];
		for (j = 0; j < m_nIndexNo[i]; j++) {
			for (int k = 0; k < 3; k++) {
				tlvertex3d* verts = &rp->verts[vertOffs];
				int vertNo = indices[vertOffs++];

				vec.x = srcVert[vertNo].Position[0];
				vec.y = srcVert[vertNo].Position[1];
				vec.z = srcVert[vertNo].Position[2];
				g_renderer->ProjectVertex(vec, final, verts);
			}
		}

		rp->tex = m_pTex[i];
		rp->numVerts = m_nIndexNo[i] * 3;
		g_renderer->AddRP(rp, 0);
	}

	if (m_GrannyActorRes->CalculateHitArea(info, vtm, ltm, *pView->GetEeyeFromVector()))
		SetRenderInfo(info, x, y);
	else
		SetTlvert(x, y);
}

void C3dGrannyGameActor::RenderNormalFast(matrix& view) {}

void C3dGrannyGameActor::RenderCell(matrix& view) {}

void C3dGrannyGameActor::RenderGuildMark(matrix& view)
{
	int pTex = 0;
	int pFaceArr = 0;

	if (m_gid == -1)
		return;

	m_nVertCol = (m_RenderAlpha << 24) + 0xffffff;

	CGameMode* gameMode = g_modeMgr.GetGameMode();

	matrix poseMat;
	poseMat = _IdentityMatrix;

	matrix ltm;
	if (strstr(m_strJobFn, "90"))
		ltm.MakeXRotation(pi * 0.5f);
	else
		ltm.MakeXRotation(pi);
	ltm.AppendYRotation((m_fCurRot + 180.0f) * pi / 180.0f);
	ltm.AppendTranslate(m_pos);

	matrix shadowMat;
	shadowMat.MakeZRotation(pi * m_fCurRot / 180.0f);

	matrix final;
	final.MultMatrix(ltm, view);

	vector3d eyeVec;
	gameMode->GetView()->GetEeyeVector(&eyeVec);
	eyeVec.MatrixMult(eyeVec, view);

	eyeVec.Normalize();

	float x;
	float y;
	float oow;
	g_renderer->ProjectVertex(m_pos, view, &x, &y, &oow);

	RENDER_INFO_RECT info;
	info.left = g_renderer->m_width;
	info.top = g_renderer->m_height;
	info.right = 0;
	info.bottom = 0;
	info.oow = oow;

	if (0) { eyeVec = eyeVec; eyeVec.Normalize(); eyeVec.Normalize(); eyeVec.Normalize(); eyeVec.Normalize(); eyeVec.Normalize(); }
	RPMesh* rp;
	unsigned int vertOffs;
	vector3d vec;
	int j;
	int i;
	int nIndexNo;
	granny_pnt332_vertex* srcVert;
	for (i = 0; i < m_GrannyActorRes->GetMeshNo(); i++) {
		m_GrannyActorRes->SetMatVer(m_matVer[i]);
		srcVert = (granny_pnt332_vertex*)m_GrannyActorRes->RenderMesh(i, &pTex, &pFaceArr, &nIndexNo, &poseMat, 0);
		if (!srcVert)
			continue;

		RenderEffect(i, srcVert, vec);
		unsigned short* indices = (unsigned short*)pFaceArr;
		vertOffs = 0;
		for (j = 0; j < nIndexNo; j++) {
			rp = m_rp[i];
			for (int k = 0; k < 3; k++) {
				int vertNo = indices[vertOffs];
				tlvertex3d* verts = &rp->verts[vertOffs++];
				granny_pnt332_vertex* src = &srcVert[vertNo];

				vec.x = src->Position[0];
				vec.y = src->Position[1];
				vec.z = src->Position[2];

				g_renderer->ProjectVertex(vec, final, verts);

				verts->tu = src->UV[0];
				verts->tv = src->UV[1];
				vector3d normal;
				normal.x = src->Normal[0];
				normal.y = src->Normal[1];
				normal.z = src->Normal[2];

				float light = eyeVec.DotProduct(normal);
				if (light > 1.0f) light = 1.0f;
				else if (light < 0.0f) light = 0.0f;
				m_nVertCol = (int)(light * 100.0f + 155.0f);
				verts->color = (m_RenderAlpha << 24) + ((m_nVertCol & 0xff) << 16) + ((m_nVertCol & 0xff) << 8) + (m_nVertCol & 0xff);
			}
		}

		vec.MatrixMult(vec, shadowMat);
		vec.x += (int)m_pos.x % 5;
		vec.z += (int)m_pos.z % 5;
		m_shadowDotList.push_back(vec);

		if (i == 1)
			rp->tex = GetEmblemTexture();
		else
			rp->tex = (CTexture*)pTex;
		rp->numVerts = vertOffs;
		if (rp->tex)
			g_renderer->AddRP(rp, 0);
	}

	if (m_GrannyActorRes->CalculateHitArea(info, view, ltm,*g_modeMgr.GetGameMode()->GetView()->GetEeyeFromVector()))
		SetRenderInfo(info, x, y);
	else
		SetTlvert(x, y);
}

CTexture* C3dGrannyGameActor::GetEmblemTexture() {
	if (!m_emblemTex) {
		m_emblemTex = g_texMgr.CreateTexture(24UL, 24UL, (PixelFormat)0, (IDirectDrawSurface7*)0);
		char strEbm[260];
		g_session.GetEmblemFileName(strEbm, GetGdid(), GetEmblemVersion());
		unsigned int dwEmblem[24 * 24];
		CEmblemRes* res = (CEmblemRes*)g_resMgr().Get(strEbm, 0);
		if (res) {
			unsigned int* src = res->GetBitmap()->m_data;
			for (int j = 0; j < 24; j++) {
				for (int i = 0; i < 24; i++) {
					dwEmblem[j * 24 + i] = src[j * 24 + i];
				}
			}
		} else {
			memset(dwEmblem, 0, sizeof(dwEmblem));
		}
		m_emblemTex->Update(0, 0, 24, 24, dwEmblem, false, 0);
	}
	return m_emblemTex;
}

void C3dGrannyGameActor::RenderEffect(int nMesh, granny_pnt332_vertex* vertArr, vector3d tv) {
	if (m_job == 0x49d) {
		if (nMesh == 28) {
			vector3d fDeltaPos;
			fDeltaPos.x = vertArr[64].Position[0];
			fDeltaPos.y = vertArr[64].Position[1];
			fDeltaPos.z = vertArr[64].Position[2];
			LaunchEffect(0x175, fDeltaPos, -m_fCurRot);
		}
	}
}


void C3dGrannyGameActor::RenderShadow(matrix& vtm, vector3d lv, bool isBakRender)
{
	CGameMode* gameMode = g_modeMgr.GetGameMode();
	float sinDeg = GetSin(int(m_fCurRot) + 90);
	float cosDeg = GetCos(int(m_fCurRot) + 90);
	int attrX;
	int attrY;
	int cellX;
	int cellY;
	int addx;
	int addy;
	vector3d vShadow;
	C3dGround* ground;
	float cos_r;
	float sin_r;
	vector2d uv[4];
	int i;
	int j;
	float v0;
	float v1;
	float u0;
	float u1;

	if (!isBakRender) {
		gameMode->GetServerCoor(m_pos.x, m_pos.z, attrX, attrY, cellX, cellY);

		float cx;
		float cy;
		gameMode->GetClientCoor(attrX, attrY, cx, cy);

		addx = int(m_pos.x - cx);
		addy = int(m_pos.z - cy);

		memset(tmpShadowDotBuf, 0, sizeof(tmpShadowDotBuf));
		std::vector<vector3d>::iterator it = m_shadowDotList.begin();
		while (it != m_shadowDotList.end()) {
			vShadow = *it;
			vShadow.x *= 0.35f;
			vShadow.y *= 0.35f;
			int dotX = int((vShadow.y * cosDeg) + (vShadow.x * sinDeg) + 17.0f);
			dotX += addx;
			int dotY = int((vShadow.x * cosDeg) - (vShadow.y * sinDeg) + 17.0f);
			dotY += addy;
			if (dotX >= 0 && dotX < 34 && dotY >= 0 && dotY < 34) {
				int idx = dotY * 35 + dotX;
				tmpShadowDotBuf[idx] = 0x5f000000;
				tmpShadowDotBuf[idx + 1] |= 0x2f000000;
				tmpShadowDotBuf[idx + 35] |= 0x2f000000;
			}
			++it;
		}

		std::vector<vector3d>& shadowDotList = m_shadowDotList;
		shadowDotList.erase(shadowDotList.begin(), shadowDotList.end());
		m_shadowTex->Update(0, 0, 35, 35, tmpShadowDotBuf, false, 0);
	}

	gameMode->GetServerCoor(m_pos.x, m_pos.z, attrX, attrY, cellX, cellY);

	ground = gameMode->GetWorld()->m_ground;
	attrX -= 2;
	attrY -= 2;

	sin_r = _sin(0.0f);
	cos_r = _cos(0.0f);

	for (i = 0; i < 5; ++i) {
		v0 = i - 2.5f;
		v1 = i + 1 - 2.5f;
		for (j = 0; j < 5; ++j) {
			u0 = j - 2.5f;
			u1 = j + 1 - 2.5f;

			uv[0].Set(v0 * sin_r + u0 * cos_r, v0 * cos_r - u0 * sin_r);
			uv[1].Set(v0 * sin_r + u1 * cos_r, v0 * cos_r - u1 * sin_r);
			uv[2].Set(v1 * sin_r + u0 * cos_r, v1 * cos_r - u0 * sin_r);
			uv[3].Set(v1 * sin_r + u1 * cos_r, v1 * cos_r - u1 * sin_r);

			uv[0].Set((uv[0].x + 2.5f) / 5.0f, (uv[0].y + 2.5f) / 5.0f);
			uv[1].Set((uv[1].x + 2.5f) / 5.0f, (uv[1].y + 2.5f) / 5.0f);
			uv[2].Set((uv[2].x + 2.5f) / 5.0f, (uv[2].y + 2.5f) / 5.0f);
			uv[3].Set((uv[3].x + 2.5f) / 5.0f, (uv[3].y + 2.5f) / 5.0f);

			ground->RenderGroundTile(vtm, attrX + j, attrY + i, 0x70ffffff, m_shadowTex, uv);
		}
	}
}

void C3dGrannyGameActor::MakeCorpse() {
	CGameMode* gm = g_modeMgr.GetGameMode();
	gm->MakeGrannyCorpse3d(m_pos.x, m_pos.y, m_pos.z, m_fCurRot, m_strJobFn);
}

void C3dGrannyGameActor::ProcessMotion() {
	if (m_stateId == 2)
		m_isMotionFinished = false;
	else
		m_isMotionFinished = true;
	if (m_stateId == 2) {
		if (m_modifyFactorOfmotionSpeed2 < 1.0f) {
			m_fAniCnt = GrannyGetSystemSeconds();
			float dt = GrannyGetSecondsElapsed(&m_fLastAniCnt, &m_fAniCnt) * 2.0f;
			m_GameClock += dt / m_modifyFactorOfmotionSpeed2;
			m_GrannyActorRes->UpdateAni(m_GameClock, m_Instance);
			m_fLastAniCnt = m_fAniCnt;
			if (m_nLastActAnimation == 2 && m_Control[2]) {
				if (GrannyGetControlDurationLeft(m_Control[2]) <= 0.0) {
					m_nLastActAnimation = 0;
					m_Control[m_nLastActAnimation] =
						m_GrannyActorRes->ActAnimation(0, m_Instance, m_Control[m_nLastActAnimation], 0);
					m_modifyFactorOfmotionSpeed2 = 1.0f;
				}
			}
		}
	}
}

void C3dGrannyGameActor::ProcessMotionWithDist() {}

void C3dGrannyGameActor::SetAction(int action, int speed, int type)
{
	m_stateId = 0;
	switch (action) {
	case 48:
		m_stateId = 4;
		break;
	case 80:
	case 88:
		m_stateId = 2;
		break;
	case 64:
		m_stateId = 3;
		break;
	case 0:
		m_stateId = 0;
		break;
	case 8:
		m_stateId = 1;
		break;
	case 24:
		m_stateId = 5;
		break;
	case 32:
		m_stateId = 11;
		break;
	case 40:
		m_stateId = 12;
		break;
	default:
		break;
	}

	m_nLastActAnimation = m_stateId;
	for (int i = 0; i < 20; i++) {
		m_GrannyActorRes->DeActAnimation(m_Control[i]);
		m_Control[i] = NULL;
	}

	m_GameClock = 0;
	if (type == 1) {
		m_Control[m_nLastActAnimation] = m_GrannyActorRes->ActAnimation(
			m_nLastActAnimation, m_Instance, m_Control[m_nLastActAnimation], 1);
	} else {
		m_Control[m_nLastActAnimation] = m_GrannyActorRes->ActAnimation(
			m_nLastActAnimation, m_Instance, m_Control[m_nLastActAnimation], 0);
	}
}

int C3dGrannyGameActor::GetAttackMotion() { return 0; }

CGrannyCorpse3d::CGrannyCorpse3d(char* strJobFn) {
	m_RenderAlpha = 0xff;
	m_Instance = 0;
	m_Control = 0;
	strcpy(m_strJobFn, strJobFn);
	OnInit();
}

CGrannyCorpse3d::~CGrannyCorpse3d() {
	OnExit();
}

void CGrannyCorpse3d::OnInit()
{
	int i;

	m_GrannyActorRes = NULL;
	for (i = 0; i < 30; i++)
		m_rp[i] = NULL;

	m_GrannyActorRes = (C3dGrannyModelRes*)g_resMgr().Get(m_strJobFn, false);
	m_Instance = m_GrannyActorRes->GetModelInstance();

	for (i = 0; i < m_GrannyActorRes->GetMeshNo(); i++) {
		m_rp[i] = new RPMesh;
		m_rp[i]->AllocVerts(m_GrannyActorRes->GetMeshIndexCount(i));
		m_matVer[i] = (granny_pnt332_vertex*)operator new(m_GrannyActorRes->GetMeshIndexCount(i) * 3 * sizeof(granny_pnt332_vertex));
		memset(m_matVer[i], 0, m_GrannyActorRes->GetMeshIndexCount(i) * 3 * sizeof(granny_pnt332_vertex));
	}

	m_fLastAniCnt = GrannyGetSystemSeconds();
	m_GameClock = 0;
	m_nStateCnt = 0;
	m_Control = m_GrannyActorRes->ActAnimation(3, m_Instance, m_Control, 1);
}

void CGrannyCorpse3d::OnExit() {
	m_GrannyActorRes->DeActAnimation(m_Control);
	for (int i = 0; i < 30; i++) {
		if (m_rp[i]) {
			delete m_rp[i];
			m_rp[i] = 0;
			delete m_matVer[i];
			m_matVer[i] = 0;
		}
	}
	GrannyFreeModelInstance(m_Instance);
}

bool CGrannyCorpse3d::OnProcess() {
	if (m_nStateCnt > 0x163) m_RenderAlpha = (unsigned char)(0x58 - (unsigned char)m_nStateCnt);

	if (m_nStateCnt++ > 0x258) return false;
	return true;
}

bool CGrannyCorpse3d::ProcessAni() {
	m_fAniCnt = GrannyGetSystemSeconds();
	float dt = GrannyGetSecondsElapsed(&m_fLastAniCnt, &m_fAniCnt) * 2.0f;
	m_GameClock += dt;
	m_GrannyActorRes->UpdateAni(m_GameClock, m_Instance);
	m_fLastAniCnt = m_fAniCnt;
	return true;
}

void CGrannyCorpse3d::Render(matrix &view)
{
	RPMesh* rp;
	int pTex = 0;
	granny_pnt332_vertex* srcVert;
	float oow;
	matrix revView;
	unsigned int vertOffs;
	int pFaceArr = 0;
	float sinDeg;
	float y;
	int i;
	float x;
	matrix ltm;
	vector3d vec;
	matrix poseMat;
	vector3d eyeVec;
	float cosDeg;
	int nIndexNo;

	ProcessAni();

	CWorld* world = g_modeMgr.GetGameMode()->GetWorld();
	sinDeg = GetSin((int)m_fCurRot);
	cosDeg = GetCos((int)m_fCurRot);
	float s1 = sinDeg * -40.0f;
	float c1 = cosDeg * -40.0f;
	float s2 = sinDeg * 40.0f;
	float c2 = cosDeg * 40.0f;
	world->m_attr->GetHeight(m_pos.x + (0.0f * cosDeg - s1), m_pos.z + (0.0f * sinDeg + c1));
	world->m_attr->GetHeight(m_pos.x + (0.0f * cosDeg - s2), m_pos.z + (0.0f * sinDeg + c2));

	poseMat = _IdentityMatrix;
	if (strstr(m_strJobFn, "90"))
		ltm.MakeXRotation(pi * 0.5f);
	else
		ltm.MakeXRotation(pi);
	ltm.AppendYRotation((m_fCurRot + 180.0f) * pi / 180.0f);
	ltm.AppendTranslate(m_pos);

	(void)g_modeMgr.GetGameMode();

	eyeVec.Set(0.0f, -300.0f, 0.0f);
	revView.MakeInverse(view);
	eyeVec.MatrixMult(eyeVec, revView);
	eyeVec.Normalize();

	g_renderer->ProjectVertex(m_pos, view, &x, &y, &oow);

	for (i = 0; i < m_GrannyActorRes->GetMeshNo(); i++) {
		m_GrannyActorRes->SetMatVer(m_matVer[i]);
		srcVert = (granny_pnt332_vertex*)m_GrannyActorRes->RenderMesh(i, &pTex, &pFaceArr, &nIndexNo, &poseMat, 0);
		if (!srcVert || !pTex)
			continue;

		matrix worldMat;
		matrix final;
		worldMat.MultMatrix(poseMat, ltm);
		final.MultMatrix(worldMat, view);

		vertOffs = 0;
		unsigned short* indices = (unsigned short*)pFaceArr;
		for (int j = 0; j < nIndexNo; j++) {
			rp = m_rp[i];
			for (int k = 0; k < 3; k++) {
				tlvertex3d* verts = &rp->verts[vertOffs];
				int vertNo = indices[vertOffs++];
				granny_pnt332_vertex* src = &srcVert[vertNo];
				vec.x = src->Position[0];
				vec.y = src->Position[1];
				vec.z = src->Position[2];
				g_renderer->ProjectVertex(vec, final, verts);
				verts->tu = src->UV[0];
				verts->tv = src->UV[1];

				vector3d normal;
				normal.x = src->Normal[0];
				normal.y = src->Normal[1];
				normal.z = src->Normal[2];

				float light = eyeVec.DotProduct(normal);
				if (light > 1.0f)
					light = 1.0f;
				else if (light < 0.0f)
					light = 0.0f;

				m_nVertCol = (int)(light * 100.0f + 155.0f);
				int col = m_nVertCol & 0xff;
				verts->color = (m_RenderAlpha << 24) + (col << 16) + (col << 8) + col;
			}
		}

		rp->tex = (CTexture*)pTex;
		rp->numVerts = vertOffs;
		g_renderer->AddRP(rp, 0);
	}
}
