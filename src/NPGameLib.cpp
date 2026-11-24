#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif

#include <windows.h>
#include <mmsystem.h>
#include <io.h>
#include <mbstring.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <tlhelp32.h>

#include "FileAuthClient.h"
#include "CommPipe.h"

struct _GG_AUTH_DATA;

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

struct LANGANDCODEPAGE {
    WORD language;
    WORD codePage;
};
LANGANDCODEPAGE* lpTranslate;
static CSafeAPI safeWinInet;

struct rc4_key_st {
    unsigned int x;
    unsigned int y;
    unsigned int data[256];
};

class CCriticalSector {
public:
    CCriticalSector() { InitializeCriticalSection(&m_section); }
    ~CCriticalSector();

private:
    CRITICAL_SECTION m_section;
};

class CLog {
public:
    CLog() : m_file(INVALID_HANDLE_VALUE), m_count(0) {}
    ~CLog();
    bool Open(const char* fileName);
    bool Write(const char* fmt, ...);
    void Close();

private:
    HANDLE m_file;
    CCriticalSector m_section;
    char m_line[0x1000];
    SYSTEMTIME m_time;
    unsigned int m_pad;
    rc4_key_st m_rc4;
    unsigned int m_xorSeed;
    unsigned int m_limit;
    unsigned int m_count;
};

class CFvCrc32 {
public:
    CFvCrc32();
    ~CFvCrc32();
    bool Crc32File(const char* filename, unsigned int* crc);
    unsigned int GetError() { return m_error; }

private:
    unsigned int m_table[256];
    unsigned int m_error;
};

char* _Dect(char* text);
char* _Dect1(char* text);
char* _Enct(char* text);
void RC4(rc4_key_st* key, unsigned int len, unsigned char* indata,
         unsigned char* outdata);
void RC4_set_key(rc4_key_st* key, int len, unsigned char* data);
unsigned int MemGetPrivateProfileString(const char* section, const char* key,
                                         const char* defaultValue,
                                         char* output, unsigned int size,
                                         char* data);
unsigned int MemGetPrivateProfileInt(const char* section, const char* key,
                                     int defaultValue, char* data);

static void GetProcessList();
static void GetModuleList();
static bool CheckGameMonCrc(const char* fileName, char* valueName, unsigned int expected);
static unsigned int ReadMemCrc(PROCESS_INFORMATION& process);
static bool CheckVersion(char* fileName, unsigned short major, unsigned short minor, unsigned short build, unsigned short revision);
static unsigned int InitTimeAddr();
static unsigned int CheckTimeAddr();
static unsigned int InitSocketAddr();
static unsigned int CheckSocketAddr();
static bool CheckRsaBase();
static bool RemoveCompatibleMode();
static void CheckEncryptedClient();
static DWORD WINAPI SpeedCheckEventThread(void* guard);
static LONG WINAPI FvUnhandledExceptionFilter(_EXCEPTION_POINTERS* exceptionInfo);
static BOOL SendGameMonMessage(const void* message, unsigned int size);
static FARPROC WINAPI MyGetProcAddress(HINSTANCE module, LPCSTR name);

typedef unsigned int (__cdecl *NPSCInitProc)(unsigned int gameMonId, unsigned int arg1, unsigned int arg2, HANDLE event);
typedef void (__cdecl *NPSCCloseProc)(unsigned int gameMonId, const char* userId);

class CGameGuard {
public:
    CGameGuard();
    ~CGameGuard();

    unsigned int InitNPGameMonI();
    unsigned int PreInitNPGameMonI(const char* userId);
    void SetHwndToGameMonI(HWND hwnd);
    void RunFAQWebPageI(unsigned int errorCode);
    unsigned int CheckNPGameMonI();
    bool SendCSAuthToGameMonI(unsigned int auth);
    bool SendUserIDToGameMonI(const char* userId);
    const char* GetInfoFromGameMonI();
    bool SendCSAuth2ToGameMonI(_GG_AUTH_DATA* auth);
    bool CloseNPGameMonI();
    bool InitNPSC();
    unsigned int IsDebugger();
    void FtpSendErl();

private:
    friend void GetProcessList();
    friend void GetModuleList();
    friend bool CheckGameMonCrc(const char* fileName, char* valueName, unsigned int expected);
    friend bool CheckVersion(char* fileName, unsigned short major, unsigned short minor, unsigned short build, unsigned short revision);
    friend LONG WINAPI FvUnhandledExceptionFilter(_EXCEPTION_POINTERS* exceptionInfo);
    friend FARPROC WINAPI MyGetProcAddress(HINSTANCE module, LPCSTR name);
    friend unsigned int InitTimeAddr();
    friend unsigned int CheckTimeAddr();
    friend unsigned int InitSocketAddr();
    friend unsigned int CheckSocketAddr();
    friend bool CheckRsaBase();
    friend bool RemoveCompatibleMode();
    friend DWORD WINAPI SpeedCheckEventThread(void* guard);

    bool GetGameGuardDirectory(char* path, unsigned int size);
    bool GetGameDirectory(char* path, unsigned int size);
    bool IsWinNT();
    bool CloseNPSC();
    int CheckSingleInstance(unsigned int timeout);
    int CheckGameMonSingleInstance(unsigned int timeout);

    unsigned char m_active;
    unsigned char m_closing;
    unsigned char m_pad02[2];
    HWND m_hwnd;
    unsigned int m_callbackMsg;
    unsigned int m_callbackArg;
    unsigned int m_initResult;
    HANDLE m_closeEvent;
    HANDLE m_readyEvent;
    unsigned char m_npscActive;
    unsigned char m_debuggerFlag;
    unsigned char m_useNpsc;
    unsigned char m_pad1f;
    CLog m_log;
    CCommPipe m_pipe;
    CFileAuthClient m_authClient;
    unsigned int m_processId;
    char m_clientId[32];
    HMODULE m_npscModule;
    HANDLE m_npscThread;
    char m_userId[64];
    unsigned char m_flag1670;
    unsigned char m_isWinNt;
    unsigned char m_pad1672[2];
    unsigned int m_npscInterval;
    unsigned int m_npscOptions;
    HANDLE m_gameMonMutex;
    HANDLE m_gameMonReadyEvent;
    unsigned char m_disableAppCompat;
    unsigned char m_pad1685[3];
    NPSCInitProc m_npscInit;
    NPSCCloseProc m_npscClose;
};

inline CGameGuard::CGameGuard()
    : m_active(0), m_closing(0), m_hwnd(0), m_callbackMsg(0),
      m_callbackArg(0), m_initResult(0), m_closeEvent(0), m_readyEvent(0),
      m_npscActive(0), m_debuggerFlag(0), m_useNpsc(0), m_pad1f(0),
      m_processId(0), m_npscModule(0), m_npscThread(0), m_flag1670(0),
      m_isWinNt(0), m_npscInterval(1000), m_npscOptions(0),
      m_gameMonMutex(0), m_gameMonReadyEvent(0), m_disableAppCompat(0),
      m_npscInit(0), m_npscClose(0) {
    m_userId[0] = 0;
}

void* __cdecl operator new(unsigned int size);

BOOL CALLBACK NPGameMonCallback(DWORD msg, DWORD arg);
bool CALLBACK CommPipeCallback(CCommPipe* pipe, unsigned int msg,
                               unsigned int value);
#define s_logInitGameMonInactive "\001\014\211\306\3746(\210c\220>\305H\315|\243\363*IQ\371\215"
#define s_logInitGameMonActive "\001\346\027k\216o\017\217\345\263\263\300\263\245\276\273\336'\030\3603\026\340\210\216"

