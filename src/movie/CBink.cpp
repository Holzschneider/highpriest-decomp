#include "common.h"





#include <windows.h>
#include <ddraw.h>
#include "CBink.h"
#if defined(BF_PORT)
// The SDK's names on the port's Bink reader (port/compat/bink, M12.4a).
#include "bf_bink_sdk.h"
#include "../base/File.h"
#endif

#include "../3dlib/Device.h"
#include "../3dlib/D3dutil.h"
#include "../3dlib/Renderer.h"


extern const char* g_regPath;



namespace {
struct BinkSleepState {
    int total_sleep;
    int slept_in_advance;
};
}
extern BinkSleepState g_binkSleep;
extern unsigned __int64 g_binkFrequency;
extern int g_binkGotFrequency;



CBink::CBink()
{
    m_Bink = 0;
    m_BinkBuffer = 0;
    m_IsBinkPlaying = 0;
}

int CBink::BinkShowNextFrame(BINK* bink, HWND__* window)
{
    BinkDoFrame(bink);

    LPDIRECTDRAWSURFACE7 lpSurfaceDest = g_3dDevice.GetFrontBuffer();
    LPDIRECTDRAWSURFACE7 lpSurfaceSrc  = g_3dDevice.GetBackBuffer();
    DDSURFACEDESC2       ddsd         = { sizeof(DDSURFACEDESC2), 0, };
    int                  surface_type;
    RECT                 src          = { 0, 0, 0x200, 0x15b };
    RECT                 dest;

    surface_type = BinkDDSurfaceType(lpSurfaceSrc);
#if defined(BF_PORT)
    // Bink only writes: without WRITEONLY the adapter would read the whole
    // target back first (port/compat/d3d, "Lazy render-target lock").
    if (FAILED(lpSurfaceSrc->Lock(NULL, &ddsd, DDLOCK_WAIT | DDLOCK_WRITEONLY, NULL))) {
#else
    if (FAILED(lpSurfaceSrc->Lock(NULL, &ddsd, DDLOCK_WAIT, NULL))) {
#endif
        return 0;
    }
    BinkCopyToBuffer(bink, ddsd.lpSurface, ddsd.lPitch, ddsd.dwHeight, 0, 0, surface_type);
    lpSurfaceSrc->Unlock(NULL);

    GetClientRect(g_hMainWnd, &dest);
    ClientToScreen(g_hMainWnd, (POINT*)&dest.left);
    ClientToScreen(g_hMainWnd, (POINT*)&dest.right);

    lpSurfaceDest->Blt(&dest, lpSurfaceSrc, &src, DDBLT_WAIT, NULL);

    if (bink->FrameNum == bink->Frames) {
        return 0;
    }
    BinkNextFrame(bink);
    return 1;
}



void CBink::BinkGoodSleepUs(int microseconds)
{
    static int total_sleep      = 0;
    static int slept_in_advance = 0;
    static unsigned __int64 frequency = 1000;
    static int  got_frequency    = 0;

    if (!got_frequency) {
        got_frequency = 1;
        QueryPerformanceFrequency((LARGE_INTEGER*)&frequency);
    }

    total_sleep += microseconds;

    if ((total_sleep - slept_in_advance) > 1000) {
        unsigned __int64 start, end;
        total_sleep -= slept_in_advance;

        QueryPerformanceCounter((LARGE_INTEGER*)&start);
        Sleep(total_sleep / 1000);
        QueryPerformanceCounter((LARGE_INTEGER*)&end);

        end = ((end - start) * (unsigned __int64)1000000) / frequency;

        slept_in_advance = (unsigned int)end - total_sleep;
        total_sleep %= 1000;
    }
}

int CBink::InitOpenning()
{
    BinkSoundUseDirectSound(0);
#if defined(BF_PORT)
    // BF_INTRO_MOVIE names another movie in place of openning.bik, which
    // none of the port's data has: an absolute path, or a name the file
    // manager finds in the archives or the game folder (data\video\ad\01.bik).
    const char* other = getenv("BF_INTRO_MOVIE");
    if (other && *other) {
        unsigned int size = 0;
        void* data = other[0] == '/' ? NULL : g_fileMgr.GetData(other, &size);
        m_Bink = data ? BfBinkOpenMemory(data, size, other) : BinkOpen(other, 0);
        if (data) g_fileMgr.ReleaseData(data);
        return m_Bink != 0;
    }
#endif
    m_Bink = BinkOpen("openning.bik", 0);
    return m_Bink != 0;
}

int CBink::CloseOpenning()
{
    if (m_Bink) {
        BinkClose(m_Bink);
        m_Bink = 0;
    }
    return 1;
}

int CBink::Break(unsigned short param)
{
    if (m_IsBinkPlaying) {
        if (param == 'g') BinkGoto(m_Bink, 1500, BINKGOTOQUICK);
        else              m_IsBinkPlaying = 0;

        if (param == 'R') m_BinkMode = 0;
        else              m_BinkMode = 1;
        return 0;
    }
    return 1;
}

void CBink::Pause(int flag)
{
#if defined(BF_PORT)
    // Spec section 3: in the background the client keeps running at full
    // rate; the movie too.
#else
    BinkPause(m_Bink, flag);
#endif
}

int CBink::ReadBinkModeRegistry()
{
    LONG  lResult;
    HKEY  hKey;
    DWORD reg_Type;
    DWORD Datalength;

    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, g_regPath, 0, KEY_QUERY_VALUE, &hKey);
    if (lResult == ERROR_SUCCESS) {
        reg_Type   = REG_DWORD;
        Datalength = sizeof(DWORD);
        RegQueryValueEx(hKey, "BINKMODE", NULL, &reg_Type, (LPBYTE)&m_BinkMode, &Datalength);
        RegCloseKey(hKey);
    }
    return 1;
}

