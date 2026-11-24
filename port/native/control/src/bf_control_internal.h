// Shared between the core (socket, registry, requests) and the built-in
// commands. Internal; main thread unless said otherwise.
#ifndef BF_CONTROL_INTERNAL_H
#define BF_CONTROL_INTERNAL_H

#include "bf_control.h"
#include "bf_control_json.h"

#include <deque>
#include <string>
#include <vector>

struct bf_control_request {
	uint64_t gen = 0;              // the connection it came on
	std::string idJson = "null";   // the id as it is echoed back
	std::string cmd;
	bf_control_value args;
	bf_control_writer fields;
	bool deferred = false;
	bool replied = false;
	bool cancelled = false;
	int inCallback = 0;
	bf_control_check check = nullptr;
	void *checkContext = nullptr;
	void (*freeContext)(void *) = nullptr;   // built-ins: frees checkContext with the request
};

namespace bfctl {

struct EventRecord {
	uint64_t seq;
	std::string name;
	bf_control_value message;      // the whole event object
};

// The time stamp of events, milliseconds since start, from the sys clock.
uint64_t NowMs();

// True while bf_control_frame_done is running its checks.
bool InFrameDone();

// The current connection's events (empty when none) and the seq of the
// reply to its previous request.
const std::deque<EventRecord> &History();
uint64_t LastReplySeq();

// State parts: whether a provider exists; its value as JSON text (a
// provider that writes nonsense gives "null").
bool HasState(const char *part);
std::string StateJson(const char *part);
// Names of the registered parts, in registration order.
std::vector<std::string> StateParts();

const bf_control_hooks &Hooks();
const std::string &Version();
std::vector<std::string> CommandNames();
std::vector<std::string> EventNames();

// After `quit`: no further requests are carried out.
void StopRequests();

// Registers a built-in command; available() decides whether hello lists it
// and whether it runs (unsupported otherwise).
void RegisterBuiltin(const char *name, bf_control_handler handler, bool (*available)());
void RegisterBuiltins();

// The defer used by built-ins: the context is freed with the request.
void DeferOwned(bf_control_request *r, bf_control_check check, void *context, void (*freeContext)(void *));

bool IsReservedCommand(const char *name);
bool IsReservedPart(const char *name);
bool IsReservedEvent(const char *name);

} // namespace bfctl

#endif
