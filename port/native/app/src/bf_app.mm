// The AppKit side of the application module: the process as a regular app
// without [NSApp run], the window and its Metal layer, AppKit events turned
// into queue entries, single instance, App Nap, the clipboard.

#import <AppKit/AppKit.h>
#import <QuartzCore/CAMetalLayer.h>
#import <Metal/Metal.h>

#include "bf_app_internal.h"
#include "bf_trace.h"

#include <atomic>
#include <fcntl.h>
#include <math.h>
#include <string.h>
#include <sys/file.h>
#include <unistd.h>

#ifndef BF_APP_NAME
#error BF_APP_NAME must be defined by the build (port/name.cmake)
#endif
#ifndef BF_BUNDLE_ID
#error BF_BUNDLE_ID must be defined by the build (port/name.cmake)
#endif

@class BFAppView;

namespace {

const short kWakeSubtype = 0x4246;   // an NSEventTypeApplicationDefined that only ends a wait

bool g_initialised = false;
id<NSObject> g_activity = nil;
NSWindow *g_window = nil;
BFAppView *g_view = nil;
id g_delegate = nil;

bool g_inPump = false;
std::atomic<bool> g_pumpWaiting{false};

void (*g_modalTick)(void *) = nullptr;
void *g_modalTickContext = nullptr;
NSTimer *g_modalTimer = nil;

bool g_cursorHidden = false;
NSCursor *g_blankCursor = nil;

// What AppKit has reported held, so that focus loss can release it.
bool g_akKeys[128];
uint32_t g_akButtons;
float g_lastPx, g_lastPy, g_lastPtX, g_lastPtY;

// bf_app_set_text_filter.
bf_app_text_filter g_textFilter = nullptr;
void *g_textFilterContext = nullptr;

// Single instance.
int g_lockFd = -1;
NSString *g_lockPath = nil;
CFMessagePortRef g_port = NULL;

uint32_t ModsFromFlags(NSEventModifierFlags f)
{
	uint32_t m = 0;
	if (f & NSEventModifierFlagShift) m |= BF_APP_MOD_SHIFT;
	if (f & NSEventModifierFlagControl) m |= BF_APP_MOD_CTRL;
	if (f & NSEventModifierFlagOption) m |= BF_APP_MOD_ALT;
	if (f & NSEventModifierFlagCommand) m |= BF_APP_MOD_CMD;
	if (f & NSEventModifierFlagCapsLock) m |= BF_APP_MOD_CAPSLOCK;
	// AppKit sets NSEventModifierFlagFunction on arrow and function keys
	// too; Fn counts only while the Fn key itself is held.
	if (g_akKeys[0x3F]) m |= BF_APP_MOD_FN;
	return m;
}

void PostSimple(uint32_t type, int32_t value)
{
	bf_app_event e = {};
	e.type = type;
	e.value = value;
	e.mods = ModsFromFlags([NSEvent modifierFlags]);
	bf_app_enqueue_appkit(&e);
}

uint64_t PostKey(uint32_t type, unsigned keycode, uint32_t mods, bool repeat)
{
	bf_app_event e = {};
	e.type = type;
	e.mods = mods;
	e.key.mac_keycode = (uint16_t)keycode;
	e.key.key = (uint16_t)bf_app_key_from_mac(keycode);
	e.key.repeat = repeat ? 1 : 0;
	if (keycode < 128)
		g_akKeys[keycode] = (type == BF_APP_EVENT_KEY_DOWN);
	return bf_app_enqueue_appkit(&e);
}

void ReleaseAllHeld()
{
	uint32_t mods = 0;
	for (unsigned k = 0; k < 128; ++k)
		if (g_akKeys[k])
			PostKey(BF_APP_EVENT_KEY_UP, k, mods, false);
	for (uint32_t b = 0; b < BF_APP_BUTTON_COUNT; ++b) {
		if (!(g_akButtons & (1u << b)))
			continue;
		g_akButtons &= ~(1u << b);
		bf_app_event e = {};
		e.type = BF_APP_EVENT_BUTTON_UP;
		e.flags = BF_APP_EVF_DELTA_VALID;
		e.pointer.x_px = g_lastPx; e.pointer.y_px = g_lastPy;
		e.pointer.x_pt = g_lastPtX; e.pointer.y_pt = g_lastPtY;
		e.pointer.button = b;
		bf_app_enqueue_appkit(&e);
	}
}

NSString *SupportFolder()
{
	NSURL *base = [[NSFileManager defaultManager] URLForDirectory:NSApplicationSupportDirectory
	                                                     inDomain:NSUserDomainMask
	                                            appropriateForURL:nil create:YES error:nil];
	if (!base)
		return nil;
	NSURL *dir = [base URLByAppendingPathComponent:@BF_APP_NAME isDirectory:YES];
	[[NSFileManager defaultManager] createDirectoryAtURL:dir withIntermediateDirectories:YES attributes:nil error:nil];
	return dir.path;
}

void BringToFront()
{
	if (g_window) {
		if (g_window.miniaturized)
			[g_window deminiaturize:nil];
		[g_window makeKeyAndOrderFront:nil];
	}
	[NSApp activate];
}

CFDataRef PortCallback(CFMessagePortRef, SInt32 msgid, CFDataRef, void *)
{
	if (msgid == 1) {
		BF_TRACE(BFT_INPUT, "app: a second instance was started; coming to the front");
		BringToFront();
		PostSimple(BF_APP_EVENT_REOPEN, 1);
	}
	pid_t pid = getpid();
	return CFDataCreate(NULL, (const UInt8 *)&pid, sizeof pid);
}

// Returns BF_APP_OK if this process now owns the instance, BF_APP_ALREADY_RUNNING otherwise.
int ClaimInstance(const char *instanceId)
{
	NSString *folder = SupportFolder();
	NSString *portName = [NSString stringWithFormat:@"%s.instance.%s", BF_BUNDLE_ID, instanceId];
	bool owner = true;

	if (folder) {
		g_lockPath = [folder stringByAppendingPathComponent:[NSString stringWithFormat:@"instance-%s.lock", instanceId]];
		int fd = open(g_lockPath.fileSystemRepresentation, O_RDWR | O_CREAT | O_CLOEXEC, 0644);
		if (fd < 0) {
			bf_tracef(BFT_ERR, "app: cannot open the instance lock %s: %s", g_lockPath.UTF8String, strerror(errno));
		} else if (flock(fd, LOCK_EX | LOCK_NB) == 0) {
			char text[32];
			int n = snprintf(text, sizeof text, "%d\n", (int)getpid());
			ftruncate(fd, 0);
			pwrite(fd, text, (size_t)n, 0);
			g_lockFd = fd;
		} else {
			owner = false;
			close(fd);
		}
	} else {
		bf_tracef(BFT_ERR, "app: no Application Support folder; single instance by message port only");
	}

	if (owner) {
		CFMessagePortContext ctx = {};
		Boolean shouldFree = false;
		g_port = CFMessagePortCreateLocal(NULL, (__bridge CFStringRef)portName, PortCallback, &ctx, &shouldFree);
		if (!g_port) {
			// The name is taken: without a lock file this is the only sign of
			// another instance; with one, a stale registration we ignore.
			if (g_lockFd < 0)
				owner = false;
			else
				bf_tracef(BFT_ERR, "app: cannot register the instance port %s", portName.UTF8String);
		} else {
			CFRunLoopSourceRef src = CFMessagePortCreateRunLoopSource(NULL, g_port, 0);
			CFRunLoopAddSource(CFRunLoopGetMain(), src, kCFRunLoopCommonModes);
			CFRelease(src);
			return BF_APP_OK;
		}
		if (owner)
			return BF_APP_OK;
	}

	// Another instance owns the slot: tell it to come forward. It may still
	// be starting, so the port is retried for two seconds.
	pid_t otherPid = 0;
	for (int attempt = 0; attempt < 40; ++attempt) {
		CFMessagePortRef remote = CFMessagePortCreateRemote(NULL, (__bridge CFStringRef)portName);
		if (remote) {
			CFDataRef reply = NULL;
			SInt32 r = CFMessagePortSendRequest(remote, 1, NULL, 2.0, 2.0, kCFRunLoopDefaultMode, &reply);
			if (r == kCFMessagePortSuccess && reply && CFDataGetLength(reply) == sizeof(pid_t))
				memcpy(&otherPid, CFDataGetBytePtr(reply), sizeof otherPid);
			if (reply)
				CFRelease(reply);
			CFRelease(remote);
			if (r == kCFMessagePortSuccess)
				break;
		}
		usleep(50000);
	}
	if (otherPid > 0) {
		NSRunningApplication *other = [NSRunningApplication runningApplicationWithProcessIdentifier:otherPid];
		if (other) {
			[NSApp yieldActivationToApplication:other];
			[other activateWithOptions:NSApplicationActivateAllWindows];
		}
		BF_TRACE(BFT_INPUT, "app: instance '%s' runs as pid %d; it was brought to the front", instanceId, (int)otherPid);
	} else {
		bf_tracef(BFT_ERR, "app: instance '%s' is locked but did not answer", instanceId);
	}
	return BF_APP_ALREADY_RUNNING;
}

void ReleaseInstance()
{
	if (g_port) {
		CFMessagePortInvalidate(g_port);
		CFRelease(g_port);
		g_port = NULL;
	}
	if (g_lockFd >= 0) {
		ftruncate(g_lockFd, 0);
		close(g_lockFd);   // releases the flock; the file stays for the next start
		g_lockFd = -1;
	}
}

void WakeHook()
{
	if (!g_pumpWaiting.load())
		return;
	NSEvent *wake = [NSEvent otherEventWithType:NSEventTypeApplicationDefined location:NSZeroPoint
	                              modifierFlags:0 timestamp:0 windowNumber:0 context:nil
	                                    subtype:kWakeSubtype data1:0 data2:0];
	[NSApp postEvent:wake atStart:NO];
}

NSMenuItem *AddItem(NSMenu *menu, NSString *title, SEL action, NSString *key, id target)
{
	NSMenuItem *item = [menu addItemWithTitle:title action:action keyEquivalent:key];
	item.target = target;
	return item;
}

} // namespace

