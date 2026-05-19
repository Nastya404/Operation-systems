#pragma once
#include <windows.h>

#define MAX_LOCK_SLOTS 128

class LockManager {
public:
    LockManager();
    ~LockManager();

    bool AcquireRead(int recordId);
    bool AcquireWrite(int recordId);
    void ReleaseRead(int recordId);
    void ReleaseWrite(int recordId);

private:
    struct Slot {
        int id = 0;
        bool inUse = false;
        int readers = 0;
        bool writing = false;
        CRITICAL_SECTION cs;        // защищает readers / writing
        HANDLE canWrite;  // manual-reset Event
    };

    Slot* GetOrCreate(int recordId);

    CRITICAL_SECTION _guard;            // защищает поиск/создание слотов
    Slot             _slots[MAX_LOCK_SLOTS];
};