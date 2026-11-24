// Shared between the queue (C++) and the AppKit side (Objective-C++).
#ifndef BF_APP_INTERNAL_H
#define BF_APP_INTERNAL_H

#include "bf_app.h"

// Appends an event from AppKit (main thread); returns its seq, 0 if dropped.
uint64_t bf_app_enqueue_appkit(const bf_app_event *event);

// Called by bf_app_post_event after queueing, from any thread, so that a
// pump waiting for AppKit events wakes up. Set by the AppKit side.
extern void (*bf_app_wake_hook)(void);

// The backing scale, so that injected pointer events get points from pixels.
void bf_app_queue_set_scale(double scale);

// Empties the queue and resets the state (bf_app_shutdown).
void bf_app_queue_reset(void);

#endif
