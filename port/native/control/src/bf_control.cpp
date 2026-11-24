// The control socket: the I/O thread, the connection, requests in order,
// replies and events with their seq, the registry and the state providers.
//
// The I/O thread owns the sockets. It reads lines into the inbox and writes
// the outbox; it never parses or carries out anything. The main thread takes
// the inbox in bf_control_poll, carries out requests one after another and
// appends replies and events to the outbox. A connection is known to both
// sides by a generation number, so that output meant for a connection that
// has meanwhile closed is dropped.

#include "bf_control_internal.h"
#include "bf_clock.h"
#include "bf_trace.h"

#include <algorithm>
#include <atomic>
#include <errno.h>
#include <fcntl.h>
#include <mutex>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <thread>
#include <unistd.h>

#ifndef BF_APP_NAME
#error "BF_APP_NAME must be defined by the build (port/name.cmake)"
#endif

namespace {

using bfctl::EventRecord;

const size_t kInboxThrottle = 16u << 20;   // stop reading while this much waits
const size_t kOutboxMax = 64u << 20;       // a reader that does not read is dropped
const size_t kHistoryMax = 4096;           // events kept per connection for `wait`

const char *const kReservedCommands[] = {"step", "pause", "resume", "render", "subscribe", "trace", "config", "act"};
const char *const kReservedParts[] = {"inventory", "equipment", "skills", "party", "guild", "status"};
const char *const kReservedEvents[] = {"trace", "frame", "packet"};

struct Item {
	enum Kind { kConnected, kLine, kOversized, kDisconnected } kind;
	uint64_t gen;
	std::string text;
};

// ---------------------------------------------------------------- I/O side

struct Io {
	std::mutex lock;
	std::deque<Item> inbox;
	std::string out;          // for connection curGen
	uint64_t curGen = 0;      // the I/O thread's connection, 0 when none
	bool overflow = false;
	std::atomic<size_t> queued{0};   // line bytes read and not yet carried out
	std::atomic<bool> stop{false};
	int wakeRead = -1, wakeWrite = -1, listenFd = -1;
	std::thread thread;
};

Io g_io;

void Wake()
{
	if (g_io.wakeWrite >= 0) {
		char c = 1;
		ssize_t n = write(g_io.wakeWrite, &c, 1);
		(void)n;
	}
}

void Push(Item::Kind kind, uint64_t gen, std::string text = std::string())
{
	std::lock_guard<std::mutex> guard(g_io.lock);
	g_io.queued += text.size();
	g_io.inbox.push_back(Item{kind, gen, std::move(text)});
}

bool Blank(const std::string &s)
{
	for (char c : s)
		if (c != ' ' && c != '\t' && c != '\r') return false;
	return true;
}

void SetNonBlocking(int fd, bool on)
{
	int fl = fcntl(fd, F_GETFL);
	fcntl(fd, F_SETFL, on ? (fl | O_NONBLOCK) : (fl & ~O_NONBLOCK));
}

void IoLoop()
{
	int conn = -1;
	uint64_t gen = 0, nextGen = 0;
	std::string partial;
	bool discarding = false;
	static char buf[65536];

	auto disconnect = [&](const char *why) {
		BF_TRACE(BFT_NET, "control: connection %llu closed (%s)", (unsigned long long)gen, why);
		close(conn);
		conn = -1;
		partial.clear();
		discarding = false;
		{
			std::lock_guard<std::mutex> guard(g_io.lock);
			g_io.curGen = 0;
			g_io.out.clear();
			g_io.overflow = false;
		}
		Push(Item::kDisconnected, gen);
	};

	for (;;) {
		bool wantOut;
		{
			std::lock_guard<std::mutex> guard(g_io.lock);
			wantOut = conn >= 0 && !g_io.out.empty();
		}
		bool throttled = g_io.queued.load() > kInboxThrottle;
		pollfd fds[3];
		int n = 0;
		fds[n++] = pollfd{g_io.wakeRead, POLLIN, 0};
		fds[n++] = pollfd{g_io.listenFd, POLLIN, 0};
		int connIndex = -1;
		if (conn >= 0) {
			short ev = (throttled ? 0 : POLLIN) | (wantOut ? POLLOUT : 0);
			connIndex = n;
			fds[n++] = pollfd{conn, ev, 0};
		}
		int r = poll(fds, n, throttled ? 20 : -1);
		if (r < 0 && errno != EINTR) {
			bf_tracef(BFT_ERR, "control: poll failed: %s", strerror(errno));
			usleep(10000);
		}
		if (fds[0].revents & POLLIN) {
			char drain[64];
			while (read(g_io.wakeRead, drain, sizeof drain) > 0) {}
		}
		if (g_io.stop.load())
			break;

		if (fds[1].revents & POLLIN) {
			int fd = accept(g_io.listenFd, nullptr, nullptr);
			if (fd >= 0) {
				int one = 1;
				setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof one);
				if (conn >= 0) {
					static const char busy[] = "{\"event\":\"error\",\"code\":\"busy\"}\n";
					ssize_t w = send(fd, busy, sizeof busy - 1, 0);
					(void)w;
					close(fd);
					BF_TRACE(BFT_NET, "control: second connection refused (busy)");
				} else {
					SetNonBlocking(fd, true);
					conn = fd;
					gen = ++nextGen;
					{
						std::lock_guard<std::mutex> guard(g_io.lock);
						g_io.curGen = gen;
						g_io.out.clear();
					}
					Push(Item::kConnected, gen);
					BF_TRACE(BFT_NET, "control: connection %llu opened", (unsigned long long)gen);
					continue;   // poll again with the connection in the set
				}
			}
		}

		if (conn >= 0 && connIndex >= 0 && (fds[connIndex].revents & (POLLIN | POLLHUP | POLLERR))) {
			ssize_t got = recv(conn, buf, sizeof buf, 0);
			if (got > 0) {
				const char *p = buf, *end = buf + got;
				while (p < end) {
					const char *nl = (const char *)memchr(p, '\n', end - p);
					const char *stop = nl ? nl : end;
					if (!discarding) {
						partial.append(p, stop - p);
						if (partial.size() > BF_CONTROL_LINE_MAX) {
							partial.clear();
							discarding = true;
							Push(Item::kOversized, gen);
						}
					}
					if (nl) {
						if (discarding)
							discarding = false;
						else if (!Blank(partial))
							Push(Item::kLine, gen, std::move(partial));
						partial.clear();
						p = nl + 1;
					} else {
						p = end;
					}
				}
			} else if (got == 0 || (errno != EAGAIN && errno != EINTR)) {
				disconnect(got == 0 ? "end of stream" : strerror(errno));
				continue;
			}
		}

		if (conn >= 0) {
			std::unique_lock<std::mutex> guard(g_io.lock);
			const char *failure = nullptr;
			if (g_io.overflow) {
				failure = "outbox overflow";
			} else {
				while (!g_io.out.empty()) {
					ssize_t w = send(conn, g_io.out.data(), g_io.out.size(), 0);
					if (w > 0) {
						g_io.out.erase(0, (size_t)w);
						continue;
					}
					if (w < 0 && (errno == EAGAIN || errno == EINTR))
						break;
					failure = w < 0 ? strerror(errno) : "write failed";
					break;
				}
			}
			guard.unlock();
			if (failure) {
				if (strcmp(failure, "outbox overflow") == 0)
					bf_tracef(BFT_ERR, "control: the reader does not read; more than %zu MiB unsent, connection dropped",
					          kOutboxMax >> 20);
				disconnect(failure);
			}
		}
	}

