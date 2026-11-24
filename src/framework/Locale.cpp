#include "common.h"



#include "../base/RegMgr.h"
#include "../Session.h"
#include "Language.h"
#include "Locale.h"
#include "../base/Res.h"
#include "../View.h"
#include "../GameWorld.h"
#include "../GameMode.h"
#include "../GameActor.h"
#include "../3dlib/Prim.h"
#include "../resource/Attr.h"
#include "../resource/Wave.h"
#include "../device/Sound.h"
#include "../3dActor.h"
#include "../packet.h"
#include "../base/Util.h"
#include "../RagEffect.h"
#include "../RagEffectPrim.h"
#include "../base/xml.h"

#include <algorithm>
#include <vector>
#include <string>
#include <stdio.h>
#include <string.h>
#include <windows.h>





const char*               g_licence      = "";
int                       g_version         = 0;
const char*               g_accountAddr     = "";
const char*               g_accountPort     = "";
const char*               g_regstrationWeb  = "";
const char*               PATCH_CLIENT   = "";
const char*               PATCH_UP       = "";
const char*               PLEASE_EXCUTE_LAUNCHER = "";
const char*               WINDOW_NAME    = "";
const char*               g_regPath      = "";
const char*               TITLE_FILE     = "";
bool                      g_hideAccountList = false;
bool                      g_passwordEncrypt = false;
bool                      g_passwordEncrypt2 = false;
char                      g_passwordWrong = 0;
bool                      g_extended_slot = false;
bool                      g_multiLang    = false;
bool                      g_sendUDPPacket = false;
char                      title_random;
bool                      NameBalloonShorten  = false;
bool                      NameBalloonfontBold = false;
int                       NameBalloonfontSize = 12;




ServiceType               g_serviceType  = ServiceKorea;
LanguageType              g_languageType = LanguageKorean;
ServerType                g_serverType   = ServerSakray;
DWORD                     dwAdminAID[] = {
	100118, 2750446, 2750961, 2750963, 2750964, 2750966, 2750967, 2750969,
	2750971, 2750972, 2750973, 3143679, 3143680, 3143681, 3143682, 3143684,
	3143685, 3143686, 3143687, 3143688, 3143689, 3143690, 3143691, 3143692,
	3143693, 3143695, 3143696, 3143699, 3143700, 3143701, 3143702, 2919557,
	3304372, 3376608, 4663634, 4663643, 4663644, 4663633, 4663642, 4663649,
	4663632, 4663638, 4663645, 4841414, 4841415, 4841416, 4841422, 5046468,
	5046469, 5046470, 5046471, 5046472, 5046473, 5046474, 5046475, 5046476,
	5046477, 5046478, 5046479, 5046480
};
DWORD                     dwYellowAID[] = {
	100118, 2750446, 2750961, 2750963, 2750964, 2750966, 2750967, 2750969,
	2750971, 2750972, 2750973, 3143679, 3143680, 3143681, 3143682, 3143684,
	3143685, 3143686, 3143687, 3143688, 3143689, 3143690, 3143691, 3143692,
	3143693, 3143695, 3143696, 3143699, 3143700, 3143701, 3143702, 2919557,
	3304372, 3304363, 3304378, 3304372, 3376608, 4663634, 4663643, 4663644,
	4663633, 4663642, 4663649, 4663632, 4663638, 4663645, 4841414, 4841415,
	4841416, 4841422, 5046468, 5046469, 5046470, 5046471, 5046472, 5046473,
	5046474, 5046475, 5046476, 5046477, 5046478, 5046479, 5046480
};
DWORD                     g_codePage;
CLanguage*                g_language;
XMLDocument               document;
std::vector<DWORD>        s_dwAdminAID;
std::vector<DWORD>        s_dwYellowAID;
std::vector<std::string>  s_loadingScreenList;
extern char               g_EXE_way;

extern bool g_readFolderFirst;


void InitLanguage();
void InitDefaultServiceType();
void SelectKoreaClientInfo();
void SelectClientInfo(int number);
void InitDefaultLoadingScreen(int count = 0);


