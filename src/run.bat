@echo off
@REM the -pedantic-errors flag is here to ensure consistency regardless of C++ compiler
g++ main.cpp -std=c++17 -pedantic-errors -o GreggyOS.exe && GreggyOS.exe