// ---------------------------------------------------------------- delegate

@interface BFAppDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate>
@end

@implementation BFAppDelegate

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender
{
	// Never terminate behind the client's back: it decides when to exit.
	PostSimple(BF_APP_EVENT_QUIT, BF_APP_QUIT_SYSTEM);
	return NSTerminateCancel;
}

- (BOOL)applicationShouldHandleReopen:(NSApplication *)sender hasVisibleWindows:(BOOL)flag
{
	BringToFront();
	PostSimple(BF_APP_EVENT_REOPEN, 0);
	return NO;
}

- (BOOL)applicationSupportsSecureRestorableState:(NSApplication *)app
{
	return YES;
}

- (void)bfQuit:(id)sender
{
	PostSimple(BF_APP_EVENT_QUIT, BF_APP_QUIT_COMMAND);
}

- (void)bfToggleFullscreen:(id)sender
{
	bf_app_window_toggle_fullscreen();
}

- (void)bfModalTick:(NSTimer *)timer
{
	if (g_modalTick)
		g_modalTick(g_modalTickContext);
}

- (BOOL)windowShouldClose:(NSWindow *)sender
{
	PostSimple(BF_APP_EVENT_QUIT, BF_APP_QUIT_CLOSE_BUTTON);
	return NO;
}

