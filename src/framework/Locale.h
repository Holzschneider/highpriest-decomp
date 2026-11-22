#ifndef HIGHPRIEST_FRAMEWORK_LOCALE_H
#define HIGHPRIEST_FRAMEWORK_LOCALE_H



typedef enum {
	ServiceKorea = 0,
	ServiceAmerica = 1,
	ServiceJapan = 2,
	ServiceChina = 3,
	ServiceTaiwan = 4,
	ServiceThai = 5,
	ServiceIndonesia = 6,
	ServicePhilippine = 7,
	ServiceMalaysia = 8,
	ServiceSingapore = 9,
	ServiceGermany = 10,
	ServiceIndia = 11,
	ServiceBrazil = 12,
	ServiceAustralia = 13,
	ServiceRussia = 14,
	ServiceVietnam = 15,
	ServiceIndoor = 16,
	ServiceChile = 17,
	ServiceFrance = 18,
	ServiceLast = 19,
} ServiceType;

typedef enum {
	ServerPrimary = 0,
	ServerSakray = 1,
	ServerLocal = 2,
	ServerInstantEvent = 3,
	ServerPVP = 4,
	ServerPK = 5,
	ServerSakPVP = 6,
	ServerSakPK = 7,
	ServerEvent = 8,
	ServerPrimary2 = 9,
	ServerTest = 10,
	ServerSakray2 = 11,
	ServerSakTest = 12,
	ServerADTest = 13,
	ServerEC = 14,
	ServerSakEC = 15,
	ServerLast = 16,
} ServerType;

extern ServiceType g_serviceType;
extern ServerType g_serverType;

#endif
