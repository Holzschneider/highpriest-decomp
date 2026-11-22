#ifndef HIGHPRIEST_CSECUREGAMEPACK_H
#define HIGHPRIEST_CSECUREGAMEPACK_H


class cSecureGamePack;

class cSecureGamePack {
protected:
	cSecureGamePack();
	static cSecureGamePack* m_pInst;
public:
	~cSecureGamePack();
	static cSecureGamePack* __cdecl GetInstance();
	static void __cdecl Create();
	static void __cdecl Destroy();
	static void __cdecl OnNotifyFromSGP(void* lpNotifyData);
	static unsigned char __cdecl IsAvailable();
	bool InitSecureGamePack();
private:
	void DestroySecureGamePack();
	void LeaveSecureGamePack();
};

#endif
