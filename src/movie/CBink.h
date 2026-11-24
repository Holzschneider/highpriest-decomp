#ifndef HIGHPRIEST_MOVIE_CBINK_H
#define HIGHPRIEST_MOVIE_CBINK_H

#ifndef BF_PORT
#include "RAD.H"
#include "BINK.H"
#endif

struct BINK;
struct BINKBUFFER;
class CBink;
struct HWND__;

class CBink {
protected:
    int BinkShowNextFrame(BINK* bink, HWND__* window);
    void BinkGoodSleepUs(int microseconds);
    int InitOpenning();
    int CloseOpenning();
    int ReadBinkModeRegistry();
    int WriteBinkModeRegistry();
public:
    CBink();
    int Break(unsigned short param);
    void Pause(int flag);
    unsigned char IsBinkPlaying() { return m_IsBinkPlaying; }
    void OpenningMain(int flag);
protected:
    BINK* m_Bink;
    BINKBUFFER* m_BinkBuffer;
    unsigned char m_IsBinkPlaying;
    int m_BinkMode;
};

#endif
