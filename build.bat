
if not exist .\build mkdir .\build
pushd .\build

zig c++ -g ../source/main.cpp

popd
