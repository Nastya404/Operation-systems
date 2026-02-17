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

class ValidationException : public std::runtime_error {
public:
    explicit ValidationException(const std::string& msg) : std::runtime_error(msg) {}
};

class FileException : public std::runtime_error {
public:
    explicit FileException(const std::string& msg) : std::runtime_error(msg) {}
};

inline int parsePositiveInteger(const std::string& str,
    const std::string& fieldName,
    int minValue = 1,
    int maxValue = INT_MAX) {

    if (str.empty()) {
        throw ValidationException(fieldName + " cannot be empty");
    }

    int value;
    try {
        size_t pos;
        value = std::stoi(str, &pos);

        if (pos != str.length()) {
            throw ValidationException(
                fieldName + " contains non-numeric characters: " + str);
        }
    }
    catch (const std::invalid_argument&) {
        throw ValidationException(fieldName + " is not a valid number: " + str);
    }
    catch (const std::out_of_range&) {
        throw ValidationException(fieldName + " is out of range: " + str);
    }

    if (value < minValue) {
        throw ValidationException(
            fieldName + " must be at least " + std::to_string(minValue) +
            ", got: " + std::to_string(value));
    }
    if (value > maxValue) {
        throw ValidationException(
            fieldName + " must be at most " + std::to_string(maxValue) +
            ", got: " + std::to_string(value));
    }

    return value;
}

inline double parsePositiveDouble(const std::string& str,
    const std::string& fieldName,
    double minValue = 0.0,
    double maxValue = 1000000.0) {

    if (str.empty()) {
        throw ValidationException(fieldName + " cannot be empty");
    }

    double value;
    try {
        size_t pos;
        value = std::stod(str, &pos);
        if (pos != str.length()) {
            throw ValidationException(
                fieldName + " contains invalid characters: " + str);
        }
    }
    catch (const std::invalid_argument&) {
        throw ValidationException(fieldName + " is not a valid number: " + str);
    }
    catch (const std::out_of_range&) {
        throw ValidationException(fieldName + " is out of range: " + str);
    }

    if (value < minValue) {
        throw ValidationException(
            fieldName + " must be at least " + std::to_string(minValue));
    }
    if (value > maxValue) {
        throw ValidationException(
            fieldName + " must be at most " + std::to_string(maxValue));
    }
    return value;
}

inline int parseRecordCount(const std::string& str) {
    return parsePositiveInteger(str, "Record count", 1, 10000);
}

inline int parseEmployeeId(const std::string& str) {
    return parsePositiveInteger(str, "Employee ID", 1);
}

inline double parseHours(const std::string& str) {
    return parsePositiveDouble(str, "Working hours", 0.0, 744.0);
}

#endif // EMPLOYEE_H