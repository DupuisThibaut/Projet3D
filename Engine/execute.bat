@echo off
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
cd ..
Engine.exe ..\Jeu\ -r