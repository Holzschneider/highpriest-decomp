#include "common.h"








#include "../pdb_shared.h"
#include "../pdb_enums.h"
#include "Sound.h"
#include "../3dlib/Prim.h"

void CloseStream();


inline CMssVariables::CMssVariables()
{
	numSamples = MAXSAMPLE;
	num3DSamples = MAX3DSAMPLE;
	for (int i = 0; i < MAXSAMPLE; ++i)
		hSamples[i] = 0;
	provider_default = 0;
	for (i = 0; i < MAX_PROVIDERS; ++i) {
		provider_names[i] = NULL;
		provider_handles[i] = 0;
	}
	enum3D = HPROENUM_FIRST;
	for (i = 0; i < MAX3DSAMPLE; ++i)
		h3DSamples[i] = 0;
	hListener = 0;
	usingEAX = 0;
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

	AIL_set_redist_directory(".\\");
	AIL_startup();

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

	AIL_set_preference(DIG_USE_WAVEOUT, g_registry.m_useWaveout);
	g_mssVar.hDigital = AIL_open_digital_driver(g_mssVar.digitalrate,
												g_mssVar.digitalbits,
												g_mssVar.digitalchannels,
												AIL_OPEN_DIGITAL_FORCE_PREFERENCE);
	if (g_mssVar.hDigital == 0)
		return false;

	if (g_mssVar.hDigitalStream) {
		AIL_close_stream(g_mssVar.hDigitalStream);
		g_mssVar.hDigitalStream = 0;
	}

	if (g_registry.m_soundMode == 1) {
		for (int i = 0; i < g_mssVar.numSamples; i++) {
			g_mssVar.hSamples[i] = AIL_allocate_sample_handle(g_mssVar.hDigital);
			if (g_mssVar.hSamples[i] == 0)
				break;
		}
	} else if (g_registry.m_soundMode == 2) {
		unsigned int provider_count = 0;
		strcpy(g_registry.m_providerName, "Miles Fast 2D Positional Audio");
		while (AIL_enumerate_3D_providers(&g_mssVar.enum3D,
										  &g_mssVar.provider_handles[provider_count],
										  &g_mssVar.provider_names[provider_count])) {
			if (strcmp(g_mssVar.provider_names[provider_count], g_registry.m_providerName) == 0) {
				Trace("3d Audio Success!!");
				g_mssVar.provider_default = g_mssVar.provider_handles[provider_count];
				break;
			}
			provider_count++;
		}
		if (g_mssVar.provider_default) {
			if (AIL_open_3D_provider(g_mssVar.provider_default))
				return false;
			if (AIL_3D_speaker_type(g_mssVar.provider_default) != -1)
				AIL_set_3D_speaker_type(g_mssVar.provider_default, g_registry.m_speakerType);
			g_mssVar.hListener = AIL_open_3D_listener(g_mssVar.provider_default);
			if (g_mssVar.hListener == 0)
				return false;
			for (int i = 0; i < g_mssVar.num3DSamples; i++) {
				g_mssVar.h3DSamples[i] = AIL_allocate_3D_sample_handle(g_mssVar.provider_default);
				if (g_mssVar.h3DSamples[i] == 0)
					break;
			}
		}
	}
	return true;
}

void UnInitMSS()
{
	if (g_mssVar.provider_default) {
		for (int i = 0; i < MAX3DSAMPLE; i++) {
			if (g_mssVar.h3DSamples[i]) {
				AIL_release_3D_sample_handle(g_mssVar.h3DSamples[i]);
				g_mssVar.h3DSamples[i] = 0;
			}
		}
		AIL_close_3D_listener(g_mssVar.hListener);
		AIL_close_3D_provider(g_mssVar.provider_default);
		g_mssVar.provider_default = 0;
	}

	CloseStream();

	for (int i = 0; i < MAXSAMPLE; i++) {
		if (g_mssVar.hSamples[i]) {
			AIL_release_sample_handle(g_mssVar.hSamples[i]);
			g_mssVar.hSamples[i] = 0;
		}
	}
	if (g_mssVar.hDigital) {
		AIL_close_digital_driver(g_mssVar.hDigital);
		g_mssVar.hDigital = 0;
	}

	AIL_shutdown();
}

void CloseStream()
{
	memset(g_mssVar.streamFileName, 0, sizeof(g_mssVar.streamFileName));
	if (g_mssVar.hDigitalStream) {
		AIL_close_stream(g_mssVar.hDigitalStream);
		g_mssVar.hDigitalStream = 0;
	}
}

int GetStreamVolume()
{
	return g_mssVar.streamVolume;
}

