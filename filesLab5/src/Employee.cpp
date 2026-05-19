#include "Employee.h"
#include <iostream>
#include <cstring>
#include <cstdio>

void EmployeeInit(Employee* e)
{
    e->num = 0;
    e->hours = 0.0;
    memset(e->name, 0, sizeof(e->name));
}

void EmployeeSet(Employee* e, int num, const char* name, double hours)
{
    e->num = num;
    e->hours = hours;
    memset(e->name, 0, sizeof(e->name));
    strncpy_s(e->name, sizeof(e->name), name, _TRUNCATE);
}

void EmployeePrint(const Employee* e)
{
    std::cout << "  ID: " << e->num
        << "\tName: " << e->name
        << "\tHours: " << e->hours << "\n";
}