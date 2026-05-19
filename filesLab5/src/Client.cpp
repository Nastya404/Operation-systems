#include "Request.h"
#include <windows.h>
#include <iostream>
#include <cstring>
#include <cstdio>
#include <conio.h>


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

static int ReadID()
{
    char buf[32];
    while (TRUE) {
        std::cout << "  Employee ID: ";
        std::cin.getline(buf, sizeof(buf));
        int v;
        if (!ParseInt(buf, &v)) {
            std::cout << "  [!] Not a valid integer. Try again.\n";
            continue;
        }
        if (v <= 0) {
            std::cout << "  [!] ID must be a positive number.\n";
            continue;
        }
        return v;
    }
}

static double ReadHours()
{
    char buf[32];
    while (TRUE) {
        std::cout << "  New hours              : ";
        std::cin.getline(buf, sizeof(buf));
        double v;
        if (!ParseDouble(buf, &v)) {
            std::cout << "  [!] Not a valid number. Try again.\n";
            continue;
        }
        if (v <= 0.0) {
            std::cout << "  [!] Hours must be greater than 0.\n";
            continue;
        }
        if (v > 744.0) {
            std::cout << "  [!] Hours cannot exceed 744 (31 days * 24 h).\n";
            continue;
        }
        return v;
    }
}

static void ReadName(char* buf, int bufSize)
{
    const int maxLen = bufSize - 1;
    char tmp[64];
    while (TRUE) {
        std::cout << "  New name  (max " << maxLen << " chars): ";
        std::cin.getline(tmp, sizeof(tmp));

        int len = static_cast<int>(strlen(tmp));
        if (len == 0) {
            std::cout << "  [!] Name cannot be empty.\n";
            continue;
        }
        if (len > maxLen) {
            std::cout << "  [!] Name too long (max " << maxLen << " characters).\n";
            continue;
        }
        BOOL allDigits = TRUE;
        for (int i = 0; i < len; ++i)
            if (tmp[i] < '0' || tmp[i] > '9') { allDigits = FALSE; break; }
        if (allDigits) {
            std::cout << "  [!] Name cannot consist of digits only.\n";
            continue;
        }
        BOOL hasSpace = FALSE;
        for (int i = 0; i < len; ++i)
            if (tmp[i] == ' ') { hasSpace = TRUE; break; }
        if (hasSpace) {
            std::cout << "  [!] Name must be a single word (no spaces).\n";
            continue;
        }
        strncpy_s(buf, bufSize, tmp, _TRUNCATE);
        return;
    }
}


static void PrintMenu()
{
    std::cout << "  1. Read record             \n";
    std::cout << "  2. Modify record           \n";
    std::cout << "  3. Exit                    \n";
    std::cout << "Choice: ";
}

static void PrintEmployee(const Employee* e)
{
    std::cout << "   ID    : " << e->num << "\n";
    std::cout << "   Name  : " << e->name << "\n";
    std::cout << "   Hours : " << e->hours << "\n";
}


static HANDLE ConnectToPipe()
{
    std::cout << "[Client] Connecting to server...\n";

    HANDLE h = INVALID_HANDLE_VALUE;
    while (h == INVALID_HANDLE_VALUE) {
        WaitNamedPipe(PIPE_NAME, NMPWAIT_WAIT_FOREVER);
        h = CreateFile(PIPE_NAME,
            GENERIC_READ | GENERIC_WRITE,
            0, NULL, OPEN_EXISTING, 0, NULL);
        if (h == INVALID_HANDLE_VALUE &&
            GetLastError() != ERROR_PIPE_BUSY) {
            std::cout << "[Client] CreateFile failed  (err "
                << GetLastError() << "), retrying...\n";
            Sleep(200);
        }
    }
    DWORD mode = PIPE_READMODE_MESSAGE;
    SetNamedPipeHandleState(h, &mode, NULL, NULL);
    std::cout << "[Client] Connected.\n";
    return h;
}


static void DoRead(HANDLE hPipe)
{
    int id = ReadID();

    Request req;
    memset(&req, 0, sizeof(req));
    req.type = REQ_READ;
    req.emp_id = id;

    if (!SendRequest(hPipe, req)) {
        std::cout << "  [!] Failed to send request.\n";
        return;
    }

    Response resp;
    memset(&resp, 0, sizeof(resp));
    if (!ReadResponse(hPipe, resp)) {
        std::cout << "  [!] Failed to receive response.\n";
        return;
    }

    if (resp.success)
        PrintEmployee(&resp.data);
    else
        std::cout << "  Server says: " << resp.message << "\n";

    std::cout << "  Нажмите Enter чтобы отпустить доступ на чтение...";
    std::cin.get();

    Request done;
    memset(&done, 0, sizeof(done));
    done.type = REQ_READ_DONE;
    done.emp_id = id;
    SendRequest(hPipe, done);

    Response doneResp;
    memset(&doneResp, 0, sizeof(doneResp));
    ReadResponse(hPipe, doneResp);
}

static void DoModify(HANDLE hPipe)
{
    int id = ReadID();

    Request req;
    memset(&req, 0, sizeof(req));
    req.type = REQ_MODIFY_START;
    req.emp_id = id;

    if (!SendRequest(hPipe, req)) {
        std::cout << "  [!] Failed to send request.\n";
        return;
    }

    Response resp;
    memset(&resp, 0, sizeof(resp));
    if (!ReadResponse(hPipe, resp)) {
        std::cout << "  [!] Failed to receive response.\n";
        return;
    }

    if (!resp.success) {
        std::cout << "  Server says: " << resp.message << "\n";
        return;
    }

    std::cout << "  Current record:\n";
    PrintEmployee(&resp.data);

    char   newName[10] = {};
    ReadName(newName, sizeof(newName));
    double newHours = ReadHours();

    memset(&req, 0, sizeof(req));
    req.type = REQ_MODIFY_COMMIT;
    req.emp_id = id;
    req.data.num = id;
    strncpy_s(req.data.name, sizeof(req.data.name), newName, _TRUNCATE);
    req.data.hours = newHours;

    if (!SendRequest(hPipe, req)) {
        std::cout << "  [!] Failed to send commit.\n";
        return;
    }

    Response commitResp;
    memset(&commitResp, 0, sizeof(commitResp));
    if (!ReadResponse(hPipe, commitResp)) {
        std::cout << "  [!] Failed to receive commit response.\n";
        return;
    }

    std::cout << "  Server says: " << commitResp.message << "\n";
}


int main()
{
    HANDLE hPipe = ConnectToPipe();

    BOOL running = TRUE;
    while (running) {
        PrintMenu();

        char buf[8];
        std::cin.getline(buf, sizeof(buf));

        int choice;
        if (!ParseInt(buf, &choice)) {
            std::cout << "  [!] Enter 1, 2 or 3.\n";
            continue;
        }

        switch (choice) {
        case 1:
            DoRead(hPipe);
            break;
        case 2:
            DoModify(hPipe);
            break;
        case 3: {
            Request req;
            memset(&req, 0, sizeof(req));
            req.type = REQ_EXIT;
            SendRequest(hPipe, req);
            running = FALSE;
            break;
        }
        default:
            std::cout << "  [!] Unknown option. Enter 1, 2 or 3.\n";
        }
    }

    CloseHandle(hPipe);
    std::cout << "[Client] Disconnected. Goodbye.\n";
    std::cout << "Press any key to close...";
    _getch();
    return 0;
}