// bifrost-control-test: the control module against PROTOCOL.md, with fake
// hooks and providers. The test is both sides: it plays the client's frame
// loop (drain the app queue, bf_control_poll, bf_control_frame_done) and the
// connection's other end, on the main thread with non-blocking sockets. The
// app queue is used without a window. One line per check; exit status 1 on
// a failure.

#include "bf_app.h"
#include "bf_clock.h"
#include "bf_control.h"
#include "bf_control_json.h"
#include "bf_trace.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>
#include <vector>

extern "C" int bf_control_test_header_c(void);
int bf_control_test_header_cxx14();

namespace {

int g_failures = 0;
int g_checks = 0;

void Check(const char *name, bool ok, const std::string &detail = std::string())
{
	g_checks++;
	if (ok) {
		printf("ok    %s\n", name);
	} else {
		g_failures++;
		printf("FAIL  %s%s%s\n", name, detail.empty() ? "" : ": ", detail.c_str());
	}
	fflush(stdout);
}

uint64_t NowMs() { return bf_clock_us() / 1000; }

// ------------------------------------------------------------ JSON helpers

struct Json {
	bf_control_value v;
	std::string text;
	bool ok = false;

	explicit Json(const std::string &s = "{}") : text(s)
	{
		ok = bfctl::ParseJson(s.data(), s.size(), &v, nullptr);
	}
	const bf_control_value *operator[](const char *k) const { return bf_control_value_get(&v, k); }
	bool Has(const char *k) const { return (*this)[k] != nullptr; }
	int64_t Int(const char *k, int64_t def = -999) const
	{
		int64_t i;
		return bf_control_value_int((*this)[k], &i) ? i : def;
	}
	std::string Str(const char *k) const
	{
		const char *s = bf_control_value_string((*this)[k], nullptr);
		return s ? s : "<none>";
	}
	bool IsReply() const { return Has("ok"); }
	bool Ok() const { int b = 0; return bf_control_value_bool((*this)["ok"], &b) && b; }
	std::string Code() const
	{
		const char *s = bf_control_value_string(bf_control_value_get((*this)["error"], "code"), nullptr);
		return s ? s : "<none>";
	}
	std::string IdText() const
	{
		std::string out;
		const bf_control_value *id = (*this)["id"];
		if (id) bfctl::AppendValue(out, *id); else out = "<absent>";
		return out;
	}
};

bool HasName(const bf_control_value *list, const char *name)
{
	for (size_t i = 0; i < bf_control_value_count(list); i++) {
		const char *s = bf_control_value_string(bf_control_value_at(list, i), nullptr);
		if (s && strcmp(s, name) == 0) return true;
	}
	return false;
}

// ------------------------------------------------------- the fake client

struct Got {
	bf_app_event e;
	int step;     // the frame in which the client took it
};

struct Sim {
	bool drain = true;
	bool frames = true;
	uint64_t frame = 0;
	int step = 0;
	std::vector<Got> got;

	void Tick()
	{
		step++;
		if (drain) {
			bf_app_event e;
			while (bf_app_poll_event(&e)) got.push_back(Got{e, step});
		}
		bf_control_poll();
		if (frames) bf_control_frame_done(++frame);
	}
	// Frames at about 1 ms each, so that the I/O thread gets to run.
	void Ticks(int n) { for (int i = 0; i < n; i++) { Tick(); usleep(1000); } }
	void Reset() { got.clear(); }
};

Sim g_sim;

struct Client {
	int fd = -1;
	std::string buf;
	std::vector<Json> events;
	std::vector<Json> replies;   // not yet taken
	bool eof = false;

	bool Connect(const std::string &path)
	{
		fd = socket(AF_UNIX, SOCK_STREAM, 0);
		sockaddr_un a{};
		a.sun_family = AF_UNIX;
		strncpy(a.sun_path, path.c_str(), sizeof a.sun_path - 1);
		if (connect(fd, (sockaddr *)&a, sizeof a) != 0) {
			close(fd);
			fd = -1;
			return false;
		}
		int one = 1;
		setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof one);
		fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
		return true;
	}
	void Close()
	{
		if (fd >= 0) close(fd);
		fd = -1;
	}
	void Send(const std::string &s)
	{
		size_t off = 0;
		while (off < s.size()) {
			ssize_t n = send(fd, s.data() + off, s.size() - off, 0);
			if (n > 0) { off += n; continue; }
			if (n < 0 && errno == EAGAIN) { g_sim.Tick(); usleep(500); continue; }
			return;
		}
	}
	void Read()
	{
		char tmp[65536];
		for (;;) {
			ssize_t n = recv(fd, tmp, sizeof tmp, 0);
			if (n > 0) { buf.append(tmp, n); continue; }
			if (n == 0) eof = true;
			break;
		}
		size_t nl;
		while ((nl = buf.find('\n')) != std::string::npos) {
			Json j(buf.substr(0, nl));
			buf.erase(0, nl + 1);
			if (j.IsReply()) replies.push_back(j); else events.push_back(j);
		}
	}
	// Ticks the fake client until a reply arrives (or the time is up).
	Json Reply(int timeoutMs = 3000)
	{
		uint64_t end = NowMs() + timeoutMs;
		for (;;) {
			Read();
			if (!replies.empty()) {
				Json j = replies.front();
				replies.erase(replies.begin());
				return j;
			}
			if (NowMs() > end || eof) return Json("{\"timeout\":true}");
			g_sim.Tick();
			usleep(300);
		}
	}
	Json Request(const std::string &line, int timeoutMs = 3000)
	{
		Send(line + "\n");
		return Reply(timeoutMs);
	}
	// Ticks n frames and says whether a reply came.
	bool ReplyWithin(int ticks)
	{
		for (int i = 0; i < ticks; i++) {
			g_sim.Tick();
			usleep(300);
			Read();
			if (!replies.empty()) return true;
		}
		usleep(2000);
		Read();
		return !replies.empty();
	}
};

// --------------------------------------------------- hooks and providers

struct Fake {
	int quitCalls = 0;
	int captureCalls = 0;
	int captureCode = BF_CONTROL_E_NONE;
	bf_control_resize_args lastResize{};
	int resizeCode = BF_CONTROL_E_NONE;
	int busyFrames = 0;
	double scale = 2;   // logical to pixels
	std::string mode = "login";
	bool inGame = false;
	int selfX = 150, selfY = 150;
	bool moving = false;
	int viewW = 1024, viewH = 768;
} g_fake;

void QuitHook(void *) { g_fake.quitCalls++; }

int CaptureHook(void *, const char *path, bf_control_capture_info *out, char *msg, size_t size)
{
	g_fake.captureCalls++;
	if (g_fake.captureCode != BF_CONTROL_E_NONE) {
		snprintf(msg, size, "fake failure");
		return g_fake.captureCode;
	}
	FILE *f = fopen(path, "wb");
	if (!f) return BF_CONTROL_E_IO;
	fwrite("abc", 1, 3, f);
	fclose(f);
	out->width = 640;
	out->height = 480;
	out->scale = 1;
	out->logical_w = 640;
	out->logical_h = 480;
	return BF_CONTROL_E_NONE;
}

