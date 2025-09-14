
if not exist .\build mkdir .\build
pushd .\build

zig c++ -g ../source/windows_main.cpp -l gdi32 -o main.exe

popd
