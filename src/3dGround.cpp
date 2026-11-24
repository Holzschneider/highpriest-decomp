#include "common.h"








#include "3dGround.h"
#include "RagEffect.h"
#include "RagEffectPrim.h"
#include "resource/Attr.h"
#include "resource/Ground.h"
#include "3dlib/Prim.h"
#include "3dlib/Texture.h"
#include <math.h>

extern CRenderer *g_renderer;
extern int g_isLightMap;
extern int g_isBlackWorld;
extern bool g_isSupportLMTex;

int g_isBlackWorld = FALSE;
vector2d g_uv_all[4] = {
	vector2d(0.0f, 0.0f),
	vector2d(1.0f, 0.0f),
	vector2d(0.0f, 1.0f),
	vector2d(1.0f, 1.0f)
};

const int lightmapWidthTexture = 256 / 8;
const int lightmapHeightTexture = 256 / 8;
const int lightmapPerTexture = lightmapWidthTexture * lightmapHeightTexture;

#ifndef LMAPWIDTH
#endif
#ifndef LMAPHEIGHT
#endif
#ifndef LMAPPF
#define LMAPPF PF_A4R4G4B4
#endif
inline FLOAT fmax(FLOAT a, FLOAT b) { return (a>b?a:b); }
inline FLOAT fmin(FLOAT a, FLOAT b) { return (a<b?a:b); }

class C3dGroundHeightDispatch {
public:
	virtual ~C3dGroundHeightDispatch();
	virtual void Create(int w, int h);
	virtual void AssignGnd(CGndRes *gnd, vector3d &lightDir, vector3d &diffuseCol, vector3d &ambientCol);
	virtual void Reset();
	virtual void FlushGround(const matrix &wtm);
	virtual void RenderLod(const matrix &wtm, const tagRECT &area, bool needClip);
	virtual void Render(const matrix &wtm, const tagRECT &area, bool needClip);
	virtual void GetHeightMinMax(int x, int y, float &topHeight, float &bottomHeight);
};


CLightmapMgr::CLightmapMgr()
{
}

CLightmapMgr::~CLightmapMgr()
{
	Reset();
}

void CLightmapMgr::Reset()
{
	m_lightmaps.clear();

	while (m_lmSurfaces.size()) { delete *m_lmSurfaces.rbegin(); m_lmSurfaces.pop_back(); }
}

void CLightmapMgr::Create(CGndRes *gnd)
{
	Reset();
	m_numLightmaps = gnd->m_numLightmap;
	m_lightmaps.resize(m_numLightmaps);

	m_numLmSurfaces = (m_numLightmaps + lightmapPerTexture-1) / lightmapPerTexture;
	m_lmSurfaces.reserve(m_numLmSurfaces);

	DWORD buffer[256][256];

	int i;
	for (i = 0; i < m_numLightmaps; i += lightmapPerTexture) {
		int surfaceId = i / lightmapPerTexture;

		for (int j = 0; j < lightmapPerTexture; j++) {
			if (i + j >= m_numLightmaps)
				break;

			CLMInfo lminfo;
			gnd->GetLMInfo(i + j, &lminfo);

			CLightmap *lmap = &m_lightmaps[i + j];

			int surfaceOffset = (i + j) - surfaceId * lightmapPerTexture;
			int u = surfaceOffset % lightmapWidthTexture;
			int v = surfaceOffset / lightmapWidthTexture;

			for (int y = 0; y < LMAPHEIGHT; y++) {
				for (int x = 0; x < LMAPWIDTH; x++) {
					buffer[y + v * LMAPHEIGHT][x + u * LMAPWIDTH] =
						PACK32BIT8888(lminfo.idata[y][x],
									  lminfo.sdata[y][x][0],
									  lminfo.sdata[y][x][1],
									  lminfo.sdata[y][x][2]);
				}
			}

			lmap->coor[0].Set((u * LMAPWIDTH + 1) / 256.0f, (v * LMAPHEIGHT + 1) / 256.0f);
			lmap->coor[1].Set(((u + 1) * LMAPWIDTH - 1) / 256.0f, (v * LMAPHEIGHT + 1) / 256.0f);
			lmap->coor[2].Set((u * LMAPWIDTH + 1) / 256.0f, ((v + 1) * LMAPHEIGHT - 1) / 256.0f);
			lmap->coor[3].Set(((u + 1) * LMAPWIDTH - 1) / 256.0f, ((v + 1) * LMAPHEIGHT - 1) / 256.0f);

			lmap->brightObj[0] = lminfo.sdata[LMAPHEIGHT >> 1][LMAPWIDTH >> 1][0];
			lmap->brightObj[1] = lminfo.sdata[LMAPHEIGHT >> 1][LMAPWIDTH >> 1][1];
			lmap->brightObj[2] = lminfo.sdata[LMAPHEIGHT >> 1][LMAPWIDTH >> 1][2];

			lmap->intensity[0].a = lminfo.idata[1][1];
			lmap->intensity[0].r = lminfo.sdata[1][1][0];
			lmap->intensity[0].g = lminfo.sdata[1][1][1];
			lmap->intensity[0].b = lminfo.sdata[1][1][2];

			lmap->intensity[1].a = lminfo.idata[1][5];
			lmap->intensity[1].r = lminfo.sdata[1][5][0];
			lmap->intensity[1].g = lminfo.sdata[1][5][1];
			lmap->intensity[1].b = lminfo.sdata[1][5][2];

			lmap->intensity[2].a = lminfo.idata[5][1];
			lmap->intensity[2].r = lminfo.sdata[5][1][0];
			lmap->intensity[2].g = lminfo.sdata[5][1][1];
			lmap->intensity[2].b = lminfo.sdata[5][1][2];

			lmap->intensity[3].a = lminfo.idata[5][5];
			lmap->intensity[3].r = lminfo.sdata[5][5][0];
			lmap->intensity[3].g = lminfo.sdata[5][5][1];
			lmap->intensity[3].b = lminfo.sdata[5][5][2];
		}

		m_lmSurfaces.push_back(g_texMgr.CreateTexture(256, 256, &(buffer[0][0]), LMAPPF, false));
	}

	for (i = 0; i < m_numLightmaps; i++) {
		m_lightmaps[i].surface = m_lmSurfaces[i / lightmapPerTexture];
	}
}


C3dGround::C3dGround()
{
	m_zoom         = 10;
	m_waterLevel   = 0;
	m_texAnimCycle = 3;
	m_wavePitch    = 50;
	m_waveSpeed    = 2;
	m_waveHeight   = 1;
	m_waterSet     = 0;
	m_waterTex     = NULL;
	m_attr         = NULL;
	m_pBumpMap     = NULL;
	m_isNewVer     = FALSE;
}







