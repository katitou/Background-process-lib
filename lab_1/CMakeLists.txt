cmake_minimum_required(VERSION 3.10)
project(HelloWorld)

set(CMAKE_CXX_STANDARD 17)

if (WIN32)
    set(CMAKE_CXX_FLAGS_DEBUG "/Zi /EHsc") # CMAKE_CXX_FLAGS_DEBUG хранит отладочные флаги 
    set(CMAKE_CXX_FLAGS "/Wall /EHsc") # CMAKE_CXX_FLAGS хранит флаг предупреждений о проблемах в коде -Wall
else()
    set(CMAKE_CXX_FLAGS "-Wall")
    set(CMAKE_CXX_FLAGS_DEBUG "-g")  # для отладочной информации на Unix-подобных системах
endif()

set(CMAKE_BUILD_TYPE Debug) # конфигурация сборки (Debug/Release)

add_executable(HelloWorld main.cpp)