int ResizeHook(void *, const bf_control_resize_args *a, char *msg, size_t size)
{
	g_fake.lastResize = *a;
	if (g_fake.resizeCode != BF_CONTROL_E_NONE) {
		snprintf(msg, size, "fake refusal");
		return g_fake.resizeCode;
	}
	if (a->has_width) g_fake.viewW = a->width;
	if (a->has_height) g_fake.viewH = a->height;
	return BF_CONTROL_E_NONE;
}

int ResizeBusy(void *)
{
	if (g_fake.busyFrames > 0) { g_fake.busyFrames--; return 1; }
	return 0;
}

void ToPx(void *, double x, double y, float *px, float *py)
{
	*px = (float)(x * g_fake.scale);
	*py = (float)(y * g_fake.scale);
}

void FromPx(void *, float px, float py, double *x, double *y)
{
	*x = px / g_fake.scale;
	*y = py / g_fake.scale;
}

void SetHooks(bool capture = true)
{
	bf_control_hooks h{};
	h.quit = QuitHook;
	h.capture = capture ? CaptureHook : nullptr;
	h.resize = ResizeHook;
	h.resize_busy = ResizeBusy;
	h.logical_to_px = ToPx;
	h.px_to_logical = FromPx;
	bf_control_set_hooks(&h);
}

void ModePart(bf_control_writer *w, void *) { bf_control_write_string(w, g_fake.mode.c_str()); }

void ViewPart(bf_control_writer *w, void *)
{
	bf_control_write_begin_object(w);
	bf_control_write_key(w, "width"); bf_control_write_int(w, g_fake.viewW);
	bf_control_write_key(w, "height"); bf_control_write_int(w, g_fake.viewH);
	bf_control_write_end_object(w);
}

void WindowsPart(bf_control_writer *w, void *)
{
	bf_control_write_begin_array(w);
	bf_control_write_begin_object(w);
	bf_control_write_key(w, "id"); bf_control_write_int(w, 3);
	bf_control_write_key(w, "name"); bf_control_write_string(w, "LOGINWND");
	bf_control_write_end_object(w);
	bf_control_write_end_array(w);
}

void SelfPart(bf_control_writer *w, void *)
{
	if (!g_fake.inGame) { bf_control_write_null(w); return; }
	bf_control_write_begin_object(w);
	bf_control_write_key(w, "x"); bf_control_write_int(w, g_fake.selfX);
	bf_control_write_key(w, "y"); bf_control_write_int(w, g_fake.selfY);
	bf_control_write_key(w, "moving"); bf_control_write_bool(w, g_fake.moving);
	bf_control_write_end_object(w);
}

void BrokenPart(bf_control_writer *w, void *)
{
	bf_control_write_int(w, 1);
	bf_control_write_int(w, 2);   // two values: the part becomes null
}

// ------------------------------------------------------- custom commands

bf_control_request *g_later = nullptr;

void CmdLater(bf_control_request *r, void *)
{
	g_later = r;
	bf_control_defer(r, nullptr, nullptr);
}

struct Countdown { int left; };
Countdown g_countdown;

void CheckCountdown(bf_control_request *r, void *ctx)
{
	Countdown *c = (Countdown *)ctx;
	if (--c->left <= 0) {
		bf_control_write_key(bf_control_reply_writer(r), "done");
		bf_control_write_bool(bf_control_reply_writer(r), 1);
		bf_control_reply(r);
	}
}

void CmdCountdown(bf_control_request *r, void *)
{
	int64_t n = 3;
	bf_control_value_int(bf_control_request_arg(r, "n"), &n);
	g_countdown.left = (int)n;
	bf_control_defer(r, CheckCountdown, &g_countdown);
}

void CmdSilent(bf_control_request *, void *) {}

void CmdMalformed(bf_control_request *r, void *)
{
	bf_control_write_int(bf_control_reply_writer(r), 5);   // a value without a key
	bf_control_reply(r);
}

void EmitChat(const char *channel, const char *text)
{
	bf_control_writer *w = bf_control_event_begin("chat");
	bf_control_write_key(w, "channel"); bf_control_write_string(w, channel);
	bf_control_write_key(w, "from"); bf_control_write_null(w);
	bf_control_write_key(w, "text"); bf_control_write_string(w, text);
	bf_control_event_end(w);
}

void CmdEmit(bf_control_request *r, void *)
{
	EmitChat("normal", "hello world");
	EmitChat("party", "second line");
	bf_control_reply(r);
}

void CmdEcho(bf_control_request *r, void *)
{
	bf_control_writer *w = bf_control_reply_writer(r);
	bf_control_write_key(w, "args");
	bf_control_write_value(w, bf_control_request_args(r));
	bf_control_reply(r);
}

uint64_t ClockMs() { return bf_clock_us() / 1000; }

std::string Hex(const std::string &s)
{
	std::string out;
	char b[4];
	for (unsigned char c : s) { snprintf(b, sizeof b, "%02x", c); out += b; }
	return out;
}

// ------------------------------------------------------------------ tests

void TestJson()
{
	struct Case { const char *text; bool ok; };
	const Case cases[] = {
		{"{\"a\":[1,2.5,-0,1e3,\"x\\u00e9\\ud83d\\ude00\",true,false,null],\"b\":{}}", true},
		{"  [ ]  ", true},
		{"\"\\\"\\\\\\/\\b\\f\\n\\r\\t\"", true},
		{"{\"a\":1,}", false}, {"[1,]", false}, {"[01]", false}, {"[1.]", false}, {"[-]", false},
		{"{\"a\" 1}", false}, {"tru", false}, {"\"\\ud800\"", false}, {"\"\\udc00\"", false},
		{"\"\x01\"", false}, {"\"\xff\"", false}, {"\"\xc0\xaf\"", false}, {"{} {}", false},
		{"[1e999]", false}, {"\"\\x\"", false}, {"", false}, {"{\"a\":1", false}, {"/*c*/{}", false},
	};
	int bad = 0;
	std::string which;
	for (const Case &c : cases) {
		bf_control_value v;
		bool ok = bfctl::ParseJson(c.text, strlen(c.text), &v, nullptr);
		if (ok != c.ok) { bad++; which += std::string(" [") + c.text + "]"; }
	}
	Check("json: accepts and refuses per RFC 8259", bad == 0, which);

	std::string deep(100, '[');
	deep += std::string(100, ']');
	bf_control_value v;
	Check("json: nesting limited", !bfctl::ParseJson(deep.data(), deep.size(), &v, nullptr));

	Json j("{\"i\":7,\"f\":2.5,\"e\":1e3,\"big\":123456789012345678901,\"s\":\"x\\u00e9\\ud83d\\ude00\",\"k\":1,\"k\":2}");
	int64_t i;
	Check("json: integers and numbers apart",
	      j.Int("i") == 7 && !bf_control_value_int(j["f"], &i) && !bf_control_value_int(j["e"], &i) &&
	      !bf_control_value_int(j["big"], &i) && bf_control_value_type(j["big"]) == BF_CONTROL_T_NUMBER);
	Check("json: \\u escapes and surrogate pairs to UTF-8", j.Str("s") == "x\xc3\xa9\xf0\x9f\x98\x80", Hex(j.Str("s")));
	Check("json: the last of duplicate keys wins", j.Int("k") == 2);

	bf_control_writer w;
	w.Reset(bf_control_writer::kValue);
	bf_control_write_begin_object(&w);
	bf_control_write_key(&w, "s");
	bf_control_write_string(&w, "a\"\\\n\x01\xff" "b");
	bf_control_write_key(&w, "n");
	bf_control_write_number(&w, 0.0 / 0.0);
	bf_control_write_key(&w, "d");
	bf_control_write_number(&w, 0.1);
	bf_control_write_end_object(&w);
	Check("json: writer escapes, U+FFFD for bad bytes, NaN as null",
	      w.Complete() && w.out == "{\"s\":\"a\\\"\\\\\\n\\u0001\xef\xbf\xbd" "b\",\"n\":null,\"d\":0.1}", w.out);

	bf_control_writer m;
	m.Reset(bf_control_writer::kValue);
	bf_control_write_begin_object(&m);
	bf_control_write_int(&m, 1);
	Check("json: writer refuses a value without a key", !m.Complete());
	bf_control_write_null(nullptr);   // NULL writer: nothing happens
}

