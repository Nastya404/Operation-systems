#include "threads.h"
#include <iostream>
#include <cmath>

using std::cin;
using std::cout;
using std::cerr;
using std::endl;

int main() {
    int n = 0;
    cout << "Enter array size: ";
    if (!(cin >> n) || n <= 0) {
        cerr << "Error: invalid array size." << endl;
        return 1;
    }

    SharedData data;
    data.array.resize(static_cast<size_t>(n));

    cout << "Enter " << n << " integer elements: ";
    for (int i = 0; i < n; ++i) {
        if (!(cin >> data.array[static_cast<size_t>(i)])) {
            cerr << "Error: invalid input for element " << i << "." << endl;
            return 1;
        }
    }

    cout << "Original array: ";
    for (size_t i = 0; i < data.array.size(); ++i) {
        cout << data.array[i];
        if (i + 1 < data.array.size()) cout << " ";
    }
    cout << endl;

    DWORD idMinMax = 0;
    DWORD idAverage = 0;

    HANDLE hMinMax = CreateThread(NULL, 0, MinMaxThread, &data, 0, &idMinMax);
    if (hMinMax == NULL) {
        cerr << "Error creating min_max thread. Code: " << GetLastError() << endl;
        return static_cast<int>(GetLastError());
    }

    HANDLE hAverage = CreateThread(NULL, 0, AverageThread, &data, 0, &idAverage);
    if (hAverage == NULL) {
        cerr << "Error creating average thread. Code: " << GetLastError() << endl;
        CloseHandle(hMinMax);
        return static_cast<int>(GetLastError());
    }

    WaitForSingleObject(hMinMax, INFINITE);
    WaitForSingleObject(hAverage, INFINITE);

    CloseHandle(hAverage);
    CloseHandle(hMinMax);

    int avgInt = static_cast<int>(std::round(data.average));
    data.array[data.minIndex] = avgInt;
    data.array[data.maxIndex] = avgInt;

    cout << "Result array: ";
    for (size_t i = 0; i < data.array.size(); ++i) {
        cout << data.array[i];
        if (i + 1 < data.array.size()) cout << " ";
    }
    cout << endl;

    return 0;
}