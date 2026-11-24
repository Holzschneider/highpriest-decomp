#ifndef HIGHPRIEST_COMMPIPE_H
#define HIGHPRIEST_COMMPIPE_H

#include <windows.h>

unsigned int __stdcall CommPipeThread(void* param);

class CCommPipe {
public:
    CCommPipe();
    ~CCommPipe();

    bool InternalSendCommPipe(unsigned int msg, unsigned int code, unsigned int value);
    bool SendCommPipe(unsigned int msg, unsigned int code, unsigned int value);
    bool InitCommPipe();
    unsigned int ConnectCommPipe();
    unsigned int ConnectCommPipe(void* readHandle, void* writeHandle);
    unsigned int RunCommPipe();
    bool CloseCommPipe();
    void* GetChildReadHandle();
    void* GetChildWriteHandle();
    const char* GetUserInfo();
    unsigned int* GetCSAuth2();

private:
    friend class CGameGuard;
    friend unsigned int __stdcall CommPipeThread(void* param);
    unsigned int CreateEventAndThread();

    // Layout recovered from the original CommPipe.obj constructor, accessors and thread.
    HANDLE m_thread;
    HANDLE m_writePipe;
    HANDLE m_readPipe;
    unsigned int m_seed;
    HANDLE m_closeEvent;
    bool m_waitingForHandshake;
    unsigned int m_csAuth2[4];
    unsigned int m_reserved;
    char m_userInfo[MAX_PATH];
    HANDLE m_originalInput;
    HANDLE m_originalOutput;
    HANDLE m_childInputRead;
    HANDLE m_childInputWrite;
    HANDLE m_childOutputRead;
    HANDLE m_childOutputWrite;
    unsigned int m_lastMessage;
    unsigned int m_lastCode;
    unsigned int m_lastValue;
    unsigned int m_repeatedMessages;
};

#endif
