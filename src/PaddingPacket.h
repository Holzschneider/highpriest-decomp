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


// The requests above are the ones whose ids and layouts the later packet
// versions change (port/tools/packetdiff.py, 20080910 on: 15 requests). Each
// is built here and nowhere else (M10.3): a sender passes the fields, the
// builder writes header, padding and fields into buf and returns the length
// to send, taken from packet.cpp's table. The client receives none of them,
// so there is no parsing side. docs/port/packet-versions.md says what a
// later version changes here.
const int	PADDING_PACKET_BUFSIZE = 256;	// enough for any of them, in any version

// The padding bytes the 2008 client writes (CGameMode::SetPaddingValue's);
// defined in GameMode.cpp.
void	FillPacketPadding(char *pad, int len);

inline	int		BuildCzEnter(char *buf, DWORD AID, DWORD GID, int AuthCode, DWORD clientTime, BYTE Sex)
{
	PACKET_CZ_ENTER *p = (PACKET_CZ_ENTER *)buf;
	p->PacketType = HEADER_CZ_ENTER;
	FillPacketPadding(p->pad, sizeof(p->pad));
	FillPacketPadding(p->pad2, sizeof(p->pad2));
	FillPacketPadding(p->pad3, sizeof(p->pad3));
	p->clientTime = clientTime;
	p->AID = AID;
	p->GID = GID;
	p->AuthCode = AuthCode;
	p->Sex = Sex;
	return GetPacketSize(HEADER_CZ_ENTER);
}

inline	int		BuildCzRequestMove(char *buf, int x, int y)
{
	PACKET_CZ_REQUEST_MOVE *p = (PACKET_CZ_REQUEST_MOVE *)buf;
	p->PacketType = HEADER_CZ_REQUEST_MOVE;
	FillPacketPadding(p->pad, sizeof(p->pad));
	EncodePosDir(x, y, 0, p->dest);
	return GetPacketSize(HEADER_CZ_REQUEST_MOVE);
}

inline	int		BuildCzRequestAct(char *buf, DWORD targetGID, BYTE action)
{
	PACKET_CZ_REQUEST_ACT *p = (PACKET_CZ_REQUEST_ACT *)buf;
	p->PacketType = HEADER_CZ_REQUEST_ACT;
	FillPacketPadding(p->pad, sizeof(p->pad));
	FillPacketPadding(p->pad2, sizeof(p->pad2));
	p->action = action;
	p->targetGID = targetGID;
	return GetPacketSize(HEADER_CZ_REQUEST_ACT);
}

inline	int		BuildCzRequestTime(char *buf, DWORD clientTime)
{
	PACKET_CZ_REQUEST_TIME *p = (PACKET_CZ_REQUEST_TIME *)buf;
	p->PacketType = HEADER_CZ_REQUEST_TIME;
	FillPacketPadding(p->pad, sizeof(p->pad));
	p->clientTime = clientTime;
	return GetPacketSize(HEADER_CZ_REQUEST_TIME);
}

inline	int		BuildCzReqnameBygid(char *buf, DWORD GID)
{
	PACKET_CZ_REQNAME_BYGID *p = (PACKET_CZ_REQNAME_BYGID *)buf;
	p->PacketType = HEADER_CZ_REQNAME_BYGID;
	FillPacketPadding(p->pad, sizeof(p->pad));
	p->GID = GID;
	return GetPacketSize(HEADER_CZ_REQNAME_BYGID);
}

inline	int		BuildCzReqname(char *buf, DWORD AID)
{
	PACKET_CZ_REQNAME *p = (PACKET_CZ_REQNAME *)buf;
	p->PacketType = HEADER_CZ_REQNAME;
	FillPacketPadding(p->pad, sizeof(p->pad));
	p->AID = AID;
	return GetPacketSize(HEADER_CZ_REQNAME);
}

inline	int		BuildCzChangeDirection(char *buf, short headDir, BYTE dir)
{
	PACKET_CZ_CHANGE_DIRECTION *p = (PACKET_CZ_CHANGE_DIRECTION *)buf;
	p->PacketType = HEADER_CZ_CHANGE_DIRECTION;
	FillPacketPadding(p->pad, sizeof(p->pad));
	FillPacketPadding(p->pad2, sizeof(p->pad2));
	p->dir = dir;
	p->headDir = headDir;
	return GetPacketSize(HEADER_CZ_CHANGE_DIRECTION);
}

inline	int		BuildCzItemPickup(char *buf, DWORD ITAID)
{
	PACKET_CZ_ITEM_PICKUP *p = (PACKET_CZ_ITEM_PICKUP *)buf;
	p->PacketType = HEADER_CZ_ITEM_PICKUP;
	FillPacketPadding(p->pad, sizeof(p->pad));
	p->ITAID = ITAID;
	return GetPacketSize(HEADER_CZ_ITEM_PICKUP);
}

