// The commands of PROTOCOL.md section 3 that the module carries out itself:
// hello, key, pointer, wait, capture, state, resize, quit.
//
// Injection (key, pointer): the events of one request are split into logic
// steps. The first step is posted when the request is carried out; each
// later step is posted from the bf_control_poll after the previous step has
// been taken out of the app queue (bf_app_consumed_seq), so that every step
// reaches the client in a frame of its own, and the reply is sent from the
// poll after the last step has been taken.

#include "bf_control_internal.h"
#include "bf_app.h"
#include "bf_trace.h"

#include <CommonCrypto/CommonDigest.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace {

using bfctl::Hooks;

const uint64_t kFrameTimeoutMs = 20000;   // capture and resize without a frame

// ------------------------------------------------------------ arguments

const bf_control_value *Arg(bf_control_request *r, const char *key)
{
	return bf_control_request_arg(r, key);
}

void BadRequest(bf_control_request *r, const std::string &message)
{
	bf_control_reply_error(r, BF_CONTROL_E_BAD_REQUEST, message.c_str());
}

// Optional integer argument; false (and a bad_request reply) when present but not an integer.
bool OptInt(bf_control_request *r, const char *key, bool *present, int64_t *out)
{
	const bf_control_value *v = Arg(r, key);
	*present = v != nullptr;
	if (!v) return true;
	if (!bf_control_value_int(v, out)) {
		BadRequest(r, std::string(key) + " must be an integer");
		return false;
	}
	return true;
}

// ------------------------------------------------------------------ keys

struct KeyName { const char *name; int key; };

const KeyName kKeyNames[] = {
	{"escape", BF_KEY_ESCAPE}, {"enter", BF_KEY_RETURN}, {"tab", BF_KEY_TAB}, {"space", BF_KEY_SPACE},
	{"backspace", BF_KEY_BACKSPACE}, {"delete", BF_KEY_DELETE}, {"insert", BF_KEY_INSERT},
	{"home", BF_KEY_HOME}, {"end", BF_KEY_END}, {"pageup", BF_KEY_PAGEUP}, {"pagedown", BF_KEY_PAGEDOWN},
	{"left", BF_KEY_LEFT}, {"right", BF_KEY_RIGHT}, {"up", BF_KEY_UP}, {"down", BF_KEY_DOWN},
	{"minus", BF_KEY_MINUS}, {"equal", BF_KEY_EQUAL}, {"leftbracket", BF_KEY_LEFTBRACKET},
	{"rightbracket", BF_KEY_RIGHTBRACKET}, {"backslash", BF_KEY_BACKSLASH}, {"semicolon", BF_KEY_SEMICOLON},
	{"quote", BF_KEY_QUOTE}, {"comma", BF_KEY_COMMA}, {"period", BF_KEY_PERIOD}, {"slash", BF_KEY_SLASH},
	{"grave", BF_KEY_GRAVE},
	{"kpenter", BF_KEY_KP_ENTER}, {"kpplus", BF_KEY_KP_PLUS}, {"kpminus", BF_KEY_KP_MINUS},
	{"kpmultiply", BF_KEY_KP_MULTIPLY}, {"kpdivide", BF_KEY_KP_DIVIDE}, {"kpdecimal", BF_KEY_KP_PERIOD},
	{"shift", BF_KEY_LSHIFT}, {"ctrl", BF_KEY_LCTRL}, {"alt", BF_KEY_LALT}, {"cmd", BF_KEY_LCMD},
};

// The bf_key of a PROTOCOL.md section 4 name, BF_KEY_NONE if none.
int KeyFromName(const std::string &n)
{
	if (n.size() == 1 && n[0] >= 'a' && n[0] <= 'z') return BF_KEY_A + (n[0] - 'a');
	if (n.size() == 1 && n[0] >= '1' && n[0] <= '9') return BF_KEY_1 + (n[0] - '1');
	if (n == "0") return BF_KEY_0;
	// f13, f14, f15: the Print Screen, Scroll Lock and Pause of a PC
	// keyboard on a Mac (the game's screenshot keys, port/client bf_input).
	if (n == "f13") return BF_KEY_F13;
	if (n == "f14") return BF_KEY_F14;
	if (n == "f15") return BF_KEY_F15;
	if (n.size() >= 2 && n.size() <= 3 && n[0] == 'f' && n[1] >= '1' && n[1] <= '9') {
		int k = n[1] - '0';
		if (n.size() == 3) {
			if (n[1] != '1' || n[2] < '0' || n[2] > '2') return BF_KEY_NONE;
			k = 10 + (n[2] - '0');
		}
		return BF_KEY_F1 + (k - 1);
	}
	if (n.size() == 3 && n[0] == 'k' && n[1] == 'p' && n[2] >= '0' && n[2] <= '9')
		return n[2] == '0' ? BF_KEY_KP_0 : BF_KEY_KP_1 + (n[2] - '1');
	for (const KeyName &k : kKeyNames)
		if (n == k.name) return k.key;
	return BF_KEY_NONE;
}