int CBink::WriteBinkModeRegistry()
{
    HKEY  hKey;
    LONG  lResult;
    DWORD dwDisposition = 0;

    lResult = RegCreateKeyEx(HKEY_LOCAL_MACHINE, g_regPath, 0, NULL,
                             REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                             &hKey, &dwDisposition);
    if (lResult == ERROR_SUCCESS) {
        RegSetValueEx(hKey, "BINKMODE", NULL, REG_DWORD, (LPBYTE)&m_BinkMode, sizeof(DWORD));
        RegCloseKey(hKey);
    }
    return 1;
}

void CBink::OpenningMain(int flag)
{
    MSG msg;

    ReadBinkModeRegistry();
#if defined(BF_PORT)
    // At the start (flag 1) the option SkipIntroScreens (spec section 2, on
    // by default) skips the movie; the button of the login window plays it.
    if (flag == 1 && bf_port_option(BF_OPT_SKIP_INTRO)) {
        BF_TRACE(BFT_UI, "intro: skipped (option %s)", BF_OPT_SKIP_INTRO);
        return;
    }
    if (m_BinkMode == 1 && flag == 1) {
        BF_TRACE(BFT_UI, "intro: skipped (BINKMODE 1: a key stopped it before)");
        return;
    }
    if (!InitOpenning()) {
        BF_TRACE(BFT_UI, "intro: no movie (%s)", BinkGetError());
        return;
    }
    BF_TRACE(BFT_UI, "intro: start, %u frames", m_Bink->Frames);
#else
    if (m_BinkMode == 1 && flag == 1) return;
    if (!InitOpenning()) return;
#endif
    m_IsBinkPlaying = true;

    for (;;) {
#if defined(BF_PORT)
        // The control socket, the window's size and captures (WinMain's
        // CheckSystemMessage does this once per frame of the modes).
        bf_port_frame();
        if (g_sysQuit) break;
#endif
        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            if (!BinkWait(m_Bink)) {
                if (!BinkShowNextFrame(m_Bink, g_hMainWnd) || m_IsBinkPlaying == false) break;
            } else {
                BinkGoodSleepUs(500);
            }
        }
    }
    m_IsBinkPlaying = false;
#if defined(BF_PORT)
    BF_TRACE(BFT_UI, "intro: end at frame %u of %u (%s)", m_Bink->LastFrameNum, m_Bink->Frames,
             m_Bink->LastFrameNum >= m_Bink->Frames ? "ended" : "stopped");
#endif
    CloseOpenning();
    WriteBinkModeRegistry();
}

