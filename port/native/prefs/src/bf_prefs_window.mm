// bf_prefs: the settings window and its menu item.
//
// One plain window laid out with an NSGridView: labels right-aligned in the
// first column, controls in the second. Every control writes through
// bf_settings_shared() at once and saves; the change callback names the
// value. Nothing here runs a loop of its own: bf_app_pump drives the window,
// and the "Choose..." panel is a sheet on it.

#import <AppKit/AppKit.h>

#include "bf_prefs.h"
#include "bf_prefs_internal.h"
#include "bf_sys.h"
#include "bf_trace.h"

#include <string.h>

#include <initializer_list>

namespace {

bf_prefs_changed_fn g_callback = nullptr;
void *g_callbackContext = nullptr;
bool g_refreshing = false;   // controls being set from the settings: no writes

const NSInteger kMenuTag = 0x62667072;   // 'bfpr'

struct OptionRow {
	const char *option;
	NSString *title;
};
const OptionRow kOptions[] = {
	{ BF_OPT_SKIP_INTRO,          @"Skip the licence and intro screens" },
	{ BF_OPT_FREE_CAMERA,         @"Free camera: no limits on zoom and rotation" },
	{ BF_OPT_LAUNCH_ARGUMENTS,    @"Launch arguments go straight into the game" },
	{ BF_OPT_ATTACK_WITHOUT_CTRL, @"Attack without Ctrl" },
};

NSString *Ns(const char *utf8)
{
	return utf8 ? [NSString stringWithUTF8String:utf8] : nil;
}

void Save()
{
	bf_settings *s = bf_settings_shared();
	if (bf_settings_save(s) < 0)
		bf_tracef(BFT_ERR, "prefs: settings could not be saved to %s", bf_settings_path(s));
}

NSTextField *Label(NSString *text)
{
	NSTextField *label = [NSTextField labelWithString:text];
	label.alignment = NSTextAlignmentRight;
	return label;
}

NSTextField *Note(NSString *text)
{
	NSTextField *note = [NSTextField labelWithString:text];
	note.font = [NSFont systemFontOfSize:[NSFont smallSystemFontSize]];
	note.textColor = NSColor.secondaryLabelColor;
	return note;
}

}  // namespace

@interface BFPrefsController : NSObject <NSWindowDelegate, NSTextFieldDelegate>
@property(strong) NSWindow *window;
@property(strong) NSTextField *folderField;
@property(strong) NSSegmentedControl *scaleControl;
@property(strong) NSMutableArray<NSButton *> *optionBoxes;
@property(strong) NSSlider *musicSlider, *effectSlider;
@property(strong) NSTextField *musicValue, *effectValue;
@property(strong) NSTextField *accountField;
@property(strong) NSButton *forgetButton;
@property(strong) NSTextField *status;
@property(strong) id<NSOpenSavePanelDelegate> panelDelegate;
@end

static BFPrefsController *g_controller;

@implementation BFPrefsController

- (NSStackView *)row:(NSArray<NSView *> *)views
{
	NSStackView *stack = [NSStackView stackViewWithViews:views];
	stack.orientation = NSUserInterfaceLayoutOrientationHorizontal;
	stack.alignment = NSLayoutAttributeFirstBaseline;
	stack.spacing = 8;
	return stack;
}

- (NSSlider *)sliderWithId:(NSString *)ident value:(NSTextField *__strong *)valueLabel
{
	NSSlider *slider = [NSSlider sliderWithValue:100 minValue:0 maxValue:BF_PREFS_VOLUME_MAX
	                                      target:self action:@selector(volumeChanged:)];
	slider.identifier = ident;
	slider.continuous = YES;
	[slider.widthAnchor constraintEqualToConstant:220].active = YES;
	NSTextField *value = [NSTextField labelWithString:@"127"];
	value.identifier = [ident stringByAppendingString:@"Value"];
	value.font = [NSFont monospacedDigitSystemFontOfSize:[NSFont systemFontSize] weight:NSFontWeightRegular];
	[value.widthAnchor constraintEqualToConstant:34].active = YES;
	*valueLabel = value;
	return slider;
}