#define s_logSendCSAuth "\0015\004N[4B\3313\332\225\217xd\204"
#define s_gameGuardDirectorySuffix "\001\012\203\310#\361:'r9\3063]\242"
#define s_kernel32Dll "\001\332\363\036\004\343\271\364b\042\020\2201\362\227F"
#define s_process32First "\001D1\301\307\177\004\026\374\236X\335n\024\0320}"
#define s_process32Next "\001\024\241\017\333\205\253\227\340\274\322\203\223T\240\340\234w\357\361\014\363\322"
#define s_logCloseGameMonBegin "\001\2336\317\211\373\221@\267\341\026L;*k.\0151vh\236\226l\347"
#define s_logCloseGameMonEnd "\001G:\364\255o-\214S\225\232\320\237^\267\342)ez\003a\015\007"
#define s_logSetHwndBegin "\001\316\317\265\026\207G=\346\2667\3015.C\021\3643B\243\306\265\251"
#define s_logSetHwndCallback "\001\220\025a\303i\353\206\274\266\340\254\261\201\216W\351\261\231m\255\234\014\362\336D@\004\004"
#define s_logSetHwndCallbackFailed "\001Wjb\375r\036\343\204\212\231L\2161gK\203i\344\367\330\022\376\234c"
#define s_logSetHwndEnd "\0016\007V\036\237\077\305\236^\377)=V\233\311l\333\012\013\316\255\021\226\077%y\033"
#define s_logProcessFormat "\x01\x5a\x73\x8e\xce\x23\x66\x28\xb7\xbd\x83\x02\xba\x23\x17\xdf\x17\xfe\xf6\x87\x23\x53\x8b\x1a\xc7\xce\x23\x62\xda\x05\xb7\xae"
#define s_logProcessHeader "\x01\xbb\x96\xd4\x0b\xe7\x2c\x3c\x51\xaa\xd5\x58\x9a\x29\x0f\xee\xa7\x5f\x96\x7b\x6a\x37\x5e\x93\x72\x0f\xe6\xab\xfa\x97\xc7\xa7\xc2\xff\x82\xd3\x18\x12\xff\x77\x01\xaf\xc6\xbb\x88\x4e\xc1\x31\x6b\x4b\x8f"
#define s_logModuleFormat "\x01\x73\xbe\x6b\xb3\xca\x2b\x79\x2a\xf4\xee\x83\x07\xaf\x2e\x23\x8a\x17\xfe\xf6\x8a\x23\x53\x8b\x1a\xc7\xcb\x70"
#define s_logModuleHeader "\x01\x4d\x4c\x2c\xb1\xa4\x9b\x71\x0d\xcd\x79\xad\xae\xb8\xa9\xe0\xe8\xb1\xcc\xdd\x10\xe9\xb4\xd5\x38\xa1\x9c\x2f\xe1\xca\x01\x24\x0c\x15\xfe\xfd\xf0\x89\x76\xb4\xcb\x04\x0f\x24\x5a\xfc\x24\x65\x28\x01\x4d\xc9\x18"
#define s_logExceptionBytes "\x01\xcf\xd2\xa2\x67\x26\x05\x6a\x2e\x07\x17\x32\x26\x01\xca\xd2\x6d"
#define s_exceptionByteFormat "\x01\xe1\x08\x54\x09\x8d\x42\xd1\xf4"
#define s_logUnhandledException "\x01\xaf\x72\xbe\xc7\xc6\xa5\x0a\x8e\x20\x6d\x5b\x93\x21\xaa\xf2\x8e\x4b\xca\xc3\x26\x2a\x7e\xe3\xbc\x99\x02\xba\x26\x43\xf2\x1b\xfe\xb0\xc6\x26\x42\x91\x1a\xc2\x82\x76\x6e\xdf\x51\xaf\xa3\xf3\xc4\x13\xf7\xc3\x33"
#define s_logResolverException "\x01\x87\xfa\x0c\x83\x1c\x47\x47\x95\x7e\xfa\xa3\xd7\x1c\xfc\xcf\x6f\x25\x3a\xca\x89\xf3\x63\xb3\x36\x18\xc5\x53\xbb\xe9\x92\x6a\x16\x93\x42\xcb\xce\x62\x26\x9b\x44\xe1\xea\xb2\xce\x0b\xaa"
#define s_versionDll "\x01\xf6\x47\x05\x35\x07\x2d\x25\x52\x85\x99\x30\xf7\x9e\x63"
#define s_versionSizeProc "\x01\x6b\xa6\x3c\xfd\x02\x1a\xc5\xab\x13\xd3\x0d\x2f\x65\x0d\xda\x3d\x41\x8f\xe5\xbc\xa8\xdd\x8a\x98\xba\x97"
#define s_versionInfoProc "\x01\xc2\xab\x29\xa0\x8b\x77\x04\x96\x5a\xbe\x9c\xf2\x8c\x40\xbb\xc0\x28\x42\x34\x21\x61\x22"
#define s_versionQueryProc "\x01\x4e\x4f\x28\xfd\x5f\x95\xbf\x76\x27\x8d\x4f\x8d\xab\xfb\x8b\x56\x93"
#define s_logFileVersion "\x01\x31\xf8\x35\xea\x68\x12\x2a\x2d\x6a\x26\x31\x49\x0e\x9c\x69\x31\x19\xed\x2d\x1c\xc8\x6c\x4c"
#define s_registryKey "\x01\xc7\xba\x26\x3d\xfa\xb1\x0b\xd3\x36\x16\x54\x0b\xc1\x20\x4a\xa8\xff\x99\xa8\xe0\x97\x90\xb0\xba\xe8\x0f\x03\xc2\x50\x1d\xe1\x92\x99\x95\x4b\xf2\x10\x67\x4c\x85\x72\xe5\xb9\x9e\x3e\xf3\x81\x67\x2c\x7f\xf5\x92\xe2\x52\xef\x55\x14\xc7\x47\xbf\xa7\xdb\x2d\x4f\xc7\x53\x93\x97"
#define s_registryValue "\x01\x40\x25\xb8\x75\xa5\xee\x71\x0b\x18\xf0\xb9\x51\xec\x4e\x0f\x34\x34\x64\x3c"
#define s_logTimeGetTimeHookModule "\x01\x75\xc4\x96\x80\xfe\x83\x96\x64\x8e\xc0\xd9\x38\xe9\xf1\x02\xe4\x83\x44\x40\x58\x49\xd4"
#define s_logTimeGetTimePointers "\x01\xf9\x50\x26\xe0\x12\x6c\xdb\xfc\xe8\x90\x21\xdc\xe9\xa8\xf4\x9c\x05\xa8"
#define s_timeGetTimeProc "\x01\x48\x3d\xfb\x7d\x3d\x58\xbd\x86\x19\xd9\x14\x90\x49\xc0"
#define s_winmmDll "\x01\x96\x27\xa7\x34\x6b\x51\x9b\x76\xa4\xb3\xd2\x1f"
#define s_logTickCountHookModule "\x01\x55\x64\x76\x20\x1e\xe3\xb6\x97\xfd\x77\xf9\x98\x09\x11\x22\x44\xa3\x24\x60\x38\x69\x74"
#define s_logTickCountPointers "\x01\x27\xda\xc8\xc9\xb7\xa1\xe5\xf6\x9e\x5a\x4f\xc6\x3f\x52\xaa\xd6\xd3\xa2"
#define s_tickCountProc "\x01\xf6\x47\x02\x24\x07\x2b\x02\x52\x89\x9c\x5d\xfc\x87\x61\x12"
#define s_logPerformanceHookModule "\x01\x44\x31\xdf\xd5\x7f\x06\x1f\xc4\xa8\x62\xb0\x2d\x08\x14\x2b\x61\x72\x31\x49\x4d\x88\x11"
#define s_logPerformancePointers "\x01\x62\x8b\xd5\x96\xde\xa0\xd8\xff\xf3\x8b\x52\x2f\xb2\x33\x57\xbf\xde\xb3"
#define s_performanceProc "\x01\xc4\xb1\x5b\x4c\xe5\x8c\x86\x6c\x28\xc4\x6e\xeb\x8f\xab\xa9\xe4\xa6\xf2\x89\xbe\x5f\xbc\xfa\x81\x7d\xf3"
#define s_clockKernelDll "\x01\xd6\xe7\xe2\x68\x27\x8d\x58\xbe\xa6\xa4\xcc\x1d\xb6\xc3\x2a"
#define s_ttCoreDll "\x01\x95\x24\xaf\x7c\xe5\x0f\x72\xe2\x8c\xda\x71\x14\xcd"
#define s_acLayersDll "\x01\xda\xf3\x1e\x2e\xe5\x87\xfb\x7e\x2b\x51\xd1\x31\xf2\x97\x46"
#define s_logTimeGetTimeHook "\x01\x3f\x22\x91\x5e\x14\xc5\x5c\xfe\x87\xf5\x1b\x0a\x8b\x1f\xd7\xd6\x5b"
#define s_logTickCountHook "\x01\xeb\x26\xa5\x72\x88\x81\x68\x62\xb8\x62\x98\xe6\xb7\xdb\x03\xea\xf7"
#define s_logPerformanceHook "\x01\xfb\x56\x35\xa2\x98\x71\xf8\xd2\x5e\x36\x28\x56\x87\x0b\xf3\xba\xe7"
#define s_logSendHookModule "\x01\x86\xf7\x0a\xdb\x9d\x60\x0d\x18\x1f\xc9\x4a\xef\xa2\xd7\x3b\xf4\x6e\x6d\x1e\xd6\x22\x57\xde"
#define s_logSendPointers "\x01\x38\x0d\x70\x2a\x21\x6b\x2c\x2b\x4c\x58\x80\x71\x4c\x59\xb8\x24\x0c\xd5\x58"
#define s_sendProc "\x01\xfd\x5c\x49\x53\xfc\x6a\x21"
#define s_logWSASendHookModule "\x01\xc6\xb7\x49\x1b\x5d\xa0\xcd\x7c\xe9\x26\x3d\xe6\xac\x1b\x9a\x7b\x22\x5a\x11\xd7\x68\x0f\xe3\xbb\xe2\x9f"
#define s_logWSASendPointers "\x01\x8e\x0f\x75\x3c\x29\xe6\x7d\xa6\xec\xdb\x4c\xbb\x2f\x67\x06\xab\x3e\x4f\xa3\xfb\xa2\x5f"
#define s_wsaSendProc "\x01\x18\xad\x47\xae\x77\xe4\x7b\xd4\x22\x79"
#define s_ws2Dll "\x01\xf5\x44\x0f\x3f\x62\x5e\x22\x83\x7b\x3a\x11\xf4\x6d"
#define s_logSendHook "\x01\x10\x95\xf7\x69\xf3\x62\x0b\x79\x37\x60\x26\x75\x40\x5d\x95\x79\x2c\x2d"
#define s_logWSASendHook "\x01\xb8\x8d\xf2\x91\xab\xea\xa3\xb1\xbb\xae\x71\x9a\xf1\x9b\x7c\xad\x9c\x48\xb0\x81\x3c"
#define s_providerSignatureIE5 "\x01\xa1\x48\x18\x3f\x14\xd7\x27\x75\x01\xed\x73\x59"
#define s_logProviderIE5 "\x01\x9d\x3c\xfa\x52\x4a\x85\x87\x89\x82\x69\x73\x34\x45\xd8\x75\x51\x92\xfc\x84\xe5\x39\xb1\xeb\xb8\xf6\xdf"
#define s_providerSignatureIE6 "\x01\x49\x40\xf0\x77\xcc\x4f\xdf\x2d\x69\xe5\x9b\x91"
#define s_logProviderIE6 "\x01\xe4\x11\x7b\x2f\xc3\x28\x76\x14\xeb\x64\x12\x89\x6c\x55\xc4\x8c\x9b\xd1\x45\x18\x70\x1f\x9a\x65\x1f\x92"
#define s_logProviderVersion "\x01\x12\x9b\x1a\x21\x5b\x81\x28\x4f\xa3\xaf\xb4\x22\x2a\x0d\x87\x7b\xb8\xde\x4e"
#define s_providerSuffix "\x01\x4e\x4f\x2a\xf7\x48\x94\x8f\x61\x23\x8c\x53\xf5\xae\xfb\x92"
#define s_logProviderSignature "\x01\xb3\x7e\xad\x01\x46\xa1\xe5\xbb\xb3\xfb\x91\x87\xe5\xf6\x9e\x5a\x45\xe6\x33\x57\xbf\xd4\x93\xda\x02\x9e\x71\x7a\x1f\xd3\x2b\xb8\xaf"
#define s_providerSignatureName "\x01\x46\x37\xd7\x80\xdb\x28\x48\xca\x4e\x92\x9c\x66"
#define s_providerRegistryKey "\x01\x7e\xdf\x8b\xe8\x05\x11\xea\x64\x13\xfd\x83\xb7\xb7\x4e\xcd\x31\x6d\x4c\x99\x7d\xfe\x8b\xfd\x01\xeb\x9f\x72\x24\x7d\xf5\xaf\xd3\x4a\xe6\x4a\x3f\xcf\x51\xbf\xa6\xde\x3b\x55\xf7\x6a\x95\x81\x75\x2b\x9b\x53\xa9\x96\xda\x97\x50\xa0\xc0\x35\x64\x3c\x33\x2e\x21\x03\x2c\x33\x1b\xa9\x85\x67\xe3\x86\x60\x01\x19\x1b\xd7\x46\xaa\xe1\x9f\x26\xe9\x65\x21\x57\x97\x77\x1d\xa6\xbd\xab\x29\x7e\x7f"
#define s_logIEVersion "\x01\x2e\xef\x08\x02\x5f\xa7\x98\xc6\x50\xec\x7f\x14\xc4\x0d\xfe\xf6\xc1"
#define s_ieVersionName "\x01\x66\x97\xf9\x65\xb7\xdd\x35\x62\x35\x29"
#define s_ieRegistryKey "\x01\x21\xc8\xb4\xbf\x92\x56\xbd\xe3\x94\x6a\xe4\xe0\x20\xe9\xda\x16\x0a\x1b\x1e\xea\xa9\x8c\xe0\x5a\xa1\xdd\x13\x32\x28\x54\xb9\x41\x3d\x78\x3d\x43\xcf\x15\xfb\x88"
#define s_logCompatibilityMode "\x01\xf0\x35\xcb\x82\x13\xc0\x30\x98\x50\xcc\x4a\xdd\x29\x3d\xdd\x86\x90\x70\x42\x81\x39\xfe"
#define s_compatibilityLayersKey "\x01\x44\x31\x8e\xce\x7f\x0f\x00\xe2\x99\x53\xf9\x51\x2d\x30\x27\x77\x27\x62\x03\x1b\xc4\x15\x43\x1c\xf6\x65\x53\x9a\x73\x19\xaa\xb1\xb4\xb2\x79\x2f\x22\x4c\xda\x21\x6e\x84\xae\xbe\xc9\x76\xea\x99\xc9\xa1\xdc\x7e\x9f\x64\x24\x54\xac\x87\x10\xcc\x27\x8a\x78\xe9\x49\xc8\x29\x6f\xe3"
#define s_logNpscDebugger "\x01\x0b\x86\xc4\xf3\x63\x3e\x19\x82\x3a\xf2\xd7\x0a\xc4\x37\x69\x12\xea\xc2"
#define s_logProviderFileSize "\x01\x22\xcb\x93\x74\x27\x59\xc7\x25\xb6\xde\x46\xc2"
#define s_logProviderOpenFailed "\x01\x0f\x92\xff\x69\x3b\x7f\xe9\xee\xc5\x43\xf6\x7a\x5b\x8f\x53\xf2\xf3\x97\x3c"
#define s_runFaqLocalSuffix "\001p\265N\035\273L\255\034\343P\311C\250\301w\016\021\347\212N\356#\004<7"
#define s_runFaqLocalPageAuth "\001\306\267Zb\002\377\227"
#define s_runFaqLocalPageUpdate "\001\310\275t\270\344\205j"
#define s_runFaqLocalPageInit "\001\316\317\245\032\213S"
#define s_runFaqLocalFormat "\001}\334\305\205j\244\340\373\377\211N"
#define s_runFaqLocalCreateFailed "\001>\037\217\211_\331~4U\252\324Y\325\025@\347\247\014\226>8e\021\301h\017\343\357"
#define s_runFaqLocalCreateOk "\001\032\263]]\263\345\372\240\351\206\220\255\271\311PW\273`"
#define s_runFaqWebPageDefault "\001e\224\366)\260\211"
#define s_runFaqWebPageHack "\001\362;\357\305.\243\302>"
#define s_runFaqWebPageSpeedHack "\001\370M%\2534u8c"
#define s_runFaqWebPageVirus "\001\370M%\2534u8b"
#define s_runFaqWebPageCollision "\001\362;\357\305.\243\302;"
#define s_runFaqWebPageExecute "\001\204\361\011o`\031\204`"
#define s_runFaqWebPageInit "\001\004q\211\357\340\231\004\343"
#define s_runFaqWebPageUnknown "\001\320\325\275Sl}\020\256"
#define s_runFaqWebPageGameMon "\001\025\244&\231\000\370"
#define s_runFaqWebPageAuth "\001*\343\353\255\263\322\004\004\352r\036\343"
#define s_runFaqRegistryKey "\001\316\317\216x\365!:\324\203-\363\007\011{\037\300!J\265\327\222\223\332\263\276\254\276\336\006\033\302|\002\352\210\267\231H\303.cQ\222"
#define s_runFaqRegistryFormat "\001LI?U\270iH\231p\003\313\312\237\206{s7H\3310_\224\275\277\3047\347\252\246\272\336\022\226h%\032\236\200-\362e\212\012\315\\\334"
#define s_runFaqRegistryCreateFailed "\001\302\253\031\265\233mb\266s\373\214\366\227_\362\311)yz$a\015\014:5O\312\261_\302\322\\\017\037\037\207\013\247"
#define s_runFaqRegistryCreateOk "\001F7\322\225\363\036\006\330S\223\2139b\332E"
#define s_logCloseNPSCBegin "\001\254i{\326\224N\357h.\011\027FhB\030\034\302="
#define s_logCloseNPSCTimeout "\001\207\3724\372&pR\237u\353\243\333S\372\274E\037r\365\253\302F"
#define s_logCloseNPSCEnd "\001o\262Ae\307U\224\213M\022\254u\373\216\370\220V"
#define s_npscFileName "\001\3572\307\310[\311\004@\347\247\014"
#define s_logInitNPSCBegin "\001\331\360\007\035[\261\2652\375\023\272\004\366\134\320>i"
#define s_logInitNPSCPath "\001[v\235y4\004\3237\034"
#define s_logInitNPSCAuthFailed "\0018\015y81q 1\012\034\331%4\033\350r_\303dU\210\311\310\324h}uM"
#define s_logInitNPSCLoadLibraryFailed "\001\346\027j\377=N\242\307\263\245\374\202\220\246\366\335\013\036\362\077R\252\202"
#define s_logInitNPSCProcFailed "\001\275\234\036'<0U:~\017<\324-fU\376\140U\201 \034\200"
#define s_logInitNPSCCreateEventFailed "\001\201\350\323O/5H\3000}\227\271\243\3249\327\265\355\264\310u\221j\077p\256\244\022\331\140\237E\314D\221iy"
#define s_logInitNPSCCreateThreadFailed "\001\215\014C\023[\3214L\204\210\245\322|\345\241\250\267\315T\234)'V\254\251\016\310!\235\010\205\015\325"
#define s_logInitNPSCThread "\001\321\330\334/\335%\234@\346@\324/v\325\237\221{\014\365t\377\205\270\240\245\201\325\326\335\025\245\341"
#define s_logInitNPSCResultFailed "\001\202\353\355n\300*vl\206~\357\263\375\033\367\214mk|\346\247\317\016\2773\037"
#define s_logInitNPSCSuccess "\001\345\024x\321oU\231Ni\267\246\310\265c35"
#define s_singleInstanceMutex "\001#\316\255e\363y\031\313[\202\236\3067h\373}\253"
#define s_singleInstanceMutexAlt "\001\374YCH<i\042-\367\005"
#define s_logSingleInstanceCreateFailed "\001\245T*\237\240\021\310\014\220F\205k\320\042:\344\311\227g\035\300h\350\300\264\261\361\255\351\315\335\025\245\341"
#define s_gameMonSingleInstanceMutex "\001\007z\251i\257\355\335\027\367V\032J\213\134\077\301\206"
#define s_gameMonSingleInstanceMutexAlt "\001\344\021k0\3041Z%\337L"
#define s_logGameMonSingleInstanceCreateFailed "\0012\3735\360\077>W\202\311E\232$\017\355\345\013\22688r\037\3077\017\253\376\256\242\366\302\302\372\272\316"
#define s_logGameMonSingleInstanceTimeout "\001~\337\300\374\013\032\373^\035\341\306\212\226U\313\042fF\326^\362\276\315\007\263"
#define s_gameMonCheckMutex "\001\322\333\304\320\222\134\260\316*W\0273v-2\030\021"
#define s_gameMonCheckMutexAlt "\001\2336\334\207\343\206}\202\350\001"
#define s_logGameMonInactiveTooLong "\001\216\017q(\022\302M\250\314\3571\372g2s\234|O\340\252\363kn\025\223/"
#define s_logGameMonNotRunning "\001g\232\020\015K\307|\375\265z\360\077>W\202\311E\232\001\017\352\256_\200nz"
#define s_logGameMonExited "\001\021\230\026\177\205eZ\217\253\270\266m05d\333;\030\207\275\244\3149\262\367\270"
#define s_logGameMonMutexMissing "\001b\213\305\204\346\206\201\264\230\353-\026\363v\077\340\210\313\235@\216,rZ\230;\354\266\327\037\262\312j>"
#define s_logGameMonSingleMutexMissing "\001\031\260UW\035\375bW\243P~\205\210\215\274c3pF\3040V\301\272\254\311u\244\340\344\244"
#define s_ftpBegin "\001\327\352\375x\347\202\174\003\005\036\302\134\257"
#define s_ftpRegistryKey "\001\277\242\056\305\264l\343\011\022\140\212\372b\364\044\057\312\254\013\323\051\044r\012\357\074\177\264\344\256\242\355\227\245\276\273\336\055\002\377a\026\323\250\273\275F\303\046NV\224"
#define s_ftpRegistryBuild "\001\045\324\270\034\015\260\015\224\020\311\100\373E\255\360\140"
#define s_ftpRegistryValue "\001\007z\255K\261\360\320\004\370e\042P\207"
#define s_ftpRegistryBuildSet "\001\265\204\310\314\235\340\035\244\1409P\213\265\035\3000"
#define s_ftpBuildTimestamp "Tue Apr 17 14:15:17 2007"
#define s_ftpCloseLog "\001\321\330\312\032\3310\212A\313L\324\076q\261"
#define s_ftpWinInetDll "\001\256o\215\274\363i\047M\307k\270\237F\333"
#define s_ftpInternetOpenA "\001\175\334\300\351w\360\240\372\277\311I\277y1\133\231"
#define s_ftpInternetConnectA "\001\000ex8\342\251\265\333Z\260\314\0423\043N\374g1I"
#define s_ftpInternetCloseHandle "\001\323\336\300\373\041R\316H\211\213w\001\223Y\250\257\337\237\135\266\303\043"
#define s_ftpFtpPutFileA "\001\262\173\241q\252\243\342\072R\355S\213\213B"
#define s_ftpInternetAgent "\001\027\2522\260\243\325\043G"
#define s_ftpPassword "\001\266\207\340\326H\274sD\353\000\232\371\313\333\310\353\031\316\306Is\233\007\267\373\033g\002\273\236\052\072\046\073\202\201\325\336\367\204\206\334A\253\237\075E\046\076"
#define s_ftpUser "\001\316\317\243L\335\002\034\357"
#define s_ftpServer "\001i\240\022\262\364\246\346\224\023\303\074z\007\341"
#define s_ftpReason1 "\001\321\330\307\035\3017\230\135\326"
#define s_ftpReason2 "\0016\007KF\320m\371\211"
#define s_ftpReason3 "\001KF\01083h"
#define s_ftpReason4 "\001\362\073\356\236p\372\206"
#define s_ftpReason5 "\001\252cf68\135\205"
#define s_ftpReason6 "\001\347\032\217\240\314W\357r\032\336R"
#define s_ftpReason7 "\001\305\264SM\221\270\254\324\174"
#define s_ftpReason8 "\001\026\247\053\240\352\332\025\360"
#define s_ftpReason9 "\001Vgj\354\254\034\323"
#define s_ftpReasonOther "\001\323\336\320\305\047\137"
#define s_ftpRemoteFormat "\001\332\363F3\301\252\367b\011V\303m\362\247D\3079\077n\352\325t\375\053K\347\235Z\305\004os\041\226\041p\343\370\205\342\375\274\307\357\342\337\331\225\273\043\100\353Y\022\337\027\234\047\302\360\323\053\270\263\003\260\267\3374\057\240W\353\223\020\3738\136\232\004\272\020xaC\331V"
#define s_logOpenFormat ""
#define s_logWriteFormat "\001\310\275j\254\344\207<{\331\035\362\035\236\000\230\003\250\2475YG\361\330\001\274(\023TN"
#define s_logCloseFormat "\001\024\241L\260\335\344\371\270\365\254\321\300\015\351\330\232\177\241\372\004\356\320\027\015\015\025\254\274\244\211\021\345\214\005sh\020\253\140j-a\036\331P\254\344\207<{\331\035\362\035\236\000\230\003\250\2475YG\361\335R\2740]dy8u,Q@\215D\036x\222\0146"
#define s_preInitLogRetryFormat ""
#define s_logCommPipeCallback "\001\353&\260T\227\211b/\232Z\262\250\267\235R\276\303$j9,4CG3#\033\317\233k"
#define s_logCommPipeThreshold "\001=\034\237\243\226\211\310\230\330\274\270o\205\333\246]\255\234H\354\314"
#define s_logCommPipeCallbackFailed "\001\230-\331\011\315U\315\021\270\365b\014\025\361\330B\360b\023<d $dLX\334<"
#define s_logCommPipeSetValue "\001\352#\257\\\376\236I\334~\020a\216\234\013\237\037"
#define s_logCommPipeTicketStart "\001u\304\211\276\364\236\216Y\246\372\325~\340\325\001"
#define s_logCommPipeTicketMismatch "\001\014\211\367\373\010\014\243n\2174\351u\341a\211\302G&&\336\261o\333-\0040\"cd1I\021\305e4P\364t"
#define s_preInitLogBegin ""
#define s_preInitLogProcess ""
#define s_preInitCreateDirectoryFailed ""
#define s_preInitCreateEventsFailed ""
#define s_preInitSingleInstanceFailed ""
#define s_preInitGameMonWait ""
#define s_preInitGameMonStarted ""
#define s_preInitGameMonStartFailed ""
#define s_preInitGameMonPath ""
#define s_preInitGameMonPathAlt ""
#define s_preInitGameMonFile ""
#define s_preInitConfigFile ""
#define s_preInitConfigSection ""
#define s_preInitConfigUserKey ""
#define s_preInitConfigMainValue ""
#define s_preInitConfigDelayValue ""
#define s_preInitConfigLaunchValue ""
#define s_preInitConfigNpscValue ""
#define s_preInitConfigLimitValue ""
#define s_preInitConfigResultValue ""
#define s_preInitAuthFailed ""
#define s_preInitAuthFileOpenFailed ""
#define s_preInitAuthFileSize ""
#define s_preInitAuthFooter ""
#define s_preInitAuthRetry ""
#define s_preInitDecryptFailed ""
#define s_preInitUserMismatch ""
#define s_preInitConfigLoaded ""
#define s_preInitFileMissing ""
#define s_preInitLaunchPathBegin ""
#define s_preInitLaunchPathPrimaryMissing ""
#define s_preInitLaunchPathAlt ""
#define s_preInitLaunchPathReady ""
#define s_preInitSafeApiGetProcBytes ""
#define s_preInitSafeApiGetModuleBytes ""
#define s_preInitCommPipeFailed ""
#define s_preInitCommPipeStarted ""
#define s_preInitCommPipeRun "\0012\373(\336h"
#define s_gameMonCrcValueName "\0017\012Zz\277^O\316\252\351o\030B\316j\363"
#define s_preInitWaitFailed ""
#define s_preInitWaitExitCodeFailed ""
#define s_preInitWaitExitCode ""
#define s_preInitWaitReady ""
#define s_preInitWaitRetryEvent ""
#define s_preInitWaitRetryTimeout ""
#define s_preInitReadyEventPresent ""
#define s_preInitReadyEventMissing ""
#define s_preInitNpscFailed ""
#define s_preInitNpscProbe ""
#define s_preInitNpscProbe2 ""
#define s_preInitResultProbeFormat ""
#define s_preInitResultProbeLog ""
#define s_preInitResultProbeEncryptedLog ""
#define s_preInitReady ""
#define s_preInitEventName ""
#define s_preInitCommandLine ""
#define s_preInitCommandLineLog ""
#define s_preInitCommandLineLengthLog ""
#define s_preInitCommandLineEncryptedLog ""
#define s_preInitCommandLineHandoffFailed ""
#define s_preInitGameMonMessageFailed "\001\021\230\034k\237iM\201\266\245\274\177\072pl\306\047W\223\360\355\205u\361\351\250\364\337\021\320\054\047"
#define s_gameMonMessageMapping "\001\375\134Cg\365k\047i\075p\356\035\361\232\345\037\006"
#define s_gameMonMessageMappingAlt "\001\346\027y\340\077W\210\333\235\200"
#define s_gameMonMessageMutex "\001\256o\210\214\366h\054B\316C\333\217R\360\031\000\177"
#define s_gameMonMessageMutexAlt "\001\253fl7\323V\204\305\354\073"
#define s_preInitCreateProcessDetail ""
#define s_preInitCreateProcessFailedDetail ""
#define s_preInitCommPipePriority ""
#define s_preInitCommPipeHandleLog ""
#define s_preInitFinalSendLog ""
#define s_preInitFinalReadyWaitLog ""
#define s_preInitFinalStateLog ""
#define s_preInitLaunchDelayTooShort ""

