// bf_prefs: shared between the module's own sources only.
#ifndef BF_PREFS_INTERNAL_H
#define BF_PREFS_INTERNAL_H

#ifdef __OBJC__
#import <AppKit/AppKit.h>
// An open panel for folders that refuses invalid ones; *keep holds its delegate.
NSOpenPanel *bf_prefs_internal_make_panel(const char *reason, id<NSOpenSavePanelDelegate> *keep);
#endif

// Tells the change callback, and refreshes the window if it is open.
void bf_prefs_internal_notify(const char *group, const char *name);

#endif