void InitClientInfo(const char* clientInfo)
{
	CFile        fp;
	SYSTEMTIME   sTime;

	if (fp.Open(clientInfo, CFile::modeRead))
	{
		document.ReadDocument((const char*)fp.GetBuf(),
							  (const char*)fp.GetBuf() + fp.GetLength());
		fp.Close();
	}
#if defined(BF_PORT)
	else
	{
		BF_TRACE(BFT_FILE, "clientinfo: %s not found; built-in defaults", clientInfo);
		bf_count("file.missing.clientinfo", 1);
	}
#endif

	InitDefaultServiceType();

	switch (g_serviceType)
	{
	case ServiceKorea:
		SelectKoreaClientInfo();
		break;
	case ServiceAmerica:
	case ServiceJapan:
	case ServiceChina:
	case ServiceTaiwan:
	case ServiceThai:
	case ServiceIndonesia:
	case ServicePhilippine:
	case ServiceMalaysia:
	case ServiceSingapore:
	case ServiceGermany:
	case ServiceIndia:
	case ServiceBrazil:
	case ServiceAustralia:
	case ServiceRussia:
	case ServiceVietnam:
	case ServiceChile:
	case ServiceFrance:
		SelectClientInfo(0);
		break;
	default:
		ErrorMsg("Unknown ServiceType !!!");
		break;
	}
#if defined(BF_PORT)
	BF_TRACE(BFT_FILE, "clientinfo: %s parsed: service type %d, server type %d, account server %s:%s, readfolder %d",
			 clientInfo, (int)g_serviceType, (int)g_serverType, g_accountAddr, g_accountPort, (int)g_readFolderFirst);
#endif

	InitLanguage();

	if (g_serviceType == ServiceKorea || g_serviceType == ServiceIndonesia)
	{
		NameBalloonShorten  = true;
		NameBalloonfontBold = true;
		NameBalloonfontSize = 13;
	}
	else
	{
		NameBalloonShorten  = false;
		NameBalloonfontBold = false;
		NameBalloonfontSize = 12;
	}

	if (strstr(clientInfo, "psakclientinfo"))
	{
		g_licence = "..\\pvplicence.txt";
	}
	else if (strstr(clientInfo, "pksakclientinfo"))
	{
		g_licence = "..\\pklicence.txt";
	}
	else if (g_serviceType == ServiceAmerica && g_serverType == ServerSakray)
	{
		g_licence = "..\\slicence.txt";
		goto set_title_bmp;
	}
	else
	{
		g_licence = "..\\licence.txt";
	}

	if (g_serviceType == ServiceKorea)
	{
		GetLocalTime(&sTime);
		if (g_serviceType == ServiceKorea)
		{
			title_random = (char)((int)sTime.wSecond % 4);
		}
		else
		{
			title_random = (char)((int)sTime.wSecond % 2);
		}
		goto switch_server;
	}

set_title_bmp:
	TITLE_FILE = "\xC0\xAF\xC0\xFA\xC0\xCE\xC5\xCD\xC6\xE4\xC0\xCC\xBD\xBA\\bgi_temp.bmp";

switch_server:
	g_regPath   = "Software\\" BF_APP_NAME;
	WINDOW_NAME = BF_APP_NAME;

	switch (g_serverType)
	{
	case ServerSakray:
		PATCH_CLIENT           = "sakray.exe";
		PATCH_UP               = "Sakup.exe";
		PLEASE_EXCUTE_LAUNCHER = "Please run sakray.exe";
		break;
	case ServerLocal:
		PATCH_CLIENT           = "RagIndoor.exe";
		PATCH_UP               = "Indoorup.exe";
		PLEASE_EXCUTE_LAUNCHER = "Please run RagIndoor.exe";
		break;
	case ServerInstantEvent:
		PATCH_CLIENT           = "RagGuild.exe";
		PATCH_UP               = "Guildup.exe";
		PLEASE_EXCUTE_LAUNCHER = "Please run RagGuild.exe";
		break;
	case ServerPK:
		PATCH_UP               = "Pkup.exe";
		PATCH_CLIENT           = "RagnarokPK.exe";
		PLEASE_EXCUTE_LAUNCHER = "Please run RagnarokPK.exe";
		break;
	default:
	case ServerPVP:
		PATCH_CLIENT           = "Ragnarok.exe";
		PATCH_UP               = "Patchup.exe";
		PLEASE_EXCUTE_LAUNCHER = "Please run Ragnarok.exe";
		break;
	}
}

