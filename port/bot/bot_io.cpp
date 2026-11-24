/*
 * Command channel of bifrost-bot; see bot_io.h.
 */
#include "bot_io.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <sstream>

namespace {

struct Client {
	int fd;
	std::string pending;
};

bool s_useStdin = false;
bool s_stdinClosed = false;
std::string s_stdinPending;
int s_listenFd = -1;
std::string s_controlPath;
std::vector<Client> s_clients;

void SetNonBlocking(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

// Moves complete lines out of a buffer, dropping a carriage return before
// the newline and empty lines.
void TakeLines(std::string *pending, std::vector<std::string> *lines)
{
	size_t start = 0, nl;
	while ((nl = pending->find('\n', start)) != std::string::npos) {
		std::string line = pending->substr(start, nl - start);
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);
		if (!line.empty())
			lines->push_back(line);
		start = nl + 1;
	}
	pending->erase(0, start);
}

// Reads what is available; false on end of file or error.
bool ReadInto(int fd, std::string *pending)
{
	char buf[4096];
	for (;;) {
		ssize_t n = read(fd, buf, sizeof(buf));
		if (n > 0) {
			pending->append(buf, (size_t)n);
			if (n < (ssize_t)sizeof(buf))
				return true;
			continue;
		}
		if (n == 0)
			return false;
		if (errno == EINTR)
			continue;
		return errno == EAGAIN || errno == EWOULDBLOCK;
	}
}

void WriteAll(int fd, const char *data, size_t size)
{
	while (size > 0) {
		ssize_t n = write(fd, data, size);
		if (n > 0) {
			data += n;
			size -= (size_t)n;
			continue;
		}
		if (n < 0 && errno == EINTR)
			continue;
		if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
			struct pollfd p = { fd, POLLOUT, 0 };
			poll(&p, 1, 100);
			continue;
		}
		return;
	}
}

// JSON parsing, just enough for flat command objects.
struct Parser {
	const std::string &s;
	size_t i;
	explicit Parser(const std::string &text) : s(text), i(0) {}

	void Space() { while (i < s.size() && (s[i] == ' ' || s[i] == '\t' || s[i] == '\r' || s[i] == '\n')) ++i; }

	static void PutUtf8(std::string *out, unsigned code)
	{
		if (code < 0x80) {
			out->push_back((char)code);
		} else if (code < 0x800) {
			out->push_back((char)(0xc0 | (code >> 6)));
			out->push_back((char)(0x80 | (code & 0x3f)));
		} else {
			out->push_back((char)(0xe0 | (code >> 12)));
			out->push_back((char)(0x80 | ((code >> 6) & 0x3f)));
			out->push_back((char)(0x80 | (code & 0x3f)));
		}
	}

	bool String(std::string *out)
	{
		if (i >= s.size() || s[i] != '"')
			return false;
		++i;
		while (i < s.size()) {
			char c = s[i++];
			if (c == '"')
				return true;
			if (c != '\\') {
				out->push_back(c);
				continue;
			}
			if (i >= s.size())
				return false;
			char e = s[i++];
			switch (e) {
			case '"': case '\\': case '/': out->push_back(e); break;
			case 'n': out->push_back('\n'); break;
			case 't': out->push_back('\t'); break;
			case 'r': out->push_back('\r'); break;
			case 'b': out->push_back('\b'); break;
			case 'f': out->push_back('\f'); break;
			case 'u': {
				if (i + 4 > s.size())
					return false;
				unsigned code = (unsigned)strtoul(s.substr(i, 4).c_str(), NULL, 16);
				i += 4;
				// \u00XX stands for the raw byte XX, as bf_bot_json_string writes it.
				if (code < 0x100)
					out->push_back((char)code);
				else
					PutUtf8(out, code);
				break;
			}
			default: return false;
			}
		}
		return false;
	}

	bool Bare(std::string *out)
	{
		size_t start = i;
		while (i < s.size() && s[i] != ',' && s[i] != '}' && s[i] != ' ' && s[i] != '\t')
			++i;
		*out = s.substr(start, i - start);
		return !out->empty();
	}
};

} // namespace

