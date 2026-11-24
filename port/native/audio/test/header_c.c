/* Proves the contract compiles as plain C (and, built again as C++14 by
 * header_cxx14.cpp, as C++14). */
#include "bf_audio.h"

int bf_audio_header_c_check(void)
{
	bf_audio_counters c;
	bf_audio_info info;
	bf_audio_config cfg = { BF_AUDIO_MODE_SILENT, 0, 0 };
	(void)cfg; (void)info;
	bf_audio_get_counters(&c);
	return (int)sizeof c;
}