void C3dGround::SetWaterInfo(float waterLevel, int textureType, int textureAnimSpeed, int pitch, int speed, float height)
{
	m_waterLevel   = waterLevel;
	m_texAnimCycle = textureAnimSpeed;
	m_wavePitch    = pitch;
	m_waveSpeed    = speed % 360;
	m_waterSet     = textureType;
	m_waveHeight   = height;
}

void C3dGround::ConvertToCellCoor(float x, float z, int &cx, int &cy)
{
	cx = (int)floor(x / m_zoom) + m_width / 2;
	cy = (int)floor(z / m_zoom) + m_height / 2;
}

CTexture* C3dGround::GetWaterTexture()
{
	if (!m_waterTex) UpdateWater();

	return m_waterTex;
}

bool C3dGround::GetGroundVertics(vector3d *nwvert, int attrX, int attrY)
{
	CAttrCell *attrCell = m_attr->GetCell(attrX, attrY);
	if (!attrCell) return false;

	nwvert[0].y = attrCell->h1;
	nwvert[1].y = attrCell->h2;
	nwvert[2].y = attrCell->h3;
	nwvert[3].y = attrCell->h4;
	return true;
}

void C3dGround::UpdateWater()
{
	char waterName[20];
	m_waterCnt++;
	if (m_waterCnt >= 32 * m_texAnimCycle)
		m_waterCnt = 0;

	sprintf(waterName, "\xbf\xf6\xc5\xcd\\water%d%02d.jpg", m_waterSet, (m_waterCnt / m_texAnimCycle) % 32);

	m_waterTex = g_texMgr.GetTexture(waterName, false);

	if (!m_waterTex) {
		sprintf(waterName, "\xbf\xf6\xc5\xcd\\water%d%02d.jpg", m_waterSet % 6, (m_waterCnt / m_texAnimCycle) % 32);
		m_waterTex = g_texMgr.GetTexture(waterName, false);
	}

	m_waterOffset += m_waveSpeed;
	if (m_waterOffset > 180) m_waterOffset -= 360;
}


void C3dGround::RenderGroundTile(const matrix& wtm, int attrX, int attrY, unsigned int color, CTexture* tex, vector2d* uv)
{
	vector3d nwvert[4];

	if (GetGroundVertics(nwvert, attrX, attrY) == false)
		return;

	int xOff = attrX % 2;
	int zOff = attrY % 2;

	int k;
	for (k = 0; k < 4; k++) {
		vector3d& wvert = nwvert[k];
		if (xOff) {
			if (wvert.x < nwvert[1].x) {
				wvert.x += (nwvert[1].x - wvert.x) / 2;
			}
		} else {
			if (wvert.x > nwvert[0].x) {
				wvert.x -= (wvert.x - nwvert[0].x) / 2;
			}
		}

		if (zOff) {
			if (wvert.z < nwvert[2].z) {
				wvert.z += (nwvert[2].z - wvert.z) / 2;
			}
		} else {
			if (wvert.z > nwvert[0].z) {
				wvert.z -= (wvert.z - nwvert[0].z) / 2;
			}
		}
	}

	RPQuadFace& rp = g_renderer->BorrowQuadRP();

	for (k = 0; k < 4; k++) {
		g_renderer->ProjectVertex(nwvert[k], wtm, &rp.verts[k]);
		rp.verts[k].z -= 0.000030517578125f;
	}

	float uAdjust = tex->GetUAdjust();
	float vAdjust = tex->GetVAdjust();

	rp.verts[0].tu = uv[0].x * uAdjust;
	rp.verts[0].tv = uv[0].y * vAdjust;
	rp.verts[1].tu = uv[1].x * uAdjust;
	rp.verts[1].tv = uv[1].y * vAdjust;
	rp.verts[2].tu = uv[2].x * uAdjust;
	rp.verts[2].tv = uv[2].y * vAdjust;
	rp.verts[3].tu = uv[3].x * uAdjust;
	rp.verts[3].tv = uv[3].y * vAdjust;
	rp.srcAlphaMode = D3DBLEND_SRCALPHA;
	rp.destAlphaMode = D3DBLEND_INVSRCALPHA;
	rp.tex = tex;

	rp.verts[0].color = color;
	rp.verts[1].color = color;
	rp.verts[2].color = color;
	rp.verts[3].color = color;

	g_renderer->AddRP(&rp, RF_ALPHA);
}


void C3dGround15::Reset()
{
	m_V15surfaces.clear();
	m_lmRGBIntensities.clear();
	m_V15Cells.clear();
}

void C3dGround15::Create(int w, int h)
{
	Reset();

	m_width  = w;
	m_height = h;

	LightmapRGBIntensity lmIntensity;
	lmIntensity.rIntensity = 1.0f;
	lmIntensity.gIntensity = 1.0f;
	lmIntensity.bIntensity = 1.0f;

	m_V15Cells.resize(w * h);
	m_lmRGBIntensities.resize(w * 2 * h * 2, lmIntensity);

	m_waterCnt    = 0;
	m_waterOffset = 0;
}