int ModKey(const std::string &m)
{
	if (m == "shift") return BF_KEY_LSHIFT;
	if (m == "ctrl") return BF_KEY_LCTRL;
	if (m == "alt") return BF_KEY_LALT;
	if (m == "cmd") return BF_KEY_LCMD;
	return BF_KEY_NONE;
}

// Characters Windows-1252 has in 0x80-0x9F.
const uint32_t kCp1252High[] = {
	0x20ac, 0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021, 0x02c6, 0x2030, 0x0160, 0x2039, 0x0152,
	0x017d, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014, 0x02dc, 0x2122, 0x0161, 0x203a,
	0x0153, 0x017e, 0x0178};

bool TypableCp1252(uint32_t cp)
{
	if (cp >= 0x20 && cp <= 0x7e) return true;
	if (cp >= 0xa0 && cp <= 0xff) return true;
	for (uint32_t c : kCp1252High)
		if (c == cp) return true;
	return false;
}

// Splits valid UTF-8 into characters.
std::vector<std::string> Characters(const std::string &s, std::vector<uint32_t> *cps)
{
	std::vector<std::string> out;
	size_t i = 0;
	while (i < s.size()) {
		unsigned char c = (unsigned char)s[i];
		size_t len = c < 0x80 ? 1 : c < 0xe0 ? 2 : c < 0xf0 ? 3 : 4;
		uint32_t cp = len == 1 ? c : len == 2 ? (c & 0x1f) : len == 3 ? (c & 0x0f) : (c & 0x07);
		for (size_t k = 1; k < len && i + k < s.size(); k++)
			cp = (cp << 6) | ((unsigned char)s[i + k] & 0x3f);
		out.push_back(s.substr(i, len));
		cps->push_back(cp);
		i += len;
	}
	return out;
}

// ------------------------------------------------------------- injection

typedef std::vector<bf_app_event> Step;

struct Injection {
	std::vector<Step> steps;
	size_t next = 0;
	uint64_t lastSeq = 0;
	bool pointerReply = false;
};

void FreeInjection(void *p) { delete (Injection *)p; }

bf_app_event KeyEvent(int key, bool down)
{
	bf_app_event e{};
	e.type = down ? BF_APP_EVENT_KEY_DOWN : BF_APP_EVENT_KEY_UP;
	e.key.key = (uint16_t)key;
	e.key.mac_keycode = 0xffff;
	return e;
}

bf_app_event PointerEvent(uint32_t type, float x, float y)
{
	bf_app_event e{};
	e.type = type;
	e.pointer.x_px = x;
	e.pointer.y_px = y;
	return e;
}

// Posts the next step. False when the queue refused an event.
bool PostStep(Injection *inj)
{
	const Step &step = inj->steps[inj->next++];
	for (const bf_app_event &e : step) {
		uint64_t seq = bf_app_post_event_seq(&e);
		if (!seq) return false;
		inj->lastSeq = seq;
	}
	return true;
}

void LogicalPointer(double *x, double *y)
{
	bf_app_pointer p;
	bf_app_pointer_get(&p);
	const bf_control_hooks &h = Hooks();
	if (h.px_to_logical) {
		h.px_to_logical(h.context, p.x_px, p.y_px, x, y);
	} else {
		*x = p.x_px;
		*y = p.y_px;
	}
}

void ReplyInjection(bf_control_request *r, Injection *inj)
{
	if (inj->pointerReply) {
		double x, y;
		LogicalPointer(&x, &y);
		bf_control_writer *w = bf_control_reply_writer(r);
		bf_control_write_key(w, "x");
		bf_control_write_int(w, (int64_t)lround(x));
		bf_control_write_key(w, "y");
		bf_control_write_int(w, (int64_t)lround(y));
	}
	bf_control_reply(r);
}

