// user32's MessageBox and shell32's ShellExecute on bf_sys's alerts and
// NSWorkspace opening.
#include "bf_win32_internal.h"

#include <shellapi.h>

#include <string.h>
#include <string>

using namespace bf_win32;

// Every message box is traced; it is shown with bf_sys_alert unless
// BF_NO_ALERTS is set, there is no window server, or the call is off the
// main thread, in which case it is answered with its first button (IDOK or
// IDYES), which is what the client's callers continue with.
INT WINAPI MessageBoxA(HWND window, LPCSTR text, LPCSTR caption, UINT type)
{
	(void)window;
	std::string message = ToUtf8(text ? text : "");
	std::string title = ToUtf8(caption ? caption : "Error");
	bf_tracef(BFT_UI, "MessageBox \"%s\": %s", title.c_str(), message.c_str());

	bf_sys_alert_buttons buttons;
	switch (type & MB_TYPEMASK) {
	case MB_OKCANCEL:
	case MB_RETRYCANCEL:      buttons = BF_ALERT_BUTTONS_OK_CANCEL; break;
	case MB_YESNO:
	case MB_YESNOCANCEL:      buttons = BF_ALERT_BUTTONS_YES_NO; break;
	default:                  buttons = BF_ALERT_BUTTONS_OK; break;
	}
	bf_sys_alert_style style;
	switch (type & MB_ICONMASK) {
	case MB_ICONHAND:         style = BF_ALERT_CRITICAL; break;
	case MB_ICONEXCLAMATION:  style = BF_ALERT_WARNING; break;
	default:                  style = BF_ALERT_INFO; break;
	}
	bf_sys_alert_answer answer = bf_sys_alert(title.c_str(), message.c_str(), style, buttons);
	if ((type & MB_TYPEMASK) == MB_RETRYCANCEL && answer == BF_ALERT_OK)
		return IDRETRY;
	return (INT)answer;   // IDOK, IDCANCEL, IDYES, IDNO have the same values
}

// "open" of a URL opens it in the browser, of a file or folder in its
// application. Returns a value above 32 on success, as ShellExecute does.
HINSTANCE WINAPI ShellExecuteA(HWND window, LPCSTR operation, LPCSTR file, LPCSTR parameters, LPCSTR directory,
                               INT show)
{
	(void)window;
	(void)directory;
	(void)show;
	if (!file || !*file)
		return (HINSTANCE)(INT_PTR)SE_ERR_FNF;
	if (operation && *operation && strcasecmp(operation, "open") != 0 && strcasecmp(operation, "explore") != 0) {
		BF_TRACE(BFT_W32, "ShellExecute(%s, %s): not supported", operation, file);
		return (HINSTANCE)(INT_PTR)SE_ERR_NOASSOC;
	}
	if (parameters && *parameters)
		BF_TRACE(BFT_W32, "ShellExecute(%s): parameters \"%s\" ignored", file, parameters);
	std::string target = ToUtf8(file);
	bool url = target.find("://") != std::string::npos || strncasecmp(target.c_str(), "mailto:", 7) == 0;
	if (url)
		return (HINSTANCE)(INT_PTR)(bf_sys_open_url(target.c_str()) ? 33 : SE_ERR_NOASSOC);
	char resolved[2048];
	if (bf_file_resolve(file, BF_RESOLVE_READ, resolved, sizeof(resolved)) < 0)
		return (HINSTANCE)(INT_PTR)SE_ERR_FNF;
	return (HINSTANCE)(INT_PTR)(bf_sys_open_path(resolved) ? 33 : SE_ERR_NOASSOC);
}
