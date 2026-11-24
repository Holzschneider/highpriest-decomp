#ifndef HIGHPRIEST_RESOURCE_ATTR_H
#define HIGHPRIEST_RESOURCE_ATTR_H

#include "../base/Res.h"
#include "../3dlib/Prim.h"

struct ray3d;
struct vector2d;
class C3dAttr;
struct CAttrCell;
struct tagRECT;

struct CAttrCell {
public:
	CAttrCell();
	float h1;
	float h2;
	float h3;
	float h4;
	int flag;
};

class C3dAttr : public CRes {
public:
	DECLARE_RESOURCE(C3dAttr)
	C3dAttr();
	void Create(int w, int h);
	virtual bool Load(const char* fName);
	virtual void Reset();
	float GetHeight(float x, float z);
	int GetAttr(float x, float z);
	CAttrCell* GetCell(int, int);
	float RayTest(const ray3d& ray, int x, int y);
	void SetCellInfo(int Type, int Cellx, int Celly);
	void GetWorldPos(float attrX, float attrY, vector2d& pos);
	void ConvertToCellCoor(float, float, int&, int&);
	void GetHeightMinMax(const tagRECT& area, float& topHeight, float& bottomHeight);
	int m_width;
	int m_height;
	int m_zoom;
	std::vector<CAttrCell> m_cells;
};

inline	CAttrCell *	C3dAttr :: GetCell(int x, int y)
{
	if (x<0 || x>=m_width || y<0 || y>=m_height) return NULL;

	return	&m_cells[y*m_width+x];
}

inline	void	C3dAttr :: ConvertToCellCoor(float x, float z, int &cx, int &cy)
{
	cx = (int)floor(x/m_zoom)+m_width/2;
	cy = (int)floor(z/m_zoom)+m_height/2;
}

#endif
