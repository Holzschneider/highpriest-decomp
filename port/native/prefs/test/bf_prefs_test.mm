// bifrost-prefs-test: checks the game-folder chooser and the settings window.
//
//   bifrost-prefs-test                the checks, one line each; non-zero exit on a failure
//   bifrost-prefs-test --interactive  shows the real chooser panel, then the settings
//                                     window, logging every change, until Cmd+Q
//
// Needs no person and no game data: the settings live under the build
// directory (BF_HOME), the folders are made there, and the window's
// controls are set programmatically. The chooser's panel is never shown
// here (BF_NO_ALERTS), except with --interactive. Writes PNGs of the window
// to the build directory (prefs-test/settings-*.png).

#import <AppKit/AppKit.h>

#include "bf_app.h"
#include "bf_prefs.h"
#include "bf_sys.h"
#include "bf_trace.h"

#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <string>
#include <vector>

namespace {

int g_failures = 0;

void Report(const char *status, const char *name, const char *fmt, ...)
{
	char detail[1024];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(detail, sizeof detail, fmt, ap);
	va_end(ap);
	printf("%-4s %s: %s\n", status, name, detail);
	fflush(stdout);
	if (strcmp(status, "FAIL") == 0)
		++g_failures;
}
#define OK(name, ...)   Report("ok", name, __VA_ARGS__)
#define FAIL(name, ...) Report("FAIL", name, __VA_ARGS__)
#define SKIP(name, ...) Report("skip", name, __VA_ARGS__)
#define CHECK(cond, name, ...) do { if (cond) OK(name, __VA_ARGS__); else FAIL(name, __VA_ARGS__); } while (0)

std::string g_dir;   // BF_PREFS_TEST_DIR

struct Change {
	std::string group, name;
};
std::vector<Change> g_changes;

void OnChange(void *context, const char *group, const char *name)
{
	g_changes.push_back({ group, name });
	if (context)
		printf("     changed %s/%s\n", group, name);
}

bool Changed(const char *group, const char *name)
{
	for (const Change &c : g_changes)
		if (c.group == group && c.name == name)
			return true;
	return false;
}

void MakeFolder(const std::string &path)
{
	std::string cmd = "mkdir -p '" + path + "'";
	if (system(cmd.c_str()) != 0)
		fprintf(stderr, "cannot make %s\n", path.c_str());
}

void WriteFile(const std::string &path, const void *data, size_t size)
{
	FILE *f = fopen(path.c_str(), "wb");
	if (!f)
		return;
	fwrite(data, 1, size, f);
	fclose(f);
}

void WriteGrf(const std::string &path)
{
	// A GRF 0x200 header: signature, 14 key bytes, table offset, seeds, count, version.
	unsigned char head[46] = {};
	memcpy(head, "Master of Magic", 15);
	head[42] = 0x00; head[43] = 0x02;
	WriteFile(path, head, sizeof head);
}

std::string RealHome()
{
	struct passwd *pw = getpwuid(getuid());
	return pw && pw->pw_dir ? pw->pw_dir : "";
}

std::string Location()
{
	const char *p = bf_file_location(BF_LOCATION_GAME);
	return p ? p : "";
}

NSDictionary *Plist()
{
	return [NSDictionary dictionaryWithContentsOfFile:@(bf_settings_path(bf_settings_shared()))];
}

id PlistValue(NSString *keyPath)
{
	return [Plist() valueForKeyPath:keyPath];
}

NSView *Find(NSView *root, NSString *ident)
{
	if ([root.identifier isEqualToString:ident])
		return root;
	for (NSView *sub in root.subviews)
		if (NSView *v = Find(sub, ident))
			return v;
	return nil;
}

void Pump(double ms)
{
	double end = [NSProcessInfo processInfo].systemUptime + ms / 1000.0;
	bf_app_event e;
	while ([NSProcessInfo processInfo].systemUptime < end) {
		bf_app_pump(10);
		while (bf_app_poll_event(&e)) {
		}
	}
}

// Renders the whole window (title bar included) into a PNG; returns its pixel size.
NSSize Render(NSWindow *w, NSString *appearanceName, const std::string &path)
{
	w.appearance = [NSAppearance appearanceNamed:appearanceName];
	NSView *frame = w.contentView.superview ?: w.contentView;
	[frame layoutSubtreeIfNeeded];
	[frame displayIfNeeded];
	NSBitmapImageRep *rep = [frame bitmapImageRepForCachingDisplayInRect:frame.bounds];
	[frame cacheDisplayInRect:frame.bounds toBitmapImageRep:rep];
	NSData *png = [rep representationUsingType:NSBitmapImageFileTypePNG properties:@{}];
	[png writeToFile:@(path.c_str()) atomically:YES];
	w.appearance = nil;
	return png.length ? NSMakeSize(rep.pixelsWide, rep.pixelsHigh) : NSZeroSize;
}

// ---- The checks ----

void CheckValidation()
{
	char why[1024];
	std::string base = g_dir + "/folders";
	system(("rm -rf '" + base + "'").c_str());

	int r = bf_prefs_game_folder_valid((base + "/missing").c_str(), why, sizeof why);
	CHECK(r == 0 && strstr(why, "does not exist"), "valid-missing", "%d, \"%s\"", r, why);

	MakeFolder(base);
	WriteFile(base + "/plain.txt", "x", 1);
	r = bf_prefs_game_folder_valid((base + "/plain.txt").c_str(), why, sizeof why);
	CHECK(r == 0 && strstr(why, "not a folder"), "valid-file", "%d, \"%s\"", r, why);

	std::string empty = base + "/empty";
	MakeFolder(empty);
	WriteFile(empty + "/readme.txt", "x", 1);
	r = bf_prefs_game_folder_valid(empty.c_str(), why, sizeof why);
	CHECK(r == 0 && strstr(why, "holds no .grf archive"), "valid-empty", "%d, \"%s\"", r, why);

	std::string bad = base + "/bad";
	MakeFolder(bad);
	char zeros[64] = {};
	WriteFile(bad + "/data.grf", zeros, sizeof zeros);
	r = bf_prefs_game_folder_valid(bad.c_str(), why, sizeof why);
	CHECK(r == 0 && strstr(why, "not a GRF archive"), "valid-bad-header", "%d, \"%s\"", r, why);

	std::string good = base + "/good";
	MakeFolder(good);
	WriteGrf(good + "/DATA.GRF");   // other letter case on purpose
	why[0] = 0;
	r = bf_prefs_game_folder_valid(good.c_str(), why, sizeof why);
	CHECK(r == 1, "valid-dummy", "%d (DATA.GRF with a GRF header)", r);

	r = bf_prefs_game_folder_valid(NULL, why, sizeof why);
	CHECK(r == 0, "valid-null", "%d, \"%s\"", r, why);

	const char *rdataOnly[] = { "rdata.grf" };
	bf_prefs_set_archives(rdataOnly, 1);
	int withoutListed = bf_prefs_game_folder_valid(good.c_str(), why, sizeof why);
	std::string whyList = why;
	const char *both[] = { "rdata.grf", "data.grf" };
	bf_prefs_set_archives(both, 2);
	int withListed = bf_prefs_game_folder_valid(good.c_str(), why, sizeof why);
	bf_prefs_set_archives(NULL, 0);
	CHECK(withoutListed == 0 && strstr(whyList.c_str(), "rdata.grf") && withListed == 1, "valid-archive-list",
	      "list {rdata.grf}: %d \"%s\"; list {rdata.grf, data.grf}: %d", withoutListed, whyList.c_str(), withListed);

	std::string real = RealHome() + "/Games/" BF_APP_NAME;
	struct stat st;
	if (stat((real + "/data.grf").c_str(), &st) == 0) {
		r = bf_prefs_game_folder_valid(real.c_str(), why, sizeof why);
		CHECK(r == 1, "valid-real-game-folder", "%d for %s%s%s", r, real.c_str(), r ? "" : ": ", r ? "" : why);
	} else {
		SKIP("valid-real-game-folder", "%s/data.grf absent", real.c_str());
	}
}

void CheckOrder()
{
	bf_settings *s = bf_settings_shared();
	std::string base = g_dir + "/folders";
	std::string good = base + "/good";
	std::string other = base + "/other";
	MakeFolder(other);
	WriteGrf(other + "/rdata.grf");

	unsetenv("BF_CLIENT_DIR");
	bf_settings_remove(s, BF_SETTINGS_PATHS, BF_SETTING_GAME_FOLDER);
	bf_file_set_game_folder(NULL);
	std::string def = RealHome() + "/Games/" BF_APP_NAME;
	CHECK(Location() == def, "order-default", "%s", Location().c_str());

	bf_settings_set_string(s, BF_SETTINGS_PATHS, BF_SETTING_GAME_FOLDER, good.c_str());
	bf_file_set_game_folder(NULL);
	CHECK(Location() == good, "order-setting", "Paths/GameFolder: %s", Location().c_str());

	setenv("BF_CLIENT_DIR", other.c_str(), 1);
	bf_file_set_game_folder(NULL);
	CHECK(Location() == other, "order-env", "BF_CLIENT_DIR over the setting: %s", Location().c_str());

	bf_file_set_game_folder(good.c_str());
	CHECK(Location() == good, "order-setter", "bf_file_set_game_folder over both: %s", Location().c_str());

	unsetenv("BF_CLIENT_DIR");
	bf_file_set_game_folder(NULL);
}

void CheckChooser()
{
	bf_settings *s = bf_settings_shared();
	std::string good = g_dir + "/folders/good";
	std::string bad = g_dir + "/folders/bad";
	char out[1024];

	// The fast path: configured and valid. BF_NO_ALERTS makes a wrongly shown panel a failure (-1).
	setenv("BF_NO_ALERTS", "1", 1);
	bf_settings_set_string(s, BF_SETTINGS_PATHS, BF_SETTING_GAME_FOLDER, good.c_str());
	bf_file_set_game_folder(NULL);
	out[0] = 0;
	double t0 = [NSProcessInfo processInfo].systemUptime;
	int r = bf_prefs_choose_game_folder(out, sizeof out);
	double ms = ([NSProcessInfo processInfo].systemUptime - t0) * 1000;
	CHECK(r == 1 && good == out, "chooser-fast-path", "%d \"%s\" in %.1f ms, no panel", r, out, ms);

	char tiny[4];
	r = bf_prefs_choose_game_folder(tiny, sizeof tiny);
	CHECK(r == -1, "chooser-small-buffer", "%d", r);

	// Configured but invalid, nobody to ask: failure, nothing stored.
	bf_settings_set_string(s, BF_SETTINGS_PATHS, BF_SETTING_GAME_FOLDER, bad.c_str());
	bf_settings_save(s);
	bf_file_set_game_folder(NULL);
	r = bf_prefs_choose_game_folder(out, sizeof out);
	char stored[1024] = "";
	bf_settings_get_string(s, BF_SETTINGS_PATHS, BF_SETTING_GAME_FOLDER, stored, sizeof stored, NULL);
	CHECK(r == -1 && bad == stored, "chooser-invalid-no-ui", "%d, setting unchanged: %s", r, stored);

	// From the environment, invalid: same.
	setenv("BF_CLIENT_DIR", (g_dir + "/folders/empty").c_str(), 1);
	bf_file_set_game_folder(NULL);
	r = bf_prefs_choose_game_folder(out, sizeof out);
	CHECK(r == -1, "chooser-invalid-env", "%d", r);
	unsetenv("BF_CLIENT_DIR");
	bf_file_set_game_folder(NULL);
	unsetenv("BF_NO_ALERTS");

	// Storing a folder.
	g_changes.clear();
	char why[1024] = "";
	r = bf_prefs_set_game_folder(bad.c_str(), why, sizeof why);
	CHECK(r == 0 && [PlistValue(@"Paths.GameFolder") isEqual:@(bad.c_str())] && !Changed(BF_SETTINGS_PATHS, BF_SETTING_GAME_FOLDER),
	      "set-folder-refused", "%d \"%s\"", r, why);
	r = bf_prefs_set_game_folder(good.c_str(), why, sizeof why);
	CHECK(r == 1 && [PlistValue(@"Paths.GameFolder") isEqual:@(good.c_str())] && Changed(BF_SETTINGS_PATHS, BF_SETTING_GAME_FOLDER),
	      "set-folder-stored", "%d, plist Paths/GameFolder = %s, callback fired", r,
	      [[PlistValue(@"Paths.GameFolder") description] UTF8String]);
}

void CheckMenu()
{
	int r1 = bf_prefs_install_menu_item();
	int r2 = bf_prefs_install_menu_item();
	NSMenu *appMenu = NSApp.mainMenu.itemArray[0].submenu;
	int count = 0;
	NSMenuItem *found = nil;
	for (NSMenuItem *item in appMenu.itemArray)
		if ([item.title isEqualToString:@"Settings…"]) {
			++count;
			found = item;
		}
	CHECK(r1 == 1 && r2 == 1 && count == 1 && [found.keyEquivalent isEqualToString:@","] &&
	      found.keyEquivalentModifierMask == NSEventModifierFlagCommand && [appMenu indexOfItem:found] == 0,
	      "menu-item", "\"%s\" Cmd+%s at index %ld of the %s menu, %d time(s)", found.title.UTF8String,
	      found.keyEquivalent.UTF8String, (long)[appMenu indexOfItem:found], appMenu.title.UTF8String, count);

	// Cmd+, through the menu's key equivalent opens the window.
	NSEvent *key = [NSEvent keyEventWithType:NSEventTypeKeyDown location:NSZeroPoint
	                           modifierFlags:NSEventModifierFlagCommand timestamp:0 windowNumber:0
	                                 context:nil characters:@"," charactersIgnoringModifiers:@","
	                               isARepeat:NO keyCode:43];
	BOOL handled = [NSApp.mainMenu performKeyEquivalent:key];
	Pump(200);
	CHECK(handled && bf_prefs_settings_visible(), "menu-opens-window", "Cmd+, handled %d, window visible %d",
	      handled, bf_prefs_settings_visible());
	bf_prefs_close_settings();
	Pump(50);
	CHECK(!bf_prefs_settings_visible(), "close-window", "visible %d after bf_prefs_close_settings", bf_prefs_settings_visible());
}

void CheckWindow()
{
	bf_settings *s = bf_settings_shared();
	// Known start: defaults everywhere except the folder.
	for (const char *opt : { BF_OPT_SKIP_INTRO, BF_OPT_FREE_CAMERA, BF_OPT_LAUNCH_ARGUMENTS, BF_OPT_ATTACK_WITHOUT_CTRL })
		bf_settings_remove(s, BF_SETTINGS_OPTIONS, opt);
	bf_settings_remove(s, BF_PREFS_DISPLAY, NULL);
	bf_settings_set_int(s, BF_PREFS_CLIENT_OPTION, BF_PREFS_MUSIC_VOLUME, 40);   // as the client's /bv would
	bf_settings_set_account_name(s, "alice");
	bf_settings_save(s);

	int expectScale = (int)(NSScreen.mainScreen.backingScaleFactor + 0.5);
	expectScale = expectScale < 1 ? 1 : expectScale > 3 ? 3 : expectScale;
	CHECK(bf_prefs_ui_scale() == expectScale, "ui-scale-default", "%d with no setting (main screen backing scale %.1f)",
	      bf_prefs_ui_scale(), NSScreen.mainScreen.backingScaleFactor);

	NSWindow *w = (__bridge NSWindow *)bf_prefs_settings_window();
	NSView *v = w.contentView;
	NSSegmentedControl *scale = (NSSegmentedControl *)Find(v, @"uiScale");
	NSSlider *music = (NSSlider *)Find(v, @"musicVolume");
	NSSlider *effect = (NSSlider *)Find(v, @"effectVolume");
	NSTextField *musicValue = (NSTextField *)Find(v, @"musicVolumeValue");
	NSTextField *account = (NSTextField *)Find(v, @"accountName");
	NSTextField *folder = (NSTextField *)Find(v, @"gameFolder");
	NSButton *forget = (NSButton *)Find(v, @"forgetPassword");
	NSTextField *status = (NSTextField *)Find(v, @"status");
	NSButton *boxes[4];
	const char *opts[4] = { BF_OPT_SKIP_INTRO, BF_OPT_FREE_CAMERA, BF_OPT_LAUNCH_ARGUMENTS, BF_OPT_ATTACK_WITHOUT_CTRL };
	bool all = scale && music && effect && musicValue && account && folder && forget && status && Find(v, @"chooseGameFolder");
	for (int i = 0; i < 4; ++i)
		all = (boxes[i] = (NSButton *)Find(v, @(opts[i]))) && all;
	CHECK(all, "window-controls", "every identified control present in a %.0fx%.0f pt window",
	      w.frame.size.width, w.frame.size.height);
	if (!all)
		return;

	// The window shows what the settings hold.
	std::string good = g_dir + "/folders/good";
	CHECK(scale.selectedSegment == expectScale - 1 && boxes[0].state == NSControlStateValueOn &&
	      boxes[1].state == NSControlStateValueOn && boxes[2].state == NSControlStateValueOn &&
	      boxes[3].state == NSControlStateValueOff && music.integerValue == 40 && effect.integerValue == 100 &&
	      [musicValue.stringValue isEqualToString:@"40"] && [account.stringValue isEqualToString:@"alice"] &&
	      [folder.toolTip isEqualToString:@(good.c_str())] &&
	      [folder.stringValue isEqualToString:@(good.c_str()).stringByAbbreviatingWithTildeInPath],
	      "window-reads-settings", "scale %ld, options %ld%ld%ld%ld, music %ld, effects %ld, account \"%s\", folder %s",
	      (long)scale.selectedSegment + 1, (long)boxes[0].state, (long)boxes[1].state, (long)boxes[2].state,
	      (long)boxes[3].state, (long)music.integerValue, (long)effect.integerValue, account.stringValue.UTF8String,
	      folder.toolTip.UTF8String);

	// Each control, set as a click would, lands in the plist and in the callback.
	for (int value : { 3, 1 }) {
		g_changes.clear();
		[scale setSelectedSegment:value - 1];
		[scale sendAction:scale.action to:scale.target];
		id stored = PlistValue(@"Display.UIScale");
		CHECK([stored isEqual:@(value)] && bf_prefs_ui_scale() == value && Changed(BF_PREFS_DISPLAY, BF_PREFS_UI_SCALE),
		      value == 3 ? "set-ui-scale-3" : "set-ui-scale-1", "plist Display/UIScale = %s, callback %d",
		      [[stored description] UTF8String], Changed(BF_PREFS_DISPLAY, BF_PREFS_UI_SCALE));
	}

	for (int i = 0; i < 4; ++i) {
		g_changes.clear();
		int before = bf_settings_option(s, opts[i]);
		[boxes[i] performClick:nil];
		id stored = PlistValue([NSString stringWithFormat:@"Options.%s", opts[i]]);
		char name[64];
		snprintf(name, sizeof name, "set-option-%s", opts[i]);
		CHECK([stored isEqual:@(!before)] && bf_settings_option(s, opts[i]) == !before && Changed(BF_SETTINGS_OPTIONS, opts[i]),
		      name, "%d -> plist %s, callback %d", before, [[stored description] UTF8String], Changed(BF_SETTINGS_OPTIONS, opts[i]));
	}

	struct { NSSlider *slider; const char *key; int value; } vols[] = {
		{ music, BF_PREFS_MUSIC_VOLUME, 77 }, { effect, BF_PREFS_EFFECT_VOLUME, 0 }, { effect, BF_PREFS_EFFECT_VOLUME, 127 },
	};
	for (auto &t : vols) {
		g_changes.clear();
		t.slider.integerValue = t.value;
		[t.slider sendAction:t.slider.action to:t.slider.target];
		id stored = PlistValue([NSString stringWithFormat:@"Client.Option.%s", t.key]);
		char name[64];
		snprintf(name, sizeof name, "set-volume-%s-%d", t.key, t.value);
		CHECK([stored isEqual:@(t.value)] && Changed(BF_PREFS_CLIENT_OPTION, t.key), name,
		      "plist Client/Option/%s = %s, callback %d", t.key, [[stored description] UTF8String],
		      Changed(BF_PREFS_CLIENT_OPTION, t.key));
	}
	CHECK([musicValue.stringValue isEqualToString:@"77"], "volume-label", "music value label \"%s\"", musicValue.stringValue.UTF8String);

	g_changes.clear();
	account.stringValue = @"  bob ";
	[account sendAction:account.action to:account.target];
	id storedName = PlistValue(@"Account.Name");
	CHECK([storedName isEqual:@"bob"] && [account.stringValue isEqualToString:@"bob"] &&
	      Changed(BF_SETTINGS_ACCOUNT, BF_SETTING_ACCOUNT_NAME) && forget.enabled,
	      "set-account-name", "plist Account/Name = \"%s\", field \"%s\" (trimmed), callback %d, Forget enabled %d",
	      [[storedName description] UTF8String], account.stringValue.UTF8String,
	      Changed(BF_SETTINGS_ACCOUNT, BF_SETTING_ACCOUNT_NAME), forget.enabled);
	g_changes.clear();
	[account sendAction:account.action to:account.target];
	CHECK(g_changes.empty(), "account-unchanged-no-callback", "%zu callbacks for the same name", g_changes.size());

	// Forget Saved Password: a real Keychain item under a test account, when the Keychain is usable.
	if (bf_sys_keychain_available()) {
		char acct[64];
		snprintf(acct, sizeof acct, "bifrost-prefs-test-%d", (int)getpid());
		int stored = bf_sys_keychain_store(acct, "secret");
		account.stringValue = @(acct);
		[account sendAction:account.action to:account.target];
		g_changes.clear();
		[forget performClick:nil];
		char back[64];
		int after = bf_sys_keychain_read(acct, back, sizeof back);
		CHECK(stored == 1 && after == 0 && Changed(BF_SETTINGS_ACCOUNT, BF_PREFS_SAVED_PASSWORD), "forget-password",
		      "stored %d, read after Forget %d, status \"%s\"", stored, after, status.stringValue.UTF8String);
		bf_sys_keychain_delete(acct);
	} else {
		g_changes.clear();
		[forget performClick:nil];
		SKIP("forget-password", "Keychain not usable from this session; the button reports \"%s\"",
		     status.stringValue.UTF8String);
	}
	account.stringValue = @"bob";
	[account sendAction:account.action to:account.target];

	// Values changed behind the window's back are shown when it is opened.
	bf_settings_set_int(s, BF_PREFS_CLIENT_OPTION, BF_PREFS_MUSIC_VOLUME, 12);
	bf_prefs_settings_window();
	CHECK(music.integerValue == 12 && [musicValue.stringValue isEqualToString:@"12"], "window-refresh",
	      "music slider %ld after an outside change to 12", (long)music.integerValue);

	// A folder stored from outside appears in the window.
	std::string other = g_dir + "/folders/other";
	bf_prefs_set_game_folder(other.c_str(), NULL, 0);
	CHECK([folder.toolTip isEqualToString:@(other.c_str())], "window-folder-follows", "%s", folder.toolTip.UTF8String);
	bf_prefs_set_game_folder(good.c_str(), NULL, 0);

	// Writes through the settings file, read back by a separate object.
	bf_settings *fresh = bf_settings_open_file(bf_settings_path(s));
	int32_t sv = -1;
	bf_settings_get_int(fresh, BF_PREFS_CLIENT_OPTION, BF_PREFS_EFFECT_VOLUME, &sv);
	CHECK(sv == 127 && bf_settings_option(fresh, BF_OPT_ATTACK_WITHOUT_CTRL) == 1, "file-round-trip",
	      "a second object on %s reads soundVolume %d, AttackWithoutCtrl %d", bf_settings_path(s), sv,
	      bf_settings_option(fresh, BF_OPT_ATTACK_WITHOUT_CTRL));
	bf_settings_close(fresh);

	// Layout: every control inside the content area, none overlapping another.
	[v layoutSubtreeIfNeeded];
	NSArray<NSString *> *ids = @[ @"gameFolder", @"chooseGameFolder", @"uiScale", @(opts[0]), @(opts[1]), @(opts[2]),
	                              @(opts[3]), @"musicVolume", @"musicVolumeValue", @"effectVolume", @"effectVolumeValue",
	                              @"accountName", @"forgetPassword", @"status" ];
	std::vector<NSRect> rects;
	bool inside = true, overlap = false;
	NSString *bad = @"";
	for (NSString *ident in ids) {
		NSView *c = Find(v, ident);
		NSRect r = [c convertRect:c.bounds toView:v];
		if (!NSContainsRect(v.bounds, r) || r.size.width < 1 || r.size.height < 1) {
			inside = false;
			bad = ident;
		}
		for (const NSRect &o : rects)
			if (NSIntersectsRect(NSInsetRect(o, 1, 1), NSInsetRect(r, 1, 1))) {
				overlap = true;
				bad = ident;
			}
		rects.push_back(r);
	}
	CHECK(inside && !overlap && w.frame.size.width < 700 && w.frame.size.height < 700, "window-layout",
	      "%lu controls inside a %.0fx%.0f pt content area, no overlap%s%s", (unsigned long)ids.count,
	      v.bounds.size.width, v.bounds.size.height, bad.length ? "; offender " : "", bad.UTF8String);

	// Pictures for a person to judge, in the state a new user sees: defaults,
	// the default game folder (shown with ~), and a neutral account name.
	for (const char *opt : { BF_OPT_SKIP_INTRO, BF_OPT_FREE_CAMERA, BF_OPT_LAUNCH_ARGUMENTS, BF_OPT_ATTACK_WITHOUT_CTRL })
		bf_settings_remove(s, BF_SETTINGS_OPTIONS, opt);
	bf_settings_remove(s, BF_PREFS_DISPLAY, NULL);
	bf_settings_remove(s, BF_PREFS_CLIENT_OPTION, BF_PREFS_MUSIC_VOLUME);
	bf_settings_remove(s, BF_PREFS_CLIENT_OPTION, BF_PREFS_EFFECT_VOLUME);
	bf_settings_remove(s, BF_SETTINGS_PATHS, BF_SETTING_GAME_FOLDER);
	bf_settings_set_account_name(s, "player");
	bf_file_set_game_folder(NULL);
	bf_prefs_settings_window();
	status.stringValue = @"";
	for (auto [appearance, file] : { std::pair{ NSAppearanceNameAqua, "settings-light.png" },
	                                 std::pair{ NSAppearanceNameDarkAqua, "settings-dark.png" } }) {
		std::string path = g_dir + "/" + file;
		NSSize px = Render(w, appearance, path);
		CHECK(px.width > 0, file, "%.0fx%.0f px written to %s", px.width, px.height, path.c_str());
	}

	// Shown, then driven by the client's pump.
	int shown = bf_prefs_show_settings();
	Pump(300);
	CHECK(shown == 1 && bf_prefs_settings_visible() && w.isVisible, "show-and-pump", "shown %d, visible %d after 300 ms of bf_app_pump",
	      shown, bf_prefs_settings_visible());
	bf_prefs_close_settings();
	Pump(50);
}

int Interactive()
{
	bf_prefs_set_change_callback(OnChange, (void *)1);
	char out[1024];
	int r = bf_prefs_ask_game_folder("This is the chooser the application shows when no usable game folder is set.", out, sizeof out);
	printf("chooser: %d%s%s\n", r, r == 1 ? " " : "", r == 1 ? out : "");
	if (r == 1)
		printf("settings file: %s\n", bf_settings_path(bf_settings_shared()));
	bf_prefs_install_menu_item();
	bf_prefs_show_settings();
	printf("settings window open; Cmd+, reopens it, Cmd+Q ends\n");
	for (;;) {
		bf_app_pump(50);
		bf_app_event e;
		while (bf_app_poll_event(&e))
			if (e.type == BF_APP_EVENT_QUIT)
				return 0;
	}
}

}  // namespace

