// Shared between the two halves of the module; not part of the contract.
#ifndef BF_AUDIO_INTERNAL_H
#define BF_AUDIO_INTERNAL_H

#include <atomic>

extern std::atomic<long long> bf_audio_n_decodes;
extern std::atomic<long long> bf_audio_n_decode_failures;
extern std::atomic<long long> bf_audio_n_decode_fallbacks;

#endif
