@echo off
set file_name=%1

@REM extract file_name without extension
set file_base=%~n1

if "%file_name%"=="" (
    echo "Please provide a file name to run tests on."
    exit /b 1
)
g++ -std=c++11 %file_name% -o %file_base%_test.exe

if %errorlevel% neq 0 (
    echo "Compilation failed."
    exit /b %errorlevel%
)

%file_base%_test.exe
del %file_base%_test.exe