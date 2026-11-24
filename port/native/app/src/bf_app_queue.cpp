// The one event queue, and the key and pointer state kept from it.
//
// Producers: the AppKit side on the main thread, and bf_app_post_event from
// any thread. Consumer: bf_app_poll_event on the main thread, which also
// applies each event to the state, so the state always describes what the
// client has seen.

#include "bf_app_internal.h"
#include "bf_trace.h"

#include <atomic>
#include <deque>
#include <mutex>
#include <string.h>
#include <time.h>

void (*bf_app_wake_hook)(void) = nullptr;

namespace {

const size_t kQueueMax = 4096;

std::mutex g_lock;
std::deque<bf_app_event> g_queue;
uint64_t g_nextSeq = 1;
std::atomic<uint64_t> g_consumedSeq{0};
std::atomic<bf_app_clock_fn> g_clock{nullptr};
std::atomic<double> g_scale{1.0};

// State, main thread only.
bool g_keys[BF_KEY_COUNT];
bool g_macKeys[128];
uint32_t g_lastMods;
bf_app_pointer g_pointer;
float g_accDx, g_accDy, g_accWheel;

// Placeholder until the sys clock is installed with bf_app_set_clock.
uint64_t PlaceholderClock()
{
	static uint64_t start = clock_gettime_nsec_np(CLOCK_UPTIME_RAW);
	return (clock_gettime_nsec_np(CLOCK_UPTIME_RAW) - start) / 1000000;
}

uint64_t Enqueue(const bf_app_event *event, uint32_t source)
{
	bf_app_event e = *event;
	e.source = source;
	if (e.time_ms == 0)
		e.time_ms = bf_app_now_ms();
	std::lock_guard<std::mutex> guard(g_lock);
	if (g_queue.size() >= kQueueMax) {
		bf_tracef(BFT_ERR, "app: event queue full, event of type %u dropped", e.type);
		bf_count("app.queue_dropped", 1);
		return 0;
	}
	e.seq = g_nextSeq++;
	g_queue.push_back(e);
	return e.seq;
}

uint32_t ModsFromKeys()
{
	uint32_t m = 0;
	if (g_keys[BF_KEY_LSHIFT] || g_keys[BF_KEY_RSHIFT]) m |= BF_APP_MOD_SHIFT;
	if (g_keys[BF_KEY_LCTRL] || g_keys[BF_KEY_RCTRL]) m |= BF_APP_MOD_CTRL;
	if (g_keys[BF_KEY_LALT] || g_keys[BF_KEY_RALT]) m |= BF_APP_MOD_ALT;
	if (g_keys[BF_KEY_LCMD] || g_keys[BF_KEY_RCMD]) m |= BF_APP_MOD_CMD;
	if (g_keys[BF_KEY_FN]) m |= BF_APP_MOD_FN;
	return m;
}

bool IsPointerEvent(uint32_t type)
{
	return type == BF_APP_EVENT_POINTER_MOVE || type == BF_APP_EVENT_BUTTON_DOWN ||
	       type == BF_APP_EVENT_BUTTON_UP || type == BF_APP_EVENT_WHEEL ||
	       type == BF_APP_EVENT_POINTER_ENTER || type == BF_APP_EVENT_POINTER_LEAVE;
}

void Apply(bf_app_event *e)
{
	if (e->source == BF_APP_SOURCE_INJECTED &&
	    (e->type == BF_APP_EVENT_KEY_DOWN || e->type == BF_APP_EVENT_KEY_UP ||
	     e->type == BF_APP_EVENT_TEXT || IsPointerEvent(e->type)))
		e->mods |= ModsFromKeys();

	switch (e->type) {
	case BF_APP_EVENT_KEY_DOWN:
	case BF_APP_EVENT_KEY_UP: {
		bool down = e->type == BF_APP_EVENT_KEY_DOWN;
		if (e->source == BF_APP_SOURCE_INJECTED && e->key.key != BF_KEY_NONE) {
			int mac = bf_app_key_to_mac(e->key.key);
			e->key.mac_keycode = mac < 0 ? 0xffff : (uint16_t)mac;
		} else if (e->key.key == BF_KEY_NONE && e->key.mac_keycode < 128) {
			e->key.key = (uint16_t)bf_app_key_from_mac(e->key.mac_keycode);
		}
		if (e->key.key < BF_KEY_COUNT && e->key.key != BF_KEY_NONE)
			g_keys[e->key.key] = down;
		if (e->key.mac_keycode < 128)
			g_macKeys[e->key.mac_keycode] = down;
		break;
	}
	default:
		break;
	}

	if (IsPointerEvent(e->type)) {
		if (e->source == BF_APP_SOURCE_INJECTED) {
			double scale = g_scale.load();
			e->pointer.x_pt = (float)(e->pointer.x_px / scale);
			e->pointer.y_pt = (float)(e->pointer.y_px / scale);
		}
		if (!(e->flags & BF_APP_EVF_DELTA_VALID)) {
			e->pointer.dx_px = e->pointer.x_px - g_pointer.x_px;
			e->pointer.dy_px = e->pointer.y_px - g_pointer.y_px;
			e->flags |= BF_APP_EVF_DELTA_VALID;
		}
		g_accDx += e->pointer.dx_px;
		g_accDy += e->pointer.dy_px;
		g_pointer.x_px = e->pointer.x_px;
		g_pointer.y_px = e->pointer.y_px;
		g_pointer.x_pt = e->pointer.x_pt;
		g_pointer.y_pt = e->pointer.y_pt;
		uint32_t bit = e->pointer.button < 32 ? 1u << e->pointer.button : 0;
		if (e->type == BF_APP_EVENT_BUTTON_DOWN)
			g_pointer.buttons |= bit;
		else if (e->type == BF_APP_EVENT_BUTTON_UP)
			g_pointer.buttons &= ~bit;
		else if (e->type == BF_APP_EVENT_WHEEL)
			g_accWheel += e->pointer.wheel_y;
		else if (e->type == BF_APP_EVENT_POINTER_ENTER)
			g_pointer.inside = 1;
		else if (e->type == BF_APP_EVENT_POINTER_LEAVE)
			g_pointer.inside = 0;
		e->pointer.buttons = g_pointer.buttons;
	}

	if (e->source == BF_APP_SOURCE_APPKIT)
		g_lastMods = e->mods;
}

} // namespace