static bool DisableCompatibleMode(int value);
typedef void* HINTERNET;
typedef HINTERNET (WINAPI *InternetOpenAProc)(LPCSTR agent, DWORD accessType,
                                              LPCSTR proxy,
                                              LPCSTR proxyBypass,
                                              DWORD flags);
typedef HINTERNET (WINAPI *InternetConnectAProc)(HINTERNET internet,
                                                 LPCSTR serverName,
                                                 WORD serverPort,
                                                 LPCSTR userName,
                                                 LPCSTR password,
                                                 DWORD service,
                                                 DWORD flags,
                                                 DWORD context);
typedef BOOL (WINAPI *FtpPutFileAProc)(HINTERNET connect,
                                       LPCSTR localFile,
                                       LPCSTR remoteFile,
                                       DWORD flags,
                                       DWORD context);
typedef BOOL (WINAPI *InternetCloseHandleProc)(HINTERNET handle);
struct tagPROCESSENTRY32;
struct tagMODULEENTRY32;
typedef HANDLE (WINAPI *CreateToolhelp32SnapshotProc)(DWORD flags, DWORD processId);
typedef BOOL (WINAPI *Process32FirstProc)(HANDLE snapshot, tagPROCESSENTRY32* entry);
typedef BOOL (WINAPI *Process32NextProc)(HANDLE snapshot, tagPROCESSENTRY32* entry);
typedef BOOL (WINAPI *Module32FirstProc)(HANDLE snapshot, tagMODULEENTRY32* entry);
typedef BOOL (WINAPI *Module32NextProc)(HANDLE snapshot, tagMODULEENTRY32* entry);

static int nSendErl = 0;
static HINSTANCE g_hInst = 0;
static unsigned int g_dwThreadId = 0;
static unsigned int g_dwWaitTime = 0;
static unsigned int g_dwNoUpdate = 0;
static unsigned int dwClientSequenceNumber = 0;
static unsigned int dwServerSequenceNumber = 0;
static unsigned int dwWorkingPID = 0;
static unsigned int bCheckGameMon = 0;
static unsigned int dwDE = 0;
static unsigned int dwRE = 0;
unsigned int dwGameMonPID = 0;
HANDLE hGameMonHandle = 0;
static bool bEncryptedClient = 0;
static bool bEncryptedClientArm = 0;
static unsigned char bIsWin2K = 0;
static CGameGuard* gg = 0;
static char* CONTAINER_NAME = "\x01\x19\xb0\x46\x7a\x25\xea\x6e\x48\x88\x63\x4d\xa3\x84\x85\x94\x41\x32\x3e";
static unsigned int nSendErlIni = 1;
static unsigned int g_dwLoseAllowFactor = 4;
static char szLogFile[MAX_PATH];
static char szGameMon[MAX_PATH];

CreateToolhelp32SnapshotProc pCreateToolhelp32Snapshot;
Process32FirstProc pProcess32First;
Process32NextProc pProcess32Next;
Module32FirstProc pModule32First;
Module32NextProc pModule32Next;


static inline CGameGuard* GameGuard() {
    return gg;
}

extern "C" unsigned int InitNPGameMon() {
    CGameGuard* gameGuard = gg;
    if (!gameGuard) {
        return 0;
    }
    return gameGuard->InitNPGameMonI();
}


extern "C" void SetHwndToGameMon(HWND hwnd) {
    CGameGuard* gameGuard = GameGuard();
    if (gameGuard) {
        gameGuard->SetHwndToGameMonI(hwnd);
    }
}


extern "C" void RunFAQWebPage(unsigned int errorCode) {
    CGameGuard* gameGuard = GameGuard();
    if (gameGuard) {
        gameGuard->RunFAQWebPageI(errorCode);
    }
}

extern "C" void SetCallbackToGameMon() {
}

extern "C" unsigned int PreInitNPGameMonA(const char* userId);


extern "C" unsigned int PreInitNPGameMonW(const wchar_t* userId) {
    char converted[32];
    WideCharToMultiByte(0, 0, userId, wcslen(userId) + 1,
                        converted, sizeof(converted), 0, 0);
    return PreInitNPGameMonA(converted);
}


extern "C" unsigned int PreInitNPGameMonA(const char* userId) {
    if (gg) {
        return 0;
    }

    LPTOP_LEVEL_EXCEPTION_FILTER (WINAPI *setUnhandledExceptionFilter)(
        LPTOP_LEVEL_EXCEPTION_FILTER) = SetUnhandledExceptionFilter;
    LPTOP_LEVEL_EXCEPTION_FILTER previousFilter =
        setUnhandledExceptionFilter(FvUnhandledExceptionFilter);

    gg = new CGameGuard;
    *(unsigned int*)((unsigned char*)gg + 0x10) =
        gg->PreInitNPGameMonI(userId);

    HANDLE readyEvent = *(HANDLE*)((unsigned char*)gg + 0x18);
    if (readyEvent) {
        SetEvent(readyEvent);
    }

    if (previousFilter) {
        setUnhandledExceptionFilter(previousFilter);
    }

    return *(unsigned int*)((unsigned char*)gg + 0x10);
}

extern "C" unsigned int CheckNPGameMon() {
    CGameGuard* gameGuard = GameGuard();
    if (!gameGuard) {
        return 0;
    }
    return gameGuard->CheckNPGameMonI();
}

extern "C" int SendUserIDToGameMonA(const char* userId);


extern "C" int SendUserIDToGameMonW(const wchar_t* userId) {
    if (!userId) {
        return 0;
    }

    char converted[64];
    WideCharToMultiByte(0, 0, userId, wcslen(userId),
                        converted, sizeof(converted), 0, 0);
    return SendUserIDToGameMonA(converted);
}


extern "C" int SendUserIDToGameMonA(const char* userId) {
    CGameGuard* gameGuard = gg;
    if (!gameGuard) {
        return 0;
    }
    if (!userId) {
        return (int)userId;
    }
    return gameGuard->SendUserIDToGameMonI(userId);
}


extern "C" int SendCSAuthToGameMon(unsigned int auth) {
    CGameGuard* gameGuard = gg;
    if (!gameGuard) {
        return 0;
    }
    return gameGuard->SendCSAuthToGameMonI(auth);
}


extern "C" int SendCSAuth2ToGameMon(_GG_AUTH_DATA* auth) {
    CGameGuard* gameGuard = gg;
    if (!gameGuard) {
        return 0;
    }
    return gameGuard->SendCSAuth2ToGameMonI(auth);
}

extern "C" const char* GetInfoFromGameMon() {
    CGameGuard* gameGuard = gg;
    if (!gameGuard) {
        return 0;
    }
    return gameGuard->GetInfoFromGameMonI();
}


extern "C" int CloseNPGameMon() {
    CGameGuard* gameGuard = gg;
    if (!gameGuard) {
        return 0;
    }

    unsigned char closed = gameGuard->CloseNPGameMonI();
    CGameGuard* deleting = gg;
    if (deleting) {
        deleting->CGameGuard::~CGameGuard();
        operator delete(deleting);
    }
    gg = 0;
    return closed;
}

CGameGuard::~CGameGuard() {
}

unsigned int CGameGuard::InitNPGameMonI() {
    if (!m_active) {
        m_log.Write(_Dect1(s_logInitGameMonInactive), m_initResult);
        return m_initResult;
    }

    m_log.Write(_Dect1(s_logInitGameMonActive));
    return m_initResult;
}

inline bool CLog::Open(const char* fileName) {
    unsigned char publicKey[0x54] = {
        0x06, 0x02, 0x00, 0x00, 0x00, 0xa4, 0x00, 0x00,
        0x52, 0x53, 0x41, 0x31, 0x00, 0x02, 0x00, 0x00,
        0x01, 0x00, 0x01, 0x00, 0x69, 0xb3, 0xdd, 0xe8,
        0x3d, 0x70, 0xa5, 0xfb, 0xd7, 0xb8, 0xfb, 0x60,
        0x68, 0x0f, 0x99, 0xc2, 0xb9, 0xe4, 0x20, 0x83,
        0xb5, 0xe6, 0x5d, 0xbf, 0x69, 0x62, 0x23, 0x29,
        0x6e, 0xe6, 0x68, 0x0b, 0xea, 0xea, 0x60, 0xde,
        0xba, 0x87, 0x7e, 0x39, 0x37, 0x26, 0x45, 0xd7,
        0x3b, 0x2a, 0xac, 0x79, 0xb0, 0x40, 0xb2, 0x26,
        0xcb, 0xdb, 0xb8, 0x4d, 0x55, 0x12, 0x17, 0x65,
        0x17, 0xc9, 0x85, 0xba
    };
    if (m_file != INVALID_HANDLE_VALUE)
        return false;
    m_xorSeed = 0;
    m_limit = 1000;

    char path[MAX_PATH];
    if (strlen(fileName) > 3 && fileName[1] != ':') {
        GetModuleFileNameA(0, path, sizeof(path));
        unsigned char* slash = _mbsrchr((unsigned char*)path, '\\');
        strcpy(slash ? (char*)slash + 1 : path, fileName);
    } else {
        strcpy(path, fileName);
    }
    m_file = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, 0,
                         CREATE_ALWAYS, 0x80000080, 0);
    if (m_file == INVALID_HANDLE_VALUE)
        return false;

    GetLocalTime(&m_time);
    sprintf(m_line, _Dect("\001\257r\306\333\326\307\032\223~?\002\313&\252\233\321\024\262\274r*h\363\253\307\002\376b[\217\007\352\267\235j\026\231^\310\3133p\233\026\376\372\245\232\011\367\237to`b>Q\006es\013\331\223>\256\3172[VG\232\023\376\277\262|"),
            m_time.wYear, m_time.wMonth, m_time.wDay, m_time.wHour,
            m_time.wMinute, m_time.wSecond, m_time.wMilliseconds);
    int length = strlen(m_line);
    if (m_limit == 999)
        length = 0;

    DWORD written;
    if (m_xorSeed) {
        for (int i = 0; i < length; ++i) {
            m_xorSeed += 2;
            m_line[i] ^= (char)(m_xorSeed + 0x43);
        }
    } else {
        unsigned int mix = (m_time.wSecond + m_time.wMilliseconds + GetTickCount() + m_time.wMinute) *
                            (m_time.wDay * m_time.wMonth * m_time.wYear * m_time.wMilliseconds + 1);
        mix = (mix & 0xffff) | 0xdc000000;
        m_pad = (((mix % 10) ^ 0x55) << 16) | mix;

        unsigned char randomKey[0x10];
        srand(GetTickCount());
        for (int i = 0; i < 0x10; ++i)
            randomKey[i] = (unsigned char)(rand() % 256);
        RC4_set_key(&m_rc4, 0x10, randomKey);

        HCRYPTPROV provider = 0;
        HCRYPTKEY key = 0;
        if (CryptAcquireContextA(&provider, 0, MS_DEF_PROV_A, PROV_RSA_FULL,
                                 CRYPT_VERIFYCONTEXT)) {
            if (CryptImportKey(provider, publicKey, sizeof(publicKey), 0, 0, &key)) {
                DWORD dataLength = 0;
                CryptEncrypt(key, 0, TRUE, 0, 0, &dataLength, 0);
                DWORD size = dataLength;
                if (size) {
                    unsigned char* encrypted = new unsigned char[size];
                    dataLength = 0x10;
                    memcpy(encrypted, randomKey, 0x10);
                    if (CryptEncrypt(key, 0, TRUE, 0, encrypted, &dataLength, size)) {
                        DWORD marker = 0xccdc00;
                        WriteFile(m_file, &marker, sizeof(marker), &dataLength, 0);
                        WriteFile(m_file, encrypted, size, &dataLength, 0);
                    }
                    delete encrypted;
                } else {
                    DWORD marker = 0xccdd00;
                    unsigned char maskedKey[0x10];
                    WriteFile(m_file, &marker, sizeof(marker), &dataLength, 0);
                    for (int i = 0; i < 0x10; ++i)
                        maskedKey[i] = publicKey[0x14 + i] ^ randomKey[i];
                    WriteFile(m_file, maskedKey, sizeof(maskedKey), &dataLength, 0);
                }
                CryptDestroyKey(key);
            }
            CryptReleaseContext(provider, 0);
        }
        RC4(&m_rc4, length, (unsigned char*)m_line, (unsigned char*)m_line);
    }
    WriteFile(m_file, m_line, length, &written, 0);
    return true;
}

