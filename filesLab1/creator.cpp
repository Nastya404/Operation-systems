#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <windows.h>
#include "employee.h"

using std::cout;
using std::cin;
using std::cerr;
using std::endl;

int main(int argc, char* argv[]) {
    try {
        if (argc != 3) {
            cerr << "Usage: Creator.exe <filename> <record_count>" << endl;
            return 1;
        }

        char* fileName = argv[1];
        int count = atoi(argv[2]);

        if (count <= 0) {
            cerr << "Error: record count must be positive." << endl;
            return 1;
        }

        HANDLE hFile = CreateFileA(
            fileName,              // имя файла
            GENERIC_WRITE,         // доступ на запись
            0,                     // без совместного доступа
            NULL,                  // безопасность по умолчанию
            CREATE_ALWAYS,         // создать новый 
            FILE_ATTRIBUTE_NORMAL, // обычный файл
            NULL                   // без шаблона
        );

        if (hFile == INVALID_HANDLE_VALUE) {
            cerr << "Error: cannot create file '" << fileName
                << "'. Code: " << GetLastError() << endl;
            return 1;
        }

        cout << "Enter " << count << " employee records:\n" << endl;

        for (int i = 0; i < count; ++i) {
            employee emp;
            memset(&emp, 0, sizeof(employee));

            cout << "Record " << (i + 1) << ":" << endl;

            cout << "  ID: ";
            cin >> emp.num;
            if (cin.fail()) {
                cerr << "Error: invalid ID." << endl;
                CloseHandle(hFile);
                return 1;
            }

            cout << "  Name (max 9 chars): ";
            cin >> emp.name;
            emp.name[9] = '\0';

            cout << "  Hours: ";
            cin >> emp.hours;
            if (cin.fail() || emp.hours < 0) {
                cerr << "Error: invalid hours." << endl;
                CloseHandle(hFile);
                return 1;
            }

            DWORD bytesWritten;
            if (!WriteFile(hFile, &emp, sizeof(employee), &bytesWritten, NULL)
                || bytesWritten != sizeof(employee))
            {
                cerr << "Error: write failed. Code: " << GetLastError() << endl;
                CloseHandle(hFile);
                return 1;
            }

            cout << endl;
        }

        CloseHandle(hFile);
        return 0;
    }
    catch (const std::exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
}