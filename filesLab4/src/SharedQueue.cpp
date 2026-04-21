#include "headers/sharedQueue.h"
#include <algorithm>

SharedQueue::SharedQueue()
    : hFile_(INVALID_HANDLE_VALUE),
      hMap_(NULL),
      baseView_(NULL),
      mappedSize_(0),
      slots_(NULL),
      hMutex_(NULL),
      hSemEmpty_(NULL),
      hSemFull_(NULL),
      hAllReadyEvent_(NULL)
{
}

SharedQueue::~SharedQueue()
{
    CloseAll();
}

std::string MakeObjectBaseName(const std::string &fileName)
{
    std::string base;
    base.reserve(fileName.size());
    for (size_t i = 0; i < fileName.size(); ++i)
    {
        char c = fileName[i];
        if (c == '\\' || c == '/' || c == ':')
            base.push_back('_');
        else
            base.push_back(c);
    }
    return base;
}

std::string MakeNamedObject(const std::string &base, const std::string &suffix)
{
    return std::string("IPC_") + base + "_" + suffix;
}

void SharedQueue::CloseAll()
{
    if (baseView_)
    {
        UnmapViewOfFile(baseView_);
        baseView_ = NULL;
        slots_ = NULL;
        mappedSize_ = 0;
    }
    if (hMap_)
    {
        CloseHandle(hMap_);
        hMap_ = NULL;
    }
    if (hFile_ != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile_);
        hFile_ = INVALID_HANDLE_VALUE;
    }
    if (hMutex_)
    {
        CloseHandle(hMutex_);
        hMutex_ = NULL;
    }
    if (hSemEmpty_)
    {
        CloseHandle(hSemEmpty_);
        hSemEmpty_ = NULL;
    }
    if (hSemFull_)
    {
        CloseHandle(hSemFull_);
        hSemFull_ = NULL;
    }
    if (hAllReadyEvent_)
    {
        CloseHandle(hAllReadyEvent_);
        hAllReadyEvent_ = NULL;
    }
}

