#ifndef __PADDINGPACKET_H
#define	__PADDINGPACKET_H









#if _MSC_VER > 1000
#pragma once
#pragma warning(disable : 4786)
#pragma warning(disable : 4150)
#endif

#if !defined COMPLEX_PACKET

#pragma pack(1)


struct	PACKET_CZ_ENTER
{
	short	PacketType;
	char	pad[2];
	DWORD	AID;
	char	pad2[1];
	DWORD	GID;
	char	pad3[4];
	int		AuthCode;
	DWORD	clientTime;
	BYTE	Sex;
};


struct	PACKET_CZ_REQUEST_MOVE
{
	short	PacketType;
	char	pad[3];
	BYTE	dest[3];
};


struct	PACKET_CZ_REQUEST_ACT
{
	short	PacketType;
	char	pad[3];
	DWORD	targetGID;
	char	pad2[9];
	BYTE	action;
};


struct	PACKET_CZ_REQUEST_TIME
{
	short	PacketType;
	char	pad[2];
	DWORD	clientTime;
};

struct	PACKET_CZ_REQNAME_BYGID
{
	short	PacketType;
	char	pad[9];
	DWORD	GID;
};

struct	PACKET_CZ_REQNAME
{
	short	PacketType;
	char	pad[5];
	DWORD	AID;
};


struct	PACKET_CZ_CHANGE_DIRECTION
{
	short	PacketType;
	char	pad[5];
	short	headDir;
	char	pad2[1];
	BYTE	dir;
};


struct	PACKET_CZ_ITEM_PICKUP
{
	short	PacketType;
	char	pad[2];
	DWORD	ITAID;
};


struct	PACKET_CZ_ITEM_THROW
{
	short	PacketType;
	char	pad[3];
	WORD	Index;
	char	pad2[1];
	short	count;
};


struct	PACKET_CZ_USE_ITEM
{
	short	PacketType;
	char	pad[2];
	WORD	index;
	char	pad2[4];
	DWORD	AID;
};


struct	PACKET_CZ_MOVE_ITEM_FROM_BODY_TO_STORE
{
	short	PacketType;
	char	pad[5];
	short	index;
	char	pad2[1];
	int		count;
};


struct	PACKET_CZ_MOVE_ITEM_FROM_STORE_TO_BODY
{
	short	PacketType;
	char	pad[12];
	short	index;
	char	pad2[2];
	int		count;
};


struct	PACKET_CZ_USE_SKILL
{
	short	PacketType;
	char	pad[4];
	short	selectedLevel;
	char	pad2[2];
	WORD	SKID;
	char	pad3[9];
	DWORD	targetID;
};


struct	PACKET_CZ_USE_SKILL_TOGROUND
{
	short	PacketType;
	char	pad[3];
	short	selectedLevel;
	char	pad2[2];
	WORD	SKID;
	char	pad3[1];
	short	xPos;
	char	pad4[6];
	short	yPos;
};

struct	PACKET_CZ_USE_SKILL_TOGROUND_WITHTALKBOX : 	PACKET_CZ_USE_SKILL_TOGROUND
{
	char	contents[TALKBOX_CHAT_LENGTH];
};


#pragma pack()

#endif

#endif