void C3dGround15::AssignGnd(CGndRes *gnd, vector3d &light, vector3d &diffuseCol, vector3d &ambientCol)
{
	int i, j, k;

	m_isNewVer = gnd->m_newVer;
	Create(gnd->m_width, gnd->m_height);

	CellNormal *faceNormal = new CellNormal[gnd->m_width * gnd->m_height];
	m_zoom = gnd->m_zoom;

	if (g_isLightMap) m_lightmapMgr.Create(gnd);

	m_V15surfaces.clear();
	m_V15surfaces.resize(gnd->m_numSurface);

	for (i = 0; i < m_height; i++) {
		for (j = 0; j < m_width; j++) {
			const GndCellFmt17 &fileCell = gnd->GetCells(i * m_width + j);
			CellNormal &normal = faceNormal[i * m_width + j];
			CGndCell &memCell = *GetCell15(j, i);

			memCell.h[0] = fileCell.height[0];
			memCell.h[1] = fileCell.height[1];
			memCell.h[2] = fileCell.height[2];
			memCell.h[3] = fileCell.height[3];

			memCell.watervert[0].Set(float(j - m_width / 2) * m_zoom, 0, float(i - m_height / 2) * m_zoom);
			memCell.watervert[1].Set(float(j + 1 - m_width / 2) * m_zoom, 0, float(i - m_height / 2) * m_zoom);
			memCell.watervert[2].Set(float(j - m_width / 2) * m_zoom, 0, float(i + 1 - m_height / 2) * m_zoom);
			memCell.watervert[3].Set(float(j + 1 - m_width / 2) * m_zoom, 0, float(i + 1 - m_height / 2) * m_zoom);

			GndSurfaceFmt *fmt = NULL;

			if (!(fileCell.topSurfaceId < 0) && fileCell.topSurfaceId < gnd->m_numSurface) {
				CGndSurface *gndSurface = &m_V15surfaces[fileCell.topSurfaceId];
				fmt = gnd->GetSurface(fileCell.topSurfaceId);
				gndSurface->argb = fmt->argb;
				gndSurface->tex = g_texMgr.GetTexture(gnd->GetTextureName(fmt->textureId), false);

				gndSurface->vertex[0].wvert.Set(float(j - m_width / 2) * m_zoom, memCell.h[0], float(i - m_height / 2) * m_zoom);
				gndSurface->vertex[1].wvert.Set(float(j + 1 - m_width / 2) * m_zoom, memCell.h[1], float(i - m_height / 2) * m_zoom);
				gndSurface->vertex[2].wvert.Set(float(j - m_width / 2) * m_zoom, memCell.h[2], float(i + 1 - m_height / 2) * m_zoom);
				gndSurface->vertex[3].wvert.Set(float(j + 1 - m_width / 2) * m_zoom, memCell.h[3], float(i + 1 - m_height / 2) * m_zoom);

				float uAdjust = gndSurface->tex->GetUAdjust();
				float vAdjust = gndSurface->tex->GetVAdjust();
				gndSurface->vertex[0].uv.Set(fmt->u[0] * uAdjust, fmt->v[0] * vAdjust);
				gndSurface->vertex[1].uv.Set(fmt->u[1] * uAdjust, fmt->v[1] * vAdjust);
				gndSurface->vertex[2].uv.Set(fmt->u[2] * uAdjust, fmt->v[2] * vAdjust);
				gndSurface->vertex[3].uv.Set(fmt->u[3] * uAdjust, fmt->v[3] * vAdjust);

				if (g_isLightMap) {
					gndSurface->lmap = &(m_lightmapMgr.m_lightmaps[fmt->lightmapId]);
					gndSurface->lmtex = gndSurface->lmap->surface;
					gndSurface->vertex[0].uv2.Set(gndSurface->lmap->coor[0].u, gndSurface->lmap->coor[0].v);
					gndSurface->vertex[1].uv2.Set(gndSurface->lmap->coor[1].u, gndSurface->lmap->coor[1].v);
					gndSurface->vertex[2].uv2.Set(gndSurface->lmap->coor[2].u, gndSurface->lmap->coor[2].v);
					gndSurface->vertex[3].uv2.Set(gndSurface->lmap->coor[3].u, gndSurface->lmap->coor[3].v);

					memCell.cellColor[0] = min((gndSurface->lmap->brightObj[0] << 1), 255);
					memCell.cellColor[1] = min((gndSurface->lmap->brightObj[1] << 1), 255);
					memCell.cellColor[2] = min((gndSurface->lmap->brightObj[2] << 1), 255);
				} else {
					gndSurface->lmtex = NULL;
				}

				vector3d v[4];
				v[0].Set(0, memCell.h[0], 0);
				v[1].Set(m_zoom, memCell.h[1], 0);
				v[2].Set(0, memCell.h[2], m_zoom);
				v[3].Set(m_zoom, memCell.h[3], m_zoom);
				normal.top[0].CrossProduct(v[2] - v[1], v[2] - v[0]);
				normal.top[0].Normalize();
				normal.top[1].CrossProduct(v[3] - v[1], v[3] - v[2]);
				normal.top[1].Normalize();

				memCell.top = gndSurface;
			}

			if (i < m_height - 1) {
				const GndCellFmt17 &srcd = gnd->GetCells((i + 1) * m_width + j);
				if (!(fileCell.frontSurfaceId < 0) && fileCell.frontSurfaceId < gnd->m_numSurface) {
					CGndSurface *gndSurface = &m_V15surfaces[fileCell.frontSurfaceId];
					fmt = gnd->GetSurface(fileCell.frontSurfaceId);
					gndSurface->argb = fmt->argb;
					gndSurface->tex = g_texMgr.GetTexture(gnd->GetTextureName(fmt->textureId), false);

					gndSurface->vertex[0].wvert.Set(float(j - m_width / 2) * m_zoom, fileCell.height[2], float(i + 1 - m_height / 2) * m_zoom);
					gndSurface->vertex[1].wvert.Set(float(j + 1 - m_width / 2) * m_zoom, fileCell.height[3], float(i + 1 - m_height / 2) * m_zoom);
					gndSurface->vertex[2].wvert.Set(float(j - m_width / 2) * m_zoom, srcd.height[0], float(i + 1 - m_height / 2) * m_zoom);
					gndSurface->vertex[3].wvert.Set(float(j + 1 - m_width / 2) * m_zoom, srcd.height[1], float(i + 1 - m_height / 2) * m_zoom);

					float uAdjust = gndSurface->tex->GetUAdjust();
					float vAdjust = gndSurface->tex->GetVAdjust();
					gndSurface->vertex[0].uv.Set(fmt->u[0] * uAdjust, fmt->v[0] * vAdjust);
					gndSurface->vertex[1].uv.Set(fmt->u[1] * uAdjust, fmt->v[1] * vAdjust);
					gndSurface->vertex[2].uv.Set(fmt->u[2] * uAdjust, fmt->v[2] * vAdjust);
					gndSurface->vertex[3].uv.Set(fmt->u[3] * uAdjust, fmt->v[3] * vAdjust);

					if (g_isLightMap) {
						gndSurface->lmap = &(m_lightmapMgr.m_lightmaps[fmt->lightmapId]);
						gndSurface->lmtex = gndSurface->lmap->surface;
						gndSurface->vertex[0].uv2.Set(gndSurface->lmap->coor[0].u, gndSurface->lmap->coor[0].v);
						gndSurface->vertex[1].uv2.Set(gndSurface->lmap->coor[1].u, gndSurface->lmap->coor[1].v);
						gndSurface->vertex[2].uv2.Set(gndSurface->lmap->coor[2].u, gndSurface->lmap->coor[2].v);
						gndSurface->vertex[3].uv2.Set(gndSurface->lmap->coor[3].u, gndSurface->lmap->coor[3].v);
					} else {
						gndSurface->lmtex = NULL;
					}

					if (fileCell.height[2] < srcd.height[0])
						normal.front.Set(0, 0, -1);
					else
						normal.front.Set(0, 0, 1);

					memCell.front = gndSurface;
				}
			}

			if (j < m_width - 1) {
				const GndCellFmt17 &srcr = gnd->GetCells(i * m_width + (j + 1));
				if (!(fileCell.rightSurfaceId < 0) && fileCell.rightSurfaceId < gnd->m_numSurface) {
					CGndSurface *gndSurface = &m_V15surfaces[fileCell.rightSurfaceId];
					fmt = gnd->GetSurface(fileCell.rightSurfaceId);
					gndSurface->argb = fmt->argb;
					gndSurface->tex = g_texMgr.GetTexture(gnd->GetTextureName(fmt->textureId), false);

					gndSurface->vertex[0].wvert.Set(float(j + 1 - m_width / 2) * m_zoom, fileCell.height[3], float(i + 1 - m_height / 2) * m_zoom);
					gndSurface->vertex[1].wvert.Set(float(j + 1 - m_width / 2) * m_zoom, fileCell.height[1], float(i - m_height / 2) * m_zoom);
					gndSurface->vertex[2].wvert.Set(float(j + 1 - m_width / 2) * m_zoom, srcr.height[2], float(i + 1 - m_height / 2) * m_zoom);
					gndSurface->vertex[3].wvert.Set(float(j + 1 - m_width / 2) * m_zoom, srcr.height[0], float(i - m_height / 2) * m_zoom);

					float uAdjust = gndSurface->tex->GetUAdjust();
					float vAdjust = gndSurface->tex->GetVAdjust();
					gndSurface->vertex[0].uv.Set(fmt->u[0] * uAdjust, fmt->v[0] * vAdjust);
					gndSurface->vertex[1].uv.Set(fmt->u[1] * uAdjust, fmt->v[1] * vAdjust);
					gndSurface->vertex[2].uv.Set(fmt->u[2] * uAdjust, fmt->v[2] * vAdjust);
					gndSurface->vertex[3].uv.Set(fmt->u[3] * uAdjust, fmt->v[3] * vAdjust);

					if (g_isLightMap) {
						gndSurface->lmap = &(m_lightmapMgr.m_lightmaps[fmt->lightmapId]);
						gndSurface->lmtex = gndSurface->lmap->surface;
						gndSurface->vertex[0].uv2.Set(gndSurface->lmap->coor[0].u, gndSurface->lmap->coor[0].v);
						gndSurface->vertex[1].uv2.Set(gndSurface->lmap->coor[1].u, gndSurface->lmap->coor[1].v);
						gndSurface->vertex[2].uv2.Set(gndSurface->lmap->coor[2].u, gndSurface->lmap->coor[2].v);
						gndSurface->vertex[3].uv2.Set(gndSurface->lmap->coor[3].u, gndSurface->lmap->coor[3].v);
					} else {
						gndSurface->lmtex = NULL;
					}

					memCell.renderSignature = -1;
					if (fileCell.height[2] < srcr.height[0])
						normal.right.Set(-1, 0, 0);
					else
						normal.right.Set(1, 0, 0);

					memCell.right = gndSurface;
				}
			}
		}
	}

	for (i = 0; i < m_height; i++) {
		for (j = 0; j < m_width; j++) {
			CellNormal *normals[9];
			CGndCell *srcgnds[9];

			srcgnds[0] = GetCell15(j, i);
			CGndCell *cell = srcgnds[0];
			normals[0] = &faceNormal[i * m_width + j];

			vector3d vertNormal[4];
			float lightIntensity[4];

			if (j >= 1 && i >= 1) {
				srcgnds[1] = GetCell15(j - 1, i - 1);
				normals[1] = &faceNormal[j - 1 + (i - 1) * m_width];
			} else {
				normals[1] = NULL;
				srcgnds[1] = NULL;
			}
			if (i >= 1) {
				srcgnds[2] = GetCell15(j, i - 1);
				normals[2] = &faceNormal[j + (i - 1) * m_width];
			} else {
				normals[2] = NULL;
				srcgnds[2] = NULL;
			}
			if (j < m_width - 1 && i >= 1) {
				srcgnds[3] = GetCell15(j + 1, i - 1);
				normals[3] = &faceNormal[j + 1 + (i - 1) * m_width];
			} else {
				normals[3] = NULL;
				srcgnds[3] = NULL;
			}
			if (j >= 1) {
				srcgnds[4] = GetCell15(j - 1, i);
				normals[4] = &faceNormal[j - 1 + i * m_width];
			} else {
				normals[4] = NULL;
				srcgnds[4] = NULL;
			}
			if (j < m_width - 1) {
				srcgnds[5] = GetCell15(j + 1, i);
				normals[5] = &faceNormal[j + 1 + i * m_width];
			} else {
				normals[5] = NULL;
				srcgnds[5] = NULL;
			}
			if (j >= 1 && i < m_height - 1) {
				srcgnds[6] = GetCell15(j - 1, i + 1);
				normals[6] = &faceNormal[j - 1 + (i + 1) * m_width];
			} else {
				normals[6] = NULL;
				srcgnds[6] = NULL;
			}
			if (i < m_height - 1) {
				srcgnds[7] = GetCell15(j, i + 1);
				normals[7] = &faceNormal[j + (i + 1) * m_width];
			} else {
				normals[7] = NULL;
				srcgnds[7] = NULL;
			}
			if (j < m_width - 1 && i < m_height - 1) {
				srcgnds[8] = GetCell15(j + 1, i + 1);
				normals[8] = &faceNormal[j + 1 + (i + 1) * m_width];
			} else {
				normals[8] = NULL;
				srcgnds[8] = NULL;
			}

			vertNormal[0].Set(0, 0, 0);
			vertNormal[0] += normals[0]->top[0];
			if (srcgnds[2])
				if (srcgnds[2]->h[2] == srcgnds[0]->h[0]) {
					vertNormal[0] += normals[2]->top[0];
					vertNormal[0] += normals[2]->top[1];
				}
			if (srcgnds[4])
				if (srcgnds[4]->h[1] == srcgnds[0]->h[0]) {
					vertNormal[0] += normals[4]->top[0];
					vertNormal[0] += normals[4]->top[1];
				}
			if (srcgnds[1])
				if (srcgnds[1] && (srcgnds[1]->h[3] == srcgnds[0]->h[0])) {
					vertNormal[0] += normals[1]->top[1];
				}

			vertNormal[1].Set(0, 0, 0);
			vertNormal[1] += normals[0]->top[0];
			vertNormal[1] += normals[0]->top[1];
			if (srcgnds[5])
				if (srcgnds[5]->h[0] == srcgnds[0]->h[1]) {
					vertNormal[1] += normals[5]->top[0];
				}
			if (srcgnds[2])
				if (srcgnds[2]->h[3] == srcgnds[0]->h[1]) {
					vertNormal[1] += normals[2]->top[1];
				}
			if (srcgnds[3])
				if (srcgnds[3]->h[2] == srcgnds[0]->h[1]) {
					vertNormal[1] += normals[3]->top[0];
					vertNormal[1] += normals[3]->top[1];
				}

			vertNormal[2].Set(0, 0, 0);
			vertNormal[2] += normals[0]->top[0];
			vertNormal[2] += normals[0]->top[1];
			if (srcgnds[4])
				if (srcgnds[4]->h[3] == srcgnds[0]->h[2]) {
					vertNormal[2] += normals[4]->top[1];
				}
			if (srcgnds[7])
				if (srcgnds[7]->h[0] == srcgnds[0]->h[2]) {
					vertNormal[2] += normals[7]->top[0];
				}
			if (srcgnds[6])
				if (srcgnds[6]->h[1] == srcgnds[0]->h[2]) {
					vertNormal[2] += normals[6]->top[0];
					vertNormal[2] += normals[6]->top[1];
				}

			vertNormal[3].Set(0, 0, 0);
			vertNormal[3] += normals[0]->top[0];
			if (srcgnds[5])
				if (srcgnds[5]->h[2] == srcgnds[0]->h[3]) {
					vertNormal[3] += normals[5]->top[0];
					vertNormal[3] += normals[5]->top[1];
				}
			if (srcgnds[7])
				if (srcgnds[7]->h[1] == srcgnds[0]->h[3]) {
					vertNormal[3] += normals[7]->top[0];
					vertNormal[3] += normals[7]->top[1];
				}
			if (srcgnds[8])
				if (srcgnds[8]->h[0] == srcgnds[0]->h[3]) {
					vertNormal[3] += normals[8]->top[0];
				}

			float r, g, b;
			int intr, intg, intb;

			if (cell->top) {
				for (k = 0; k < 4; k++) {
					intr = intg = intb = 0;
					vertNormal[k].Normalize();
					lightIntensity[k] = fmax(vertNormal[k].DotProduct(light), 0);
					r = fmin(ambientCol.x + lightIntensity[k] * diffuseCol.x, 1);
					g = fmin(ambientCol.y + lightIntensity[k] * diffuseCol.y, 1);
					b = fmin(ambientCol.z + lightIntensity[k] * diffuseCol.z, 1);

					COLOR color;
					color.color = 0;
					switch (k) {
					case 0:
						color = srcgnds[0]->top->argb;
						break;
					case 1:
						if (srcgnds[5] && srcgnds[5]->top) color = srcgnds[5]->top->argb;
						break;
					case 2:
						if (srcgnds[7] && srcgnds[7]->top) color = srcgnds[7]->top->argb;
						break;
					case 3:
						if (srcgnds[8] && srcgnds[8]->top) color = srcgnds[8]->top->argb;
						break;
					}
					color.a = 255;
					color.r *= r;
					color.g *= g;
					color.b *= b;
					cell->top->vertex[k].argbs = color;
				}
			}

			if (cell->front) {
				for (k = 0; k < 4; k++) {
					intr = intg = intb = 0;
					lightIntensity[k] = fmax(normals[0]->front.DotProduct(light), 0);
					r = fmin(ambientCol.x + lightIntensity[k] * diffuseCol.x, 1);
					g = fmin(ambientCol.y + lightIntensity[k] * diffuseCol.y, 1);
					b = fmin(ambientCol.z + lightIntensity[k] * diffuseCol.z, 1);

					COLOR color;
					color.color = 0;
					switch (k) {
					case 0:
					case 2:
						if (srcgnds[7] && srcgnds[7]->top) color = srcgnds[7]->top->argb;
						break;
					case 1:
					case 3:
						if (srcgnds[8] && srcgnds[8]->top) color = srcgnds[8]->top->argb;
						break;
					}
					color.a = 255;
					color.r *= r;
					color.g *= g;
					color.b *= b;
					cell->front->vertex[k].argbs = color;
				}
			}

			if (cell->right) {
				for (k = 0; k < 4; k++) {
					lightIntensity[k] = fmax(normals[0]->right.DotProduct(light), 0);
					r = fmin(ambientCol.x + lightIntensity[k] * diffuseCol.x, 1);
					g = fmin(ambientCol.y + lightIntensity[k] * diffuseCol.y, 1);
					b = fmin(ambientCol.z + lightIntensity[k] * diffuseCol.z, 1);

					COLOR color;
					color.color = 0;
					switch (k) {
					case 0:
					case 2:
						if (srcgnds[8] && srcgnds[8]->top) color = srcgnds[8]->top->argb;
						break;
					case 1:
					case 3:
						if (srcgnds[5] && srcgnds[5]->top) color = srcgnds[5]->top->argb;
						break;
					}
					color.a = 255;
					color.r *= r;
					color.g *= g;
					color.b *= b;
					cell->right->vertex[k].argbs = color;
				}
			}
		}
	}

	delete[] faceNormal;
}


