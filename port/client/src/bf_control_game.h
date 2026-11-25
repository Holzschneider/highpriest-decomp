// The control socket's game parts and events (PROTOCOL.md section 3: `map`,
// `self`, `actors`, `pointer`; section 5: `map`, `chat`), written as JSON
// from what bf_client_game.cpp reads out of the client. Port code
// (bf_compat_flags, in bf_port_main); bf_main.cpp calls it.
#ifndef BF_CONTROL_GAME_H
#define BF_CONTROL_GAME_H

#include "bf_control.h"

// Registers the parts and declares the events; before bf_control_open.
void bf_control_game_install(void);

// Once per frame, before bf_control_poll: sends `map` when game mode has
// entered a map, and `chat` for each line added to the chat window.
void bf_control_game_frame(void);

// Writes the client's Windows-1252 bytes as a UTF-8 string (PROTOCOL.md
// section 3, "Text in state"); NULL as null.
void bf_control_game_write_text(bf_control_writer *w, const char *text);

#endif