int main(int argc, char **argv)
{
	@autoreleasepool {
		bool interactive = argc > 1 && strcmp(argv[1], "--interactive") == 0;
		g_dir = BF_PREFS_TEST_DIR;
		MakeFolder(g_dir + "/home");
		// Nothing touches the real Application Support folder.
		setenv("BF_HOME", (g_dir + "/home").c_str(), 1);
		if (!interactive) {
			unsetenv("BF_CLIENT_DIR");
			std::string plist = g_dir + "/home/Library/Application Support/" BF_APP_NAME "/Settings.plist";
			unlink(plist.c_str());
		}

		char id[64];
		snprintf(id, sizeof id, "prefs-test-%d", (int)getpid());
		bf_app_config config = { id, 1 };
		int r = bf_app_init(&config);
		if (r != BF_APP_OK) {
			FAIL("app-init", "bf_app_init returned %d", r);
			return 1;
		}
		if (interactive)
			return Interactive();

		printf("info settings: %s\n", bf_settings_path(bf_settings_shared()));
		bf_prefs_set_change_callback(OnChange, nullptr);
		CheckValidation();
		CheckOrder();
		CheckChooser();
		CheckMenu();
		CheckWindow();
		bf_app_shutdown();
		printf("%s: %d failure(s)\n", g_failures ? "FAILED" : "passed", g_failures);
	}
	return g_failures ? 1 : 0;
}
