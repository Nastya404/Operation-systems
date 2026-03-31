#include "threads.h"
#include <iostream>

using std::cout;
using std::endl;

DWORD WINAPI MinMaxThread(LPVOID lpParam) {
    SharedData* data = static_cast<SharedData*>(lpParam);

    data->minValue = data->array[0];
    data->maxValue = data->array[0];
    data->minIndex = 0;
    data->maxIndex = 0;

    for (size_t i = 1; i < data->array.size(); ++i) {
        if (data->array[i] < data->minValue) {
            data->minValue = data->array[i];
            data->minIndex = static_cast<int>(i);
        }
        if (data->array[i] > data->maxValue) {
            data->maxValue = data->array[i];
            data->maxIndex = static_cast<int>(i);
        }
        Sleep(7);
    }

    cout << "min_max thread: min = " << data->minValue
        << " (index " << data->minIndex << "), max = " << data->maxValue
        << " (index " << data->maxIndex << ")" << endl;

    return 0;
}

DWORD WINAPI AverageThread(LPVOID lpParam) {
    SharedData* data = static_cast<SharedData*>(lpParam);

    long long sum = data->array[0];
    for (size_t i = 1; i < data->array.size(); ++i) {
        sum += data->array[i];
        Sleep(12);
    }

    data->average = static_cast<double>(sum) / static_cast<double>(data->array.size());

    cout << "average thread: average = " << data->average << endl;

    return 0;
}