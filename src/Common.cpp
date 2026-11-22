




#include <windows.h>
#include <stdlib.h>
#include <string.h>

static char* CONTAINER_NAME = "\x01\x19\xb0\x46\x7a\x25\xea\x6e\x48\x88\x63\x4d\xa3\x84\x85\x94\x41\x32\x3e";

class CCriticalSector {
public:
    CCriticalSector() { InitializeCriticalSection(&m_cs); }
    ~CCriticalSector() { DeleteCriticalSection(&m_cs); }
    void Lock() { EnterCriticalSection(&m_cs); }
    void Unlock() { LeaveCriticalSection(&m_cs); }

private:
    CRITICAL_SECTION m_cs;
};

char* _Enct(char* text)
{
    static char buf[1024];
    int len = lstrlenA(text);
    if (len) {
        int key = (int)text & 0xff;
        buf[1] = (char)key;
        buf[0] = 1;
        key *= 3;
        int key1 = key + 0x65;
        key++;
        key *= 3;
        int key2 = key + 0x65;
        key++;
        int pos = 4;
        for (int i = 0; i < len; i++) {
            key *= 3;
            buf[pos++] = text[i] ^ (char)(key + 0x65);
            key++;
        }
        int size = pos - 4;
        buf[pos] = 0;
        buf[2] = (char)(size >> 8);
        buf[3] = (char)size;
        buf[2] ^= key1;
        buf[3] ^= key2;
    }
    return buf;
}

char* _Dect(char* text)
{
    if (text == NULL)
        return NULL;
    if (text[0] == 1) {
        int key = text[1];
        text[2] ^= (char)((key *= 3) + 0x65);
        key++;
        text[3] ^= (char)((key *= 3) + 0x65);
        key++;
        int len = MAKEWORD(text[3], text[2]);
        for (int i = 0; i < len; i++) {
            key *= 3;
            text[i] = text[i + 4] ^ (char)(key + 0x65);
            key++;
        }
        text[len] = 0;
    }
    return text;
}

char* _Dect1(char* text)
{
    static CCriticalSector cs;
    static DWORD dwLastTid[10];
    static DWORD dwLastIndex = 0;
    static char* lpszLast[10];
    char tmp[1024];

    if (text == NULL)
        return NULL;

    cs.Lock();
    if (text[0] == 1) {
        int key = text[1];
        text[2] ^= (char)((key *= 3) + 0x65);
        key++;
        text[3] ^= (char)((key *= 3) + 0x65);
        key++;
        int len = MAKEWORD(text[3], text[2]);
        for (int i = 0; i < len; i++) {
            key *= 3;
            text[i] = text[i + 4] ^ (char)(key + 0x65);
            key++;
        }
        text[len] = 0;
    }

    DWORD index = 0;
    DWORD tid = GetCurrentThreadId();
    for (int i = 1; i < 10; i++) {
        if (dwLastTid[i] == tid) {
            index = i;
            break;
        }
    }
    if (index == 0 && dwLastIndex < 9) {
        index = ++dwLastIndex;
        dwLastTid[index] = tid;
    }

    char* last = lpszLast[index];
    lpszLast[index] = text;
    if (last && last != text && text[0] != 1 && last[0] != 1) {
        lstrcpyA(tmp, last);
        int len = lstrlenA(tmp);
        if (len) {
            int key = (int)tmp & 0xff;
            last[1] = (char)key;
            last[0] = 1;
            key *= 3;
            int key1 = key + 0x65;
            key++;
            key *= 3;
            int key2 = key + 0x65;
            key++;
            int pos = 4;
            for (int i = 0; i < len; i++) {
                key *= 3;
                last[pos++] = tmp[i] ^ (char)(key + 0x65);
                key++;
            }
            int size = pos - 4;
            last[pos] = 0;
            last[2] = (char)(size >> 8);
            last[3] = (char)size;
            last[2] ^= key1;
            last[3] ^= key2;
        }
    }
    cs.Unlock();
    return text;
}

