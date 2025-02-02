#include <iostream>
#include <fstream>
#include <chrono>
#include <ctime>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <sstream>
#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif
#include "logger.h"

std::string LOG_FILE = "log.txt";
std::string AVERAGE_PER_HOUR = "av_per_hour.txt";
std::string AVERAGE_PER_DAY = "av_per_day.hour.txt";
#ifdef _WIN32
std::string USB_PORT = "\\\\.\\COM3";
#else
std::string USB_PORT = "/dev/pts/2";
#endif
int SECONDS_IN_HOUR = 3600;
int SECONDS_IN_DAY = SECONDS_IN_HOUR * 24;
int SECONDS_IN_MONTH = SECONDS_IN_DAY * 30;
int SECONDS_IN_YEAR = SECONDS_IN_DAY * 365;

std::mutex usbPortMutex;
std::mutex deleteMutex;
std::mutex getMutex;

struct TemperatureData {
  int temperature;
  unsigned int timestemp; 
};

std::vector<TemperatureData> getDataFromLogFile(std::string filename, int lastSeconds) {
  std::lock_guard<std::mutex> lock(getMutex);

  std::vector<TemperatureData> dataList;
  std::ifstream file(filename, std::ios::in);
  
  if (!file.is_open()) {
    std::cout << "Failed to open file " << filename << std::endl;

    return dataList;
  }

  TemperatureData data; 
  unsigned int currentTime = static_cast<unsigned int>(std::time(nullptr));

  while (file >> data.temperature >> data.timestemp) {
    if (data.timestemp < currentTime - lastSeconds) {
      continue;
    }
    dataList.push_back(data);
  }

  file.close();

  return dataList;
}

void deleteOldLogs(std::string filename, int deleteUntill) {
  std::lock_guard<std::mutex> lock(deleteMutex);
  std::vector<TemperatureData> actualData = getDataFromLogFile(filename, deleteUntill);

  for (auto& data : actualData) {
    std::cout << data.temperature << " " << data.timestemp << std::endl;
  }

  std::ofstream file(filename, std::ios::trunc);
  if (!file.is_open()) {
    std::cout << "Failed to open file " << filename << " for re-write";
    return;
  }

  for (auto& data : actualData) {

    file << data.temperature << " " << data.timestemp << std::endl;
  }

  file.close();

}

void writeDataFromUsbToLog(TemperatureData data) {
  std::lock_guard<std::mutex> lock(usbPortMutex);

  Logger::get(LOG_FILE) << data.temperature << " " << data.timestemp << std::endl;
  deleteOldLogs(LOG_FILE, SECONDS_IN_HOUR);

}

#ifdef _WIN32

bool setNonBlocking(HANDLE& hComm) {
    COMMTIMEOUTS timeouts;
    if (!GetCommTimeouts(hComm, &timeouts)) {
        std::cerr << "Error: Failed to get port timeouts." << std::endl;
        return false;
    }

    timeouts.ReadIntervalTimeout = 50;  // Время ожидания, если нет данных (в миллисекундах)
    timeouts.ReadTotalTimeoutConstant = 0;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 0;
    timeouts.WriteTotalTimeoutMultiplier = 0;

    if (!SetCommTimeouts(hComm, &timeouts)) {
        std::cerr << "Error: Failed to set port timeouts." << std::endl;
        return false;
    }

    return true;
}