void CheckInjection(bf_control_request *r, void *context)
{
	Injection *inj = (Injection *)context;
	if (bfctl::InFrameDone()) return;   // one step per poll
	if (bf_app_consumed_seq() < inj->lastSeq) return;
	if (inj->next < inj->steps.size()) {
		// Also after a cancel, so that no key or button is left held.
		if (!PostStep(inj))
			bf_control_reply_error(r, BF_CONTROL_E_INTERNAL, "the app event queue is full");
		return;
	}
	ReplyInjection(r, inj);
}

void StartInjection(bf_control_request *r, Injection *inj)
{
	if (inj->steps.empty()) {
		ReplyInjection(r, inj);
		delete inj;
		return;
	}
	if (!PostStep(inj)) {
		bf_control_reply_error(r, BF_CONTROL_E_INTERNAL, "the app event queue is full");
		delete inj;
		return;
	}
	bfctl::DeferOwned(r, CheckInjection, inj, FreeInjection);
}

// ------------------------------------------------------------------- key

void CmdKey(bf_control_request *r, void *)
{
	const char *action = bf_control_value_string(Arg(r, "action"), nullptr);
	if (!action) { BadRequest(r, "action must be a string"); return; }
	std::string a = action;
	Injection *inj = new Injection;

	if (a == "text") {
		const bf_control_value *tv = Arg(r, "text");
		if (bf_control_value_type(tv) != BF_CONTROL_T_STRING) {
			delete inj;
			BadRequest(r, "text must be a string");
			return;
		}
		std::vector<uint32_t> cps;
		std::vector<std::string> chars = Characters(tv->s, &cps);
		for (size_t i = 0; i < chars.size(); i++) {
			if (!TypableCp1252(cps[i])) {
				delete inj;
				char msg[96];
				snprintf(msg, sizeof msg, "character U+%04X is not a printable Windows-1252 character", cps[i]);
				bf_control_reply_error(r, BF_CONTROL_E_BAD_TEXT, msg);
				return;
			}
			bf_app_event e{};
			e.type = BF_APP_EVENT_TEXT;
			memcpy(e.text.utf8, chars[i].data(), chars[i].size());
			inj->steps.push_back(Step{e});
		}
		StartInjection(r, inj);
		return;
	}

	if (a != "down" && a != "up" && a != "press") {
		delete inj;
		BadRequest(r, "action must be down, up, press or text");
		return;
	}
	const bf_control_value *kv = Arg(r, "key");
	if (bf_control_value_type(kv) != BF_CONTROL_T_STRING) {
		delete inj;
		BadRequest(r, "key must be a string");
		return;
	}
	int key = KeyFromName(kv->s);
	if (key == BF_KEY_NONE) {
		delete inj;
		std::string msg = "no key named " + kv->s;
		bf_control_reply_error(r, BF_CONTROL_E_BAD_KEY, msg.c_str());
		return;
	}
	std::vector<int> mods;   // modifier keys to press around the event
	const bf_control_value *mv = Arg(r, "mods");
	if (mv && mv->type != BF_CONTROL_T_NULL) {
		if (mv->type != BF_CONTROL_T_ARRAY) {
			delete inj;
			BadRequest(r, "mods must be a list");
			return;
		}
		for (const bf_control_value &m : mv->items) {
			int mk = m.type == BF_CONTROL_T_STRING ? ModKey(m.s) : BF_KEY_NONE;
			if (mk == BF_KEY_NONE) {
				delete inj;
				BadRequest(r, "mods are shift, ctrl, alt and cmd");
				return;
			}
			bool seen = false;
			for (int k : mods) seen |= k == mk;
			if (!seen && !bf_app_key_is_down(mk) && mk != key)
				mods.push_back(mk);
		}
	}
	Step first, second;
	for (int mk : mods) first.push_back(KeyEvent(mk, true));
	if (a == "down" || a == "press") first.push_back(KeyEvent(key, true));
	if (a == "up") first.push_back(KeyEvent(key, false));
	if (a == "press") second.push_back(KeyEvent(key, false));
	for (size_t i = mods.size(); i-- > 0;) second.push_back(KeyEvent(mods[i], false));
	inj->steps.push_back(first);
	if (!second.empty()) inj->steps.push_back(second);
	StartInjection(r, inj);
}

// --------------------------------------------------------------- pointer

