#include "common.h"
#include "PathFinder.h"
#include "resource/Attr.h"
#include <math.h>

CPathInfo::CPathInfo()
{
	ResetStartCell();
}

void	CPathInfo::ResetStartCell()
{
	m_startCell = 0;
}

int		CPathInfo::GetPos(int speedFactor, int time, float &xPos, float &yPos, int &dir)
{
	if (m_pathData.size() == 0 || m_pathData[0].time > time)
	{
		xPos = m_pathData[0].x;
		yPos = m_pathData[0].y;
		dir  = m_pathData[1].dir;
		return 0;
	}

	int index;
	for ( index=m_startCell; index<m_pathData.size() - 1; index++)
	{
		if (m_pathData[index].time <= time && time < m_pathData[index+1].time)
		{

			int movingTime = time - m_pathData[index].time;
			float xlen,ylen;
			if (index == 0)
			{
				xlen = m_pathData[1].x - m_startPointX;
				ylen = m_pathData[1].y - m_startPointY;
			}
			else
			{
				xlen = m_pathData[index+1].x - m_pathData[index].x;
				ylen = m_pathData[index+1].y - m_pathData[index].y;
			}

			float xMovingDistance = xlen*float(movingTime)/float(m_pathData[index+1].time - m_pathData[index].time);

			if (index == 0)
			{
				xPos = m_startPointX + xMovingDistance;
			}
			else
			{
				xPos = m_pathData[index].x + xMovingDistance;
			}

			float yMovingDistance = ylen*float(movingTime)/float(m_pathData[index+1].time - m_pathData[index].time);

			if (index == 0)
			{
				yPos = m_startPointY + yMovingDistance;
			}
			else
			{
				yPos = m_pathData[index].y + yMovingDistance;
			}

			if ( time < (m_pathData[index].time + (m_pathData[index+1].time - m_pathData[index].time)/2 ))
				dir = m_pathData[index].dir;
			else
				dir = m_pathData[index+1].dir;

			m_startCell = index;
			return index;
		}
	}

	dir  = m_pathData[index].dir;
	xPos = m_pathData[index].x;
	yPos = m_pathData[index].y;
	return -1;
}

int		CPathInfo::GetStartCellNumber()
{
	return m_startCell;
}

void	CPathInfo::SetStartPointX(float x)
{
	m_startPointX = x;
}

void	CPathInfo::SetStartPointY(float y)
{
	m_startPointY = y;
}

void	CPathInfo::SetStartCellNumber(int num)
{
	m_startCell = num;
}

float	CPathInfo::GetStartPointX()
{
	return m_startPointX;
}

float	CPathInfo::GetStartPointY()
{
	return m_startPointY;
}

DWORD	CPathInfo::GetTotalExpectedMovingTime()
{
	int size = m_pathData.size();
	return m_pathData[size-1].time - m_pathData[0].time;
}

void	CPathInfo::GetLastCellTime(DWORD & lastCellTime)
{
	lastCellTime = m_pathData[m_pathData.size()-1].time;
}

int		CPathInfo::GetNextCellInfo(DWORD currentTime, DWORD& Time, float &x, float &y)
{
	if (m_pathData[0].time == NULL)
	{
		return -1;
	}
	if (m_pathData[0].time > currentTime)
	{
		Time = m_pathData[1].time;
		x = m_pathData[1].x;
		y = m_pathData[1].y;
		return 0;
	}

	int index = 0;
	int size = m_pathData.size();
	for ( ; index<size - 1; index++)
	{
		if (m_pathData[index].time <= currentTime && currentTime < m_pathData[index+1].time)
		{
			Time = m_pathData[index+1].time;
			x = m_pathData[index+1].x;
			y = m_pathData[index+1].y;
			return index;
		}
	}

	Time = m_pathData[index].time;
	x = m_pathData[index].x;
	y = m_pathData[index].y;

	return -1;

}

CPathFinder :: CPathFinder()
{
}

CPathFinder :: ~CPathFinder()
{
	Reset();
}

void	CPathFinder :: Reset()
{
	m_masterNodes.erase(m_masterNodes.begin(), m_masterNodes.end());
	m_openNodes.Reset();
}

