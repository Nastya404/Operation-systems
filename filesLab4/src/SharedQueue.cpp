#include "SharedQueue.h"
#include <algorithm>

SharedQueue::SharedQueue()
    : fileHandle(INVALID_HANDLE_VALUE)
    , mappingHandle(NULL)
    , viewPtr(NULL)
    , viewSize(0)
    , messageSlots(NULL)
    , syncMutex(NULL)
    , semFreeSlots(NULL)
    , semUsedSlots(NULL)
    , allReadyEvent(NULL) {
}

SharedQueue::~SharedQueue() {
    releaseResources();
}

static std::string sanitizeFileName(const std::string& fileName) {
    std::string result;
    result.reserve(fileName.size());
    for (size_t i = 0; i < fileName.size(); i++) {
        char ch = fileName[i];
        bool isSeparator = (ch == '\\' || ch == '/' || ch == ':');
        result.push_back(isSeparator ? '_' : ch);
    }
    return result;
}

static std::string buildObjectName(const std::string& baseName, const std::string& suffix) {
    return "IPC_" + baseName + "_" + suffix;
}

void SharedQueue::releaseResources() {
    if (viewPtr != NULL) {
        UnmapViewOfFile(viewPtr);
        viewPtr = NULL;
        messageSlots = NULL;
        viewSize = 0;
    }

    if (mappingHandle != NULL) {
        CloseHandle(mappingHandle);
        mappingHandle = NULL;
    }

    if (fileHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(fileHandle);
        fileHandle = INVALID_HANDLE_VALUE;
    }

    if (syncMutex != NULL) { CloseHandle(syncMutex);    syncMutex = NULL; }
    if (semFreeSlots != NULL) { CloseHandle(semFreeSlots); semFreeSlots = NULL; }
    if (semUsedSlots != NULL) { CloseHandle(semUsedSlots); semUsedSlots = NULL; }
    if (allReadyEvent != NULL) { CloseHandle(allReadyEvent);allReadyEvent = NULL; }
}