inline	int		BuildCzItemThrow(char *buf, WORD Index, short count)
{
	PACKET_CZ_ITEM_THROW *p = (PACKET_CZ_ITEM_THROW *)buf;
	p->PacketType = HEADER_CZ_ITEM_THROW;
	FillPacketPadding(p->pad, sizeof(p->pad));
	FillPacketPadding(p->pad2, sizeof(p->pad2));
	p->Index = Index;
	p->count = count;
	return GetPacketSize(HEADER_CZ_ITEM_THROW);
}

inline	int		BuildCzUseItem(char *buf, WORD index, DWORD AID)
{
	PACKET_CZ_USE_ITEM *p = (PACKET_CZ_USE_ITEM *)buf;
	p->PacketType = HEADER_CZ_USE_ITEM;
	FillPacketPadding(p->pad, sizeof(p->pad));
	FillPacketPadding(p->pad2, sizeof(p->pad2));
	p->AID = AID;
	p->index = index;
	return GetPacketSize(HEADER_CZ_USE_ITEM);
}

inline	int		BuildCzMoveItemFromBodyToStore(char *buf, short index, int count)
{
	PACKET_CZ_MOVE_ITEM_FROM_BODY_TO_STORE *p = (PACKET_CZ_MOVE_ITEM_FROM_BODY_TO_STORE *)buf;
	p->PacketType = HEADER_CZ_MOVE_ITEM_FROM_BODY_TO_STORE;
	FillPacketPadding(p->pad, sizeof(p->pad));
	FillPacketPadding(p->pad2, sizeof(p->pad2));
	p->index = index;
	p->count = count;
	return GetPacketSize(HEADER_CZ_MOVE_ITEM_FROM_BODY_TO_STORE);
}

inline	int		BuildCzMoveItemFromStoreToBody(char *buf, short index, int count)
{
	PACKET_CZ_MOVE_ITEM_FROM_STORE_TO_BODY *p = (PACKET_CZ_MOVE_ITEM_FROM_STORE_TO_BODY *)buf;
	p->PacketType = HEADER_CZ_MOVE_ITEM_FROM_STORE_TO_BODY;
	FillPacketPadding(p->pad, sizeof(p->pad));
	FillPacketPadding(p->pad2, sizeof(p->pad2));
	p->index = index;
	p->count = count;
	return GetPacketSize(HEADER_CZ_MOVE_ITEM_FROM_STORE_TO_BODY);
}

inline	int		BuildCzUseSkill(char *buf, short selectedLevel, WORD SKID, DWORD targetID)
{
	PACKET_CZ_USE_SKILL *p = (PACKET_CZ_USE_SKILL *)buf;
	p->PacketType = HEADER_CZ_USE_SKILL;
	FillPacketPadding(p->pad, sizeof(p->pad));
	FillPacketPadding(p->pad2, sizeof(p->pad2));
	FillPacketPadding(p->pad3, sizeof(p->pad3));
	p->selectedLevel = selectedLevel;
	p->SKID = SKID;
	p->targetID = targetID;
	return GetPacketSize(HEADER_CZ_USE_SKILL);
}

// The 2008 client left pad4, and every pad of the talk-box form, as the
// stack had them; the builders pad them like the rest.
inline	void	FillCzUseSkillToground(PACKET_CZ_USE_SKILL_TOGROUND *p, short selectedLevel, WORD SKID, short xPos, short yPos)
{
	FillPacketPadding(p->pad, sizeof(p->pad));
	FillPacketPadding(p->pad2, sizeof(p->pad2));
	FillPacketPadding(p->pad3, sizeof(p->pad3));
	FillPacketPadding(p->pad4, sizeof(p->pad4));
	p->selectedLevel = selectedLevel;
	p->SKID = SKID;
	p->xPos = xPos;
	p->yPos = yPos;
}

inline	int		BuildCzUseSkillToground(char *buf, short selectedLevel, WORD SKID, short xPos, short yPos)
{
	PACKET_CZ_USE_SKILL_TOGROUND *p = (PACKET_CZ_USE_SKILL_TOGROUND *)buf;
	p->PacketType = HEADER_CZ_USE_SKILL_TOGROUND;
	FillCzUseSkillToground(p, selectedLevel, SKID, xPos, yPos);
	return GetPacketSize(HEADER_CZ_USE_SKILL_TOGROUND);
}

inline	int		BuildCzUseSkillTogroundWithTalkbox(char *buf, short selectedLevel, WORD SKID, short xPos, short yPos, const char *contents)
{
	PACKET_CZ_USE_SKILL_TOGROUND_WITHTALKBOX *p = (PACKET_CZ_USE_SKILL_TOGROUND_WITHTALKBOX *)buf;
	p->PacketType = HEADER_CZ_USE_SKILL_TOGROUND_WITHTALKBOX;
	FillCzUseSkillToground(p, selectedLevel, SKID, xPos, yPos);
	strcpy(p->contents, contents);
	return GetPacketSize(HEADER_CZ_USE_SKILL_TOGROUND_WITHTALKBOX);
}

#endif

#endif