void TestArgs()
{
	char a0[] = "client", a1[] = "--control", a2[] = "/tmp/x.sock", a3[] = "--control=/tmp/y.sock";
	char *v1[] = {a0, a1, a2, nullptr};
	char *v2[] = {a0, a3, nullptr};
	char *v3[] = {a0, nullptr};
	setenv("BF_CONTROL", "/tmp/env.sock", 1);
	const char *p1 = bf_control_path_from_args(3, v1);
	const char *p2 = bf_control_path_from_args(2, v2);
	const char *p3 = bf_control_path_from_args(1, v3);
	bool env = p3 && strcmp(p3, "/tmp/env.sock") == 0;   // before unsetenv frees it
	unsetenv("BF_CONTROL");
	const char *p4 = bf_control_path_from_args(1, v3);
	Check("args: --control wins over BF_CONTROL; --control=; none",
	      p1 && strcmp(p1, "/tmp/x.sock") == 0 && p2 && strcmp(p2, "/tmp/y.sock") == 0 && env && p4 == nullptr);
}

std::string g_path;

void TestOpen()
{
	std::string longPath = "/tmp/" + std::string(120, 'x') + ".sock";
	bf_control_config c{longPath.c_str(), "test-1"};
	Check("open: a path over 103 bytes is refused", bf_control_open(&c) == BF_CONTROL_ERROR && !bf_control_is_open());

	FILE *f = fopen(g_path.c_str(), "w");
	if (f) { fputs("stale", f); fclose(f); }
	bf_control_config ok{g_path.c_str(), "test-1"};
	int r = bf_control_open(&ok);
	struct stat st;
	Check("open: stale file replaced by the socket",
	      r == BF_CONTROL_OK && stat(g_path.c_str(), &st) == 0 && S_ISSOCK(st.st_mode));
	Check("open: a second open is refused", bf_control_open(&ok) == BF_CONTROL_ERROR && bf_control_is_open());
	g_sim.Ticks(3);   // the client runs without anyone connected
	Check("open: runs without a connection", !bf_control_is_connected());
}

void TestHello(Client &c)
{
	Json h = c.Request("{\"id\":1,\"cmd\":\"hello\",\"protocol\":0,\"name\":\"test\"}");
	Check("hello: ok, protocol 0, client name, version, pid",
	      h.Ok() && h.Int("id") == 1 && h.Int("protocol") == 0 && h.Str("client") == BF_APP_NAME &&
	      h.Str("version") == "test-1" && h.Int("pid") == getpid() && h.Int("seq") == 0, h.text);
	bool all = true;
	for (const char *n : {"hello", "key", "pointer", "capture", "state", "wait", "resize", "quit", "echo", "later"})
		all &= HasName(h["commands"], n);
	Check("hello: commands list built-ins and registered ones", all, h.text);
	Check("hello: events list bye, error and declared ones",
	      HasName(h["events"], "bye") && HasName(h["events"], "error") && HasName(h["events"], "chat") &&
	      HasName(h["events"], "mode"), h.text);
	Json s = c.Request("{\"id\":\"str-id\",\"cmd\":\"hello\"}");
	Check("hello: a string id is echoed", s.Ok() && s.Str("id") == "str-id", s.text);
	Json p = c.Request("{\"id\":2,\"cmd\":\"hello\",\"protocol\":\"zero\"}");
	Check("hello: a protocol that is not an integer is bad_request", p.Code() == "bad_request", p.text);
}

void TestFraming(Client &c)
{
	c.Send("{\"id\":10,\"cmd\":\"hel");
	bool early = c.ReplyWithin(10);
	c.Send("lo\"}\n");
	Json r = c.Reply();
	Check("framing: a line in two writes is one request", !early && r.Ok() && r.Int("id") == 10, r.text);

	c.Send("{\"id\":11,\"cmd\":\"hello\"}\n{\"id\":12,\"cmd\":\"hello\"}\n");
	Json a = c.Reply(), b = c.Reply();
	Check("framing: two requests in one write, replies in order", a.Int("id") == 11 && b.Int("id") == 12);

	std::string big(BF_CONTROL_LINE_MAX + 100, 'x');
	c.Send(big + "\n{\"id\":13,\"cmd\":\"hello\"}\n");
	Json o = c.Reply(5000), n = c.Reply();
	Check("framing: an oversized line is bad_request with id null",
	      !o.Ok() && o.Code() == "bad_request" && o.IdText() == "null", o.text.substr(0, 200));
	Check("framing: the line after an oversized one is served", n.Ok() && n.Int("id") == 13, n.text);

	std::string near = "{\"id\":14,\"cmd\":\"echo\",\"pad\":\"" + std::string(BF_CONTROL_LINE_MAX - 64, 'y') + "\"}";
	Json e = c.Request(near, 5000);
	Check("framing: a line just under 1 MiB is served", e.Ok() && e.Int("id") == 14);

	c.Send("\n   \n\r\n");
	Json bl = c.Request("{\"id\":15,\"cmd\":\"hello\"}");
	Check("framing: blank lines are ignored", bl.Int("id") == 15, bl.text);

	struct Bad { const char *line; const char *code; const char *id; };
	const Bad bads[] = {
		{"this is not json", "bad_request", "null"},
		{"[1,2]", "bad_request", "null"},
		{"{\"id\":99}", "bad_request", "99"},
		{"{\"id\":98,\"cmd\":7}", "bad_request", "98"},
		{"{\"id\":1.5,\"cmd\":\"hello\"}", "bad_request", "null"},
		{"{\"id\":97,\"cmd\":\"fly\"}", "unknown_command", "97"},
		{"{\"id\":96,\"cmd\":\"step\",\"n\":10}", "unsupported", "96"},
		{"{\"id\":95,\"cmd\":\"subscribe\"}", "unsupported", "95"},
		{"{\"id\":94,\"cmd\":\"hello\"", "bad_request", "null"},
		{"{\"id\":93,\"cmd\":\"\xff\"}", "bad_request", "null"},
	};
	for (const Bad &x : bads) {
		Json j = c.Request(x.line);
		std::string name = std::string("errors: ") + x.code + " for " + x.line;
		Check(name.c_str(), !j.Ok() && j.Code() == x.code && j.IdText() == x.id && j.Has("seq") &&
		      bf_control_value_type(bf_control_value_get(j["error"], "message")) == BF_CONTROL_T_STRING, j.text);
	}
	Json after = c.Request("{\"id\":16,\"cmd\":\"hello\"}");
	Check("errors: the connection survives malformed input", after.Ok() && after.Int("id") == 16);
}

