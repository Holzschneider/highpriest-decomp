#ifndef _PACKET_H
#define _PACKET_H

#include	"common.h"

extern USHORT	g_PacketSize[USHRT_MAX];
void InitializePacketSize(void);

#pragma pack(push)
#pragma pack(1)

enum {
	CHARACTERSET_ACSII					= 1,
	CHARACTERSET_UTF8,
};


#define CLIENT_MODE_GENERAL				1001
#define CLIENT_MODE_RAGNAROK_1		1002

#define CHAT_WINDOW_OPEN_S		1
#define CHAT_WINDOW_OPEN_R		2





#define PACKET_HEADER_SIZE			8
#define MAX_PASSWORD_SIZE			25
#define MAX_ID_SIZE					25
#define MAX_USER_NAME_SIZE			31
#define MAX_EMAIL_SIZE				101
#define MAX_REGIST_DATETIME_SIZE	13
#define MAX_NICK_NAME_SIZE			451
#define MAX_IP_ADDR_SIZE			16
#define	MAX_IPV4_ADDRESS_SIZE		16
#define MAX_IPV6_ADDRESS_SIZE		24
#define MAX_MSG_SIZE				751
#define	MAX_MSG_SIZE2			501
#define MAX_GROUP_NAME_SIZE			301
#define MAX_MEMO_SIZE				901
#define	MAX_FILE_NAME_SIZE			256
#define	MAX_FILE_ROOM_KEY			33
#define	MAX_MSG_SIZE				751

#define MAX_SVRINFO_SIZE			100

#define MAX_GROUP_NUMBER			58
#define MAX_FRIEND_NUMBER			5
#define MAX_GROUP_NUMBER2			100
#define MAX_FRIEND_NUMBER2			251
#define MAX_CHATUSER_NUMBER			125
#define MAX_CHATUSVR_LIST			10
#define MAX_MEMO_COUNT				100
#define	MAX_IP_COUNT					40

#define MAX_CHAT_FRIEND_COUNT		99






#define HEADER_CD_REQ_NS_IP	1003
#define HEADER_DC_ACK_NS_IP	1004


#define HEADER_CN_REQ_LOGIN 2101
#define HEADER_NC_ACK_LOGIN 2102
#define HEADER_NC_NOTI_DUPLE_LOGIN 2103

#define HEADER_CN_REQ_LOGIN_CONFIRM		2104
#define HEADER_NC_ACK_LOGIN_CONFIRM		2105


#define HEADER_CN_REQ_LOGOUT	2111
#define HEADER_NC_ACK_LOGOUT	2112


#define HEADER_CN_REQ_MYINFO				2201
#define HEADER_NC_ACK_MYINFO			2202

#define HEADER_CN_REQ_SYNC			2211
#define HEADER_NC_ACK_ALL_GROUPINFO			2212
#define HEADER_NC_ACK_ALL_FRIENDINFO		2213


#define HEADER_NC_ACK_UPDATED_GROUP_NAME		2221
#define HEADER_NC_ACK_UPDATED_GROUP_DEPTH		2222
#define HEADER_NC_ACK_DELETED_GROUP_INFO		2223
#define HEADER_NC_ACK_ADDED_GROUP_INFO		2224


#define HEADER_NC_ACK_ADDED_FRIEND_INFO		2225
#define HEADER_NC_ACK_UPDATED_FRIEND_INFO		2226
#define HEADER_NC_ACK_UPDATED_FRIEND_GROUP		2227
#define HEADER_NC_ACK_UPDATED_FRIEND_BLOCK		2228
#define HEADER_NC_ACK_DELETED_FRIEND_INFO		2229

#define HEADER_CN_REQ_FRIEND_STATUS		2241
#define HEADER_NC_ACK_FRIEND_STATUS		2242


#define HEADER_CN_REQ_CHANGE_CONDITION	2401
#define HEADER_NC_ACK_CHANGE_CONDITION	2402
#define HEADER_NC_NOTI_CHANGE_CONDITION	2403


#define HEADER_CN_REQ_CHANGE_NICKNAME	2411
#define HEADER_NC_NOTI_CHANGE_NICKNAME	2413
#define HEADER_NC_ACK_CHANGE_NICKNAME	2412


#define HEADER_CN_REQ_ADD_GROUP	2501
#define HEADER_NC_ACK_ADD_GROUP	2502


#define HEADER_CN_REQ_DEL_GROUP	2511
#define HEADER_NC_ACK_DEL_GROUP	2512


#define HEADER_CN_REQ_CHANGE_GROUP_NAME	2521
#define HEADER_NC_ACK_CHANGE_GROUP_NAME	2522


#define HEADER_CN_REQ_GROUP_DEPTH	2531
#define HEADER_NC_ACK_GROUP_DEPTH	2532


#define HEADER_CN_REQ_ADD_FRIEND	2601
#define HEADER_NC_ACK_ADD_FRIEND	2602
#define	HEADER_NC_REQ_AGREE_FRIEND	2603
#define	HEADER_CN_ACK_AGREE_FRIEND	2604
#define	HEADER_NC_ACK_AGREE_FRIEND	2605


#define HEADER_CN_REQ_DEL_FRIEND	2611
#define HEADER_NC_ACK_DEL_FRIEND	2612


#define HEADER_CN_REQ_CHANGEGROUP_FRIEND	2621
#define HEADER_NC_ACK_CHANGEGROUP_FRIEND	2622