unsigned int CGameGuard::PreInitNPGameMonI(const char* userId) {
    char* encrypted;
    char* cursor;
    int length;
    if (m_active)
        return 0x755;

    m_isWinNt = IsWinNT();
    m_processId = GetCurrentProcessId();
    g_dwThreadId = GetCurrentThreadId();
    _mbsnbcpy((unsigned char*)m_clientId, (const unsigned char*)userId, 0x20);
    m_clientId[31] = 0;

    GetGameGuardDirectory(szLogFile, MAX_PATH);
    if (_access(szLogFile, 0) == -1 && !CreateDirectoryA(szLogFile, 0)) {
        GetLastError();
        m_log.Write(_Dect1("\001\337\002(\265i\357\266\312\026\326\206t.y\363\241\321[\277p\032\303[\376\366\326"), GetLastError());
        return 0x6f;
    }
    lstrcatA(szLogFile, "\\");
    lstrcatA(szLogFile, _Dect1("\001\212\003J\221F\274\246\271\233A\276"));

    if (!m_log.Open(szLogFile)) {
        char* dot = (char*)_mbsrchr((unsigned char*)szLogFile, '.');
        if (dot) {
            for (int i = 1; i < 10; ++i) {
                sprintf(dot, _Dect1("\0019\020oQ\361\326D\356a"), i);
                if (m_log.Open(szLogFile))
                    break;
            }
        }
    }

    m_log.Write(_Dect("\001NO\017\206\027\312\316M\022\270W\266\257\333\227Q\374\303/iz\003KC\024:$H\203\230p\263\327kFQZ\202F\260\242\222[\266"), 0x2f, "Tue Apr 17 14:15:17 2007");
    m_log.Write(_Dect1("\001\320\325\2021.(i\367m1F\001k\334\035\354\231\3326{\334I\267>\305H\315a\205\331*IQ\371\215\015\274J\0014!K)|\011G\220lgY\270@X\211r\003\304\300\330\311T"),
                m_processId, userId, InitNPGameMon);

    GetProcessList();
    GetModuleList();
    if (m_disableAppCompat)
        DisableCompatibleMode(1);

    if (!CheckSingleInstance(0x251c))
        m_log.Write(_Dect1("\001\337\002)\261z\347\262\362\032\360\317g'h\342\257\307[\277S\003\303D\252\362"));

    if (!CheckGameMonSingleInstance(0)) {
        ++dwDE;
        HANDLE event = OpenEventA(SYNCHRONIZE, FALSE, _Dect1("\001\2349\352\242\204\236n<<u\361;@\257\214\212\355"));
        if (event) {
            if (WaitForSingleObject(event, 0) != WAIT_TIMEOUT) {
                if (!CheckGameMonSingleInstance(0x251c))
                    return 0x6e;
                dwDE = 0;
            }
            CloseHandle(event);
        }
    }

    unsigned int launchCookie = GetTickCount() & 0xfff00000;
    SECURITY_ATTRIBUTES security;
    security.nLength = sizeof(security);
    security.lpSecurityDescriptor = 0;
    security.bInheritHandle = TRUE;
    if (!(m_gameMonReadyEvent = CreateEventA(&security, FALSE, FALSE, 0))) {
        GetLastError();
        m_log.Write(_Dect1("\001]|\263\003\213A\304\\\324lt\376\200\200f\015\302<\350\226\274\252\361\350\367\215\224\\\345\264\320t\364"), GetLastError());
        return 0xab;
    }
    if (!(m_readyEvent = CreateEventA(&security, TRUE, FALSE, 0))) {
        GetLastError();
        m_log.Write(_Dect1("\001\304\261S^\342\214\225a\035\201{\352\206\260\252\354\273\371\314\230F\254\372\2018\347\335\004\354\225D@\004\004"), GetLastError());
        return 0xab;
    }
    if (!(m_gameMonMutex = CreateEventA(&security, FALSE, FALSE, 0))) {
        GetLastError();
        m_log.Write(_Dect1("\001\272\223\354L\024\016\033\323K\343\353\321\037\357l>P\323w\031\343\245\356'(B\313s\272\333\017\333+"), GetLastError());
        return 0xab;
    }
    m_log.Write(_Dect1("\001\300\245*\326}"));

    STARTUPINFOA startup;
    PROCESS_INFORMATION process;
    char commandLine[0x100];
    char encryptedCommandLine[0x400];
    char updatePath[MAX_PATH];
    {
        GetGameDirectory(updatePath, MAX_PATH);
        strcat(updatePath, "\\");
        strcat(updatePath, _Dect1("\0014\0011\252aT\201\242\235\220~9~M\321&"));
        if (_access(updatePath, 0) == -1) {
            m_log.Write(_Dect1("\001\021\230\015R\202t\031\202\212\235\237h}uZ"), updatePath);
            return 0x99;
        }

        bool retryFailed = false;
        if (!m_authClient.Auth(updatePath, true)) {
            bool badSignature = false;
            if (m_authClient.GetError() == 8 && GetLastError() == 0x80090006)
                badSignature = true;
            GetLastError();
            m_log.Write(_Dect1("\001\2349\362\244\235\205d}6H\3359\030\304\260\270\2149\241\251\375\375\214\030\203"), m_authClient.GetError(), GetLastError(), updatePath);

            HANDLE file = CreateFileA(updatePath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
            if (file == INVALID_HANDLE_VALUE) {
                m_log.Write(_Dect1("\001\017\222\377i;\177\351\356\305C\366z[\217S\362\363\227<"), GetLastError(), updatePath);
            } else {
                m_log.Write(_Dect1("\001\"\313\223t'Y\307%\266\336F\302"), GetFileSize(file, 0));
                SetFilePointer(file, -4, 0, FILE_END);
                DWORD footer = 0;
                DWORD read;
                ReadFile(file, &footer, sizeof(footer), &read, 0);
                m_log.Write(_Dect1("\001\347\032\214\276\314Q\353p\022\322\015\376\366\312"), footer);
                CloseHandle(file);
            }

            if (badSignature) {
                if (m_isWinNt || !CheckRsaBase())
                    return 0x9b;
                if (!m_authClient.Auth(updatePath, true)) {
                    if (m_authClient.GetError() == 8 && GetLastError() == 0x80090006)
                        return 0x9b;
                    retryFailed = true;
                }
                m_log.Write(_Dect1("\001\356/\324\371\251\246\354\202\221\272\370\337\006\033\276u\033\367\203\217\333"));
                if (!retryFailed)
                    goto authenticated;
            }
            m_log.Write(_Dect1("\001w\312\211\271R\277\312\001~;5jC#*\"S\312\221\177\372\236!F\016\002\316Z\343\354\320\001\265"));
            return 0x7c;
        }
authenticated:
        m_log.Write(_Dect1("\001\333\366\031\355\345"));

        memset(&startup, 0, sizeof(startup));
        startup.cb = sizeof(startup);
        memset(&process, 0, sizeof(process));

        wsprintfA(commandLine, _Dect1("\0018\015syai=1I\016\220llU\275y\034\310x\031\301\235"),
                  launchCookie, m_clientId, m_gameMonReadyEvent, m_readyEvent,
                  m_processId, dwDE);
        m_log.Write("cmd: %s", commandLine);
        encrypted = _Enct(commandLine);
        cursor = encryptedCommandLine;
        length = strlen(commandLine);
        m_log.Write("len: %d", length);
        for (int i = 0; i < length + 4; ++i)
            cursor += wsprintfA(cursor, "\\x%02x", (unsigned char)*encrypted++);
        *cursor = 0;
        m_log.Write("enc: %s", encryptedCommandLine);

        if (!CreateProcessA(updatePath, encryptedCommandLine, 0, 0, TRUE, 0, 0, 0, &startup, &process)) {
            GetLastError();
            m_log.Write(_Dect1("\001\227*\225\035!W\256\322N\352\025\001\340\247\014\305{,v\027\337r\012\265\247\372\342\352\317\302\274\273\337PW\273`"), updatePath, GetLastError(), commandLine);
            return 0xaa;
        }
        m_log.Write(_Dect1("\001Udg\017B"));
    }

    DWORD exitCode = 0;
    HANDLE updateHandles[2];
    updateHandles[0] = m_gameMonReadyEvent;
    updateHandles[1] = process.hProcess;
    if (WaitForMultipleObjects(2, updateHandles, FALSE, INFINITE) != WAIT_OBJECT_0) {
        if (!GetExitCodeProcess(process.hProcess, &exitCode))
            m_log.Write(_Dect1("\001R[TP\033\307\027W\257\377\231\250\352\206\262\255\271\330\017\004\3553\024\356\217\207\237C\200c'S\203"), GetLastError());
        m_log.Write(_Dect1("\001\345\024j\337`Q\210GL\205\227\214\321e39]\307 [\204\252\250\316m\244\243\351\270\300\035\325e!"), exitCode);
        CloseHandle(process.hProcess);
        CloseHandle(process.hThread);
        return exitCode;
    }
    m_log.Write(_Dect1("\001\367J\025\031\207"));
    unsigned int memoryCrc = ReadMemCrc(process);
    CloseHandle(process.hProcess);
    CloseHandle(process.hThread);

    char configPath[MAX_PATH];
    GetGameGuardDirectory(configPath, MAX_PATH);
    strcat(configPath, "\\");
    strcat(configPath, m_clientId);
    strcat(configPath, _Dect1("\001\345\024q\266hR\204"));
    if (!m_authClient.Auth(configPath, true)) {
        GetLastError();
        m_log.Write(_Dect1("\001\255ln\331'}U\331tH\205 _\205\214\204\321)9p\014\320"), m_authClient.GetError(), GetLastError());
        return 0x78;
    }

    char configData[0x800];
    if (!m_authClient.DecryptMem(configPath, configData, 0x7ff, _Dect1(CONTAINER_NAME) + 5)) {
        GetLastError();
        m_log.Write(_Dect1("\001b\213\302\256\340\212\302\233\263\330\030\016\356gR\351\207\202\226\007\213/w\037\323w\377"), m_authClient.GetError(), GetLastError());
        return 0x82;
    }

    char* section = _Dect("\001Vgi\304\2032\363\026\005Y");
    char configUser[0x20];
    MemGetPrivateProfileString(section, _Dect1("\001~\337\337\374+:\333L<\316\253\256"), _Dect("\001JC\005j\270P\304\230\351="), configUser, sizeof(configUser), configData);
    if (_mbsicmp((const unsigned char*)configUser, (const unsigned char*)m_clientId)) {
        m_log.Write(_Dect1("\001\277\242\015\321\232G\322\0202_\252\206E\325\023N\356\243\013\3253"));
        return 0x8e;
    }
    nSendErlIni = MemGetPrivateProfileInt(section, _Dect1("\001:\023u\334\243\245\276b\374\017"), 1, configData);
    g_dwWaitTime = MemGetPrivateProfileInt(section, _Dect1("\001\334\371,\362i\370\030I\331\244\261"), 0x186a0, configData);
    g_dwNoUpdate = MemGetPrivateProfileInt(section, _Dect1("\001KF\002\024\010Q62\033\027o\257"), 0, configData);
    unsigned int gameMonVersion = MemGetPrivateProfileInt(section, _Dect1("\001JC\012m\263M\303\204\367<\334"), 0, configData);
    unsigned int gameMonOption = MemGetPrivateProfileInt(section, _Dect1("\001\250]Wn\365\030}\242\216\216"), 0, configData);
    m_useNpsc = !((MemGetPrivateProfileInt(section, _Dect1("\001\244Q \362 \335\235\372\026\036\252l\214,\341"), 0, configData) >> 1) & 1);
    m_npscInterval = MemGetPrivateProfileInt(section, _Dect1("\001h\235\003:$\320\275e\337E%\032\017Z\001_88\342\037U9"), 1000, configData);
    m_npscOptions = MemGetPrivateProfileInt(section, _Dect1("\001\317\322\240\025[\037\002J *\032\025p\265\244R\334\245"), 0, configData);
    memset(configData, 0, sizeof(configData));
    m_log.Write(_Dect1("\001\220\025z\306)"));

    GetGameGuardDirectory(szGameMon, MAX_PATH);
    strcat(szGameMon, "\\");
    strcat(szGameMon, _Dect1("\001\343\016h%>;^\247\230p\275\226j\025"));
    if (gg) {
        unsigned char* code = (unsigned char*)GetProcAddress;
        gg->m_log.Write(_Dect1("\001TaA\012p\330>e-a\036\305P\254\344\207 a\331\035\362\001\204\000\230\003\264\2755YF\355"), code[0], code[1], code[2], code[3], code[4]);
    }
    if (gg) {
        unsigned char* code = (unsigned char*)GetModuleHandleA;
        gg->m_log.Write(_Dect1("\001\256o\233\214\327Ot\003\207/\244\203\012\222naJ\357\203\033\210\037N\246\362M\316{o'L\313"), code[0], code[1], code[2], code[3], code[4]);
    }
    m_log.Write(_Dect1("\001<\031\206\254\271"));
    if (_access(szGameMon, 0) == -1) {
        GetGameGuardDirectory(szGameMon, MAX_PATH);
        strcat(szGameMon, "\\");
        strcat(szGameMon, _Dect1("\001%\324\276\037 \221H\215\026\312\013\315I\251"));
        if (_access(szGameMon, 0) == -1) {
            m_log.Write(_Dect1("\001%\324\240\037 \221H\215\026\312\005\306^\270\275v\006\001\373\234\015\274(\023"), szGameMon);
            return 0x9a;
        }
    }
    if (!m_authClient.Auth(szGameMon, true)) {
        GetLastError();
        m_log.Write(_Dect1("\001\2349\362\244\235\205d}6H\3359\030\304\260\270\2149\241\251\375\375\214\030\203"), m_authClient.GetError(), GetLastError(), szGameMon);
        return 0x7a;
    }
    m_log.Write(_Dect1("\001\003n\201\253M"));

    if (!m_pipe.InitCommPipe()) {
        m_log.Write(_Dect1("\001\362;\344\276p\372\206/%\004\027\312\016\245\343\326\032"));
        return 0xa0;
    }
    SetThreadPriority(m_pipe.m_thread, THREAD_PRIORITY_HIGHEST);
    m_log.Write(_Dect1("\001Zs\220\2065"));

    launchCookie = GetTickCount() & 0xfff00000;
    if (gameMonVersion < 0x2f) {
        m_log.Write(_Dect1("\001.\357\034\031\177\361\247\320K\360xW\212^\260\272\210o\003\317\026\307\202j \305\026\376\256"), gameMonVersion, 0x2f);
        return 0x78;
    }
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    memset(&process, 0, sizeof(process));
    ResetEvent(m_gameMonReadyEvent);

    wsprintfA(commandLine, _Dect1("\001\210\375\020\351\261\231m\241\231\001\365\231A\011\035Q\251\261\245\211\021\271\315Ay>\000\274|e-)\014\230\010"),
              launchCookie, m_processId, m_isWinNt, m_pipe.GetChildReadHandle(),
              m_pipe.GetChildWriteHandle(), m_clientId, m_gameMonMutex,
              m_gameMonReadyEvent, gameMonOption);
    encrypted = _Enct(commandLine);
    cursor = encryptedCommandLine;
    length = strlen(commandLine);
    for (int i = 0; i < length + 4; ++i)
        cursor += wsprintfA(cursor, "\\x%02x", (unsigned char)*encrypted++);
    *cursor = 0;
    m_log.Write(_Dect1("\001~\337\324\322^"));

    if (dwDE && !CheckGameMonSingleInstance(0)) {
        HANDLE event = OpenEventA(SYNCHRONIZE, FALSE, _Dect1("\001\2349\352\242\204\236n<<u\361;@\257\214\212\355"));
        if (event) {
            if (WaitForSingleObject(event, 0) != WAIT_TIMEOUT) {
                if (!CheckGameMonSingleInstance(0x251c))
                    return 0x6e;
                dwDE = 0;
            }
            CloseHandle(event);
        }
    }
    if (dwDE) {

        struct Header { unsigned int size; unsigned int command; unsigned int processId; };
        struct {
            Header header;
            char text[0xfc];
        } message;
        unsigned int size = strlen(encryptedCommandLine) + 0x0d;
        Header header = { size, 0x7eb, m_processId };
        message.header = header;
        strcpy(message.text, encryptedCommandLine);
        if (!SendGameMonMessage(&message, size)) {
            GetLastError();
            m_log.Write(_Dect1(s_preInitGameMonMessageFailed), GetLastError(), szGameMon, commandLine);
            return 0xaa;
        }
    } else if (!dwRE) {
        if (!CreateProcessA(szGameMon, encryptedCommandLine, 0, 0, FALSE, CREATE_SUSPENDED, 0, 0, &startup, &process)) {
            GetLastError();
            m_log.Write(_Dect1("\001\227*\225\035!W\256\322N\352\025\001\340\247\014\305{,v\027\337r\012\265\247\372\342\352\317\302\274\273\337PW\273`"), szGameMon, GetLastError(), commandLine);
            return 0xaa;
        }
        m_log.Write(_Dect1("\001\011\200\277\015PHT\340\250"), process.dwProcessId);
    }

    if (!(bCheckGameMon = g_dwNoUpdate))
        bCheckGameMon = CheckGameMonCrc(szGameMon, s_gameMonCrcValueName, memoryCrc);
    m_log.Write(_Dect1(s_preInitCommPipeRun));

    unsigned int threadId = m_pipe.RunCommPipe();
    if (!threadId) {
        GetLastError();
        m_log.Write(_Dect1("\001\263~\274\000Z\250\310\265\252\343\303\204\276\277\327JR\372"), GetLastError());
        return 0xa0;
    }
    m_log.Write(_Dect1("\001|\331\326\306\247\374\201\251X\273\361\224|\241\365\011\272\231A\011\035\001\257r\235\245\204\227J\372+pZ\227\177\252\236\332I\262\312j>"), threadId);

    if (!dwDE) {
        ResumeThread(process.hThread);
        HANDLE handles[2];
        handles[0] = m_gameMonMutex;
        handles[1] = process.hProcess;
        if (WaitForMultipleObjects(2, handles, FALSE, g_dwWaitTime) != WAIT_OBJECT_0) {
            HANDLE event = OpenEventA(SYNCHRONIZE, FALSE, _Dect1("\001\016\217\350\254\226H\314\"nc\263u\362\220\363 \327"));
            if (event) {
                CloseHandle(event);
                if (WaitForSingleObject(m_gameMonMutex, g_dwWaitTime) != WAIT_OBJECT_0) {
                    m_log.Write(_Dect1("\001\366G\030\012\0146\"\033\215\232m\366\322a\011\037Z\336K\267\256\237S\367\177"), g_dwWaitTime);
                    return 0x72;
                }
            } else {
                m_log.Write(_Dect1("\001\353&\272S\211\207wb\221Y\257\352\356\233G\376\217cg/"), g_dwWaitTime);
                return 0x72;
            }
        }
    } else if (WaitForSingleObject(m_gameMonMutex, g_dwWaitTime) != WAIT_OBJECT_0) {
        m_log.Write(_Dect1("\001\353&\272S\211\207wb\221Y\257\352\356\233G\376\217cg/"), g_dwWaitTime);
        return 0x72;
    }
    m_log.Write(_Dect1("\001\303\256KkQ\237o\252\261\327\035"));

    if (WaitForSingleObject(m_gameMonReadyEvent, 0) != WAIT_OBJECT_0) {
        m_log.Write(_Dect1("\001H=\341N5X\275\214\023\303`\260J\314\\\221*|\371\205"));
        return 0x72;
    }
    HANDLE readyEvent = OpenEventA(SYNCHRONIZE, FALSE, _Dect1("\001\016\217\350\254\226H\314\"nc\263u\362\220\363 \327"));
    if (readyEvent) {
        CloseHandle(readyEvent);
        m_log.Write(_Dect1("\001\271\220\355\223x\013\304"));
    } else {
        dwGameMonPID = process.dwProcessId;
        hGameMonHandle = process.hProcess;
    }
    m_log.Write(_Dect1("\001KF\0113p"));
    if (dwDE) {
        m_log.Write(_Dect1("\001\327\352\360Z\326\322b\011\017\037"));
        dwGameMonPID = 0;
        hGameMonHandle = 0;
    }
    if (!bCheckGameMon)
        return 0x7a;

    CheckEncryptedClient();
    unsigned int timeAddr = InitTimeAddr();
    if (!timeAddr)
        CommPipeCallback(&m_pipe, 0x62d, 0x0b);
    m_log.Write(_Dect1("\001\364A\354d\256\020\320q\301\\\251\334t\015\006\257\330\004\370"), timeAddr);
    unsigned int socketAddr = InitSocketAddr();
    if (!socketAddr)
        m_pipe.SendCommPipe(0x8b1, 0x630, 0x0d);
    m_log.Write(_Dect1("\001%\324\247\021/\225Y\223\026\307N\315E\215\371t\033N\265\335E"), socketAddr);

    if (m_useNpsc && !InitNPSC()) {
        m_log.Write(_Dect1("\001\322\333\336\304\216V\267\313\005c?$eC+1?O\312\221\177\372\236"));
        return 0xdc;
    }
    m_log.Write(_Dect1("\001q\270c5u"));
    m_pipe.SendCommPipe(0x8b1, 0x611, m_processId);
    m_pipe.SendCommPipe(0x8b1, 0x617, memoryCrc);
    m_log.Write(_Dect1("\001\251`Ay8hA\036\030\371'}\001\326Q{\214m\\\251\212\206\321h2>L"));
    m_active = 1;
    return 0x755;
}

inline CCriticalSector::~CCriticalSector() {
    DeleteCriticalSection((CRITICAL_SECTION*)this);
}

inline void CLog::Close() {
    if (m_file != INVALID_HANDLE_VALUE) {
        GetLocalTime(&m_time);

        const char* suffix = (m_limit && m_count > m_limit) ? " +" : "";

        sprintf(m_line, _Dect(s_logCloseFormat),
                m_time.wYear, m_time.wMonth, m_time.wDay, m_time.wHour,
                m_time.wMinute, m_time.wSecond, m_time.wMilliseconds, suffix);

        int length = strlen(m_line);
        if (m_xorSeed) {
            if (length > 0) {
                for (int i = 0; i < length; ++i) {
                    m_xorSeed += 2;
                    m_line[i] ^= (unsigned char)(m_xorSeed + 0x43);
                }
            }
        } else {
            RC4(&m_rc4, length, (unsigned char*)m_line, (unsigned char*)m_line);
        }

        DWORD written;
        if (m_limit != 999) {
            WriteFile(m_file, m_line, length, &written, 0);
        }
        SetEndOfFile(m_file);
        CloseHandle(m_file);
        m_file = INVALID_HANDLE_VALUE;
    }
}

inline CLog::~CLog() {
    if (m_file != INVALID_HANDLE_VALUE) {
        GetLocalTime(&m_time);

        const char* suffix = (m_limit && m_count > m_limit) ? " +" : "";

        sprintf(m_line, _Dect(s_logCloseFormat),
                m_time.wYear, m_time.wMonth, m_time.wDay, m_time.wHour,
                m_time.wMinute, m_time.wSecond, m_time.wMilliseconds, suffix);

        int length = strlen(m_line);
        if (m_xorSeed) {
            if (length > 0) {
                for (int i = 0; i < length; ++i) {
                    m_xorSeed += 2;
                    m_line[i] ^= (unsigned char)(m_xorSeed + 0x43);
                }
            }
        } else {
            RC4(&m_rc4, length, (unsigned char*)m_line, (unsigned char*)m_line);
        }

        DWORD written;
        if (m_limit != 999) {
            WriteFile(m_file, m_line, length, &written, 0);
        }
        SetEndOfFile(m_file);
        CloseHandle(m_file);
        m_file = INVALID_HANDLE_VALUE;
    }
}

bool CLog::Write(const char* fmt, ...) {
    if (m_file == INVALID_HANDLE_VALUE) {
        return false;
    }

    if (m_limit) {
        if (*fmt != '!') {
            if (m_count > m_limit) {
                return false;
            }
            ++m_count;
        }
    }

    CRITICAL_SECTION* section = (CRITICAL_SECTION*)&m_section;
    EnterCriticalSection(section);

    char text[0x1000];
    va_list args;
    va_start(args, fmt);
    vsprintf(text, fmt, args);
    va_end(args);

    GetLocalTime(&m_time);

    if (*(unsigned char*)sprintf == 0xcc ||
        *(unsigned char*)WriteFile == 0xcc) {
        LeaveCriticalSection(section);
        return false;
    }

    sprintf(m_line, _Dect(s_logWriteFormat),
            m_time.wHour, m_time.wMinute, m_time.wSecond,
            m_time.wMilliseconds, text);

    int length = strlen(m_line);
    if (m_xorSeed) {
        if (length > 0) {
            for (int i = 0; i < length; ++i) {
                m_xorSeed += 2;
                m_line[i] ^= (unsigned char)(m_xorSeed + 0x43);
            }
        }
    } else {
        RC4(&m_rc4, length, (unsigned char*)m_line, (unsigned char*)m_line);
    }

    DWORD written;
    WriteFile(m_file, m_line, length, &written, 0);
    LeaveCriticalSection(section);
    return true;
}

void CGameGuard::SetHwndToGameMonI(HWND hwnd) {
    unsigned char* raw = (unsigned char*)this;
    register CLog* log = (CLog*)(raw + 0x20);
    log->Write(_Dect1(s_logSetHwndBegin));

    GetProcessList();
    GetModuleList();

    *(HWND*)(raw + 4) = hwnd;
    ((CCommPipe*)(raw + 0x1464))->SendCommPipe(0x8b1, 0x612,
                                               (unsigned int)hwnd);

    if (*(unsigned int*)(raw + 8)) {
        log->Write(_Dect1(s_logSetHwndCallback),
                   *(unsigned int*)(raw + 8),
                   *(unsigned int*)(raw + 0x0c));
        if (!NPGameMonCallback(*(unsigned int*)(raw + 8),
                               *(unsigned int*)(raw + 0x0c))) {
            log->Write(_Dect1(s_logSetHwndCallbackFailed));
        }
    }

    log->Write(_Dect1(s_logSetHwndEnd));
}

void CGameGuard::RunFAQWebPageI(unsigned int errorCode) {
    char page[0x10] = {0};
    char currentDir[MAX_PATH] = {0};

    GetCurrentDirectoryA(sizeof(currentDir), currentDir);
    strcat(currentDir, _Dect1(s_runFaqLocalSuffix));
    if (_access(currentDir, 0) != -1) {
        if (errorCode == 0x3e9)
            strcpy(page, _Dect1(s_runFaqLocalPageAuth));
        else if (errorCode == 0x3ea)
            strcpy(page, _Dect1(s_runFaqLocalPageUpdate));
        else if (errorCode == 0x72)
            strcpy(page, _Dect1(s_runFaqLocalPageInit));

        char command[MAX_PATH] = {0};
        sprintf(command, _Dect1(s_runFaqLocalFormat), currentDir, page, m_clientId);

        STARTUPINFOA startup;
        PROCESS_INFORMATION process;
        memset(&startup, 0, sizeof(startup));
        memset(&process, 0, sizeof(process));
        startup.cb = sizeof(startup);
        if (!CreateProcessA(0, command, 0, 0, FALSE, 0, 0, 0, &startup, &process)) {
            m_log.Write(_Dect1(s_runFaqLocalCreateFailed), GetLastError());
        } else {
            m_log.Write(_Dect1(s_runFaqLocalCreateOk), command);
            WaitForSingleObject(process.hProcess, INFINITE);
        }
    }

    memset(page, 0, sizeof(page));
    if (errorCode == 0x73)
        strcpy(page, _Dect1(s_runFaqWebPageDefault));
    else if (errorCode == 0x99 || errorCode == 0x7c)
        strcpy(page, _Dect1(s_runFaqWebPageAuth));
    else if (errorCode == 0x72 || errorCode == 0x64)
        strcpy(page, _Dect1(s_runFaqWebPageGameMon));
    else if (errorCode == 0x4e21)
        strcpy(page, _Dect1(s_runFaqWebPageHack));
    else if (errorCode == 0x4e22)
        strcpy(page, _Dect1(s_runFaqWebPageSpeedHack));
    else if (errorCode == 0x4e23)
        strcpy(page, _Dect1(s_runFaqWebPageVirus));
    else if (errorCode == 0x4e24)
        strcpy(page, _Dect1(s_runFaqWebPageCollision));
    else if (errorCode == 0x4e25)
        strcpy(page, _Dect1(s_runFaqWebPageExecute));
    else if (errorCode == 0x4e26)
        strcpy(page, _Dect1(s_runFaqWebPageInit));
    else if (errorCode == 0x4e27)
        strcpy(page, _Dect1(s_runFaqWebPageUnknown));
    else
        return;

    STARTUPINFOA startup;
    PROCESS_INFORMATION process;
    memset(&startup, 0, sizeof(startup));
    memset(&process, 0, sizeof(process));
    startup.cb = sizeof(startup);
    HKEY key = 0;
    char browser[0x208] = {0};
    DWORD size = MAX_PATH;
    RegOpenKeyExA(HKEY_LOCAL_MACHINE, _Dect1(s_runFaqRegistryKey), 0, KEY_QUERY_VALUE, &key);
    if (key && RegQueryValueExA(key, 0, 0, 0, (LPBYTE)browser, &size) == ERROR_SUCCESS) {
        char command[MAX_PATH] = {0};
        sprintf(command, _Dect1(s_runFaqRegistryFormat), page);
        strcat(browser, command);
        if (!CreateProcessA(0, browser, 0, 0, FALSE, 0, 0, 0, &startup, &process))
            m_log.Write(_Dect1(s_runFaqRegistryCreateFailed), GetLastError());
        else
            m_log.Write(_Dect1(s_runFaqRegistryCreateOk), browser);
    }
    RegCloseKey(key);
}

bool CGameGuard::IsWinNT() {
    OSVERSIONINFOA version;
    version.dwOSVersionInfoSize = sizeof(version);
    GetVersionExA(&version);

    HMODULE kernel = GetModuleHandleA(_Dect1(s_kernel32Dll));
    FARPROC first = GetProcAddress(kernel, _Dect1(s_process32First));
    if (first) {
        FARPROC next = GetProcAddress(kernel, _Dect1(s_process32Next));
        if (next) {
            if (version.dwMajorVersion < 5 ||
                (version.dwMajorVersion == 5 && version.dwMinorVersion < 1)) {
                *(unsigned char*)((unsigned char*)this + 0x1684) = 1;
                return true;
            }
        }
    }

    if (version.dwPlatformId == VER_PLATFORM_WIN32_NT) {
        bIsWin2K = 1;
        if (version.dwMajorVersion == 5 && version.dwMinorVersion == 0) {
            g_dwLoseAllowFactor *= 2;
        }
        return true;
    }

    return false;
}

bool CGameGuard::GetGameGuardDirectory(char* path, unsigned int size) {
    GetGameDirectory(path, size);
    strcat(path, _Dect1(s_gameGuardDirectorySuffix));
    return true;
}

bool CGameGuard::GetGameDirectory(char* path, unsigned int size) {
    if (!GetModuleFileNameA(0, path, size)) {
        strcpy(path, ".");
        return false;
    }

    char* slash = (char*)_mbsrchr((unsigned char*)path, '\\');
    if (slash) {
        *slash = 0;
        return true;
    }

    strcpy(path, ".");
    return true;
}

unsigned int CGameGuard::CheckNPGameMonI() {
    static unsigned int dwCount = 0;
    ++dwCount;

    if (m_closing) {
        return 0x755;
    }

    if (!m_active) {
        if (dwCount < 3) {
            return 0x755;
        } else {
            m_log.Write(_Dect1(s_logGameMonInactiveTooLong));
            return 0x262;
        }
    }

    if (!bCheckGameMon) {
        m_log.Write(_Dect1(s_logGameMonNotRunning));
        return 0x28a;
    }

    if (hGameMonHandle) {
        DWORD exitCode = 0;
        if (GetExitCodeProcess(hGameMonHandle, &exitCode) &&
            exitCode != STILL_ACTIVE) {
            m_log.Write(_Dect1(s_logGameMonExited));
            return 0x26c;
        }
    }

    HANDLE (WINAPI *openMutex)(DWORD, BOOL, LPCSTR) = OpenMutexA;
    HANDLE mutex = openMutex(SYNCHRONIZE, FALSE,
                             _Dect1(s_gameMonCheckMutex));
    if (!mutex) {
        mutex = openMutex(SYNCHRONIZE, FALSE,
                          _Dect1(s_gameMonCheckMutexAlt));
    }

    if (mutex) {
        BOOL (WINAPI *closeHandle)(HANDLE) = CloseHandle;
        closeHandle(mutex);

        mutex = openMutex(SYNCHRONIZE, FALSE,
                          _Dect1(s_gameMonSingleInstanceMutex));
        if (!mutex) {
            mutex = openMutex(SYNCHRONIZE, FALSE,
                              _Dect1(s_gameMonSingleInstanceMutexAlt));
        }

        if (mutex) {
            closeHandle(mutex);
            return 0x755;
        } else {
            m_log.Write(_Dect1(s_logGameMonSingleMutexMissing),
                        GetLastError());
            return 0x276;
        }
    } else {
        m_log.Write(_Dect1(s_logGameMonMutexMissing), GetLastError());
        return 0x294;
    }
}

bool CGameGuard::SendCSAuthToGameMonI(unsigned int auth) {
    if (*(unsigned char*)this) {
        ((CLog*)((unsigned char*)this + 0x20))->Write(_Dect1(s_logSendCSAuth), auth);
        return ((CCommPipe*)((unsigned char*)this + 0x1464))->SendCommPipe(
            0x8b1, 0x616, auth);
    }
    return false;
}

bool CGameGuard::SendUserIDToGameMonI(const char* userId) {
    if (*(unsigned char*)this) {
        strcpy((char*)this + 0x1630, userId);
        return ((CCommPipe*)((unsigned char*)this + 0x1464))->SendCommPipe(
            0x8b1, 0x613, (unsigned int)userId);
    }
    return false;
}

const char* CGameGuard::GetInfoFromGameMonI() {
    if (*(unsigned char*)this) {
        return ((CCommPipe*)((unsigned char*)this + 0x1464))->GetUserInfo();
    }
    return 0;
}

bool CGameGuard::SendCSAuth2ToGameMonI(_GG_AUTH_DATA* auth) {
    if (*(unsigned char*)this) {
        return ((CCommPipe*)((unsigned char*)this + 0x1464))->SendCommPipe(
            0x8b1, 0x619, (unsigned int)auth);
    }
    return false;
}

bool CGameGuard::CloseNPGameMonI() {
    unsigned char* raw = (unsigned char*)this;
    if (raw[1]) {
        return true;
    }

    CLog* log = (CLog*)(raw + 0x20);
    log->Write(_Dect1(s_logCloseGameMonBegin));
    raw[1] = 1;

    if (raw[0x1c]) {
        CloseNPSC();
        raw[0x1c] = 0;
    }

    BOOL (WINAPI *closeHandle)(HANDLE) = CloseHandle;
    HANDLE gameMonHandle = *(HANDLE*)(raw + 0x167c);
    if (gameMonHandle) {
        closeHandle(gameMonHandle);
        *(HANDLE*)(raw + 0x167c) = 0;
    }

    HANDLE readyEvent = *(HANDLE*)(raw + 0x18);
    if (readyEvent) {
        closeHandle(readyEvent);
        *(HANDLE*)(raw + 0x18) = 0;
    }

    HANDLE gameMonEvent = *(HANDLE*)(raw + 0x1680);
    if (gameMonEvent) {
        closeHandle(gameMonEvent);
        *(HANDLE*)(raw + 0x1680) = 0;
    }

    if (raw[0x1684]) {
        DisableCompatibleMode(0);
        RemoveCompatibleMode();
    }

    if (nSendErlIni && nSendErl) {
        FtpSendErl();
    }

    log->Write(_Dect1(s_logCloseGameMonEnd));
    raw[0] = 0;
    return ((CCommPipe*)(raw + 0x1464))->CloseCommPipe();
}

bool CGameGuard::InitNPSC() {
    unsigned char* raw = (unsigned char*)this;
    if (raw[0x1c]) {
        return true;
    }

    CLog* log = (CLog*)(raw + 0x20);
    log->Write(_Dect1(s_logInitNPSCBegin));

    char path[0x104];
    GetGameGuardDirectory(path, sizeof(path));
    strcat(path, "\\");
    strcat(path, _Dect1(s_npscFileName));
    log->Write(_Dect1(s_logInitNPSCPath), path);

    if (!((CFileAuthClient*)(raw + 0x15bc))->Auth(path, true)) {
        DWORD (WINAPI *getLastError)() = GetLastError;
        getLastError();
        unsigned int authResult = *(unsigned int*)(raw + 0x1600);
        log->Write(_Dect1(s_logInitNPSCAuthFailed),
                   authResult, getLastError());
        return false;
    }

    HMODULE module = LoadLibraryA(path);
    *(HMODULE*)(raw + 0x1628) = module;
    if (!module) {
        log->Write(_Dect1(s_logInitNPSCLoadLibraryFailed), GetLastError());
        return false;
    }

    *(NPSCInitProc*)(raw + 0x1688) =
        (NPSCInitProc)MyGetProcAddress(module, (LPCSTR)2);
    *(NPSCCloseProc*)(raw + 0x168c) =
        (NPSCCloseProc)MyGetProcAddress(*(HMODULE*)(raw + 0x1628), (LPCSTR)1);
    if (!*(NPSCInitProc*)(raw + 0x1688) ||
        !*(NPSCCloseProc*)(raw + 0x168c)) {
        log->Write(_Dect1(s_logInitNPSCProcFailed), GetLastError());
        return false;
    }

    HANDLE event = CreateEventA(0, 0, 0, 0);
    *(HANDLE*)(raw + 0x14) = event;
    if (!event) {
        log->Write(_Dect1(s_logInitNPSCCreateEventFailed), GetLastError());
        return false;
    }

    DWORD threadId;
    raw[0x1c] = 1;
    HANDLE thread = CreateThread(0, 0, SpeedCheckEventThread, this, 0, &threadId);
    *(HANDLE*)(raw + 0x162c) = thread;
    if (!thread) {
        log->Write(_Dect1(s_logInitNPSCCreateThreadFailed), GetLastError());
        return false;
    }

    log->Write(_Dect1(s_logInitNPSCThread), threadId);
    unsigned int result = (*(NPSCInitProc*)(raw + 0x1688))(
        *(unsigned int*)(raw + 0x1604),
        *(unsigned int*)(raw + 0x1678),
        *(unsigned int*)(raw + 0x1674),
        *(HANDLE*)(raw + 0x14));
    if (result != 0x6b) {
        log->Write(_Dect1(s_logInitNPSCResultFailed), result);
        return false;
    }

    log->Write(_Dect1(s_logInitNPSCSuccess));
    return true;
}

bool CGameGuard::CloseNPSC() {
    unsigned char* raw = (unsigned char*)this;
    if (!raw[0x1c]) {
        return true;
    }

    CLog* log = (CLog*)(raw + 0x20);
    log->Write(_Dect1(s_logCloseNPSCBegin));

    NPSCCloseProc closeProc = *(NPSCCloseProc*)(raw + 0x168c);
    closeProc(*(unsigned int*)(raw + 0x1604), (const char*)(raw + 0x1630));

    HANDLE event = *(HANDLE*)(raw + 0x14);
    raw[0x1c] = 0;
    if (event) {
        SetEvent(event);
    }

    if (WaitForSingleObject(*(HANDLE*)(raw + 0x162c), 2000) == WAIT_TIMEOUT) {
        log->Write(_Dect1(s_logCloseNPSCTimeout));
        TerminateThread(*(HANDLE*)(raw + 0x162c), 0);
    }

    CloseHandle(*(HANDLE*)(raw + 0x162c));

    HMODULE module = *(HMODULE*)(raw + 0x1628);
    if (module) {
        FreeLibrary(module);
    }

    *(unsigned int*)(raw + 0x1688) = 0;
    *(unsigned int*)(raw + 0x168c) = 0;
    log->Write(_Dect1(s_logCloseNPSCEnd));
    return true;
}


static DWORD WINAPI SpeedCheckEventThread(void* param)
{
    CGameGuard* guard = (CGameGuard*)param;
    for (;;) {
        if (WaitForSingleObject(guard->m_closeEvent, 0xde3) == WAIT_OBJECT_0) {
            if (!guard->m_npscActive)
                break;
            CommPipeCallback(&guard->m_pipe, 0x62d, 0);
        } else if (!CheckTimeAddr()) {
            CommPipeCallback(&guard->m_pipe, 0x62d, 10);
        } else if (!CheckSocketAddr()) {
            guard->m_pipe.SendCommPipe(0x8b1, 0x630, 12);
        } else if (guard->IsDebugger() || guard->m_debuggerFlag) {
            if (gg)
                gg->m_log.Write(_Dect1(s_logNpscDebugger), guard->IsDebugger(), guard->m_debuggerFlag);
            CommPipeCallback(&guard->m_pipe, 0x1f4, 0);
        }
    }
    return 0;
}

int CGameGuard::CheckSingleInstance(unsigned int timeout) {
    SECURITY_ATTRIBUTES attributes;
    SECURITY_DESCRIPTOR descriptor;
    attributes.nLength = sizeof(attributes);
    attributes.lpSecurityDescriptor = &descriptor;
    attributes.bInheritHandle = 0;
    SECURITY_ATTRIBUTES* attributesPtr = 0;

    if (InitializeSecurityDescriptor(&descriptor, SECURITY_DESCRIPTOR_REVISION)) {
        if (SetSecurityDescriptorDacl(&descriptor, TRUE, 0, FALSE)) {
            attributesPtr = &attributes;
        }
    }

    HANDLE (WINAPI *createMutex)(LPSECURITY_ATTRIBUTES, BOOL, LPCSTR) =
        CreateMutexA;
    HANDLE mutex = createMutex(attributesPtr, FALSE,
                               _Dect1(s_singleInstanceMutex));
    if (!mutex) {
        mutex = createMutex(attributesPtr, FALSE,
                            _Dect1(s_singleInstanceMutexAlt));
        if (!mutex) {
            DWORD (WINAPI *getLastError)() = GetLastError;
            getLastError();
            ((CLog*)((unsigned char*)this + 0x20))->Write(
                _Dect1(s_logSingleInstanceCreateFailed), getLastError());
            return false;
        }
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(mutex);
        return false;
    }

    if (WaitForSingleObject(mutex, timeout) == WAIT_TIMEOUT) {
        CloseHandle(mutex);
        return false;
    }

    return true;
}

int CGameGuard::CheckGameMonSingleInstance(unsigned int timeout) {
    SECURITY_ATTRIBUTES attributes;
    SECURITY_DESCRIPTOR descriptor;
    attributes.nLength = sizeof(attributes);
    attributes.lpSecurityDescriptor = &descriptor;
    attributes.bInheritHandle = 0;
    SECURITY_ATTRIBUTES* attributesPtr = 0;

    if (InitializeSecurityDescriptor(&descriptor, SECURITY_DESCRIPTOR_REVISION)) {
        if (SetSecurityDescriptorDacl(&descriptor, TRUE, 0, FALSE)) {
            attributesPtr = &attributes;
        }
    }

    HANDLE (WINAPI *createMutex)(LPSECURITY_ATTRIBUTES, BOOL, LPCSTR) =
        CreateMutexA;
    HANDLE mutex = createMutex(attributesPtr, FALSE,
                               _Dect1(s_gameMonSingleInstanceMutex));
    if (!mutex) {
        mutex = createMutex(attributesPtr, FALSE,
                            _Dect1(s_gameMonSingleInstanceMutexAlt));
        if (!mutex) {
            DWORD (WINAPI *getLastError)() = GetLastError;
            getLastError();
            ((CLog*)((unsigned char*)this + 0x20))->Write(
                _Dect1(s_logGameMonSingleInstanceCreateFailed),
                getLastError());
            return false;
        }
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        if (WaitForSingleObject(mutex, timeout) == WAIT_TIMEOUT) {
            ((CLog*)((unsigned char*)this + 0x20))->Write(
                _Dect1(s_logGameMonSingleInstanceTimeout));
            CloseHandle(mutex);
            return false;
        }
    }

    ReleaseMutex(mutex);
    CloseHandle(mutex);
    return true;
}

unsigned int CGameGuard::IsDebugger()
{
    if (bEncryptedClientArm)
        return 0;
    unsigned int detected = 0;

    if (m_isWinNt) {
        __asm {
            mov eax, fs:[18h]
            mov eax, [eax + 30h]
            movzx eax, byte ptr [eax + 2]
            mov detected, eax
        }
    } else {
        __asm {
            xor eax, eax
            mov al, fs:[20h]
            mov detected, eax
        }
    }
    return detected;
}

bool CALLBACK CommPipeCallback(CCommPipe* pipe, unsigned int msg,
                               unsigned int value) {
    if (gg) {
        ((CLog*)((unsigned char*)gg + 0x20))->Write(
            _Dect1(s_logCommPipeCallback), msg, value);
    }

    unsigned int callbackMsg;
    switch (msg) {
    case 0x60f:
        callbackMsg = 0x3e9;
        if (!*(unsigned char*)gg) {
            *(unsigned int*)((unsigned char*)gg + 8) = callbackMsg;
            *(unsigned int*)((unsigned char*)gg + 0x0c) = value;
            goto done;
        }
        break;

    case 0x610:
        callbackMsg = 0x3ea;
        if (!*(unsigned char*)gg) {
            *(unsigned int*)((unsigned char*)gg + 8) = callbackMsg;
            *(unsigned int*)((unsigned char*)gg + 0x0c) = value;
            goto done;
        }
        break;

    case 0x618:
        if (value < g_dwLoseAllowFactor * 800) {
            goto done;
        }
        GetProcessList();
        GetModuleList();
        if (gg) {
            ((CLog*)((unsigned char*)gg + 0x20))->Write(
                _Dect1(s_logCommPipeThreshold), value);
        }
        callbackMsg = 0x3e9;
        break;

    case 0x63a:
        if (gg) {
            ((CLog*)((unsigned char*)gg + 0x20))->Write(
                _Dect1(s_logCommPipeSetValue), value);
        }
        callbackMsg = 0x3f8;
        break;

    case 0x62d:
        callbackMsg = 0x3f3;
        if (!*(HWND*)((unsigned char*)gg + 4)) {
            *(unsigned int*)((unsigned char*)gg + 8) = callbackMsg;
            *(unsigned int*)((unsigned char*)gg + 0x0c) = value;
            goto done;
        }
        break;

    case 0x62e:
        callbackMsg = 0x3f4;
        if (!*(HWND*)((unsigned char*)gg + 4)) {
            *(unsigned int*)((unsigned char*)gg + 8) = callbackMsg;
            *(unsigned int*)((unsigned char*)gg + 0x0c) = value;
            goto done;
        }
        break;

    case 0x62f:
        callbackMsg = 0x3f5;
        if (!*(HWND*)((unsigned char*)gg + 4)) {
            *(unsigned int*)((unsigned char*)gg + 8) = callbackMsg;
            *(unsigned int*)((unsigned char*)gg + 0x0c) = value;
            goto done;
        }
        break;

    case 0x630:
        callbackMsg = 0x3f7;
        if (!*(HWND*)((unsigned char*)gg + 4)) {
            *(unsigned int*)((unsigned char*)gg + 8) = callbackMsg;
            *(unsigned int*)((unsigned char*)gg + 0x0c) = value;
            goto done;
        }
        break;

    case 0x637:
        callbackMsg = 0x3f6;
        value = 0xd2;
        if (!*(HWND*)((unsigned char*)gg + 4)) {
            *(unsigned int*)((unsigned char*)gg + 8) = callbackMsg;
            *(unsigned int*)((unsigned char*)gg + 0x0c) = value;
            goto done;
        }
        break;

    case 0x638:
        callbackMsg = 0x3f6;
        value = 0xdc;
        if (!*(HWND*)((unsigned char*)gg + 4)) {
            *(unsigned int*)((unsigned char*)gg + 8) = callbackMsg;
            *(unsigned int*)((unsigned char*)gg + 0x0c) = value;
            goto done;
        }
        break;

    case 0x639:
        callbackMsg = 0x3f6;
        value = 0xe6;
        if (!*(HWND*)((unsigned char*)gg + 4)) {
            *(unsigned int*)((unsigned char*)gg + 8) = callbackMsg;
            *(unsigned int*)((unsigned char*)gg + 0x0c) = value;
            goto done;
        }
        break;

    case 0x619:
        value = (unsigned int)pipe->GetCSAuth2();
        callbackMsg = 0x3f9;
        break;

    case 0x615:
        dwWorkingPID = value;
        if (!dwGameMonPID) {
            dwGameMonPID = value;
        } else if (value != dwGameMonPID) {
            if (gg) {
                ((CLog*)((unsigned char*)gg + 0x20))->Write(
                    _Dect1(s_logCommPipeTicketMismatch), value,
                    dwGameMonPID);
            }
            nSendErl = 2;
            callbackMsg = 0x3f6;
            value = 0xe6;
            if (!*(HWND*)((unsigned char*)gg + 4)) {
                *(unsigned int*)((unsigned char*)gg + 8) = callbackMsg;
                *(unsigned int*)((unsigned char*)gg + 0x0c) = value;
                goto done;
            }
            break;
        }
        if (CheckVersion(szGameMon, 0x7d6, 10, 5, 1)) {
            goto done;
        }
        if (gg) {
            ((CLog*)((unsigned char*)gg + 0x20))->Write(
                _Dect1(s_logCommPipeTicketStart));
        }
        callbackMsg = 0x3f6;
        value = 0xe6;
        if (!*(HWND*)((unsigned char*)gg + 4)) {
            *(unsigned int*)((unsigned char*)gg + 8) = callbackMsg;
            *(unsigned int*)((unsigned char*)gg + 0x0c) = value;
            goto done;
        }
        break;

    default:
        if (msg == 0x1f4) {
            callbackMsg = 0x3f6;
            value = 0x1f4;
        } else {
            callbackMsg = 0x3e8;
        }
        if (!*(HWND*)((unsigned char*)gg + 4)) {
            *(unsigned int*)((unsigned char*)gg + 8) = callbackMsg;
            *(unsigned int*)((unsigned char*)gg + 0x0c) = value;
            goto done;
        }
        break;
    }

    if (!NPGameMonCallback(callbackMsg, value)) {
        if (gg) {
            ((CLog*)((unsigned char*)gg + 0x20))->Write(
                _Dect1(s_logCommPipeCallbackFailed), callbackMsg, value);
        }
        ((unsigned char*)gg)[0x1f] = 1;
        return false;
    }
done:
    return true;
}

void CGameGuard::FtpSendErl() {
    unsigned char* raw = (unsigned char*)this;
    CLog* log = (CLog*)(raw + 0x20);

    log->Write(_Dect1(s_ftpBegin));

    HKEY key;
    DWORD disposition;
    DWORD size;
    DWORD value = 0;
    bool sameBuild = false;
    if (RegCreateKeyExA(HKEY_CURRENT_USER, _Dect1(s_ftpRegistryKey), 0, 0, 0,
                        KEY_ALL_ACCESS, 0, &key, &disposition) ==
        ERROR_SUCCESS) {
        char registryBuild[0x40];
        char currentBuild[0x40];
        if (disposition == REG_OPENED_EXISTING_KEY) {
            size = sizeof(registryBuild);
            RegQueryValueExA(key, _Dect1(s_ftpRegistryBuild), 0, 0,
                             (LPBYTE)registryBuild, &size);
            wsprintfA(currentBuild, "%hs", s_ftpBuildTimestamp);
            if (size && !lstrcmpA(registryBuild, currentBuild)) {
                sameBuild = true;
            }
            size = sizeof(value);
            RegQueryValueExA(key, _Dect1(s_ftpRegistryValue), 0, 0,
                             (LPBYTE)&value, &size);
        }

        wsprintfA(registryBuild, "%hs", s_ftpBuildTimestamp);
        size = lstrlenA(registryBuild) + 1;
        RegSetValueExA(key, _Dect1(s_ftpRegistryBuildSet), 0, REG_SZ,
                       (const BYTE*)registryBuild, size);
        size = sizeof(value);
        if (sameBuild) {
            ++value;
        } else {
            value = 0;
        }
        RegSetValueExA(key, _Dect1(s_ftpRegistryValue), 0, REG_DWORD,
                       (const BYTE*)&value, size);
        RegCloseKey(key);
    }

    if (value == 1 || value > 5) {
        return;
    }
    if (sameBuild && nSendErl >= 9) {
        return;
    }

    log->Write(_Dect1(s_ftpCloseLog));
    m_log.Close();

    if (!safeWinInet.GetModule()) {
        if (!safeWinInet.Init(_Dect1(s_ftpWinInetDll))) {
            return;
        }
    }

    InternetOpenAProc internetOpen = (InternetOpenAProc)MyGetProcAddress(
        safeWinInet.GetModule(), _Dect1(s_ftpInternetOpenA));
    InternetConnectAProc internetConnect =
        (InternetConnectAProc)MyGetProcAddress(
            safeWinInet.GetModule(),
            _Dect1(s_ftpInternetConnectA));
    InternetCloseHandleProc internetClose =
        (InternetCloseHandleProc)MyGetProcAddress(
            safeWinInet.GetModule(),
            _Dect1(s_ftpInternetCloseHandle));
    FtpPutFileAProc ftpPutFile = (FtpPutFileAProc)MyGetProcAddress(
        safeWinInet.GetModule(), _Dect1(s_ftpFtpPutFileA));
    if (!internetOpen || !internetConnect || !internetClose || !ftpPutFile) {
        return;
    }

    HINTERNET internet = internetOpen(_Dect(s_ftpInternetAgent), 0, 0, 0, 0);
    if (!internet) {
        return;
    }

    HINTERNET connect = internetConnect(
        internet, _Dect(s_ftpServer), 0x17ac, _Dect(s_ftpUser),
        _Dect1(s_ftpPassword), 1, 0x08000000, 0);
    if (!connect) {
        internetClose(internet);
        return;
    }

    char computerName[0x80];
    char userName[0x80];
    DWORD nameSize;
    computerName[0] = 0;
    nameSize = sizeof(computerName);
    GetComputerNameA(computerName, &nameSize);
    userName[0] = 0;
    nameSize = sizeof(userName);
    GetUserNameA(userName, &nameSize);

    SYSTEMTIME time;
    GetLocalTime(&time);

    char* reason;
    if (nSendErl == 1) {
        reason = _Dect(s_ftpReason1);
    } else if (nSendErl == 2) {
        reason = _Dect(s_ftpReason2);
    } else if (nSendErl == 3) {
        reason = _Dect(s_ftpReason3);
    } else if (nSendErl == 4) {
        reason = _Dect(s_ftpReason4);
    } else if (nSendErl == 5) {
        reason = _Dect(s_ftpReason5);
    } else if (nSendErl == 6) {
        reason = _Dect(s_ftpReason6);
    } else if (nSendErl == 7) {
        reason = _Dect(s_ftpReason7);
    } else if (nSendErl == 8) {
        reason = _Dect(s_ftpReason8);
    } else if (nSendErl == 9) {
        reason = _Dect(s_ftpReason9);
    } else {
        reason = _Dect(s_ftpReasonOther);
    }

    char remoteFile[0x104];
    _snprintf(remoteFile, sizeof(remoteFile), _Dect1(s_ftpRemoteFormat),
              raw + 0x1608, '/', reason,
              *(unsigned int*)((unsigned char*)gg + 0x10), computerName,
              userName, raw + 0x1630, time.wYear, time.wMonth, time.wDay,
              time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);

    ftpPutFile(connect, szLogFile, remoteFile, 0x04000002, 0);
    GetLastError();
    internetClose(internet);
    internetClose(connect);
}

bool InitToolhelp32() {
    static bool bInit = 0;
    if (bInit) {
        return true;
    }

    HMODULE kernel = GetModuleHandleA(_Dect1("\x01\x47\x3a\xeb\x85\x66\x30\x91\x53\xb7\xf9\xa5\xd0\x57\xbe\xc3"));
    if (!kernel) {
        return false;
    }

    pCreateToolhelp32Snapshot = (CreateToolhelp32SnapshotProc)MyGetProcAddress(
        kernel, _Dect1("\x01\x0e\x8f\xfe\xa8\x88\x42\xcf\x37\x67\x6b\x99\x74\xe6\xbf\xdb\x1f\xe2\xdc\x34\x18\x74\xe6\xbe\xd0\x4a\xf0\x62"));
    if (!pCreateToolhelp32Snapshot) {
        return false;
    }

    pProcess32First = (Process32FirstProc)MyGetProcAddress(
        kernel, _Dect1("\x01\xc4\xb1\x42\x4d\xe2\x86\x97\x70\x0b\xd2\x2f\xbf\xa6\xb0\xb6\xf6\xbc"));
    if (!pProcess32First) {
        return false;
    }

    pProcess32Next = (Process32NextProc)MyGetProcAddress(
        kernel, _Dect1("\x01\x6b\xa6\x26\xea\x15\x01\xe0\xa7\x0c\xc5\x68\x78\x59\x1b\xcb\x26"));
    if (!pProcess32Next) {
        return false;
    }

    pModule32First = (Module32FirstProc)MyGetProcAddress(
        kernel, _Dect1("\x01\x4e\x4f\x2b\xe6\x55\x83\x9b\x6f\x27\xcc\x04\x9d\xa3\xe5\x8d\x47"));
    if (!pModule32First) {
        return false;
    }

    pModule32Next = (Module32NextProc)MyGetProcAddress(
        kernel, _Dect1("\x01\x90\x15\x74\xec\x73\xe9\x95\xb5\xa1\xb6\xfa\xdf\x89\x85\x44"));
    if (!pModule32Next) {
        return false;
    }

    bInit = true;
    return true;
}


static void GetProcessList()
{
    if (!InitToolhelp32())
        return;

    PROCESSENTRY32 entry = {0};
    HANDLE snapshot = pCreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        return;
    entry.dwSize = sizeof(entry);
    if (pProcess32First(snapshot, &entry)) {
        if (gg)
            gg->m_log.Write(_Dect1(s_logProcessHeader));
        do {
            if (gg)
                gg->m_log.Write(_Dect1(s_logProcessFormat), entry.szExeFile, entry.th32ProcessID, entry.cntThreads, entry.pcPriClassBase);
        } while (pProcess32Next(snapshot, &entry));
    }
    CloseHandle(snapshot);
}

static void GetModuleList()
{
    if (!InitToolhelp32())
        return;

    MODULEENTRY32 entry = {0};
    HANDLE snapshot = pCreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
    if (snapshot == INVALID_HANDLE_VALUE)
        return;
    entry.dwSize = sizeof(entry);
    if (pModule32First(snapshot, &entry)) {
        if (gg)
            gg->m_log.Write(_Dect1(s_logModuleHeader));
        do {
            if (gg)
                gg->m_log.Write(_Dect1(s_logModuleFormat), entry.szModule, entry.modBaseAddr, entry.modBaseSize, entry.szExePath);
        } while (pModule32Next(snapshot, &entry));
    }
    CloseHandle(snapshot);
}


static LONG WINAPI FvUnhandledExceptionFilter(_EXCEPTION_POINTERS* exceptionInfo)
{
    static bool bInException = 0;
    if (bInException)
        return EXCEPTION_EXECUTE_HANDLER;
    bInException = true;
    EXCEPTION_RECORD* record = exceptionInfo->ExceptionRecord;
    if (gg)
        gg->m_log.Write(_Dect1(s_logUnhandledException), record->ExceptionCode, record->ExceptionAddress, GetCurrentThreadId(), g_dwThreadId);

    char dump[128];
    int length = 0;
    const unsigned char* code = (const unsigned char*)record->ExceptionAddress;
    for (int i = 0; i < 32; ++i)
        length += sprintf(dump + length, _Dect(s_exceptionByteFormat), code[i]);
    dump[length] = 0;
    if (gg)
        gg->m_log.Write(_Dect1(s_logExceptionBytes), dump);
    GetProcessList();
    GetModuleList();
    if (gg && !gg->m_active) {
        gg->m_initResult += 10000;
        ExitProcess(100);
    }
    return EXCEPTION_CONTINUE_SEARCH;
}


extern "C" void* NPDect(void* value) {
    return value;
}

static FARPROC lpGTC = 0;
static FARPROC lpQPC = 0;
static FARPROC lpTGT = 0;
static FARPROC lpWSASend = 0;
static FARPROC lpsend = 0;
static FARPROC lpGTC2 = 0;
static FARPROC lpQPC2 = 0;
static FARPROC lpTGT2 = 0;
static FARPROC lpWSASend2 = 0;
static FARPROC lpsend2 = 0;
static bool bInitTA = 0;
static bool bInitSA = 0;


static unsigned int InitTimeAddr()
{
    int hooks = 0;
    HMODULE acLayers = GetModuleHandleA(_Dect1(s_acLayersDll));
    HMODULE ttCore = GetModuleHandleA(_Dect1(s_ttCoreDll));
    HMODULE kernel = GetModuleHandleA(_Dect1(s_clockKernelDll));
    if (!kernel)
        return 2;

    lpQPC = (FARPROC)MyGetProcAddress(kernel, _Dect1(s_performanceProc));
    if (lpQPC) {
        lpQPC2 = (FARPROC)QueryPerformanceCounter;
        if (gg)
            gg->m_log.Write(_Dect1(s_logPerformancePointers), lpQPC, lpQPC2);
        if ((unsigned int)lpQPC < (unsigned int)kernel && !ttCore && (!acLayers || (unsigned int)lpQPC < (unsigned int)acLayers)) {
            if (gg)
                gg->m_log.Write(_Dect1(s_logPerformanceHookModule), kernel);
            hooks = 1;
        }
    }

    lpGTC = (FARPROC)MyGetProcAddress(kernel, _Dect1(s_tickCountProc));
    if (lpGTC) {
        lpGTC2 = (FARPROC)GetTickCount;
        if (gg)
            gg->m_log.Write(_Dect1(s_logTickCountPointers), lpGTC, lpGTC2);
        if ((unsigned int)lpGTC < (unsigned int)kernel && !ttCore && (!acLayers || (unsigned int)lpQPC < (unsigned int)acLayers)) {
            if (gg)
                gg->m_log.Write(_Dect1(s_logTickCountHookModule), kernel);
            ++hooks;
        }
    }

    FARPROC timer = (FARPROC)timeGetTime;
    timeGetTime();
    HMODULE winmm = GetModuleHandleA(_Dect1(s_winmmDll));
    if (winmm) {
        lpTGT = (FARPROC)MyGetProcAddress(winmm, _Dect1(s_timeGetTimeProc));
        if (lpTGT) {
            lpTGT2 = timer;
            if (gg)
                gg->m_log.Write(_Dect1(s_logTimeGetTimePointers), lpTGT, lpTGT2);
            if ((unsigned int)lpTGT < (unsigned int)winmm && !ttCore && (!acLayers || (unsigned int)lpQPC < (unsigned int)acLayers)) {
                if (gg)
                    gg->m_log.Write(_Dect1(s_logTimeGetTimeHookModule), winmm);
                ++hooks;
            }
        }
    }
    bInitTA = true;
    return hooks == 0;
}


static unsigned int CheckTimeAddr()
{
    if (bEncryptedClient)
        return 1;
    if (!bInitTA)
        return 2;
    int hooks = 0;
    if (lpQPC2 != (FARPROC)QueryPerformanceCounter) {
        if (gg)
            gg->m_log.Write(_Dect1(s_logPerformanceHook), QueryPerformanceCounter);
        hooks = 1;
    }
    if (lpGTC2 != (FARPROC)GetTickCount) {
        if (gg)
            gg->m_log.Write(_Dect1(s_logTickCountHook), GetTickCount);
        ++hooks;
    }
    if (lpTGT2 != (FARPROC)timeGetTime) {
        if (gg)
            gg->m_log.Write(_Dect1(s_logTimeGetTimeHook), timeGetTime);
        ++hooks;
    }
    return hooks == 0;
}


static unsigned int InitSocketAddr()
{
    int hooks = 0;
    HMODULE sockets = GetModuleHandleA(_Dect1(s_ws2Dll));
    if (!sockets)
        return 2;
    lpWSASend = (FARPROC)MyGetProcAddress(sockets, _Dect1(s_wsaSendProc));
    if (lpWSASend) {
        lpWSASend2 = (FARPROC)WSASend;
        if (gg)
            gg->m_log.Write(_Dect1(s_logWSASendPointers), lpWSASend, lpWSASend2);
        if ((unsigned int)lpWSASend < (unsigned int)sockets) {
            if (gg)
                gg->m_log.Write(_Dect1(s_logWSASendHookModule), sockets);
            hooks = 1;
        }
    }
    lpsend = (FARPROC)MyGetProcAddress(sockets, _Dect1(s_sendProc));
    if (lpsend) {
        lpsend2 = (FARPROC)send;
        if (gg)
            gg->m_log.Write(_Dect1(s_logSendPointers), lpsend, lpsend2);
        if ((unsigned int)lpsend < (unsigned int)sockets) {
            if (gg)
                gg->m_log.Write(_Dect1(s_logSendHookModule), sockets);
            ++hooks;
        }
    }
    bInitSA = true;
    return hooks == 0;
}


static unsigned int CheckSocketAddr()
{
    if (!bInitSA)
        return 2;
    int hooks = 0;
    if (lpWSASend2 != (FARPROC)WSASend) {
        if (gg)
            gg->m_log.Write(_Dect1(s_logWSASendHook), WSASend);
        hooks = 1;
    }
    if (lpsend2 != (FARPROC)send) {
        if (gg)
            gg->m_log.Write(_Dect1(s_logSendHook), send);
        ++hooks;
    }
    return hooks == 0;
}

extern "C" void PPEncryptClientPacket(void* packet, unsigned int size, void* key);
extern "C" void PPDecryptServerPacket(void* packet, unsigned int size, void* key);
extern "C" void PPDecryptClientPacket(void* packet, unsigned int size, void* key);


extern "C" void EncryptPacket(void* packet, unsigned int size) {
    PPEncryptClientPacket(packet, size, &dwClientSequenceNumber);
}


extern "C" void DecryptPacket(void* packet, unsigned int size) {
    PPDecryptServerPacket(packet, size, &dwServerSequenceNumber);
}


extern "C" void EncryptPeerPacket(void* packet, unsigned int size) {
    PPEncryptClientPacket(packet, size, &size);
}


extern "C" void DecryptPeerPacket(void* packet, unsigned int size) {
    PPDecryptClientPacket(packet, size, &size);
}

#define s_logGameMonCrcFileFailed "\001\302\253\"\204\234`q\315P\262\246\362\336U\263\306*+\1774\"CG;v\036\216"
#define s_gameMonCrcKey "\001\273\226\325y\370\030\007e\216\364n\346. \300\2036\330//e\020\326&s\250\333\250\250\372\206\201\253\221\332\007\022\331f\023\375\202\267\257W\312\"vZ"
#define s_logGameMonCrcRegistryMismatch "\001\"\313\210U+D\341m\365\333N\33685\022\352\336\007\232B\026"
#define s_logGameMonCrcKeyFailed "\001\370M3\332l k:\014\317\025\356\364\3239(\220\027\340\\\310P"
#define s_logGameMonCrcMismatch "\001G:\365\243f/\274D\270\352\363\227U\264\217csvg+\033"

static bool CheckGameMonCrc(const char* fileName, char* valueName, unsigned int expected)
{
    DWORD size = sizeof(DWORD);
    DWORD stored = 0;
    unsigned int crc = 1;
    CFvCrc32 crc32;
    HKEY key;
    DWORD disposition;

    if (!crc32.Crc32File(fileName, &crc)) {
        GetLastError();
        if (gg)
            gg->m_log.Write(_Dect1(s_logGameMonCrcFileFailed), fileName, crc32.GetError(), GetLastError());
    }
    LONG result = RegCreateKeyExA(HKEY_CURRENT_USER, _Dect1(s_gameMonCrcKey), 0, 0, 0, KEY_ALL_ACCESS, 0, &key, &disposition);
    if (result == ERROR_SUCCESS) {
        if (disposition == REG_OPENED_EXISTING_KEY)
            RegQueryValueExA(key, _Dect1(valueName), 0, 0, (BYTE*)&stored, &size);
        RegCloseKey(key);
        if (crc != stored) {
            if (gg)
                gg->m_log.Write(_Dect1(s_logGameMonCrcRegistryMismatch), crc, stored);
            return false;
        }
    } else if (gg) {
        gg->m_log.Write(_Dect1(s_logGameMonCrcKeyFailed), result);
    }
    if (crc != expected) {
        if (gg)
            gg->m_log.Write(_Dect1(s_logGameMonCrcMismatch), crc, expected);
        return false;
    }
    return true;
}

typedef BOOL (WINAPI *ReadProcessMemoryProc)(HANDLE process, LPCVOID address, LPVOID buffer, DWORD size, LPDWORD read);
typedef BOOL (WINAPI *Toolhelp32ReadProcessMemoryProc)(DWORD processId, LPCVOID address, LPVOID buffer, DWORD size, LPDWORD read);



static unsigned int ReadMemCrc(PROCESS_INFORMATION& process)
{
    unsigned char header[0x40];
    unsigned int crc = 0;
    HMODULE kernel = GetModuleHandleA(_Dect1("\001\341\010]G\330\002\347\261\331ks\322I\254\025"));
    Toolhelp32ReadProcessMemoryProc toolhelpRead = (Toolhelp32ReadProcessMemoryProc)MyGetProcAddress(kernel, _Dect1("\001\337\002$\242t\345\273\326\026\376\2375yH\342\257\307r\355y\030\317D\255\236\327\"I\331C"));
    ReadProcessMemoryProc readMemory = (ReadProcessMemoryProc)MyGetProcAddress(kernel, _Dect1("\001@%\271c\251\374t9\006\372\233D\357~-<)j:h"));
    BOOL read = FALSE;
    if (readMemory)
        read = readMemory(process.hProcess, (LPCVOID)0x400000, header, sizeof(header), 0);
    if (toolhelpRead && !read)
        read = toolhelpRead(process.dwProcessId, (LPCVOID)0x400000, header, sizeof(header), 0);
    if (read)
        crc = ~*(unsigned int*)(header + 0x24);
    return crc;
}


static bool CheckVersion(char* fileName, unsigned short major, unsigned short minor, unsigned short build, unsigned short revision)
{
    bool compatible = true;
    HMODULE version = LoadLibraryA(_Dect1(s_versionDll));
    if (version) {
        typedef DWORD (WINAPI *VersionSizeProc)(LPCSTR, LPDWORD);
        typedef BOOL (WINAPI *VersionInfoProc)(LPCSTR, DWORD, DWORD, LPVOID);
        typedef BOOL (WINAPI *VersionQueryProc)(LPCVOID, LPCSTR, LPVOID*, PUINT);
        VersionSizeProc getSize = (VersionSizeProc)MyGetProcAddress(version, _Dect1(s_versionSizeProc));
        VersionInfoProc getInfo = (VersionInfoProc)MyGetProcAddress(version, _Dect1(s_versionInfoProc));
        VersionQueryProc query = (VersionQueryProc)MyGetProcAddress(version, _Dect1(s_versionQueryProc));
        if (getSize && getInfo && query) {
            DWORD handle;
            DWORD size = getSize(fileName, &handle);
            if (size) {
                void* data = operator new(size);
                if (getInfo(fileName, handle, size, data)) {
                    VS_FIXEDFILEINFO* info;
                    UINT infoSize;
                    query(data, "\\", (void**)&info, &infoSize);
                    if (info->dwSignature == 0xfeef04bd) {
                        unsigned int installed = ((HIWORD(info->dwFileVersionMS) * 100 + LOWORD(info->dwFileVersionMS)) * 100 + HIWORD(info->dwFileVersionLS)) * 100 + LOWORD(info->dwFileVersionLS);
                        unsigned int required = (((WORD)major * 100 + (WORD)minor) * 100 + (WORD)build) * 100 + (WORD)revision;
                        if (gg)
                            gg->m_log.Write(_Dect1(s_logFileVersion), fileName, installed, required);
                        if (installed < required)
                            compatible = false;
                    }
                }
                operator delete(data);
            }
        }
        FreeLibrary(version);
    }
    return compatible;
}


static FARPROC WINAPI MyGetProcAddress(HINSTANCE module, LPCSTR name)
{
    if (IsBadReadPtr(module, 4))
        return 0;

    FARPROC proc = 0;
    void* pAddr = 0;
    __try {
        IMAGE_DOS_HEADER* pDosHeader = (IMAGE_DOS_HEADER*)module;
        if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
            return 0;
        IMAGE_NT_HEADERS32* pNTHeader = (IMAGE_NT_HEADERS32*)((unsigned char*)module + pDosHeader->e_lfanew);
        if (pNTHeader->Signature != IMAGE_NT_SIGNATURE)
            return 0;

        unsigned int exportRva = 0;
        unsigned int exportSize = 0;
        if (pNTHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
            IMAGE_NT_HEADERS64* nt64 = (IMAGE_NT_HEADERS64*)pNTHeader;
            exportRva = nt64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
            exportSize = nt64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
        } else {
            exportRva = pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
            exportSize = pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
        }
        if (!exportRva)
            return 0;

        IMAGE_EXPORT_DIRECTORY* pExportDirectory = (IMAGE_EXPORT_DIRECTORY*)((unsigned char*)module + exportRva);
        unsigned int index = 0;
        if ((unsigned int)name < 0x10000) {

            index = (unsigned int)name;
            index--;
        } else {
            unsigned int length = 0;
            const char* cursor = name;
            while (*cursor) {
                ++cursor;
                if (++length >= 0x100)
                    return 0;
            }

            unsigned int* nameTable = (unsigned int*)((unsigned char*)module + pExportDirectory->AddressOfNames);
            bool found = false;
            for (index = 0; index < pExportDirectory->NumberOfNames; ++index) {
                unsigned char* exportName = (unsigned char*)module + *nameTable;
                cursor = name;
                if (IsBadReadPtr(exportName, 4))
                    return 0;

                bool bEqual = true;
                for (unsigned int nIdx = 0; nIdx < length; ++nIdx) {
                    if (*exportName - *cursor != 0) {
                        bEqual = false;
                        break;
                    }
                    ++exportName;
                    ++cursor;
                }
                if (bEqual) {
                    found = true;
                    break;
                }
                ++nameTable;
            }
            if (!found)
                return 0;
            unsigned short* ordinals = (unsigned short*)((unsigned char*)module + pExportDirectory->AddressOfNameOrdinals);
            ordinals += index;
            index = *ordinals;
        }

        unsigned int* pFuncs = (unsigned int*)((unsigned char*)module + pExportDirectory->AddressOfFunctions);
        pFuncs += index;
        proc = (FARPROC)((unsigned char*)module + *pFuncs);
        if ((unsigned char*)proc >= (unsigned char*)module + exportRva && (unsigned char*)proc < (unsigned char*)module + exportRva + exportSize) {
            char forwarder[128];
            strcpy(forwarder, (const char*)proc);
            char* p = (char*)_mbsstr((unsigned char*)forwarder, (const unsigned char*)".");
            if (p) {
                *p = 0;
                ++p;
            }
            proc = MyGetProcAddress(GetModuleHandleA(forwarder), p);
        }
        return proc;
    } __except ((LONG)(pAddr = (GetExceptionInformation())->ExceptionRecord->ExceptionAddress)) {
        if (gg)
            gg->m_log.Write(_Dect1(s_logResolverException), GetExceptionCode(), pAddr);
    }
    return proc;
}


static bool CheckRsaBase()
{
    HKEY key;
    unsigned char registryValue[MAX_PATH];
    DWORD size;
    bool repaired = false;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, _Dect1(s_ieRegistryKey), 0, KEY_QUERY_VALUE, &key) == ERROR_SUCCESS) {
        size = sizeof(registryValue);
        if (RegQueryValueExA(key, _Dect1(s_ieVersionName), 0, 0, registryValue, &size) == ERROR_SUCCESS && gg)
            gg->m_log.Write(_Dect1(s_logIEVersion), registryValue);
        RegCloseKey(key);
    }

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, _Dect1(s_providerRegistryKey), 0, KEY_QUERY_VALUE | KEY_SET_VALUE, &key) == ERROR_SUCCESS) {
        size = sizeof(registryValue);
        if (RegQueryValueExA(key, _Dect1(s_providerSignatureName), 0, 0, registryValue, &size) == ERROR_SUCCESS && gg)
            gg->m_log.Write(_Dect1(s_logProviderSignature), registryValue[0], registryValue[1], registryValue[2], registryValue[3]);

        char path[MAX_PATH];
        GetSystemDirectoryA(path, sizeof(path));
        strcat(path, _Dect1(s_providerSuffix));
        DWORD fileSize = 0;
        HANDLE file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        if (file == INVALID_HANDLE_VALUE) {
            if (gg)
                gg->m_log.Write(_Dect1(s_logProviderOpenFailed), GetLastError(), path);
        } else {
            fileSize = GetFileSize(file, 0);
            if (gg)
                gg->m_log.Write(_Dect1(s_logProviderFileSize), fileSize);
            CloseHandle(file);
        }

        HMODULE version = LoadLibraryA(_Dect1(s_versionDll));
        if (version) {
            typedef DWORD (WINAPI *VersionSizeProc)(LPCSTR, LPDWORD);
            typedef BOOL (WINAPI *VersionInfoProc)(LPCSTR, DWORD, DWORD, LPVOID);
            typedef BOOL (WINAPI *VersionQueryProc)(LPCVOID, LPCSTR, LPVOID*, PUINT);
            VersionSizeProc getSize = (VersionSizeProc)MyGetProcAddress(version, _Dect1(s_versionSizeProc));
            VersionInfoProc getInfo = (VersionInfoProc)MyGetProcAddress(version, _Dect1(s_versionInfoProc));
            VersionQueryProc query = (VersionQueryProc)MyGetProcAddress(version, _Dect1(s_versionQueryProc));
            if (getSize && getInfo && query) {
                DWORD handle;
                DWORD versionSize = getSize(path, &handle);
                if (versionSize) {
                    void* data = operator new(versionSize);
                    getInfo(path, handle, versionSize, data);
                    VS_FIXEDFILEINFO* info;
                    UINT infoSize;
                    query(data, "\\", (void**)&info, &infoSize);
                    if (info->dwSignature == 0xfeef04bd && gg)
                        gg->m_log.Write(_Dect1(s_logProviderVersion), HIWORD(info->dwFileVersionMS), LOWORD(info->dwFileVersionMS), HIWORD(info->dwFileVersionLS), LOWORD(info->dwFileVersionLS));
                    operator delete(data);
                }
            }
            FreeLibrary(version);
        }

        if (fileSize == 0x18d10) {
            if (gg)
                gg->m_log.Write(_Dect1(s_logProviderIE6));
            unsigned char signature[136] = {
                0x2a, 0xc7, 0x0e, 0x10, 0x06, 0xbe, 0xa7, 0xcd, 0xf5, 0xfa, 0x66, 0x0f, 0x86, 0xa3, 0x8a, 0x41,
                0x58, 0xc7, 0xaf, 0x6e, 0xf5, 0xc1, 0x92, 0x3d, 0xf5, 0x56, 0xa8, 0x81, 0x48, 0x2e, 0x89, 0x7c,
                0x04, 0x36, 0x53, 0xe3, 0x64, 0x79, 0x16, 0x34, 0x7d, 0xe8, 0xb0, 0xae, 0xd7, 0x3c, 0xc5, 0xeb,
                0x7a, 0x25, 0x2d, 0xf2, 0xb8, 0xe4, 0xf4, 0xf2, 0xcd, 0x95, 0x0b, 0xdd, 0x4f, 0x0a, 0x9f, 0x27,
                0x19, 0xb5, 0xf6, 0x52, 0x16, 0x77, 0x01, 0x2c, 0xf6, 0xae, 0x9a, 0xd3, 0x7f, 0xbf, 0x6a, 0x22,
                0x91, 0x6b, 0xf3, 0xdb, 0x93, 0x0d, 0x1c, 0xd2, 0x00, 0xb5, 0xb8, 0x14, 0x1c, 0x3e, 0x9e, 0x94,
                0x7b, 0xa9, 0x8e, 0xc6, 0x84, 0xf5, 0x2b, 0xfa, 0xb5, 0x95, 0x00, 0xa5, 0x95, 0xb2, 0x65, 0x8c,
                0x31, 0x19, 0x77, 0x00, 0xcd, 0x5b, 0x4e, 0x92, 0x68, 0xe4, 0x42, 0x66, 0x17, 0xb5, 0xd4, 0x93,
                0, 0, 0, 0, 0, 0, 0, 0
            };
            RegSetValueExA(key, _Dect1(s_providerSignatureIE6), 0, REG_BINARY, signature, sizeof(signature));
            repaired = true;
        } else if (fileSize == 0x17110) {
            if (gg)
                gg->m_log.Write(_Dect1(s_logProviderIE5));
            unsigned char signature[136] = {
                0x5f, 0x71, 0x69, 0x53, 0x8a, 0x8e, 0x54, 0xac, 0x09, 0x3f, 0x65, 0x5a, 0x26, 0x8d, 0x77, 0x15,
                0x9e, 0x68, 0xbb, 0x07, 0xed, 0x8f, 0xf0, 0xb7, 0xf1, 0x99, 0x05, 0x15, 0x80, 0x64, 0xa8, 0xd8,
                0x87, 0xc3, 0x58, 0xa9, 0xce, 0x26, 0xfe, 0xb3, 0x07, 0xd7, 0xa3, 0x81, 0xbe, 0xb8, 0x6e, 0x07,
                0x15, 0x68, 0x44, 0x17, 0xbe, 0x81, 0x80, 0x75, 0xa1, 0xdb, 0xab, 0x21, 0xd1, 0x1f, 0xbc, 0x9c,
                0x1d, 0x49, 0xf0, 0xe5, 0xdd, 0x49, 0x7f, 0xb5, 0xe4, 0x09, 0xf0, 0x39, 0x94, 0x2a, 0x6b, 0x83,
                0x0f, 0xdb, 0x56, 0x0e, 0xa3, 0x61, 0x9f, 0x12, 0xd4, 0xd7, 0x20, 0x90, 0x54, 0x9b, 0xc6, 0x99,
                0x0c, 0x44, 0x03, 0xd5, 0x1b, 0x9a, 0x2a, 0xa0, 0xa2, 0x8c, 0x1c, 0xf6, 0x68, 0xa5, 0x57, 0x2e,
                0x11, 0x40, 0xa5, 0x8f, 0x77, 0x71, 0xec, 0x6a, 0x69, 0xa0, 0xcb, 0x1e, 0x20, 0x8e, 0xe0, 0x05,
                0, 0, 0, 0, 0, 0, 0, 0
            };
            RegSetValueExA(key, _Dect1(s_providerSignatureIE5), 0, REG_BINARY, signature, sizeof(signature));
            repaired = true;
        }
        RegCloseKey(key);
    }
    return repaired;
}


