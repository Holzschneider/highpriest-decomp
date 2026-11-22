#ifndef FILE_AUTH_CLIENT_H
#define FILE_AUTH_CLIENT_H

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#include <windows.h>
#include <WINCRYPT.H>

class CFileAuthClient {
public:
    CFileAuthClient();
    ~CFileAuthClient();

    bool Auth(const char* fileName, bool checkName);
    bool DecryptMem(const char* fileName, char* output,
                    unsigned long outputSize, const char* password);
    bool DecryptMem2(const char* fileName, char* output,
                     unsigned long outputSize, const char* password);



    unsigned long GetError() { return m_lastError; }

private:
    friend class CGameGuard;
    bool DecryptRC4(unsigned char* data, unsigned long size);

    HCRYPTPROV m_provider;
    unsigned char* m_buffer;
    unsigned long m_payloadSize;
    HCRYPTHASH m_hash;
    HCRYPTKEY m_publicKey;
    unsigned char* m_signature;
    unsigned long m_signatureSize;
    unsigned char m_hasFooter;
    unsigned long m_dataOffset;
    unsigned long m_fileSize;
    unsigned long m_footer[4];
    unsigned long m_nameSize;
    HCRYPTKEY m_decryptKey;
    HCRYPTHASH m_passwordHash;
    unsigned long m_lastError;
};

#endif
