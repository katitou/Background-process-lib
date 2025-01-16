#include <atomic>
#include <cstddef>
#include <iostream>
#include <chrono>
#include <ostream>
#include <thread>
#include <mutex>
#include <cstring>
#include "logger.h"
#include <iomanip>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/mman.h>
#endif

constexpr const char* WINDOWS_PROGRAMM_NAME = "app.exe";
constexpr const char* SHARED_MEMORY_NAME = "MY_SHARED_MEMORY";
constexpr const size_t SHARED_MEMORY_SIZE = sizeof(int);
bool CREATE_SHM = false;
constexpr const char* LOCK_FILE = "file.lock";

// int counter = 0;
std::string SEMAPHORE = "main_process_semaphore";
std::mutex mtx;
std::mutex processMutex;

struct SharedCounter {
  int counter;
};

SharedCounter* createSharedMemory(const char* name, bool create) {
#ifdef _WIN32
    // Для Windows
    HANDLE hMapFile;
    if (create) {
        hMapFile = CreateFileMapping(
            INVALID_HANDLE_VALUE,    // Используем файл подкачки
            NULL,                    // Атрибуты безопасности по умолчанию
            PAGE_READWRITE,          // Доступ на чтение и запись
            0,                       // Размер старшего слова
            sizeof(SharedCounter),      // Размер младшего слова
            name                     // Имя разделяемой памяти
        );
    } else {
        hMapFile = OpenFileMapping(
            FILE_MAP_ALL_ACCESS,     // Доступ на чтение и запись
            FALSE,                   // Наследование не требуется
            name                     // Имя разделяемой памяти
        );
    }

    if (hMapFile == NULL) {
        std::cerr << "Could not create/open shared memory." << std::endl;
        return nullptr;
    }

    // Отображаем память в адресное пространство процесса
    SharedCounter* data = (SharedCounter*)MapViewOfFile(
        hMapFile,                   // Дескриптор объекта
        FILE_MAP_ALL_ACCESS,        // Доступ на чтение и запись
        0, 0, sizeof(SharedCounter)
    );

    if (data == nullptr) {
        std::cerr << "Could not map view of file." << std::endl;
        CloseHandle(hMapFile);
        return nullptr;
    }

    return data;

#else
    // Для Linux/Unix
    int shm_fd;
    if (create) {
        shm_fd = shm_open(name, O_CREAT | O_RDWR, 0666);
        if (shm_fd == -1) {
            std::cerr << "Failed to create shared memory." << std::endl;
            return nullptr;
        }
        ftruncate(shm_fd, sizeof(SharedCounter));
    } else {
        shm_fd = shm_open(name, O_RDWR, 0666);
        if (shm_fd == -1) {
            std::cerr << "Failed to open shared memory." << std::endl;
            return nullptr;
        }
    }

    // Отображаем память в адресное пространство процесса
    SharedCounter* data = (SharedCounter*)mmap(
        nullptr, sizeof(SharedCounter),
        PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0
    );

    if (data == MAP_FAILED) {
        std::cerr << "Failed to map shared memory." << std::endl;
        close(shm_fd);
        return nullptr;
    }

    return data;
#endif
}


void cleanupSharedMemory() {
#ifdef _WIN32
    UnmapViewOfFile(SHARED_MEMORY_NAME);
#else
    shm_unlink(SHARED_MEMORY_NAME);
#endif
}