const char *TypeName(uint32_t t)
{
	switch (t) {
	case BF_APP_EVENT_KEY_DOWN: return "kd";
	case BF_APP_EVENT_KEY_UP: return "ku";
	case BF_APP_EVENT_TEXT: return "tx";
	case BF_APP_EVENT_POINTER_MOVE: return "mv";
	case BF_APP_EVENT_BUTTON_DOWN: return "bd";
	case BF_APP_EVENT_BUTTON_UP: return "bu";
	case BF_APP_EVENT_WHEEL: return "wh";
	default: return "??";
	}
}

std::string Describe(const std::vector<Got> &got)
{
	std::string s;
	char b[96];
	for (const Got &g : got) {
		const bf_app_event &e = g.e;
		if (e.type == BF_APP_EVENT_KEY_DOWN || e.type == BF_APP_EVENT_KEY_UP)
			snprintf(b, sizeof b, "%s:%s@%d ", TypeName(e.type), bf_app_key_name(e.key.key), g.step);
		else if (e.type == BF_APP_EVENT_TEXT)
			snprintf(b, sizeof b, "tx:%s@%d ", e.text.utf8, g.step);
		else
			snprintf(b, sizeof b, "%s:%g,%g,b%u,c%u@%d ", TypeName(e.type), e.pointer.x_px, e.pointer.y_px,
			         e.pointer.button, e.pointer.clicks, g.step);
		s += b;
	}
	return s;
}

bool AllInjected(const std::vector<Got> &got)
{
	for (const Got &g : got)
		if (g.e.source != BF_APP_SOURCE_INJECTED) return false;
	return !got.empty();
}

bool DistinctSteps(const std::vector<Got> &got)
{
	for (size_t i = 1; i < got.size(); i++)
		if (got[i].step <= got[i - 1].step) return false;
	return true;
}

void TestKeys(Client &c)
{
	g_sim.Reset();
	Json r = c.Request("{\"id\":20,\"cmd\":\"key\",\"action\":\"press\",\"key\":\"a\"}");
	const std::vector<Got> &g = g_sim.got;
	Check("key press: down then up, injected, in consecutive frames",
	      r.Ok() && g.size() == 2 && g[0].e.type == BF_APP_EVENT_KEY_DOWN && g[0].e.key.key == BF_KEY_A &&
	      g[1].e.type == BF_APP_EVENT_KEY_UP && g[1].e.key.key == BF_KEY_A && AllInjected(g) &&
	      g[1].step == g[0].step + 1, Describe(g));

	// The reply waits until the client has taken the events.
	g_sim.Reset();
	g_sim.drain = false;
	c.Send("{\"id\":21,\"cmd\":\"key\",\"action\":\"press\",\"key\":\"enter\"}\n");
	bool early = c.ReplyWithin(15);
	int waiting = bf_app_queue_size();
	g_sim.drain = true;
	Json r2 = c.Reply();
	Check("key: no reply before the client consumed the events; one step at a time",
	      !early && waiting == 1 && r2.Ok() && g_sim.got.size() == 2 &&
	      g_sim.got[0].e.key.key == BF_KEY_RETURN, Describe(g_sim.got));

	g_sim.Reset();
	Json r3 = c.Request("{\"id\":22,\"cmd\":\"key\",\"action\":\"press\",\"key\":\"v\",\"mods\":[\"shift\",\"cmd\"]}");
	const std::vector<Got> &m = g_sim.got;
	bool order = m.size() == 6 && m[0].e.key.key == BF_KEY_LSHIFT && m[1].e.key.key == BF_KEY_LCMD &&
	             m[2].e.key.key == BF_KEY_V && m[2].e.type == BF_APP_EVENT_KEY_DOWN &&
	             m[3].e.key.key == BF_KEY_V && m[3].e.type == BF_APP_EVENT_KEY_UP &&
	             m[4].e.key.key == BF_KEY_LCMD && m[4].e.type == BF_APP_EVENT_KEY_UP &&
	             m[5].e.key.key == BF_KEY_LSHIFT && m[5].e.type == BF_APP_EVENT_KEY_UP &&
	             m[0].step == m[2].step && m[3].step == m[2].step + 1 && m[5].step == m[3].step;
	Check("key mods: held around the key, released after", r3.Ok() && order, Describe(m));
	Check("key mods: the key-down reads Shift+Cmd",
	      m.size() == 6 && (m[2].e.mods & (BF_APP_MOD_SHIFT | BF_APP_MOD_CMD)) == (BF_APP_MOD_SHIFT | BF_APP_MOD_CMD) &&
	      !bf_app_key_is_down(BF_KEY_LSHIFT) && !bf_app_key_is_down(BF_KEY_LCMD));

	g_sim.Reset();
	c.Request("{\"id\":23,\"cmd\":\"key\",\"action\":\"down\",\"key\":\"shift\"}");
	bool held = bf_app_key_is_down(BF_KEY_LSHIFT);
	c.Request("{\"id\":24,\"cmd\":\"key\",\"action\":\"press\",\"key\":\"b\",\"mods\":[\"shift\"]}");
	bool stillHeld = bf_app_key_is_down(BF_KEY_LSHIFT);
	c.Request("{\"id\":25,\"cmd\":\"key\",\"action\":\"up\",\"key\":\"shift\"}");
	Check("key down/up: state follows; mods already held are left alone",
	      held && stillHeld && !bf_app_key_is_down(BF_KEY_LSHIFT) && g_sim.got.size() == 4, Describe(g_sim.got));

	bool names = true;
	std::string failed;
	for (const char *k : {"z", "0", "9", "f1", "f10", "f12", "kp0", "kp9", "kpenter", "kpdecimal", "insert",
	                      "grave", "pagedown", "alt", "cmd"}) {
		Json j = c.Request(std::string("{\"id\":26,\"cmd\":\"key\",\"action\":\"press\",\"key\":\"") + k + "\"}");
		if (!j.Ok()) { names = false; failed += std::string(k) + " "; }
	}
	Check("key: names of PROTOCOL.md section 4 accepted", names, failed);

	struct Bad { const char *line; const char *code; };
	const Bad bads[] = {
		{"{\"id\":27,\"cmd\":\"key\",\"action\":\"press\",\"key\":\"hyper\"}", "bad_key"},
		{"{\"id\":27,\"cmd\":\"key\",\"action\":\"press\",\"key\":\"f13\"}", "bad_key"},
		{"{\"id\":27,\"cmd\":\"key\",\"action\":\"press\",\"key\":\"return\"}", "bad_key"},
		{"{\"id\":27,\"cmd\":\"key\",\"action\":\"press\"}", "bad_request"},
		{"{\"id\":27,\"cmd\":\"key\",\"action\":\"press\",\"key\":\"a\",\"mods\":[\"hyper\"]}", "bad_request"},
		{"{\"id\":27,\"cmd\":\"key\",\"action\":\"press\",\"key\":\"a\",\"mods\":\"shift\"}", "bad_request"},
		{"{\"id\":27,\"cmd\":\"key\",\"action\":\"hold\",\"key\":\"a\"}", "bad_request"},
		{"{\"id\":27,\"cmd\":\"key\"}", "bad_request"},
	};
	bool allBad = true;
	for (const Bad &b : bads) {
		Json j = c.Request(b.line);
		if (j.Code() != b.code) { allBad = false; failed += j.text + " "; }
	}
	Check("key: bad_key and bad_request", allBad, failed);
}