void CmdPointer(bf_control_request *r, void *)
{
	const char *action = bf_control_value_string(Arg(r, "action"), nullptr);
	if (!action) { BadRequest(r, "action must be a string"); return; }
	std::string a = action;

	uint32_t button = BF_APP_BUTTON_LEFT;
	if (const bf_control_value *bv = Arg(r, "button")) {
		const char *b = bf_control_value_string(bv, nullptr);
		if (b && strcmp(b, "left") == 0) button = BF_APP_BUTTON_LEFT;
		else if (b && strcmp(b, "right") == 0) button = BF_APP_BUTTON_RIGHT;
		else if (b && strcmp(b, "middle") == 0) button = BF_APP_BUTTON_MIDDLE;
		else { BadRequest(r, "button must be left, right or middle"); return; }
	}
	const bf_control_value *xv = Arg(r, "x"), *yv = Arg(r, "y");
	int64_t x = 0, y = 0;
	bool hasXY = xv || yv;
	if (hasXY && !(bf_control_value_int(xv, &x) && bf_control_value_int(yv, &y))) {
		BadRequest(r, "x and y are integers, both or neither");
		return;
	}
	int64_t dy = 0;
	if (a == "wheel" && !bf_control_value_int(Arg(r, "dy"), &dy)) {
		BadRequest(r, "wheel needs an integer dy");
		return;
	}
	if (a == "move" && !hasXY) {
		BadRequest(r, "move needs x and y");
		return;
	}
	if (a != "move" && a != "down" && a != "up" && a != "click" && a != "doubleclick" && a != "wheel") {
		BadRequest(r, "action must be move, down, up, click, doubleclick or wheel");
		return;
	}

	bf_app_pointer cur;
	bf_app_pointer_get(&cur);
	float px = cur.x_px, py = cur.y_px;
	Injection *inj = new Injection;
	inj->pointerReply = true;
	if (hasXY) {
		const bf_control_hooks &h = Hooks();
		if (h.logical_to_px) {
			h.logical_to_px(h.context, (double)x, (double)y, &px, &py);
		} else {
			px = (float)x;
			py = (float)y;
		}
		inj->steps.push_back(Step{PointerEvent(BF_APP_EVENT_POINTER_MOVE, px, py)});
	}
	auto buttonEvent = [&](bool down, uint32_t clicks) {
		bf_app_event e = PointerEvent(down ? BF_APP_EVENT_BUTTON_DOWN : BF_APP_EVENT_BUTTON_UP, px, py);
		e.pointer.button = button;
		e.pointer.clicks = down ? clicks : 0;
		return e;
	};
	if (a == "down" || a == "up") {
		inj->steps.push_back(Step{buttonEvent(a == "down", 1)});
	} else if (a == "click" || a == "doubleclick") {
		int n = a == "click" ? 1 : 2;
		for (int i = 1; i <= n; i++) {
			inj->steps.push_back(Step{buttonEvent(true, (uint32_t)i)});
			inj->steps.push_back(Step{buttonEvent(false, 0)});
		}
	} else if (a == "wheel") {
		bf_app_event e = PointerEvent(BF_APP_EVENT_WHEEL, px, py);
		e.pointer.wheel_y = (float)dy;   // positive: away from the user, both here and in bf_app
		inj->steps.push_back(Step{e});
	}
	StartInjection(r, inj);
}

// ----------------------------------------------------------------- state

void CmdState(bf_control_request *r, void *)
{
	const bf_control_value *fv = Arg(r, "fields");
	std::vector<std::string> want;
	bool all = true;
	if (fv && fv->type != BF_CONTROL_T_NULL) {
		if (fv->type != BF_CONTROL_T_ARRAY) { BadRequest(r, "fields is a list of names"); return; }
		for (const bf_control_value &f : fv->items) {
			if (f.type != BF_CONTROL_T_STRING) { BadRequest(r, "fields is a list of names"); return; }
			want.push_back(f.s);
		}
		all = false;
	}
	bf_control_writer *w = bf_control_reply_writer(r);
	for (const std::string &part : bfctl::StateParts()) {
		bool take = all;
		for (const std::string &f : want) take |= f == part;
		if (!take) continue;
		w->Key(part.data(), part.size());
		w->Raw(bfctl::StateJson(part.c_str()));
	}
	bf_control_reply(r);
}

// ------------------------------------------------------------------ wait