static bool RemoveCompatibleMode()
{
    bool removed = false;
    wchar_t modulePath[MAX_PATH];
    char directory[MAX_PATH];
    GetModuleFileNameW(0, modulePath, MAX_PATH);
    WideCharToMultiByte(0, 0, modulePath, wcslen(modulePath) + 1, directory, sizeof(directory), 0, 0);
    unsigned char* slash = _mbsrchr((unsigned char*)directory, '\\');
    if (slash)
        *slash = 0;

    HKEY key;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, _Dect1(s_compatibilityLayersKey), 0, KEY_QUERY_VALUE | KEY_SET_VALUE, &key) == ERROR_SUCCESS) {
        DWORD index = 0;
        DWORD size = MAX_PATH;
        char name[MAX_PATH];
        char value[32];
        LONG status = RegEnumValueA(key, index++, name, &size, 0, 0, 0, 0);
        while (status != ERROR_NO_MORE_ITEMS) {
            if (status == ERROR_SUCCESS) {
                char valueDirectory[MAX_PATH];
                strcpy(valueDirectory, name);
                slash = _mbsrchr((unsigned char*)valueDirectory, '\\');
                if (slash)
                    *slash = 0;
                if (_mbsicmp((unsigned char*)valueDirectory, (unsigned char*)directory) == 0) {
                    if (RegQueryValueExA(key, name, 0, 0, (BYTE*)value, &size) == ERROR_SUCCESS) {
                        if (gg)
                            gg->m_log.Write(_Dect1(s_logCompatibilityMode), value);
                        if (RegDeleteValueA(key, name) == ERROR_SUCCESS)
                            removed = true;
                    }
                }
            }
            size = MAX_PATH;
            status = RegEnumValueA(key, index++, name, &size, 0, 0, 0, 0);
        }
        RegCloseKey(key);
    }
    return removed;
}


