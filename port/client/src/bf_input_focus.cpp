// What bf_input asks of the client (bf_input.h, bf_input_client_text_focus):
// whether a text field holds the keyboard, for the Option rule. Compiled with
// the client's flags and headers (BF_CLIENT_PORT_SOURCES in CMakeLists.txt).
#include "common.h"
#include "window/UISys.h"

#include "bf_input.h"

extern UIWindowMgr g_windowMgr;

extern "C" int bf_input_client_text_focus(void)
{
	return g_windowMgr.GetFocusEdit() != NULL;
}
