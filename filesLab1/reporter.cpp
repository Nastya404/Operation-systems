#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <windows.h>
#include "employee.h"

using std::cout;
using std::cerr;
using std::endl;

int main(int argc, char* argv[]) {
    try {
        if (argc != 4) {
            cerr << "Usage: Reporter.exe <input_file> <report_file> <hourly_pay>" << endl;
            return 1;
        }

        char* inputFileName = argv[1];
        char* reportFileName = argv[2];
        double pay = atof(argv[3]);

        if (pay <= 0) {
            cerr << "Error: hourly pay must be positive." << endl;
            return 1;
        }

        HANDLE hInput = CreateFileA(
            inputFileName,
            GENERIC_READ,
            0,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );

        if (hInput == INVALID_HANDLE_VALUE) {
            cerr << "Error: cannot open '" << inputFileName
                << "'. Code: " << GetLastError() << endl;
            return 1;
        }

        HANDLE hReport = CreateFileA(
            reportFileName,
            GENERIC_WRITE,
            0,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );

        if (hReport == INVALID_HANDLE_VALUE) {
            cerr << "Error: cannot create '" << reportFileName
                << "'. Code: " << GetLastError() << endl;
            CloseHandle(hInput); 
            return 1;
        }

        char line[512];
        DWORD bytesWritten;

        sprintf(line, "Report for file \"%s\"\r\n", inputFileName);
        WriteFile(hReport, line, strlen(line), &bytesWritten, NULL);

        sprintf(line, "%-10s %-10s %-10s %-10s\r\n",
            "Number", "Name", "Hours", "Salary");
        WriteFile(hReport, line, strlen(line), &bytesWritten, NULL);

        employee emp;
        DWORD bytesRead;
        int recordCount = 0;

        while (ReadFile(hInput, &emp, sizeof(employee), &bytesRead, NULL)
            && bytesRead == sizeof(employee))
        {
            double salary = emp.hours * pay;

            sprintf(line, "%-10d %-10s %-10.1f %-10.2f\r\n",
                emp.num, emp.name, emp.hours, salary);
            WriteFile(hReport, line, strlen(line), &bytesWritten, NULL);

            ++recordCount;
        }

        CloseHandle(hReport);
        CloseHandle(hInput);

        cout << "Report '" << reportFileName << "' created ("
            << recordCount << " records)." << endl;
        return 0;
    }
    catch (const std::exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
}