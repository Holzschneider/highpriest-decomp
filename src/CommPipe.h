#ifndef HIGHPRIEST_COMMPIPE_H
#define HIGHPRIEST_COMMPIPE_H

#include <windows.h>

unsigned long __stdcall CommPipeThread(void* param);

class CCommPipe {
public:
    CCommPipe();
    ~CCommPipe();

    bool InternalSendCommPipe(unsigned long msg, unsigned long code, unsigned long value);
    bool SendCommPipe(unsigned long msg, unsigned long code, unsigned long value);
    bool InitCommPipe();
    unsigned long ConnectCommPipe();
    unsigned long ConnectCommPipe(void* readHandle, void* writeHandle);
    unsigned long RunCommPipe();
    bool CloseCommPipe();
    void* GetChildReadHandle();
    void* GetChildWriteHandle();
    const char* GetUserInfo();
    unsigned long* GetCSAuth2();

private:
    friend class CGameGuard;
    friend unsigned long __stdcall CommPipeThread(void* param);
    unsigned long CreateEventAndThread();

    // Layout recovered from the original CommPipe.obj constructor, accessors and thread.
    HANDLE m_thread;
    HANDLE m_writePipe;
    HANDLE m_readPipe;
    unsigned long m_seed;
    HANDLE m_closeEvent;
    bool m_waitingForHandshake;
    unsigned long m_csAuth2[4];
    unsigned long m_reserved;
    char m_userInfo[MAX_PATH];
    HANDLE m_originalInput;
    HANDLE m_originalOutput;
    HANDLE m_childInputRead;
    HANDLE m_childInputWrite;
    HANDLE m_childOutputRead;
    HANDLE m_childOutputWrite;
    unsigned long m_lastMessage;
    unsigned long m_lastCode;
    unsigned long m_lastValue;
    unsigned long m_repeatedMessages;
};

#endif
