#ifndef HIGHPRIEST_PATHFINDER_H
#define HIGHPRIEST_PATHFINDER_H

#include <algorithm>

class C3dAttr;
class CPathFinder;
class CPathInfo;
class CPriorityQueue;

struct PathCell {
	int x, y;
	int dir;
	DWORD time;
};

struct PathNode {
	int x, y;
	PathNode* parent;
	int cost;
	int total;
	int type;
	int dir;
	enum { UNEXPLORED, OPEN, CLOSED };
};

class CPathInfo {
public:
	CPathInfo();
	int GetPos(int speedFactor, int time, float& xPos, float& yPos, int& dir);
	int GetStartCellNumber();
	float GetStartPointX();
	float GetStartPointY();
	unsigned long GetTotalExpectedMovingTime();
	int GetNextCellInfo(unsigned long currentTime, unsigned long& Time, float& x, float& y);
	void GetLastCellTime(unsigned long& lastCellTime);
	void SetStartCellNumber(int num);
	void SetStartPointX(float x);
	void SetStartPointY(float x);
protected:
	void ResetStartCell();
protected:
	std::vector<PathCell> m_pathData;
	int m_startCell;
	float m_startPointX;
	float m_startPointY;
public:

	friend class CPathFinder;
};

class CPriorityQueue {
public:
	class NodeTotalGreater {
	public:
		unsigned char operator()(PathNode* a, PathNode* b) const {
			return a->total > b->total;
		}
	};

public:



	void Reset() {
		heap.erase(heap.begin(), heap.end());
	}
	void Push(PathNode* node) {
		heap.push_back(node);
		std::push_heap(heap.begin(), heap.end(), NodeTotalGreater());
	}
	PathNode* Pop() {
		PathNode* node = heap.front();
		std::pop_heap(heap.begin(), heap.end(), NodeTotalGreater());
		heap.pop_back();
		return node;
	}
	void UpdateNode(PathNode* node) {
		std::vector<PathNode *>::iterator i;
		for (i = heap.begin(); i != heap.end(); ++i) {
			if ((*i)->x == node->x && (*i)->y == node->y) {
				std::push_heap(heap.begin(), i + 1, NodeTotalGreater());
				return;
			}
		}
	}
	unsigned char IsEmpty() { return heap.empty(); }
private:
	std::vector<PathNode *> heap;
};

const int MAX_PATHNODE = 150;


inline void TransformFloatPosCellPos(float fPos, int& cell, int& subcell) {
	if (fPos <= 0) {
		cell = 0;
		subcell = 0;
		return;
	}

	cell = int(fPos + 0.5);
	subcell = int(float(fPos + 0.5 - cell) * 16);
}

inline void TransformCellPosFloatPos(float& fPos, int cell, int subcell) {
	if (cell <= 0) {
		fPos = float(subcell) / 16;
		return;
	}

	fPos = int(cell - 0.5 + float(subcell) / 16);
}


class CPathFinder {
public:
	CPathFinder();
	~CPathFinder();
	void Reset();
	void SetMap(C3dAttr* newMap);
	bool FindPath(unsigned long startTime, int sx, int sy, int xM, int yM, int dx, int dy, int speedFactor, CPathInfo* pathInfo);
private:
	int GetHeuristicCost(int, int);
	bool IsConnected(int sx, int sy, int dx, int dy);
	int GetMapWidth();
	int GetMapHeight();
	bool ProcessNode(PathNode* parent, int traverseCost, int x, int y, int dir);
	void BuildResultPath(unsigned long startTime, PathNode* goalNode, int speedFactor, CPathInfo* path);
	unsigned long GetSecondNodeArrivalTime(unsigned long startTime, CPathInfo* path, int speedFactor);
	PathNode* GetNewNode();
	PathNode* GetNode(int x, int y);
	PathNode* FindNode(int x, int y);
	void AddNode(PathNode* node) { m_masterNodes[node->x+node->y*GetMapWidth()] = node; }
private:
	C3dAttr* m_mapData;
	int m_destX;
	int m_destY;
	int m_poolCount;
	PathNode m_nodePool[150];
	std::map<int, PathNode *> m_masterNodes;
	CPriorityQueue m_openNodes;
};

inline int CPathFinder::GetHeuristicCost(int sx, int sy)
{
	return (abs(sx - m_destX) + abs(sy - m_destY)) * 10;
}

inline PathNode* CPathFinder::FindNode(int x, int y)
{
	std::map<int, PathNode *>::iterator i = m_masterNodes.find(x + y * GetMapWidth());

	if (i == m_masterNodes.end())
		return NULL;
	else
		return i->second;
}

#endif
