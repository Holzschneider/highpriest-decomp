#ifndef __PADDING_H_ 
#define	__PADDING_H_ 

#include <windows.h>
#include <vector>

using namespace std;

#define MAX_PADDING_PACKET_LEN	3000
#define MAX_PSODOPAD			5
#define MAX_PADDING				39

DWORD GetPaddingKey(DWORD startTime,DWORD clientTime,DWORD AID,short packetID);

bool PackPaddingPacket(char* buf,short& len,DWORD paddingKey,short header,vector<DWORD>& v,int psodoPad = MAX_PSODOPAD);


bool UnPackPaddingPacket(char* buf,DWORD paddingKey,vector<DWORD>& v,int count);

#if defined(__CLIENT_) && defined(_CRYPTO_PADDING)
#include "device\connection.h"
class	CEncryptionTime
{	
public:
	enum TYPE
	{
		ACCEPTTIME,
		REQUESTTIME,
	};
	CEncryptionTime():m_acceptTime(0),m_requestTime(0){}
	static	CEncryptionTime*	instanceR()
	{
		static CEncryptionTime* _instance;

		if( _instance == NULL)
			_instance = new CEncryptionTime;
		
		return _instance;
	}
	//DWORD Get() { return m_acceptTime; }
	void Set(TYPE type,DWORD time)
	{ 
		switch(type)
		{
		case ACCEPTTIME:
			m_acceptTime = time;
			break;
		case REQUESTTIME:
			m_requestTime = time;
		}
	}
	void Send(DWORD AID,short packetID)
	{
		static char buf[MAX_PADDING_PACKET_LEN];
		short size = 0;
		DWORD paddingKey=GetPaddingKey(m_acceptTime,m_requestTime,AID,packetID);
		PackPaddingPacket(buf,size,paddingKey,packetID,m_data);
		m_data.clear();
		g_connection.SendPacket(size, (const char *)(buf));
	}
	void ClearData()			{ m_data.clear(); }
	void AddData(DWORD data)	{ m_data.push_back(data); }
private:
	vector<DWORD> m_data;
	DWORD m_acceptTime;
	DWORD m_requestTime;
};

#define g_encryptionTime (*CEncryptionTime::instanceR())

#endif //__CLIENT_








void EncryptStorePassword (int password, BYTE code[]);


#if !defined(__CLIENT_)

void DecryptStorePassword (BYTE code[], int& password);
#endif



#endif //__PADDING_H_ 