- (void)windowDidBecomeKey:(NSNotification *)n
{
	PostSimple(BF_APP_EVENT_FOCUS, 1);
}

- (void)windowDidResignKey:(NSNotification *)n
{
	ReleaseAllHeld();
	PostSimple(BF_APP_EVENT_FOCUS, 0);
}

- (void)windowDidEnterFullScreen:(NSNotification *)n
{
	PostSimple(BF_APP_EVENT_FULLSCREEN, 1);
}

- (void)windowDidFailToEnterFullScreen:(NSWindow *)window
{
	bf_tracef(BFT_ERR, "app: the window could not enter fullscreen");
}

- (void)windowDidExitFullScreen:(NSNotification *)n
{
	PostSimple(BF_APP_EVENT_FULLSCREEN, 0);
}

- (void)windowDidChangeOcclusionState:(NSNotification *)n
{
	PostSimple(BF_APP_EVENT_VISIBILITY, bf_app_window_is_visible());
}

@end

// -------------------------------------------------------------------- view

@interface BFAppView : NSView <NSTextInputClient>
- (void)bfUpdateSize:(BOOL)announce;
- (void)bfStartAnnouncing;
- (void)bfStopAnnouncing;
@property (readonly) bf_app_size bfSize;
@end

@implementation BFAppView {
	bf_app_size _size;
	BOOL _announce;
	NSTrackingArea *_tracking;
	NSEvent *_lastMoved;
	uint64_t _currentKeySeq;
	NSString *_marked;
}

- (instancetype)initWithFrame:(NSRect)frame
{
	if ((self = [super initWithFrame:frame])) {
		self.wantsLayer = YES;
		self.layerContentsRedrawPolicy = NSViewLayerContentsRedrawDuringViewResize;
		self.layerContentsPlacement = NSViewLayerContentsPlacementTopLeft;
		_marked = @"";
	}
	return self;
}

- (CALayer *)makeBackingLayer
{
	CAMetalLayer *layer = [CAMetalLayer layer];
	layer.device = MTLCreateSystemDefaultDevice();
	layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
	return layer;
}

- (BOOL)isFlipped { return YES; }
- (BOOL)acceptsFirstResponder { return YES; }
- (BOOL)acceptsFirstMouse:(NSEvent *)event { return YES; }
- (bf_app_size)bfSize { return _size; }

- (void)bfUpdateSize:(BOOL)announce
{
	NSSize pt = self.bounds.size;
	double scale = self.window ? self.window.backingScaleFactor : 1.0;
	bf_app_size s;
	s.width_pt = pt.width;
	s.height_pt = pt.height;
	s.width_px = (int)lround(pt.width * scale);
	s.height_px = (int)lround(pt.height * scale);
	s.scale = scale;
	CAMetalLayer *layer = (CAMetalLayer *)self.layer;
	if ([layer isKindOfClass:[CAMetalLayer class]]) {
		layer.contentsScale = scale;
		if (s.width_px > 0 && s.height_px > 0)
			layer.drawableSize = CGSizeMake(s.width_px, s.height_px);
	}
	bf_app_queue_set_scale(scale);
	bool changed = s.width_px != _size.width_px || s.height_px != _size.height_px ||
	               s.scale != _size.scale || s.width_pt != _size.width_pt || s.height_pt != _size.height_pt;
	_size = s;
	if (changed && announce && _announce) {
		bf_app_event e = {};
		e.type = BF_APP_EVENT_RESIZE;
		e.size.size = s;
		bf_app_enqueue_appkit(&e);
		BF_TRACE(BFT_INPUT, "app: resize %.0fx%.0f pt, %dx%d px, scale %.1f",
		         s.width_pt, s.height_pt, s.width_px, s.height_px, s.scale);
	}
}

- (void)bfStartAnnouncing { _announce = YES; }
- (void)bfStopAnnouncing { _announce = NO; }

- (void)setFrameSize:(NSSize)size
{
	[super setFrameSize:size];
	[self bfUpdateSize:YES];
}

- (void)viewDidChangeBackingProperties
{
	[super viewDidChangeBackingProperties];
	[self bfUpdateSize:YES];
}

- (void)viewDidMoveToWindow
{
	[super viewDidMoveToWindow];
	[self bfUpdateSize:YES];
}

- (void)updateTrackingAreas
{
	if (_tracking)
		[self removeTrackingArea:_tracking];
	_tracking = [[NSTrackingArea alloc] initWithRect:NSZeroRect
	                                         options:NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved |
	                                                 NSTrackingCursorUpdate | NSTrackingActiveAlways |
	                                                 NSTrackingInVisibleRect
	                                           owner:self userInfo:nil];
	[self addTrackingArea:_tracking];
	[super updateTrackingAreas];
}