	// Stopping: write what is left, waiting at most a second, then close.
	if (conn >= 0) {
		std::string rest;
		{
			std::lock_guard<std::mutex> guard(g_io.lock);
			rest.swap(g_io.out);
			g_io.curGen = 0;
		}
		SetNonBlocking(conn, false);
		timeval tv = {1, 0};
		setsockopt(conn, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof tv);
		size_t off = 0;
		while (off < rest.size()) {
			ssize_t w = send(conn, rest.data() + off, rest.size() - off, 0);
			if (w <= 0) break;
			off += (size_t)w;
		}
		close(conn);
	}
}

// --------------------------------------------------------------- main side

struct Conn {
	uint64_t gen;
	uint64_t seq = 0;
	uint64_t replySeq = 0;
	std::deque<Item> lines;
	bf_control_request *pending = nullptr;
	std::deque<EventRecord> history;
};

struct Command {
	std::string name;
	bf_control_handler handler;
	void *context;
	bool (*available)();   // built-ins only
};

struct StatePart {
	std::string name;
	bf_control_state_fn fn;
	void *context;
};

struct Main {
	bool open = false;
	std::string path;
	std::string version = "unknown";
	Conn *conn = nullptr;
	std::vector<bf_control_request *> orphans;   // deferred, connection gone
	std::vector<Command> commands;
	std::vector<StatePart> parts;
	std::vector<std::string> events{"bye", "error"};
	bf_control_hooks hooks{};
	uint64_t frame = 0;
	bool inFrameDone = false;
	bool stopRequests = false;
	int depth = 0;                 // inside poll or frame_done
	bool closeLater = false;
	std::string closeReason;
	bool eventOpen = false;
	std::string eventName;
	bf_control_writer eventWriter;
	bool initialised = false;
};

