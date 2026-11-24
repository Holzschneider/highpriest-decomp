// Reconstructed from original GameGuard evidence.

#include <mbstring.h>
#include <stdio.h>
#include <string.h>

static char* GetLine(char* text, char* line);
unsigned int MemGetPrivateProfileString(const char* section, const char* key, const char* defaultText, char* result, unsigned int size, char* text);

unsigned int MemGetPrivateProfileInt(const char* section, const char* key, int defaultValue, char* text) {
    char defaultText[64];
    char value[64];
    int number;
    sprintf(defaultText, "%ld", defaultValue);
    MemGetPrivateProfileString(section, key, defaultText, value, sizeof(value), text);
    number = 0;
    sscanf(value, "%ld", &number);
    return number;
}

unsigned int MemGetPrivateProfileString(const char* section, const char* key, const char* defaultText, char* result, unsigned int size, char* text) {
    char keyPrefix[64];
    char sectionPrefix[64];
    char line[256];
    sprintf(sectionPrefix, "[%s]", section);
    sprintf(keyPrefix, "%s=", key);
    size_t sectionLen = strlen(sectionPrefix);
    size_t keyLen = strlen(keyPrefix);
    text = GetLine(text, line);
    while (_mbsnbicmp((unsigned char*)sectionPrefix, (unsigned char*)line, sectionLen)) {
        if (!*text) {
            _mbsnbcpy((unsigned char*)result, (const unsigned char*)defaultText, size);
            result[size - 1] = 0;
            return strlen(result);
        }
        text = GetLine(text, line);
    }
    bool found = false;
    text = GetLine(text, line);
    while (line[0] != '[') {
        if (!_mbsnbicmp((unsigned char*)keyPrefix, (unsigned char*)line, keyLen)) {
            found = true;
            break;
        }
        if (!*text) break;
        text = GetLine(text, line);
    }
    if (!found) {
        _mbsnbcpy((unsigned char*)result, (const unsigned char*)defaultText, size);
    } else {
        _mbsnbcpy((unsigned char*)result, (unsigned char*)line + strlen(keyPrefix), size);
    }
    result[size - 1] = 0;
    return strlen(result);
}

static char* GetLine(char* text, char* line) {
    int length = 0;
    while (*text) {
        if (*text == '\r' || *text == '\n') break;
        *line++ = *text++;
        if (++length >= 255) break;
    }
    *line = 0;
    if (*text == '\r') ++text;
    if (*text == '\n') ++text;
    return text;
}

unsigned int MemGetPrivateProfileSectionNames(char* result, unsigned int size, char* text) {
    char line[256];
    char* dest = result;
    *dest = 0;
    do {
        text = GetLine(text, line);
        if (line[0] == '[') {
            char* close = (char*)_mbsrchr((unsigned char*)line, ']');
            if (close) {
                *close = 0;
                size_t length = strlen(line + 1);
                if (size < length + 2) length = size - 2;
                _mbsnbcat((unsigned char*)dest, (unsigned char*)line + 1, length);
                dest += length;
                *dest = 0;
                size -= length + 1;
                ++dest;
                if (size <= 1) break;
            }
        }
    } while (*text);
    *dest = 0;
    int length = dest - result;
    if (length > 0) --length;
    return length;
}
