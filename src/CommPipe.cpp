// Reconstructed from original GameGuard evidence.

#include "CommPipe.h"
#include <string.h>

bool __stdcall CommPipeCallback(CCommPipe* pipe, unsigned int code, unsigned int value);

void* CCommPipe::GetChildReadHandle() {
    return m_childInputRead;
}

void* CCommPipe::GetChildWriteHandle() {
    return m_childOutputWrite;
}

const char* CCommPipe::GetUserInfo() {
    return m_userInfo;
}

unsigned int* CCommPipe::GetCSAuth2() {
    return m_csAuth2;
}

bool CCommPipe::InternalSendCommPipe(unsigned int msg, unsigned int code, unsigned int value) {
    HANDLE pipe = m_writePipe;
    if (pipe == INVALID_HANDLE_VALUE) {
        return false;
    }

    struct Header {
        unsigned int msg;
        unsigned int seed;
        unsigned int code;
        unsigned int value;
    } header;
    DWORD written;
    BOOL ok;
    header.msg = msg;
    header.code = code;
    header.seed = m_seed;
    header.value = value;

    if (code == 0x613) {
        struct {
            Header header;
            char data[0x104];
        } packet;

        header.value = lstrlenA((const char*)value) + 1;
        if (header.value > 0x104) {
            header.value = 0x104;
            ((char*)value)[0x103] = 0;
        }

        packet.header = header;
        memcpy(packet.data, (const void*)value, header.value);
        ok = WriteFile(m_writePipe, &packet, header.value + 0x10, &written, 0);
    } else if (code == 0x619) {
        struct {
            Header header;
            unsigned int data[4];
        } packet;

        header.value = 0x10;
        packet.header = header;
        packet.data[0] = *(unsigned int*)value;
        packet.data[1] = *(unsigned int*)(value + 4);
        packet.data[2] = *(unsigned int*)(value + 8);
        packet.data[3] = *(unsigned int*)(value + 0x0c);
        ok = WriteFile(m_writePipe, &packet, 0x20, &written, 0);
    } else {
        ok = WriteFile(m_writePipe, &header, 0x10, &written, 0);
    }

    if (!ok) {
        DWORD error = GetLastError();
        if (error == ERROR_BROKEN_PIPE || error == ERROR_NO_DATA ||
            error == ERROR_INVALID_HANDLE) {
            return false;
        }
    }

    return true;
}

bool CCommPipe::SendCommPipe(unsigned int msg, unsigned int code, unsigned int value) {
    if (m_lastCode != 0x619) {
        if (m_lastMessage == msg &&
            m_lastCode == code &&
            m_lastValue == value) {
            unsigned int repeat = ++m_repeatedMessages;
            if (repeat > 2) {
                return true;
            }
        } else {
            m_repeatedMessages = 0;
        }

        m_lastMessage = msg;
        m_lastCode = code;
        m_lastValue = value;
    }

    return InternalSendCommPipe(msg, code, value);
}

CCommPipe::CCommPipe() {
    SYSTEMTIME time;
    GetLocalTime(&time);

    m_seed = (time.wYear * time.wMonth * time.wDay) ^ 0xdad53a;

    m_writePipe = INVALID_HANDLE_VALUE;
    m_readPipe = INVALID_HANDLE_VALUE;
    m_thread = 0;
    m_closeEvent = 0;
    m_waitingForHandshake = 0;
    m_reserved = 0;
    m_userInfo[0] = 0;
}

CCommPipe::~CCommPipe() {
    CloseCommPipe();
}

bool CCommPipe::InitCommPipe() {
    if (m_writePipe != INVALID_HANDLE_VALUE) {
        return false;
    }

    SECURITY_ATTRIBUTES security;
    security.nLength = sizeof(security);
    security.bInheritHandle = TRUE;
    security.lpSecurityDescriptor = 0;

    HANDLE originalOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    m_originalOutput = originalOutput;

    if (!CreatePipe(&m_childOutputRead,
                    &m_childOutputWrite, &security, 0)) {
        return false;
    }

    if (!SetStdHandle(STD_OUTPUT_HANDLE,
                      m_childOutputWrite)) {
        return false;
    }

    if (!DuplicateHandle(GetCurrentProcess(), m_childOutputRead,
                         GetCurrentProcess(), &m_readPipe,
                         0, FALSE, DUPLICATE_SAME_ACCESS)) {
        return false;
    }
    CloseHandle(m_childOutputRead);

    HANDLE originalInput = GetStdHandle(STD_INPUT_HANDLE);
    m_originalInput = originalInput;

    if (!CreatePipe(&m_childInputRead,
                    &m_childInputWrite, &security, 0)) {
        return false;
    }

    if (!SetStdHandle(STD_INPUT_HANDLE,
                      m_childInputRead)) {
        return false;
    }

    if (!DuplicateHandle(GetCurrentProcess(), m_childInputWrite,
                         GetCurrentProcess(), &m_writePipe,
                         0, FALSE, DUPLICATE_SAME_ACCESS)) {
        return false;
    }
    CloseHandle(m_childInputWrite);
    return true;
}

