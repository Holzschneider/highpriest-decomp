/*
 * The command channel of bifrost-bot: JSON lines in, JSON lines out.
 *
 * Kept apart from bot.cpp because it needs the POSIX socket headers, which
 * do not mix with the Windows-shaped headers the game's sources include.
 *
 * Commands come from stdin and, with a control path, from any client of a
 * Unix stream socket listening there. Every output line goes to stdout and to
 * every connected control client.
 */
#ifndef BF_BOT_IO_H
#define BF_BOT_IO_H

#include <map>
#include <string>
#include <vector>

typedef std::map<std::string, std::string> BfBotFields;

// useStdin: read commands from stdin. controlPath: also listen on this Unix
// socket (may be null). Returns false if the socket cannot be created.
bool bf_bot_io_open(bool useStdin, const char *controlPath);
void bf_bot_io_close();

// Waits up to timeoutMs for input and appends every complete line received.
void bf_bot_io_poll(int timeoutMs, std::vector<std::string> *lines);

// True once stdin has reached end of file (only when reading stdin).
bool bf_bot_io_stdin_closed();

// Writes one line (a newline is appended) to stdout and every control client.
void bf_bot_io_emit(const std::string &line);

// JSON helpers: a string literal with quotes, and a flat object of string,
// number, boolean and null values (numbers and booleans are kept as text).
std::string bf_bot_json_string(const std::string &text);
bool bf_bot_json_parse_object(const std::string &text, BfBotFields *fields);

// A command line: either a JSON object with "cmd", or the plain form
// "say hello", "whisper Name text", "move 150 180", "gm @zeny 100" ...
bool bf_bot_parse_command(const std::string &line, BfBotFields *fields, std::string *error);

#endif