#include "rag_symbol_shift.h"
void C3dGround15::Render(const matrix &wtm, const RECT &area, bool needClip)
{
	RAG_SYMBOL_SHIFT(3)
	tlvertex3d tlvert[4];
	vector3d ambient = g_Weather.GetAmbient();

	for (int i = area.top; i <= area.bottom; i++) {
		for (int j = area.left; j <= area.right; j++) {
			CGndCell &cell = m_V15Cells[i * m_width + j];

			if (cell.renderSignature == g_renderer->m_curFrame)
				continue;
			cell.renderSignature = g_renderer->m_curFrame;

			if (cell.top) {
				float waterLevel[3];
				float waterOffset;
				if (m_waveHeight != 0) {
					waterOffset = m_waterOffset + (i + j) * m_wavePitch;
					waterLevel[0] = m_waterLevel + GetSin(waterOffset) * m_waveHeight;
					waterOffset = m_waterOffset + (i + j - 1) * m_wavePitch;
					waterLevel[1] = m_waterLevel + GetSin(waterOffset) * m_waveHeight;
					waterOffset = m_waterOffset + (i + j + 1) * m_wavePitch;
					waterLevel[2] = m_waterLevel + GetSin(waterOffset) * m_waveHeight;
				} else {
					waterLevel[0] = waterLevel[1] = waterLevel[2] = m_waterLevel;
				}

				if (cell.top->vertex[0].wvert.y > waterLevel[0] || cell.top->vertex[1].wvert.y > waterLevel[0] || cell.top->vertex[2].wvert.y > waterLevel[0] || cell.top->vertex[3].wvert.y > waterLevel[0]) {
					cell.watervert[0].y = waterLevel[1];
					cell.watervert[1].y = waterLevel[0];
					cell.watervert[2].y = waterLevel[0];
					cell.watervert[3].y = waterLevel[2];
					for (int k = 0; k < 4; k++) {
						g_renderer->ProjectVertex(cell.watervert[k], wtm, &tlvert[k]);
					}

					if ((tlvert[1].x - tlvert[0].x) * (tlvert[2].y - tlvert[0].y) < (tlvert[2].x - tlvert[0].x) * (tlvert[1].y - tlvert[0].y)) {
						RPQuadFace &rp = g_renderer->BorrowQuadRP();
						rp.SetGeomInfo(0, tlvert[0]);
						rp.SetGeomInfo(1, tlvert[1]);
						rp.SetGeomInfo(2, tlvert[2]);
						rp.SetGeomInfo(3, tlvert[3]);

						if (g_isBlackWorld) {
							rp.verts[0].color = 0x90ffffff;
							rp.verts[1].color = 0x90ffffff;
							rp.verts[2].color = 0x90ffffff;
							rp.verts[3].color = 0x90ffffff;
						} else if (m_waterSet == 4) {
							rp.verts[0].color = PACK32BIT8888(0xff, (int)(0xff * ambient.x), (int)(0xff * ambient.y), (int)(0xff * ambient.z));
							rp.verts[1].color = rp.verts[0].color;
							rp.verts[2].color = rp.verts[0].color;
							rp.verts[3].color = rp.verts[0].color;
						} else {
							rp.verts[0].color = 0x90ffffff;
							rp.verts[1].color = 0x90ffffff;
							rp.verts[2].color = 0x90ffffff;
							rp.verts[3].color = 0x90ffffff;
						}

						float iofs = j % WATERTEXTURESIZE;
						float jofs = i % WATERTEXTURESIZE;
						rp.verts[0].tu = iofs * WATERTEXUV;
						rp.verts[0].tv = jofs * WATERTEXUV;
						rp.verts[1].tu = (iofs + 1) * WATERTEXUV;
						rp.verts[1].tv = jofs * WATERTEXUV;
						rp.verts[2].tu = iofs * WATERTEXUV;
						rp.verts[2].tv = (jofs + 1) * WATERTEXUV;
						rp.verts[3].tu = (iofs + 1) * WATERTEXUV;
						rp.verts[3].tv = (jofs + 1) * WATERTEXUV;
						rp.tex = GetWaterTexture();

						g_renderer->AddRP(&rp, RF_ALPHA);
					}
				}
				RenderNew(wtm, cell.top, needClip);
			}

			if (cell.front) RenderNew(wtm, cell.front, needClip);
			if (cell.right) RenderNew(wtm, cell.right, needClip);
		}
	}
}