#include "rag_budget_fill.h"
void SetOption(XMLElement* clientinfo)
{
	RAG_BUDGET_FILL(20)
	static bool check = false;
	XMLElement* element;
	char temp[256];

	if ((element = clientinfo->FindChild("servicetype"))) {
		if (element->GetContents() == "korea")
			g_serviceType = ServiceKorea;
		else if (element->GetContents() == "america")
			g_serviceType = ServiceAmerica;
		else if (element->GetContents() == "japan")
			g_serviceType = ServiceJapan;
		else if (element->GetContents() == "china")
			g_serviceType = ServiceChina;
		else if (element->GetContents() == "taiwan")
			g_serviceType = ServiceTaiwan;
		else if (element->GetContents() == "thai")
			g_serviceType = ServiceThai;
		else if (element->GetContents() == "indonesia")
			g_serviceType = ServiceIndonesia;
		else if (element->GetContents() == "philippine")
			g_serviceType = ServicePhilippine;
		else if (element->GetContents() == "malaysia")
			g_serviceType = ServiceMalaysia;
		else if (element->GetContents() == "singapore")
			g_serviceType = ServiceSingapore;
		else if (element->GetContents() == "germany")
			g_serviceType = ServiceGermany;
		else if (element->GetContents() == "india")
			g_serviceType = ServiceIndia;
		else if (element->GetContents() == "brazil")
			g_serviceType = ServiceBrazil;
		else if (element->GetContents() == "australia")
			g_serviceType = ServiceAustralia;
		else if (element->GetContents() == "russia")
			g_serviceType = ServiceRussia;
		else if (element->GetContents() == "vietnam")
			g_serviceType = ServiceVietnam;
		else if (element->GetContents() == "chile")
			g_serviceType = ServiceChile;
		else if (element->GetContents() == "france")
			g_serviceType = ServiceFrance;
		else
			ErrorMsg("ServiceType Error !!");
	}
	else if (!check) {
		ErrorMsg("No ServiceType !!");
	}

	if ((element = clientinfo->FindChild("servertype"))) {
		if (element->GetContents() == "primary")
			g_serverType = ServerPrimary;
		else if (element->GetContents() == "sakray")
			g_serverType = ServerSakray;
		else if (element->GetContents() == "local")
			g_serverType = ServerLocal;
		else if (element->GetContents() == "pk")
			g_serverType = ServerPK;
		else
			ErrorMsg("ServerType Error !!");
	}
	else if (!check) {
		ErrorMsg("No ServerType !!");
	}

	if (clientinfo->FindChild("hideaccountlist")) {
		g_hideAccountList = true;
	}
	if (clientinfo->FindChild("passwordencrypt")) {
		g_passwordEncrypt = true;
	}
	if (clientinfo->FindChild("passwordencrypt2")) {
		g_passwordEncrypt = true;
		g_passwordEncrypt2 = true;
	}
	if (clientinfo->FindChild("extendedslot")) {
		g_extended_slot = true;
	}
	if (clientinfo->FindChild("readfolder")) {
		g_readFolderFirst = true;
	}

	if ((element = clientinfo->FindChild("loading"))) {
		element = element->FindChild("image");
		while (element) {
			sprintf(temp, "\xC0\xAF\xC0\xFA\xC0\xCE\xC5\xCD\xC6\xE4\xC0\xCC\xBD\xBA\\%s",
					element->GetContents().c_str());
			s_loadingScreenList.push_back(std::string(temp));
			element = element->FindNext("image");
		}
	}

	check = true;
}

void InitDefaultClientInfo()
{
	switch (g_serviceType)
	{
	case ServiceKorea:
		SelectKoreaClientInfo();
		break;
	case ServiceAmerica:
	case ServiceJapan:
	case ServiceChina:
	case ServiceTaiwan:
	case ServiceThai:
	case ServiceIndonesia:
	case ServicePhilippine:
	case ServiceMalaysia:
	case ServiceSingapore:
	case ServiceGermany:
	case ServiceIndia:
	case ServiceBrazil:
	case ServiceAustralia:
	case ServiceRussia:
	case ServiceVietnam:
	case ServiceChile:
	case ServiceFrance:
		SelectClientInfo(0);
		break;
	default:
		ErrorMsg("Unknown ServiceType !!!");
		break;
	}
	InitLanguage();
}

