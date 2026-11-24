#include "FileAuthClient.h"
#include <stdlib.h>
#include <string.h>

unsigned char pbPubKey[] = {
    0x06, 0x02, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00,
    0x52, 0x53, 0x41, 0x31, 0x00, 0x02, 0x00, 0x00,
    0x01, 0x00, 0x01, 0x00, 0xfb, 0xe3, 0xfc, 0x09,
    0xaf, 0xae, 0x65, 0x8c, 0x96, 0x4c, 0xc5, 0x37,
    0xd2, 0xa4, 0x77, 0xe7, 0x4c, 0x41, 0xc2, 0xcf,
    0xf2, 0xfe, 0x2d, 0x9c, 0x80, 0x94, 0x0c, 0x88,
    0x6d, 0xb3, 0x84, 0x9f, 0x8c, 0x22, 0xa0, 0xc9,
    0xcd, 0xc0, 0xab, 0x30, 0x65, 0x82, 0x42, 0x3c,
    0xee, 0x3c, 0xa8, 0xb7, 0x11, 0xd6, 0x22, 0xfa,
    0xfb, 0x23, 0xf7, 0x72, 0xcd, 0xe7, 0xd0, 0x6f,
    0x6a, 0x8e, 0x96, 0xe3
};
unsigned int dwPubKeyLen = sizeof(pbPubKey);


unsigned int g_GGVerRand0 = 0xffffffffUL;
unsigned int g_GGVerRand1 = 1;
unsigned int g_GGVerRand2 = 2;
unsigned int g_GGVerRand3 = 3;