void C3dGround15::RenderNew(const matrix &wtm, CGndSurface *surface, bool needClip)
{
	tlvertex3d tlvert[4];
	texCoor    lmcoord[4];
	vector3d   envdiff = g_Weather.GetEnvDiff();

	unsigned int k;
	for (k = 0; k < 4; ++k) {
		g_renderer->ProjectVertex(surface->vertex[k].wvert, wtm, &tlvert[k]);
	}

	if (needClip) {
		if (min(min(tlvert[0].x, tlvert[1].x), min(tlvert[2].x, tlvert[3].x)) >= g_renderer->m_width)
			return;
		if (max(max(tlvert[0].x, tlvert[1].x), max(tlvert[2].x, tlvert[3].x)) < 0)
			return;
		if (min(min(tlvert[0].y, tlvert[1].y), min(tlvert[2].y, tlvert[3].y)) >= g_renderer->m_height)
			return;
		if (max(max(tlvert[0].y, tlvert[1].y), max(tlvert[2].y, tlvert[3].y)) < 0)
			return;
	}

	if (g_isBlackWorld) {
		for (k = 0; k < 4; k++) {
			tlvert[k].tu = surface->vertex[k].uv.x;
			tlvert[k].tv = surface->vertex[k].uv.y;
			lmcoord[k].u = surface->vertex[k].uv2.x;
			lmcoord[k].v = surface->vertex[k].uv2.y;
			COLOR color  = surface->vertex[k].argbs;
			color.r *= envdiff.x;
			color.g *= envdiff.y;
			color.b *= envdiff.z;
			tlvert[k].argb = color;
		}
	} else if (surface->alpha) {
		for (k = 0; k < 4; k++) {
			tlvert[k].tu = surface->vertex[k].uv.x;
			tlvert[k].tv = surface->vertex[k].uv.y;
			lmcoord[k].u = surface->vertex[k].uv2.x;
			lmcoord[k].v = surface->vertex[k].uv2.y;

			tlvert[k].argb = surface->vertex[k].argbs;
			tlvert[k].argb.a = 128;
		}
	} else {
		for (k = 0; k < 4; k++) {
			tlvert[k].tu = surface->vertex[k].uv.x;
			tlvert[k].tv = surface->vertex[k].uv.y;
			lmcoord[k].u = surface->vertex[k].uv2.x;
			lmcoord[k].v = surface->vertex[k].uv2.y;
			COLOR color  = surface->vertex[k].argbs;

			color.r *= envdiff.x;
			color.g *= envdiff.y;
			color.b *= envdiff.z;
			tlvert[k].argb = color;
		}
	}

	if (g_isSupportLMTex && g_isLightMap) {
		RPLmQuadFace &rp = g_renderer->BorrowLmQuadRP();
		rp.lmverts[0].vert = tlvert[0];
		rp.lmverts[0].lmcoord = lmcoord[0];
		rp.lmverts[1].vert = tlvert[1];
		rp.lmverts[1].lmcoord = lmcoord[1];
		rp.lmverts[2].vert = tlvert[2];
		rp.lmverts[2].lmcoord = lmcoord[2];
		rp.lmverts[3].vert = tlvert[3];
		rp.lmverts[3].lmcoord = lmcoord[3];
		rp.tex  = surface->tex;
		rp.tex2 = surface->lmtex;
		g_renderer->AddLmRP(&rp, RF_LM_LIGHTMAP);
		return;
	} else if (g_isLightMap) {
		RPQuadFace &rp = g_renderer->BorrowQuadRP();
		memcpy(rp.verts, tlvert, sizeof(tlvertex3d) * 4);
		*(texCoor *)&rp.verts[0].tu = lmcoord[0];
		*(texCoor *)&rp.verts[1].tu = lmcoord[1];
		*(texCoor *)&rp.verts[2].tu = lmcoord[2];
		*(texCoor *)&rp.verts[3].tu = lmcoord[3];
		rp.tex = surface->lmtex;
		g_renderer->AddRP(&rp, RF_LIGHT);
	}

	RPQuadFace &rp = g_renderer->BorrowQuadRP();
	memcpy(rp.verts, tlvert, sizeof(tlvertex3d) * 4);
	rp.tex = surface->tex;
	g_renderer->AddRP(&rp, RF_GROUND);
}