void TestText(Client &c)
{
	g_sim.Reset();
	Json r = c.Request("{\"id\":30,\"cmd\":\"key\",\"action\":\"text\",\"text\":\"h\\u00e9\\u20ac\"}");
	const std::vector<Got> &g = g_sim.got;
	Check("text: one TEXT event per character, one per frame",
	      r.Ok() && g.size() == 3 && g[0].e.type == BF_APP_EVENT_TEXT && strcmp(g[0].e.text.utf8, "h") == 0 &&
	      strcmp(g[1].e.text.utf8, "\xc3\xa9") == 0 && strcmp(g[2].e.text.utf8, "\xe2\x82\xac") == 0 &&
	      DistinctSteps(g) && AllInjected(g), Describe(g));
	Json k = c.Request("{\"id\":31,\"cmd\":\"key\",\"action\":\"text\",\"text\":\"\\uc548\\ub155\"}");
	Json n = c.Request("{\"id\":32,\"cmd\":\"key\",\"action\":\"text\",\"text\":\"a\\nb\"}");
	Json u = c.Request("{\"id\":33,\"cmd\":\"key\",\"action\":\"text\",\"text\":\"\\u0081\"}");
	Check("text: Korean, control characters, undefined 0x81 are bad_text",
	      k.Code() == "bad_text" && n.Code() == "bad_text" && u.Code() == "bad_text", k.text + n.text + u.text);
	g_sim.Reset();
	Json e = c.Request("{\"id\":34,\"cmd\":\"key\",\"action\":\"text\",\"text\":\"\"}");
	Check("text: empty text is ok and injects nothing", e.Ok() && g_sim.got.empty());
	Json t = c.Request("{\"id\":35,\"cmd\":\"key\",\"action\":\"text\",\"text\":5}");
	Check("text: text must be a string", t.Code() == "bad_request");
}

void TestPointer(Client &c)
{
	g_fake.scale = 2;
	g_sim.Reset();
	Json r = c.Request("{\"id\":40,\"cmd\":\"pointer\",\"action\":\"click\",\"x\":10,\"y\":20}");
	const std::vector<Got> &g = g_sim.got;
	Check("pointer click: move, down, up in consecutive frames, in pixels via the converter",
	      r.Ok() && g.size() == 3 && g[0].e.type == BF_APP_EVENT_POINTER_MOVE && g[0].e.pointer.x_px == 20 &&
	      g[0].e.pointer.y_px == 40 && g[1].e.type == BF_APP_EVENT_BUTTON_DOWN &&
	      g[1].e.pointer.button == BF_APP_BUTTON_LEFT && g[1].e.pointer.clicks == 1 && g[1].e.pointer.x_px == 20 &&
	      g[2].e.type == BF_APP_EVENT_BUTTON_UP && DistinctSteps(g) && AllInjected(g), Describe(g));
	Check("pointer click: reply has the logical position", r.Int("x") == 10 && r.Int("y") == 20, r.text);

	g_sim.Reset();
	Json d = c.Request("{\"id\":41,\"cmd\":\"pointer\",\"action\":\"down\",\"button\":\"right\"}");
	bf_app_pointer p;
	bf_app_pointer_get(&p);
	Check("pointer down without x, y: stays, right button held",
	      d.Ok() && d.Int("x") == 10 && d.Int("y") == 20 && g_sim.got.size() == 1 &&
	      g_sim.got[0].e.pointer.button == BF_APP_BUTTON_RIGHT && (p.buttons & (1u << BF_APP_BUTTON_RIGHT)),
	      d.text + Describe(g_sim.got));
	g_sim.Reset();
	Json u = c.Request("{\"id\":42,\"cmd\":\"pointer\",\"action\":\"up\",\"x\":15,\"y\":25,\"button\":\"right\"}");
	bf_app_pointer_get(&p);
	Check("pointer up with x, y: move then up",
	      u.Ok() && u.Int("x") == 15 && u.Int("y") == 25 && g_sim.got.size() == 2 &&
	      g_sim.got[0].e.pointer.x_px == 30 && g_sim.got[1].e.type == BF_APP_EVENT_BUTTON_UP && p.buttons == 0,
	      Describe(g_sim.got));

	g_sim.Reset();
	Json w = c.Request("{\"id\":43,\"cmd\":\"pointer\",\"action\":\"wheel\",\"dy\":-2}");
	Check("pointer wheel: dy notches, Windows sign kept",
	      w.Ok() && g_sim.got.size() == 1 && g_sim.got[0].e.type == BF_APP_EVENT_WHEEL &&
	      g_sim.got[0].e.pointer.wheel_y == -2, Describe(g_sim.got));

	g_sim.Reset();
	Json dc = c.Request("{\"id\":44,\"cmd\":\"pointer\",\"action\":\"doubleclick\",\"x\":5,\"y\":6}");
	const std::vector<Got> &h = g_sim.got;
	Check("pointer doubleclick: two clicks, the second with clicks 2",
	      dc.Ok() && h.size() == 5 && h[1].e.pointer.clicks == 1 && h[3].e.type == BF_APP_EVENT_BUTTON_DOWN &&
	      h[3].e.pointer.clicks == 2 && h[4].e.type == BF_APP_EVENT_BUTTON_UP && DistinctSteps(h), Describe(h));

	g_sim.Reset();
	Json mv = c.Request("{\"id\":45,\"cmd\":\"pointer\",\"action\":\"move\",\"x\":-30,\"y\":5000}");
	Check("pointer move: outside the window allowed",
	      mv.Ok() && mv.Int("x") == -30 && mv.Int("y") == 5000 && g_sim.got.size() == 1, mv.text);

	const char *bads[] = {
		"{\"id\":46,\"cmd\":\"pointer\",\"action\":\"click\",\"x\":1}",
		"{\"id\":46,\"cmd\":\"pointer\",\"action\":\"click\",\"x\":1.5,\"y\":2}",
		"{\"id\":46,\"cmd\":\"pointer\",\"action\":\"move\"}",
		"{\"id\":46,\"cmd\":\"pointer\",\"action\":\"wheel\"}",
		"{\"id\":46,\"cmd\":\"pointer\",\"action\":\"click\",\"button\":\"side\"}",
		"{\"id\":46,\"cmd\":\"pointer\",\"action\":\"jump\"}",
		"{\"id\":46,\"cmd\":\"pointer\"}",
	};
	bool all = true;
	std::string failed;
	for (const char *b : bads) {
		Json j = c.Request(b);
		if (j.Code() != "bad_request") { all = false; failed += j.text; }
	}
	Check("pointer: malformed arguments are bad_request", all, failed);
}

