#!/bin/bash

git pull origin master

if [ ! -d "build" ]; then
    mkdir build
fi

g++ -std=c++17 -o build/app main.cpp

echo Build success.