void C3dGround15::RenderLod(const matrix &wtm, const RECT &area, bool needClip)
{
	for (int i=area.top; i<=area.bottom; i++) {
		for (int j=area.left; j<=area.right; j++) {
			CGndCell &cell = m_V15Cells[i * m_width + j];
			if (cell.renderSignature == g_renderer->m_curFrame) continue;
			cell.renderSignature = g_renderer->m_curFrame;

			if (cell.top) {
				RenderNewLod(wtm, cell.top, needClip);
			}

			if (cell.front) RenderNewLod(wtm, cell.front, needClip);
			if (cell.right) RenderNewLod(wtm, cell.right, needClip);
		}
	}
}

void C3dGround15::RenderNewLod(const matrix &wtm, CGndSurface *surface, bool needClip)
{
	tlvertex3d tlvert[4];
	int k;
	for (k = 0; k < 4; ++k) {
		g_renderer->ProjectVertex(surface->vertex[k].wvert, wtm, &tlvert[k]);
	}

	if (needClip) {
		if (min(min(tlvert[0].x, tlvert[1].x), min(tlvert[2].x, tlvert[3].x)) >= g_renderer->m_width)  return;
		if (max(max(tlvert[0].x, tlvert[1].x), max(tlvert[2].x, tlvert[3].x)) <  0)                    return;
		if (min(min(tlvert[0].y, tlvert[1].y), min(tlvert[2].y, tlvert[3].y)) >= g_renderer->m_height) return;
		if (max(max(tlvert[0].y, tlvert[1].y), max(tlvert[2].y, tlvert[3].y)) <  0)                    return;
	}

	for (k = 0; k < 4; k++) {
		tlvert[k].tu = surface->vertex[k].uv.x;
		tlvert[k].tv = surface->vertex[k].uv.y;
		tlvert[k].argb   = surface->vertex[k].argbs;
		tlvert[k].argb.a = 128;
	}

	RPQuadFace &rp = g_renderer->BorrowQuadRP();
	memcpy(rp.verts, tlvert, sizeof(tlvertex3d) * 4);
	rp.tex = surface->tex;
	g_renderer->AddRP(&rp, RF_GROUND);
}

