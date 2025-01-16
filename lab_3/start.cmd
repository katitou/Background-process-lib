git pull origin master

if not exist build (
    mkdir build
)

g++ -std=c++17 -o build/app main.cpp

echo Build success.