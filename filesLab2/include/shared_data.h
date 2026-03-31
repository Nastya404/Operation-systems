#ifndef SHARED_DATA_H
#define SHARED_DATA_H

#include <vector>

struct SharedData {
    std::vector<int> array;
    int minValue = 0;
    int maxValue = 0;
    int minIndex = 0;
    int maxIndex = 0;
    double average = 0.0;
};

#endif // SHARED_DATA_H