bool SharedQueue::openMappedFile(const std::string& path, bool createNew, unsigned int capacity) {
    DWORD disposition = createNew ? CREATE_ALWAYS : OPEN_EXISTING;

    fileHandle = CreateFileA(
        path.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        disposition,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (fileHandle == INVALID_HANDLE_VALUE) {
        PrintLastErrorA("Create/Open file failed");
        return false;
    }

    DWORD totalBytes;

    if (createNew) {
        totalBytes = (DWORD)(HEADER_BINARY_SIZE + capacity * sizeof(MessageSlot));
    }
    else {
        LARGE_INTEGER fileSize;
        if (!GetFileSizeEx(fileHandle, &fileSize)) {
            PrintLastErrorA("GetFileSizeEx");
            return false;
        }
        totalBytes = (DWORD)fileSize.QuadPart;
        if (totalBytes < HEADER_BINARY_SIZE) {
            printf("[SharedQueue] file is too small to contain a valid header\n");
            return false;
        }
    }

    mappingHandle = CreateFileMappingA(fileHandle, NULL, PAGE_READWRITE, 0, totalBytes, NULL);
    if (mappingHandle == NULL) {
        PrintLastErrorA("CreateFileMapping");
        return false;
    }

    viewPtr = (unsigned char*)MapViewOfFile(mappingHandle, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (viewPtr == NULL) {
        PrintLastErrorA("MapViewOfFile");
        return false;
    }

    viewSize = totalBytes;
    messageSlots = (MessageSlot*)(viewPtr + HEADER_BINARY_SIZE);
    return true;
}

void SharedQueue::readHeader(QueueHeader& out) const {
    const unsigned int* fields = reinterpret_cast<const unsigned int*>(viewPtr);
    out.capacity = fields[0];
    out.msgLen = fields[1];
    out.readIndex = fields[2];
    out.writeIndex = fields[3];
    out.senderReadyCount = fields[4];
    out.expectedSenders = fields[5];
    out.shuttingDown = viewPtr[24];
    out.reserved[0] = viewPtr[25];
    out.reserved[1] = viewPtr[26];
    out.reserved[2] = viewPtr[27];
}

void SharedQueue::writeHeader(const QueueHeader& hdr) {
    unsigned int* fields = reinterpret_cast<unsigned int*>(viewPtr);
    fields[0] = hdr.capacity;
    fields[1] = hdr.msgLen;
    fields[2] = hdr.readIndex;
    fields[3] = hdr.writeIndex;
    fields[4] = hdr.senderReadyCount;
    fields[5] = hdr.expectedSenders;
    viewPtr[24] = hdr.shuttingDown;
    viewPtr[25] = hdr.reserved[0];
    viewPtr[26] = hdr.reserved[1];
    viewPtr[27] = hdr.reserved[2];
}

bool SharedQueue::initSyncObjects(bool createNew, unsigned int capacity, unsigned int expectedSenders) {
    std::string mutexName = buildObjectName(baseName, "mtx");
    std::string emptyName = buildObjectName(baseName, "semEmpty");
    std::string fullName = buildObjectName(baseName, "semFull");
    std::string eventName = buildObjectName(baseName, "allReady");

    if (createNew) {
        syncMutex = CreateMutexA(NULL, FALSE, mutexName.c_str());
        if (!syncMutex) { PrintLastErrorA("CreateMutex"); return false; }

        semFreeSlots = CreateSemaphoreA(NULL, capacity, capacity, emptyName.c_str());
        if (!semFreeSlots) { PrintLastErrorA("CreateSemaphore empty"); return false; }

        semUsedSlots = CreateSemaphoreA(NULL, 0, capacity, fullName.c_str());
        if (!semUsedSlots) { PrintLastErrorA("CreateSemaphore full"); return false; }

        allReadyEvent = CreateEventA(NULL, TRUE, FALSE, eventName.c_str());
        if (!allReadyEvent) { PrintLastErrorA("CreateEvent allReady"); return false; }

        QueueHeader hdr;
        hdr.capacity = capacity;
        hdr.msgLen = MAX_MESSAGE_LEN;
        hdr.readIndex = 0;
        hdr.writeIndex = 0;
        hdr.senderReadyCount = 0;
        hdr.expectedSenders = expectedSenders;
        hdr.shuttingDown = 0;
        hdr.reserved[0] = hdr.reserved[1] = hdr.reserved[2] = 0;
        writeHeader(hdr);
    }
    else {
        syncMutex = OpenMutexA(SYNCHRONIZE | MUTEX_MODIFY_STATE, FALSE, mutexName.c_str());
        if (!syncMutex) { PrintLastErrorA("OpenMutex"); return false; }

        semFreeSlots = OpenSemaphoreA(SEMAPHORE_MODIFY_STATE | SYNCHRONIZE, FALSE, emptyName.c_str());
        if (!semFreeSlots) { PrintLastErrorA("OpenSemaphore empty"); return false; }

        semUsedSlots = OpenSemaphoreA(SEMAPHORE_MODIFY_STATE | SYNCHRONIZE, FALSE, fullName.c_str());
        if (!semUsedSlots) { PrintLastErrorA("OpenSemaphore full"); return false; }

        allReadyEvent = OpenEventA(SYNCHRONIZE | EVENT_MODIFY_STATE, FALSE, eventName.c_str());
        if (!allReadyEvent) { PrintLastErrorA("OpenEvent allReady"); return false; }
    }

    return true;
}

bool SharedQueue::CreateAsReceiver(const std::string& fileName,
    unsigned int capacity,
    unsigned int expectedSenders) {
    baseName = sanitizeFileName(fileName);

    if (!openMappedFile(fileName, true, capacity))
        return false;

    if (!initSyncObjects(true, capacity, expectedSenders))
        return false;

    printf("[Receiver] queue ready: capacity=%u, expected senders=%u\n",
        capacity, expectedSenders);
    return true;
}

bool SharedQueue::OpenAsSender(const std::string& fileName) {
    baseName = sanitizeFileName(fileName);

    if (!openMappedFile(fileName, false, 0))
        return false;

    if (!initSyncObjects(false, 0, 0))
        return false;

    QueueHeader hdr;
    readHeader(hdr);
    if (hdr.msgLen != MAX_MESSAGE_LEN) {
        printf("[Sender] message length mismatch: expected %u, got %u\n",
            MAX_MESSAGE_LEN, hdr.msgLen);
        return false;
    }

    return true;
}

bool SharedQueue::IsValid() const {
    return viewPtr != NULL;
}

unsigned int SharedQueue::Capacity() const {
    QueueHeader hdr;
    readHeader(hdr);
    return hdr.capacity;
}

unsigned int SharedQueue::ReadIndex() const {
    QueueHeader hdr;
    readHeader(hdr);
    return hdr.readIndex;
}

unsigned int SharedQueue::WriteIndex() const {
    QueueHeader hdr;
    readHeader(hdr);
    return hdr.writeIndex;
}

bool SharedQueue::WaitAllSendersReady(DWORD timeoutMs) {
    DWORD result = WaitForSingleObject(allReadyEvent, timeoutMs);

    if (result == WAIT_OBJECT_0)
        return true;

    if (result == WAIT_TIMEOUT)
        printf("[Receiver] timed out waiting for senders\n");
    else
        PrintLastErrorA("WaitAllSendersReady");

    return false;
}

bool SharedQueue::SignalSenderReady() {
    if (WaitForSingleObject(syncMutex, INFINITE) != WAIT_OBJECT_0) {
        PrintLastErrorA("Mutex wait in SignalSenderReady");
        return false;
    }

    QueueHeader hdr;
    readHeader(hdr);

    hdr.senderReadyCount++;
    if (hdr.senderReadyCount == hdr.expectedSenders)
        SetEvent(allReadyEvent);

    writeHeader(hdr);
    ReleaseMutex(syncMutex);
    return true;
}

void SharedQueue::SignalShutdown() {
    if (viewPtr == NULL)
        return;

    WaitForSingleObject(syncMutex, INFINITE);

    QueueHeader hdr;
    readHeader(hdr);
    hdr.shuttingDown = 1;
    writeHeader(hdr);

    unsigned int cap = hdr.capacity;
    ReleaseMutex(syncMutex);

    for (unsigned int i = 0; i < cap; i++) {
        ReleaseSemaphore(semUsedSlots, 1, NULL);
        ReleaseSemaphore(semFreeSlots, 1, NULL);
    }

    SetEvent(allReadyEvent);
}

bool SharedQueue::IsShuttingDown() const {
    QueueHeader hdr;
    readHeader(hdr);
    return hdr.shuttingDown != 0;
}

bool SharedQueue::PopMessage(std::string& outMsg, bool verbose) {
    outMsg.clear();

    if (!IsValid())
        return false;

    DWORD waitResult = WaitForSingleObject(semUsedSlots, INFINITE);
    if (waitResult != WAIT_OBJECT_0) {
        PrintLastErrorA("WaitForSingleObject semUsedSlots");
        return false;
    }

    WaitForSingleObject(syncMutex, INFINITE);

    QueueHeader hdr;
    readHeader(hdr);

    if (hdr.shuttingDown && hdr.readIndex == hdr.writeIndex) {
        std::cout << "No messages";
        ReleaseMutex(syncMutex);
        return false;
    }

    unsigned int slotIndex = hdr.readIndex % hdr.capacity;
    outMsg.assign(messageSlots[slotIndex].data);

    hdr.readIndex = (hdr.readIndex + 1) % hdr.capacity;
    writeHeader(hdr);

    ReleaseMutex(syncMutex);
    ReleaseSemaphore(semFreeSlots, 1, NULL);

    if (verbose)
        printf("[Receiver] got message: '%s'\n", outMsg.c_str());

    return true;
}

bool SharedQueue::PushMessage(const std::string& msg, bool verbose) {
    if (!IsValid())
        return false;

    if (msg.size() >= MAX_MESSAGE_LEN) {
        printf("[Sender] message too long (max %u characters)\n", MAX_MESSAGE_LEN);
        return false;
    }

    for (;;) {
        DWORD waitResult = WaitForSingleObject(semFreeSlots, 200);

        if (waitResult == WAIT_OBJECT_0)
            break;

        if (waitResult == WAIT_TIMEOUT) {
            if (IsShuttingDown())
                return false;
            continue;
        }

        PrintLastErrorA("WaitForSingleObject semFreeSlots");
        return false;
    }

    if (WaitForSingleObject(syncMutex, INFINITE) != WAIT_OBJECT_0) {
        PrintLastErrorA("Mutex wait in PushMessage");
        return false;
    }

    QueueHeader hdr;
    readHeader(hdr);

    if (hdr.shuttingDown) {
        ReleaseMutex(syncMutex);
        return false;
    }

    unsigned int slotIndex = hdr.writeIndex % hdr.capacity;
    std::fill(messageSlots[slotIndex].data,
        messageSlots[slotIndex].data + MAX_MESSAGE_LEN,
        '\0');
    std::copy(msg.begin(), msg.end(), messageSlots[slotIndex].data);

    hdr.writeIndex = (hdr.writeIndex + 1) % hdr.capacity;
    writeHeader(hdr);

    ReleaseMutex(syncMutex);
    ReleaseSemaphore(semUsedSlots, 1, NULL);

    if (verbose)
        printf("[Sender] sent message: '%s'\n", msg.c_str());

    return true;
}