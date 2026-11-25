#ifndef HIGHPRIEST_MOVIE_CBINK_H
#define HIGHPRIEST_MOVIE_CBINK_H

struct bf_movie;
class CBink;
struct HWND__;

class CBink {
protected:
    int BinkShowNextFrame(bf_movie* bink, HWND__* window);
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
    bf_movie* m_Bink;
    void* m_BinkBuffer;
    unsigned char m_IsBinkPlaying;
    int m_BinkMode;
};

#endif