- (void)build
{
	NSWindow *w = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 480, 400)
	                                          styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable
	                                            backing:NSBackingStoreBuffered defer:NO];
	w.releasedWhenClosed = NO;
	w.title = @"Settings";
	w.identifier = @"bfPrefsWindow";
	w.delegate = self;
	w.tabbingMode = NSWindowTabbingModeDisallowed;
	w.collectionBehavior = NSWindowCollectionBehaviorFullScreenAuxiliary | NSWindowCollectionBehaviorMoveToActiveSpace;
	self.window = w;

	// Game folder.
	NSTextField *folder = [NSTextField labelWithString:@""];
	folder.identifier = @"gameFolder";
	folder.selectable = YES;
	folder.lineBreakMode = NSLineBreakByTruncatingMiddle;
	[folder.widthAnchor constraintEqualToConstant:300].active = YES;
	[folder setContentCompressionResistancePriority:NSLayoutPriorityDefaultLow
	                                 forOrientation:NSLayoutConstraintOrientationHorizontal];
	self.folderField = folder;
	NSButton *choose = [NSButton buttonWithTitle:@"Choose…" target:self action:@selector(chooseFolder:)];
	choose.identifier = @"chooseGameFolder";
	NSTextField *folderNote = Note(@"A new folder is used from the next start.");

	// UI scale.
	NSSegmentedControl *scale = [NSSegmentedControl segmentedControlWithLabels:@[ @"1×", @"2×", @"3×" ]
	                                                              trackingMode:NSSegmentSwitchTrackingSelectOne
	                                                                    target:self action:@selector(scaleChanged:)];
	scale.identifier = @"uiScale";
	for (NSInteger i = 0; i < 3; ++i)
		[scale setWidth:44 forSegment:i];
	self.scaleControl = scale;

	// Options.
	self.optionBoxes = [NSMutableArray array];
	for (const OptionRow &o : kOptions) {
		NSButton *box = [NSButton checkboxWithTitle:o.title target:self action:@selector(optionChanged:)];
		box.identifier = Ns(o.option);
		[self.optionBoxes addObject:box];
	}

	// Volume.
	NSTextField *mv = nil, *ev = nil;
	self.musicSlider = [self sliderWithId:@"musicVolume" value:&mv];
	self.effectSlider = [self sliderWithId:@"effectVolume" value:&ev];
	self.musicValue = mv;
	self.effectValue = ev;

	// Account.
	NSTextField *account = [NSTextField textFieldWithString:@""];
	account.identifier = @"accountName";
	account.placeholderString = @"none";
	account.target = self;
	account.action = @selector(accountChanged:);
	account.delegate = self;
	account.cell.sendsActionOnEndEditing = YES;
	[account.widthAnchor constraintEqualToConstant:200].active = YES;
	self.accountField = account;
	NSButton *forget = [NSButton buttonWithTitle:@"Forget Saved Password" target:self action:@selector(forgetPassword:)];
	forget.identifier = @"forgetPassword";
	self.forgetButton = forget;
	NSTextField *status = Note(@"");
	status.identifier = @"status";
	status.lineBreakMode = NSLineBreakByTruncatingTail;
	[status.widthAnchor constraintEqualToConstant:300].active = YES;
	self.status = status;

	NSView *empty = [NSGridCell emptyContentView];
	NSArray<NSArray<NSView *> *> *rows = @[
		@[ Label(@"Game folder:"), folder ],
		@[ empty, [self row:@[ choose, folderNote ]] ],
		@[ Label(@"UI scale:"), scale ],
		@[ Label(@"Options:"), self.optionBoxes[0] ],
		@[ empty, self.optionBoxes[1] ],
		@[ empty, self.optionBoxes[2] ],
		@[ empty, self.optionBoxes[3] ],
		@[ Label(@"Music volume:"), [self row:@[ self.musicSlider, mv ]] ],
		@[ Label(@"Effects volume:"), [self row:@[ self.effectSlider, ev ]] ],
		@[ Label(@"Account:"), account ],
		@[ empty, forget ],
		@[ empty, status ],
	];
	NSGridView *grid = [NSGridView gridViewWithViews:rows];
	grid.translatesAutoresizingMaskIntoConstraints = NO;
	grid.rowSpacing = 8;
	grid.columnSpacing = 8;
	grid.rowAlignment = NSGridRowAlignmentFirstBaseline;
	[grid columnAtIndex:0].xPlacement = NSGridCellPlacementTrailing;
	// Space between the sections.
	for (NSInteger r : { 2, 3, 7, 9 })
		[grid rowAtIndex:r].topPadding = 10;
	[grid rowAtIndex:11].topPadding = -2;

	NSView *content = w.contentView;
	[content addSubview:grid];
	[NSLayoutConstraint activateConstraints:@[
		[grid.leadingAnchor constraintEqualToAnchor:content.leadingAnchor constant:20],
		[grid.trailingAnchor constraintEqualToAnchor:content.trailingAnchor constant:-20],
		[grid.topAnchor constraintEqualToAnchor:content.topAnchor constant:20],
		[grid.bottomAnchor constraintEqualToAnchor:content.bottomAnchor constant:-16],
	]];
	[content layoutSubtreeIfNeeded];
	[w setContentSize:content.fittingSize];
	[w center];
}

