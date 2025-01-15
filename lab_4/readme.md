# 

## Инструкция

Для симуляции работы usb-девайса используется socat для unix-систем и com0com для Windows

### Unix

#### установить socat

Пр. Arch Linux

```
sudo pacman -S socat
```
#### Запустить socat с помощью

```
socat -d -d pty,raw,echo=0 pty,raw,echo=0
```

Путь к портам передать в соответвующие приложения

Пр. вывода
```
2025/01/15 20:45:53 socat[4272] N PTY is /dev/pts/1 - симулятор
2025/01/15 20:45:53 socat[4272] N PTY is /dev/pts/2 - получатель
```

#### Скомпилировать и запустить оба файла

```
g++ -std=c++17 -o app main.cpp
g++ -std=c++17 -o simulator simulator.cpp
```

Терминальная сессия 1
```
./app /dev/pts/2
```

Терминальная сессия 2
```
./simulator /dev/pts/1
```

### Windows

#### Установить com0com

#### Создать пару виртуальных COM-портов
"Пуск" -> "Setup Command Promt"

В открывшейся консоли выполнить
```
install PortName=COM3 PortName=COM4
```
COM3 - для симулятора

COM4 - для получателя

#### Скомпилировать и запустить, передав соответсвующие порты

Симулятор для Windows - port_simulator_win.cpp
