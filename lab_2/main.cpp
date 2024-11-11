#include <iostream>
#include "BackProcess.h"

int main() {
    std::string command = "echo Hello World";
    auto result = BackgroundProcess::executeProcess(command);

    BackgroundProcess::ProcessInfo info = result.get();
    std::cout << "Process ID: " << info.processID << std::endl;
    std::cout << "Exit Code: " << info.exitCode << std::endl;
    std::cout << "Elapsed Time: " << info.elapsedTime << " ms" << std::endl;

    return 0;
}