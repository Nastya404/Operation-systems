#pragma once
#include "Employee.h"
#include <windows.h>

#define PIPE_NAME TEXT("\\\\.\\pipe\\EmployeePipe")

enum RequestType {
    REQ_READ = 0,
    REQ_READ_DONE = 1,
    REQ_MODIFY_START = 2,
    REQ_MODIFY_COMMIT = 3,
    REQ_EXIT = 4
};

#define RESP_MSG_MAX 64

struct Request {
    RequestType type;
    int         emp_id;
    Employee    data;
};

struct Response {
    BOOL     success;
    Employee data;
    char     message[RESP_MSG_MAX];
};


static inline BOOL PipeSendRaw(HANDLE hPipe, const void* data, DWORD size)
{
    DWORD written = 0;
    if (!WriteFile(hPipe, &size, sizeof(DWORD), &written, NULL)) return FALSE;
    if (written != sizeof(DWORD)) return FALSE;
    if (size == 0) return TRUE;
    if (!WriteFile(hPipe, data, size, &written, NULL)) return FALSE;
    return written == size;
}

static inline BOOL PipeRecvRaw(HANDLE hPipe, void* data, DWORD expectedSize)
{
    DWORD frameSize = 0, nRead = 0;
    if (!ReadFile(hPipe, &frameSize, sizeof(DWORD), &nRead, NULL)) return FALSE;
    if (nRead != sizeof(DWORD) || frameSize != expectedSize)        return FALSE;
    if (frameSize == 0) return TRUE;

    DWORD total = 0;
    while (total < frameSize) {
        DWORD chunk = 0;
        if (!ReadFile(hPipe, static_cast<char*>(data) + total,
            frameSize - total, &chunk, NULL)) return FALSE;
        if (chunk == 0) break;
        total += chunk;
    }
    return total == frameSize;
}

static inline BOOL SendRequest(HANDLE h, const Request& r) { return PipeSendRaw(h, &r, sizeof(r)); }
static inline BOOL ReadRequest(HANDLE h, Request& r) { return PipeRecvRaw(h, &r, sizeof(r)); }
static inline BOOL SendResponse(HANDLE h, const Response& r) { return PipeSendRaw(h, &r, sizeof(r)); }
static inline BOOL ReadResponse(HANDLE h, Response& r) { return PipeRecvRaw(h, &r, sizeof(r)); }