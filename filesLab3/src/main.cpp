#include <windows.h>
#include <iostream>
#include "array.h"
#include "marker.h"
using namespace std;

HANDLE hConsoleMutex;

int main() {
    hConsoleMutex = CreateMutex(NULL, FALSE, NULL);

    int arraySize;
    cout << "Array size: ";
    cin >> arraySize;
    Array array(arraySize);

    int markerCount;
    cout << "Marker count: ";
    cin >> markerCount;

    HANDLE hStartEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    Marker** markers = new Marker * [markerCount];
    for (int i = 0; i < markerCount; i++)
        markers[i] = new Marker(i + 1, &array, hStartEvent);

    SetEvent(hStartEvent);

    int alive = markerCount;

    while (alive > 0) {

        HANDLE* doneEvents = new HANDLE[alive];
        int k = 0;
        for (int i = 0; i < markerCount; i++)
            if (markers[i] != nullptr)
                doneEvents[k++] = markers[i]->getDoneEvent();

        WaitForMultipleObjects(alive, doneEvents, TRUE, INFINITE);
        delete[] doneEvents;

        cout << "\nArray: ";
        array.print();

        int killNumber;
        while (true) {
            cout << "Enter marker number to stop: ";
            cin >> killNumber;
            bool valid = false;
            for (int i = 0; i < markerCount; i++)
                if (markers[i] != nullptr && markers[i]->getNumber() == killNumber)
                {
                    valid = true; break;
                }
            if (valid) break;
            cout << "Invalid number. Try again.\n";
        }

        for (int i = 0; i < markerCount; i++) {
            if (markers[i] != nullptr && markers[i]->getNumber() == killNumber) {
                markers[i]->stop();
                WaitForSingleObject(markers[i]->getThreadHandle(), INFINITE);
                delete markers[i];
                markers[i] = nullptr;
                alive--;
                break;
            }
        }

        cout << "Array after cleanup: ";
        array.print();

        if (alive > 0) {
            for (int i = 0; i < markerCount; i++)
                if (markers[i] != nullptr)
                    markers[i]->cont(); 
        }
    }

    cout << "All markers finished.\n";
    array.print();

    delete[] markers;
    CloseHandle(hStartEvent);
    CloseHandle(hConsoleMutex);
    return 0;
}
