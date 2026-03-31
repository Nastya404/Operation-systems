#pragma once
#include <windows.h>
#include "array.h"

extern HANDLE hConsoleMutex;

class Marker {
private:
    int     number;
    Array* array;

    HANDLE  hThread;
    HANDLE  hStartEvent;   
    HANDLE  hContinueEvent; 
    HANDLE  hStopEvent;     
    HANDLE  hDoneEvent;   

    static DWORD WINAPI threadFunc(LPVOID param);
    void run(); 

public:
    Marker(int number, Array* array,
        HANDLE hStartEvent);
    ~Marker();

    void    stop();
    void   cont();
    HANDLE  getDoneEvent() const;
    HANDLE  getThreadHandle() const;
    int getNumber() const;
};