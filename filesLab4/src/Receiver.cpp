#include "headers/sharedQueue.h"
#include <iostream>
#include <string>
#include <vector>

static unsigned int to_uint(const std::string &s)
{
    return static_cast<unsigned int>(atoi(s.c_str()));
}

int main()
{
    std::cout << "=== Receiver ===\n";

    std::cout << "Input binary file name: ";
    std::string fileName;
    std::getline(std::cin, fileName);

    std::cout << "Input records count (capacity): ";
    std::string tmp;
    std::getline(std::cin, tmp);
    unsigned int capacity = to_uint(tmp);

    std::cout << "Input desired Sender process count: ";
    std::getline(std::cin, tmp);
    unsigned int senderCount = to_uint(tmp);

    if (capacity == 0 || senderCount == 0)
    {
        std::cerr << "Capacity and sender count must be > 0.\n";
        return 1;
    }

    SharedQueue queue;
    if (!queue.CreateAsReceiver(fileName, capacity, senderCount))
    {
        std::cerr << "Error creating queue.\n";
        return 1;
    }

    std::cout << "Launching " << senderCount << " instance(s) of sender process...\n";
    for (unsigned int i = 0; i < senderCount; ++i)
    {
        std::string cmd = "\".\\sender.exe\" \"" + fileName + "\"";

        STARTUPINFOA si;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        PROCESS_INFORMATION pi;
        ZeroMemory(&pi, sizeof(pi));

        std::vector<char> buf(cmd.begin(), cmd.end());
        buf.push_back('\0');

        if (!CreateProcessA(NULL, &buf[0], NULL, NULL, FALSE,
                            CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi))
        {
            PrintLastErrorA("CreateProcess sender");
        }
        else
        {
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
        }
    }

    std::cout << "Waiting for all Senders to be ready...\n";
    queue.WaitAllSendersReady(INFINITE);
    std::cout << "All Senders ready. Commands: r - read, q - quit.\n";

    while (true)
    {
        std::cout << "[Receiver] Command (r/q): ";
        std::string cmdLine;
        if (!std::getline(std::cin, cmdLine))
            break;

        if (cmdLine == "r")
        {
            std::string msg;
            if (!queue.PopMessage(msg, true))
            {
                if (queue.IsShuttingDown())
                    std::cout << "[Receiver] Quitting...\n";
                else
                    std::cout << "[Receiver] No messages\n";
            }
        }
        else if (cmdLine == "q")
        {
            std::cout << "[Receiver] Finishing. shutdown signal.\n";
            queue.SignalShutdown();
            break;
        }
        else
        {
            std::cout << "Unknown command.\n";
        }
    }
    return 0;
}