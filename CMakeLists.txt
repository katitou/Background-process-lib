cmake_minimum_required(VERSION 3.10)
project(HelloWorld)
set(CMAKE_CXX_STANDARD 17) 
set(CMAKE_CXX_FLAGS_DEBUG "/Zi /EHsc") # переменная CMAKE_CXX_FLAGS_DEBUG хранит отладочные флаги (для win)
set(CMAKE_CXX_FLAGS "/Wall /EHsc") # переменная CMAKE_CXX_FLAGS хранит флаг предупреждений о проблемах в коде -Wall
set(CMAKE_BUILD_TYPE Debug) # конфигурация сборки (Debug/Release)
add_executable(HelloWorld main.cpp) 