void TestDeferred(Client &c)
{
	g_later = nullptr;
	c.Send("{\"id\":50,\"cmd\":\"later\"}\n{\"id\":51,\"cmd\":\"hello\"}\n");
	bool early = c.ReplyWithin(10);
	Check("deferred: no reply while deferred, and the request behind waits", !early && g_later != nullptr);
	if (g_later) {
		bf_control_write_key(bf_control_reply_writer(g_later), "late");
		bf_control_write_bool(bf_control_reply_writer(g_later), 1);
		bf_control_reply(g_later);
	}
	Json a = c.Reply(), b = c.Reply();
	Check("deferred: reply from outside, then the next request, in order",
	      a.Int("id") == 50 && a.Ok() && a.Has("late") && b.Int("id") == 51, a.text + b.text);

	Json cd = c.Request("{\"id\":52,\"cmd\":\"countdown\",\"n\":5}");
	Check("deferred with a check: replies from the check", cd.Ok() && cd.Has("done") && g_countdown.left == 0, cd.text);

	Json s = c.Request("{\"id\":53,\"cmd\":\"silent\"}");
	Check("handler without a reply: internal", s.Code() == "internal", s.text);
	Json m = c.Request("{\"id\":54,\"cmd\":\"malformed\"}");
	Check("handler writing a malformed reply: internal", m.Code() == "internal", m.text);
	Json e = c.Request("{\"id\":55,\"cmd\":\"echo\",\"x\":[1,{\"y\":\"z\"}]}");
	const bf_control_value *args = e["args"];
	Check("custom command: arguments readable and echoable",
	      e.Ok() && bf_control_value_count(bf_control_value_get(args, "x")) == 2, e.text);
}

void TestEvents(Client &c)
{
	c.events.clear();
	Json before = c.Request("{\"id\":60,\"cmd\":\"hello\"}");
	int64_t base = before.Int("seq");
	Json r = c.Request("{\"id\":61,\"cmd\":\"emit\"}");
	Check("events: sent before the reply, seq numbered, reply seq = last event",
	      c.events.size() == 2 && c.events[0].Int("seq") == base + 1 && c.events[1].Int("seq") == base + 2 &&
	      r.Int("seq") == base + 2 && c.events[0].Str("event") == "chat" && c.events[0].Has("t") &&
	      !c.events[0].Has("id") && !c.events[0].Has("ok"), r.text);
	EmitChat("system", "outside a request");
	Json h = c.Request("{\"id\":62,\"cmd\":\"hello\"}");
	Check("events: emitted between requests counted in the next reply's seq",
	      c.events.size() == 3 && h.Int("seq") == base + 3, h.text);
	Check("events: t is the client clock in ms", c.events[2].Int("t") >= 0 &&
	      (uint64_t)c.events[2].Int("t") <= ClockMs());
}

void TestWait(Client &c)
{
	uint64_t f0 = g_sim.frame;
	Json r = c.Request("{\"id\":70,\"cmd\":\"wait\",\"until\":{\"frames\":3}}");
	Check("wait frames: met after 3 frames, with mode and frame",
	      r.Ok() && r.Int("frame") >= (int64_t)f0 + 3 && r.Str("mode") == "login" &&
	      bf_control_value_type(r["met"]) == BF_CONTROL_T_OBJECT, r.text);

	Json e = c.Request("{\"id\":71,\"cmd\":\"wait\",\"until\":{\"event\":\"chat\",\"contains\":\"world\"},\"after\":0}");
	Check("wait event: an earlier event after `after` matches at once",
	      e.Ok() && bf_control_value_get(e["event"], "text") &&
	      strcmp(bf_control_value_string(bf_control_value_get(e["event"], "text"), nullptr), "hello world") == 0, e.text);

	// Default `after`: the previous reply's seq, so old events do not count.
	c.Send("{\"id\":72,\"cmd\":\"wait\",\"until\":{\"event\":\"chat\",\"match\":{\"channel\":\"party\"}},\"timeout\":3000}\n");
	bool early = c.ReplyWithin(8);
	EmitChat("normal", "not this one");
	bool early2 = c.ReplyWithin(5);
	EmitChat("party", "this one");
	Json w = c.Reply();
	const char *text = bf_control_value_string(bf_control_value_get(w["event"], "text"), nullptr);
	Check("wait event: default after, match on fields, events before the reply",
	      !early && !early2 && w.Ok() && text && strcmp(text, "this one") == 0 &&
	      w.Int("seq") == c.events.back().Int("seq") && c.events.back().Str("text") == "this one", w.text);

	uint64_t t0 = ClockMs();
	Json t = c.Request("{\"id\":73,\"cmd\":\"wait\",\"until\":{\"mode\":\"game\"},\"timeout\":200}");
	uint64_t took = ClockMs() - t0;
	Check("wait timeout: error timeout with mode and frame, after the timeout",
	      t.Code() == "timeout" && t.Str("mode") == "login" && t.Has("frame") && took >= 200 && took < 1500,
	      t.text + " took " + std::to_string(took));

	Json m = c.Request("{\"id\":74,\"cmd\":\"wait\",\"until\":{\"mode\":\"login\"}}");
	Json win = c.Request("{\"id\":75,\"cmd\":\"wait\",\"until\":{\"window\":\"LOGINWND\",\"open\":true}}");
	Json closed = c.Request("{\"id\":76,\"cmd\":\"wait\",\"until\":{\"window\":\"MAKECHARWND\",\"open\":false}}");
	Check("wait mode, window open and closed: through the providers", m.Ok() && win.Ok() && closed.Ok(),
	      m.text + win.text + closed.text);

	g_fake.inGame = true;
	g_fake.moving = true;
	c.Send("{\"id\":77,\"cmd\":\"wait\",\"until\":{\"position\":{\"x\":150,\"y\":151},\"moving\":false}}\n");
	bool soon = c.ReplyWithin(5);
	g_fake.selfY = 151;
	bool partly = c.ReplyWithin(5);
	g_fake.moving = false;
	Json pos = c.Reply();
	Check("wait position and moving: both must hold", !soon && !partly && pos.Ok(), pos.text);
	g_fake.inGame = false;

	struct Bad { const char *line; const char *code; };
	const Bad bads[] = {
		{"{\"id\":78,\"cmd\":\"wait\",\"until\":{\"weather\":\"rain\"}}", "bad_request"},
		{"{\"id\":78,\"cmd\":\"wait\",\"until\":{}}", "bad_request"},
		{"{\"id\":78,\"cmd\":\"wait\"}", "bad_request"},
		{"{\"id\":78,\"cmd\":\"wait\",\"until\":{\"frames\":-1}}", "bad_request"},
		{"{\"id\":78,\"cmd\":\"wait\",\"until\":{\"frames\":1},\"timeout\":700000}", "bad_request"},
		{"{\"id\":78,\"cmd\":\"wait\",\"until\":{\"position\":{\"x\":1}}}", "bad_request"},
		{"{\"id\":78,\"cmd\":\"wait\",\"until\":{\"open\":true}}", "bad_request"},
		{"{\"id\":78,\"cmd\":\"wait\",\"until\":{\"mode\":3}}", "bad_request"},
		{"{\"id\":78,\"cmd\":\"wait\",\"until\":{\"map\":\"prontera\"}}", "unsupported"},
	};
	bool all = true;
	std::string failed;
	for (const Bad &b : bads) {
		Json j = c.Request(b.line);
		if (j.Code() != b.code) { all = false; failed += j.text; }
	}
	Check("wait: malformed conditions bad_request; a part with no provider unsupported", all, failed);

	Json z = c.Request("{\"id\":79,\"cmd\":\"wait\",\"until\":{\"mode\":\"game\"},\"timeout\":0}");
	Check("wait timeout 0: checked once", z.Code() == "timeout", z.text);
}

