#!/bin/bash

git pull origin master
g++ main.cpp -o background-process-lib
./background-process-lib