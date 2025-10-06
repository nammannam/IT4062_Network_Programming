@echo off
REM Build script for lab_w1.c program (Windows batch alternative to Makefile)

if "%1"=="clean" goto clean
if "%1"=="run" goto run
if "%1"=="debug" goto debug
if "%1"=="release" goto release
if "%1"=="help" goto help
if "%1"=="" goto build

:build
echo Building lab_w1.exe...
gcc -Wall -Wextra -std=c99 -g -c lab_w1.c
if errorlevel 1 goto error
gcc -Wall -Wextra -std=c99 -g -o lab_w1.exe lab_w1.o
if errorlevel 1 goto error
echo Build successful!
goto end

:clean
echo Cleaning compiled files...
if exist lab_w1.exe del lab_w1.exe
if exist lab_w1.o del lab_w1.o
echo Clean completed!
goto end

:run
call :build
if errorlevel 1 goto error
echo Running program...
lab_w1.exe
goto end

:debug
echo Building debug version...
gcc -Wall -Wextra -std=c99 -g -DDEBUG -O0 -c lab_w1.c
if errorlevel 1 goto error
gcc -Wall -Wextra -std=c99 -g -DDEBUG -O0 -o lab_w1.exe lab_w1.o
if errorlevel 1 goto error
echo Debug build successful!
goto end

:release
echo Building release version...
gcc -Wall -Wextra -std=c99 -O2 -DNDEBUG -c lab_w1.c
if errorlevel 1 goto error
gcc -Wall -Wextra -std=c99 -O2 -DNDEBUG -o lab_w1.exe lab_w1.o
if errorlevel 1 goto error
echo Release build successful!
goto end

:help
echo Available commands:
echo   build.bat        - Build the program (default)
echo   build.bat clean  - Remove compiled files
echo   build.bat run    - Build and run the program
echo   build.bat debug  - Build with debug flags
echo   build.bat release- Build optimized release version
echo   build.bat help   - Show this help message
goto end

:error
echo Build failed!
exit /b 1

:end