static bool DisableCompatibleMode(int value)
{
    HKEY key;
    bool written = false;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, _Dect1(s_registryKey), 0, KEY_ALL_ACCESS, &key) == ERROR_SUCCESS) {
        if (RegSetValueExA(key, _Dect1(s_registryValue), 0, REG_DWORD, (const BYTE*)&value, sizeof(value)) == ERROR_SUCCESS)
            written = true;
        RegCloseKey(key);
    }
    return written;
}


static void CheckEncryptedClient()
{
    IMAGE_DOS_HEADER* image = (IMAGE_DOS_HEADER*)GetModuleHandleA(0);
    if (image->e_magic == IMAGE_DOS_SIGNATURE) {
        const unsigned int* markers = (const unsigned int*)&image->e_res2[4];
        if (markers[1] && markers[2]) {
            bEncryptedClient = true;
            if (!markers[0])
                bEncryptedClientArm = true;
        }
        if (markers[2] && markers[1] && markers[0]) {
            unsigned int difference = markers[1] > markers[2] ? markers[1] - markers[2] : markers[2] - markers[1];
            if (markers[0] == difference || (markers[1] == markers[2] && markers[0] == markers[1]))
                bEncryptedClientArm = true;
        }
    }
}



static BOOL SendGameMonMessage(const void* message, unsigned int size) {
    HANDLE mapping = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE,
                                      _Dect1(s_gameMonMessageMapping));
    if (!mapping) {
        mapping = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE,
                                   _Dect1(s_gameMonMessageMappingAlt));
        if (!mapping) {
            return FALSE;
        }
    }

    char* view = (char*)MapViewOfFile(mapping, FILE_MAP_ALL_ACCESS, 0, 0, 0x1000);
    if (!view) {
        CloseHandle(mapping);
        return FALSE;
    }

    HANDLE mutex = OpenMutexA(SYNCHRONIZE, FALSE, _Dect1(s_gameMonMessageMutex));
    if (!mutex) {
        mutex = OpenMutexA(SYNCHRONIZE, FALSE, _Dect1(s_gameMonMessageMutexAlt));
        if (!mutex) {
            UnmapViewOfFile(view);
            CloseHandle(mapping);
            return FALSE;
        }
    }

    WaitForSingleObject(mutex, INFINITE);
    char* end = view;
    unsigned int length = *(unsigned int*)end;
    while (length) {
        end += length;
        if ((unsigned int)(end - view) >= 0xffc) {
            end = view;
            break;
        }
        length = *(unsigned int*)end;
    }

    if ((unsigned int)(end - view) >= 0xffc - size) {
        end = view;
    } else {
        memcpy(end, message, size);
        end += size;
    }
    *(unsigned int*)end = 0;

    UnmapViewOfFile(view);
    CloseHandle(mapping);
    ReleaseMutex(mutex);
    CloseHandle(mutex);
    return TRUE;
}

class CNPDword {
public:
    CNPDword();
    ~CNPDword();

    unsigned int Get();
    void Set(unsigned int value);

private:
    void Guard();
    void Release();

    unsigned int* m_value;
};

CNPDword::CNPDword() {
    m_value = (unsigned int*)VirtualAlloc(0, sizeof(unsigned int),
                                           MEM_COMMIT, PAGE_READWRITE);
}

CNPDword::~CNPDword() {
    VirtualFree(m_value, sizeof(unsigned int), MEM_DECOMMIT);
}

void CNPDword::Guard() {
    unsigned int oldProtect;
    VirtualProtect(m_value, sizeof(unsigned int), PAGE_READONLY, &oldProtect);
}

void CNPDword::Release() {
    unsigned int oldProtect;
    VirtualProtect(m_value, sizeof(unsigned int), PAGE_READWRITE, &oldProtect);
}

void CNPDword::Set(unsigned int value) {
    Release();
    *m_value = value;
    Guard();
}

unsigned int CNPDword::Get() {
    return *m_value;
}