Main g;
const std::deque<EventRecord> kNoHistory;

void FrameProvider(bf_control_writer *w, void *)
{
	bf_control_write_int(w, (int64_t)g.frame);
}

void Init()
{
	if (g.initialised) return;
	g.initialised = true;
	g.parts.push_back(StatePart{"frame", FrameProvider, nullptr});
	bfctl::RegisterBuiltins();
}

void Send(uint64_t gen, const std::string &line)
{
	{
		std::lock_guard<std::mutex> guard(g_io.lock);
		if (gen == 0 || gen != g_io.curGen)
			return;
		g_io.out += line;
		if (g_io.out.size() > kOutboxMax)
			g_io.overflow = true;
	}
	Wake();
}

void FreeRequest(bf_control_request *r)
{
	g.orphans.erase(std::remove(g.orphans.begin(), g.orphans.end(), r), g.orphans.end());
	if (g.conn && g.conn->pending == r)
		g.conn->pending = nullptr;
	if (r->freeContext)
		r->freeContext(r->checkContext);
	delete r;
}

void CloseConn()
{
	Conn *c = g.conn;
	if (!c) return;
	g.conn = nullptr;
	for (const Item &it : c->lines)
		g_io.queued -= it.text.size();
	if (c->pending) {
		c->pending->cancelled = true;
		g.orphans.push_back(c->pending);
	}
	delete c;
}

void SendReply(bf_control_request *r, int code, const char *message)
{
	if (r->replied) return;
	r->replied = true;
	if (g.conn && g.conn->pending == r)
		g.conn->pending = nullptr;
	if (r->cancelled || !g.conn || g.conn->gen != r->gen)
		return;
	if (code == BF_CONTROL_E_NONE && !r->fields.Complete()) {
		bf_tracef(BFT_ERR, "control: reply to %s malformed (%s)", r->cmd.c_str(),
		          r->fields.problem.empty() ? "unclosed" : r->fields.problem.c_str());
		r->fields.Reset(bf_control_writer::kFields);
		code = BF_CONTROL_E_INTERNAL;
		message = "the handler wrote a malformed reply";
	}
	std::string line = "{\"id\":" + r->idJson;
	char seq[48];
	snprintf(seq, sizeof seq, ",\"ok\":%s,\"seq\":%llu", code == BF_CONTROL_E_NONE ? "true" : "false",
	         (unsigned long long)g.conn->seq);
	line += seq;
	if (code != BF_CONTROL_E_NONE) {
		line += ",\"error\":{\"code\":\"";
		line += bf_control_code_name(code);
		line += "\",\"message\":";
		const char *m = message ? message : "";
		bfctl::AppendString(line, m, strlen(m));
		line += '}';
		if (r->fields.Complete())
			line += r->fields.out;
	} else {
		line += r->fields.out;
	}
	line += "}\n";
	g.conn->replySeq = g.conn->seq;
	Send(r->gen, line);
}

