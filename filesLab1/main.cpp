#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <fstream>
#include <string>
#define NOMINMAX
#include <windows.h>
#include "employee.h"

using std::cout;
using std::cin;
using std::cerr;
using std::endl;
using std::string;
using std::ifstream;
using std::ios;

bool runProcess(const char* commandLine) {
    char cmd[512];
    strncpy(cmd, commandLine, sizeof(cmd) - 1);
    cmd[sizeof(cmd) - 1] = '\0';

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    if (!CreateProcess(
        NULL,
        cmd,
        NULL,
        NULL,
        FALSE,
        CREATE_NEW_CONSOLE,
        NULL,
        NULL,
        &si,
        &pi))
    {
        DWORD error = GetLastError();
        cerr << "CreateProcess failed. Error code: " << error << endl;
        return false;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    return (exitCode == 0);
}

bool safeReadString(string& str) {
    if (!(cin >> str)) {
        cin.clear();
        cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return false;
    }
    return true;
}

int main() {
    try {
        string fileName;
        string reportName;
        string input;

        cout << "Enter binary file name: ";
        if (!safeReadString(fileName)) {
            throw ValidationException("Failed to read file name");
        }

        cout << "Enter number of employees: ";
        if (!safeReadString(input)) {
            throw ValidationException("Failed to read employee count");
        }
        int amount = parseRecordCount(input);

        char cmd[512];
        snprintf(cmd, sizeof(cmd), "Creator.exe %s %d", fileName.c_str(), amount);

        cout << "\nStarting Creator..." << endl;
        if (!runProcess(cmd)) {
            throw FileException("Creator failed or exited with error");
        }

        ifstream file(fileName, ios::binary);
        if (!file.is_open()) {
            throw FileException("Cannot open file: " + fileName);
        }

        employee emp;
        cout << "\n--- File contents ---" << endl;
        int count = 0;
        while (file.read(reinterpret_cast<char*>(&emp), sizeof(employee))) {
            cout << emp.num << " " << emp.name << " " << emp.hours << endl;
            ++count;
        }

        if (file.bad()) {
            file.close();
            throw FileException("Error reading file: " + fileName);
        }
        file.close();

        if (count == 0) {
            cerr << "Warning: file is empty" << endl;
        }

        cout << "\nEnter report file name: ";
        if (!safeReadString(reportName)) {
            throw ValidationException("Failed to read report file name");
        }

        cout << "Enter hourly pay: ";
        if (!safeReadString(input)) {
            throw ValidationException("Failed to read hourly pay");
        }
        double salary = parsePositiveDouble(input, "hourly pay", 0.01, 1000000.0);

        snprintf(cmd, sizeof(cmd), "Reporter.exe %s %s %.2f",
            fileName.c_str(), reportName.c_str(), salary);

        if (!runProcess(cmd)) {
            throw FileException("Reporter failed or exited with error");
        }

        ifstream report(reportName);
        if (!report.is_open()) {
            throw FileException("Cannot open report: " + reportName);
        }

        string line;
        cout << "\n--- Report ---" << endl;
        while (std::getline(report, line)) {
            cout << line << endl;
        }

        if (report.bad()) {
            report.close();
            throw FileException("Error reading report: " + reportName);
        }
        report.close();

        return 0;
    }
    catch (const FileException& e) {
        cerr << "File Error: " << e.what() << endl;
        return 1;
    }
    catch (const ValidationException& e) {
        cerr << "Validation Error: " << e.what() << endl;
        return 2;
    }
    catch (const std::exception& e) {
        cerr << "Unexpected Error: " << e.what() << endl;
        return 3;
    }
}