void InitDefaultServiceType()
{
	XMLElement* clientinfo;

	if (clientinfo = document.FindElement("clientinfo"))
	{
		SetOption(clientinfo);
	}

	if (CFile::IsFileExist("etcinfo.txt"))
	{
		CommonObject s_obj;
		s_loadingScreenList.clear();
		std::vector<const char*> pCharList;
		s_obj.TokenFileToPCharList(pCharList, "etcinfo.txt", 0);

		for (std::vector<const char*>::iterator it = pCharList.begin(); it != pCharList.end(); ++it)
		{
			if (strcmp(*it, "loadingscreen") == 0)
			{
				int count = atoi(*(it + 1));
				if (count > 0)
					InitDefaultLoadingScreen(count);
			}
			else if (strcmp(*it, "event") == 0)
			{
				int count = atoi(*(it + 1));
				if (count > 0)
					g_fileMgr.AddPak("event.grf");
			}
			else if (strcmp(*it, "weather") == 0)
			{
				std::string map = *(++it);
				std::string weather = *(++it);
				MakeStringLower(map);
				MakeStringLower(weather);
				g_Weather.AddScript(map, weather);
			}
		}
	}

	if (s_loadingScreenList.empty()) InitDefaultLoadingScreen();
}

void SelectClientInfo(int number)
{
	XMLElement* connection;
	XMLElement* element;

	if (!(connection = document.GetRootElement()->FindChild("clientinfo"))) return;

	if (!(connection = connection->FindChild("connection"))) return;

	while (number && connection)
	{
		--number;
		connection = connection->FindNext("connection");
	}
	if (!connection) return;

	SetOption(connection);

	if (element = connection->FindChild("address")) g_accountAddr = element->GetContents().c_str();
	if (element = connection->FindChild("port")) g_accountPort = element->GetContents().c_str();
	if (element = connection->FindChild("version")) g_version = atoi(element->GetContents().c_str());
	if (element = connection->FindChild("langtype")) g_serviceType = (ServiceType)atoi(element->GetContents().c_str());
	if (element = connection->FindChild("registrationweb")) g_regstrationWeb = element->GetContents().c_str();
	element = connection->FindChild("aid");
	if (element) { s_dwAdminAID.clear();
		element = element->FindChild("admin");
		do {
			number = atoi(element->GetContents().c_str()); s_dwAdminAID.push_back(number);
		} while (element = element->FindNext("admin"));
		std::sort(s_dwAdminAID.begin(), s_dwAdminAID.end());
	} else
		s_dwAdminAID.clear();


	if (element = connection->FindChild("yellow")) {
		s_dwYellowAID.clear();
		element = element->FindChild("admin");
		do {
			number = atoi(element->GetContents().c_str()); s_dwYellowAID.push_back(number);
		} while (element = element->FindNext("admin"));
		std::sort(s_dwYellowAID.begin(), s_dwYellowAID.end());
	} else
		s_dwYellowAID.assign(s_dwAdminAID.begin(), s_dwAdminAID.end());


	InitLanguage();
}