#define HEADER_CN_REQ_BLOCK_FRIEND	2631
#define HEADER_NC_ACK_BLOCK_FRIEND	2632
#define HEADER_NC_NOTI_BLOCK_FRIEND	2633


#define HEADER_CN_REQ_SEND_MEMO	2701
#define HEADER_NC_ACK_SEND_MEMO	2702
#define HEADER_NC_NOTI_RECV_MEMO  2703

#define HEADER_CN_REQ_READ_MEMO	2711
#define HEADER_NC_ACK_READ_MEMO	2712

#define HEADER_CN_REQ_DEL_MEMO	2721
#define HEADER_NC_ACK_DEL_MEMO	2722

#define HEADER_CN_REQ_READ_TOTAL_MEMO	2731
#define HEADER_NC_ACK_READ_TOTAL_MEMO	2732

#define HEADER_CN_REQ_CONFIRM_READ_MEMO 2741
#define HEADER_NC_ACK_CONFIRM_READ_MEMO 2742




#define HEADER_CN_REQ_CHAT_CREATE			2301
#define HEADER_NC_ACK_CHAT_CREATE			2302

#define HEADER_CN_REQ_CHAT_INVITE			2311
#define HEADER_NC_ACK_CHAT_INVITE			2312
#define HEADER_NC_NOTI_CHAT_INVITE			2313

#define HEADER_CN_REQ_CHAT_CONNECT_FAIL		2314
#define HEADER_NC_ACK_CHAT_CONNECT_FAIL		2315
#define HEADER_NC_NOTI_CHAT_CONNECT_FAIL	2316

#define HEADER_CN_REQ_CHATTING_FRIEND_INFO		2321
#define HEADER_NC_ACK_CHATTING_FRIEND_INFO		2322







#define HEADER_CH_NOTIFY_PING	3011



#define HEADER_CH_REQ_CHAT_INITIALIZE	3101
#define HEADER_HC_ACK_CHAT_INITALIZE	3102

#define HEADER_CH_REQ_CAHT_SEARCH		3201
#define HEADER_HC_ACK_CHAT_SEARCH		3202

#define HEADER_CH_REQ_CHAT_ENTER		3211
#define HEADER_HC_ACK_CHAT_ENTER		3212
#define HEADER_HC_NOTI_CHAT_ENTER		3213

#define HEADER_CH_REQ_CHAT_OPENWINDOW	3214
#define HEADER_HC_ACK_CHAT_OPENWINDOW	3215

#define HEADER_CH_REQ_CHAT_ADD_USER		3301
#define HEADER_HC_ACK_CHAT_ADD_USER		3302

#define HEADER_CH_REQ_CHAT_MSG_TICK		3401
#define HEADER_HC_NOTI_CHAT_MSG_TICK	3402
#define HEADER_CH_REQ_CHAT_MESSAGE		3403
#define HEADER_HC_NOTI_CHAT_MESSAGE		3404
#define HEADER_HC_ACK_CHAT_MESSAGE_SENDFAIL	3411

#define HEADER_CH_REQ_CHAT_CLOSE		3223
#define HEADER_HC_ACK_CHAT_CLOSE		3224
#define HEADER_HC_NOTI_CHAT_CLOSE		3225

#define HEADER_CH_REQ_CHAT_BYE				3226
#define HEADER_HC_ACK_CHAT_BYE				3227





#define HEADER_CH_REQ_CHAT_SAVE_LOG		3801
#define HEADER_HC_REQ_CHAT_SAVE_LOG		3802
#define HEADER_HC_ACK_CHAT_SAVE_LOG		3803
#define HEADER_CH_ACK_CHAT_SAVE_LOG		3804
#define HEADER_CH_NOTI_CHAT_LOG_START	3805

#define HEADER_CH_REQ_CHAT_END_LOG		3811
#define HEADER_HC_REQ_CHAT_END_LOG		3812
#define HEADER_HC_ACK_CHAT_END_LOG		3813
#define HEADER_CH_ACK_CHAT_END_LOG		3814
#define HEADER_CH_NOTI_CHAT_LOG_STOP	3815

#define HEADER_CH_REQ_LOG_START 3801
#define HEADER_HC_REQ_LOG_START 3802
#define HEADER_CH_ACK_LOG_START 3803
#define HEADER_HC_ACK_LOG_START 3804
#define HEADER_HC_NOTI_LOG_READY 3805

#define HEADER_CH_REQ_LOG_END 3811
#define HEADER_HC_REQ_LOG_END 3812
#define HEADER_HC_ACK_LOG_END 3813
#define HEADER_CH_ACK_LOG_END 3814
#define HEADER_CH_NOTI_LOG_END 3815

#define HEADER_HC_NOTI_SAVE_LOG_FAIL 3821


#define HEADER_CH_REQ_FILE_SEND 3901
#define HEADER_HC_ACK_FILE_SEND 3902
#define HEADER_HC_REQ_FILE_SEND	3903
#define HEADER_CH_ACK_FILE_SEND	3904

#define HEADER_CH_REQ_FILE_SEND_END	3991
#define HEADER_HC_REQ_FILE_SEND_END	3993
#define HEADER_CH_ACK_FILE_SEND_END	3994
#define HEADER_HC_ACK_FILE_SEND_END	3992

