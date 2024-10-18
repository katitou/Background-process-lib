#!/bin/bash

git pull origin master

if [ ! -d "build" ]; then
    mkdir build
fi

cd build
cmake ..
make

./HelloWorld