extern "C" void bf_app_set_clock(bf_app_clock_fn now_ms)
{
	g_clock.store(now_ms);
}

extern "C" uint64_t bf_app_now_ms(void)
{
	bf_app_clock_fn fn = g_clock.load();
	if (fn)
		return fn();
	static std::atomic<bool> said{false};
	if (!said.exchange(true))
		BF_TRACE(BFT_INPUT, "app: no clock installed (bf_app_set_clock), using a placeholder");
	return PlaceholderClock();
}

uint64_t bf_app_enqueue_appkit(const bf_app_event *event)
{
	return Enqueue(event, BF_APP_SOURCE_APPKIT);
}

void bf_app_queue_set_scale(double scale)
{
	if (scale > 0)
		g_scale.store(scale);
}

void bf_app_queue_reset(void)
{
	{
		std::lock_guard<std::mutex> guard(g_lock);
		g_queue.clear();
		g_consumedSeq.store(g_nextSeq - 1);   // dropped events count as taken
	}
	memset(g_keys, 0, sizeof g_keys);
	memset(g_macKeys, 0, sizeof g_macKeys);
	memset(&g_pointer, 0, sizeof g_pointer);
	g_lastMods = 0;
	g_accDx = g_accDy = g_accWheel = 0;
}

extern "C" uint64_t bf_app_post_event_seq(const bf_app_event *event)
{
	if (!event)
		return 0;
	uint64_t seq = Enqueue(event, BF_APP_SOURCE_INJECTED);
	if (!seq)
		return 0;
	if (bf_app_wake_hook)
		bf_app_wake_hook();
	return seq;
}

extern "C" int bf_app_post_event(const bf_app_event *event)
{
	return bf_app_post_event_seq(event) ? BF_APP_OK : BF_APP_ERROR;
}

extern "C" uint64_t bf_app_consumed_seq(void)
{
	return g_consumedSeq.load();
}

extern "C" int bf_app_poll_event(bf_app_event *out)
{
	{
		std::lock_guard<std::mutex> guard(g_lock);
		if (g_queue.empty())
			return 0;
		*out = g_queue.front();
		g_queue.pop_front();
		g_consumedSeq.store(out->seq);
	}
	Apply(out);
	return 1;
}

extern "C" int bf_app_queue_size(void)
{
	std::lock_guard<std::mutex> guard(g_lock);
	return (int)g_queue.size();
}

extern "C" int bf_app_key_is_down(int key)
{
	return key > 0 && key < BF_KEY_COUNT && g_keys[key];
}

extern "C" int bf_app_mac_key_is_down(unsigned mac_keycode)
{
	return mac_keycode < 128 && g_macKeys[mac_keycode];
}

extern "C" uint32_t bf_app_modifiers(void)
{
	return ModsFromKeys() | (g_lastMods & BF_APP_MOD_CAPSLOCK);
}

extern "C" void bf_app_pointer_get(bf_app_pointer *out)
{
	if (out)
		*out = g_pointer;
}

extern "C" void bf_app_pointer_take_delta(float *dx_px, float *dy_px, float *wheel_y)
{
	if (dx_px) *dx_px = g_accDx;
	if (dy_px) *dy_px = g_accDy;
	if (wheel_y) *wheel_y = g_accWheel;
	g_accDx = g_accDy = g_accWheel = 0;
}
