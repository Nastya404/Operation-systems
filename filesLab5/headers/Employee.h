#pragma once
#include <windows.h>  

struct Employee {
    int    num;
    char   name[10];
    double hours;
};

void EmployeeInit(Employee* e);
void EmployeeSet(Employee* e, int num, const char* name, double hours);
void EmployeePrint(const Employee* e);