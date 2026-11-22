#include "common.h"
#include "framework/Locale.h"


#include "TipOfTheDay.h"
#include "base/File.h"
#include "framework/Language.h"

#include <stdio.h>

extern ServiceType g_serviceType;

CTipOfTheDay g_tipOfTheDay;


int CTipOfTheDay::Load()
{
	const char *fName;

	m_bTryLoading   = true;
	bool bDataFolder= false;
	char * fileBuf  = NULL;
	int fileSize    = 0;

	if ( g_serviceType == ServiceKorea )
		fName = "tipOfTheday.txt";
	else
	{
		fName = g_language->LanguageFile("tipofday.txt");
		if ( CFile::IsFileExist(fName) )
				bDataFolder = true;
		else
		{
			switch(g_serviceType)
			{
				case ServiceAmerica:
					fName = "tipOfTheday_e.txt";
					break;
				case ServiceJapan:
					fName = "tipOfTheday_j.txt";
					break;
				default:
					fName = "tipOfTheday.txt";
					break;
			}
		}
	}

	if ( bDataFolder )
	{
		CFile fp;
		if(!fp.Open(fName,CFile::modeRead)) return false;
		fileSize = fp.GetLength();
		fileBuf = new char[fileSize+1];
		fp.Read(fileBuf,fileSize);
		fp.Close();
	}
	else
	{
		FILE *file = fopen(fName, "r");
		if (file==NULL)
			return FALSE;

		fseek(file, 0, SEEK_END);
		fileSize = ftell(file);

		fileBuf = new char[fileSize+1];

		fseek(file, 0, SEEK_SET);
		fread(fileBuf, fileSize, 1, file);
		fclose(file);
	}
	fileBuf[fileSize] = 0;

	char * baseBuf = fileBuf;
	StringList tmpList;
	for (int i=0; i < fileSize; ++i) {
		switch (fileBuf[i]) {
			case '#':
				{
					fileBuf[i] = 0;
					tmpList.push_back(baseBuf);
					m_tipList.push_back(tmpList);
					tmpList.clear();
					baseBuf = fileBuf + i + 1;
				}
				break;
			case 10:
				fileBuf[i] = 0;
				tmpList.push_back(baseBuf);
				baseBuf = fileBuf + i + 1;
				break;
		}
	}

	if (fileBuf)
		delete [] fileBuf;
	return TRUE;
}

StringList& CTipOfTheDay::GetTip(int index)
{
	if ( m_bTryLoading == false )
		Load();
	if (index >= m_tipList.size() || index<0)
		return m_tipList[0];
	return m_tipList[index];
}
