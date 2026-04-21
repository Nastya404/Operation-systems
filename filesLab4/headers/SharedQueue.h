#ifndef SHAREDQUEUE_H
#define SHAREDQUEUE_H

#include <windows.h>
#include <string>
#include <iostream>
#include <stdio.h>

static const unsigned int MAX_MESSAGE_LEN = 20;
static const size_t HEADER_BINARY_SIZE = 28;

struct QueueHeader {
    unsigned int capacity;
    unsigned int msgLen;
    unsigned int readIndex;
    unsigned int writeIndex;
    unsigned int senderReadyCount;
    unsigned int expectedSenders;
    unsigned char shuttingDown;
    unsigned char reserved[3];
};

struct MessageSlot {
    char data[MAX_MESSAGE_LEN];
};

static std::string sanitizeFileName(const std::string& fileName);
static std::string buildObjectName(const std::string& base, const std::string& suffix);

inline void PrintLastErrorA(const char* msg) {
    DWORD e = GetLastError();
    LPSTR buf = NULL;
    FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, e,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&buf, 0, NULL
    );
    printf("%s (code=%lu): %s\n", msg, (unsigned long)e, buf ? buf : "(no message)");
    if (buf) LocalFree(buf);
}

class SharedQueue {
public:
    SharedQueue();
    ~SharedQueue();

    bool CreateAsReceiver(const std::string& fileName,
        unsigned int capacity,
        unsigned int expectedSenders);
    bool OpenAsSender(const std::string& fileName);

    bool IsValid() const;
    bool PopMessage(std::string& outMsg, bool verbose);
    bool PushMessage(const std::string& msg, bool verbose);
    bool WaitAllSendersReady(DWORD timeoutMs);
    bool SignalSenderReady();
    void SignalShutdown();
    bool IsShuttingDown() const;

    unsigned int Capacity() const;
    unsigned int ReadIndex() const;
    unsigned int WriteIndex() const;

private:
    HANDLE fileHandle;
    HANDLE mappingHandle;
    unsigned char* viewPtr;
    size_t viewSize;
    MessageSlot* messageSlots;
    HANDLE syncMutex;
    HANDLE semFreeSlots;
    HANDLE semUsedSlots;
    HANDLE allReadyEvent;
    std::string baseName;

    void readHeader(QueueHeader& out) const;
    void writeHeader(const QueueHeader& hdr);
    bool openMappedFile(const std::string& path, bool createNew, unsigned int capacity);
    bool initSyncObjects(bool createNew, unsigned int capacity, unsigned int expectedSenders);
    void releaseResources();
};

#endif