void AfterReply(bf_control_request *r)
{
	if (!r->inCallback)
		FreeRequest(r);
}

void ErrorLine(uint64_t gen, const std::string &idJson, int code, const char *message)
{
	bf_control_request r;
	r.gen = gen;
	r.idJson = idJson;
	r.cmd = "?";
	r.fields.Reset(bf_control_writer::kFields);
	SendReply(&r, code, message);
}

const Command *FindCommand(const std::string &name)
{
	for (const Command &c : g.commands)
		if (c.name == name) return &c;
	return nullptr;
}

void HandleLine(const Item &item)
{
	uint64_t gen = item.gen;
	if (item.kind == Item::kOversized) {
		ErrorLine(gen, "null", BF_CONTROL_E_BAD_REQUEST, "line longer than 1 MiB");
		return;
	}
	const std::string &text = item.text;
	if (!bfctl::ValidUtf8(text.data(), text.size())) {
		ErrorLine(gen, "null", BF_CONTROL_E_BAD_REQUEST, "not UTF-8");
		return;
	}
	bf_control_request *r = new bf_control_request;
	r->gen = gen;
	r->fields.Reset(bf_control_writer::kFields);
	std::string err;
	if (!bfctl::ParseJson(text.data(), text.size(), &r->args, &err)) {
		std::string msg = "not JSON: " + err;
		ErrorLine(gen, "null", BF_CONTROL_E_BAD_REQUEST, msg.c_str());
		delete r;
		return;
	}
	if (r->args.type != BF_CONTROL_T_OBJECT) {
		ErrorLine(gen, "null", BF_CONTROL_E_BAD_REQUEST, "not a JSON object");
		delete r;
		return;
	}
	const bf_control_value *id = bf_control_value_get(&r->args, "id");
	if (id) {
		if (id->type == BF_CONTROL_T_STRING || (id->type == BF_CONTROL_T_NUMBER && id->isInt)) {
			r->idJson.clear();
			bfctl::AppendValue(r->idJson, *id);
		} else if (id->type != BF_CONTROL_T_NULL) {
			ErrorLine(gen, "null", BF_CONTROL_E_BAD_REQUEST, "id must be an integer or a string");
			delete r;
			return;
		}
	}
	const bf_control_value *cmd = bf_control_value_get(&r->args, "cmd");
	if (!cmd || cmd->type != BF_CONTROL_T_STRING) {
		ErrorLine(gen, r->idJson, BF_CONTROL_E_BAD_REQUEST, "no cmd");
		delete r;
		return;
	}
	r->cmd = cmd->s;
	const Command *c = FindCommand(r->cmd);
	int refuse = BF_CONTROL_E_NONE;
	std::string why;
	if (bfctl::IsReservedCommand(r->cmd.c_str())) {
		refuse = BF_CONTROL_E_UNSUPPORTED;
		why = r->cmd + " is reserved for a later protocol version";
	} else if (!c) {
		refuse = BF_CONTROL_E_UNKNOWN_COMMAND;
		why = "no command " + r->cmd;
	} else if (c->available && !c->available()) {
		refuse = BF_CONTROL_E_UNSUPPORTED;
		why = r->cmd + ": the client has not provided what it needs";
	}
	if (refuse != BF_CONTROL_E_NONE) {
		SendReply(r, refuse, why.c_str());
		delete r;
		return;
	}
	BF_TRACE(BFT_INPUT, "control: request %s %s", r->idJson.c_str(), r->cmd.c_str());
	r->inCallback++;
	c->handler(r, c->context);
	r->inCallback--;
	if (!r->replied && !r->deferred) {
		bf_tracef(BFT_ERR, "control: handler of %s neither replied nor deferred", r->cmd.c_str());
		SendReply(r, BF_CONTROL_E_INTERNAL, "the handler gave no reply");
	}
	if (r->replied) {
		FreeRequest(r);
	} else if (g.conn && g.conn->gen == gen) {
		g.conn->pending = r;
	} else {
		r->cancelled = true;
		g.orphans.push_back(r);
	}
}

