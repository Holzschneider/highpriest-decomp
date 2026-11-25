#include "common.h"
#include "bf_setting_key.h"





#include "bf_types.h"
#include "CBink.h"
#if defined(BF_PORT)
// The movie player of the port (port/client, bf_movie.h) in place of the
// Bink SDK: the reader, the sound module and the clock (M12.4a, M11.10).
#include "bf_movie.h"
#include "bf_input.h"
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

int CBink::BinkShowNextFrame(bf_movie* bink, HWND__* window)
{
    bf_movie_decode(bink);

    RECT                 src          = { 0, 0, 0x200, 0x15b };
    int                  pitch, width, height;
    void*                bits;

    // The back buffer is the render target (bf_render.h, M11.9), 32-bit
    // X8R8G8B8 (the Bink SDK's BinkDDSurfaceType of it: BGRA8888). Bink only
    // writes: a write-only lock reads nothing back.
    if (!g_3dDevice.IsInitialized() || (bits = bf_render_lock(1, &pitch)) == NULL) {
        return 0;
    }
    bf_render_size(&width, &height);
    bf_movie_copy(bink, bits, pitch, height, 0, 0, BF_MOVIE_BGRA8888);
    bf_render_unlock();

    // The movie's rectangle into the window's client area: DirectDraw
    // stretched it there; the window shows the whole target, so it is
    // stretched over the target and presented.
    bf_render_present_part(src.left, src.top, src.right - src.left, src.bottom - src.top);

    if (bf_movie_frame(bink) == bf_movie_frames(bink)) {
        return 0;
    }
    bf_movie_next(bink);
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
        frequency = 1000000;   // bf_clock_us counts microseconds
    }

    total_sleep += microseconds;

    if ((total_sleep - slept_in_advance) > 1000) {
        unsigned __int64 start, end;
        total_sleep -= slept_in_advance;

        start = bf_clock_us();
        bf_clock_sleep_ms(total_sleep / 1000);
        end = bf_clock_us();

        end = ((end - start) * (unsigned __int64)1000000) / frequency;

        slept_in_advance = (unsigned int)end - total_sleep;
        total_sleep %= 1000;
    }
}

int CBink::InitOpenning()
{
    bf_movie_use_sound(1);   // the SDK's DirectSound: the movie's sound plays
#if defined(BF_PORT)
    // BF_INTRO_MOVIE names another movie in place of openning.bik, which
    // none of the port's data has: an absolute path, or a name the file
    // manager finds in the archives or the game folder (data\video\ad\01.bik).
    const char* other = getenv("BF_INTRO_MOVIE");
    if (other && *other) {
        unsigned int size = 0;
        void* data = other[0] == '/' ? NULL : g_fileMgr.GetData(other, &size);
        m_Bink = data ? bf_movie_open_memory(data, size, other) : bf_movie_open(other);
        if (data) g_fileMgr.ReleaseData(data);
        return m_Bink != 0;
    }
#endif
    m_Bink = bf_movie_open("openning.bik");
    return m_Bink != 0;
}

int CBink::CloseOpenning()
{
    if (m_Bink) {
        bf_movie_close(m_Bink);
        m_Bink = 0;
    }
    return 1;
}

int CBink::Break(unsigned short param)
{
    if (m_IsBinkPlaying) {
        if (param == 'g') bf_movie_goto(m_Bink, 1500);
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
    bf_movie_pause(m_Bink, flag);
#endif
}

int CBink::ReadBinkModeRegistry()
{
    bool  lResult;
    bf_setting_key  hKey;

    lResult = bf_setting_open(hKey, g_regPath);
    if (lResult) {
        bf_setting_get(hKey, "BINKMODE", m_BinkMode);
        bf_setting_close(hKey);
    }
    return 1;
}

int CBink::WriteBinkModeRegistry()
{
    bf_setting_key  hKey;
    bool  lResult;

    lResult = bf_setting_create(hKey, g_regPath);
    if (lResult) {
        bf_setting_set(hKey, "BINKMODE", m_BinkMode);
        bf_setting_close(hKey);
    }
    return 1;
}

void CBink::OpenningMain(int flag)
{
#if defined(BF_PORT)
    bf_input_msg msg;
#else
    MSG msg;
#endif

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
        BF_TRACE(BFT_UI, "intro: no movie (%s)", bf_movie_error());
        return;
    }
    BF_TRACE(BFT_UI, "intro: start, %u frames", bf_movie_frames(m_Bink));
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
#if defined(BF_PORT)
        if (bf_input_peek(&msg)) {
            if (msg.message == WM_QUIT) break;
            bf_input_dispatch(&msg);
        } else {
#else
        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
#endif
            if (!bf_movie_wait(m_Bink)) {
                if (!BinkShowNextFrame(m_Bink, g_hMainWnd) || m_IsBinkPlaying == false) break;
            } else {
                BinkGoodSleepUs(500);
            }
        }
    }
    m_IsBinkPlaying = false;
#if defined(BF_PORT)
    BF_TRACE(BFT_UI, "intro: end at frame %u of %u (%s)", bf_movie_last_frame(m_Bink), bf_movie_frames(m_Bink),
             bf_movie_last_frame(m_Bink) >= bf_movie_frames(m_Bink) ? "ended" : "stopped");
#endif
    CloseOpenning();
    WriteBinkModeRegistry();
}

