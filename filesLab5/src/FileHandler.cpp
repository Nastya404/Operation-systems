#include "FileHandler.h"
#include <iostream>
#include <cstring>
#include <cstdio>

FileHandler::FileHandler(const char* filename)
{
    strncpy_s(_filename, MAX_PATH, filename, _TRUNCATE);
}


void FileHandler::CreateFileWithData(const Employee* data, int count)
{
    HANDLE hf = CreateFileA(
        _filename,
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hf == INVALID_HANDLE_VALUE) {
        std::cerr << "[FileHandler] Cannot create '" << _filename
            << "'  (err " << GetLastError() << ")\n";
        return;
    }

    for (int i = 0; i < count; ++i) {
        DWORD written = 0;
        WriteFile(hf, &data[i], sizeof(Employee), &written, NULL);
    }

    CloseHandle(hf);
}

void FileHandler::ReadAllAndPrint() const
{
    HANDLE hf = CreateFileA(
        _filename,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    if (hf == INVALID_HANDLE_VALUE) {
        std::cerr << "[FileHandler] Cannot open '" << _filename
            << "' for reading  (err " << GetLastError() << ")\n";
        return;
    }

    std::cout << "\n+--------+-----------+----------+\n";
    std::cout << "| ID     | Name      | Hours    |\n";
    std::cout << "+--------+-----------+----------+\n";

    Employee e;
    DWORD    nRead = 0;
    while (ReadFile(hf, &e, sizeof(Employee), &nRead, NULL) && nRead == sizeof(Employee)) {
        char line[128];
        char hoursBuf[32];
        sprintf_s(hoursBuf, sizeof(hoursBuf), "%8.2f", e.hours);
        sprintf_s(line, sizeof(line), "| %-6d | %-9s | %s |\n",
            e.num, e.name, hoursBuf);
        std::cout << line;
    }

    std::cout << "+--------+-----------+----------+\n\n";
    CloseHandle(hf);
}

bool FileHandler::GetEmployee(int id, Employee& outEmp) const
{
    HANDLE hf = CreateFileA(
        _filename,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    if (hf == INVALID_HANDLE_VALUE) return false;

    Employee e;
    DWORD    nRead = 0;
    bool     found = false;

    while (ReadFile(hf, &e, sizeof(Employee), &nRead, NULL) && nRead == sizeof(Employee)) {
        if (e.num == id) {
            outEmp = e;
            found = true;
            break;
        }
    }

    CloseHandle(hf);
    return found;
}

bool FileHandler::UpdateEmployee(const Employee& emp)
{
    HANDLE hf = CreateFileA(
        _filename,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    if (hf == INVALID_HANDLE_VALUE) return false;

    Employee all[MAX_EMPLOYEES];
    int      count = 0;
    DWORD    nRead = 0;

    while (count < MAX_EMPLOYEES &&
        ReadFile(hf, &all[count], sizeof(Employee), &nRead, NULL) &&
        nRead == sizeof(Employee)) {
        ++count;
    }
    CloseHandle(hf);

    bool found = false;
    for (int i = 0; i < count; ++i) {
        if (all[i].num == emp.num) {
            all[i] = emp;
            found = true;
            break;
        }
    }
    if (!found) return false;

    CreateFileWithData(all, count);
    return true;
}