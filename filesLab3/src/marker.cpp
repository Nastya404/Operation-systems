#include "marker.h"
#include <iostream>
using namespace std;

Marker::Marker(int number, Array* array,
    HANDLE hStartEvent)
    : number(number), array(array),
    hStartEvent(hStartEvent)
{
    hStopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (hStopEvent == NULL)
        throw std::runtime_error("Failed to create stop event");

    hDoneEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (hDoneEvent == NULL) {
        CloseHandle(hStopEvent);
        throw std::runtime_error("Failed to create done event");
    }

    hContinueEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (hContinueEvent == NULL) {
        CloseHandle(hContinueEvent);
        throw std::runtime_error("Failed to create close event");
    }

    DWORD id;
    hThread = CreateThread(NULL, 0, threadFunc, this, 0, &id);

    if (hThread == NULL) {
        CloseHandle(hDoneEvent);
        CloseHandle(hStopEvent);
        CloseHandle(hContinueEvent);
        throw std::runtime_error("Failed to create thread");
    }
}

Marker::~Marker() {
    CloseHandle(hContinueEvent);
    CloseHandle(hStopEvent);
    CloseHandle(hDoneEvent);
    CloseHandle(hThread);
}

DWORD WINAPI Marker::threadFunc(LPVOID param) {
    Marker* m = (Marker*)param;
    m->run();
    return 0;
}

void Marker::run() {
    WaitForSingleObject(hStartEvent, INFINITE);

    srand(number);

    int markedCount = 0;

    while (true) {
        int index = rand() % array->getSize();

        if (array->getElement(index) == 0) {
            Sleep(5);
            array->setElement(index, number);
            Sleep(5);
            markedCount++;
        }
        else {
            WaitForSingleObject(hConsoleMutex, INFINITE);
            cout << "Marker " << number
                << ": marked=" << markedCount
                << ", blocked at index=" << index << endl;
            ReleaseMutex(hConsoleMutex);

            SetEvent(hDoneEvent);

            HANDLE signals[2] = { hContinueEvent, hStopEvent };
            DWORD result = WaitForMultipleObjects(2, signals, FALSE, INFINITE);

            if (result == WAIT_OBJECT_0 + 1) {
                break;
            }
        }
    }

    array->clearByValue(number);
}

void Marker::stop() { SetEvent(hStopEvent); }
void Marker::cont() { SetEvent(hContinueEvent); }

HANDLE Marker::getDoneEvent() const { return hDoneEvent; }
HANDLE Marker::getThreadHandle() const { return hThread; }
int Marker::getNumber() const { return number; }