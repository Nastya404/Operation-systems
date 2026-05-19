#include "Request.h"
#include "FileHandler.h"
#include "LockManager.h"
#include <windows.h>
#include <iostream>
#include <cstring>
#include <cstdio>

struct ClientContext {
    HANDLE       hPipe;
    FileHandler* fileHandler;
    LockManager* lockManager;
};

static DWORD WINAPI ClientThread(LPVOID param)
{
    ClientContext* ctx = static_cast<ClientContext*>(param);
    HANDLE pipe = ctx->hPipe;
    FileHandler& fh = *ctx->fileHandler;
    LockManager& lm = *ctx->lockManager;
    delete ctx;

    int pendingReadLock = -1;
    int pendingWriteLock = -1;

    while (TRUE) {
        Request req;
        memset(&req, 0, sizeof(req));
        if (!ReadRequest(pipe, req)) {
            std::cout << "[Server] Client disconnected (pipe broken)\n";
            break;
        }

        Response resp;
        memset(&resp, 0, sizeof(resp));
        resp.success = FALSE;

        if (req.type == REQ_READ) {
            std::cout << "[Server] READ  id=" << req.emp_id << "\n";

            lm.AcquireRead(req.emp_id);
            if (fh.GetEmployee(req.emp_id, resp.data)) {
                pendingReadLock = req.emp_id;
                resp.success = TRUE;
                strncpy_s(resp.message, sizeof(resp.message), "OK", _TRUNCATE);
            }
            else {
                lm.ReleaseRead(req.emp_id);
                strncpy_s(resp.message, sizeof(resp.message),
                    "Employee not found", _TRUNCATE);
            }
        }
        else if (req.type == REQ_READ_DONE) {
            std::cout << "[Server] READ_DONE  id=" << req.emp_id << "\n";
            if (pendingReadLock != -1) {
                lm.ReleaseRead(pendingReadLock);
                pendingReadLock = -1;
            }
            resp.success = TRUE;
            strncpy_s(resp.message, sizeof(resp.message), "OK", _TRUNCATE);
        }
        else if (req.type == REQ_MODIFY_START) {
            std::cout << "[Server] MODIFY_START  id=" << req.emp_id << "\n";
            lm.AcquireWrite(req.emp_id);

            if (fh.GetEmployee(req.emp_id, resp.data)) {
                pendingWriteLock = req.emp_id;
                resp.success = TRUE;
                strncpy_s(resp.message, sizeof(resp.message),
                    "Record locked for editing", _TRUNCATE);
            }
            else {
                lm.ReleaseWrite(req.emp_id);
                strncpy_s(resp.message, sizeof(resp.message),
                    "Employee not found", _TRUNCATE);
            }
        }
        else if (req.type == REQ_MODIFY_COMMIT) {
            std::cout << "[Server] MODIFY_COMMIT  id=" << req.data.num << "\n";

            BOOL ok = fh.UpdateEmployee(req.data) ? TRUE : FALSE;
            if (pendingWriteLock != -1) {
                lm.ReleaseWrite(pendingWriteLock);
                pendingWriteLock = -1;
            }
            resp.success = ok;
            strncpy_s(resp.message, sizeof(resp.message),
                ok ? "Record updated" : "Update failed", _TRUNCATE);
        }
        else if (req.type == REQ_EXIT) {
            std::cout << "[Server] Client disconnecting\n";
            if (pendingReadLock != -1) lm.ReleaseRead(pendingReadLock);
            if (pendingWriteLock != -1) lm.ReleaseWrite(pendingWriteLock);
            resp.success = TRUE;
            strncpy_s(resp.message, sizeof(resp.message), "Goodbye", _TRUNCATE);
            SendResponse(pipe, resp);
            break;
        }

        if (!SendResponse(pipe, resp)) break;
    }

    if (pendingReadLock != -1) lm.ReleaseRead(pendingReadLock);
    if (pendingWriteLock != -1) lm.ReleaseWrite(pendingWriteLock);

    FlushFileBuffers(pipe);
    DisconnectNamedPipe(pipe);
    CloseHandle(pipe);
    return 0;
}