void DrainInbox()
{
	std::deque<Item> items;
	{
		std::lock_guard<std::mutex> guard(g_io.lock);
		items.swap(g_io.inbox);
	}
	for (Item &it : items) {
		switch (it.kind) {
		case Item::kConnected:
			CloseConn();
			g.conn = new Conn;
			g.conn->gen = it.gen;
			break;
		case Item::kDisconnected:
			if (g.conn && g.conn->gen == it.gen)
				CloseConn();
			break;
		default:
			if (g.conn && g.conn->gen == it.gen)
				g.conn->lines.push_back(std::move(it));
			else
				g_io.queued -= it.text.size();
			break;
		}
	}
}

void RunChecks()
{
	std::vector<bf_control_request *> list = g.orphans;
	if (g.conn && g.conn->pending)
		list.push_back(g.conn->pending);
	for (bf_control_request *r : list) {
		bool alive = (g.conn && g.conn->pending == r) ||
		             std::find(g.orphans.begin(), g.orphans.end(), r) != g.orphans.end();
		if (!alive || !r->check || r->replied) continue;
		r->inCallback++;
		r->check(r, r->checkContext);
		r->inCallback--;
		if (r->replied)
			FreeRequest(r);
	}
}

void ProcessLines()
{
	while (g.conn && !g.conn->pending && !g.stopRequests && !g.conn->lines.empty()) {
		Item it = std::move(g.conn->lines.front());
		g.conn->lines.pop_front();
		g_io.queued -= it.text.size();
		HandleLine(it);
	}
}

void DoClose(const char *reason);

void LeaveDepth()
{
	if (--g.depth == 0 && g.closeLater) {
		g.closeLater = false;
		DoClose(g.closeReason.c_str());
	}
}

void DoClose(const char *reason)
{
	if (!g.open) return;
	DrainInbox();
	if (g.conn) {
		bf_control_writer *w = bf_control_event_begin("bye");
		bf_control_write_key(w, "reason");
		bf_control_write_string(w, reason && *reason ? reason : "quit");
		bf_control_event_end(w);
	}
	g_io.stop = true;
	Wake();
	if (g_io.thread.joinable())
		g_io.thread.join();
	CloseConn();
	close(g_io.listenFd);
	close(g_io.wakeRead);
	close(g_io.wakeWrite);
	g_io.listenFd = g_io.wakeRead = g_io.wakeWrite = -1;
	{
		std::lock_guard<std::mutex> guard(g_io.lock);
		g_io.inbox.clear();
		g_io.out.clear();
		g_io.curGen = 0;
		g_io.overflow = false;
	}
	g_io.queued = 0;
	unlink(g.path.c_str());
	std::vector<bf_control_request *> orphans = g.orphans;
	for (bf_control_request *r : orphans) {
		r->cancelled = true;
		if (r->check && !r->inCallback)
			FreeRequest(r);
	}
	g.open = false;
	g.stopRequests = false;
	BF_TRACE(BFT_NET, "control: closed %s", g.path.c_str());
}

} // namespace

// ----------------------------------------------------- internal interface