- (void)cursorUpdate:(NSEvent *)event
{
	if (g_cursorHidden)
		[g_blankCursor set];
	else
		[[NSCursor arrowCursor] set];
}

// ----------------------------------------------------------- pointer

- (void)bfPointer:(NSEvent *)event type:(uint32_t)type button:(uint32_t)button
{
	NSPoint p;
	if (event.window == self.window)
		p = [self convertPoint:event.locationInWindow fromView:nil];
	else
		p = [self convertPoint:[self.window convertPointFromScreen:[NSEvent mouseLocation]] fromView:nil];
	double scale = _size.scale > 0 ? _size.scale : 1.0;

	bf_app_event e = {};
	e.type = type;
	e.mods = ModsFromFlags(event.modifierFlags);
	e.pointer.x_pt = (float)p.x;
	e.pointer.y_pt = (float)p.y;
	e.pointer.x_px = (float)(p.x * scale);
	e.pointer.y_px = (float)(p.y * scale);
	e.pointer.button = button;
	if (type == BF_APP_EVENT_WHEEL) {
		e.pointer.wheel_x = (float)event.deltaX;
		e.pointer.wheel_y = (float)event.deltaY;
		if (event.hasPreciseScrollingDeltas)
			e.flags |= BF_APP_EVF_PRECISE;
		e.flags |= BF_APP_EVF_DELTA_VALID;   // a wheel does not move the pointer
	} else if (type == BF_APP_EVENT_POINTER_MOVE) {
		e.pointer.dx_px = (float)(event.deltaX * scale);
		e.pointer.dy_px = (float)(event.deltaY * scale);
		e.flags |= BF_APP_EVF_DELTA_VALID;
	} else if (type == BF_APP_EVENT_BUTTON_DOWN || type == BF_APP_EVENT_BUTTON_UP) {
		e.flags |= BF_APP_EVF_DELTA_VALID;
		e.pointer.clicks = (uint32_t)event.clickCount;
		if (type == BF_APP_EVENT_BUTTON_DOWN)
			g_akButtons |= 1u << button;
		else
			g_akButtons &= ~(1u << button);
	} else {
		e.flags |= BF_APP_EVF_DELTA_VALID;   // enter and leave carry no movement
	}
	e.pointer.buttons = g_akButtons;
	g_lastPx = e.pointer.x_px; g_lastPy = e.pointer.y_px;
	g_lastPtX = e.pointer.x_pt; g_lastPtY = e.pointer.y_pt;
	bf_app_enqueue_appkit(&e);
}

- (void)bfMoved:(NSEvent *)event
{
	// The window (acceptsMouseMovedEvents) and the tracking area may both
	// hand the same event over.
	if (event == _lastMoved)
		return;
	_lastMoved = event;
	[self bfPointer:event type:BF_APP_EVENT_POINTER_MOVE button:0];
}

static uint32_t OtherButton(NSEvent *event)
{
	NSInteger n = event.buttonNumber;
	if (n == 2) return BF_APP_BUTTON_MIDDLE;
	if (n == 3) return BF_APP_BUTTON_X1;
	if (n == 4) return BF_APP_BUTTON_X2;
	return BF_APP_BUTTON_COUNT;
}

- (void)mouseMoved:(NSEvent *)e        { [self bfMoved:e]; }
- (void)mouseDragged:(NSEvent *)e      { [self bfMoved:e]; }
- (void)rightMouseDragged:(NSEvent *)e { [self bfMoved:e]; }
- (void)otherMouseDragged:(NSEvent *)e { [self bfMoved:e]; }
- (void)mouseDown:(NSEvent *)e         { [self bfPointer:e type:BF_APP_EVENT_BUTTON_DOWN button:BF_APP_BUTTON_LEFT]; }
- (void)mouseUp:(NSEvent *)e           { [self bfPointer:e type:BF_APP_EVENT_BUTTON_UP button:BF_APP_BUTTON_LEFT]; }
- (void)rightMouseDown:(NSEvent *)e    { [self bfPointer:e type:BF_APP_EVENT_BUTTON_DOWN button:BF_APP_BUTTON_RIGHT]; }
- (void)rightMouseUp:(NSEvent *)e      { [self bfPointer:e type:BF_APP_EVENT_BUTTON_UP button:BF_APP_BUTTON_RIGHT]; }
- (void)otherMouseDown:(NSEvent *)e
{
	uint32_t b = OtherButton(e);
	if (b < BF_APP_BUTTON_COUNT)
		[self bfPointer:e type:BF_APP_EVENT_BUTTON_DOWN button:b];
}
- (void)otherMouseUp:(NSEvent *)e
{
	uint32_t b = OtherButton(e);
	if (b < BF_APP_BUTTON_COUNT)
		[self bfPointer:e type:BF_APP_EVENT_BUTTON_UP button:b];
}
- (void)scrollWheel:(NSEvent *)e       { [self bfPointer:e type:BF_APP_EVENT_WHEEL button:0]; }
- (void)mouseEntered:(NSEvent *)e      { [self bfPointer:e type:BF_APP_EVENT_POINTER_ENTER button:0]; }
- (void)mouseExited:(NSEvent *)e       { [self bfPointer:e type:BF_APP_EVENT_POINTER_LEAVE button:0]; }

