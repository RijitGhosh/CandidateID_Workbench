@echo off
REM Builds the Windows package on a Windows 11 machine that has: Visual Studio 2022 Build Tools (or MinGW) + CMake + Qt 6 (e.g. 6.5 LTS, msvc2019_64).
REM Usage (from a Developer Command Prompt):  scripts\package_windows.bat C:\Qt\6.5.3\msvc2019_64
REM Result: dist\AetherWorkbench\Launch.exe  (double-click it).  This script runs the C++ tests first and stops if any fail.
setlocal
if "%~1"=="" ( echo usage: package_windows.bat ^<Qt prefix, e.g. C:\Qt\6.5.3\msvc2019_64^> & exit /b 2 )
set QTPREFIX=%~1
cmake -S . -B build-win -DCMAKE_PREFIX_PATH=%QTPREFIX% -DCMAKE_BUILD_TYPE=Release || exit /b 1
cmake --build build-win --config Release || exit /b 1
ctest --test-dir build-win -C Release --output-on-failure || ( echo TESTS FAILED - not packaging & exit /b 1 )
set OUT=dist\AetherWorkbench
rmdir /s /q dist 2>nul
mkdir %OUT%\bin
copy /y build-win\Release\Launch.exe %OUT%\ >nul
copy /y build-win\src\frontend\Release\AetherWorkbench.exe %OUT%\bin\ >nul
"%QTPREFIX%\bin\windeployqt.exe" --release --no-translations --no-opengl-sw --no-system-d3d-compiler %OUT%\bin\AetherWorkbench.exe || exit /b 1
xcopy /e /i /y data %OUT%\data >nul
xcopy /e /i /y docs %OUT%\docs >nul
xcopy /e /i /y results %OUT%\results >nul
copy /y README.md %OUT%\ >nul
copy /y LICENSES.md %OUT%\ >nul
echo Done: %OUT%\Launch.exe
