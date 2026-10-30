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

:: -- Build both targets --
cmake --build %BUILD_DIR% --config Release -j4

if errorlevel 1 (
    echo [!] Build failed
    pause
    exit /b 1
)

:: -- Package ffkernel (needs driver) --
set BINARY_K=%BUILD_DIR%\ffkernel
python3 make_sh.py %BINARY_K% ffkernel.sh
if errorlevel 1 ( copy /Y "%BINARY_K%" ffkernel.sh )

:: -- Package ffext (no driver check) --
set BINARY_E=%BUILD_DIR%\ffext
python3 make_sh.py --ext %BINARY_E% ffext.sh
if errorlevel 1 ( copy /Y "%BINARY_E%" ffext.sh )

echo.
echo ============================================================
echo [+] ffkernel.sh  — RTkernel driver  (needs 5.10.sh loaded)
echo [+] ffext.sh     — external proc_mem (root only, no driver)
echo.
echo Push kernel build:
echo   adb push ffkernel.sh /data/local/tmp/
echo   adb push key.txt      /data/local/tmp/
echo   adb shell chmod 755 /data/local/tmp/ffkernel.sh
echo   adb shell /data/local/tmp/ffkernel.sh
echo.
echo Push external build:
echo   adb push ffext.sh /data/local/tmp/
echo   adb shell chmod 755 /data/local/tmp/ffext.sh
echo   adb shell /data/local/tmp/ffext.sh
echo ============================================================
pause
