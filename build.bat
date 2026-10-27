@echo off
:: ============================================================
:: Build script for ffkernel — Windows + Android NDK
:: Requirements:
::   - Android NDK r25+ installed
::   - CMake 3.22+ in PATH
::   - ANDROID_NDK_ROOT env set  (e.g. C:\Users\you\AppData\Local\Android\Sdk\ndk\25.2.9519653)
:: ============================================================

if "%ANDROID_NDK_ROOT%"=="" (
    echo ERROR: ANDROID_NDK_ROOT not set
    echo Set it to your NDK path, e.g.:
    echo   set ANDROID_NDK_ROOT=C:\Users\you\AppData\Local\Android\Sdk\ndk\25.2.9519653
    pause
    exit /b 1
)

set BUILD_DIR=build_android
set OUTPUT=ffkernel.sh

:: -- Download ImGui if not present --
if not exist "src\imgui\imgui.cpp" (
    echo [*] Downloading ImGui 1.90.9...
    powershell -NoProfile -ExecutionPolicy Bypass -Command ^
        "Invoke-WebRequest -Uri 'https://github.com/ocornut/imgui/archive/refs/tags/v1.90.9.zip' -OutFile 'imgui.zip'"
    powershell -NoProfile -ExecutionPolicy Bypass -Command ^
        "Expand-Archive -Path 'imgui.zip' -DestinationPath '.' -Force"
    xcopy /E /Y "imgui-1.90.9\*.h"   "src\imgui\" >nul
    xcopy /E /Y "imgui-1.90.9\*.cpp" "src\imgui\" >nul
    xcopy /E /Y "imgui-1.90.9\backends\imgui_impl_android.*"  "src\imgui\" >nul
    xcopy /E /Y "imgui-1.90.9\backends\imgui_impl_opengl3.*"  "src\imgui\" >nul
    del imgui.zip
    rmdir /S /Q imgui-1.90.9
    echo [+] ImGui ready
)

:: -- Configure --
cmake -B %BUILD_DIR% ^
    -DCMAKE_TOOLCHAIN_FILE="%ANDROID_NDK_ROOT%\build\cmake\android.toolchain.cmake" ^
    -DANDROID_ABI=arm64-v8a ^
    -DANDROID_PLATFORM=android-29 ^
    -DCMAKE_BUILD_TYPE=Release ^
    -G "Ninja"

if errorlevel 1 (
    echo [!] CMake configure failed
    pause
    exit /b 1
)

:: -- Build --
cmake --build %BUILD_DIR% --config Release -j4

if errorlevel 1 (
    echo [!] Build failed
    pause
    exit /b 1
)

:: -- Package as shell-wrapper .sh --
:: Shell script header that extracts + runs the embedded ELF
set BINARY=%BUILD_DIR%\ffkernel
if not exist "%BINARY%" set BINARY=%BUILD_DIR%\ffkernel.exe

python3 make_sh.py %BINARY% %OUTPUT%
if errorlevel 1 (
    :: fallback: just copy binary with .sh extension
    copy /Y "%BINARY%" "%OUTPUT%"
)

echo.
echo ============================================================
echo [+] Done: %OUTPUT%
echo Push to phone:
echo   adb push %OUTPUT% /data/local/tmp/
echo   adb push key.txt   /data/local/tmp/
echo   adb shell chmod 755 /data/local/tmp/%OUTPUT%
echo   adb shell /data/local/tmp/%OUTPUT%
echo ============================================================
pause