bool	CPathFinder :: FindPath(DWORD startTime, int sx, int sy, int xM, int yM, int dx, int dy, int speedFactor, CPathInfo *pathInfo)
{

	m_destX = dx;
	m_destY = dy;
	if (sx == dx && sy == dy)
		return false;





	pathInfo->ResetStartCell();
	Reset();
	m_poolCount = 0;
	PathNode *	startNode = GetNode(sx, sy);
	startNode->type = PathNode::OPEN;
	startNode->parent = NULL;
	startNode->cost = 0;
	startNode->total = GetHeuristicCost(sx, sy);
	m_openNodes.Push(startNode);

	while (!m_openNodes.IsEmpty()) {
		PathNode *	bestNode = m_openNodes.Pop();
		if (bestNode->x == dx && bestNode->y == dy) {
			float fPos;
			TransformCellPosFloatPos(fPos,sx,xM);
			pathInfo->SetStartPointX(fPos);
			TransformCellPosFloatPos(fPos,sy,yM);
			pathInfo->SetStartPointY(fPos);
			BuildResultPath(startTime, bestNode, speedFactor, pathInfo);
			return	true;
		}

		bool	result;
		if (IsConnected(bestNode->x, bestNode->y, bestNode->x+1, bestNode->y-1)) {
			result = ProcessNode(bestNode, 14, bestNode->x+1, bestNode->y-1, 5);
			if (!result) {
				Trace("find path aborted : buffer overflow");
				return	false;
			}
		}
		if (IsConnected(bestNode->x, bestNode->y, bestNode->x+1, bestNode->y)) {
			result = ProcessNode(bestNode, 10, bestNode->x+1, bestNode->y, 6);
			if (!result) {
				Trace("find path aborted : buffer overflow");
				return	false;
			}
		}
		if (IsConnected(bestNode->x, bestNode->y, bestNode->x+1, bestNode->y+1)) {
			result = ProcessNode(bestNode, 14, bestNode->x+1, bestNode->y+1, 7);
			if (!result) {
				Trace("find path aborted : buffer overflow");
				return	false;
			}
		}
		if (IsConnected(bestNode->x, bestNode->y, bestNode->x, bestNode->y+1)) {
			result = ProcessNode(bestNode, 10, bestNode->x, bestNode->y+1, 0);
			if (!result) {
				Trace("find path aborted : buffer overflow");
				return	false;
			}
		}
		if (IsConnected(bestNode->x, bestNode->y, bestNode->x-1, bestNode->y+1)) {
			result = ProcessNode(bestNode, 14, bestNode->x-1, bestNode->y+1, 1);
			if (!result) {
				Trace("find path aborted : buffer overflow");
				return	false;
			}
		}
		if (IsConnected(bestNode->x, bestNode->y, bestNode->x-1, bestNode->y)) {
			result = ProcessNode(bestNode, 10, bestNode->x-1, bestNode->y, 2);
			if (!result) {
				Trace("find path aborted : buffer overflow");
				return	false;
			}
		}
		if (IsConnected(bestNode->x, bestNode->y, bestNode->x-1, bestNode->y-1)) {
			result = ProcessNode(bestNode, 14, bestNode->x-1, bestNode->y-1, 3);
			if (!result) {
				Trace("find path aborted : buffer overflow");
				return	false;
			}
		}
		if (IsConnected(bestNode->x, bestNode->y, bestNode->x, bestNode->y-1)) {
			result = ProcessNode(bestNode, 10, bestNode->x, bestNode->y-1, 4);
			if (!result) {
				Trace("find path aborted : buffer overflow");
				return	false;
			}
		}
		bestNode->type = PathNode::CLOSED;
	}
	Trace("find path failed : no path found");
	return	false;
}

bool	CPathFinder :: ProcessNode(PathNode *parent, int traverseCost, int x, int y, int dir)
{
	int		newCost = parent->cost + traverseCost;
	PathNode *	node = GetNode(x, y);
	if (node == NULL)
		return	false;

	if (node->type != PathNode::UNEXPLORED) {
		if (node->cost <= newCost) {
			return	true;
		} else {
			node->parent = parent;
			node->cost = newCost;
			node->total = newCost + GetHeuristicCost(x, y);
			node->dir = dir;
			if (node->type == PathNode::OPEN) {
				m_openNodes.UpdateNode(node);
			} else if (node->type == PathNode::CLOSED) {
				node->type = PathNode::OPEN;
			}
		}
	} else {
		node->type = PathNode::OPEN;
		node->parent = parent;
		node->cost = newCost;
		node->total = newCost + GetHeuristicCost(x, y);
		node->dir = dir;
		m_openNodes.Push(node);
	}
	return	true;
}

PathNode *	CPathFinder :: GetNewNode()
{
	if (m_poolCount >= MAX_PATHNODE-1)
		return	NULL;
	else
		return	&m_nodePool[m_poolCount++];
}

PathNode *	CPathFinder :: GetNode(int x, int y)
{
	PathNode *	node = FindNode(x, y);

	if (node) {
		return	node;
	} else {
		node = GetNewNode();
		if (!node)
			return	NULL;
		node->x = x;
		node->y = y;
		node->type = PathNode::UNEXPLORED;
		AddNode(node);
		return	node;
	}
}


void	CPathFinder :: SetMap(C3dAttr *newMap)
{
	m_mapData = newMap;
}

int		CPathFinder :: GetMapWidth()
{
	return	m_mapData->m_width;
}

int		CPathFinder :: GetMapHeight()
{
	return	m_mapData->m_height;
}

DWORD	CPathFinder::GetSecondNodeArrivalTime(DWORD startTime, CPathInfo * path, int speedFactor)
{
	float xlen = path->m_pathData[1].x - path->m_startPointX;
	float ylen = path->m_pathData[1].y - path->m_startPointY;
	float distance = _sqrt(xlen*xlen + ylen*ylen);

	return startTime + DWORD(distance*speedFactor);
}

void	CPathFinder :: BuildResultPath(DWORD startTime, PathNode *goalNode, int speedFactor, CPathInfo *path)
{

	int		len = 0;
	PathNode *	tempnode = goalNode;
	while (tempnode) {
		len++;
		tempnode = tempnode->parent;
	}


	path->m_pathData.resize(len);
	if (len <= 1)
		return;


	tempnode = goalNode;
	int i ;
	for (i=0; i<len; i++) {
		path->m_pathData[len-i-1].x = tempnode->x;
		path->m_pathData[len-i-1].y = tempnode->y;
		path->m_pathData[len-i-1].dir = tempnode->dir;
		tempnode = tempnode->parent;
	}


	int		diagonalMoveFactor = float(speedFactor)*1.414f;
	path->m_pathData[0].time = startTime;
	path->m_pathData[1].time = GetSecondNodeArrivalTime(startTime, path, speedFactor);

	for (i=1; i<len-1; i++) {
		if (path->m_pathData[i+1].dir%2) {
			path->m_pathData[i+1].time = path->m_pathData[i].time+diagonalMoveFactor;
		} else {
			path->m_pathData[i+1].time = path->m_pathData[i].time+speedFactor;
		}
	}
}

bool	CPathFinder :: IsConnected(int sx, int sy, int dx, int dy)
{
	return true;
}