- (void)refresh
{
	g_refreshing = true;
	bf_settings *s = bf_settings_shared();
	char buf[1024];
	NSString *path;
	if (bf_settings_get_string(s, BF_SETTINGS_PATHS, BF_SETTING_GAME_FOLDER, buf, sizeof buf, NULL) == 1 && buf[0])
		path = Ns(buf);
	else
		path = Ns(bf_file_location(BF_LOCATION_GAME));
	self.folderField.toolTip = path ?: @"";
	self.folderField.stringValue = path.stringByAbbreviatingWithTildeInPath ?: @"";
	self.scaleControl.selectedSegment = bf_prefs_ui_scale() - 1;
	for (NSButton *box in self.optionBoxes)
		box.state = bf_settings_option(s, box.identifier.UTF8String) ? NSControlStateValueOn : NSControlStateValueOff;
	int32_t music = bf_settings_int(s, BF_PREFS_CLIENT_OPTION, BF_PREFS_MUSIC_VOLUME, 100);
	int32_t effect = bf_settings_int(s, BF_PREFS_CLIENT_OPTION, BF_PREFS_EFFECT_VOLUME, 100);
	self.musicSlider.integerValue = music;
	self.effectSlider.integerValue = effect;
	self.musicValue.integerValue = self.musicSlider.integerValue;
	self.effectValue.integerValue = self.effectSlider.integerValue;
	if (bf_settings_account_name(s, buf, sizeof buf) == 1)
		self.accountField.stringValue = Ns(buf) ?: @"";
	else
		self.accountField.stringValue = @"";
	[self updateForget];
	g_refreshing = false;
}

- (void)updateForget
{
	self.forgetButton.enabled = self.accountField.stringValue.length > 0;
}

- (void)setStatusText:(NSString *)text
{
	self.status.stringValue = text ?: @"";
}

// ---- Actions ----

- (void)scaleChanged:(NSSegmentedControl *)sender
{
	if (g_refreshing || sender.selectedSegment < 0)
		return;
	bf_settings_set_int(bf_settings_shared(), BF_PREFS_DISPLAY, BF_PREFS_UI_SCALE, (int32_t)sender.selectedSegment + 1);
	Save();
	bf_prefs_internal_notify(BF_PREFS_DISPLAY, BF_PREFS_UI_SCALE);
}

- (void)optionChanged:(NSButton *)sender
{
	if (g_refreshing)
		return;
	const char *option = sender.identifier.UTF8String;
	bf_settings_set_option(bf_settings_shared(), option, sender.state == NSControlStateValueOn);
	Save();
	bf_prefs_internal_notify(BF_SETTINGS_OPTIONS, option);
}

- (void)volumeChanged:(NSSlider *)sender
{
	if (g_refreshing)
		return;
	bool music = sender == self.musicSlider;
	const char *name = music ? BF_PREFS_MUSIC_VOLUME : BF_PREFS_EFFECT_VOLUME;
	int32_t value = (int32_t)sender.integerValue;
	(music ? self.musicValue : self.effectValue).integerValue = value;
	bf_settings *s = bf_settings_shared();
	int32_t old = -1;
	bool changed = !bf_settings_get_int(s, BF_PREFS_CLIENT_OPTION, name, &old) || old != value;
	if (changed)
		bf_settings_set_int(s, BF_PREFS_CLIENT_OPTION, name, value);
	// Saved when the drag ends, not at every step of it.
	NSEvent *event = NSApp.currentEvent;
	if (!(event && event.type == NSEventTypeLeftMouseDragged))
		Save();
	if (changed)
		bf_prefs_internal_notify(BF_PREFS_CLIENT_OPTION, name);
}

- (void)accountChanged:(NSTextField *)sender
{
	if (g_refreshing)
		return;
	NSString *name = [sender.stringValue stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet];
	if (![name isEqualToString:sender.stringValue])
		sender.stringValue = name;
	bf_settings *s = bf_settings_shared();
	char old[1024] = "";
	bf_settings_account_name(s, old, sizeof old);
	[self updateForget];
	if (strcmp(old, name.UTF8String) == 0)
		return;
	bf_settings_set_account_name(s, name.UTF8String);
	Save();
	bf_prefs_internal_notify(BF_SETTINGS_ACCOUNT, BF_SETTING_ACCOUNT_NAME);
}

- (void)controlTextDidChange:(NSNotification *)note
{
	[self updateForget];
}