void C3dGround15::FlushGround(const matrix& wtm)
{
}


void C3dGround15::GetHeightMinMax(int x, int y, float& topHeight, float& bottomHeight)
{
	if (!(x >= 0 && x < m_width && y >= 0 && y < m_height)) {
		topHeight = 0;
		bottomHeight = 0;
		return;
	}

	topHeight = 999999;
	bottomHeight = -999999;

	CGndCell* cells = &m_V15Cells[0];
	CGndCell* cell = &cells[y * m_width + x];
	if (cell->top) {
		float h[4];
		h[0] = cell->top->vertex[0].wvert.y;
		h[1] = cell->top->vertex[1].wvert.y;
		h[2] = cell->top->vertex[2].wvert.y;
		h[3] = cell->top->vertex[3].wvert.y;

		if (bottomHeight < h[0])
			bottomHeight = h[0];
		if (bottomHeight < h[1])
			bottomHeight = h[1];
		if (bottomHeight < h[2])
			bottomHeight = h[2];
		if (bottomHeight < h[3])
			bottomHeight = h[3];
		if (topHeight > h[0])
			topHeight = h[0];
		if (topHeight > h[1])
			topHeight = h[1];
		if (topHeight > h[2])
			topHeight = h[2];
		if (topHeight > h[3])
			topHeight = h[3];
	}
}


void C3dGround15::GetHeightMinMax(const tagRECT& area, float& topHeight, float& bottomHeight)
{
	bool outOfMap = false;
	if (!(area.left >= 0 && area.left < m_width))
		outOfMap = true;
	if (!(area.right >= 0 && area.right < m_width))
		outOfMap = true;
	if (!(area.top >= 0 && area.top < m_height))
		outOfMap = true;
	if (!(area.bottom >= 0 && area.bottom < m_height))
		outOfMap = true;
	if (!(area.right > area.left && area.bottom > area.top))
		outOfMap = true;

	if (outOfMap) {
		topHeight = 0;
		bottomHeight = 0;
		return;
	}

	topHeight = 999999;
	bottomHeight = -999999;

	for (int i = area.left; i <= area.right; i++) {
		for (int j = area.top; j <= area.bottom; j++) {
			CGndCell* cell = &m_V15Cells[j * m_width + i];
			if (cell->top) {
				float h[4];
				h[0] = cell->top->vertex[0].wvert.y;
				h[1] = cell->top->vertex[1].wvert.y;
				h[2] = cell->top->vertex[2].wvert.y;
				h[3] = cell->top->vertex[3].wvert.y;

				if (bottomHeight < h[0])
					bottomHeight = h[0];
				if (bottomHeight < h[1])
					bottomHeight = h[1];
				if (bottomHeight < h[2])
					bottomHeight = h[2];
				if (bottomHeight < h[3])
					bottomHeight = h[3];
				if (topHeight > h[0])
					topHeight = h[0];
				if (topHeight > h[1])
					topHeight = h[1];
				if (topHeight > h[2])
					topHeight = h[2];
				if (topHeight > h[3])
					topHeight = h[3];
			}
		}
	}
}

void C3dGround15::CheckAlpha(const ray3d &ray, int x, int y, float nearest)
{
	float u, v;
	float dist;
	if (!((x) >= 0 && (x) < m_width && (y) >= 0 && (y) < m_height)) {
		return;
	}

	CGndCell *cell = &m_V15Cells[y * m_width + x];
	if (cell->top) {
		vector3d vec[4];
		vec[0] = cell->top->vertex[0].wvert;
		vec[1] = cell->top->vertex[1].wvert;
		vec[2] = cell->top->vertex[2].wvert;
		vec[3] = cell->top->vertex[3].wvert;

		if (CheckRayTriIntersect(ray, vec[0], vec[1], vec[2], &u, &v, &dist)) {
			if (dist < nearest)
				cell->top->alpha = 1;
		}
		if (!cell->top->alpha) {
			if (CheckRayTriIntersect(ray, vec[1], vec[3], vec[2], &u, &v, &dist)) {
				if (dist < nearest)
					cell->top->alpha = 1;
			}
		}
	}
	if (cell->front) {
		vector3d vec[4];
		vec[0] = cell->front->vertex[0].wvert;
		vec[1] = cell->front->vertex[1].wvert;
		vec[2] = cell->front->vertex[2].wvert;
		vec[3] = cell->front->vertex[3].wvert;

		if (CheckRayTriIntersect(ray, vec[0], vec[1], vec[2], &u, &v, &dist)) {
			if (dist < nearest)
				cell->front->alpha = 1;
		}
		if (!cell->front->alpha) {
			if (CheckRayTriIntersect(ray, vec[1], vec[3], vec[2], &u, &v, &dist)) {
				if (dist < nearest)
					cell->front->alpha = 1;
			}
		}
	}
	if (cell->right) {
		vector3d vec[4];
		vec[0] = cell->right->vertex[0].wvert;
		vec[1] = cell->right->vertex[1].wvert;
		vec[2] = cell->right->vertex[2].wvert;
		vec[3] = cell->right->vertex[3].wvert;

		if (CheckRayTriIntersect(ray, vec[0], vec[1], vec[2], &u, &v, &dist)) {
			if (dist < nearest)
				cell->right->alpha = 1;
		}
		if (!cell->right->alpha) {
			if (CheckRayTriIntersect(ray, vec[1], vec[3], vec[2], &u, &v, &dist)) {
				if (dist < nearest)
					cell->right->alpha = 1;
			}
		}
	}
}