unsigned int CCommPipe::ConnectCommPipe() {
    if (m_writePipe == INVALID_HANDLE_VALUE) {
        m_writePipe = GetStdHandle(STD_OUTPUT_HANDLE);
        m_readPipe = GetStdHandle(STD_INPUT_HANDLE);
        if (m_writePipe != INVALID_HANDLE_VALUE &&
            m_readPipe != INVALID_HANDLE_VALUE) {
            return CreateEventAndThread();
        }
        GetLastError();
    }
    return 0;
}

unsigned int CCommPipe::ConnectCommPipe(void* readHandle, void* writeHandle) {
    m_writePipe = writeHandle;
    m_readPipe = readHandle;
    if (writeHandle != INVALID_HANDLE_VALUE && readHandle != INVALID_HANDLE_VALUE) {
        return CreateEventAndThread();
    }
    GetLastError();
    return 0;
}

unsigned int CCommPipe::CreateEventAndThread() {
    DWORD threadId;
    m_closeEvent = CreateEventA(0, TRUE, FALSE, 0);
    if (!m_closeEvent) {
        return 0;
    }

    m_thread = CreateThread(0, 0, CommPipeThread, this, 0, &threadId);
    if (!m_thread) {
        return 0;
    }

    m_waitingForHandshake = 1;
    return threadId;
}

unsigned int CCommPipe::RunCommPipe() {
    DWORD threadId;
    if (m_writePipe == INVALID_HANDLE_VALUE) {
        return 0;
    }

    if (!SetStdHandle(STD_INPUT_HANDLE, m_originalInput)) {
        return 0;
    }
    if (!SetStdHandle(STD_OUTPUT_HANDLE, m_originalOutput)) {
        return 0;
    }

    m_closeEvent = CreateEventA(0, TRUE, FALSE, 0);
    if (!m_closeEvent) {
        return 0;
    }

    m_thread = CreateThread(0, 0, CommPipeThread, this, 0, &threadId);
    return m_thread ? threadId : 0;
}

bool CCommPipe::CloseCommPipe() {
    if (m_writePipe == INVALID_HANDLE_VALUE) {
        return false;
    }

    if (m_closeEvent) {
        SetEvent(m_closeEvent);
    }

    if (m_thread) {
        if (WaitForSingleObject(m_thread, 100) == WAIT_TIMEOUT) {
            TerminateThread(m_thread, 0);
        }
        CloseHandle(m_thread);
        m_thread = 0;
    }

    if (m_writePipe != INVALID_HANDLE_VALUE) {
        CloseHandle(m_writePipe);
        m_writePipe = INVALID_HANDLE_VALUE;
    }

    if (m_readPipe != INVALID_HANDLE_VALUE) {
        CloseHandle(m_readPipe);
        m_readPipe = INVALID_HANDLE_VALUE;
    }

    return true;
}

