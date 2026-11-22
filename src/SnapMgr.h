#ifndef HIGHPRIEST_SNAPMGR_H
#define HIGHPRIEST_SNAPMGR_H

class UIWindow;

class SnapObject {


	friend class CSnapMgr;
public:
	SnapObject();
	virtual ~SnapObject();
	bool IsOverLapped(int s1, int d1, int s2, int d2);
	int CheckSnapType(SnapObject* snapObject);
	void ConnectSnapObjectEachOther(SnapObject* snapObject, int snapType);
	void ConnectSnapObject(SnapObject* snapObject, int snapType);
	void DisconnectSnapObject(SnapObject* snapObject);
	bool IsConnectedRoot(SnapObject* snapObject);
	bool IsConnected(SnapObject* snapObject);
	void MoveConnectedObject(int dx, int dy, int snapType);
	void MoveConnectedObjectRoot(int dx, int dy, int snapType);
	bool IsSnapedTop(SnapObject* snapObject);
	bool IsSnapedTopRoot(SnapObject* snapObject);
	void GetHeightOfGroup(int& dh, int snapType);
	int GetHeight();
private:
	UIWindow* m_careWindow;
	std::list<SnapObject *> m_snapObjectList[4];
	int m_isProcessed;
};

class CSnapMgr {
public:
	CSnapMgr();
	virtual ~CSnapMgr();
	void RemoveAll();
	void ClearWayAll();
	void AddSnapMasterObject(UIWindow* window);
	void AddSnapObject(UIWindow* window);
	void RemoveSnapObject(UIWindow* window);
	void OrganizeSnap(SnapObject* snapObject);
	bool IsWindowSnapedToMaster(UIWindow* window);
	void BeginMove(UIWindow* window);
	void EndMove(UIWindow* window);
	void MoveDelta(UIWindow* masterWindow, int deltaDragX, int deltaDragY);
	bool IsMasterSnap(SnapObject* window);
	bool IsMasterSnap(UIWindow* window);
	void MoveObject(UIWindow* masterWindow, int deltaDragX, int deltaDragY);
	bool IsConnected(UIWindow* window1, UIWindow* window2);
	void GetDeltaDrag(UIWindow* masterWindow, int& deltaDragX, int& deltaDragY);
	bool IsRegisteredToSnap(UIWindow* window);
	void MoveConnectedObject(UIWindow* window, int dx, int dy, int snapType);
	bool IsSnapedTopOfMaster(SnapObject* window);
	bool IsThisGroupSnapedBottomOfScreen(SnapObject* snapObject, int dh);
	bool IsThisGroupSnapedTopOfScreen(SnapObject* window);
	SnapObject* GetSnapObject(UIWindow* window);
private:
	SnapObject* m_masterObject;
	std::list<SnapObject *> m_snapObjectList;
};

#endif