#define HEADER_CH_REQ_FILE_SEND_CANCEL	3921
#define HEADER_HC_REQ_FILE_SEND_CANCEL	3923
#define HEADER_CH_ACK_FILE_SEND_CANCEL	3924
#define HEADER_HC_ACK_FILE_SEND_CANCEL	3922

#define HEADER_CH_REQ_FILE_CREATE_ROOM	3911
#define HEADER_HC_ACK_FILE_CREATE_ROOM	3912
#define HEADER_HC_NOTI_FILE_CREATE_ROOM 3913






typedef struct _GRON_SVR_INFO {
	UINT uiServerKey;
	UINT uiServerRole;
	char cServerIP[MAX_IP_ADDR_SIZE];
	UINT uiServerPort;
	UINT uiUserPort;
} GRON_SVR_INFO, *LPGRON_SVR_INFO;

typedef struct _GRON_MY_INFO {
	char	cUserName[MAX_USER_NAME_SIZE];
	char	cEmail[MAX_EMAIL_SIZE];
	char	cRegistDateTime[MAX_REGIST_DATETIME_SIZE];
	UINT	uiMemoCount;
	UINT	uiNickNameSize;
	char	cNickName[MAX_NICK_NAME_SIZE];
} GRON_MY_INFO, *LPGRON_MY_INFO;

typedef struct _GRON_GROUP_INFO {
	UINT	uiGroupKey;
	UINT	uiRootKey;
	UINT	uiParentKey;
	UINT	uiGroupNameSize;
	char	cGroupName[MAX_GROUP_NAME_SIZE];
} GRON_GROUP_INFO, *LPGRON_GROUP_INFO;

typedef struct _UPDATED_GROUP_NAME {
	UINT	uiGroupKey;
	UINT	uiRootKey;
	UINT	uiGroupNameSize;
	char	cGroupName[MAX_GROUP_NAME_SIZE];
} UPDATED_GROUP_NAME ;

typedef struct _UPDATED_GROUP_DEPTH {
	UINT	uiGroupKey;
	UINT	uiRootKey;
	UINT	uiParentKey;
} UPDATED_GROUP_DEPTH ;

typedef struct _DELETED_GROUP_INFO {
	UINT	uiGroupKey;
	UINT	uiRootKey;
} DELETED_GROUP_INFO ;

typedef struct _GRON_FRIEND_INFO {
	char	cFriendID[MAX_ID_SIZE];
	UINT	uiFriendKey;
	UINT	uiRootKey;
	UINT	uiGroupNumID;
	char	cFriendName[MAX_USER_NAME_SIZE];
	char	cEmail[MAX_EMAIL_SIZE];
	UINT	uiFriendCurrentStatus;
	UINT	uiApprovalStatus;
	UINT	uiNickNameSize;
	char	cNickName[MAX_NICK_NAME_SIZE];

	bool operator==(const UINT friendKey) const
	{
		return	uiFriendKey == friendKey ;
	}

} GRON_FRIEND_INFO, *LPGRON_FRIEND_INFO;

typedef struct _UPDATED_FRIEND_INFO {
	UINT	uiFriendKey;
	UINT	uiNickNameSize;
	char	cNickName[MAX_NICK_NAME_SIZE];
} UPDATED_FRIEND_INFO ;

typedef struct _UPDATED_FRIEND_GROUP {
	UINT	uiFriendKey;
	UINT	uiRootKey;
	UINT	uiGroupKey;
} UPDATED_FRIEND_GROUP ;

typedef struct _UPDATED_FRIEND_BLOCK {
	UINT	uiFriendKey;
	UINT	uiBlockStatus;
} UPDATED_FRIEND_BLOCK ;

typedef struct _DELETED_FRIEND_INFO {
	UINT	uiFriendKey;
} DELETED_FRIEND_INFO ;

typedef struct _GRON_STATE_INFO {
	UINT	uiUserNo;
	UINT	uiUserState;
} GRON_STATE_INFO, *LPGRON_STATE_INFO;

typedef struct  _GRON_MEMO_INFO{
	UINT	uiMemoClass;
	UINT	uiMemoKey;
	UINT	uiRootKey;
	UINT	uiFriendKey;
	UINT	uiReadFlag;
	char	cSendDate[13];
	UINT	uiMemoSize;
	char	cMemo[MAX_MEMO_SIZE];
} GRON_MEMO_INFO, *LPGRON_MEMO_INFO;

typedef struct  _GRON_MEMO_INFO2{
	GRON_MEMO_INFO	memoInfo ;
	GRON_FRIEND_INFO * friendInfo ;
} GRON_MEMO_INFO2, *LPGRON_MEMO_INFO2;

typedef struct _GRON_FRIEND_POSITION {
	UINT	uiNotificationNum;
	UINT	uiFriendNumID;
} GRON_FRIEND_POSITION, *LPGRON_FRIEND_POSITION;

typedef struct _GRON_CHATTING_FRIEND_INFO {
	UINT	uiFriendKey;
	char	cFriendID[MAX_ID_SIZE];
	char	cFriendName[MAX_USER_NAME_SIZE];
	UINT	uiNickNameSize;
	char	cNickName[MAX_NICK_NAME_SIZE];
} GRON_CHATTING_FRIEND_INFO, *LPGRON_CHATTING_FRIEND_INFO;

