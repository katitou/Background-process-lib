#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <fstream>
#include <string>
#include <mutex>


class Logger {
private:
    static std::ofstream logFile;
    static std::mutex logMutex;

    Logger() = default;

public:
    static void init() {
        std::lock_guard<std::mutex> lock(logMutex);
        if (!logFile.is_open()) {
            logFile.open("log.txt", std::ios::app);
            if (!logFile.is_open()) {
                throw std::runtime_error("Не удалось открыть файл для логирования: log.txt");
            }
        }
    }

    static void close() {
        std::lock_guard<std::mutex> lock(logMutex);
        if (logFile.is_open()) {
            logFile.close();
        }
    }

    static Logger& get() {
        if (!logFile.is_open()) {
          Logger::init();
        }
        static Logger instance;
        return instance;
    }

    template <typename T>
    Logger& operator<<(const T& data) {
        std::lock_guard<std::mutex> lock(logMutex);
        if (logFile.is_open()) {
            logFile << data;
        } else {
            throw std::runtime_error("Файл для логирования не инициализирован.");
        }
        return *this;
    }

    Logger& operator<<(std::ostream& (*manip)(std::ostream&)) {
      std::lock_guard<std::mutex> lock(logMutex);
      if (logFile.is_open()) {
        manip(logFile); 
      }

      return *this;
    }
};

std::ofstream Logger::logFile;
std::mutex Logger::logMutex;

#endif // LOGGER_H

