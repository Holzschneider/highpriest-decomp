// The AVAudioEngine half: a fixed pool of player nodes for sound effects
// feeding an effects mixer, one player node for the music stream feeding a
// music mixer, both into the main mixer.
//
// Every node is wired once, at the engine's rate, before the engine starts.
// A player wired (or re-wired) at another rate while the engine runs starts
// late, by an amount that grows with the time the engine has run (measured:
// 0.4 s after 5 s, offline); so sounds are converted to the engine rate once
// and cached, each voice slot has a mono and a stereo player, and the stream
// player lets AVAudioPlayerNode convert the file's rate. One recursive lock serialises every
// public call; AVFoundation's completion handlers only touch atomics or hand
// work to a private serial queue, so stopping a node while holding the lock
// cannot deadlock.

#include "bf_audio.h"
#include "bf_audio_internal.h"
#include "bf_trace.h"

#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>

#include <string.h>
#include <stdlib.h>

#include <atomic>
#include <mutex>
#include <string>

struct bf_audio_sound {
	AVAudioPCMBuffer *buf;      // at the source rate
	AVAudioPCMBuffer *cached;   // at cached_rate, made on first play
	double cached_rate;
	int64_t frames;
	double rate;
	int channels;
};

namespace {

struct Voice {
	AVAudioPlayerNode *mono = nil, *stereo = nil;
	AVAudioPlayerNode *node = nil;   // the one in use: mono or stereo
	uint32_t gen = 0;             // bumped on every play; part of the handle
	bool active = false;
	bool loop = false;
	uint64_t seq = 0;             // start order, for the stealing rule
	AVAudioFramePosition length = 0;
	std::atomic<uint32_t> finished{0};   // the highest generation whose buffer has been rendered or stopped
};

struct Counters {
	long long voices_started = 0, voices_stolen = 0, voices_rejected = 0, conversions = 0;
	long long device_failures = 0, engine_restarts = 0, streams_opened = 0, stream_failures = 0;
};

struct State {
	std::recursive_mutex lock;
	int mode = -1;
	int max_voices = 0;
	double rate = 0;
	AVAudioEngine *engine = nil;
	AVAudioMixerNode *fx = nil, *music = nil;
	Voice voices[BF_AUDIO_MAX_VOICES];
	uint64_t seq = 0;
	int silent_next = 0;
	id observer = nil;
	dispatch_queue_t queue = nullptr;
	AVAudioPCMBuffer *render_buf = nil;

	float fx_volume = 1, music_volume = 1;

	AVAudioPlayerNode *stream_node = nil;
	AVAudioFile *stream_file = nil;
	double stream_pass = 0;      // one pass of the file in engine frames
	std::string stream_path;
	int stream_status = BF_AUDIO_STREAM_NONE;
	int loop_count = 1;          // as set; applies from the next start
	int active_loop_count = 1;   // the one the playing stream uses
	int loops_done = 0;
	int scheduled = 0;
	uint32_t stream_gen = 0;
	std::atomic<uint32_t> stream_gen_now{0};      // stream_gen, for completion handlers
	std::atomic<int> stream_completions{0};      // passes reported rendered for stream_gen_now
	float stream_volume = 1;

