#ifndef THREADS_H
#define THREADS_H

#include <windows.h>
#include "shared_data.h"

DWORD WINAPI MinMaxThread(LPVOID lpParam);
DWORD WINAPI AverageThread(LPVOID lpParam);

#endif // THREADS_H