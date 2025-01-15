#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <chrono>
#include <windows.h>

bool setCommParams(HANDLE hComm) {
    DCB dcbSerialParams = {0};
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

    if (!GetCommState(hComm, &dcbSerialParams)) {
        std::cerr << "Error: Could not get COM port state." << std::endl;
        return false;
    }

    // Настройка параметров порта (например, 9600, N, 8, 1)
    dcbSerialParams.BaudRate = CBR_9600; // Бодовая скорость
    dcbSerialParams.ByteSize = 8;         // Размер байта
    dcbSerialParams.StopBits = ONESTOPBIT; // Количество стоп-битов
    dcbSerialParams.Parity = NOPARITY;     // Без проверки четности

    if (!SetCommState(hComm, &dcbSerialParams)) {
        std::cerr << "Error: Could not set COM port state." << std::endl;
        return false;
    }

    return true;
}

void simulate_temperature_device(const std::string& port) {
    // Открытие COM порта
    HANDLE hComm = CreateFile(port.c_str(), GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    
    if (hComm == INVALID_HANDLE_VALUE) {
        std::cerr << "Ошибка: Не удалось открыть порт для записи: " << port << std::endl;
        return;
    }

    // Настройка параметров порта
    if (!setCommParams(hComm)) {
        CloseHandle(hComm);
        return;
    }

    int temperature = 20;
    char buffer[256];

    while (true) {
        // Генерация данных
        temperature += (std::rand() % 5 - 2);
        
        // Форматирование данных в строку
        std::snprintf(buffer, sizeof(buffer), "%d\n", temperature);

        // Отправка данных в порт
        DWORD bytesWritten;
        BOOL result = WriteFile(hComm, buffer, std::strlen(buffer), &bytesWritten, NULL);
        
        if (!result) {
            std::cerr << "Ошибка: Не удалось отправить данные на порт." << std::endl;
            break;
        }

        std::cout << "Sended: " << temperature << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(3));
    }

    // Закрытие порта
    CloseHandle(hComm);
}

int main() {
    const std::string device = "\\\\.\\COM3";  
    simulate_temperature_device(device);
    return 0;
}

