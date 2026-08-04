@echo off
setlocal
rem Build script for MineSweeper (Qt 5.15.2 MSVC from Anaconda + VS 18 Community)
set QTDIR=D:\Develop\anaconda3\Library
set VCVARSALL=D:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat

if not exist "%VCVARSALL%" (
    echo [ERROR] vcvarsall.bat not found: %VCVARSALL%
    exit /b 1
)
if not exist "%QTDIR%\bin\qmake.exe" (
    echo [ERROR] qmake not found: %QTDIR%\bin\qmake.exe
    exit /b 1
)

call "%VCVARSALL%" amd64
if errorlevel 1 exit /b 1

set PATH=%QTDIR%\bin;%PATH%

qmake MineSweeper.pro
if errorlevel 1 exit /b 1

nmake
if errorlevel 1 exit /b 1

rem Deploy Qt runtime DLLs next to the exe when windeployqt is available
if exist "%QTDIR%\bin\windeployqt.exe" (
    "%QTDIR%\bin\windeployqt.exe" --release --no-translations bin\MineSweeper.exe
    if errorlevel 1 echo [WARN] windeployqt finished with warnings.
)

rem Copy conda-specific runtime DLLs that windeployqt does not know about
for %%F in (zlib.dll zstd.dll libpng16.dll libcrypto-3-x64.dll libssl-3-x64.dll msvcp140.dll msvcp140_1.dll vcruntime140.dll vcruntime140_1.dll) do (
    if exist "%QTDIR%\bin\%%F" (
        if not exist "bin\%%F" copy /y "%QTDIR%\bin\%%F" "bin\%%F" >nul
    )
)

echo.
echo Build OK: %CD%\bin\MineSweeper.exe
endlocal