void TestCapture(Client &c, const std::string &dir)
{
	std::string path = dir + "/control-test-capture.png";
	unlink(path.c_str());
	g_sim.frames = false;
	c.Send("{\"id\":80,\"cmd\":\"capture\",\"path\":\"" + path + "\"}\n");
	bool early = c.ReplyWithin(8);
	uint64_t f = g_sim.frame;
	g_sim.frames = true;
	Json r = c.Reply();
	Check("capture: waits for the next completed frame", !early && g_fake.captureCalls == 1, r.text);
	Check("capture: path, size, scale, logical, frame, sha256 of the file",
	      r.Ok() && r.Str("path") == path && r.Int("width") == 640 && r.Int("height") == 480 && r.Int("scale") == 1 &&
	      bf_control_value_get(r["logical"], "w") && r.Int("frame") == (int64_t)f + 1 &&
	      r.Str("sha256") == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad", r.text);

	Json rel = c.Request("{\"id\":81,\"cmd\":\"capture\",\"path\":\"relative.png\"}");
	Json miss = c.Request("{\"id\":82,\"cmd\":\"capture\",\"path\":\"" + dir + "/no-such-dir/x.png\"}");
	g_fake.captureCode = BF_CONTROL_E_BAD_STATE;
	Json fail = c.Request("{\"id\":83,\"cmd\":\"capture\",\"path\":\"" + path + "\"}");
	g_fake.captureCode = BF_CONTROL_E_NONE;
	Check("capture: relative path bad_request, missing directory io, hook failure passed on",
	      rel.Code() == "bad_request" && miss.Code() == "io" && fail.Code() == "bad_state", rel.text + miss.text + fail.text);

	SetHooks(false);
	Json un = c.Request("{\"id\":84,\"cmd\":\"capture\",\"path\":\"" + path + "\"}");
	Json h = c.Request("{\"id\":85,\"cmd\":\"hello\"}");
	SetHooks(true);
	Check("capture without a hook: unsupported, and not in hello's commands",
	      un.Code() == "unsupported" && !HasName(h["commands"], "capture") && HasName(h["commands"], "key"), un.text);
	unlink(path.c_str());
}

void TestState(Client &c)
{
	Json all = c.Request("{\"id\":90,\"cmd\":\"state\"}");
	Check("state: every registered part, the built-in frame",
	      all.Ok() && all.Str("mode") == "login" && all.Has("view") && all.Has("windows") && all.Has("self") &&
	      all.Int("frame") >= (int64_t)g_sim.frame - 1 && all.Int("frame") <= (int64_t)g_sim.frame, all.text);
	Check("state: a provider writing two values gives null; unprovided parts absent",
	      all.Has("broken") && bf_control_value_type(all["broken"]) == BF_CONTROL_T_NULL && !all.Has("actors") &&
	      bf_control_value_type(all["self"]) == BF_CONTROL_T_NULL, all.text);
	Json only = c.Request("{\"id\":91,\"cmd\":\"state\",\"fields\":[\"mode\",\"inventory\"]}");
	Check("state fields: only those asked; reserved and unknown parts absent",
	      only.Ok() && bf_control_value_count(&only.v) == 4 && only.Str("mode") == "login", only.text);
	Json bad = c.Request("{\"id\":92,\"cmd\":\"state\",\"fields\":\"mode\"}");
	Json bad2 = c.Request("{\"id\":93,\"cmd\":\"state\",\"fields\":[1]}");
	Check("state: fields must be a list of names", bad.Code() == "bad_request" && bad2.Code() == "bad_request");
	Check("state: reserved part names refused at registration",
	      bf_control_register_state("inventory", ModePart, nullptr) == BF_CONTROL_ERROR &&
	      bf_control_register("pause", CmdEcho, nullptr) == BF_CONTROL_ERROR);
}

void TestResize(Client &c)
{
	g_fake.busyFrames = 0;
	uint64_t f = g_sim.frame;
	Json r = c.Request("{\"id\":100,\"cmd\":\"resize\",\"width\":1600,\"height\":1000,\"scale\":2}");
	const bf_control_resize_args &a = g_fake.lastResize;
	Check("resize: hook gets the arguments; reply after the next frame with view",
	      r.Ok() && a.has_width && a.width == 1600 && a.has_height && a.height == 1000 && a.has_scale && a.scale == 2 &&
	      !a.has_fullscreen && g_sim.frame > f && bf_control_value_get(r["view"], "width") &&
	      r.Int("seq") >= 0, r.text);
	Json v = Json(r.text);
	int64_t w = 0;
	bf_control_value_int(bf_control_value_get(v["view"], "width"), &w);
	Check("resize: view reflects the new size", w == 1600);

	g_fake.busyFrames = 6;
	f = g_sim.frame;
	Json fs = c.Request("{\"id\":101,\"cmd\":\"resize\",\"fullscreen\":true}");
	Check("resize: waits while resize_busy", fs.Ok() && g_sim.frame >= f + 6 && g_fake.lastResize.has_fullscreen &&
	      g_fake.lastResize.fullscreen == 1, fs.text);

	g_fake.resizeCode = BF_CONTROL_E_BAD_STATE;
	Json st = c.Request("{\"id\":102,\"cmd\":\"resize\",\"width\":800}");
	g_fake.resizeCode = BF_CONTROL_E_NONE;
	const char *bads[] = {
		"{\"id\":103,\"cmd\":\"resize\",\"scale\":4}",
		"{\"id\":103,\"cmd\":\"resize\",\"width\":100}",
		"{\"id\":103,\"cmd\":\"resize\",\"height\":9000}",
		"{\"id\":103,\"cmd\":\"resize\",\"fullscreen\":\"yes\"}",
		"{\"id\":103,\"cmd\":\"resize\",\"width\":\"wide\"}",
	};
	bool all = true;
	for (const char *b : bads) all &= c.Request(b).Code() == "bad_request";
	Check("resize: hook refusal passed on; out of range bad_request", st.Code() == "bad_state" && all, st.text);
}

void TestBusyAndReconnect(Client &c)
{
	Client second;
	bool connected = second.Connect(g_path);
	std::string line;
	bool closed = false;
	for (int i = 0; i < 200 && !closed; i++) {
		g_sim.Tick();
		usleep(1000);
		second.Read();
		closed = second.eof;
	}
	Check("busy: a second connection gets the busy event and is closed",
	      connected && second.events.size() == 1 && second.events[0].text == "{\"event\":\"error\",\"code\":\"busy\"}" && closed,
	      second.events.empty() ? "nothing" : second.events[0].text);
	second.Close();
	Json still = c.Request("{\"id\":110,\"cmd\":\"hello\"}");
	Check("busy: the first connection is unaffected", still.Ok() && still.Int("id") == 110);

	// Leave with a wait pending and a key press half injected.
	g_sim.Reset();
	g_sim.drain = false;
	c.Send("{\"id\":111,\"cmd\":\"key\",\"action\":\"press\",\"key\":\"q\"}\n");
	g_sim.Ticks(3);
	c.Close();
	g_sim.Ticks(3);
	g_sim.drain = true;
	g_sim.Ticks(10);
	bool released = g_sim.got.size() == 2 && g_sim.got[1].e.type == BF_APP_EVENT_KEY_UP && !bf_app_key_is_down(BF_KEY_Q);
	Check("disconnect: an injection still completes, no key left held", released, Describe(g_sim.got));

	Client again;
	bool ok = again.Connect(g_path);
	again.Send("{\"id\":112,\"cmd\":\"wait\",\"until\":{\"frames\":100000},\"timeout\":60000}\n");
	g_sim.Ticks(5);
	again.Close();
	g_sim.Ticks(5);

	Client third;
	ok = ok && third.Connect(g_path);
	Json h = third.Request("{\"id\":113,\"cmd\":\"hello\"}");
	Check("reconnect: the next connection is accepted; the old wait does not block it; seq from 0",
	      ok && h.Ok() && h.Int("id") == 113 && h.Int("seq") == 0, h.text);
	Json e = third.Request("{\"id\":114,\"cmd\":\"emit\"}");
	Check("reconnect: events numbered from 1 again", third.events.size() == 2 && third.events[0].Int("seq") == 1 &&
	      e.Int("seq") == 2, e.text);
	Json w = third.Request("{\"id\":115,\"cmd\":\"wait\",\"until\":{\"event\":\"chat\",\"contains\":\"world\"},\"after\":0,\"timeout\":0}");
	Check("reconnect: event history is per connection", w.Ok() && w.Int("seq") == 2, w.text);
	c = third;
}

void TestQuit(Client &c)
{
	c.Send("{\"id\":120,\"cmd\":\"quit\"}\n{\"id\":121,\"cmd\":\"hello\"}\n");
	Json r = c.Reply();
	bool more = c.ReplyWithin(10);
	Check("quit: reply, then the quit hook; later requests are not carried out",
	      r.Ok() && r.Int("id") == 120 && g_fake.quitCalls == 1 && !more, r.text);
	bf_control_close("quit");
	for (int i = 0; i < 100 && !c.eof; i++) { usleep(2000); c.Read(); }
	Check("quit: bye with reason quit, then the socket closes",
	      !c.events.empty() && c.events.back().Str("event") == "bye" && c.events.back().Str("reason") == "quit" && c.eof,
	      c.events.empty() ? "no events" : c.events.back().text);
	struct stat st;
	Check("close: the socket file is removed", stat(g_path.c_str(), &st) != 0 && !bf_control_is_open());
	bf_control_close("quit");   // a second close does nothing
	c.Close();

	// Open again; a close while connected says why.
	bf_control_config cfg{g_path.c_str(), "test-2"};
	bool reopened = bf_control_open(&cfg) == BF_CONTROL_OK;
	Client d;
	bool conn = reopened && d.Connect(g_path);
	Json h = d.Request("{\"id\":122,\"cmd\":\"hello\"}");
	bf_control_close("signal");
	for (int i = 0; i < 100 && !d.eof; i++) { usleep(2000); d.Read(); }
	Check("reopen: served again; close(signal) sends bye signal",
	      conn && h.Ok() && h.Str("version") == "test-2" && !d.events.empty() &&
	      d.events.back().Str("reason") == "signal" && d.eof, h.text);
	d.Close();
}

void TestConsumedSeq()
{
	bf_app_event e{};
	e.type = BF_APP_EVENT_USER;
	uint64_t s1 = bf_app_post_event_seq(&e);
	uint64_t s2 = bf_app_post_event_seq(&e);
	bool before = bf_app_consumed_seq() < s1;
	bf_app_event out;
	bf_app_poll_event(&out);
	bool mid = bf_app_consumed_seq() == s1;
	bf_app_poll_event(&out);
	Check("app: post_event_seq numbers events; consumed_seq follows polling",
	      s1 && s2 == s1 + 1 && before && mid && bf_app_consumed_seq() == s2);
}

} // namespace

