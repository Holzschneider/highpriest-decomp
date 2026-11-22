#include "common.h"
#include "SnapMgr.h"
#include "window/UIFrameWnd.h"

extern UIWindowMgr g_windowMgr;
extern CRenderer* g_renderer;
extern CSnapMgr g_snapMgr;
extern bool s_enableSnap;
extern int oppositDir[];

int oppositDir[4] = { 2, 3, 0, 1 };
bool s_enableSnap = true;
CSnapMgr g_snapMgr;

SnapObject::SnapObject() : m_careWindow(0), m_isProcessed(0) {}

SnapObject::~SnapObject() {}

bool SnapObject::IsOverLapped(int s1, int d1, int s2, int d2) {
	if (s2 >= d1 && s2 <= d2)
		return true;
	if (d2 >= s1 && d2 <= s2)
		return true;
	if (s1 >= d1 && s1 <= d2)
		return true;
	if (d1 >= s1 && d1 <= s2)
		return true;
	return false;
}

int SnapObject::CheckSnapType(SnapObject *snapObject)
{
	int sw = m_careWindow->m_w;
	int sx = m_careWindow->m_x;
	int sy = m_careWindow->m_y;
	int sh = m_careWindow->m_h;

	UIWindow *window = snapObject->m_careWindow;
	int dx = window->m_x;
	int dw = window->m_w;
	int dy = window->m_y;
	int dh = window->m_h;

	if (sx == dx + dw && IsOverLapped(sy, dy, sy + sh, dy + dh))
		return 0;
	if (dx == sx + sw && IsOverLapped(dy, sy, dy + dh, sy + sh))
		return 2;
	if (sy == dy + dh && IsOverLapped(sx, dx, sx + sw, dx + dw))
		return 1;
	if (dy == sy + sh && IsOverLapped(dx, sx, dx + dw, sx + sw))
		return 3;
	return -1;
}

void SnapObject::MoveConnectedObject(int dx, int dy, int snapType) {
	if (snapType == -1) return;
	if (m_isProcessed == 1) return;
	m_isProcessed = 1;
	if (m_careWindow != 0) {
		m_careWindow->m_x += dx;
		m_careWindow->m_y += dy;
		g_windowMgr.UpdateSnapInfo(m_careWindow);
	}
	std::list<SnapObject*>::iterator it = m_snapObjectList[snapType].begin();
	while (it != m_snapObjectList[snapType].end()) {
		(*it)->MoveConnectedObject(dx, dy, snapType);
		++it;
	}
}

void SnapObject::DisconnectSnapObject(SnapObject* snapObject) {
	if (snapObject == 0) return;
	for (int i = 0; i < 4; i++) {
		std::list<SnapObject*>::iterator it = m_snapObjectList[i].begin();
		while (it != m_snapObjectList[i].end()) {
			if (*it == snapObject) {
				m_snapObjectList[i].erase(it);
				break;
			}
			++it;
		}
	}
}

void SnapObject::MoveConnectedObjectRoot(int dx, int dy, int snapType)
{
	if (snapType == -1) return;

	if (m_isProcessed == 1) return;

	m_isProcessed = 1;

	bool snapedTop = g_snapMgr.IsThisGroupSnapedTopOfScreen(this);
	bool snapedBottom = g_snapMgr.IsThisGroupSnapedBottomOfScreen(this, dy);
	if (!snapedTop && !(g_snapMgr.IsMasterSnap(this) && !snapedBottom)) {
		if (snapType == 3 &&
			(g_snapMgr.IsSnapedTopOfMaster(this) || snapedBottom))
		{
			snapType = oppositDir[snapType];
			dy = -dy;
		}
		if (snapType == 1 &&
			m_careWindow) {
			m_careWindow->m_x += dx;
			m_careWindow->m_y += dy;
			g_windowMgr.UpdateSnapInfo(m_careWindow);
		}
	}


	std::list<SnapObject *>::iterator it = m_snapObjectList[snapType].begin();
	while (it != m_snapObjectList[snapType].end()) {
		(*it)->MoveConnectedObject(dx, dy, snapType);
		++it;
	}
}

