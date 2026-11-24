// Reconstructed from original GameGuard evidence.

#include <windows.h>

class CFvCrc32 {
public:
    CFvCrc32();
    ~CFvCrc32();
    bool Crc32Mem(unsigned char* data, unsigned int len, unsigned int* crc);
    bool Crc32MemCont(unsigned char* data, unsigned int len, unsigned int* crc);
    bool Crc32File(const char* filename, unsigned int* crc);
    bool Crc32FileRange(const char* filename, unsigned int offset, unsigned int len, unsigned int* crc);
    bool Crc32FileRangeCont(const char* filename, unsigned int offset, unsigned int len, unsigned int* crc);
private:
    unsigned int m_table[256];
    unsigned int m_error;
};

CFvCrc32::CFvCrc32() {
    for (int i = 0; i < 256; ++i) {
        unsigned int c = i;
        for (int j = 8; j != 0; --j) {
            if (c & 1) c = (c >> 1) ^ 0xEDB88320;
            else c >>= 1;
        }
        m_table[i] = c;
    }
}

CFvCrc32::~CFvCrc32() {}

bool CFvCrc32::Crc32Mem(unsigned char* data, unsigned int len, unsigned int* crc) {
    *crc = 0xffffffff;
    Crc32MemCont(data, len, crc);
    *crc = ~*crc;
    return true;
}

bool CFvCrc32::Crc32MemCont(unsigned char* data, unsigned int len, unsigned int* crc) {
    if (!data) {
        m_error = 1;
        return false;
    }
    unsigned char* end = data + len;
    for (; data < end; ++data) {
        *crc = m_table[*data ^ *(unsigned char*)crc] ^ (*crc >> 8);
    }
    return true;
}

bool CFvCrc32::Crc32File(const char* filename, unsigned int* crc) {
    unsigned char buffer[4096];
    DWORD bytesRead;
    *crc = 0xffffffff;
    if (!filename || !*filename) {
        m_error = 1;
        return false;
    }
    HANDLE file = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (file == INVALID_HANDLE_VALUE) {
        m_error = 2;
        return false;
    }
    BOOL ok = ReadFile(file, buffer, sizeof(buffer), &bytesRead, 0);
    while (ok && bytesRead) {
        unsigned char* data = buffer;
        unsigned char* end = buffer + bytesRead;
        for (; data < end; ++data) {
            *crc = m_table[*(unsigned char*)crc ^ *data] ^ (*crc >> 8);
        }
        ok = ReadFile(file, buffer, sizeof(buffer), &bytesRead, 0);
    }
    if (!ok) m_error = 3;
    CloseHandle(file);
    *crc = ~*crc;
    return true;
}

bool CFvCrc32::Crc32FileRange(const char* filename, unsigned int offset, unsigned int len, unsigned int* crc) {
    *crc = 0xffffffff;
    Crc32FileRangeCont(filename, offset, len, crc);
    *crc = ~*crc;
    return true;
}

bool CFvCrc32::Crc32FileRangeCont(const char* filename, unsigned int offset, unsigned int len, unsigned int* crc) {
    unsigned char buffer[4096];
    DWORD bytesRead;
    if (!filename || !*filename) {
        m_error = 1;
        return false;
    }
    HANDLE file = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (file == INVALID_HANDLE_VALUE) {
        m_error = 2;
        return false;
    }
    if (SetFilePointer(file, offset, 0, FILE_BEGIN) == 0xffffffffUL) {
        m_error = 4;
        CloseHandle(file);
        return false;
    }
    BOOL ok = ReadFile(file, buffer, sizeof(buffer), &bytesRead, 0);
    while (ok && bytesRead) {
        unsigned char* data = buffer;
        unsigned char* end;
        if (len > bytesRead) {
            end = buffer + bytesRead;
            len -= bytesRead;
        } else {
            end = buffer + len;
            len = 0;
        }
        for (; data < end; ++data) {
            *crc = m_table[*data ^ *(unsigned char*)crc] ^ (*crc >> 8);
        }
        if (!len) break;
        ok = ReadFile(file, buffer, sizeof(buffer), &bytesRead, 0);
    }
    if (!ok) m_error = 3;
    CloseHandle(file);
    return true;
}
