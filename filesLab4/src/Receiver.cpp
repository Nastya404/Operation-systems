#include "SharedQueue.h"
#include <iostream>
#include <string>
#include <vector>
#include <thread>

static unsigned int parseUnsigned(const std::string& s) {
    return static_cast<unsigned int>(std::stoi(s));
}

int main() {
    std::cout << "=== Receiver ===\n";

    std::cout << "Input binary file name: ";
    std::string fileName;
    std::getline(std::cin, fileName);

    std::cout << "Input records count (capacity): ";
    std::string tmp;
    std::getline(std::cin, tmp);
    unsigned int capacity = parseUnsigned(tmp);

    std::cout << "Input desired Sender process count: ";
    std::getline(std::cin, tmp);
    unsigned int senderCount = parseUnsigned(tmp);

    if (capacity == 0 || senderCount == 0) {
        std::cerr << "Capacity and sender count must be > 0.\n";
        return 1;
    }

    SharedQueue queue;
    if (!queue.CreateAsReceiver(fileName, capacity, senderCount)) {
        std::cerr << "Failed to initialize the message queue.\n";
        return 1;
    }

    char selfPath[MAX_PATH];
    GetModuleFileNameA(NULL, selfPath, MAX_PATH);
    std::string exeDir = selfPath;
    size_t lastSlash = exeDir.find_last_of("\\/");
    if (lastSlash != std::string::npos)
        exeDir = exeDir.substr(0, lastSlash + 1);

    std::cout << "Launching " << senderCount << " sender process(es)...\n";

    std::vector<HANDLE> senderProcesses;

    for (unsigned int i = 0; i < senderCount; i++) {
        std::string cmdLine = "\"" + exeDir + "sender.exe\" \"" + fileName + "\"";

        STARTUPINFOA si;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);

        PROCESS_INFORMATION pi;
        ZeroMemory(&pi, sizeof(pi));

        std::vector<char> buf(cmdLine.begin(), cmdLine.end());
        buf.push_back('\0');

        BOOL ok = CreateProcessA(
            NULL, buf.data(),
            NULL, NULL,
            FALSE, CREATE_NEW_CONSOLE,
            NULL, NULL,
            &si, &pi
        );

        if (!ok) {
            PrintLastErrorA("CreateProcess");
        }
        else {
            CloseHandle(pi.hThread);
            senderProcesses.push_back(pi.hProcess);
        }
    }

    std::cout << "Waiting for all senders to become ready...\n";
    queue.WaitAllSendersReady(INFINITE);
    std::cout << "All senders ready. Commands: r - read message, q - quit.\n";
    std::cout << "[Receiver] command (r/q): ";

    std::thread watcher([&senderProcesses, &queue]() {
        if (senderProcesses.empty()) return;

        WaitForMultipleObjects(
            (DWORD)senderProcesses.size(),
            senderProcesses.data(),
            TRUE,
            INFINITE
        );

        if (!queue.IsShuttingDown()) {
            std::cout << "\n[Receiver] all senders have exited. Shutting down.\n";
            queue.SignalShutdown();
            CloseHandle(GetStdHandle(STD_INPUT_HANDLE));
        }
        });
    watcher.detach();

    std::string command;
    while (std::getline(std::cin, command)) {
        if (command == "r") {
            std::string msg;
            bool ok = queue.PopMessage(msg, true);
            if (!ok) {
                if (queue.IsShuttingDown()) {
                    std::cout << "[Receiver] shutting down.\n";
                    break;
                }
                std::cout << "[Receiver] no messages available.\n";
            }
        }
        else if (command == "q") {
            std::cout << "[Receiver] sending shutdown signal.\n";
            queue.SignalShutdown();
            break;
        }
        else {
            std::cout << "Unknown command. Use r or q.\n";
        }

        if (queue.IsShuttingDown()) break;

        std::cout << "[Receiver] command (r/q): ";
    }

    for (size_t i = 0; i < senderProcesses.size(); i++)
        CloseHandle(senderProcesses[i]);

    return 0;
}