void SnapObject::ConnectSnapObjectEachOther(SnapObject *snapObject, int snapType)
{
	DisconnectSnapObject(snapObject);
	snapObject->DisconnectSnapObject(this);
	if (snapType != -1) {
		m_snapObjectList[snapType].push_back(snapObject);
		snapObject->ConnectSnapObject(this, oppositDir[snapType]);
	}
}


bool SnapObject::IsConnectedRoot(SnapObject *snapObject)
{
	if (m_isProcessed == 1)
		return false;
	if (snapObject == this)
		return false;

	m_isProcessed = 1;
	for (int i = 0; i < 4; i++)
	{
		if (m_snapObjectList[i].size() != 0)
		{
			std::list<SnapObject *>::iterator it = m_snapObjectList[i].begin();
			while (it != m_snapObjectList[i].end())
			{
				if ((*it)->IsConnected(snapObject))
					return true;
				++it;
			}
		}
	}
	return false;
}

void SnapObject::GetHeightOfGroup(int &dh, int snapType)
{
	dh += m_careWindow->m_h;

	SnapObject *found = 0;
	std::list<SnapObject *>::iterator it = m_snapObjectList[snapType].begin();
	while (it != m_snapObjectList[snapType].end())
	{
		if ((*it)->m_careWindow->m_h > 0)
			found = *it;
		++it;
	}

	if (found)
		found->GetHeightOfGroup(dh, snapType);
}

void SnapObject::ConnectSnapObject(SnapObject* snapObject, int snapType) {
	if (snapType == -1) return;
	m_snapObjectList[snapType].push_back(snapObject);
}

bool SnapObject::IsSnapedTop(SnapObject* snapObject) {
	if (this == snapObject)
		return true;
	std::list<SnapObject*>::iterator it = m_snapObjectList[1].begin();
	while (it != m_snapObjectList[1].end()) {
		if ((*it)->IsSnapedTop(snapObject))
			return true;
		++it;
	}
	return false;
}

bool SnapObject::IsSnapedTopRoot(SnapObject* snapObject) {
	if (this == snapObject)
		return false;
	std::list<SnapObject*>::iterator it = m_snapObjectList[1].begin();
	while (it != m_snapObjectList[1].end()) {
		if ((*it)->IsSnapedTop(snapObject))
			return true;
		++it;
	}
	return false;
}

bool SnapObject::IsConnected(SnapObject* snapObject) {
	if (m_isProcessed == 1)
		return false;
	if (snapObject == this)
		return true;
	m_isProcessed = 1;
	for (int i = 0; i < 4; i++) {
		std::list<SnapObject*>::iterator it = m_snapObjectList[i].begin();
		while (it != m_snapObjectList[i].end()) {
			if ((*it)->IsConnected(snapObject)) return true;
			++it;
		}
	}
	return false;
}

int SnapObject::GetHeight() { return m_careWindow->m_h; }

CSnapMgr::CSnapMgr() : m_masterObject(0) {}

CSnapMgr::~CSnapMgr() {
	RemoveAll();
}

void CSnapMgr::ClearWayAll() {
	std::list<SnapObject*>::iterator it = m_snapObjectList.begin();
	while (it != m_snapObjectList.end()) {
		(*it)->m_isProcessed = 0;
		++it;
	}
}

void CSnapMgr::RemoveSnapObject(UIWindow *window)
{
	if (m_masterObject != 0)
		if (m_masterObject->m_careWindow == window)
			m_masterObject = 0;

	SnapObject *snapObject = 0;

	std::list<SnapObject *>::iterator it = m_snapObjectList.begin();
	while (it != m_snapObjectList.end()) {
		if ((*it)->m_careWindow == window) {
			snapObject = *it;
			delete snapObject;
			m_snapObjectList.erase(it);
			break;
		}
		++it;
	}

	if (snapObject == 0)
		return;

	it = m_snapObjectList.begin();
	while (it != m_snapObjectList.end()) {
		(*it)->DisconnectSnapObject(snapObject);
		++it;
	}
}

void CSnapMgr::RemoveAll() {
	std::list<SnapObject*>::iterator it = m_snapObjectList.begin();
	while (it != m_snapObjectList.end()) {
		delete *it;
		++it;
	}
	m_snapObjectList.clear();
	m_masterObject = 0;
}