void C3dGround15::GetLmIntensityRGBByGatCoor(int cx, int cy, BYTE &in, BYTE &rI, BYTE &gI, BYTE &bI)
{
	if (!((cx) >= 0 && (cx / 2) < m_width && (cy) >= 0 && (cy / 2) < m_height) || m_V15Cells.empty()) {
		in = 255; rI = 255; gI = 255; bI = 255;
		return;
	}

	CGndCell *cells = &m_V15Cells[0];
	CGndCell *gndCell = &cells[(cy / 2) * m_width + (cx / 2)];

	if (gndCell->top) {
		if (gndCell->top->lmap) {
			if (cx % 2 == 0 && cy % 2 == 0) {
				in = gndCell->top->lmap->intensity[0].a;
				rI = gndCell->top->lmap->intensity[0].r;
				gI = gndCell->top->lmap->intensity[0].g;
				bI = gndCell->top->lmap->intensity[0].b;
				return;
			} else if (cx % 2 == 1 && cy % 2 == 0) {
				in = gndCell->top->lmap->intensity[1].a;
				rI = gndCell->top->lmap->intensity[1].r;
				gI = gndCell->top->lmap->intensity[1].g;
				bI = gndCell->top->lmap->intensity[1].b;
				return;
			} else if (cx % 2 == 0 && cy % 2 == 1) {
				in = gndCell->top->lmap->intensity[2].a;
				rI = gndCell->top->lmap->intensity[2].r;
				gI = gndCell->top->lmap->intensity[2].g;
				bI = gndCell->top->lmap->intensity[2].b;
				return;
			} else {
				in = gndCell->top->lmap->intensity[3].a;
				rI = gndCell->top->lmap->intensity[3].r;
				gI = gndCell->top->lmap->intensity[3].g;
				bI = gndCell->top->lmap->intensity[3].b;
				return;
			}
		}
	}

	in = 255; rI = 255; gI = 255; bI = 255;
}


void C3dGround15::SetLmIntensityFloatRGBByGatCoor(int cx, int cy, float& r, float& g, float& b)
{
	if (m_lmRGBIntensities.empty()) return;
	int index = cx + cy * m_width * 2;
	LightmapRGBIntensity& lmIntensity = m_lmRGBIntensities[index];
	lmIntensity.rIntensity = r;
	lmIntensity.gIntensity = g;
	lmIntensity.bIntensity = b;
}

int C3dGround15::GetLmIntensityFloatRGBByGatCoor(int cx, int cy, float& r, float& g, float& b)
{
	if ((!((cx) >= 0 && (cx / 2) < m_width && (cy) >= 0 && (cy / 2) < m_height)) || m_lmRGBIntensities.empty()) {
		b = g = r = 1.0f;
		return TRUE;
	}

	int index = cx + cy * m_width * 2;
	LightmapRGBIntensity& lmIntensity = m_lmRGBIntensities[index];
	r = lmIntensity.rIntensity;
	g = lmIntensity.gIntensity;
	b = lmIntensity.bIntensity;
	return TRUE;
}

static int off[5][2] = { {0,0}, {0,-1}, {0,1}, {1,0}, {-1,0}, };

void C3dGround15::CalcLmIntensityFloatRGBAll()
{
	if (g_isLightMap == FALSE) return;
	if (!IsNewVer()) return;

	for (int sy = 0; sy < m_height * 2; ++sy) {
		for (int sx = 0; sx < m_width * 2; ++sx) {
			CAttrCell *cell = m_attr->GetCell(sx, sy);
			float gatHeight = 0;
			if (cell) gatHeight = (cell->h1 + cell->h2 + cell->h3 + cell->h4) / 4.0f;

			float topHeight, bottomHeight;
			((C3dGroundHeightDispatch *)this)->GetHeightMinMax(sx / 2, sy / 2, topHeight, bottomHeight);
			bool isOnGround = (gatHeight >= topHeight - 2.5f);
			float lmRIntensity = 1.0f;
			float lmGIntensity = 1.0f;
			float lmBIntensity = 1.0f;
			if (isOnGround) {
				int totalIntensity  = 0;
				int rTotalIntensity = 0;
				int gTotalIntensity = 0;
				int bTotalIntensity = 0;

				for (int cnt = 0; cnt < 5; ++cnt) {
					BYTE in, rI, gI, bI;
					GetLmIntensityRGBByGatCoor(sx + off[cnt][0], sy + off[cnt][1], in, rI, gI, bI);
					totalIntensity  += in;
					rTotalIntensity += rI;
					gTotalIntensity += gI;
					bTotalIntensity += bI;
				}

				float lmIntensity = float(totalIntensity) / float(5) / 255.0f;
				const float div = 128.0f;
				lmRIntensity = (float(rTotalIntensity) / float(5) / div) + lmIntensity;
				lmGIntensity = (float(gTotalIntensity) / float(5) / div) + lmIntensity;
				lmBIntensity = (float(bTotalIntensity) / float(5) / div) + lmIntensity;

				float maxColorI = lmRIntensity;
				if (lmGIntensity > maxColorI)
					maxColorI = lmGIntensity;
				if (lmBIntensity > maxColorI)
					maxColorI = lmBIntensity;

				if (maxColorI > 1) {
					lmRIntensity /= maxColorI;
					lmGIntensity /= maxColorI;
					lmBIntensity /= maxColorI;
				}
			}
			SetLmIntensityFloatRGBByGatCoor(sx, sy, lmRIntensity, lmGIntensity, lmBIntensity);
		}
	}
}

void C3dGround15::GetBrightObjVal(vector3d v, int *rgb)
{
	int x, y;
	ConvertToCellCoor(v.x, v.z, x, y);
	if (x < 0 || y < 0 || x >= m_width || y >= m_height) {
		rgb[0] = rgb[1] = rgb[2] = 255;
		return;
	}
	CGndCell* arr = &m_V15Cells[0];
	CGndCell& cell = arr[y * m_width + x];
	rgb[0] = cell.cellColor[0];
	rgb[1] = cell.cellColor[1];
	rgb[2] = cell.cellColor[2];
}

bool C3dGround15::GetGroundVertics(vector3d *nwvert, int attrX, int attrY)
{
	if (attrX < 0 || attrX / 2 >= m_width || attrY < 0 || attrY / 2 >= m_height) return false;
	CGndCell *cells = &m_V15Cells[0];
	CGndSurface *surface2 = cells[(attrY / 2) * m_width + (attrX / 2)].top;

	if (!surface2) return false;

	nwvert[0] = surface2->vertex[0].wvert;
	nwvert[1] = surface2->vertex[1].wvert;
	nwvert[2] = surface2->vertex[2].wvert;
	nwvert[3] = surface2->vertex[3].wvert;

	C3dGround::GetGroundVertics(nwvert, attrX, attrY);
	return true;
}


void C3dGround15::RenderAttrTile(const matrix& wtm, int attrX, int attrY, DWORD color)
{
	if (attrX < 0 || attrX / 2 >= m_width || attrY < 0 || attrY / 2 >= m_height)
		return;

	CGndSurface *surface2 = GetCell15(attrX / 2, attrY / 2)->top;

	if (!surface2)
		return;

	RenderGroundTile(wtm, attrX, attrY, color, g_texMgr.GetTexture("grid.tga", false), g_uv_all);
}