typedef struct _G_CHAT_FRIENDINFO {
	UINT	uiFriendNo;
	UINT	uiClientMode;
} G_CHAT_FRIENDINFO, *LPG_CHAT_FRIENDINFO;

typedef struct _G_FONT {
	LOGFONTA	LogFont;
	COLORREF	ColorRef;
	UINT		uiFontSize;
} G_FONT, *LPG_FONT;







typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiType;
} PACKET_CD_REQ_NS_IP;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiNotiServerPort;
	UINT	uiNsCount;
	char	cNotiServerIP[MAX_IPV6_ADDRESS_SIZE * MAX_IP_COUNT];
} PACKET_DC_ACK_NS_IP;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	UINT	uiVersionCode;
	UINT	uiCharacterSet;
	char	cUserID[MAX_ID_SIZE];
	char	cPassword[MAX_PASSWORD_SIZE];
} PACKET_CN_REQ_LOGIN;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	int		iResult;
} PACKET_NC_ACK_LOGIN;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
} PACKET_NC_NOTI_DUPLE_LOGIN;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	UINT	uiVersionCode;
} PACKET_CN_REQ_LOGIN_CONFIRM;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	int		iResult;
	UINT	uiNewVersionCode;
} PACKET_NC_ACK_LOGIN_CONFIRM;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	UINT	uiAuthKey;
} PACKET_CN_REQ_MYINFO;


typedef struct {
	UINT			uiPacketType;
	UINT			uiBodySize;
	UINT			uiUserKey;
	GRON_MY_INFO	myInfo;
} PACKET_NC_ACK_MYINFO;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	UINT	uiGrvyYn;
	UINT	uiVersionCode;
} PACKET_CN_REQ_SYNC;



typedef struct {
	UINT			uiPacketType;
	UINT			uiBodySize;
	UINT			uiUserKey;
	UINT			uiRootKey;
	UINT			uiTotalGroupNumber;
	UINT			uiGroupCount;
	GRON_GROUP_INFO	GroupInfo[MAX_GROUP_NUMBER];
} PACKET_NC_ACK_ALL_GROUPINFO;



typedef struct {
	UINT				uiPacketType;
	UINT				uiBodySize;
	UINT				uiUserKey;
	UINT				uiRootKey;
	UINT				uiTotalFriendNumber;
	UINT				uiFriendCount;
	GRON_FRIEND_INFO	friendInfo[MAX_FRIEND_NUMBER];
} PACKET_NC_ACK_ALL_FRIENDINFO;


typedef struct {
	UINT			uiPacketType;
	UINT			uiBodySize;
	UINT			uiUserKey;
	UINT			uiRootKey;
	UINT			uiTotalGroupNumber;
	UINT			uiGroupCount;
	UPDATED_GROUP_NAME	GroupInfo[MAX_GROUP_NUMBER];
} PACKET_NC_ACK_UPDATED_GROUP_NAME;


typedef struct {
	UINT			uiPacketType;
	UINT			uiBodySize;
	UINT			uiUserKey;
	UINT			uiRootKey;
	UINT			uiTotalGroupNumber;
	UINT			uiGroupCount;
	UPDATED_GROUP_DEPTH	GroupInfo[MAX_GROUP_NUMBER];
} PACKET_NC_ACK_UPDATED_GROUP_DEPTH;


typedef struct {
	UINT			uiPacketType;
	UINT			uiBodySize;
	UINT			uiUserKey;
	UINT			uiRootKey;
	UINT			uiTotalGroupNumber;
	UINT			uiGroupCount;
	DELETED_GROUP_INFO	GroupInfo[MAX_GROUP_NUMBER];
} PACKET_NC_ACK_DELETED_GROUP_INFO;


typedef struct {
	UINT				uiPacketType;
	UINT				uiBodySize;
	UINT				uiUserKey;
	UINT				uiRootKey;
	UINT				uiTotalFriendNumber;
	UINT				uiFriendCount;
	UPDATED_FRIEND_INFO	friendInfo[MAX_FRIEND_NUMBER];
} PACKET_NC_ACK_UPDATED_FRIEND_INFO;


typedef struct {
	UINT				uiPacketType;
	UINT				uiBodySize;
	UINT				uiUserKey;
	UINT				uiRootKey;
	UINT				uiTotalFriendNumber;
	UINT				uiFriendCount;
	UPDATED_FRIEND_GROUP	friendInfo[MAX_FRIEND_NUMBER];
} PACKET_NC_ACK_UPDATED_FRIEND_GROUP;


typedef struct {
	UINT				uiPacketType;
	UINT				uiBodySize;
	UINT				uiUserKey;
	UINT				uiRootKey;
	UINT				uiTotalFriendNumber;
	UINT				uiFriendCount;
	UPDATED_FRIEND_BLOCK	friendInfo[MAX_FRIEND_NUMBER];
} PACKET_NC_ACK_UPDATED_FRIEND_BLOCK;


