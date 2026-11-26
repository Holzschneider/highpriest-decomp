// bifrost-app-test: checks the application module with a real window.
//
//   bifrost-app-test                  the checks, one line each; non-zero exit on a failure
//   bifrost-app-test --interactive    logs every event until the window is closed or Cmd+Q
//                                     (--hide-cursor hides the pointer over the window)
//   bifrost-app-test --second-instance <id>   used by the single-instance check
//
// No game data. Opens a window, takes focus, goes fullscreen for a moment
// and touches the clipboard (restored afterwards).

#import <AppKit/AppKit.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include "bf_app.h"
#include "bf_trace.h"

#include <crt_externs.h>
#include <fcntl.h>
#include <math.h>
#include <spawn.h>
#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

namespace {

int g_failures = 0;
std::vector<bf_app_event> g_seen;   // every event polled, for later checks

void Report(const char *status, const char *name, const char *fmt, ...)
{
	char detail[512];
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
#define INFO(name, ...) Report("info", name, __VA_ARGS__)
#define CHECK(cond, name, ...) do { if (cond) OK(name, __VA_ARGS__); else FAIL(name, __VA_ARGS__); } while (0)

double NowMs()
{
	return [NSProcessInfo processInfo].systemUptime * 1000.0;
}

// Pumps and polls until an event satisfies pred, or the time runs out.
template <class Pred>
bool WaitFor(double timeout_ms, bf_app_event *out, Pred pred)
{
	double end = NowMs() + timeout_ms;
	while (NowMs() < end) {
		bf_app_pump(10);
		bf_app_event e;
		while (bf_app_poll_event(&e)) {
			g_seen.push_back(e);
			if (pred(e)) {
				if (out)
					*out = e;
				return true;
			}
		}
	}
	return false;
}

void Drain(double ms)
{
	WaitFor(ms, nullptr, [](const bf_app_event &) { return false; });
}

NSWindow *Window()
{
	return (__bridge NSWindow *)bf_app_window_nswindow();
}

NSEvent *KeyEvent(NSEventType type, unsigned short code, NSEventModifierFlags flags, NSString *chars, NSString *plain)
{
	return [NSEvent keyEventWithType:type location:NSZeroPoint modifierFlags:flags
	                       timestamp:[NSProcessInfo processInfo].systemUptime
	                    windowNumber:Window().windowNumber context:nil
	                      characters:chars charactersIgnoringModifiers:plain isARepeat:NO keyCode:code];
}

NSEvent *MouseEvent(NSEventType type, NSPoint topLeftPt)
{
	NSWindow *w = Window();
	NSView *v = w.contentView;
	NSPoint inWindow = [v convertPoint:topLeftPt toView:nil];
	return [NSEvent mouseEventWithType:type location:inWindow modifierFlags:0
	                         timestamp:[NSProcessInfo processInfo].systemUptime
	                      windowNumber:w.windowNumber context:nil eventNumber:0 clickCount:1 pressure:1.0f];
}

// The screen of an unattended Mac is usually locked: no app can then become
// active, windows count as occluded and fullscreen is refused.
bool ScreenLocked()
{
	CFDictionaryRef d = CGSessionCopyCurrentDictionary();
	if (!d)
		return false;
	CFBooleanRef v = (CFBooleanRef)CFDictionaryGetValue(d, CFSTR("CGSSessionScreenIsLocked"));
	bool locked = v && CFBooleanGetValue(v);
	CFRelease(d);
	return locked;
}

// Sends a synthetic event the way AppKit would. Through the application's
// queue when the window is key; otherwise (the app cannot be activated)
// straight to the main menu (for Cmd shortcuts) and the window, which skips
// only NSApplication's routing.
bool g_direct = false;
void Deliver(NSEvent *e)
{
	if (!g_direct) {
		[NSApp postEvent:e atStart:NO];
		return;
	}
	if (e.type == NSEventTypeKeyDown && (e.modifierFlags & NSEventModifierFlagCommand) &&
	    [NSApp.mainMenu performKeyEquivalent:e])
		return;
	[Window() sendEvent:e];
}

const char *TypeName(uint32_t t)
{
	static const char *names[] = { "none", "key_down", "key_up", "text", "pointer_move", "button_down",
	                               "button_up", "wheel", "pointer_enter", "pointer_leave", "resize", "focus",
	                               "visibility", "fullscreen", "quit", "reopen", "user" };
	return t < sizeof names / sizeof names[0] ? names[t] : "?";
}

void PrintEvent(const bf_app_event &e)
{
	printf("%8llu %6llu %-13s %s mods=%02x ", (unsigned long long)e.time_ms, (unsigned long long)e.seq,
	       TypeName(e.type), e.source ? "inj" : "ak ", e.mods);
	switch (e.type) {
	case BF_APP_EVENT_KEY_DOWN:
	case BF_APP_EVENT_KEY_UP:
		printf("mac=0x%02x key=0x%02x (%s)%s", e.key.mac_keycode, e.key.key, bf_app_key_name(e.key.key),
		       e.key.repeat ? " repeat" : "");
		break;
	case BF_APP_EVENT_TEXT:
		printf("\"%s\" key_seq=%llu", e.text.utf8, (unsigned long long)e.text.key_seq);
		break;
	case BF_APP_EVENT_RESIZE:
		printf("%.0fx%.0f pt %dx%d px scale %.1f", e.size.size.width_pt, e.size.size.height_pt,
		       e.size.size.width_px, e.size.size.height_px, e.size.size.scale);
		break;
	case BF_APP_EVENT_USER:
		printf("code=%u a=%lld", e.user.code, (long long)e.user.a);
		break;
	default:
		if (e.type >= BF_APP_EVENT_POINTER_MOVE && e.type <= BF_APP_EVENT_POINTER_LEAVE)
			printf("px=(%.1f,%.1f) pt=(%.1f,%.1f) d=(%.1f,%.1f) wheel=(%.2f,%.2f) btn=%u held=%x clicks=%u%s",
			       e.pointer.x_px, e.pointer.y_px, e.pointer.x_pt, e.pointer.y_pt, e.pointer.dx_px, e.pointer.dy_px,
			       e.pointer.wheel_x, e.pointer.wheel_y, e.pointer.button, e.pointer.buttons, e.pointer.clicks,
			       (e.flags & BF_APP_EVF_PRECISE) ? " precise" : "");
		else
			printf("value=%d", e.value);
	}
	printf("\n");
	fflush(stdout);
}

// ----------------------------------------------------------------- modes

int SecondInstance(const char *id)
{
	bf_app_config cfg = { id, 1 };
	int r = bf_app_init(&cfg);
	if (r == BF_APP_ALREADY_RUNNING)
		return 3;
	if (r == BF_APP_OK) {
		bf_app_shutdown();
		return 0;
	}
	return 1;
}

int Interactive(bool hideCursor)
{
	bf_app_config cfg = { "interactive", 1 };
	int r = bf_app_init(&cfg);
	if (r != BF_APP_OK) {
		fprintf(stderr, "bf_app_init: %d\n", r);
		return 1;
	}
	bf_app_window_create(800, 600, "bifrost-app-test: type, click, scroll; close or Cmd+Q to end");
	bf_app_set_cursor_hidden_in_window(hideCursor);
	printf("    time    seq type          src details\n");
	for (;;) {
		bf_app_pump(16);
		bf_app_event e;
		while (bf_app_poll_event(&e)) {
			PrintEvent(e);
			if (e.type == BF_APP_EVENT_QUIT) {
				bf_app_shutdown();
				return 0;
			}
		}
	}
}

int RunChild(const char *const *args, bool multi)
{
	pid_t pid;
	std::vector<char *> env;
	for (char **p = *_NSGetEnviron(); *p; ++p)
		if (strncmp(*p, "BF_MULTI_INSTANCE=", 18) != 0)
			env.push_back(*p);
	char multiVar[] = "BF_MULTI_INSTANCE=1";
	if (multi)
		env.push_back(multiVar);
	env.push_back(nullptr);
	if (posix_spawn(&pid, args[0], nullptr, nullptr, (char *const *)args, env.data()) != 0)
		return -1;
	// Keep pumping meanwhile: the first instance answers on the main run loop.
	int status = 0;
	double end = NowMs() + 15000;
	while (NowMs() < end) {
		pid_t r = waitpid(pid, &status, WNOHANG);
		if (r == pid)
			return WIFEXITED(status) ? WEXITSTATUS(status) : -2;
		Drain(20);
	}
	kill(pid, SIGKILL);
	waitpid(pid, &status, 0);
	return -3;
}

// Frames per second presented to the layer for a while; nil drawables counted.
void MeasureDrawing(const char *what, double ms, int *frames, int *nils)
{
	CAMetalLayer *layer = (__bridge CAMetalLayer *)bf_app_window_metal_layer();
	id<MTLCommandQueue> queue = [layer.device newCommandQueue];
	*frames = 0;
	*nils = 0;
	double end = NowMs() + ms;
	int i = 0;
	while (NowMs() < end) {
		@autoreleasepool {
			bf_app_pump(0);
			bf_app_event e;
			while (bf_app_poll_event(&e))
				g_seen.push_back(e);
			id<CAMetalDrawable> drawable = [layer nextDrawable];
			if (!drawable) {
				++*nils;
				continue;
			}
			MTLRenderPassDescriptor *pass = [MTLRenderPassDescriptor renderPassDescriptor];
			pass.colorAttachments[0].texture = drawable.texture;
			pass.colorAttachments[0].loadAction = MTLLoadActionClear;
			pass.colorAttachments[0].storeAction = MTLStoreActionStore;
			pass.colorAttachments[0].clearColor = MTLClearColorMake((i++ % 60) / 60.0, 0.2, 0.4, 1);
			id<MTLCommandBuffer> cb = [queue commandBuffer];
			[[cb renderCommandEncoderWithDescriptor:pass] endEncoding];
			[cb presentDrawable:drawable];
			[cb commit];
			++*frames;
		}
	}
	(void)what;
}

// ------------------------------------------------------------------ checks

int RunChecks(const char *exe)
{
	char instanceId[64];
	snprintf(instanceId, sizeof instanceId, "test-%d", (int)getpid());
	const char *multi = getenv("BF_MULTI_INSTANCE");
	if (multi && strcmp(multi, "1") == 0)
		unsetenv("BF_MULTI_INSTANCE");   // this program tests single instance itself

	// --- init
	bf_app_config cfg = { instanceId, 1 };
	int r = bf_app_init(&cfg);
	CHECK(r == BF_APP_OK, "init", "bf_app_init returned %d (instance %s)", r, instanceId);
	if (r != BF_APP_OK)
		return 1;
	CHECK(NSApp.activationPolicy == NSApplicationActivationPolicyRegular, "regular-app",
	      "activation policy %ld (0 = regular: Dock icon and menu bar)", (long)NSApp.activationPolicy);
	NSMenuItem *quit = nil;
	for (NSMenuItem *top in NSApp.mainMenu.itemArray)
		for (NSMenuItem *item in top.submenu.itemArray)
			if ([item.keyEquivalent isEqualToString:@"q"] && item.keyEquivalentModifierMask == NSEventModifierFlagCommand)
				quit = item;
	CHECK(quit != nil, "menu", "Quit item with Cmd+Q: %s", quit ? quit.title.UTF8String : "missing");
	CHECK(bf_app_activity_held(), "app-nap", "activity held (NSActivityUserInitiatedAllowingIdleSystemSleep | LatencyCritical)");

	// --- window and layer
	r = bf_app_window_create(640, 480, "bifrost-app-test");
	CHECK(r == BF_APP_OK, "window", "bf_app_window_create(640, 480) returned %d", r);
	if (r != BF_APP_OK)
		return 1;
	Drain(300);
	id layerObj = (__bridge id)bf_app_window_metal_layer();
	bf_app_size s;
	bf_app_window_get_size(&s);
	bool isMetal = [layerObj isKindOfClass:[CAMetalLayer class]];
	CHECK(isMetal, "metal-layer", "layer class %s, device %s", layerObj ? [NSStringFromClass([layerObj class]) UTF8String] : "none",
	      isMetal && [(CAMetalLayer *)layerObj device] ? [[(CAMetalLayer *)layerObj device].name UTF8String] : "none");
	if (isMetal) {
		CGSize d = [(CAMetalLayer *)layerObj drawableSize];
		bool sizeOk = s.width_pt == 640 && s.height_pt == 480 && s.width_px == (int)lround(640 * s.scale) &&
		              s.height_px == (int)lround(480 * s.scale) && d.width == s.width_px && d.height == s.height_px;
		CHECK(sizeOk, "layer-size", "content %.0fx%.0f pt, %dx%d px, scale %.1f, drawable %.0fx%.0f",
		      s.width_pt, s.height_pt, s.width_px, s.height_px, s.scale, d.width, d.height);
	}

	// --- focus (unattended: the app may or may not get to the front)
	NSWindow *w = Window();
	bool focused = false;
	for (int i = 0; i < 50 && !(focused = bf_app_window_is_focused()); ++i)
		Drain(20);
	INFO("focus", "window %s key, app %s active, window %s visible, screen %s", w.keyWindow ? "is" : "is not",
	     NSApp.active ? "is" : "is not", bf_app_window_is_visible() ? "is" : "is not", ScreenLocked() ? "LOCKED" : "unlocked");

	// --- programmatic resize
	bf_app_window_set_content_size(800, 600);
	bf_app_event e = {};
	bool got = WaitFor(2000, &e, [](const bf_app_event &x) {
		return x.type == BF_APP_EVENT_RESIZE && x.size.size.width_pt == 800 && x.size.size.height_pt == 600;
	});
	bf_app_window_get_size(&s);
	CGSize d = isMetal ? [(CAMetalLayer *)layerObj drawableSize] : CGSizeZero;
	CHECK(got && e.source == BF_APP_SOURCE_APPKIT && e.size.size.width_px == (int)lround(800 * s.scale) &&
	      d.width == e.size.size.width_px && d.height == e.size.size.height_px,
	      "resize-event", "%s: %.0fx%.0f pt, %dx%d px; drawable %.0fx%.0f", got ? "received" : "not received",
	      e.size.size.width_pt, e.size.size.height_pt, e.size.size.width_px, e.size.size.height_px, d.width, d.height);

	// --- minimum content size and the screen
	{
		double mw = 0, mh = 0;
		bf_app_window_get_min_content_size(&mw, &mh);
		bool dflt = mw == 320 && mh == 240;
		bf_app_window_set_min_content_size(500, 400);
		double nw = 0, nh = 0;
		bf_app_window_get_min_content_size(&nw, &nh);
		bf_app_window_set_min_content_size(320, 240);
		CHECK(dflt && nw == 500 && nh == 400 && w.contentMinSize.width == 320, "min-content-size",
		      "default %.0fx%.0f, set 500x400 read back %.0fx%.0f, restored", mw, mh, nw, nh);
		bf_app_screen sc;
		int rc = bf_app_window_get_screen(&sc);
		CHECK(rc == BF_APP_OK && sc.scale == s.scale && sc.visible_width_pt > 0 && sc.visible_width_pt <= sc.width_pt &&
		      sc.visible_height_pt > 0 && sc.visible_height_pt < sc.height_pt, "screen",
		      "%.0fx%.0f pt, visible content %.0fx%.0f pt, scale %.1f", sc.width_pt, sc.height_pt,
		      sc.visible_width_pt, sc.visible_height_pt, sc.scale);
	}

	// --- events posted from another thread, read back in order
	Drain(100);
	const int kCount = 2000;
	std::thread poster([] {
		for (int i = 0; i < kCount; ++i) {
			bf_app_event u = {};
			u.type = BF_APP_EVENT_USER;
			u.user.code = 7;
			u.user.a = i;
			while (bf_app_post_event(&u) != BF_APP_OK)
				usleep(100);
			if (i % 100 == 0)
				usleep(1000);
		}
	});
	int next = 0;
	bool order = true, sourceOk = true;
	uint64_t lastSeq = 0, lastTime = 0;
	double end = NowMs() + 5000;
	while (next < kCount && NowMs() < end) {
		bf_app_pump(5);
		bf_app_event x;
		while (bf_app_poll_event(&x)) {
			if (x.type != BF_APP_EVENT_USER) {
				g_seen.push_back(x);
				continue;
			}
			if (x.user.a != next || x.seq <= lastSeq || x.time_ms < lastTime)
				order = false;
			if (x.source != BF_APP_SOURCE_INJECTED)
				sourceOk = false;
			lastSeq = x.seq;
			lastTime = x.time_ms;
			++next;
		}
	}
	poster.join();
	CHECK(next == kCount && order && sourceOk, "post-from-thread", "%d of %d events back, order %s, source %s",
	      next, kCount, order ? "kept" : "BROKEN", sourceOk ? "injected" : "WRONG");

	// --- a post wakes a waiting pump
	{
		Drain(50);
		std::thread late([] {
			usleep(50000);
			bf_app_event u = {};
			u.type = BF_APP_EVENT_USER;
			u.user.code = 8;
			bf_app_post_event(&u);
		});
		double t0 = NowMs();
		bool woke = false;
		while (NowMs() - t0 < 3000 && !woke) {
			bf_app_pump(2000);
			bf_app_event x;
			while (bf_app_poll_event(&x))
				if (x.type == BF_APP_EVENT_USER && x.user.code == 8)
					woke = true;
				else
					g_seen.push_back(x);
		}
		double took = NowMs() - t0;
		late.join();
		CHECK(woke && took < 1000, "pump-wake", "pump(2000) returned with the posted event after %.0f ms", took);
	}

	// --- injected keys and pointer drive the state
	{
		Drain(50);
		auto key = [](uint32_t type, int k) {
			bf_app_event x = {};
			x.type = type;
			x.key.key = (uint16_t)k;
			bf_app_post_event(&x);
		};
		key(BF_APP_EVENT_KEY_DOWN, BF_KEY_LSHIFT);
		key(BF_APP_EVENT_KEY_DOWN, BF_KEY_A);
		bf_app_event a = {};
		bool gotA = WaitFor(500, &a, [](const bf_app_event &x) { return x.type == BF_APP_EVENT_KEY_DOWN && x.key.key == BF_KEY_A; });
		bool stateDown = bf_app_key_is_down(BF_KEY_A) && bf_app_key_is_down(BF_KEY_LSHIFT) && bf_app_mac_key_is_down(0x00) &&
		                 (bf_app_modifiers() & BF_APP_MOD_SHIFT);
		key(BF_APP_EVENT_KEY_UP, BF_KEY_A);
		key(BF_APP_EVENT_KEY_UP, BF_KEY_LSHIFT);
		Drain(50);
		bool stateUp = !bf_app_key_is_down(BF_KEY_A) && !bf_app_key_is_down(BF_KEY_LSHIFT) && !(bf_app_modifiers() & BF_APP_MOD_SHIFT);
		CHECK(gotA && (a.mods & BF_APP_MOD_SHIFT) && a.key.mac_keycode == 0x00 && stateDown && stateUp, "inject-keys",
		      "Shift+A: mods %02x, mac 0x%02x, held while down %s, released %s", a.mods, a.key.mac_keycode,
		      stateDown ? "yes" : "no", stateUp ? "yes" : "no");

		float dx0, dy0, w0;
		bf_app_pointer_take_delta(&dx0, &dy0, &w0);
		auto ptr = [](uint32_t type, float x, float y, uint32_t button, float wheel) {
			bf_app_event p = {};
			p.type = type;
			p.pointer.x_px = x;
			p.pointer.y_px = y;
			p.pointer.button = button;
			p.pointer.wheel_y = wheel;
			bf_app_post_event(&p);
		};
		bf_app_pointer before;
		bf_app_pointer_get(&before);
		ptr(BF_APP_EVENT_POINTER_MOVE, 100, 50, 0, 0);
		ptr(BF_APP_EVENT_BUTTON_DOWN, 100, 50, BF_APP_BUTTON_RIGHT, 0);
		ptr(BF_APP_EVENT_POINTER_MOVE, 130, 40, 0, 0);
		ptr(BF_APP_EVENT_WHEEL, 130, 40, 0, -2);
		bf_app_event m = {};
		WaitFor(500, &m, [](const bf_app_event &x) { return x.type == BF_APP_EVENT_WHEEL && x.source == BF_APP_SOURCE_INJECTED; });
		bf_app_pointer p;
		bf_app_pointer_get(&p);
		float dx, dy, wheel;
		bf_app_pointer_take_delta(&dx, &dy, &wheel);
		float wantDx = 130 - before.x_px, wantDy = 40 - before.y_px;
		bool held = (p.buttons & (1u << BF_APP_BUTTON_RIGHT)) != 0;
		ptr(BF_APP_EVENT_BUTTON_UP, 130, 40, BF_APP_BUTTON_RIGHT, 0);
		Drain(50);
		bf_app_pointer after;
		bf_app_pointer_get(&after);
		bool ok = p.x_px == 130 && p.y_px == 40 && fabsf(p.x_pt - 130 / (float)s.scale) < 0.01f && held &&
		          fabsf(dx - wantDx) < 0.01f && fabsf(dy - wantDy) < 0.01f && wheel == -2 && m.pointer.buttons == 2 &&
		          !(after.buttons & 2);
		CHECK(ok, "inject-pointer", "at (%.0f,%.0f) px = (%.1f,%.1f) pt, right held %s then released %s, delta (%.0f,%.0f) want (%.0f,%.0f), wheel %.0f",
		      p.x_px, p.y_px, p.x_pt, p.y_pt, held ? "yes" : "no", !(after.buttons & 2) ? "yes" : "no", dx, dy, wantDx, wantDy, wheel);
	}

	// --- the keyboard layout, read through the system without selecting it
	{
		char b[16];
		int dead = 0;
		bool de = bf_app_key_layout_use("com.apple.keylayout.German") == BF_APP_OK;
		std::string deL, deN;
		int deNdead = 0, dePlainAt = 1;
		if (de) {
			bf_app_key_layout_text(0x25, BF_APP_MOD_ALT, b, sizeof b, &dead);
			deL = b;
			bf_app_key_layout_text(0x2D, BF_APP_MOD_ALT, b, sizeof b, &deNdead);
			deN = b;
			dePlainAt = bf_app_key_layout_types_plain("@");
		}
		bool us = bf_app_key_layout_use("com.apple.keylayout.US") == BF_APP_OK;
		std::string usL, usShift2;
		if (us) {
			bf_app_key_layout_text(0x25, BF_APP_MOD_ALT, b, sizeof b, nullptr);
			usL = b;
			bf_app_key_layout_text(0x13, BF_APP_MOD_SHIFT, b, sizeof b, nullptr);
			usShift2 = b;
		}
		bool bogus = bf_app_key_layout_use("com.example.no-such-layout") == BF_APP_ERROR &&
		             bf_app_key_layout_named() && strcmp(bf_app_key_layout_named(), "com.apple.keylayout.US") == 0;
		bf_app_key_layout_use(nullptr);
		CHECK(de && us && deL == "@" && !dead && deN == "~" && deNdead && !dePlainAt && usL == "\xc2\xac" && usShift2 == "@" && bogus &&
		      !bf_app_key_layout_named(),
		      "key-layout", "German Option+L \"%s\", Option+N \"%s\" (dead %d), @ without Option %d; US Option+L \"%s\", "
		      "Shift+2 \"%s\"; an unknown id refused, the choice kept; NULL back to the layout in effect",
		      deL.c_str(), deN.c_str(), deNdead, dePlainAt, usL.c_str(), usShift2.c_str());
	}

	// --- the AppKit path, with synthetic NSEvents through the window server queue
	focused = bf_app_window_is_focused();
	const bool locked = ScreenLocked();
	g_direct = !focused;
	INFO("appkit-path", "synthetic NSEvents go %s%s", focused ? "through [NSApp postEvent:] and the pump" :
	     "to the main menu and [NSWindow sendEvent:] directly, since the window is not key", locked ? " (screen is locked)" : "");
	{
		Drain(50);
		g_seen.clear();
		Deliver(KeyEvent(NSEventTypeKeyDown, 0x00, 0, @"a", @"a"));
		Deliver(KeyEvent(NSEventTypeKeyUp, 0x00, 0, @"a", @"a"));
		unichar f1c = NSF1FunctionKey;
		NSString *f1 = [NSString stringWithCharacters:&f1c length:1];
		Deliver(KeyEvent(NSEventTypeKeyDown, 0x7A, NSEventModifierFlagFunction, f1, f1));
		Deliver(KeyEvent(NSEventTypeKeyUp, 0x7A, NSEventModifierFlagFunction, f1, f1));
		Deliver(KeyEvent(NSEventTypeKeyDown, 0x00, NSEventModifierFlagOption, @"å", @"a"));
		Deliver(KeyEvent(NSEventTypeKeyUp, 0x00, NSEventModifierFlagOption, @"å", @"a"));
		Drain(300);
		const bf_app_event *aDown = nullptr, *aText = nullptr, *f1Down = nullptr, *f1Text = nullptr, *optDown = nullptr, *optText = nullptr;
		int aUps = 0;
		for (const bf_app_event &x : g_seen) {
			if (x.source != BF_APP_SOURCE_APPKIT)
				continue;
			if (x.type == BF_APP_EVENT_KEY_DOWN && x.key.key == BF_KEY_A && !(x.mods & BF_APP_MOD_ALT) && !aDown) aDown = &x;
			else if (x.type == BF_APP_EVENT_KEY_DOWN && x.key.key == BF_KEY_A && (x.mods & BF_APP_MOD_ALT)) optDown = &x;
			else if (x.type == BF_APP_EVENT_KEY_DOWN && x.key.key == BF_KEY_F1) f1Down = &x;
			else if (x.type == BF_APP_EVENT_KEY_UP && x.key.key == BF_KEY_A) ++aUps;
			else if (x.type == BF_APP_EVENT_TEXT) {
				if (aDown && x.text.key_seq == aDown->seq) aText = &x;
				else if (f1Down && x.text.key_seq == f1Down->seq) f1Text = &x;
				else if (optDown && x.text.key_seq == optDown->seq) optText = &x;
			}
		}
		CHECK(aDown && aText && strcmp(aText->text.utf8, "a") == 0 && aUps == 2 && !(aDown->mods & BF_APP_MOD_FN),
		      "appkit-keys", "a: key_down mac 0x00 %s, text \"%s\" linked to it, %d key-ups",
		      aDown ? "seen" : "MISSING", aText ? aText->text.utf8 : "(none)", aUps);
		CHECK(f1Down && !f1Text && !(f1Down->mods & BF_APP_MOD_FN), "appkit-f1", "F1 (kVK 0x7A): key_down %s, text %s, mods %02x",
		      f1Down ? "seen as BF_KEY_F1" : "MISSING", f1Text ? "WRONGLY typed" : "none", f1Down ? f1Down->mods : 0);
		CHECK(optDown && (optDown->mods & BF_APP_MOD_ALT) && optText && strcmp(optText->text.utf8, "å") == 0,
		      "appkit-option", "Option+A: key_down with Alt %s, its text \"%s\" (separate, so the adapter can drop it)",
		      optDown ? "seen" : "MISSING", optText ? optText->text.utf8 : "(none)");

		// The text filter keeps a key-down from the text system: no text.
		static uint64_t filterSeq = 0;
		bf_app_set_text_filter([](const bf_app_event *k, void *) {
			filterSeq = k->seq;
			return (k->mods & BF_APP_MOD_ALT) ? 0 : 1;
		}, nullptr);
		g_seen.clear();
		Deliver(KeyEvent(NSEventTypeKeyDown, 0x00, NSEventModifierFlagOption, @"å", @"a"));
		Deliver(KeyEvent(NSEventTypeKeyUp, 0x00, NSEventModifierFlagOption, @"å", @"a"));
		Deliver(KeyEvent(NSEventTypeKeyDown, 0x00, 0, @"a", @"a"));
		Deliver(KeyEvent(NSEventTypeKeyUp, 0x00, 0, @"a", @"a"));
		Drain(300);
		bf_app_set_text_filter(nullptr, nullptr);
		const bf_app_event *fDown = nullptr, *fText = nullptr, *pText = nullptr;
		uint64_t plainSeq = 0;
		for (const bf_app_event &x : g_seen) {
			if (x.source != BF_APP_SOURCE_APPKIT)
				continue;
			if (x.type == BF_APP_EVENT_KEY_DOWN && (x.mods & BF_APP_MOD_ALT)) fDown = &x;
			else if (x.type == BF_APP_EVENT_KEY_DOWN) plainSeq = x.seq;
			else if (x.type == BF_APP_EVENT_TEXT && fDown && x.text.key_seq == fDown->seq) fText = &x;
			else if (x.type == BF_APP_EVENT_TEXT && plainSeq && x.text.key_seq == plainSeq) pText = &x;
		}
		CHECK(fDown && !fText && pText && filterSeq == plainSeq, "appkit-text-filter",
		      "Option+A kept from the text system: key_down %s, text %s; A then typed \"%s\"; the filter saw seq %llu (want %llu)",
		      fDown ? "seen" : "MISSING", fText ? "WRONGLY typed" : "none", pText ? pText->text.utf8 : "(none)",
		      (unsigned long long)filterSeq, (unsigned long long)plainSeq);

		// Modifiers through flagsChanged, with device bits for the left Shift.
		g_seen.clear();
		Deliver(KeyEvent(NSEventTypeFlagsChanged, 0x38, NSEventModifierFlagShift | 0x2, @"", @""));
		Drain(100);
		bool shiftDown = bf_app_key_is_down(BF_KEY_LSHIFT) && !bf_app_key_is_down(BF_KEY_RSHIFT);
		Deliver(KeyEvent(NSEventTypeFlagsChanged, 0x38, 0, @"", @""));
		Drain(100);
		CHECK(shiftDown && !bf_app_key_is_down(BF_KEY_LSHIFT), "appkit-modifiers", "left Shift down %s, released %s",
		      shiftDown ? "yes" : "no", !bf_app_key_is_down(BF_KEY_LSHIFT) ? "yes" : "no");

		// Pointer: a click at (200,100) pt from the top left.
		g_seen.clear();
		Deliver(MouseEvent(NSEventTypeLeftMouseDown, NSMakePoint(200, 100)));
		Deliver(MouseEvent(NSEventTypeLeftMouseUp, NSMakePoint(200, 100)));
		bf_app_event up = {};
		bool gotUp = WaitFor(500, &up, [](const bf_app_event &x) { return x.type == BF_APP_EVENT_BUTTON_UP && x.source == BF_APP_SOURCE_APPKIT; });
		bool downSeen = false;
		for (const bf_app_event &x : g_seen)
			if (x.type == BF_APP_EVENT_BUTTON_DOWN && x.pointer.button == BF_APP_BUTTON_LEFT && x.pointer.buttons == 1)
				downSeen = true;
		CHECK(gotUp && downSeen && up.pointer.x_pt == 200 && up.pointer.y_pt == 100 && up.pointer.x_px == (float)(200 * s.scale),
		      "appkit-click", "left down %s, up at (%.0f,%.0f) pt = (%.0f,%.0f) px", downSeen ? "seen" : "MISSING",
		      up.pointer.x_pt, up.pointer.y_pt, up.pointer.x_px, up.pointer.y_px);

		// Cmd+Q is a quit event, not termination.
		Deliver(KeyEvent(NSEventTypeKeyDown, 0x0C, NSEventModifierFlagCommand, @"q", @"q"));
		bf_app_event q = {};
		bool gotQ = WaitFor(1000, &q, [](const bf_app_event &x) { return x.type == BF_APP_EVENT_QUIT; });
		CHECK(gotQ && q.value == BF_APP_QUIT_COMMAND, "cmd-q", "%s (reason %d); process still running",
		      gotQ ? "quit event" : "no quit event", q.value);
	}

	// --- close button -> quit event, window stays
	{
		[Window() performClose:nil];
		bf_app_event q = {};
		bool gotQ = WaitFor(1000, &q, [](const bf_app_event &x) { return x.type == BF_APP_EVENT_QUIT; });
		CHECK(gotQ && q.value == BF_APP_QUIT_CLOSE_BUTTON && Window().visible, "close-button", "%s (reason %d), window %s",
		      gotQ ? "quit event" : "no quit event", q.value, Window().visible ? "still open" : "GONE");
	}

	// --- terminate request (the Dock's Quit) -> quit event
	{
		[NSApp terminate:nil];
		bf_app_event q = {};
		bool gotQ = WaitFor(1000, &q, [](const bf_app_event &x) { return x.type == BF_APP_EVENT_QUIT; });
		CHECK(gotQ && q.value == BF_APP_QUIT_SYSTEM, "terminate", "[NSApp terminate:] gave %s (reason %d)",
		      gotQ ? "a quit event" : "no quit event", q.value);
	}

	// --- clipboard round trip, previous contents restored
	{
		NSPasteboard *pb = [NSPasteboard generalPasteboard];
		NSMutableArray<NSPasteboardItem *> *saved = [NSMutableArray array];
		for (NSPasteboardItem *item in pb.pasteboardItems) {
			NSPasteboardItem *copy = [NSPasteboardItem new];
			for (NSPasteboardType t in item.types) {
				NSData *data = [item dataForType:t];
				if (data)
					[copy setData:data forType:t];
			}
			[saved addObject:copy];
		}
		NSString *before = [pb stringForType:NSPasteboardTypeString];
		const char *text = "Bifrost clipboard: \xc3\xa9t\xc3\xa9 \xe2\x82\xac 12";
		int sr = bf_app_clipboard_set_text(text);
		char buf[256];
		size_t len = bf_app_clipboard_get_text(buf, sizeof buf);
		char small[22];   // cuts into the euro sign: must back off to a boundary
		size_t len2 = bf_app_clipboard_get_text(small, sizeof small);
		bool roundTrip = sr == BF_APP_OK && len == strlen(text) && strcmp(buf, text) == 0;
		bool truncated = len2 == strlen(text) && strlen(small) < sizeof small && [NSString stringWithUTF8String:small] != nil;
		[pb clearContents];
		if (saved.count)
			[pb writeObjects:saved];
		NSString *after = [pb stringForType:NSPasteboardTypeString];
		bool restored = (before == nil && after == nil) || [before isEqualToString:after];
		CHECK(roundTrip && truncated && restored, "clipboard", "round trip %s, truncation at a character boundary %s (\"%s\"), previous contents %s",
		      roundTrip ? "ok" : "WRONG", truncated ? "ok" : "WRONG", small, restored ? "restored" : "NOT restored");
	}

	// --- fullscreen: Cmd+F (if focused) in, the function out
	{
		Deliver(KeyEvent(NSEventTypeKeyDown, 0x03, NSEventModifierFlagCommand, @"f", @"f"));   // Cmd+F
		bf_app_event f = {};
		bool fsIn = WaitFor(5000, &f, [](const bf_app_event &x) { return x.type == BF_APP_EVENT_FULLSCREEN && x.value == 1; });
		bf_app_size fs;
		bf_app_window_get_size(&fs);
		bool isFs = bf_app_window_is_fullscreen();
		Drain(300);
		bf_app_window_toggle_fullscreen();
		bool fsOut = WaitFor(5000, &f, [](const bf_app_event &x) { return x.type == BF_APP_EVENT_FULLSCREEN && x.value == 0; });
		Drain(300);
		bf_app_size back;
		bf_app_window_get_size(&back);
		if (!fsIn && !isFs && locked)
			SKIP("fullscreen", "Cmd+F asked for fullscreen, which macOS refuses while the screen is locked (see the err trace line)");
		else
			CHECK(fsIn && isFs && fsOut && !bf_app_window_is_fullscreen() && back.width_pt == 800, "fullscreen",
			      "entered via Cmd+F (%.0fx%.0f pt), left via bf_app_window_toggle_fullscreen, back to %.0fx%.0f pt",
			      fs.width_pt, fs.height_pt, back.width_pt, back.height_pt);
	}

	// --- drawing while visible and while minimised (what the background costs)
	if (isMetal) {
		int frames = 0, nils = 0;
		MeasureDrawing("visible", 1000, &frames, &nils);
		INFO("draw-visible", "%d frames presented in 1 s, %d nil drawables", frames, nils);
		[Window() miniaturize:nil];
		Drain(1000);
		bool mini = Window().miniaturized;
		int frames2 = 0, nils2 = 0;
		MeasureDrawing("minimised", 1000, &frames2, &nils2);
		INFO("draw-minimised", "window %s minimised, visible() = %d: %d frames in 1 s, %d nil drawables",
		     mini ? "is" : "is NOT", bf_app_window_is_visible(), frames2, nils2);
		bool sawHidden = false;
		for (const bf_app_event &x : g_seen)
			if (x.type == BF_APP_EVENT_VISIBILITY && x.value == 0)
				sawHidden = true;
		[Window() deminiaturize:nil];
		Drain(1000);
		if (locked)
			SKIP("visibility-event", "the screen is locked, so the window counts as occluded throughout");
		else
			CHECK(!mini || sawHidden, "visibility-event", "minimising %s a visibility(0) event", sawHidden ? "gave" : "did NOT give");
		CHECK(frames2 >= 50, "draw-background", "%d frames per second while minimised (>= 50 wanted)", frames2);
	}

	// --- single instance: a second launch with the same id brings us forward and exits
	{
		g_seen.clear();
		const char *args[] = { exe, "--second-instance", instanceId, nullptr };
		double t0 = NowMs();
		int code = RunChild(args, false);
		double took = NowMs() - t0;
		bool reopen = false;
		for (const bf_app_event &x : g_seen)
			if (x.type == BF_APP_EVENT_REOPEN && x.value == 1)
				reopen = true;
		if (!reopen)
			reopen = WaitFor(500, nullptr, [](const bf_app_event &x) { return x.type == BF_APP_EVENT_REOPEN && x.value == 1; });
		CHECK(code == 3 && reopen, "single-instance", "second launch exited with %d (3 = already running) after %.0f ms; reopen event %s; app %s active",
		      code, took, reopen ? "received" : "MISSING", NSApp.active ? "is" : "is not");
		const char *args2[] = { exe, "--second-instance", instanceId, nullptr };
		int code2 = RunChild(args2, true);
		CHECK(code2 == 0, "multi-instance", "with BF_MULTI_INSTANCE=1 the second launch initialised normally (exit %d)", code2);
	}

	// --- shutdown releases the instance lock
	bf_app_shutdown();
	{
		NSURL *base = [[NSFileManager defaultManager] URLForDirectory:NSApplicationSupportDirectory inDomain:NSUserDomainMask
		                                            appropriateForURL:nil create:NO error:nil];
		NSString *path = [[base.path stringByAppendingPathComponent:@BF_APP_NAME]
			stringByAppendingPathComponent:[NSString stringWithFormat:@"instance-%s.lock", instanceId]];
		int fd = open(path.fileSystemRepresentation, O_RDWR);
		bool lockFree = fd >= 0 && flock(fd, LOCK_EX | LOCK_NB) == 0;
		if (fd >= 0)
			close(fd);
		unlink(path.fileSystemRepresentation);   // test instances leave no file behind
		CHECK(lockFree && !bf_app_activity_held(), "shutdown", "instance lock %s, activity %s", lockFree ? "released" : "STILL HELD",
		      bf_app_activity_held() ? "STILL HELD" : "ended");
	}

	printf("%s: %d failure(s)\n", g_failures ? "FAILED" : "PASSED", g_failures);
	return g_failures ? 1 : 0;
}

} // namespace

int main(int argc, char **argv)
{
	@autoreleasepool {
		bf_trace_init();
		if (argc >= 3 && strcmp(argv[1], "--second-instance") == 0)
			return SecondInstance(argv[2]);
		bool interactive = false, hide = false;
		for (int i = 1; i < argc; ++i) {
			if (strcmp(argv[i], "--interactive") == 0) interactive = true;
			else if (strcmp(argv[i], "--hide-cursor") == 0) hide = true;
		}
		if (interactive)
			return Interactive(hide);
		NSString *exePath = [[NSBundle mainBundle] executablePath];
		return RunChecks(exePath ? exePath.fileSystemRepresentation : argv[0]);
	}
}