bool bf_bot_io_open(bool useStdin, const char *controlPath)
{
	signal(SIGPIPE, SIG_IGN);
	s_useStdin = useStdin;
	if (useStdin)
		SetNonBlocking(0);
	if (!controlPath || !*controlPath)
		return true;

	struct sockaddr_un addr;
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	if (strlen(controlPath) >= sizeof(addr.sun_path)) {
		fprintf(stderr, "bifrost-bot: control path too long: %s\n", controlPath);
		return false;
	}
	strncpy(addr.sun_path, controlPath, sizeof(addr.sun_path) - 1);
	unlink(controlPath);
	s_listenFd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (s_listenFd < 0 || bind(s_listenFd, (struct sockaddr *)&addr, sizeof(addr)) != 0 || listen(s_listenFd, 4) != 0) {
		fprintf(stderr, "bifrost-bot: cannot listen on %s: %s\n", controlPath, strerror(errno));
		return false;
	}
	SetNonBlocking(s_listenFd);
	s_controlPath = controlPath;
	return true;
}

void bf_bot_io_close()
{
	for (size_t i = 0; i < s_clients.size(); ++i)
		close(s_clients[i].fd);
	s_clients.clear();
	if (s_listenFd >= 0) {
		close(s_listenFd);
		unlink(s_controlPath.c_str());
		s_listenFd = -1;
	}
}

void bf_bot_io_poll(int timeoutMs, std::vector<std::string> *lines)
{
	std::vector<struct pollfd> fds;
	if (s_useStdin && !s_stdinClosed) {
		struct pollfd p = { 0, POLLIN, 0 };
		fds.push_back(p);
	}
	if (s_listenFd >= 0) {
		struct pollfd p = { s_listenFd, POLLIN, 0 };
		fds.push_back(p);
	}
	for (size_t i = 0; i < s_clients.size(); ++i) {
		struct pollfd p = { s_clients[i].fd, POLLIN, 0 };
		fds.push_back(p);
	}
	if (fds.empty()) {
		if (timeoutMs > 0)
			usleep(timeoutMs * 1000);
		return;
	}
	if (poll(&fds[0], fds.size(), timeoutMs) <= 0)
		return;

	for (size_t k = 0; k < fds.size(); ++k) {
		if (!(fds[k].revents & (POLLIN | POLLHUP | POLLERR)))
			continue;
		int fd = fds[k].fd;
		if (fd == 0) {
			if (!ReadInto(0, &s_stdinPending)) {
				s_stdinPending.push_back('\n');
				s_stdinClosed = true;
			}
			TakeLines(&s_stdinPending, lines);
		} else if (fd == s_listenFd) {
			int c = accept(s_listenFd, NULL, NULL);
			if (c >= 0) {
				SetNonBlocking(c);
				int one = 1;
				setsockopt(c, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof(one));
				Client client = { c, std::string() };
				s_clients.push_back(client);
			}
		} else {
			for (size_t i = 0; i < s_clients.size(); ++i) {
				if (s_clients[i].fd != fd)
					continue;
				bool open = ReadInto(fd, &s_clients[i].pending);
				if (!open)
					s_clients[i].pending.push_back('\n');
				TakeLines(&s_clients[i].pending, lines);
				if (!open) {
					close(fd);
					s_clients.erase(s_clients.begin() + i);
				}
				break;
			}
		}
	}
}

bool bf_bot_io_stdin_closed()
{
	return s_stdinClosed;
}

void bf_bot_io_emit(const std::string &line)
{
	std::string out = line + "\n";
	WriteAll(1, out.data(), out.size());
	for (size_t i = 0; i < s_clients.size(); ++i)
		WriteAll(s_clients[i].fd, out.data(), out.size());
}

std::string bf_bot_json_string(const std::string &text)
{
	std::string out = "\"";
	for (size_t i = 0; i < text.size(); ++i) {
		unsigned char c = (unsigned char)text[i];
		switch (c) {
		case '"': out += "\\\""; break;
		case '\\': out += "\\\\"; break;
		case '\n': out += "\\n"; break;
		case '\r': out += "\\r"; break;
		case '\t': out += "\\t"; break;
		default:
			// Game text is in a legacy code page, not UTF-8: every byte that is
			// not printable ASCII is written as \u00XX, so the line stays valid
			// JSON and the bytes can be recovered.
			if (c < 0x20 || c >= 0x7f) {
				char hex[8];
				snprintf(hex, sizeof(hex), "\\u%04x", c);
				out += hex;
			} else {
				out.push_back((char)c);
			}
		}
	}
	return out + "\"";
}