void SelectClientInfo2(int number, int number2)
{
	XMLElement* connection;
	XMLElement* subconnection;
	XMLElement* element;

	if (!(connection = document.GetRootElement()->FindChild("clientinfo"))) return;

	if (!(connection = connection->FindChild("connection"))) return;

	while (number && connection)
	{
		--number;
		connection = connection->FindNext("connection");
	}
	if (!connection) return;
	if (subconnection = connection->FindChild("subconnection"))
	{
		while (number2 && subconnection)
		{
			--number2;
			subconnection = subconnection->FindNext("subconnection");
		}

		if (subconnection)
		{
			SetOption(subconnection);

			if (element = subconnection->FindChild("address")) g_accountAddr = element->GetContents().c_str();
			if (element = subconnection->FindChild("port")) g_accountPort = element->GetContents().c_str();
			if (element = subconnection->FindChild("version")) g_version = atoi(element->GetContents().c_str());
			if (element = subconnection->FindChild("langtype")) g_serviceType = (ServiceType)atoi(element->GetContents().c_str());
			if (element = subconnection->FindChild("registrationweb")) g_regstrationWeb = element->GetContents().c_str();

			if (element = subconnection->FindChild("aid")) {
				s_dwAdminAID.clear();
				XMLElement* admin = element->FindChild("admin");
				do
					s_dwAdminAID.push_back(atoi(admin->GetContents().c_str()));
				while (admin = admin->FindNext("admin"));
				std::sort(s_dwAdminAID.begin(), s_dwAdminAID.end());
			} else
				s_dwAdminAID.clear();


			if (element = subconnection->FindChild("yellow")) {
				s_dwYellowAID.clear();
				XMLElement* admin = element->FindChild("admin");
				do
					s_dwYellowAID.push_back(atoi(admin->GetContents().c_str()));
				while (admin = admin->FindNext("admin"));
				std::sort(s_dwYellowAID.begin(), s_dwYellowAID.end());
			} else
				s_dwYellowAID.assign(s_dwAdminAID.begin(), s_dwAdminAID.end());
		}
	}
	else
	{
		SetOption(connection);

		if (element = connection->FindChild("address")) g_accountAddr = element->GetContents().c_str();
		if (element = connection->FindChild("port")) g_accountPort = element->GetContents().c_str();
		if (element = connection->FindChild("version")) g_version = atoi(element->GetContents().c_str());
		if (element = connection->FindChild("langtype")) g_serviceType = (ServiceType)atoi(element->GetContents().c_str());
		if (element = connection->FindChild("registrationweb")) g_regstrationWeb = element->GetContents().c_str();

		if (element = connection->FindChild("aid")) {
			s_dwAdminAID.clear();
			XMLElement* admin = element->FindChild("admin");
			do {
				s_dwAdminAID.push_back(atoi(admin->GetContents().c_str()));
			} while (admin = admin->FindNext("admin"));
			std::sort(s_dwAdminAID.begin(), s_dwAdminAID.end());
		} else
			s_dwAdminAID.clear();


		if (element = connection->FindChild("yellow")) {
			s_dwYellowAID.clear();
			XMLElement* admin = element->FindChild("admin");
			do {
				s_dwYellowAID.push_back(atoi(admin->GetContents().c_str()));
			} while (admin = admin->FindNext("admin"));
			std::sort(s_dwYellowAID.begin(), s_dwYellowAID.end());
		} else
			s_dwYellowAID.assign(s_dwAdminAID.begin(), s_dwAdminAID.end());
	}

	InitLanguage();
}

void InitLanguage()
{
	static CLanguage   language;
	static CLanguageTC languageTC;
	static CLanguageTH languageTH;

	switch (g_serviceType)
	{
	case ServiceKorea:
		g_languageType = LanguageKorean;
		g_codePage     = 949;
		g_language     = &language;
		break;
	case ServiceJapan:
		g_languageType = LanguageJapanese;
		g_codePage     = 932;
		g_language     = &language;
		break;
	case ServiceChina:
		g_languageType = LanguageSimplifiedChinese;
		g_codePage     = 936;
		g_language     = &languageTC;
		break;
	case ServiceTaiwan:
		g_languageType = LanguageTraditionalChinese;
		g_codePage     = 950;
		g_language     = &languageTC;
		break;
	case ServiceThai:
		g_languageType = LanguageThai;
		g_codePage     = 874;
		g_language     = &languageTH;
		break;
	case ServiceIndia:
		g_languageType = LanguageEnglish;
		g_codePage     = 0;
		g_language     = &language;
		g_multiLang    = false;
		break;
	case ServiceGermany:
	case ServiceAustralia:
		g_languageType = LanguageEnglish;
		g_codePage     = 0;
		g_language     = &language;
		g_multiLang    = true;
		break;
	case ServiceRussia:
		g_languageType = LanguageEnglish;
		g_codePage     = 1251;
		g_language     = &language;
		g_multiLang    = false;
		break;
	case ServiceVietnam:
		g_languageType = LanguageVietnam;
		g_codePage     = 1258;
		g_language     = &language;
		g_multiLang    = false;
		break;
	case ServiceChile:
		g_languageType = LanguageEnglish;
		g_codePage     = 1145;
		g_language     = &language;
		g_multiLang    = false;
		break;
	case ServiceBrazil:
	case ServiceFrance:
		g_languageType = LanguagePortuguese;
		g_codePage     = 1252;
		g_language     = &language;
		g_multiLang    = false;
		break;
	default:
		ErrorMsg("no language setting !!!");

	case ServiceAmerica:
	case ServiceIndonesia:
	case ServicePhilippine:
	case ServiceMalaysia:
	case ServiceSingapore:
		g_languageType = LanguageEnglish;
		g_codePage     = 0;
		g_language     = &language;
		break;
	}

	if (g_serverType == ServerInstantEvent)
	{
		g_multiLang = true;
	}
}