bool isMainProcess(const std::string& lock_file) {
#if defined(_WIN32) || defined(_WIN64)
    // Windows: используем CreateFile для создания/блокировки файла
    HANDLE file_handle = CreateFile(
        lock_file.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (file_handle == INVALID_HANDLE_VALUE) {
        return false; // Не удалось получить доступ к файлу
    }

    static HANDLE lock_handle = file_handle; // Сохраняем handle, чтобы не закрывать
    return true;

#else
    // Linux/Unix: используем файловую блокировку через open и lockf
    int fd = open(lock_file.c_str(), O_RDWR | O_CREAT, 0666);
    if (fd == -1) {
        return false; // Не удалось открыть файл
    }

    if (lockf(fd, F_TLOCK, 0) == -1) {
        close(fd); // Уже заблокирован, это не главный процесс
        return false;
    }

    static int lock_fd = fd; // Сохраняем fd, чтобы не закрывать
    return true;
#endif
}

void setCounter(int newValue) {
  std::lock_guard<std::mutex> lock(mtx);
  
  SharedCounter* sharedCounter = createSharedMemory(SHARED_MEMORY_NAME, CREATE_SHM);
  sharedCounter->counter = newValue;
  std::cout << "Counter value changed to " << sharedCounter->counter << std::endl;
}

unsigned int getPid() {
#ifdef _WIN32
  return static_cast<unsigned int>(GetCurrentProcessId());
#else
  return static_cast<unsigned int>(getpid());
#endif
}

auto getTime() {
  std::chrono::system_clock::time_point startTime = std::chrono::system_clock::now();
  std::time_t startTimeT = std::chrono::system_clock::to_time_t(startTime);

  return std::put_time(std::localtime(&startTimeT), "%Y-%m-%d %H:%M:%S");
}

void logStartupInfo() { 
  
  Logger::get() << "Process ID: " << getPid() << " started at: " << getTime() << std::endl;

}

void incrementCountTimer() {
  SharedCounter* shared = createSharedMemory(SHARED_MEMORY_NAME, CREATE_SHM);
  for (int i = 0; true; i++) {
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    shared->counter += 1;
  }

}

void logInfoTimer() {
  SharedCounter* shared = createSharedMemory(SHARED_MEMORY_NAME, CREATE_SHM);
  while (true) {
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    Logger::get() << "Process id: " << getPid() << " Time: " << getTime() << " Counter " << shared->counter << std::endl;
  }
}

void tui() {
  SharedCounter* shared = createSharedMemory(SHARED_MEMORY_NAME, CREATE_SHM);

  while(true) {
    int newValue;
    std::cout << "Enter counter value: ";
    std::cin >> newValue;

    if (newValue < 0) exit(0);

    Logger::get() << "Counter value changed from " << shared->counter << " to ";
  
    setCounter(newValue);

    Logger::get() << shared->counter << std::endl;
  }
}

void launchCopy1() { 
  std::lock_guard<std::mutex> lock(processMutex);
  SharedCounter* shared = createSharedMemory(SHARED_MEMORY_NAME, CREATE_SHM);
  Logger::get() << "Process id: " << getPid() << " Started at: " << getTime() << std::endl;

  shared->counter += 10;

  Logger::get() << "Process id: " << getPid() << " Exited on: " << getTime() << std::endl;

  exit(0);
}

void launchCopy2() {
  std::lock_guard<std::mutex> lock(processMutex);
  SharedCounter* shared = createSharedMemory(SHARED_MEMORY_NAME, CREATE_SHM);
  Logger::get() << "Process id: " << getPid() << " Started at: " << getTime() << std::endl;

  shared->counter *= 2;

  std::this_thread::sleep_for(std::chrono::seconds(2));

  shared->counter /= 2;

  Logger::get() << "Process id: " << getPid() << " Exited on: " << getTime() << std::endl;

  exit(0);

}

bool run_process_and_wait(const std::string& program, const std::string& argument, int& exit_code) {
#ifdef _WIN32
    STARTUPINFOA si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(si);

    std::string command = program + " " + argument;

    if (!CreateProcessA(
            NULL, const_cast<char*>(command.c_str()), NULL, NULL, FALSE,
            CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        return false;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD code;
    if (GetExitCodeProcess(pi.hProcess, &code)) {
        exit_code = static_cast<int>(code);
    } else {
        exit_code = -1;
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return true;
#else
    pid_t pid = fork();
    if (pid < 0) {
        return false;
    } else if (pid == 0) {
        execl(program.c_str(), program.c_str(), argument.c_str(), NULL);
        _exit(EXIT_FAILURE);
    }

    int status;
    if (waitpid(pid, &status, 0) == -1) {
        return false;
    }

    if (WIFEXITED(status)) {
        exit_code = WEXITSTATUS(status);
    } else {
        exit_code = -1;
    }

    return true;
#endif
}

void monitorProcesses() {
    std::vector<bool> copy_status = {true, true};
    std::mutex status_mutex;

    while (true) {
        for (int i = 0; i < 2; ++i) {
            std::lock_guard<std::mutex> lock(status_mutex); // Блокируем доступ к copy_status
            if (copy_status[i]) {
                copy_status[i] = false; // Устанавливаем статус "занят"

                std::thread([&copy_status, i, &status_mutex]() {
                    int exitCode;
                    std::string argument = (i == 0) ? "copy1" : "copy2";

                    bool success = run_process_and_wait("./app", argument, exitCode);


                    {
                        std::lock_guard<std::mutex> lock(status_mutex);
                        copy_status[i] = true; // Освобождаем копию
                    }

                    if (!success) {
                        Logger::get() << "Process " << argument << " failed with code " << std::to_string(exitCode) << std::endl;
                    }
                }).detach();

            } else {
                Logger::get() << "Copy " << std::to_string(i + 1) << " is running. Continue" << std::endl;
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(3));
    }
}

  
int main(int argc, char* argv[]) {
  
  bool isMain = isMainProcess(LOCK_FILE);
  
  SharedCounter* sharedCounter = createSharedMemory(SHARED_MEMORY_NAME, isMain);

  if (!sharedCounter) {
        std::cerr << "Shared Memory Error" << std::endl;
        return 1;
  }  
  
  // пункт 5
  // Если запускается копия, то будет вызывана только соответсвующая функция
  if (argc > 1) { 

    std::string mode = argv[1];
    // std::cout << argv[1] << std::endl; 
    if (mode == "copy1") {
      launchCopy1();

      Logger::close();
      cleanupSharedMemory();

      std::cout << "Exit copy1 with code 0" << std::endl;

      return 0;
    } else if (mode == "copy2") {
      launchCopy2();

      Logger::close();
      cleanupSharedMemory();

      std::cout << "Exit copy2 with code 0" << std::endl;
    
      return 0;
    }

 }

  Logger::get() << "Process ID: " << getPid() << " started at: " << getTime() << std::endl;
  
  std::thread incrementCountTimerThred(incrementCountTimer);
  std::thread tuiThread(tui); 

  if (isMain) {
    std::thread logInfoTimerThred(logInfoTimer);
    std::thread monitorProcessesThread(monitorProcesses);
    logInfoTimerThred.detach();
    monitorProcessesThread.detach();
  }
  incrementCountTimerThred.detach();
  tuiThread.join();
  
  Logger::close();

  if (isMain) {
    cleanupSharedMemory();
  }

  return 0;
}
