// Alerts and opening URLs and files: NSAlert and NSWorkspace.

#include "bf_sys.h"
#include "bf_trace.h"

#import <AppKit/AppKit.h>
#include <CoreGraphics/CoreGraphics.h>

#include <stdlib.h>

namespace {

bool HasWindowServer()
{
	CFDictionaryRef session = CGSessionCopyCurrentDictionary();
	if (!session)
		return false;
	CFRelease(session);
	return true;
}

bool Disabled(const char *variable)
{
	const char *value = getenv(variable);
	return value && *value && *value != '0';
}

bf_sys_alert_answer FirstButton(bf_sys_alert_buttons buttons)
{
	return buttons == BF_ALERT_BUTTONS_YES_NO ? BF_ALERT_YES : BF_ALERT_OK;
}

} // namespace

extern "C" int bf_sys_alerts_enabled(void)
{
	return !Disabled("BF_NO_ALERTS") && HasWindowServer();
}

extern "C" bf_sys_alert_answer bf_sys_alert(const char *title, const char *message,
                                            bf_sys_alert_style style, bf_sys_alert_buttons buttons)
{
	const char *t = title ? title : "";
	const char *m = message ? message : "";
	if (!bf_sys_alerts_enabled() || !bf_sys_is_main_thread()) {
		bf_tracef(BFT_ERR, "alert%s: %s: %s", bf_sys_is_main_thread() ? "" : " (not on the main thread)", t, m);
		return FirstButton(buttons);
	}
	@autoreleasepool {
		NSApplication *app = [NSApplication sharedApplication];
		if (app.activationPolicy == NSApplicationActivationPolicyProhibited)
			[app setActivationPolicy:NSApplicationActivationPolicyAccessory];
		NSAlert *alert = [[NSAlert alloc] init];
		alert.messageText = [NSString stringWithUTF8String:t] ?: @"";
		alert.informativeText = [NSString stringWithUTF8String:m] ?: @"";
		alert.alertStyle = style == BF_ALERT_CRITICAL ? NSAlertStyleCritical
		                 : style == BF_ALERT_WARNING  ? NSAlertStyleWarning
		                                              : NSAlertStyleInformational;
		switch (buttons) {
		case BF_ALERT_BUTTONS_OK_CANCEL:
			[alert addButtonWithTitle:@"OK"];
			[alert addButtonWithTitle:@"Cancel"];
			break;
		case BF_ALERT_BUTTONS_YES_NO:
			[alert addButtonWithTitle:@"Yes"];
			[alert addButtonWithTitle:@"No"];
			break;
		default:
			[alert addButtonWithTitle:@"OK"];
			break;
		}
		[app activateIgnoringOtherApps:YES];
		NSModalResponse response = [alert runModal];
		bool first = response == NSAlertFirstButtonReturn;
		switch (buttons) {
		case BF_ALERT_BUTTONS_OK_CANCEL: return first ? BF_ALERT_OK : BF_ALERT_CANCEL;
		case BF_ALERT_BUTTONS_YES_NO:    return first ? BF_ALERT_YES : BF_ALERT_NO;
		default:                         return BF_ALERT_OK;
		}
	}
}

extern "C" int bf_sys_open_url(const char *url)
{
	@autoreleasepool {
		NSString *text = url ? [NSString stringWithUTF8String:url] : nil;
		NSURL *target = text ? [NSURL URLWithString:text] : nil;
		if (!target || !target.scheme.length) {
			bf_tracef(BFT_ERR, "sys: not a URL: %s", url ? url : "(null)");
			return 0;
		}
		if (Disabled("BF_NO_OPEN")) {
			bf_tracef(BFT_ERR, "sys: would open %s", url);
			return 1;
		}
		return [[NSWorkspace sharedWorkspace] openURL:target] ? 1 : 0;
	}
}

extern "C" int bf_sys_open_path(const char *path)
{
	@autoreleasepool {
		NSString *text = path ? [NSString stringWithUTF8String:path] : nil;
		if (!text || ![[NSFileManager defaultManager] fileExistsAtPath:text]) {
			bf_tracef(BFT_ERR, "sys: cannot open %s: no such file", path ? path : "(null)");
			return 0;
		}
		if (Disabled("BF_NO_OPEN")) {
			bf_tracef(BFT_ERR, "sys: would open %s", path);
			return 1;
		}
		return [[NSWorkspace sharedWorkspace] openURL:[NSURL fileURLWithPath:text]] ? 1 : 0;
	}
}