void getTemperatureDataFromUsbPort(const std::string& port) {
    std::cout << "Opening port " << port << std::endl;

    HANDLE hComm = CreateFile(port.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 
                              NULL, OPEN_EXISTING, 0, NULL);

    if (hComm == INVALID_HANDLE_VALUE) {
        DWORD dwError = GetLastError();
        std::cerr << "Error: Unable to open port " << port << ". Error code: " << dwError << std::endl;
        return;
    }

    DWORD dwError;
    if (!ClearCommError(hComm, &dwError, NULL)) {
        std::cerr << "Error: Unable to clear errors for port " << port << ". Error code: " << dwError << std::endl;
        CloseHandle(hComm);
        return;
    }

    if (!setNonBlocking(hComm)) {
        std::cerr << "Error: Failed to set non-blocking mode." << std::endl;
        CloseHandle(hComm);
        return;
    }

    char byte;
    std::string buffer;

    while (true) {
        DWORD bytesRead;
        BOOL result = ReadFile(hComm, &byte, 1, &bytesRead, NULL);

        if (!result) {
            DWORD dwError = GetLastError();
            if (dwError == ERROR_IO_PENDING) {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                continue;
            } else {
                std::cerr << "Error reading from port. Error code: " << dwError << std::endl;
                break;
            }
        }

        buffer += byte;

        if (byte == '\n') {
            std::cout << "Received: " << buffer << std::endl;

            std::istringstream iss(buffer);
            TemperatureData data;

            if (iss >> data.temperature) {
                data.timestemp = static_cast<unsigned int>(std::time(nullptr));
                writeDataFromUsbToLog(data);
            } else {
                std::cout << "Error: Incorrect data format: " << buffer << std::endl;
            }

            buffer.clear();
        }
    }

    CloseHandle(hComm);
}


#else
void setNonBlocking(int fd) {
    fcntl(fd, F_SETFL, O_NONBLOCK); 
}

void getTemperatureDataFromUsbPort(std::string& port) {

    int fd = open(port.c_str(), O_RDONLY | O_NONBLOCK);
    if (fd == -1) {
        std::cout << "Failed to open port for read data" << std::endl;
        return;
    }

    setNonBlocking(fd); 

    char byte;
    std::string buffer; 

    while (true) {

        ssize_t bytesRead = read(fd, &byte, 1);

        if (bytesRead == -1) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        buffer += byte;

        if (byte == '\n') {
            std::cout << "Received: " << buffer << std::endl;

            std::istringstream iss(buffer);
            TemperatureData data;

            if (iss >> data.temperature) {
                data.timestemp = static_cast<unsigned int>(std::time(nullptr));
                writeDataFromUsbToLog(data);
            } else {
                std::cout << "Error: Incorrect data format: " << buffer << std::endl;
            }

            buffer.clear();
        }
    }

    close(fd);
}
#endif

void writeAveragePerTime(std::string filename, int lastSeconds) {
  std::vector<TemperatureData> dataList = getDataFromLogFile(LOG_FILE, lastSeconds);
  
  unsigned int currentTime = static_cast<unsigned int>(std::time(nullptr));
  double average = 0;
  unsigned int summ = 0;

  if (dataList.size() == 0) {
    Logger::get(filename) << average << " " << currentTime;
    return;
  }

  for (auto& data : dataList) {
    summ += data.timestemp;
  }

  average = static_cast<double>(summ) / dataList.size();

  Logger::get(filename) << average << " " << currentTime;
  
  if (filename == AVERAGE_PER_HOUR) {
    deleteOldLogs(filename, SECONDS_IN_MONTH);
  } else if (filename == AVERAGE_PER_DAY) {
    deleteOldLogs(filename, SECONDS_IN_YEAR);
  }

}

int main(int argc, char* argv[]) {
  std::cout << "Run programm" << std::endl;

  if (argc <= 1) {
    std::cout << "Required argument USB_PORT must be provided" << std::endl;
    return 0;
  }

  USB_PORT = argv[1];

  std::thread readDataThread(getTemperatureDataFromUsbPort, std::ref(USB_PORT));

  std::thread calcPerHourThread([]() {
    while (true) {
      std::this_thread::sleep_for(std::chrono::seconds(SECONDS_IN_HOUR));
      writeAveragePerTime(AVERAGE_PER_HOUR, SECONDS_IN_HOUR);
    }
  });

  std::thread calcPerDayThread([]() {
    while (true) {
      std::this_thread::sleep_for(std::chrono::seconds(SECONDS_IN_DAY));
      writeAveragePerTime(AVERAGE_PER_DAY, SECONDS_IN_DAY);
    }
  });

  calcPerHourThread.detach();
  calcPerDayThread.detach();
  readDataThread.join();

  return 0;
}
