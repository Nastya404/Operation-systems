#ifndef EMPLOYEE_H
#define EMPLOYEE_H

#include <stdexcept>
#include <string>
#include <climits>

struct employee {
    int num;        // идентификационный номер сотрудника
    char name[10];  // имя сотрудника
    double hours;   // количество отработанных часов
};

#endif // EMPLOYEE_H