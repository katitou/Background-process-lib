#include <iostream>
#include <chrono>
#include <thread>
#include <future>
#include <string>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/wait.h>   
#endif

namespace BackgroundProcess {

    struct ProcessInfo {
        int processID;
        int exitCode;
        double elapsedTime; 
    };

    std::future<ProcessInfo> executeProcess(const std::string& command) {
        std::promise<ProcessInfo> promise;
        auto future = promise.get_future();

#ifdef _WIN32
        std::thread([command, promise = std::move(promise)]() mutable {
            STARTUPINFO si = { sizeof(STARTUPINFO) };
            PROCESS_INFORMATION pi = {};
            auto start = std::chrono::high_resolution_clock::now();

            if (CreateProcess(NULL, const_cast<char*>(command.c_str()), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
                DWORD processID = GetProcessId(pi.hProcess);
                WaitForSingleObject(pi.hProcess, INFINITE);

                DWORD exitCode;
                GetExitCodeProcess(pi.hProcess, &exitCode);
                auto end = std::chrono::high_resolution_clock::now();
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);

                std::chrono::duration<double, std::milli> elapsed = end - start;
                ProcessInfo info = {static_cast<int>(processID), static_cast<int>(exitCode), elapsed.count()};
                promise.set_value(info);
            } else {
                promise.set_value(ProcessInfo{-1, -1, 0.0}); 
            }
        }).detach();
#else
        std::thread([command, promise = std::move(promise)]() mutable {
            auto start = std::chrono::high_resolution_clock::now();
            pid_t processID = fork();

            if (processID == 0) { 
                execl("/bin/sh", "sh", "-c", command.c_str(), (char*)0);
                exit(EXIT_FAILURE); 
            } else if (processID > 0) { 
                int status;
                waitpid(processID, &status, 0);
                auto end = std::chrono::high_resolution_clock::now();

                std::chrono::duration<double, std::milli> elapsed = end - start;
                int exitCode = (WIFEXITED(status)) ? WEXITSTATUS(status) : -1;
                ProcessInfo info = {static_cast<int>(processID), exitCode, elapsed.count()};
                promise.set_value(info);
            } else {
                promise.set_value(ProcessInfo{-1, -1, 0.0}); 
            }
        }).detach();
#endif
        return future;
    }
}