bool SharedQueue::MapFile(const std::string &path, bool create, unsigned int capacity)
{
    DWORD access = GENERIC_READ | GENERIC_WRITE;
    DWORD share = FILE_SHARE_READ | FILE_SHARE_WRITE;
    DWORD disp = create ? CREATE_ALWAYS : OPEN_EXISTING;

    hFile_ = CreateFileA(path.c_str(), access, share, NULL, disp, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile_ == INVALID_HANDLE_VALUE)
    {
        PrintLastErrorA("Create/Open file failed");
        return false;
    }

    DWORD totalSize;
    if (create)
    {
        totalSize = (DWORD)(HEADER_BINARY_SIZE + capacity * sizeof(MessageSlot));
    }
    else
    {
        LARGE_INTEGER sz;
        if (!GetFileSizeEx(hFile_, &sz))
        {
            PrintLastErrorA("GetFileSizeEx");
            return false;
        }
        totalSize = (DWORD)sz.QuadPart;
        if (totalSize < HEADER_BINARY_SIZE)
        {
            printf("[SharedQueue] Invalid file size\n");
            return false;
        }
    }

    hMap_ = CreateFileMappingA(hFile_, NULL, PAGE_READWRITE, 0, totalSize, NULL);
    if (!hMap_)
    {
        PrintLastErrorA("CreateFileMapping");
        return false;
    }

    baseView_ = (unsigned char *)MapViewOfFile(hMap_, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (!baseView_)
    {
        PrintLastErrorA("MapViewOfFile");
        return false;
    }
    mappedSize_ = totalSize;
    slots_ = (MessageSlot *)(baseView_ + HEADER_BINARY_SIZE);
    return true;
}

void SharedQueue::ReadHeader(QueueHeaderBinary &out) const
{
    const unsigned char *p = baseView_;
    const unsigned int *pu = reinterpret_cast<const unsigned int *>(p);
    out.capacity = pu[0];
    out.msgLen = pu[1];
    out.readIndex = pu[2];
    out.writeIndex = pu[3];
    out.senderReadyCount = pu[4];
    out.expectedSenders = pu[5];
    out.shuttingDown = p[24];
    out.reserved[0] = p[25];
    out.reserved[1] = p[26];
    out.reserved[2] = p[27];
}

void SharedQueue::WriteHeader(const QueueHeaderBinary &hdr)
{
    unsigned char *p = baseView_;
    unsigned int *pu = (unsigned int *)p;
    pu[0] = hdr.capacity;
    pu[1] = hdr.msgLen;
    pu[2] = hdr.readIndex;
    pu[3] = hdr.writeIndex;
    pu[4] = hdr.senderReadyCount;
    pu[5] = hdr.expectedSenders;
    p[24] = hdr.shuttingDown;
    p[25] = hdr.reserved[0];
    p[26] = hdr.reserved[1];
    p[27] = hdr.reserved[2];
}

bool SharedQueue::OpenSyncObjects(bool create, unsigned int capacity, unsigned int expectedSenders)
{
    std::string mutexName = MakeNamedObject(baseName_, "mtx");
    std::string semEmptyName = MakeNamedObject(baseName_, "semEmpty");
    std::string semFullName = MakeNamedObject(baseName_, "semFull");
    std::string readyEventName = MakeNamedObject(baseName_, "allReady");

    if (create)
    {
        hMutex_ = CreateMutexA(NULL, FALSE, mutexName.c_str());
        if (!hMutex_)
        {
            PrintLastErrorA("CreateMutex");
            return false;
        }

        hSemEmpty_ = CreateSemaphoreA(NULL, capacity, capacity, semEmptyName.c_str());
        if (!hSemEmpty_)
        {
            PrintLastErrorA("CreateSemaphore empty");
            return false;
        }

        hSemFull_ = CreateSemaphoreA(NULL, 0, capacity, semFullName.c_str());
        if (!hSemFull_)
        {
            PrintLastErrorA("CreateSemaphore full");
            return false;
        }

        hAllReadyEvent_ = CreateEventA(NULL, TRUE, FALSE, readyEventName.c_str());
        if (!hAllReadyEvent_)
        {
            PrintLastErrorA("CreateEvent allReady");
            return false;
        }

        QueueHeaderBinary hdr;
        hdr.capacity = capacity;
        hdr.msgLen = MAX_MESSAGE_LEN;
        hdr.readIndex = 0;
        hdr.writeIndex = 0;
        hdr.senderReadyCount = 0;
        hdr.expectedSenders = expectedSenders;
        hdr.shuttingDown = 0;
        hdr.reserved[0] = hdr.reserved[1] = hdr.reserved[2] = 0;
        WriteHeader(hdr);
    }
    else
    {
        hMutex_ = OpenMutexA(SYNCHRONIZE | MUTEX_MODIFY_STATE, FALSE, mutexName.c_str());
        if (!hMutex_)
        {
            PrintLastErrorA("OpenMutex");
            return false;
        }

        hSemEmpty_ = OpenSemaphoreA(SEMAPHORE_MODIFY_STATE | SYNCHRONIZE, FALSE, semEmptyName.c_str());
        if (!hSemEmpty_)
        {
            PrintLastErrorA("OpenSemaphore empty");
            return false;
        }

        hSemFull_ = OpenSemaphoreA(SEMAPHORE_MODIFY_STATE | SYNCHRONIZE, FALSE, semFullName.c_str());
        if (!hSemFull_)
        {
            PrintLastErrorA("OpenSemaphore full");
            return false;
        }

        hAllReadyEvent_ = OpenEventA(SYNCHRONIZE | EVENT_MODIFY_STATE, FALSE, readyEventName.c_str());
        if (!hAllReadyEvent_)
        {
            PrintLastErrorA("OpenEvent allReady");
            return false;
        }
    }
    return true;
}

bool SharedQueue::CreateAsReceiver(const std::string &fileName,
                                   unsigned int capacity,
                                   unsigned int expectedSenders)
{
    baseName_ = MakeObjectBaseName(fileName);
    if (!MapFile(fileName, true, capacity))
        return false;
    if (!OpenSyncObjects(true, capacity, expectedSenders))
        return false;
    printf("[Receiver] Queue created: capacity=%u, senders=%u\n", capacity, expectedSenders);
    return true;
}

bool SharedQueue::OpenAsSender(const std::string &fileName)
{
    baseName_ = MakeObjectBaseName(fileName);
    if (!MapFile(fileName, false, 0))
        return false;
    if (!OpenSyncObjects(false, 0, 0))
        return false;

    QueueHeaderBinary hdr;
    ReadHeader(hdr);
    if (hdr.msgLen != MAX_MESSAGE_LEN)
    {
        printf("[Sender] Incompatible message length\n");
        return false;
    }
    return true;
}

bool SharedQueue::IsValid() const
{
    return baseView_ != NULL;
}

unsigned int SharedQueue::Capacity() const
{
    QueueHeaderBinary hdr;
    ReadHeader(hdr);
    return hdr.capacity;
}
unsigned int SharedQueue::ReadIndex() const
{
    QueueHeaderBinary hdr;
    ReadHeader(hdr);
    return hdr.readIndex;
}
unsigned int SharedQueue::WriteIndex() const
{
    QueueHeaderBinary hdr;
    ReadHeader(hdr);
    return hdr.writeIndex;
}

bool SharedQueue::WaitAllSendersReady(DWORD timeoutMs)
{
    DWORD res = WaitForSingleObject(hAllReadyEvent_, timeoutMs);
    if (res == WAIT_OBJECT_0)
        return true;
    if (res == WAIT_TIMEOUT)
        printf("[Receiver] Sender readiness wait timeout\n");
    else
        PrintLastErrorA("WaitAllSendersReady");
    return false;
}

bool SharedQueue::SignalSenderReady()
{
    DWORD waitRes = WaitForSingleObject(hMutex_, INFINITE);
    if (waitRes != WAIT_OBJECT_0)
    {
        PrintLastErrorA("Mutex wait SignalSenderReady");
        return false;
    }
    QueueHeaderBinary hdr;
    ReadHeader(hdr);
    hdr.senderReadyCount++;
    if (hdr.senderReadyCount == hdr.expectedSenders)
    {
        SetEvent(hAllReadyEvent_);
    }
    WriteHeader(hdr);
    ReleaseMutex(hMutex_);
    return true;
}

void SharedQueue::SignalShutdown()
{
    if (!baseView_)
        return;
    WaitForSingleObject(hMutex_, INFINITE);
    QueueHeaderBinary hdr;
    ReadHeader(hdr);
    hdr.shuttingDown = 1;
    WriteHeader(hdr);
    ReleaseMutex(hMutex_);

    ReleaseSemaphore(hSemFull_, hdr.capacity, NULL);
    ReleaseSemaphore(hSemEmpty_, hdr.capacity, NULL);
    SetEvent(hAllReadyEvent_);
}

bool SharedQueue::IsShuttingDown() const
{
    QueueHeaderBinary hdr;
    ReadHeader(hdr);
    return hdr.shuttingDown != 0;
}

bool SharedQueue::PopMessage(std::string &outMsg, bool verbose)
{
    outMsg.clear();
    if (!IsValid())
        return false;

    DWORD wr = WaitForSingleObject(hSemFull_, INFINITE);
    if (wr != WAIT_OBJECT_0)
    {
        PrintLastErrorA("Wait semFull");
        return false;
    }

    WaitForSingleObject(hMutex_, INFINITE);

    QueueHeaderBinary hdr;
    ReadHeader(hdr);

    if (hdr.shuttingDown && hdr.readIndex == hdr.writeIndex)
    {
        ReleaseMutex(hMutex_);
        return false;
    }

    unsigned int idx = hdr.readIndex % hdr.capacity;
    const char *src = slots_[idx].data;
    outMsg.assign(src);

    hdr.readIndex = (hdr.readIndex + 1) % hdr.capacity;
    WriteHeader(hdr);

    ReleaseMutex(hMutex_);
    ReleaseSemaphore(hSemEmpty_, 1, NULL);

    if (verbose)
    {
        printf("[Receiver] Read message: '%s'\n", outMsg.c_str());
    }
    return true;
}

bool SharedQueue::PushMessage(const std::string &msg, bool verbose)
{
    if (!IsValid())
        return false;
    if (msg.size() >= MAX_MESSAGE_LEN)
    {
        printf("[Sender] Message too long (>= %u)\n", MAX_MESSAGE_LEN);
        return false;
    }

    DWORD wr = WaitForSingleObject(hSemEmpty_, INFINITE);
    if (wr != WAIT_OBJECT_0)
    {
        PrintLastErrorA("Wait semEmpty");
        return false;
    }

    DWORD mw = WaitForSingleObject(hMutex_, INFINITE);
    if (mw != WAIT_OBJECT_0)
    {
        PrintLastErrorA("Mutex wait push");
        return false;
    }

    QueueHeaderBinary hdr;
    ReadHeader(hdr);
    if (hdr.shuttingDown)
    {
        ReleaseMutex(hMutex_);
        return false;
    }

    unsigned int idx = hdr.writeIndex % hdr.capacity;
    std::fill(slots_[idx].data, slots_[idx].data + MAX_MESSAGE_LEN, '\0');
    std::copy(msg.begin(), msg.end(), slots_[idx].data);

    hdr.writeIndex = (hdr.writeIndex + 1) % hdr.capacity;
    WriteHeader(hdr);

    ReleaseMutex(hMutex_);
    ReleaseSemaphore(hSemFull_, 1, NULL);

    if (verbose)
    {
        printf("[Sender] Sent message: '%s'\n", msg.c_str());
    }
    return true;
}