	Counters c;
};

State &S()
{
	static State *s = new State;   // never destroyed: completion handlers may outlive main
	return *s;
}

typedef std::lock_guard<std::recursive_mutex> Guard;

float clamp(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

void mark_finished(std::atomic<uint32_t> &a, uint32_t gen)
{
	uint32_t cur = a.load();
	while (cur < gen && !a.compare_exchange_weak(cur, gen)) {}
}

bf_audio_voice handle_of(int slot, uint32_t gen) { return (gen << 6) | (uint32_t)slot; }

AVAudioFramePosition played(Voice &v)
{
	AVAudioTime *t = v.node.lastRenderTime;
	// Before the first render (offline: before the first bf_audio_render)
	// the node time is neither sample- nor host-valid, and
	// playerTimeForNodeTime throws on it.
	if (!t || !(t.sampleTimeValid || t.hostTimeValid)) return 0;
	AVAudioTime *p = [v.node playerTimeForNodeTime:t];
	if (!p || !p.sampleTimeValid) return 0;
	return p.sampleTime;
}

bool voice_done(Voice &v)
{
	if (!v.active) return true;
	if (v.finished.load() >= v.gen || (!v.loop && played(v) >= v.length)) {
		v.active = false;
		return true;
	}
	return false;
}

// The voice a handle names, if it is still that voice.
Voice *voice_of(bf_audio_voice h)
{
	State &s = S();
	int slot = (int)(h & 63);
	uint32_t gen = h >> 6;
	if (h == 0 || s.mode < 0 || slot >= s.max_voices) return nullptr;
	Voice &v = s.voices[slot];
	if (v.gen != gen || voice_done(v)) return nullptr;
	return &v;
}

AVAudioFormat *standard(double rate, int channels)
{
	return [[AVAudioFormat alloc] initStandardFormatWithSampleRate:rate channels:(AVAudioChannelCount)channels];
}

// The sound's buffer at the engine rate, converted on first use.
AVAudioPCMBuffer *engine_buffer(bf_audio_sound *snd)
{
	State &s = S();
	if (snd->rate == s.rate) return snd->buf;
	if (snd->cached && snd->cached_rate == s.rate) return snd->cached;
	AVAudioFormat *to = standard(s.rate, snd->channels);
	AVAudioConverter *cv = [[AVAudioConverter alloc] initFromFormat:snd->buf.format toFormat:to];
	if (!cv) return nil;
	cv.sampleRateConverterQuality = AVAudioQualityHigh;
	cv.primeMethod = AVAudioConverterPrimeMethod_None;
	AVAudioFrameCount cap = (AVAudioFrameCount)ceil(snd->frames * s.rate / snd->rate) + 64;
	AVAudioPCMBuffer *out = [[AVAudioPCMBuffer alloc] initWithPCMFormat:to frameCapacity:cap];
	AVAudioPCMBuffer *in = snd->buf;
	__block bool given = false;
	NSError *err = nil;
	AVAudioConverterOutputStatus st = [cv convertToBuffer:out error:&err
	        withInputFromBlock:^AVAudioBuffer *(AVAudioPacketCount, AVAudioConverterInputStatus *status) {
		if (given) { *status = AVAudioConverterInputStatus_EndOfStream; return nil; }
		given = true;
		*status = AVAudioConverterInputStatus_HaveData;
		return in;
	}];
	if (st == AVAudioConverterOutputStatus_Error || out.frameLength == 0) {
		BF_TRACE(BFT_SND, "audio: rate conversion %.0f -> %.0f Hz failed: %s\n", snd->rate, s.rate,
		         err ? err.localizedDescription.UTF8String : "no output");
		return nil;
	}
	snd->cached = out;
	snd->cached_rate = s.rate;
	s.c.conversions++;
	return out;
}

double stream_pass_frames()
{
	State &s = S();
	return (double)s.stream_file.length * s.rate / s.stream_file.processingFormat.sampleRate;
}

void stream_copy_done(uint32_t gen);

// Keeps two passes of the file queued (fewer when the loop count is nearly
// reached), so the loop is gapless.
void stream_fill()
{
	State &s = S();
	if (!s.engine || !s.stream_file) return;
	while (s.scheduled - s.loops_done < 2 && (s.active_loop_count == 0 || s.scheduled < s.active_loop_count)) {
		s.scheduled++;
		uint32_t gen = s.stream_gen;
		dispatch_queue_t q = s.queue;
		[s.stream_node scheduleSegment:s.stream_file startingFrame:0 frameCount:(AVAudioFrameCount)s.stream_file.length atTime:nil
		        completionCallbackType:AVAudioPlayerNodeCompletionDataRendered
		             completionHandler:^(AVAudioPlayerNodeCompletionCallbackType) {
			// Inline when the lock is free or already ours (offline rendering
			// calls this from bf_audio_render); otherwise later, on the queue.
			State &st = S();
			if (st.stream_gen_now.load() == gen) st.stream_completions++;
			if (st.lock.try_lock()) {
				stream_copy_done(gen);
				st.lock.unlock();
			} else {
				dispatch_async(q, ^{ stream_copy_done(gen); });
			}
		}];
	}
}

// Counts the passes rendered, from the player's own clock and from the
// completions, whichever is further, then tops the queue up. Called from the
// completion handlers, after every offline render chunk, and when a caller
// asks for the stream's state.
void stream_update()
{
	State &s = S();
	if (!s.engine || !s.stream_file) return;
	if (s.stream_status != BF_AUDIO_STREAM_PLAYING && s.stream_status != BF_AUDIO_STREAM_PAUSED) return;
	int passes = s.stream_completions.load();
	AVAudioTime *t = s.stream_node.lastRenderTime;
	AVAudioTime *p = t && (t.sampleTimeValid || t.hostTimeValid) ? [s.stream_node playerTimeForNodeTime:t] : nil;
	if (p && p.sampleTimeValid && s.stream_pass > 0) {
		int byclock = (int)((p.sampleTime + 0.5) / s.stream_pass);
		if (byclock > passes) passes = byclock;
	}
	if (passes > s.scheduled) passes = s.scheduled;
	if (passes <= s.loops_done) return;
	s.loops_done = passes;
	BF_TRACE(BFT_SND, "audio: stream pass %d of %s done\n", s.loops_done, s.stream_path.c_str());
	if (s.active_loop_count != 0 && s.loops_done >= s.active_loop_count) {
		s.stream_status = BF_AUDIO_STREAM_DONE;
		return;
	}
	stream_fill();
}

void stream_copy_done(uint32_t gen)
{
	State &s = S();
	Guard g(s.lock);
	if (gen != s.stream_gen) return;   // stopped, closed or restarted since
	stream_update();
}

void stream_begin()
{
	State &s = S();
	s.stream_gen++;
	s.stream_gen_now = s.stream_gen;
	s.loops_done = 0;
	s.scheduled = 0;
	s.active_loop_count = s.loop_count;
	s.stream_status = BF_AUDIO_STREAM_PLAYING;
	if (!s.engine) return;   // SILENT
	[s.stream_node stop];
	s.stream_completions = 0;
	s.stream_pass = stream_pass_frames();
	s.stream_node.volume = s.stream_volume;
	stream_fill();
	[s.stream_node play];
}

void teardown()
{
	State &s = S();
	if (s.observer) {
		[[NSNotificationCenter defaultCenter] removeObserver:s.observer];
		s.observer = nil;
	}
	s.stream_gen++;
	s.stream_gen_now = s.stream_gen;
	if (s.engine) {
		for (int i = 0; i < s.max_voices; ++i) { [s.voices[i].mono stop]; [s.voices[i].stereo stop]; }
		[s.stream_node stop];
		[s.engine stop];
	}
	for (int i = 0; i < BF_AUDIO_MAX_VOICES; ++i) {
		Voice &v = s.voices[i];
		v.node = v.mono = v.stereo = nil;
		v.active = false;
	}
	s.stream_node = nil;
	s.fx = s.music = nil;
	s.render_buf = nil;
	s.engine = nil;
	s.rate = 0;
}

bool build_graph()
{
	State &s = S();
	AVAudioEngine *e = s.engine;
	AVAudioMixerNode *main = e.mainMixerNode;
	s.fx = [AVAudioMixerNode new];
	s.music = [AVAudioMixerNode new];
	[e attachNode:s.fx];
	[e attachNode:s.music];
	AVAudioFormat *stereo = standard(s.rate, 2);
	[e connect:s.fx to:main fromBus:0 toBus:0 format:stereo];
	[e connect:s.music to:main fromBus:0 toBus:1 format:stereo];
	s.fx.outputVolume = s.fx_volume;
	s.music.outputVolume = s.music_volume;
	AVAudioFormat *mono = standard(s.rate, 1);
	for (int i = 0; i < s.max_voices; ++i) {
		Voice &v = s.voices[i];
		v.mono = [AVAudioPlayerNode new];
		v.stereo = [AVAudioPlayerNode new];
		v.node = v.mono;
		[e attachNode:v.mono];
		[e attachNode:v.stereo];
		[e connect:v.mono to:s.fx fromBus:0 toBus:(AVAudioNodeBus)(2 * i) format:mono];
		[e connect:v.stereo to:s.fx fromBus:0 toBus:(AVAudioNodeBus)(2 * i + 1) format:stereo];
	}
	s.stream_node = [AVAudioPlayerNode new];
	[e attachNode:s.stream_node];
	[e connect:s.stream_node to:s.music fromBus:0 toBus:0 format:stereo];   // the player converts the file's rate
	s.stream_node.volume = s.stream_volume;
	[e prepare];
	NSError *err = nil;
	if (![e startAndReturnError:&err]) {
		BF_TRACE(BFT_SND, "audio: engine does not start: %s\n", err.localizedDescription.UTF8String);
		return false;
	}
	return true;
}

void go_silent(const char *why)
{
	State &s = S();
	BF_TRACE(BFT_SND, "audio: running silent: %s\n", why);
	teardown();
	s.mode = BF_AUDIO_MODE_SILENT;
	s.c.device_failures++;
	bf_count("audio.device_failures", 1);
	if (s.stream_status == BF_AUDIO_STREAM_PLAYING || s.stream_status == BF_AUDIO_STREAM_PAUSED)
		s.stream_gen++;   // state is kept; nothing sounds
}

// After an output device change the engine has stopped itself: start it
// again and restart the music from the top. Voices that were sounding are
// dropped (they were short).
void restart_after_change()
{
	State &s = S();
	Guard g(s.lock);
	if (s.mode != BF_AUDIO_MODE_DEVICE || !s.engine) return;
	s.c.engine_restarts++;
	bf_count("audio.engine_restarts", 1);
	for (int i = 0; i < s.max_voices; ++i) s.voices[i].active = false;
	AVAudioFormat *out = [s.engine.outputNode outputFormatForBus:0];
	if (out.sampleRate <= 0 || out.channelCount == 0) { go_silent("the output device went away"); return; }
	if (out.sampleRate != s.rate)
		BF_TRACE(BFT_SND, "audio: device rate changed %.0f -> %.0f Hz; the graph keeps %.0f\n", s.rate, out.sampleRate, s.rate);
	NSError *err = nil;
	if (![s.engine startAndReturnError:&err]) { go_silent("the engine does not restart"); return; }
	BF_TRACE(BFT_SND, "audio: engine restarted after a device change (%.0f Hz)\n", out.sampleRate);
	if (s.stream_status == BF_AUDIO_STREAM_PLAYING) {
		int loops = s.loops_done;
		stream_begin();
		s.loops_done = loops;
	}
}

} // namespace

extern "C" {

int bf_audio_start(const bf_audio_config *config)
{
	State &s = S();
	Guard g(s.lock);
	bf_audio_config cfg = { BF_AUDIO_MODE_DEVICE, 0, 0 };
	if (config) cfg = *config;
	if (s.mode >= 0) { BF_TRACE(BFT_SND, "audio: already started\n"); return -1; }
	if (cfg.max_voices == 0) cfg.max_voices = BF_AUDIO_DEFAULT_VOICES;
	if (cfg.offline_rate == 0) cfg.offline_rate = 44100;
	if (cfg.mode < BF_AUDIO_MODE_DEVICE || cfg.mode > BF_AUDIO_MODE_SILENT || cfg.max_voices < 1
	    || cfg.max_voices > BF_AUDIO_MAX_VOICES || cfg.offline_rate < 8000 || cfg.offline_rate > 192000) {
		BF_TRACE(BFT_SND, "audio: invalid configuration\n");
		return -1;
	}
	if (!s.queue) s.queue = dispatch_queue_create("bf_audio", DISPATCH_QUEUE_SERIAL);
	s.max_voices = cfg.max_voices;
	for (int i = 0; i < BF_AUDIO_MAX_VOICES; ++i) s.voices[i].active = false;

	if (cfg.mode == BF_AUDIO_MODE_SILENT) {
		s.mode = BF_AUDIO_MODE_SILENT;
		BF_TRACE(BFT_SND, "audio: started silent, %d voices\n", s.max_voices);
		return s.mode;
	}
	@autoreleasepool {
		s.engine = [AVAudioEngine new];
		if (cfg.mode == BF_AUDIO_MODE_OFFLINE) {
			s.rate = cfg.offline_rate;
			NSError *err = nil;
			AVAudioFormat *f = standard(s.rate, 2);
			if (![s.engine enableManualRenderingMode:AVAudioEngineManualRenderingModeOffline format:f maximumFrameCount:4096 error:&err]) {
				BF_TRACE(BFT_SND, "audio: no offline rendering: %s\n", err.localizedDescription.UTF8String);
				teardown();
				return -1;
			}
			s.render_buf = [[AVAudioPCMBuffer alloc] initWithPCMFormat:s.engine.manualRenderingFormat frameCapacity:4096];
			if (!build_graph()) { teardown(); return -1; }
			s.mode = BF_AUDIO_MODE_OFFLINE;
		} else {
			AVAudioFormat *out = [s.engine.outputNode outputFormatForBus:0];
			if (out.sampleRate <= 0 || out.channelCount == 0) {
				go_silent("no output device");
				return s.mode;
			}
			s.rate = out.sampleRate;
			if (!build_graph()) { go_silent("the engine does not start"); return s.mode; }
			s.mode = BF_AUDIO_MODE_DEVICE;
			dispatch_queue_t q = s.queue;
			s.observer = [[NSNotificationCenter defaultCenter] addObserverForName:AVAudioEngineConfigurationChangeNotification
			        object:s.engine queue:nil usingBlock:^(NSNotification *) {
				dispatch_async(q, ^{ restart_after_change(); });
			}];
		}
	}
	BF_TRACE(BFT_SND, "audio: started %s at %.0f Hz, %d voices\n",
	         s.mode == BF_AUDIO_MODE_DEVICE ? "on the output device" : "offline", s.rate, s.max_voices);
	// A stream opened while stopped (SILENT before) keeps going.
	if (s.stream_file && s.stream_status == BF_AUDIO_STREAM_PLAYING) stream_begin();
	return s.mode;
}

void bf_audio_stop(void)
{
	State &s = S();
	Guard g(s.lock);
	if (s.mode < 0) return;
	@autoreleasepool {
		teardown();
	}
	s.stream_file = nil;
	s.stream_path.clear();
	s.stream_status = BF_AUDIO_STREAM_NONE;
	s.mode = -1;
	BF_TRACE(BFT_SND, "audio: stopped\n");
}

int bf_audio_mode(void)
{
	State &s = S();
	Guard g(s.lock);
	return s.mode;
}

int bf_audio_sample_rate(void)
{
	State &s = S();
	Guard g(s.lock);
	return (int)(s.rate + 0.5);
}

void bf_audio_set_effects_volume(float volume)
{
	State &s = S();
	Guard g(s.lock);
	s.fx_volume = clamp(volume, 0, 1);
	if (s.fx) s.fx.outputVolume = s.fx_volume;
}

float bf_audio_effects_volume(void)
{
	State &s = S();
	Guard g(s.lock);
	return s.fx_volume;
}

void bf_audio_set_music_volume(float volume)
{
	State &s = S();
	Guard g(s.lock);
	s.music_volume = clamp(volume, 0, 1);
	if (s.music) s.music.outputVolume = s.music_volume;
}

float bf_audio_music_volume(void)
{
	State &s = S();
	Guard g(s.lock);
	return s.music_volume;
}

int64_t bf_audio_render(float *dst, int64_t frames)
{
	State &s = S();
	Guard g(s.lock);
	if (s.mode != BF_AUDIO_MODE_OFFLINE || !dst || frames < 0) return -1;
	int64_t done = 0;
	@autoreleasepool {
		while (done < frames) {
			AVAudioFrameCount n = (AVAudioFrameCount)((frames - done) > 4096 ? 4096 : (frames - done));
			NSError *err = nil;
			AVAudioEngineManualRenderingStatus st = [s.engine renderOffline:n toBuffer:s.render_buf error:&err];
			if (st != AVAudioEngineManualRenderingStatusSuccess) {
				BF_TRACE(BFT_SND, "audio: offline rendering failed (status %d)\n", (int)st);
				return done > 0 ? done : -1;
			}
			const float *l = s.render_buf.floatChannelData[0];
			const float *r = s.render_buf.floatChannelData[s.render_buf.format.channelCount > 1 ? 1 : 0];
			AVAudioFrameCount got = s.render_buf.frameLength;
			for (AVAudioFrameCount i = 0; i < got; ++i) {
				dst[2 * (done + i)] = l[i];
				dst[2 * (done + i) + 1] = r[i];
			}
			done += got;
			stream_update();
			if (got == 0) break;
		}
	}
	return done;
}

bf_audio_sound *bf_audio_sound_create(const int16_t *samples, int64_t frames, int channels, int sample_rate)
{
	if ((!samples && frames > 0) || frames < 0 || frames > (int64_t)1 << 31 || channels < 1 || channels > 2
	    || sample_rate < 1000 || sample_rate > 192000) {
		BF_TRACE(BFT_SND, "audio: sound with invalid arguments (%lld frames, %d channels, %d Hz)\n",
		         (long long)frames, channels, sample_rate);
		return nullptr;
	}
	@autoreleasepool {
		AVAudioFrameCount cap = frames > 0 ? (AVAudioFrameCount)frames : 1;   // an empty sound is one silent frame
		AVAudioPCMBuffer *buf = [[AVAudioPCMBuffer alloc] initWithPCMFormat:standard(sample_rate, channels) frameCapacity:cap];
		if (!buf) return nullptr;
		buf.frameLength = cap;
		for (int c = 0; c < channels; ++c) {
			float *d = buf.floatChannelData[c];
			if (frames == 0) { d[0] = 0; continue; }
			for (int64_t i = 0; i < frames; ++i) d[i] = samples[i * channels + c] * (1.0f / 32768.0f);
		}
		bf_audio_sound *s = new bf_audio_sound;
		s->buf = buf;
		s->cached = nil;
		s->cached_rate = 0;
		s->frames = cap;
		s->rate = sample_rate;
		s->channels = channels;
		return s;
	}
}

bf_audio_sound *bf_audio_sound_create_from_memory(const void *data, size_t size)
{
	bf_audio_info info;
	int64_t frames = 0;
	int16_t *pcm = bf_audio_decode_alloc(data, size, &info, &frames);
	if (!pcm) return nullptr;
	bf_audio_sound *s = nullptr;
	if (info.channels <= 2)
		s = bf_audio_sound_create(pcm, frames, info.channels, info.sample_rate);
	else
		BF_TRACE(BFT_SND, "audio: %d-channel sound not supported\n", info.channels);
	bf_audio_free(pcm);
	return s;
}

void bf_audio_sound_destroy(bf_audio_sound *sound)
{
	if (!sound) return;
	State &s = S();
	Guard g(s.lock);
	@autoreleasepool {
		sound->buf = nil;   // a player node that still has it queued keeps its own reference
		sound->cached = nil;
	}
	delete sound;
}

double bf_audio_sound_duration(const bf_audio_sound *sound)
{
	return sound ? sound->frames / sound->rate : 0;
}

int64_t bf_audio_sound_frames(const bf_audio_sound *sound)
{
	return sound ? sound->frames : 0;
}

bf_audio_voice bf_audio_play(bf_audio_sound *sound, float volume, float pan, int loop)
{
	State &s = S();
	Guard g(s.lock);
	if (s.mode < 0 || !sound) {
		s.c.voices_rejected++;
		BF_TRACE(BFT_SND, "audio: play refused: %s\n", s.mode < 0 ? "not started" : "no sound");
		return 0;
	}
	if (s.mode == BF_AUDIO_MODE_SILENT) {
		int i = s.silent_next++ % s.max_voices;
		Voice &v = s.voices[i];
		v.gen++;
		v.active = false;
		s.c.voices_started++;
		bf_count("audio.voices_started", 1);
		return handle_of(i, v.gen);
	}
	int slot = -1, oldest = -1;
	for (int i = 0; i < s.max_voices; ++i) {
		Voice &v = s.voices[i];
		if (voice_done(v)) { slot = i; break; }
		if (!v.loop && (oldest < 0 || v.seq < s.voices[oldest].seq)) oldest = i;
	}
	bool stolen = false;
	if (slot < 0) {
		if (oldest < 0) {
			s.c.voices_rejected++;
			bf_count("audio.voices_rejected", 1);
			BF_TRACE(BFT_SND, "audio: play refused: all %d voices are looping\n", s.max_voices);
			return 0;
		}
		slot = oldest;
		stolen = true;
	}
	Voice &v = s.voices[slot];
	@autoreleasepool {
		AVAudioPCMBuffer *buf = engine_buffer(sound);
		if (!buf) {
			s.c.voices_rejected++;
			return 0;
		}
		[v.node stop];   // also resets the player's timeline, which voice_done reads
		v.node = sound->channels == 1 ? v.mono : v.stereo;
		[v.node stop];
		v.gen++;
		if (v.gen >= (1u << 26)) v.gen = 1;   // the handle keeps 26 bits of it
		v.finished.store(0);
		v.active = true;
		v.loop = loop != 0;
		v.seq = ++s.seq;
		v.length = buf.frameLength;
		v.node.volume = clamp(volume, 0, 1);
		v.node.pan = clamp(pan, -1, 1);
		uint32_t gen = v.gen;
		std::atomic<uint32_t> *fin = &v.finished;
		[v.node scheduleBuffer:buf atTime:nil options:(loop ? AVAudioPlayerNodeBufferLoops : 0)
		completionCallbackType:AVAudioPlayerNodeCompletionDataRendered
		     completionHandler:^(AVAudioPlayerNodeCompletionCallbackType) { mark_finished(*fin, gen); }];
		[v.node play];
	}
	s.c.voices_started++;
	bf_count("audio.voices_started", 1);
	if (stolen) {
		s.c.voices_stolen++;
		bf_count("audio.voices_stolen", 1);
	}
	return handle_of(slot, v.gen);
}

void bf_audio_voice_stop(bf_audio_voice voice)
{
	State &s = S();
	Guard g(s.lock);
	Voice *v = voice_of(voice);
	if (!v) return;
	[v->node stop];
	v->active = false;
}

int bf_audio_voice_playing(bf_audio_voice voice)
{
	State &s = S();
	Guard g(s.lock);
	return voice_of(voice) != nullptr;
}

void bf_audio_voice_set_volume(bf_audio_voice voice, float volume)
{
	State &s = S();
	Guard g(s.lock);
	if (Voice *v = voice_of(voice)) v->node.volume = clamp(volume, 0, 1);
}

void bf_audio_voice_set_pan(bf_audio_voice voice, float pan)
{
	State &s = S();
	Guard g(s.lock);
	if (Voice *v = voice_of(voice)) v->node.pan = clamp(pan, -1, 1);
}

void bf_audio_stop_all_voices(void)
{
	State &s = S();
	Guard g(s.lock);
	if (s.mode < 0) return;
	for (int i = 0; i < s.max_voices; ++i) {
		Voice &v = s.voices[i];
		if (v.active) [v.node stop];
		v.active = false;
	}
}

int bf_audio_voices_playing(void)
{
	State &s = S();
	Guard g(s.lock);
	if (s.mode < 0) return 0;
	int n = 0;
	for (int i = 0; i < s.max_voices; ++i)
		if (!voice_done(s.voices[i])) n++;
	return n;
}

int bf_audio_stream_open(const char *path)
{
	State &s = S();
	Guard g(s.lock);
	bf_audio_stream_close();
	if (!path || !*path) {
		s.c.stream_failures++;
		BF_TRACE(BFT_SND, "audio: stream with no path\n");
		return -1;
	}
	@autoreleasepool {
		NSString *p = [NSString stringWithUTF8String:path];
		NSError *err = nil;
		AVAudioFile *f = p ? [[AVAudioFile alloc] initForReading:[NSURL fileURLWithPath:p] error:&err] : nil;
		if (!f || f.length <= 0) {
			s.c.stream_failures++;
			bf_count("audio.stream_failures", 1);
			BF_TRACE(BFT_SND, "audio: cannot open stream %s: %s\n", path,
			         err ? err.localizedDescription.UTF8String : "empty or not UTF-8");
			return -1;
		}
		s.stream_file = f;
	}
	s.stream_path = path;
	s.stream_status = BF_AUDIO_STREAM_OPEN;
	s.loop_count = 1;
	s.loops_done = 0;
	s.c.streams_opened++;
	bf_count("audio.streams_opened", 1);
	BF_TRACE(BFT_SND, "audio: stream %s opened (%.1f s)\n", path, bf_audio_stream_duration());
	return 0;
}

void bf_audio_stream_set_loop_count(int count)
{
	State &s = S();
	Guard g(s.lock);
	s.loop_count = count < 0 ? 0 : count;
}

int bf_audio_stream_start(void)
{
	State &s = S();
	Guard g(s.lock);
	if (!s.stream_file) {
		BF_TRACE(BFT_SND, "audio: stream start without a file\n");
		return -1;
	}
	@autoreleasepool {
		stream_begin();
	}
	return 0;
}

void bf_audio_stream_pause(int paused)
{
	State &s = S();
	Guard g(s.lock);
	if (paused && s.stream_status == BF_AUDIO_STREAM_PLAYING) {
		[s.stream_node pause];
		s.stream_status = BF_AUDIO_STREAM_PAUSED;
	} else if (!paused && s.stream_status == BF_AUDIO_STREAM_PAUSED) {
		[s.stream_node play];
		s.stream_status = BF_AUDIO_STREAM_PLAYING;
	}
}

void bf_audio_stream_close(void)
{
	State &s = S();
	Guard g(s.lock);
	s.stream_gen++;
	s.stream_gen_now = s.stream_gen;
	if (s.stream_node) [s.stream_node stop];
	s.stream_file = nil;
	s.stream_path.clear();
	s.stream_status = BF_AUDIO_STREAM_NONE;
	s.loops_done = 0;
}

void bf_audio_stream_set_volume(float volume)
{
	State &s = S();
	Guard g(s.lock);
	s.stream_volume = clamp(volume, 0, 1);
	if (s.stream_node) s.stream_node.volume = s.stream_volume;
}

float bf_audio_stream_volume(void)
{
	State &s = S();
	Guard g(s.lock);
	return s.stream_volume;
}

int bf_audio_stream_status(void)
{
	State &s = S();
	Guard g(s.lock);
	stream_update();
	return s.stream_status;
}

double bf_audio_stream_duration(void)
{
	State &s = S();
	Guard g(s.lock);
	if (!s.stream_file) return 0;
	return (double)s.stream_file.length / s.stream_file.processingFormat.sampleRate;
}

void bf_audio_get_counters(bf_audio_counters *out)
{
	if (!out) return;
	State &s = S();
	Guard g(s.lock);
	stream_update();
	memset(out, 0, sizeof *out);
	out->mode = s.mode;
	out->max_voices = s.mode >= 0 ? s.max_voices : 0;
	out->voices_playing = bf_audio_voices_playing();
	out->voices_started = s.c.voices_started;
	out->voices_stolen = s.c.voices_stolen;
	out->voices_rejected = s.c.voices_rejected;
	out->conversions = s.c.conversions;
	out->decodes = bf_audio_n_decodes.load();
	out->decode_failures = bf_audio_n_decode_failures.load();
	out->decode_fallbacks = bf_audio_n_decode_fallbacks.load();
	out->device_failures = s.c.device_failures;
	out->engine_restarts = s.c.engine_restarts;
	out->streams_opened = s.c.streams_opened;
	out->stream_failures = s.c.stream_failures;
	out->stream_status = s.stream_status;
	out->stream_loops = s.loops_done;
	out->stream_duration = bf_audio_stream_duration();
	strncpy(out->stream_file, s.stream_path.c_str(), sizeof out->stream_file - 1);
}

void bf_audio_reset_counters(void)
{
	State &s = S();
	Guard g(s.lock);
	s.c = Counters();
	bf_audio_n_decodes = 0;
	bf_audio_n_decode_failures = 0;
	bf_audio_n_decode_fallbacks = 0;
}

} // extern "C"