void SetStreamVolume(int volume)
{
	if (volume > 127)
		volume = 127;
	if (volume < 0)
		volume = 0;
	g_mssVar.streamVolume = volume;
	if (g_mssVar.hDigitalStream)
		AIL_set_stream_volume(g_mssVar.hDigitalStream, g_mssVar.streamVolume);
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
	if (g_mssVar.hDigitalStream)
		AIL_pause_stream(g_mssVar.hDigitalStream, bgmIsPaused);
	AIL_stream_volume(g_mssVar.hDigitalStream);
}

void PlayStream(const char* streamFileName)
{
	if (!g_soundMode)
		return;
	if (strcmp(streamFileName, g_mssVar.streamFileName) == 0)
		return;

	CloseStream();

	strcpy(g_mssVar.streamFileName, streamFileName);
	g_mssVar.hDigitalStream = AIL_open_stream(g_mssVar.hDigital, streamFileName, 0);
	sAssert(g_mssVar.hDigitalStream != NULL);

	AIL_set_stream_volume(g_mssVar.hDigitalStream, g_mssVar.streamVolume);
	AIL_set_stream_loop_count(g_mssVar.hDigitalStream, 0);
	AIL_start_stream(g_mssVar.hDigitalStream);
}

void EndDigitalSound()
{
	for (int i = 0; i < MAXSAMPLE; i++) {
		if (g_mssVar.hSamples[i]) {
			AIL_end_sample(g_mssVar.hSamples[i]);
		}
	}
	if (g_mssVar.provider_default) {
		for (int i = 0; i < MAX3DSAMPLE; i++) {
			if (g_mssVar.h3DSamples[i]) {
				AIL_end_3D_sample(g_mssVar.h3DSamples[i]);
			}
		}
	}
}

bool PlayDigital(BYTE* fileImage, float& x, float& y, float& z,
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
		if (g_mssVar.hSamples[i]) {
			if (AIL_sample_status(g_mssVar.hSamples[i]) == SMP_DONE) {
				AIL_init_sample(g_mssVar.hSamples[i]);
				AIL_set_sample_file(g_mssVar.hSamples[i], fileImage, 0);
				AIL_set_sample_volume(g_mssVar.hSamples[i], volume);
				AIL_start_sample(g_mssVar.hSamples[i]);
				return true;
			}
		}
	}
	Trace("Failed PlayDigital(BYTE *fileImage)");
	return false;
}

bool Play3DDigital(BYTE* fileImage, float& x, float& y, float& z,
				   int volumeMaxDist,
				   int volumeMinDist,
				   float vFactor)
{
	int volume = float(g_mssVar.digital3DVolume) * vFactor;
	if (g_mssVar.provider_default) {
		for (int i = 0; i < g_mssVar.num3DSamples; i++) {
			if (g_mssVar.h3DSamples[i]) {
				if (AIL_3D_sample_status(g_mssVar.h3DSamples[i]) == SMP_DONE) {
					if (AIL_set_3D_sample_file(g_mssVar.h3DSamples[i], fileImage) == 0)
						return false;

					if (g_mssVar.roomType == ENVIRONMENT_GENERIC)
						AIL_set_3D_sample_effects_level(g_mssVar.h3DSamples[i], 0.0f);
					else
						AIL_set_3D_sample_effects_level(g_mssVar.h3DSamples[i], 1.0f);

					AIL_set_3D_position(g_mssVar.h3DSamples[i], x, y, z);
					AIL_set_3D_orientation(g_mssVar.h3DSamples[i], -x, -y, -z, 0.0f, 1.0f, 0.0f);
					AIL_set_3D_sample_volume(g_mssVar.h3DSamples[i], volume);
					AIL_set_3D_sample_distances(g_mssVar.h3DSamples[i], volumeMaxDist, volumeMinDist);
					AIL_start_3D_sample(g_mssVar.h3DSamples[i]);
					return true;
				}
			}
		}
	} else {
		if (PlayDigital(fileImage, x, y, z))
			return true;
	}
	return false;
}

void SetRoomEffect(int val)
{
	if (g_mssVar.provider_default) {
		int result = AIL_3D_room_type(g_mssVar.provider_default);
		g_mssVar.usingEAX = (result != -1) ? 1 : 0;
		if (g_mssVar.provider_default && g_mssVar.usingEAX)
			AIL_set_3D_room_type(g_mssVar.provider_default, val);
	}
}

void SetListenerOrient(float roty)
{
	roty = 180 - roty;
	float vx = GetSin(roty);
	float vz = -GetCos(roty);
	if (g_mssVar.hListener) {
		AIL_set_3D_orientation(g_mssVar.hListener, vx, 0, vz, 0.0f, 1.0f, 0.0f);
	}
}
