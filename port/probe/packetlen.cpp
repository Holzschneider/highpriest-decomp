/*
 * bifrost-packetlen: prints the client's packet length table, one "0xNNNN length"
 * line per header (-1 marks a variable-length packet). The table is built by
 * the game's own packet.cpp, so the lengths are the sizes its structures
 * have in this build. port/tools/packetdiff.py compares them with rAthena's.
 */
#include <map>
#include <cstdio>

typedef std::map<int, int> PacketLenMap;
extern PacketLenMap g_packetLenMap;
extern void InitPacketLenWithClient();

int main()
{
	InitPacketLenWithClient();
	for (PacketLenMap::const_iterator it = g_packetLenMap.begin(); it != g_packetLenMap.end(); ++it)
		printf("0x%04x %d\n", it->first, it->second);
	return 0;
}