struct Wait {
	bf_control_value until;
	bool hasMode = false, hasMap = false, hasWindow = false, hasPos = false, hasMoving = false;
	bool hasFrames = false, hasEvent = false, hasContains = false;
	std::string mode, map, window, event, contains;
	bool open = true, moving = false;
	int64_t posX = 0, posY = 0, frames = 0;
	uint64_t frame0 = 0, after = 0, deadlineMs = 0;
	const bf_control_value *match = nullptr;   // into until
};

void FreeWait(void *p) { delete (Wait *)p; }

bf_control_value StateValue(const char *part)
{
	bf_control_value v;
	std::string json = bfctl::StateJson(part);
	bfctl::ParseJson(json.data(), json.size(), &v, nullptr);
	return v;
}

// The matching event (or a placeholder for conditions without one) when met; NULL otherwise.
bool Evaluate(const Wait &c, const bfctl::EventRecord **event)
{
	*event = nullptr;
	if (c.hasMode) {
		bf_control_value v = StateValue("mode");
		if (v.type != BF_CONTROL_T_STRING || v.s != c.mode) return false;
	}
	if (c.hasMap) {
		bf_control_value v = StateValue("map");
		const char *name = bf_control_value_string(bf_control_value_get(&v, "name"), nullptr);
		if (!name || c.map != name) return false;
	}
	if (c.hasWindow) {
		bf_control_value v = StateValue("windows");
		bool found = false;
		for (const bf_control_value &w : v.items) {
			const char *name = bf_control_value_string(bf_control_value_get(&w, "name"), nullptr);
			if (name && c.window == name) found = true;
		}
		if (found != c.open) return false;
	}
	if (c.hasPos || c.hasMoving) {
		bf_control_value v = StateValue("self");
		if (v.type != BF_CONTROL_T_OBJECT) return false;
		if (c.hasPos) {
			int64_t x, y;
			if (!bf_control_value_int(bf_control_value_get(&v, "x"), &x) ||
			    !bf_control_value_int(bf_control_value_get(&v, "y"), &y) || x != c.posX || y != c.posY)
				return false;
		}
		if (c.hasMoving) {
			int m;
			if (!bf_control_value_bool(bf_control_value_get(&v, "moving"), &m) || (m != 0) != c.moving)
				return false;
		}
	}
	if (c.hasFrames && bf_control_frame() < c.frame0 + (uint64_t)c.frames)
		return false;
	if (c.hasEvent) {
		for (const bfctl::EventRecord &e : bfctl::History()) {
			if (e.seq <= c.after || e.name != c.event) continue;
			if (c.hasContains) {
				const char *text = bf_control_value_string(bf_control_value_get(&e.message, "text"), nullptr);
				if (!text || !strstr(text, c.contains.c_str())) continue;
			}
			bool ok = true;
			for (size_t i = 0; c.match && i < c.match->keys.size(); i++) {
				const bf_control_value *f = bf_control_value_get(&e.message, c.match->keys[i].c_str());
				if (!f || !bfctl::JsonEqual(*f, c.match->items[i])) { ok = false; break; }
			}
			if (ok) { *event = &e; return true; }
		}
		return false;
	}
	return true;
}

void WriteModeFrame(bf_control_writer *w)
{
	w->Key("mode", 4);
	w->Raw(bfctl::HasState("mode") ? bfctl::StateJson("mode") : "null");
	bf_control_write_key(w, "frame");
	bf_control_write_int(w, (int64_t)bf_control_frame());
}

// Replies when met or timed out; true if it replied.
bool TryWait(bf_control_request *r, Wait *c)
{
	if (bf_control_request_cancelled(r)) {
		bf_control_reply(r);   // discarded
		return true;
	}
	const bfctl::EventRecord *event;
	if (Evaluate(*c, &event)) {
		bf_control_writer *w = bf_control_reply_writer(r);
		bf_control_write_key(w, "met");
		bf_control_write_value(w, &c->until);
		if (event) {
			bf_control_write_key(w, "event");
			bf_control_write_value(w, &event->message);
		}
		WriteModeFrame(w);
		bf_control_reply(r);
		return true;
	}
	if (bfctl::NowMs() >= c->deadlineMs) {
		WriteModeFrame(bf_control_reply_writer(r));
		bf_control_reply_error(r, BF_CONTROL_E_TIMEOUT, "condition not met in time");
		return true;
	}
	return false;
}

void CheckWait(bf_control_request *r, void *context)
{
	TryWait(r, (Wait *)context);
}