int main(int argc, char **argv)
{
	setvbuf(stdout, nullptr, _IOLBF, 0);
	bf_app_set_clock(ClockMs);
	Check("header: compiles as C and as C++14", bf_control_test_header_c() == 1 && bf_control_test_header_cxx14() == 1);

	std::string dir = argv[0];
	size_t slash = dir.rfind('/');
	dir = slash == std::string::npos ? "." : dir.substr(0, slash);
	char real[PATH_MAX];
	if (realpath(dir.c_str(), real)) dir = real;
	g_path = dir + "/ctl-test-" + std::to_string(getpid()) + ".sock";
	if (g_path.size() > BF_CONTROL_PATH_MAX) {
		const char *tmp = getenv("TMPDIR");
		g_path = std::string(tmp ? tmp : "/tmp") + "/bf-ctl-test-" + std::to_string(getpid()) + ".sock";
	}
	printf("socket: %s\n", g_path.c_str());

	SetHooks(true);
	bf_control_register_state("mode", ModePart, nullptr);
	bf_control_register_state("view", ViewPart, nullptr);
	bf_control_register_state("windows", WindowsPart, nullptr);
	bf_control_register_state("self", SelfPart, nullptr);
	bf_control_register_state("broken", BrokenPart, nullptr);
	bf_control_register("echo", CmdEcho, nullptr);
	bf_control_register("later", CmdLater, nullptr);
	bf_control_register("countdown", CmdCountdown, nullptr);
	bf_control_register("silent", CmdSilent, nullptr);
	bf_control_register("malformed", CmdMalformed, nullptr);
	bf_control_register("emit", CmdEmit, nullptr);
	bf_control_declare_event("mode");
	bf_control_declare_event("chat");

	TestJson();
	TestArgs();
	TestConsumedSeq();
	TestOpen();
	if (!bf_control_is_open()) {
		printf("cannot continue without the socket\n");
		return 1;
	}
	Client c;
	bool connected = c.Connect(g_path);
	for (int i = 0; i < 500 && !bf_control_is_connected(); i++) g_sim.Ticks(1);
	Check("connect: accepted", connected && bf_control_is_connected());
	TestHello(c);
	TestFraming(c);
	TestKeys(c);
	TestText(c);
	TestPointer(c);
	TestDeferred(c);
	TestEvents(c);
	TestWait(c);
	TestCapture(c, dir);
	TestState(c);
	TestResize(c);
	TestBusyAndReconnect(c);
	TestQuit(c);

	printf("%d checks, %d failed\n", g_checks, g_failures);
	return g_failures ? 1 : 0;
}
