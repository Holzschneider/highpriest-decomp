#ifndef HIGHPRIEST_RAGEFFECTPRIM_H
#define HIGHPRIEST_RAGEFFECTPRIM_H

#include "RagEffect.h"

class CWeather;
struct WeatherInfo;

struct WeatherInfo {
public:
	std::string map;
	std::string weather;
};

class CWeather {
public:
	CWeather();
	~CWeather();
	vector3d GetEnvDiff() { return m_EnvDiff; }
	vector3d GetAmbient() { return m_Ambient; }
	unsigned char GetNight() { return m_bNight; }
	void SetAmbient(vector3d v) { m_Ambient = v; }
	void SetDiffuse(vector3d v) { m_Diffuse = v; }
	void SetMaxDiffuse(vector3d v) { m_MaxDiffuse = v; }
	void SetObjLight(unsigned char objLight) { m_bObjLight = objLight; }
	void SetNightDegree();
	unsigned char IsObjLight() { return m_bObjLight; }
	void Process();
	void StopSnow();
	void StopNight();
	void StopSakura();
	void StopMaple();
	void LaunchSnow();
	void LaunchSakura();
	void LaunchNight();
	void LaunchMaple();
	void LaunchCloud(char F1);
	void LaunchPokJuk();
	void Reset();
	vector3d GetSunWorldLoc();
	void ScriptProcess(std::string map);
	void AddScript(std::string map, std::string weather);
	void LaunchBackGroundTexture(char F1);
private:
	unsigned char m_bNight;
	unsigned char m_bObjLight;
	std::vector<WeatherInfo> m_weatherInfo;
	CPlayer* m_pPlayer;
	vector3d m_Diffuse;
	vector3d m_MaxDiffuse;
	vector3d m_Ambient;
	vector3d m_EnvDiff;
	CRagEffect* m_Effect;
	bf_clock_date m_st;
	int m_nDayCount;
};
extern CWeather g_Weather;

extern CWeather g_Weather;

#endif