CFileAuthClient::CFileAuthClient() {
    m_signature = 0;
    m_hash = 0;
    m_provider = 0;
    m_buffer = 0;
    m_payloadSize = 0;
    m_fileSize = 0;
    m_nameSize = 0;
    m_passwordHash = 0;
    m_decryptKey = 0;
    m_signatureSize = 0;
    m_hasFooter = 0;
    m_dataOffset = 0;
    m_lastError = 0;
    m_publicKey = 0;

    if (!CryptAcquireContextA(&m_provider, 0, MS_DEF_PROV_A,
                              PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        m_provider = 0;
    }
}

CFileAuthClient::~CFileAuthClient() {
    if (m_buffer) {
        free(m_buffer);
    }
    if (m_signature) {
        free(m_signature);
    }
    if (m_hash) {
        CryptDestroyHash(m_hash);
    }
    if (m_publicKey) {
        CryptDestroyKey(m_publicKey);
    }
    if (m_decryptKey) {
        CryptDestroyKey(m_decryptKey);
    }
    if (m_passwordHash) {
        CryptDestroyHash(m_passwordHash);
    }

    HCRYPTPROV provider = m_provider;
    if (provider) {
        CryptReleaseContext(provider, 0);
    }
}

bool CFileAuthClient::Auth(const char* fileName, bool checkName) {
    if (!fileName) {
        m_lastError = 1;
        return false;
    }
    if (!fileName[0]) {
        m_lastError = 2;
        return false;
    }

    HANDLE file = CreateFileA(fileName, GENERIC_READ, 3, 0, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, 0);
    if (file == INVALID_HANDLE_VALUE) {
        m_lastError = 3;
        return false;
    }

    m_fileSize = GetFileSize(file, 0);
    if (m_fileSize == 0xffffffff) {
        CloseHandle(file);
        m_lastError = 0x21;
        return false;
    }

    if (m_buffer) {
        free(m_buffer);
        m_buffer = 0;
    }

    m_buffer = (unsigned char*)malloc(m_fileSize);
    if (!m_buffer) {
        CloseHandle(file);
        m_lastError = 4;
        return false;
    }

    DWORD bytesRead;
    ReadFile(file, m_buffer, m_fileSize, &bytesRead, 0);
    if (bytesRead != m_fileSize) {
        CloseHandle(file);
        m_lastError = 5;
        return false;
    }

    memcpy(m_footer, m_buffer + m_fileSize - 0x10, 0x10);
    if (m_footer[0] == 0x32812622 && m_footer[3] == 0x32812621) {
        m_hasFooter = 1;
        m_dataOffset = m_footer[2] + m_footer[1] + 0x10;
        m_payloadSize = m_fileSize - m_dataOffset;
        m_nameSize = m_footer[1];
    } else {
        memset(m_footer, 0, sizeof(m_footer));
        m_hasFooter = 0;
        m_dataOffset = 0;
        m_payloadSize = m_fileSize;
        m_nameSize = 0;
    }

    if (m_fileSize < m_dataOffset) {
        m_hasFooter = 0;
        m_dataOffset = 0;
        m_fileSize = 0;
        m_payloadSize = 0;
        m_nameSize = 0;
    }

    CloseHandle(file);

    if (!m_hasFooter) {
        m_lastError = 6;
        return false;
    }

    m_signatureSize = m_footer[2];
    if (m_signature) {
        free(m_signature);
        m_signature = 0;
    }

    m_signature = (unsigned char*)malloc(m_signatureSize);
    if (!m_signature) {
        m_lastError = 7;
        return false;
    }

    unsigned char* signatureSource = m_buffer + m_payloadSize;
    signatureSource += m_nameSize;
    memcpy(m_signature, signatureSource, m_signatureSize);

    if (!m_provider) {
        if (!CryptAcquireContextA(&m_provider, 0, MS_DEF_PROV_A,
                                  PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
            m_lastError = 8;
            return false;
        }
    }

    if (m_publicKey) {
        CryptDestroyKey(m_publicKey);
        m_publicKey = 0;
    }

    if (!CryptImportKey(m_provider, pbPubKey, dwPubKeyLen, 0, 0,
                        &m_publicKey)) {
        if (GetLastError() == ERROR_INVALID_PARAMETER) {
            if (!CryptAcquireContextA(&m_provider, 0, MS_DEF_PROV_A,
                                      PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
                m_lastError = 0x62;
                return false;
            }
            if (!CryptImportKey(m_provider, pbPubKey, dwPubKeyLen, 0, 0,
                                &m_publicKey)) {
                m_lastError = 9;
                return false;
            }
        } else {
            m_lastError = 9;
            return false;
        }
    }

    if (m_hash) {
        CryptDestroyHash(m_hash);
        m_hash = 0;
    }
    if (!CryptCreateHash(m_provider, CALG_MD5, 0, 0, &m_hash)) {
        m_lastError = 0x0a;
        return false;
    }

    if (!CryptHashData(m_hash, m_buffer, m_payloadSize + m_nameSize, 0)) {
        m_lastError = 0x0b;
        return false;
    }

    if (!CryptVerifySignatureA(m_hash, m_signature, m_signatureSize,
                               m_publicKey, 0, 0)) {
        if (GetLastError() == ERROR_INVALID_PARAMETER) {
            if (!CryptAcquireContextA(&m_provider, 0, MS_DEF_PROV_A,
                                      PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
                m_lastError = 0x80;
                return false;
            }
            if (!CryptVerifySignatureA(m_hash, m_signature, m_signatureSize,
                                       m_publicKey, 0, 0)) {
                m_lastError = 0x0c;
                return false;
            }
        } else {
            m_lastError = 0x0c;
            return false;
        }
    }

    if (checkName) {
        const char* name = strrchr(fileName, '\\');
        if (name) {
            ++name;
        } else {
            name = fileName;
        }

        if (_strnicmp(name, (const char*)(m_buffer + m_payloadSize),
                      m_nameSize)) {
            m_lastError = 0x0d;
            return false;
        }
    }

    if (m_buffer) {
        free(m_buffer);
        m_buffer = 0;
    }
    return true;
}

bool CFileAuthClient::DecryptMem(const char* fileName, char* output,
                                 unsigned int outputSize,
                                 const char* password) {
    HANDLE file = CreateFileA(fileName, GENERIC_READ, 3, 0, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, 0);
    if (file == INVALID_HANDLE_VALUE) {
        m_lastError = 1;
        return false;
    }

    m_fileSize = GetFileSize(file, 0);
    if (m_fileSize == 0xffffffff) {
        CloseHandle(file);
        m_lastError = 0x0b;
        return false;
    }

    if (m_buffer) {
        free(m_buffer);
        m_buffer = 0;
    }

    m_buffer = (unsigned char*)malloc(m_fileSize);
    if (!m_buffer) {
        CloseHandle(file);
        m_lastError = 2;
        return false;
    }

    DWORD bytesRead;
    ReadFile(file, m_buffer, m_fileSize, &bytesRead, 0);
    if (bytesRead != m_fileSize) {
        CloseHandle(file);
        m_lastError = 3;
        return false;
    }

    memcpy(m_footer, m_buffer + m_fileSize - 0x10, 0x10);
    if (m_footer[0] == 0x32812622 && m_footer[3] == 0x32812621) {
        m_hasFooter = 1;
        m_dataOffset = m_footer[2] + m_footer[1] + 0x10;
        m_payloadSize = m_fileSize - m_dataOffset;
        m_nameSize = m_footer[1];
    } else {
        memset(m_footer, 0, sizeof(m_footer));
        m_hasFooter = 0;
        m_dataOffset = 0;
        m_payloadSize = m_fileSize;
        m_nameSize = 0;
    }

    if (m_fileSize < m_dataOffset) {
        m_hasFooter = 0;
        m_dataOffset = 0;
        m_fileSize = 0;
        m_payloadSize = 0;
        m_nameSize = 0;
    }

    CloseHandle(file);

    if (!m_provider) {
        if (!CryptAcquireContextA(&m_provider, 0, MS_DEF_PROV_A,
                                  PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
            m_lastError = 4;
            return false;
        }
    }

    if (m_passwordHash) {
        CryptDestroyHash(m_passwordHash);
        m_passwordHash = 0;
    }
    if (!CryptCreateHash(m_provider, CALG_MD5, 0, 0, &m_passwordHash)) {
        m_lastError = 5;
        return false;
    }

    if (!CryptHashData(m_passwordHash, (unsigned char*)password,
                       strlen(password), 0)) {
        m_lastError = 6;
        return false;
    }

    if (m_decryptKey) {
        CryptDestroyKey(m_decryptKey);
        m_decryptKey = 0;
    }
    if (!CryptDeriveKey(m_provider, CALG_RC4, m_passwordHash, 0,
                        &m_decryptKey)) {
        m_lastError = 7;
        return false;
    }

    DWORD dataSize = m_payloadSize;
    if (!CryptDecrypt(m_decryptKey, 0, TRUE, 0, m_buffer, &dataSize)) {
        if (GetLastError() != NTE_PERM) {
            m_lastError = 8;
            return false;
        }
        if (!DecryptRC4(m_buffer, dataSize)) {
            m_lastError = 0x58;
            return false;
        }
    }

    unsigned int payloadSize = m_payloadSize;
    if (outputSize > payloadSize) {
        outputSize = payloadSize;
    }
    memcpy(output, m_buffer, outputSize);
    output[outputSize] = 0;

    if (m_buffer) {
        free(m_buffer);
        m_buffer = 0;
    }
    return true;
}

bool CFileAuthClient::DecryptMem2(const char* fileName, char* output,
                                  unsigned int outputSize,
                                  const char* password) {
    HANDLE file = CreateFileA(fileName, GENERIC_READ, 3, 0, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, 0);
    if (file == INVALID_HANDLE_VALUE) {
        m_lastError = 1;
        return false;
    }

    m_fileSize = GetFileSize(file, 0);
    if (m_fileSize == 0xffffffff) {
        CloseHandle(file);
        m_lastError = 0x0b;
        return false;
    }

    if (m_buffer) {
        free(m_buffer);
        m_buffer = 0;
    }

    m_buffer = (unsigned char*)malloc(m_fileSize);
    if (!m_buffer) {
        CloseHandle(file);
        m_lastError = 2;
        return false;
    }

    DWORD bytesRead;
    ReadFile(file, m_buffer, m_fileSize, &bytesRead, 0);
    if (bytesRead != m_fileSize) {
        CloseHandle(file);
        m_lastError = 3;
        return false;
    }

    memcpy(m_footer, m_buffer + m_fileSize - 0x10, 0x10);
    if (m_footer[0] == 0x32812622 && m_footer[3] == 0x32812621) {
        m_hasFooter = 1;
        m_dataOffset = m_footer[2] + m_footer[1] + 0x10;
        m_payloadSize = m_fileSize - m_dataOffset;
        m_nameSize = m_footer[1];
    } else {
        memset(m_footer, 0, sizeof(m_footer));
        m_hasFooter = 0;
        m_dataOffset = 0;
        m_payloadSize = m_fileSize;
        m_nameSize = 0;
    }

    if (m_fileSize < m_dataOffset) {
        m_hasFooter = 0;
        m_dataOffset = 0;
        m_fileSize = 0;
        m_payloadSize = 0;
        m_nameSize = 0;
    }

    CloseHandle(file);

    if (!m_provider) {
        if (!CryptAcquireContextA(&m_provider, 0, MS_DEF_PROV_A,
                                  PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
            m_lastError = 4;
            return false;
        }
    }

    if (m_passwordHash) {
        CryptDestroyHash(m_passwordHash);
        m_passwordHash = 0;
    }
    if (!CryptCreateHash(m_provider, CALG_MD5, 0, 0, &m_passwordHash)) {
        m_lastError = 5;
        return false;
    }

    if (!CryptHashData(m_passwordHash, (unsigned char*)password,
                       strlen(password), 0)) {
        m_lastError = 6;
        return false;
    }

    if (m_decryptKey) {
        CryptDestroyKey(m_decryptKey);
        m_decryptKey = 0;
    }
    if (!CryptDeriveKey(m_provider, CALG_RC4, m_passwordHash, 0,
                        &m_decryptKey)) {
        m_lastError = 7;
        return false;
    }

    unsigned int dataSize = m_payloadSize;
    if (!DecryptRC4(m_buffer, dataSize)) {
        m_lastError = 0x58;
        return false;
    }

    unsigned int payloadSize = m_payloadSize;
    if (outputSize > payloadSize) {
        outputSize = payloadSize;
    }
    memcpy(output, m_buffer, outputSize);
    output[outputSize] = 0;

    if (m_buffer) {
        free(m_buffer);
        m_buffer = 0;
    }
    return true;
}

struct rc4_key_st {
    unsigned int x;
    unsigned int y;
    unsigned int data[256];
};

void RC4_set_key(rc4_key_st* key, int len, unsigned char* data);
void RC4(rc4_key_st* key, unsigned int len, unsigned char* indata,
         unsigned char* outdata);

bool CFileAuthClient::DecryptRC4(unsigned char* data, unsigned int size) {
    rc4_key_st key;
    unsigned char keyBytes[16];
    DWORD keySize = sizeof(keyBytes);

    if (!CryptGetHashParam(m_passwordHash, HP_HASHVAL, keyBytes, &keySize, 0)) {
        return false;
    }

    memset(keyBytes + 5, 0, 11);
    RC4_set_key(&key, sizeof(keyBytes), keyBytes);
    RC4(&key, size, data, data);
    return true;
}
