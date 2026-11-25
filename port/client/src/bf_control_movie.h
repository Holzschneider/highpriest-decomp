// The control socket's `movie` part and `movie` event (PROTOCOL.md sections
// 3 and 5), read from the movie player (bf_movie.h, bf_movie_get_state):
// the intro movie CBink plays. Port code (bf_port_flags, in bf_port_main);
// bf_main.cpp calls it.
#ifndef BF_CONTROL_MOVIE_H
#define BF_CONTROL_MOVIE_H

// Registers the part and declares the event; before bf_control_open.
void bf_control_movie_install(void);

// Once per frame, before bf_control_poll: `movie` events for movies opened
// (`start`) and closed (`end`) since the last call.
void bf_control_movie_frame(void);

#endif
