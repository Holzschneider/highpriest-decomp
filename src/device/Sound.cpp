#include "common.h"








#include "../pdb_shared.h"
#include "../pdb_enums.h"
#include "Sound.h"
#include "../3dlib/Prim.h"
#include "bf_file.h"
#include <mutex>

void CloseStream();

// The sound module (port/native/audio, bf_audio.h) in place of Miles (M11.6).
// What the Miles adapter of stage A (port/compat/miles) decided is done here:
// a sample handle is a CMssSample whose voice is done when the module no
// longer plays it; volumes are Miles' 0..127 over 127; there is no 3D
// provider, so Play3DDigital falls back to PlayDigital; the music file is
// found in the game folder by its client name. The sound a wave decodes to is
// kept by the wave (resource/Wave.cpp).

static void StopSample(CMssSample &sample)
{
	if (sample.voice)
		bf_audio_voice_stop(sample.voice);
	sample.voice = 0;
}

static float UnitVolume(int volume)
{
	if (volume < 0)
		volume = 0;
	if (volume > 127)
		volume = 127;
	return float(volume) / 127.0f;
}

// Miles' open digital driver: the module on the output device. A module that
// already runs (the intro movie's, CBink.cpp, or a test's) is adopted, and one
// that fell back to SILENT is still a driver, so that voices and the stream
// keep being counted.
static bool OpenDigitalDriver()
{
	if (bf_audio_mode() < 0) {
		bf_audio_config config = { BF_AUDIO_MODE_DEVICE, BF_AUDIO_DEFAULT_VOICES, 0 };
		int mode = bf_audio_start(&config);
		if (mode < 0) {
			BF_TRACE(BFT_SND, "sound: open digital driver: the sound engine did not start");
			return false;
		}
		BF_TRACE(BFT_SND, "sound: open digital driver(%d, %d, %d): engine mode %d at %d Hz",
				 g_mssVar.digitalrate, g_mssVar.digitalbits, g_mssVar.digitalchannels, mode, bf_audio_sample_rate());
	}
	return true;
}

// The client's name ("bgm\\01.mp3") relative to the game folder, in any letter
// case (BGM/). A file that is not there is still handed to the module, so that
// it counts the failure.
static bool OpenStream(const char *streamFileName)
{
	char path[2048];
	if (bf_file_resolve(streamFileName, BF_RESOLVE_READ, path, sizeof path) < 0) {
		const char *game = bf_file_location(BF_LOCATION_GAME);
		size_t n = 0;
		if (streamFileName[0] != '/')
			n = snprintf(path, sizeof path, "%s/", game ? game : ".");
		for (const char *c = streamFileName; *c && n + 1 < sizeof path; ++c)
			path[n++] = *c == '\\' ? '/' : *c;
		path[n] = 0;
	}
	if (!g_mssVar.hDigital) {
		BF_TRACE(BFT_SND, "sound: open stream(%s) with no driver open", streamFileName);
		return false;
	}
	if (bf_audio_stream_open(path) != 0) {
		BF_TRACE(BFT_SND, "sound: open stream: %s (%s) cannot be opened", streamFileName, path);
		return false;
	}
	return true;
}

// The fade thread (FadeBgm in Mode.cpp) and the main thread both use the
// stream and its volume. Recursive: PlayStream calls CloseStream.
static std::recursive_mutex g_streamLock;


inline CMssVariables::CMssVariables()
{
	numSamples = MAXSAMPLE;
	num3DSamples = MAX3DSAMPLE;
	int i;
	for (i = 0; i < MAXSAMPLE; ++i) {
		hSamples[i].allocated = false;
		hSamples[i].voice = 0;
		hSamples[i].volume = 127;
	}
	roomType = ENVIRONMENT_GENERIC;
	streamVolume = 100;
	digital2DVolume = 100;
	digital3DVolume = 100;
	memset(streamFileName, 0, sizeof(streamFileName));
	volumeMaxDist = VOLUMEMAXDIST;
	volumeMinDist = VOLUMEMINDIST;
	digitalrate = 22050;
	digitalbits = 16;
	digitalchannels = 2;
}

extern Registry2     g_registry;
extern CMssVariables g_mssVar;

unsigned int g_soundMode = 2;
CMssVariables g_mssVar;






bool InitMSS()
{
	switch (g_registry.m_numSampleType) {
		case 0:
			g_mssVar.numSamples   = MAXSAMPLE;
			g_mssVar.num3DSamples = MAX3DSAMPLE;
			break;
		case 1:
			g_mssVar.numSamples   = 32;
			g_mssVar.num3DSamples = 32;
			break;
		case 2:
			g_mssVar.numSamples   = 16;
			g_mssVar.num3DSamples = 16;
			break;
		default:
			g_mssVar.numSamples   = 16;
			g_mssVar.num3DSamples = 16;
			break;
	}

	g_mssVar.volumeMaxDist = VOLUMEMAXDIST;
	g_mssVar.volumeMinDist = VOLUMEMINDIST;

	sAssert(g_mssVar.hDigital == 0);

	if (g_registry.m_soundMode == 1)
		g_mssVar.digitalchannels = 1;

	switch (g_registry.m_digitalrateType) {
		case 0:  g_mssVar.digitalrate = 22050; break;
		case 1:  g_mssVar.digitalrate = 11025; break;
		case 2:  g_mssVar.digitalrate = 8000;  break;
		default: g_mssVar.digitalrate = 8000;  break;
	}

	switch (g_registry.m_digitalbitsType) {
		case 0:  g_mssVar.digitalbits = 16; break;
		case 1:  g_mssVar.digitalbits = 8;  break;
		default: g_mssVar.digitalbits = 8;  break;
	}

	g_mssVar.hDigital = OpenDigitalDriver();
	if (!g_mssVar.hDigital)
		return false;

	if (g_mssVar.hDigitalStream) {
		bf_audio_stream_close();
		g_mssVar.hDigitalStream = false;
	}

	if (g_registry.m_soundMode == 1) {
		for (int i = 0; i < g_mssVar.numSamples; i++) {
			g_mssVar.hSamples[i].allocated = true;
			g_mssVar.hSamples[i].voice = 0;
			g_mssVar.hSamples[i].volume = 127;
		}
	} else if (g_registry.m_soundMode == 2) {
		// No 3D provider here (2008: "Miles Fast 2D Positional Audio"), so no
		// 3D handles; and no 2D handles either in this mode, as in 2008.
		strcpy(g_registry.m_providerName, "Miles Fast 2D Positional Audio");
	}
	return true;
}

