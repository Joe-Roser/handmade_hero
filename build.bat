
if not exist .\build mkdir .\build
pushd .\build

zig c++ -g ../source/main.cpp -l gdi32 -o main.exe

popd
