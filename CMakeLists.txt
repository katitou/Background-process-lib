cmake_minimum_required(VERSION 3.10)
project(HelloWorld)
set(CMAKE_CXX_STANDARD 17) 
add_executable(HelloWorld main.cpp) 
set(CMAKE_CXX_FLAGS "-Wall") # переменная CMAKE_CXX_FLAGS хранит флаг предупреждений о проблемах в коде -Wall
set(CMAKE_BUILD_TYPE Debug) # конфигурация сборки (Debug/Release)
set(CMAKE_CXX_FLAGS_DEBUG "-g")