- (void)forgetPassword:(id)sender
{
	[self.window makeFirstResponder:nil];   // a pending edit of the name first
	NSString *name = self.accountField.stringValue;
	if (!name.length)
		return;
	int r = bf_sys_keychain_delete(name.UTF8String);
	if (r == 1) {
		[self setStatusText:[NSString stringWithFormat:@"The saved password of “%@” was removed.", name]];
		bf_prefs_internal_notify(BF_SETTINGS_ACCOUNT, BF_PREFS_SAVED_PASSWORD);
	} else if (r == 0) {
		[self setStatusText:[NSString stringWithFormat:@"No password is saved for “%@”.", name]];
	} else {
		[self setStatusText:@"The Keychain could not be changed; it may be locked."];
	}
}

- (void)chooseFolder:(id)sender
{
	id<NSOpenSavePanelDelegate> keep = nil;
	NSOpenPanel *panel = bf_prefs_internal_make_panel(NULL, &keep);
	self.panelDelegate = keep;
	NSString *current = self.folderField.toolTip;
	if (current.length)
		panel.directoryURL = [NSURL fileURLWithPath:current isDirectory:YES];
	[panel beginSheetModalForWindow:self.window completionHandler:^(NSModalResponse answer) {
		self.panelDelegate = nil;
		if (answer != NSModalResponseOK || !panel.URL)
			return;
		char why[1024];
		if (bf_prefs_set_game_folder(panel.URL.fileSystemRepresentation, why, sizeof why))
			[self setStatusText:@"The new game folder is used from the next start."];
		else
			[self setStatusText:Ns(why)];
	}];
}

- (void)showSettings:(id)sender
{
	bf_prefs_show_settings();
}

- (BOOL)windowShouldClose:(NSWindow *)sender
{
	[sender makeFirstResponder:nil];   // writes a pending edit
	return YES;
}

@end

// ---- C interface -----------------------------------------------------------

void bf_prefs_internal_notify(const char *group, const char *name)
{
	BF_TRACE(BFT_UI, "prefs: changed %s/%s", group, name);
	// A folder set from outside the window (or by its sheet) shows at once.
	if (g_controller && !g_refreshing && strcmp(group, BF_SETTINGS_PATHS) == 0)
		[g_controller refresh];
	if (g_callback)
		g_callback(g_callbackContext, group, name);
}

static BFPrefsController *Controller()
{
	if (!g_controller) {
		[NSApplication sharedApplication];
		g_controller = [BFPrefsController new];
		[g_controller build];
	}
	return g_controller;
}

extern "C" void bf_prefs_set_change_callback(bf_prefs_changed_fn fn, void *context)
{
	g_callback = fn;
	g_callbackContext = context;
}

extern "C" int bf_prefs_install_menu_item(void)
{
	NSMenu *bar = NSApp.mainMenu;
	if (!bar || bar.numberOfItems == 0 || !bar.itemArray[0].submenu) {
		bf_tracef(BFT_ERR, "prefs: no application menu to add Settings to (bf_app_init first)");
		return 0;
	}
	NSMenu *appMenu = bar.itemArray[0].submenu;
	if ([appMenu itemWithTag:kMenuTag])
		return 1;
	Controller();
	NSMenuItem *item = [[NSMenuItem alloc] initWithTitle:@"Settings…" action:@selector(showSettings:) keyEquivalent:@","];
	item.keyEquivalentModifierMask = NSEventModifierFlagCommand;
	item.target = g_controller;
	item.tag = kMenuTag;
	[appMenu insertItem:item atIndex:0];
	[appMenu insertItem:[NSMenuItem separatorItem] atIndex:1];
	return 1;
}

extern "C" void *bf_prefs_settings_window(void)
{
	BFPrefsController *c = Controller();
	[c refresh];
	return (__bridge void *)c.window;
}

extern "C" int bf_prefs_show_settings(void)
{
	if (!NSApp || NSApp.activationPolicy != NSApplicationActivationPolicyRegular) {
		bf_tracef(BFT_ERR, "prefs: settings window wanted before bf_app_init");
		return 0;
	}
	BFPrefsController *c = Controller();
	if (!c.window.visible) {
		[c refresh];
		[c setStatusText:@""];
	}
	[c.window makeKeyAndOrderFront:nil];
	[NSApp activate];
	return 1;
}

extern "C" void bf_prefs_close_settings(void)
{
	if (!g_controller || !g_controller.window.visible)
		return;
	[g_controller.window makeFirstResponder:nil];
	[g_controller.window orderOut:nil];
}

extern "C" int bf_prefs_settings_visible(void)
{
	return g_controller && g_controller.window.visible;
}