namespace bfctl {

uint64_t NowMs() { return bf_clock_us() / 1000; }
bool InFrameDone() { return g.inFrameDone; }
const std::deque<EventRecord> &History() { return g.conn ? g.conn->history : kNoHistory; }
uint64_t LastReplySeq() { return g.conn ? g.conn->replySeq : 0; }
const bf_control_hooks &Hooks() { return g.hooks; }
const std::string &Version() { return g.version; }
void StopRequests() { g.stopRequests = true; }

bool HasState(const char *part)
{
	for (const StatePart &p : g.parts)
		if (p.name == part) return true;
	return false;
}

std::string StateJson(const char *part)
{
	for (const StatePart &p : g.parts) {
		if (p.name != part) continue;
		bf_control_writer w;
		w.Reset(bf_control_writer::kValue);
		p.fn(&w, p.context);
		if (!w.Complete()) {
			bf_tracef(BFT_ERR, "control: state provider %s wrote no single value (%s)", part,
			          w.problem.empty() ? "incomplete" : w.problem.c_str());
			return "null";
		}
		return w.out;
	}
	return "null";
}

std::vector<std::string> StateParts()
{
	std::vector<std::string> names;
	for (const StatePart &p : g.parts) names.push_back(p.name);
	return names;
}

std::vector<std::string> CommandNames()
{
	std::vector<std::string> names;
	for (const Command &c : g.commands)
		if (!c.available || c.available()) names.push_back(c.name);
	return names;
}

std::vector<std::string> EventNames() { return g.events; }

void RegisterBuiltin(const char *name, bf_control_handler handler, bool (*available)())
{
	g.commands.push_back(Command{name, handler, nullptr, available});
}

void DeferOwned(bf_control_request *r, bf_control_check check, void *context, void (*freeContext)(void *))
{
	bf_control_defer(r, check, context);
	r->freeContext = freeContext;
}

static bool InList(const char *name, const char *const *list, size_t n)
{
	for (size_t i = 0; i < n; i++)
		if (strcmp(name, list[i]) == 0) return true;
	return false;
}

bool IsReservedCommand(const char *name) { return InList(name, kReservedCommands, sizeof kReservedCommands / sizeof *kReservedCommands); }
bool IsReservedPart(const char *name) { return InList(name, kReservedParts, sizeof kReservedParts / sizeof *kReservedParts); }
bool IsReservedEvent(const char *name) { return InList(name, kReservedEvents, sizeof kReservedEvents / sizeof *kReservedEvents); }

} // namespace bfctl

// ------------------------------------------------------------- C interface