typedef struct {
	UINT				uiPacketType;
	UINT				uiBodySize;
	UINT				uiUserKey;
	UINT				uiRootKey;
	UINT				uiTotalFriendNumber;
	UINT				uiFriendCount;
	DELETED_FRIEND_INFO	friendInfo[MAX_FRIEND_NUMBER];
} PACKET_NC_ACK_DELETED_FRIEND_INFO;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	UINT	uiGrvyYn;
} PACKET_CN_REQ_FRIEND_STATUS;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	UINT	uiRootKey;
	UINT	uiCloseFlag;
	UINT	uiFriendCount;
	GRON_STATE_INFO	statusCode[MAX_FRIEND_NUMBER];
} PACKET_NC_ACK_FRIEND_STATUS;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	UINT	uiFriendKey;
	UINT	uiFriendCondition;
} PACKET_NC_NOTI_CHANGE_CONDITION;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	UINT	uiVersionCode;
} PACKET_CN_REQ_LOGOUT;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	int		iResult;
} PACKET_NC_ACK_LOGOUT;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	UINT	uiCondition;
} PACKET_CN_REQ_CHANGE_CONDITION;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	int		iResult;
} PACKET_NC_ACK_CHANGE_CONDITION;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	UINT	uiNickNameSize;
	char	cNickName[MAX_NICK_NAME_SIZE];
} PACKET_CN_REQ_CHANGE_NICKNAME;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	int		iResult;
} PACKET_NC_ACK_CHANGE_NICKNAME;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	UINT	uiFriendKey;
	UINT	uiFriendNickNameSize;
	char	cFriendNickName[MAX_NICK_NAME_SIZE];
} PACKET_NC_NOTI_CHANGE_NICKNAME;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	UINT	uiVersionCode;
	UINT	uiReqKey;
	UINT	uiRootKey;
	UINT	uiPrntKey;
	UINT	uiGroupNameSize;
	char	cGroupName[MAX_GROUP_NAME_SIZE];
} PACKET_CN_REQ_ADD_GROUP;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	int		iResult;
	UINT	uiReqKey;
	UINT	uiRootKey;
	UINT	uiGroupKey;
} PACKET_NC_ACK_ADD_GROUP;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	UINT	uiVersionCode;
	UINT	uiRootKey;
	UINT	uiGroupKey;
} PACKET_CN_REQ_DEL_GROUP;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	int		iResult;
	UINT	uiRootKey;
	UINT	uiGroupKey;
} PACKET_NC_ACK_DEL_GROUP;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	UINT	uiVersionCode;
	UINT	uiRootKey;
	UINT	uiGroupKey;
	UINT	uiGroupNameSize;
	char	cGroupName[MAX_GROUP_NAME_SIZE];
} PACKET_CN_REQ_CHANGE_GROUP_NAME;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	int		iResult;
	UINT	uiRootKey;
	UINT	uiGroupKey;
} PACKET_NC_ACK_CHANGE_GROUP_NAME;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	UINT	uiVersionCode;
	UINT	uiRootKey;
	UINT	uiGroupKey;
	UINT	uiPrntKey;
} PACKET_CN_REQ_GROUP_DEPTH;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	int		iResult;
	UINT	uiRootKey;
	UINT	uiGroupKey;
	UINT	uiPrntKey;
} PACKET_NC_ACK_GROUP_DEPTH;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	UINT	uiVersionCode;
	char	cFriendID[MAX_ID_SIZE];
	UINT	uiRootKey;
} PACKET_CN_REQ_ADD_FRIEND;


typedef struct {
	UINT				uiPacketType;
	UINT				uiBodySize;
	UINT				uiUserKey;
	int					iResult;
	UINT				uiRootKey;
	GRON_FRIEND_INFO	friendInfo;
} PACKET_NC_ACK_ADD_FRIEND;


typedef struct {
	UINT				uiPacketType;
	UINT				uiBodySize;
	UINT				uiUserKey;
	GRON_FRIEND_INFO	friendInfo;
} PACKET_NC_REQ_AGREE_FRIEND;


typedef struct {
	UINT				uiPacketType;
	UINT				uiBodySize;
	UINT				uiUserKey;
	int					iResult;
	UINT				uiFriendKey;
} PACKET_CN_ACK_AGREE_FRIEND;



typedef struct {
	UINT				uiPacketType;
	UINT				uiBodySize;
	UINT				uiUserKey;
	int					iResult;
	UINT				uiFriendKey;
} PACKET_NC_ACK_AGREE_FRIEND;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	UINT	uiVersionCode;
	UINT	uiRootKey;
	UINT	uiFriendKey;
} PACKET_CN_REQ_DEL_FRIEND;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	int		iResult;
	UINT	uiRootKey;
} PACKET_NC_ACK_DEL_FRIEND;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	UINT	uiVersionCode;
	UINT	uiRootKey;
	UINT	uiGroupKey;
	UINT	uiFriendKey;
} PACKET_CN_REQ_CHANGEGROUP_FRIEND;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	int		iResult;
	UINT	uiRootKey;
} PACKET_NC_ACK_CHANGEGROUP_FRIEND;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	UINT	uiVersionCode;
	UINT	uiRootKey;
	UINT	uiFriendKey;
	UINT	uiBlockCode;
} PACKET_CN_REQ_BLOCK_FRIEND;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	int		iResult;
	UINT	uiRootKey;
	UINT	uiFriendKey;
	UINT	uiBlockCode;
} PACKET_NC_ACK_BLOCK_FRIEND;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	UINT	uiFriendKey;
	UINT	uiBlockCode;
	UINT	uiFriendCondition;
} PACKET_NC_NOTI_BLOCK_FRIEND;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	UINT	uiRootKey;
	UINT	uiFriendKey;
	UINT	uiMemoSize;
	char	cMemo[MAX_MEMO_SIZE];
} PACKET_CN_REQ_SEND_MEMO;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	int		iResult;
	UINT	uiRootKey;
} PACKET_NC_ACK_SEND_MEMO;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	UINT	uiFriendKey;
	UINT	uiMemoKey;
} PACKET_NC_NOTI_RECV_MEMO;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	UINT	uiMemoKey;
} PACKET_CN_REQ_READ_MEMO;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	UINT	uiMemoKey;
	GRON_MEMO_INFO	memoInfo;
} PACKET_NC_ACK_READ_MEMO;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	UINT	uiMemoClass;
	UINT	uiMemoKey;
} PACKET_CN_REQ_DEL_MEMO;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	int		iResult;
	UINT	uiMemoClass;
	UINT	uiMemoKey;
} PACKET_NC_ACK_DEL_MEMO;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	UINT	uiMemoClass;
} PACKET_CN_REQ_READ_TOTAL_MEMO;