void CmdWait(bf_control_request *r, void *)
{
	const bf_control_value *until = Arg(r, "until");
	if (bf_control_value_type(until) != BF_CONTROL_T_OBJECT || until->items.empty()) {
		BadRequest(r, "until must be a non-empty condition object");
		return;
	}
	bool present;
	int64_t timeout = 10000, after = 0;
	if (!OptInt(r, "timeout", &present, &timeout)) return;
	if (timeout < 0 || timeout > 600000) { BadRequest(r, "timeout is 0..600000 ms"); return; }
	if (!OptInt(r, "after", &present, &after)) return;
	if (after < 0) { BadRequest(r, "after must not be negative"); return; }
	if (!present) after = (int64_t)bfctl::LastReplySeq();

	Wait *c = new Wait;
	c->until = *until;
	const bf_control_value &u = c->until;
	std::string bad;
	const char *needs = nullptr;
	for (size_t i = 0; i < u.keys.size() && bad.empty(); i++) {
		const std::string &k = u.keys[i];
		const bf_control_value &v = u.items[i];
		if (k == "mode") {
			if (v.type != BF_CONTROL_T_STRING) bad = "mode must be a string";
			c->hasMode = true; c->mode = v.s;
			if (!bfctl::HasState("mode")) needs = "mode";
		} else if (k == "map") {
			if (v.type != BF_CONTROL_T_STRING) bad = "map must be a string";
			c->hasMap = true; c->map = v.s;
			if (!bfctl::HasState("map")) needs = "map";
		} else if (k == "window") {
			if (v.type != BF_CONTROL_T_STRING) bad = "window must be a string";
			c->hasWindow = true; c->window = v.s;
			if (!bfctl::HasState("windows")) needs = "windows";
		} else if (k == "open") {
			if (v.type != BF_CONTROL_T_BOOL) bad = "open must be true or false";
			if (!bf_control_value_get(&u, "window")) bad = "open needs window";
			c->open = v.b;
		} else if (k == "position") {
			if (!bf_control_value_int(bf_control_value_get(&v, "x"), &c->posX) ||
			    !bf_control_value_int(bf_control_value_get(&v, "y"), &c->posY))
				bad = "position must be {\"x\": int, \"y\": int}";
			c->hasPos = true;
			if (!bfctl::HasState("self")) needs = "self";
		} else if (k == "moving") {
			if (v.type != BF_CONTROL_T_BOOL) bad = "moving must be true or false";
			c->hasMoving = true; c->moving = v.b;
			if (!bfctl::HasState("self")) needs = "self";
		} else if (k == "frames") {
			if (!bf_control_value_int(&v, &c->frames) || c->frames < 0) bad = "frames must be a non-negative integer";
			c->hasFrames = true;
		} else if (k == "event") {
			if (v.type != BF_CONTROL_T_STRING) bad = "event must be a string";
			c->hasEvent = true; c->event = v.s;
		} else if (k == "contains") {
			if (v.type != BF_CONTROL_T_STRING) bad = "contains must be a string";
			if (!bf_control_value_get(&u, "event")) bad = "contains needs event";
			c->hasContains = true; c->contains = v.s;
		} else if (k == "match") {
			if (v.type != BF_CONTROL_T_OBJECT) bad = "match must be an object";
			if (!bf_control_value_get(&u, "event")) bad = "match needs event";
			c->match = &v;
		} else {
			bad = "unknown condition " + k;
		}
	}
	if (!bad.empty()) {
		delete c;
		BadRequest(r, bad);
		return;
	}
	if (needs) {
		delete c;
		std::string msg = std::string("the client provides no state part ") + needs;
		bf_control_reply_error(r, BF_CONTROL_E_UNSUPPORTED, msg.c_str());
		return;
	}
	c->frame0 = bf_control_frame();
	c->after = (uint64_t)after;
	c->deadlineMs = bfctl::NowMs() + (uint64_t)timeout;
	if (TryWait(r, c)) {
		delete c;
		return;
	}
	bfctl::DeferOwned(r, CheckWait, c, FreeWait);
}

// --------------------------------------------------------------- capture

struct Capture {
	std::string path;
	uint64_t frame0;
	uint64_t deadlineMs;
};

void FreeCapture(void *p) { delete (Capture *)p; }