void UnInitMSS()
{
	CloseStream();

	for (int i = 0; i < MAXSAMPLE; i++) {
		if (g_mssVar.hSamples[i].allocated) {
			StopSample(g_mssVar.hSamples[i]);
			g_mssVar.hSamples[i].allocated = false;
		}
	}
	if (g_mssVar.hDigital) {
		bf_audio_stop();	// whoever started it
		g_mssVar.hDigital = false;
	}
}

void CloseStream()
{
	std::lock_guard<std::recursive_mutex> lock(g_streamLock);
	memset(g_mssVar.streamFileName, 0, sizeof(g_mssVar.streamFileName));
	if (g_mssVar.hDigitalStream) {
		bf_audio_stream_close();
		g_mssVar.hDigitalStream = false;
	}
}

int GetStreamVolume()
{
	std::lock_guard<std::recursive_mutex> lock(g_streamLock);
	return g_mssVar.streamVolume;
}

void SetStreamVolume(int volume)
{
	std::lock_guard<std::recursive_mutex> lock(g_streamLock);
	if (volume > 127)
		volume = 127;
	if (volume < 0)
		volume = 0;
	g_mssVar.streamVolume = volume;
	if (g_mssVar.hDigitalStream)
		bf_audio_stream_set_volume(UnitVolume(g_mssVar.streamVolume));
}

int Get2DVolume()
{
	return g_mssVar.digital2DVolume;
}

void Set2DVolume(int volume)
{
	if (volume > 127)
		volume = 127;
	if (volume < 0)
		volume = 0;
	g_mssVar.digital2DVolume = volume;
}

int Get3DVolume()
{
	return g_mssVar.digital3DVolume;
}

void Set3DVolume(int volume)
{
	if (volume > 127)
		volume = 127;
	if (volume < 0)
		volume = 0;
	g_mssVar.digital3DVolume = volume;
}

void PauseStream(int bgmIsPaused)
{
	std::lock_guard<std::recursive_mutex> lock(g_streamLock);
	if (g_mssVar.hDigitalStream)
		bf_audio_stream_pause(bgmIsPaused ? 1 : 0);
}

void PlayStream(const char* streamFileName)
{
	if (!g_soundMode)
		return;
	std::lock_guard<std::recursive_mutex> lock(g_streamLock);
	if (strcmp(streamFileName, g_mssVar.streamFileName) == 0)
		return;

	CloseStream();

	strcpy(g_mssVar.streamFileName, streamFileName);
	g_mssVar.hDigitalStream = OpenStream(streamFileName);
	sAssert(g_mssVar.hDigitalStream);

	if (g_mssVar.hDigitalStream) {
		bf_audio_stream_set_volume(UnitVolume(g_mssVar.streamVolume));
		bf_audio_stream_set_loop_count(0);	// forever
		bf_audio_stream_start();
	}
}

void EndDigitalSound()
{
	for (int i = 0; i < MAXSAMPLE; i++) {
		if (g_mssVar.hSamples[i].allocated) {
			StopSample(g_mssVar.hSamples[i]);
		}
	}
}

bool PlayDigital(bf_audio_sound* sound, float& x, float& y, float& z,
				 int volumeMaxDist,
				 int volumeMinDist,
				 float vFactor)
{
	float xz   = _sqrt(x * x + z * z);
	float dist = _sqrt(y * y + xz * xz);

	int volume = g_mssVar.digital2DVolume;
	if (dist >= volumeMaxDist) {
		volume = 0;
		return false;
	} else if (dist <= volumeMinDist) {
		volume = g_mssVar.digital2DVolume;
	} else {
		volume = volume * float(volumeMinDist) / dist;
	}

	volume = float(volume) * vFactor;

	for (int i = 0; i < g_mssVar.numSamples; i++) {
		CMssSample &sample = g_mssVar.hSamples[i];
		if (sample.allocated) {
			if (!bf_audio_voice_playing(sample.voice)) {
				StopSample(sample);
				sample.volume = volume;
				// a wave that cannot be decoded takes the handle and plays nothing
				if (sound)
					sample.voice = bf_audio_play(sound, UnitVolume(sample.volume), 0.0f, 0);
				return true;
			}
		}
	}
	Trace("Failed PlayDigital(BYTE *fileImage)");
	return false;
}

bool Play3DDigital(bf_audio_sound* sound, float& x, float& y, float& z,
				   int volumeMaxDist,
				   int volumeMinDist,
				   float vFactor)
{
	// no 3D provider: PlayDigital at its own distances, as in 2008 without one
	if (PlayDigital(sound, x, y, z))
		return true;
	return false;
}

void SetRoomEffect(int val)
{
	// rooms are a 3D provider's (EAX); there is none
}

void SetListenerOrient(float roty)
{
	// the listener is a 3D provider's; there is none
}