unsigned int __stdcall CommPipeThread(void* param)
{
    CCommPipe* pCommPipe = (CCommPipe*)param;
    unsigned int dwAvailable = 0;
    unsigned int bytesRead;
    int connectionTicks = 0;
    int nIdleTicks = 0;
    void* faultPC;
    struct {
        unsigned int message;
        unsigned int seed;
        unsigned int code;
        unsigned int value;
    } packet;

    __try {
        for (;;) {
            if (dwAvailable <= 0) {
                if (!PeekNamedPipe(pCommPipe->m_readPipe, 0, 0, 0, &dwAvailable, 0)) {
                    if (!CommPipeCallback(pCommPipe, 0x60f, 0))
                        break;
                }
                if (dwAvailable == 0) {
                    if (WaitForSingleObject(pCommPipe->m_closeEvent, 50) == WAIT_OBJECT_0)
                        break;

                    ++nIdleTicks;
                    if (nIdleTicks >= 200) {
                        if (!pCommPipe->InternalSendCommPipe(0x8b1, 0x60e, 0)) {
                            if (!CommPipeCallback(pCommPipe, 0x60f, 0))
                                break;
                        }
                        if (nIdleTicks >= 800 && !CommPipeCallback(pCommPipe, 0x618, nIdleTicks))
                            break;
                    }

                    if (pCommPipe->m_waitingForHandshake) {
                        ++connectionTicks;
                        if (connectionTicks >= 80) {
                            connectionTicks = 0;
                            pCommPipe->m_waitingForHandshake = false;
                            if (!pCommPipe->InternalSendCommPipe(0x8b0, 0, 0)) {
                                if (!CommPipeCallback(pCommPipe, 0x60f, 0))
                                    break;
                            }
                        }
                    }
                    continue;
                }
            }

            nIdleTicks = 0;
            if (!ReadFile(pCommPipe->m_readPipe, &packet, sizeof(packet), &bytesRead, 0) || bytesRead == 0) {
                if (!CommPipeCallback(pCommPipe, 0x60f, 0))
                    break;
            }
            dwAvailable -= bytesRead;
            if (packet.code == 0x60e)
                continue;

            if (packet.message == 0x8b1) {
                if (packet.code == 0x613) {
                    if (packet.value > sizeof(pCommPipe->m_userInfo))
                        break;
                    if (!ReadFile(pCommPipe->m_readPipe, pCommPipe->m_userInfo, packet.value, &bytesRead, 0) || bytesRead == 0) {
                        if (!CommPipeCallback(pCommPipe, 0x60f, 0))
                            break;
                    }
                    dwAvailable -= bytesRead;
                    continue;
                }
                if (packet.code == 0x619) {
                    if (packet.value > sizeof(pCommPipe->m_csAuth2))
                        break;
                    if (!ReadFile(pCommPipe->m_readPipe, pCommPipe->m_csAuth2, packet.value, &bytesRead, 0) || bytesRead == 0) {
                        if (!CommPipeCallback(pCommPipe, 0x60f, 0))
                            break;
                    }
                    dwAvailable -= bytesRead;
                    if (!CommPipeCallback(pCommPipe, packet.code, packet.value))
                        break;
                    continue;
                }
            }

            if (packet.message == 0x8b2) {
                if (packet.code == 0) {
                    pCommPipe->m_waitingForHandshake = false;
                    ++pCommPipe->m_seed;
                } else if (packet.code == 0x610) {
                    if (!CommPipeCallback(pCommPipe, packet.code, 0))
                        break;
                }
                continue;
            }

            if (packet.message == 0x8b0) {
                if (pCommPipe->m_seed != packet.seed) {
                    if (!CommPipeCallback(pCommPipe, 0x60f, 0))
                        break;
                } else if (!pCommPipe->InternalSendCommPipe(0x8b2, packet.code, 0)) {
                    if (!CommPipeCallback(pCommPipe, 0x60f, 0))
                        break;
                }
                pCommPipe->m_waitingForHandshake = true;
                ++pCommPipe->m_seed;
                continue;
            }

            if (packet.message == 0x8b1) {
                if (packet.code == 0x610 && !CommPipeCallback(pCommPipe, packet.code, 0))
                    break;
                if (!pCommPipe->InternalSendCommPipe(0x8b2, packet.code, packet.value)) {
                    if (!CommPipeCallback(pCommPipe, 0x60f, 0))
                        break;
                    continue;
                }
                if (!CommPipeCallback(pCommPipe, packet.code, packet.value))
                    break;
            }
        }

        if (pCommPipe->m_readPipe != INVALID_HANDLE_VALUE) {
            CloseHandle(pCommPipe->m_readPipe);
            pCommPipe->m_readPipe = INVALID_HANDLE_VALUE;
        }
        if (WaitForSingleObject(pCommPipe->m_closeEvent, 30000) == WAIT_OBJECT_0 && packet.code != 0x610 && packet.code != 0x60f)
            pCommPipe->InternalSendCommPipe(0x8b1, 0x610, 0);
    } __except ((LONG)(faultPC = (GetExceptionInformation())->ExceptionRecord->ExceptionAddress)) {
    }
    return 0;
}