bool Sha256File(const std::string &path, std::string *hex)
{
	FILE *f = fopen(path.c_str(), "rb");
	if (!f) return false;
	CC_SHA256_CTX ctx;
	CC_SHA256_Init(&ctx);
	unsigned char buf[65536];
	size_t n;
	while ((n = fread(buf, 1, sizeof buf, f)) > 0)
		CC_SHA256_Update(&ctx, buf, (CC_LONG)n);
	bool ok = !ferror(f);
	fclose(f);
	unsigned char md[CC_SHA256_DIGEST_LENGTH];
	CC_SHA256_Final(md, &ctx);
	static const char digits[] = "0123456789abcdef";
	hex->clear();
	for (unsigned char b : md) {
		*hex += digits[b >> 4];
		*hex += digits[b & 15];
	}
	return ok;
}

void CheckCapture(bf_control_request *r, void *context)
{
	Capture *c = (Capture *)context;
	if (bf_control_request_cancelled(r)) { bf_control_reply(r); return; }
	if (bfctl::InFrameDone() && bf_control_frame() > c->frame0) {
		const bf_control_hooks &h = Hooks();
		bf_control_capture_info info{};
		char message[256] = "";
		int code = h.capture ? h.capture(h.context, c->path.c_str(), &info, message, sizeof message)
		                     : BF_CONTROL_E_UNSUPPORTED;
		if (code != BF_CONTROL_E_NONE) {
			bf_control_reply_error(r, code, message[0] ? message : "capture failed");
			return;
		}
		std::string sha;
		if (!Sha256File(c->path, &sha)) {
			std::string msg = "cannot read back " + c->path;
			bf_control_reply_error(r, BF_CONTROL_E_IO, msg.c_str());
			return;
		}
		bf_control_writer *w = bf_control_reply_writer(r);
		bf_control_write_key(w, "path"); bf_control_write_string(w, c->path.c_str());
		bf_control_write_key(w, "width"); bf_control_write_int(w, info.width);
		bf_control_write_key(w, "height"); bf_control_write_int(w, info.height);
		bf_control_write_key(w, "scale");
		if (info.scale == floor(info.scale)) bf_control_write_int(w, (int64_t)info.scale);
		else bf_control_write_number(w, info.scale);
		bf_control_write_key(w, "logical");
		bf_control_write_begin_object(w);
		bf_control_write_key(w, "w"); bf_control_write_int(w, info.logical_w);
		bf_control_write_key(w, "h"); bf_control_write_int(w, info.logical_h);
		bf_control_write_end_object(w);
		bf_control_write_key(w, "frame"); bf_control_write_int(w, (int64_t)bf_control_frame());
		bf_control_write_key(w, "sha256"); bf_control_write_string(w, sha.c_str());
		bf_control_reply(r);
		return;
	}
	if (bfctl::NowMs() >= c->deadlineMs)
		bf_control_reply_error(r, BF_CONTROL_E_BAD_STATE, "no frame was completed");
}

void CmdCapture(bf_control_request *r, void *)
{
	const char *path = bf_control_value_string(Arg(r, "path"), nullptr);
	if (!path || path[0] != '/') { BadRequest(r, "path must be an absolute path"); return; }
	std::string p = path;
	std::string dir = p.substr(0, p.rfind('/'));
	if (dir.empty()) dir = "/";
	struct stat st;
	if (stat(dir.c_str(), &st) != 0 || !S_ISDIR(st.st_mode)) {
		std::string msg = "no directory " + dir;
		bf_control_reply_error(r, BF_CONTROL_E_IO, msg.c_str());
		return;
	}
	Capture *c = new Capture{p, bf_control_frame(), bfctl::NowMs() + kFrameTimeoutMs};
	bfctl::DeferOwned(r, CheckCapture, c, FreeCapture);
}

// ---------------------------------------------------------------- resize

struct Resize {
	uint64_t frame0;
	uint64_t deadlineMs;
};

void FreeResize(void *p) { delete (Resize *)p; }

void CheckResize(bf_control_request *r, void *context)
{
	Resize *c = (Resize *)context;
	if (bf_control_request_cancelled(r)) { bf_control_reply(r); return; }
	const bf_control_hooks &h = Hooks();
	// Asked once per frame, after it was drawn.
	bool busy = bfctl::InFrameDone() && h.resize_busy && h.resize_busy(h.context);
	if (bfctl::InFrameDone() && bf_control_frame() > c->frame0 && !busy) {
		bf_control_writer *w = bf_control_reply_writer(r);
		if (bfctl::HasState("view")) {
			w->Key("view", 4);
			w->Raw(bfctl::StateJson("view"));
		}
		bf_control_reply(r);
		return;
	}
	if (busy && bfctl::InFrameDone())
		c->frame0 = bf_control_frame();   // the frame after the change has settled
	if (bfctl::NowMs() >= c->deadlineMs)
		bf_control_reply_error(r, BF_CONTROL_E_BAD_STATE, "the resize did not complete");
}