static BOOL ParseInt(const char* str, int* out)
{
    if (!str || str[0] == '\0') return FALSE;
    char* end = NULL;
    long  v = strtol(str, &end, 10);
    if (end == str || *end != '\0') return FALSE;
    *out = static_cast<int>(v);
    return TRUE;
}

static BOOL ParseDouble(const char* str, double* out)
{
    if (!str || str[0] == '\0') return FALSE;
    char* end = NULL;
    double v = strtod(str, &end);
    if (end == str || *end != '\0') return FALSE;
    *out = v;
    return TRUE;
}

static int ReadInt(const char* prompt, int minVal, int maxVal,
    BOOL checkDuplicates, const int* usedIDs, int usedCount)
{
    char buf[64];
    while (TRUE) {
        std::cout << prompt;
        std::cin.getline(buf, sizeof(buf));
        int v;
        if (!ParseInt(buf, &v)) { std::cout << "  [!] Not an integer. Repeat.\n"; continue; }
        if (v < minVal) { std::cout << "  [!] Value must be >= " << minVal << ".\n"; continue; }
        if (v > maxVal) { std::cout << "  [!] Value must be <= " << maxVal << ".\n"; continue; }
        if (checkDuplicates) {
            BOOL dup = FALSE;
            for (int i = 0; i < usedCount; ++i)
                if (usedIDs[i] == v) { dup = TRUE; break; }
            if (dup) { std::cout << "  [!] This ID is already taken.\n"; continue; }
        }
        return v;
    }
}

static double ReadDouble(const char* prompt, double minVal, double maxVal)
{
    char buf[64];
    while (TRUE) {
        std::cout << prompt;
        std::cin.getline(buf, sizeof(buf));
        double v;
        if (!ParseDouble(buf, &v)) { std::cout << "  [!] NaN. Repeat.\n"; continue; }
        if (v <= minVal) { std::cout << "  [!] Value must be > " << minVal << ".\n"; continue; }
        if (v > maxVal) { std::cout << "  [!] Value must be <= " << maxVal << ".\n"; continue; }
        return v;
    }
}

static void ReadName(const char* prompt, char* buf, int bufSize)
{
    const int maxLen = bufSize - 1;
    char tmp[64];
    while (TRUE) {
        std::cout << prompt;
        std::cin.getline(tmp, sizeof(tmp));
        int len = static_cast<int>(strlen(tmp));
        if (len == 0) { std::cout << "  [!] Name can't be empty.\n"; continue; }
        if (len > maxLen) { std::cout << "  [!] Too long (max " << maxLen << ").\n"; continue; }
        BOOL allDigits = TRUE;
        for (int i = 0; i < len; ++i)
            if (tmp[i] < '0' || tmp[i] > '9') { allDigits = FALSE; break; }
        if (allDigits) { std::cout << "  [!] Name can't be only digits.\n"; continue; }
        BOOL hasSpace = FALSE;
        for (int i = 0; i < len; ++i)
            if (tmp[i] == ' ') { hasSpace = TRUE; break; }
        if (hasSpace) { std::cout << "  [!] Name must be one word.\n"; continue; }
        strncpy_s(buf, bufSize, tmp, _TRUNCATE);
        return;
    }
}

