#pragma once
#include "Employee.h"
#include <windows.h>

#define MAX_EMPLOYEES 100

class FileHandler {
public:
    explicit FileHandler(const char* filename);

    void CreateFileWithData(const Employee* data, int count);
    void ReadAllAndPrint()                          const;
    bool GetEmployee(int id, Employee& outEmp)     const;
    bool UpdateEmployee(const Employee& emp);

private:
    char _filename[MAX_PATH];
};