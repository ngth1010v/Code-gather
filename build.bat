@echo off
setlocal

echo ================================
echo   CPP - BUILD
echo ================================

cd /d %~dp0

REM BUILD FOLDER
if not exist build (
    mkdir build
)

REM BIN FOLDER (Ensure it exists)
if not exist bin (
    mkdir bin
)

cd build

echo Configuring CMake (Release)...
cmake .. -DCMAKE_BUILD_TYPE=Release

if %errorlevel% neq 0 (
    echo.
    echo [ERROR] CMake configure failed!
    pause
    exit /b %errorlevel%
)

echo.
echo Building project...
cmake --build .

if %errorlevel% neq 0 (
    echo.
    echo [ERROR] Build failed!
    pause
    exit /b %errorlevel%
)

echo.
echo Copying .exe to bin folder...
copy /y "*.exe" "..\bin\"
copy /y "*.a" "..\bin\"

echo.
echo ================================
echo Build and Copy SUCCESS
echo ================================

echo.
endlocal