#include "headers/sharedQueue.h"
#include <iostream>
#include <string>

int main(int argc, char *argv[])
{
    std::cout << "=== Sender ===\n";
    if (argc < 2)
    {
        std::cout << "Usage: sender.exe <fileName>\n";
        return 1;
    }
    std::string fileName = argv[1];

    SharedQueue queue;
    if (!queue.OpenAsSender(fileName))
    {
        std::cerr << "[Sender] Can't open queue.\n";
        return 1;
    }

    if (!queue.SignalSenderReady())
    {
        std::cerr << "[Sender] Can't send ready signal.\n";
        return 1;
    }
    std::cout << "[Sender] Ready. Commands: s - send, q - quit.\n";

    while (true)
    {
        std::cout << "[Sender] Command (s/q): ";
        std::string cmdLine;
        if (!std::getline(std::cin, cmdLine))
            break;
        if (cmdLine == "s")
        {
            if (queue.IsShuttingDown())
            {
                std::cout << "[Sender] Receiver finishes. Quitting.\n";
                break;
            }
            std::cout << "Input message (<20 symbols): ";
            std::string msg;
            if (!std::getline(std::cin, msg))
                break;
            if (msg.size() >= MAX_MESSAGE_LEN)
            {
                std::cout << "[Sender] Message too long.\n";
                continue;
            }
            if (!queue.PushMessage(msg, true))
            {
                std::cout << "[Sender] Error.\n";
                if (queue.IsShuttingDown())
                    break;
            }
        }
        else if (cmdLine == "q")
        {
            std::cout << "[Sender] Quitting.\n";
            break;
        }
        else
        {
            std::cout << "Unknown commands.\n";
        }
    }
    return 0;
}