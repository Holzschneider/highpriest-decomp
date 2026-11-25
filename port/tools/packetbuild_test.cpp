// bifrost-packetbuild-test (port/tools/packetbuild_test.sh): compares each M10.3 builder with the 2008 client's own construction of the
// request (the code the builders replaced), byte for byte.
#include "common.h"
#include "packet.h"
#include "PaddingPacket.h"
#include <map>
#include <cstdio>
#include <cstring>
extern PacketLenMap g_packetLenMap;
static int s_seq;
void FillPacketPadding(char *pad, int len) { for (int i = 0; i < len; i++) pad[i] = (char)('a' + (s_seq++ % 26)); pad[len-1] = 0; }
static void SetPaddingValue(char *pad, int len) { FillPacketPadding(pad, len); }
static int fails, checks;
static void cmp(const char *name, const void *old, int oldLen, const char *nw, int newLen)
{
	checks++;
	if (oldLen != newLen || memcmp(old, nw, oldLen)) { fails++; printf("DIFFER %s (old %d, new %d)\n", name, oldLen, newLen); }
	else printf("same   %-45s %3d bytes, header 0x%04x\n", name, newLen, *(unsigned short *)nw & 0xffff);
}
#define BUF char nb[PADDING_PACKET_BUFSIZE]; memset(nb, 0x55, sizeof(nb))
#define OLD(T) T p; memset(&p, 0x55, sizeof(p)); s_seq = 0
int main()
{
	InitPacketLenWithClient();
	{ OLD(PACKET_CZ_ENTER); p.PacketType = 0x9b; SetPaddingValue(p.pad, 2); SetPaddingValue(p.pad2, 1); SetPaddingValue(p.pad3, 4);
	  p.clientTime = 0x11223344; p.AID = 2000101; p.GID = 150001; p.AuthCode = 0x55667788; p.Sex = 1;
	  BUF; s_seq = 0; int n = BuildCzEnter(nb, 2000101, 150001, 0x55667788, 0x11223344, 1); cmp("CZ_ENTER", &p, g_packetLenMap[0x9b], nb, n); }
	{ OLD(PACKET_CZ_REQUEST_MOVE); p.PacketType = 0xa7; SetPaddingValue(p.pad, 3); EncodePosDir(155, 183, 0, p.dest);
	  BUF; s_seq = 0; int n = BuildCzRequestMove(nb, 155, 183); cmp("CZ_REQUEST_MOVE", &p, g_packetLenMap[0xa7], nb, n); }
	{ OLD(PACKET_CZ_REQUEST_ACT); p.PacketType = 0x190; SetPaddingValue(p.pad, 3); SetPaddingValue(p.pad2, 9); p.action = 7; p.targetGID = 110000123;
	  BUF; s_seq = 0; int n = BuildCzRequestAct(nb, 110000123, 7); cmp("CZ_REQUEST_ACT", &p, g_packetLenMap[0x190], nb, n); }
	{ OLD(PACKET_CZ_REQUEST_TIME); p.PacketType = 0x89; SetPaddingValue(p.pad, sizeof(p.pad)); p.clientTime = 0xdeadbeef;
	  BUF; s_seq = 0; int n = BuildCzRequestTime(nb, 0xdeadbeef); cmp("CZ_REQUEST_TIME", &p, g_packetLenMap[0x89], nb, n); }
	{ OLD(PACKET_CZ_REQNAME); p.PacketType = 0x8c; SetPaddingValue(p.pad, sizeof(p.pad)); p.AID = 2000222;
	  BUF; s_seq = 0; int n = BuildCzReqname(nb, 2000222); cmp("CZ_REQNAME", &p, g_packetLenMap[0x8c], nb, n); }
	{ OLD(PACKET_CZ_REQNAME_BYGID); p.PacketType = 0xa2; SetPaddingValue(p.pad, sizeof(p.pad)); p.GID = 150333;
	  BUF; s_seq = 0; int n = BuildCzReqnameBygid(nb, 150333); cmp("CZ_REQNAME_BYGID", &p, g_packetLenMap[0xa2], nb, n); }
	{ OLD(PACKET_CZ_CHANGE_DIRECTION); p.PacketType = 0x85; SetPaddingValue(p.pad, 5); SetPaddingValue(p.pad2, 1); p.dir = 3; p.headDir = 2;
	  BUF; s_seq = 0; int n = BuildCzChangeDirection(nb, 2, 3); cmp("CZ_CHANGE_DIRECTION", &p, g_packetLenMap[0x85], nb, n); }
	{ OLD(PACKET_CZ_ITEM_PICKUP); p.PacketType = 0xf5; SetPaddingValue(p.pad, 2); p.ITAID = 4321;
	  BUF; s_seq = 0; int n = BuildCzItemPickup(nb, 4321); cmp("CZ_ITEM_PICKUP", &p, g_packetLenMap[0xf5], nb, n); }
	{ OLD(PACKET_CZ_ITEM_THROW); p.PacketType = 0x116; SetPaddingValue(p.pad, 3); SetPaddingValue(p.pad2, 1); p.Index = 5; p.count = 9;
	  BUF; s_seq = 0; int n = BuildCzItemThrow(nb, 5, 9); cmp("CZ_ITEM_THROW", &p, g_packetLenMap[0x116], nb, n); }
	{ OLD(PACKET_CZ_USE_ITEM); p.PacketType = 0x9f; SetPaddingValue(p.pad, 2); SetPaddingValue(p.pad2, 4); p.AID = 2000101; p.index = 6;
	  BUF; s_seq = 0; int n = BuildCzUseItem(nb, 6, 2000101); cmp("CZ_USE_ITEM", &p, g_packetLenMap[0x9f], nb, n); }
	{ OLD(PACKET_CZ_MOVE_ITEM_FROM_BODY_TO_STORE); p.PacketType = 0x94; SetPaddingValue(p.pad, 5); SetPaddingValue(p.pad2, 1); p.index = 4; p.count = 40;
	  BUF; s_seq = 0; int n = BuildCzMoveItemFromBodyToStore(nb, 4, 40); cmp("CZ_MOVE_ITEM_FROM_BODY_TO_STORE", &p, g_packetLenMap[0x94], nb, n); }
	{ OLD(PACKET_CZ_MOVE_ITEM_FROM_STORE_TO_BODY); p.PacketType = 0xf7; SetPaddingValue(p.pad, 12); SetPaddingValue(p.pad2, 2); p.index = 1; p.count = 1;
	  BUF; s_seq = 0; int n = BuildCzMoveItemFromStoreToBody(nb, 1, 1); cmp("CZ_MOVE_ITEM_FROM_STORE_TO_BODY", &p, g_packetLenMap[0xf7], nb, n); }
	{ OLD(PACKET_CZ_USE_SKILL); p.PacketType = 0x72; SetPaddingValue(p.pad, 4); SetPaddingValue(p.pad2, 2); SetPaddingValue(p.pad3, 9);
	  p.selectedLevel = 5; p.SKID = 5; p.targetID = 110000999;
	  BUF; s_seq = 0; int n = BuildCzUseSkill(nb, 5, 5, 110000999); cmp("CZ_USE_SKILL", &p, g_packetLenMap[0x72], nb, n); }
	// The 2008 code left pad4 (and every pad of the talk-box form) unwritten; the
	// old side writes them in the builder's order so that the fields are compared.
	{ OLD(PACKET_CZ_USE_SKILL_TOGROUND); p.PacketType = 275; SetPaddingValue(p.pad, 3); SetPaddingValue(p.pad2, 2); SetPaddingValue(p.pad3, 1); SetPaddingValue(p.pad4, 6);
	  p.selectedLevel = 3; p.SKID = 12; p.xPos = 150; p.yPos = 180;
	  BUF; s_seq = 0; int n = BuildCzUseSkillToground(nb, 3, 12, 150, 180); cmp("CZ_USE_SKILL_TOGROUND (pad4 padded)", &p, g_packetLenMap[275], nb, n); }
	{ OLD(PACKET_CZ_USE_SKILL_TOGROUND_WITHTALKBOX); p.PacketType = 126; SetPaddingValue(p.pad, 3); SetPaddingValue(p.pad2, 2); SetPaddingValue(p.pad3, 1); SetPaddingValue(p.pad4, 6);
	  p.selectedLevel = 1; p.SKID = 220; p.xPos = 151; p.yPos = 181; memset(p.contents, 0x55, sizeof(p.contents)); strcpy(p.contents, "graffiti");
	  BUF; s_seq = 0; int n = BuildCzUseSkillTogroundWithTalkbox(nb, 1, 220, 151, 181, "graffiti"); cmp("CZ_USE_SKILL_TOGROUND_WITHTALKBOX (pads padded)", &p, g_packetLenMap[126], nb, n); }
	printf("buildertest: %d of %d requests byte-identical\n", checks - fails, checks);
	return fails != 0;
}
