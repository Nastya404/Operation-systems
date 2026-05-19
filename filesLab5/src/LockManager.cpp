#include "LockManager.h"
#include <cstring>
#include <iostream>

LockManager::LockManager()
{
    InitializeCriticalSection(&_guard);
    memset(_slots, 0, sizeof(_slots));
}

LockManager::~LockManager()
{
    for (int i = 0; i < MAX_LOCK_SLOTS; ++i) {
        if (!_slots[i].inUse) continue;
        DeleteCriticalSection(&_slots[i].cs);
        CloseHandle(_slots[i].canWrite);
        _slots[i].inUse = false;
    }
    DeleteCriticalSection(&_guard);
}

LockManager::Slot* LockManager::GetOrCreate(int recordId)
{
    EnterCriticalSection(&_guard);

    for (int i = 0; i < MAX_LOCK_SLOTS; ++i)
        if (_slots[i].inUse && _slots[i].id == recordId) {
            LeaveCriticalSection(&_guard);
            return &_slots[i];
        }

    for (int i = 0; i < MAX_LOCK_SLOTS; ++i) {
        if (_slots[i].inUse) continue;
        Slot& s = _slots[i];
        s.id = recordId;
        s.inUse = true;
        s.readers = 0;
        s.writing = false;
        InitializeCriticalSection(&s.cs);
        s.canWrite = CreateEvent(NULL, TRUE, TRUE, NULL);
        LeaveCriticalSection(&_guard);
        return &s;
    }

    LeaveCriticalSection(&_guard);
    return NULL;
}

bool LockManager::AcquireRead(int recordId)
{
    Slot* s = GetOrCreate(recordId);
    if (!s) return false;

    while (TRUE) {
        WaitForSingleObject(s->canWrite, INFINITE);

        EnterCriticalSection(&s->cs);
        if (!s->writing) {            
            ++s->readers;
            if (s->readers == 1)
                ResetEvent(s->canWrite);    
            LeaveCriticalSection(&s->cs);
            std::cout << "[Lock] AcquireRead  id=" << recordId
                << "  readers=" << s->readers << "\n";
            return true;
        }
        LeaveCriticalSection(&s->cs);
    }
}

void LockManager::ReleaseRead(int recordId)
{
    Slot* s = GetOrCreate(recordId);
    if (!s) return;

    EnterCriticalSection(&s->cs);
    if (s->readers > 0) --s->readers;
    std::cout << "[Lock] ReleaseRead  id=" << recordId
        << "  readers=" << s->readers << "\n";
    if (s->readers == 0 && !s->writing)
        SetEvent(s->canWrite);
    LeaveCriticalSection(&s->cs);
}

bool LockManager::AcquireWrite(int recordId)
{
    Slot* s = GetOrCreate(recordId);
    if (!s) return false;

    std::cout << "[Lock] AcquireWrite id=" << recordId << "  waiting...\n";

    while (TRUE) {
        WaitForSingleObject(s->canWrite, INFINITE);

        EnterCriticalSection(&s->cs);
        if (s->readers == 0 && !s->writing) {
            s->writing = true;
            ResetEvent(s->canWrite);
            LeaveCriticalSection(&s->cs);
            std::cout << "[Lock] AcquireWrite id=" << recordId << "  ACQUIRED\n";
            return true;
        }
        LeaveCriticalSection(&s->cs);
    }
}

void LockManager::ReleaseWrite(int recordId)
{
    Slot* s = GetOrCreate(recordId);
    if (!s) return;

    EnterCriticalSection(&s->cs);
    s->writing = false;
    SetEvent(s->canWrite);
    std::cout << "[Lock] ReleaseWrite id=" << recordId << "\n";
    LeaveCriticalSection(&s->cs);
}