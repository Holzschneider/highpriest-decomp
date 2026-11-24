









#include "Padding.h"
#include "packet.h"
#include "CipherImpl.h"

void EncryptStorePassword(int password, BYTE code[])
{
	unsigned int keybase[] = {
				84635513, 33735033, 14811424, 83510243,
				24753864, 93797874, 8228203, 147562201 };

	static BOOL initKey = false;
	if (false == initKey) {
		crypton_set_key(keybase, sizeof(keybase));
		initKey = true;
	}

	unsigned int data[4] = {0,};
	memcpy(data, &password, sizeof(password));
	crypton_encrypt(data, (DWORD*)code);
}