char* _Dect2(char* text)
{
    static CCriticalSector cs;
    static char* lpszLast;
    char tmp[1024];

    if (text == NULL)
        return NULL;

    cs.Lock();
    if (text[0] == 1) {
        int key = text[1];
        text[2] ^= (char)((key *= 3) + 0x65);
        key++;
        text[3] ^= (char)((key *= 3) + 0x65);
        key++;
        int len = MAKEWORD(text[3], text[2]);
        for (int i = 0; i < len; i++) {
            key *= 3;
            text[i] = text[i + 4] ^ (char)(key + 0x65);
            key++;
        }
        text[len] = 0;
    }

    char* last = lpszLast;
    lpszLast = text;
    if (last && last != text && text[0] != 1 && last[0] != 1) {
        lstrcpyA(tmp, last);
        int len = lstrlenA(tmp);
        if (len) {
            int key = (int)tmp & 0xff;
            last[1] = (char)key;
            last[0] = 1;
            key *= 3;
            int key1 = key + 0x65;
            key++;
            key *= 3;
            int key2 = key + 0x65;
            key++;
            int pos = 4;
            for (int i = 0; i < len; i++) {
                key *= 3;
                last[pos++] = tmp[i] ^ (char)(key + 0x65);
                key++;
            }
            int size = pos - 4;
            last[pos] = 0;
            last[2] = (char)(size >> 8);
            last[3] = (char)size;
            last[2] ^= key1;
            last[3] ^= key2;
        }
    }
    cs.Unlock();
    return text;
}

wchar_t* _WDect(wchar_t* text)
{
    static CCriticalSector cs;
    static wchar_t* lpszLast;
    wchar_t tmp[1024];

    if (text == NULL)
        return NULL;

    cs.Lock();
    if (text[0] == 1) {
        int key = text[1];
        text[2] ^= (wchar_t)((key *= 3) + 0x65);
        key++;
        text[3] ^= (wchar_t)((key *= 3) + 0x65);
        key++;
        int len = MAKEWORD(text[3], text[2]);
        for (int i = 0; i < len; i++) {
            key *= 3;
            text[i] = text[i + 4] ^ (wchar_t)(key + 0x65);
            key++;
        }
        text[len] = 0;
    }

    wchar_t* last = lpszLast;
    lpszLast = text;
    if (last && last != text && text[0] != 1 && last[0] != 1) {
        wcscpy(tmp, last);
        int len = wcslen(tmp);
        if (len) {
            int key = (int)tmp & 0xff;
            last[1] = (wchar_t)key;
            last[0] = 1;
            key *= 3;
            int key1 = key + 0x65;
            key++;
            key *= 3;
            int key2 = key + 0x65;
            key++;
            int pos = 4;
            for (int i = 0; i < len; i++) {
                key *= 3;
                last[pos++] = tmp[i] ^ (wchar_t)(key + 0x65);
                key++;
            }
            int size = pos - 4;
            last[pos] = 0;
            last[2] = (wchar_t)(size >> 8);
            last[3] = (wchar_t)size;
            last[2] ^= key1;
            last[3] ^= key2;
        }
    }
    cs.Unlock();
    return text;
}

char* stristr(char* text, const char* find)
{
    if (text == NULL || find == NULL || strlen(find) == 0)
        return text;

    char* lowerText = _strlwr(_strdup(text));
    char* lowerFind = _strlwr(_strdup(find));
    if (lowerText == NULL || lowerFind == NULL) {
        free(lowerText);
        free(lowerFind);
        return NULL;
    }

    char* found = strstr(lowerText, lowerFind);
    if (found)
        found = text + (found - lowerText);
    free(lowerText);
    free(lowerFind);
    return found;
}

wchar_t* wcsistr(wchar_t* text, const wchar_t* find)
{
    if (text == NULL || find == NULL || wcslen(find) == 0)
        return text;

    wchar_t* lowerText = _wcslwr(_wcsdup(text));
    wchar_t* lowerFind = _wcslwr(_wcsdup(find));
    if (lowerText == NULL || lowerFind == NULL) {
        free(lowerText);
        free(lowerFind);
        return NULL;
    }

    wchar_t* found = wcsstr(lowerText, lowerFind);
    if (found)
        found = text + (found - lowerText);
    free(lowerText);
    free(lowerFind);
    return found;
}
