#ifndef HIGHPRIEST_LOGINMODE_H
#define HIGHPRIEST_LOGINMODE_H


#include "Mode.h"
#include "packet.h"
#include "resource/Sprite.h"
#include "resource/Action.h"

enum {
	LMM_CONNECT_TO_ACSVR = 10000,
	LMM_MAKECHARACTER = 10001,
	LMM_TRANSFERCHARACTER = 10002,
	LMM_SELECTSVR = 10003,
	LMM_SELECTCHARACTER = 10004,
	LMM_SENDCHARINFO = 10005,
	LMM_CANCEL = 10006,
	LMM_PASSWORD = 10007,
	LMM_ID = 10008,
	LMM_SELECT_CHARACTER = 10009,
	LMM_DELETE_CHARACTER = 10010,
	LMM_GOTO_LOGIN = 10011,
	LMM_SET_EMAIL_ADDRESS = 10012,
	LMM_GET_EMAIL_ADDRESS = 10013,
	LMM_GET_MAKING_CHAR_NAME = 10014,
	LMM_SET_MAKING_CHAR_NAME = 10015,
	LMM_GET_CHAR_PARAM = 10016,
	LMM_SET_CHAR_PARAM = 10017,
	LMM_GOTOSELECTACCOUNT = 10018,
	LMM_SELECTACCOUNT = 10019,
	LMM_GET_BILLING_INFO = 10020,
	LMM_DISCONNECT_ACSVR = 10021,
	LMM_SELECTACCOUNT2 = 10022,
	LMM_GOTOLICENCE = 10023,
};
enum {
	LSM_WARNING,
	LSM_LICENCE,
	LSM_SELECTACCOUNT,
	LSM_IDENTRY,
	LSM_WAITRESP_FROM_ACSVR,
	LSM_WAITRESP_FROM_CHSVR,
	LSM_SELECTSVR,
	LSM_SELECTCHAR,
	LSM_MAKECHAR,
	LSM_WAITRESP_FROM_CHSVR_SELECTCHAR,
	LSM_WAITRESP_FROM_CHSVR_MAKECHAR,
	LSM_WAITRESP_FROM_CHSVR_DELETECHAR,
	LSM_READY_FOR_ZONESVR,
	LSM_SELECTACCOUNT2,
	LSM_REQ_GAME_GUARD_CHECK,
	LSM_ACK_GAME_GUARD,
	LSM_REQ_GAME_GUARD_CHECK2,
	LSM_REQ_PTNUM,
	LSM_REQ_PTACINFO,
	LSM_SELECTCHAR2,
	LSM_LAST,
};

#pragma pack(push,billing)
#pragma pack(1)
struct BILLING_INFO {
	char dummy1;
	unsigned int code;
	unsigned int time1;
	unsigned int time2;
	char dummy2[7];
};
#pragma pack(pop,billing)

class UIWaitWnd;
class CLoginMode;
struct accountInfo;

class CLoginMode : public CMode {
public:
	CLoginMode();
	virtual ~CLoginMode();
	virtual void OnInit(const char* modeName);
	virtual int OnRun();
	virtual void OnExit();
	virtual void OnUpdate();
	virtual int SendMsg(int messageId, int val1, int val2, int val3);
	void PreLoadAlwaysNeededRes();
	void SetKeycryptErrorInfo(const char* modeName);
private:
	virtual void OnChangeState(int state);
	void PollNetworkStatus();
	void OnProcessState(int state);
	void OnRenderState(int state);
	void OpenSettlePage();
	void CheckPingTime();
	void CheckExeHashFromAccServer();
	void CheckHashFromCharServer();
	int GenerateExeHash(unsigned char Hash[16]);
	char* SetPaddingValue(char* pad, int len);
	void Ac_Accept_Login(const char* modeName);
	void Ac_Refuse_Login(const char* modeName);
	void Ac_Event_Result(const char* modeName);
	void Hc_Accept_Enter(const char* modeName);
	void Hc_Block_Character(const char* modeName);
	void Hc_Refuse_Enter(const char* modeName);
	void Hc_Accept_Makechar(const char* modeName);
	void Hc_Refuse_Makechar(const char* modeName);
	void Hc_Accept_Deletechar(const char* modeName);
	void Hc_Refuse_Deletechar(const char* modeName);
	void Hc_Notify_Zonesvr(const char* modeName);
	void Zc_Accept_Enter(const char* modeName);
	void Zc_Accept_Enter2(const char* modeName);
	void Zc_Refuse_Enter(const char* modeName);
	void Sc_Notify_Ban(const char* modeName);
	void Sc_Billing_Info(const char* modeName);
	void Ac_Ask_Pngameroom(const char* modeName);
	void Sc_Ack_Encryption(const char* modeName);
	void Ac_Ack_Hash(const char* modeName);
	void Ac_Notify_Error(const char* modeName);
	void Hc_Refuse_SelectChar(const char* modeName);
	void Zc_Aid(const char* modeName);
	void Hc_CharNotBeenSelected(const char* modeName);
	void Hc_Ack_Is_Valid_CharName(const char* modeName);
	void Hc_Ack_Change_CharName(const char* modeName);
	void SendCaIpSecurity();
	bool InitAccountInfo();
	bool InitAccountInfo2(int accNumber);
	int SendKeycryptErrorInfo();
private:
	int m_charParam[8];
	char m_makingCharName[64];
	char m_emaiAddress[128];
	char m_userPassword[64];
	char m_userId[64];
	int m_numServer;
	int m_serverSelected;
	int m_numChar;
	unsigned int m_subModeStartTime;
	SERVER_ADDR m_serverInfo[100];
	CHARACTER_INFO m_charInfo[12];
	std::string m_wallPaperBmpName;
	BILLING_INFO m_billingInfo;
	unsigned int m_syncRequestTime;
	UIWaitWnd* m_wndWait;
	std::vector<accountInfo> m_accountInfo;
	std::vector<accountInfo> m_accountInfo2;
	bool m_multiLang;
	int m_nSelectedAccountNo;
	int m_nSelectedAccountNo2;
	std::string m_strErrorInfo;
};
void SendUDPPacket(LPCSTR szURL, USHORT uPort, const char* p, int pLen);

struct accountInfo {
public:
	accountInfo() {}
	accountInfo(const accountInfo& info) { CopyContents(info); }
	const accountInfo& operator=(const accountInfo& info) { CopyContents(info); return *this; }
private:
	void CopyContents(const accountInfo& info) {
		display = info.display;
		desc = info.desc;
		address = info.address;
		port = info.port;
		type = info.type;
		version = info.version;
		balloon = info.balloon;
	}
public:
	std::string display;
	std::string desc;
	std::string balloon;
	std::string address;
	std::string port;
	int type;
	unsigned int version;
};

#endif