// ---------------------------------------------------------- keyboard

- (void)keyDown:(NSEvent *)event
{
	uint32_t mods = ModsFromFlags(event.modifierFlags);
	_currentKeySeq = PostKey(BF_APP_EVENT_KEY_DOWN, event.keyCode, mods, event.ARepeat);
	// Cmd combinations are commands, never text. Everything else goes
	// through the text system, which handles dead keys and input methods,
	// unless the text filter keeps it out (an Option shortcut).
	bool text = !(mods & BF_APP_MOD_CMD);
	if (text && g_textFilter) {
		bf_app_event e = {};
		e.type = BF_APP_EVENT_KEY_DOWN;
		e.source = BF_APP_SOURCE_APPKIT;
		e.mods = mods;
		e.seq = _currentKeySeq;
		e.key.mac_keycode = (uint16_t)event.keyCode;
		e.key.key = (uint16_t)bf_app_key_from_mac(event.keyCode);
		e.key.repeat = event.ARepeat ? 1 : 0;
		text = g_textFilter(&e, g_textFilterContext) != 0;
	}
	if (text)
		[self interpretKeyEvents:@[ event ]];
	_currentKeySeq = 0;
}

- (void)keyUp:(NSEvent *)event
{
	PostKey(BF_APP_EVENT_KEY_UP, event.keyCode, ModsFromFlags(event.modifierFlags), false);
}

- (void)flagsChanged:(NSEvent *)event
{
	unsigned code = event.keyCode;
	NSUInteger raw = event.modifierFlags;
	// Device-dependent bits tell left from right (IOLLEvent.h NX_DEVICE*KEYMASK).
	NSUInteger bit = 0, general = 0;
	switch (code) {
	case 0x38: bit = 0x0002; general = NSEventModifierFlagShift; break;
	case 0x3C: bit = 0x0004; general = NSEventModifierFlagShift; break;
	case 0x3B: bit = 0x0001; general = NSEventModifierFlagControl; break;
	case 0x3E: bit = 0x2000; general = NSEventModifierFlagControl; break;
	case 0x3A: bit = 0x0020; general = NSEventModifierFlagOption; break;
	case 0x3D: bit = 0x0040; general = NSEventModifierFlagOption; break;
	case 0x37: bit = 0x0008; general = NSEventModifierFlagCommand; break;
	case 0x36: bit = 0x0010; general = NSEventModifierFlagCommand; break;
	case 0x3F: general = NSEventModifierFlagFunction; break;
	case 0x39: {
		// Caps Lock reports its latch, not the key: a press and a release.
		uint32_t mods = ModsFromFlags(raw);
		PostKey(BF_APP_EVENT_KEY_DOWN, code, mods, false);
		PostKey(BF_APP_EVENT_KEY_UP, code, mods, false);
		return;
	}
	default:
		return;
	}
	bool down;
	if (bit && (raw & 0x207F))
		down = (raw & bit) != 0;
	else
		down = (raw & general) != 0;   // synthetic events carry no device bits
	if (code < 128 && g_akKeys[code] == down)
		return;
	if (code < 128)
		g_akKeys[code] = down;   // so that ModsFromFlags sees Fn
	PostKey(down ? BF_APP_EVENT_KEY_DOWN : BF_APP_EVENT_KEY_UP, code, ModsFromFlags(raw), false);
}

// ---------------------------------------------------- NSTextInputClient

- (void)insertText:(id)string replacementRange:(NSRange)range
{
	NSString *text = [string isKindOfClass:[NSAttributedString class]] ? [string string] : string;
	_marked = @"";
	char chunk[BF_APP_TEXT_MAX];
	size_t used = 0;
	auto flush = [&]() {
		if (!used)
			return;
		chunk[used] = 0;
		bf_app_event e = {};
		e.type = BF_APP_EVENT_TEXT;
		e.mods = ModsFromFlags([NSEvent modifierFlags]);
		memcpy(e.text.utf8, chunk, used + 1);
		e.text.key_seq = _currentKeySeq;
		bf_app_enqueue_appkit(&e);
		used = 0;
	};
	for (NSUInteger i = 0; i < text.length;) {
		NSRange r = [text rangeOfComposedCharacterSequenceAtIndex:i];
		i = NSMaxRange(r);
		unichar c = [text characterAtIndex:r.location];
		if (c < 0x20 || c == 0x7F || (c >= 0xF700 && c <= 0xF8FF))
			continue;   // control characters and AppKit's function-key code points
		const char *u = [text substringWithRange:r].UTF8String;
		size_t n = u ? strlen(u) : 0;
		if (n == 0 || n >= BF_APP_TEXT_MAX)
			continue;
		if (used + n >= BF_APP_TEXT_MAX)
			flush();
		memcpy(chunk + used, u, n);
		used += n;
	}
	flush();
}

- (void)doCommandBySelector:(SEL)selector
{
	// The key event is already in the queue; nothing to do, and no beep.
}

- (void)setMarkedText:(id)string selectedRange:(NSRange)sel replacementRange:(NSRange)range
{
	_marked = [string isKindOfClass:[NSAttributedString class]] ? [string string] : string;
}

