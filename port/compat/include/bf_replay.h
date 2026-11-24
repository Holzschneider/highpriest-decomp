/*
 * Packet replay (implementation plan M6.5, technical plan 3.6): with
 * BF_PKT_REPLAY=<log> the client's connections open no socket and take what
 * they receive from a log that BF_PKT_LOG recorded (port/instrument/bf_trace.h
 * gives the line format). device/Connection.cpp calls these under BF_PORT;
 * the implementation is port/compat/stubs/bf_replay.cpp. C library only.
 *
 * The log is cut into connections by its seventh field ("account/1",
 * "char/2", "zone/3"); a log without it is cut where the role of a sent
 * packet changes (CA_, CH_, CZ_). The client's n-th connect is the log's
 * n-th connection, whatever address it names.
 *
 * Pacing: received bytes are handed over in the log's order, up to the next
 * packet the client sent in the recording; that packet releases the rest.
 * A sent packet is matched by header against the pending sent packets up to
 * the next received one (passing over the ones the client did not send,
 * such as keep-alives); a mismatch is reported and never blocks. A sent
 * packet the client does not send within BF_PKT_REPLAY_WAIT milliseconds
 * (default 5000; 0 waits for ever) is passed over. Lengths are compared;
 * bytes only with BF_PKT_REPLAY_BYTES=1.
 *
 * A connection whose log has run out stays open until the client closes it,
 * except the log's last one: BF_PKT_REPLAY_HOLD milliseconds after its last
 * packet (default 5000; -1 never) the server closes it, as when a server
 * drops the connection, with a trace line.
 *
 * Trace lines are category net ("replay: ..."); counters replay.*.
 */
#ifndef BF_REPLAY_H
#define BF_REPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

/* 1 when BF_PKT_REPLAY is set. Reads the log on first call; an unreadable
   log still counts as replay (every connect then fails), so that the client
   never falls back to the network. */
int bf_replay_active(void);

/* The client connects: takes the log's next connection. 0 when the log has
   none left (the connect fails). */
int bf_replay_connect(const char *ip, int port);

/* The client closes the connection. */
void bf_replay_disconnect(void);

/* The client sends a packet: compared with the log. */
void bf_replay_sent(const void *data, int size);

/* Bytes the server sends now, at most size: the count, 0 when nothing is due,
   -1 when the log has ended and the server closes the connection. */
int bf_replay_receive(char *buf, int size);

#ifdef __cplusplus
}
#endif

#endif
