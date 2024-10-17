git pull origin master

if not exist build (
    mkdir build
)

cd build
cmake ..
cmake --build .
cd Debug
HelloWorld.exe