typedef struct {
	UINT			uiPacketType;
	UINT			uiBodySize;
	UINT			uiUserKey;
	UINT			uiMemoClass;
	UINT			uiMemoTotalCount;
	UINT			uiMemoCount;
	GRON_MEMO_INFO	memoInfo[MAX_MEMO_COUNT];
} PACKET_NC_ACK_READ_TOTAL_MEMO;


typedef struct {
	UINT			uiPacketType;
	UINT			uiBodySize;
	UINT			uiUserKey;
	UINT			uiMemoKey;
} PACKET_CN_REQ_CONFIRM_READ_MEMO;


typedef struct {
	UINT			uiPacketType;
	UINT			uiBodySize;
	UINT			uiUserKey;
	int				iResult;
	UINT			uiMemoKey;
} PACKET_NC_ACK_CONFIRM_READ_MEMO;




typedef struct {
	UINT			uiPacketType;
	UINT			uiBodySize;
	UINT			uiUserNo;
	UINT			uiRootKey;
	UINT			uiPreRoomNo;
} PACKET_CN_REQ_CHAT_CREATE;


typedef struct {
	UINT			uiPacketType;
	UINT			uiBodySize;
	UINT			uiUserNo;
	int					iResult;
	UINT			uiRootKey;
	UINT			uiRoomNo;
	UINT			uiPreRoomNo;
	char			cChatSvrIp[MAX_IPV4_ADDRESS_SIZE];
	UINT			uiChatSvrPort;
} PACKET_NC_ACK_CHAT_CREATE;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserNo;
	UINT	uiRootKey;
	UINT	uiRoomNo;
	char	cChatSvrIp[MAX_IPV4_ADDRESS_SIZE];
	UINT	uiChatSvrPort;
	UINT	uiCount;
	UINT	uiFriendList[MAX_CHAT_FRIEND_COUNT];
} PACKET_CN_REQ_CHAT_INVITE;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserNo;
	int		iResult;
	UINT	uiRootKey;
	UINT	uiRoomNo;
	UINT	uiCount;
	UINT	uiFriendList[MAX_CHAT_FRIEND_COUNT];
} PACKET_NC_ACK_CHAT_INVITE;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserNo;
	UINT	uiRootKey;
	UINT	uiRoomNo;
	UINT	uiFriendNo;
	char	cChatSvrIp[MAX_IPV4_ADDRESS_SIZE];
	UINT	uiChatSvrPort;
} PACKET_NC_NOTI_CHAT_INVITE;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserNo;
	int		iResult;
	UINT	uiRootKey;
	UINT	uiRoomNo;
	UINT	uiFriendNo;
} PACKET_CN_REQ_CHAT_CONNECT_FAIL;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserNo;
	int		iResult;
	UINT	uiRootKey;
	UINT	uiRoomNo;
	UINT	uiFriendNo;
} PACKET_NC_ACK_CHAT_CONNECT_FAIL;



typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserNo;
	int	iResult;
	UINT	uiRootKey;
	UINT	uiRoomNo;
	UINT	uiFriendNo;
} PACKET_NC_NOTI_CHAT_CONNECT_FAIL;


typedef struct {
	UINT			uiPacketType;
	UINT			uiBodySize;
	UINT			uiUserNo;
	UINT			uiRootKey;
	UINT			uiRoomNo;
	UINT			uiCount;
	UINT			uiFriendList[MAX_CHAT_FRIEND_COUNT];
} PACKET_CN_REQ_CHATTING_FRIEND_INFO;


typedef struct {
	UINT						uiPacketType;
	UINT						uiBodySize;
	UINT						uiUserKey;
	int							iResult;
	UINT						uiRootKey;
	UINT						uiRoomNumber;
	UINT						uiTotalFriendNumber;
	UINT						uiFriendCount;
	GRON_CHATTING_FRIEND_INFO	chattingFriendInfo[MAX_CHAT_FRIEND_COUNT];
} PACKET_NC_ACK_CHATTING_FRIEND_INFO;








typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
} PACKET_CH_NOTIFY_PING;




typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserNo;
	UINT	uiRoomNo;
} PACKET_CH_REQ_CHAT_INITALIZE;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserNo;
	int		iResult;
	UINT	uiRoomNo;
} PACKET_HC_ACK_CHAT_INITALIZE;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserNo;
	UINT	uiRoomNo;
} PACKET_CH_REQ_CHAT_SEARCH;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserNo;
	int		iResult;
	UINT	uiRoomNo;
} PACKET_HC_ACK_CHAT_SEARCH;