extern "C" {

const char *bf_control_code_name(int code)
{
	switch (code) {
	case BF_CONTROL_E_BAD_REQUEST: return "bad_request";
	case BF_CONTROL_E_UNKNOWN_COMMAND: return "unknown_command";
	case BF_CONTROL_E_UNSUPPORTED: return "unsupported";
	case BF_CONTROL_E_BAD_KEY: return "bad_key";
	case BF_CONTROL_E_BAD_TEXT: return "bad_text";
	case BF_CONTROL_E_BAD_STATE: return "bad_state";
	case BF_CONTROL_E_TIMEOUT: return "timeout";
	case BF_CONTROL_E_IO: return "io";
	default: return "internal";
	}
}

const char *bf_control_path_from_args(int argc, char *const *argv)
{
	for (int i = 1; argv && i < argc; i++) {
		if (!argv[i]) continue;
		if (strcmp(argv[i], "--control") == 0 && i + 1 < argc && argv[i + 1])
			return argv[i + 1];
		if (strncmp(argv[i], "--control=", 10) == 0)
			return argv[i] + 10;
	}
	const char *env = getenv("BF_CONTROL");
	return env && *env ? env : nullptr;
}

int bf_control_open(const bf_control_config *config)
{
	Init();
	if (g.open) {
		bf_tracef(BFT_ERR, "control: already open on %s", g.path.c_str());
		return BF_CONTROL_ERROR;
	}
	if (!config || !config->path || !*config->path) {
		bf_tracef(BFT_ERR, "control: no socket path");
		return BF_CONTROL_ERROR;
	}
	const char *path = config->path;
	sockaddr_un addr{};
	if (strlen(path) > BF_CONTROL_PATH_MAX || strlen(path) >= sizeof addr.sun_path) {
		bf_tracef(BFT_ERR, "control: socket path longer than %d bytes, running without the socket: %s",
		          BF_CONTROL_PATH_MAX, path);
		return BF_CONTROL_ERROR;
	}
	struct stat st;
	if (lstat(path, &st) == 0) {
		if (S_ISDIR(st.st_mode)) {
			bf_tracef(BFT_ERR, "control: %s is a directory", path);
			return BF_CONTROL_ERROR;
		}
		unlink(path);
	}
	int fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd < 0) {
		bf_tracef(BFT_ERR, "control: socket: %s", strerror(errno));
		return BF_CONTROL_ERROR;
	}
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, path);
	if (bind(fd, (sockaddr *)&addr, sizeof addr) != 0 || listen(fd, 4) != 0) {
		bf_tracef(BFT_ERR, "control: cannot listen on %s: %s", path, strerror(errno));
		close(fd);
		return BF_CONTROL_ERROR;
	}
	SetNonBlocking(fd, true);
	int pipefd[2];
	if (pipe(pipefd) != 0) {
		bf_tracef(BFT_ERR, "control: pipe: %s", strerror(errno));
		close(fd);
		unlink(path);
		return BF_CONTROL_ERROR;
	}
	SetNonBlocking(pipefd[0], true);
	SetNonBlocking(pipefd[1], true);
	g_io.listenFd = fd;
	g_io.wakeRead = pipefd[0];
	g_io.wakeWrite = pipefd[1];
	g_io.stop = false;
	g_io.queued = 0;
	g.path = path;
	g.version = config->version ? config->version : "unknown";
	g.open = true;
	g.stopRequests = false;
	g_io.thread = std::thread(IoLoop);
	BF_TRACE(BFT_NET, "control: listening on %s", path);
	return BF_CONTROL_OK;
}

void bf_control_close(const char *reason)
{
	if (g.depth > 0) {
		g.closeLater = true;
		g.closeReason = reason ? reason : "quit";
		return;
	}
	DoClose(reason);
}

int bf_control_is_open(void) { return g.open; }
int bf_control_is_connected(void) { return g.conn != nullptr; }

void bf_control_poll(void)
{
	Init();
	if (!g.open || g.depth > 0) return;
	g.depth++;
	DrainInbox();
	RunChecks();
	ProcessLines();
	LeaveDepth();
}

void bf_control_frame_done(uint64_t frames_drawn)
{
	Init();
	g.frame = frames_drawn;
	if (!g.open || g.depth > 0) return;
	g.depth++;
	g.inFrameDone = true;
	RunChecks();
	g.inFrameDone = false;
	ProcessLines();
	LeaveDepth();
}

uint64_t bf_control_frame(void) { return g.frame; }

int bf_control_register(const char *name, bf_control_handler handler, void *context)
{
	Init();
	if (!name || !*name || bfctl::IsReservedCommand(name)) {
		bf_tracef(BFT_ERR, "control: command name %s refused", name ? name : "(null)");
		return BF_CONTROL_ERROR;
	}
	for (size_t i = 0; i < g.commands.size(); i++) {
		if (g.commands[i].name != name) continue;
		if (handler)
			g.commands[i] = Command{name, handler, context, nullptr};
		else
			g.commands.erase(g.commands.begin() + i);
		return BF_CONTROL_OK;
	}
	if (handler)
		g.commands.push_back(Command{name, handler, context, nullptr});
	return BF_CONTROL_OK;
}

const char *bf_control_request_command(const bf_control_request *r) { return r ? r->cmd.c_str() : nullptr; }
const bf_control_value *bf_control_request_args(const bf_control_request *r) { return r ? &r->args : nullptr; }

