// The client's message boxes and "open" (bf_port.h: bf_port_alert,
// bf_port_open), as user32's MessageBox and shell32's ShellExecute adapters
// of stage A did them (port/compat/win32/src/bf_win32_shell.cpp, deleted).
// The message box is bf_sys_alert_codepage, the one helper for the client's
// text in alerts (Mode.cpp's ErrorMsg calls it directly), with the trace
// line and the default title the client's MessageBox calls had.
#include "bf_port.h"

#include <string.h>
#include <strings.h>
#include <string>

#include "bf_text.h"
#include "bf_trace.h"

namespace {

// The client's code page to UTF-8; bytes that do not convert are kept.
std::string ToUtf8(const char *text)
{
	if (!text)
		return std::string();
	int len = (int)strlen(text);
	int cp = (int)bf_file_user_codepage();
	int n = bf_text_to_utf8(cp, text, len, NULL, 0);
	if (n <= 0)
		return n == 0 ? std::string() : std::string(text, (size_t)len);
	std::string out((size_t)n, '\0');
	bf_text_to_utf8(cp, text, len, &out[0], n);
	return out;
}

}  // namespace

extern "C" bf_sys_alert_answer bf_port_alert(const char *title, const char *message, bf_sys_alert_style style,
                                             bf_sys_alert_buttons buttons)
{
	if (!title)
		title = "Error";
	BF_TRACE(BFT_UI, "MessageBox \"%s\": %s", ToUtf8(title).c_str(), ToUtf8(message ? message : "").c_str());
	return bf_sys_alert_codepage(0, title, message, style, buttons);
}

extern "C" int bf_port_open(const char *target)
{
	if (!target || !*target)
		return 0;
	std::string utf8 = ToUtf8(target);
	if (utf8.find("://") != std::string::npos || strncasecmp(utf8.c_str(), "mailto:", 7) == 0)
		return bf_sys_open_url(utf8.c_str()) ? 1 : 0;
	char resolved[2048];
	if (bf_file_resolve(target, BF_RESOLVE_READ, resolved, sizeof(resolved)) < 0)
		return 0;
	return bf_sys_open_path(resolved) ? 1 : 0;
}