void CSnapMgr::AddSnapMasterObject(UIWindow *masterWindow)
{
	if (!s_enableSnap) return;

	if (IsRegisteredToSnap(masterWindow)) return;

	if (m_masterObject != 0) return;
	SnapObject *snapObject = new SnapObject;
	snapObject->m_careWindow = masterWindow;
	m_masterObject = snapObject;
	m_snapObjectList.push_back(snapObject);
}

void CSnapMgr::OrganizeSnap(SnapObject *snapObject)
{
	std::list<SnapObject *>::iterator it = m_snapObjectList.begin();
	while (it != m_snapObjectList.end()) {
		int snapType = snapObject->CheckSnapType(*it);
		snapObject->ConnectSnapObjectEachOther(*it, snapType);
		++it;
	}
}


void CSnapMgr::AddSnapObject(UIWindow *window)
{
	if (!s_enableSnap)
		return;
	if (m_masterObject != 0)
		if (m_masterObject->m_careWindow == window)
			return;

	if (IsRegisteredToSnap(window))
		return;

	SnapObject *snapObject = new SnapObject;
	snapObject->m_careWindow = window;

	OrganizeSnap(snapObject);
	m_snapObjectList.push_back(snapObject);
}

bool CSnapMgr::IsMasterSnap(UIWindow* window) {
	if (!m_masterObject)
		return false;
	return m_masterObject->m_careWindow == window;
}

bool CSnapMgr::IsConnected(UIWindow* window1, UIWindow* window2)
{
	ClearWayAll();
	SnapObject* obj1 = GetSnapObject(window1);
	if (!obj1)
		return false;
	SnapObject* obj2 = GetSnapObject(window2);
	if (!obj2)
		return false;
	return obj1->IsConnectedRoot(obj2);
}

void CSnapMgr::EndMove(UIWindow *moveWindow)
{
	if (!IsRegisteredToSnap(moveWindow))
		return;
	std::list<SnapObject *>::iterator it = m_snapObjectList.begin();
	while (it != m_snapObjectList.end()) {
		OrganizeSnap(*it);
		++it;
	}
}


void CSnapMgr::BeginMove(UIWindow* masterWindow) {
	if (m_masterObject == 0)
		return;
	if (masterWindow == 0)
		return;
	if (masterWindow != m_masterObject->m_careWindow) return;
	std::list<SnapObject*>::iterator it = m_snapObjectList.begin();
	while (it != m_snapObjectList.end()) {
		UIWindow* w = (*it)->m_careWindow;
		if (w != 0)
		{
			if (IsWindowSnapedToMaster(w))
			{
				w->SaveOriginalPos();
			}
		}
		++it;
	}
}

bool CSnapMgr::IsWindowSnapedToMaster(UIWindow* childWindow) {
	if (m_masterObject == 0)
		return false;
	if (m_masterObject->m_careWindow == childWindow) return false;
	std::list<SnapObject*>::iterator it = m_snapObjectList.begin();
	while (it != m_snapObjectList.end()) {
		(*it)->m_isProcessed = 0;
		++it;
	}
	it = m_snapObjectList.begin();
	while (it != m_snapObjectList.end()) {
		if ((*it)->m_careWindow == childWindow) {
			if ((*it)->IsConnectedRoot(m_masterObject))
				return true;
			return false;
		}
		++it;
	}
	return false;
}

SnapObject* CSnapMgr::GetSnapObject(UIWindow* window) {
	std::list<SnapObject*>::iterator it = m_snapObjectList.begin();
	while (it != m_snapObjectList.end()) {
		if ((*it)->m_careWindow == window)
			return *it;
		++it;
	}
	return 0;
}

bool CSnapMgr::IsMasterSnap(SnapObject* snapObject) {
	if (!m_masterObject)
		return false;
	return m_masterObject == snapObject;
}