- (void)unmarkText { _marked = @""; }
- (NSRange)selectedRange { return NSMakeRange(_marked.length, 0); }
- (NSRange)markedRange { return _marked.length ? NSMakeRange(0, _marked.length) : NSMakeRange(NSNotFound, 0); }
- (BOOL)hasMarkedText { return _marked.length > 0; }
- (NSArray<NSAttributedStringKey> *)validAttributesForMarkedText { return @[]; }
- (NSUInteger)characterIndexForPoint:(NSPoint)point { return NSNotFound; }

- (NSAttributedString *)attributedSubstringForProposedRange:(NSRange)range actualRange:(NSRangePointer)actual
{
	return nil;
}

- (NSRect)firstRectForCharacterRange:(NSRange)range actualRange:(NSRangePointer)actual
{
	// Input-method windows open at the bottom left of the content area.
	NSRect r = [self convertRect:NSMakeRect(0, self.bounds.size.height - 20, 0, 20) toView:nil];
	return self.window ? [self.window convertRectToScreen:r] : r;
}

@end

// ---------------------------------------------------------------------- C API

extern "C" void bf_app_set_text_filter(bf_app_text_filter filter, void *context)
{
	g_textFilter = filter;
	g_textFilterContext = context;
}

extern "C" int bf_app_init(const bf_app_config *config)
{
	if (g_initialised)
		return BF_APP_OK;
	if (![NSThread isMainThread]) {
		bf_tracef(BFT_ERR, "app: bf_app_init must be called on the main thread");
		return BF_APP_ERROR;
	}
	const char *instanceId = config && config->instance_id ? config->instance_id : "main";
	bool single = config ? config->single_instance != 0 : true;
	const char *multi = getenv("BF_MULTI_INSTANCE");
	if (multi && strcmp(multi, "1") == 0)
		single = false;

	[NSApplication sharedApplication];

	// Before the Dock icon appears: a second instance leaves without one.
	if (single && ClaimInstance(instanceId) == BF_APP_ALREADY_RUNNING)
		return BF_APP_ALREADY_RUNNING;

	[[NSUserDefaults standardUserDefaults] registerDefaults:@{ @"NSFullScreenMenuItemEverywhere": @NO,
	                                                          @"ApplePressAndHoldEnabled": @NO }];
	NSWindow.allowsAutomaticWindowTabbing = NO;
	[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
	g_delegate = [BFAppDelegate new];
	NSApp.delegate = g_delegate;

	NSString *name = @BF_APP_NAME;
	NSMenu *bar = [NSMenu new];
	NSMenu *appMenu = [[NSMenu alloc] initWithTitle:name];
	AddItem(appMenu, [@"Hide " stringByAppendingString:name], @selector(hide:), @"h", nil);
	NSMenuItem *others = AddItem(appMenu, @"Hide Others", @selector(hideOtherApplications:), @"h", nil);
	others.keyEquivalentModifierMask = NSEventModifierFlagCommand | NSEventModifierFlagOption;
	AddItem(appMenu, @"Show All", @selector(unhideAllApplications:), @"", nil);
	[appMenu addItem:[NSMenuItem separatorItem]];
	AddItem(appMenu, [@"Quit " stringByAppendingString:name], @selector(bfQuit:), @"q", g_delegate);
	NSMenu *viewMenu = [[NSMenu alloc] initWithTitle:@"View"];
	AddItem(viewMenu, @"Toggle Full Screen", @selector(bfToggleFullscreen:), @"f", g_delegate);
	NSMenu *windowMenu = [[NSMenu alloc] initWithTitle:@"Window"];
	AddItem(windowMenu, @"Minimize", @selector(performMiniaturize:), @"m", nil);
	for (NSMenu *m in @[ appMenu, viewMenu, windowMenu ]) {
		NSMenuItem *top = [bar addItemWithTitle:m.title action:nil keyEquivalent:@""];
		top.submenu = m;
	}
	NSApp.mainMenu = bar;
	NSApp.windowsMenu = windowMenu;

	[NSApp finishLaunching];

	// No App Nap, no timer coalescing; the system may still sleep when idle.
	g_activity = [[NSProcessInfo processInfo]
		beginActivityWithOptions:NSActivityUserInitiatedAllowingIdleSystemSleep | NSActivityLatencyCritical
		                  reason:@"game client keeps its connection and drawing going"];

	NSImage *blank = [[NSImage alloc] initWithSize:NSMakeSize(16, 16)];
	g_blankCursor = [[NSCursor alloc] initWithImage:blank hotSpot:NSZeroPoint];

	bf_app_wake_hook = WakeHook;
	[NSApp activate];
	g_initialised = true;
	BF_TRACE(BFT_INPUT, "app: initialised (instance '%s', single instance %s)", instanceId, single ? "on" : "off");
	return BF_APP_OK;
}

extern "C" void bf_app_shutdown(void)
{
	if (!g_initialised)
		return;
	bf_app_set_modal_tick(nullptr, nullptr);
	bf_app_window_destroy();
	bf_app_wake_hook = nullptr;
	if (g_activity) {
		[[NSProcessInfo processInfo] endActivity:g_activity];
		g_activity = nil;
	}
	ReleaseInstance();
	bf_app_queue_reset();
	memset(g_akKeys, 0, sizeof g_akKeys);
	g_akButtons = 0;
	g_initialised = false;
}

extern "C" int bf_app_pump(int timeout_ms)
{
	if (g_inPump || !g_initialised)
		return 0;
	g_inPump = true;
	int count = 0;
	NSDate *until = timeout_ms > 0 ? [NSDate dateWithTimeIntervalSinceNow:timeout_ms / 1000.0] : [NSDate distantPast];
	for (;;) {
		@autoreleasepool {
			bool wait = timeout_ms > 0 && count == 0;
			if (wait) {
				// Dekker with bf_app_post_event: either this sees its event,
				// or it sees g_pumpWaiting and posts a wake-up.
				g_pumpWaiting.store(true);
				if (bf_app_queue_size() > 0)
					wait = false;
			}
			NSEvent *event = [NSApp nextEventMatchingMask:NSEventMaskAny
			                                    untilDate:wait ? until : [NSDate distantPast]
			                                       inMode:NSDefaultRunLoopMode dequeue:YES];
			g_pumpWaiting.store(false);
			if (!event)
				break;
			if (event.type == NSEventTypeApplicationDefined && event.subtype == kWakeSubtype) {
				timeout_ms = 0;
				continue;
			}
			if (event.type == NSEventTypeKeyUp && (event.modifierFlags & NSEventModifierFlagCommand) &&
			    g_window && event.window == g_window)
				[g_window sendEvent:event];   // NSApplication swallows key-ups while Cmd is held
			else
				[NSApp sendEvent:event];
			++count;
		}
	}
	[NSApp updateWindows];
	g_inPump = false;
	return count;
}

extern "C" void bf_app_set_modal_tick(void (*tick)(void *), void *context)
{
	g_modalTick = tick;
	g_modalTickContext = context;
	if (tick && !g_modalTimer && g_delegate) {
		g_modalTimer = [NSTimer timerWithTimeInterval:1.0 / 60 target:g_delegate
		                                     selector:@selector(bfModalTick:) userInfo:nil repeats:YES];
		// Only the modes AppKit's own loops run in: never inside bf_app_pump.
		[[NSRunLoop mainRunLoop] addTimer:g_modalTimer forMode:NSEventTrackingRunLoopMode];
		[[NSRunLoop mainRunLoop] addTimer:g_modalTimer forMode:NSModalPanelRunLoopMode];
	} else if (!tick && g_modalTimer) {
		[g_modalTimer invalidate];
		g_modalTimer = nil;
	}
}

extern "C" int bf_app_activity_held(void)
{
	return g_activity != nil;
}

extern "C" int bf_app_window_create(double width_pt, double height_pt, const char *title)
{
	if (!g_initialised) {
		bf_tracef(BFT_ERR, "app: bf_app_window_create before bf_app_init");
		return BF_APP_ERROR;
	}
	if (g_window)
		return BF_APP_OK;
	NSRect rect = NSMakeRect(0, 0, width_pt, height_pt);
	NSWindow *w = [[NSWindow alloc] initWithContentRect:rect
	                                          styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
	                                                    NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable
	                                            backing:NSBackingStoreBuffered defer:NO];
	if (!w) {
		bf_tracef(BFT_ERR, "app: cannot create the window");
		return BF_APP_ERROR;
	}
	w.releasedWhenClosed = NO;
	w.collectionBehavior = NSWindowCollectionBehaviorFullScreenPrimary;
	w.title = [NSString stringWithUTF8String:title ? title : ""] ?: @"";
	w.delegate = g_delegate;
	w.acceptsMouseMovedEvents = YES;
	w.contentMinSize = NSMakeSize(320, 240);
	w.restorable = NO;
	BFAppView *view = [[BFAppView alloc] initWithFrame:rect];
	w.contentView = view;
	g_window = w;
	g_view = view;
	[view bfUpdateSize:NO];
	[view bfStartAnnouncing];
	[w center];
	/* BF_WINDOW_SCREEN=retina (the test harness sets it): open on the first screen with a
	   backing scale of 2 or more instead of the one with the key window, so captured
	   frames do not depend on where the owner's other windows are. */
	const char *want = getenv("BF_WINDOW_SCREEN");
	if (want && strcmp(want, "retina") == 0) {
		for (NSScreen *s in NSScreen.screens) {
			if (s.backingScaleFactor < 2.0)
				continue;
			if (s != w.screen) {
				NSRect vis = s.visibleFrame;
				NSRect f = w.frame;
				f.origin.x = NSMidX(vis) - f.size.width / 2;
				f.origin.y = NSMidY(vis) - f.size.height / 2;
				[w setFrame:f display:NO];
			}
			break;
		}
	}
	[w makeKeyAndOrderFront:nil];
	[w makeFirstResponder:view];
	[NSApp activate];
	bf_app_size s = view.bfSize;
	BF_TRACE(BFT_INPUT, "app: window %.0fx%.0f pt, %dx%d px, scale %.1f",
	         s.width_pt, s.height_pt, s.width_px, s.height_px, s.scale);
	return BF_APP_OK;
}

extern "C" void bf_app_window_destroy(void)
{
	if (!g_window)
		return;
	g_window.delegate = nil;
	[g_view bfStopAnnouncing];
	[g_window orderOut:nil];
	[g_window close];
	g_window = nil;
	g_view = nil;
}

extern "C" void *bf_app_window_nswindow(void)
{
	return (__bridge void *)g_window;
}

extern "C" void *bf_app_window_metal_layer(void)
{
	return g_view ? (__bridge void *)g_view.layer : NULL;
}

extern "C" void bf_app_window_get_size(bf_app_size *out)
{
	if (!out)
		return;
	if (g_view)
		*out = g_view.bfSize;
	else
		memset(out, 0, sizeof *out);
}

extern "C" void bf_app_window_set_content_size(double width_pt, double height_pt)
{
	if (!g_window)
		return;
	/* Keep the top-left corner where it is and the window on its screen: setContentSize
	   alone keeps the bottom edge, and AppKit then cuts the height at the menu bar, so a
	   window that sat low on the screen came out shorter than asked. */
	NSRect frame = [g_window frameRectForContentRect:NSMakeRect(0, 0, width_pt, height_pt)];
	NSRect cur = g_window.frame;
	frame.origin.x = cur.origin.x;
	frame.origin.y = NSMaxY(cur) - frame.size.height;
	NSScreen *screen = g_window.screen ?: [NSScreen mainScreen];
	if (screen) {
		NSRect vis = screen.visibleFrame;
		if (NSMaxX(frame) > NSMaxX(vis))
			frame.origin.x = NSMaxX(vis) - frame.size.width;
		if (frame.origin.x < vis.origin.x)
			frame.origin.x = vis.origin.x;
		if (frame.origin.y < vis.origin.y)
			frame.origin.y = vis.origin.y;
		if (NSMaxY(frame) > NSMaxY(vis))
			frame.origin.y = NSMaxY(vis) - frame.size.height;
	}
	[g_window setFrame:frame display:YES];
}

extern "C" void bf_app_window_set_min_content_size(double width_pt, double height_pt)
{
	if (g_window && width_pt > 0 && height_pt > 0)
		g_window.contentMinSize = NSMakeSize(width_pt, height_pt);
}

extern "C" void bf_app_window_get_min_content_size(double *width_pt, double *height_pt)
{
	NSSize s = g_window ? g_window.contentMinSize : NSZeroSize;
	if (width_pt) *width_pt = s.width;
	if (height_pt) *height_pt = s.height;
}

extern "C" int bf_app_window_get_screen(bf_app_screen *out)
{
	if (!out)
		return BF_APP_ERROR;
	memset(out, 0, sizeof *out);
	NSScreen *screen = (g_window && g_window.screen) ? g_window.screen : [NSScreen mainScreen];
	if (!screen)
		return BF_APP_ERROR;
	NSWindowStyleMask mask = g_window ? (g_window.styleMask & ~NSWindowStyleMaskFullScreen)
	                                  : (NSWindowStyleMaskTitled | NSWindowStyleMaskResizable);
	NSRect content = [NSWindow contentRectForFrameRect:screen.visibleFrame styleMask:mask];
	out->width_pt = screen.frame.size.width;
	out->height_pt = screen.frame.size.height;
	out->visible_width_pt = content.size.width;
	out->visible_height_pt = content.size.height;
	out->scale = screen.backingScaleFactor;
	return BF_APP_OK;
}

extern "C" void bf_app_window_set_title(const char *title)
{
	if (g_window)
		g_window.title = [NSString stringWithUTF8String:title ? title : ""] ?: @"";
}

extern "C" void bf_app_window_toggle_fullscreen(void)
{
	if (g_window)
		[g_window toggleFullScreen:nil];
}

extern "C" int bf_app_window_is_fullscreen(void)
{
	return g_window && (g_window.styleMask & NSWindowStyleMaskFullScreen) != 0;
}

extern "C" int bf_app_window_is_visible(void)
{
	return g_window && (g_window.occlusionState & NSWindowOcclusionStateVisible) != 0 && !g_window.miniaturized;
}

extern "C" int bf_app_window_is_focused(void)
{
	return g_window && g_window.keyWindow && NSApp.active;
}

extern "C" void bf_app_set_cursor_hidden_in_window(int hidden)
{
	g_cursorHidden = hidden != 0;
	if (!g_window || !g_view)
		return;
	NSPoint p = [g_view convertPoint:[g_window convertPointFromScreen:[NSEvent mouseLocation]] fromView:nil];
	if (NSPointInRect(p, g_view.bounds) && g_window.keyWindow)
		[g_cursorHidden ? g_blankCursor : [NSCursor arrowCursor] set];
}

extern "C" size_t bf_app_clipboard_get_text(char *buf, size_t size)
{
	@autoreleasepool {
		NSString *s = [[NSPasteboard generalPasteboard] stringForType:NSPasteboardTypeString];
		const char *u = s.UTF8String;
		size_t len = u ? strlen(u) : 0;
		if (buf && size > 0) {
			size_t n = len < size - 1 ? len : size - 1;
			while (n > 0 && n < len && ((unsigned char)u[n] & 0xC0) == 0x80)
				--n;   // do not cut a character in two
			if (n)
				memcpy(buf, u, n);
			buf[n] = 0;
		}
		return len;
	}
}

extern "C" int bf_app_clipboard_set_text(const char *utf8)
{
	@autoreleasepool {
		NSString *s = [NSString stringWithUTF8String:utf8 ? utf8 : ""];
		if (!s) {
			bf_tracef(BFT_ERR, "app: clipboard text is not UTF-8");
			return BF_APP_ERROR;
		}
		NSPasteboard *pb = [NSPasteboard generalPasteboard];
		[pb clearContents];
		if (![pb setString:s forType:NSPasteboardTypeString]) {
			bf_tracef(BFT_ERR, "app: cannot write the clipboard");
			return BF_APP_ERROR;
		}
		return BF_APP_OK;
	}
}
