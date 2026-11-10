@echo off
setlocal
cd /d "%~dp0"

set "GXX=C:\WinLibs\mingw64\bin\g++.exe"
set "BUILD_DIR=Builds"
set "TEMP_EXE=%BUILD_DIR%\NekoStack_build.tmp.exe"
set "BACKUP_04=%BUILD_DIR%\NekoStack_0.4_Launcher.exe"
set "BACKUP_051=%BUILD_DIR%\NekoStack_0.5.1_Settings.exe"

if not exist "%GXX%" (
    echo ERROR: No se encontro MinGW en:
    echo %GXX%
    echo Ajusta la ruta GXX dentro de Build_NekoStack.bat si es necesario.
    pause
    exit /b 1
)

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

if exist "NekoStack.exe" if not exist "%BACKUP_04%" (
    copy /y "NekoStack.exe" "%BACKUP_04%" >nul
    if errorlevel 1 echo AVISO: No se pudo crear la copia de seguridad 0.4.
)
if exist "NekoStack.exe" if not exist "%BACKUP_051%" (
    copy /y "NekoStack.exe" "%BACKUP_051%" >nul
    if errorlevel 1 echo AVISO: No se pudo crear la copia de seguridad 0.5.1.
)

echo Compilando NekoStack 0.5.2 con selector de ejecutables...
"%GXX%" -std=c++17 -O2 -Wall -Wextra -static -static-libgcc -static-libstdc++ -mwindows -municode -I. NekoStack.cpp Core\NekoCore.cpp Modules\NekoDesktop.cpp Modules\NekoLauncher.cpp Modules\NekoSettings.cpp -o "%TEMP_EXE%" -luser32 -lgdi32 -lshell32 -lcomdlg32
if errorlevel 1 (
    echo.
    echo ERROR: La compilacion no termino correctamente.
    if exist "%TEMP_EXE%" del "%TEMP_EXE%"
    pause
    exit /b 1
)

copy /y "%TEMP_EXE%" "NekoStack.exe" >nul
if errorlevel 1 (
    echo.
    echo ERROR: No se pudo actualizar NekoStack.exe.
    echo Cierra NekoStack si sigue abierto e intenta de nuevo.
    pause
    exit /b 1
)

del "%TEMP_EXE%" >nul 2>nul
echo.
echo Compilacion correcta: NekoStack.exe
echo Respaldo 0.2 anterior: se conserva si ya existe
echo Respaldo 0.4: Builds\NekoStack_0.4_Launcher.exe
echo Respaldo 0.5.1: Builds\NekoStack_0.5.1_Settings.exe
echo Comprueba Modules.ini: Launcher=1 y NekoSettings=1.
pause
endlocal
