// Built with -std=c++14: the contract must stay usable from the client.
#include "bf_audio.h"

int bf_audio_header_cxx14_check()
{
	bf_audio_voice v = 0;
	return (int)v + BF_AUDIO_KIND_MP3;
}
