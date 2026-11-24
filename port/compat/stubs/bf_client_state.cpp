// The client's state as the control socket names it (PROTOCOL.md section 3,
// part `mode`), read from the mode manager. Compiled with the client's flags
// and headers (BF_CLIENT_PORT_SOURCES in CMakeLists.txt), so that the start-up
// glue in bf_main.cpp needs no client header.
#include "common.h"
#include "Mode.h"
#include "LoginMode.h"

extern "C" const char *bf_port_mode(const char **detail);

namespace {

struct SubMode {
	int id;
	const char *mode;
	const char *detail;
};

const SubMode kLogin[] = {
	{ LSM_WARNING,                        "login",      "LSM_WARNING" },
	{ LSM_LICENCE,                        "login",      "LSM_LICENCE" },
	{ LSM_SELECTACCOUNT,                  "login",      "LSM_SELECTACCOUNT" },
	{ LSM_IDENTRY,                        "login",      "LSM_IDENTRY" },
	{ LSM_WAITRESP_FROM_ACSVR,            "connecting", "LSM_WAITRESP_FROM_ACSVR" },
	{ LSM_WAITRESP_FROM_CHSVR,            "connecting", "LSM_WAITRESP_FROM_CHSVR" },
	{ LSM_SELECTSVR,                      "servers",    "LSM_SELECTSVR" },
	{ LSM_SELECTCHAR,                     "charselect", "LSM_SELECTCHAR" },
	{ LSM_MAKECHAR,                       "makechar",   "LSM_MAKECHAR" },
	{ LSM_WAITRESP_FROM_CHSVR_SELECTCHAR, "charselect", "LSM_WAITRESP_FROM_CHSVR_SELECTCHAR" },
	{ LSM_WAITRESP_FROM_CHSVR_MAKECHAR,   "makechar",   "LSM_WAITRESP_FROM_CHSVR_MAKECHAR" },
	{ LSM_WAITRESP_FROM_CHSVR_DELETECHAR, "charselect", "LSM_WAITRESP_FROM_CHSVR_DELETECHAR" },
	{ LSM_READY_FOR_ZONESVR,              "loading",    "LSM_READY_FOR_ZONESVR" },
	{ LSM_SELECTACCOUNT2,                 "login",      "LSM_SELECTACCOUNT2" },
	{ LSM_REQ_GAME_GUARD_CHECK,           "connecting", "LSM_REQ_GAME_GUARD_CHECK" },
	{ LSM_ACK_GAME_GUARD,                 "connecting", "LSM_ACK_GAME_GUARD" },
	{ LSM_REQ_GAME_GUARD_CHECK2,          "connecting", "LSM_REQ_GAME_GUARD_CHECK2" },
	{ LSM_REQ_PTNUM,                      "connecting", "LSM_REQ_PTNUM" },
	{ LSM_REQ_PTACINFO,                   "connecting", "LSM_REQ_PTACINFO" },
	{ LSM_SELECTCHAR2,                    "charselect", "LSM_SELECTCHAR2" },
};

}  // namespace

// One of start, login, connecting, servers, charselect, makechar, loading,
// game, quitting; *detail the client's own name for it.
extern "C" const char *bf_port_mode(const char **detail)
{
	const char *unused;
	if (!detail)
		detail = &unused;
	if (g_sysQuit || !g_modeMgr.GetLoopCond()) {
		*detail = "g_sysQuit";
		return "quitting";
	}
	if (g_modeMgr.GetGameMode()) {
		*detail = "MT_GAME";
		return "game";
	}
	CLoginMode *login = g_modeMgr.GetLoginMode();
	if (!login || !g_modeMgr.GetCurMode()) {
		*detail = "none";
		return "start";
	}
	int sub = login->GetSubMode();
	for (size_t i = 0; i < sizeof(kLogin) / sizeof(kLogin[0]); ++i) {
		if (kLogin[i].id == sub) {
			*detail = kLogin[i].detail;
			return kLogin[i].mode;
		}
	}
	*detail = "LSM_?";
	return "login";
}