const bf_control_value *bf_control_request_arg(const bf_control_request *r, const char *key)
{
	return r ? bf_control_value_get(&r->args, key) : nullptr;
}

bf_control_writer *bf_control_reply_writer(bf_control_request *r) { return r ? &r->fields : nullptr; }

void bf_control_reply(bf_control_request *r)
{
	if (!r || r->replied) return;
	SendReply(r, BF_CONTROL_E_NONE, nullptr);
	AfterReply(r);
}

void bf_control_reply_error(bf_control_request *r, int code, const char *message)
{
	if (!r || r->replied) return;
	if (code == BF_CONTROL_E_NONE) code = BF_CONTROL_E_INTERNAL;
	SendReply(r, code, message);
	AfterReply(r);
}

void bf_control_defer(bf_control_request *r, bf_control_check check, void *context)
{
	if (!r || r->replied) return;
	r->deferred = true;
	r->check = check;
	r->checkContext = context;
}

int bf_control_request_cancelled(const bf_control_request *r)
{
	return r ? (r->cancelled || !g.conn || g.conn->gen != r->gen) : 1;
}

void bf_control_declare_event(const char *name)
{
	if (!name || !*name || bfctl::IsReservedEvent(name)) return;
	if (std::find(g.events.begin(), g.events.end(), name) == g.events.end())
		g.events.push_back(name);
}

bf_control_writer *bf_control_event_begin(const char *name)
{
	if (!g.conn || !name || !*name) return nullptr;
	if (g.eventOpen) {
		bf_tracef(BFT_ERR, "control: event %s begun inside event %s", name, g.eventName.c_str());
		return nullptr;
	}
	g.eventOpen = true;
	g.eventName = name;
	g.eventWriter.Reset(bf_control_writer::kFields);
	return &g.eventWriter;
}

void bf_control_event_end(bf_control_writer *w)
{
	if (!w || w != &g.eventWriter || !g.eventOpen) return;
	g.eventOpen = false;
	if (!g.conn) return;
	if (!w->Complete()) {
		bf_tracef(BFT_ERR, "control: event %s malformed (%s), not sent", g.eventName.c_str(),
		          w->problem.empty() ? "unclosed" : w->problem.c_str());
		return;
	}
	Conn *c = g.conn;
	uint64_t seq = ++c->seq;
	std::string line = "{\"event\":";
	bfctl::AppendString(line, g.eventName.data(), g.eventName.size());
	char head[64];
	snprintf(head, sizeof head, ",\"seq\":%llu,\"t\":%llu", (unsigned long long)seq,
	         (unsigned long long)bfctl::NowMs());
	line += head;
	line += w->out;
	line += '}';
	EventRecord rec;
	rec.seq = seq;
	rec.name = g.eventName;
	bfctl::ParseJson(line.data(), line.size(), &rec.message, nullptr);
	c->history.push_back(std::move(rec));
	if (c->history.size() > kHistoryMax)
		c->history.pop_front();
	line += '\n';
	Send(c->gen, line);
}

int bf_control_register_state(const char *part, bf_control_state_fn fn, void *context)
{
	Init();
	if (!part || !*part || bfctl::IsReservedPart(part)) {
		bf_tracef(BFT_ERR, "control: state part name %s refused", part ? part : "(null)");
		return BF_CONTROL_ERROR;
	}
	for (size_t i = 0; i < g.parts.size(); i++) {
		if (g.parts[i].name != part) continue;
		if (fn)
			g.parts[i] = StatePart{part, fn, context};
		else
			g.parts.erase(g.parts.begin() + i);
		return BF_CONTROL_OK;
	}
	if (fn)
		g.parts.push_back(StatePart{part, fn, context});
	return BF_CONTROL_OK;
}

void bf_control_set_hooks(const bf_control_hooks *hooks)
{
	Init();
	if (hooks)
		g.hooks = *hooks;
	else
		g.hooks = bf_control_hooks{};
}

} // extern "C"
