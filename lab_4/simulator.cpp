#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <chrono>

void simulate_temperature_device(const std::string& port) {
    std::ofstream output(port);
    if (!output.is_open()) {
        std::cerr << "Ошибка: Не удалось открыть порт для записи: " << port << std::endl;
        return;
    }

    int temperature = 20;
    while (true) {
        // Генерация данных
        temperature += (std::rand() % 5 - 2);

        // Отправка данных в порт
        output << std::to_string(temperature) + "\n";
        output.flush(); // Убедимся, что данные записаны

        std::cout << "Sended: " << temperature << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(3));
    }
}

int main(int argc, char* argv[]) {

    if (argc <= 1) {
        std::cout << "Required argument USB_PORT must be provided" << std::endl;
        return 0;
    }

    const std::string device = argv[1];
    simulate_temperature_device(device);
    return 0;
}

