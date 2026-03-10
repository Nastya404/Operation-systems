#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <cstring>
#include <windows.h>
#include "employee.h"

using std::cout;
using std::cin;
using std::cerr;
using std::endl;

bool runProcess(const char* commandLine) {
    char cmd[512];
    sprintf_s(cmd, sizeof(cmd), "%s", commandLine);

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    if (!CreateProcessA(
        NULL,               // lpApplicationName
        cmd,                // lpCommandLine: командная строка целиком
        NULL,               // lpProcessAttributes: безопасность процесса
        NULL,               // lpThreadAttributes: безопасность потока
        FALSE,              // bInheritHandles: не наследовать дескрипторы
        CREATE_NEW_CONSOLE, // dwCreationFlags: новое окно консоли
        NULL,               // lpEnvironment
        NULL,               // lpCurrentDirectory
        &si,                // lpStartupInfo: настройки запуска
        &pi))               // lpProcessInformation: инфо о процессе
    {
        cerr << "CreateProcess failed. Error: " << GetLastError() << endl;
        return false;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    return true;
}

int main() {
    char filename[256];
    char reportName[256];

    cout << "Enter binary file name: ";
    cin >> filename;
    if (cin.fail() || strlen(filename) == 0) {
        cerr << "Error: invalid file name." << endl;
        return 1;
    }

    cout << "Enter number of records: ";
    int count;
    cin >> count;
    if (cin.fail() || count <= 0) {
        cerr << "Error: count must be a positive number." << endl;
        return 1;
    }

    char cmd[512];
    sprintf_s(cmd, sizeof(cmd), "Creator.exe %s %d", filename, count);

    if (!runProcess(cmd)) {
        cerr << "Error: failed to start Creator." << endl;
        return 1;
    }

    HANDLE hFile = CreateFileA(
        filename,              // lpFileName: имя файла
        GENERIC_READ,          // dwDesiredAccess: доступ на чтение
        0,                     // dwShareMode: без совместного доступа
        NULL,                  // lpSecurityAttributes: по умолчанию
        OPEN_EXISTING,         // dwCreationDisposition: файл должен существовать
        FILE_ATTRIBUTE_NORMAL, // dwFlagsAndAttributes: обычный файл
        NULL                   // hTemplateFile: без шаблона
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        cerr << "Error: cannot open '" << filename
            << "'. Code: " << GetLastError() << endl;
        return 1;
    }

    employee emp;
    DWORD bytesRead;

    cout << "\n--- Binary file contents ---" << endl;
    while (ReadFile(hFile, &emp, sizeof(emp), &bytesRead, NULL)
        && bytesRead == sizeof(emp))
    {
        cout << emp.num << " "
            << emp.name << " "
            << emp.hours << endl;
    }

    CloseHandle(hFile);


    cout << "\nEnter report file name: ";
    cin >> reportName;
    if (cin.fail() || strlen(reportName) == 0) {
        cerr << "Error: invalid report file name." << endl;
        return 1;
    }

    cout << "Enter hourly pay: ";
    double pay;
    cin >> pay;
    if (cin.fail() || pay <= 0) {
        cerr << "Error: pay must be a positive number." << endl;
        return 1;
    }

    sprintf_s(cmd, sizeof(cmd), "Reporter.exe %s %s %.2f",
        filename, reportName, pay);

    if (!runProcess(cmd)) {
        cerr << "Error: failed to start Reporter." << endl;
        return 1;
    }

    HANDLE hReport = CreateFileA(
        reportName,
        GENERIC_READ,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hReport == INVALID_HANDLE_VALUE) {
        cerr << "Error: cannot open report '" << reportName
            << "'. Code: " << GetLastError() << endl;
        return 1;
    }

    char buffer[256];
    cout << "\n--- Report ---" << endl;
    while (ReadFile(hReport, buffer, sizeof(buffer) - 1, &bytesRead, NULL)
        && bytesRead > 0)
    {
        buffer[bytesRead] = '\0';
        cout << buffer;
    }

    CloseHandle(hReport);

    return 0;
}