void CmdResize(bf_control_request *r, void *)
{
	bf_control_resize_args a{};
	int64_t v;
	bool present;
	if (!OptInt(r, "width", &present, &v)) return;
	if (present) {
		if (v < 320 || v > 8192) { BadRequest(r, "width is 320..8192"); return; }
		a.has_width = 1; a.width = (int)v;
	}
	if (!OptInt(r, "height", &present, &v)) return;
	if (present) {
		if (v < 240 || v > 8192) { BadRequest(r, "height is 240..8192"); return; }
		a.has_height = 1; a.height = (int)v;
	}
	if (!OptInt(r, "scale", &present, &v)) return;
	if (present) {
		if (v < 1 || v > 3) { BadRequest(r, "scale is 1, 2 or 3"); return; }
		a.has_scale = 1; a.scale = (int)v;
	}
	if (const bf_control_value *fv = Arg(r, "fullscreen")) {
		int fs;
		if (!bf_control_value_bool(fv, &fs)) { BadRequest(r, "fullscreen must be true or false"); return; }
		a.has_fullscreen = 1; a.fullscreen = fs;
	}
	const bf_control_hooks &h = Hooks();
	char message[256] = "";
	int code = h.resize(h.context, &a, message, sizeof message);
	if (code != BF_CONTROL_E_NONE) {
		bf_control_reply_error(r, code, message[0] ? message : "resize refused");
		return;
	}
	Resize *c = new Resize{bf_control_frame(), bfctl::NowMs() + kFrameTimeoutMs};
	bfctl::DeferOwned(r, CheckResize, c, FreeResize);
}

// ------------------------------------------------------------ hello, quit

void WriteNames(bf_control_writer *w, const char *key, const std::vector<std::string> &names)
{
	bf_control_write_key(w, key);
	bf_control_write_begin_array(w);
	for (const std::string &n : names) bf_control_write_string(w, n.c_str());
	bf_control_write_end_array(w);
}

void CmdHello(bf_control_request *r, void *)
{
	bool present;
	int64_t protocol;
	if (!OptInt(r, "protocol", &present, &protocol)) return;
	const bf_control_value *nv = Arg(r, "name");
	if (nv && nv->type != BF_CONTROL_T_STRING) { BadRequest(r, "name must be a string"); return; }
	BF_TRACE(BFT_NET, "control: hello from %s (protocol %lld)", nv ? nv->s.c_str() : "?",
	         present ? (long long)protocol : -1LL);
	bf_control_writer *w = bf_control_reply_writer(r);
	bf_control_write_key(w, "protocol"); bf_control_write_int(w, BF_CONTROL_PROTOCOL);
	bf_control_write_key(w, "client"); bf_control_write_string(w, BF_APP_NAME);
	bf_control_write_key(w, "version"); bf_control_write_string(w, bfctl::Version().c_str());
	bf_control_write_key(w, "pid"); bf_control_write_int(w, (int64_t)getpid());
	WriteNames(w, "commands", bfctl::CommandNames());
	WriteNames(w, "events", bfctl::EventNames());
	bf_control_reply(r);
}

void CmdQuit(bf_control_request *r, void *)
{
	bf_control_reply(r);
	bfctl::StopRequests();
	const bf_control_hooks &h = Hooks();
	h.quit(h.context);
}

bool HaveCapture() { return Hooks().capture != nullptr; }
bool HaveResize() { return Hooks().resize != nullptr; }
bool HaveQuit() { return Hooks().quit != nullptr; }

} // namespace

namespace bfctl {

void RegisterBuiltins()
{
	RegisterBuiltin("hello", CmdHello, nullptr);
	RegisterBuiltin("key", CmdKey, nullptr);
	RegisterBuiltin("pointer", CmdPointer, nullptr);
	RegisterBuiltin("capture", CmdCapture, HaveCapture);
	RegisterBuiltin("state", CmdState, nullptr);
	RegisterBuiltin("wait", CmdWait, nullptr);
	RegisterBuiltin("resize", CmdResize, HaveResize);
	RegisterBuiltin("quit", CmdQuit, HaveQuit);
}

} // namespace bfctl