static void ReadFilename(char* buf, int bufSize)
{
    char tmp[MAX_PATH];
    while (TRUE) {
        std::cout << "Filename : ";
        std::cin.getline(tmp, sizeof(tmp));
        int len = static_cast<int>(strlen(tmp));
        if (len == 0) { std::cout << "  [!] Name can't be empty.\n"; continue; }
        if (len > 64) { std::cout << "  [!] Too long (max 64).\n"; continue; }
        BOOL hasSep = FALSE;
        for (int i = 0; i < len; ++i)
            if (tmp[i] == '/' || tmp[i] == '\\') { hasSep = TRUE; break; }
        if (hasSep) { std::cout << "  [!] Name can't contain path separators.\n"; continue; }
        strncpy_s(buf, bufSize, tmp, _TRUNCATE);
        return;
    }
}


int main()
{
    std::cout << "       Employee File Server      \n\n";

    char filename[MAX_PATH] = {};
    ReadFilename(filename, sizeof(filename));

    int empCount = ReadInt("Employee amount : ", 1, MAX_EMPLOYEES, FALSE, NULL, 0);

    Employee employees[MAX_EMPLOYEES];
    int usedIDs[MAX_EMPLOYEES];
    memset(employees, 0, sizeof(employees));
    memset(usedIDs, 0, sizeof(usedIDs));

    for (int i = 0; i < empCount; ++i) {
        std::cout << "\n  Employee " << (i + 1) << " of " << empCount << "\n";
        int id = ReadInt("  ID    : ", 1, 999999, TRUE, usedIDs, i);
        usedIDs[i] = id;
        char   name[10] = {};
        ReadName("  Name  : ", name, sizeof(name));
        double h = ReadDouble("  Hours : ", 0.0, 744.0);
        EmployeeSet(&employees[i], id, name, h);
    }

    FileHandler fh(filename);
    fh.CreateFileWithData(employees, empCount);
    std::cout << "\n[Server] File created.\n";
    fh.ReadAllAndPrint();

    int clientCount = ReadInt("Client processes number : ", 1, 64, FALSE, NULL, 0);

    LockManager lm;

    for (int i = 0; i < clientCount; ++i) {
        STARTUPINFOA        si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si)); si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));
        char cmdLine[] = "client.exe";
        if (!CreateProcessA(NULL, cmdLine, NULL, NULL, FALSE,
            CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi)) {
            std::cout << "[Server] Failed to start client " << (i + 1)
                << "  (err " << GetLastError() << ")\n";
        }
        else {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
    }

    HANDLE threads[64] = {};
    int    threadCount = 0;

    for (int i = 0; i < clientCount; ++i) {
        HANDLE hPipe = CreateNamedPipe(
            PIPE_NAME,
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            PIPE_UNLIMITED_INSTANCES,
            4096, 4096, 0, NULL
        );
        if (hPipe == INVALID_HANDLE_VALUE) {
            std::cout << "[Server] CreateNamedPipe failed  (err " << GetLastError() << ")\n";
            continue;
        }
        if (ConnectNamedPipe(hPipe, NULL) == FALSE &&
            GetLastError() != ERROR_PIPE_CONNECTED) {
            std::cout << "[Server] ConnectNamedPipe failed  (err " << GetLastError() << ")\n";
            CloseHandle(hPipe);
            continue;
        }

        ClientContext* ctx = new ClientContext;
        ctx->hPipe = hPipe;
        ctx->fileHandler = &fh;
        ctx->lockManager = &lm;

        HANDLE hThread = CreateThread(NULL, 0, ClientThread, ctx, 0, NULL);
        if (hThread) {
            threads[threadCount++] = hThread;
        }
        else {
            std::cout << "[Server] CreateThread failed  (err " << GetLastError() << ")\n";
            delete ctx;
            CloseHandle(hPipe);
        }
    }

    if (threadCount > 0)
        WaitForMultipleObjects(threadCount, threads, TRUE, INFINITE);

    for (int i = 0; i < threadCount; ++i)
        CloseHandle(threads[i]);

    std::cout << "\n[Server] All clients done.\n";
    std::cout << "[Server] Final file contents:\n";
    fh.ReadAllAndPrint();

    std::cout << "Press Enter to exit...";
    std::cin.get();
    return 0;
}