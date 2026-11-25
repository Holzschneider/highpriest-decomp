/*
 * What the probes need from the rest of the game, in the smallest form that
 * lets device/Connection.cpp link on its own. The clock is bf_clock_ticks
 * (port/native/sys), as in the client.
 */
#include "common.h"

void ErrorMsg(const char *msg)
{
	BF_TRACE(BFT_ERR, "ErrorMsg: %s", msg);
}
