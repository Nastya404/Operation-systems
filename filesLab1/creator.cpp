#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <fstream>
#include <string>
#include <limits>
#include <cstring>
#include "employee.h"

using std::string;
using std::cout;
using std::cerr;
using std::cin;
using std::endl;

int main(int argc, char* argv[]) {
    std::ofstream file;

    try {
        if (argc != 3) {
            throw ValidationException(
                "Usage: Creator.exe <filename> <count>");
        }

        string fileName = argv[1];
        int count = parseRecordCount(argv[2]);

        file.open(fileName, std::ios::binary | std::ios::trunc);

        if (!file.is_open()) {
            throw FileException("Cannot create file: " + fileName);
        }

        for (int i = 0; i < count; ++i) {
            employee emp{};

            cout << "Employee " << i + 1 << endl;

            cout << "  num: ";
            string input;
            if (!(cin >> input)) {
                throw ValidationException("Failed to read employee number");
            }
            emp.num = parseEmployeeId(input);

            cout << "  name: ";
            if (!(cin >> input)) {
                throw ValidationException("Failed to read employee name");
            }
            if (input.length() >= sizeof(emp.name)) {
                throw ValidationException("Name too long (max 9 chars)");
            }
            strncpy(emp.name, input.c_str(), sizeof(emp.name) - 1);
            emp.name[sizeof(emp.name) - 1] = '\0';

            cout << "  hours: ";
            if (!(cin >> input)) {
                throw ValidationException("Failed to read hours");
            }
            emp.hours = parseHours(input);

            file.write(reinterpret_cast<const char*>(&emp), sizeof(employee));

            if (file.fail()) {
                throw FileException("Failed to write record #" + std::to_string(i + 1));
            }

            cout << endl;
        }

        file.close();
        if (file.fail()) {
            throw FileException("Error closing file (disk full?)");
        }

        cout << "File '" << fileName << "' created successfully ("
            << count << " records)." << endl;
    }
    catch (const ValidationException& e) {
        cerr << "Validation Error: " << e.what() << endl;
        if (file.is_open()) {
            file.close();
        }
        return 1;
    }
    catch (const FileException& e) {
        cerr << "File Error: " << e.what() << endl;
        if (file.is_open()) {
            file.close();
        }
        return 2;
    }
    catch (const std::exception& e) {
        cerr << "Unexpected Error: " << e.what() << endl;
        if (file.is_open()) {
            file.close();
        }
        return 3;
    }

    return 0;
}