bool bf_bot_json_parse_object(const std::string &text, BfBotFields *fields)
{
	Parser p(text);
	p.Space();
	if (p.i >= text.size() || text[p.i] != '{')
		return false;
	++p.i;
	p.Space();
	if (p.i < text.size() && text[p.i] == '}')
		return true;
	for (;;) {
		p.Space();
		std::string key, value;
		if (!p.String(&key))
			return false;
		p.Space();
		if (p.i >= text.size() || text[p.i] != ':')
			return false;
		++p.i;
		p.Space();
		if (p.i < text.size() && text[p.i] == '"') {
			if (!p.String(&value))
				return false;
		} else if (!p.Bare(&value)) {
			return false;
		}
		if (value != "null")
			(*fields)[key] = value;
		p.Space();
		if (p.i < text.size() && text[p.i] == ',') {
			++p.i;
			continue;
		}
		return p.i < text.size() && text[p.i] == '}';
	}
}

bool bf_bot_parse_command(const std::string &line, BfBotFields *fields, std::string *error)
{
	size_t first = line.find_first_not_of(" \t");
	if (first == std::string::npos) {
		*error = "empty command";
		return false;
	}
	if (line[first] == '{') {
		if (!bf_bot_json_parse_object(line.substr(first), fields)) {
			*error = "not a flat JSON object";
			return false;
		}
		if (fields->find("cmd") == fields->end()) {
			*error = "no \"cmd\" field";
			return false;
		}
		return true;
	}

	// Plain form: the first word is the command; the rest is split by what
	// the command takes, the last argument keeping its spaces.
	std::string rest = line.substr(first);
	size_t sp = rest.find_first_of(" \t");
	std::string cmd = rest.substr(0, sp);
	size_t arg = sp == std::string::npos ? std::string::npos : rest.find_first_not_of(" \t", sp);
	rest = arg == std::string::npos ? "" : rest.substr(arg);
	(*fields)["cmd"] = cmd;

	std::istringstream words(rest);
	std::string a, b, c;
	if (cmd == "say" || cmd == "gm" || cmd == "party_say") {
		// gm "@zeny 100": the quotes are optional.
		if (rest.size() >= 2 && rest[0] == '"' && rest[rest.size() - 1] == '"')
			rest = rest.substr(1, rest.size() - 2);
		(*fields)["text"] = rest;
	} else if (cmd == "whisper") {
		size_t s2 = rest.find_first_of(" \t");
		if (s2 == std::string::npos || rest.find_first_not_of(" \t", s2) == std::string::npos) {
			*error = "usage: whisper <name> <text>";
			return false;
		}
		(*fields)["name"] = rest.substr(0, s2);
		(*fields)["text"] = rest.substr(rest.find_first_not_of(" \t", s2));
	} else if (cmd == "move") {
		words >> a >> b;
		(*fields)["x"] = a;
		(*fields)["y"] = b;
	} else if (cmd == "trade_add") {
		// trade_add zeny <amount> | trade_add item <item id> <count> | trade_add index <index> <count>
		words >> a >> b >> c;
		(*fields)["kind"] = a;
		(*fields)["amount"] = b;
		if (!c.empty())
			(*fields)["count"] = c;
	} else if (cmd == "set") {
		words >> a;
		std::string value;
		std::getline(words, value);
		size_t v = value.find_first_not_of(" \t");
		(*fields)["key"] = a;
		(*fields)["value"] = v == std::string::npos ? "" : value.substr(v);
	} else if (cmd == "raw") {
		(*fields)["hex"] = rest;
	} else if (!rest.empty()) {
		// invite_party, request_trade, invite_guild, create_party, ...: one name
		(*fields)["name"] = rest;
	}
	return true;
}