void SelectKoreaClientInfo()
{
	g_accountPort = "6900";
	g_regstrationWeb = "http://www.ragnarok.co.kr";
	g_extended_slot = true;

	switch (g_serverType)
	{
	case ServerPrimary:
		g_accountAddr = "211.172.247.192";
		g_version = 19;
		if (g_EXE_way == 2)
		{
			g_version = 19;
			g_accountAddr = "211.172.246.193";
		}
		s_dwAdminAID.assign(dwAdminAID, dwAdminAID + sizeof(dwAdminAID) / sizeof(DWORD));
		s_dwYellowAID.assign(dwYellowAID, dwYellowAID + sizeof(dwYellowAID) / sizeof(DWORD));
		return;
	case ServerSakray:
		g_accountAddr = "211.172.247.192";
		g_version = 29;
		if (g_EXE_way == 2)
		{
			g_version = 14;
			g_accountAddr = "192.168.20.170";
		}
		s_dwAdminAID.assign(dwAdminAID, dwAdminAID + sizeof(dwAdminAID) / sizeof(DWORD));
		s_dwYellowAID.assign(dwYellowAID, dwYellowAID + sizeof(dwYellowAID) / sizeof(DWORD));
		return;
	case ServerLocal:
		g_version = 14;
		g_accountAddr = "192.168.20.170";
		s_dwAdminAID.clear();
		s_dwYellowAID.clear();
		return;
	case ServerInstantEvent:
		g_accountPort = "6900";
		g_accountAddr = "211.172.247.115";
		g_version = 29;
		s_dwAdminAID.assign(dwAdminAID, dwAdminAID + sizeof(dwAdminAID) / sizeof(DWORD));
		s_dwYellowAID.assign(dwYellowAID, dwYellowAID + sizeof(dwYellowAID) / sizeof(DWORD));
		return;
	default:
		return;
	}
}


void InitDefaultLoadingScreen(int count)
{
	char temp[256];
	sprintf(temp, "\xC0\xAF\xC0\xFA\xC0\xCE\xC5\xCD\xC6\xE4\xC0\xCC\xBD\xBA\\\xC7\xCF\xC7\xC1" "0206.jpg");
	if (g_resMgr().IsExist(temp))
	{
		s_loadingScreenList.push_back(std::string(temp));
	}
}

const char* GetLicenceText()
{
	static std::string strLicence;

	if (strLicence.empty())
	{
		CFile fp;
		if (fp.Open(g_licence, CFile::modeRead))
		{
			strLicence.assign((const char*)fp.GetBuf(),
							  (const char*)(fp.GetBuf() + fp.GetLength()));
			fp.Close();
		}
		else
		{
			strLicence.assign("No EULA text file. (licence.txt)");
		}
	}
	return strLicence.c_str();
}

XMLElement* GetClientInfo()
{
	return document.FindElement("clientinfo");
}

bool IsNameYellow(DWORD aid)
{
	return std::find(s_dwYellowAID.begin(), s_dwYellowAID.end(), aid) != s_dwYellowAID.end();
}

bool IsGravityAid(DWORD aid)
{
	return std::find(s_dwAdminAID.begin(), s_dwAdminAID.end(), aid) != s_dwAdminAID.end();
}

void CSession::SetTextType(bool isShorten, bool isBold)
{
	if (g_serviceType == ServiceKorea ||
		g_serviceType == ServiceIndonesia ||
		g_serviceType == ServiceGermany ||
		g_serviceType == ServiceBrazil)
	{
		NameBalloonShorten  = isShorten;
		NameBalloonfontBold = isBold;
	}
}

int IsNameSpecific(DWORD aid)
{
	if (ServiceIndonesia == g_serviceType)
	{
		switch (aid)
		{
		case 100065:
		case 100068:
		case 100075:
		case 100076:
		case 441487:
		case 100001:
		case 540397:
		case 441470:
		case 441474:
		case 100064:
			return RGB(255, 0, 0);
		}
	}
	return false;
}
