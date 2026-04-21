#include "SharedQueue.h"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    std::cout << "=== Sender ===\n";

    if (argc < 2) {
        std::cout << "Usage: sender.exe <fileName>\n";
        return 1;
    }

    std::string fileName = argv[1];

    SharedQueue queue;
    if (!queue.OpenAsSender(fileName)) {
        std::cerr << "[Sender] failed to open the message queue.\n";
        return 1;
    }

    if (!queue.SignalSenderReady()) {
        std::cerr << "[Sender] failed to signal readiness.\n";
        return 1;
    }

    std::cout << "[Sender] ready. Commands: s - send message, q - quit.\n";

    std::string command;
    while (std::getline(std::cin, command)) {
        if (command == "s") {
            if (queue.IsShuttingDown()) {
                std::cout << "[Sender] receiver has shut down. Exiting.\n";
                break;
            }

            std::cout << "Input message (< " << MAX_MESSAGE_LEN << " symbols): ";
            std::string msg;
            if (!std::getline(std::cin, msg))
                break;

            if (msg.size() >= MAX_MESSAGE_LEN) {
                std::cout << "[Sender] message is too long, try again.\n";
                continue;
            }

            if (!queue.PushMessage(msg, true)) {
                std::cout << "[Sender] failed to send message.\n";
                if (queue.IsShuttingDown())
                    break;
            }
        }
        else if (command == "q") {
            std::cout << "[Sender] exiting.\n";
            break;
        }
        else {
            std::cout << "Unknown command. Use s or q.\n";
        }

        std::cout << "[Sender] command (s/q): ";
    }

    return 0;
}