typedef struct {
	UINT			uiPacketType;
	UINT			uiBodySize;
	UINT			uiUserNo;
	UINT			uiRoomNo;
	UINT			uiCharacterSet;
	UINT			uiClientMode;
} PACKET_CH_REQ_CHAT_ENTER;


typedef struct {
	UINT				uiPacketType;
	UINT				uiBodySize;
	UINT				uiUserNo;
	int					iResult;
	UINT				uiRoomNo;
	UINT				uiCount;
	G_CHAT_FRIENDINFO	ChatFriendInfo[MAX_CHAT_FRIEND_COUNT];
} PACKET_HC_ACK_CHAT_ENTER;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserNo;
	UINT	uiRoomNo;
	UINT	uiFriendNo;
	UINT	uiClientMode;
} PACKET_HC_NOTI_CHAT_ENTER;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserNo;
	UINT	uiRoomNo;
	UINT	uiOpenMode;
} PACKET_CH_REQ_CHAT_OPENWINDOW;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserNo;
	int		iResult;
	UINT	uiRoomNo;
	UINT	uiOpenMode;
} PACKET_HC_ACK_CHAT_OPENWINDOW;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserNo;
	UINT	uiRoomNo;
} PACKET_CH_REQ_CHAT_CLOSE;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserNo;
	int		iResult;
	UINT	uiRoomNo;
	UINT	uiCloseMode;
} PACKET_HC_ACK_CHAT_CLOSE;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserNo;
	UINT	uiRoomNo;
	UINT	uiFriendNo;
	UINT	uiCloseMode;
} PACKET_HC_NOTI_CHAT_CLOSE;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserNo;
	UINT	uiRoomNo;
} PACKET_CH_REQ_CHAT_BYE;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserNo;
	int		iResult;
	UINT	uiRoomNo;
} PACKET_HC_ACK_CHAT_BYE;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserNo;
	UINT	uiRoomNo;
	UINT	uiCount;
	UINT	uiFriendList[MAX_CHAT_FRIEND_COUNT];
} PACKET_CH_REQ_CHAT_ADD_USER;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserNo;
	int		iResult;
	UINT	uiRoomNo;
	UINT	uiCount;
	UINT	uiFriendList[MAX_CHAT_FRIEND_COUNT];
} PACKET_HC_ACK_CHAT_ADD_USER;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserNo;
	UINT	uiRoomNo;
} PACKET_CH_REQ_CHAT_MSG_TICK;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserNo;
	UINT	uiRoomNo;
	UINT	uiFriendNo;
} PACKET_HC_NOTI_CHAT_MSG_TICK;


typedef struct {
	UINT		uiPacketType;
	UINT		uiBodySize;
	UINT		uiUserNo;
	UINT		uiRoomNo;
	G_FONT		Font;
	UINT		uiMsgSize;
	char		cMsg[MAX_MSG_SIZE2];
} PACKET_CH_REQ_CHAT_MESSAGE;


typedef struct {
	UINT		uiPacketType;
	UINT		uiBodySize;
	UINT		uiUserNo;
	UINT		uiRoomNo;
	UINT		uiFriendNo;
	G_FONT		Font;
	UINT		uiMsgSize;
	char		cMsg[MAX_MSG_SIZE2];
} PACKET_HC_NOTI_CHAT_MESSAGE;


typedef struct {
	UINT		uiPacketType;
	UINT		uiBodySize;
	UINT		uiUserNo;
	UINT		uiRoomNo;
	G_FONT		Font;
	UINT		uiMsgSize;
	char		cMsg[MAX_MSG_SIZE2];
} PACKET_ACK_CHAT_MESSAGE_SENDFAIL;






typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserNo;
	UINT	uiRoomNo;
	UINT	uiFriendNo;
} PACKET_CH_REQ_CHAT_SAVE_LOG;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserNo;
	UINT	uiRoomNo;
	UINT	uiFriendNo;
} PACKET_HC_REQ_CHAT_SAVE_LOG;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserNo;
	int		iResult;
	UINT	uiRoomNo;
	UINT	uiFriendNo;
} PACKET_HC_ACK_CHAT_SAVE_LOG;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserNo;
	int		iResult;
	UINT	uiRoomNo;
	UINT	uiFriendNo;
} PACKET_CH_ACK_CHAT_SAVE_LOG;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserNo;
	int		iResult;
	UINT	uiRoomNo;
} PACKET_CH_NOTI_CHAT_LOG_START;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserNo;
	UINT	uiRoomNo;
	UINT	uiFriendNo;
	UINT	uiMode;
} PACKET_CH_REQ_CHAT_END_LOG;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserNo;
	UINT	uiRoomNo;
	UINT	uiFriendNo;
	UINT	uiMode;
} PACKET_HC_REQ_CHAT_END_LOG;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserNo;
	int		iResult;
	UINT	uiRoomNo;
	UINT	uiFriendNo;
} PACKET_HC_ACK_CHAT_END_LOG;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserNo;
	int		iResult;
	UINT	uiRoomNo;
	UINT	uiFriendNo;
} PACKET_CH_ACK_CHAT_END_LOG;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserNo;
	int		iResult;
	UINT	uiRoomNo;
	UINT	uiFriendNo;
} PACKET_CH_NOTI_CHAT_LOG_STOP;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	UINT	uiRoomNumber;
	UINT	uiFriendKey;
} PACKET_CH_REQ_LOG_START;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	UINT	uiRoomNumber;
	UINT	uiFriendKey;
} PACKET_HC_REQ_LOG_START;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	int		iResult;
	UINT	uiRoomNumber;
	UINT	uiFriendKey;
} PACKET_CH_ACK_LOG_START;



typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	int		iResult;
	UINT	uiRoomNumber;
	UINT	uiFriendKey;
} PACKET_HC_ACK_LOG_START;



typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	int		iResult;
	UINT	uiRoomNumber;
} PACKET_HC_NOTI_LOG_READY;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	UINT	uiRoomNumber;
	UINT	uiFriendKey;
	UINT	uiMode;
} PACKET_CH_REQ_LOG_END;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	UINT	uiRoomNumber;
	UINT	uiFriendKey;
	UINT	uiMode;
} PACKET_HC_REQ_LOG_END;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	int		iResult;
	UINT	uiRoomNumber;
	UINT	uiFriendKey;
	UINT	uiMode;
} PACKET_HC_ACK_LOG_END;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	int		iResult;
	UINT	uiRoomNumber;
	UINT	uiFriendKey;
	UINT	uiMode;
} PACKET_CH_ACK_LOG_END;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	int		iResult;
	UINT	uiRoomNumber;
} PACKET_CH_NOTI_LOG_END;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	int		iResult;
	UINT	uiRoomNumber;
} PACKET_HC_NOTI_SAVE_LOG_FAIL;



typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	UINT	uiRoomNumber;
	UINT	uiFriendKey;
	UINT	uiPort;
	UINT	uiFileKey;
	unsigned __int64 i64FileSize;
	UINT	uiFileNameSize;
	char	CFileName[MAX_FILE_NAME_SIZE];
} PACKET_CH_REQ_FILE_SEND;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	int		iResult;
	UINT	uiRoomNumber;
	UINT	uiFriendKey;
	UINT	uiFileKey;
	char	cIpAddr[MAX_IPV4_ADDRESS_SIZE];
	UINT	uiPort;
} PACKET_HC_ACK_FILE_SEND;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	UINT	uiRoomNumber;
	UINT	uiFriendKey;
	char	cIpAddr[MAX_IPV4_ADDRESS_SIZE];
	UINT	uiPort;
	UINT	uiFileKey;
	unsigned __int64 i64FileSize;
	UINT	uiFileNameSize;
	char	CFileName[MAX_FILE_NAME_SIZE];
} PACKET_HC_REQ_FILE_SEND;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	int		iResult;
	UINT	uiRoomNumber;
	UINT	uiFriendKey;
	UINT	uiPort;
	UINT	uiFileKey;
} PACKET_CH_ACK_FILE_SEND;



typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	UINT	uiRoomNumber;
	UINT	uiFriendKey;
	UINT	uiFileKey;
} PACKET_CH_REQ_FILE_SEND_END;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	UINT	uiRoomNumber;
	UINT	uiFriendKey;
	UINT	uiFileKey;
} PACKET_HC_REQ_FILE_SEND_END;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	UINT	uiRoomNumber;
	UINT	uiFriendKey;
	UINT	uiFileKey;
	int		iResult;
} PACKET_CH_ACK_FILE_SEND_END;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	UINT	uiRoomNumber;
	UINT	uiFriendKey;
	UINT	uiFileKey;
	int		iResult;
} PACKET_HC_ACK_FILE_SEND_END;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	UINT	uiRoomNumber;
	UINT	uiFriendKey;
	UINT	uiFileKey;
} PACKET_CH_REQ_FILE_SEND_CANCEL;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	UINT	uiRoomNumber;
	UINT	uiFriendKey;
	UINT	uiFileKey;
} PACKET_HC_REQ_FILE_SEND_CANCEL;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	UINT	uiRoomNumber;
	UINT	uiFriendKey;
	UINT	uiFileKey;
	int		iResult;
} PACKET_CH_ACK_FILE_SEND_CANCEL;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	UINT	uiRoomNumber;
	UINT	uiFriendKey;
	UINT	uiFileKey;
	int		iResult;
} PACKET_HC_ACK_FILE_SEND_CANCEL;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	UINT	uiRoomNumber;
	UINT	uiFriendKey;
	UINT	uiFileKey;
} PACKET_CH_REQ_FILE_CREATE_ROOM;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	UINT	uiRoomNumber;
	int		iResult;
	UINT	uiFileKey;
	char	CGameFileRoomKey[MAX_FILE_ROOM_KEY];
	char	cIpAddr[MAX_IPV4_ADDRESS_SIZE];
	UINT	uiPort;
} PACKET_HC_ACK_FILE_CREATE_ROOM;


typedef struct {
	UINT	uiPacketType;
	UINT	uiBodySize;
	UINT	uiUserKey;
	UINT	uiRoomNumber;
	int		iResult;
	UINT	uiFileKey;
	char	CGameFileRoomKey[MAX_FILE_ROOM_KEY];
	char	cIpAddr[MAX_IPV4_ADDRESS_SIZE];
	UINT	uiPort;
} PACKET_HC_NOTI_FILE_CREATE_ROOM;


#pragma pack(pop)

#endif
