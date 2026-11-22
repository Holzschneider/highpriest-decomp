



#include "common.h"
#include "GronPacket.h"

USHORT g_PacketSize[USHRT_MAX];

void InitializePacketSize(void)
{
	g_PacketSize[HEADER_CD_REQ_NS_IP]                   = sizeof(PACKET_CD_REQ_NS_IP);
	g_PacketSize[HEADER_DC_ACK_NS_IP]                   = USHRT_MAX;

	g_PacketSize[HEADER_CN_REQ_LOGIN]                   = sizeof(PACKET_CN_REQ_LOGIN);
	g_PacketSize[HEADER_NC_ACK_LOGIN]                   = sizeof(PACKET_NC_ACK_LOGIN);
	g_PacketSize[HEADER_NC_NOTI_DUPLE_LOGIN]            = sizeof(PACKET_NC_NOTI_DUPLE_LOGIN);
	g_PacketSize[HEADER_CN_REQ_LOGIN_CONFIRM]           = sizeof(PACKET_CN_REQ_LOGIN_CONFIRM);
	g_PacketSize[HEADER_NC_ACK_LOGIN_CONFIRM]           = sizeof(PACKET_NC_ACK_LOGIN_CONFIRM);
	g_PacketSize[HEADER_CN_REQ_LOGOUT]                  = sizeof(PACKET_CN_REQ_LOGOUT);
	g_PacketSize[HEADER_NC_ACK_LOGOUT]                  = sizeof(PACKET_NC_ACK_LOGOUT);
	g_PacketSize[HEADER_CN_REQ_MYINFO]                  = sizeof(PACKET_CN_REQ_MYINFO);
	g_PacketSize[HEADER_NC_ACK_MYINFO]                  = USHRT_MAX;

	g_PacketSize[HEADER_CN_REQ_SYNC]                    = sizeof(PACKET_CN_REQ_SYNC);
	g_PacketSize[HEADER_NC_ACK_ALL_GROUPINFO]           = USHRT_MAX;
	g_PacketSize[HEADER_NC_ACK_ALL_FRIENDINFO]          = USHRT_MAX;
	g_PacketSize[HEADER_NC_ACK_UPDATED_GROUP_NAME]      = USHRT_MAX;
	g_PacketSize[HEADER_NC_ACK_UPDATED_GROUP_DEPTH]     = USHRT_MAX;
	g_PacketSize[HEADER_NC_ACK_DELETED_GROUP_INFO]      = USHRT_MAX;
	g_PacketSize[HEADER_NC_ACK_ADDED_GROUP_INFO]        = USHRT_MAX;

	g_PacketSize[HEADER_NC_ACK_ADDED_FRIEND_INFO]       = USHRT_MAX;
	g_PacketSize[HEADER_NC_ACK_UPDATED_FRIEND_INFO]     = USHRT_MAX;
	g_PacketSize[HEADER_NC_ACK_UPDATED_FRIEND_GROUP]    = USHRT_MAX;
	g_PacketSize[HEADER_NC_ACK_UPDATED_FRIEND_BLOCK]    = USHRT_MAX;
	g_PacketSize[HEADER_NC_ACK_DELETED_FRIEND_INFO]     = USHRT_MAX;

	g_PacketSize[HEADER_CN_REQ_FRIEND_STATUS]           = sizeof(PACKET_CN_REQ_FRIEND_STATUS);
	g_PacketSize[HEADER_NC_ACK_FRIEND_STATUS]           = USHRT_MAX;

	g_PacketSize[HEADER_CN_REQ_CHANGE_CONDITION]        = sizeof(PACKET_CN_REQ_CHANGE_CONDITION);
	g_PacketSize[HEADER_NC_ACK_CHANGE_CONDITION]        = sizeof(PACKET_NC_ACK_CHANGE_CONDITION);
	g_PacketSize[HEADER_NC_NOTI_CHANGE_CONDITION]       = sizeof(PACKET_NC_NOTI_CHANGE_CONDITION);
	g_PacketSize[HEADER_CN_REQ_CHANGE_NICKNAME]         = USHRT_MAX;
	g_PacketSize[HEADER_NC_ACK_CHANGE_NICKNAME]         = sizeof(PACKET_NC_ACK_CHANGE_NICKNAME);
	g_PacketSize[HEADER_NC_NOTI_CHANGE_NICKNAME]        = USHRT_MAX;
	g_PacketSize[HEADER_CN_REQ_ADD_GROUP]               = USHRT_MAX;
	g_PacketSize[HEADER_NC_ACK_ADD_GROUP]               = sizeof(PACKET_NC_ACK_ADD_GROUP);
	g_PacketSize[HEADER_CN_REQ_DEL_GROUP]               = sizeof(PACKET_CN_REQ_DEL_GROUP);
	g_PacketSize[HEADER_NC_ACK_DEL_GROUP]               = sizeof(PACKET_NC_ACK_DEL_GROUP);
	g_PacketSize[HEADER_CN_REQ_CHANGE_GROUP_NAME]       = USHRT_MAX;
	g_PacketSize[HEADER_NC_ACK_CHANGE_GROUP_NAME]       = sizeof(PACKET_NC_ACK_CHANGE_GROUP_NAME);

	g_PacketSize[HEADER_CN_REQ_GROUP_DEPTH]             = sizeof(PACKET_CN_REQ_GROUP_DEPTH);
	g_PacketSize[HEADER_NC_ACK_GROUP_DEPTH]             = sizeof(PACKET_NC_ACK_GROUP_DEPTH);
	g_PacketSize[HEADER_CN_REQ_ADD_FRIEND]              = sizeof(PACKET_CN_REQ_ADD_FRIEND);
	g_PacketSize[HEADER_NC_ACK_ADD_FRIEND]              = USHRT_MAX;
	g_PacketSize[HEADER_NC_REQ_AGREE_FRIEND]            = USHRT_MAX;
	g_PacketSize[HEADER_CN_ACK_AGREE_FRIEND]            = sizeof(PACKET_CN_ACK_AGREE_FRIEND);
	g_PacketSize[HEADER_NC_ACK_AGREE_FRIEND]            = sizeof(PACKET_NC_ACK_AGREE_FRIEND);

	g_PacketSize[HEADER_CN_REQ_DEL_FRIEND]              = sizeof(PACKET_CN_REQ_DEL_FRIEND);
	g_PacketSize[HEADER_NC_ACK_DEL_FRIEND]              = sizeof(PACKET_NC_ACK_DEL_FRIEND);
	g_PacketSize[HEADER_CN_REQ_CHANGEGROUP_FRIEND]      = sizeof(PACKET_CN_REQ_CHANGEGROUP_FRIEND);
	g_PacketSize[HEADER_NC_ACK_CHANGEGROUP_FRIEND]      = sizeof(PACKET_NC_ACK_CHANGEGROUP_FRIEND);

	g_PacketSize[HEADER_CN_REQ_BLOCK_FRIEND]            = sizeof(PACKET_CN_REQ_BLOCK_FRIEND);
	g_PacketSize[HEADER_NC_ACK_BLOCK_FRIEND]            = sizeof(PACKET_NC_ACK_BLOCK_FRIEND);
	g_PacketSize[HEADER_NC_NOTI_BLOCK_FRIEND]           = sizeof(PACKET_NC_NOTI_BLOCK_FRIEND);


	g_PacketSize[HEADER_CN_REQ_CHAT_CREATE]             = sizeof(PACKET_CN_REQ_CHAT_CREATE);
	g_PacketSize[HEADER_NC_ACK_CHAT_CREATE]             = sizeof(PACKET_NC_ACK_CHAT_CREATE);

	g_PacketSize[HEADER_CN_REQ_CHAT_INVITE]             = USHRT_MAX;
	g_PacketSize[HEADER_NC_ACK_CHAT_INVITE]             = USHRT_MAX;
	g_PacketSize[HEADER_NC_NOTI_CHAT_INVITE]            = sizeof(PACKET_NC_NOTI_CHAT_INVITE);

	g_PacketSize[HEADER_CN_REQ_CHAT_CONNECT_FAIL]       = sizeof(PACKET_CN_REQ_CHAT_CONNECT_FAIL);
	g_PacketSize[HEADER_NC_ACK_CHAT_CONNECT_FAIL]       = sizeof(PACKET_NC_ACK_CHAT_CONNECT_FAIL);
	g_PacketSize[HEADER_NC_NOTI_CHAT_CONNECT_FAIL]      = sizeof(PACKET_NC_NOTI_CHAT_CONNECT_FAIL);

	g_PacketSize[HEADER_CN_REQ_CHATTING_FRIEND_INFO]    = USHRT_MAX;
	g_PacketSize[HEADER_NC_ACK_CHATTING_FRIEND_INFO]    = USHRT_MAX;


	g_PacketSize[HEADER_CN_REQ_SEND_MEMO]               = USHRT_MAX;
	g_PacketSize[HEADER_NC_ACK_SEND_MEMO]               = sizeof(PACKET_NC_ACK_SEND_MEMO);
	g_PacketSize[HEADER_NC_NOTI_RECV_MEMO]              = sizeof(PACKET_NC_NOTI_RECV_MEMO);
	g_PacketSize[HEADER_CN_REQ_READ_MEMO]               = sizeof(PACKET_CN_REQ_READ_MEMO);
	g_PacketSize[HEADER_NC_ACK_READ_MEMO]               = USHRT_MAX;
	g_PacketSize[HEADER_CN_REQ_DEL_MEMO]                = sizeof(PACKET_CN_REQ_DEL_MEMO);
	g_PacketSize[HEADER_NC_ACK_DEL_MEMO]                = sizeof(PACKET_NC_ACK_DEL_MEMO);
	g_PacketSize[HEADER_CN_REQ_READ_TOTAL_MEMO]         = sizeof(PACKET_CN_REQ_READ_TOTAL_MEMO);
	g_PacketSize[HEADER_NC_ACK_READ_TOTAL_MEMO]         = USHRT_MAX;
	g_PacketSize[HEADER_CN_REQ_CONFIRM_READ_MEMO]       = sizeof(PACKET_CN_REQ_CONFIRM_READ_MEMO);
	g_PacketSize[HEADER_NC_ACK_CONFIRM_READ_MEMO]       = sizeof(PACKET_NC_ACK_CONFIRM_READ_MEMO);





	g_PacketSize[HEADER_CH_NOTIFY_PING]                 = sizeof(PACKET_CH_NOTIFY_PING);

	g_PacketSize[HEADER_CH_REQ_CHAT_ENTER]              = sizeof(PACKET_CH_REQ_CHAT_ENTER);
	g_PacketSize[HEADER_HC_ACK_CHAT_ENTER]              = USHRT_MAX;
	g_PacketSize[HEADER_HC_NOTI_CHAT_ENTER]             = sizeof(PACKET_HC_NOTI_CHAT_ENTER);

	g_PacketSize[HEADER_CH_REQ_CHAT_OPENWINDOW]         = sizeof(PACKET_CH_REQ_CHAT_OPENWINDOW);
	g_PacketSize[HEADER_HC_ACK_CHAT_OPENWINDOW]         = sizeof(PACKET_HC_ACK_CHAT_OPENWINDOW);

	g_PacketSize[HEADER_CH_REQ_CHAT_ADD_USER]           = USHRT_MAX;
	g_PacketSize[HEADER_HC_ACK_CHAT_ADD_USER]           = USHRT_MAX;

	g_PacketSize[HEADER_CH_REQ_CHAT_MSG_TICK]           = sizeof(PACKET_CH_REQ_CHAT_MSG_TICK);
	g_PacketSize[HEADER_HC_NOTI_CHAT_MSG_TICK]          = sizeof(PACKET_HC_NOTI_CHAT_MSG_TICK);
	g_PacketSize[HEADER_CH_REQ_CHAT_MESSAGE]            = USHRT_MAX;
	g_PacketSize[HEADER_HC_NOTI_CHAT_MESSAGE]           = USHRT_MAX;
	g_PacketSize[HEADER_HC_ACK_CHAT_MESSAGE_SENDFAIL]   = USHRT_MAX;

	g_PacketSize[HEADER_CH_REQ_CHAT_CLOSE]              = sizeof(PACKET_CH_REQ_CHAT_CLOSE);
	g_PacketSize[HEADER_HC_ACK_CHAT_CLOSE]              = sizeof(PACKET_HC_ACK_CHAT_CLOSE);
	g_PacketSize[HEADER_HC_NOTI_CHAT_CLOSE]             = sizeof(PACKET_HC_NOTI_CHAT_CLOSE);

	g_PacketSize[HEADER_CH_REQ_CHAT_BYE]                = sizeof(PACKET_CH_REQ_CHAT_BYE);
	g_PacketSize[HEADER_HC_ACK_CHAT_BYE]                = sizeof(PACKET_HC_ACK_CHAT_BYE);

	g_PacketSize[HEADER_CH_REQ_LOG_START]               = sizeof(PACKET_CH_REQ_LOG_START);
	g_PacketSize[HEADER_HC_REQ_LOG_START]               = sizeof(PACKET_HC_REQ_LOG_START);
	g_PacketSize[HEADER_CH_ACK_LOG_START]               = sizeof(PACKET_CH_ACK_LOG_START);
	g_PacketSize[HEADER_HC_ACK_LOG_START]               = sizeof(PACKET_HC_ACK_LOG_START);
	g_PacketSize[HEADER_HC_NOTI_LOG_READY]              = sizeof(PACKET_HC_NOTI_LOG_READY);

	g_PacketSize[HEADER_CH_REQ_LOG_END]                 = sizeof(PACKET_CH_REQ_LOG_END);
	g_PacketSize[HEADER_HC_REQ_LOG_END]                 = sizeof(PACKET_HC_REQ_LOG_END);
	g_PacketSize[HEADER_HC_ACK_LOG_END]                 = sizeof(PACKET_HC_ACK_LOG_END);
	g_PacketSize[HEADER_CH_ACK_LOG_END]                 = sizeof(PACKET_CH_ACK_LOG_END);
	g_PacketSize[HEADER_CH_NOTI_LOG_END]                = sizeof(PACKET_CH_NOTI_LOG_END);

	g_PacketSize[HEADER_HC_NOTI_SAVE_LOG_FAIL]          = sizeof(PACKET_HC_NOTI_SAVE_LOG_FAIL);
}
