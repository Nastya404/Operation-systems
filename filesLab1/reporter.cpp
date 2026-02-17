#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
#include "employee.h"

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::ifstream;
using std::ofstream;
using std::ios;

int main(int argc, char* argv[]) {
    ifstream input;
    ofstream report;

    try {
        if (argc != 4) {
            throw ValidationException(
                "Usage: Reporter.exe <input_file> <report_file> <hourly_pay>");
        }

        string inputFileName = argv[1];
        string reportFileName = argv[2];
        double pay = parsePositiveDouble(argv[3], "hourly pay", 0.01, 1000000.0);

        input.open(inputFileName, ios::binary);
        if (!input.is_open()) {
            throw FileException("Cannot open input file: " + inputFileName);
        }

        report.open(reportFileName);
        if (!report.is_open()) {
            input.close();
            throw FileException("Cannot create report file: " + reportFileName);
        }

        report << "Report for file " << inputFileName << endl;
        report << std::setw(10) << "Number"
            << std::setw(10) << "Name"
            << std::setw(10) << "Hours"
            << std::setw(12) << "Salary"
            << endl;
        report << string(42, '-') << endl;

        if (report.fail()) {
            throw FileException("Failed to write report header");
        }

        employee emp;
        int recordCount = 0;

        while (input.read(reinterpret_cast<char*>(&emp), sizeof(employee))) {
            double salary = emp.hours * pay;

            report << std::setw(10) << emp.num
                << std::setw(10) << emp.name
                << std::setw(10) << std::fixed << std::setprecision(1) << emp.hours
                << std::setw(12) << std::fixed << std::setprecision(2) << salary
                << endl;

            if (report.fail()) {
                throw FileException(
                    "Failed to write record #" + std::to_string(recordCount + 1));
            }
            ++recordCount;
        }

        if (input.bad()) {
            throw FileException("Error reading input file");
        }

        if (recordCount == 0) {
            cerr << "Input file is empty." << endl;
        }

        report.close();
        if (report.fail()) {
            throw FileException("Error closing report file (disk full?)");
        }

        input.close();
        return 0;
    }
    catch (const FileException& e) {
        cerr << "File Error: " << e.what() << endl;
        if (input.is_open()) input.close();
        if (report.is_open()) report.close();
        return 1;
    }
    catch (const ValidationException& e) {
        cerr << "Validation Error: " << e.what() << endl;
        if (input.is_open()) input.close();
        if (report.is_open()) report.close();
        return 2;
    }
    catch (const std::exception& e) {
        cerr << "Unexpected Error: " << e.what() << endl;
        if (input.is_open()) input.close();
        if (report.is_open()) report.close();
        return 3;
    }
}