/*
 * bf_port.h: what the client's own sources call under BF_PORT (M3.2).
 *
 * The start-up glue in port/compat/stubs (bf_main.cpp, bf_zlib.cpp) defines
 * the bf_port_* and bf_zlib_* functions; the native headers below are
 * included by relative path so that the census, which knows only the
 * client's include paths, compiles the same units.
 */
#ifndef BF_PORT_H
#define BF_PORT_H

#include "../../native/codec/include/bf_codec.h"
#include "../../native/sys/include/bf_file.h"
#include "../../native/sys/include/bf_settings.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Non-zero when the client was started with --headless (M3.7): WinMain runs
   the start-up up to the login mode's construction and returns 0 before the
   window, the graphics device and the first frame. */
int bf_port_headless(void);

/* After InitApp has created the window: creates the gfx device on the
   window's Metal layer at the view's logical size and UI scale and hands it
   to the Direct3D adapter (bf_compat_d3d_set_device) and the view. Called
   before SetupDriver2. 0 on failure (traced). */
int bf_port_start_graphics(void);

/* Once per frame, from CheckSystemMessage: reports the frame last presented
   to the control socket (captures are taken then) and carries out its
   requests. Does nothing without a socket. */
void bf_port_frame(void);

/* An option of the settings (BF_OPT_*), 0 or 1, with its default. */
int bf_port_option(const char *option);

/* zlib with the client's 32-bit lengths (common.h maps uncompress and
   compress here): the system zlib's uLongf is 64 bits on macOS. Return
   zlib's codes: 0 Z_OK, -3 Z_DATA_ERROR, -4 Z_MEM_ERROR, -5 Z_BUF_ERROR. */
int bf_zlib_uncompress(unsigned char *dest, unsigned int *dest_len, const unsigned char *source, unsigned int source_len);
int bf_zlib_compress(unsigned char *dest, unsigned int *dest_len, const unsigned char *source, unsigned int source_len);

#ifdef __cplusplus
}
#endif

#endif
