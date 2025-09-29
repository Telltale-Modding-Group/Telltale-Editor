@echo off

cd ../..

cmake -S . -B build
cmake --build build --config Release

pause