void CSnapMgr::MoveDelta(UIWindow* masterWindow, int deltaDragX, int deltaDragY) {
	if (m_masterObject == 0) return;
	if (masterWindow != m_masterObject->m_careWindow) return;
	std::list<SnapObject*>::iterator it = m_snapObjectList.begin();
	while (it != m_snapObjectList.end()) {
		UIWindow* w = (*it)->m_careWindow;
		if (IsWindowSnapedToMaster(w)) {
			w->MoveDelta(deltaDragX, deltaDragY);
			g_windowMgr.UpdateSnapInfo(w);
		}
		++it;
	}
}

void CSnapMgr::MoveObject(UIWindow *masterWindow, int deltaX, int deltaY)
{
	if (m_masterObject != 0 && m_masterObject->m_careWindow == masterWindow)
	{
		MoveDelta(masterWindow, deltaX, deltaY);

		int maxDx = 0;
		int minDy = 0;
		std::list<SnapObject *>::iterator it = m_snapObjectList.begin();
		while (it != m_snapObjectList.end())
		{
			UIWindow *window = (*it)->m_careWindow;
			if (IsWindowSnapedToMaster(window))
			{
				int dx;
				int dy;
				g_windowMgr.GetDeltaSnap(window, dx, dy, 0);
				if (dx > maxDx)
					maxDx = dx;
				if (dy > minDy)
					minDy = dy;
			}
			++it;
		}

		masterWindow->MoveDelta(maxDx + deltaX, maxDx + deltaY);
		g_windowMgr.UpdateSnapInfo(masterWindow);
		MoveDelta(masterWindow, maxDx + deltaX, maxDx + deltaY);
		return;
	}

	masterWindow->MoveDelta(deltaX, deltaY);
	masterWindow->RefreshSnap();
}

bool CSnapMgr::IsRegisteredToSnap(UIWindow* window) {
	std::list<SnapObject*>::iterator it = m_snapObjectList.begin();
	while (it != m_snapObjectList.end()) {
		if (window == (*it)->m_careWindow)
			return true;
		++it;
	}
	return false;
}

void CSnapMgr::GetDeltaDrag(UIWindow *masterWindow, int &deltaDragX, int &deltaDragY)
{
	int maxDx = 0;
	if (m_masterObject == 0)
		return;

	int maxDy = 0;
	if (masterWindow == m_masterObject->m_careWindow)
	{
		std::list<SnapObject *>::iterator it = m_snapObjectList.begin();
		while (it != m_snapObjectList.end())
		{
			UIWindow *window = (*it)->m_careWindow;
			if (IsWindowSnapedToMaster(window))
			{
				int saveY = window->m_y;
				window->MoveDelta(deltaDragX, deltaDragY);

				int dx;
				int dy;
				g_windowMgr.GetDeltaSnap(window, dx, dy, 1);
				window->m_x = saveY;

				if (abs(maxDx) < abs(dx))
					maxDx = dx;
				if (abs(maxDy) < abs(dy))
					maxDy = dy;
			}
			++it;
		}
	}

	deltaDragX = maxDx;
	deltaDragY = maxDy;
}

void CSnapMgr::MoveConnectedObject(UIWindow *window, int dx, int dy, int snapType)
{
	ClearWayAll();
	std::list<SnapObject *>::iterator it = m_snapObjectList.begin();
	while (it != m_snapObjectList.end()) {
		if (window == (*it)->m_careWindow) {
			(*it)->MoveConnectedObjectRoot(dx, dy, snapType);
			EndMove(window);
			return;
		}
		++it;
	}
}


bool CSnapMgr::IsSnapedTopOfMaster(SnapObject* snapObject) {
	return m_masterObject->IsSnapedTopRoot(snapObject);
}


bool CSnapMgr::IsThisGroupSnapedTopOfScreen(SnapObject* snapObject) {
	int h = 0;
	snapObject->GetHeightOfGroup(h, 1);
	bool ret = snapObject->m_careWindow->m_h + snapObject->m_careWindow->m_y - h == 0;
	return ret;
}

bool CSnapMgr::IsThisGroupSnapedBottomOfScreen(SnapObject* snapObject, int dh) {
	int h = 0;
	snapObject->GetHeightOfGroup(h, 3);
	bool ret = snapObject->m_careWindow->m_y - dh + h == g_renderer->m_height;
	return ret;
}
